#pragma once

#include "./tickle_protocol.h"

#include <experimental/memory>

namespace tickle {
class SharedDeviceManager;
class Device;
using DeviceHandle = std::experimental::observer_ptr<tickle::Device>;
class Client;
using ClientHandle = std::experimental::observer_ptr<tickle::Client>;

struct Position {
    uint8_t x, y;
};

struct NormalizedPosition {
    float x, y;
    bool operator!=(const NormalizedPosition& rhs) const {
        return x != rhs.x || y != rhs.y;
    }
};

}  // namespace tickle
