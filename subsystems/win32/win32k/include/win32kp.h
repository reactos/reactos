/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32K
 * FILE:            subsystems/win32/win32k/include/win32kp.h
 * PURPOSE:         Internal Win32K Header
 * PROGRAMMER:      Stefan Ginsberg (stefan__100__@hotmail.com)
 */

#ifndef _INCLUDE_INTERNAL_WIN32K_H
#define _INCLUDE_INTERNAL_WIN32K_H

/* INCLUDES ******************************************************************/

/* Prototypes */
NTSTATUS
APIENTRY
NtGdiFlushUserBatch(
    VOID
);

/* Internal  Win32K Headers */
#include <gdiobj.h>
#include <engobj.h>
#include <userobj.h>


#endif
