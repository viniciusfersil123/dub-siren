#pragma once

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

// Daisy setup
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

// DecayEnvelope
class DecayEnvelope
{
public:
    DecayEnvelope()
    {
        this->Init();
        decay_env.SetTime(ADSR_SEG_ATTACK, .1f);
        decay_env.SetTime(ADSR_SEG_DECAY, .0f);
        decay_env.SetSustainLevel(1.0f);
        decay_env.SetTime(ADSR_SEG_RELEASE, .1f);
    }

    Adsr decay_env;
    float DecayValue;

    void Init();
    void SetDecayTime(float time);
    float Process(bool gate, float in);
    void Retrigger();
};
// DecayEnvelope



// Sweep
class Sweep
{
public:
    Sweep()
    {
        this->SweepValue = 0;
    }

    float SweepValue;
    bool IsSweepToTuneActive();
};
// Sweep



// Triggers
class Triggers
{
public:
    Triggers()
    {
    }

    bool IsBankSelectActive();
    bool Triggered();
    bool Pressed();
    int ActiveIndex();
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
    float values[4][2]; // oscillator value and modsig value

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
    Vco()
    {
        this->Init();
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);
        osc.SetAmp(1.0f);
        osc.SetFreq(440.0f);
    }

    Oscillator osc;
    float TuneValue;

    void Init();
    void SetFreq(float freq);
    float Process(float in);
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
        this->VolumeValue = 0.0f;
    }

    float VolumeValue;

    void SetVolume(float volume);
    float Process(float in);
};
// OutAmp



// Function declarations
void InitComponents();

class KnobHandler
{
public:
    virtual void InitAll();
    virtual void UpdateAll();
};

class KnobHandlerDaisy : public KnobHandler 
{
public:
    void InitAll() override;
    void UpdateAll() override;
};

class ButtonHandler
{
public:
    ButtonHandler()
    {
        for (int i = 0; i < 4; i++) { this->triggersStates[i] = false; }
        this->triggered = false;
        this->bankSelectState = false;
        this->sweepToTuneState = false;
    }

    bool triggersStates[4];
    bool triggered;
    bool bankSelectState;
    bool sweepToTuneState;

    virtual void InitAll();
    virtual void DebounceAll();
    virtual void UpdateAll();
};

class ButtonHandlerDaisy : public ButtonHandler
{
public:
    Switch triggers[4];
    Switch bankSelect;
    Switch sweepToTune;

    void InitAll() override;
    void DebounceAll() override;
    void UpdateAll() override;
};
