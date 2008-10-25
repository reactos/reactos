/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/print.c
 * PURPOSE:         Print on the blue screen.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
#include "smss.h"

#define NDEBUG
#include <debug.h>

VOID STDCALL DisplayString(LPCWSTR lpwString)
{
   UNICODE_STRING us;

   RtlInitUnicodeString (&us, lpwString);
   NtDisplayString (&us);
}

VOID STDCALL PrintString (char* fmt, ...)
{
   char buffer[512];
   va_list ap;
   UNICODE_STRING UnicodeString;
   ANSI_STRING AnsiString;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);
   DPRINT1("%s", buffer);

   RtlInitAnsiString (&AnsiString, buffer);
   RtlAnsiStringToUnicodeString (&UnicodeString,
				 &AnsiString,
				 TRUE);
   NtDisplayString(&UnicodeString);
   RtlFreeUnicodeString (&UnicodeString);
}

/* EOF */
