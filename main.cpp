#include "pico/stdlib.h"
#include "daisysp.h"

// Simple low-pass FIR coefficients (7-tap, tail-first order)
static const float kCoeffs[] = {0.05f, 0.1f, 0.2f, 0.3f, 0.2f, 0.1f, 0.05f};
static constexpr size_t kNumTaps = 7;
static constexpr size_t kBlockSize = 32;

int main() {
    daisysp::FIRFilterImplARM<kNumTaps, kBlockSize> fir;
    fir.SetIR(kCoeffs, kNumTaps, false);

    float input[kBlockSize] = {1.0f}; // impulse
    float output[kBlockSize] = {0.0f};
    fir.ProcessBlock(input, output, kBlockSize);

    while(true) {
    }
}
