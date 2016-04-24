/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/bootok/bootok.c
 * PURPOSE:         Boot Acceptance Application
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <tchar.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>

/* FUNCTIONS ****************************************************************/

int
_tmain(int argc, TCHAR *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    if (!NotifyBootConfigStatus(TRUE))
    {
        _tprintf(_T("NotifyBootConfigStatus failed! (Error: %lu)\n"),
                 GetLastError());
    }

    return 0;
}
