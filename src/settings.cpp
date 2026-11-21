#include "nhc/features.h"
#include "nhc/platform.h"

namespace nhc {
namespace settings {
namespace {

struct Spec { const char* key; int lo; int hi; int def; };
const Spec kSpecs[] = {
    {"UiScale", 60, 160, 100},
    {"Theme", 0, 2, 0},
    {"Visibility", 0, 2, 0},   // 0 = swipe + floating button, 1 = floating button only, 2 = swipe only
};

const Spec* Find(const std::string& key) {
    for (const Spec& s : kSpecs)
        if (key == s.key) return &s;
    return nullptr;
}

int Clamp(const Spec& s, int v) { return v < s.lo ? s.lo : (v > s.hi ? s.hi : v); }

} // namespace

int Get(const std::string& key, int def) {
    const Spec* s = Find(key);
    if (!s) return def;
    return Clamp(*s, plat::CfgGetInt("Settings", key.c_str(), s->def));
}

void Set(const std::string& key, int value) {
    const Spec* s = Find(key);
    if (!s) return;
    plat::CfgSetInt("Settings", key.c_str(), Clamp(*s, value));
    plat::CfgSave();
}

} // namespace settings
} // namespace nhc
