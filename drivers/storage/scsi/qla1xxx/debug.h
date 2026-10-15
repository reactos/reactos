/*
 * PROJECT:     QLogic ISP SCSI Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Debug support header file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#if DBG

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

#define TRACE(fmt, ...) \
    StorPortDebugPrint(0, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define INFO(fmt, ...) \
    StorPortDebugPrint(1, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define WARN(fmt, ...) \
    StorPortDebugPrint(2, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define ERR(fmt, ...) \
    StorPortDebugPrint(3, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#else

#if defined(_MSC_VER)
#define TRACE  __noop
#define INFO   __noop
#define WARN   __noop
#define ERR    __noop
#else
#define TRACE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define INFO(...)  do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define WARN(...)  do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define ERR(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

#endif
