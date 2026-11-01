#pragma once
#include <string>
#include <functional>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <any>
#include <stdexcept>

namespace uew::api {

enum class JobState { Pending, Running, Done, Failed, Cancelled };

struct Job {
    std::string id;
    std::string type;
    JobState    state{JobState::Pending};
    int         progress{0};  // 0–100
    std::string message;
    std::string error;
    std::any    result;       // type-erased result payload
    std::atomic<bool> cancel_requested{false};
    std::thread worker;

    Job() = default;
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;
};

class JobManager {
public:
    static JobManager& instance() {
        static JobManager jm;
        return jm;
    }

    /// Create a new job and return its ID.
    std::string create(const std::string& type);

    /// Run a job asynchronously.
    void run(const std::string& id, std::function<void(Job&)> fn);

    /// Get a snapshot of job state (thread-safe read).
    struct JobSnapshot {
        std::string id, type;
        JobState    state;
        int         progress;
        std::string message;
        std::string error;
        bool        has_result;
    };
    JobSnapshot get(const std::string& id) const;

    /// Request cancellation.
    bool cancel(const std::string& id);

    /// Get the typed result (throws if not done or wrong type).
    template<typename T>
    T get_result(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = jobs_.find(id);
        if (it == jobs_.end()) throw std::runtime_error("Job not found: " + id);
        return std::any_cast<T>(it->second->result);
    }

private:
    mutable std::mutex mtx_;
    std::map<std::string, std::shared_ptr<Job>> jobs_;
    std::atomic<uint64_t> counter_{1};

    JobManager() = default;
};

} // namespace uew::api
