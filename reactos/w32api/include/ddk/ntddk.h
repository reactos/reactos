/*
 * ntddk.h
 *
 * Windows Device Driver Kit
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
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
 * DEFINES:
 *    DBG          - Debugging enabled/disabled (0/1)
 *    POOL_TAGGING - Enable pool tagging
 *    _X86_        - X86 environment
 */

#ifndef __NTDDK_H
#define __NTDDK_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)

#include <stdarg.h>
#include <windef.h>
#include <ntdef.h>
#include <basetyps.h>

/* Base types, structures and definitions */
typedef short CSHORT;
typedef CONST int CINT;
typedef CONST char *PCSZ;

#ifndef STATIC
#define STATIC static
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef DECL_IMPORT
#define DECL_IMPORT __attribute__((dllimport))
#endif

#ifndef DECL_EXPORT
#define DECL_EXPORT __attribute__((dllexport))
#endif

/* Windows NT status codes */
#include "ntstatus.h"

/* Windows NT definitions exported to user mode */
#include <winnt.h>

/* Windows Device Driver Kit */
#include "winddk.h"

/* Definitions only in Windows XP */
#include "winxp.h"

/* Definitions only in Windows 2000 */
#include "win2k.h"

/* Definitions only in Windows NT 4 */
#include "winnt4.h"

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __NTDDK_H */
