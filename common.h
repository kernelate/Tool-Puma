#ifndef __COMMON_H__
#define __COMMON_H__

#define CUSTOM_LOG 0

//#include <cutils/log.h>
#define CUSTOM_TAG "Autoloaderd"
/*
#if CUSTOM_LOG
    #define DPRINTF(fmt, arg...)    SLOGI(fmt, ## arg)
#else
    #define DPRINTF(...)  \
    ((void)__android_log_buf_print(LOG_ID_SYSTEM, ANDROID_LOG_INFO, CUSTOM_TAG, __VA_ARGS__))
#endif
*/

//#define LOG_TAG "Autoloaderd" 
/* TODO: why do i get LOG_TAG redefined
 *  from cutils/log.h, this changes the logcat TAG 
 */

#endif //__COMMON_H__
