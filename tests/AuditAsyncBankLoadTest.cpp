#include "AsyncBankLoad.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
using awe64::guest::AsyncBankLoad;
using namespace std::chrono;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
template<class F> void await(F f){
    const auto deadline=steady_clock::now()+seconds(5);
    while(!f()&&steady_clock::now()<deadline)std::this_thread::sleep_for(milliseconds(1));
    CHECK(f());
}
int main(){
    AsyncBankLoad loader;
    std::atomic<bool> entered{false},unblock{false};
    std::atomic<unsigned> committed{0};
    const auto start=steady_clock::now();
    CHECK(loader.start([&](const std::atomic<bool>& cancel){
        entered=true;
        while(!unblock&&!cancel)std::this_thread::sleep_for(milliseconds(1));
        if(cancel)return false;
        ++committed;return true;
    }));
    CHECK(steady_clock::now()-start<milliseconds(200));
    await([&]{return entered.load();});
    CHECK(loader.status()==AsyncBankLoad::Status::Loading);
    CHECK(!loader.start([&](const std::atomic<bool>&){++committed;return true;}));
    CHECK(committed==0);
    loader.cancelAndJoin();
    CHECK(loader.status()==AsyncBankLoad::Status::Idle&&committed==0);
    CHECK(loader.start([](const std::atomic<bool>&){return false;}));
    await([&]{return loader.status()==AsyncBankLoad::Status::Failed;});
    CHECK(loader.start([](const std::atomic<bool>&)->bool{throw 1;}));
    await([&]{return loader.status()==AsyncBankLoad::Status::Failed;});
    CHECK(loader.start([&](const std::atomic<bool>&){++committed;return true;}));
    await([&]{return loader.status()==AsyncBankLoad::Status::Idle;});
    CHECK(committed==1);
    // Destructor cancels an active task; referenced fixture outlives loader.
    std::atomic<bool> ended{false};
    {AsyncBankLoad temporary;CHECK(temporary.start([&](const std::atomic<bool>& cancel){
        while(!cancel)std::this_thread::sleep_for(milliseconds(1));
        ended=true;return false;
    }));await([&]{return temporary.status()==AsyncBankLoad::Status::Loading;});
    // Wait for entry explicitly via a separate handshake in the next case.
    }
    std::atomic<bool> began{false};
    {AsyncBankLoad temporary;CHECK(temporary.start([&](const std::atomic<bool>& cancel){
        began=true;while(!cancel)std::this_thread::sleep_for(milliseconds(1));ended=true;return false;
    }));await([&]{return began.load();});ended=false;}
    CHECK(ended);
    std::puts("ASYNC_BANK_LOAD_CANCEL_FAILURE_RETRY_LIFETIME_PASS");
}
