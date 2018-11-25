#include "./client.h++"
using tickle::Client;
#include "./device.h++"
using tickle::DeviceHandle;

using tickle::NormalizedPosition;
using tickle::Position;

#include <fmt/format.h>
#include <iostream>
using std::cout;

Client::Client() {
    _ring_buffer.fill(0);
}

void Client::run_tests() {
    _verify_read_idx_record_buffer();
}

void Client::_verify_read_idx_record_buffer() {
    cout << "recorded " << _read_idx_record_buffer_idx << " samples\n";
    for (auto number : _read_idx_record_buffer) {
        cout << number << "\n";
    }
    cout << "recorded " << _read_idx_record_buffer_idx << " samples\n";
}

void Client::notify_device_was_connected(DeviceHandle device_handle) {
    _device_handle = device_handle;
}

void Client::notify_device_was_disconnected() {
    _device_handle = {};
}

void Client::copy_frame(const isoc_frame& frame) {
    std::lock_guard frame_guard{_frame_mutex};
    _current_frame = frame;
    _copy_samples();
}

Client::FrameChanges Client::compare_frames() {
    FrameChanges changes;

    isoc_frame current = _current_frame;

    bool was_down = _previous_frame.x_is_valid && _previous_frame.y_is_valid;
    bool is_down = current.x_is_valid && current.y_is_valid;
    if (was_down != is_down) {
        changes.touch = {};  // is_down;
    } else {
        changes.touch = {};
    }

    changes.position = {};

    // there has been an error in CapSense_GetCentroidPos
    if (current.x == 0 || current.y == 0) {
        if (is_down) {
            current.x = _previous_frame.x;
            current.y = _previous_frame.y;
        } else {
            if (was_down) {
                changes.position = {-1.f, -1.f};
            }
        }
    } else {
        if (current.x != _previous_frame.x || current.y != _previous_frame.y) {
            if (is_down) {
                changes.position = {current.x / 255.f, current.y / 255.f};
            }
        } else {
            changes.position = {};
        }
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
    bool override_change = _previous_changes.position.has_value() &&
                           _previous_changes.position.value().x == -1 &&
                           not changes.position.has_value();
    _previous_changes = changes;
    if (override_change) {
        _previous_changes.position = {-1, -1};
        _previous_frame.x_is_valid = false;
    }
    return changes;
}

void Client::prepare_dsp(float sample_rate, int buffer_size) {
    _dsp_state = DSPState::kWillStart;
}

void Client::_copy_samples() {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }

    //     std::lock_guard guard{_frame_mutex};
    auto n_samples = _current_frame.n_samples;
    if (_skip_write && n_samples > RingbufferChunkSize) {
        n_samples = RingbufferChunkSize;
        // cout << "_skip_write " << _rw_distance << "\n";
    }
    for (auto idx = 0; idx < n_samples; ++idx) {
        _ring_buffer[_write_index % _ring_size] = _current_frame.samples[idx];
        ++_write_index;
    }
}

void Client::set_n_ring_chunks(uint16_t n_chunks) {
    if (n_chunks > MaxRingbufferChunks || n_chunks == 0) {
        return;
    }

    std::lock_guard guard{_frame_mutex};
    _ring_size = n_chunks * RingbufferChunkSize;
    _dsp_state = DSPState::kWillStart;
}

void Client::fill_audio_buffer(float* out, uint32_t n_samples) {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }

    std::lock_guard frame_guard{_frame_mutex};
    if (_dsp_state == DSPState::kWillStart) {
        _read_index = -_ring_size / 2;
        _write_index = 0;
        if (_device_handle) {
            _dsp_state = DSPState::kIsRunning;
        }
    }

    _rw_distance = _write_index - _read_index;
    int32_t max_distance = _ring_size * 3 / 4;
    _skip_write = _rw_distance > max_distance;
    if (_skip_write) {
        // cout << distance << " : " << target_distance << "\n";
    }

    int32_t min_distance = _ring_size * 1 / 4;
    _skip_read = _rw_distance < min_distance;
    if (_skip_read) {
        cout << "_skip_read " << _rw_distance << "\n";
        if (_rw_distance < 0) {
            cout << "_skip_read ouch\n";
        }
    }

    float sample{0};
    for (uint32_t sample_idx = 0; sample_idx < n_samples; ++sample_idx) {
        if (_read_index >= 0) {
            sample =
                static_cast<float>(_ring_buffer[_read_index % _ring_size]) /
                32768.f;
        }
        out[sample_idx] = sample;
        if (_skip_read) {
            _skip_read = false;
            continue;
        }
        ++_read_index;
    }

    /*
        if (_read_index < 0) {
            return;
        }


        if (distance < 0 || distance > _ring_size) {
            if (_ring_size == MaxRingbufferCapacity) {
                return;
            }
            _ring_size += RingbufferChunkSize;
            _dsp_state = DSPState::kWillStart;
        } */
}
