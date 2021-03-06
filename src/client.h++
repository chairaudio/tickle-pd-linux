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

    uint16_t get_ring_size() const { return _ring_size; }
    void set_n_ring_chunks(uint16_t);

    static constexpr uint16_t RingbufferChunkSize{48};
    static constexpr uint16_t MaxRingbufferChunks{1024};
    static constexpr uint16_t MaxRingbufferCapacity{RingbufferChunkSize *
                                                    MaxRingbufferChunks};

    void run_tests();

  private:
    std::optional<DeviceHandle> _device_handle;

    std::mutex _frame_mutex;
    isoc_frame _current_frame, _previous_frame;

    FrameChanges _previous_changes;
    DSPState _dsp_state{DSPState::kUndefined};
    void _copy_samples();

    std::atomic<uint16_t> _ring_size{RingbufferChunkSize};
    std::array<int16_t, MaxRingbufferCapacity> _ring_buffer;
    std::atomic<int32_t> _read_index{0}, _write_index{0}, _rw_distance{0};
    std::atomic<bool> _skip_write{false}, _skip_read{false};
};
