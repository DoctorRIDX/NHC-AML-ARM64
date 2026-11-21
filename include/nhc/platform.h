#pragma once
// Thin facade over the AML API. Only platform.cpp and main.cpp include AML headers.
// Everything else talks to this interface, which lets the logic be unit-tested on a PC.
#include <cstddef>
#include <cstdint>
#include <string>

namespace nhc {
namespace plat {

struct HostInfo {
    std::string gamePackage;
    std::string versionName;
    int64_t     versionCode = 0;
    std::string apkMd5;
    int         androidApi  = 0;
    bool        isArm64     = false;
    bool        headerOk    = false;   // compiled against AML mod headers >= 1.4.0
    std::string configPath;
    std::string dataPath;
};

enum LogLevel { LOG_INFO = 0, LOG_WARN = 1, LOG_ERROR = 2 };

void BindAml(void* aml, void* cfg, void* logger);   // called once from OnModPreLoad
void Log(int level, const char* fmt, ...);
HostInfo QueryHost();

uintptr_t LibBase(const char* lib);
size_t    LibLength(const char* lib);
uintptr_t ResolveSymbol(const char* lib, const char* symbol);   // 0 if not exported
bool      SafeRead(uintptr_t addr, void* out, size_t len);      // false instead of crashing
bool      InstallHook(uintptr_t target, void* replacement, void** original);

std::string CfgGetString(const char* section, const char* key);
int         CfgGetInt(const char* section, const char* key, int def);
void        CfgSetInt(const char* section, const char* key, int value);
bool        CfgHasKey(const char* section, const char* key);
void        CfgSave();

bool        WriteTextFile(const std::string& path, const std::string& text);

void* JavaVmPtr();                                         // JavaVM*
void* ActivityObject();                                    // jobject owned by AML
bool  RunOnJavaUiThread(void (*fn)(void*), void* data);

} // namespace plat
} // namespace nhc
