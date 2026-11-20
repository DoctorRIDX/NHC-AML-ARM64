LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := nhc
LOCAL_SRC_FILES := main.cpp mod/logger.cpp mod/config.cpp mod/flogger.cpp
LOCAL_CPPFLAGS += -O2 -DNDEBUG -std=c++17
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)
