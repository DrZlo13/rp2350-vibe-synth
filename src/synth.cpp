#include "synth.h"

#include <cmath>

void Synth::init(int sample_rate) {
    osc_.Init(sample_rate);
    osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
    osc_.SetAmp(1.0f);

    lfo_.Init(sample_rate);
    lfo_.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    lfo_.SetFreq(0.4f);
    lfo_.SetAmp(1.0f);

    env_.Init(sample_rate);
    env_.SetAttackTime(0.01f);
    env_.SetDecayTime(0.1f);
    env_.SetSustainLevel(0.7f);
    env_.SetReleaseTime(0.3f);

    fenv_.Init(sample_rate);
    fenv_.SetAttackTime(0.005f);
    fenv_.SetDecayTime(0.4f);
    fenv_.SetSustainLevel(0.2f);
    fenv_.SetReleaseTime(0.5f);

    svf_.Init(sample_rate);
    svf_.SetRes(0.1f);
    svf_.SetDrive(0.0f);
}

void Synth::note_on(int note, int vel) {
    (void)vel;
    freq_ = daisysp::mtof(note);
    gate_ = true;
}

void Synth::note_off() {
    gate_ = false;
}

float Synth::process() {
    osc_.SetFreq(freq_);

    // LFO -1..1 -> pw 0.15..0.85 (avoid artifacts near the edges)
    float pw = 0.5f + lfo_.Process() * 0.35f;
    osc_.SetPw(pw);

    // fenv 0..1 -> cutoff 200..12000 Hz (exponential)
    float fenv_val = fenv_.Process(gate_);
    svf_.SetFreq(200.0f * powf(60.0f, fenv_val));

    float amplitude = env_.Process(gate_);
    svf_.Process(osc_.Process() * amplitude);
    return svf_.Low();
}
