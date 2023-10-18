/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug support header file
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

#if DBG

// #define DEBUG_TRACE
// #define DEBUG_INFO
#define DEBUG_INFO_VERB
#define DEBUG_WARN
#define DEBUG_ERR

#ifdef DEBUG_TRACE
#define TRACE(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define TRACE
#endif

#ifdef DEBUG_INFO
#define INFO(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define INFO
#endif

#ifdef DEBUG_INFO_VERB
#define INFO_VERB(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define INFO_VERB
#endif

#ifdef DEBUG_WARN
#define WARN(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define WARN
#endif

#ifdef DEBUG_ERR
#define ERR(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define ERR
#endif

PCSTR
MediaNumber2Str(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG MediaNumber);

PCSTR
DcDbgBusError(
    _In_ ULONG InterruptStatus);

#else

#define TRACE
#define INFO
#define INFO_VERB
#define WARN
#define ERR
#define MediaNumber2Str
#define DcDbgBusError

#endif
