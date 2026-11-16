#pragma once
#include "CanineDrivesAPI.h"
namespace PartyLight {
constexpr bool OverheadMatches(const CDAPI_SnapshotV1& value,unsigned kind) noexcept
{
    if(kind==0) return value.enabled && (value.flags&CDAPI_FLAG_HUNGER_WARNING);
    if(kind==1) return value.enabled && (value.flags&CDAPI_FLAG_TIRED_WARNING);
    if(kind>=2 && kind<=13) return value.activity==kind-2;
    return kind==14 && value.activity>CDAPI_ACTIVITY_FIGHTING;
}
}
