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

VOID NTAPI
RosUserConnectCsrss();

WINE_DEFAULT_DEBUG_CHANNEL(userlegacy);

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

/*
 * Private calls for CSRSS
 */
VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
    UNIMPLEMENTED;
}

VOID
WINAPI
PrivateCsrssInitialized(VOID)
{
    /* Perform a reactos only CSRSS connection */
    RosUserConnectCsrss();
}

