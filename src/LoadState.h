#pragma once
#include <atomic>

namespace PartyLight {
class LoadState {
public:
    void BeginLoad() noexcept
    {
        inGame = false;
        resetRequested = true;
    }
    void FinishLoad(const void* result) noexcept
    {
        // SKSE encodes success in the pointer value (0/1); it is not an address.
        inGame = result != nullptr;
    }
    void StartNewGame() noexcept
    {
        resetRequested = true;
        inGame = true;
    }
    bool IsInGame() const noexcept { return inGame.load(); }
    bool ConsumeReset() noexcept { return resetRequested.exchange(false); }
private:
    std::atomic_bool inGame{}, resetRequested{};
};
}
