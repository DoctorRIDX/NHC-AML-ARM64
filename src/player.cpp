#include "nhc/features.h"

#include <atomic>

namespace nhc {
namespace player {
namespace {
std::atomic<bool> g_moneyLoop{false};
std::atomic<bool> g_invincible{false};
std::chrono::steady_clock::time_point g_lastMoney{};
}

void SetMoneyLoop(bool on) { g_moneyLoop = on; }
bool MoneyLoop() { return g_moneyLoop; }
void SetInvincible(bool on) { g_invincible = on; }
bool Invincible() { return g_invincible; }

bool AddMoneyNow(const GameApi& api, std::string& msg) {
    if (!api.FindPlayerPed || !api.MoneyArmourHealthCheat) { msg = "Money: game functions unavailable"; return false; }
    if (!api.FindPlayerPed(0)) { msg = "Money: no player in the world (menu or loading?)"; return false; }
    api.MoneyArmourHealthCheat();
    msg = "Money: game cheat function called (+$250,000, health and armor refilled)";
    return true;
}

bool JetpackNow(const GameApi& api, std::string& msg) {
    if (!api.FindPlayerPed || !api.JetpackCheat) { msg = "Jetpack: game functions unavailable"; return false; }
    if (!api.FindPlayerPed(0)) { msg = "Jetpack: no player in the world (menu or loading?)"; return false; }
    api.JetpackCheat();
    msg = "Jetpack: game cheat function called";
    return true;
}

void TickMoney(const GameApi& api, std::chrono::steady_clock::time_point now, std::string& msg) {
    if (!g_moneyLoop) return;
    if (g_lastMoney.time_since_epoch().count() != 0 &&
        now - g_lastMoney < std::chrono::seconds(kMoneyIntervalSeconds))
        return;
    g_lastMoney = now;
    if (AddMoneyNow(api, msg)) msg.clear();   // only failures are worth reporting from the loop
}

bool ShouldBlockDamage(const GameApi& api, void* ped) {
    if (!g_invincible || !ped || !api.FindPlayerPed) return false;
    return ped == api.FindPlayerPed(0);
}

} // namespace player
} // namespace nhc
