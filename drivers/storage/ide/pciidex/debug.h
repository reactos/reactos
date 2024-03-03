/*
 * PROJECT:     ReactOS ATA Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
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
#define DEBUG_WARN
#define DEBUG_ERR

#ifdef DEBUG_TRACE
#define TRACE(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#if defined(_MSC_VER)
#define TRACE   __noop
#else
#define TRACE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif
#endif

#ifdef DEBUG_INFO
#define INFO(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#if defined(_MSC_VER)
#define INFO   __noop
#else
#define INFO(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif
#endif

#ifdef DEBUG_WARN
#define WARN(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#if defined(_MSC_VER)
#define WARN   __noop
#else
#define WARN(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif
#endif

#ifdef DEBUG_ERR
#define ERR(fmt, ...) \
    do { \
      if (DbgPrint("(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)) \
          DbgPrint("(%s:%d) DbgPrint() failed!\n", __RELFILE__, __LINE__); \
    } while (0)

#else
#if defined(_MSC_VER)
#define ERR   __noop
#else
#define ERR(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif
#endif

#else

#if defined(_MSC_VER)
#define TRACE  __noop
#define INFO   __noop
#define WARN   __noop
#define ERR    __noop
#else
#define TRACE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define INFO(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define WARN(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define ERR(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

#endif
