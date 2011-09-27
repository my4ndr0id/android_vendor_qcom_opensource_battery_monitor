ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
commonSources := battery_file_access.c

include $(CLEAR_VARS)
LOCAL_MODULE := battery_monitor
LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_FLAGS := -lstringl
LOCAL_SRC_FILES += $(commonSources) battery_monitor.c

LOCAL_SHARED_LIBRARIES := \
        libc \
        libcutils \
        libutils

LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := battery_shutdown
LOCAL_C_INCLUDES := $(TARGET_OUT_HEADERS)/common/inc
LOCAL_C_FLAGS := -lstringl
LOCAL_SRC_FILES += $(commonSources) battery_shutdown.c

LOCAL_SHARED_LIBRARIES := \
        libc \
        libcutils \
        libutils

LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

endif
