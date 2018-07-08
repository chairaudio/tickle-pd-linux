#include "./device.h++"
using tickle::Device;

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fmt/format.h>

#include <chrono>
using namespace std::chrono_literals;

Device::Device() {}

Device::~Device() {}

void Device::set_connection(int device_fd) {
    _device_fd = device_fd;
    if (device_fd == -1) {
        _info = {};
    } else {
        _read_device_info();
    }
}

void Device::_read_device_info() {
    _info = {.serial = "", .version = ""};

    SmallBuffer buffer;
    auto result = ioctl(_device_fd, TICKLE_IOC_GET_DEVICE_SERIAL, &buffer);
    if (result == 0) {
        if (buffer.size != 0) {
            std::string_view serial_view(
                reinterpret_cast<char*>(&buffer.data[0]), buffer.size);
            _info.value().serial = std::string(serial_view);
        } else {
            // fmt::print("no device connected\n");
        }
    } else {
        // fmt::print("ioctl failed to get serial\n");
    }

    result = ioctl(_device_fd, TICKLE_IOC_GET_DEVICE_VERSION, &buffer);
    if (result == 0) {
        if (buffer.size != 0) {
            std::string_view version_view(
                reinterpret_cast<char*>(&buffer.data[0]), buffer.size);
            _info.value().version = std::string(version_view);
        } else {
            // fmt::print("no device connected\n");
        }
    } else {
        // fmt::print("ioctl failed to get serial\n");
    }
}

void Device::_copy_samples() {
    auto& buffer = _audio_buffer[_write_chunk % n_chunks];
    memcpy(buffer.data(), _frame.samples, samples_per_chunk * sizeof(int16_t));
    ++_write_chunk;
}

void Device::fill_audio_buffer(float* out, uint32_t n_samples) {
    if (not _audio_is_running) {
        // _logger.write("starting audio");
        _read_chunk = _write_chunk - n_chunks;
        _audio_is_running = true;
    }

    for (uint32_t sample_idx = 0; sample_idx < n_samples; sample_idx += 2) {
        if (_read_index % samples_per_chunk == 0) {
            ++_read_chunk;
        }
        auto& buffer = _read_chunk >= 0 ? _audio_buffer[_read_chunk % n_chunks]
                                        : _silence_buffer;
        float sample =
            static_cast<float>(buffer[_read_index % samples_per_chunk]) /
            32768.f;
        out[sample_idx] = out[sample_idx + 1] = sample;
        ++_read_index;
    }
}
