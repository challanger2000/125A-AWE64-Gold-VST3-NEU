#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include <utility>
namespace awe64::guest {
// One explicit UI request at a time. Worker owns the request and all blocking
// operations. No queued request can outlive its component. No GUI/SDK calls.
class AsyncBankLoad final {
public:
    enum class Status { Idle, Loading, Failed };
private:
    std::mutex control_;
    std::thread worker_;
    std::atomic<bool> cancel_{false};
    std::atomic<Status> status_{Status::Idle};
public:
    AsyncBankLoad()=default;
    AsyncBankLoad(const AsyncBankLoad&)=delete;
    AsyncBankLoad& operator=(const AsyncBankLoad&)=delete;
    ~AsyncBankLoad(){cancelAndJoin();}
    Status status() const noexcept {return status_.load(std::memory_order_acquire);}
    template<class Work>
    bool start(Work&& work) noexcept {
        try {
            std::unique_lock<std::mutex> lock(control_,std::try_to_lock);
            if(!lock.owns_lock()||status()==Status::Loading)return false;
            if(worker_.joinable())worker_.join(); // completed worker only
            cancel_.store(false,std::memory_order_release);
            status_.store(Status::Loading,std::memory_order_release);
            try {
                worker_=std::thread([this,task=std::forward<Work>(work)]() mutable {
                    bool ok=false;
                    try {if(!cancel_.load(std::memory_order_acquire))ok=task(cancel_);}
                    catch(...) {}
                    const auto next=(cancel_.load(std::memory_order_acquire)||ok)?Status::Idle:Status::Failed;
                    status_.store(next,std::memory_order_release);
                });
            }catch(...){status_.store(Status::Failed,std::memory_order_release);return false;}
            return true;
        }catch(...){return false;}
    }
    // Host lifecycle supersedes an uncommitted UI request. The task must
    // poll cancellation at all expensive waits and immediately before commit.
    void cancelAndJoin() noexcept {
        std::lock_guard<std::mutex> lock(control_);
        cancel_.store(true,std::memory_order_release);
        if(worker_.joinable())worker_.join();
        status_.store(Status::Idle,std::memory_order_release);
    }
};
}
