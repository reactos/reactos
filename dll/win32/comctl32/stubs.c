#include <windef.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(comctl32);

typedef PVOID PREADERMODEINFO;

HWND
WINAPI
CreatePage(
    DWORD dwInitParam,
    HWND hWndParent)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
WINAPI
CreateProxyPage(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return NULL;
}

VOID
WINAPI
DoReaderMode(
    PREADERMODEINFO prmi)
{
    UNIMPLEMENTED;
}

VOID
WINAPI
SHGetProcessDword(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
}
