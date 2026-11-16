#pragma once
#include "CanineDrivesAPI.h"
#include <cmath>
#include <optional>

namespace PartyLight {
using CanineReadFn=uint32_t (*)(uint32_t,void*,uint32_t);
inline std::optional<CDAPI_SnapshotV1> ReadCanineSnapshot(uint32_t actor,CanineReadFn read)
{
    if(!read || !actor) return std::nullopt;
    CDAPI_SnapshotV1 value{};
    if(read(actor,&value,sizeof(value))!=1 || value.structSize<sizeof(value) || value.apiVersion<1 || value.dogRefID!=actor || value.enabled>1) return std::nullopt;
    const auto number=[](float n){return std::isfinite(n) && n>=0 && n<=100;};
    if(!number(value.hunger) || !number(value.tiredness) || !number(value.thresholdNoticeable) ||
        !number(value.thresholdStrong) || !number(value.thresholdUrgent) || !number(value.hysteresis) ||
        value.thresholdNoticeable>value.thresholdStrong || value.thresholdStrong>value.thresholdUrgent) return std::nullopt;
    return value;
}
}
