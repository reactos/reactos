/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    badapps.c

Abstract:

    Implements a library for CheckBadApps key.

Author:

    Calin Negreanu (calinn)  20-Jan-1999

Revision History:

    <alias> <date> <comments>

--*/

#include <windows.h>
#include <winnt.h>
#include <shlwapi.h>
#include <badapps.h>
#include "utils.h"
#include "version.h"
#include "modules.h"

typedef struct {
    PCTSTR FileName;
    BOOL FindDataLoaded;
    WIN32_FIND_DATA FindData;
    PBYTE MappedImage;
    HANDLE FileHandle;
    HANDLE MapHandle;
    VERSION_STRUCT VersionData;
    HKEY PrevOsKey;
} FILE_DATA, *PFILE_DATA;

typedef BOOL (VERSION_CHECK_PROTOTYPE) (PFILE_DATA FileData, DWORD DataSize, PBYTE Data);
typedef VERSION_CHECK_PROTOTYPE * PVERSION_CHECK_PROTOTYPE;

typedef struct {
    DWORD   VersionId;
    PVERSION_CHECK_PROTOTYPE VersionCheck;
} VERSION_DATA, *PVERSION_DATA;

#define LIBARGS(id, fn) VERSION_CHECK_PROTOTYPE fn;
#define TOOLARGS(name, dispName, allowance, edit, query, output)

VERSION_STAMPS

#undef TOOLARGS
#undef LIBARGS

#define LIBARGS(id, fn) {id, fn},
#define TOOLARGS(name, dispName, allowance, edit, query, output)
VERSION_DATA g_VersionData [] = {
                  VERSION_STAMPS
                  {0, NULL}
                  };
#undef TOOLARGS
#undef LIBARGS

#define FD_FINDDATA     0x00000001
#define FD_MAPPINGDATA  0x00000002
#define FD_VERSIONDATA  0x00000003
#define FD_PREVOSDATA   0x00000004

BOOL
ShLoadFileData (
    IN OUT  PFILE_DATA FileData,
    IN      DWORD FileDataType
    )
{
    LONG status;
    HANDLE findHandle;
    UINT oldMode;

    switch (FileDataType) {
    case FD_FINDDATA:
        if (!FileData->FindDataLoaded) {

            oldMode = SetErrorMode (SEM_FAILCRITICALERRORS);

            findHandle = FindFirstFile (FileData->FileName, &FileData->FindData);

            SetErrorMode(oldMode);

            if (findHandle == INVALID_HANDLE_VALUE) {
                return FALSE;
            } else {
                FindClose (findHandle);
                FileData->FindDataLoaded = TRUE;
            }
        }
        break;
    case FD_MAPPINGDATA:
        if (!FileData->MappedImage) {
            FileData->MappedImage = ShMapFileIntoMemory (
                                        FileData->FileName,
                                        &FileData->FileHandle,
                                        &FileData->MapHandle
                                        );
            if (!FileData->MappedImage) {
                return FALSE;
            }
        }
        break;
    case FD_VERSIONDATA:
        if (!FileData->VersionData.VersionBuffer) {
            if (!ShCreateVersionStruct (&FileData->VersionData, FileData->FileName)) {
                FileData->VersionData.VersionBuffer = NULL;
                return FALSE;
            }
        }
        break;
    case FD_PREVOSDATA:
        if (!FileData->PrevOsKey) {
            status = RegOpenKey (
                        HKEY_LOCAL_MACHINE,
                        S_KEY_PREVOSVERSION,
                        &FileData->PrevOsKey
                        );
            if (status != ERROR_SUCCESS) {
                return FALSE;
            }
        }
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL
ShFreeFileData (
    IN OUT  PFILE_DATA FileData
    )
{
    FileData->FindDataLoaded = FALSE;
    if (FileData->MappedImage) {
        ShUnmapFile (
            FileData->MappedImage,
            FileData->FileHandle,
            FileData->MapHandle
            );
        FileData->MappedImage = NULL;
        FileData->FileHandle = NULL;
        FileData->MapHandle = NULL;
    }
    if (FileData->VersionData.VersionBuffer) {
        ShDestroyVersionStruct (&FileData->VersionData);
        FileData->VersionData.VersionBuffer = NULL;
    }
    if (FileData->PrevOsKey) {
        RegCloseKey (FileData->PrevOsKey);
        FileData->PrevOsKey = NULL;
    }
    return TRUE;
}

BOOL
ShCheckFileSize (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    if (!ShLoadFileData (FileData, FD_FINDDATA)) {
        return FALSE;
    }
    return (*((PDWORD) Data) == FileData->FindData.nFileSizeLow);
}

BOOL
ShCheckModuleType (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    if (!ShLoadFileData (FileData, FD_MAPPINGDATA)) {
        return FALSE;
    }
    return (*((PDWORD) Data) == ShGetModuleType (FileData->MappedImage));
}

BOOL
ShCheckBinFileVer (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONGLONG value;
    PULONGLONG mask;
    ULONGLONG currVer;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        currVer = 0;
    } else {
        currVer = ShVerGetFileVer (&FileData->VersionData);
    }
    value = (PULONGLONG) Data;
    mask = (PULONGLONG) (Data + sizeof (ULONGLONG));
    return (((*value) & (*mask)) == currVer);
}

BOOL
ShCheckBinProductVer (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONGLONG value;
    PULONGLONG mask;
    ULONGLONG currVer;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        currVer = 0;
    } else {
        currVer = ShVerGetProductVer (&FileData->VersionData);
    }
    value = (PULONGLONG) Data;
    mask = (PULONGLONG) (Data + sizeof (ULONGLONG));
    return (((*value) & (*mask)) == currVer);
}

BOOL
ShCheckUpToBinProductVer (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PDWORD ProdVerMS;
    PDWORD ProdVerLS;
    ULONGLONG ProdVerData;
    PDWORD BadProdVerMS;
    PDWORD BadProdVerLS;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        ProdVerData = 0;
    } else {
        ProdVerData = ShVerGetProductVer (&FileData->VersionData);
    }
    BadProdVerMS = BadProdVerLS = (PDWORD) Data;
    BadProdVerMS ++;
    ProdVerMS = ProdVerLS = (PDWORD) (&ProdVerData);
    ProdVerMS ++;
    return (*ProdVerMS < *BadProdVerMS) || ((*ProdVerMS == *BadProdVerMS) && (*ProdVerLS <= *BadProdVerLS));
}

BOOL
ShCheckFileDateHi (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONGLONG value;
    ULONGLONG currVer;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        currVer = 0;
    } else {
        currVer = ShVerGetFileDateHi (&FileData->VersionData);
    }
    value = (PULONGLONG) Data;
    return (*value == currVer);
}

BOOL
ShCheckFileDateLo (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONGLONG value;
    ULONGLONG currVer;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        currVer = 0;
    } else {
        currVer = ShVerGetFileDateLo (&FileData->VersionData);
    }
    value = (PULONGLONG) Data;
    return (*value == currVer);
}

BOOL
ShCheckFileVerOs (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONG value;
    ULONG currVer;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        currVer = 0;
    } else {
        currVer = ShVerGetFileVerOs (&FileData->VersionData);
    }
    value = (PULONG) Data;
    return (*value == currVer);
}

BOOL
ShCheckFileVerType (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONG value;
    ULONG currVer;
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        currVer = 0;
    } else {
        currVer = ShVerGetFileVerType (&FileData->VersionData);
    }
    value = (PULONG) Data;
    return (*value == currVer);
}

BOOL
ShCheckFileCheckSum (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONG value;
    if (!ShLoadFileData (FileData, FD_FINDDATA)) {
        return FALSE;
    }
    if (!ShLoadFileData (FileData, FD_MAPPINGDATA)) {
        return FALSE;
    }
    value = (PULONG) Data;
    return (*value == ShGetCheckSum (FileData->FindData.nFileSizeLow, FileData->MappedImage));
}

BOOL
ShCheckFilePECheckSum (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PULONG value;
    if (!ShLoadFileData (FileData, FD_MAPPINGDATA)) {
        return FALSE;
    }
    value = (PULONG) Data;
    return (*value == ShGetPECheckSum (FileData->MappedImage));
}

BOOL
ShCheckStrVersion (
    IN      PFILE_DATA FileData,
    IN      PCTSTR ValueToCheck,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    BOOL result = FALSE;
    PTSTR localData = NULL;
#ifndef UNICODE
    PSTR convStr = NULL;
    INT converted;
#endif
    if (!ShLoadFileData (FileData, FD_VERSIONDATA)) {
        return FALSE;
    }

#ifdef UNICODE
    localData = (PTSTR)Data;
#else
    convStr = HeapAlloc (GetProcessHeap (), 0, DataSize);
    converted = WideCharToMultiByte (
                    CP_ACP,
                    0,
                    (PWSTR)Data,
                    -1,
                    convStr,
                    DataSize,
                    NULL,
                    NULL
                    );
    if (!converted) {
        HeapFree (GetProcessHeap (), 0, convStr);
        return FALSE;
    }
    localData = convStr;
#endif
    result = ShGlobalVersionCheck (&FileData->VersionData, ValueToCheck, localData);
#ifndef UNICODE
    HeapFree (GetProcessHeap (), 0, convStr);
#endif
    return result;
}

BOOL
ShCheckCompanyName (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_COMPANYNAME, DataSize, Data));
}

BOOL
ShCheckProductVersion (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_PRODUCTVERSION, DataSize, Data));
}

BOOL
ShCheckProductName (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_PRODUCTNAME, DataSize, Data));
}

BOOL
ShCheckFileDescription (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_FILEDESCRIPTION, DataSize, Data));
}

BOOL
ShCheckFileVersion (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_FILEVERSION, DataSize, Data));
}

BOOL
ShCheckOriginalFileName (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_ORIGINALFILENAME, DataSize, Data));
}

BOOL
ShCheckInternalName (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_INTERNALNAME, DataSize, Data));
}

BOOL
ShCheckLegalCopyright (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    return (ShCheckStrVersion (FileData, S_VER_LEGALCOPYRIGHT, DataSize, Data));
}

BOOL
ShCheck16BitDescription (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    PTSTR value;
    BOOL result;
    PTSTR localData = NULL;
#ifndef UNICODE
    PSTR convStr = NULL;
    INT converted;
#endif

    value = ShGet16ModuleDescription (FileData->MappedImage);
    if (!value) {
        return FALSE;
    }
#ifdef UNICODE
    localData = (PTSTR)Data;
#else
    convStr = HeapAlloc (GetProcessHeap (), 0, DataSize);
    converted = WideCharToMultiByte (
                    CP_ACP,
                    0,
                    (PWSTR)Data,
                    -1,
                    convStr,
                    DataSize,
                    NULL,
                    NULL
                    );
    if (!converted) {
        HeapFree (GetProcessHeap (), 0, convStr);
        return FALSE;
    }
    localData = convStr;
#endif
    result = ShIsPatternMatch (localData, value);
#ifndef UNICODE
    HeapFree (GetProcessHeap (), 0, convStr);
#endif
    HeapFree (GetProcessHeap (), 0, value);
    return result;
}

BOOL
pShLoadPrevOsData (
    IN      PFILE_DATA FileData,
    IN      PCTSTR ValueName,
    OUT     PDWORD Value
    )
{
    LONG status;
    BOOL result = FALSE;
    DWORD type;
    DWORD valueSize = sizeof (DWORD);

    if (ShLoadFileData (FileData, FD_PREVOSDATA)) {

        status = RegQueryValueEx (FileData->PrevOsKey, ValueName, NULL, &type, (PBYTE)Value, &valueSize);
        if ((status == ERROR_SUCCESS) &&
            (type == REG_DWORD)
            ) {
            result = TRUE;
        }
    }
    return result;
}

BOOL
ShCheckPrevOsMajorVersion (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    BOOL result = FALSE;
    DWORD value = 0;

    if (pShLoadPrevOsData (FileData, S_VAL_MAJORVERSION, &value)) {
        result = (value == *(PDWORD)(Data));
    }
    return result;
}

BOOL
ShCheckPrevOsMinorVersion (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    BOOL result = FALSE;
    DWORD value = 0;

    if (pShLoadPrevOsData (FileData, S_VAL_MINORVERSION, &value)) {
        result = (value == *(PDWORD)(Data));
    }
    return result;
}

BOOL
ShCheckPrevOsPlatformId (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    BOOL result = FALSE;
    DWORD value = 0;

    if (pShLoadPrevOsData (FileData, S_VAL_PLATFORMID, &value)) {
        result = (value == *(PDWORD)(Data));
    }
    return result;
}

BOOL
ShCheckPrevOsBuildNo (
    IN      PFILE_DATA FileData,
    IN      DWORD DataSize,
    IN      PBYTE Data
    )
{
    BOOL result = FALSE;
    DWORD value = 0;

    if (pShLoadPrevOsData (FileData, S_VAL_BUILDNO, &value)) {
        result = (value == *(PDWORD)(Data));
    }
    return result;
}

BOOL
DoesPathExist (
    IN      PCTSTR Path
    )
{
    BOOL result = FALSE;
    DWORD errMode;

    if (Path)
    {
        errMode = SetErrorMode (SEM_FAILCRITICALERRORS);

        result = (GetFileAttributes (Path) != 0xFFFFFFFF);

        SetErrorMode(errMode);
    }

    return result;
}

BOOL
pShCheckBlob (
    IN      PCTSTR FileName,
    IN      PBYTE Blob,
    IN      BOOL QuickMode
    )
{
    FILE_DATA fileData;
    PVERSION_DATA p;
    DWORD dataId;
    DWORD dataSize;
    BOOL result = TRUE;
    PTSTR reqFile = NULL;
	PTSTR oldReqFile = NULL;
    PCTSTR filePtr = NULL;
    UINT prefixPathChars;
    PTSTR localData = NULL;
#ifndef UNICODE
    PSTR convStr = NULL;
    INT converted;
#endif

    ZeroMemory (&fileData, sizeof (FILE_DATA));

    fileData.FileName = FileName;
    if (!DoesPathExist (fileData.FileName)) {
        return FALSE;
    }

    filePtr = ShGetFileNameFromPath (FileName);
    if (!filePtr) {
        return FALSE;
    }

    prefixPathChars = (UINT)(filePtr - FileName);

    __try {
        dataId = *((PDWORD) Blob);
        while (dataId) {
            if (dataId == VTID_REQFILE) {

                Blob += sizeof (DWORD);
                dataSize = *((PDWORD) Blob);
                if (!dataSize) {
                    // should never happen
                    dataSize = 1;
                }
                Blob += sizeof (DWORD);

                // if this is the first additional file, reqFile is NULL
				oldReqFile = reqFile;

                // dataSize includes terminating nul character
                reqFile = HeapAlloc (GetProcessHeap (), 0, prefixPathChars * sizeof (TCHAR) + dataSize);

                if (!reqFile) {
                    result = FALSE;
                    __leave;
                }

                lstrcpyn (reqFile, fileData.FileName, prefixPathChars + 1);

                // if this is the first additional file, oldReqFile is NULL
				if (oldReqFile) {
                    HeapFree (GetProcessHeap (), 0, oldReqFile);
                }

                #ifdef UNICODE
                    localData = (PTSTR)Blob;
                #else
                    convStr = HeapAlloc (GetProcessHeap (), 0, dataSize);
                    converted = WideCharToMultiByte (
                                    CP_ACP,
                                    0,
                                    (PWSTR)Blob,
                                    -1,
                                    convStr,
                                    dataSize,
                                    NULL,
                                    NULL
                                    );
                    if (!converted) {
                        HeapFree (GetProcessHeap (), 0, convStr);
                        result = FALSE;
                        __leave;
                    }
                    localData = convStr;
                #endif

                lstrcpyn (reqFile + prefixPathChars, localData, dataSize / sizeof (TCHAR));

                reqFile [prefixPathChars + (dataSize / sizeof (TCHAR)) - 1] = 0;

                ShFreeFileData (&fileData);

                fileData.FileName = reqFile;

                if (!DoesPathExist (fileData.FileName)) {
                    result = FALSE;
                    __leave;
                }

                Blob += dataSize;

            } else {
                if (dataId >= VTID_LASTID) {
                    result = FALSE;
                    __leave;
                }

                p = g_VersionData + (dataId - VTID_REQFILE - 1);

                if (p->VersionId != dataId) {
                    result = FALSE;
                    __leave;
                }

                Blob += sizeof (DWORD);
                dataSize = *((PDWORD) Blob);
                Blob += sizeof (DWORD);
                if (!QuickMode) {
                    if (!p->VersionCheck (&fileData, dataSize, Blob)) {
                        result = FALSE;
                        __leave;
                    }
                }
                Blob += dataSize;
            }
            dataId = *((PDWORD) Blob);
        }
    }
    __finally {
        if (reqFile) {
            HeapFree (GetProcessHeap (), 0, reqFile);
        }
        ShFreeFileData (&fileData);
    }
    return result;
}

BOOL
SHIsBadApp (
    IN      PBADAPP_DATA Data,
    OUT     PBADAPP_PROP Prop
    )
{
    BOOL result = FALSE;
    PBADAPP_PROP appProp;

    __try {
        if (Data->Size != sizeof (BADAPP_DATA)) {
            return FALSE;
        }
        if (Prop->Size != sizeof (BADAPP_PROP)) {
            return FALSE;
        }
        if (*(PDWORD)(Data->Blob) != sizeof (BADAPP_PROP)) {
            return FALSE;
        }
        if (pShCheckBlob (Data->FilePath, Data->Blob + sizeof (BADAPP_PROP), TRUE)) {
            result = pShCheckBlob (Data->FilePath, Data->Blob + sizeof (BADAPP_PROP), FALSE);
        }
        if (result) {
            appProp = (PBADAPP_PROP) Data->Blob;
            Prop->MsgId = appProp->MsgId;
            Prop->AppType = appProp->AppType;
        }
    }
    __except (1) {
        result = FALSE;
    }
    return result;
}
