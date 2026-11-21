#include "nhc/features.h"

namespace nhc {

// eWeaponType ids of GTA SA (1-46).
const std::vector<CatalogItem>& WeaponCatalog() {
    static const std::vector<CatalogItem> v = {
        {1, "Brass Knuckles", 1}, {2, "Golf Club", 1}, {3, "Nightstick", 1}, {4, "Knife", 1},
        {5, "Baseball Bat", 1}, {6, "Shovel", 1}, {7, "Pool Cue", 1}, {8, "Katana", 1}, {9, "Chainsaw", 1},
        {14, "Flowers", 1}, {15, "Cane", 1},
        {16, "Grenade", 25}, {17, "Tear Gas", 25}, {18, "Molotov Cocktail", 25},
        {22, "Pistol", 150}, {23, "Silenced Pistol", 150}, {24, "Desert Eagle", 100},
        {25, "Shotgun", 60}, {26, "Sawn-Off Shotgun", 60}, {27, "Combat Shotgun", 60},
        {28, "Micro SMG", 300}, {29, "MP5", 300}, {30, "AK-47", 300}, {31, "M4", 300}, {32, "Tec-9", 300},
        {33, "Country Rifle", 50}, {34, "Sniper Rifle", 50},
        {35, "RPG", 10}, {36, "HS Rocket", 10}, {37, "Flamethrower", 500}, {38, "Minigun", 1000},
        {39, "Satchel Charge", 10}, {40, "Detonator", 1},
        {41, "Spraycan", 500}, {42, "Fire Extinguisher", 500}, {43, "Camera", 50},
        {44, "Night Vision", 1}, {45, "Thermal Vision", 1}, {46, "Parachute", 1},
    };
    return v;
}

namespace weapons {

bool GiveNow(const GameApi& api, int weapon, int ammo, std::string& msg) {
    bool known = false;
    for (const CatalogItem& it : WeaponCatalog())
        if (it.id == weapon) { known = true; break; }
    if (!known) { msg = "Weapon: unknown weapon id " + std::to_string(weapon); return false; }
    if (ammo < 1) ammo = 1;
    if (ammo > 9999) ammo = 9999;
    if (!api.FindPlayerPed || !api.GiveWeapon) { msg = "Weapon: game functions unavailable"; return false; }
    void* ped = api.FindPlayerPed(0);
    if (!ped) { msg = "Weapon: no player in the world (menu or loading?)"; return false; }
    api.GiveWeapon(ped, weapon, static_cast<unsigned>(ammo), true);
    msg = "Weapon: game function called for weapon " + std::to_string(weapon) + " with " + std::to_string(ammo) + " ammo";
    return true;
}

} // namespace weapons
} // namespace nhc
