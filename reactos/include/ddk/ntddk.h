/*
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

#include <windows.h>

#define NT_SUCCESS(StatCode)  ((NTSTATUS)(StatCode) >= 0)
#define NTKERNELAPI

   #define CTL_CODE(Dev, Func, Meth, Acc) ( ((Dev)<<16) | ((Acc)<<14) | ((Func)<<2) | (Meth))

//  IOCTL Parameter buffering methods
#define METHOD_BUFFERED    0
#define METHOD_IN_DIRECT   1
#define METHOD_OUT_DIRECT  2
#define METHOD_NEITHER     3

//  IOCTL File access type
#define FILE_ANY_ACCESS    0
#define FILE_READ_ACCESS   1
#define FILE_WRITE_ACCESS  2

#define QUAD_PART(LI)  (*(LONGLONG *)(&LI))

enum {
  STATUS_NOT_SUPPORTED = 9999,
  STATUS_DISK_OPERATION_FAILED
};

#define  IO_DISK_INCREMENT  4

#define  FILE_WORD_ALIGNMENT  0x0001

#define  FILE_OPENED          0x0001
   
#include <ddk/defines.h>
#include <ddk/types.h>
#include <ddk/structs.h>
#include <ddk/setypes.h>
   
#include <internal/hal/ddk.h>
   
#include <ddk/rtl.h>
#include <ddk/zw.h>
#include <ddk/exfuncs.h>
#include <ddk/mmfuncs.h>
#include <ddk/kefuncs.h>
#include <ddk/iofuncs.h> 
#include <ddk/psfuncs.h>
#include <ddk/obfuncs.h>
   
ULONG DbgPrint(PCH Format,...);   
   
#ifdef __cplusplus
};
#endif

#endif /* __NTDDK_H */

