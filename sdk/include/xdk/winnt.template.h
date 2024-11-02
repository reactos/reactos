/*
 * winnt.h
 *
 * Windows NT native definitions for user mode
 *
 * This file is part of the ReactOS PSDK package.
 *
 * This file is auto-generated from ReactOS XDK.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once
#ifndef _WINNT_
#define _WINNT_

/* We require WDK / VS 2008 or newer */
#if defined(_MSC_VER) && (_MSC_VER < 1500)
#error Compiler too old!
#endif

#if defined(__LP64__) || (!defined(_M_AMD64) && defined(__WINESRC__))
#if !defined(__ROS_LONG64__)
#define __ROS_LONG64__
#endif
#endif

#include <ctype.h>
//#include <winapifamily.h>
#ifdef __GNUC__
#include <msvctarget.h>
#endif
#include <specstrings.h>
#include <kernelspecs.h>

#include <excpt.h>
#include <basetsd.h>
#include <guiddef.h>
#include <intrin.h>

#undef __need_wchar_t
#include <winerror.h>
#include <stddef.h>
#include <sdkddkver.h>
#ifndef RC_INVOKED
#include <string.h>
#endif

/* Silence some MSVC warnings */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#endif

#ifdef __cplusplus
extern "C" {
#endif

$define(_WINNT_)
$define(ULONG=DWORD)
$define(USHORT=WORD)
$define(UCHAR=BYTE)
$include(ntbasedef.h)
$include(interlocked.h)
$include(ketypes.h)
$include(extypes.h)
$include(rtltypes.h)
$include(rtlfuncs.h)
$include(winnt_old.h)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _WINNT_ */
