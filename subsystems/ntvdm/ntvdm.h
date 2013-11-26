/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.h
 * PURPOSE:         Header file to define commonly used stuff
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _NTVDM_H_
#define _NTVDM_H_

/* INCLUDES *******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <conio.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#include <winnls.h>
#include <winuser.h>

#include <debug.h>

/* DEFINES ********************************************************************/

/* Basic Memory Management */
#define TO_LINEAR(seg, off) (((seg) << 4) + (off))
#define MAX_SEGMENT 0xFFFF
#define MAX_OFFSET  0xFFFF
#define MAX_ADDRESS 0x1000000 // 16 MB of RAM

#define FAR_POINTER(x)  \
    (PVOID)((ULONG_PTR)BaseAddress + TO_LINEAR(HIWORD(x), LOWORD(x)))

#define SEG_OFF_TO_PTR(seg, off)    \
    (PVOID)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), (off)))

/* BCD-Binary conversion */
#define BINARY_TO_BCD(x) ((((x) / 1000) << 12) + (((x) / 100) << 8) + (((x) / 10) << 4) + ((x) % 10))
#define BCD_TO_BINARY(x) (((x) >> 12) * 1000 + ((x) >> 8) * 100 + ((x) >> 4) * 10 + ((x) & 0x0F))

/* Processor speed */
#define STEPS_PER_CYCLE 256

/* FUNCTIONS ******************************************************************/

extern LPVOID BaseAddress;
extern BOOLEAN VdmRunning;

VOID DisplayMessage(LPCWSTR Format, ...);

#endif // _NTVDM_H_

/* EOF */
