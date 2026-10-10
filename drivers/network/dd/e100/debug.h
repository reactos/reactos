/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Debug support header file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
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

#else

#if defined(_MSC_VER)
#define TRACE     __noop
#define INFO      __noop
#define INFO_VERB __noop
#define WARN      __noop
#define ERR       __noop
#else
#define TRACE(...)     do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define INFO(...)      do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define INFO_VERB(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define WARN(...)      do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define ERR(...)       do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

#endif
