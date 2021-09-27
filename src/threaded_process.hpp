#ifndef CLASS_THREADED_SUBPROCESS
#define CLASS_THREADED_SUBPROCESS

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>
#include <mutex>

class ThreadedProcess {
public:
    ThreadedProcess(uint64_t const unique_id);
    virtual ~ThreadedProcess();

    ThreadedProcess(ThreadedProcess &&other) = delete;
    ThreadedProcess &operator=(ThreadedProcess &&other) = delete;

    uint64_t get_unique_id() const noexcept { return _unique_id; }
    
    // PURE VIRTUAL FUNCTIONS
    virtual std::string process_name() const noexcept = 0;
    virtual void start() = 0;
    // END OF PURE VIRTUAL FUNCTIONS

    void add_dependency(ThreadedProcess& process);
    void remove_dependency(uint64_t const process_id);

    bool can_exit_loop(bool const control_bool) const;
    bool has_dependencies() const;
    bool is_deletable() const;
    
    void add_dependent_callback(std::function<void(uint64_t)>&& callback);

    virtual void shutdown() noexcept final;
protected:
    mutable std::mutex _dependencies_mutex;
    std::vector<uint64_t> _dependencies;
    std::vector<std::function<void(uint64_t)>> _dependent_callbacks;
    
    std::shared_ptr<std::thread> _thread { nullptr };
    std::atomic<bool> _running { false };
    std::atomic<bool> _shutdown { false };
    std::atomic<bool> _deletable { false };
    uint64_t _unique_id { 0 };
};

#endif
