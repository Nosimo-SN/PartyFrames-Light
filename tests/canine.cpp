#include "CanineSnapshot.h"
#include "OverheadPolicy.h"
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <iostream>
static_assert(sizeof(CDAPI_SnapshotV1)==56);
static_assert(offsetof(CDAPI_SnapshotV1,activity)==52);
CDAPI_SnapshotV1 sample{56,1,0x1234,1,38.f,70.f,40.f,65.f,85.f,5.f,1,2,3,CDAPI_ACTIVITY_NAPPING};
bool published=true;
uint32_t Mock(uint32_t actor,void* output,uint32_t size)
{
    if(!published || actor!=sample.dogRefID || !output || size<56) return 0;
    std::memcpy(output,&sample,56);return 1;
}
void Require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
int main()
{
    Require(!PartyLight::ReadCanineSnapshot(0x1234,nullptr),"Missing API must be optional");
    published=false;
    Require(!PartyLight::ReadCanineSnapshot(0x1234,Mock),"Unpublished/main-menu snapshot must stay unavailable");
    published=true;
    Require(!PartyLight::ReadCanineSnapshot(0x4321,Mock),"Another follower must not receive the dog's state");
    auto value=PartyLight::ReadCanineSnapshot(0x1234,Mock);
    Require(value && value->hunger==38.f && (value->flags&CDAPI_FLAG_HUNGER_WARNING),"Keep the provider's warning below threshold during hysteresis");
    Require(PartyLight::OverheadMatches(*value,0) && PartyLight::OverheadMatches(*value,1),"Overhead warning slots must follow flags");
    for(unsigned activity=0;activity<=11;++activity) {
        value->activity=activity;
        for(unsigned kind=2;kind<=14;++kind) Require(PartyLight::OverheadMatches(*value,kind)==(kind==activity+2),"Exactly one overhead activity must match");
    }
    value->activity=999;value->enabled=0;
    Require(!PartyLight::OverheadMatches(*value,0) && !PartyLight::OverheadMatches(*value,1) && PartyLight::OverheadMatches(*value,14),"Disabled drives suppress warnings; future activities use fallback");
    sample.activity=999;sample.apiVersion=2;sample.structSize=64;
    Require(PartyLight::ReadCanineSnapshot(0x1234,Mock).has_value(),"Future appended snapshots and activities must be accepted");
    sample.hunger=std::numeric_limits<float>::quiet_NaN();
    Require(!PartyLight::ReadCanineSnapshot(0x1234,Mock),"Malformed numeric state must be ignored");
    std::cout<<"PASS: 56-byte ABI, optional API, unpublished state, actor isolation, authoritative warning flags and future activities\n";
}
