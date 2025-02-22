/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        dll/win32/advapi32/misc/unicode.c
 * PURPOSE:     Unicode helper. Needed because RtlIsTextUnicode returns a
 *              BOOLEAN (byte) while IsTextUnicode returns a BOOL (long).
 *              The high bytes of the return value should be correctly set,
 *              hence a direct redirection cannot be done.
 */

#include <advapi32.h>

/**************************************************************************
 *  IsTextUnicode (ADVAPI32.@)
 *
 * Attempt to guess whether a text buffer is Unicode.
 *
 * PARAMS
 *  lpv       [I] Text buffer to test
 *  iSize     [I] Length of lpv
 *  lpiResult [O] Destination for test results
 *
 * RETURNS
 *  TRUE if the buffer is likely Unicode, FALSE otherwise.
 */
BOOL WINAPI
IsTextUnicode(IN CONST VOID* lpv,
              IN INT iSize,
              IN OUT LPINT lpiResult OPTIONAL)
{
    return (RtlIsTextUnicode(lpv, iSize, lpiResult) != FALSE);
}

/* EOF */
