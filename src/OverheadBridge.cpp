#include "OverheadBridge.h"
#include "OverheadPolicy.h"
#include "SIF_API.h"

namespace PartyLight {
namespace {
std::atomic_bool allowed{};
std::atomic_uint dogRef{};
std::atomic<std::shared_ptr<const CDAPI_SnapshotV1>> published;
SIF::IAPI* registered{};
constexpr std::array<const char*,15> names{
    "partyLightDogHungry","partyLightDogTired","partyLightDogActivity0","partyLightDogActivity1",
    "partyLightDogActivity2","partyLightDogActivity3","partyLightDogActivity4","partyLightDogActivity5",
    "partyLightDogActivity6","partyLightDogActivity7","partyLightDogActivity8","partyLightDogActivity9",
    "partyLightDogActivity10","partyLightDogActivity11","partyLightDogActivityOther"};
class Condition final : public SIF::ICondition {
public:
    explicit Condition(unsigned value):kind(value) {}
    bool Match(RE::TESObjectREFR* ref) const override
    {
        if(!allowed.load() || !ref || ref->GetFormID()!=dogRef.load()) return false;
        auto value=published.load();
        return value && value->dogRefID==ref->GetFormID() && OverheadMatches(*value,kind);
    }
private:
    unsigned kind;
};
void OnFramework(SKSE::MessagingInterface::Message* message)
{
    if(!message || message->type!=SIF::kMessage_GetAPI || !message->data || message->dataLen!=sizeof(SIF::IAPI*)) return;
    auto* api=static_cast<SIF::IAPI*>(message->data);
    if(api==registered) return;
    if(api->GetVersion()!=1) { SKSE::log::warn("Unsupported Status Indicator Framework API version"); return; }
    unsigned count=0;
    for(unsigned kind=0;kind<names.size();++kind) {
        // The condition keys are parameterless; our rules use {} as their value.
        if(api->RegisterCondition(names[kind],[kind](const Json::Value&,RE::FormType)->std::unique_ptr<SIF::ICondition> {
            return std::make_unique<Condition>(kind);
        })) ++count;
    }
    SKSE::log::info("Status Indicator Framework: registered {} of {} Hjalti debug conditions",count,names.size());
    if(count==names.size()) registered=api;
}
}
void InitializeOverhead()
{
    if(REL::Module::IsVR()) {
        SKSE::log::info("Overhead debug integration disabled in VR: requires a separately verified VR SIF host");return;
    }
    if(!GetModuleHandleW(L"StatusIndicatorFramework.dll")) {
        SKSE::log::info("Status Indicator Framework absent; overhead debug icons unavailable");return;
    }
    const bool subscribed=SKSE::GetMessagingInterface()->RegisterListener("StatusIndicatorFramework",OnFramework);
    SKSE::log::info("Status Indicator Framework registration listener: {}",subscribed);
}
void AllowOverhead(bool value) { allowed=value; }
void PublishOverhead(const CDAPI_SnapshotV1& value)
{
    published.store(std::make_shared<const CDAPI_SnapshotV1>(value));
    dogRef=value.dogRefID;
}
void ForgetOverhead(std::uint32_t actor)
{
    if(!actor || dogRef.load()==actor) { dogRef=0;published.store(nullptr); }
}
}
