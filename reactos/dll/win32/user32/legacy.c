#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(userlegacy);

LPVOID
WINAPI
GlobalLock16(HGLOBAL16 h)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL16
WINAPI
GlobalUnlock16(HGLOBAL16 h)
{
    UNIMPLEMENTED;
    return FALSE;
}

HGLOBAL16
WINAPI
GlobalAlloc16(UINT16 u, DWORD d)
{
    UNIMPLEMENTED;
    return 0;
}

HGLOBAL16
WINAPI
GlobalFree16(HGLOBAL16 h)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
WINAPI
GlobalSize16(HGLOBAL16 h)
{
    UNIMPLEMENTED;
    return 0;
}

SEGPTR
WINAPI
LocalLock16(HLOCAL16 h)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL16
WINAPI
LocalUnlock16(HLOCAL16 h)
{
    UNIMPLEMENTED;
    return FALSE;
}

LPVOID
WINAPI
LockResource16(HGLOBAL16 h)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL16
WINAPI
FreeResource16(HGLOBAL16 h)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINAPI
ReleaseThunkLock(DWORD *mutex_count)
{
}

VOID
WINAPI
RestoreThunkLock(DWORD mutex_count)
{
}
