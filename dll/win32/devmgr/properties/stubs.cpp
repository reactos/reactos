/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/stubs.c
 * PURPOSE:         devmgr.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 *
 *                  Some helpful resources:
 *                    http://support.microsoft.com/default.aspx?scid=kb;%5BLN%5D;815320 (DEAD_LINK)
 *                    https://web.archive.org/web/20050321020634/http://www.jsifaq.com/SUBO/tip7400/rh7482.htm
 *                    https://web.archive.org/web/20050909185602/http://www.jsifaq.com/SUBM/tip6400/rh6490.htm
 *
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */

#include "precomp.h"

// remove me
BOOL
WINAPI
InstallDevInst(
IN HWND hWndParent,
IN LPCWSTR InstanceId,
IN BOOL bUpdate,
OUT LPDWORD lpReboot)
{
    return FALSE;
}

unsigned long __stdcall pSetupGuidFromString(wchar_t const *, struct _GUID *)
{
    return 1;
}
