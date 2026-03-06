/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug support header file
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define My_DbgPrint DbgPrint

#if DBG

// #define DEBUG_TRACE
#define DEBUG_INFO
#define DEBUG_INFO_VERB
#define DEBUG_WARN
#define DEBUG_ERR
// #define DEBUG_TRACE_IO

#ifdef DEBUG_TRACE
#define TRACE(fmt, ...) \
    do { \
      if (My_DbgPrint("ATAPI: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define TRACE(fmt, ...)
#endif

#ifdef DEBUG_INFO
#define INFO(fmt, ...) \
    do { \
      if (My_DbgPrint("ATAPI: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define INFO(fmt, ...)
#endif

#ifdef DEBUG_INFO_VERB
#define INFO_VERB(fmt, ...) \
    do { \
      if (My_DbgPrint("ATAPI: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define INFO_VERB(fmt, ...)
#endif

#ifdef DEBUG_WARN
#define WARN(fmt, ...) \
    do { \
      if (My_DbgPrint("ATAPI: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define WARN(fmt, ...)
#endif

#ifdef DEBUG_ERR
#define ERR(fmt, ...) \
    do { \
      if (My_DbgPrint("ATAPI: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define ERR(fmt, ...)
#endif

#ifdef DEBUG_TRACE_IO
#define TRACE_IO(fmt, ...) \
    do { \
      if (My_DbgPrint("ATAPI: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define TRACE_IO(fmt, ...)
#endif

#else

#define TRACE(fmt, ...)
#define INFO(fmt, ...)
#define INFO_VERB(fmt, ...)
#define WARN(fmt, ...)
#define ERR(fmt, ...)
#define TRACE_IO(fmt, ...)

#endif
