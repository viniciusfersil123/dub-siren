#pragma once

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Daisy setup
KnobInitializerDaisy* knobInit;
enum AdcChannel
{
    VolumeKnob = 0,
    DecayKnob,
    DepthKnob,
    TuneKnob,
    SweepKnob,
    RateKnob,
    NUM_ADC_CHANNELS
};


// Dub Siren components
DecayEnvelope* decay_env;
Sweep* sweep;
Triggers* triggers;
Lfo* lfo;
Vco* vco;
Vcf* vcf;
OutAmp* out_amp;



// DecayEnvelope
class DecayEnvelope
{
public:
    DecayEnvelope()
    {
        this->Init();
        decay_env.SetAttackTime(0);
        decay_env.SetDecayTime(0);
        decay_env.SetSustainLevel(0);
        decay_env.SetReleaseTime(0);
        // decay_env.SetTime();
    }

    Adsr decay_env;
    float DecayValue;

    void Init();
    void SetDecayTime(float time);
    float Process();
    void Retrigger();
};
// DecayEnvelope



// Sweep
class Sweep
{
public:
    Sweep()
    {
        this->Init();
    }

    float SweepValue;
    Switch SweepToTuneButton;

    void Init();
    void Debounce();
    bool Pressed();
};
// Sweep



// Triggers
class Triggers
{
public:
    Triggers()
    {
        this->Init();
    }

    Switch button[4];
    bool buttonState[4];
    int activeEnvelopeIndex;
    Switch BankSelect;

    void Init();
    void DebounceAllButtons();
    const int GetActiveEnvelopeIndex();
    const bool AnyButtonPressed();
};
// Triggers



// Lfo
class Lfo
{
public:
    Lfo()
    {
        this->Init();
        osc[0].SetWaveform(Oscillator::WAVE_SIN);
        osc[1].SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
        osc[2].SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
        osc[3].SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);
    }

    float DepthValue;
    float RateValue;
    Oscillator osc[4];
    float value[4];

    void Init();
    void SetAmpAll(float amp);
    void SetFreqAll(float freq);
    void ResetPhaseAll();
    float ProcessAll();
}; // Lfo


// Vco
class Vco
{
public:
    Oscillator osc;
    float TuneValue;

    Vco()
    {
        this->Init();
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
    }

    void Init();
    void SetFreq(float freq);
    float Process();
};
// Vco



// Vcf
class Vcf
{
public:
    Vcf()
    {
        filter.Init();
        filter.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    }
    
    OnePole filter;

    void SetFreq(float freq);
    float Process(float in);
};
// Vcf



// OutAmp
class OutAmp
{
public:
    OutAmp()
    {
    }

    float VolumeValue;
};
// OutAmp



// Function declarations
void InitComponents();

class KnobInitializer
{
public:
    virtual void InitKnobs() = 0;
};