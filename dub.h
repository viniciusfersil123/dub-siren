#pragma once

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

// Knob values
float VolumeValue;
float DecayValue;
float DepthValue;
float TuneValue;
float SweepValue;
float RateValue;

// Dub Siren components
Vco* vco;
Lfo* lfo;
Vcf* vcf;
EnvelopeGenerator* env_gen;

// Switch lfoButton1;
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

class Vco
{
public:
    Oscillator osc;
    float value;

    Vco (float sample_rate)
    {
        osc.Init(sample_rate);
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
    }

    void SetFreq(float freq)
    {
        osc.SetFreq(freq);
    }

    float Process()
    {
        return osc.Process();
    }
};

class Lfo
{
public:
    bool  lfoStates[4] = {false, false, false, false};
    Oscillator osc;

    Lfo(float sample_rate)
    {
        osc.Init(sample_rate);
        osc.SetWaveform(osc.WAVE_POLYBLEP_SQUARE);
    }

    void SetAmp(float amp)
    {
        osc.SetAmp(amp);
    }

    void SetFreq(float freq)
    {
        osc.SetFreq(freq);
    }

    float Process()
    {
        return osc.Process();
    }
};

class Vcf
{
public:
    OnePole filter;
    float value;

    Vcf()
    {
        filter.Init();
        filter.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    }
    
    void SetFreq(float freq)
    {
        filter.SetFrequency(freq);
    }

    float Process(float in)
    {
        return filter.Process(in);
    }
};

class Envelopes
{
public:
    Envelopes()
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

    float ProcessAll();

private:
    Oscillator osc[4];
    float value[4];
};

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

class DecayEnvelope
{
public:
    DecayEnvelope()
    {
        decay_envelope.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
        decay_envelope.SetAttackTime(0);
        decay_envelope.SetDecayTime(0);
        decay_envelope.SetSustainLevel(0);
        decay_envelope.SetReleaseTime(0);
        // decay_envelope.SetTime();
    }

    void SetDecayTime(float time);
    float Process();
    void Retrigger();

private:
    Adsr decay_envelope;
};

class EnvelopeGenerator
{
public:
    EnvelopeGenerator() { }

    float Process();

private:
    Triggers triggers;
    Envelopes envelopes;
    DecayEnvelope decay_envelope;
};


// Function declarations
void init_knobs();
void init_components();