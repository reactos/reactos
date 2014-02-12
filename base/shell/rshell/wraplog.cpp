#include "precomp.h"
#include "wraplog.h"
#include <stdio.h>

static UINT openCount = 0;
static UINT callLevel;
static FILE*log;

static UINT nTemps;
static CHAR strTemp[10][256];

void WrapLogOpen()
{
    if (openCount == 0)
    {
        log = fopen("G:\\RShellWrap.log", "w");
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

void __cdecl WrapLogMsg(_Printf_format_string_ const char* msg, ...)
{
    va_list args;
    for (int i = 0; i < callLevel; i++)
        fputs("  ", log);
    fputs("-- ", log);
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
    fputs("ENTER >> ", log);
    va_start(args, msg);
    vfprintf(log, msg, args);
    va_end(args);
    fflush(log);
    callLevel++;
    nTemps = 0;
}

void __cdecl WrapLogExit(_Printf_format_string_ const char* msg, ...)
{
    va_list args;
    callLevel--;
    for (int i = 0; i < callLevel; i++)
        fputs("  ", log);
    fputs("EXIT <<< ", log);
    va_start(args, msg);
    vfprintf(log, msg, args);
    va_end(args);
    fflush(log);
    nTemps = 0;
}

template <class T>
LPSTR Wrap(const T& value);

template <>
LPSTR Wrap<GUID>(REFGUID guid)
{
    LPSTR cGuid = strTemp[nTemps++];
    StringCchPrintfA(cGuid, _countof(strTemp[0]),
        "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return cGuid;
}

template <>
LPSTR Wrap<RECT>(const RECT& rect)
{
    LPSTR cGuid = strTemp[nTemps++];
    StringCchPrintfA(cGuid, _countof(strTemp[0]),
        "{L: %d, T: %d, R: %d, B: %d}",
        rect.left, rect.top, rect.right, rect.bottom);
    return cGuid;
}

template <>
LPSTR Wrap<OLECMD>(const OLECMD& cmd)
{
    LPSTR cGuid = strTemp[nTemps++];
    StringCchPrintfA(cGuid, _countof(strTemp[0]),
        "{ID: %d, F: %d}",
        cmd.cmdID, cmd.cmdf);
    return cGuid;
}

template <>
LPSTR Wrap<MSG>(const MSG& msg)
{
    LPSTR cGuid = strTemp[nTemps++];
    StringCchPrintfA(cGuid, _countof(strTemp[0]),
        "{HWND: %d, Code: %d, W: %p, L: %p, T: %d, P.X: %d, P.Y: %d}",
        msg.hwnd, msg.message, msg.wParam, msg.lParam, msg.time, msg.pt.x, msg.pt.y);
    return cGuid;
}
