/* $Id: ntddk.h,v 1.35 2003/05/28 18:09:09 chorns Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           include/ddk/ntddk.h
 * PURPOSE:        Interface definitions for drivers
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

#ifdef __USE_W32API

#include_next <ddk/ntddk.h>

#else /* __USE_W32API */

#ifndef __NTDDK_H
#define __NTDDK_H

#ifdef __cplusplus
extern "C"
{
#endif

/* INCLUDES ***************************************************************/

#define FASTCALL  __attribute__((fastcall))

#define STATIC static

#ifndef _GNU_H_WINDOWS_H
/* NASTY HACK! Our msvcrt are messed up, causing msvcrt.dll to crash when
 * the headers are mixed with MinGW msvcrt headers. Not including stdlib.h
 * seems to correct this.
 */
#include <stdlib.h>
#include <string.h>
#endif
#include <ntos/types.h>
#include <ntos/time.h>
#include <ntos/cdrom.h>
#include <ntos/disk.h>
#include <ntos/registry.h>
#include <ntos/port.h>
#include <ntos/synch.h>
#include <napi/types.h>

#include <pe.h>

#include <ddk/status.h>
#include <ddk/ntdef.h>
#include <ddk/defines.h>
#include <ddk/types.h>
#include <ddk/cmtypes.h>
#include <ddk/ketypes.h>
#include <ntos/security.h>
#include <ddk/setypes.h>
#include <ddk/mmtypes.h>
#include <ddk/potypes.h>
#include <ddk/pnptypes.h>
#include <ddk/iotypes.h>
#include <ddk/extypes.h>
#include <ddk/pstypes.h>
#include <ntos/ldrtypes.h>
#include <ntos/zwtypes.h>
#include <ddk/ioctrl.h>
#include <ntos/rtltypes.h>
#include <napi/shared_data.h>

#include <ntos/zw.h>
#include <ntos/rtl.h>
#include <ddk/dbgfuncs.h>
#include <ddk/ldrfuncs.h>
#if defined(__NTOSKRNL__) || defined(__NTDRIVER__) || defined(__NTHAL__)
#include <ddk/exfuncs.h>
#include <ddk/halfuncs.h>
#include <ddk/mmfuncs.h>
#include <ddk/kdfuncs.h>
#include <ddk/kefuncs.h>
#include <ddk/pofuncs.h>
#include <ddk/pnpfuncs.h>
#include <ddk/iofuncs.h>
#include <ddk/psfuncs.h>
#include <ddk/obfuncs.h>
#include <ddk/sefuncs.h>
#endif /*__NTOSKRNL__ || __NTDRIVER__ || __NTHAL__ */

#ifdef __cplusplus
};
#endif

#endif /* __NTDDK_H */

#endif /* __USE_W32API */
