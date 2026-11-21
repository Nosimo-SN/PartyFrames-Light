#pragma once
#include <cstdint>

namespace PartyLight {
// Package types/procedures are numbered enum values, not bit flags, even when
// CommonLib stores them in EnumSet. Do not use EnumSet::any() to identify them.
template<class Value, class... Choices>
constexpr bool EnumEqualsAny(Value value, Choices... choices) noexcept
{
    return ((value==choices) || ...);
}
struct FollowerEvidence {
    std::uint32_t severFlags{};
    bool waiting{};
    bool engineFollowingPlayer{};
    bool packageFollowingPlayer{};
    bool playerTeammate{};
    bool campSandbox{};
    bool casualInRange{};
    bool playerMinion{};
    bool excludedObservationPackage{};
    bool horse{};
    bool operator==(const FollowerEvidence&) const = default;
};
enum class FollowerState {
    NotFollowing, SeverCasual, Camp, Waiting, SeverHeld, SeverRecruited,
    EngineFollowing, PackageFollowing, TeammateOnly, SeverRosterOnly, SummonShown, SummonHidden,
    CasualShown, CasualOutOfRange, OnlySeverCompanions, KnownObservationPackage, HorseExcluded
};
struct FollowerOptions {
    bool showCasual{};
    bool hideDistantCasual{true};
    bool onlySeverCompanions{};
    bool showSummons{};
    bool excludeHorses{true};
};
constexpr bool IsCasualInRange(bool sharedSpace, bool loaded, float meters, float limit) noexcept
{
    return sharedSpace && loaded && meters>=0 && limit>0 && meters<=limit;
}
struct FollowerDecision {
    FollowerState state{};
    bool included{};
    bool operator==(const FollowerDecision&) const = default;
};
constexpr const char* FollowerStateName(FollowerState state) noexcept
{
    switch(state) {
    case FollowerState::SeverCasual: return "sever-casual-not-recruited";
    case FollowerState::Camp: return "hearth-camp-package";
    case FollowerState::Waiting: return "waiting";
    case FollowerState::SeverHeld: return "sever-hold";
    case FollowerState::SeverRecruited: return "sever-recruited-following";
    case FollowerState::EngineFollowing: return "engine-follow-link";
    case FollowerState::PackageFollowing: return "player-targeted-follow-package";
    case FollowerState::TeammateOnly: return "teammate-only-not-following";
    case FollowerState::SeverRosterOnly: return "sever-roster-only-not-following";
    case FollowerState::SummonShown: return "player-summon-or-minion-shown";
    case FollowerState::SummonHidden: return "player-summon-or-minion-hidden";
    case FollowerState::CasualShown: return "sever-casual-enabled";
    case FollowerState::CasualOutOfRange: return "sever-casual-out-of-range-or-unavailable";
    case FollowerState::OnlySeverCompanions: return "only-sever-companions-not-recruited";
    case FollowerState::KnownObservationPackage: return "excluded-suspicious-city-guards-observation-package";
    case FollowerState::HorseExcluded: return "horse-excluded-by-setting";
    default: return "no-active-follow-signal";
    }
}
constexpr FollowerDecision ClassifyFollower(const FollowerEvidence& evidence, FollowerOptions options={}) noexcept
{
    constexpr std::uint32_t heldElsewhere = 2u|4u|8u|16u|32u|64u|256u;
    constexpr std::uint32_t severFollowPackage = 1u;
    constexpr std::uint32_t severRoster = 1024u;
    constexpr std::uint32_t custody = 8u|16u|32u|64u;
    if(evidence.horse && options.excludeHorses) return {FollowerState::HorseExcluded,false};
    if(evidence.excludedObservationPackage) return {FollowerState::KnownObservationPackage,false};
    const bool casual=(evidence.severFlags & severFollowPackage) && !(evidence.severFlags & severRoster);
    if(options.onlySeverCompanions && !(evidence.severFlags & severRoster))
        return {FollowerState::OnlySeverCompanions,false};
    // Apply casual settings before shared engine/package/camp checks.
    if(casual) {
        if(!options.showCasual) return {FollowerState::SeverCasual,false};
        if(evidence.severFlags & heldElsewhere) return {FollowerState::SeverHeld,false};
        if(evidence.waiting) return {FollowerState::Waiting,false};
        if(options.hideDistantCasual && !evidence.casualInRange) return {FollowerState::CasualOutOfRange,false};
        return {FollowerState::CasualShown,true};
    }
    if(evidence.playerMinion && !options.onlySeverCompanions)
        return {options.showSummons?FollowerState::SummonShown:FollowerState::SummonHidden,options.showSummons};
    if (evidence.campSandbox && !(evidence.severFlags & custody)) return {FollowerState::Camp,true};
    if (evidence.severFlags & heldElsewhere) return {FollowerState::SeverHeld,false};
    if (evidence.waiting) return {FollowerState::Waiting,false};
    if (evidence.severFlags & severFollowPackage) return {FollowerState::SeverRecruited,true};
    if (evidence.engineFollowingPlayer) return {FollowerState::EngineFollowing,true};
    if (evidence.packageFollowingPlayer) return {FollowerState::PackageFollowing,true};
    // Teammate is a relationship/combat flag, not a live recruitment signal.
    // Quest actors and dismissed companions can retain it after following ends,
    // whether or not SeverActions knows them. Keep it only for diagnostics.
    if (evidence.severFlags & severRoster) return {FollowerState::SeverRosterOnly,false};
    if (evidence.playerTeammate) return {FollowerState::TeammateOnly,false};
    return {FollowerState::NotFollowing,false};
}
constexpr bool IsActivelyFollowing(const FollowerEvidence& evidence) noexcept
{
    return ClassifyFollower(evidence).included;
}
}
