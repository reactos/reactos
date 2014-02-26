#include "precomp.h"
#include "wraplog.h"
#include <stdio.h>

static INT openCount = 0;
static INT callLevel;
static FILE*log;

static INT nTemps;
static CHAR strTemp[10][256];

void WrapLogOpen()
{
    if (openCount == 0)
    {
        log = fopen("\\RShellWrap.log", "w");
        nTemps = 0;
        callLevel = 0;
    }
    openCount++;
}

void WrapLogClose()
{
    openCount--;
    if (openCount == 0)
    {
        fclose(log);
        log = NULL;
    }
}

void __cdecl WrapLogPrint(_Printf_format_string_ const char* msg, ...)
{
    va_list args;
    for (int i = 0; i < callLevel; i++)
        fputs("  ", log);
    va_start(args, msg);
    vfprintf(log, msg, args);
    va_end(args);
    fflush(log);
    nTemps = 0;
}

void __cdecl WrapLogPre(_Printf_format_string_ const char* msg, ...)
{
    va_list args;
    for (int i = 0; i < callLevel; i++)
        fputs("  ", log);
    fputs("pre: ", log);
    va_start(args, msg);
    vfprintf(log, msg, args);
    va_end(args);
    fflush(log);
    nTemps = 0;
}

void __cdecl WrapLogPost(_Printf_format_string_ const char* msg, ...)
{
    va_list args;
    for (int i = 0; i < callLevel; i++)
        fputs("  ", log);
    fputs("post: ", log);
    va_start(args, msg);
    vfprintf(log, msg, args);
    va_end(args);
    fflush(log);
    nTemps = 0;
}

void __cdecl WrapLogEnter(_Printf_format_string_ const char* msg, ...)
{
    va_list args;
    for (int i = 0; i < callLevel; i++)
        fputs("  ", log);
    fputs("CALL ", log);
    va_start(args, msg);
    vfprintf(log, msg, args);
    va_end(args);
    fflush(log);
    callLevel++;
    nTemps = 0;
}

void __cdecl WrapLogExit(const char* msg, HRESULT code)
{
    //if (FAILED(code))
    //    WrapLogPrint("RETURN %s = %08x\n", msg, code);
    //else if (code == S_OK)
    //    WrapLogPrint("RETURN %s = S_OK\n", msg);
    //else if (code == S_FALSE)
    //    WrapLogPrint("RETURN %s = S_FALSE\n", msg);
    //else
    //    WrapLogPrint("RETURN %s = %d\n", msg, code);
    if (FAILED(code))
        WrapLogPrint("RETURN %08x\n", code);
    else if (code == S_OK)
        WrapLogPrint("RETURN S_OK\n");
    else if (code == S_FALSE)
        WrapLogPrint("RETURN S_FALSE\n");
    else
        WrapLogPrint("RETURN %d\n", code);
    callLevel--;
}

template <class T>
LPSTR Wrap(const T& value);

template <>
LPSTR Wrap<GUID>(REFGUID guid)
{
    LPSTR cStr = strTemp[nTemps++];
    StringCchPrintfA(cStr, _countof(strTemp[0]),
        "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return cStr;
}

template <>
LPSTR Wrap<RECT>(const RECT& rect)
{
    LPSTR cStr = strTemp[nTemps++];
    StringCchPrintfA(cStr, _countof(strTemp[0]),
        "{L: %d, T: %d, R: %d, B: %d}",
        rect.left, rect.top, rect.right, rect.bottom);
    return cStr;
}

template <>
LPSTR Wrap<OLECMD>(const OLECMD& cmd)
{
    LPSTR cStr = strTemp[nTemps++];
    StringCchPrintfA(cStr, _countof(strTemp[0]),
        "{ID: %d, F: %d}",
        cmd.cmdID, cmd.cmdf);
    return cStr;
}

template <>
LPSTR Wrap<MSG>(const MSG& msg)
{
    LPSTR cStr = strTemp[nTemps++];
    StringCchPrintfA(cStr, _countof(strTemp[0]),
        "{HWND: %d, Code: %d, W: %p, L: %p, T: %d, P.X: %d, P.Y: %d}",
        msg.hwnd, msg.message, msg.wParam, msg.lParam, msg.time, msg.pt.x, msg.pt.y);
    return cStr;
}

template <>
LPSTR Wrap<BANDSITEINFO>(const BANDSITEINFO& bsi)
{
    LPSTR cStr = strTemp[nTemps++];
    StringCchPrintfA(cStr, _countof(strTemp[0]),
        "{dwMask: %08x, dwState: %08x, dwStyle: %08x}",
        bsi.dwMask, bsi.dwState, bsi.dwStyle);
    return cStr;
}
