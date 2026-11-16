#pragma once
namespace PartyLight {
void InitializeCanine();
void ResetCanine();
nlohmann::json CanineStatus(std::uint32_t actor);
}
