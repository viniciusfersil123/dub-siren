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

DaisySeed hw;

// Init functions
class KnobInitializerDaisy : public KnobInitializer
{
public:
    void InitKnobs() override
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
};

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



// EnvelopeGenerator functions
// float EnvelopeGenerator::Process()
// {
//     return (this->decay_env.Process() * lfo->ProcessAll());
// }
// EnvelopeGenerator functions



// DecayEnvelope functions
void DecayEnvelope::Init()
{
    decay_env.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
}

void DecayEnvelope::SetDecayTime(float time)
{
    this->SetDecayTime(time);
}

float DecayEnvelope::Process()
{
    return decay_env.Process(false);
}

void DecayEnvelope::Retrigger()
{
    decay_env.Retrigger(false);
}
// DecayEnvelope functions



// Sweep functions
void Sweep::Init()
{
    this->SweepToTuneButton.Init(hw.GetPin(32), 50);
}

void Sweep::Debounce()
{
    this->SweepToTuneButton.Debounce();
}

bool Sweep::Pressed()
{
    return this->SweepToTuneButton.Pressed();
}
// Sweep functions



// Triggers functions
void Triggers::Init()
{
    for (int i = 0; i < 4; i++)
    {
        this->button[i].Init(hw.GetPin(28 + i), 50);
        this->buttonState[i] = false;
    }
    this->activeEnvelopeIndex = 0;

    this->BankSelect.Init(hw.GetPin(31), 50);
}

void Triggers::DebounceAllButtons()
{
    for (int i = 0; i < 4; i++)
    {
        this->button[i].Debounce();
        if (this->button[i].RisingEdge())
        {
            this->activeEnvelopeIndex = i;
        }
        this->buttonState[i] = this->button[i].Pressed();
    }
}

const int Triggers::GetActiveEnvelopeIndex()
{
    return this->activeEnvelopeIndex;
}

const bool Triggers::AnyButtonPressed()
{
    for (int i = 0; i < 4; i++)
    {
        if (buttonState[i]) return true;
    }
    return false;
}
// Triggers functions



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
    for (int i = 0; i < 4; i++)
        this->osc[i].SetFreq(freq);
}

void Lfo::ResetPhaseAll()
{
    for (int i = 0; i < 4; i++)
        this->osc[i].Reset();
}

float Lfo::ProcessAll()
{
    // Process all 4 envelopes and store in value array
    for (int i = 0; i < 4; i++)
        this->value[i] = this->osc[i].Process();

    return this->value[triggers->GetActiveEnvelope()];
}
// Lfo functions



// VCO functions
void Vco::Init()
{
    osc.Init(hw.AudioSampleRate());
}

void Vco::SetFreq(float freq)
{
    osc.SetFreq(freq);
}

float Vco::Process()
{
    return osc.Process();
}
// VCO functions



// VCF functions
void Vcf::SetFreq(float freq)
{
    filter.SetFrequency(freq);
}

float Vcf::Process(float in)
{
    return filter.Process(in);
}
// VCF functions



// Main functions
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    float output = 0; // MONO

    for(size_t i = 0; i < size; i++)
    {
        // Set and apply Decay Envelope
        decay_env->SetDecayTime(decay_env->DecayValue);
        if (triggers->AnyButtonPressed()) {
            lfo->ResetPhaseAll();
            decay_env->Retrigger();
        }
        output += decay_env->Process();

        // Set and apply LFO
        lfo->SetAmpAll(lfo->DepthValue);
        lfo->SetFreqAll(20 * lfo->RateValue);
        output *= lfo->ProcessAll();

        // Set and apply VCO
        if (sweep->SweepToTuneButton.Pressed()) { // FILTER + TUNE
            vco->SetFreq(30.0f + 9000.0f * vco->TuneValue);
        } else { // FILTER
            vco->SetFreq(30.0f + 9000.0f * sweep->SweepValue);
        }
        output *= vco->Process();

        // Set and apply VCF low-pass filter
        vcf->SetFreq((20.0f + 12000.0f * sweep->SweepValue) / hw.AudioSampleRate()); // Must be normalized to sample rate
        output *= vcf->Process(output);
        
        // Apply volume
        output *= out_amp->VolumeValue();

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

    knobInit = new KnobInitializerDaisy;
    knobInit->InitKnobs();

    InitComponents();

    // hw.StartLog();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // Debounce all buttons
        triggers->DebounceAllButtons();
        sweep->Debounce();
        
        // Envelope Generator knobs
        decay_env->DecayValue = hw.adc.GetFloat(DecayKnob);
        sweep->SweepValue = hw.adc.GetFloat(SweepKnob);

        // LFO depth and rate knobs
        lfo->DepthValue = hw.adc.GetFloat(DepthKnob);
        lfo->RateValue  = hw.adc.GetFloat(RateKnob);

        // VCO tune knobs
        vco->TuneValue = hw.adc.GetFloat(TuneKnob);

        // OutAmp volume knob
        out_amp->VolumeValue = hw.adc.GetFloat(VolumeKnob);

        // hw.PrintLine("Volume: " FLT_FMT3,
        //              FLT_VAR3(hw.adc.GetFloat(VolumeKnob)));
        // hw.PrintLine("Decay: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(DecayKnob)));
        // hw.PrintLine("Depth: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(DepthKnob)));
        // hw.PrintLine("Tune: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(TuneKnob)));
        // hw.PrintLine("Sweep: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(SweepKnob)));
        // hw.PrintLine("Rate: " FLT_FMT3, FLT_VAR3(hw.adc.GetFloat(RateKnob)));
        // hw.PrintLine("\n");
        // System::Delay(1000);
    }
}
