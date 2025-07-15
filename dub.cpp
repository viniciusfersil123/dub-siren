#include "daisy_seed.h"
#include "daisysp.h"
#include "dub.h"

/* ADCs and pin numbers (in Daisy Seed)

KNOB   | PIN NUMBER
-------------------
Volume | 21
Decay  | 22
Depth  | 23
Tune   | 24
Sweep  | 25
Rate   | 26

BUTTON      | PIN NUMBER
------------------------
Trigger 1   | 27
Trigger 2   | 28
Trigger 3   | 29
Trigger 4   | 30
BankSelect  | 31
SweepToTune | 32
*/
using namespace daisy;
using namespace daisysp;

// Daisy setup components
KnobHandlerDaisy*   knob_handler   = new KnobHandlerDaisy();
ButtonHandlerDaisy* button_handler = new ButtonHandlerDaisy();
int                 SAMPLE_RATE = 0, BLOCK_SIZE = 0;
float               output, adsr_vcf, adsr_output, vco_output, vco_modulation;
std::pair<float, float> lfo_output = std::make_pair(0, 0);

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
bool           test = false; // Used to test the Sweep LED
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
    envelope->ReleaseValue = hw.adc.GetFloat(DecayKnob);

    sweep->ReleaseValue = hw.adc.GetFloat(SweepKnob);

    // LFO depth and rate knobs
    lfo->DepthValue = fclamp(hw.adc.GetFloat(DepthKnob), 0.f, 1.f);
    lfo->RateValue  = hw.adc.GetFloat(RateKnob);

    // OutAmp volume knob
    out_amp->VolumeValue = hw.adc.GetFloat(VolumeKnob);
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
    // Update trigger states with latch mechanism
    for(int i = 0; i < 4; i++)
    {
        // Only set triggered to true, don't clear it here (latch mechanism)
        if(this->triggers[i].RisingEdge())
        {
            this->triggersStates[i][0] = true; // Latch the trigger
            this->LastIndex            = i;
        }

        // Trigger is being pressed
        this->triggersStates[i][1] = this->triggers[i].Pressed();

        // Trigger is released
        this->triggersStates[i][2] = this->triggers[i].FallingEdge();
    }

    // Update bank select and sweep to tune states
    if(this->bankSelect.RisingEdge())
    {
        this->bankSelectState = !this->bankSelectState;
    }
    if(this->sweepToTune.RisingEdge())
    {
        this->sweepToTuneState = !this->sweepToTuneState;
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

void DecayEnvelope::Retrigger()
{
    envelope.Retrigger(true);
}
// DecayEnvelope functions


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

void Sweep::Retrigger()
{
    envelope.Retrigger(true);
}
// Sweep functions


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
    int  index = button_handler->LastIndex;
    bool bankB = button_handler->bankSelectState;

    UpdateWaveforms(index, bankB);
    float lfo_val = MixLfoSignals(index, bankB);

    float scaled_lfo = lfo_val * DepthValue; // Scale LFO to [-depth,+depth]
    float modsig     = 0.5f + scaled_lfo;    // Add DC offset

    return std::make_pair(lfo_val, modsig);
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
// Vco functions


// Vcf functions
void Vcf::SetFreq(float freq)
{
    // Svf filter accepts frequency up to SAMPLE_RATE / 3
    float limited_freq = fclamp(freq, VCF_MIN_FREQ, SAMPLE_RATE / 4);
    this->filter.SetFreq(limited_freq);
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
    for(size_t i = 0; i < size; i++)
    {
        bool  triggered = triggers->Triggered();
        bool  pressed   = triggers->Pressed();
        float sweepVal  = hw.adc.GetFloat(SweepKnob);

        // Reset envelope and LFO on trigger
        if(triggered)
        {
            envelope->Retrigger();
        }

        // Set and process envelope
        envelope->SetReleaseTime(
            ADSR_MIN_RELEASE_TIME
            + (envelope->ReleaseValue
               * (ADSR_RELEASE_TIME - ADSR_MIN_RELEASE_TIME)));
        adsr_output = envelope->Process(pressed);

        // --- Filter frequency (VCF) logic ---
        // static float cutoff_exponent = 0.0f;
        float cutoff_exponent = sweepVal;

        if(pressed)
        {
            // When pressed, control VCF freq with sweep knob using exponential curve
            // cutoff_exponent = powf(sweepVal, 0.5f);
            sweep->CutoffFreq
                = VCF_MIN_FREQ
                  * powf(VCF_MAX_FREQ / VCF_MIN_FREQ, cutoff_exponent);
            vcf->SetFreq(sweep->CutoffFreq);
        }
        else
        {
            // When not pressed, modulate VCF freq based on envelope and sweep direction
            float start_exp = cutoff_exponent;
            float end_exp;

            if(sweepVal < 0.5f) // Sweep goes up
            {
                end_exp = 1.0f; // VCF_MAX_FREQ exponent
            }
            else // Sweep goes down
            {
                end_exp = 0.0f; // VCF_MIN_FREQ exponent
            }

            // Exponentially interpolate in the exponent domain
            float sweep_exp
                = start_exp + (end_exp - start_exp) * (1.0f - adsr_output);

            // Apply the exponential mapping
            float cutoff
                = VCF_MIN_FREQ * powf(VCF_MAX_FREQ / VCF_MIN_FREQ, sweep_exp);

            vcf->SetFreq(cutoff);
        }

        // Initial output from envelope
        output = adsr_output;

        // --- LFO processing ---
        lfo->SetFreqAll(fmap(lfo->RateValue, LFO_MIN_FREQ, LFO_MAX_FREQ));
        // maps 0→0.1, 0.5→~0.316, 1→1
        float depth_exp = powf(10.f, (lfo->DepthValue - 1.0f));
        lfo->SetAmpAll(depth_exp);

        if(triggered)
        {
            lfo->ResetPhaseAll();
            triggers->ClearTriggered();
        }

        lfo_output = lfo->ProcessAll();

        // --- VCO frequency and modulation ---
        vco_modulation = lfo_output.second;

        // Calculate VCO frequency with proper depth scaling
        // Convert [0,1] range to [-1,1] and scale by depth, then add to base tune
        float modulation_amount
            = (vco_modulation - 0.5f) * 2.0f * lfo->DepthValue;
        float tune_with_mod
            = vco->TuneValue
              + modulation_amount * 0.5f; // Scale modulation range
        tune_with_mod
            = fclamp(tune_with_mod, 0.0f, 1.0f); // Keep within valid range

        // VCO frequency is exponentially mapped
        float vco_freq
            = VCO_MIN_FREQ * powf(VCO_MAX_FREQ / VCO_MIN_FREQ, tune_with_mod);

        // Optional sweep modulation mapped to VCO frequency
        if(button_handler->sweepToTuneState)
        {
            // Calculate start and end exponents for exponential interpolation
            float start_exp = tune_with_mod;
            float end_exp;

            if(sweepVal < 0.5f) // Sweep goes up
            {
                end_exp = 1.0f; // VCO_MAX_FREQ exponent
            }
            else // Sweep goes down
            {
                end_exp = 0.0f; // VCO_MIN_FREQ exponent
            }

            // Exponentially interpolate in the exponent domain
            float sweep_exp
                = start_exp + (end_exp - start_exp) * (1.0f - adsr_output);

            // Apply the exponential mapping
            vco_freq
                = VCO_MIN_FREQ * powf(VCO_MAX_FREQ / VCO_MIN_FREQ, sweep_exp);
        }

        vco->SetFreq(vco_freq);
        vco_output = vco->Process();
        output *= vco_output;

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
        if(button_handler->sweepToTune.RisingEdge())
        {
            sweep->IsSweepToTuneActive = !sweep->IsSweepToTuneActive;
        }
        if(button_handler->bankSelect.RisingEdge())
        {
            test = !test;
        }
        led_sweep.Write(button_handler->sweepToTuneState);
        led_bank.Write(test);

        if(DEBUG)
        {
            // PrintKnobValues();
            // PrintButtonStates();
            // PrintOutputs();
            // hw.PrintLine("Active Trigger: %d", triggers->LastIndex);
            System::Delay(10);
        }
    }
}
