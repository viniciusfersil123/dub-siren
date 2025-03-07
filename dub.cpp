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
    vco = new Vco(hw.AudioSampleRate());
    lfo = new Lfo(hw.AudioSampleRate());
    vcf = new Vcf();
    env_gen = new EnvelopeGenerator();
    decay_env = new DecayEnvelope();
}
// Init functions



// Envelopes functions
void Envelopes::SetAmpAll(float amp)
{
    for (int i = 0; i < 4; i++)
        osc[i].SetAmp(amp);
}

void Envelopes::SetFreqAll(float freq)
{
    for (int i = 0; i < 4; i++)
        osc[i].SetFreq(freq);
}

float Envelopes::ProcessAll()
{
    // Process all 4 envelopes and store in value array
    for (int i = 0; i < 4; i++)
    {
        value[i] = osc[i].Process();
    }

    return value[env_gen->triggers.GetActiveEnvelope()];
}
// Envelopes functions



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



// EnvelopeGenerator functions
float EnvelopeGenerator::Process()
{
    return this->envelopes.ProcessAll();
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
        // lfo->SetAmp(DepthValue);
        // lfo->SetFreq(20 * RateValue);

        env_gen->envelopes.SetAmpAll(DepthValue);
        env_gen->envelopes.SetFreqAll(20 * RateValue);

        // vco->SetFreq(30.0f + 9000.0f * TuneValue + 9000.0f * lfo->Process());

        vco->SetFreq(30.0f + 9000.0f * TuneValue + 9000.0f * env_gen->Process());

        vcf->SetFreq((20.0f + 12000.0f * vcf->value) / hw.AudioSampleRate()); // Must be normalized to sample rate

        env_gen->SetDecay(DecayValue);

        // Generate sound from VCO
        output = vco->Process();

        // Apply VCF low pass filter 
        output = vcf->Process(output);
        
        // Apply decay envelope
        if (env_gen->triggers.AnyButtonPressed())
            decay_env->Retrigger();
        output = decay_env->Process() * output;

        // Apply volume
        output = VolumeValue * output;

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
        // lfoButton1.Debounce();
        env_gen->triggers.DebounceAllButtons();
        
        // Volume knob
        VolumeValue = hw.adc.GetFloat(VolumeKnob);

        // VCO knobs
        TuneValue = hw.adc.GetFloat(TuneKnob);

        // LFO knobs
        DepthValue = hw.adc.GetFloat(DepthKnob);
        RateValue  = hw.adc.GetFloat(RateKnob);
        // hw.SetLed(lfoButton1.Pressed());

        // VCF cutoff frequency knob
        vcf->value = hw.adc.GetFloat(VcfKnob);

        // Envelope Generator decay knob
        DecayValue = hw.adc.GetFloat(DecayKnob);

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
