#include "./client.h++"
using tickle::Client;
#include "./device.h++"
using tickle::DeviceHandle;

using tickle::NormalizedPosition;
using tickle::Position;

#include <fmt/format.h>

Client::Client() {
    _ring_buffer.fill(0);
}

void Client::notify_device_was_connected(DeviceHandle device_handle) {
    _device_handle = device_handle;
}

void Client::notify_device_was_disconnected() {
    _device_handle = {};
}

void Client::copy_frame(const isoc_frame& frame) {
    _current_frame = frame;
    _copy_samples();
}

Client::FrameChanges Client::compare_frames() {
    FrameChanges changes;

    // TODO: lock current frame b4 copy
    isoc_frame current = _current_frame;

    bool was_down = _previous_frame.x_is_valid && _previous_frame.y_is_valid;
    bool is_down = current.x_is_valid && current.y_is_valid;
    if (was_down != is_down) {
        changes.touch = is_down;
    } else {
        changes.touch = {};
    }

    // there hase been an error in CapSense_GetCentroidPos
    if (current.x == 0 || current.y == 0) {
        current.x = _previous_frame.x;
        current.y = _previous_frame.y;
        changes.position = {};
    } else {
        if (current.x != _previous_frame.x || current.y != _previous_frame.y ||
            (changes.touch && is_down)) {
            changes.position = {current.x / 255.f, current.y / 255.f};
        } else {
            changes.position = {};
        }
    }

    if (changes.touch && not is_down) {
        changes.position = {-1.f, -1.f};
    }

    for (int8_t rotary_index = 0; rotary_index < int8_t(changes.rotary.size());
         ++rotary_index) {
        if (_previous_frame.quad_encoder[rotary_index] !=
            current.quad_encoder[rotary_index]) {
            auto clockwise = current.quad_encoder[rotary_index] >
                             _previous_frame.quad_encoder[rotary_index];
            if (abs(_previous_frame.quad_encoder[rotary_index] -
                    current.quad_encoder[rotary_index]) > 64) {
                clockwise = not clockwise;
            }
            changes.rotary[rotary_index] = {
                .index = rotary_index, .value = int8_t(clockwise ? -1 : 1)};
        } else {
            changes.rotary[rotary_index] = {};
        }
    }

    for (int8_t button_index = 0; button_index < int8_t(changes.buttons.size());
         ++button_index) {
        bool is_down = current.button_state[button_index];
        bool was_down = _previous_frame.button_state[button_index];
        if (is_down != was_down) {
            changes.buttons[button_index] = {.index = button_index,
                                             .value = is_down};
        } else {
            changes.buttons[button_index] = {};
        }
    }

    _previous_frame = current;

    return changes;
}

void Client::prepare_dsp(float sample_rate, int buffer_size) {
    _dsp_state = DSPState::kWillStart;
}

void Client::_copy_samples() {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }

    std::lock_guard guard{_frame_mutex};
    for (auto idx = 0; idx < _current_frame.n_samples; ++idx) {
        _ring_buffer[_write_index % _ring_size] = _current_frame.samples[idx];
        ++_write_index;
    }
}

void Client::fill_audio_buffer(float* out, uint32_t n_samples) {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }

    std::lock_guard guard{_frame_mutex};
    if (_dsp_state == DSPState::kWillStart) {
        _read_index = -_ring_size / 2;
        _write_index = 0;
        if (_device_handle) {
            _dsp_state = DSPState::kIsRunning;
        }
    }

    int32_t distance = _write_index - _read_index;
    int32_t target_distance = _ring_size / 2;

    float sample{0};
    bool needs_skip = distance < target_distance;
    for (uint32_t sample_idx = 0; sample_idx < n_samples; ++sample_idx) {
        if (_read_index >= 0) {
            sample =
                static_cast<float>(_ring_buffer[_read_index % _ring_size]) /
                32768.f;
        }
        out[sample_idx] = sample;

        // read less than required
        if (needs_skip && _read_index >= 0) {
            needs_skip = false;
        } else {
            ++_read_index;
        }
    }

    if (_read_index < 0) {
        return;
    }

    if (distance < 0 || distance > _ring_size) {
        if (_ring_size == MaxRingbufferCapacity) {
            return;
        }
        _ring_size += RingbufferChunkSize;
        _dsp_state = DSPState::kWillStart;
    }
}
