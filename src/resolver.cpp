#include "nhc/resolver.h"
#include "nhc/platform.h"

#include <cctype>
#include <cstdlib>

namespace nhc {

bool ParseHex(const std::string& text, uint64_t& out) {
    size_t i = 0;
    while (i < text.size() && isspace(static_cast<unsigned char>(text[i]))) i++;
    if (i + 1 < text.size() && text[i] == '0' && (text[i + 1] == 'x' || text[i + 1] == 'X')) i += 2;
    size_t start = i;
    uint64_t v = 0;
    for (; i < text.size() && isxdigit(static_cast<unsigned char>(text[i])); i++) {
        char c = text[i];
        int d = (c >= '0' && c <= '9') ? c - '0' : (tolower(c) - 'a' + 10);
        if (v >> 60) return false;   // would overflow
        v = (v << 4) | static_cast<uint64_t>(d);
    }
    while (i < text.size() && isspace(static_cast<unsigned char>(text[i]))) i++;
    if (i != text.size() || i == start) return false;
    out = v;
    return true;
}

bool ParseHexBytes(const std::string& text, std::vector<uint8_t>& out) {
    out.clear();
    std::string digits;
    for (char c : text) {
        if (isspace(static_cast<unsigned char>(c)) || c == ':') continue;
        if (!isxdigit(static_cast<unsigned char>(c))) return false;
        digits.push_back(c);
    }
    if (digits.empty() || digits.size() % 2 != 0) return false;
    for (size_t i = 0; i < digits.size(); i += 2) {
        uint64_t b = 0;
        if (!ParseHex(digits.substr(i, 2), b)) return false;
        out.push_back(static_cast<uint8_t>(b));
    }
    return true;
}

const char* StateName(EntryState s) {
    switch (s) {
        case EntryState::Ok: return "OK";
        case EntryState::NoSource: return "no table entry / symbol not exported";
        case EntryState::BadFormat: return "bad format in nhc.ini";
        case EntryState::BuildMismatch: return "table belongs to another build";
        case EntryState::OutOfRange: return "rva outside the game library";
        case EntryState::Unreadable: return "memory not readable";
        case EntryState::SignatureMismatch: return "signature mismatch";
    }
    return "?";
}

static Resolved Fail(EntryState s, const std::string& detail) {
    Resolved r;
    r.state = s;
    r.detail = detail;
    return r;
}

Resolved ResolveEntry(const EntryDef& def, const std::string& lib,
                      uintptr_t libBase, size_t libLen, bool tableUsable) {
    const std::string section = std::string("Offsets_") + def.name;

    if (plat::CfgHasKey(section.c_str(), "rva")) {
        if (!tableUsable)
            return Fail(EntryState::BuildMismatch, "nhc.ini [Build] does not match this game build");

        uint64_t rva = 0;
        if (!ParseHex(plat::CfgGetString(section.c_str(), "rva"), rva))
            return Fail(EntryState::BadFormat, "rva is not a hex number");

        std::vector<uint8_t> sig;
        if (!ParseHexBytes(plat::CfgGetString(section.c_str(), "sig"), sig) || sig.size() < 8 || sig.size() > 32)
            return Fail(EntryState::BadFormat, "sig must be 8-32 bytes of hex (mandatory)");

        if (rva > libLen || sig.size() > libLen - rva)
            return Fail(EntryState::OutOfRange, "rva is beyond the mapped library");

        uint8_t actual[32] = {0};
        if (!plat::SafeRead(libBase + static_cast<uintptr_t>(rva), actual, sig.size()))
            return Fail(EntryState::Unreadable, "cannot read bytes at rva");

        for (size_t i = 0; i < sig.size(); i++)
            if (actual[i] != sig[i])
                return Fail(EntryState::SignatureMismatch, "bytes at rva differ from sig");

        Resolved r;
        r.state = EntryState::Ok;
        r.address = libBase + static_cast<uintptr_t>(rva);
        r.how = "table rva+signature";
        return r;
    }

    // No table entry: try exported symbols (user override first, then built-in candidates).
    std::string candidates;
    if (plat::CfgHasKey(section.c_str(), "symbol"))
        candidates = plat::CfgGetString(section.c_str(), "symbol");
    if (def.symbols && *def.symbols) {
        if (!candidates.empty()) candidates += ",";
        candidates += def.symbols;
    }

    size_t tried = 0, pos = 0;
    while (pos <= candidates.size()) {
        size_t comma = candidates.find(',', pos);
        std::string sym = candidates.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        if (!sym.empty()) {
            tried++;
            uintptr_t addr = plat::ResolveSymbol(lib.c_str(), sym.c_str());
            if (addr) {
                Resolved r;
                r.state = EntryState::Ok;
                r.address = addr;
                r.how = "exported symbol " + sym;
                return r;
            }
        }
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return Fail(EntryState::NoSource,
                "no [" + section + "] in nhc.ini and none of " + std::to_string(tried) + " candidate symbols is exported");
}

} // namespace nhc
