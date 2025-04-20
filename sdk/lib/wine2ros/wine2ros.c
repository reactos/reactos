/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Reducing dependency on Wine
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wine2ros.h"

BOOL
IntIsDebugChannelEnabled(_In_ DEBUGCHANNEL channel)
{
    CHAR szValue[MAX_PATH], *pch0, *pch;

    if (!GetEnvironmentVariableA("DEBUGCHANNEL", szValue, _countof(szValue)))
        return FALSE;

    for (pch0 = szValue;; pch0 = pch + 1)
    {
        pch = strchr(pch0, ',');
        if (pch)
            *pch = ANSI_NULL;
        if (channel == DbgCh_imm && _stricmp(pch0, "imm") == 0)
            return TRUE;
        if (channel == DbgCh_netapi32 && _stricmp(pch0, "netapi32") == 0)
            return TRUE;
        if (channel == DbgCh_netbios && _stricmp(pch0, "netbios") == 0)
            return TRUE;
        if (!pch)
            return FALSE;
    }
}

#define DEBUG_ESCAPE_TAIL_LEN 5

static PSTR
debugstr_escape(PSTR pszBuf, INT cchBuf, PCSTR pszSrc)
{
#if DBG
    PCH pch = pszBuf;
    PCCH pchSrc = pszSrc;

    if (!pszSrc)
        return "(null)";

    if (!((ULONG_PTR)pszSrc >> 16))
    {
        sprintf(pszBuf, "#%04lx", (DWORD)(ULONG_PTR)pszSrc);
        return pszBuf;
    }

    *pch++ = '"';
    --cchBuf;

    for (; cchBuf > DEBUG_ESCAPE_TAIL_LEN; ++pchSrc)
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
                    *pch++ = "0123456789ABCDEF"[*pchSrc & 0xF];
                    *pch++ = "0123456789ABCDEF"[(*pchSrc >> 4) & 0xF];
                    cchBuf -= 4;
                }
                break;
            }
        }
    }

    if (cchBuf <= DEBUG_ESCAPE_TAIL_LEN)
    {
        *pch++ = '.';
        *pch++ = '.';
        *pch++ = '.';
    }
    *pch++ = '"';
    *pch = ANSI_NULL;
    return pszBuf;
#else
    return NULL;
#endif
}

#define DEBUG_BUF_SIZE MAX_PATH
#define DEBUG_NUM_BUFS 4
#define DEBUG_NUM_BUFS 4

PCSTR
debugstr_a(_In_opt_ PCSTR pszA)
{
#if DBG
    static CHAR s_bufs[DEBUG_NUM_BUFS][DEBUG_BUF_SIZE];
    static SIZE_T s_index = 0;
    PCHAR ptr = debugstr_escape(s_bufs[s_index], _countof(s_bufs[s_index]), pszA);
    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
#else
    return NULL;
#endif
}

PCSTR
debugstr_w(_In_opt_ PCWSTR pszW)
{
#if DBG
    static CHAR s_bufs[DEBUG_NUM_BUFS][DEBUG_BUF_SIZE];
    static SIZE_T s_index = 0;
    CHAR buf[DEBUG_BUF_SIZE];
    PCHAR ptr;

    if (!pszW)
        return "(null)";

    if (!((ULONG_PTR)pszW >> 16))
    {
        sprintf(buf, "#%04lx", (DWORD)(ULONG_PTR)pszW);
    }
    else
    {
        WideCharToMultiByte(CP_ACP, 0, pszW, -1, buf, _countof(buf), NULL, NULL);
        buf[_countof(buf) - 1] = ANSI_NULL;
    }

    ptr = debugstr_escape(s_bufs[s_index], _countof(s_bufs[s_index]), buf);
    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
#else /* !DBG */
    return NULL;
#endif /* !DBG */
}

PCSTR
debugstr_guid(_In_opt_ const GUID *id)
{
#if DBG
    static CHAR s_bufs[DEBUG_NUM_BUFS][50];
    static SIZE_T s_index = 0;
    PCHAR ptr;

    if (!id)
        return "(null)";

    ptr = s_bufs[s_index];
    if (!((ULONG_PTR)id >> 16))
    {
        sprintf(ptr, "<guid-0x%04lx>", (ULONG_PTR)id & 0xffff);
    }
    else
    {
        sprintf(ptr, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                id->Data1, id->Data2, id->Data3,
                id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    }

    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
#else
    return NULL;
#endif
}
