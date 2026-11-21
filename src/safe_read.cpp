#include "nhc/safe_read.h"

#include <unistd.h>

namespace nhc {
namespace posix {

bool SafeRead(uintptr_t addr, void* out, size_t len) {
    if (len == 0 || len > 4096 || addr == 0) return false;
    int fds[2];
    if (pipe(fds) != 0) return false;
    bool ok = false;
    ssize_t w = write(fds[1], reinterpret_cast<const void*>(addr), len);
    if (w == static_cast<ssize_t>(len)) {
        ssize_t r = read(fds[0], out, len);
        ok = (r == static_cast<ssize_t>(len));
    }
    close(fds[0]);
    close(fds[1]);
    return ok;
}

} // namespace posix
} // namespace nhc
