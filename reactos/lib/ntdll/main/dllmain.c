/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/main/dllmain.c
 * PURPOSE:         
 * PROGRAMMER:      
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntdll/ntdll.h>

void dprintf(char* fmt,...)
{
   char buffer[512];
   va_list ap;
   WCHAR bufferw[512];
   UNICODE_STRING UnicodeString;
   ULONG i;
   
   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   for (i=0; buffer[i] != 0; i++)
     {
	bufferw[i] = buffer[i];
     }
   bufferw[i] = 0;
   RtlInitUnicodeString(&UnicodeString, bufferw);
   NtDisplayString(&UnicodeString);
   va_end(ap);
}

BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDll, 
			      DWORD fdwReason, 
			      LPVOID fImpLoad)
{
  return TRUE;
}

/* EOF */
