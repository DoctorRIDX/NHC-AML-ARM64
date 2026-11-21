#pragma once
#include <cstddef>
#include <cstdint>

namespace nhc {
namespace posix {
// Reads len bytes (max 4096) from an arbitrary address without risking SIGSEGV:
// the kernel copies the bytes through a pipe and reports EFAULT for unmapped memory.
bool SafeRead(uintptr_t addr, void* out, size_t len);
}
}
