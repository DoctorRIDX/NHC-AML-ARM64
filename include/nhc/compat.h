#pragma once
#include "nhc/platform.h"

#include <string>

namespace nhc {

constexpr int kMinAndroidApi = 26;   // InMemoryDexClassLoader (menu UI) needs API 26+
constexpr const char* kGamePackage = "com.rockstargames.gtasa";

enum class BuildStatus { NoTable, TableMatches, TableMismatch };

struct CompatReport {
    plat::HostInfo host;
    bool abiOk = false;
    bool amlHeaderOk = false;
    bool gameOk = false;
    bool androidOk = false;
    bool libOk = false;
    std::string libName;
    uintptr_t libBase = 0;
    size_t libLength = 0;
    BuildStatus build = BuildStatus::NoTable;

    bool canRun() const { return abiOk && amlHeaderOk && gameOk && androidOk && libOk; }
    bool uiPossible() const { return abiOk && androidOk; }
    std::string FirstProblem() const;   // empty when canRun()
    std::string Summary() const;        // multi-line text for the menu and the log
};

CompatReport RunCompatCheck();

} // namespace nhc
