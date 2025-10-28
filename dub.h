#pragma once

#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Constants
#define VCO_WAVEFORM Oscillator::WAVE_POLYBLEP_SQUARE
#define VCO_MIN_FREQ 32.703f // Corresponds to a midi C1
#define VCO_MAX_FREQ 1046.5f // Corresponds to a midi C6

#define ADSR_ATTACK_TIME 0.3f
#define ADSR_DECAY_TIME 0.1f
#define ADSR_SUSTAIN_LEVEL 1.f
#define ADSR_RELEASE_TIME 15.f
#define ADSR_MIN_RELEASE_TIME 0.1f

#define LFO_0_WAVEFORM Oscillator::WAVE_SIN
#define LFO_1_WAVEFORM Oscillator::WAVE_SQUARE
#define LFO_2_WAVEFORM Oscillator::WAVE_SAW
#define LFO_3_WAVEFORM Oscillator::WAVE_RAMP
#define LFO_MIN_FREQ 0.0f
#define LFO_MAX_FREQ 20.0f

#define VCF_FILTER OnePole::FILTER_MODE_LOW_PASS
#define VCF_MIN_FREQ 15.0f
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
};
// DecayEnvelope


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
        this->prevIndex    = -1;
        this->currIndex    = -1;
        this->fadeProgress = 1.0f;                        // 1.0 significa fim
        this->fadeRate     = 1.0f / (sample_rate * 0.1f); // 100ms fade
    }


    float                   DepthValue;
    float                   RateValue;
    Oscillator              osc[4];
    Oscillator              osc_harm[4];
    float                   values[5][2]; // oscillator value and modsig value
    int                     prevIndex;
    int                     currIndex;
    float                   fadeProgress;
    float                   fadeRate;
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
        this->CutoffFreq = VCF_MIN_FREQ;
    }

    Svf filter;
    //OnePole filter;
    float CutoffFreq;
    float CutoffExponent;

    void  SetFreq(float freq);
    void  UpdateCutoffPressed(float sweepValue);
    float Process(float in);
};
// Vcf


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

    Adsr  envelope;
    float ReleaseValue;  // Knob value from 0.0f to 1.0f
    float EnvelopeValue; // Current envelope value from 0.0f to 1.0f

    void  SetReleaseTime(float time);
    float Process(bool gate);
    float CalculateFilterIntensity(float sweepValue);
    float CalculateVcoIntensity(float sweepValue);
    float UpdateCutoffFreq(float sweepValue, Vcf* vcf, float adsrOutput);
};
// Sweep


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
        this->currentBankState = false; // Banco atualmente ativo
    }

    // There are 4 trigger buttons.
    // Each one has 3 states (true of false): Triggered, Pressed, Released.
    bool triggersStates[4][3];
    bool bankSelectState;
    bool currentBankState; // Banco atualmente ativo
    bool sweepToTuneState;
    bool sweepToTuneActive;
    int  LastIndex;

    virtual void InitAll();
    virtual void DebounceAll();
    virtual void UpdateAll();
};

class ButtonHandlerDaisy : public ButtonHandler
{
  public:
    ButtonHandlerDaisy()
    {
        this->bankSelectState   = false; // já existe
        this->currentBankState  = false; // já existe
        this->sweepToTuneState  = false; // já existe (pendente)
        this->sweepToTuneActive = false; // novo - estado real (ativo)
    }


    Switch triggers[4];
    Switch bankSelect;
    Switch sweepToTune;

    bool bankSelectState;
    bool currentBankState;
    bool sweepToTuneState;
    bool sweepToTuneActive;

    void InitAll() override;
    void DebounceAll() override;
    void UpdateAll() override;
};