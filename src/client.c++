#include "./client.h++"
using tickle::Client;
#include "./device.h++"
using tickle::DeviceHandle;

using tickle::NormalizedPosition;
using tickle::Position;

Client::Client() {
    memset(_silence_buffer.data(), 0, samples_per_chunk * 2);
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

    if (current.x != _previous_frame.x || current.y != _previous_frame.y ||
        changes.touch) {
        // override with previous position?
        if (current.x == 255 || current.y == 255) {
            current.x = _previous_frame.x;
            current.y = _previous_frame.y;
        }
        changes.position = {current.x / 255.f, current.y / 255.f};
    } else {
        changes.position = {};
    }

    for (int8_t rotary_index = 0; rotary_index < int8_t(changes.rotary.size());
         ++rotary_index) {
        if (_previous_frame.quad[rotary_index] != current.quad[rotary_index]) {
            auto clockwise =
                current.quad[rotary_index] > _previous_frame.quad[rotary_index];
            if (abs(_previous_frame.quad[rotary_index] -
                    current.quad[rotary_index]) > 64) {
                clockwise = not clockwise;
            }
            changes.rotary[rotary_index] = {
                .index = rotary_index, .value = int8_t(clockwise ? 1 : -1)};
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

void Client::prepare_dsp(float sample_rate, int buffer_size)
{
    fmt::print("{} {} {}\n", __PRETTY_FUNCTION__, sample_rate, buffer_size);
    _dsp_state = DSPState::kWillStart;
}

void Client::_copy_samples()
{
    if (_dsp_state == DSPState::kUndefined) {
        return;
    }
    auto& buffer = _audio_buffer[_write_chunk % n_chunks];
    memcpy(buffer.data(), _current_frame.samples, samples_per_chunk * sizeof(int16_t));
    ++_write_chunk;    
    // fmt::print("{} {}\n", __PRETTY_FUNCTION__, _current_frame.n_samples);
}

void Client::fill_audio_buffer(float* out, uint32_t n_samples) {

    if (_dsp_state == DSPState::kUndefined) {
        return;
    }
    
    if (_dsp_state == DSPState::kWillStart) {
        _write_chunk = 0;
        _read_chunk = _write_chunk - n_chunks;
        _dsp_state = DSPState::kIsRunning;
    }

    // fmt::print("{} {}\n", __PRETTY_FUNCTION__, n_samples);
    

    
    for (uint32_t sample_idx = 0; sample_idx < n_samples; sample_idx += 2) {
        if (_read_index % samples_per_chunk == 0) {
            ++_read_chunk;
        }
        auto& buffer = (_read_chunk >= 0 && _device_handle) ? _audio_buffer[_read_chunk % n_chunks]
                                        : _silence_buffer;

        float sample = static_cast<float>(buffer[_read_index % samples_per_chunk]) / 32768.f;
        out[sample_idx] = out[sample_idx + 1] = sample;
        ++_read_index;
    }    
}






