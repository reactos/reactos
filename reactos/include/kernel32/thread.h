/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/kernel32/thread.h
 * PURPOSE:         Include file for lib/kernel32/thread.c
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#include <windows.h>
#include <ddk/ntddk.h>
NT_TEB *GetTeb(VOID);



