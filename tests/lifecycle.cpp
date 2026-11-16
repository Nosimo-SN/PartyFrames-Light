#include "LoadState.h"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>

void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
int main()
{
    PartyLight::LoadState state;
    Require(!state.IsInGame(),"Startup must not access the world");
    state.BeginLoad();
    Require(state.ConsumeReset(),"Preload must request a cache reset");
    Require(!state.ConsumeReset(),"Reset must be consumed once");
    // Reproduce the exact 0x1 payload and worker-thread delivery in the crash.
    std::thread loader([&] { state.FinishLoad(reinterpret_cast<void*>(std::uintptr_t{1})); });
    loader.join();
    Require(state.IsInGame(),"Successful load payload 0x1 must be accepted without dereferencing it");
    state.BeginLoad();
    Require(!state.IsInGame(),"Reload must stop world updates immediately");
    state.FinishLoad(nullptr);
    Require(!state.IsInGame(),"Null payload means failed load");
    state.ConsumeReset();
    state.StartNewGame();
    Require(state.IsInGame(),"New game must enable the world");
    Require(state.ConsumeReset(),"New game must discard the prior party");
    std::thread reloader([&] { state.BeginLoad(); state.FinishLoad(reinterpret_cast<void*>(std::uintptr_t{1})); });
    reloader.join();
    Require(state.IsInGame() && state.ConsumeReset(),"A fast reload must retain the reset request until the game task consumes it");
    std::cout << "PASS: 0x1 success, null failure, worker-thread callbacks, reload and new-game reset\n";
}
