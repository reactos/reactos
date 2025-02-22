/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS pending moves operations interactions tool
 * FILE:            cmdutils/movefile/movefile.c
 * PURPOSE:         Queue move operations for next reboot
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    /* We need source + target */
    if (argc < 3)
    {
        _ftprintf(stderr, _T("Missing arguments\nUsage: %s source target\nUse \"\" as target is you want deletion\n"), argv[0]);
        return 1;
    }

    /* If target is empty, it means deletion, so provide null pointer */
    if (!MoveFileEx(argv[1], (argv[2][0] == 0 ? NULL : argv[2]), MOVEFILE_DELAY_UNTIL_REBOOT))
    {
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
        return 1;
    }

    return 0;
}
