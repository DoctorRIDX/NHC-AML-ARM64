#include "nhc/core.h"
#include "nhc/menu.h"
#include "nhc/platform.h"

#include <chrono>
#include <cstdio>
#include <deque>
#include <initializer_list>
#include <mutex>

namespace nhc {
namespace {

// Symbols are only *candidates*: they are used when the game library actually exports them.
// Nothing here is an address. Table entries (nhc.ini) always need a signature, see resolver.cpp.
const EntryDef kEntries[E_COUNT] = {
    {"FindPlayerPed", "FindPlayerPed(int)", "_Z13FindPlayerPedi"},
    {"CGame_Process", "CGame::Process()", "_ZN5CGame7ProcessEv"},
    {"CCheat_MoneyArmourHealthCheat", "CCheat::MoneyArmourHealthCheat()", "_ZN6CCheat22MoneyArmourHealthCheatEv"},
    {"CCheat_JetpackCheat", "CCheat::JetpackCheat()", "_ZN6CCheat12JetpackCheatEv"},
    {"CCheat_VehicleCheat", "CCheat::VehicleCheat(int)", "_ZN6CCheat12VehicleCheatEi,_ZN6CCheat12VehicleCheatE8eModelID"},
    {"CPed_GiveWeapon", "CPed::GiveWeapon(weapon, ammo, flag)", "_ZN4CPed10GiveWeaponE11eWeaponTypejb"},
    {"CPed_InflictDamage", "CPed::InflictDamage(...)", "_ZN4CPed13InflictDamageEP7CEntity11eWeaponTypef14ePedPieceTypesh"},
};

const char* const kFeatureNames[F_COUNT] = {"Infinite Money", "Invincibility", "Jetpack", "Vehicle Spawn", "Weapon Selector"};

std::mutex g_mutex;
std::deque<Command> g_queue;
CompatReport g_compat;
Resolution g_res;
GameApi g_api;
bool g_available[F_COUNT] = {false, false, false, false, false};
std::string g_reason[F_COUNT];
std::string g_last = "Ready.";

void (*g_origProcess)() = nullptr;
bool (*g_origInflict)(void*, void*, int, float, int, unsigned char) = nullptr;

void HookProcess() {
    if (g_origProcess) g_origProcess();
    Tick();
}

bool HookInflict(void* self, void* damager, int weapon, float damage, int piece, unsigned char dir) {
    if (BlockDamageFor(self)) return false;
    return g_origInflict ? g_origInflict(self, damager, weapon, damage, piece, dir) : false;
}

std::string Evaluate(const CompatReport& c, const Resolution& r, std::initializer_list<int> needs,
                     bool needFrameHook, bool needDamageHook) {
    if (!c.canRun()) return "Prerequisites failed: " + c.FirstProblem();
    std::string missing;
    for (int id : needs) {
        if (r.entry[id].ok()) continue;
        if (!missing.empty()) missing += ", ";
        missing += kEntries[id].label;
    }
    if (!missing.empty())
        return "No verified game function for: " + missing + " (see About and NHC_diagnostics.txt)";
    if (needFrameHook && !r.frameHookOk) return "Frame hook on CGame::Process() could not be installed";
    if (needDamageHook && !r.damageHookOk) return "Damage hook on CPed::InflictDamage() could not be installed";
    return "";
}

Feature FeatureOf(Command::Type t) {
    switch (t) {
        case Command::AddMoney: return F_MONEY;
        case Command::Jetpack: return F_JETPACK;
        case Command::SpawnVehicle: return F_VEHICLE;
        case Command::GiveWeapon: return F_WEAPON;
    }
    return F_MONEY;
}

void SetLast(const std::string& s) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_last = s;
}

} // namespace

const EntryDef& EntryDefOf(int id) { return kEntries[id]; }

void ApplyResolution(const CompatReport& c, const Resolution& res, const GameApi& api) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_compat = c;
    g_res = res;
    g_api = api;
    g_queue.clear();

    std::string r[F_COUNT];
    r[F_MONEY]   = Evaluate(c, res, {E_FPP, E_PROCESS, E_MONEY}, true, false);
    r[F_GOD]     = Evaluate(c, res, {E_FPP, E_INFLICT}, false, true);
    r[F_JETPACK] = Evaluate(c, res, {E_FPP, E_PROCESS, E_JETPACK}, true, false);
    r[F_VEHICLE] = Evaluate(c, res, {E_FPP, E_PROCESS, E_VEHICLE}, true, false);
    r[F_WEAPON]  = Evaluate(c, res, {E_FPP, E_PROCESS, E_GIVEWEAPON}, true, false);
    for (int f = 0; f < F_COUNT; f++) {
        g_available[f] = r[f].empty();
        g_reason[f] = r[f];
    }
}

void Start() {
    CompatReport c = RunCompatCheck();
    plat::Log(plat::LOG_INFO, "Compatibility report:\n%s", c.Summary().c_str());

    Resolution res;
    GameApi api;
    if (c.canRun()) {
        const bool tableUsable = (c.build == BuildStatus::TableMatches);
        for (int i = 0; i < E_COUNT; i++)
            res.entry[i] = ResolveEntry(kEntries[i], c.libName, c.libBase, c.libLength, tableUsable);

        if (res.entry[E_FPP].ok())
            api.FindPlayerPed = reinterpret_cast<void* (*)(int)>(res.entry[E_FPP].address);
        if (res.entry[E_MONEY].ok())
            api.MoneyArmourHealthCheat = reinterpret_cast<void (*)()>(res.entry[E_MONEY].address);
        if (res.entry[E_JETPACK].ok())
            api.JetpackCheat = reinterpret_cast<void (*)()>(res.entry[E_JETPACK].address);
        if (res.entry[E_VEHICLE].ok())
            api.VehicleCheat = reinterpret_cast<void (*)(int)>(res.entry[E_VEHICLE].address);
        if (res.entry[E_GIVEWEAPON].ok())
            api.GiveWeapon = reinterpret_cast<int (*)(void*, int, unsigned, bool)>(res.entry[E_GIVEWEAPON].address);

        if (res.entry[E_PROCESS].ok())
            res.frameHookOk = plat::InstallHook(res.entry[E_PROCESS].address,
                                                reinterpret_cast<void*>(&HookProcess),
                                                reinterpret_cast<void**>(&g_origProcess));
        if (res.entry[E_INFLICT].ok())
            res.damageHookOk = plat::InstallHook(res.entry[E_INFLICT].address,
                                                 reinterpret_cast<void*>(&HookInflict),
                                                 reinterpret_cast<void**>(&g_origInflict));
    }
    ApplyResolution(c, res, api);

    // Writing the defaults creates nhc.ini, so the user can find the file and add [Build] / [Offsets_*].
    settings::Set("UiScale", settings::Get("UiScale", 100));
    settings::Set("Theme", settings::Get("Theme", 0));
    settings::Set("Visibility", settings::Get("Visibility", 0));

    const std::string diag = DiagnosticsText();
    plat::Log(plat::LOG_INFO, "%s", diag.c_str());
    std::string dir = c.host.dataPath;
    if (!dir.empty() && dir.back() != '/') dir += "/";
    if (!plat::WriteTextFile(dir + "NHC_diagnostics.txt", diag))
        plat::Log(plat::LOG_WARN, "Could not write NHC_diagnostics.txt");

    if (c.uiPossible()) menu::StartAsync();
    else plat::Log(plat::LOG_ERROR, "Menu disabled: %s", c.FirstProblem().c_str());
}

void Stop() { plat::Log(plat::LOG_INFO, "NHC unloaded"); }

const CompatReport& Compat() { return g_compat; }

bool FeatureAvailable(Feature f) {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_available[f];
}

std::string FeatureReason(Feature f) {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_reason[f];
}

bool FeatureEnabled(Feature f) {
    if (f == F_MONEY) return player::MoneyLoop();
    if (f == F_GOD) return player::Invincible();
    return false;
}

bool SetFeatureEnabled(Feature f, bool on) {
    if (f != F_MONEY && f != F_GOD) return false;
    if (on && !FeatureAvailable(f)) {
        SetLast(std::string("NOT IMPLEMENTED: ") + FeatureReason(f));
        return false;
    }
    if (f == F_MONEY) player::SetMoneyLoop(on);
    else player::SetInvincible(on);
    SetLast(std::string(kFeatureNames[f]) + (on ? " ON" : " OFF"));
    return true;
}

bool Enqueue(const Command& c) {
    const Feature f = FeatureOf(c.type);
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_available[f]) {
        g_last = "NOT IMPLEMENTED: " + g_reason[f];
        return false;
    }
    if (g_queue.size() >= 16) {
        g_last = "Busy: too many queued actions";
        return false;
    }
    g_queue.push_back(c);
    g_last = "Queued, runs on the next game frame";
    return true;
}

std::string LastResult() {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_last;
}

void Tick() {
    std::deque<Command> work;
    GameApi api;
    bool moneyAvailable;
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        work.swap(g_queue);
        api = g_api;
        moneyAvailable = g_available[F_MONEY];
    }
    for (const Command& c : work) {
        std::string msg;
        switch (c.type) {
            case Command::AddMoney: player::AddMoneyNow(api, msg); break;
            case Command::Jetpack: player::JetpackNow(api, msg); break;
            case Command::SpawnVehicle: vehicles::SpawnNow(api, c.a, msg); break;
            case Command::GiveWeapon: weapons::GiveNow(api, c.a, c.b, msg); break;
        }
        if (!msg.empty()) SetLast(msg);
    }
    if (moneyAvailable) {
        std::string msg;
        player::TickMoney(api, std::chrono::steady_clock::now(), msg);
        if (!msg.empty()) SetLast(msg);
    }
}

bool BlockDamageFor(void* ped) {
    GameApi api;
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        if (!g_available[F_GOD]) return false;
        api = g_api;
    }
    return player::ShouldBlockDamage(api, ped);
}

std::string DiagnosticsText() {
    std::string s = "NHC diagnostics\n===============\n";
    CompatReport c;
    Resolution res;
    bool avail[F_COUNT];
    std::string reason[F_COUNT];
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        c = g_compat;
        res = g_res;
        for (int f = 0; f < F_COUNT; f++) { avail[f] = g_available[f]; reason[f] = g_reason[f]; }
    }
    s += c.Summary();
    s += "AML config path: " + c.host.configPath + " (file nhc.ini)\n";
    s += "\nGame functions (nhc.ini section Offsets_<Name>, or exported symbol):\n";
    for (int i = 0; i < E_COUNT; i++) {
        const Resolved& r = res.entry[i];
        s += std::string("- ") + kEntries[i].name + " = " + kEntries[i].label + ": ";
        if (r.ok()) {
            char buf[64];
            snprintf(buf, sizeof(buf), " (rva 0x%llx)", static_cast<unsigned long long>(r.address - c.libBase));
            s += "OK via " + r.how + (r.how.rfind("exported", 0) == 0 ? "" : buf);
        } else {
            s += std::string(StateName(r.state)) + " - " + r.detail;
        }
        s += "\n";
    }
    s += std::string("Frame hook installed: ") + (res.frameHookOk ? "yes" : "no") + "\n";
    s += std::string("Damage hook installed: ") + (res.damageHookOk ? "yes" : "no") + "\n";
    s += "\nFeatures:\n";
    for (int f = 0; f < F_COUNT; f++) {
        s += std::string("- ") + kFeatureNames[f] + ": " + (avail[f] ? "READY (not tested in game)" : "NOT IMPLEMENTED - " + reason[f]) + "\n";
    }
    return s;
}

} // namespace nhc
