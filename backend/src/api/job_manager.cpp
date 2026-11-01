#include "api/job_manager.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace uew::api {

std::string JobManager::create(const std::string& type) {
    std::lock_guard<std::mutex> lk(mtx_);
    uint64_t n = counter_.fetch_add(1);
    std::ostringstream oss;
    oss << "job-" << std::setw(6) << std::setfill('0') << n;
    std::string id = oss.str();
    auto job = std::make_shared<Job>();
    job->id   = id;
    job->type = type;
    job->state = JobState::Pending;
    jobs_[id] = job;
    return id;
}

void JobManager::run(const std::string& id, std::function<void(Job&)> fn) {
    std::shared_ptr<Job> job;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = jobs_.find(id);
        if (it == jobs_.end()) throw std::runtime_error("Job not found: " + id);
        job = it->second;
        job->state = JobState::Running;
    }

    job->worker = std::thread([job, fn]() {
        try {
            fn(*job);
            if (!job->cancel_requested.load()) {
                job->state    = JobState::Done;
                job->progress = 100;
            }
        } catch (const std::exception& e) {
            job->state = JobState::Failed;
            job->error = e.what();
        } catch (...) {
            job->state = JobState::Failed;
            job->error = "Unknown error in job worker.";
        }
    });
    job->worker.detach();
}

JobManager::JobSnapshot JobManager::get(const std::string& id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = jobs_.find(id);
    if (it == jobs_.end()) throw std::runtime_error("Job not found: " + id);
    const auto& j = *it->second;
    return {j.id, j.type, j.state, j.progress, j.message, j.error, j.result.has_value()};
}

bool JobManager::cancel(const std::string& id) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = jobs_.find(id);
    if (it == jobs_.end()) return false;
    auto& j = *it->second;
    if (j.state == JobState::Running || j.state == JobState::Pending) {
        j.cancel_requested.store(true);
        j.state = JobState::Cancelled;
        return true;
    }
    return false;
}

} // namespace uew::api
