#include "nhc/ui_dex.h"

// generated/nhc_ui_dex.h is created by tools/build_ui_dex.sh (the GitHub Actions workflow runs it).
#if __has_include("nhc_ui_dex.h")
#include "nhc_ui_dex.h"
#else
#error "generated/nhc_ui_dex.h is missing. Run tools/build_ui_dex.sh (needs JDK 17 and the Android SDK) or use the GitHub Actions workflow."
#endif

namespace nhc {
const unsigned char* UiDexData() { return nhc_ui_dex; }
size_t UiDexSize() { return nhc_ui_dex_len; }
}
