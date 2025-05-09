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

BUTTON      | PIN NUMBER ------------------------
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
KnobHandlerDaisy* knob_handler = new KnobHandlerDaisy();
ButtonHandlerDaisy* button_handler = new ButtonHandlerDaisy();
int SAMPLE_RATE = 0, BLOCK_SIZE = 0;

// Dub Siren components
DecayEnvelope* envelope;
Sweep* sweep;
Triggers* triggers;
Lfo* lfo;
Vco* vco;
Vcf* vcf;
OutAmp* out_amp;

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
    vco->TuneValue = hw.adc.GetFloat(TuneKnob);

    // Decay Envelope knobs
    envelope->ReleaseValue = hw.adc.GetFloat(DecayKnob);
    sweep->SweepValue = hw.adc.GetFloat(SweepKnob);

    // LFO depth and rate knobs
    lfo->DepthValue = hw.adc.GetFloat(DepthKnob);
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
    for (int i = 0; i < 4; i++) { this->triggers[i].Debounce(); }
    this->bankSelect.Debounce();
    this->sweepToTune.Debounce();
}

void ButtonHandlerDaisy::UpdateAll()
{
    // Update trigger states
    for (int i = 0; i < 4; i++) {
        this->triggersStates[i][0] = this->triggers[i].RisingEdge(); // acabou de ser pressionado
        this->triggersStates[i][1] = this->triggers[i].Pressed(); // está pressionado no momento 
        this->triggersStates[i][2] = this->triggers[i].FallingEdge(); // acabou de ser solto
    }

    // Update bank select and sweep to tune states
    if (this->bankSelect.RisingEdge()) {
        this->bankSelectState = !this->bankSelectState;
    }
    if (this->sweepToTune.RisingEdge()) {
        this->sweepToTuneState = !this->sweepToTuneState;
    }
}
// ButtonHandler functions



// Init functions
void InitComponents(int sample_rate, int block_size)
{
    triggers = new Triggers();
    envelope = new DecayEnvelope(sample_rate, block_size);
    sweep = new Sweep();
    lfo = new Lfo(sample_rate);
    vco = new Vco(sample_rate);
    vcf = new Vcf();
    out_amp = new OutAmp();
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
int Triggers::ActiveIndex()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggersStates[i][1])
            return i;
    }
    return -1;
}

bool Triggers::Triggered()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggersStates[i][0])
            return true;
    }
    return false;
}

bool Triggers::Pressed()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggersStates[i][1])
            return true;
    }
    return false;
}

bool Triggers::Released()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggersStates[i][2])
            return true;
    }
    return false;
}

bool Triggers::IsBankSelectActive()
{
    return button_handler->bankSelectState;
}
// Triggers functions



// Sweep functions
bool Sweep::IsSweepToTuneActive()
{
    return button_handler->sweepToTuneState;
}
// Sweep functions



// Lfo functions
void Lfo::SetAmpAll(float amp)
{
    for (int i = 0; i < 4; i++)
        this->osc[i].SetAmp(amp);
}

void Lfo::SetFreqAll(float freq)
{
    for (int i = 0; i < 4; i++) {
        this->osc[i].SetFreq(freq);
    }
}

void Lfo::ResetPhaseAll()
{
    for (int i = 0; i < 4; i++)
        this->osc[i].Reset();
}

std::pair<float, float> Lfo::ProcessAll()
{
    // Process all 4 envelopes and store in value array
    for (int i = 0; i < 4; i++) {
        // Based on the Tremolo.cpp example in daisysp
        this->values[i][0] = this->osc[i].Process();

        float dc_os_ = 0.5f * (1 - fclamp(this->DepthValue, 0.f, 1.f));
        this->values[i][1] = dc_os_ + this->values[i][0];
    }

    // Return the value of the active trigger
    int activeIndex = triggers->ActiveIndex();
    if (activeIndex == -1) return std::make_pair(0, 0); // No active trigger
    return std::make_pair(this->values[activeIndex][0], this->values[activeIndex][1]);
}
// Lfo functions



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
    this->filter.SetFrequency(freq);
}

float Vcf::Process(float in)
{
    return this->filter.Process(in);
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
    hw.Print("Volume: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(VolumeKnob)));
    hw.Print("   Decay:  " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(DecayKnob)));
    hw.Print("   Depth:  " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(DepthKnob)));
    hw.PrintLine("");
    hw.Print("Tune:   " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(TuneKnob)));
    hw.Print("   Sweep:  " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(SweepKnob)));
    hw.Print("   Rate:   " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(RateKnob)));
    hw.PrintLine("");

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
    for (int i = 0; i < 4; i++)
    {
        hw.Print("Trigger %d: %d %d %d | ",
            i + 1,
            button_handler->triggers[i].RisingEdge(),
            button_handler->triggers[i].Pressed(),
            button_handler->triggers[i].FallingEdge()
        );
    }
    hw.PrintLine("");
}
// Debug functions



// Main functions
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float output; // MONO
        float adsr_output;
        std::pair<float, float> lfo_output = std::make_pair(0, 0);
        float vco_output = 0;
        float vco_modulation = 0;
        bool triggered = triggers->Triggered();
        bool pressed = triggers->Pressed();

        // The LFO and Decay Envelope must reset when a trigger is triggered
        if (triggered) {
            lfo->ResetPhaseAll();
            envelope->Retrigger();
        }

        // Set and apply Decay Envelope
        envelope->SetReleaseTime(ADSR_MIN_RELEASE_TIME + (envelope->ReleaseValue * (ADSR_RELEASE_TIME - ADSR_MIN_RELEASE_TIME)));
        adsr_output = envelope->Process(pressed);
        output = adsr_output;
        
        // Set and apply LFO
        lfo->SetFreqAll(LFO_MAX_FREQ * lfo->RateValue);
        lfo->SetAmpAll(lfo->DepthValue);
        lfo_output = lfo->ProcessAll();
        output *= lfo_output.second; // modsig value

        // Set and apply VCO
        // if (!sweep->IsSweepToTuneActive()) { // FILTER
        //     vco->SetFreq(30.0f + 9000.0f * vco->TuneValue);
        // } else { // FILTER + TUNE
        //     vco->SetFreq(30.0f + 9000.0f * sweep->SweepValue);
        // }
        // output = vco->Process(output);

        // Set and apply VCO
        vco_modulation = 0.5f * (lfo_output.second + 1);
        vco->SetFreq(vco_modulation * (VCO_MIN_FREQ + (VCO_MAX_FREQ * vco->TuneValue)));
        vco_output = vco->Process();
        output *= vco_output;

        // Set and apply VCF low-pass filter
        vcf->SetFreq((VCF_MIN_FREQ + (sweep->SweepValue * VCF_MAX_FREQ)) / SAMPLE_RATE); // Must be normalized to sample rate
        output = vcf->Process(output);
        // TODO: Add the release behavior to the filter

        // Apply volume
        output = out_amp->Process(output);

        // Output to both channels
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
    BLOCK_SIZE = hw.AudioBlockSize();

    knob_handler->InitAll();
    button_handler->InitAll();
    InitComponents(SAMPLE_RATE, BLOCK_SIZE);

    // hw.StartLog(true);
    hw.PrintLine("Daisy Dub Siren");
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // hw.PrintLine("Monitoring...");

        knob_handler->UpdateAll();
        button_handler->DebounceAll();
        button_handler->UpdateAll();

        // PrintKnobValues();
        // PrintButtonStates();
        // hw.PrintLine("Active Trigger: %d", triggers->ActiveIndex());
        // hw.PrintLine("LFO value: " FLT_FMT3, FLT_VAR3(lfo->values[triggers->ActiveIndex()][1]));
        // hw.PrintLine("Envelope value: " FLT_FMT3, FLT_VAR3(envelope->EnvelopeValue));
        // hw.PrintLine("Current segment: %d", envelope->envelope.GetCurrentSegment());
        // hw.PrintLine("");
        // System::Delay(500);
    }
}
