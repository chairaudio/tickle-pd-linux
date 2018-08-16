#include "./client.h++"
using tickle::Client;
#include "./device.h++"
using tickle::DeviceHandle;

using tickle::NormalizedPosition;
using tickle::Position;

#include <iostream>

Client::Client() {
    memset(_silence_buffer.samples.data(), 0, samples_per_chunk * 2);
    _silence_buffer.n_valid_samples = samples_per_chunk;
    
    uint16_t chunk_count = 0;
    for (auto& chunk : _audio_buffer) {
        chunk.chunk_id = chunk_count++;
    }
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
    auto& buffer = _audio_buffer[_write_chunk % n_chunks];
    /*
    if (_current_frame.n_samples < 24
        || _current_frame.n_samples > 25) 
    {
        fmt::print("{} {}\n", __PRETTY_FUNCTION__, _current_frame.n_samples);
        // reject the buffer, something went wrong
        return;
    } */
    // fmt::print("{} {}\n", __PRETTY_FUNCTION__, _current_frame.n_samples);
    
    if (_current_frame.n_samples > buffer.samples.size()) {
        fmt::print("{}: too many samples, got {}\n", __PRETTY_FUNCTION__, _current_frame.n_samples);
        return;
    }
    
    memcpy(buffer.samples.data(), _current_frame.samples,
           _current_frame.n_samples * sizeof(int16_t));
    buffer.n_valid_samples = _skip ? samples_per_chunk : _current_frame.n_samples;
    // fmt::print("{} {} {}\n", buffer.n_valid_samples, _current_frame.n_samples, samples_per_chunk);
    // buffer.n_valid_samples = samples_per_chunk;    
    ++_write_chunk;    
    _write_index_abs += buffer.n_valid_samples;

    /*
    static bool zero = true;
    std::fill(buffer.samples.begin(), buffer.samples.end(), zero ? 0 : 6789);
    zero = not zero;
    */
    // fmt::print("fn: {} {} {} : {} -> {}\n", _current_frame.number, _current_frame.n_samples, zero, _write_chunk, _write_chunk % n_chunks);
}

void Client::fill_audio_buffer(float* out, uint32_t n_samples) {
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }

    if (_dsp_state == DSPState::kWillStart) {
        _write_chunk = 0;//n_chunks / 2;
        _read_chunk = - ((n_chunks/4)*3);// / 2);
        _read_index = 0;
        _read_index_abs = _read_chunk * samples_per_chunk;
        _write_index_abs = 0;//_write_chunk * samples_per_chunk;
        if (_device_handle) {
            _dsp_state = DSPState::kIsRunning;
        }
    }

    std::lock_guard guard {m};
    
    static uint32_t frame_idx = 0;
    ++frame_idx;
    
    float sample {0};
    int32_t n_samples_in_buffer = samples_per_chunk;
    int32_t flows = 0;
    for (uint32_t sample_idx = 0; sample_idx < n_samples; ++sample_idx) {
        if (_read_chunk >= 0) {            
            auto& buffer = _audio_buffer[_read_chunk % n_chunks];    
            sample = static_cast<float>(buffer.samples[_read_index]) / 32768.f;
            n_samples_in_buffer = buffer.n_valid_samples;
            
            /*
            if (sample_idx == 0) {
                fmt::print("fab: {} {} * {} -> {}\n", n_samples_in_buffer, buffer.chunk_id, _read_chunk, _read_chunk % n_chunks);
            }  */           
        }
        
        /*
        if (frame_idx % 2) {
            sample = 0.5;
        } else {
            sample = 0.0;
        } */
        
        out[sample_idx] = sample;
        ++_read_index;
        
        if (_read_index >= n_samples_in_buffer) {
            ++_read_chunk;
            _read_index = 0;
            ++flows;
        }
        _read_index_abs++;
    }
 
    int32_t d = _write_index_abs - _read_index_abs;
    _skip = d > (n_chunks * samples_per_chunk / 2); 
    
    
    
    // if (_read_index_abs > _write_index_abs) {
    //     std::cout << "should never happen" << std::endl;
    // }
            
    std::cout << frame_idx << " " << _write_index_abs << " " << _read_index_abs << " " << d << " " << _skip << "\n";
    std::cout << _write_chunk << "/" << _read_chunk << std::endl
                << _write_chunk % n_chunks << "/" << _read_chunk % n_chunks << std::endl;
                // << "/" << n_samples_in_buffer << " " << flows << std::endl;
}
