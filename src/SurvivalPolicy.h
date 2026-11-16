#pragma once
namespace PartyLight {
constexpr bool HasSurvivalPenalty(float value,float severity,bool systemEnabled,bool needEnabled,bool excluded,bool bleedout) noexcept
{
    return systemEnabled && needEnabled && !excluded && !bleedout && severity>0 && value>=25;
}
}
