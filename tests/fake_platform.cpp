#include "fake_platform.h"

#include "nhc/safe_read.h"

#include <cstdarg>
#include <cstdio>

namespace {
alignas(16) uint8_t g_lib[4096];
}

uint8_t* FakeLib() { return g_lib; }

FakeEnv& Env() {
    static FakeEnv e;
    return e;
}

void FakeEnv::Reset() {
    for (size_t i = 0; i < sizeof(g_lib); i++) g_lib[i] = static_cast<uint8_t>((i * 7 + 3) & 0xFF);
    ini.clear();
    symbols.clear();
    host = nhc::plat::HostInfo();
    host.gamePackage = "com.rockstargames.gtasa";
    host.versionName = "2.11.311";
    host.versionCode = 311;
    host.androidApi = 35;
    host.isArm64 = true;
    host.headerOk = true;
    host.configPath = "/fake/config/";
    host.dataPath = "/fake/data/files";
    libName = "libGame.so";
    libBase = reinterpret_cast<uintptr_t>(g_lib);
    libLength = sizeof(g_lib);
    hookOk = true;
    hooksInstalled = 0;
    written.clear();
    writtenPath.clear();
    saves = 0;
}

namespace nhc {
namespace plat {

void BindAml(void*, void*, void*) {}

void Log(int, const char* fmt, ...) {
    (void)fmt;   // silent in tests
}

HostInfo QueryHost() { return Env().host; }

uintptr_t LibBase(const char* lib) { return Env().libName == lib ? Env().libBase : 0; }
size_t LibLength(const char* lib) { return Env().libName == lib ? Env().libLength : 0; }

uintptr_t ResolveSymbol(const char*, const char* symbol) {
    auto it = Env().symbols.find(symbol);
    return it == Env().symbols.end() ? 0 : it->second;
}

bool SafeRead(uintptr_t addr, void* out, size_t len) { return posix::SafeRead(addr, out, len); }

bool InstallHook(uintptr_t, void*, void**) {
    if (Env().hookOk) Env().hooksInstalled++;
    return Env().hookOk;
}

static std::string Key(const char* section, const char* key) { return std::string(section) + "|" + key; }

std::string CfgGetString(const char* section, const char* key) {
    auto it = Env().ini.find(Key(section, key));
    return it == Env().ini.end() ? "" : it->second;
}

int CfgGetInt(const char* section, const char* key, int def) {
    auto it = Env().ini.find(Key(section, key));
    return it == Env().ini.end() ? def : atoi(it->second.c_str());
}

void CfgSetInt(const char* section, const char* key, int value) { Env().ini[Key(section, key)] = std::to_string(value); }

bool CfgHasKey(const char* section, const char* key) { return Env().ini.count(Key(section, key)) != 0; }

void CfgSave() { Env().saves++; }

bool WriteTextFile(const std::string& path, const std::string& text) {
    Env().writtenPath = path;
    Env().written = text;
    return true;
}

void* JavaVmPtr() { return nullptr; }
void* ActivityObject() { return nullptr; }
bool RunOnJavaUiThread(void (*)(void*), void*) { return false; }

} // namespace plat
} // namespace nhc
