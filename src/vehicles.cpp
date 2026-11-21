#include "nhc/features.h"

namespace nhc {

// GTA SA vehicle model ids (400-611). Trailers, RC toys and trains are left out on purpose:
// the game's own vehicle-cheat path is meant for drivable vehicles.
const std::vector<CatalogItem>& VehicleCatalog() {
    static const std::vector<CatalogItem> v = {
        {400, "Landstalker", 0}, {401, "Bravura", 0}, {402, "Buffalo", 0}, {404, "Perennial", 0},
        {405, "Sentinel", 0}, {410, "Manana", 0}, {411, "Infernus", 0}, {415, "Cheetah", 0},
        {419, "Esperanto", 0}, {420, "Taxi", 0}, {421, "Washington", 0}, {424, "BF Injection", 0},
        {426, "Premier", 0}, {429, "Banshee", 0}, {434, "Hotknife", 0}, {439, "Stallion", 0},
        {451, "Turismo", 0}, {458, "Solair", 0}, {466, "Glendale", 0}, {467, "Oceanic", 0},
        {475, "Sabre", 0}, {477, "ZR-350", 0}, {480, "Comet", 0}, {489, "Rancher", 0},
        {494, "Hotring Racer", 0}, {496, "Blista Compact", 0}, {506, "Super GT", 0}, {507, "Elegant", 0},
        {516, "Nebula", 0}, {517, "Majestic", 0}, {518, "Buccaneer", 0}, {526, "Fortune", 0},
        {527, "Cadrona", 0}, {529, "Willard", 0}, {533, "Feltzer", 0}, {534, "Remington", 0},
        {535, "Slamvan", 0}, {536, "Blade", 0}, {541, "Bullet", 0}, {542, "Clover", 0},
        {545, "Hustler", 0}, {549, "Tampa", 0}, {550, "Sunrise", 0}, {551, "Merit", 0},
        {555, "Windsor", 0}, {558, "Uranus", 0}, {559, "Jester", 0}, {560, "Sultan", 0},
        {561, "Stratum", 0}, {562, "Elegy", 0}, {565, "Flash", 0}, {567, "Savanna", 0},
        {575, "Broadway", 0}, {576, "Tornado", 0}, {579, "Huntley", 0}, {580, "Stafford", 0},
        {585, "Emperor", 0}, {587, "Euros", 0}, {589, "Club", 0}, {602, "Alpha", 0}, {603, "Phoenix", 0},
        // Utility and off-road
        {431, "Bus", 0}, {444, "Monster", 0}, {495, "Sandking", 0}, {573, "Dune", 0},
        {571, "Kart", 0}, {572, "Mower", 0}, {531, "Tractor", 0}, {530, "Forklift", 0},
        // Emergency and military
        {416, "Ambulance", 0}, {407, "Firetruck", 0}, {596, "Police Car (LS)", 0}, {597, "Police Car (SF)", 0},
        {598, "Police Car (LV)", 0}, {599, "Police Ranger", 0}, {601, "S.W.A.T.", 0}, {427, "Enforcer", 0},
        {523, "HPV1000", 0}, {432, "Rhino", 0}, {433, "Barracks", 0},
        // Bikes
        {461, "PCJ-600", 0}, {462, "Faggio", 0}, {463, "Freeway", 0}, {468, "Sanchez", 0},
        {471, "Quad", 0}, {521, "FCR-900", 0}, {522, "NRG-500", 0}, {581, "BF-400", 0},
        {586, "Wayfarer", 0}, {481, "BMX", 0}, {509, "Bike", 0}, {510, "Mountain Bike", 0}, {448, "Pizzaboy", 0},
        // Boats
        {446, "Squalo", 0}, {452, "Speeder", 0}, {453, "Reefer", 0}, {454, "Tropic", 0},
        {472, "Coastguard", 0}, {473, "Dinghy", 0}, {493, "Jetmax", 0}, {595, "Launch", 0},
        // Aircraft
        {417, "Leviathan", 0}, {425, "Hunter", 0}, {447, "Seasparrow", 0}, {460, "Skimmer", 0},
        {469, "Sparrow", 0}, {476, "Rustler", 0}, {487, "Maverick", 0}, {488, "News Maverick", 0},
        {497, "Police Maverick", 0}, {511, "Beagle", 0}, {512, "Cropduster", 0}, {513, "Stuntplane", 0},
        {519, "Shamal", 0}, {520, "Hydra", 0}, {548, "Cargobob", 0}, {553, "Nevada", 0},
        {563, "Raindance", 0}, {577, "AT-400", 0}, {593, "Dodo", 0}, {592, "Andromada", 0}, {539, "Vortex", 0},
    };
    return v;
}

namespace vehicles {

bool SpawnNow(const GameApi& api, int model, std::string& msg) {
    bool known = false;
    for (const CatalogItem& it : VehicleCatalog())
        if (it.id == model) { known = true; break; }
    if (!known) { msg = "Vehicle: unknown model id " + std::to_string(model); return false; }
    if (!api.FindPlayerPed || !api.VehicleCheat) { msg = "Vehicle: game functions unavailable"; return false; }
    if (!api.FindPlayerPed(0)) { msg = "Vehicle: no player in the world (menu or loading?)"; return false; }
    // The game's own vehicle cheat picks the spawn position next to the player.
    api.VehicleCheat(model);
    msg = "Vehicle: game cheat function called for model " + std::to_string(model);
    return true;
}

} // namespace vehicles
} // namespace nhc
