/* $Id: print.c,v 1.1 2000/01/10 20:31:23 ekohl Exp $
 *

 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

ULONG
DbgPrint (PCH Format, ...)
{
	CHAR Buffer[512];
	va_list ap;
	UNICODE_STRING UnicodeString;
	ANSI_STRING AnsiString;

	va_start (ap, Format);
	vsprintf (Buffer, Format, ap);
	va_end (ap);

	RtlInitAnsiString (&AnsiString,
	                   Buffer);
	RtlAnsiStringToUnicodeString (&UnicodeString,
	                              &AnsiString,
	                              TRUE);

	/* FIXME: send string to debugging subsystem */
	NtDisplayString (&UnicodeString);

	RtlFreeUnicodeString (&UnicodeString);

	return (strlen (Buffer));
}

/* EOF */
