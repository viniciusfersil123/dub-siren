#include "daisy_seed.h"
#include "daisysp.h"
#include "dub.h"

using namespace daisy;
using namespace daisysp;

// Init functions
void init_knobs()
{
    AdcChannelConfig my_adc_config[NUM_ADC_CHANNELS];
    my_adc_config[VolumeKnob].InitSingle(daisy::seed::A0);
    my_adc_config[DecayKnob].InitSingle(daisy::seed::A1);
    my_adc_config[DepthKnob].InitSingle(daisy::seed::A2);
    my_adc_config[TuneKnob].InitSingle(daisy::seed::A3);
    my_adc_config[SweepKnob].InitSingle(daisy::seed::A4);
    my_adc_config[RateKnob].InitSingle(daisy::seed::A5);
    my_adc_config[VcfKnob].InitSingle(daisy::seed::A6);
    hw.adc.Init(my_adc_config, NUM_ADC_CHANNELS);
    hw.adc.Start();
}

void init_components()
{
    triggers = new Triggers();
    env_gen = new EnvelopeGenerator();
    lfo = new Lfo();
    vco = new Vco();
    vcf = new Vcf();
}
// Init functions



// VCO functions
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



// Lfo functions
void Lfo::SetAmpAll(float amp)
{
    for (int i = 0; i < 4; i++)
        osc[i].SetAmp(amp);
}

void Lfo::SetFreqAll(float freq)
{
    for (int i = 0; i < 4; i++)
        osc[i].SetFreq(freq);
}

void Lfo::ResetPhaseAll()
{
    for (int i = 0; i < 4; i++)
        osc[i].Reset();
}

float Lfo::ProcessAll()
{
    // Process all 4 envelopes and store in value array
    for (int i = 0; i < 4; i++)
        value[i] = osc[i].Process();

    return value[env_gen->triggers.GetActiveEnvelope()];
}
// Lfo functions



// Triggers functions
void Triggers::DebounceAllButtons()
{
    for (int i = 0; i < 4; i++)
    {
        button[i].Debounce();
        if (button[i].RisingEdge())
        {
            activeEnvelopeIndex = i;
        }
        buttonState[i] = button[i].Pressed();
    }
}

const int Triggers::GetActiveEnvelopeIndex()
{
    return activeEnvelopeIndex;
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



// DecayEnvelope functions
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



// SweepToTuneButton functions
void SweepToTuneButton::Debounce()
{
    button.Debounce();
}

bool SweepToTuneButton::Pressed()
{
    return button.Pressed();
}
// SweepToTuneButton functions



// EnvelopeGenerator functions
float EnvelopeGenerator::Process()
{
    return (this->decay_env.Process() * lfo->ProcessAll());
}
// EnvelopeGenerator functions




// Main functions
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    float output;

    for(size_t i = 0; i < size; i++)
    {
        // Set Decay Envelope parameters
        env_gen->decay_env.SetDecayTime(env_gen->decay_env.DecayValue);

        // Set Decay Envelope trigger 
        if (env_gen->triggers.AnyButtonPressed()) {
            lfo->
            env_gen->decay_env.Retrigger();
        }

        // Set LFO parameters
        env_gen->envelopes.SetAmpAll(env_gen->envelopes.DepthValue);
        env_gen->envelopes.SetFreqAll(20 * env_gen->envelopes.RateValue);

        // Generate the LFO signal
        output = env_gen->Process();

        // Set and apply VCO
        if (env_gen->sweep_to_tune_button.Pressed()) { // FILTER + TUNE
            vco->SetFreq(30.0f + 9000.0f * vco->TuneValue);
        } else { // FILTER
            vco->SetFreq(30.0f + 9000.0f * env_gen->SweepValue);
        }
        output *= vco->Process();

        // Set and apply VCF low-pass filter
        vcf->SetFreq((20.0f + 12000.0f * env_gen->SweepValue) / hw.AudioSampleRate()); // Must be normalized to sample rate
        output = vcf->Process(output);
        
        // Apply volume
        output *= VolumeValue;

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

    init_knobs();
    init_components();

    hw.StartLog();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // Debounce all buttons
        env_gen->triggers.DebounceAllButtons();
        env_gen->sweep_to_tune_button.Debounce();
        
        // OutAmp volume knob
        VolumeValue = hw.adc.GetFloat(VolumeKnob);

        // VCO tune knobs
        vco->TuneValue = hw.adc.GetFloat(TuneKnob);

        // LFO depth and rate knobs
        env_gen->envelopes.DepthValue = hw.adc.GetFloat(DepthKnob);
        env_gen->envelopes.RateValue  = hw.adc.GetFloat(RateKnob);

        // Envelope Generator knobs
        env_gen->decay_env.DecayValue = hw.adc.GetFloat(DecayKnob);
        env_gen->SweepValue = hw.adc.GetFloat(SweepKnob);

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
