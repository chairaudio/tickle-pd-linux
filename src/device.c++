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
