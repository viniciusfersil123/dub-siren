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
        osc.SetWaveform(osc.WAVE_POLYBLEP_SAW);
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


// Function declarations
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size);
void init_knobs();
void init_components();