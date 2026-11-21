// Unit tests for the platform-independent logic of NHC. Run: make -C tests
#include "fake_platform.h"

#include "nhc/compat.h"
#include "nhc/core.h"
#include "nhc/features.h"
#include "nhc/menu.h"
#include "nhc/resolver.h"
#include "nhc/safe_read.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <set>

namespace nhc { namespace menu { void StartAsync() {} } }   // the real one needs JNI

static int g_fail = 0, g_checks = 0;
#define CHECK(cond) do { g_checks++; if (!(cond)) { g_fail++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

using namespace nhc;

static std::string Hex(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; i++) { s += d[p[i] >> 4]; s += d[p[i] & 15]; }
    return s;
}

static void AddTableEntry(const char* name, unsigned rva, size_t sigLen = 16) {
    std::string sec = std::string("Offsets_") + name;
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%x", rva);
    Env().ini[sec + "|rva"] = buf;
    Env().ini[sec + "|sig"] = Hex(FakeLib() + rva, sigLen);
}

static void AddBuildSection() {
    Env().ini["Build|version_name"] = "2.11.311";
}

// ---------------------------------------------------------------- tests

static void TestParsing() {
    uint64_t v = 0;
    CHECK(ParseHex("0x1A2b", v) && v == 0x1A2B);
    CHECK(ParseHex("ff", v) && v == 0xFF);
    CHECK(ParseHex("  0x10  ", v) && v == 0x10);
    CHECK(!ParseHex("", v));
    CHECK(!ParseHex("0x", v));
    CHECK(!ParseHex("12g", v));
    CHECK(!ParseHex("0x1ffffffffffffffff", v));

    std::vector<uint8_t> b;
    CHECK(ParseHexBytes("de AD:be ef", b) && b.size() == 4 && b[0] == 0xDE && b[3] == 0xEF);
    CHECK(!ParseHexBytes("abc", b));
    CHECK(!ParseHexBytes("zz", b));
    CHECK(!ParseHexBytes("", b));
}

static void TestSafeRead() {
    uint8_t buf[16];
    CHECK(posix::SafeRead(reinterpret_cast<uintptr_t>(FakeLib()), buf, 16));
    CHECK(!posix::SafeRead(0, buf, 16));
    CHECK(!posix::SafeRead(0x10, buf, 16));
    long page = sysconf(_SC_PAGESIZE);
    void* m = mmap(nullptr, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(m != MAP_FAILED);
    CHECK(!posix::SafeRead(reinterpret_cast<uintptr_t>(m), buf, 16));   // would SIGSEGV with memcpy
    munmap(m, page);
    CHECK(!posix::SafeRead(reinterpret_cast<uintptr_t>(FakeLib()), buf, 0));
    CHECK(!posix::SafeRead(reinterpret_cast<uintptr_t>(FakeLib()), buf, 5000));
}

static const EntryDef kDef = {"FindPlayerPed", "FindPlayerPed(int)", "_Z13FindPlayerPedi,_Zother"};

static void TestResolver() {
    Env().Reset();
    const uintptr_t base = Env().libBase;
    const size_t len = Env().libLength;

    // 1. nothing configured, nothing exported
    Resolved r = ResolveEntry(kDef, "libGame.so", base, len, true);
    CHECK(r.state == EntryState::NoSource);

    // 2. exported symbol (second candidate in the list)
    Env().symbols["_Zother"] = 0x1234;
    r = ResolveEntry(kDef, "libGame.so", base, len, true);
    CHECK(r.ok() && r.address == 0x1234 && r.how.find("_Zother") != std::string::npos);
    Env().symbols.clear();

    // 3. table entry with a matching signature
    AddTableEntry("FindPlayerPed", 0x100);
    r = ResolveEntry(kDef, "libGame.so", base, len, true);
    CHECK(r.ok() && r.address == base + 0x100);

    // 4. same entry but the table belongs to another build
    r = ResolveEntry(kDef, "libGame.so", base, len, false);
    CHECK(r.state == EntryState::BuildMismatch);

    // 5. wrong signature: refused, never used
    Env().ini["Offsets_FindPlayerPed|sig"] = "00112233445566778899aabbccddeeff";
    r = ResolveEntry(kDef, "libGame.so", base, len, true);
    CHECK(r.state == EntryState::SignatureMismatch && r.address == 0);

    // 6. signature missing or too short
    Env().ini.erase("Offsets_FindPlayerPed|sig");
    r = ResolveEntry(kDef, "libGame.so", base, len, true);
    CHECK(r.state == EntryState::BadFormat);
    Env().ini["Offsets_FindPlayerPed|sig"] = "0011";
    r = ResolveEntry(kDef, "libGame.so", base, len, true);
    CHECK(r.state == EntryState::BadFormat);

    // 7. bad rva text, rva outside the library
    AddTableEntry("FindPlayerPed", 0x100);
    Env().ini["Offsets_FindPlayerPed|rva"] = "banana";
    CHECK(ResolveEntry(kDef, "libGame.so", base, len, true).state == EntryState::BadFormat);
    Env().ini["Offsets_FindPlayerPed|rva"] = "0xFFF8";
    CHECK(ResolveEntry(kDef, "libGame.so", base, len, true).state == EntryState::OutOfRange);
    Env().ini["Offsets_FindPlayerPed|rva"] = "0xFFFFFFFFFFFFFFF0";
    CHECK(ResolveEntry(kDef, "libGame.so", base, len, true).state == EntryState::OutOfRange);
}

static void TestCompat() {
    Env().Reset();
    CompatReport c = RunCompatCheck();
    CHECK(c.canRun() && c.libName == "libGame.so" && c.build == BuildStatus::NoTable);
    CHECK(c.Summary().find("not listed") != std::string::npos);
    CHECK(c.Summary().find("NOT DONE") != std::string::npos);

    AddBuildSection();
    CHECK(RunCompatCheck().build == BuildStatus::TableMatches);

    Env().ini["Build|version_name"] = "2.10";
    CHECK(RunCompatCheck().build == BuildStatus::TableMismatch);

    AddBuildSection();
    Env().ini["Build|version_code"] = "311";
    CHECK(RunCompatCheck().build == BuildStatus::TableMatches);
    Env().ini["Build|version_code"] = "999";
    CHECK(RunCompatCheck().build == BuildStatus::TableMismatch);
    Env().ini.erase("Build|version_code");

    Env().host.apkMd5 = "ABCDEF";
    Env().ini["Build|apk_md5"] = "abcdef";
    CHECK(RunCompatCheck().build == BuildStatus::TableMatches);
    Env().ini["Build|apk_md5"] = "123456";
    CHECK(RunCompatCheck().build == BuildStatus::TableMismatch);

    Env().Reset();
    Env().host.isArm64 = false;
    c = RunCompatCheck();
    CHECK(!c.canRun() && c.FirstProblem().find("arm64") != std::string::npos);

    Env().Reset();
    Env().host.androidApi = 25;
    c = RunCompatCheck();
    CHECK(!c.canRun() && !c.uiPossible());

    Env().Reset();
    Env().host.gamePackage = "com.example.other";
    CHECK(!RunCompatCheck().canRun());

    Env().Reset();
    Env().libName = "libother.so";
    c = RunCompatCheck();
    CHECK(!c.libOk && !c.canRun());

    Env().Reset();
    Env().libName = "libGTASA.so";   // 32-bit era name, still detected
    CHECK(RunCompatCheck().libName == "libGTASA.so");
}

static void TestCatalogs() {
    std::set<int> ids;
    for (const CatalogItem& v : VehicleCatalog()) {
        CHECK(v.id >= 400 && v.id <= 611);
        CHECK(ids.insert(v.id).second);   // no duplicate model ids
        CHECK(v.name && *v.name);
    }
    CHECK(VehicleCatalog().size() > 100);
    ids.clear();
    for (const CatalogItem& w : WeaponCatalog()) {
        CHECK(w.id >= 1 && w.id <= 46);
        CHECK(ids.insert(w.id).second);
        CHECK(w.defaultAmmo >= 1 && w.defaultAmmo <= 9999);
    }
}

// ---- fake game functions used by the feature tests
static int g_moneyCalls, g_jetCalls, g_vehCalls, g_vehModel, g_giveCalls, g_giveWeapon, g_giveAmmo;
static void* g_ped = nullptr;
static void* FakeFindPlayerPed(int) { return g_ped; }
static void FakeMoney() { g_moneyCalls++; }
static void FakeJet() { g_jetCalls++; }
static void FakeVehicle(int m) { g_vehCalls++; g_vehModel = m; }
static int FakeGive(void* ped, int w, unsigned ammo, bool) { (void)ped; g_giveCalls++; g_giveWeapon = w; g_giveAmmo = static_cast<int>(ammo); return 0; }

static GameApi FakeApi() {
    GameApi a;
    a.FindPlayerPed = FakeFindPlayerPed;
    a.MoneyArmourHealthCheat = FakeMoney;
    a.JetpackCheat = FakeJet;
    a.VehicleCheat = FakeVehicle;
    a.GiveWeapon = FakeGive;
    return a;
}

static Resolution AllOk() {
    Resolution r;
    for (int i = 0; i < E_COUNT; i++) { r.entry[i].state = EntryState::Ok; r.entry[i].address = 1; }
    r.frameHookOk = true;
    r.damageHookOk = true;
    return r;
}

static void ResetCounters() { g_moneyCalls = g_jetCalls = g_vehCalls = g_vehModel = g_giveCalls = g_giveWeapon = g_giveAmmo = 0; }

static void TestFeatures() {
    Env().Reset();
    int dummyPed = 0;
    g_ped = &dummyPed;
    ResetCounters();
    player::SetMoneyLoop(false);
    player::SetInvincible(false);

    ApplyResolution(RunCompatCheck(), AllOk(), FakeApi());
    for (int f = 0; f < F_COUNT; f++) CHECK(FeatureAvailable(static_cast<Feature>(f)) && FeatureReason(static_cast<Feature>(f)).empty());

    // queued commands run only when the game thread ticks
    Command c; c.type = Command::SpawnVehicle; c.a = 411;
    CHECK(Enqueue(c));
    CHECK(g_vehCalls == 0);
    Tick();
    CHECK(g_vehCalls == 1 && g_vehModel == 411);
    CHECK(LastResult().find("411") != std::string::npos);

    // unknown ids never reach the game
    c.a = 9999;
    CHECK(Enqueue(c));
    Tick();
    CHECK(g_vehCalls == 1 && LastResult().find("unknown") != std::string::npos);

    Command w; w.type = Command::GiveWeapon; w.a = 31; w.b = 250;
    CHECK(Enqueue(w));
    Tick();
    CHECK(g_giveCalls == 1 && g_giveWeapon == 31 && g_giveAmmo == 250);
    w.a = 31; w.b = 123456;    // ammo is clamped
    Enqueue(w); Tick();
    CHECK(g_giveAmmo == 9999);
    w.a = 999;                  // unknown weapon
    Enqueue(w); Tick();
    CHECK(g_giveCalls == 2);

    Command j; j.type = Command::Jetpack;
    CHECK(Enqueue(j)); Tick();
    CHECK(g_jetCalls == 1);

    Command m; m.type = Command::AddMoney;
    CHECK(Enqueue(m)); Tick();
    CHECK(g_moneyCalls == 1);

    // no player in the world (menu / loading): nothing is called, user sees why
    g_ped = nullptr;
    CHECK(Enqueue(m)); Tick();
    CHECK(g_moneyCalls == 1 && LastResult().find("no player") != std::string::npos);
    g_ped = &dummyPed;

    // money loop: first tick fires, then once every kMoneyIntervalSeconds
    g_moneyCalls = 0;
    std::string msg;
    auto t0 = std::chrono::steady_clock::now();
    GameApi api = FakeApi();
    player::SetMoneyLoop(true);
    player::TickMoney(api, t0, msg);
    CHECK(g_moneyCalls == 1);
    player::TickMoney(api, t0 + std::chrono::seconds(1), msg);
    CHECK(g_moneyCalls == 1);
    player::TickMoney(api, t0 + std::chrono::seconds(player::kMoneyIntervalSeconds), msg);
    CHECK(g_moneyCalls == 2);
    player::SetMoneyLoop(false);
    player::TickMoney(api, t0 + std::chrono::seconds(60), msg);
    CHECK(g_moneyCalls == 2);

    // invincibility only blocks damage to the player ped
    CHECK(!BlockDamageFor(&dummyPed));              // off
    CHECK(SetFeatureEnabled(F_GOD, true));
    CHECK(BlockDamageFor(&dummyPed));
    int other = 0;
    CHECK(!BlockDamageFor(&other));
    CHECK(!BlockDamageFor(nullptr));
    CHECK(SetFeatureEnabled(F_GOD, false));
    CHECK(!BlockDamageFor(&dummyPed));
}

static void TestUnavailable() {
    Env().Reset();
    ResetCounters();
    player::SetMoneyLoop(false);
    player::SetInvincible(false);

    // everything resolved except the frame hook: game-thread features must refuse
    Resolution r = AllOk();
    r.frameHookOk = false;
    ApplyResolution(RunCompatCheck(), r, FakeApi());
    CHECK(!FeatureAvailable(F_MONEY) && FeatureReason(F_MONEY).find("Frame hook") != std::string::npos);
    CHECK(FeatureAvailable(F_GOD));
    Command c; c.type = Command::SpawnVehicle; c.a = 411;
    CHECK(!Enqueue(c));
    CHECK(LastResult().find("NOT IMPLEMENTED") != std::string::npos);
    Tick();
    CHECK(g_vehCalls == 0);
    CHECK(!SetFeatureEnabled(F_MONEY, true) && !FeatureEnabled(F_MONEY));

    // one entry missing: the dependent features name it
    r = AllOk();
    r.entry[E_GIVEWEAPON].state = EntryState::NoSource;
    ApplyResolution(RunCompatCheck(), r, FakeApi());
    CHECK(!FeatureAvailable(F_WEAPON) && FeatureReason(F_WEAPON).find("GiveWeapon") != std::string::npos);
    CHECK(FeatureAvailable(F_VEHICLE));

    // failed prerequisite (wrong ABI) disables everything
    Env().host.isArm64 = false;
    ApplyResolution(RunCompatCheck(), AllOk(), FakeApi());
    for (int f = 0; f < F_COUNT; f++) CHECK(!FeatureAvailable(static_cast<Feature>(f)));
    CHECK(FeatureReason(F_GOD).find("arm64") != std::string::npos);
}

static void TestStartEndToEnd() {
    // A) fresh install: no nhc.ini content -> every feature is NOT IMPLEMENTED, honestly
    Env().Reset();
    Start();
    for (int f = 0; f < F_COUNT; f++) CHECK(!FeatureAvailable(static_cast<Feature>(f)));
    CHECK(Env().hooksInstalled == 0);
    CHECK(Env().writtenPath == "/fake/data/files/NHC_diagnostics.txt");
    CHECK(Env().written.find("NOT IMPLEMENTED") != std::string::npos);
    CHECK(Env().written.find("/fake/config/") != std::string::npos);
    CHECK(Env().ini.count("Settings|UiScale") == 1 && Env().saves > 0);   // nhc.ini gets created

    // B) a verified table for this build + matching signatures -> features become ready, hooks installed
    Env().Reset();
    AddBuildSection();
    AddTableEntry("FindPlayerPed", 0x100);
    AddTableEntry("CGame_Process", 0x200);
    AddTableEntry("CCheat_MoneyArmourHealthCheat", 0x300);
    AddTableEntry("CCheat_JetpackCheat", 0x400);
    AddTableEntry("CCheat_VehicleCheat", 0x500);
    AddTableEntry("CPed_GiveWeapon", 0x600);
    AddTableEntry("CPed_InflictDamage", 0x700);
    Start();
    for (int f = 0; f < F_COUNT; f++) CHECK(FeatureAvailable(static_cast<Feature>(f)));
    CHECK(Env().hooksInstalled == 2);
    CHECK(Env().written.find("READY (not tested in game)") != std::string::npos);

    // C) same table but for a different game build -> ignored
    Env().ini["Build|version_name"] = "2.10";
    Start();
    for (int f = 0; f < F_COUNT; f++) CHECK(!FeatureAvailable(static_cast<Feature>(f)));

    // D) one corrupted signature disables only what depends on it
    Env().ini["Build|version_name"] = "2.11.311";
    Env().ini["Offsets_CCheat_JetpackCheat|sig"] = "ffffffffffffffffffffffffffffffff";
    Start();
    CHECK(!FeatureAvailable(F_JETPACK) && FeatureAvailable(F_VEHICLE));

    // E) hook refused by AML -> features that need it stay off
    Env().hookOk = false;
    Start();
    CHECK(!FeatureAvailable(F_MONEY) && !FeatureAvailable(F_GOD));
}

static void TestSettings() {
    Env().Reset();
    CHECK(settings::Get("UiScale", 100) == 100);
    settings::Set("UiScale", 500);
    CHECK(settings::Get("UiScale", 100) == 160);
    settings::Set("UiScale", 10);
    CHECK(settings::Get("UiScale", 100) == 60);
    settings::Set("Theme", 7);
    CHECK(settings::Get("Theme", 0) == 2);
    settings::Set("NotAKey", 5);
    CHECK(Env().ini.count("Settings|NotAKey") == 0);
    CHECK(settings::Get("NotAKey", 42) == 42);
}

int main() {
    TestParsing();
    TestSafeRead();
    TestResolver();
    TestCompat();
    TestCatalogs();
    TestFeatures();
    TestUnavailable();
    TestStartEndToEnd();
    TestSettings();
    printf("%d checks, %d failed\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
