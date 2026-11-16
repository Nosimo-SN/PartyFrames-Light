#include "CanineBridge.h"
#include "CanineSnapshot.h"
#include "OverheadBridge.h"

namespace PartyLight {
namespace {
CanineReadFn readSnapshot{};
struct Entry {
    std::chrono::steady_clock::time_point next{};
    nlohmann::json status=nullptr;
    bool announced{};
};
std::map<std::uint32_t,Entry> cache;
}
void InitializeCanine()
{
    auto module=GetModuleHandleW(L"CanineDrives.dll");
    using VersionFn=uint32_t (*)(void);
    auto version=module?reinterpret_cast<VersionFn>(GetProcAddress(module,"CDAPI_GetVersion")):nullptr;
    auto api=version?version():0;
    if(api>=CDAPI_VERSION) readSnapshot=reinterpret_cast<CanineReadFn>(GetProcAddress(module,"CDAPI_GetSnapshot"));
    SKSE::log::info("Canine Drives API version {}; snapshot reader {}",api,readSnapshot!=nullptr);
}
void ResetCanine() { cache.clear(); ForgetOverhead(); }
nlohmann::json CanineStatus(std::uint32_t actor)
{
    if(!readSnapshot) return nullptr;
    auto& entry=cache[actor];
    auto now=std::chrono::steady_clock::now();
    if(now<entry.next) return entry.status;
    entry.next=now+1s;
    auto value=ReadCanineSnapshot(actor,readSnapshot);
    if(!value) { entry.status=nullptr; ForgetOverhead(actor); return nullptr; }
    const auto& snapshot=*value;
    PublishOverhead(snapshot);
    entry.status={{"hunger",snapshot.hunger},{"tiredness",snapshot.tiredness},
        {"hungerWarning",snapshot.enabled && (snapshot.flags&CDAPI_FLAG_HUNGER_WARNING)!=0},
        {"tiredWarning",snapshot.enabled && (snapshot.flags&CDAPI_FLAG_TIRED_WARNING)!=0},
        {"hungerTier",snapshot.hungerTier},{"tiredTier",snapshot.tiredTier},
        {"threshold",snapshot.thresholdNoticeable},{"activity",snapshot.activity}};
    if(!entry.announced) {
        SKSE::log::info("Canine snapshot accepted for {:08X}: hunger {:.1f}, tired {:.1f}, flags {}, activity {}",actor,snapshot.hunger,snapshot.tiredness,snapshot.flags,snapshot.activity);
        entry.announced=true;
    }
    return entry.status;
}
}
