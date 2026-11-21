// The only code that touches the AML API. Compiled as part of main.cpp (#include), see main.cpp.
// Every AML call below is taken from the official AML 1.4.0 mod documentation
// (docs/DOCUMENTATION_EN.MD in https://github.com/RusJJ/AndroidModLoader).
#include "nhc/platform.h"
#include "nhc/safe_read.h"

#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>

#include <cstdarg>
#include <cstdio>
#include <string>

namespace {
IAML*   s_aml = nullptr;
Config* s_cfg = nullptr;
Logger* s_log = nullptr;

std::string Str(const char* p) { return std::string(p ? p : ""); }
}

namespace nhc {
namespace plat {

void BindAml(void* aml, void* cfg, void* logger) {
    s_aml = static_cast<IAML*>(aml);
    s_cfg = static_cast<Config*>(cfg);
    s_log = static_cast<Logger*>(logger);
}

void Log(int level, const char* fmt, ...) {
    if (!s_log) return;
    char buf[2000];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (level == LOG_ERROR) s_log->Error("%s", buf);
    else if (level == LOG_WARN) s_log->Print(LogP_Warn, "%s", buf);
    else s_log->Info("%s", buf);
}

HostInfo QueryHost() {
    HostInfo h;
    if (!s_aml) return h;
    h.gamePackage = Str(s_aml->GetCurrentGame());
    h.versionName = Str(s_aml->GetAppVersionName());
    h.versionCode = s_aml->GetAppVersionCode();
    h.apkMd5 = Str(s_aml->GetApkMD5());
    h.androidApi = s_aml->GetAndroidVersion();
#if defined(__aarch64__)
    h.isArm64 = true;
#endif
#if defined(IAML_VER) && IAML_VER >= 01040000
    h.headerOk = true;
#endif
    h.configPath = Str(s_aml->GetConfigPath());
    h.dataPath = Str(s_aml->GetAndroidDataPath());
    return h;
}

uintptr_t LibBase(const char* lib) { return s_aml ? s_aml->GetLib(lib) : 0; }
size_t LibLength(const char* lib) { return s_aml ? static_cast<size_t>(s_aml->GetLibLength(lib)) : 0; }

uintptr_t ResolveSymbol(const char* lib, const char* symbol) {
    if (!s_aml) return 0;
    void* handle = s_aml->GetLibHandle(lib);
    if (!handle) return 0;
    return s_aml->GetSym(handle, symbol);
}

bool SafeRead(uintptr_t addr, void* out, size_t len) { return posix::SafeRead(addr, out, len); }

bool InstallHook(uintptr_t target, void* replacement, void** original) {
    return s_aml && s_aml->Hook(reinterpret_cast<void*>(target), replacement, original);
}

std::string CfgGetString(const char* section, const char* key) {
    if (!s_cfg) return "";
    char buf[256];
    buf[0] = '\0';
    s_cfg->GetString(buf, sizeof(buf), key, "", section);
    return buf;
}

int CfgGetInt(const char* section, const char* key, int def) {
    return s_cfg ? s_cfg->GetInt(key, def, section) : def;
}

void CfgSetInt(const char* section, const char* key, int value) {
    if (!s_cfg) return;
    ConfigEntry* e = s_cfg->Bind(key, value, section);
    e->SetInt(value);
    delete e;
}

bool CfgHasKey(const char* section, const char* key) { return s_cfg && s_cfg->HasKey(section, key); }

void CfgSave() {
    if (s_cfg) s_cfg->Save();
}

bool WriteTextFile(const std::string& path, const std::string& text) {
    if (!s_aml) return false;
    size_t slash = path.find_last_of('/');
    if (slash != std::string::npos) s_aml->CreateDirRecursive(path.substr(0, slash).c_str());
    return s_aml->WriteBufferToFile(path.c_str(), text.data(), text.size());
}

void* JavaVmPtr() { return s_aml ? static_cast<void*>(s_aml->GetJavaVM()) : nullptr; }
void* ActivityObject() { return s_aml ? static_cast<void*>(s_aml->GetCurrentActivity()) : nullptr; }

bool RunOnJavaUiThread(void (*fn)(void*), void* data) {
    return s_aml && s_aml->PushToJavaUIThread(fn, data);
}

} // namespace plat
} // namespace nhc
