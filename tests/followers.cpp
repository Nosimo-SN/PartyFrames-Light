#include "FollowerPolicy.h"
#include "SurvivalPolicy.h"
#include <iostream>
#include <stdexcept>
using PartyLight::FollowerEvidence;
using PartyLight::IsActivelyFollowing;
void Check(FollowerEvidence evidence, bool expected, const char* label)
{
    if (IsActivelyFollowing(evidence)!=expected) throw std::runtime_error(label);
}
int main()
{
    Check({},false,"Nearby NPC is not a follower");
    Check({.severFlags=1024},false,"Sever roster alone must not qualify");
    Check({.severFlags=1024,.playerTeammate=true},false,"Home roster companion retaining teammate/faction state must not qualify");
    Check({.severFlags=1025,.playerTeammate=true},true,"Actively led Sever companion qualifies");
    Check({.severFlags=1},true,"Casual Sever follower qualifies");
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
