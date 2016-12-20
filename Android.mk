LOCAL_PATH:= $(call my-dir)

common_src_files := \
	device_info.c	\
	internet_connection.c	\
	devlist_checker_API.c	\
	check.c	\
	reply.c	\
	hardware_checker.c


common_c_includes := \
	$(KERNEL_HEADERS) \
	external/openssl/include

common_shared_libraries := \
	libcutils 	\
	libsysutils \
	libcrypto

include $(CLEAR_VARS)

LOCAL_MODULE:= test_farm

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	test_client.c \
	$(common_src_files)

LOCAL_C_INCLUDES := $(common_c_includes)

LOCAL_LDLIBS := -llog

LOCAL_CFLAGS := 

LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

include $(BUILD_EXECUTABLE)
#include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
