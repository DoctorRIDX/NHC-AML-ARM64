#include "nhc/compat.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace nhc {
namespace {

bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++)
        if (tolower(static_cast<unsigned char>(a[i])) != tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}

std::string Hex(uintptr_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(v));
    return buf;
}

const char* YesNo(bool ok) { return ok ? "OK" : "FAIL"; }

} // namespace

CompatReport RunCompatCheck() {
    CompatReport r;
    r.host = plat::QueryHost();
    r.abiOk = r.host.isArm64;
    r.amlHeaderOk = r.host.headerOk;
    r.gameOk = (r.host.gamePackage == kGamePackage);
    r.androidOk = (r.host.androidApi >= kMinAndroidApi);

    static const char* const kLibs[] = {"libGame.so", "libGTASA.so"};
    for (const char* lib : kLibs) {
        uintptr_t base = plat::LibBase(lib);
        if (!base) continue;
        r.libName = lib;
        r.libBase = base;
        r.libLength = plat::LibLength(lib);
        r.libOk = (r.libLength > 0);
        break;
    }

    // An offset table is only trusted when it names the exact build that is running.
    if (!plat::CfgHasKey("Build", "version_name")) {
        r.build = BuildStatus::NoTable;
    } else {
        bool match = (plat::CfgGetString("Build", "version_name") == r.host.versionName);
        if (match && plat::CfgHasKey("Build", "version_code")) {
            const std::string code = plat::CfgGetString("Build", "version_code");
            char* end = nullptr;
            long long v = strtoll(code.c_str(), &end, 10);
            match = (end != code.c_str() && *end == '\0' && v == static_cast<long long>(r.host.versionCode));
        }
        if (match && plat::CfgHasKey("Build", "apk_md5") && !r.host.apkMd5.empty())
            match = EqualsIgnoreCase(plat::CfgGetString("Build", "apk_md5"), r.host.apkMd5);
        r.build = match ? BuildStatus::TableMatches : BuildStatus::TableMismatch;
    }
    return r;
}

std::string CompatReport::FirstProblem() const {
    if (!abiOk) return "ABI is not arm64-v8a";
    if (!amlHeaderOk) return "built against AML mod headers older than 1.4.0";
    if (!gameOk) return "game package is '" + host.gamePackage + "', expected " + kGamePackage;
    if (!androidOk) return "Android API " + std::to_string(host.androidApi) + " is below " + std::to_string(kMinAndroidApi);
    if (!libOk) return "game library (libGame.so / libGTASA.so) not found in the process";
    return "";
}

std::string CompatReport::Summary() const {
    std::string s;
    s += "ABI: " + std::string(abiOk ? "arm64-v8a" : "not arm64") + " [" + YesNo(abiOk) + "]\n";
    s += "AML mod headers 1.4.0+: [" + std::string(YesNo(amlHeaderOk)) + "]\n";
    s += "Android API " + std::to_string(host.androidApi) + " (needs " + std::to_string(kMinAndroidApi) + "+): [" + YesNo(androidOk) + "]\n";
    s += "Game package: " + host.gamePackage + " [" + YesNo(gameOk) + "]\n";
    s += "Game version: " + host.versionName + " (code " + std::to_string(host.versionCode) + ")\n";
    if (libOk)
        s += "Game library: " + libName + " base " + Hex(libBase) + " length " + Hex(libLength) + "\n";
    else
        s += "Game library: NOT FOUND [FAIL]\n";

    switch (build) {
        case BuildStatus::NoTable:
            s += "Offset table (nhc.ini [Build]): none. Only exported game symbols can be used.\n"; break;
        case BuildStatus::TableMatches:
            s += "Offset table (nhc.ini [Build]): matches this game build. Every entry is still signature-checked.\n"; break;
        case BuildStatus::TableMismatch:
            s += "Offset table (nhc.ini [Build]): belongs to ANOTHER build. Table entries are ignored.\n"; break;
    }
    if (host.versionName == "2.10")
        s += "AML ecosystem: aml-psdk lists GTA SA 2.10 (64-bit).\n";
    else
        s += "AML ecosystem: aml-psdk lists only GTA SA 2.00 and 2.10 (64-bit); this build is not listed.\n";
    s += "In-game testing of NHC: NOT DONE.\n";
    return s;
}

} // namespace nhc
