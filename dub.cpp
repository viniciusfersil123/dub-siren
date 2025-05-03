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
KnobHandlerDaisy* knob_handler = new KnobHandlerDaisy();
ButtonHandlerDaisy* button_handler = new ButtonHandlerDaisy();
int SR = 0;

// Dub Siren components
DecayEnvelope* decay_env;
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
    decay_env->DecayValue = hw.adc.GetFloat(DecayKnob);
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
        this->triggersStates[i] = this->triggers[i].Pressed();
    }

    // Check if any trigger is pressed
    this->triggered = false;
    for (int i = 0; i < 4; i++) {
        if (this->triggers[i].RisingEdge()) {
            this->triggered = true;
            break;
        }
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
void InitComponents()
{
    triggers = new Triggers();
    decay_env = new DecayEnvelope();
    sweep = new Sweep();
    lfo = new Lfo();
    vco = new Vco();
    vcf = new Vcf();
    out_amp = new OutAmp();
}
// Init functions



// DecayEnvelope functions
void DecayEnvelope::Init()
{
    this->decay_env.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
}

void DecayEnvelope::SetDecayTime(float time)
{
    this->decay_env.SetTime(ADSR_SEG_RELEASE, time);
}

float DecayEnvelope::Process(bool gate, float in)
{
    return in * decay_env.Process(gate);
}

void DecayEnvelope::Retrigger()
{
    decay_env.Retrigger(true);
}
// DecayEnvelope functions



// Triggers functions
bool Triggers::IsBankSelectActive()
{
    return button_handler->bankSelectState;
}

bool Triggers::Triggered()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggered) return true;
    }
    return false;
}

bool Triggers::Pressed()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggers[i].Pressed())
            return true;
    }
    return false;
}

int Triggers::ActiveIndex()
{
    for (int i = 0; i < 4; i++) {
        if (button_handler->triggersStates[i])
            return i;
    }
    return -1;
}
// Triggers functions



// Sweep functions
bool Sweep::IsSweepToTuneActive()
{
    return button_handler->sweepToTuneState;
}
// Sweep functions



// Lfo functions
void Lfo::Init()
{
    this->osc[0].Init(hw.AudioSampleRate());
    this->osc[1].Init(hw.AudioSampleRate());
    this->osc[2].Init(hw.AudioSampleRate());
    this->osc[3].Init(hw.AudioSampleRate());
}

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

float Lfo::ProcessAll()
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
    if (activeIndex == -1)
        return 0.0f; // No active trigger
    return this->values[activeIndex][1];
}
// Lfo functions



// Vco functions
void Vco::Init()
{
    this->osc.Init(hw.AudioSampleRate());
}

void Vco::SetFreq(float freq)
{
    this->osc.SetFreq(freq);
}

float Vco::Process(float in)
{
    return in * this->osc.Process();
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
    // hw.Print("   Decay:  " FLT_FMT3, FLT_VAR3(decay_env->DecayValue));
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
            button_handler->triggers[i].Pressed(),
            button_handler->triggers[i].RisingEdge(),
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
        float output = 0, lfo_output = 0; // MONO

        // // The LFO and Decay Envelope should reset when a trigger is pressed
        if (triggers->Triggered()) {
            lfo->ResetPhaseAll();
            decay_env->Retrigger();
        }

        // Set and apply Decay Envelope
        decay_env->SetDecayTime(0.1f + (decay_env->DecayValue * 9.9f));
        output = decay_env->Process(true, 1);
        
        // Set and apply LFO
        lfo->SetFreqAll(20 * lfo->RateValue);
        lfo->SetAmpAll(lfo->DepthValue);
        lfo_output = lfo->ProcessAll();
        output *= lfo_output;

        // Set and apply VCO
        // if (!sweep->IsSweepToTuneActive()) { // FILTER
        //     vco->SetFreq(30.0f + 9000.0f * vco->TuneValue);
        // } else { // FILTER + TUNE
        //     vco->SetFreq(30.0f + 9000.0f * sweep->SweepValue);
        // }
        // output = vco->Process(output);

        // Set and apply VCO
        vco->SetFreq(30.0f + 9000.0f * vco->TuneValue);
        output = vco->Process(output);

        // Set and apply VCF low-pass filter
        // vcf->SetFreq((20.0f + (lfo_output * 12000.0f)) / SR); // Must be normalized to sample rate
        // output = vcf->Process(output);
        
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
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    SR = hw.AudioSampleRate();

    knob_handler->InitAll();
    button_handler->InitAll();
    InitComponents();

    hw.StartLog(true);
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
        hw.PrintLine("LFO value: " FLT_FMT3, FLT_VAR3(lfo->values[triggers->ActiveIndex()][1]));
        // hw.PrintLine("");
        // System::Delay(250);
    }
}
