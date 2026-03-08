#include "pico/stdlib.h"
#include "tusb.h"
#include "pico/audio_i2s.h"
#include "SEGGER_RTT.h"

#include "voice_manager.h"

#define LOG(...) SEGGER_RTT_printf(0, __VA_ARGS__)
// RTT printf does not support %f — split float into integer and fractional parts
#define FI(f)    ((int)(f))
#define FF1(f)   ((int)(((f) - (int)(f)) * 10))

#define SAMPLE_RATE   44100
#define BUFFER_FRAMES 256
#define VOICES        8

// PCM5102A setup:
//   SCK - solder bridge close (SCK to GND)
//   H1L (FLT)  = L
//   H2L (DEMP) = L
//   H3L (XSMT) = H
//   H4L (FMT)  = L
#define I2S_PIN_DATA 2
#define I2S_PIN_CLK  3 // BCLK=3, LRCLK=4

static VoiceManager<VOICES, SAMPLE_RATE> voice_manager;

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
            voice_manager.note_on(note, vel);
        } else if(status == 0x80 || (status == 0x90 && vel == 0)) { // Note Off
            voice_manager.note_off(note);
        }
    }
}

int main() {
    tusb_init();
    audio_buffer_pool_t* pool = audio_init();

    LOG("=== 2350 Vibe Synth ===\n");
    LOG("Sample rate : %d Hz\n", SAMPLE_RATE);
    LOG("Buffer size : %d frames\n", BUFFER_FRAMES);
    LOG("Osc         : POLYBLEP_SQUARE\n");
    LOG("LFO         : TRI %d.%d Hz -> PW\n", FI(0.4f), FF1(0.4f));
    LOG("Env A/D/S/R : %dms / %dms / %d%% / %dms\n", FI(10.0f), FI(100.0f), FI(70.0f), FI(300.0f));
    LOG("FEnv A/D/S/R: %dms / %dms / %d%% / %dms\n", FI(5.0f), FI(400.0f), FI(20.0f), FI(500.0f));
    LOG("Filter      : SVF lowpass, res=0.1, cutoff 200..12000 Hz\n");
    LOG("I2S         : DATA=GP%d, BCLK=GP%d, LRCLK=GP%d\n", I2S_PIN_DATA, I2S_PIN_CLK, I2S_PIN_CLK + 1);
    LOG("Ready. Waiting for MIDI...\n\n");

    // Time budget per buffer in microseconds
    static const uint32_t BUDGET_US = BUFFER_FRAMES * 1000000u / SAMPLE_RATE;
    // Number of buffers per log interval (~1 sec)
    static const uint32_t LOG_INTERVAL = SAMPLE_RATE / BUFFER_FRAMES;

    uint32_t buf_count = 0;
    uint64_t load_acc_us = 0; // accumulated DSP time over the log interval

    while(true) {
        tud_task();

        audio_buffer_t* buf = take_audio_buffer(pool, false);
        if(!buf) continue;

        uint64_t t0 = time_us_64();

        int16_t* samples = (int16_t*)buf->buffer->bytes;
        for(uint32_t i = 0; i < buf->max_sample_count; i++) {
            int16_t val = (int16_t)(voice_manager.process() * 32767.0f);
            samples[i * 2 + 0] = val; // L
            samples[i * 2 + 1] = val; // R
        }

        load_acc_us += time_us_64() - t0;

        buf->sample_count = buf->max_sample_count;
        give_audio_buffer(pool, buf);

        if(++buf_count >= LOG_INTERVAL) {
            // average DSP load over the interval in percent
            uint32_t avg_us = (uint32_t)(load_acc_us / LOG_INTERVAL);
            uint32_t load_pct = avg_us * 100 / BUDGET_US;
            LOG("CPU load: %d%% (avg %d us / budget %d us)\n", load_pct, avg_us, BUDGET_US);
            buf_count = 0;
            load_acc_us = 0;
        }
    }
}
