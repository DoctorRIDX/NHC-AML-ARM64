#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace nhc {

enum class EntryState {
    Ok,
    NoSource,            // no table entry and no exported symbol found
    BadFormat,
    BuildMismatch,       // table exists but belongs to another game build
    OutOfRange,
    Unreadable,
    SignatureMismatch,
};

struct Resolved {
    EntryState  state = EntryState::NoSource;
    uintptr_t   address = 0;
    std::string how;      // "table rva+signature" / "exported symbol X"
    std::string detail;   // human readable reason when not Ok
    bool ok() const { return state == EntryState::Ok; }
};

// name: key used in nhc.ini as section "Offsets_<name>".
// symbols: comma separated mangled-name candidates tried when the table has no entry.
struct EntryDef {
    const char* name;
    const char* label;     // human name shown in the menu
    const char* symbols;
};

bool ParseHex(const std::string& text, uint64_t& out);
bool ParseHexBytes(const std::string& text, std::vector<uint8_t>& out);
const char* StateName(EntryState s);

// tableUsable: false when [Build] in nhc.ini does not match the running game build.
Resolved ResolveEntry(const EntryDef& def, const std::string& lib,
                      uintptr_t libBase, size_t libLen, bool tableUsable);

} // namespace nhc
