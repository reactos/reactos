/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug support header file
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

#if DBG

// #define DEBUG_TRACE
// #define DEBUG_INFO
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

#else

#define TRACE
#define INFO
#define WARN
#define ERR

#endif
