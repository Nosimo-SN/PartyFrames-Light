#pragma once
#include "CanineDrivesAPI.h"
namespace PartyLight {
void InitializeOverhead();
void AllowOverhead(bool allowed);
void PublishOverhead(const CDAPI_SnapshotV1& snapshot);
void ForgetOverhead(std::uint32_t actor=0);
}
