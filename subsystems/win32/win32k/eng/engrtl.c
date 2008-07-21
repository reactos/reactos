/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engrtl.c
 * PURPOSE:         Rtl Wrapper Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
APIENTRY
EngGetCurrentCodePage(OUT PUSHORT OemCodePage,
                      OUT PUSHORT AnsiCodePage)
{
    /* Call Rtl */
    return RtlGetDefaultCodePage(AnsiCodePage, OemCodePage);
}

VOID
APIENTRY
EngMultiByteToUnicodeN(
  OUT LPWSTR  UnicodeString,
  IN ULONG  MaxBytesInUnicodeString,
  OUT PULONG  BytesInUnicodeString,
  IN PCHAR  MultiByteString,
  IN ULONG  BytesInMultiByteString)
{
    /* Call Rtl */
    RtlMultiByteToUnicodeN(UnicodeString,
                           MaxBytesInUnicodeString,
                           BytesInUnicodeString,
                           MultiByteString,
                           BytesInMultiByteString);
}

VOID
APIENTRY
EngUnicodeToMultiByteN(OUT PCHAR MultiByteString,
                       IN ULONG MaxBytesInMultiByteString,
                       OUT PULONG BytesInMultiByteString,
                       IN PWSTR UnicodeString,
                       IN ULONG BytesInUnicodeString)
{
    /* Call Rtl */
	RtlUnicodeToMultiByteN(MultiByteString,
                           MaxBytesInMultiByteString,
                           BytesInMultiByteString,
                           UnicodeString,
                           BytesInUnicodeString);
}

INT
APIENTRY
EngMultiByteToWideChar(IN UINT CodePage,
                       OUT PWSTR WideCharString,
                       IN INT BytesInWideCharString,
                       IN PSTR MultiByteString,
                       IN INT BytesInMultiByteString)
{
    UNIMPLEMENTED;
	return 0;
}

INT
APIENTRY
EngWideCharToMultiByte(IN UINT CodePage,
                       IN PWSTR WideCharString,
                       IN INT BytesInWideCharString,
                       OUT PSTR MultiByteString,
                       IN INT BytesInMultiByteString)
{
    UNIMPLEMENTED;
	return 0;
}
