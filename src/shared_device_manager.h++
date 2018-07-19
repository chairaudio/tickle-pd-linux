#pragma once

#include "./tickle.h++"

#include <memory>
#include <vector>
#include <thread>
#include <atomic>

#include <m_pd.h>

class tickle::SharedDeviceManager {
  public:
    SharedDeviceManager();
    ~SharedDeviceManager();

    const std::string& get_kernel_module_version() const {
        return _kernel_module_version;
    }

    ClientHandle create_client();
    void dispose_client(ClientHandle);

    void set_color(uint32_t, uint32_t);
    void dim(uint32_t);

  private:
    std::unique_ptr<Device> _device{nullptr};
    void _on_device_connection();
    void _on_device_disconnection();

    std::string _kernel_module_version{""};
    std::vector<std::unique_ptr<Client>> _clients;

    int _kmod_fd{-1};
    std::thread _device_thread;
    std::atomic<bool> _keep_running{false};
    void _spawn();
    void _connect_to_kmod();
};