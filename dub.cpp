#include "daisy_seed.h"
#include "daisysp.h"
#include "dub.h"

using namespace daisy;
using namespace daisysp;

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
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    float output;

    for(size_t i = 0; i < size; i++)
    {
        lfo->SetAmp(DepthValue);
        lfo->SetFreq(20 * RateValue);

        vco->SetFreq(30.0f + 9000.0f * TuneValue + 9000.0f * lfo->Process());

        vcf->SetFreq((20.0f + 12000.0f * vcf->value) / hw.AudioSampleRate()); // must be normalized to sample rate

        output = vco->Process();
        output = vcf->Process(output);
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
        
        // Volume knob
        VolumeValue = hw.adc.GetFloat(VolumeKnob);

        // VCO knobs
        TuneValue = hw.adc.GetFloat(TuneKnob);

        // LFO knobs
        DepthValue = hw.adc.GetFloat(DepthKnob);
        RateValue  = hw.adc.GetFloat(DecayKnob);
        // hw.SetLed(lfoButton1.Pressed());

        // VCF cutoff frequency knob
        vcf->value = hw.adc.GetFloat(VcfKnob);

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
