#pragma once
#include "nhc/platform.h"

#include <cstdint>
#include <map>
#include <string>

// A fake game process for the unit tests: in-memory nhc.ini, one fake library, fake symbols.
struct FakeEnv {
    std::map<std::string, std::string> ini;      // "section|key" -> value
    std::map<std::string, uintptr_t> symbols;    // exported symbol -> address
    nhc::plat::HostInfo host;
    std::string libName = "libGame.so";
    uintptr_t libBase = 0;
    size_t libLength = 0;
    bool hookOk = true;
    int hooksInstalled = 0;
    std::string written;                         // last WriteTextFile content
    std::string writtenPath;
    int saves = 0;
    void Reset();
};

FakeEnv& Env();
uint8_t* FakeLib();            // 4096-byte buffer standing in for the mapped game library
