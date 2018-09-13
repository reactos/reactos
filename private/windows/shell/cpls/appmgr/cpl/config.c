/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This module contains code for getting machine configuration.

Author:

    Dave Hastings (daveh) creation-date-30-May-1997

Revision History:

--*/

#include <windows.h>
#include <userenv.h>
#include "pip.h"
#include "appmgr.h"

DWORD
ConfigThreadFunction(
    LPVOID Parameter
    );

APPMGRCONFIG AppMgrConfig;
HANDLE ConfigSyncHandle;

VOID
GetMachineConfig(
    VOID
    )
/*++

Routine Description:

    This routine gets the machine configuration information asynchronously.
    This includes things such as available media, whether the machine is 
    administered, and so forth.

Arguments:

    None

Return Value:

    None.  After completion, the global information structure is filled in
    and the associated event is signaled.

--*/
{

    HANDLE Thread;
    DWORD ThreadID;

    //
    // Create an event so we can synchronize the aux thread and page display
    //
    ConfigSyncHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

    //
    // Initialize the information structure
    //
    AppMgrConfig.Floppy = NotAvailableError;
    AppMgrConfig.CdRom = NotAvailableError;
    AppMgrConfig.Administered = NotAvailableError;
    AppMgrConfig.Internet = NotAvailableError;

    //
    // Create the thread that will do the information gathering
    //
    Thread = CreateThread(NULL, 0, ConfigThreadFunction, 0, 0, &ThreadID);

    //
    // Close the thread handle
    //
    if (Thread != INVALID_HANDLE_VALUE) {
        CloseHandle(Thread);
    }
}

DWORD
ConfigThreadFunction(
    LPVOID Parameter
    )
/*++

Routine Description:

    This routine does the actual work to figure out the machine
    configuration.

Arguments:

    Parameter -- unused.
    
Return Value:

    None.  When the thread is done, the ConfigSyncHandle will be signaled,
    and the info structure will be filled in.

BUGBUG:  policy should go here too

--*/
{
    // 105 = 26 * 4 + 1 (for extra null)
    WCHAR DriveStrings[105];
    PWCHAR CurrentDrive;
    DWORD DriveType;
    PWCHAR GptPath = NULL;
   
    //
    // Get the strings for all of the drives
    //
    GetLogicalDriveStrings(105, DriveStrings);

    //
    // Check each of the drives for a match to the correct type
    //
    CurrentDrive = DriveStrings;
    while (*CurrentDrive != L'\0') {

        DriveType = GetDriveType(CurrentDrive);

        if (DriveType == DRIVE_REMOVABLE) {
            //
            // if it's removable, indicate floppy availability
            //
            AppMgrConfig.Floppy = Available;
        } else if (DriveType == DRIVE_CDROM) {
            //
            // If it's a CD, indicate CD availability
            //
            AppMgrConfig.CdRom = Available;
        }

        //
        // Move on to the next drive letter
        //
        CurrentDrive += lstrlen(CurrentDrive) + 1;
    }
#if 0
    //
    // Check for GPTs
    //

    //
    // BugBug:  Should check the return value from this function
    // and do something appropriate.
    //

    GetGPTInfo(NULL, NULL, &GPTPath, NULL);

    if (GptPath != NULL) {
        AppMgrConfig.Administered = Available;
        // bugbug keep this info?
        LocalFree(GptPath);
    }
#endif
    //
    // Signal the event so that AppMgr can continue
    //
    SetEvent(ConfigSyncHandle);

    return 0;
}
    
