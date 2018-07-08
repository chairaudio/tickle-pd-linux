#pragma once

#include "./tickle.h++"
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <string>
#include <optional>
#include <sstream>
#include <fmt/format.h>

class tickle::Device {
  public:
    Device();
    virtual ~Device();

    void set_connection(int);
    bool is_connected() const { return _device_fd != -1; }
    // const isoc_frame& read_frame();

    void fill_audio_buffer(float* out, uint32_t n_samples);

    struct Info {
        std::string serial, version;
    };
    std::optional<Info> get_info() const { return _info; }

  private:
    int _device_fd{-1};

    std::optional<Info> _info;
    void _read_device_info();

    isoc_frame _frame;

    // Position _position;
    // bool _got_touch{false};
    // bool _button_state[4];
    // std::array<uint8_t, 3> _quad_data;

    void _copy_samples();
    static constexpr uint32_t samples_per_chunk = 24;
    static constexpr uint32_t n_chunks = 16;
    using chunk_t = std::array<int16_t, samples_per_chunk>;
    chunk_t _silence_buffer;
    std::array<chunk_t, n_chunks> _audio_buffer;
    std::atomic<int32_t> _read_chunk{0}, _read_index{0}, _write_chunk{0};
    bool _audio_is_running{false};
};
