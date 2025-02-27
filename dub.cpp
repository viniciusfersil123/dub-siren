#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;


DaisySeed hw;

float VolumeValue;
float DecayValue;
float DepthValue;
float TuneValue;
float SweepValue;
float RateValue;
bool  lfoStates[4] = {false, false, false, false};
// Switch lfoButtons[4];
Oscillator vco;
Oscillator lfo;

OnePole vcf;
float VcfValue;

// Switch lfoButton1;


enum AdcChannel
{
    VolumeKnob,
    DecayKnob,
    DepthKnob,
    TuneKnob,
    SweepKnob,
    RateKnob,
    VcfKnob,
    NUM_ADC_CHANNELS
};

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

void init_vco()
{
    vco.Init(hw.AudioSampleRate());
    vco.SetWaveform(vco.WAVE_POLYBLEP_SAW);
}

void init_lfo()
{
    lfo.Init(hw.AudioSampleRate());
    lfo.SetWaveform(lfo.WAVE_SIN);
}

void init_vcf()
{
    vcf.Init();
    vcf.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    float output;

    for(size_t i = 0; i < size; i++)
    {
        lfo.SetAmp(DepthValue);
        lfo.SetFreq(20 * RateValue);

        vco.SetFreq(30.0f + 9000.0f * TuneValue + 9000.0f * lfo.Process());

        vcf.SetFrequency((20.0f + 12000.0f * VcfValue) / hw.AudioSampleRate());

        output = vco.Process();
        output = vcf.Process(output);
        output = VolumeValue * output;

        out[0][i] = output;
        out[1][i] = output;
    }
}

int main(void)
{
    hw.Init();
    init_knobs();
    init_vco();
    init_lfo();
    init_vcf();
    // lfoButton1.Init(hw.GetPin(21), 50);
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartLog();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // lfoButton1.Debounce();
        TuneValue   = hw.adc.GetFloat(TuneKnob);
        VolumeValue = hw.adc.GetFloat(VolumeKnob);

        DepthValue = hw.adc.GetFloat(DepthKnob);
        RateValue  = hw.adc.GetFloat(DecayKnob);
        // hw.SetLed(lfoButton1.Pressed());

        // vcf cutoff frequency helper knob
        VcfValue = hw.adc.GetFloat(VcfKnob);

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
