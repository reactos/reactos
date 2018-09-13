/*
**------------------------------------------------------------------------------
** Module:  System Data Cleaner COM object
** File:    common.h
**
** Purpose: various common stuff for this module
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef COMMON_H
#define COMMON_H


/*
**------------------------------------------------------------------------------
**  Microsoft C++ include files
**------------------------------------------------------------------------------
*/
#ifndef STRICT
	#define STRICT
#endif

#define INC_OLE2           // WIN32, get ole2 from windows.h

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>

/*
**------------------------------------------------------------------------------
** Global Defines 
**------------------------------------------------------------------------------
*/

#define DDEVCF_DOSUBDIRS            0x00000001 	// recursively search/remove 
#define DDEVCF_REMOVEAFTERCLEAN     0x00000002 	// remove from the registry after run once
#define DDEVCF_REMOVEREADONLY       0x00000004  // remove file even if it is read-only
#define DDEVCF_REMOVESYSTEM         0x00000008  // remove file even if it is system
#define DDEVCF_REMOVEHIDDEN         0x00000010  // remove file even if it is hidden
#define DDEVCF_DONTSHOWIFZERO       0x00000020  // don't show this cleaner if it has nothing to clean
#define DDEVCF_REMOVEDIRS           0x00000040  // Match filelist against directories and remove everything under them.
#define DDEVCF_RUNIFOUTOFDISKSPACE  0x00000080  // Only run if machine is out of disk space.
#define DDEVCF_REMOVEPARENTDIR      0x00000100  // remove the parent directory once done.
#define DDEVCF_PRIVATE_LASTACCESS   0x10000000  // use LastAccessTime

#define FILETIME_HOUR_HIGH          0x000000C9  // High DWORD for a FILETIME hour
#define FILETIME_HOUR_LOW           0x2A69C000  // Low DWORD for a FILETIME hour

#define CLSID_STRING_SIZE           39
#define DESCRIPTION_LENGTH          512
#define BUTTONTEXT_LENGTH           50
#define DISPLAYNAME_LENGTH          128
#define ARRAYSIZE(x)                (sizeof(x)/sizeof(x[0]))

#define REGSTR_VAL_BITMAPDISPLAY                TEXT("BitmapDisplay")
#define REGSTR_VAL_URL                          TEXT("URL")
#define REGSTR_VAL_FOLDER                       TEXT("folder")
#define REGSTR_VAL_FILELIST                     TEXT("FileList")
#define REGSTR_VAL_LASTACCESS                   TEXT("LastAccess")
#define REGSTR_VAL_FLAGS                        TEXT("Flags")
#define REGSTR_VAL_CLEANUPSTRING                TEXT("CleanupString")
#define REGSTR_VAL_FAILIFPROCESSRUNNING         TEXT("FailIfProcessRunning")
#define REGSTR_PATH_SETUP_SETUP                 TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup")

typedef struct tag_CleanFileStruct
{
    TCHAR           file[MAX_PATH];
    ULARGE_INTEGER  ulFileSize;
    BOOL            bSelected;
    BOOL            bDirectory;
    struct tag_CleanFileStruct    *pNext;
} CLEANFILESTRUCT, *PCLEANFILESTRUCT;

/*
**------------------------------------------------------------------------------
** Global function prototypes
**------------------------------------------------------------------------------
*/

// Dll Object count methods
UINT getDllObjectCount (void);

// Dll Lock count methods
UINT getDllLockCount (void);


#ifdef _DEBUG
   #define DEBUG
#endif

#ifdef DEBUG
#define MI_TRAP                     _asm int 3

void
DebugPrint(
    HRESULT hr,
    LPCTSTR  lpFormat,
    ...
    );

#define MiDebugMsg( args )          DebugPrint args

#else

#define MI_TRAP
#define MiDebugMsg( args )

#endif // DEBUG

#endif // COMMON_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
