#pragma once
#include "nhc/gameapi.h"

#include <chrono>
#include <string>
#include <vector>

namespace nhc {

struct CatalogItem {
    int id;
    const char* name;
    int defaultAmmo;   // weapons only
};

const std::vector<CatalogItem>& VehicleCatalog();
const std::vector<CatalogItem>& WeaponCatalog();

namespace player {
void SetMoneyLoop(bool on);
bool MoneyLoop();
void SetInvincible(bool on);
bool Invincible();
bool AddMoneyNow(const GameApi& api, std::string& msg);
bool JetpackNow(const GameApi& api, std::string& msg);
void TickMoney(const GameApi& api, std::chrono::steady_clock::time_point now, std::string& msg);
bool ShouldBlockDamage(const GameApi& api, void* ped);
constexpr int kMoneyIntervalSeconds = 3;
} // namespace player

namespace vehicles {
bool SpawnNow(const GameApi& api, int model, std::string& msg);
}

namespace weapons {
bool GiveNow(const GameApi& api, int weapon, int ammo, std::string& msg);
}

namespace settings {
// Whitelisted keys: UiScale (60..160, default 100), Theme (0..2), Visibility (0..2).
int  Get(const std::string& key, int def);
void Set(const std::string& key, int value);
}

} // namespace nhc
