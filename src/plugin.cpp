#include "MagelightUI_Mod.h"
#include "LoadState.h"
#include "FollowerPolicy.h"
#include "SurvivalBridge.h"
#include "CanineBridge.h"
#include "OverheadBridge.h"
#include <wincrypt.h>

namespace {
using json = nlohmann::json;
namespace ML = MAGELIGHT_API;
// Process-lifetime callbacks can outlive static teardown in other DLLs.
auto& mod = *new ML::MagelightMod;
ML::ViewId hud{}, settings{};
std::atomic_bool ready{}, pending{}, toggleRequested{};
PartyLight::LoadState loadState;
std::atomic_uint hotkey{66}, modifiers{1};
json config;
std::map<RE::FormID, RE::ActorHandle> tracked;
struct FollowerObservation {
    PartyLight::FollowerEvidence evidence;
    PartyLight::FollowerDecision decision;
    RE::FormID package{},target{};
    bool operator==(const FollowerObservation&) const = default;
};
std::map<RE::FormID,FollowerObservation> followerObservations;
struct Portrait { std::uint64_t stamp{}; std::string data; std::uint32_t kind{}; };
std::map<RE::FormID, Portrait> portraits;
using VersionFn = std::uint32_t (*)();
using BusyFn = std::uint32_t (*)(std::uint32_t);
using PortraitPathFn = std::uint32_t (*)(std::uint32_t, char*, std::uint32_t, std::uint32_t*);
using PortraitStampFn = std::uint64_t (*)(std::uint32_t);
using ThemeFn = std::uint32_t (*)(char*,std::uint32_t);
ThemeFn getTheme{};
RE::TESPackage* campSandbox{};
BusyFn busy{};
PortraitPathFn portraitPath{};
PortraitStampFn portraitStamp{};

std::filesystem::path DataPath()
{
    wchar_t path[32768]{};
    GetModuleFileNameW(nullptr, path, 32768);
    return std::filesystem::path(path).parent_path() / L"Data";
}
std::string Utf8(const std::filesystem::path& path)
{
    auto s = path.u8string();
    return {reinterpret_cast<const char*>(s.data()), s.size()};
}
json Defaults()
{
    return {{"enabled",true},{"x",0.01},{"y",0.4},{"scale",1.0},{"portraitSize",60},
        {"barWidth",88},{"barHeight",13},{"gap",12},{"portraits",true},{"numbers",true},
        {"horizontal",false},{"maxRows",4},{"maxColumns",2},{"showSummons",false},{"distance",true},{"overheadDebug",false},
        {"showCasualFollowers",false},{"hideDistantCasualFollowers",true},{"casualFollowerRange",100.0},{"onlySeverCompanions",false},
        {"hotkey",66},{"shift",true},{"ctrl",false},{"alt",false},
        {"vrButton",7},{"vrModifier",2},{"vrHand",1},{"cards",json::object()}};
}
json Validate(const json& input)
{
    auto out = Defaults();
    for (auto key : {"enabled","portraits","numbers","horizontal","shift","ctrl","alt","showSummons","distance","overheadDebug","showCasualFollowers","hideDistantCasualFollowers","onlySeverCompanions"})
        if (input.contains(key) && input[key].is_boolean()) out[key] = input[key];
    for (auto [key, low, high] : std::initializer_list<std::tuple<const char*,double,double>>{
        {"x",0,0.98},{"y",0,0.98},{"scale",0.5,3},{"portraitSize",24,140},
        {"barWidth",55,240},{"barHeight",8,28},{"gap",0,80},{"hotkey",1,237},{"maxRows",1,12},{"maxColumns",1,12},{"casualFollowerRange",10,500}}) {
        if (input.contains(key) && input[key].is_number()) {
            double v = input[key].get<double>();
            if (std::isfinite(v)) out[key] = std::clamp(v,low,high);
        }
    }
    if (input.contains("cards") && input["cards"].is_object() && input["cards"].size() <= 128) {
        for (auto it = input["cards"].begin(); it != input["cards"].end(); ++it) {
            if (it.key().size() > 180 || !it.value().is_object()) continue;
            const auto& v = it.value();
            json card = json::object();
            for (auto key : {"x","y","scale"}) if (v.contains(key) && v[key].is_number()) {
                double n = v[key].get<double>();
                if (std::isfinite(n)) card[key] = std::clamp(n, std::string_view(key)=="scale" ? 0.5 : 0.0, std::string_view(key)=="scale" ? 3.0 : 0.98);
            }
            if (v.contains("portraits") && v["portraits"].is_boolean()) card["portraits"] = v["portraits"];
            out["cards"][it.key()] = card;
        }
    }
    for(auto [key,allowed]:std::initializer_list<std::pair<const char*,std::vector<unsigned>>>{
        {"vrButton",{1,7,32,33,35}},{"vrModifier",{0,2}},{"vrHand",{0,1,2}}}) {
        if(input.contains(key) && input[key].is_number_unsigned()) {
            const auto value=input[key].get<unsigned>();
            if(std::find(allowed.begin(),allowed.end(),value)!=allowed.end()) out[key]=value;
        }
    }
    if(REL::Module::IsVR()) out["overheadDebug"]=false;
    return out;
}
void Send(ML::ViewId view, const char* channel, const json& payload)
{
    mod.v4()->InteropCall(view, channel, payload.dump(-1,' ',false,json::error_handler_t::replace).c_str());
}
void LogSettings()
{
    static std::string previous;
    auto effective=config.dump(-1,' ',false,json::error_handler_t::replace);
    if(effective!=previous) {
        SKSE::log::info("Effective settings: {}",effective);
        previous=std::move(effective);
    }
}
void PublishConfig(bool includeSettings=true)
{
    LogSettings();
    hotkey = config["hotkey"].get<unsigned>();
    modifiers = (config["shift"].get<bool>() ? 1u : 0u) | (config["ctrl"].get<bool>() ? 2u : 0u) | (config["alt"].get<bool>() ? 4u : 0u);
    Send(hud,"partyConfig",config);
    if(includeSettings) {
        Send(settings,"partyRuntime",{{"vr",REL::Module::IsVR()}});
        Send(settings,"partyConfig",config);
    }
    if(REL::Module::IsVR() && settings) {
        ML::VRHotkeyDesc key{};
        key.size=sizeof(key);key.button=config["vrButton"].get<unsigned>();key.modifier=config["vrModifier"].get<unsigned>();
        key.hand=config["vrHand"].get<std::uint8_t>();key.action=ML::kHotkeyActionToggleUIModePaused;
        auto result=mod.v4()->BindVRHotkey(settings,&key);
        if(result!=ML::Result::Ok) SKSE::log::warn("VR settings chord binding returned {}",static_cast<int>(result));
    }
}
void OnSettings(ML::ViewId, const char* text, void*)
{
    if (!text || std::strlen(text) > 65536) return;
    auto* copy = new std::string(text);
    auto result = mod.v4()->PostGameTask([](void* user) {
        std::unique_ptr<std::string> value(static_cast<std::string*>(user));
        try {
            config = Validate(json::parse(*value));
            PublishConfig(false);
            auto path = DataPath()/L"SKSE"/L"Plugins"/L"PartyLight.json";
            auto temp = path; temp += L".tmp";
            std::ofstream file(temp, std::ios::binary|std::ios::trunc);
            file << config.dump(2); file.close();
            if (!file || !MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
                SKSE::log::warn("Could not save settings to {}", Utf8(path));
        } catch (const std::exception& e) { SKSE::log::warn("Settings rejected: {}", e.what()); }
    },copy);
    if (result != ML::Result::Ok) delete copy;
}
void OnClose(ML::ViewId, const char*, void*) { mod.v4()->ReleaseUIMode(mod.id()); mod.v4()->ShowView(settings,false); }
void OnReady(ML::ViewId)
{
    mod.v4()->PostGameTask([](void*) {
        PublishConfig();
        for (const auto& [id,p] : portraits) if (!p.data.empty()) {
            json value{{"id",id},{"src",p.data},{"kind",p.kind}};
            Send(hud,"partyPortrait",value); Send(settings,"partyPortrait",value);
        }
    },nullptr);
}
std::string StableKey(RE::Actor* actor)
{
    auto* file = actor->GetFile(0);
    if (file && (actor->GetFormID() >> 24) != 0xFF)
        return std::format("{}:{:06X}",file->fileName,actor->GetLocalFormID());
    return std::format("dynamic:{:08X}",actor->GetFormID());
}
bool IsActive(RE::Actor* actor, const std::set<RE::FormID>& following)
{
    if (!actor || actor->IsPlayerRef()) return false;
    const auto excluded=[&](const char* reason) {
        if(followerObservations.erase(actor->GetFormID()))
            SKSE::log::info("Follower excluded: {} [{:08X}]; state={}",actor->GetDisplayFullName(),actor->GetFormID(),reason);
        return false;
    };
    if(actor->IsDead()) return excluded("dead");
    if(actor->IsDisabled()) return excluded("disabled");
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return false;
    if(actor->IsHostileToActor(player)) return excluded("hostile-to-player");
    const auto commander=actor->GetCommandingActor();
    const bool minion=actor->IsSummonedByPlayer() || (actor->IsCommandedActor() && commander.get()==player);
    auto flags = busy ? busy(actor->GetFormID()) : 0;
    const PartyLight::FollowerOptions options{config.value("showCasualFollowers",false),config.value("hideDistantCasualFollowers",true),config.value("onlySeverCompanions",false),config.value("showSummons",false)};
    PartyLight::FollowerEvidence evidence{
        .severFlags=flags,
        .waiting=actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kWaitingForPlayer)!=0.0f,
        .engineFollowingPlayer=following.contains(actor->GetFormID()),
        .playerTeammate=actor->IsPlayerTeammate(),
        .playerMinion=minion
    };
    if((flags&1u) && !(flags&1024u)) {
        auto* actorCell=actor->GetParentCell();auto* playerCell=player->GetParentCell();
        const bool sharedSpace=actorCell && playerCell && (actorCell==playerCell ||
            (!actorCell->IsInteriorCell() && !playerCell->IsInteriorCell() && actor->GetWorldspace() && actor->GetWorldspace()==player->GetWorldspace()));
        const float meters=sharedSpace?(actor->GetPosition()-player->GetPosition()).Length()/70.f:-1.f;
        evidence.casualInRange=PartyLight::IsCasualInRange(sharedSpace,actor->Is3DLoaded(),meters,config["casualFollowerRange"].get<float>());
    }
    RE::FormID packageID{},targetID{};
    std::string packagePlugin;
    auto* process = actor->GetActorRuntimeData().currentProcess;
    if (process) {
        RE::BSSpinLockGuard lock(process->currentPackage.packageLock);
        auto* package = process->currentPackage.package;
        packageID=package ? package->GetFormID():0;
        if(package) if(auto* file=package->GetFile(0)) packagePlugin=file->fileName;
        evidence.campSandbox=campSandbox && package==campSandbox;
        const auto target = process->currentPackage.target.get();
        targetID=target ? target->GetFormID():0;
        if (package && target.get() == player &&
            (package->packData.packType.any(RE::PACKAGE_TYPE::kFollow,RE::PACKAGE_TYPE::kEscort,RE::PACKAGE_TYPE::kAccompany) ||
             package->procedureType.any(RE::PACKAGE_PROCEDURE_TYPE::kFollowWithEscort,RE::PACKAGE_PROCEDURE_TYPE::kFollowWithoutEscort,RE::PACKAGE_PROCEDURE_TYPE::kEscortActor,RE::PACKAGE_PROCEDURE_TYPE::kAccompany))) evidence.packageFollowingPlayer=true;
    }
    const auto decision=PartyLight::ClassifyFollower(evidence,options);
    const FollowerObservation observation{evidence,decision,packageID,targetID};
    const auto previous=followerObservations.find(actor->GetFormID());
    const bool candidate=flags&1025u || evidence.engineFollowingPlayer || evidence.packageFollowingPlayer || evidence.playerTeammate || evidence.campSandbox || minion;
    if(candidate || previous!=followerObservations.end()) {
        if(previous==followerObservations.end() || previous->second!=observation) {
            const char* membership=(flags&1024u)?"sever-recruited":(flags&1u)?"sever-casual":"not-established-by-sever-api";
            SKSE::log::info("Follower {}: {} [{:08X}]; state={} membership={} severFlags={} waiting={} engineLink={} liveFollowPackage={} teammate={} camp={} casualInRange={} package={:08X} packagePlugin={} target={:08X}",
                decision.included?"included":"excluded",actor->GetDisplayFullName(),actor->GetFormID(),PartyLight::FollowerStateName(decision.state),membership,evidence.severFlags,evidence.waiting,
                evidence.engineFollowingPlayer,evidence.packageFollowingPlayer,evidence.playerTeammate,evidence.campSandbox,evidence.casualInRange,packageID,packagePlugin.empty()?"unknown":packagePlugin,targetID);
        }
        if(!candidate) followerObservations.erase(actor->GetFormID());
        else followerObservations.insert_or_assign(actor->GetFormID(),observation);
    }
    return decision.included;
}
void UpdatePortrait(RE::Actor* actor)
{
    if (!portraitPath || !portraitStamp) return;
    auto id = actor->GetFormID();
    auto stamp = portraitStamp(id);
    auto found = portraits.find(id);
    if (found != portraits.end() && found->second.stamp == stamp) return;
    Portrait p; p.stamp = stamp;
    std::vector<char> path(32768);
    auto length = portraitPath(id,path.data(),static_cast<std::uint32_t>(path.size()),&p.kind);
    if (length > 0 && length < path.size()) {
        auto location = std::filesystem::u8path(path.data());
        std::ifstream file(location,std::ios::binary|std::ios::ate);
        auto size = file ? static_cast<std::streamoff>(file.tellg()) : 0;
        if (size > 0 && size <= 4*1024*1024) {
            std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
            file.seekg(0); file.read(reinterpret_cast<char*>(bytes.data()),size); file.close();
            if (file) {
                DWORD count{};
                CryptBinaryToStringA(bytes.data(),static_cast<DWORD>(bytes.size()),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&count);
                std::string encoded(count,'\0');
                if (CryptBinaryToStringA(bytes.data(),static_cast<DWORD>(bytes.size()),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,encoded.data(),&count)) {
                    encoded.resize(count);
                    while (!encoded.empty() && encoded.back()=='\0') encoded.pop_back();
                    p.data = std::string(p.kind==1 ? "data:image/jpeg;base64," : "data:image/png;base64,")+encoded;
                }
            }
        }
        // A rename can race the read; retry on the next discovery pass.
        if (p.data.empty()) return;
    }
    portraits[id] = p;
    json value{{"id",id},{"src",p.data},{"kind",p.kind}};
    Send(hud,"partyPortrait",value); Send(settings,"partyPortrait",value);
}
void Tick()
{
    struct Complete { ~Complete() { pending = false; } } complete;
    if (!ready) return;
    if (loadState.ConsumeReset()) {
        tracked.clear(); portraits.clear(); followerObservations.clear(); PartyLight::ResetSurvival(); PartyLight::ResetCanine(); toggleRequested=false;
        mod.v4()->ShowView(hud,false);
        Send(hud,"partyReset",{}); Send(settings,"partyReset",{});
    }
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* ui = RE::UI::GetSingleton();
    bool world = loadState.IsInGame() && player && player->Is3DLoaded() && ui && !ui->IsMenuOpen(RE::MainMenu::MENU_NAME) && !ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME);
    if (toggleRequested.exchange(false) && world && (!ui->GameIsPaused() || mod.v4()->GetUIModeOwner()==mod.id())) {
        if (mod.v4()->GetUIModeOwner()==mod.id()) OnClose(0,nullptr,nullptr);
        else if (!mod.v4()->IsUIModeActive()) mod.v4()->RequestUIMode(settings,ML::kUIModeFlagPause);
    }
    const bool visible = world && !ui->GameIsPaused() && !mod.v4()->IsUIModeActive() && config.value("enabled",true);
    PartyLight::AllowOverhead(world && !ui->GameIsPaused() && !mod.v4()->IsUIModeActive() && config.value("overheadDebug",false));
    mod.v4()->ShowView(hud,visible);
    if (!world) return;
    static auto nextScan = std::chrono::steady_clock::time_point{};
    auto now = std::chrono::steady_clock::now();
    if (now >= nextScan) {
        nextScan = now + 1s;
        if(getTheme) {
            char theme[64]{};
            if(getTheme(theme,sizeof(theme))>0) { Send(hud,"partyTheme",std::string(theme)); Send(settings,"partyTheme",std::string(theme)); }
        }
        std::set<RE::FormID> following;
        if (auto* extra = player->extraList.GetByType<RE::ExtraFollower>())
            for (const auto& follower : extra->actorFollowers) if (auto a = follower.actor.get()) {
                following.insert(a->GetFormID()); tracked[a->GetFormID()] = a->GetHandle();
            }
        if (auto* processes = RE::ProcessLists::GetSingleton()) processes->ForAllActors([](RE::Actor* actor) {
            if (actor && !actor->IsPlayerRef()) {
                if(config.value("overheadDebug",false)) PartyLight::CanineStatus(actor->GetFormID());
                tracked[actor->GetFormID()] = actor->GetHandle();
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });
        for (auto it = tracked.begin(); it != tracked.end();) {
            auto a = it->second.get();
            if (!a || !IsActive(a.get(),following)) {
                if(!a) {
                    if(followerObservations.erase(it->first)) SKSE::log::info("Follower excluded: [{:08X}]; state=reference-unavailable",it->first);
                }
                portraits.erase(it->first); it = tracked.erase(it);
            }
            else { UpdatePortrait(a.get()); ++it; }
        }
    }
    json actors = json::array();
    for (const auto& [id,handle] : tracked) if (auto a = handle.get()) {
        const auto capacity=config["maxRows"].get<unsigned>()*config["maxColumns"].get<unsigned>();
        if(actors.size()>=capacity) break;
        json row{{"id",id},{"key",StableKey(a.get())},{"name",a->GetDisplayFullName()},{"loaded",a->Is3DLoaded()}};
        row["combat"]=a->IsInCombat();
        if(busy) PartyLight::PollSurvival(a.get());
        row["needs"]=PartyLight::SurvivalIcons(a.get());
        row["canine"]=PartyLight::CanineStatus(id);
        auto commander=a->GetCommandingActor();
        row["summon"]=a->IsSummonedByPlayer() || (a->IsCommandedActor() && commander.get()==player);
        row["distance"]=nullptr; row["heading"]=nullptr;
        auto* actorCell=a->GetParentCell(); auto* playerCell=player->GetParentCell();
        bool sharedSpace=actorCell && playerCell && (actorCell==playerCell ||
            (!actorCell->IsInteriorCell() && !playerCell->IsInteriorCell() && a->GetWorldspace() && a->GetWorldspace()==player->GetWorldspace()));
        if(sharedSpace) {
            auto delta=a->GetPosition()-player->GetPosition();
            float meters=delta.Length()/70.f;
            float heading=player->GetHeadingAngle(a->GetPosition(),false);
            if(std::isfinite(meters)) row["distance"]=meters;
            if(std::isfinite(heading)) row["heading"]=heading;
        }
        for (auto [key,av] : {std::pair{"health",RE::ActorValue::kHealth},{"stamina",RE::ActorValue::kStamina},{"magicka",RE::ActorValue::kMagicka}}) {
            float current = a->AsActorValueOwner()->GetActorValue(av);
            float maximum = a->GetActorValueMax(av);
            row[key] = {{"current",std::isfinite(current) ? std::max(0.f,current) : 0.f},{"max",std::isfinite(maximum) ? std::max(0.f,maximum) : 0.f}};
        }
        actors.push_back(std::move(row));
    }
    Send(hud,"partyState",actors); Send(settings,"partyState",actors);
}
class Input final : public RE::BSTEventSink<RE::InputEvent*> {
public:
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override
    {
        if (!events || !ready) return RE::BSEventNotifyControl::kContinue;
        for (auto* event = *events; event; event = event->next) {
            auto* button = event->AsButtonEvent();
            if (!button || button->GetDevice()!=RE::INPUT_DEVICE::kKeyboard || !button->IsDown() || button->GetIDCode()!=hotkey.load()) continue;
            unsigned held = (GetAsyncKeyState(VK_SHIFT)&0x8000 ? 1u:0u) | (GetAsyncKeyState(VK_CONTROL)&0x8000 ? 2u:0u) | (GetAsyncKeyState(VK_MENU)&0x8000 ? 4u:0u);
            if (held==modifiers.load()) toggleRequested = true;
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
void OnMessage(SKSE::MessagingInterface::Message* message)
{
    if(message->type==SKSE::MessagingInterface::kPostLoad) PartyLight::InitializeOverhead();
    if (message->type==SKSE::MessagingInterface::kPreLoadGame) {
        PartyLight::AllowOverhead(false);
        loadState.BeginLoad();
    }
    if (message->type==SKSE::MessagingInterface::kNewGame) {
        PartyLight::AllowOverhead(false);
        loadState.StartNewGame();
    }
    if (message->type==SKSE::MessagingInterface::kPostLoadGame) {
        loadState.FinishLoad(message->data);
        SKSE::log::info("Save load finished; success={}",loadState.IsInGame());
    }
    if (message->type!=SKSE::MessagingInterface::kDataLoaded) return;
    config=Defaults();
    try { std::ifstream file(DataPath()/L"SKSE"/L"Plugins"/L"PartyLight.json"); if(file) config=Validate(json::parse(file)); }
    catch(const std::exception& e) { SKSE::log::warn("Using defaults: {}",e.what()); }
    if (!mod.acquire("PartyLight","PartyFrames Light",1,9,0)) { SKSE::log::error("Magelight UI 1.9+ required"); return; }
    auto sa = GetModuleHandleW(L"SeverActionsNative.dll");
    auto version = sa ? reinterpret_cast<VersionFn>(GetProcAddress(sa,"SAAPI_GetVersion")) : nullptr;
    if (version && version()>=1) busy=reinterpret_cast<BusyFn>(GetProcAddress(sa,"SAAPI_GetBusyReasons"));
    if (version && version()>=1) getTheme=reinterpret_cast<ThemeFn>(GetProcAddress(sa,"SAAPI_GetUiTheme"));
    if (version && version()>=2) {
        portraitPath=reinterpret_cast<PortraitPathFn>(GetProcAddress(sa,"SAAPI_GetPortraitPath"));
        portraitStamp=reinterpret_cast<PortraitStampFn>(GetProcAddress(sa,"SAAPI_GetPortraitStamp"));
    }
    SKSE::log::info("SeverActions API {}; portraits {}",version ? version():0,portraitPath && portraitStamp);
    if(auto* data=RE::TESDataHandler::GetSingleton()) campSandbox=data->LookupForm<RE::TESPackage>(0x801,"SeversHearth.esp");
    SKSE::log::info("Sever's Hearth camp package: {:08X}",campSandbox ? campSandbox->GetFormID():0);
    PartyLight::InitializeCanine();
    auto base = DataPath()/L"Magelight"/L"PartyLight"/L"views";
    hud=mod.createView("hud",Utf8(base/L"hud"/L"index.html").c_str(),ML::Layer::Hud,{.fullscreen=true,.clickThrough=true,.onDomReady=OnReady});
    settings=mod.createView("settings",Utf8(base/L"settings"/L"index.html").c_str(),ML::Layer::Panel,{.fullscreen=true,.onDomReady=OnReady});
    if(!hud || !settings) { SKSE::log::error("View creation failed: {}",mod.v4()->GetLastErrorMessage(mod.id())); return; }
    if(REL::Module::IsVR()) {
        ML::VRPlacementDesc placement{};
        placement.size=sizeof(placement);placement.mode=0;placement.distanceMeters=1.6f;placement.widthMeters=1.4f;
        mod.v4()->SetViewVRPlacement(hud,&placement);
        placement.mode=3;placement.heightOffset=-0.15f;
        mod.v4()->SetViewVRPlacement(settings,&placement);
        SKSE::log::info("VR HUD placement configured; settings use the Magelight controller laser");
    }
    mod.v4()->RegisterJSListenerEx(settings,"partySave",OnSettings,nullptr);
    mod.v4()->RegisterJSListenerEx(settings,"partyClose",OnClose,nullptr);
    mod.onClose(settings,[](ML::ViewId) { mod.v4()->ShowView(settings,false); });
    auto* input = RE::BSInputDeviceManager::GetSingleton();
    if(input) input->AddEventSink(new Input);
    ready=true; PublishConfig();
    std::thread([] {
        for (;;) {
            std::this_thread::sleep_for(100ms);
            if (ready && !pending.exchange(true)) SKSE::GetTaskInterface()->AddTask([] {
                try { Tick(); } catch(const std::exception& e) { pending=false; SKSE::log::error("HUD update: {}",e.what()); }
            });
        }
    }).detach();
    SKSE::log::info("PartyFrames Light 0.4.4 ready; runtime {}; settings hotkey scancode {} modifiers {}",REL::Module::IsVR()?"VR":"SE/AE",hotkey.load(),modifiers.load());
}
}
SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    auto path=SKSE::log::log_directory();
    if(!path) return false;
    auto sink=std::make_shared<spdlog::sinks::basic_file_sink_mt>((*path/L"PartyLight.log").string(),true);
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("PartyLight",std::move(sink)));
    spdlog::flush_on(spdlog::level::info);
    if(skse->IsEditor()) return false;
#if defined(EXCLUSIVE_SKYRIM_VR)
    if(skse->RuntimeVersion()!=REL::Version{1,4,15,0}) { spdlog::error("This PartyLight build requires Skyrim VR 1.4.15");return false; }
#else
    if(skse->RuntimeVersion()==REL::Version{1,4,15,0}) { spdlog::error("Install PartyLight's VR build for Skyrim VR");return false; }
#endif
    SKSE::Init(skse,false);
    return SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
}
