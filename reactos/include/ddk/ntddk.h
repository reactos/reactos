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

//
// Dependencies
//
#define NT_INCLUDED
#include <wdm.h>
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

#include <stdarg.h> // FIXME
#include <basetyps.h> // FIXME



//
// GUID and UUID
//
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif
typedef GUID UUID;



/* Windows Device Driver Kit */
#include "winddk.h"

/* Definitions only in Windows XP */
#include "winxp.h"

/* Definitions only in Windows 2000 */
#include "win2k.h"

/* Definitions only in Windows NT 4 */
#include "winnt4.h"

#endif /* __NTDDK_H */
