/*
 * SetupAPI DiskSpace functions
 *
 * Copyright 2004 CodeWeavers (Aric Stewart)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "setupapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

typedef struct {
    WCHAR   lpzName[20];
    LONGLONG dwFreeSpace;
    LONGLONG dwWantedSpace;
} DRIVE_ENTRY, *LPDRIVE_ENTRY;

typedef struct {
    DWORD   dwDriveCount;
    DRIVE_ENTRY Drives[26];
} DISKSPACELIST, *LPDISKSPACELIST;


/***********************************************************************
 *		SetupCreateDiskSpaceListW  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListW(PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    WCHAR drives[255];
    DWORD rc;
    WCHAR *ptr;
    LPDISKSPACELIST list=NULL;

    rc = GetLogicalDriveStringsW(255,drives);

    if (rc == 0)
        return NULL;

    list = (LPDISKSPACELIST)HeapAlloc(GetProcessHeap(),0,sizeof(DISKSPACELIST));

    list->dwDriveCount = 0;
    
    ptr = drives;
    
    while (*ptr)
    {
        DWORD type = GetDriveTypeW(ptr);
        DWORD len;
        if (type == DRIVE_FIXED)
        {
            DWORD clusters;
            DWORD sectors;
            DWORD bytes;
            DWORD total;
            lstrcpyW(list->Drives[list->dwDriveCount].lpzName,ptr);
            GetDiskFreeSpaceW(ptr,&sectors,&bytes,&clusters,&total);
            list->Drives[list->dwDriveCount].dwFreeSpace = clusters * sectors *
                                                           bytes;
            list->Drives[list->dwDriveCount].dwWantedSpace = 0;
            list->dwDriveCount++;
        }
       len = lstrlenW(ptr);
       len++;
       ptr+=sizeof(WCHAR)*len;
    }
    return  (HANDLE)list;
}


/***********************************************************************
 *		SetupCreateDiskSpaceListA  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListA(PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    return SetupCreateDiskSpaceListW( Reserved1, Reserved2, Flags );
}


/***********************************************************************
 *		SetupAddInstallSectionToDiskSpaceListA  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddInstallSectionToDiskSpaceListA(HDSKSPC DiskSpace, 
                        HINF InfHandle, HINF LayoutInfHandle, 
                        LPSTR SectionName, PVOID Reserved1, UINT Reserved2)
{
    FIXME ("Stub\n");
    return TRUE;
}

/***********************************************************************
*		SetupQuerySpaceRequiredOnDriveA  (SETUPAPI.@)
*/
BOOL WINAPI SetupQuerySpaceRequiredOnDriveA(HDSKSPC DiskSpace, 
                        LPSTR DriveSpec, LONGLONG* SpaceRequired, 
                        PVOID Reserved1, UINT Reserved2)
{
    WCHAR driveW[20];
    int i;
    LPDISKSPACELIST list = (LPDISKSPACELIST)DiskSpace;
    BOOL rc = FALSE;
    WCHAR bkslsh[]= {'\\',0};

    MultiByteToWideChar(CP_ACP,0,DriveSpec,-1,driveW,20);

    lstrcatW(driveW,bkslsh);

    TRACE("Looking for drive %s\n",debugstr_w(driveW));
 
    for (i = 0; i < list->dwDriveCount; i++)
    {
        TRACE("checking drive %s\n",debugstr_w(list->Drives[i].lpzName));
        if (lstrcmpW(driveW,list->Drives[i].lpzName)==0)
        {
            rc = TRUE;
            *SpaceRequired = list->Drives[i].dwWantedSpace;
            break;
        }
    }

    return rc;
}

/***********************************************************************
*		SetupDestroyDiskSpaceList  (SETUPAPI.@)
*/
BOOL WINAPI SetupDestroyDiskSpaceList(HDSKSPC DiskSpace)
{
    LPDISKSPACELIST list = (LPDISKSPACELIST)DiskSpace;
    HeapFree(GetProcessHeap(),0,list);
    return TRUE; 
}
