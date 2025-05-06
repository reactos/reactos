/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Reducing dependency on Wine
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#if DBG

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wine2ros.h"

BOOL
IntIsDebugChannelEnabled(_In_ PCSTR channel)
{
    CHAR szValue[MAX_PATH];
    PCHAR pch0, pch1;
    BOOL ret;
    DWORD error;

    error = GetLastError();
    ret = GetEnvironmentVariableA("DEBUGCHANNEL", szValue, _countof(szValue));
    SetLastError(error);

    if (!ret)
        return FALSE;

    for (pch0 = szValue;; pch0 = pch1 + 1)
    {
        pch1 = strchr(pch0, ',');
        if (pch1)
            *pch1 = ANSI_NULL;
        if (_stricmp(pch0, channel) == 0)
            return TRUE;
        if (!pch1)
            return FALSE;
    }
}

#define DEBUGSTR_HEX "0123456789ABCDEF"
#define DEBUGSTR_QUOTE_TAIL_LEN 8

static PSTR
debugstr_quote_a(
    _Out_ PSTR pszBuf,
    _In_ SIZE_T cchBuf,
    _In_opt_ PCSTR pszSrc)
{
    PCH pch = pszBuf;
    PCCH pchSrc = pszSrc;

    if (!pszSrc)
        return "(null)";

    if (!((ULONG_PTR)pszSrc >> 16))
    {
        snprintf(pszBuf, cchBuf, "%p", pszSrc);
        return pszBuf;
    }

    *pch++ = '"';
    --cchBuf;

    for (; cchBuf > DEBUGSTR_QUOTE_TAIL_LEN; ++pchSrc)
    {
        switch (*pchSrc)
        {
            case '\'': case '\"': case '\\': case '\t': case '\r': case '\n':
                *pch++ = '\\';
                if (*pchSrc == '\t')
                    *pch++ = 't';
                else if (*pchSrc == '\r')
                    *pch++ = 'r';
                else if (*pchSrc == '\n')
                    *pch++ = 'n';
                else
                    *pch++ = *pchSrc;
                cchBuf -= 2;
                break;
            default:
            {
                if (*pchSrc >= ' ')
                {
                    *pch++ = *pchSrc;
                    --cchBuf;
                }
                else
                {
                    *pch++ = '\\';
                    *pch++ = 'x';
                    *pch++ = DEBUGSTR_HEX[(*pchSrc >> 4) & 0xF];
                    *pch++ = DEBUGSTR_HEX[*pchSrc & 0xF];
                    cchBuf -= 4;
                }
                break;
            }
        }
    }

    if (cchBuf <= DEBUGSTR_QUOTE_TAIL_LEN)
    {
        *pch++ = '.';
        *pch++ = '.';
        *pch++ = '.';
    }
    *pch++ = '"';
    *pch = ANSI_NULL;
    return pszBuf;
}

static PSTR
debugstr_quote_w(
    _Out_ PSTR pszBuf,
    _In_ SIZE_T cchBuf,
    _In_opt_ PCWSTR pszSrc)
{
    PCH pch = pszBuf;
    PCWCH pchSrc = pszSrc;

    if (!pszSrc)
        return "(null)";

    if (!((ULONG_PTR)pszSrc >> 16))
    {
        snprintf(pszBuf, cchBuf, "%p", pszSrc);
        return pszBuf;
    }

    *pch++ = '"';
    --cchBuf;

    for (; cchBuf > DEBUGSTR_QUOTE_TAIL_LEN; ++pchSrc)
    {
        switch (*pchSrc)
        {
            case L'\'': case L'\"': case L'\\': case L'\t': case L'\r': case L'\n':
                *pch++ = '\\';
                if (*pchSrc == L'\t')
                    *pch++ = 't';
                else if (*pchSrc == L'\r')
                    *pch++ = 'r';
                else if (*pchSrc == L'\n')
                    *pch++ = 'n';
                else
                    *pch++ = *pchSrc;
                cchBuf -= 2;
                break;
            default:
            {
                if (*pchSrc >= L' ' && *pchSrc < 0x100)
                {
                    *pch++ = (CHAR)*pchSrc;
                    --cchBuf;
                }
                else
                {
                    *pch++ = '\\';
                    *pch++ = 'x';
                    *pch++ = DEBUGSTR_HEX[(*pchSrc >> 12) & 0xF];
                    *pch++ = DEBUGSTR_HEX[(*pchSrc >> 8) & 0xF];
                    *pch++ = DEBUGSTR_HEX[(*pchSrc >> 4) & 0xF];
                    *pch++ = DEBUGSTR_HEX[*pchSrc & 0xF];
                    cchBuf -= 6;
                }
                break;
            }
        }
    }

    if (cchBuf <= DEBUGSTR_QUOTE_TAIL_LEN)
    {
        *pch++ = '.';
        *pch++ = '.';
        *pch++ = '.';
    }
    *pch++ = '"';
    *pch = ANSI_NULL;
    return pszBuf;
}

#define DEBUGSTR_NUM_BUFFS 5
#define DEBUGSTR_BUFF_SIZE MAX_PATH

static LPSTR
debugstr_next_buff(void)
{
    static CHAR s_bufs[DEBUGSTR_NUM_BUFFS][DEBUGSTR_BUFF_SIZE];
    static SIZE_T s_index = 0;
    PCHAR ptr = s_bufs[s_index];
    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
}

PCSTR
debugstr_a(_In_opt_ PCSTR pszA)
{
    return debugstr_quote_a(debugstr_next_buff(), DEBUGSTR_BUFF_SIZE, pszA);
}

PCSTR
debugstr_w(_In_opt_ PCWSTR pszW)
{
    return debugstr_quote_w(debugstr_next_buff(), DEBUGSTR_BUFF_SIZE, pszW);
}

PCSTR
debugstr_guid(_In_opt_ const GUID *id)
{
    PCHAR ptr;

    if (!id)
        return "(null)";

    ptr = debugstr_next_buff();

    if (!((ULONG_PTR)id >> 16))
    {
        snprintf(ptr, DEBUGSTR_BUFF_SIZE, "%p", id);
    }
    else
    {
        snprintf(ptr, DEBUGSTR_BUFF_SIZE,
                 "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 id->Data1, id->Data2, id->Data3,
                 id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                 id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    }

    return ptr;
}

#endif /* DBG */
