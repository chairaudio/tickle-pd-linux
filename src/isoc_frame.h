#pragma once

#include <stdint.h>

#define N_SAMPLES_PER_MS 48

struct __attribute__((__packed__)) isoc_frame {
    uint32_t number;                    // timing info [0:3]
    uint16_t n_samples;                 // [4:5]
    int16_t samples[N_SAMPLES_PER_MS];  // audio data [6:101]
    uint8_t button_state[4];            // button state [102:105]
    uint8_t x_is_valid;                 // [106]
    uint8_t x;                          // [107]
    uint8_t y_is_valid;                 // [108]
    uint8_t y;                          // [109]
    uint8_t quad[3];                    // [110:112]
    uint8_t padding[15];                // [113:127]
};
