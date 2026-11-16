#include "SurvivalBridge.h"
#include "SurvivalPolicy.h"

namespace PartyLight {
namespace {
struct Values {
    std::atomic_bool module{}, enabled{}, hungerEnabled{}, fatigueEnabled{}, coldEnabled{};
    std::atomic<float> severity{};
};
struct Needs {
    std::atomic<float> hunger{}, fatigue{}, cold{};
    std::atomic_bool excluded{true};
    std::chrono::steady_clock::time_point next{};
};
auto options=std::make_shared<Values>();
std::map<RE::FormID,std::shared_ptr<Needs>> cache;
auto nextOptions=std::chrono::steady_clock::time_point{};
class Result final : public RE::BSScript::IStackCallbackFunctor {
public:
    explicit Result(std::function<void(RE::BSScript::Variable)> callback): receive(std::move(callback)) {}
    void operator()(RE::BSScript::Variable value) override { receive(value); }
    void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
private:
    std::function<void(RE::BSScript::Variable)> receive;
};
template<class... Args>
void Call(const char* script,const char* function,std::function<void(RE::BSScript::Variable)> callback,Args... values)
{
    auto* vm=RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if(!vm) return;
    std::unique_ptr<RE::BSScript::IFunctionArguments> args{RE::MakeFunctionArguments(std::move(values)...)};
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result{new Result(std::move(callback))};
    vm->DispatchStaticCall(script,function,args.get(),result);
}
float Number(RE::BSScript::Variable value)
{
    if(value.IsFloat()) return value.GetFloat();
    if(value.IsInt()) return static_cast<float>(value.GetSInt());
    return 0;
}
}
void ResetSurvival()
{
    cache.clear(); options=std::make_shared<Values>(); nextOptions={};
}
void PollSurvival(RE::Actor* actor)
{
    const auto now=std::chrono::steady_clock::now();
    if(now>=nextOptions) {
        nextOptions=now+5s;
        auto current=options;
        Call("SeverActionsNativeExt2","Module_IsUsable",[current](auto v){current->module=v.IsBool()&&v.GetBool();},std::string("survival"));
        for(auto [name,field]:std::initializer_list<std::pair<const char*,std::atomic_bool*>>{
            {"Enabled",&current->enabled},{"HungerEnabled",&current->hungerEnabled},{"FatigueEnabled",&current->fatigueEnabled},{"ColdEnabled",&current->coldEnabled}})
            Call("SeverActionsNativeExt2","Script_GetBool",[current,field](auto v){*field=v.IsBool()&&v.GetBool();},std::string("SeverActions_Survival"),std::string(name),false);
        Call("SeverActionsNativeExt2","Script_GetFloat",[current](auto v){current->severity=Number(v);},std::string("SeverActions_Survival"),std::string("DebuffSeverity"),0.f);
    }
    auto& value=cache[actor->GetFormID()];
    if(!value) value=std::make_shared<Needs>();
    if(now<value->next) return;
    value->next=now+5s;
    auto current=value;
    for(auto [name,field]:std::initializer_list<std::pair<const char*,std::atomic<float>*>>{
        {"Native_Survival_GetHunger",&current->hunger},{"Native_Survival_GetFatigue",&current->fatigue},{"Native_Survival_GetCold",&current->cold}})
        Call("SeverActionsNative",name,[current,field](auto v){*field=Number(v);},actor);
    Call("SeverActionsNative","Native_Survival_IsExcluded",[current](auto v){current->excluded=!v.IsBool()||v.GetBool();},actor);
}
nlohmann::json SurvivalIcons(RE::Actor* actor)
{
    auto found=cache.find(actor->GetFormID());
    if(found==cache.end()) return {false,false,false};
    auto& value=*found->second;
    bool active=options->module && options->enabled;
    // IsInBleedout has no AE relocation in this CommonLib checkout.
    bool bleedout=actor->AsActorState()->IsBleedingOut();
    return {HasSurvivalPenalty(value.hunger,options->severity,active,options->hungerEnabled,value.excluded,bleedout),
        HasSurvivalPenalty(value.fatigue,options->severity,active,options->fatigueEnabled,value.excluded,bleedout),
        HasSurvivalPenalty(value.cold,options->severity,active,options->coldEnabled,value.excluded,bleedout)};
}
}
