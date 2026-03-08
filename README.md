# RP2350 Vibe Synth

An experiment in LLM-assisted embedded development. I define the architecture and hardware specs — Claude Code writes the implementation.

## Hardware

| Component | Details |
|-----------|---------|
| MCU | Raspberry Pi Pico 2 (RP2350) |
| DAC | PCM5102A (I2S) |
| Interface | USB MIDI (TinyUSB) |

**PCM5102A wiring:**

| PCM5102A | RP2350 |
|----------|--------|
| DIN | GP2 |
| BCK | GP3 |
| LCK | GP4 |
| VCC | 3.3V |
| GND | GND |
| SCK | GND (internal PLL) |
| FMT | GND (I2S) |
| XSMT | 3.3V (unmute) |

## Software Stack

- **Pico SDK 2.2.0** — hardware abstraction, USB, I2S via pico-extras
- **DaisySP** — audio DSP (oscillators, filters, effects)
- **CMSIS-DSP** — ARM-optimized math (via Cortex-M33 on RP2350)
- **TinyUSB** — USB MIDI device class

## Debugging / RTT Logging

The firmware uses **SEGGER RTT** (Real-Time Transfer) for logging. Output goes over the debug probe — no UART needed.

### Viewing logs with OpenOCD + telnet

```bash
# Start OpenOCD (Debug Probe / picoprobe)
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg

# In another terminal — connect and read RTT
telnet localhost 4444
> rtt setup 0x20000000 0x40000 "SEGGER RTT"
> rtt start
> rtt channels
```

## Building

Requires [Pico SDK](https://github.com/raspberrypi/pico-sdk) and [pico-extras](https://github.com/raspberrypi/pico-extras).

```bash
mkdir build && cd build
cmake ..
make
```

Flash `2350-vibe-synth.uf2` to the board in BOOTSEL mode.
