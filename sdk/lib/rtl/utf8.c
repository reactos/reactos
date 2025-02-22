/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Rtl functions of Vista+
 * COPYRIGHT:   2016 Thomas Faber <thomas.faber@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/******************************************************************************
 * RtlUnicodeToUTF8N [NTDLL.@]
 */
NTSTATUS NTAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                 ULONG *utf8_bytes_written,
                                 const WCHAR *uni_src, ULONG uni_bytes)
{
    NTSTATUS status;
    ULONG i;
    ULONG written;
    ULONG ch;
    BYTE utf8_ch[4];
    ULONG utf8_ch_len;

    if (!uni_src)
        return STATUS_INVALID_PARAMETER_4;
    if (!utf8_bytes_written)
        return STATUS_INVALID_PARAMETER;
    if (utf8_dest && uni_bytes % sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER_5;

    written = 0;
    status = STATUS_SUCCESS;

    for (i = 0; i < uni_bytes / sizeof(WCHAR); i++)
    {
        /* decode UTF-16 into ch */
        ch = uni_src[i];
        if (ch >= 0xdc00 && ch <= 0xdfff)
        {
            ch = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
        else if (ch >= 0xd800 && ch <= 0xdbff)
        {
            if (i + 1 < uni_bytes / sizeof(WCHAR))
            {
                ch -= 0xd800;
                ch <<= 10;
                if (uni_src[i + 1] >= 0xdc00 && uni_src[i + 1] <= 0xdfff)
                {
                    ch |= uni_src[i + 1] - 0xdc00;
                    ch += 0x010000;
                    i++;
                }
                else
                {
                    ch = 0xfffd;
                    status = STATUS_SOME_NOT_MAPPED;
                }
            }
            else
            {
                ch = 0xfffd;
                status = STATUS_SOME_NOT_MAPPED;
            }
        }

        /* encode ch as UTF-8 */
        ASSERT(ch <= 0x10ffff);
        if (ch < 0x80)
        {
            utf8_ch[0] = ch & 0x7f;
            utf8_ch_len = 1;
        }
        else if (ch < 0x800)
        {
            utf8_ch[0] = 0xc0 | (ch >>  6 & 0x1f);
            utf8_ch[1] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 2;
        }
        else if (ch < 0x10000)
        {
            utf8_ch[0] = 0xe0 | (ch >> 12 & 0x0f);
            utf8_ch[1] = 0x80 | (ch >>  6 & 0x3f);
            utf8_ch[2] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 3;
        }
        else if (ch < 0x200000)
        {
            utf8_ch[0] = 0xf0 | (ch >> 18 & 0x07);
            utf8_ch[1] = 0x80 | (ch >> 12 & 0x3f);
            utf8_ch[2] = 0x80 | (ch >>  6 & 0x3f);
            utf8_ch[3] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 4;
        }

        if (!utf8_dest)
        {
            written += utf8_ch_len;
            continue;
        }

        if (utf8_bytes_max >= utf8_ch_len)
        {
            memcpy(utf8_dest, utf8_ch, utf8_ch_len);
            utf8_dest += utf8_ch_len;
            utf8_bytes_max -= utf8_ch_len;
            written += utf8_ch_len;
        }
        else
        {
            utf8_bytes_max = 0;
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    *utf8_bytes_written = written;
    return status;
}


/******************************************************************************
 * RtlUTF8ToUnicodeN [NTDLL.@]
 */
NTSTATUS NTAPI RtlUTF8ToUnicodeN(WCHAR *uni_dest, ULONG uni_bytes_max,
                                 ULONG *uni_bytes_written,
                                 const CHAR *utf8_src, ULONG utf8_bytes)
{
    NTSTATUS status;
    ULONG i, j;
    ULONG written;
    ULONG ch;
    ULONG utf8_trail_bytes;
    WCHAR utf16_ch[3];
    ULONG utf16_ch_len;

    if (!utf8_src)
        return STATUS_INVALID_PARAMETER_4;
    if (!uni_bytes_written)
        return STATUS_INVALID_PARAMETER;

    written = 0;
    status = STATUS_SUCCESS;

    for (i = 0; i < utf8_bytes; i++)
    {
        /* read UTF-8 lead byte */
        ch = (BYTE)utf8_src[i];
        utf8_trail_bytes = 0;
        if (ch >= 0xf5)
        {
            ch = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
        else if (ch >= 0xf0)
        {
            ch &= 0x07;
            utf8_trail_bytes = 3;
        }
        else if (ch >= 0xe0)
        {
            ch &= 0x0f;
            utf8_trail_bytes = 2;
        }
        else if (ch >= 0xc2)
        {
            ch &= 0x1f;
            utf8_trail_bytes = 1;
        }
        else if (ch >= 0x80)
        {
            /* overlong or trail byte */
            ch = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }

        /* read UTF-8 trail bytes */
        if (i + utf8_trail_bytes < utf8_bytes)
        {
            for (j = 0; j < utf8_trail_bytes; j++)
            {
                if ((utf8_src[i + 1] & 0xc0) == 0x80)
                {
                    ch <<= 6;
                    ch |= utf8_src[i + 1] & 0x3f;
                    i++;
                }
                else
                {
                    ch = 0xfffd;
                    utf8_trail_bytes = 0;
                    status = STATUS_SOME_NOT_MAPPED;
                    break;
                }
            }
        }
        else
        {
            ch = 0xfffd;
            utf8_trail_bytes = 0;
            status = STATUS_SOME_NOT_MAPPED;
            i = utf8_bytes;
        }

        /* encode ch as UTF-16 */
        if ((ch > 0x10ffff) ||
            (ch >= 0xd800 && ch <= 0xdfff) ||
            (utf8_trail_bytes == 2 && ch < 0x00800) ||
            (utf8_trail_bytes == 3 && ch < 0x10000))
        {
            /* invalid codepoint or overlong encoding */
            utf16_ch[0] = 0xfffd;
            utf16_ch[1] = 0xfffd;
            utf16_ch[2] = 0xfffd;
            utf16_ch_len = utf8_trail_bytes;
            status = STATUS_SOME_NOT_MAPPED;
        }
        else if (ch >= 0x10000)
        {
            /* surrogate pair */
            ch -= 0x010000;
            utf16_ch[0] = 0xd800 + (ch >> 10 & 0x3ff);
            utf16_ch[1] = 0xdc00 + (ch >>  0 & 0x3ff);
            utf16_ch_len = 2;
        }
        else
        {
            /* single unit */
            utf16_ch[0] = ch;
            utf16_ch_len = 1;
        }

        if (!uni_dest)
        {
            written += utf16_ch_len;
            continue;
        }

        for (j = 0; j < utf16_ch_len; j++)
        {
            if (uni_bytes_max >= sizeof(WCHAR))
            {
                *uni_dest++ = utf16_ch[j];
                uni_bytes_max -= sizeof(WCHAR);
                written++;
            }
            else
            {
                uni_bytes_max = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }

    *uni_bytes_written = written * sizeof(WCHAR);
    return status;
}
