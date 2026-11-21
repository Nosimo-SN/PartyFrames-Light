#include "FollowerPolicy.h"
#include "SurvivalPolicy.h"
#include <iostream>
#include <stdexcept>
#include <limits>
#include <compare>
#include <concepts>
#include <type_traits>
#include <REX/REX/EnumSet.h>
using PartyLight::FollowerEvidence;
using PartyLight::IsActivelyFollowing;
void Check(FollowerEvidence evidence, bool expected, const char* label)
{
    if (IsActivelyFollowing(evidence)!=expected) throw std::runtime_error(label);
}
int main()
{
    // Reproduce the actual CommonLib storage wrapper and its overlapping enum
    // values: bitmask matching admitted e.g. Dialogue=15 and KeepAnEyeOn=48.
    enum class PackageKind : unsigned {Follow=1,Escort=2,Accompany=7,Dialogue=15,Package=18};
    enum class ProcedureKind : unsigned {FollowEscort=6,Escort=8,Accompany=25,Follow=43,Observe=48};
    for(unsigned value=0;value<44;++value) {
        REX::EnumSet<PackageKind> kind=static_cast<PackageKind>(value);
        if(PartyLight::EnumEqualsAny(kind,PackageKind::Follow,PackageKind::Escort,PackageKind::Accompany)!=(value==1 || value==2 || value==7))
            throw std::runtime_error("Package classification must use exact enum values");
    }
    for(unsigned value=0;value<49;++value) {
        REX::EnumSet<ProcedureKind> kind=static_cast<ProcedureKind>(value);
        if(PartyLight::EnumEqualsAny(kind,ProcedureKind::FollowEscort,ProcedureKind::Escort,ProcedureKind::Accompany,ProcedureKind::Follow)!=(value==6 || value==8 || value==25 || value==43))
            throw std::runtime_error("Procedure classification must not mistake observation or dialogue for follow");
    }
    FollowerEvidence observer{.engineFollowingPlayer=true,.packageFollowingPlayer=true,.excludedObservationPackage=true};
    Check(observer,false,"Reported SCG observer must not qualify through either live follow signal");
    if(PartyLight::ClassifyFollower(observer).state!=PartyLight::FollowerState::KnownObservationPackage)
        throw std::runtime_error("Observation exclusion must have an explicit diagnostic reason");
    observer.severFlags=1025;observer.playerTeammate=true;
    Check(observer,false,"Known surveillance task vetoes other admission paths");
    observer.excludedObservationPackage=false;
    Check(observer,true,"Same recruited actor remains eligible outside the observation task");
    FollowerEvidence horse{.severFlags=1025,.engineFollowingPlayer=true,.packageFollowingPlayer=true,.playerTeammate=true,.campSandbox=true,.casualInRange=true,.playerMinion=true,.horse=true};
    if(PartyLight::ClassifyFollower(horse,{.showCasual=true,.showSummons=true}).state!=PartyLight::FollowerState::HorseExcluded)
        throw std::runtime_error("Default horse exclusion must precede all admission paths");
    if(!PartyLight::ClassifyFollower(horse,{.showSummons=true,.excludeHorses=false}).included)
        throw std::runtime_error("Turning off horse exclusion restores normal follower rules");
    if(PartyLight::ClassifyFollower({.horse=true},{.excludeHorses=false}).included)
        throw std::runtime_error("Horse opt-in must not make ownership or species a follow signal");
    Check({.engineFollowingPlayer=true,.horse=false},true,"Other animal followers are unaffected by horse exclusion");
    Check({},false,"Nearby NPC is not a follower");
    Check({.severFlags=1024},false,"Sever roster alone must not qualify");
    Check({.severFlags=1024,.playerTeammate=true},false,"Home roster companion retaining teammate/faction state must not qualify");
    Check({.severFlags=1025,.playerTeammate=true},true,"Actively led Sever companion qualifies");
    Check({.severFlags=1},false,"Casual Sever follower is not a recruited party member");
    const FollowerEvidence houseCat{.severFlags=1,.engineFollowingPlayer=true};
    Check(houseCat,false,"Reported House Cat: casual follow cannot qualify through engine link");
    Check({.severFlags=1,.engineFollowingPlayer=true,.packageFollowingPlayer=true,.playerTeammate=true,.campSandbox=true},false,"Casual veto precedes every shared follow and camp signal");
    if(PartyLight::ClassifyFollower(houseCat).state!=PartyLight::FollowerState::SeverCasual)
        throw std::runtime_error("Cat diagnostic must identify casual, not engine following");
    auto recruitedCat=houseCat;recruitedCat.severFlags=1025;
    Check(recruitedCat,true,"Recruiting an animal admits it without a species blacklist");
    if(PartyLight::ClassifyFollower(recruitedCat).state!=PartyLight::FollowerState::SeverRecruited)
        throw std::runtime_error("Recruitment changes the diagnostic state");
    if(PartyLight::ClassifyFollower({.engineFollowingPlayer=true,.packageFollowingPlayer=true}).state!=PartyLight::FollowerState::EngineFollowing)
        throw std::runtime_error("Non-SA engine following is not evidence of SA recruitment");
    using PartyLight::ClassifyFollower;
    using PartyLight::FollowerOptions;
    auto casual=houseCat;casual.casualInRange=true;
    const FollowerOptions showCasual{.showCasual=true};
    if(!ClassifyFollower(casual,showCasual).included) throw std::runtime_error("Enabled casual inside range must show");
    casual.casualInRange=false;
    if(ClassifyFollower(casual,showCasual).state!=PartyLight::FollowerState::CasualOutOfRange) throw std::runtime_error("Enabled casual outside range must hide by default");
    if(!ClassifyFollower(casual,{.showCasual=true,.hideDistantCasual=false}).included) throw std::runtime_error("Disabling range allows a far or unloaded casual");
    casual.waiting=true;
    if(ClassifyFollower(casual,{.showCasual=true,.hideDistantCasual=false}).included) throw std::runtime_error("Casual options must not bypass waiting");
    casual.waiting=false;casual.severFlags=9;
    if(ClassifyFollower(casual,{.showCasual=true,.hideDistantCasual=false}).included) throw std::runtime_error("Casual options must not bypass custody");
    const FollowerOptions onlySever{.showCasual=true,.hideDistantCasual=false,.onlySeverCompanions=true};
    if(ClassifyFollower(houseCat,onlySever).state!=PartyLight::FollowerState::OnlySeverCompanions ||
       ClassifyFollower({.engineFollowingPlayer=true},onlySever).included ||
       ClassifyFollower({.packageFollowingPlayer=true},onlySever).included ||
       ClassifyFollower({.severFlags=1024},onlySever).included)
        throw std::runtime_error("Companions-only overrides casual options and excludes independent followers and inactive roster members");
    if(!ClassifyFollower(recruitedCat,onlySever).included ||
       !ClassifyFollower({.severFlags=1024,.engineFollowingPlayer=true},onlySever).included ||
       !ClassifyFollower({.severFlags=1026,.waiting=true,.campSandbox=true},onlySever).included)
        throw std::runtime_error("Companions-only keeps recruited active, framework-led and camp companions");
    if(!ClassifyFollower(recruitedCat,showCasual).included) throw std::runtime_error("Casual range must not apply to recruited companions");
    if(ClassifyFollower({.playerMinion=true},{.onlySeverCompanions=true,.showSummons=true}).included ||
       ClassifyFollower({.severFlags=1024,.playerMinion=true},{.onlySeverCompanions=true,.showSummons=true}).included ||
       ClassifyFollower({.severFlags=1,.playerMinion=true},{.showSummons=true}).included ||
       !ClassifyFollower({.playerMinion=true},{.showSummons=true}).included ||
       ClassifyFollower({.playerMinion=true}).included)
        throw std::runtime_error("Summons must respect companions-only and casual visibility precedence");
    using PartyLight::IsCasualInRange;
    if(!IsCasualInRange(true,true,100,100) || IsCasualInRange(true,true,100.01f,100) ||
       IsCasualInRange(false,true,1,100) || IsCasualInRange(true,false,1,100) ||
       IsCasualInRange(true,true,-1,100) || IsCasualInRange(true,true,std::numeric_limits<float>::quiet_NaN(),100) ||
       IsCasualInRange(true,true,std::numeric_limits<float>::infinity(),100))
        throw std::runtime_error("Casual range boundary, unknown space, unloaded or invalid distance failed");
    Check({.severFlags=1024,.engineFollowingPlayer=true,.playerTeammate=true},true,"Vanilla/framework follower tracked by Sever qualifies through engine link");
    Check({.severFlags=1024,.packageFollowingPlayer=true},true,"Quest follower tracked by Sever qualifies through live follow target");
    Check({.playerTeammate=true},false,"Quest teammate after its escort ends must not qualify alone");
    Check({.engineFollowingPlayer=true,.playerTeammate=true},true,"Base game follower slot qualifies through its live engine link");
    Check({.packageFollowingPlayer=true,.playerTeammate=true},true,"Active quest teammate qualifies through its player-targeted package");
    Check({.severFlags=0,.playerTeammate=true},false,"Dismissed framework companion without Sever roster must not qualify through stale teammate state");
    FollowerEvidence questEscort{.packageFollowingPlayer=true,.playerTeammate=true};
    Check(questEscort,true,"Quest escort is shown while actively accompanying the player");
    questEscort.packageFollowingPlayer=false;
    Check(questEscort,false,"Quest escort disappears when its live follow package ends despite retained teammate flag");
    questEscort.engineFollowingPlayer=true;
    Check(questEscort,true,"Recruited former quest escort reappears with a live engine follower link");
    Check({.engineFollowingPlayer=true},true,"Quest follower without teammate flag qualifies through engine link");
    Check({.packageFollowingPlayer=true},true,"Quest escort/accompany package qualifies");
    Check({.severFlags=1025,.waiting=true,.engineFollowingPlayer=true,.playerTeammate=true},false,"Wait overrides stale follow state");
    Check({.severFlags=1026,.waiting=true,.playerTeammate=true,.campSandbox=true},true,"Camp sandbox keeps a waiting roster companion visible");
    Check({.severFlags=1024,.waiting=true,.campSandbox=true},true,"Exact camp package qualifies even after teammate/follow state drops");
    Check({.severFlags=1026,.waiting=true,.playerTeammate=true},false,"Normal waiting companion still stays hidden without camp package");
    Check({.severFlags=1024,.campSandbox=false},false,"Unrelated home roster companion stays hidden");
    Check({.severFlags=1032,.campSandbox=true},false,"Custody still overrides a stale camp package");
    for (auto flag : {2u,4u,8u,16u,32u,64u,256u})
        Check({.severFlags=1025u|flag,.engineFollowingPlayer=true,.packageFollowingPlayer=true,.playerTeammate=true},false,"Parked/travel/custody/schedule companion must not qualify");
    using PartyLight::HasSurvivalPenalty;
    if(HasSurvivalPenalty(24.99f,1,true,true,false,false) || !HasSurvivalPenalty(25,1,true,true,false,false) ||
        HasSurvivalPenalty(100,0,true,true,false,false) || HasSurvivalPenalty(100,1,false,true,false,false) ||
        HasSurvivalPenalty(100,1,true,false,false,false) || HasSurvivalPenalty(100,1,true,true,true,false) ||
        HasSurvivalPenalty(100,1,true,true,false,true)) throw std::runtime_error("Survival icon threshold or disabled/excluded/bleedout gate failed");
    std::cout<<"PASS: stale teammate/roster exclusions, quest escort lifecycle, active Sever and engine followers, camps and wait/hold vetoes\n";
}
