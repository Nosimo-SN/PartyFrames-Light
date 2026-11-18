#pragma once
#include <cstdint>

namespace PartyLight {
struct FollowerEvidence {
    std::uint32_t severFlags{};
    bool waiting{};
    bool engineFollowingPlayer{};
    bool packageFollowingPlayer{};
    bool playerTeammate{};
    bool campSandbox{};
};
constexpr bool IsActivelyFollowing(const FollowerEvidence& evidence) noexcept
{
    constexpr std::uint32_t heldElsewhere = 2u|4u|8u|16u|32u|64u|256u;
    constexpr std::uint32_t severFollowPackage = 1u;
    constexpr std::uint32_t custody = 8u|16u|32u|64u;
    if (evidence.campSandbox && !(evidence.severFlags & custody)) return true;
    if (evidence.waiting || (evidence.severFlags & heldElsewhere)) return false;
    if ((evidence.severFlags & severFollowPackage) || evidence.engineFollowingPlayer || evidence.packageFollowingPlayer) return true;
    // Teammate is a relationship/combat flag, not a live recruitment signal.
    // Quest actors and dismissed companions can retain it after following ends,
    // whether or not SeverActions knows them. Keep it only for diagnostics.
    return false;
}
}
