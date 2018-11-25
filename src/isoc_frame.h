#pragma once

#include <stdint.h>

#define MAX_N_SAMPLES 112  // 56 * 2

struct __attribute__((__packed__)) isoc_frame {
    uint32_t number;  // timing info [0:3]
    uint16_t n_samples;
    int16_t samples[MAX_N_SAMPLES];  // audio data
    uint8_t button_state[2];         // button state
    uint8_t x_is_valid;
    uint8_t x;
    uint8_t y_is_valid;
    uint8_t y;
    uint8_t quad_encoder[3];
    uint8_t padding[17];

    isoc_frame() = default;
    isoc_frame(const isoc_frame& rhs) = default;
};
