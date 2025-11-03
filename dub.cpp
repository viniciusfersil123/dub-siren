#include "daisy_seed.h"
#include "daisysp.h"
#include "dub.h"

/* ADCs and pin numbers (in Daisy Seed)

KNOB   | PIN NUMBER
-------------------
Volume | 22
Decay  | 23
Depth  | 24
Tune   | 25
Sweep  | 26
Rate   | 27

BUTTON      | PIN NUMBER
------------------------
Trigger 1   | 28
Trigger 2   | 29
Trigger 3   | 30
Trigger 4   | 31
BankSelect  | 32
SweepToTune | 33
*/
using namespace daisy;
using namespace daisysp;

// Daisy setup components
KnobHandlerDaisy*   knob_handler   = new KnobHandlerDaisy();
ButtonHandlerDaisy* button_handler = new ButtonHandlerDaisy();
int                 SAMPLE_RATE = 0, BLOCK_SIZE = 0;
float               output, adsr_vcf, adsr_output, vco_output, vco_modulation;
volatile bool       shouldApplyToggles = false;
std::pair<float, float> lfo_output     = std::make_pair(0, 0);

// Dub Siren components
DecayEnvelope* envelope;
Sweep*         sweep;
Triggers*      triggers;
Lfo*           lfo;
Vco*           vco;
Vcf*           vcf;
OutAmp*        out_amp;
GPIO           led_sweep;
GPIO           led_bank;
Led            led_lfo;


//Initialize led1. We'll plug it into pin 28.
//false here indicates the value is uninverted

bool DEBUG = false;


// KnobHandler functions
void KnobHandlerDaisy::InitAll()
{
    AdcChannelConfig my_adc_config[NUM_ADC_CHANNELS];
    my_adc_config[VolumeKnob].InitSingle(daisy::seed::A0);
    my_adc_config[DecayKnob].InitSingle(daisy::seed::A1);
    my_adc_config[DepthKnob].InitSingle(daisy::seed::A2);
    my_adc_config[TuneKnob].InitSingle(daisy::seed::A3);
    my_adc_config[SweepKnob].InitSingle(daisy::seed::A4);
    my_adc_config[RateKnob].InitSingle(daisy::seed::A5);
    hw.adc.Init(my_adc_config, NUM_ADC_CHANNELS);
    hw.adc.Start();
}


void KnobHandlerDaisy::UpdateAll()
{
    // VCO tune knobs
    vco->TuneValue = fclamp(hw.adc.GetFloat(TuneKnob), 0.f, 1.f);

    // Decay Envelope knobs
    envelope->ReleaseValue = fclamp(hw.adc.GetFloat(DecayKnob), 0.f, 1.f);

    // Only update sweep value while a trigger is pressed
    if(triggers->Pressed())
    {
        sweep->ReleaseValue
            = fmap(hw.adc.GetFloat(SweepKnob), 0.f, 1.f, Mapping::LINEAR);
    }

    // LFO depth (vibrato intensity 0-100%) and rate knobs
    lfo->DepthValue = fclamp(hw.adc.GetFloat(DepthKnob), 0.f, 1.f);
    lfo->RateValue  = fmap(
        hw.adc.GetFloat(RateKnob), LFO_MIN_FREQ, LFO_MAX_FREQ, Mapping::EXP);

    // OutAmp volume knob
    out_amp->VolumeValue
        = fmap(hw.adc.GetFloat(VolumeKnob), 0.f, 1.f, Mapping::EXP);
}
// KnobHandler functions


// ButtonHandler functions
void ButtonHandlerDaisy::InitAll()
{
    this->triggers[0].Init(daisy::seed::D21, 50);
    this->triggers[1].Init(daisy::seed::D22, 50);
    this->triggers[2].Init(daisy::seed::D23, 50);
    this->triggers[3].Init(daisy::seed::D24, 50);
    this->bankSelect.Init(daisy::seed::D25, 50);
    this->sweepToTune.Init(daisy::seed::D26, 50);
}

void ButtonHandlerDaisy::DebounceAll()
{
    for(int i = 0; i < 4; i++)
    {
        this->triggers[i].Debounce();
    }
    this->bankSelect.Debounce();
    this->sweepToTune.Debounce();
}

void ButtonHandlerDaisy::UpdateAll()
{
    static std::vector<int> press_stack; // pilha de botões pressionados

    for(int i = 0; i < 4; i++)
    {
        // Atualiza estados
        if(this->triggers[i].RisingEdge())
        {
            this->triggersStates[i][0] = true;
            this->triggersStates[i][1] = true;

            // Remove se já estiver na pilha e adiciona no topo
            press_stack.erase(
                std::remove(press_stack.begin(), press_stack.end(), i),
                press_stack.end());
            press_stack.push_back(i);
        }
        else if(this->triggers[i].FallingEdge())
        {
            this->triggersStates[i][2] = true;
            this->triggersStates[i][1] = false;

            // Remove da pilha
            press_stack.erase(
                std::remove(press_stack.begin(), press_stack.end(), i),
                press_stack.end());
        }
        else
        {
            this->triggersStates[i][1] = this->triggers[i].Pressed();
            this->triggersStates[i][0] = false;
            this->triggersStates[i][2] = false;
        }
    }

    // Atualiza LastIndex
    if(!press_stack.empty())
    {
        this->LastIndex = press_stack.back();
    }

    // Update toggle buttons
    if(this->bankSelect.RisingEdge())
    {
        this->bankSelectState = !this->bankSelectState;
    }

    if(this->sweepToTune.RisingEdge())
    {
        this->sweepToTuneState = !this->sweepToTuneState; // só o pendente
    }
}

// ButtonHandler functions


// Init functions
void InitComponents(int sample_rate, int block_size)
{
    triggers = new Triggers();
    envelope = new DecayEnvelope(sample_rate, block_size);
    sweep    = new Sweep(sample_rate, block_size);
    lfo      = new Lfo(sample_rate);
    vco      = new Vco(sample_rate);
    vcf      = new Vcf(sample_rate);
    out_amp  = new OutAmp();
}
// Init functions


// DecayEnvelope functions
void DecayEnvelope::SetReleaseTime(float time)
{
    this->envelope.SetTime(ADSR_SEG_RELEASE, time);
}

float DecayEnvelope::Process(bool gate)
{
    this->EnvelopeValue = this->envelope.Process(gate);
    return this->EnvelopeValue;
}


// Triggers functions
bool Triggers::Triggered()
{
    for(int i = 0; i < 4; i++)
    {
        if(button_handler->triggersStates[i][0])
            return true;
    }
    return false;
}

bool Triggers::Pressed()
{
    for(int i = 0; i < 4; i++)
    {
        if(button_handler->triggersStates[i][1])
            return true;
    }
    return false;
}

bool Triggers::Released()
{
    for(int i = 0; i < 4; i++)
    {
        if(button_handler->triggersStates[i][2])
            return true;
    }
    return false;
}

void Triggers::ClearTriggered()
{
    for(int i = 0; i < 4; i++)
    {
        if(button_handler->triggersStates[i][0])
        {
            button_handler->triggersStates[i][0] = false;
            break; // Only clear one trigger per call
        }
    }
}

// bool Triggers::IsBankSelectActive()
// {
//     return button_handler->bankSelectState;
// }
// Triggers functions


// Sweep functions
/* bool Sweep::IsSweepToTuneActive()
{
    return button_handler->sweepToTuneState;
} */

void Sweep::SetReleaseTime(float time)
{
    this->envelope.SetTime(ADSR_SEG_RELEASE, time);
}

float Sweep::Process(bool gate)
{
    this->EnvelopeValue = this->envelope.Process(gate);
    return this->EnvelopeValue;
}

float Sweep::CalculateFilterIntensity(float sweepValue)
{
    // Map sweepVal from [0,1] to [-1,1]
    float direction = 2.0f * (sweepValue - 0.5f);
    float abs_dir   = fabsf(direction);

    // Dead zone threshold
    float threshold = DEADZONE_HALF_WIDTH;
    float intensity = 0.0f;

    // Assymetrical curve
    // Faster sweep for values below threshold - attack effect
    // Slower sweep for values above threshold - decay effect
    if(direction > threshold)
    {
        intensity
            = 0.5f * powf((abs_dir - threshold) / (1.0f - threshold), 2.0f);
    }
    else if(direction < -threshold)
    {
        intensity
            = 2.f * powf((abs_dir - threshold) / (1.0f - threshold), 0.5f);
        // intensity = 3.f;
    }
    // If sweep is in the threshold interval, maintain 0.

    return intensity;
}

float Sweep::CalculateVcoIntensity(float sweepValue)
{
    // Map sweepVal from [0,1] to [-1,1]
    float direction = 2.0f * (sweepValue - 0.5f);
    float abs_dir   = fabsf(direction);

    float threshold = DEADZONE_HALF_WIDTH;
    float intensity = 0.f;

    // Calculate sweep intensity for VCO:
    if(direction > threshold)
    {
        intensity
            = 0.75f * powf((abs_dir - threshold) / (1.0f - threshold), 2.0f);
    }
    else if(direction < -threshold)
    {
        intensity
            = 0.75f * powf((abs_dir - threshold) / (1.0f - threshold), 2.0f);
    }
    // If sweep is in the threshold interval, maintain 0.

    return intensity;
}

float Sweep::UpdateCutoffFreq(float sweepValue, Vcf* vcf, float adsrOutput)
{
    float base_exp = vcf->CutoffExponent;

    // Map sweepVal from [0,1] to [-1,1]
    float direction = 2.0f * (sweepValue - 0.5f);

    // Get filter intensity using the dedicated method
    float intensity = this->CalculateFilterIntensity(sweepValue);

    // Compute target exponent:
    // direction < 0 → freq goes up   → end_exp = 1
    // direction > 0 → freq goes down → end_exp = 0
    float end_exp = 0.5f - 0.5f * direction;

    // Blend start and end exponents modulated by ADSR and sweep intensity
    float sweep_exp
        = base_exp + (end_exp - base_exp) * (1.0f - adsrOutput) * intensity;

    // Final exponential frequency
    return VCF_MIN_FREQ * powf(VCF_MAX_FREQ / VCF_MIN_FREQ, sweep_exp);
}

// --- Lfo functions ---
void Lfo::UpdateWaveforms(int index, bool bankB)
{
    if(bankB)
    {
        switch(index)
        {
            case 0:
                osc[0].SetWaveform(Oscillator::WAVE_SIN);
                osc_harm[0].SetWaveform(Oscillator::WAVE_SIN);
                break;
            case 1:
                osc[1].SetWaveform(Oscillator::WAVE_SQUARE);
                osc_harm[1].SetWaveform(Oscillator::WAVE_SQUARE);
                break;
            case 2:
                osc[2].SetWaveform(Oscillator::WAVE_SQUARE);
                osc_harm[2].SetWaveform(Oscillator::WAVE_SAW);
                break;
            case 3:
                osc[3].SetWaveform(Oscillator::WAVE_RAMP);
                osc_harm[3].SetWaveform(Oscillator::WAVE_RAMP);
                break;
        }
    }
    else
    {
        static constexpr int waves[4] = {Oscillator::WAVE_SIN,
                                         Oscillator::WAVE_SQUARE,
                                         Oscillator::WAVE_SAW,
                                         Oscillator::WAVE_RAMP};

        osc[index].SetWaveform(waves[index]);
    }
}

float Lfo::MixLfoSignals(int index, bool bankB)
{
    float base = osc[index].Process();
    float harm = osc_harm[index].Process();

    if(bankB)
    {
        switch(index)
        {
            case 0: return 0.5f * (base + harm);        // sin + sin(3x)
            case 1: return 0.5f * (base + harm);        // square + square(4x)
            case 2: return 0.5f * (base + harm);        // square + saw(4x)
            case 3: return 0.5f * (base + 0.5f * harm); // ramp + 0.5*ramp(2x)
        }
    }

    return base;
}


void Lfo::SetAmpAll(float amp)
{
    for(int i = 0; i < 4; i++)
    {
        this->osc[i].SetAmp(amp);
    }
}

void Lfo::SetFreqAll(float freq)
{
    static constexpr float harm_ratios[4] = {4.0f, 8.0f, 8.0f, 2.0f};

    for(int i = 0; i < 4; i++)
    {
        osc[i].SetFreq(freq);
        osc_harm[i].SetFreq(freq * harm_ratios[i]);
    }
}


void Lfo::ResetPhaseAll()
{
    for(int i = 0; i < 4; i++)
    {
        this->osc[i].Reset(0.0f);
        this->osc_harm[i].Reset(0.0f);
    }
}

std::pair<float, float> Lfo::ProcessAll()
{
    int index = button_handler->LastIndex;
    // Use o banco atualmente ativo
    bool bankB = button_handler->currentBankState;

    // Nova seleção → inicia crossfade
    if(index != currIndex)
    {
        prevIndex    = currIndex;
        currIndex    = index;
        fadeProgress = 0.0f;
    }

    fadeProgress = fminf(fadeProgress + fadeRate, 1.0f);

    float out = 0.0f;

    if(currIndex >= 0)
    {
        UpdateWaveforms(currIndex, bankB);
        out += MixLfoSignals(currIndex, bankB) * fadeProgress;
    }

    if(prevIndex >= 0 && fadeProgress < 1.0f)
    {
        UpdateWaveforms(prevIndex, bankB);
        out += MixLfoSignals(prevIndex, bankB) * (1.0f - fadeProgress);
    }

    // Convert LFO output to modulation signal [0,1]
    // No longer scaling by DepthValue here - FM ratio handles this now
    float modsig = 0.5f + out;

    return std::make_pair(out, modsig);
}

// --- Lfo functions ---


// Vco functions
void Vco::SetFreq(float freq)
{
    this->osc.SetFreq(freq);
}

float Vco::Process()
{
    return this->osc.Process();
}

float Vco::CalculateFMFreq(float carrier_freq, float lfo_bipolar, float depth)
{
    // FM synthesis: M = C / R (Modulator freq = Carrier freq / Ratio)
    float modulator_freq = carrier_freq / this->fm_ratio;

    // Calculate deviation: D = I * M, where I is controlled by depth
    float fm_index  = LFO_FM_INDEX * depth;
    float deviation = fm_index * modulator_freq;

    // Apply LFO modulation with FM deviation scaling
    float frequency_shift = lfo_bipolar * deviation;

    // Calculate modulated frequency
    float freq = carrier_freq + frequency_shift;

    // --- Frequency folding ---
    // Fold negative frequencies to positive (spectral mirroring at 0 Hz)
    freq = fabsf(freq);

    // Fold frequencies above Nyquist back down (spectral mirroring at Nyquist limit)
    if(freq > this->nyquist_limit)
    {
        freq = 2.0f * this->nyquist_limit - freq;
        freq = fabsf(freq); // Handle double-folding for extreme cases
    }

    return freq;
}
// Vco functions


// Vcf functions
void Vcf::SetFreq(float freq)
{
    // Svf filter accepts frequency up to SAMPLE_RATE / 3
    float limited_freq = fclamp(freq, VCF_MIN_FREQ, SAMPLE_RATE / 4);
    this->filter.SetFreq(limited_freq);
}

void Vcf::UpdateCutoffPressed(float sweepValue)
{
    // Map the sweep value to a piecewise linear interpolation
    // 0%  to 50%  -> 0%  to 75%
    // 50% to 100% -> 75% to 100%
    if(sweepValue <= 0.5f)
    {
        this->CutoffExponent = (sweepValue / 0.5f) * 0.75f;
    }
    else
    {
        this->CutoffExponent = 0.75f + ((sweepValue - 0.5f) / 0.5f) * 0.25f;
    }

    this->CutoffFreq
        = VCF_MIN_FREQ
          * powf(VCF_MAX_FREQ / VCF_MIN_FREQ, this->CutoffExponent);
    this->SetFreq(this->CutoffFreq);
}

float Vcf::Process(float in)
{
    this->filter.Process(in);
    return this->filter.Low(); // Return low-pass output
}
// Vcf functions


// OutAmp functions
void OutAmp::SetVolume(float volume)
{
    this->VolumeValue = volume;
}

float OutAmp::Process(float in)
{
    return in * this->VolumeValue;
}
// OutAmp functions


// Debug functions
void PrintKnobValues()
{
    // Raw input values
    hw.Print("Volume: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(VolumeKnob)));
    hw.Print("   Decay:  " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(DecayKnob)));
    hw.Print("   Depth:  " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(DepthKnob)));
    hw.PrintLine("");
    hw.Print("Tune:   " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(TuneKnob)));
    hw.Print("   Sweep:  " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(SweepKnob)));
    hw.Print("   Rate:   " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(RateKnob)));
    hw.PrintLine("");

    // Class attribute values
    // hw.Print("Volume: " FLT_FMT3, FLT_VAR3(out_amp->VolumeValue));
    // hw.Print("   Decay:  " FLT_FMT3, FLT_VAR3(envelope->ReleaseValue));
    // hw.Print("   Depth:  " FLT_FMT3, FLT_VAR3(lfo->DepthValue));
    // hw.PrintLine("");
    // hw.Print("Tune:   " FLT_FMT3, FLT_VAR3(vco->TuneValue));
    // hw.Print("   Sweep:  " FLT_FMT3, FLT_VAR3(sweep->SweepValue));
    // hw.Print("   Rate:   " FLT_FMT3, FLT_VAR3(lfo->RateValue));
    // hw.PrintLine("");
}

void PrintButtonStates()
{
    for(int i = 0; i < 4; i++)
    {
        hw.Print("Trigger %d: %d %d %d | ",
                 i + 1,
                 button_handler->triggers[i].RisingEdge(),
                 button_handler->triggers[i].Pressed(),
                 button_handler->triggers[i].FallingEdge());
    }
    hw.PrintLine("");
}

void PrintOutputs()
{
    hw.PrintLine("ADSR: " FLT_FMT3 " | LFO: " FLT_FMT3 " " FLT_FMT3
                 " | VCO: " FLT_FMT3,
                 FLT_VAR3(adsr_output),
                 FLT_VAR3(lfo_output.first),
                 FLT_VAR3(lfo_output.first),
                 FLT_VAR3(vco_output));
}
// Debug functions


// Main functions
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    if(shouldApplyToggles)
    {
        lfo->ResetPhaseAll();

        button_handler->currentBankState  = button_handler->bankSelectState;
        button_handler->sweepToTuneActive = button_handler->sweepToTuneState;

        triggers->ClearTriggered();
        shouldApplyToggles = false;
    }

    for(size_t i = 0; i < size; i++)
    {
        bool pressed = triggers->Pressed();

        // Use frozen sweep value after release
        float sweepVal = sweep->ReleaseValue;


        // Reset envelope and LFO on trigger
        if(shouldApplyToggles)
        {
            lfo->ResetPhaseAll();

            // Aplicar mudanças pendentes
            button_handler->currentBankState = button_handler->bankSelectState;
            button_handler->sweepToTuneActive
                = button_handler->sweepToTuneState;
            triggers->ClearTriggered();
            shouldApplyToggles = false;
        }

        // Set and process envelope
        envelope->SetReleaseTime(
            ADSR_MIN_RELEASE_TIME
            + (envelope->ReleaseValue
               * (ADSR_RELEASE_TIME - ADSR_MIN_RELEASE_TIME)));
        adsr_output = envelope->Process(pressed);

        // --- Filter frequency (VCF) logic ---
        if(pressed)
        {
            vcf->UpdateCutoffPressed(sweepVal);
        }
        else
        {
            vcf->CutoffFreq
                = sweep->UpdateCutoffFreq(sweepVal, vcf, adsr_output);
            vcf->SetFreq(vcf->CutoffFreq);
        }


        // Initial output from envelope
        output = adsr_output;

        // --- LFO processing ---
        lfo->SetFreqAll(lfo->RateValue);
        lfo->SetAmpAll(
            1.0f); // LFO at full amplitude, FM ratio will scale deviation
        lfo_output = lfo->ProcessAll();

        // --- VCO frequency and modulation with FM-inspired deviation ---
        // Calculate base carrier frequency from tune knob
        float carrier_freq
            = VCO_MIN_FREQ * powf(VCO_MAX_FREQ / VCO_MIN_FREQ, vco->TuneValue);

        // Convert LFO output from [0,1] to [-1,+1]
        float lfo_bipolar = (lfo_output.second - 0.5f) * 2.0f;

        // FM synthesis: depth controls intensity, ratio stored in VCO
        float vco_freq
            = vco->CalculateFMFreq(carrier_freq, lfo_bipolar, lfo->DepthValue);

        // Optional sweep modulation mapped to VCO frequency
        if(button_handler->sweepToTuneActive)
        {
            float direction = 2.0f * (sweepVal - 0.5f);

            // Get VCO intensity using the dedicated method
            float intensity = sweep->CalculateVcoIntensity(sweepVal);

            float end_exp   = 0.5f - 0.5f * direction;
            float sweep_exp = vco->TuneValue
                              + (end_exp - vco->TuneValue)
                                    * (1.0f - adsr_output) * intensity;

            // Recalculate carrier with sweep, then apply FM modulation
            carrier_freq
                = VCO_MIN_FREQ * powf(VCO_MAX_FREQ / VCO_MIN_FREQ, sweep_exp);

            // Recalculate with swept carrier frequency
            vco_freq = vco->CalculateFMFreq(
                carrier_freq, lfo_bipolar, lfo->DepthValue);
        }


        vco->SetFreq(vco_freq);
        vco_output = vco->Process();
        output *= vco_output;

        // -- LFO LED control ---
        led_lfo.Set((lfo_output.first * 0.5f + 0.5f) * adsr_output);

        led_lfo.Update();

        // --- Apply VCF low-pass filter ---
        output = vcf->Process(output);

        // --- Apply output amplifier ---
        output = out_amp->Process(output);

        // --- Send to output buffer (stereo) ---
        out[0][i] = output;
        out[1][i] = output;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    SAMPLE_RATE = hw.AudioSampleRate();
    BLOCK_SIZE  = hw.AudioBlockSize();
    led_sweep.Init(daisy::seed::D27, GPIO::Mode::OUTPUT);
    led_bank.Init(daisy::seed::D28, GPIO::Mode::OUTPUT);
    led_lfo.Init(daisy::seed::D29, false, SAMPLE_RATE);
    knob_handler->InitAll();
    button_handler->InitAll();
    InitComponents(SAMPLE_RATE, BLOCK_SIZE);

    if(DEBUG)
    {
        hw.StartLog(true);
        hw.PrintLine("Daisy Dub Siren");
    }

    hw.StartAudio(AudioCallback);

    while(1)
    {
        knob_handler->UpdateAll();
        button_handler->DebounceAll();
        button_handler->UpdateAll();
        led_sweep.Write(button_handler->sweepToTuneState);
        led_bank.Write(button_handler->bankSelectState);

        if(triggers->Triggered())
        {
            shouldApplyToggles = true;
        }

        if(DEBUG)
        {
            System::Delay(10);
        }
    }
}
