#pragma once

#include "daisysp.h"

class Synth {
public:
    void init(int sample_rate);

    void note_on(int note, int vel);
    void note_off();

    // Process one sample, returns value in -1..1
    float process();

private:
    daisysp::Oscillator osc_;
    daisysp::Oscillator lfo_;
    daisysp::Adsr       env_;
    daisysp::Adsr       fenv_; // filter envelope
    daisysp::Svf        svf_;

    bool  gate_  = false;
    float freq_  = 440.0f;
};
