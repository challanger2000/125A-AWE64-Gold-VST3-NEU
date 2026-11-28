#include "ResidentBankDrain.hpp"
#include <cstdio>
#include <cstdlib>
#include <thread>
using M=awe64::guest::ResidentBankMailbox;
using D=awe64::guest::ResidentBankDrain;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
    M m;D d;
    unsigned recoveries=0;bool drained=false;
    auto read=[&](unsigned short port=M::commandPort){return d.read(m,port,[&]{++recoveries;},[&]{return drained;});};
    auto ack=[&](unsigned char value){d.write(m,M::ackPort,value);};
    CHECK(read()==0&&!d.shouldDrain(m));ack(M::bankA);
    for(unsigned i=0;i<10000;++i){
        const auto prior=m.current();const auto next=prior==M::bankA?M::bankB:M::bankA;
        drained=false;const auto before=recoveries;
        CHECK(m.begin(next)&&!m.transportOpen());
        CHECK(read(0x1234)==0xff&&recoveries==before);
        CHECK(!d.shouldDrain(m));CHECK(read()==0&&d.shouldDrain(m)&&recoveries==before+1);
        for(unsigned j=0;j<3;++j)CHECK(read()==0&&recoveries==before+1);
        drained=true;CHECK(read()==next&&!d.shouldDrain(m));CHECK(read()==0);
        ack(M::busy);CHECK(m.state()==M::State::Busy&&!m.transportOpen());
        const auto result=i%3==0?M::rejected:i%3==1?M::restored:next;
        ack(result);CHECK(!m.transportOpen()&&m.current()==prior);
        CHECK(m.finish()&&m.transportOpen()&&m.current()==(i%3==2?next:prior));
        // No intervening idle read is required to reset guest drain state.
    }
    CHECK(recoveries==10000);
    for(unsigned i=0;i<1000;++i){
        M concurrent;D guest;guest.write(concurrent,M::ackPort,M::bankA);
        CHECK(concurrent.begin(M::bankB));
        std::thread worker([&]{
            unsigned starts=0;auto read=[&]{return guest.read(concurrent,M::commandPort,[&]{++starts;},[]{return true;});};
            CHECK(read()==0&&starts==1);CHECK(read()==M::bankB);
            guest.write(concurrent,M::ackPort,M::busy);
            guest.write(concurrent,M::ackPort,M::bankB);
        });
        worker.join();CHECK(concurrent.state()==M::State::Loaded&&!concurrent.transportOpen());
        CHECK(concurrent.finish()&&concurrent.current()==M::bankB);
    }
    M early;D earlyDrain;earlyDrain.write(early,M::ackPort,M::bankA);CHECK(early.begin(M::bankB));
    earlyDrain.write(early,M::ackPort,M::busy);CHECK(early.state()==M::State::Failed&&!early.transportOpen());
    M fault;D faultDrain;faultDrain.write(fault,M::ackPort,M::bankA);CHECK(fault.begin(M::bankB));
    CHECK(faultDrain.read(fault,M::commandPort,[]{},[]{return true;})==0);fault.fail();
    CHECK(faultDrain.read(fault,M::commandPort,[]{},[]{return true;})==0&&!faultDrain.shouldDrain(fault));
    faultDrain.write(fault,M::ackPort,M::bankB);CHECK(!fault.finish()&&!fault.transportOpen());
    std::puts("RESIDENT_DRAIN_10000_TRANSACTIONS_1000_THREADED_EARLY_ACK_FAULT_PASS");
}
