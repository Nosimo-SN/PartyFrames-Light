#pragma once
namespace PartyLight {
void ResetSurvival();
void PollSurvival(RE::Actor* actor);
nlohmann::json SurvivalIcons(RE::Actor* actor);
}
