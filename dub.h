#pragma once

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

// Knob values
float VolumeValue;

// Dub Siren components
EnvelopeGenerator* env_gen;
Triggers* triggers;
Lfo* lfo;
Vco* vco;
Vcf* vcf;

enum AdcChannel
{
    VolumeKnob = 0,
    DecayKnob,
    DepthKnob,
    TuneKnob,
    SweepKnob,
    RateKnob,
    VcfKnob,
    NUM_ADC_CHANNELS
};



// EnvelopeGenerator
class EnvelopeGenerator
{
public:
    EnvelopeGenerator()
    {
    }

    float SweepValue;
    float Process();
private:
    DecayEnvelope decay_env;
    SweepToTuneButton sweep_to_tune_button;
};
// EnvelopeGenerator



// DecayEnvelope
class DecayEnvelope
{
public:
    DecayEnvelope()
    {
        decay_env.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
        decay_env.SetAttackTime(0);
        decay_env.SetDecayTime(0);
        decay_env.SetSustainLevel(0);
        decay_env.SetReleaseTime(0);
        // decay_env.SetTime();
    }

    float DecayValue;

    void SetDecayTime(float time);
    float Process();
    void Retrigger();
private:
    Adsr decay_env;
};
// DecayEnvelope



// SweepToTuneButton
class SweepToTuneButton
{
public:
    SweepToTuneButton()
    {
        button.Init(hw.GetPin(32),
                    50
                );
    }

    void Debounce();
    bool Pressed();
private:
    Switch button;
};
// SweepToTuneButton



// Triggers
class Triggers
{
public:
    Triggers()
    {
        for (int i = 0; i < 4; i++)
        {
            button[i].Init(hw.GetPin(28 + i), 50);
            buttonState[i] = false;
        }
        activeEnvelopeIndex = 0;
    }

    void DebounceAllButtons();
    const int GetActiveEnvelopeIndex();
    const bool AnyButtonPressed();
private:
    Switch button[4];
    bool buttonState[4];
    int activeEnvelopeIndex;
};
// Triggers



// Lfo
class Lfo
{
public:
    Lfo()
    {
        osc[0].Init(hw.AudioSampleRate());
        osc[0].SetWaveform(Oscillator::WAVE_SIN);

        osc[1].Init(hw.AudioSampleRate());
        osc[1].SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);

        osc[2].Init(hw.AudioSampleRate());
        osc[2].SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
        
        osc[3].Init(hw.AudioSampleRate());
        osc[3].SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);
    }

    void SetAmpAll(float amp);
    void SetFreqAll(float freq);
    void ResetPhaseAll();
    float ProcessAll();
private:
    Oscillator osc[4];
    float value[4];
    float DepthValue;
    float RateValue;
};
// Lfo



// Vco
class Vco
{
public:
    Oscillator osc;
    float TuneValue;

    Vco ()
    {
        osc.Init(hw.AudioSampleRate());
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
    }

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
    
    void SetFreq(float freq);
    float Process(float in);
private:
    OnePole filter;
};
// Vcf



// Function declarations
void init_knobs();
void init_components();