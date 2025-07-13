#pragma once

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Constants
#define VCO_WAVEFORM Oscillator::WAVE_POLYBLEP_SQUARE
#define VCO_MIN_FREQ 30.0f
#define VCO_MAX_FREQ 9000.0f

#define ADSR_ATTACK_TIME 0.3f
#define ADSR_DECAY_TIME 0.1f
#define ADSR_SUSTAIN_LEVEL 1.f
#define ADSR_RELEASE_TIME 10.f
#define ADSR_MIN_RELEASE_TIME 0.1f

#define LFO_0_WAVEFORM Oscillator::WAVE_SIN
#define LFO_1_WAVEFORM Oscillator::WAVE_SQUARE
#define LFO_2_WAVEFORM Oscillator::WAVE_SAW
#define LFO_3_WAVEFORM Oscillator::WAVE_RAMP
#define LFO_MIN_FREQ 1.0f
#define LFO_MAX_FREQ 20.0f

#define VCF_FILTER OnePole::FILTER_MODE_LOW_PASS
#define VCF_MIN_FREQ 24.0f
#define VCF_MAX_FREQ 15000.0f

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
    DecayEnvelope(int sample_rate, int block_size)
    {
        this->envelope.Init(sample_rate, block_size);
        this->envelope.SetTime(ADSR_SEG_ATTACK, ADSR_ATTACK_TIME);
        this->envelope.SetTime(ADSR_SEG_DECAY, ADSR_DECAY_TIME);
        this->envelope.SetTime(ADSR_SEG_RELEASE, ADSR_RELEASE_TIME);
        this->envelope.SetSustainLevel(ADSR_SUSTAIN_LEVEL);
    }

    Adsr  envelope;
    float ReleaseValue;  // Knob value from 0.0f to 1.0f
    float EnvelopeValue; // Current envelope value from 0.0f to 1.0f

    void  SetReleaseTime(float time);
    float Process(bool gate);
    void  Retrigger();
};
// DecayEnvelope


// Sweep
class Sweep
{
  public:
    Sweep(int sample_rate, int block_size)
    {
        this->SweepValue          = 0.0f;
        this->IsSweepToTuneActive = false;
        this->envelope.Init(sample_rate, block_size);
        this->envelope.SetTime(ADSR_SEG_ATTACK, ADSR_ATTACK_TIME);
        this->envelope.SetTime(ADSR_SEG_DECAY, ADSR_DECAY_TIME);
        this->envelope.SetTime(ADSR_SEG_RELEASE, ADSR_RELEASE_TIME);
        this->envelope.SetSustainLevel(ADSR_SUSTAIN_LEVEL);
    }

    float SweepValue; // Knob value from 0.0f to 1.0f
    bool  IsSweepToTuneActive;
    float CutoffFreq;

    Adsr  envelope;
    float ReleaseValue;  // Knob value from 0.0f to 1.0f
    float EnvelopeValue; // Current envelope value from 0.0f to 1.0f

    void  SetReleaseTime(float time);
    float Process(bool gate);
    void  Retrigger();
};
// Sweep


// Triggers
class Triggers
{
  public:
    Triggers() {}

    bool Triggered();
    bool Pressed();
    bool Released();
    void ClearTriggered();
    bool IsBankSelectActive = false;
};
// Triggers


// Lfo
class Lfo
{
  public:
    Lfo(int sample_rate)
    {
        for(int i = 0; i < 4; i++)
        {
            osc[i].Init(sample_rate);
            osc[i].SetWaveform(Oscillator::WAVE_SIN);
            osc_harm[i].Init(sample_rate);
            osc_harm[i].SetWaveform(Oscillator::WAVE_SIN);
        }
    }


    float      DepthValue;
    float      RateValue;
    Oscillator osc[4];
    Oscillator osc_harm[4];
    float      values[5][2]; // oscillator value and modsig value

    void                    Init();
    void                    UpdateWaveforms(int index, bool bankB);
    float                   MixLfoSignals(int index, bool bankB);
    void                    SetAmpAll(float amp);
    void                    SetFreqAll(float freq);
    void                    ResetPhaseAll();
    std::pair<float, float> ProcessAll();
}; // Lfo


// Vco
class Vco
{
  public:
    Vco(int sample_rate)
    {
        this->osc.Init(sample_rate);
        this->osc.SetWaveform(VCO_WAVEFORM);
        this->osc.SetAmp(1.0f);
        this->osc.SetFreq(440.0f);
    }

    Oscillator osc;
    float      TuneValue;

    void  Init();
    void  SetFreq(float freq);
    float Process();
};
// Vco


// Vcf
class Vcf
{
  public:
    Vcf(int sample_rate)
    {
        this->filter.Init(sample_rate);
        this->filter.SetDrive(100.0f);
        this->filter.SetRes(0.95f);
    }

    Svf filter;
    //OnePole filter;

    void  SetFreq(float freq);
    float Process(float in);
};
// Vcf


// OutAmp
class OutAmp
{
  public:
    OutAmp() { this->VolumeValue = 0.0f; }

    float VolumeValue;

    void  SetVolume(float volume);
    float Process(float in);
};
// OutAmp


// Function declarations
void InitComponents(int sample_rate, int block_size);

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
        for(int i = 0; i < 4; i++)
        {
            this->triggersStates[i][0] = false; // index 0 is Triggered
            this->triggersStates[i][1] = false; // index 1 is Pressed
            this->triggersStates[i][2] = false; // index 2 is Released
        }
        this->LastIndex        = 0;
        this->bankSelectState  = false;
        this->sweepToTuneState = false;
    }

    // There are 4 trigger buttons.
    // Each one has 3 states (true of false): Triggered, Pressed, Released.
    bool triggersStates[4][3];
    bool bankSelectState;
    bool sweepToTuneState;
    int  LastIndex;

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
