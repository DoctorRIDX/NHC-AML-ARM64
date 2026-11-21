#pragma once
#include "nhc/compat.h"
#include "nhc/features.h"
#include "nhc/resolver.h"

#include <string>

namespace nhc {

// Must stay in sync with the ids used by java/com/nhc/menu/NHCMenu.java
enum Feature { F_MONEY = 0, F_GOD = 1, F_JETPACK = 2, F_VEHICLE = 3, F_WEAPON = 4, F_COUNT = 5 };

enum EntryId {
    E_FPP = 0,       // FindPlayerPed(int)
    E_PROCESS,       // CGame::Process()            (frame hook, gives us the game thread)
    E_MONEY,         // CCheat::MoneyArmourHealthCheat()
    E_JETPACK,       // CCheat::JetpackCheat()
    E_VEHICLE,       // CCheat::VehicleCheat(int)
    E_GIVEWEAPON,    // CPed::GiveWeapon(...)
    E_INFLICT,       // CPed::InflictDamage(...)    (damage hook for invincibility)
    E_COUNT
};

struct Resolution {
    Resolved entry[E_COUNT];
    bool frameHookOk = false;
    bool damageHookOk = false;
};

struct Command {
    enum Type { AddMoney, Jetpack, SpawnVehicle, GiveWeapon };
    Type type = AddMoney;
    int a = 0;   // vehicle model or weapon id
    int b = 0;   // ammo
};

const EntryDef& EntryDefOf(int id);

void Start();   // OnModLoad: compatibility check, resolve, hook, diagnostics, menu
void Stop();

// Pure state update, also used by the unit tests.
void ApplyResolution(const CompatReport& compat, const Resolution& res, const GameApi& api);

const CompatReport& Compat();
bool FeatureAvailable(Feature f);
std::string FeatureReason(Feature f);   // empty when available
bool FeatureEnabled(Feature f);
bool SetFeatureEnabled(Feature f, bool on);
bool Enqueue(const Command& c);         // UI thread -> game thread
std::string LastResult();
std::string DiagnosticsText();

void Tick();                            // game thread, once per frame
bool BlockDamageFor(void* ped);

} // namespace nhc
