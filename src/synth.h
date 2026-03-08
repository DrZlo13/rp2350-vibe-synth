#pragma once

#include "daisysp.h"
#include <cmath>

template <int SAMPLE_RATE, int CTRL_DIV = 16>
class Synth {
public:
    static constexpr int CTRL_RATE = SAMPLE_RATE / CTRL_DIV;

    Synth() {
        osc_.Init(SAMPLE_RATE);
        osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
        osc_.SetAmp(1.0f);

        // LFO and envelopes run at control rate
        lfo_.Init(CTRL_RATE);
        lfo_.SetWaveform(daisysp::Oscillator::WAVE_TRI);
        lfo_.SetFreq(0.4f);
        lfo_.SetAmp(1.0f);

        env_.Init(CTRL_RATE);
        env_.SetAttackTime(0.01f);
        env_.SetDecayTime(0.1f);
        env_.SetSustainLevel(0.7f);
        env_.SetReleaseTime(0.3f);

        fenv_.Init(CTRL_RATE);
        fenv_.SetAttackTime(0.005f);
        fenv_.SetDecayTime(0.4f);
        fenv_.SetSustainLevel(0.2f);
        fenv_.SetReleaseTime(0.5f);

        svf_.Init(SAMPLE_RATE);
        svf_.SetRes(0.1f);
        svf_.SetDrive(0.0f);
    }

    void note_on(int note, int vel) {
        (void)vel;
        freq_ = daisysp::mtof(note);
        gate_ = true;
    }

    void note_off() {
        gate_ = false;
    }

    // Returns true when envelope has fully decayed (voice can be reused)
    bool is_silent() const {
        return amplitude_ < 0.001f;
    }

    // Process one sample, returns value in -1..1
    float process() {
        osc_.SetFreq(freq_);

        if(ctrl_counter_ == 0) {
            // LFO -1..1 -> pw 0.15..0.85 (avoid artifacts near the edges)
            pw_ = 0.5f + lfo_.Process() * 0.35f;

            // fenv 0..1 -> cutoff 200..12000 Hz (exponential)
            float fenv_val = fenv_.Process(gate_);
            svf_.SetFreq(200.0f * powf(60.0f, fenv_val));

            amplitude_ = env_.Process(gate_);
        }
        if(++ctrl_counter_ >= CTRL_DIV) ctrl_counter_ = 0;

        osc_.SetPw(pw_);
        svf_.Process(osc_.Process() * amplitude_);
        return svf_.Low();
    }

private:
    daisysp::Oscillator osc_;
    daisysp::Oscillator lfo_;
    daisysp::Adsr env_;
    daisysp::Adsr fenv_; // filter envelope
    daisysp::Svf svf_;

    bool gate_ = false;
    float freq_ = 440.0f;

    // Cached control-rate values
    int ctrl_counter_ = 0;
    float pw_ = 0.5f;
    float amplitude_ = 0.0f;
};
