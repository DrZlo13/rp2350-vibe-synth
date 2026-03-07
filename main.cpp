#include "pico/stdlib.h"
#include "tusb.h"
#include "daisysp.h"
#include "pico/audio_i2s.h"

#define SAMPLE_RATE   44100
#define BUFFER_FRAMES 256

// PCM5102A setup:
//   SCK - solder bridge close (SCK to GND)
//   H1L (FLT)  = L
//   H2L (DEMP) = L
//   H3L (XSMT) = H
//   H4L (FMT)  = L
#define I2S_PIN_DATA 2
#define I2S_PIN_CLK  3 // BCLK=3, LRCLK=4

static daisysp::Oscillator osc;
static daisysp::Oscillator lfo;
static daisysp::Adsr env;
static daisysp::Adsr fenv; // filter envelope
static daisysp::Svf svf;
static volatile bool gate = false;
static volatile float osc_freq = 440.0f;

static inline float midi_note_to_freq(uint8_t note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

static audio_buffer_pool_t* audio_init() {
    static audio_format_t fmt = {
        .sample_freq = SAMPLE_RATE,
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .channel_count = 2,
    };
    static audio_buffer_format_t producer_fmt = {
        .format = &fmt,
        .sample_stride = 4, // 2 ch * 2 bytes
    };
    audio_buffer_pool_t* pool = audio_new_producer_pool(&producer_fmt, 3, BUFFER_FRAMES);

    static const audio_i2s_config_t i2s_cfg = {
        .data_pin = I2S_PIN_DATA,
        .clock_pin_base = I2S_PIN_CLK,
        .dma_channel = 0,
        .pio_sm = 0,
    };
    audio_i2s_setup(&fmt, &i2s_cfg);
    audio_i2s_connect(pool);
    audio_i2s_set_enabled(true);
    return pool;
}

void tud_midi_rx_cb(uint8_t itf) {
    (void)itf;
    uint8_t packet[4];
    while(tud_midi_available()) {
        tud_midi_packet_read(packet);
        uint8_t status = packet[1] & 0xF0;
        uint8_t note = packet[2];
        uint8_t vel = packet[3];

        if(status == 0x90 && vel > 0) { // Note On
            osc_freq = midi_note_to_freq(note);
            gate = true;
        } else if(status == 0x80 || (status == 0x90 && vel == 0)) { // Note Off
            gate = false;
        }
    }
}

int main() {
    tusb_init();

    osc.Init(SAMPLE_RATE);
    osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
    osc.SetAmp(1.0f);

    lfo.Init(SAMPLE_RATE);
    lfo.SetWaveform(daisysp::Oscillator::WAVE_TRI);
    lfo.SetFreq(0.4f); // Hz
    lfo.SetAmp(1.0f);

    env.Init(SAMPLE_RATE);
    env.SetAttackTime(0.01f);
    env.SetDecayTime(0.1f);
    env.SetSustainLevel(0.7f);
    env.SetReleaseTime(0.3f);

    fenv.Init(SAMPLE_RATE);
    fenv.SetAttackTime(0.005f);
    fenv.SetDecayTime(0.4f);
    fenv.SetSustainLevel(0.2f);
    fenv.SetReleaseTime(0.5f);

    svf.Init(SAMPLE_RATE);
    svf.SetRes(0.1f);
    svf.SetDrive(0.0f);

    audio_buffer_pool_t* pool = audio_init();

    while(true) {
        tud_task();

        audio_buffer_t* buf = take_audio_buffer(pool, false);
        if(!buf) continue;

        osc.SetFreq(osc_freq);
        bool g = gate;
        int16_t* samples = (int16_t*)buf->buffer->bytes;
        for(uint32_t i = 0; i < buf->max_sample_count; i++) {
            // LFO -1..1 → pw 0.15..0.85 (не доходим до краёв чтобы не было артефактов)
            float pw = 0.5f + lfo.Process() * 0.35f;
            osc.SetPw(pw);

            float fenv_val = fenv.Process(g);
            // fenv 0..1 → cutoff 200..12000 Hz (экспоненциально)
            float cutoff = 200.0f * powf(60.0f, fenv_val);
            svf.SetFreq(cutoff);

            float amplitude = env.Process(g);
            svf.Process(osc.Process() * amplitude);
            int16_t val = (int16_t)(svf.Low() * 32767.0f);
            samples[i * 2 + 0] = val; // L
            samples[i * 2 + 1] = val; // R
        }
        buf->sample_count = buf->max_sample_count;
        give_audio_buffer(pool, buf);
    }
}
