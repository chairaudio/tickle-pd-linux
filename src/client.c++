#include "./client.h++"
using tickle::Client;
#include "./device.h++"
using tickle::DeviceHandle;

using tickle::NormalizedPosition;
using tickle::Position;

#include <iostream>

Client::Client() {
    /*
    memset(_silence_buffer.samples.data(), 0, samples_per_chunk * 2);
    _silence_buffer.n_valid_samples = samples_per_chunk;
    
    uint16_t chunk_count = 0;
    for (auto& chunk : _audio_buffer) {
        chunk.chunk_id = chunk_count++;
    }*/
    
    memset(_ring_buffer.data(), 0, _ring_buffer.size() * 2);
}

void Client::notify_device_was_connected(DeviceHandle device_handle) {
    _device_handle = device_handle;
    fmt::print("{}\n", __PRETTY_FUNCTION__);
}

void Client::notify_device_was_disconnected() {
    _device_handle = {};
    fmt::print("{}\n", __PRETTY_FUNCTION__);
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

    // workaround
    // override with previous position?
    if (current.x == 255 || current.y == 255) {
        current.x = _previous_frame.x;
        current.y = _previous_frame.y;
    }
    if (current.x != _previous_frame.x || current.y != _previous_frame.y ||
        (changes.touch && is_down)) {
        changes.position = {current.x / 255.f, current.y / 255.f};
    } else {
        changes.position = {};
    }

    for (int8_t rotary_index = 0; rotary_index < int8_t(changes.rotary.size());
         ++rotary_index) {
        if (_previous_frame.quad_encoder[rotary_index] != current.quad_encoder[rotary_index]) {
            auto clockwise =
                current.quad_encoder[rotary_index] > _previous_frame.quad_encoder[rotary_index];
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
    fmt::print("{} {} {}\n", __PRETTY_FUNCTION__, sample_rate, buffer_size);
    _dsp_state = DSPState::kWillStart;
}

std::mutex m;
void Client::_copy_samples() {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }
    
    std::lock_guard guard {m};
    // fmt::print("{} {}\n", __PRETTY_FUNCTION__, _current_frame.n_samples);
    for (auto idx = 0; idx < _current_frame.n_samples; ++idx) {
        // _write_index_abs += buffer.n_valid_samples;
        _ring_buffer[_write_index % _ring_size] = _current_frame.samples[idx];
        ++_write_index;
    }
}

void Client::fill_audio_buffer(float* out, uint32_t n_samples) {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }

    std::lock_guard guard {m};
    if (_dsp_state == DSPState::kWillStart) {
        _read_index = -_ring_size / 2;
        _write_index = 0;
        std::cout << "resetting ring_size " << _ring_size << std::endl;
        if (_device_handle) {
            _dsp_state = DSPState::kIsRunning;
        }
    }

 
    int32_t distance = _write_index - _read_index;
    int32_t target_distance = _ring_size / 2;
    // _skip = d > (n_chunks * samples_per_chunk / 2); 
    std::cout << distance << " : " << target_distance << std::endl;
    
    float sample {0};
    bool needs_skip = distance < target_distance;
    for (uint32_t sample_idx = 0; sample_idx < n_samples; ++sample_idx) {
        if (_read_index >= 0) {
            sample = static_cast<float>(_ring_buffer[_read_index % _ring_size]) / 32768.f;            
        }
        out[sample_idx] = sample;
        
        // read less than required
        if (needs_skip && _read_index >= 0) {
            needs_skip = false;
        } else {
            ++_read_index;
        }
    }

    if (distance < 0 || distance > _ring_size) {
        _ring_size += 96;
        // std::cout << "new ring size: " << _ring_size << std::endl;
        _dsp_state = DSPState::kWillStart;
    }
    
    
    // if (_read_index_abs > _write_index_abs) {
    //     std::cout << "should never happen" << std::endl;
    // }
    /*
    std::cout << frame_idx << " " << _write_index_abs << " " << _read_index_abs << " " << d << " " << _skip << "\n";
    std::cout << _write_chunk << "/" << _read_chunk << std::endl
                << _write_chunk % n_chunks << "/" << _read_chunk % n_chunks << std::endl;
                */
                // << "/" << n_samples_in_buffer << " " << flows << std::endl;
}
