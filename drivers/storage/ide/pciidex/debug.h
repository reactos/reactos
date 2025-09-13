/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug support header file
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
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
#define DEBUG_WARN
#define DEBUG_ERR

#ifdef DEBUG_TRACE
#define TRACE(fmt, ...) \
    do { \
      if (My_DbgPrint("PCIIDEX: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define TRACE
#endif

#ifdef DEBUG_INFO
#define INFO(fmt, ...) \
    do { \
      if (My_DbgPrint("PCIIDEX: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define INFO
#endif

#ifdef DEBUG_WARN
#define WARN(fmt, ...) \
    do { \
      if (My_DbgPrint("PCIIDEX: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define WARN
#endif

#ifdef DEBUG_ERR
#define ERR(fmt, ...) \
    do { \
      if (My_DbgPrint("PCIIDEX: (%s:%d) %s " fmt, __FILENAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          My_DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#define ERR
#endif

#else

#define TRACE
#define INFO
#define WARN
#define ERR

#endif
