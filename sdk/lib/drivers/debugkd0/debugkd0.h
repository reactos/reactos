/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            lib/drivers/debugkd0/debugkd0.h
 * PURPOSE:         Kernel phase 0 debug library header
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef _DEBUGKD0_H
#define _DEBUGKD0_H

/* Global control */
#if DBG
  #define DBG_KD0 0 // Default disabled
#else
  #define DBG_KD0 0 // Default disabled
#endif

#define COM1 1
#define COM2 2
#define COM3 3
#define COM4 4

#define COM1_ADDR (PUCHAR)0x3F8
#define COM2_ADDR (PUCHAR)0x2F8
#define COM3_ADDR (PUCHAR)0x3E8
#define COM4_ADDR (PUCHAR)0x2E8

#define DEFAULT_COM_BAUD_RATE 19200
#define DBG_COM_BAUD_RATE 115200

BOOLEAN
NTAPI
DbgKdPortInitialize(_In_ ULONG PortNumber OPTIONAL,
                    _In_ PUCHAR PortAddress OPTIONAL,
                    _In_ ULONG BaudRate OPTIONAL,
                    _Out_ PULONG OutPortId);

#if DBG

  #if DBG_KD0

    #include <cportlib/cportlib.h>
    #include <stdio.h>

    ULONG __cdecl DbgKdPrint0(_In_ PCHAR Format, ...);

    #ifndef __FILENAME__
      #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #endif

    #define DbgPrint0(fmt, ...) do { \
      if (DbgKdPrint0("(%s:%d) " fmt, __FILENAME__, __LINE__, ##__VA_ARGS__))  \
          DbgKdPrint0("(%s:%d) DbgKdPrint0() failed!\n", __FILENAME__, __LINE__); \
    } while (0)

  #else /* (#if DBG_KD0) */

    #if defined(_MSC_VER)
      #define DbgPrint0 __noop
    #else
      #define DbgPrint0
    #endif

  #endif /* (#if DBG_KD0) */

#else /* (#if DBG) */

  #if defined(_MSC_VER)
    #define DbgPrint0 __noop
  #else
    #define DbgPrint0
  #endif

#endif /* (#if DBG) */

#endif  /* _DEBUGKD0_H */
