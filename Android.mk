LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := NHC
LOCAL_SRC_FILES := \
    src/main.cpp \
    src/safe_read.cpp \
    src/resolver.cpp \
    src/compat.cpp \
    src/core.cpp \
    src/player.cpp \
    src/vehicles.cpp \
    src/weapons.cpp \
    src/settings.cpp \
    src/menu.cpp \
    src/ui_dex.cpp \
    include/mod/logger.cpp \
    include/mod/config.cpp

# src/platform.cpp is #included by src/main.cpp (single TU for the AML headers).
# include/mod/ is filled by the GitHub Actions workflow from the AML repository.
# generated/nhc_ui_dex.h is produced by the workflow (Java -> DEX -> C array).
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/generated
LOCAL_CPPFLAGS += -O2 -Wall -Wextra -Wno-unused-parameter -DNDEBUG
LOCAL_LDLIBS := -llog
# Android 15+ devices may use 16 KB pages; keep ELF segments compatible.
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384

include $(BUILD_SHARED_LIBRARY)
