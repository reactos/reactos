/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET CPU port for x86 and x64
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#ifndef _FNET_ROS_CPU_H_
#define _FNET_ROS_CPU_H_

/* Windows is LLP64 on x64, so long stays 32 bits on both targets */
typedef unsigned char           fnet_uint8_t;
typedef unsigned short          fnet_uint16_t;
typedef unsigned long           fnet_uint32_t;
typedef unsigned long long      fnet_uint64_t;
typedef signed char             fnet_int8_t;
typedef signed short            fnet_int16_t;
typedef signed long             fnet_int32_t;
typedef signed long long        fnet_int64_t;
typedef volatile fnet_uint8_t   fnet_vuint8_t;
typedef volatile fnet_uint16_t  fnet_vuint16_t;
typedef volatile fnet_uint32_t  fnet_vuint32_t;
typedef volatile fnet_uint64_t  fnet_vuint64_t;

#if defined(_WIN64)
typedef unsigned long long      fnet_uintptr_t;
#else
typedef unsigned long           fnet_uintptr_t;
#endif

#endif /* _FNET_ROS_CPU_H_ */
