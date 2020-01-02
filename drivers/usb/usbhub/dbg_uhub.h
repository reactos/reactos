/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBHub debugging declarations
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef DBG_UHUB_H__
#define DBG_UHUB_H__

#if DBG

    #ifndef NDEBUG_USBHUB_IOCTL

        #define DPRINT_IOCTL(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_IOCTL   __noop
#else
        #define DPRINT_IOCTL(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBHUB_POWER

        #define DPRINT_PWR(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_PWR   __noop
#else
        #define DPRINT_PWR(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBHUB_PNP

        #define DPRINT_PNP(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_PNP   __noop
#else
        #define DPRINT_PNP(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBHUB_SCE

        #define DPRINT_SCE(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_SCE   __noop
#else
        #define DPRINT_SCE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

    #ifndef NDEBUG_USBHUB_ENUM

        #define DPRINT_ENUM(fmt, ...) do { \
            if (DbgPrint("(%s:%d) " fmt, __RELFILE__, __LINE__, ##__VA_ARGS__))  \
                DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
        } while (0)

    #else

#if defined(_MSC_VER)
        #define DPRINT_ENUM  __noop
#else
        #define DPRINT_ENUM(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

    #endif

#else /* not DBG */

#if defined(_MSC_VER)
    #define DPRINT_IOCTL  __noop
    #define DPRINT_PWR    __noop
    #define DPRINT_PNP    __noop
    #define DPRINT_SCE    __noop
    #define DPRINT_ENUM   __noop
#else
    #define DPRINT_IOCTL(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_PWR(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_PNP(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_SCE(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT_ENUM(...)  do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif /* _MSC_VER */

#endif /* not DBG */

#endif /* DBG_UHUB_H__ */
