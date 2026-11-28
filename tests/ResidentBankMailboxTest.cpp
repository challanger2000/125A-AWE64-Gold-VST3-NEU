#include "ResidentBankMailbox.hpp"
#include <cstdio>
#include <cstdlib>
#include <thread>
using M=awe64::guest::ResidentBankMailbox;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
    M m,other;
    CHECK(!m.transportOpen()&&!m.begin(M::bankB));
    CHECK(m.read(0x1234)==0xff&&m.read(M::commandPort)==0);
    m.write(M::ackPort,M::bankA);CHECK(m.transportOpen()&&m.current()==M::bankA);
    CHECK(!other.transportOpen());other.write(M::ackPort,M::bankA);
    CHECK(!m.begin(M::bankA)&&!m.begin(0xff));
    CHECK(m.begin(M::bankB)&&!m.transportOpen()&&!m.begin(M::bankB));
    CHECK(m.read(M::commandPort)==M::bankB&&m.read(M::commandPort)==0);
    m.write(M::ackPort,M::busy);CHECK(m.state()==M::State::Busy);
    m.write(M::ackPort,M::bankB);CHECK(m.state()==M::State::Loaded&&!m.transportOpen()&&m.current()==M::bankA);
    CHECK(m.finish()&&m.current()==M::bankB&&m.transportOpen());
    for(auto ack:{M::rejected,M::restored}){
        CHECK(m.begin(M::bankA)&&m.read(M::commandPort)==M::bankA);
        m.write(M::ackPort,M::busy);m.write(M::ackPort,ack);
        CHECK(!m.transportOpen()&&m.current()==M::bankB);
        CHECK(m.finish()&&m.current()==M::bankB&&m.transportOpen());
    }
    for(unsigned i=0;i<1000;++i){
        auto slot=m.current()==M::bankA?M::bankB:M::bankA;
        CHECK(m.begin(slot));
        std::thread guest([&]{CHECK(m.read(M::commandPort)==slot);m.write(M::ackPort,M::busy);m.write(M::ackPort,slot);});
        guest.join();CHECK(m.state()==M::State::Loaded&&!m.transportOpen());
        CHECK(m.finish()&&m.current()==slot&&m.transportOpen());
    }
    CHECK(other.current()==M::bankA&&other.transportOpen());
    CHECK(m.begin(M::bankA));m.fail();m.write(M::ackPort,M::bankA);
    CHECK(m.state()==M::State::Failed&&!m.transportOpen()&&!m.finish()&&m.read(M::commandPort)==0);
    M malformed;malformed.write(M::ackPort,M::bankA);CHECK(malformed.begin(M::bankB));
    CHECK(malformed.read(M::commandPort)==M::bankB);malformed.write(M::ackPort,M::busy);malformed.write(M::ackPort,M::bankA);
    CHECK(malformed.state()==M::State::Failed&&!malformed.transportOpen());
    M premature;premature.write(M::ackPort,M::bankA);CHECK(premature.begin(M::bankB));premature.write(M::ackPort,M::bankB);
    CHECK(premature.state()==M::State::Failed);
    M bootError;bootError.write(M::ackPort,M::fatal);CHECK(bootError.state()==M::State::Failed);
    std::puts("RESIDENT_MAILBOX_1000_RELOADS_REJECTION_ROLLBACK_LATE_ACK_ISOLATION_PASS");
}
