/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/wdm.c
 * PURPOSE:         Various Windows Driver Model routines
 * PROGRAMMER:      Filip Navara (xnavara@volny.cz)
 */

#include <ddk/ntddk.h>

/*
 * @implemented
 */
BOOLEAN STDCALL
IoIsWdmVersionAvailable(
	IN UCHAR MajorVersion,
	IN UCHAR MinorVersion
	)
{
   if (MajorVersion <= 1 && MinorVersion <= 10)
      return TRUE;
   return FALSE;
}
