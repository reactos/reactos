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
#include <winreg.h>
#include <winuser.h>

#include <vddsvc.h>

#include <debug.h>

/* DEFINES ********************************************************************/

/* Processor speed */
#define STEPS_PER_CYCLE 256
#define KBD_INT_CYCLES 16

/* FUNCTIONS ******************************************************************/

VOID DisplayMessage(LPCWSTR Format, ...);

#endif // _NTVDM_H_

/* EOF */
