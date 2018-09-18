#pragma once

#include "./tickle.h++"

#include <string>
#include <optional>

class tickle::Device {
  public:
    Device();
    virtual ~Device();

    void set_connection(int);
    bool is_connected() const { return _device_fd != -1; }

    struct Info {
        std::string serial, version;
    };
    std::optional<Info> get_info() const { return _info; }

  private:
    int _device_fd{-1};

    std::optional<Info> _info;
    void _read_device_info();
};
