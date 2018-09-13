/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    localin.c

Abstract:

    This module contains helpers for installing from local media.

Author:

    Dave Hastings (daveh) creation-date 12-May-1997


Notes:

    bugbug we don't currently handle the special cases or international
    names that appwiz used to

    bugbug we don't do anything for darwin apps

Revision History:

    
--*/

#include <windows.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

BOOL
GetInstallExe(
    HINSTANCE Instance,
    PWSTR DrivePath,
    PWSTR SetupPath,
    DWORD SetupPathSize
    );

BOOL
LoadAndFixString(
    HINSTANCE Instance,
    UINT Resource,
    LPWSTR String,
    DWORD StringLength
    );

BOOL
FindInstallProgram(
    DWORD DesiredDriveType,
    LPWSTR SetupPath,
    DWORD SetupPathSize
    )
/*++

Routine Description:

    This routine scans the available drives looking for the first install program
    it can find on an appropriately typed drive.

Arguments:

    DesiredDriveType - Supplies the type of drive to check (DRIVE_CDROM or DRIVE_FLOPPY).
    SetupPath - Returns the path to the setup program.
    SetupPathSize - Supplies the size of the buffer in characters.

Return Value:

    True for success.

--*/
{
    // 105 = 26 * 4 + 1 (for extra null)
    WCHAR DriveStrings[105];
    PWCHAR CurrentDrive;
    UINT CurrentDriveType;
	BOOL Result;
    
    //
    // Get the strings for all of the drives
    //
    GetLogicalDriveStrings(105, DriveStrings);

    //
    // Check each of the drives for a match to the correct type
    //
    CurrentDrive = DriveStrings;
    while (*CurrentDrive != L'\0') {

        CurrentDriveType = GetDriveType(CurrentDrive);

        //
        // if it's the correct drive type, try to find an install program
        //
        if (CurrentDriveType == DesiredDriveType) {
            Result = GetInstallExe(Instance, CurrentDrive, SetupPath, SetupPathSize);
			if (Result) {
				return TRUE;
			}
        }

        //
        // Move on to the next drive letter
        //
        CurrentDrive += lstrlen(CurrentDrive) + 1;
    }
	return FALSE;
}

BOOL
GetInstallExe(
    HINSTANCE Instance,
    PWSTR DrivePath,
    PWSTR SetupPath,
    DWORD SetupPathSize
    )
/*++

Routine Description:

    This function locates the install executable.  If we find a 16 bit setup app,
    we remember it and keep looking for a 32 bit app.  If we don't find a 32 bit
    app, we use the 16 bit version.

Arguments:

    DrivePath - Supplies the root of the drive to check.
    SetupPath - Returns the path to the setup program.
    SetupPathSize - Supplies the length in characters of SetupPath.

Return Value:

    True for success

--*/
{
	WCHAR InstallNameStrings[100];
	WCHAR InstallNameExtensionStrings[100];
	WCHAR ExeName[MAX_PATH];
	WCHAR InstallExeName[MAX_PATH];
	PWCHAR CurrentName, CurrentExtension;
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;
	DWORD FileType;

	//
	// Get the installer names and extensions from
	// our resources
	//
	// bugbug errors?
	LoadAndFixString(
		Instance,
		IDS_INSTALL_NAMES,
		InstallNameStrings,
		100
		);

	LoadAndFixString(
		Instance,
		IDS_INSTALL_EXTENSIONS,
		InstallNameExtensionStrings,
		100
		);

	//
	// Iterate over each name and extension untill
	// we find a suitable application
	//
	CurrentName = InstallNameStrings;
	InstallExeName[0] = L'\0';
	while (*CurrentName) {
		CurrentExtension = InstallNameExtensionStrings;
		while (*CurrentExtension) {
			//
			// Build the name to search for
			//
			lstrcpy(ExeName, DrivePath);
			lstrcat(ExeName, CurrentName);
			lstrcat(ExeName, L".");
			lstrcat(ExeName, CurrentExtension);

			//
			// See if the file exists
			//
			FindHandle = FindFirstFile(ExeName, &FindData);
			
			if (FindHandle != INVALID_HANDLE_VALUE) {

				FindClose(FindHandle);

				//
				// Get the actual file and determine if it is
				// a windows app or not
				//
				lstrcpy(InstallExeName, DrivePath);
				lstrcat(InstallExeName, FindData.cFileName);

				FileType = SHGetFileInfo(InstallExeName, 0, NULL, 0, SHGFI_EXETYPE);

				if (HIWORD(FileType) != 0) {
					lstrcpy(SetupPath, InstallExeName);
					return TRUE;
				}
			}

			CurrentExtension += lstrlen(CurrentExtension) + 1;
		}

		CurrentName += lstrlen(CurrentName) + 1;
	}

	if (InstallExeName[0] != L'\0') {
		lstrcpy(SetupPath, InstallExeName);
		return TRUE;
	}

	return FALSE;
}

BOOL
LoadAndFixString(
    HINSTANCE Instance,
    UINT Resource,
    LPWSTR String,
    DWORD StringLength
    )
/*++

Routine Description:

    This function loads strings from a resource and replaces @ with '\0'.

Arguments:

    Resource - Supplies resource ID for the string.
    String - Returns the fixed string.
    StringLength - Supplies the length of String.

Return Value:

    True for success

--*/
{
    BOOL RetVal;
    PWCHAR CurrentChar;

    //
    // Load the string, saving room for the second null at the end
    //
    RetVal = LoadString(Instance, Resource, String, StringLength - 1);

    if (RetVal) {
        CurrentChar = String;
        while (*CurrentChar) {
            if (*CurrentChar == L'@') {
                *CurrentChar = L'\0';
            }
            CurrentChar++;
        }

        //
        // Add the final null
        //
        *(CurrentChar + 1) = L'\0';
    }

    return RetVal;
}

BOOL
ExecSetup(
	PWSTR SetupName,
	HWND DialogWindow,
	HINSTANCE Instance
	)
/*++

Routine Description:

    This routine starts the specified setup program using shell exec.

Arguments:

    SetupName - Supplies the path and name of the setup program.
    
Return Value:

    TRUE for success.

--*/
{
	SHELLEXECUTEINFO ExecuteInfo;
	BOOL Result;

	ExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ExecuteInfo.hwnd = DialogWindow;
	ExecuteInfo.lpVerb = NULL;
	ExecuteInfo.lpFile = SetupName;
    ExecuteInfo.lpParameters = NULL;
	ExecuteInfo.lpDirectory = NULL;
	ExecuteInfo.nShow = SW_SHOWDEFAULT;
	ExecuteInfo.hInstApp = Instance;

	Result = ShellExecuteEx(&ExecuteInfo);

	if (Result) {
		WaitForInputIdle(
			ExecuteInfo.hProcess,
			50000
			);
		CloseHandle(ExecuteInfo.hProcess);
	}

	return Result;
}

BOOL
BrowseLocalMedia(
    DWORD MediaType,
    PWCHAR SetupPath,
    HWND Parent
    )
/*++

Routine Description:

    This routine puts up a common file dialog rooted on the 
    appropriate drive, so the user can select an install program.

Arguments:

    MediaType -- Supplies the type of the media.
    SetupPath -- Returns the full path of the setup program

Return Value:

    TRUE for Success.

  bugbug -- We scan the drives everywhere.  It's probably worth
  doing it once and saving the information.

--*/
{
    OPENFILENAME FileNameInfo;
    WCHAR DriveStrings[105];
    PWCHAR CurrentDrive;
    DWORD CurrentDriveType;

    //
    // Get the strings for all of the drives
    //
    GetLogicalDriveStrings(105, DriveStrings);

    //
    // Check each of the drives for a match to the correct type
    //
    CurrentDrive = DriveStrings;
    while (*CurrentDrive != L'\0') {

        CurrentDriveType = GetDriveType(CurrentDrive);

        //
        // if it's the correct drive type, try to find an install program
        //
        if (CurrentDriveType == MediaType) {
            break;
        }

        //
        // Move on to the next drive letter
        //
        CurrentDrive += lstrlen(CurrentDrive) + 1;
    }

    //
    // Set up the OPENFILENAME structure to point at the 
    // root of the desired drive, and use the correct set
    // of extensions.
    //
    FileNameInfo.lStructSize = sizeof(OPENFILENAME);
    FileNameInfo.hwndOwner = Parent;
    FileNameInfo.hInstance = Instance;
    FileNameInfo.lpstrFilter = NULL; // bugbug
    FileNameInfo.lpstrCustomFilter = NULL; 
    FileNameInfo.nMaxCustFilter = 0;
    FileNameInfo.nFilterIndex = 0;
    FileNameInfo.lpstrFile = SetupPath;
    FileNameInfo.nMaxFile = MAX_PATH;
    FileNameInfo.lpstrFileTitle = NULL;
    FileNameInfo.nMaxFileTitle = 0;
    // bugbug the following causes drive not ready if someone
    // click browse without media in the drive
    // FileNameInfo.lpstrInitialDir = CurrentDrive;
    FileNameInfo.lpstrInitialDir = NULL;
    FileNameInfo.lpstrTitle = NULL; //bugbug
    FileNameInfo.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES;
    FileNameInfo.nFileOffset = 0;
    FileNameInfo.nFileExtension = 0;
    FileNameInfo.lpstrDefExt = NULL; // bugbug
    FileNameInfo.lCustData = 0;
    FileNameInfo.lpfnHook = NULL;
    FileNameInfo.lpTemplateName = NULL;


    return GetOpenFileName(&FileNameInfo);
}