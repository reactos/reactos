//////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2007  Qumranet All Rights Reserved
//
// Module Name:
// osdep.h
//
// Abstract:
// Windows OS dependent definitions of data types
//
// Author:
// Yan Vugenfirer  - February 2007.
//
//////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ntddk.h>

#define ENOSPC 1

#if !defined(__cplusplus) && !defined(bool)
// Important note: in MSFT C++ bool length is 1 bytes
// C++ does not define length of bool
// inconsistent definition of 'bool' may create compatibility problems
#define bool u8
#define false FALSE
#define true TRUE
#endif

#define inline __forceinline
#define SMP_CACHE_BYTES 64
