/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/include/hal.h
 * PURPOSE:         Hardware Abstraction Layer Header
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdio.h>

/* WDK HAL Compilation hack */
#ifdef _MSC_VER
#include <excpt.h>
#include <ntdef.h>
#undef _NTHAL_
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#define __declspec(dllimport)
#endif

/* IFS/DDK/NDK Headers */
#include <ntifs.h>
#include <bugcodes.h>
#include <ntdddisk.h>
#include <arc/arc.h>
#include <iotypes.h>
#include <kefuncs.h>
#include <intrin.h>
#include <halfuncs.h>
#include <iofuncs.h>
#include <ldrtypes.h>
#include <obfuncs.h>

/* Internal HAL Headers */
#include "halp.h"

/* EOF */
