/* $Id: ntddk.h,v 1.21 2001/05/01 23:08:17 chorns Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           include/ddk/ntddk.h
 * PURPOSE:        Interface definitions for drivers
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

#ifndef __NTDDK_H
#define __NTDDK_H

#ifdef __cplusplus
extern "C"
{
#endif

/* INCLUDES ***************************************************************/

/* GCC can not handle __fastcall */
#ifndef FASTCALL
#define FASTCALL STDCALL
#endif
#define STATIC static

#include <ntos/types.h>
#include <ntos/disk.h>
#include <ntos/registry.h>
#include <ntos/port.h>
#include <napi/types.h>

#include <pe.h>

#include <ddk/status.h>
#include <ddk/ntdef.h>
#include <ddk/defines.h>
#include <ddk/types.h>
#include <ddk/cmtypes.h>
#include <ddk/ketypes.h>
#include <ntos/security.h>
#include <ddk/obtypes.h>
#include <ddk/setypes.h>
#include <ddk/mmtypes.h>
#include <ddk/potypes.h>
#include <ddk/pnptypes.h>
#include <ddk/iotypes.h>
#include <ddk/extypes.h>
#include <ddk/pstypes.h>
#include <ddk/zwtypes.h>
#include <ddk/ioctrl.h>
#include <ddk/rtl.h>
#include <ddk/halddk.h>

#include <ddk/zw.h>
#include <ddk/cmfuncs.h>
#include <ddk/exfuncs.h>
#include <ddk/mmfuncs.h>
#include <ddk/kdfuncs.h>
#include <ddk/kefuncs.h>
#include <ddk/pofuncs.h>
#include <ddk/pnpfuncs.h>
#include <ddk/iofuncs.h>
#include <ddk/psfuncs.h>
#include <ddk/obfuncs.h>
#include <ddk/dbgfuncs.h>
#include <ddk/sefuncs.h>
#include <ddk/ldrfuncs.h>

#ifdef __cplusplus
};
#endif

#endif /* __NTDDK_H */

