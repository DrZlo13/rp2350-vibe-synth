#pragma once

#include "synth.h"

template <int N, int SAMPLE_RATE, int CTRL_DIV = 16>
class VoiceManager {
public:
    void note_on(int note, int vel) {
        int i = alloc_voice(note);
        states_[i] = {note, true, ++age_counter_};
        voices_[i].note_on(note, vel);
    }

    void note_off(int note) {
        for(int i = 0; i < N; i++) {
            if(states_[i].note == note && states_[i].gate) {
                states_[i].gate = false;
                voices_[i].note_off();
            }
        }
    }

    // Mix all voices, returns value in -1..1
    float process() {
        float mix = 0.0f;
        for(auto& v : voices_)
            mix += v.process();
        return mix * (1.0f / N);
    }

private:
    struct VoiceState {
        int note = -1; // assigned MIDI note (-1 = free)
        bool gate = false;
        int age = 0; // lower = older, used for stealing
    };

    using SynthVoice = Synth<SAMPLE_RATE, CTRL_DIV>;

    SynthVoice voices_[N];
    VoiceState states_[N];
    int age_counter_ = 0;

    // Priority: retrigger same note > silent > oldest releasing > oldest active.
    int alloc_voice(int note) {
        // Retrigger: same note already playing
        for(int i = 0; i < N; i++)
            if(states_[i].note == note && states_[i].gate) return i;

        // Prefer a fully silent voice
        for(int i = 0; i < N; i++)
            if(voices_[i].is_silent()) return i;

        // Steal oldest releasing voice (gate off but still decaying)
        int best = -1;
        for(int i = 0; i < N; i++)
            if(!states_[i].gate)
                if(best == -1 || states_[i].age < states_[best].age) best = i;
        if(best != -1) return best;

        // Last resort: steal oldest active voice
        best = 0;
        for(int i = 1; i < N; i++)
            if(states_[i].age < states_[best].age) best = i;
        return best;
    }
};
