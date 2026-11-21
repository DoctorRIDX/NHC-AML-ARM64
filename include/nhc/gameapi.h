#pragma once

namespace nhc {

// Function pointers into the game. A pointer stays null unless the resolver verified it
// (signature-checked table entry or an exported symbol).
struct GameApi {
    void* (*FindPlayerPed)(int)                    = nullptr;   // CPlayerPed* FindPlayerPed(int)
    void  (*MoneyArmourHealthCheat)()              = nullptr;   // CCheat::MoneyArmourHealthCheat()
    void  (*JetpackCheat)()                        = nullptr;   // CCheat::JetpackCheat()
    void  (*VehicleCheat)(int)                     = nullptr;   // CCheat::VehicleCheat(model id)
    int   (*GiveWeapon)(void*, int, unsigned, bool) = nullptr;  // CPed::GiveWeapon(this, type, ammo, flag)
};

} // namespace nhc
