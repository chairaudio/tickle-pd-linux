#include "./shared_device_manager.h++"
using tickle::SharedDeviceManager;

#include "./device.h++"
using tickle::Device;
using tickle::DeviceHandle;

#include "./client.h++"
using tickle::Client;
using tickle::ClientHandle;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#include <chrono>
using namespace std::chrono_literals;
#include <experimental/vector>
#include <iostream>
using std::cout;

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

SharedDeviceManager::SharedDeviceManager() {
    _device = std::make_unique<Device>();
    _device_thread = std::thread(&SharedDeviceManager::_spawn, this);
}

SharedDeviceManager::~SharedDeviceManager() {
    if (_keep_running) {
        _keep_running = false;
        _device_thread.join();
    }
}

void SharedDeviceManager::_spawn() {
    _keep_running = true;

    do {
        _connect_to_kmod();
        std::this_thread::sleep_for(1s);
    } while (_keep_running && _kmod_fd == -1);

    if (not _keep_running) {
        return;
    }

    isoc_frame previous_frame{.number = 0};
    bool did_post_error{false};
    while (_keep_running) {
        isoc_frame current_frame;
        auto result = ioctl(_kmod_fd, TICKLE_IOC_READ_FRAME, &current_frame);
        if (result != 0) {
            cout << "err: TICKLE_IOC_READ_FRAME failed with result " << result
                 << "\n";
            if (_device->is_connected()) {
                _on_device_disconnection();
                did_post_error = false;
            }
            if (not did_post_error) {
                did_post_error = true;
                error("please connect your tickle device");
            }
            std::this_thread::sleep_for(1s);
            continue;
        } else {
            if (not _device->is_connected()) {
                _on_device_connection();
                // continue;
            }
        }

        if (current_frame.number == previous_frame.number) {
            continue;
        }

        for (auto& client : _clients) {
            client->copy_frame(current_frame);
        }
        previous_frame = current_frame;
    }
}

void SharedDeviceManager::_on_device_connection() {
    _device->set_connection(_kmod_fd);
    for (auto& client : _clients) {
        client->notify_device_was_connected(DeviceHandle{_device.get()});
    }
}

void SharedDeviceManager::_on_device_disconnection() {
    _device->set_connection(-1);
    for (auto& client : _clients) {
        client->notify_device_was_disconnected();
    }
}

void SharedDeviceManager::_connect_to_kmod() {
    fs::path tickle_character_device{"/dev/tickle"};

    if (not fs::exists(tickle_character_device)) {
        static bool did_post_error{false};
        cout << tickle_character_device << " does not exist\n";
        if (not did_post_error) {
            did_post_error = true;
            error("tickle kernel module is not loaded");
            error(
                "download and install hardware driver from "
                "https://chair.audio");
        }
        return;
    }

    _kmod_fd = open(tickle_character_device.c_str(), O_RDWR);
    if (_kmod_fd == -1) {
        cout << "cannot open " << tickle_character_device << "\n";
        return;
    }

    SmallBuffer buffer;
    ioctl(_kmod_fd, TICKLE_IOC_GET_KERNEL_MODULE_VERSION, &buffer);
    std::string_view version_view(reinterpret_cast<char*>(&buffer.data[0]),
                                  buffer.size);
    _kernel_module_version = std::string(version_view);
}

ClientHandle SharedDeviceManager::create_client() {
    _clients.push_back(std::make_unique<Client>());
    auto handle = ClientHandle{_clients.back().get()};
    if (_device->is_connected()) {
        handle->notify_device_was_connected(DeviceHandle{_device.get()});
    }
    return handle;
}

void SharedDeviceManager::dispose_client(ClientHandle client) {
    std::experimental::erase_if(
        _clients, [client](const auto& client_candidate) {
            return client.get() == client_candidate.get();
        });
}

void SharedDeviceManager::set_color(uint32_t index, uint32_t color) {
    if (not _device->is_connected()) {
        return;
    }

    MiniBuffer buffer;
    uint32_t* index_ptr = reinterpret_cast<uint32_t*>(&buffer.data[0]);
    uint32_t* color_ptr = reinterpret_cast<uint32_t*>(&buffer.data[4]);
    *index_ptr = index;
    *color_ptr = color;
    auto result = ioctl(_kmod_fd, TICKLE_IOC_SET_COLOR, &buffer);
    if (result != 0) {
        cout << __PRETTY_FUNCTION__ << " failed with result " << result << "\n";
    }
}

void SharedDeviceManager::dim(uint32_t value) {
    set_color(3, value);
}
