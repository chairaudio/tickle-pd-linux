#pragma once

#include "./tickle.h++"

#include <optional>
#include <tuple>
#include <atomic>
#include <chrono>
#include <mutex>

class tickle::Client {
  public:
    Client();

    std::optional<DeviceHandle> get_device_handle() const {
        return _device_handle;
    }
    bool has_device() const { return get_device_handle().has_value(); }

    void notify_device_was_connected(DeviceHandle);
    void notify_device_was_disconnected();

    void copy_frame(const isoc_frame&);

    struct FrameChanges {
        std::optional<bool> touch;
        std::optional<NormalizedPosition> position;

        struct Rotary {
            int8_t index, value;
        };
        std::array<std::optional<Rotary>, 3> rotary;

        struct Button {
            int8_t index, value;
        };
        std::array<std::optional<Button>, 2> buttons;
    };

    FrameChanges compare_frames();

    enum class DSPState { kUndefined, kWillStart, kIsRunning };
    void prepare_dsp(float, int);
    void fill_audio_buffer(float* out, uint32_t n_samples);

    uint16_t get_ring_size() const
    {
        return _ring_size;
    }
    
  private:
    std::optional<DeviceHandle> _device_handle;

    isoc_frame _current_frame, _previous_frame;

    DSPState _dsp_state{DSPState::kUndefined};
    void _copy_samples();
    
    /*
    static constexpr int32_t samples_per_chunk = 96;
    static constexpr int32_t n_chunks = 16;
    
    struct chunk_t {
        std::array<int16_t, samples_per_chunk + 4> samples;
        uint16_t chunk_id;
        int32_t n_valid_samples;
    };*/
    // chunk_t _silence_buffer;
    std::atomic<uint16_t> _ring_size {96}; 
    std::array<int16_t, 48000> _ring_buffer; // max capacity: 1s
    // std::atomic<int32_t> _read_chunk{0}, _read_index{0}, _write_chunk{0};
    std::atomic<int32_t> _read_index {0}, _write_index {0};
    bool _skip {true};
};
