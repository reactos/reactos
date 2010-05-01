/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Royce Mitchell III
 *             Eric Kohl
 *             Ge van Geldorp <gvg@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infhost.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
RtlMultiByteToUnicodeN(
   IN PWCHAR UnicodeString,
   IN ULONG UnicodeSize,
   IN PULONG ResultSize,
   IN PCSTR MbString,
   IN ULONG MbSize)
{
    ULONG Size = 0;
    ULONG i;
    PUCHAR WideString;

    /* single-byte code page */
    if (MbSize > (UnicodeSize / sizeof(WCHAR)))
        Size = UnicodeSize / sizeof(WCHAR);
    else
        Size = MbSize;

    if (ResultSize != NULL)
        *ResultSize = Size * sizeof(WCHAR);

    WideString = (PUCHAR)UnicodeString;
    for (i = 0; i <= Size; i++)
    {
        WideString[2 * i + 0] = (UCHAR)MbString[i];
        WideString[2 * i + 1] = 0;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
NTAPI
RtlIsTextUnicode( PVOID buf, INT len, INT *pf )
{
    static const WCHAR std_control_chars[] = {'\r','\n','\t',' ',0x3000,0};
    static const WCHAR byterev_control_chars[] = {0x0d00,0x0a00,0x0900,0x2000,0};
    const WCHAR *s = buf;
    int i;
    unsigned int flags = MAXULONG, out_flags = 0;

    if (len < sizeof(WCHAR))
    {
        /* FIXME: MSDN documents IS_TEXT_UNICODE_BUFFER_TOO_SMALL but there is no such thing... */
        if (pf) *pf = 0;
        return FALSE;
    }
    if (pf)
        flags = (unsigned int)*pf;
    /*
     * Apply various tests to the text string. According to the
     * docs, each test "passed" sets the corresponding flag in
     * the output flags. But some of the tests are mutually
     * exclusive, so I don't see how you could pass all tests ...
     */

    /* Check for an odd length ... pass if even. */
    if (len & 1) out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

    if (((char *)buf)[len - 1] == 0)
        len--;  /* Windows seems to do something like that to avoid e.g. false IS_TEXT_UNICODE_NULL_BYTES  */

    len /= (INT)sizeof(WCHAR);
    /* Windows only checks the first 256 characters */
    if (len > 256) len = 256;

    /* Check for the special byte order unicode marks. */
    if (*s == 0xFEFF) out_flags |= IS_TEXT_UNICODE_SIGNATURE;
    if (*s == 0xFFFE) out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;

    /* apply some statistical analysis */
    if (flags & IS_TEXT_UNICODE_STATISTICS)
    {
        int stats = 0;
        /* FIXME: checks only for ASCII characters in the unicode stream */
        for (i = 0; i < len; i++)
        {
            if (s[i] <= 255) stats++;
        }
        if (stats > len / 2)
            out_flags |= IS_TEXT_UNICODE_STATISTICS;
    }

    /* Check for unicode NULL chars */
    if (flags & IS_TEXT_UNICODE_NULL_BYTES)
    {
        for (i = 0; i < len; i++)
        {
            if (!(s[i] & 0xff) || !(s[i] >> 8))
            {
                out_flags |= IS_TEXT_UNICODE_NULL_BYTES;
                break;
            }
        }
    }

    if (flags & IS_TEXT_UNICODE_CONTROLS)
    {
        for (i = 0; i < len; i++)
        {
            if (strchrW(std_control_chars, s[i]))
            {
                out_flags |= IS_TEXT_UNICODE_CONTROLS;
                break;
            }
        }
    }

    if (flags & IS_TEXT_UNICODE_REVERSE_CONTROLS)
    {
        for (i = 0; i < len; i++)
        {
            if (strchrW(byterev_control_chars, s[i]))
            {
                out_flags |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
                break;
            }
        }
    }

    if (pf)
    {
        out_flags &= (unsigned int)*pf;
        *pf = (INT)out_flags;
    }
    /* check for flags that indicate it's definitely not valid Unicode */
    if (out_flags & (IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK)) return FALSE;
    /* now check for invalid ASCII, and assume Unicode if so */
    if (out_flags & IS_TEXT_UNICODE_NOT_ASCII_MASK) return TRUE;
    /* now check for Unicode flags */
    if (out_flags & IS_TEXT_UNICODE_UNICODE_MASK) return TRUE;
    /* no flags set */
    return FALSE;
}
