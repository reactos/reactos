/*
 * SetupAPI driver-related functions
 *
 * Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR DotCoInstallers[]  = {'.','C','o','I','n','s','t','a','l','l','e','r','s',0};
static const WCHAR InfDirectory[] = {'i','n','f','\\',0};
static const WCHAR Version[]  = {'V','e','r','s','i','o','n',0};

static const WCHAR INF_MANUFACTURER[]  = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR INF_PROVIDER[]  = {'P','r','o','v','i','d','e','r',0};
static const WCHAR INF_DRIVER_VER[]  = {'D','r','i','v','e','r','V','e','r',0};


/***********************************************************************
 *		struct InfFileDetails management
 */
static VOID
ReferenceInfFile(struct InfFileDetails* infFile)
{
    InterlockedIncrement(&infFile->References);
}

VOID
DereferenceInfFile(struct InfFileDetails* infFile)
{
    if (InterlockedDecrement(&infFile->References) == 0)
    {
        SetupCloseInfFile(infFile->hInf);
        HeapFree(GetProcessHeap(), 0, infFile);
    }
}

struct InfFileDetails *
CreateInfFileDetails(
    IN LPCWSTR FullInfFileName)
{
    struct InfFileDetails *details;
    PWCHAR last;
    DWORD Needed;

    Needed = FIELD_OFFSET(struct InfFileDetails, szData)
        + strlenW(FullInfFileName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

    details = HeapAlloc(GetProcessHeap(), 0, Needed);
    if (!details)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    memset(details, 0, Needed);
    strcpyW(details->szData, FullInfFileName);
    last = strrchrW(details->szData, '\\');
    if (last)
    {
        details->DirectoryName = details->szData;
        details->FileName = last + 1;
        *last = '\0';
    }
    else
        details->FileName = details->szData;
    ReferenceInfFile(details);
    details->hInf = SetupOpenInfFileW(FullInfFileName, NULL, INF_STYLE_WIN4, NULL);
    if (details->hInf == INVALID_HANDLE_VALUE)
    {
        HeapFree(GetProcessHeap(), 0, details);
        return NULL;
    }
    return details;
}

BOOL
DestroyDriverInfoElement(struct DriverInfoElement* driverInfo)
{
    DereferenceInfFile(driverInfo->InfFileDetails);
    HeapFree(GetProcessHeap(), 0, driverInfo->MatchingId);
    HeapFree(GetProcessHeap(), 0, driverInfo);
    return TRUE;
}

/***********************************************************************
 *		Helper functions for SetupDiBuildDriverInfoList
 */
static BOOL
AddDriverToList(
    IN PLIST_ENTRY DriverListHead,
    IN DWORD DriverType, /* SPDIT_CLASSDRIVER or SPDIT_COMPATDRIVER */
    IN LPGUID ClassGuid,
    IN INFCONTEXT ContextDevice,
    IN struct InfFileDetails *InfFileDetails,
    IN LPCWSTR InfFile,
    IN LPCWSTR ProviderName,
    IN LPCWSTR ManufacturerName,
    IN LPCWSTR MatchingId,
    FILETIME DriverDate,
    DWORDLONG DriverVersion,
    IN DWORD Rank)
{
    struct DriverInfoElement *driverInfo = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD RequiredSize = 128; /* Initial buffer size */
    BOOL Result = FALSE;
    PLIST_ENTRY PreviousEntry;
    LPWSTR InfInstallSection = NULL;
    BOOL ret = FALSE;

    driverInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(struct DriverInfoElement));
    if (!driverInfo)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    memset(driverInfo, 0, sizeof(struct DriverInfoElement));

    driverInfo->Details.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    driverInfo->Details.Reserved = (ULONG_PTR)driverInfo;

    /* Copy InfFileName field */
    strncpyW(driverInfo->Details.InfFileName, InfFile, MAX_PATH - 1);
    driverInfo->Details.InfFileName[MAX_PATH - 1] = '\0';

    /* Fill InfDate field */
    hFile = CreateFile(
        InfFile,
        GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        goto cleanup;
    Result = GetFileTime(hFile, NULL, NULL, &driverInfo->Details.InfDate);
    if (!Result)
        goto cleanup;

    /* Fill SectionName field */
    Result = SetupGetStringFieldW(
        &ContextDevice,
        1,
        driverInfo->Details.SectionName, LINE_LEN,
        NULL);
    if (!Result)
        goto cleanup;

    /* Fill DrvDescription field */
    Result = SetupGetStringFieldW(
        &ContextDevice,
        0, /* Field index */
        driverInfo->Details.DrvDescription, LINE_LEN,
        NULL);

    /* Copy MatchingId information */
    if (MatchingId)
    {
        driverInfo->MatchingId = HeapAlloc(GetProcessHeap(), 0, (strlenW(MatchingId) + 1) * sizeof(WCHAR));
        if (!driverInfo->MatchingId)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        RtlCopyMemory(driverInfo->MatchingId, MatchingId, (strlenW(MatchingId) + 1) * sizeof(WCHAR));
    }
    else
        driverInfo->MatchingId = NULL;

    /* Get inf install section */
    Result = FALSE;
    RequiredSize = 128; /* Initial buffer size */
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    while (!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        HeapFree(GetProcessHeap(), 0, InfInstallSection);
        InfInstallSection = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
        if (!InfInstallSection)
            goto cleanup;
        Result = SetupGetStringFieldW(
            &ContextDevice,
            1, /* Field index */
            InfInstallSection, RequiredSize,
            &RequiredSize);
    }
    if (!Result)
        goto cleanup;

    TRACE("Adding driver '%s' [%s/%s] (Rank 0x%lx)\n",
        debugstr_w(driverInfo->Details.DrvDescription), debugstr_w(InfFile),
        debugstr_w(InfInstallSection), Rank);

    driverInfo->DriverRank = Rank;
    memcpy(&driverInfo->DriverDate, &DriverDate, sizeof(FILETIME));
    memcpy(&driverInfo->ClassGuid, ClassGuid, sizeof(GUID));
    driverInfo->Info.DriverType = DriverType;
    driverInfo->Info.Reserved = (ULONG_PTR)driverInfo;
    strncpyW(driverInfo->Info.Description, driverInfo->Details.DrvDescription, LINE_LEN - 1);
    driverInfo->Info.Description[LINE_LEN - 1] = '\0';
    strncpyW(driverInfo->Info.MfgName, ManufacturerName, LINE_LEN - 1);
    driverInfo->Info.MfgName[LINE_LEN - 1] = '\0';
    if (ProviderName)
    {
        strncpyW(driverInfo->Info.ProviderName, ProviderName, LINE_LEN - 1);
        driverInfo->Info.ProviderName[LINE_LEN - 1] = '\0';
    }
    else
        driverInfo->Info.ProviderName[0] = '\0';
    driverInfo->Info.DriverDate = DriverDate;
    driverInfo->Info.DriverVersion = DriverVersion;
    ReferenceInfFile(InfFileDetails);
    driverInfo->InfFileDetails = InfFileDetails;

    /* Insert current driver in driver list, according to its rank */
    PreviousEntry = DriverListHead->Flink;
    while (PreviousEntry != DriverListHead)
    {
        struct DriverInfoElement *CurrentDriver;
        CurrentDriver = CONTAINING_RECORD(PreviousEntry, struct DriverInfoElement, ListEntry);
        if (CurrentDriver->DriverRank > Rank ||
            (CurrentDriver->DriverRank == Rank && CurrentDriver->DriverDate.QuadPart < driverInfo->DriverDate.QuadPart))
        {
            /* Insert before the current item */
            InsertHeadList(PreviousEntry->Blink, &driverInfo->ListEntry);
            break;
        }
        PreviousEntry = PreviousEntry->Flink;
    }
    if (PreviousEntry == DriverListHead)
    {
        /* Insert at the end of the list */
        InsertTailList(DriverListHead, &driverInfo->ListEntry);
    }

    ret = TRUE;

cleanup:
    if (!ret)
    {
        if (driverInfo)
            HeapFree(GetProcessHeap(), 0, driverInfo->MatchingId);
        HeapFree(GetProcessHeap(), 0, driverInfo);
    }
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, InfInstallSection);

    return ret;
}

static BOOL
GetVersionInformationFromInfFile(
    IN HINF hInf,
    OUT LPGUID ClassGuid,
    OUT LPWSTR* pProviderName,
    OUT FILETIME* DriverDate,
    OUT DWORDLONG* DriverVersion)
{
    DWORD RequiredSize;
    WCHAR guidW[MAX_GUID_STRING_LEN + 1];
    LPWSTR DriverVer = NULL;
    LPWSTR ProviderName = NULL;
    LPWSTR pComma; /* Points into DriverVer */
    LPWSTR pVersion = NULL; /* Points into DriverVer */
    SYSTEMTIME SystemTime;
    BOOL Result;
    BOOL ret = FALSE; /* Final result */

    /* Get class Guid */
    if (!SetupGetLineTextW(
        NULL, /* Context */
        hInf,
        Version, ClassGUID,
        guidW, sizeof(guidW),
        NULL /* Required size */))
    {
        goto cleanup;
    }
    guidW[37] = '\0'; /* Replace the } by a NULL character */
    if (UuidFromStringW(&guidW[1], ClassGuid) != RPC_S_OK)
    {
        SetLastError(ERROR_GEN_FAILURE);
        goto cleanup;
    }

    /* Get provider name */
    Result = SetupGetLineTextW(
        NULL, /* Context */
        hInf, Version, INF_PROVIDER,
        NULL, 0,
        &RequiredSize);
    if (Result)
    {
        /* We know know the needed buffer size */
        ProviderName = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
        if (!ProviderName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        Result = SetupGetLineTextW(
            NULL, /* Context */
            hInf, Version, INF_PROVIDER,
            ProviderName, RequiredSize,
            &RequiredSize);
    }
    if (!Result)
        goto cleanup;
    *pProviderName = ProviderName;

    /* Read the "DriverVer" value */
    Result = SetupGetLineTextW(
        NULL, /* Context */
        hInf, Version, INF_DRIVER_VER,
        NULL, 0,
        &RequiredSize);
    if (Result)
    {
        /* We know know the needed buffer size */
        DriverVer = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
        if (!DriverVer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        Result = SetupGetLineTextW(
            NULL, /* Context */
            hInf, Version, INF_DRIVER_VER,
            DriverVer, RequiredSize,
            &RequiredSize);
    }
    if (!Result)
        goto cleanup;

    /* Get driver date and driver version, by analyzing the "DriverVer" value */
    pComma = strchrW(DriverVer, ',');
    if (pComma != NULL)
    {
        *pComma = UNICODE_NULL;
        pVersion = pComma + 1;
    }
    /* Get driver date version. Invalid date = 00/00/00 */
    memset(DriverDate, 0, sizeof(FILETIME));
    if (strlenW(DriverVer) == 10
        && (DriverVer[2] == '-' || DriverVer[2] == '/')
        && (DriverVer[5] == '-' || DriverVer[5] == '/'))
    {
        memset(&SystemTime, 0, sizeof(SYSTEMTIME));
        DriverVer[2] = DriverVer[5] = UNICODE_NULL;
        SystemTime.wMonth = ((DriverVer[0] - '0') * 10) + DriverVer[1] - '0';
        SystemTime.wDay  = ((DriverVer[3] - '0') * 10) + DriverVer[4] - '0';
        SystemTime.wYear = ((DriverVer[6] - '0') * 1000) + ((DriverVer[7] - '0') * 100) + ((DriverVer[8] - '0') * 10) + DriverVer[9] - '0';
        SystemTimeToFileTime(&SystemTime, DriverDate);
    }
    /* Get driver version. Invalid version = 0.0.0.0 */
    *DriverVersion = 0;
    if (pVersion)
    {
        WORD Major, Minor = 0, Revision = 0, Build = 0;
        LPWSTR pMinor = NULL, pRevision = NULL, pBuild = NULL;
        LARGE_INTEGER fullVersion;

        pMinor = strchrW(pVersion, '.');
        if (pMinor)
        {
            *pMinor = 0;
            pRevision = strchrW(++pMinor, '.');
            Minor = atoiW(pMinor);
        }
        if (pRevision)
        {
            *pRevision = 0;
            pBuild = strchrW(++pRevision, '.');
            Revision = atoiW(pRevision);
        }
        if (pBuild)
        {
            *pBuild = 0;
            pBuild++;
            Build = atoiW(pBuild);
        }
        Major = atoiW(pVersion);
        fullVersion.u.HighPart = Major << 16 | Minor;
        fullVersion.u.LowPart = Revision << 16 | Build;
        memcpy(DriverVersion, &fullVersion, sizeof(LARGE_INTEGER));
    }

    ret = TRUE;

cleanup:
    if (!ret)
        HeapFree(GetProcessHeap(), 0, ProviderName);
    HeapFree(GetProcessHeap(), 0, DriverVer);

    return ret;
}

static BOOL
GetHardwareAndCompatibleIDsLists(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    OUT LPWSTR *pHardwareIDs OPTIONAL,
    OUT LPDWORD pHardwareIDsRequiredSize OPTIONAL,
    OUT LPWSTR *pCompatibleIDs OPTIONAL,
    OUT LPDWORD pCompatibleIDsRequiredSize OPTIONAL)
{
    LPWSTR HardwareIDs = NULL;
    LPWSTR CompatibleIDs = NULL;
    DWORD RequiredSize;
    BOOL Result;

    /* Get hardware IDs list */
    Result = FALSE;
    RequiredSize = 512; /* Initial buffer size */
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    while (!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        MyFree(HardwareIDs);
        HardwareIDs = MyMalloc(RequiredSize);
        if (!HardwareIDs)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        Result = SetupDiGetDeviceRegistryPropertyW(
            DeviceInfoSet,
            DeviceInfoData,
            SPDRP_HARDWAREID,
            NULL,
            (PBYTE)HardwareIDs,
            RequiredSize,
            &RequiredSize);
    }
    if (!Result)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            /* No hardware ID for this device */
            MyFree(HardwareIDs);
            HardwareIDs = NULL;
            RequiredSize = 0;
        }
        else
            goto done;
    }
    if (pHardwareIDs)
        *pHardwareIDs = HardwareIDs;
    if (pHardwareIDsRequiredSize)
        *pHardwareIDsRequiredSize = RequiredSize;

    /* Get compatible IDs list */
    Result = FALSE;
    RequiredSize = 512; /* Initial buffer size */
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    while (!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        MyFree(CompatibleIDs);
        CompatibleIDs = MyMalloc(RequiredSize);
        if (!CompatibleIDs)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        Result = SetupDiGetDeviceRegistryPropertyW(
            DeviceInfoSet,
            DeviceInfoData,
            SPDRP_COMPATIBLEIDS,
            NULL,
            (PBYTE)CompatibleIDs,
            RequiredSize,
            &RequiredSize);
    }
    if (!Result)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            /* No compatible ID for this device */
            MyFree(CompatibleIDs);
            CompatibleIDs = NULL;
            RequiredSize = 0;
        }
        else
            goto done;
    }
    if (pCompatibleIDs)
        *pCompatibleIDs = CompatibleIDs;
    if (pCompatibleIDsRequiredSize)
        *pCompatibleIDsRequiredSize = RequiredSize;

    Result = TRUE;

done:
    if (!Result)
    {
        MyFree(HardwareIDs);
        MyFree(CompatibleIDs);
    }
    return Result;
}

/***********************************************************************
 *		SetupDiBuildDriverInfoList (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiBuildDriverInfoList(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN DWORD DriverType)
{
    struct DeviceInfoSet *list;
    SP_DEVINSTALL_PARAMS_W InstallParams;
    PVOID Buffer = NULL;
    struct InfFileDetails *currentInfFileDetails = NULL;
    LPWSTR ProviderName = NULL;
    LPWSTR ManufacturerName = NULL;
    WCHAR ManufacturerSection[LINE_LEN + 1];
    LPWSTR HardwareIDs = NULL;
    LPWSTR CompatibleIDs = NULL;
    LPWSTR FullInfFileName = NULL;
    LPWSTR ExcludeFromSelect = NULL;
    FILETIME DriverDate;
    DWORDLONG DriverVersion = 0;
    DWORD RequiredSize;
    BOOL ret = FALSE;

    TRACE("%p %p %ld\n", DeviceInfoSet, DeviceInfoData, DriverType);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (list->HKLM != HKEY_LOCAL_MACHINE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DriverType != SPDIT_CLASSDRIVER && DriverType != SPDIT_COMPATDRIVER)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverType == SPDIT_COMPATDRIVER && !DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        PLIST_ENTRY pDriverListHead = &list->DriverListHead;
        BOOL Result;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        Result = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        if (!Result)
            goto done;

        if (DeviceInfoData)
        {
            struct DeviceInfoElement *devInfo = (struct DeviceInfoElement *)DeviceInfoData->Reserved;
            if (!(devInfo->CreationFlags & DICD_INHERIT_CLASSDRVS))
                pDriverListHead = &devInfo->DriverListHead;
        }

        if (DriverType == SPDIT_COMPATDRIVER)
        {
            /* Get hardware and compatible IDs lists */
            Result = GetHardwareAndCompatibleIDsLists(
                DeviceInfoSet,
                DeviceInfoData,
                &HardwareIDs,
                NULL,
                &CompatibleIDs,
                NULL);
            if (!Result)
                goto done;
            if (!HardwareIDs && !CompatibleIDs)
            {
                SetLastError(ERROR_FILE_NOT_FOUND);
                goto done;
            }
        }

        if (InstallParams.Flags & DI_ENUMSINGLEINF)
        {
            /* InstallParams.DriverPath contains the name of a .inf file */
            RequiredSize = strlenW(InstallParams.DriverPath) + 2;
            Buffer = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
            if (!Buffer)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto done;
            }
            strcpyW(Buffer, InstallParams.DriverPath);
            ((LPWSTR)Buffer)[RequiredSize - 1] = 0;
            Result = TRUE;
        }
        else
        {
            /* Enumerate .inf files */
            Result = FALSE;
            RequiredSize = 32768; /* Initial buffer size */
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            while (!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                HeapFree(GetProcessHeap(), 0, Buffer);
                Buffer = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
                if (!Buffer)
                {
                    Result = FALSE;
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    break;
                }
                Result = SetupGetInfFileListW(
                    *InstallParams.DriverPath ? InstallParams.DriverPath : NULL,
                    INF_STYLE_WIN4,
                    Buffer, RequiredSize,
                    &RequiredSize);
            }
            if (!Result && GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                /* No .inf file in specified directory. So, we should
                 * success as we created an empty driver info list.
                 */
                ret = TRUE;
                goto done;
            }
        }
        if (Result)
        {
            LPCWSTR filename;
            LPWSTR pFullFilename;

            if (InstallParams.Flags & DI_ENUMSINGLEINF)
            {
                /* Only a filename */
                FullInfFileName = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
                if (!FullInfFileName)
                    goto done;
                pFullFilename = &FullInfFileName[0];
            }
            else if (*InstallParams.DriverPath)
            {
                /* Directory name specified */
                DWORD len;
                len = GetFullPathNameW(InstallParams.DriverPath, 0, NULL, NULL);
                if (len == 0)
                    goto done;
                FullInfFileName = HeapAlloc(GetProcessHeap(), 0, (len + 1 + MAX_PATH) * sizeof(WCHAR));
                if (!FullInfFileName)
                    goto done;
                len = GetFullPathNameW(InstallParams.DriverPath, len, FullInfFileName, NULL);
                if (len == 0)
                    goto done;
                if (*FullInfFileName && FullInfFileName[strlenW(FullInfFileName) - 1] != '\\')
                    strcatW(FullInfFileName, BackSlash);
                pFullFilename = &FullInfFileName[strlenW(FullInfFileName)];
            }
            else
            {
                /* Nothing specified ; need to get the %SYSTEMROOT%\ directory */
                DWORD len;
                len = GetSystemWindowsDirectoryW(NULL, 0);
                if (len == 0)
                    goto done;
                FullInfFileName = HeapAlloc(GetProcessHeap(), 0, (len + 1 + strlenW(InfDirectory) + MAX_PATH) * sizeof(WCHAR));
                if (!FullInfFileName)
                    goto done;
                len = GetSystemWindowsDirectoryW(FullInfFileName, len);
                if (len == 0)
                    goto done;
                if (*FullInfFileName && FullInfFileName[strlenW(FullInfFileName) - 1] != '\\')
                    strcatW(FullInfFileName, BackSlash);
                strcatW(FullInfFileName, InfDirectory);
                pFullFilename = &FullInfFileName[strlenW(FullInfFileName)];
            }

            for (filename = (LPCWSTR)Buffer; *filename; filename += strlenW(filename) + 1)
            {
                INFCONTEXT ContextManufacturer, ContextDevice;
                GUID ClassGuid;

                strcpyW(pFullFilename, filename);
                TRACE("Opening file %s\n", debugstr_w(FullInfFileName));

                currentInfFileDetails = CreateInfFileDetails(FullInfFileName);
                if (!currentInfFileDetails)
                    continue;

                if (!GetVersionInformationFromInfFile(
                    currentInfFileDetails->hInf,
                    &ClassGuid,
                    &ProviderName,
                    &DriverDate,
                    &DriverVersion))
                {
                    DereferenceInfFile(currentInfFileDetails);
                    currentInfFileDetails = NULL;
                    continue;
                }

                if (DriverType == SPDIT_CLASSDRIVER)
                {
                    /* Check if the ClassGuid in this .inf file is corresponding with our needs */
                    if (!IsEqualIID(&list->ClassGuid, &GUID_NULL) && !IsEqualIID(&list->ClassGuid, &ClassGuid))
                    {
                        goto next;
                    }
                }

                if (InstallParams.FlagsEx & DI_FLAGSEX_ALLOWEXCLUDEDDRVS)
                {
                    /* Read ExcludeFromSelect control flags */
                    /* FIXME */
                }
                else
                    FIXME("ExcludeFromSelect list ignored\n");

                /* Get the manufacturers list */
                Result = SetupFindFirstLineW(currentInfFileDetails->hInf, INF_MANUFACTURER, NULL, &ContextManufacturer);
                while (Result)
                {
                    Result = SetupGetStringFieldW(
                        &ContextManufacturer,
                        0, /* Field index */
                        NULL, 0,
                        &RequiredSize);
                    if (Result)
                    {
                        /* We got the needed size for the buffer */
                        ManufacturerName = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
                        if (!ManufacturerName)
                        {
                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                            goto done;
                        }
                        Result = SetupGetStringFieldW(
                            &ContextManufacturer,
                            0, /* Field index */
                            ManufacturerName, RequiredSize,
                            &RequiredSize);
                    }
                    /* Get manufacturer section name */
                    Result = SetupGetStringFieldW(
                        &ContextManufacturer,
                        1, /* Field index */
                        ManufacturerSection, LINE_LEN,
                        &RequiredSize);
                    if (Result)
                    {
                        ManufacturerSection[RequiredSize] = 0; /* Final NULL char */
                        /* Add (possible) extension to manufacturer section name */
                        Result = SetupDiGetActualSectionToInstallW(
                            currentInfFileDetails->hInf, ManufacturerSection, ManufacturerSection, LINE_LEN, NULL, NULL);
                        if (Result)
                        {
                            TRACE("Enumerating devices in manufacturer %s\n", debugstr_w(ManufacturerSection));
                            Result = SetupFindFirstLineW(currentInfFileDetails->hInf, ManufacturerSection, NULL, &ContextDevice);
                        }
                    }
                    while (Result)
                    {
                        if (DriverType == SPDIT_CLASSDRIVER)
                        {
                            /* FIXME: Check ExcludeFromSelect list */
                            if (!AddDriverToList(
                                pDriverListHead,
                                DriverType,
                                &ClassGuid,
                                ContextDevice,
                                currentInfFileDetails,
                                FullInfFileName,
                                ProviderName,
                                ManufacturerName,
                                NULL,
                                DriverDate, DriverVersion,
                                0))
                            {
                                break;
                            }
                        }
                        else /* DriverType = SPDIT_COMPATDRIVER */
                        {
                            /* 1. Get all fields */
                            DWORD FieldCount = SetupGetFieldCount(&ContextDevice);
                            DWORD DriverRank;
                            DWORD i;
                            LPCWSTR currentId;
                            BOOL DriverAlreadyAdded;

                            for (i = 2; i <= FieldCount; i++)
                            {
                                LPWSTR DeviceId = NULL;
                                Result = FALSE;
                                RequiredSize = 128; /* Initial buffer size */
                                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                                while (!Result && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                                {
                                    HeapFree(GetProcessHeap(), 0, DeviceId);
                                    DeviceId = HeapAlloc(GetProcessHeap(), 0, RequiredSize * sizeof(WCHAR));
                                    if (!DeviceId)
                                    {
                                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                                        goto done;
                                    }
                                    Result = SetupGetStringFieldW(
                                        &ContextDevice,
                                        i,
                                        DeviceId, RequiredSize,
                                        &RequiredSize);
                                }
                                if (!Result)
                                {
                                    HeapFree(GetProcessHeap(), 0, DeviceId);
                                    goto done;
                                }
                                /* FIXME: Check ExcludeFromSelect list */
                                DriverAlreadyAdded = FALSE;
                                for (DriverRank = 0, currentId = (LPCWSTR)HardwareIDs; !DriverAlreadyAdded && *currentId; currentId += strlenW(currentId) + 1, DriverRank++)
                                {
                                    if (strcmpiW(DeviceId, currentId) == 0)
                                    {
                                        AddDriverToList(
                                            pDriverListHead,
                                            DriverType,
                                            &ClassGuid,
                                            ContextDevice,
                                            currentInfFileDetails,
                                            FullInfFileName,
                                            ProviderName,
                                            ManufacturerName,
                                            currentId,
                                            DriverDate, DriverVersion,
                                            DriverRank  + (i == 2 ? 0 : 0x1000 + i - 3));
                                        DriverAlreadyAdded = TRUE;
                                    }
                                }
                                if (CompatibleIDs)
                                {
                                    for (DriverRank = 0, currentId = (LPCWSTR)CompatibleIDs; !DriverAlreadyAdded && *currentId; currentId += strlenW(currentId) + 1, DriverRank++)
                                    {
                                        if (strcmpiW(DeviceId, currentId) == 0)
                                        {
                                            AddDriverToList(
                                                pDriverListHead,
                                                DriverType,
                                                &ClassGuid,
                                                ContextDevice,
                                                currentInfFileDetails,
                                                FullInfFileName,
                                                ProviderName,
                                                ManufacturerName,
                                                currentId,
                                                DriverDate, DriverVersion,
                                                DriverRank + (i == 2 ? 0x2000 : 0x3000 + i - 3));
                                            DriverAlreadyAdded = TRUE;
                                        }
                                    }
                                }
                                HeapFree(GetProcessHeap(), 0, DeviceId);
                            }
                        }
                        Result = SetupFindNextLine(&ContextDevice, &ContextDevice);
                    }

                    HeapFree(GetProcessHeap(), 0, ManufacturerName);
                    ManufacturerName = NULL;
                    Result = SetupFindNextLine(&ContextManufacturer, &ContextManufacturer);
                }

                ret = TRUE;
next:
                HeapFree(GetProcessHeap(), 0, ProviderName);
                HeapFree(GetProcessHeap(), 0, ExcludeFromSelect);
                ProviderName = ExcludeFromSelect = NULL;

                DereferenceInfFile(currentInfFileDetails);
                currentInfFileDetails = NULL;
            }
            ret = TRUE;
        }
    }

done:
    if (ret)
    {
        if (DeviceInfoData)
        {
            InstallParams.Flags |= DI_DIDCOMPAT;
            InstallParams.FlagsEx |= DI_FLAGSEX_DIDCOMPATINFO;
        }
        else
        {
            InstallParams.Flags |= DI_DIDCLASS;
            InstallParams.FlagsEx |= DI_FLAGSEX_DIDINFOLIST;
        }
        ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
    }

    HeapFree(GetProcessHeap(), 0, ProviderName);
    HeapFree(GetProcessHeap(), 0, ManufacturerName);
    MyFree(HardwareIDs);
    MyFree(CompatibleIDs);
    HeapFree(GetProcessHeap(), 0, FullInfFileName);
    HeapFree(GetProcessHeap(), 0, ExcludeFromSelect);
    if (currentInfFileDetails)
        DereferenceInfFile(currentInfFileDetails);
    HeapFree(GetProcessHeap(), 0, Buffer);

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiDestroyDriverInfoList (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiDestroyDriverInfoList(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN DWORD DriverType)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %p 0x%lx\n", DeviceInfoSet, DeviceInfoData, DriverType);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DriverType != SPDIT_CLASSDRIVER && DriverType != SPDIT_COMPATDRIVER)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverType == SPDIT_COMPATDRIVER && !DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        PLIST_ENTRY ListEntry;
        struct DriverInfoElement *driverInfo;
        SP_DEVINSTALL_PARAMS_W InstallParams;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        if (!SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams))
            goto done;

        if (!DeviceInfoData)
            /* Fall back to destroying class driver list */
            DriverType = SPDIT_CLASSDRIVER;

        if (DriverType == SPDIT_CLASSDRIVER)
        {
            while (!IsListEmpty(&list->DriverListHead))
            {
                 ListEntry = RemoveHeadList(&list->DriverListHead);
                 driverInfo = CONTAINING_RECORD(ListEntry, struct DriverInfoElement, ListEntry);
                 DestroyDriverInfoElement(driverInfo);
            }
            InstallParams.Reserved = 0;
            InstallParams.Flags &= ~(DI_DIDCLASS | DI_MULTMFGS);
            InstallParams.FlagsEx &= ~DI_FLAGSEX_DIDINFOLIST;
            ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, NULL, &InstallParams);
        }
        else
        {
            SP_DEVINSTALL_PARAMS_W InstallParamsSet;
            struct DeviceInfoElement *deviceInfo;

            InstallParamsSet.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
            if (!SetupDiGetDeviceInstallParamsW(DeviceInfoSet, NULL, &InstallParamsSet))
                goto done;
            deviceInfo = (struct DeviceInfoElement *)DeviceInfoData->Reserved;
            while (!IsListEmpty(&deviceInfo->DriverListHead))
            {
                 ListEntry = RemoveHeadList(&deviceInfo->DriverListHead);
                 driverInfo = CONTAINING_RECORD(ListEntry, struct DriverInfoElement, ListEntry);
                 if ((PVOID)InstallParamsSet.Reserved == driverInfo)
                 {
                     InstallParamsSet.Reserved = 0;
                     SetupDiSetDeviceInstallParamsW(DeviceInfoSet, NULL, &InstallParamsSet);
                 }
                 DestroyDriverInfoElement(driverInfo);
            }
            InstallParams.Reserved = 0;
            InstallParams.Flags &= ~DI_DIDCOMPAT;
            InstallParams.FlagsEx &= ~DI_FLAGSEX_DIDCOMPATINFO;
            ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        }
    }

done:
    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiEnumDriverInfoA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiEnumDriverInfoA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN DWORD DriverType,
    IN DWORD MemberIndex,
    OUT PSP_DRVINFO_DATA_A DriverInfoData)
{
    SP_DRVINFO_DATA_V2_W driverInfoData2W;
    BOOL ret = FALSE;

    TRACE("%p %p 0x%lx %ld %p\n", DeviceInfoSet, DeviceInfoData,
        DriverType, MemberIndex, DriverInfoData);

    if (DriverInfoData == NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_A) && DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V2_A))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        driverInfoData2W.cbSize = sizeof(SP_DRVINFO_DATA_V2_W);
        ret = SetupDiEnumDriverInfoW(DeviceInfoSet, DeviceInfoData,
            DriverType, MemberIndex, &driverInfoData2W);

        if (ret)
        {
            /* Do W->A conversion */
            DriverInfoData->DriverType = driverInfoData2W.DriverType;
            DriverInfoData->Reserved = driverInfoData2W.Reserved;
            if (WideCharToMultiByte(CP_ACP, 0, driverInfoData2W.Description, -1,
                DriverInfoData->Description, LINE_LEN, NULL, NULL) == 0)
            {
                DriverInfoData->Description[0] = '\0';
                ret = FALSE;
            }
            if (WideCharToMultiByte(CP_ACP, 0, driverInfoData2W.MfgName, -1,
                DriverInfoData->MfgName, LINE_LEN, NULL, NULL) == 0)
            {
                DriverInfoData->MfgName[0] = '\0';
                ret = FALSE;
            }
            if (WideCharToMultiByte(CP_ACP, 0, driverInfoData2W.ProviderName, -1,
                DriverInfoData->ProviderName, LINE_LEN, NULL, NULL) == 0)
            {
                DriverInfoData->ProviderName[0] = '\0';
                ret = FALSE;
            }
            if (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V2_A))
            {
                /* Copy more fields */
                DriverInfoData->DriverDate = driverInfoData2W.DriverDate;
                DriverInfoData->DriverVersion = driverInfoData2W.DriverVersion;
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupDiEnumDriverInfoW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiEnumDriverInfoW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN DWORD DriverType,
    IN DWORD MemberIndex,
    OUT PSP_DRVINFO_DATA_W DriverInfoData)
{
    PLIST_ENTRY ListHead;
    BOOL ret = FALSE;

    TRACE("%p %p 0x%lx %ld %p\n", DeviceInfoSet, DeviceInfoData,
        DriverType, MemberIndex, DriverInfoData);

    if (!DeviceInfoSet || !DriverInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DriverType != SPDIT_CLASSDRIVER && DriverType != SPDIT_COMPATDRIVER)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverType == SPDIT_COMPATDRIVER && !DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_W) && DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V2_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        struct DeviceInfoElement *devInfo = NULL;
        PLIST_ENTRY ItemList;
        if (DeviceInfoData)
            devInfo = (struct DeviceInfoElement *)DeviceInfoData->Reserved;
        if (!devInfo || (devInfo->CreationFlags & DICD_INHERIT_CLASSDRVS))
        {
            ListHead = &((struct DeviceInfoSet *)DeviceInfoSet)->DriverListHead;
        }
        else
        {
            ListHead = &devInfo->DriverListHead;
        }

        ItemList = ListHead->Flink;
        while (ItemList != ListHead && MemberIndex-- > 0)
            ItemList = ItemList->Flink;
        if (ItemList == ListHead)
            SetLastError(ERROR_NO_MORE_ITEMS);
        else
        {
            struct DriverInfoElement *DrvInfo = CONTAINING_RECORD(ItemList, struct DriverInfoElement, ListEntry);

            memcpy(
                &DriverInfoData->DriverType,
                &DrvInfo->Info.DriverType,
                DriverInfoData->cbSize - FIELD_OFFSET(SP_DRVINFO_DATA_W, DriverType));
            ret = TRUE;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetSelectedDriverA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetSelectedDriverA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    OUT PSP_DRVINFO_DATA_A DriverInfoData)
{
    SP_DRVINFO_DATA_V2_W driverInfoData2W;
    BOOL ret = FALSE;

    if (DriverInfoData == NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_A) && DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V2_A))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        driverInfoData2W.cbSize = sizeof(SP_DRVINFO_DATA_V2_W);

        ret = SetupDiGetSelectedDriverW(DeviceInfoSet,
                                        DeviceInfoData,
                                        &driverInfoData2W);

        if (ret)
        {
            /* Do W->A conversion */
            DriverInfoData->DriverType = driverInfoData2W.DriverType;
            DriverInfoData->Reserved = driverInfoData2W.Reserved;
            if (WideCharToMultiByte(CP_ACP, 0, driverInfoData2W.Description, -1,
                DriverInfoData->Description, LINE_LEN, NULL, NULL) == 0)
            {
                DriverInfoData->Description[0] = '\0';
                ret = FALSE;
            }
            if (WideCharToMultiByte(CP_ACP, 0, driverInfoData2W.MfgName, -1,
                DriverInfoData->MfgName, LINE_LEN, NULL, NULL) == 0)
            {
                DriverInfoData->MfgName[0] = '\0';
                ret = FALSE;
            }
            if (WideCharToMultiByte(CP_ACP, 0, driverInfoData2W.ProviderName, -1,
                DriverInfoData->ProviderName, LINE_LEN, NULL, NULL) == 0)
            {
                DriverInfoData->ProviderName[0] = '\0';
                ret = FALSE;
            }
            if (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V2_A))
            {
                /* Copy more fields */
                DriverInfoData->DriverDate = driverInfoData2W.DriverDate;
                DriverInfoData->DriverVersion = driverInfoData2W.DriverVersion;
            }
        }
    }

    return ret;
}

/***********************************************************************
 *		SetupDiGetSelectedDriverW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetSelectedDriverW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    OUT PSP_DRVINFO_DATA_W DriverInfoData)
{
    BOOL ret = FALSE;

    TRACE("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DriverInfoData);

    if (!DeviceInfoSet || !DriverInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_W) && DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V2_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        SP_DEVINSTALL_PARAMS InstallParams;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        if (SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams))
        {
            struct DriverInfoElement *driverInfo;
            driverInfo = (struct DriverInfoElement *)InstallParams.Reserved;
            if (driverInfo == NULL)
                SetLastError(ERROR_NO_DRIVER_SELECTED);
            else
            {
                memcpy(
                    &DriverInfoData->DriverType,
                    &driverInfo->Info.DriverType,
                    DriverInfoData->cbSize - FIELD_OFFSET(SP_DRVINFO_DATA_W, DriverType));
                ret = TRUE;
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiSetSelectedDriverA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetSelectedDriverA(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PSP_DRVINFO_DATA_A DriverInfoData OPTIONAL)
{
    SP_DRVINFO_DATA_V1_W DriverInfoDataW;
    PSP_DRVINFO_DATA_W pDriverInfoDataW = NULL;
    BOOL ret = FALSE;

    if (DriverInfoData != NULL)
    {
        if (DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V2_A) &&
            DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_A));
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        DriverInfoDataW.cbSize = sizeof(SP_DRVINFO_DATA_V1_W);
        DriverInfoDataW.Reserved = DriverInfoData->Reserved;

        if (DriverInfoDataW.Reserved == 0)
        {
            DriverInfoDataW.DriverType = DriverInfoData->DriverType;

            /* convert the strings to unicode */
            if (!MultiByteToWideChar(CP_ACP,
                                     0,
                                     DriverInfoData->Description,
                                     LINE_LEN,
                                     DriverInfoDataW.Description,
                                     LINE_LEN) ||
                !MultiByteToWideChar(CP_ACP,
                                     0,
                                     DriverInfoData->ProviderName,
                                     LINE_LEN,
                                     DriverInfoDataW.ProviderName,
                                     LINE_LEN))
            {
                return FALSE;
            }
        }

        pDriverInfoDataW = (PSP_DRVINFO_DATA_W)&DriverInfoDataW;
    }

    ret = SetupDiSetSelectedDriverW(DeviceInfoSet,
                                    DeviceInfoData,
                                    pDriverInfoDataW);

    if (ret && pDriverInfoDataW != NULL)
    {
        DriverInfoData->Reserved = DriverInfoDataW.Reserved;
    }

    return ret;
}

/***********************************************************************
 *		SetupDiSetSelectedDriverW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetSelectedDriverW(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PSP_DRVINFO_DATA_W DriverInfoData OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DriverInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DriverInfoData && DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_W) && DriverInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V2_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        struct DriverInfoElement **pDriverInfo;
        PLIST_ENTRY ListHead, ItemList;

        if (DeviceInfoData)
        {
            pDriverInfo = (struct DriverInfoElement **)&((struct DeviceInfoElement *)DeviceInfoData->Reserved)->InstallParams.Reserved;
            ListHead = &((struct DeviceInfoElement *)DeviceInfoData->Reserved)->DriverListHead;
        }
        else
        {
            pDriverInfo = (struct DriverInfoElement **)&((struct DeviceInfoSet *)DeviceInfoSet)->InstallParams.Reserved;
            ListHead = &((struct DeviceInfoSet *)DeviceInfoSet)->DriverListHead;
        }

        if (!DriverInfoData)
        {
            *pDriverInfo = NULL;
            ret = TRUE;
        }
        else
        {
            /* Search selected driver in list */
            ItemList = ListHead->Flink;
            while (ItemList != ListHead)
            {
                if (DriverInfoData->Reserved != 0)
                {
                    if (DriverInfoData->Reserved == (ULONG_PTR)ItemList)
                        break;
                }
                else
                {
                    /* The caller wants to compare only DriverType, Description and ProviderName fields */
                    struct DriverInfoElement *driverInfo = CONTAINING_RECORD(ItemList, struct DriverInfoElement, ListEntry);
                    if (driverInfo->Info.DriverType == DriverInfoData->DriverType
                        && strcmpW(driverInfo->Info.Description, DriverInfoData->Description) == 0
                        && strcmpW(driverInfo->Info.ProviderName, DriverInfoData->ProviderName) == 0)
                    {
                        break;
                    }
                }
                ItemList = ItemList->Flink;
            }
            if (ItemList == ListHead)
                SetLastError(ERROR_INVALID_PARAMETER);
            else
            {
                *pDriverInfo = CONTAINING_RECORD(ItemList, struct DriverInfoElement, ListEntry);
                DriverInfoData->Reserved = (ULONG_PTR)ItemList;
                ret = TRUE;
                TRACE("Choosing driver whose rank is 0x%lx\n",
                    (*pDriverInfo)->DriverRank);
                if (DeviceInfoData)
                    memcpy(&DeviceInfoData->ClassGuid, &(*pDriverInfo)->ClassGuid, sizeof(GUID));
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDriverInfoDetailA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDriverInfoDetailA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_DRVINFO_DATA_A DriverInfoData,
    IN OUT PSP_DRVINFO_DETAIL_DATA_A DriverInfoDetailData OPTIONAL,
    IN DWORD DriverInfoDetailDataSize,
    OUT PDWORD RequiredSize OPTIONAL)
{
    SP_DRVINFO_DATA_V2_W DriverInfoDataW;
    PSP_DRVINFO_DETAIL_DATA_W DriverInfoDetailDataW = NULL;
    DWORD BufSize = 0;
    DWORD HardwareIDLen = 0;
    BOOL ret = FALSE;

    /* do some sanity checks, the unicode version might do more thorough checks */
    if (DriverInfoData == NULL ||
        (DriverInfoDetailData == NULL && DriverInfoDetailDataSize != 0) ||
        (DriverInfoDetailData != NULL &&
         (DriverInfoDetailDataSize < FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_A, HardwareID) + sizeof(CHAR) ||
          DriverInfoDetailData->cbSize != sizeof(SP_DRVINFO_DETAIL_DATA_A))))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    /* make sure we support both versions of the SP_DRVINFO_DATA structure */
    if (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V1_A))
    {
        DriverInfoDataW.cbSize = sizeof(SP_DRVINFO_DATA_V1_W);
    }
    else if (DriverInfoData->cbSize == sizeof(SP_DRVINFO_DATA_V2_A))
    {
        DriverInfoDataW.cbSize = sizeof(SP_DRVINFO_DATA_V2_W);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }
    DriverInfoDataW.DriverType = DriverInfoData->DriverType;
    DriverInfoDataW.Reserved = DriverInfoData->Reserved;

    /* convert the strings to unicode */
    if (MultiByteToWideChar(CP_ACP,
                            0,
                            DriverInfoData->Description,
                            LINE_LEN,
                            DriverInfoDataW.Description,
                            LINE_LEN) &&
        MultiByteToWideChar(CP_ACP,
                            0,
                            DriverInfoData->MfgName,
                            LINE_LEN,
                            DriverInfoDataW.MfgName,
                            LINE_LEN) &&
        MultiByteToWideChar(CP_ACP,
                            0,
                            DriverInfoData->ProviderName,
                            LINE_LEN,
                            DriverInfoDataW.ProviderName,
                            LINE_LEN))
    {
        if (DriverInfoDataW.cbSize == sizeof(SP_DRVINFO_DATA_V2_W))
        {
            DriverInfoDataW.DriverDate = ((PSP_DRVINFO_DATA_V2_A)DriverInfoData)->DriverDate;
            DriverInfoDataW.DriverVersion = ((PSP_DRVINFO_DATA_V2_A)DriverInfoData)->DriverVersion;
        }

        if (DriverInfoDetailData != NULL)
        {
            /* calculate the unicode buffer size from the ansi buffer size */
            HardwareIDLen = DriverInfoDetailDataSize - FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_A, HardwareID);
            BufSize = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_W, HardwareID) +
                      (HardwareIDLen * sizeof(WCHAR));

            DriverInfoDetailDataW = MyMalloc(BufSize);
            if (DriverInfoDetailDataW == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Cleanup;
            }

            /* initialize the buffer */
            ZeroMemory(DriverInfoDetailDataW,
                       BufSize);
            DriverInfoDetailDataW->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA_W);
        }

        /* call the unicode version */
        ret = SetupDiGetDriverInfoDetailW(DeviceInfoSet,
                                          DeviceInfoData,
                                          &DriverInfoDataW,
                                          DriverInfoDetailDataW,
                                          BufSize,
                                          RequiredSize);

        if (ret)
        {
            if (DriverInfoDetailDataW != NULL)
            {
                /* convert the SP_DRVINFO_DETAIL_DATA_W structure to ansi */
                DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA_A);
                DriverInfoDetailData->InfDate = DriverInfoDetailDataW->InfDate;
                DriverInfoDetailData->Reserved = DriverInfoDetailDataW->Reserved;
                if (WideCharToMultiByte(CP_ACP,
                                        0,
                                        DriverInfoDetailDataW->SectionName,
                                        LINE_LEN,
                                        DriverInfoDetailData->SectionName,
                                        LINE_LEN,
                                        NULL,
                                        NULL) &&
                    WideCharToMultiByte(CP_ACP,
                                        0,
                                        DriverInfoDetailDataW->InfFileName,
                                        MAX_PATH,
                                        DriverInfoDetailData->InfFileName,
                                        MAX_PATH,
                                        NULL,
                                        NULL) &&
                    WideCharToMultiByte(CP_ACP,
                                        0,
                                        DriverInfoDetailDataW->DrvDescription,
                                        LINE_LEN,
                                        DriverInfoDetailData->DrvDescription,
                                        LINE_LEN,
                                        NULL,
                                        NULL) &&
                    WideCharToMultiByte(CP_ACP,
                                        0,
                                        DriverInfoDetailDataW->HardwareID,
                                        HardwareIDLen,
                                        DriverInfoDetailData->HardwareID,
                                        HardwareIDLen,
                                        NULL,
                                        NULL))
                {
                    DWORD len, cnt = 0;
                    DWORD hwidlen = HardwareIDLen;
                    CHAR *s = DriverInfoDetailData->HardwareID;

                    /* count the strings in the list */
                    while (*s != '\0')
                    {
                        len = lstrlenA(s) + 1;
                        if (hwidlen > len)
                        {
                            cnt++;
                            s += len;
                            hwidlen -= len;
                        }
                        else
                        {
                            /* looks like the string list wasn't terminated... */
                            SetLastError(ERROR_INVALID_USER_BUFFER);
                            ret = FALSE;
                            break;
                        }
                    }

                    /* make sure CompatIDsOffset points to the second string in the
                       list, if present */
                    if (cnt > 1)
                    {
                        DriverInfoDetailData->CompatIDsOffset = lstrlenA(DriverInfoDetailData->HardwareID) + 1;
                        DriverInfoDetailData->CompatIDsLength = (DWORD)(s - DriverInfoDetailData->HardwareID) -
                                                                DriverInfoDetailData->CompatIDsOffset + 1;
                    }
                    else
                    {
                        DriverInfoDetailData->CompatIDsOffset = 0;
                        DriverInfoDetailData->CompatIDsLength = 0;
                    }
                }
                else
                {
                    ret = FALSE;
                }
            }

            if (RequiredSize != NULL)
            {
                *RequiredSize = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_A, HardwareID) +
                                (((*RequiredSize) - FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_W, HardwareID)) / sizeof(WCHAR));
            }
        }
    }

Cleanup:
    if (DriverInfoDetailDataW != NULL)
    {
        MyFree(DriverInfoDetailDataW);
    }

    return ret;
}

/***********************************************************************
 *		SetupDiGetDriverInfoDetailW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDriverInfoDetailW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN PSP_DRVINFO_DATA_W DriverInfoData,
    IN OUT PSP_DRVINFO_DETAIL_DATA_W DriverInfoDetailData OPTIONAL,
    IN DWORD DriverInfoDetailDataSize,
    OUT PDWORD RequiredSize OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%p %p %p %p %lu %p\n", DeviceInfoSet, DeviceInfoData,
        DriverInfoData, DriverInfoDetailData,
        DriverInfoDetailDataSize, RequiredSize);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!DriverInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!DriverInfoDetailData && DriverInfoDetailDataSize != 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverInfoDetailData && DriverInfoDetailDataSize < sizeof(SP_DRVINFO_DETAIL_DATA_W))
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DriverInfoDetailData && DriverInfoDetailData->cbSize != sizeof(SP_DRVINFO_DETAIL_DATA_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DriverInfoData->Reserved == 0)
        SetLastError(ERROR_NO_DRIVER_SELECTED);
    else
    {
        struct DriverInfoElement *driverInfoElement;
        LPWSTR HardwareIDs = NULL;
        LPWSTR CompatibleIDs = NULL;
        LPWSTR pBuffer = NULL;
        LPCWSTR DeviceID = NULL;
        ULONG HardwareIDsSize, CompatibleIDsSize;
        ULONG sizeNeeded, sizeLeft, size;
        BOOL Result;

        driverInfoElement = (struct DriverInfoElement *)DriverInfoData->Reserved;

        /* Get hardware and compatible IDs lists */
        Result = GetHardwareAndCompatibleIDsLists(
            DeviceInfoSet,
            DeviceInfoData,
            &HardwareIDs, &HardwareIDsSize,
            &CompatibleIDs, &CompatibleIDsSize);
        if (!Result)
            goto done;

        sizeNeeded = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_W, HardwareID)
            + HardwareIDsSize + CompatibleIDsSize;
        if (RequiredSize)
            *RequiredSize = sizeNeeded;

        if (!DriverInfoDetailData)
        {
            ret = TRUE;
            goto done;
        }

        memcpy(
            DriverInfoDetailData,
            &driverInfoElement->Details,
            driverInfoElement->Details.cbSize);
        DriverInfoDetailData->CompatIDsOffset = 0;
        DriverInfoDetailData->CompatIDsLength = 0;

        sizeLeft = (DriverInfoDetailDataSize - FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_W, HardwareID)) / sizeof(WCHAR);
        pBuffer = DriverInfoDetailData->HardwareID;
        /* Add as many as possible HardwareIDs in the list */
        DeviceID = HardwareIDs;
        while (DeviceID && *DeviceID && (size = wcslen(DeviceID)) + 1 < sizeLeft)
        {
            TRACE("Adding %s to list\n", debugstr_w(DeviceID));
            wcscpy(pBuffer, DeviceID);
            DeviceID += size + 1;
            pBuffer += size + 1;
            sizeLeft -= size + 1;
            DriverInfoDetailData->CompatIDsOffset += size + 1;
        }
        if (sizeLeft > 0)
        {
            *pBuffer = UNICODE_NULL;
            sizeLeft--;
            DriverInfoDetailData->CompatIDsOffset++;
        }
        /* Add as many as possible CompatibleIDs in the list */
        DeviceID = CompatibleIDs;
        while (DeviceID && *DeviceID && (size = wcslen(DeviceID)) + 1 < sizeLeft)
        {
            TRACE("Adding %s to list\n", debugstr_w(DeviceID));
            wcscpy(pBuffer, DeviceID);
            DeviceID += size + 1;
            pBuffer += size + 1;
            sizeLeft -= size + 1;
            DriverInfoDetailData->CompatIDsLength += size + 1;
        }
        if (sizeLeft > 0)
        {
            *pBuffer = UNICODE_NULL;
            sizeLeft--;
            DriverInfoDetailData->CompatIDsLength++;
        }

        if (sizeNeeded > DriverInfoDetailDataSize)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        else
            ret = TRUE;

done:
        MyFree(HardwareIDs);
        MyFree(CompatibleIDs);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiSelectBestCompatDrv (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSelectBestCompatDrv(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    SP_DRVINFO_DATA_W drvInfoData;
    BOOL ret;

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

    /* Drivers are sorted by rank in the driver list, so
     * the first driver in the list is the best one.
     */
    drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA_W);
    ret = SetupDiEnumDriverInfoW(
        DeviceInfoSet,
        DeviceInfoData,
        SPDIT_COMPATDRIVER,
        0, /* Member index */
        &drvInfoData);

    if (ret)
    {
        ret = SetupDiSetSelectedDriverW(
            DeviceInfoSet,
            DeviceInfoData,
            &drvInfoData);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiInstallDriverFiles (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiInstallDriverFiles(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEV_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInfoData && ((struct DeviceInfoElement *)DeviceInfoData->Reserved)->InstallParams.Reserved == 0)
        SetLastError(ERROR_NO_DRIVER_SELECTED);
    else if (!DeviceInfoData && ((struct DeviceInfoSet *)DeviceInfoSet)->InstallParams.Reserved == 0)
        SetLastError(ERROR_NO_DRIVER_SELECTED);
    else
    {
        SP_DEVINSTALL_PARAMS_W InstallParams;
        struct DriverInfoElement *SelectedDriver;
        WCHAR SectionName[MAX_PATH];
        DWORD SectionNameLength = 0;
        PVOID InstallMsgHandler;
        PVOID InstallMsgHandlerContext;
        PVOID Context = NULL;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        ret = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        if (!ret)
            goto done;

        SelectedDriver = (struct DriverInfoElement *)InstallParams.Reserved;
        if (!SelectedDriver)
        {
            SetLastError(ERROR_NO_DRIVER_SELECTED);
            goto done;
        }

        ret = SetupDiGetActualSectionToInstallW(
            SelectedDriver->InfFileDetails->hInf,
            SelectedDriver->Details.SectionName,
            SectionName, MAX_PATH - strlenW(DotCoInstallers), &SectionNameLength, NULL);
        if (!ret)
            goto done;

        if (InstallParams.InstallMsgHandler)
        {
            InstallMsgHandler = InstallParams.InstallMsgHandler;
            InstallMsgHandlerContext = InstallParams.InstallMsgHandlerContext;
        }
        else
        {
            Context = SetupInitDefaultQueueCallback(InstallParams.hwndParent);
            if (!Context)
                goto cleanup;
            InstallMsgHandler = SetupDefaultQueueCallback;
            InstallMsgHandlerContext = Context;
        }
        ret = SetupInstallFromInfSectionW(InstallParams.hwndParent,
            SelectedDriver->InfFileDetails->hInf, SectionName,
            SPINST_FILES, NULL, SelectedDriver->InfFileDetails->DirectoryName, SP_COPY_NEWER,
            InstallMsgHandler, InstallMsgHandlerContext,
            DeviceInfoSet, DeviceInfoData);
        if (!ret)
            goto done;

        /* Install files from .CoInstallers section */
        lstrcatW(SectionName, DotCoInstallers);
        ret = SetupInstallFromInfSectionW(InstallParams.hwndParent,
            SelectedDriver->InfFileDetails->hInf, SectionName,
            SPINST_FILES, NULL, SelectedDriver->InfFileDetails->DirectoryName, SP_COPY_NEWER,
            InstallMsgHandler, InstallMsgHandlerContext,
            DeviceInfoSet, DeviceInfoData);
        if (!ret)
            goto done;

        /* Set the DI_NOFILECOPY flag to prevent another
         * installation during SetupDiInstallDevice */
        InstallParams.Flags |= DI_NOFILECOPY;
        ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);

cleanup:
       if (Context)
           SetupTermDefaultQueueCallback(Context);
    }

done:
    TRACE("Returning %d\n", ret);
    return ret;
}
