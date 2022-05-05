/*
 * SetupAPI device installer
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
 *           2005-2006 Hervé Poussineau (hpoussin@reactos.org)
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

#include <pseh/pseh2.h>

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR DateFormat[]  = {'%','u','-','%','u','-','%','u',0};
static const WCHAR DotCoInstallers[]  = {'.','C','o','I','n','s','t','a','l','l','e','r','s',0};
static const WCHAR DotHW[]  = {'.','H','W',0};
static const WCHAR DotServices[]  = {'.','S','e','r','v','i','c','e','s',0};
static const WCHAR InfDirectory[] = {'i','n','f','\\',0};
static const WCHAR InstanceKeyFormat[] = {'%','0','4','l','u',0};
static const WCHAR Version[]  = {'V','e','r','s','i','o','n',0};
static const WCHAR VersionFormat[] = {'%','u','.','%','u','.','%','u','.','%','u',0};

static const WCHAR REGSTR_DRIVER_DATE[]  = {'D','r','i','v','e','r','D','a','t','e',0};
static const WCHAR REGSTR_DRIVER_DATE_DATA[]  = {'D','r','i','v','e','r','D','a','t','e','D','a','t','a',0};
static const WCHAR REGSTR_DRIVER_VERSION[]  = {'D','r','i','v','e','r','V','e','r','s','i','o','n',0};
static const WCHAR REGSTR_SECURITY[]  = {'S','e','c','u','r','i','t','y',0};
static const WCHAR REGSTR_UI_NUMBER_DESC_FORMAT[]  = {'U','I','N','u','m','b','e','r','D','e','s','c','F','o','r','m','a','t',0};

typedef DWORD
(CALLBACK* CLASS_INSTALL_PROC) (
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL);
typedef BOOL
(WINAPI* DEFAULT_CLASS_INSTALL_PROC) (
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData);
typedef DWORD
(CALLBACK* COINSTALLER_PROC) (
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context);

struct CoInstallerElement
{
    LIST_ENTRY ListEntry;

    HMODULE Module;
    COINSTALLER_PROC Function;
    BOOL DoPostProcessing;
    PVOID PrivateData;
};

struct GetSectionCallbackInfo
{
    PSP_ALTPLATFORM_INFO PlatformInfo;
    BYTE ProductType;
    WORD SuiteMask;
    DWORD PrefixLength;
    WCHAR BestSection[LINE_LEN + 1];
    DWORD BestScore1, BestScore2, BestScore3, BestScore4, BestScore5;
};



static void SETUPDI_GuidToString(const GUID *guid, LPWSTR guidStr)
{
    static const WCHAR fmt[] = {'{','%','0','8','X','-','%','0','4','X','-',
        '%','0','4','X','-','%','0','2','X','%','0','2','X','-','%','0','2',
        'X','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X','%',
        '0','2','X','}',0};

    sprintfW(guidStr, fmt, guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

DWORD
GetErrorCodeFromCrCode(const IN CONFIGRET cr)
{
  switch (cr)
  {
    case CR_ACCESS_DENIED:        return ERROR_ACCESS_DENIED;
    case CR_BUFFER_SMALL:         return ERROR_INSUFFICIENT_BUFFER;
    case CR_CALL_NOT_IMPLEMENTED: return ERROR_CALL_NOT_IMPLEMENTED;
    case CR_FAILURE:              return ERROR_GEN_FAILURE;
    case CR_INVALID_DATA:         return ERROR_INVALID_USER_BUFFER;
    case CR_INVALID_DEVICE_ID:    return ERROR_INVALID_PARAMETER;
    case CR_INVALID_MACHINENAME:  return ERROR_INVALID_COMPUTERNAME;
    case CR_INVALID_DEVNODE:      return ERROR_INVALID_PARAMETER;
    case CR_INVALID_FLAG:         return ERROR_INVALID_FLAGS;
    case CR_INVALID_POINTER:      return ERROR_INVALID_PARAMETER;
    case CR_INVALID_PROPERTY:     return ERROR_INVALID_PARAMETER;
    case CR_NO_SUCH_DEVNODE:      return ERROR_FILE_NOT_FOUND;
    case CR_NO_SUCH_REGISTRY_KEY: return ERROR_FILE_NOT_FOUND;
    case CR_NO_SUCH_VALUE:        return ERROR_FILE_NOT_FOUND;
    case CR_OUT_OF_MEMORY:        return ERROR_NOT_ENOUGH_MEMORY;
    case CR_REGISTRY_ERROR:       return ERROR_GEN_FAILURE;
    case CR_ALREADY_SUCH_DEVINST: return ERROR_DEVINST_ALREADY_EXISTS;
    case CR_SUCCESS:              return ERROR_SUCCESS;
    default:                      return ERROR_GEN_FAILURE;
  }

  /* Does not happen */
}

/* Lower scores are best ones */
static BOOL
CheckSectionValid(
    IN LPCWSTR SectionName,
    IN PSP_ALTPLATFORM_INFO PlatformInfo,
    IN BYTE ProductType,
    IN WORD SuiteMask,
    OUT PDWORD ScorePlatform,
    OUT PDWORD ScoreMajorVersion,
    OUT PDWORD ScoreMinorVersion,
    OUT PDWORD ScoreProductType,
    OUT PDWORD ScoreSuiteMask)
{
    LPWSTR Section = NULL;
    //LPCWSTR pExtensionPlatform;
    LPCWSTR pExtensionArchitecture;
    LPWSTR Fields[6];
    DWORD i;
    BOOL ret = FALSE;

    //static const WCHAR ExtensionPlatformNone[]  = {'.',0};
    static const WCHAR ExtensionPlatformNT[]  = {'.','N','T',0};
    static const WCHAR ExtensionPlatformWindows[]  = {'.','W','i','n',0};

    static const WCHAR ExtensionArchitectureNone[]  = {0};
    static const WCHAR ExtensionArchitecturealpha[]  = {'a','l','p','h','a',0};
    static const WCHAR ExtensionArchitectureamd64[]  = {'A','M','D','6','4',0};
    static const WCHAR ExtensionArchitectureia64[]  = {'I','A','6','4',0};
    static const WCHAR ExtensionArchitecturemips[]  = {'m','i','p','s',0};
    static const WCHAR ExtensionArchitectureppc[]  = {'p','p','c',0};
    static const WCHAR ExtensionArchitecturex86[]  = {'x','8','6',0};

    TRACE("%s(%s %p 0x%x 0x%x)\n",
        __FUNCTION__, debugstr_w(SectionName), PlatformInfo, ProductType, SuiteMask);

    *ScorePlatform = *ScoreMajorVersion = *ScoreMinorVersion = *ScoreProductType = *ScoreSuiteMask = 0;

    Section = pSetupDuplicateString(SectionName);
    if (!Section)
    {
        TRACE("pSetupDuplicateString() failed\n");
        goto cleanup;
    }

    /* Set various extensions values */
    switch (PlatformInfo->Platform)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            //pExtensionPlatform = ExtensionPlatformWindows;
            break;
        case VER_PLATFORM_WIN32_NT:
            //pExtensionPlatform = ExtensionPlatformNT;
            break;
        default:
            ERR("Unknown platform 0x%lx\n", PlatformInfo->Platform);
            //pExtensionPlatform = ExtensionPlatformNone;
            break;
    }
    switch (PlatformInfo->ProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_ALPHA:
            pExtensionArchitecture = ExtensionArchitecturealpha;
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            pExtensionArchitecture = ExtensionArchitectureamd64;
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            pExtensionArchitecture = ExtensionArchitectureia64;
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            pExtensionArchitecture = ExtensionArchitecturex86;
            break;
        case PROCESSOR_ARCHITECTURE_MIPS:
            pExtensionArchitecture = ExtensionArchitecturemips;
            break;
        case PROCESSOR_ARCHITECTURE_PPC:
            pExtensionArchitecture = ExtensionArchitectureppc;
            break;
        default:
            ERR("Unknown processor architecture 0x%x\n", PlatformInfo->ProcessorArchitecture);
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
            pExtensionArchitecture = ExtensionArchitectureNone;
            break;
    }

    /*
     * Field[0] Platform
     * Field[1] Architecture
     * Field[2] Major version
     * Field[3] Minor version
     * Field[4] Product type
     * Field[5] Suite mask
     * Remark: these fields may be NULL if the information is not provided
     */
    Fields[0] = Section;
    if (Fields[0] == NULL)
    {
        TRACE("No extension found\n");
        *ScorePlatform = *ScoreMajorVersion = *ScoreMinorVersion = *ScoreProductType = *ScoreSuiteMask = ULONG_MAX;
        ret = TRUE;
        goto cleanup;
    }
    Fields[1] = Fields[0] + 1;
    Fields[2] = Fields[3] = Fields[4] = Fields[5] = NULL;
    for (i = 2; Fields[i - 1] != NULL && i < 6; i++)
    {
        Fields[i] = wcschr(Fields[i - 1], '.');
        if (Fields[i])
        {
            Fields[i]++;
            *(Fields[i] - 1) = UNICODE_NULL;
        }
    }
    /* Take care of first 2 fields */
    if (strncmpiW(Fields[0], ExtensionPlatformWindows, strlenW(ExtensionPlatformWindows)) == 0)
    {
        if (PlatformInfo->Platform != VER_PLATFORM_WIN32_WINDOWS)
        {
            TRACE("Mismatch on platform field\n");
            goto cleanup;
        }
        Fields[1] += wcslen(ExtensionPlatformWindows) - 1;
    }
    else if (strncmpiW(Fields[0], ExtensionPlatformNT, strlenW(ExtensionPlatformNT)) == 0)
    {
        if (PlatformInfo->Platform != VER_PLATFORM_WIN32_NT)
        {
            TRACE("Mismatch on platform field\n");
            goto cleanup;
        }
        Fields[1] += wcslen(ExtensionPlatformNT) - 1;
    }
    else
    {
        /* No platform specified */
        *ScorePlatform |= 0x02;
    }
    if (strcmpiW(Fields[1], ExtensionArchitectureNone) == 0)
    {
        /* No architecture specified */
        *ScorePlatform |= 0x01;
    }
    else if (strcmpiW(Fields[1], pExtensionArchitecture) != 0)
    {
        TRACE("Mismatch on architecture field ('%s' and '%s')\n",
            debugstr_w(Fields[1]), debugstr_w(pExtensionArchitecture));
        goto cleanup;
    }

    /* Check if informations are matching */
    if (Fields[2] && *Fields[2])
    {
        DWORD MajorVersion, MinorVersion = 0;
        MajorVersion = strtoulW(Fields[2], NULL, 0);
        if ((MajorVersion == 0 || MajorVersion == ULONG_MAX) &&
            (errno == ERANGE || errno == EINVAL))
        {
            TRACE("Wrong MajorVersion ('%s')\n", debugstr_w(Fields[2]));
            goto cleanup;
        }
        if (Fields[3] && *Fields[3])
        {
            MinorVersion = strtoulW(Fields[3], NULL, 0);
            if ((MinorVersion == 0 || MinorVersion == ULONG_MAX) &&
                (errno == ERANGE || errno == EINVAL))
            {
                TRACE("Wrong MinorVersion ('%s')\n", debugstr_w(Fields[3]));
                goto cleanup;
            }
        }
        if (PlatformInfo->MajorVersion < MajorVersion ||
            (PlatformInfo->MajorVersion == MajorVersion && PlatformInfo->MinorVersion < MinorVersion))
        {
            TRACE("Mismatch on version field (%lu.%lu and %lu.%lu)\n",
                MajorVersion, MinorVersion, PlatformInfo->MajorVersion, PlatformInfo->MinorVersion);
            goto cleanup;
        }
        *ScoreMajorVersion = MajorVersion - PlatformInfo->MajorVersion;
        if (MajorVersion == PlatformInfo->MajorVersion)
            *ScoreMinorVersion = MinorVersion - PlatformInfo->MinorVersion;
        else
            *ScoreMinorVersion = MinorVersion;
    }
    else if (Fields[3] && *Fields[3])
    {
        TRACE("Minor version found without major version\n");
        goto cleanup;
    }
    else
    {
        *ScoreMajorVersion = PlatformInfo->MajorVersion;
        *ScoreMinorVersion = PlatformInfo->MinorVersion;
    }

    if (Fields[4] && *Fields[4])
    {
        DWORD CurrentProductType;
        CurrentProductType = strtoulW(Fields[4], NULL, 0);
        if ((CurrentProductType == 0 || CurrentProductType == ULONG_MAX) &&
            (errno == ERANGE || errno == EINVAL))
        {
            TRACE("Wrong Product type ('%s')\n", debugstr_w(Fields[4]));
            goto cleanup;
        }
        if (CurrentProductType != ProductType)
        {
            TRACE("Mismatch on product type (0x%08lx and 0x%08x)\n",
                CurrentProductType, ProductType);
            goto cleanup;
        }
    }
    else
        *ScoreProductType = 1;

    if (Fields[5] && *Fields[5])
    {
        DWORD CurrentSuiteMask;
        CurrentSuiteMask = strtoulW(Fields[5], NULL, 0);
        if ((CurrentSuiteMask == 0 || CurrentSuiteMask == ULONG_MAX) &&
            (errno == ERANGE || errno == EINVAL))
        {
            TRACE("Wrong Suite mask ('%s')\n", debugstr_w(Fields[5]));
            goto cleanup;
        }
        if ((CurrentSuiteMask & ~SuiteMask) != 0)
        {
            TRACE("Mismatch on suite mask (0x%08lx and 0x%08x)\n",
                CurrentSuiteMask, SuiteMask);
            goto cleanup;
        }
        *ScoreSuiteMask = SuiteMask & ~CurrentSuiteMask;
    }
    else
        *ScoreSuiteMask = SuiteMask;

    ret = TRUE;

cleanup:
    MyFree(Section);
    return ret;
}

static BOOL
GetSectionCallback(
    IN LPCWSTR SectionName,
    IN PVOID Context)
{
    struct GetSectionCallbackInfo *info = Context;
    DWORD Score1, Score2, Score3, Score4, Score5;
    BOOL ret;

    if (SectionName[info->PrefixLength] != '.')
        return TRUE;

    ret = CheckSectionValid(
        &SectionName[info->PrefixLength],
        info->PlatformInfo,
        info->ProductType,
        info->SuiteMask,
        &Score1, &Score2, &Score3, &Score4, &Score5);
    if (!ret)
    {
        TRACE("Section %s not compatible\n", debugstr_w(SectionName));
        return TRUE;
    }
    if (Score1 > info->BestScore1) goto done;
    if (Score1 < info->BestScore1) goto bettersection;
    if (Score2 > info->BestScore2) goto done;
    if (Score2 < info->BestScore2) goto bettersection;
    if (Score3 > info->BestScore3) goto done;
    if (Score3 < info->BestScore3) goto bettersection;
    if (Score4 > info->BestScore4) goto done;
    if (Score4 < info->BestScore4) goto bettersection;
    if (Score5 > info->BestScore5) goto done;
    if (Score5 < info->BestScore5) goto bettersection;
    goto done;

bettersection:
    strcpyW(info->BestSection, SectionName);
    info->BestScore1 = Score1;
    info->BestScore2 = Score2;
    info->BestScore3 = Score3;
    info->BestScore4 = Score4;
    info->BestScore5 = Score5;

done:
    return TRUE;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallExW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetActualSectionToInstallExW(
    IN HINF InfHandle,
    IN PCWSTR InfSectionName,
    IN PSP_ALTPLATFORM_INFO AlternatePlatformInfo OPTIONAL,
    OUT PWSTR InfSectionWithExt OPTIONAL,
    IN DWORD InfSectionWithExtSize,
    OUT PDWORD RequiredSize OPTIONAL,
    OUT PWSTR* Extension OPTIONAL,
    IN PVOID Reserved)
{
    BOOL ret = FALSE;

    TRACE("%s(%p %s %p %p %lu %p %p %p)\n", __FUNCTION__, InfHandle, debugstr_w(InfSectionName),
        AlternatePlatformInfo, InfSectionWithExt, InfSectionWithExtSize,
        RequiredSize, Extension, Reserved);

    if (!InfHandle || InfHandle == (HINF)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!InfSectionName)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (AlternatePlatformInfo && AlternatePlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (Reserved != NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        static SP_ALTPLATFORM_INFO CurrentPlatform = { 0, };
        static BYTE CurrentProductType = 0;
        static WORD CurrentSuiteMask = 0;
        PSP_ALTPLATFORM_INFO pPlatformInfo = &CurrentPlatform;
        struct GetSectionCallbackInfo CallbackInfo;
        DWORD dwFullLength;
        BYTE ProductType;
        WORD SuiteMask;

        /* Fill platform info if needed */
        if (AlternatePlatformInfo)
        {
            pPlatformInfo = AlternatePlatformInfo;
            ProductType = 0;
            SuiteMask = 0;
        }
        else
        {
            if (CurrentPlatform.cbSize != sizeof(SP_ALTPLATFORM_INFO))
            {
                /* That's the first time we go here. We need to fill in the structure */
                SYSTEM_INFO SystemInfo;
                GetSystemInfo(&SystemInfo);
                CurrentPlatform.cbSize = sizeof(SP_ALTPLATFORM_INFO);
                CurrentPlatform.Platform = OsVersionInfo.dwPlatformId;
                CurrentPlatform.MajorVersion = OsVersionInfo.dwMajorVersion;
                CurrentPlatform.MinorVersion = OsVersionInfo.dwMinorVersion;
                CurrentPlatform.ProcessorArchitecture = SystemInfo.wProcessorArchitecture;
                CurrentPlatform.Reserved = 0;
                CurrentProductType = OsVersionInfo.wProductType;
                CurrentSuiteMask = OsVersionInfo.wSuiteMask;
            }
            ProductType = CurrentProductType;
            SuiteMask = CurrentSuiteMask;
        }

        CallbackInfo.PlatformInfo = pPlatformInfo;
        CallbackInfo.ProductType = ProductType;
        CallbackInfo.SuiteMask = SuiteMask;
        CallbackInfo.PrefixLength = strlenW(InfSectionName);
        CallbackInfo.BestScore1 = ULONG_MAX;
        CallbackInfo.BestScore2 = ULONG_MAX;
        CallbackInfo.BestScore3 = ULONG_MAX;
        CallbackInfo.BestScore4 = ULONG_MAX;
        CallbackInfo.BestScore5 = ULONG_MAX;
        strcpyW(CallbackInfo.BestSection, InfSectionName);
        TRACE("EnumerateSectionsStartingWith(InfSectionName = %S)\n", InfSectionName);
        if (!EnumerateSectionsStartingWith(
            InfHandle,
            InfSectionName,
            GetSectionCallback,
            &CallbackInfo))
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto done;
        }
        TRACE("CallbackInfo.BestSection = %S\n", CallbackInfo.BestSection);

        dwFullLength = lstrlenW(CallbackInfo.BestSection);
        if (RequiredSize != NULL)
            *RequiredSize = dwFullLength + 1;

        if (InfSectionWithExtSize > 0)
        {
            if (InfSectionWithExtSize < dwFullLength + 1)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                goto done;
            }
            strcpyW(InfSectionWithExt, CallbackInfo.BestSection);
            if (Extension)
            {
                DWORD dwLength = lstrlenW(InfSectionName);
                *Extension = (dwLength == dwFullLength) ? NULL : &InfSectionWithExt[dwLength];
            }
        }

        ret = TRUE;
    }

done:
    TRACE("Returning %d\n", ret);
    return ret;
}


BOOL
CreateDeviceInfo(
    IN struct DeviceInfoSet *list,
    IN LPCWSTR InstancePath,
    IN LPCGUID pClassGuid,
    OUT struct DeviceInfo **pDeviceInfo)
{
    DWORD size;
    CONFIGRET cr;
    struct DeviceInfo *deviceInfo;

    *pDeviceInfo = NULL;

    size = FIELD_OFFSET(struct DeviceInfo, Data) + (strlenW(InstancePath) + 1) * sizeof(WCHAR);
    deviceInfo = HeapAlloc(GetProcessHeap(), 0, size);
    if (!deviceInfo)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    ZeroMemory(deviceInfo, size);

    cr = CM_Locate_DevNode_ExW(&deviceInfo->dnDevInst, (DEVINSTID_W)InstancePath, CM_LOCATE_DEVNODE_PHANTOM, list->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(GetErrorCodeFromCrCode(cr));
        return FALSE;
    }

    deviceInfo->set = list;
    deviceInfo->InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
    strcpyW(deviceInfo->Data, InstancePath);
    deviceInfo->instanceId = deviceInfo->Data;
    deviceInfo->UniqueId = strrchrW(deviceInfo->Data, '\\');
    deviceInfo->DeviceDescription = NULL;
    memcpy(&deviceInfo->ClassGuid, pClassGuid, sizeof(GUID));
    deviceInfo->CreationFlags = 0;
    InitializeListHead(&deviceInfo->DriverListHead);
    InitializeListHead(&deviceInfo->InterfaceListHead);

    *pDeviceInfo = deviceInfo;
    return TRUE;
}


static BOOL
DestroyClassInstallParams(struct ClassInstallParams* installParams)
{
    HeapFree(GetProcessHeap(), 0, installParams->PropChangeParams);
    HeapFree(GetProcessHeap(), 0, installParams->AddPropertyPageData);
    return TRUE;
}

static BOOL
DestroyDeviceInfo(struct DeviceInfo *deviceInfo)
{
    PLIST_ENTRY ListEntry;
    struct DriverInfoElement *driverInfo;
    struct DeviceInterface *deviceInterface;

    while (!IsListEmpty(&deviceInfo->DriverListHead))
    {
        ListEntry = RemoveHeadList(&deviceInfo->DriverListHead);
        driverInfo = CONTAINING_RECORD(ListEntry, struct DriverInfoElement, ListEntry);
        if (!DestroyDriverInfoElement(driverInfo))
            return FALSE;
    }
    while (!IsListEmpty(&deviceInfo->InterfaceListHead))
    {
        ListEntry = RemoveHeadList(&deviceInfo->InterfaceListHead);
        deviceInterface = CONTAINING_RECORD(ListEntry, struct DeviceInterface, ListEntry);
        if (!DestroyDeviceInterface(deviceInterface))
            return FALSE;
    }
    DestroyClassInstallParams(&deviceInfo->ClassInstallParams);
    if (deviceInfo->hmodDevicePropPageProvider)
        FreeLibrary(deviceInfo->hmodDevicePropPageProvider);
    return HeapFree(GetProcessHeap(), 0, deviceInfo);
}

static BOOL
DestroyDeviceInfoSet(struct DeviceInfoSet* list)
{
    PLIST_ENTRY ListEntry;
    struct DeviceInfo *deviceInfo;

    while (!IsListEmpty(&list->ListHead))
    {
        ListEntry = RemoveHeadList(&list->ListHead);
        deviceInfo = CONTAINING_RECORD(ListEntry, struct DeviceInfo, ListEntry);
        if (!DestroyDeviceInfo(deviceInfo))
            return FALSE;
    }
    if (list->HKLM != HKEY_LOCAL_MACHINE)
        RegCloseKey(list->HKLM);
    CM_Disconnect_Machine(list->hMachine);
    DestroyClassInstallParams(&list->ClassInstallParams);
    if (list->hmodClassPropPageProvider)
        FreeLibrary(list->hmodClassPropPageProvider);
    return HeapFree(GetProcessHeap(), 0, list);
}

/***********************************************************************
 *              SetupDiBuildClassInfoList  (SETUPAPI.@)
 *
 * Returns a list of setup class GUIDs that identify the classes
 * that are installed on a local machine.
 *
 * PARAMS
 *   Flags [I] control exclusion of classes from the list.
 *   ClassGuidList [O] pointer to a GUID-typed array that receives a list of setup class GUIDs.
 *   ClassGuidListSize [I] The number of GUIDs in the array (ClassGuidList).
 *   RequiredSize [O] pointer, which receives the number of GUIDs that are returned.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOL WINAPI SetupDiBuildClassInfoList(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
    TRACE("\n");
    return SetupDiBuildClassInfoListExW(Flags, ClassGuidList,
                                        ClassGuidListSize, RequiredSize,
                                        NULL, NULL);
}

/***********************************************************************
 *              SetupDiBuildClassInfoListExA  (SETUPAPI.@)
 *
 * Returns a list of setup class GUIDs that identify the classes
 * that are installed on a local or remote machine.
 *
 * PARAMS
 *   Flags [I] control exclusion of classes from the list.
 *   ClassGuidList [O] pointer to a GUID-typed array that receives a list of setup class GUIDs.
 *   ClassGuidListSize [I] The number of GUIDs in the array (ClassGuidList).
 *   RequiredSize [O] pointer, which receives the number of GUIDs that are returned.
 *   MachineName [I] name of a remote machine.
 *   Reserved [I] must be NULL.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOL WINAPI SetupDiBuildClassInfoListExA(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    BOOL bResult;

    TRACE("%s(0x%lx %p %lu %p %s %p)\n", __FUNCTION__, Flags, ClassGuidList,
        ClassGuidListSize, RequiredSize, debugstr_a(MachineName), Reserved);

    if (MachineName)
    {
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL) return FALSE;
    }

    bResult = SetupDiBuildClassInfoListExW(Flags, ClassGuidList,
                                           ClassGuidListSize, RequiredSize,
                                           MachineNameW, Reserved);

    MyFree(MachineNameW);

    return bResult;
}

/***********************************************************************
 *              SetupDiBuildClassInfoListExW  (SETUPAPI.@)
 *
 * Returns a list of setup class GUIDs that identify the classes
 * that are installed on a local or remote machine.
 *
 * PARAMS
 *   Flags [I] control exclusion of classes from the list.
 *   ClassGuidList [O] pointer to a GUID-typed array that receives a list of setup class GUIDs.
 *   ClassGuidListSize [I] The number of GUIDs in the array (ClassGuidList).
 *   RequiredSize [O] pointer, which receives the number of GUIDs that are returned.
 *   MachineName [I] name of a remote machine.
 *   Reserved [I] must be NULL.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOL WINAPI SetupDiBuildClassInfoListExW(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCWSTR MachineName,
        PVOID Reserved)
{
    GUID CurrentClassGuid;
    HKEY hClassKey;
    DWORD dwIndex;
    DWORD dwGuidListIndex = 0;
    HMACHINE hMachine = NULL;
    CONFIGRET cr;

    TRACE("%s(0x%lx %p %lu %p %s %p)\n", __FUNCTION__, Flags, ClassGuidList,
        ClassGuidListSize, RequiredSize, debugstr_w(MachineName), Reserved);

    if (!RequiredSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    else if (!ClassGuidList && ClassGuidListSize > 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (MachineName)
    {
        cr = CM_Connect_MachineW(MachineName, &hMachine);
        if (cr != CR_SUCCESS)
        {
            SetLastError(GetErrorCodeFromCrCode(cr));
            return FALSE;
        }
    }

    for (dwIndex = 0; ; dwIndex++)
    {
        cr = CM_Enumerate_Classes_Ex(dwIndex,
                                     &CurrentClassGuid,
                                     0,
                                     hMachine);
        if (cr == CR_SUCCESS)
        {
            TRACE("Guid: %s\n", debugstr_guid(&CurrentClassGuid));
            if (CM_Open_Class_Key_ExW(&CurrentClassGuid,
                                       NULL,
                                       KEY_QUERY_VALUE,
                                       RegDisposition_OpenExisting,
                                       &hClassKey,
                                       CM_OPEN_CLASS_KEY_INSTALLER,
                                       hMachine) != CR_SUCCESS)
            {
                SetLastError(GetErrorCodeFromCrCode(cr));
                if (hMachine)
                    CM_Disconnect_Machine(hMachine);
                return FALSE;
            }

            if (!RegQueryValueExW(hClassKey,
                                  REGSTR_VAL_NOUSECLASS,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL))
            {
                TRACE("'NoUseClass' value found!\n");
                RegCloseKey(hClassKey);
                continue;
            }

            if ((Flags & DIBCI_NOINSTALLCLASS) &&
                (!RegQueryValueExW(hClassKey,
                                   REGSTR_VAL_NOINSTALLCLASS,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL)))
            {
                TRACE("'NoInstallClass' value found!\n");
                RegCloseKey(hClassKey);
                continue;
            }

            if ((Flags & DIBCI_NODISPLAYCLASS) &&
                (!RegQueryValueExW(hClassKey,
                                   REGSTR_VAL_NODISPLAYCLASS,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL)))
            {
                TRACE("'NoDisplayClass' value found!\n");
                RegCloseKey(hClassKey);
                continue;
            }

            RegCloseKey(hClassKey);

            if (dwGuidListIndex < ClassGuidListSize)
            {
                ClassGuidList[dwGuidListIndex] = CurrentClassGuid;
            }

            dwGuidListIndex++;
        }

        if (cr != ERROR_SUCCESS)
            break;
    }

    if (hMachine)
        CM_Disconnect_Machine(hMachine);

    if (RequiredSize != NULL)
        *RequiredSize = dwGuidListIndex;

    if (ClassGuidListSize < dwGuidListIndex)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameA(
        LPCSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
    return SetupDiClassGuidsFromNameExA(ClassName, ClassGuidList,
                                        ClassGuidListSize, RequiredSize,
                                        NULL, NULL);
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameW(
        LPCWSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
    return SetupDiClassGuidsFromNameExW(ClassName, ClassGuidList,
                                        ClassGuidListSize, RequiredSize,
                                        NULL, NULL);
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameExA(
        LPCSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR ClassNameW = NULL;
    LPWSTR MachineNameW = NULL;
    BOOL bResult;

    TRACE("%s(%s %p %lu %p %s %p)\n", __FUNCTION__, debugstr_a(ClassName), ClassGuidList,
        ClassGuidListSize, RequiredSize, debugstr_a(MachineName), Reserved);

    if (!ClassName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ClassNameW = pSetupMultiByteToUnicode(ClassName, CP_ACP);
    if (ClassNameW == NULL)
        return FALSE;

    if (MachineName)
    {
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
        {
            MyFree(ClassNameW);
            return FALSE;
        }
    }

    bResult = SetupDiClassGuidsFromNameExW(ClassNameW, ClassGuidList,
                                           ClassGuidListSize, RequiredSize,
                                           MachineNameW, Reserved);

    MyFree(MachineNameW);
    MyFree(ClassNameW);

    return bResult;
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameExW(
        LPCWSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCWSTR MachineName,
        PVOID Reserved)
{
    WCHAR szKeyName[40];
    WCHAR szClassName[MAX_CLASS_NAME_LEN];
    HKEY hClassesKey;
    HKEY hClassKey;
    DWORD dwLength;
    DWORD dwIndex;
    LONG lError;
    DWORD dwGuidListIndex = 0;

    TRACE("%s(%s %p %lu %p %s %p)\n", __FUNCTION__, debugstr_w(ClassName), ClassGuidList,
        ClassGuidListSize, RequiredSize, debugstr_w(MachineName), Reserved);

    if (!ClassName || !RequiredSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!ClassGuidList && ClassGuidListSize > 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    *RequiredSize = 0;

    hClassesKey = SetupDiOpenClassRegKeyExW(NULL,
                                            KEY_ENUMERATE_SUB_KEYS,
                                            DIOCR_INSTALLER,
                                            MachineName,
                                            Reserved);
    if (hClassesKey == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    for (dwIndex = 0; ; dwIndex++)
    {
        dwLength = 40;
        lError = RegEnumKeyExW(hClassesKey,
                               dwIndex,
                               szKeyName,
                               &dwLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        TRACE("RegEnumKeyExW() returns %d\n", lError);
        if (lError == ERROR_SUCCESS || lError == ERROR_MORE_DATA)
        {
            TRACE("Key name: %p\n", szKeyName);

            if (RegOpenKeyExW(hClassesKey,
                              szKeyName,
                              0,
                              KEY_QUERY_VALUE,
                              &hClassKey))
            {
                RegCloseKey(hClassesKey);
                return FALSE;
            }

            dwLength = MAX_CLASS_NAME_LEN * sizeof(WCHAR);
            if (!RegQueryValueExW(hClassKey,
                                  REGSTR_VAL_CLASS,
                                  NULL,
                                  NULL,
                                  (LPBYTE)szClassName,
                                  &dwLength))
            {
                TRACE("Class name: %p\n", szClassName);

                if (strcmpiW(szClassName, ClassName) == 0)
                {
                    TRACE("Found matching class name\n");

                    TRACE("Guid: %p\n", szKeyName);
                    if (dwGuidListIndex < ClassGuidListSize)
                    {
                        if (szKeyName[0] == '{' && szKeyName[37] == '}')
                        {
                            szKeyName[37] = 0;
                        }
                        TRACE("Guid: %p\n", &szKeyName[1]);

                        UuidFromStringW(&szKeyName[1],
                                        &ClassGuidList[dwGuidListIndex]);
                    }

                    dwGuidListIndex++;
                }
            }

            RegCloseKey(hClassKey);
        }

        if (lError != ERROR_SUCCESS)
            break;
    }

    RegCloseKey(hClassesKey);

    if (RequiredSize != NULL)
        *RequiredSize = dwGuidListIndex;

    if (ClassGuidListSize < dwGuidListIndex)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiClassNameFromGuidA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidA(
        const GUID* ClassGuid,
        PSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize)
{
    return SetupDiClassNameFromGuidExA(ClassGuid, ClassName,
                                       ClassNameSize, RequiredSize,
                                       NULL, NULL);
}

/***********************************************************************
 *              SetupDiClassNameFromGuidW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidW(
        const GUID* ClassGuid,
        PWSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize)
{
    return SetupDiClassNameFromGuidExW(ClassGuid, ClassName,
                                       ClassNameSize, RequiredSize,
                                       NULL, NULL);
}

/***********************************************************************
 *              SetupDiClassNameFromGuidExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidExA(
        const GUID* ClassGuid,
        PSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize,
        PCSTR MachineName,
        PVOID Reserved)
{
    WCHAR ClassNameW[MAX_CLASS_NAME_LEN];
    LPWSTR MachineNameW = NULL;
    BOOL ret;

    if (MachineName)
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
    ret = SetupDiClassNameFromGuidExW(ClassGuid, ClassNameW, MAX_CLASS_NAME_LEN,
                                      RequiredSize, MachineNameW, Reserved);
    if (ret)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ClassNameW, -1, ClassName,
                                      ClassNameSize, NULL, NULL);
        if (len == 0 || len > ClassNameSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
    }
    MyFree(MachineNameW);
    return ret;
}

/***********************************************************************
 *		SetupDiClassNameFromGuidExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidExW(
        const GUID* ClassGuid,
        PWSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;
    DWORD dwRegType;
    LONG rc;
    PWSTR Buffer;

    TRACE("%s(%s %p %lu %p %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), ClassName,
        ClassNameSize, RequiredSize, debugstr_w(MachineName), Reserved);

    /* Make sure there's a GUID */
    if (ClassGuid == NULL)
    {
        SetLastError(ERROR_INVALID_CLASS);  /* On Vista: ERROR_INVALID_USER_BUFFER */
        return FALSE;
    }

    /* Make sure there's a real buffer when there's a size */
    if ((ClassNameSize > 0) && (ClassName == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);  /* On Vista: ERROR_INVALID_USER_BUFFER */
        return FALSE;
    }

    /* Open the key for the GUID */
    hKey = SetupDiOpenClassRegKeyExW(ClassGuid, KEY_QUERY_VALUE, DIOCR_INSTALLER, MachineName, Reserved);

    if (hKey == INVALID_HANDLE_VALUE)
        return FALSE;

    /* Retrieve the class name data and close the key */
    rc = QueryRegistryValue(hKey, REGSTR_VAL_CLASS, (LPBYTE *) &Buffer, &dwRegType, &dwLength);
    RegCloseKey(hKey);

    /* Make sure we got the data */
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        return FALSE;
    }

    /* Make sure the data is a string */
    if (dwRegType != REG_SZ)
    {
        MyFree(Buffer);
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }

    /* Determine the length of the class name */
    dwLength /= sizeof(WCHAR);

    if ((dwLength == 0) || (Buffer[dwLength - 1] != UNICODE_NULL))
        /* Count the null-terminator */
        dwLength++;

    /* Inform the caller about the class name */
    if ((ClassName != NULL) && (dwLength <= ClassNameSize))
    {
        memcpy(ClassName, Buffer, (dwLength - 1) * sizeof(WCHAR));
        ClassName[dwLength - 1] = UNICODE_NULL;
    }

    /* Inform the caller about the required size */
    if (RequiredSize != NULL)
        *RequiredSize = dwLength;

    /* Clean up the buffer */
    MyFree(Buffer);

    /* Make sure the buffer was large enough */
    if ((ClassName == NULL) || (dwLength > ClassNameSize))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoList (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoList(const GUID *ClassGuid,
        HWND hwndParent)
{
    return SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent, NULL, NULL);
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExA (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExA(const GUID *ClassGuid,
        HWND hwndParent,
        PCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    HDEVINFO hDevInfo;

    TRACE("%s(%s %p %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), hwndParent,
      debugstr_a(MachineName), Reserved);

    if (MachineName)
    {
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return INVALID_HANDLE_VALUE;
    }

    hDevInfo = SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent,
                                              MachineNameW, Reserved);

    MyFree(MachineNameW);

    return hDevInfo;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExW (SETUPAPI.@)
 *
 * Create an empty DeviceInfoSet list.
 *
 * PARAMS
 *   ClassGuid [I] if not NULL only devices with GUID ClassGuid are associated
 *                 with this list.
 *   hwndParent [I] hwnd needed for interface related actions.
 *   MachineName [I] name of machine to create emtpy DeviceInfoSet list, if NULL
 *                   local registry will be used.
 *   Reserved [I] must be NULL
 *
 * RETURNS
 *   Success: empty list.
 *   Failure: INVALID_HANDLE_VALUE.
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExW(const GUID *ClassGuid,
        HWND hwndParent,
        PCWSTR MachineName,
        PVOID Reserved)
{
    struct DeviceInfoSet *list = NULL;
    DWORD size = FIELD_OFFSET(struct DeviceInfoSet, szData);
    DWORD rc;
    CONFIGRET cr;
    HDEVINFO ret = INVALID_HANDLE_VALUE;

    TRACE("%s(%s %p %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), hwndParent,
      debugstr_w(MachineName), Reserved);

    if (MachineName != NULL)
    {
        SIZE_T len = strlenW(MachineName);
        if (len >= SP_MAX_MACHINENAME_LENGTH - 4)
        {
            SetLastError(ERROR_INVALID_MACHINENAME);
            goto cleanup;
        }
        if(len > 0)
            size += (len + 3) * sizeof(WCHAR);
        else
            MachineName = NULL;
    }

    if (Reserved != NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    list = MyMalloc(size);
    if (!list)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
    ZeroMemory(list, FIELD_OFFSET(struct DeviceInfoSet, szData));

    list->magic = SETUP_DEVICE_INFO_SET_MAGIC;
    memcpy(&list->ClassGuid,
            ClassGuid ? ClassGuid : &GUID_NULL,
            sizeof(list->ClassGuid));
    list->InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
    list->InstallParams.Flags |= DI_CLASSINSTALLPARAMS;
    list->InstallParams.hwndParent = hwndParent;
    if (MachineName)
    {
        rc = RegConnectRegistryW(MachineName, HKEY_LOCAL_MACHINE, &list->HKLM);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_MACHINENAME);
            goto cleanup;
        }

        list->szData[0] = list->szData[1] = '\\';
        strcpyW(list->szData + 2, MachineName);
        list->MachineName = list->szData;
    }
    else
    {
        list->HKLM = HKEY_LOCAL_MACHINE;
        list->MachineName = NULL;
    }
    cr = CM_Connect_MachineW(list->MachineName, &list->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(GetErrorCodeFromCrCode(cr));
        goto cleanup;
    }
    InitializeListHead(&list->DriverListHead);
    InitializeListHead(&list->ListHead);

    return (HDEVINFO)list;

cleanup:
    if (ret == INVALID_HANDLE_VALUE)
    {
        if (list)
        {
            if (list->HKLM != NULL && list->HKLM != HKEY_LOCAL_MACHINE)
                RegCloseKey(list->HKLM);
            MyFree(list);
        }
    }
    return ret;
}

/***********************************************************************
 *              SetupDiCreateDevRegKeyA (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDevRegKeyA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Scope,
        DWORD HwProfile,
        DWORD KeyType,
        HINF InfHandle,
        PCSTR InfSectionName)
{
    PWSTR InfSectionNameW = NULL;
    HKEY key;

    TRACE("%s(%p %p %d %d %d %p %s)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, Scope,
            HwProfile, KeyType, InfHandle, debugstr_a(InfSectionName));

    if (InfHandle)
    {
        if (!InfSectionName)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        else
        {
            InfSectionNameW = pSetupMultiByteToUnicode(InfSectionName, CP_ACP);
            if (InfSectionNameW == NULL) return INVALID_HANDLE_VALUE;
        }
    }
    key = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, Scope,
            HwProfile, KeyType, InfHandle, InfSectionNameW);
    MyFree(InfSectionNameW);
    return key;
}

static HKEY
OpenHardwareProfileKey(
    IN HKEY HKLM,
    IN DWORD HwProfile,
    IN DWORD samDesired);

/***********************************************************************
 *              SetupDiCreateDevRegKeyW (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDevRegKeyW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Scope,
        DWORD HwProfile,
        DWORD KeyType,
        HINF InfHandle,
        PCWSTR InfSectionName)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo;
    HKEY key = INVALID_HANDLE_VALUE;
    DWORD rc;
    HKEY hHWProfileKey = INVALID_HANDLE_VALUE;
    HKEY hKey = NULL;
    HKEY RootKey;

    TRACE("%s(%p %p %lu %lu %lu %p %s)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, Scope,
            HwProfile, KeyType, InfHandle, debugstr_w(InfSectionName));

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    if (KeyType != DIREG_DEV && KeyType != DIREG_DRV)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    if (InfHandle && !InfSectionName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (!InfHandle && InfSectionName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

        if (Scope == DICS_FLAG_GLOBAL)
            RootKey = set->HKLM;
        else /* Scope == DICS_FLAG_CONFIGSPECIFIC */
        {
            hHWProfileKey = OpenHardwareProfileKey(set->HKLM, HwProfile, KEY_CREATE_SUB_KEY);
            if (hHWProfileKey == INVALID_HANDLE_VALUE)
                goto cleanup;
            RootKey = hHWProfileKey;
        }

        if (KeyType == DIREG_DEV)
        {
#if _WIN32_WINNT >= 0x502
            hKey = SETUPDI_CreateDevKey(RootKey, deviceInfo, KEY_READ | KEY_WRITE);
#else
            hKey = SETUPDI_CreateDevKey(RootKey, deviceInfo, KEY_ALL_ACCESS);
#endif
            if (hKey == INVALID_HANDLE_VALUE)
                goto cleanup;

            if (Scope == DICS_FLAG_GLOBAL)
            {
                HKEY hTempKey = hKey;

                rc = RegCreateKeyExW(hTempKey,
                                     L"Device Parameters",
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
#if _WIN32_WINNT >= 0x502
                                     KEY_READ | KEY_WRITE,
#else
                                     KEY_ALL_ACCESS,
#endif
                                     NULL,
                                     &hKey,
                                     NULL);
                if (rc == ERROR_SUCCESS)
                    RegCloseKey(hTempKey);
            }
        }
        else /* KeyType == DIREG_DRV */
        {
#if _WIN32_WINNT >= 0x502
            hKey = SETUPDI_CreateDrvKey(RootKey, deviceInfo, (UUID*)&DeviceInfoData->ClassGuid, KEY_READ | KEY_WRITE);
#else
            hKey = SETUPDI_CreateDrvKey(RootKey, deviceInfo, (UUID*)&DeviceInfoData->ClassGuid, KEY_ALL_ACCESS);
#endif
            if (hKey == INVALID_HANDLE_VALUE)
                goto cleanup;
        }

        /* Do installation of the specified section */
        if (InfHandle)
        {
            FIXME("Need to install section %s in file %p\n",
                debugstr_w(InfSectionName), InfHandle);
        }
        key = hKey;

cleanup:
        if (hHWProfileKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hHWProfileKey);
        if (hKey != NULL && hKey != key)
            RegCloseKey(hKey);

    TRACE("Returning 0x%p\n", key);
    return key;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoA(
        HDEVINFO DeviceInfoSet,
        PCSTR DeviceName,
        CONST GUID *ClassGuid,
        PCSTR DeviceDescription,
        HWND hwndParent,
        DWORD CreationFlags,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret;
    LPWSTR DeviceNameW = NULL;
    LPWSTR DeviceDescriptionW = NULL;

    TRACE("\n");

    if (DeviceName)
    {
        DeviceNameW = pSetupMultiByteToUnicode(DeviceName, CP_ACP);
        if (DeviceNameW == NULL) return FALSE;
    }
    if (DeviceDescription)
    {
        DeviceDescriptionW = pSetupMultiByteToUnicode(DeviceDescription, CP_ACP);
        if (DeviceDescriptionW == NULL)
        {
            MyFree(DeviceNameW);
            return FALSE;
        }
    }

    ret = SetupDiCreateDeviceInfoW(DeviceInfoSet, DeviceNameW, ClassGuid, DeviceDescriptionW,
            hwndParent, CreationFlags, DeviceInfoData);

    MyFree(DeviceNameW);
    MyFree(DeviceDescriptionW);

    return ret;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoW(
        HDEVINFO DeviceInfoSet,
        PCWSTR DeviceName,
        CONST GUID *ClassGuid,
        PCWSTR DeviceDescription,
        HWND hwndParent,
        DWORD CreationFlags,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo = NULL;
    BOOL ret = FALSE;
    CONFIGRET cr;
    DEVINST RootDevInst;
    DEVINST DevInst;
    WCHAR GenInstanceId[MAX_DEVICE_ID_LEN];
    DWORD dwFlags;

    TRACE("%s(%p %s %s %s %p %x %p)\n", __FUNCTION__, DeviceInfoSet, debugstr_w(DeviceName),
        debugstr_guid(ClassGuid), debugstr_w(DeviceDescription),
        hwndParent, CreationFlags, DeviceInfoData);

    if (!DeviceName)
    {
        SetLastError(ERROR_INVALID_DEVINST_NAME);
        return FALSE;
    }
    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!ClassGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!IsEqualGUID(&set->ClassGuid, &GUID_NULL) &&
        !IsEqualGUID(ClassGuid, &set->ClassGuid))
    {
        SetLastError(ERROR_CLASS_MISMATCH);
        return FALSE;
    }
    if (CreationFlags & ~(DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS))
    {
        TRACE("Unknown flags: 0x%08lx\n", CreationFlags & ~(DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS));
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    /* Get the root device instance */
    cr = CM_Locate_DevInst_ExW(&RootDevInst,
                               NULL,
                               CM_LOCATE_DEVINST_NORMAL,
                               set->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    dwFlags = CM_CREATE_DEVINST_PHANTOM;
    if (CreationFlags & DICD_GENERATE_ID)
        dwFlags |= CM_CREATE_DEVINST_GENERATE_ID;

    /* Create the new device instance */
    cr = CM_Create_DevInst_ExW(&DevInst,
                               (DEVINSTID)DeviceName,
                               RootDevInst,
                               dwFlags,
                               set->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(GetErrorCodeFromCrCode(cr));
        return FALSE;
    }

    if (CreationFlags & DICD_GENERATE_ID)
    {
        /* Grab the actual instance ID that was created */
        cr = CM_Get_Device_ID_Ex(DevInst,
                                 GenInstanceId,
                                 MAX_DEVICE_ID_LEN,
                                 0,
                                 set->hMachine);
        if (cr != CR_SUCCESS)
        {
            SetLastError(GetErrorCodeFromCrCode(cr));
            return FALSE;
        }

        DeviceName = GenInstanceId;
        TRACE("Using generated instance ID: %s\n", debugstr_w(DeviceName));
    }

    if (CreateDeviceInfo(set, DeviceName, ClassGuid, &deviceInfo))
    {
        InsertTailList(&set->ListHead, &deviceInfo->ListEntry);

        if (!DeviceInfoData)
            ret = TRUE;
        else
        {
            if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
            {
                SetLastError(ERROR_INVALID_USER_BUFFER);
            }
            else
            {
                memcpy(&DeviceInfoData->ClassGuid, ClassGuid, sizeof(GUID));
                DeviceInfoData->DevInst = deviceInfo->dnDevInst;
                DeviceInfoData->Reserved = (ULONG_PTR)deviceInfo;
                ret = TRUE;
            }
        }
    }

    if (ret == FALSE)
    {
        if (deviceInfo != NULL)
        {
            /* Remove deviceInfo from List */
            RemoveEntryList(&deviceInfo->ListEntry);

            /* Destroy deviceInfo */
            DestroyDeviceInfo(deviceInfo);
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiRegisterDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRegisterDeviceInfo(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Flags,
        PSP_DETSIG_CMPPROC CompareProc,
        PVOID CompareContext,
        PSP_DEVINFO_DATA DupDeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    WCHAR DevInstId[MAX_DEVICE_ID_LEN];
    DEVINST ParentDevInst;
    CONFIGRET cr;
    DWORD dwError = ERROR_SUCCESS;

    TRACE("%s(%p %p %08x %p %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, Flags,
            CompareProc, CompareContext, DupDeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Flags & ~SPRDI_FIND_DUPS)
    {
        TRACE("Unknown flags: 0x%08lx\n", Flags & ~SPRDI_FIND_DUPS);
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (Flags & SPRDI_FIND_DUPS)
    {
        FIXME("Unimplemented codepath!\n");
    }

    CM_Get_Device_ID_Ex(DeviceInfoData->DevInst,
                        DevInstId,
                        MAX_DEVICE_ID_LEN,
                        0,
                        set->hMachine);

    CM_Get_Parent_Ex(&ParentDevInst,
                     DeviceInfoData->DevInst,
                     0,
                     set->hMachine);

    cr = CM_Create_DevInst_Ex(&DeviceInfoData->DevInst,
                              DevInstId,
                              ParentDevInst,
                              CM_CREATE_DEVINST_NORMAL | CM_CREATE_DEVINST_DO_NOT_INSTALL,
                              set->hMachine);
    if (cr != CR_SUCCESS &&
        cr != CR_ALREADY_SUCH_DEVINST)
    {
        dwError = ERROR_NO_SUCH_DEVINST;
    }

    SetLastError(dwError);

    return (dwError == ERROR_SUCCESS);
}

/***********************************************************************
 *		SetupDiEnumDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInfo(
        HDEVINFO  devinfo,
        DWORD  index,
        PSP_DEVINFO_DATA info)
{
    BOOL ret = FALSE;

    TRACE("%s(%p %d %p)\n", __FUNCTION__, devinfo, index, info);

    if(info==NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (devinfo && devinfo != INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)devinfo;
        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
        {
            if (info->cbSize != sizeof(SP_DEVINFO_DATA))
                SetLastError(ERROR_INVALID_USER_BUFFER);
            else
            {
                PLIST_ENTRY ItemList = list->ListHead.Flink;
                while (ItemList != &list->ListHead && index-- > 0)
                    ItemList = ItemList->Flink;
                if (ItemList == &list->ListHead)
                    SetLastError(ERROR_NO_MORE_ITEMS);
                else
                {
                    struct DeviceInfo *DevInfo = CONTAINING_RECORD(ItemList, struct DeviceInfo, ListEntry);
                    memcpy(&info->ClassGuid,
                        &DevInfo->ClassGuid,
                        sizeof(GUID));
                    info->DevInst = DevInfo->dnDevInst;
                    info->Reserved = (ULONG_PTR)DevInfo;
                    ret = TRUE;
                }
            }
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstanceIdA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        PSTR DeviceInstanceId,
        DWORD DeviceInstanceIdSize,
        PDWORD RequiredSize)
{
    BOOL ret = FALSE;
    DWORD size;
    PWSTR instanceId;

    TRACE("%s(%p %p %p %d %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, DeviceInstanceId,
            DeviceInstanceIdSize, RequiredSize);

    if (!DeviceInstanceId && DeviceInstanceIdSize > 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ret = SetupDiGetDeviceInstanceIdW(DeviceInfoSet,
                                DeviceInfoData,
                                NULL,
                                0,
                                &size);
    if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;
    instanceId = MyMalloc(size * sizeof(WCHAR));
    if (instanceId)
    {
        ret = SetupDiGetDeviceInstanceIdW(DeviceInfoSet,
                                          DeviceInfoData,
                                          instanceId,
                                          size,
                                          &size);
        if (ret)
        {
            int len = WideCharToMultiByte(CP_ACP, 0, instanceId, -1,
                                          DeviceInstanceId,
                                          DeviceInstanceIdSize, NULL, NULL);

            if (!len)
                ret = FALSE;
            else
            {
                if (len > DeviceInstanceIdSize)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    ret = FALSE;
                }
                if (RequiredSize)
                    *RequiredSize = len;
            }
        }
        MyFree(instanceId);
    }
    else
    {
        if (RequiredSize)
            *RequiredSize = size;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        ret = FALSE;
    }
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstanceIdW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        PWSTR DeviceInstanceId,
        DWORD DeviceInstanceIdSize,
        PDWORD RequiredSize)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%s(%p %p %p %d %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, DeviceInstanceId,
            DeviceInstanceIdSize, RequiredSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (!DeviceInstanceId && DeviceInstanceIdSize > 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (DeviceInstanceId && DeviceInstanceIdSize == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    TRACE("instance ID: %s\n", debugstr_w(devInfo->instanceId));
    if (DeviceInstanceIdSize < lstrlenW(devInfo->instanceId) + 1)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        if (RequiredSize)
            *RequiredSize = lstrlenW(devInfo->instanceId) + 1;
        return FALSE;
    }
    lstrcpyW(DeviceInstanceId, devInfo->instanceId);
    if (RequiredSize)
        *RequiredSize = lstrlenW(devInfo->instanceId) + 1;
    return TRUE;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallA(
        HINF InfHandle,
        PCSTR InfSectionName,
        PSTR InfSectionWithExt,
        DWORD InfSectionWithExtSize,
        PDWORD RequiredSize,
        PSTR *Extension)
{
    return SetupDiGetActualSectionToInstallExA(InfHandle, InfSectionName,
        NULL, InfSectionWithExt, InfSectionWithExtSize, RequiredSize,
        Extension, NULL);
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallW(
        HINF InfHandle,
        PCWSTR InfSectionName,
        PWSTR InfSectionWithExt,
        DWORD InfSectionWithExtSize,
        PDWORD RequiredSize,
        PWSTR *Extension)
{
    return SetupDiGetActualSectionToInstallExW(InfHandle, InfSectionName,
        NULL, InfSectionWithExt, InfSectionWithExtSize, RequiredSize,
        Extension, NULL);
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallExA  (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetActualSectionToInstallExA(
        IN HINF InfHandle,
        IN PCSTR InfSectionName,
        IN PSP_ALTPLATFORM_INFO AlternatePlatformInfo OPTIONAL,
        OUT PSTR InfSectionWithExt OPTIONAL,
        IN DWORD InfSectionWithExtSize,
        OUT PDWORD RequiredSize OPTIONAL,
        OUT PSTR* Extension OPTIONAL,
        IN PVOID Reserved)
{
    LPWSTR InfSectionNameW = NULL;
    LPWSTR InfSectionWithExtW = NULL;
    PWSTR ExtensionW;
    BOOL bResult = FALSE;

    TRACE("%s()\n", __FUNCTION__);

    if (InfSectionName)
    {
        InfSectionNameW = pSetupMultiByteToUnicode(InfSectionName, CP_ACP);
        if (InfSectionNameW == NULL)
            goto cleanup;
    }
    if (InfSectionWithExt)
    {
        InfSectionWithExtW = MyMalloc(InfSectionWithExtSize * sizeof(WCHAR));
        if (InfSectionWithExtW == NULL)
            goto cleanup;
    }

    bResult = SetupDiGetActualSectionToInstallExW(
        InfHandle, InfSectionNameW, AlternatePlatformInfo,
        InfSectionWithExt ? InfSectionWithExtW : NULL,
        InfSectionWithExtSize,
        RequiredSize,
        Extension ? &ExtensionW : NULL,
        Reserved);

    if (bResult && InfSectionWithExt)
    {
         bResult = WideCharToMultiByte(CP_ACP, 0, InfSectionWithExtW, -1, InfSectionWithExt,
             InfSectionWithExtSize, NULL, NULL) != 0;
    }
    if (bResult && Extension)
    {
        if (ExtensionW == NULL)
            *Extension = NULL;
         else
            *Extension = &InfSectionWithExt[ExtensionW - InfSectionWithExtW];
    }

cleanup:
    MyFree(InfSectionNameW);
    MyFree(InfSectionWithExtW);

    return bResult;
}

/***********************************************************************
 *		SetupDiGetClassDescriptionA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionA(
        const GUID* ClassGuid,
        PSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize)
{
    return SetupDiGetClassDescriptionExA(ClassGuid, ClassDescription,
                                         ClassDescriptionSize,
                                         RequiredSize, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDescriptionW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionW(
        const GUID* ClassGuid,
        PWSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize)
{
    return SetupDiGetClassDescriptionExW(ClassGuid, ClassDescription,
                                         ClassDescriptionSize,
                                         RequiredSize, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDescriptionExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionExA(
        const GUID* ClassGuid,
        PSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize,
        PCSTR MachineName,
        PVOID Reserved)
{
    PWCHAR ClassDescriptionW = NULL;
    LPWSTR MachineNameW = NULL;
    BOOL ret = FALSE;

    TRACE("%s(%s %p %lu %p %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), ClassDescription,
        ClassDescriptionSize, RequiredSize, debugstr_a(MachineName), Reserved);

    if (ClassDescriptionSize > 0)
    {
        ClassDescriptionW = MyMalloc(ClassDescriptionSize * sizeof(WCHAR));
        if (!ClassDescriptionW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }

    if (MachineName)
    {
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
        if (!MachineNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }

    ret = SetupDiGetClassDescriptionExW(ClassGuid, ClassDescriptionW,
        ClassDescriptionSize * sizeof(WCHAR), RequiredSize, MachineNameW, Reserved);
    if (ret)
    {
        DWORD len = (DWORD)WideCharToMultiByte(CP_ACP, 0, ClassDescriptionW, -1, ClassDescription,
            ClassDescriptionSize, NULL, NULL);
        if (len == 0 || len > ClassDescriptionSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
    }

cleanup:
    MyFree(ClassDescriptionW);
    MyFree(MachineNameW);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassDescriptionExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionExW(
        const GUID* ClassGuid,
        PWSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;
    DWORD dwRegType;
    LONG rc;
    PWSTR Buffer;

    TRACE("%s(%s %p %lu %p %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), ClassDescription,
        ClassDescriptionSize, RequiredSize, debugstr_w(MachineName), Reserved);

    /* Make sure there's a GUID */
    if (!ClassGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Make sure there's a real buffer when there's a size */
    if (!ClassDescription && ClassDescriptionSize > 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Open the key for the GUID */
    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     KEY_QUERY_VALUE,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
        return FALSE;

    /* Retrieve the class description data and close the key */
    rc = QueryRegistryValue(hKey, NULL, (LPBYTE *) &Buffer, &dwRegType, &dwLength);
    RegCloseKey(hKey);

    /* Make sure we got the data */
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        return FALSE;
    }

    /* Make sure the data is a string */
    if (dwRegType != REG_SZ)
    {
        MyFree(Buffer);
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }

    /* Determine the length of the class description */
    dwLength /= sizeof(WCHAR);

    /* Count the null-terminator if none is present */
    if ((dwLength == 0) || (Buffer[dwLength - 1] != UNICODE_NULL))
        dwLength++;

    /* Inform the caller about the class description */
    if ((ClassDescription != NULL) && (dwLength <= ClassDescriptionSize))
    {
        memcpy(ClassDescription, Buffer, (dwLength - 1) * sizeof(WCHAR));
        ClassDescription[dwLength - 1] = UNICODE_NULL;
    }

    /* Inform the caller about the required size */
    if (RequiredSize != NULL)
        *RequiredSize = dwLength;

    /* Clean up the buffer */
    MyFree(Buffer);

    /* Make sure the buffer was large enough */
    if ((ClassDescription == NULL) || (dwLength > ClassDescriptionSize))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiGetClassDevsA (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsA(
        CONST GUID *class,
        LPCSTR enumstr,
        HWND parent,
        DWORD flags)
{
    return SetupDiGetClassDevsExA(class, enumstr, parent,
                                  flags, NULL, NULL, NULL);
}

/***********************************************************************
 *		  SetupDiGetClassDevsExA (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExA(
        const GUID *class,
        PCSTR enumstr,
        HWND parent,
        DWORD flags,
        HDEVINFO deviceset,
        PCSTR machine,
        PVOID reserved)
{
    HDEVINFO ret;
    LPWSTR enumstrW = NULL, machineW = NULL;

    if (enumstr)
    {
        enumstrW = pSetupMultiByteToUnicode(enumstr, CP_ACP);
        if (!enumstrW)
        {
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
    }
    if (machine)
    {
        machineW = pSetupMultiByteToUnicode(machine, CP_ACP);
        if (!machineW)
        {
            MyFree(enumstrW);
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
    }
    ret = SetupDiGetClassDevsExW(class, enumstrW, parent, flags, deviceset,
            machineW, reserved);
    MyFree(enumstrW);
    MyFree(machineW);

end:
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassDevsW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsW(
        CONST GUID *class,
        LPCWSTR enumstr,
        HWND parent,
        DWORD flags)
{
    return SetupDiGetClassDevsExW(class, enumstr, parent, flags, NULL, NULL,
            NULL);
}

/***********************************************************************
 *              SetupDiGetClassDevsExW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExW(
        CONST GUID *class,
        PCWSTR enumstr,
        HWND parent,
        DWORD flags,
        HDEVINFO deviceset,
        PCWSTR machine,
        PVOID reserved)
{
    HDEVINFO hDeviceInfo = INVALID_HANDLE_VALUE;
    struct DeviceInfoSet *list;
    CONST GUID *pClassGuid;
    LONG rc;
    HDEVINFO set = INVALID_HANDLE_VALUE;

    TRACE("%s(%s %s %p 0x%08x %p %s %p)\n", __FUNCTION__, debugstr_guid(class),
            debugstr_w(enumstr), parent, flags, deviceset, debugstr_w(machine),
            reserved);

    if (!(flags & DIGCF_ALLCLASSES) && !class)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    /* Create the deviceset if not set */
    if (deviceset)
    {
        list = (struct DeviceInfoSet *)deviceset;
        if (list->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            goto cleanup;
        }
        hDeviceInfo = deviceset;
    }
    else
    {
         hDeviceInfo = SetupDiCreateDeviceInfoListExW(
             flags & (DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES) ? NULL : class,
             NULL, machine, NULL);
         if (hDeviceInfo == INVALID_HANDLE_VALUE)
             goto cleanup;
         list = (struct DeviceInfoSet *)hDeviceInfo;
    }

    if (flags & DIGCF_PROFILE)
        FIXME(": flag DIGCF_PROFILE ignored\n");

    if (flags & DIGCF_DEVICEINTERFACE)
    {
        if (!class)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto cleanup;
        }
        rc = SETUP_CreateInterfaceList(list, machine, class, enumstr, flags & DIGCF_PRESENT);
    }
    else
    {
        /* Determine which class(es) should be included in the deviceset */
        if (flags & DIGCF_ALLCLASSES)
        {
            /* The caller wants all classes. Check if
             * the deviceset limits us to one class */
            if (IsEqualIID(&list->ClassGuid, &GUID_NULL))
                pClassGuid = NULL;
            else
                pClassGuid = &list->ClassGuid;
        }
        else if (class)
        {
            /* The caller wants one class. Check if it matches deviceset class */
            if (IsEqualIID(&list->ClassGuid, class)
             || IsEqualIID(&list->ClassGuid, &GUID_NULL))
            {
                pClassGuid = class;
            }
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto cleanup;
            }
        }
        else if (!IsEqualIID(&list->ClassGuid, &GUID_NULL))
        {
            /* No class specified. Try to use the one of the deviceset */
            if (IsEqualIID(&list->ClassGuid, &GUID_NULL))
                pClassGuid = &list->ClassGuid;
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto cleanup;
            }
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto cleanup;
        }
        rc = SETUP_CreateDevicesList(list, machine, pClassGuid, enumstr);
    }
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    set = hDeviceInfo;

cleanup:
    if (!deviceset && hDeviceInfo != INVALID_HANDLE_VALUE && hDeviceInfo != set)
        SetupDiDestroyDeviceInfoList(hDeviceInfo);
    return set;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_LIST_DETAIL_DATA_A DevInfoData )
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DevInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DevInfoData ||
            DevInfoData->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA_A))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy(&DevInfoData->ClassGuid, &set->ClassGuid, sizeof(GUID));
    DevInfoData->RemoteMachineHandle = set->hMachine;
    if (set->MachineName)
    {
        FIXME("Stub\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    else
        DevInfoData->RemoteMachineName[0] = 0;

    return TRUE;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_LIST_DETAIL_DATA_W DevInfoData )
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DevInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DevInfoData ||
            DevInfoData->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA_W))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy(&DevInfoData->ClassGuid, &set->ClassGuid, sizeof(GUID));
    DevInfoData->RemoteMachineHandle = set->hMachine;
    if (set->MachineName)
        strcpyW(DevInfoData->RemoteMachineName, set->MachineName + 2);
    else
        DevInfoData->RemoteMachineName[0] = 0;

    return TRUE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInterfaceA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        const GUID *InterfaceClassGuid,
        PCSTR ReferenceString,
        DWORD CreationFlags,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    BOOL ret;
    LPWSTR ReferenceStringW = NULL;

    TRACE("%s(%p %p %s %s %08x %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData,
            debugstr_guid(InterfaceClassGuid), debugstr_a(ReferenceString),
            CreationFlags, DeviceInterfaceData);

    if (ReferenceString)
    {
        ReferenceStringW = pSetupMultiByteToUnicode(ReferenceString, CP_ACP);
        if (ReferenceStringW == NULL) return FALSE;
    }

    ret = SetupDiCreateDeviceInterfaceW(DeviceInfoSet, DeviceInfoData,
            InterfaceClassGuid, ReferenceStringW, CreationFlags,
            DeviceInterfaceData);

    MyFree(ReferenceStringW);

    return ret;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInterfaceW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        const GUID *InterfaceClassGuid,
        PCWSTR ReferenceString,
        DWORD CreationFlags,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    TRACE("%s(%p %p %s %s %08x %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData,
            debugstr_guid(InterfaceClassGuid), debugstr_w(ReferenceString),
            CreationFlags, DeviceInterfaceData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!InterfaceClassGuid)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    FIXME("%p %p %s %s %08x %p\n", DeviceInfoSet, DeviceInfoData,
            debugstr_guid(InterfaceClassGuid), debugstr_w(ReferenceString),
            CreationFlags, DeviceInterfaceData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceRegKeyA (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDeviceInterfaceRegKeyA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        DWORD Reserved,
        REGSAM samDesired,
        HINF InfHandle,
        PCSTR InfSectionName)
{
    HKEY key;
    PWSTR InfSectionNameW = NULL;

    TRACE("%s(%p %p %d %08x %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInterfaceData, Reserved,
            samDesired, InfHandle, InfSectionName);
    if (InfHandle)
    {
        if (!InfSectionName)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        InfSectionNameW = pSetupMultiByteToUnicode(InfSectionName, CP_ACP);
        if (!InfSectionNameW)
            return INVALID_HANDLE_VALUE;
    }
    key = SetupDiCreateDeviceInterfaceRegKeyW(DeviceInfoSet,
            DeviceInterfaceData, Reserved, samDesired, InfHandle,
            InfSectionNameW);
    MyFree(InfSectionNameW);
    return key;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceRegKeyW (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDeviceInterfaceRegKeyW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        DWORD Reserved,
        REGSAM samDesired,
        HINF InfHandle,
        PCWSTR InfSectionName)
{
    HKEY hKey, hDevKey;
    LPWSTR SymbolicLink;
    DWORD Length, Index;
    LONG rc;
    WCHAR bracedGuidString[39];
    struct DeviceInterface *DevItf;
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;

    TRACE("%s(%p %p %d %08x %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInterfaceData, Reserved,
            samDesired, InfHandle, InfSectionName);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (InfHandle && !InfSectionName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    hKey = SetupDiOpenClassRegKeyExW(&DeviceInterfaceData->InterfaceClassGuid, samDesired, DIOCR_INTERFACE, NULL, NULL);
    if (hKey == INVALID_HANDLE_VALUE)
    {
        hKey = SetupDiOpenClassRegKeyExW(NULL, samDesired, DIOCR_INTERFACE, NULL, NULL);
        if (hKey == INVALID_HANDLE_VALUE)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        SETUPDI_GuidToString(&DeviceInterfaceData->InterfaceClassGuid, bracedGuidString);

        if (RegCreateKeyExW(hKey, bracedGuidString, 0, NULL, 0, samDesired, NULL, &hDevKey, NULL) != ERROR_SUCCESS)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        RegCloseKey(hKey);
        hKey = hDevKey;
    }

    DevItf = (struct DeviceInterface *)DeviceInterfaceData->Reserved;

    Length = (wcslen(DevItf->SymbolicLink)+1) * sizeof(WCHAR);
    SymbolicLink = HeapAlloc(GetProcessHeap(), 0, Length);
    if (!SymbolicLink)
    {
        RegCloseKey(hKey);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    wcscpy(SymbolicLink, DevItf->SymbolicLink);

    Index = 0;
    while(SymbolicLink[Index])
    {
        if (SymbolicLink[Index] == L'\\')
        {
            SymbolicLink[Index] = L'#';
        }
        Index++;
    }

    rc = RegCreateKeyExW(hKey, SymbolicLink, 0, NULL, 0, samDesired, NULL, &hDevKey, NULL);

    RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, SymbolicLink);

    if (rc == ERROR_SUCCESS)
    {
        if (InfHandle && InfSectionName)
        {
            if (!SetupInstallFromInfSection(NULL /*FIXME */,
                                            InfHandle,
                                            InfSectionName,
                                            SPINST_INIFILES | SPINST_REGISTRY | SPINST_INI2REG | SPINST_FILES | SPINST_BITREG | SPINST_REGSVR | SPINST_UNREGSVR | SPINST_PROFILEITEMS | SPINST_COPYINF,
                                            hDevKey,
                                            NULL,
                                            0,
                                            set->SelectedDevice->InstallParams.InstallMsgHandler,
                                            set->SelectedDevice->InstallParams.InstallMsgHandlerContext,
                                            INVALID_HANDLE_VALUE,
                                            NULL))
            {
                RegCloseKey(hDevKey);
                return INVALID_HANDLE_VALUE;
            }
        }
    }

    SetLastError(rc);
    return hDevKey;
}

/***********************************************************************
 *		SetupDiDeleteDeviceInterfaceRegKey (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDeviceInterfaceRegKey(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        DWORD Reserved)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    BOOL ret = FALSE;

    TRACE("%s(%p %p %d)\n", __FUNCTION__, DeviceInfoSet, DeviceInterfaceData, Reserved);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    FIXME("%p %p %d\n", DeviceInfoSet, DeviceInterfaceData, Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ret;
}

/***********************************************************************
 *		SetupDiEnumDeviceInterfaces (SETUPAPI.@)
 *
 * PARAMS
 *   DeviceInfoSet      [I]    Set of devices from which to enumerate
 *                             interfaces
 *   DeviceInfoData     [I]    (Optional) If specified, a specific device
 *                             instance from which to enumerate interfaces.
 *                             If it isn't specified, all interfaces for all
 *                             devices in the set are enumerated.
 *   InterfaceClassGuid [I]    The interface class to enumerate.
 *   MemberIndex        [I]    An index of the interface instance to enumerate.
 *                             A caller should start with MemberIndex set to 0,
 *                             and continue until the function fails with
 *                             ERROR_NO_MORE_ITEMS.
 *   DeviceInterfaceData [I/O] Returns an enumerated interface.  Its cbSize
 *                             member must be set to
 *                             sizeof(SP_DEVICE_INTERFACE_DATA).
 *
 * RETURNS
 *   Success: non-zero value.
 *   Failure: FALSE.  Call GetLastError() for more info.
 */
BOOL WINAPI SetupDiEnumDeviceInterfaces(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        CONST GUID * InterfaceClassGuid,
        DWORD MemberIndex,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    BOOL ret = FALSE;

    TRACE("%s(%p, %p, %s, %d, %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData,
     debugstr_guid(InterfaceClassGuid), MemberIndex, DeviceInterfaceData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (DeviceInfoData && (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA) ||
                !DeviceInfoData->Reserved))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (DeviceInfoData)
    {
        struct DeviceInfo *devInfo =
            (struct DeviceInfo *)DeviceInfoData->Reserved;
        BOOL found = FALSE;
        PLIST_ENTRY InterfaceListEntry = devInfo->InterfaceListHead.Flink;
        while (InterfaceListEntry != &devInfo->InterfaceListHead && !found)
        {
            struct DeviceInterface *DevItf = CONTAINING_RECORD(InterfaceListEntry, struct DeviceInterface, ListEntry);
            if (!IsEqualIID(&DevItf->InterfaceClassGuid, InterfaceClassGuid))
            {
                InterfaceListEntry = InterfaceListEntry->Flink;
                continue;
            }
            if (MemberIndex-- == 0)
            {
                /* return this item */
                memcpy(&DeviceInterfaceData->InterfaceClassGuid,
                    &DevItf->InterfaceClassGuid,
                    sizeof(GUID));
                DeviceInterfaceData->Flags = DevItf->Flags;
                DeviceInterfaceData->Reserved = (ULONG_PTR)DevItf;
                found = TRUE;
                ret = TRUE;
            }
            InterfaceListEntry = InterfaceListEntry->Flink;
        }
        if (!found)
            SetLastError(ERROR_NO_MORE_ITEMS);
    }
    else
    {
        BOOL found = FALSE;
        PLIST_ENTRY ItemList = set->ListHead.Flink;
        while (ItemList != &set->ListHead && !found)
        {
            PLIST_ENTRY InterfaceListEntry;
            struct DeviceInfo *devInfo =
                CONTAINING_RECORD(ItemList, struct DeviceInfo, ListEntry);
            InterfaceListEntry = devInfo->InterfaceListHead.Flink;
            while (InterfaceListEntry != &devInfo->InterfaceListHead && !found)
            {
                struct DeviceInterface *DevItf = CONTAINING_RECORD(InterfaceListEntry, struct DeviceInterface, ListEntry);
                if (!IsEqualIID(&DevItf->InterfaceClassGuid, InterfaceClassGuid))
                {
                    InterfaceListEntry = InterfaceListEntry->Flink;
                    continue;
                }
                if (MemberIndex-- == 0)
                {
                    /* return this item */
                    memcpy(&DeviceInterfaceData->InterfaceClassGuid,
                        &DevItf->InterfaceClassGuid,
                        sizeof(GUID));
                    DeviceInterfaceData->Flags = DevItf->Flags;
                    DeviceInterfaceData->Reserved = (ULONG_PTR)DevItf;
                    found = TRUE;
                    ret = TRUE;
                }
                InterfaceListEntry = InterfaceListEntry->Flink;
            }
            ItemList = ItemList->Flink;

        }
        if (!found)
            SetLastError(ERROR_NO_MORE_ITEMS);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiDestroyDeviceInfoList (SETUPAPI.@)
  *
 * Destroy a DeviceInfoList and free all used memory of the list.
 *
 * PARAMS
 *   devinfo [I] DeviceInfoList pointer to list to destroy
 *
 * RETURNS
 *   Success: non zero value.
 *   Failure: zero value.
 */
BOOL WINAPI SetupDiDestroyDeviceInfoList(HDEVINFO devinfo)
{
    BOOL ret = FALSE;

    TRACE("%s(%p)\n", __FUNCTION__, devinfo);
    if (devinfo && devinfo != INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)devinfo;

        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
        {
            ret = DestroyDeviceInfoSet(list);
        }
    }

    if (ret == FALSE)
        SetLastError(ERROR_INVALID_HANDLE);

    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
        DWORD DeviceInterfaceDetailDataSize,
        PDWORD RequiredSize,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailDataW = NULL;
    DWORD sizeW = 0, bytesNeeded;
    BOOL ret = FALSE;

    TRACE("%s(%p, %p, %p, %d, %p, %p)\n", __FUNCTION__, DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (DeviceInterfaceDetailData && (DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A)))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if((DeviceInterfaceDetailDataSize != 0) &&
        (DeviceInterfaceDetailDataSize < (FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath) + sizeof(CHAR))))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }


    if (DeviceInterfaceDetailData != NULL)
    {
        sizeW = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)
            + (DeviceInterfaceDetailDataSize - FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath)) * sizeof(WCHAR);
        DeviceInterfaceDetailDataW = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)MyMalloc(sizeW);
        if (!DeviceInterfaceDetailDataW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        DeviceInterfaceDetailDataW->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    }
    if (!DeviceInterfaceDetailData || (DeviceInterfaceDetailData && DeviceInterfaceDetailDataW))
    {
        ret = SetupDiGetDeviceInterfaceDetailW(
            DeviceInfoSet,
            DeviceInterfaceData,
            DeviceInterfaceDetailDataW,
            sizeW,
            &sizeW,
            DeviceInfoData);
        bytesNeeded = (sizeW - FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)) / sizeof(WCHAR)
            + FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath);
        if (RequiredSize)
            *RequiredSize = bytesNeeded;
        if (ret && DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize >= bytesNeeded)
        {
            if (!WideCharToMultiByte(
                CP_ACP, 0,
                DeviceInterfaceDetailDataW->DevicePath, -1,
                DeviceInterfaceDetailData->DevicePath, DeviceInterfaceDetailDataSize - FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath),
                NULL, NULL))
            {
                ret = FALSE;
            }
        }
    }
    MyFree(DeviceInterfaceDetailDataW);

    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
        DWORD DeviceInterfaceDetailDataSize,
        PDWORD RequiredSize,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    BOOL ret = FALSE;

    TRACE("%s(%p, %p, %p, %d, %p, %p)\n", __FUNCTION__, DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (DeviceInterfaceDetailData && DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    if (!DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((DeviceInterfaceDetailData != NULL)
        && (DeviceInterfaceDetailDataSize < (FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)) + sizeof(WCHAR)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    else
    {
        struct DeviceInterface *deviceInterface = (struct DeviceInterface *)DeviceInterfaceData->Reserved;
        LPCWSTR devName = deviceInterface->SymbolicLink;
        DWORD sizeRequired = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) +
            (lstrlenW(devName) + 1) * sizeof(WCHAR);

        if (sizeRequired > DeviceInterfaceDetailDataSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            if (RequiredSize)
                *RequiredSize = sizeRequired;
        }
        else
        {
            strcpyW(DeviceInterfaceDetailData->DevicePath, devName);
            TRACE("DevicePath is %s\n", debugstr_w(DeviceInterfaceDetailData->DevicePath));
            if (DeviceInfoData)
            {
                memcpy(&DeviceInfoData->ClassGuid,
                    &deviceInterface->DeviceInfo->ClassGuid,
                    sizeof(GUID));
                DeviceInfoData->DevInst = deviceInterface->DeviceInfo->dnDevInst;
                DeviceInfoData->Reserved = (ULONG_PTR)deviceInterface->DeviceInfo;
            }
            ret = TRUE;
        }
    }
    return ret;
}

struct PropertyMapEntry
{
    DWORD   regType;
    LPCSTR  nameA;
    LPCWSTR nameW;
};

static struct PropertyMapEntry PropertyMap[] = {
    { REG_SZ, "DeviceDesc", REGSTR_VAL_DEVDESC },
    { REG_MULTI_SZ, "HardwareId", REGSTR_VAL_HARDWAREID },
    { REG_MULTI_SZ, "CompatibleIDs", REGSTR_VAL_COMPATIBLEIDS },
    { 0, NULL, NULL }, /* SPDRP_UNUSED0 */
    { REG_SZ, "Service", REGSTR_VAL_SERVICE },
    { 0, NULL, NULL }, /* SPDRP_UNUSED1 */
    { 0, NULL, NULL }, /* SPDRP_UNUSED2 */
    { REG_SZ, "Class", REGSTR_VAL_CLASS },
    { REG_SZ, "ClassGUID", REGSTR_VAL_CLASSGUID },
    { REG_SZ, "Driver", REGSTR_VAL_DRIVER },
    { REG_DWORD, "ConfigFlags", REGSTR_VAL_CONFIGFLAGS },
    { REG_SZ, "Mfg", REGSTR_VAL_MFG },
    { REG_SZ, "FriendlyName", REGSTR_VAL_FRIENDLYNAME },
    { REG_SZ, "LocationInformation", REGSTR_VAL_LOCATION_INFORMATION },
    { 0, NULL, NULL }, /* SPDRP_PHYSICAL_DEVICE_OBJECT_NAME */
    { REG_DWORD, "Capabilities", REGSTR_VAL_CAPABILITIES },
    { REG_DWORD, "UINumber", REGSTR_VAL_UI_NUMBER },
    { REG_MULTI_SZ, "UpperFilters", REGSTR_VAL_UPPERFILTERS },
    { REG_MULTI_SZ, "LowerFilters", REGSTR_VAL_LOWERFILTERS },
    { 0, NULL, NULL }, /* SPDRP_BUSTYPEGUID */
    { 0, NULL, NULL }, /* SPDRP_LEGACYBUSTYPE */
    { 0, NULL, NULL }, /* SPDRP_BUSNUMBER */
    { 0, NULL, NULL }, /* SPDRP_ENUMERATOR_NAME */
    { REG_BINARY, "Security", REGSTR_SECURITY },
    { 0, NULL, NULL }, /* SPDRP_SECURITY_SDS */
    { 0, NULL, NULL }, /* SPDRP_DEVTYPE */
    { 0, NULL, NULL }, /* SPDRP_EXCLUSIVE */
    { 0, NULL, NULL }, /* SPDRP_CHARACTERISTICS */
    { 0, NULL, NULL }, /* SPDRP_ADDRESS */
    { REG_SZ, "UINumberDescFormat", REGSTR_UI_NUMBER_DESC_FORMAT },
    { 0, NULL, NULL }, /* SPDRP_DEVICE_POWER_DATA */
    { 0, NULL, NULL }, /* SPDRP_REMOVAL_POLICY */
    { 0, NULL, NULL }, /* SPDRP_REMOVAL_POLICY_HW_DEFAULT */
    { 0, NULL, NULL }, /* SPDRP_REMOVAL_POLICY_OVERRIDE */
    { 0, NULL, NULL }, /* SPDRP_INSTALL_STATE */
};

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyA(
        HDEVINFO  DeviceInfoSet,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    BOOL ret;
    BOOL bIsStringProperty;
    DWORD RegType;
    DWORD RequiredSizeA, RequiredSizeW;
    DWORD PropertyBufferSizeW = 0;
    PBYTE PropertyBufferW = NULL;

    TRACE("%s(%p %p %d %p %p %d %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);

    if (PropertyBufferSize != 0)
    {
        PropertyBufferSizeW = PropertyBufferSize * 2;
        PropertyBufferW = HeapAlloc(GetProcessHeap(), 0, PropertyBufferSizeW);
        if (!PropertyBufferW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    ret = SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
                                            DeviceInfoData,
                                            Property,
                                            &RegType,
                                            PropertyBufferW,
                                            PropertyBufferSizeW,
                                            &RequiredSizeW);

    if (ret || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        bIsStringProperty = (RegType == REG_SZ || RegType == REG_MULTI_SZ || RegType == REG_EXPAND_SZ);

        if (bIsStringProperty)
           RequiredSizeA = RequiredSizeW / sizeof(WCHAR);
        else
            RequiredSizeA = RequiredSizeW;
        if (RequiredSize)
            *RequiredSize = RequiredSizeA;
        if (PropertyRegDataType)
            *PropertyRegDataType = RegType;
    }

    if (!ret)
    {
        HeapFree(GetProcessHeap(), 0, PropertyBufferW);
        return ret;
    }

    if (RequiredSizeA <= PropertyBufferSize)
    {
        if (bIsStringProperty && PropertyBufferSize > 0)
        {
            if (WideCharToMultiByte(CP_ACP, 0, (LPWSTR)PropertyBufferW, RequiredSizeW / sizeof(WCHAR), (LPSTR)PropertyBuffer, PropertyBufferSize, NULL, NULL) == 0)
            {
                /* Last error is already set by WideCharToMultiByte */
                ret = FALSE;
            }
        }
        else
            memcpy(PropertyBuffer, PropertyBufferW, RequiredSizeA);
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        ret = FALSE;
    }

    HeapFree(GetProcessHeap(), 0, PropertyBufferW);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyW(
        HDEVINFO  DeviceInfoSet,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;
    CONFIGRET cr;
    LONG lError = ERROR_SUCCESS;
    DWORD size;

    TRACE("%s(%p %p %d %p %p %d %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Property >= SPDRP_MAXIMUM_PROPERTY)
    {
        SetLastError(ERROR_INVALID_REG_PROPERTY);
        return FALSE;
    }

    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameW)
    {
        HKEY hKey;
        size = PropertyBufferSize;
        hKey = SETUPDI_OpenDevKey(set->HKLM, devInfo, KEY_QUERY_VALUE);
        if (hKey == INVALID_HANDLE_VALUE)
            return FALSE;
        lError = RegQueryValueExW(hKey, PropertyMap[Property].nameW,
                 NULL, PropertyRegDataType, PropertyBuffer, &size);
        RegCloseKey(hKey);

        if (RequiredSize)
            *RequiredSize = size;

        switch (lError)
        {
            case ERROR_SUCCESS:
                if (PropertyBuffer == NULL && size != 0)
                    lError = ERROR_INSUFFICIENT_BUFFER;
                break;
            case ERROR_MORE_DATA:
                lError = ERROR_INSUFFICIENT_BUFFER;
                break;
            default:
                break;
        }
    }
    else if (Property == SPDRP_PHYSICAL_DEVICE_OBJECT_NAME)
    {
        size = (strlenW(devInfo->Data) + 1) * sizeof(WCHAR);

        if (PropertyRegDataType)
            *PropertyRegDataType = REG_SZ;
        if (RequiredSize)
            *RequiredSize = size;
        if (PropertyBufferSize >= size)
        {
            strcpyW((LPWSTR)PropertyBuffer, devInfo->Data);
        }
        else
            lError = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        size = PropertyBufferSize;

        cr = CM_Get_DevNode_Registry_Property_ExW(devInfo->dnDevInst,
                                                  Property + (CM_DRP_DEVICEDESC - SPDRP_DEVICEDESC),
                                                  PropertyRegDataType,
                                                  PropertyBuffer,
                                                  &size,
                                                  0,
                                                  set->hMachine);
        if ((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL))
        {
            if (RequiredSize)
                *RequiredSize = size;
        }

        if (cr != CR_SUCCESS)
        {
            switch (cr)
            {
                case CR_INVALID_DEVINST:
                    lError = ERROR_NO_SUCH_DEVINST;
                    break;

                case CR_INVALID_PROPERTY:
                    lError = ERROR_INVALID_REG_PROPERTY;
                    break;

                case CR_BUFFER_SMALL:
                    lError = ERROR_INSUFFICIENT_BUFFER;
                    break;

                default :
                    lError = ERROR_INVALID_DATA;
                    break;
            }
        }
    }

    SetLastError(lError);
    return (lError == ERROR_SUCCESS);
}

/***********************************************************************
 *		Internal for SetupDiSetDeviceRegistryPropertyA/W
 */
BOOL WINAPI IntSetupDiSetDeviceRegistryPropertyAW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Property,
        const BYTE *PropertyBuffer,
        DWORD PropertyBufferSize,
        BOOL isAnsi)
{
    BOOL ret = FALSE;
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo;

    TRACE("%s(%p %p %d %p %d)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, Property,
        PropertyBuffer, PropertyBufferSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameW
        && PropertyMap[Property].nameA)
    {
        HKEY hKey;
        LONG l;
        hKey = SETUPDI_OpenDevKey(set->HKLM, deviceInfo, KEY_SET_VALUE);
        if (hKey == INVALID_HANDLE_VALUE)
            return FALSE;
        /* Write new data */
        if (isAnsi)
        {
            l = RegSetValueExA(
                hKey, PropertyMap[Property].nameA, 0,
                    PropertyMap[Property].regType, PropertyBuffer,
                    PropertyBufferSize);
        }
        else
        {
            l = RegSetValueExW(
                hKey, PropertyMap[Property].nameW, 0,
                    PropertyMap[Property].regType, PropertyBuffer,
                    PropertyBufferSize);
        }
        if (!l)
            ret = TRUE;
        else
            SetLastError(l);
        RegCloseKey(hKey);
    }
    else
    {
        ERR("Property 0x%lx not implemented\n", Property);
        SetLastError(ERROR_NOT_SUPPORTED);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}
/***********************************************************************
 *		SetupDiSetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceRegistryPropertyA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Property,
        const BYTE *PropertyBuffer,
        DWORD PropertyBufferSize)
{
    return IntSetupDiSetDeviceRegistryPropertyAW(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 Property,
                                                 PropertyBuffer,
                                                 PropertyBufferSize,
                                                 TRUE);
}

/***********************************************************************
 *		SetupDiSetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceRegistryPropertyW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Property,
        const BYTE *PropertyBuffer,
        DWORD PropertyBufferSize)
{
    return IntSetupDiSetDeviceRegistryPropertyAW(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 Property,
                                                 PropertyBuffer,
                                                 PropertyBufferSize,
                                                 FALSE);
}

/***********************************************************************
 *		SetupDiInstallClassA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallClassA(
        HWND hwndParent,
        PCSTR InfFileName,
        DWORD Flags,
        HSPFILEQ FileQueue)
{
    return SetupDiInstallClassExA(hwndParent, InfFileName, Flags, FileQueue, NULL, NULL, NULL);
}

/***********************************************************************
 *		SetupDiInstallClassExA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiInstallClassExA(
    IN HWND hwndParent OPTIONAL,
    IN PCSTR InfFileName OPTIONAL,
    IN DWORD Flags,
    IN HSPFILEQ FileQueue OPTIONAL,
    IN CONST GUID *InterfaceClassGuid OPTIONAL,
    IN PVOID Reserved1,
    IN PVOID Reserved2)
{
    PWSTR InfFileNameW = NULL;
    BOOL Result;

    if (!InfFileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    else
    {
        InfFileNameW = pSetupMultiByteToUnicode(InfFileName, CP_ACP);
        if (InfFileNameW == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    Result = SetupDiInstallClassExW(hwndParent, InfFileNameW, Flags,
        FileQueue, InterfaceClassGuid, Reserved1, Reserved2);

    MyFree(InfFileNameW);

    return Result;
}

HKEY SETUP_CreateClassKey(HINF hInf)
{
    WCHAR FullBuffer[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    DWORD RequiredSize;
    HKEY hClassKey;
    DWORD Disposition;

    /* Obtain the Class GUID for this class */
    if (!SetupGetLineTextW(NULL,
                           hInf,
                           Version,
                           REGSTR_VAL_CLASSGUID,
                           Buffer,
                           sizeof(Buffer) / sizeof(WCHAR),
                           &RequiredSize))
    {
        return INVALID_HANDLE_VALUE;
    }

    /* Build the corresponding registry key name */
    lstrcpyW(FullBuffer, REGSTR_PATH_CLASS_NT);
    lstrcatW(FullBuffer, BackSlash);
    lstrcatW(FullBuffer, Buffer);

    /* Obtain the Class name for this class */
    if (!SetupGetLineTextW(NULL,
                           hInf,
                           Version,
                           REGSTR_VAL_CLASS,
                           Buffer,
                           sizeof(Buffer) / sizeof(WCHAR),
                           &RequiredSize))
    {
        return INVALID_HANDLE_VALUE;
    }

    /* Try to open or create the registry key */
    TRACE("Opening class key %s\n", debugstr_w(FullBuffer));
#if 0 // I keep this for reference...
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      FullBuffer,
                      0,
                      KEY_SET_VALUE,
                      &hClassKey))
    {
        /* Use RegCreateKeyExW */
    }
#endif
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        FullBuffer,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_SET_VALUE,
                        NULL,
                        &hClassKey,
                        &Disposition))
    {
        ERR("RegCreateKeyExW(%s) failed\n", debugstr_w(FullBuffer));
        return INVALID_HANDLE_VALUE;
    }
    if (Disposition == REG_CREATED_NEW_KEY)
        TRACE("The class key %s was successfully created\n", debugstr_w(FullBuffer));
    else
        TRACE("The class key %s was successfully opened\n", debugstr_w(FullBuffer));

    TRACE( "setting value %s to %s\n", debugstr_w(REGSTR_VAL_CLASS), debugstr_w(Buffer) );
    if (RegSetValueExW(hClassKey,
                       REGSTR_VAL_CLASS,
                       0,
                       REG_SZ,
                       (LPBYTE)Buffer,
                       RequiredSize * sizeof(WCHAR)))
    {
        RegCloseKey(hClassKey);
        RegDeleteKeyW(HKEY_LOCAL_MACHINE,
                      FullBuffer);
        return INVALID_HANDLE_VALUE;
    }

    return hClassKey;
}

/***********************************************************************
 *		SetupDiInstallClassW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallClassW(
        HWND hwndParent,
        PCWSTR InfFileName,
        DWORD Flags,
        HSPFILEQ FileQueue)
{
    return SetupDiInstallClassExW(hwndParent, InfFileName, Flags, FileQueue, NULL, NULL, NULL);
}


/***********************************************************************
 *		SetupDiOpenClassRegKey  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKey(
        const GUID* ClassGuid,
        REGSAM samDesired)
{
    return SetupDiOpenClassRegKeyExW(ClassGuid, samDesired,
                                     DIOCR_INSTALLER, NULL, NULL);
}


/***********************************************************************
 *		SetupDiOpenClassRegKeyExA  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKeyExA(
        const GUID* ClassGuid,
        REGSAM samDesired,
        DWORD Flags,
        PCSTR MachineName,
        PVOID Reserved)
{
    PWSTR MachineNameW = NULL;
    HKEY hKey;

    TRACE("%s(%s 0x%lx 0x%lx %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), samDesired,
        Flags, debugstr_a(MachineName), Reserved);

    if (MachineName)
    {
        MachineNameW = pSetupMultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return INVALID_HANDLE_VALUE;
    }

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid, samDesired,
                                     Flags, MachineNameW, Reserved);

    MyFree(MachineNameW);

    return hKey;
}


/***********************************************************************
 *		SetupDiOpenClassRegKeyExW  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKeyExW(
        const GUID* ClassGuid,
        REGSAM samDesired,
        DWORD Flags,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY HKLM;
    HKEY hClassesKey;
    HKEY key;
    LPCWSTR lpKeyName;
    LONG l;

    TRACE("%s(%s 0x%lx 0x%lx %s %p)\n", __FUNCTION__, debugstr_guid(ClassGuid), samDesired,
        Flags, debugstr_w(MachineName), Reserved);

    if (MachineName != NULL)
    {
        l = RegConnectRegistryW(MachineName, HKEY_LOCAL_MACHINE, &HKLM);
        if (l != ERROR_SUCCESS)
        {
            SetLastError(l);
            return INVALID_HANDLE_VALUE;
        }
    }
    else
        HKLM = HKEY_LOCAL_MACHINE;

    if (Flags == DIOCR_INSTALLER)
    {
        lpKeyName = REGSTR_PATH_CLASS_NT;
    }
    else if (Flags == DIOCR_INTERFACE)
    {
        lpKeyName = REGSTR_PATH_DEVICE_CLASSES;
    }
    else
    {
        ERR("Invalid Flags parameter!\n");
        SetLastError(ERROR_INVALID_FLAGS);
        if (MachineName != NULL) RegCloseKey(HKLM);
        return INVALID_HANDLE_VALUE;
    }

    if (!ClassGuid)
    {
        if ((l = RegOpenKeyExW(HKLM,
                               lpKeyName,
                               0,
                               samDesired,
                               &hClassesKey)))
        {
            SetLastError(ERROR_INVALID_CLASS);
            hClassesKey = INVALID_HANDLE_VALUE;
        }
        if (MachineName != NULL)
            RegCloseKey(HKLM);
        key = hClassesKey;
    }
    else
    {
        WCHAR bracedGuidString[39];

        SETUPDI_GuidToString(ClassGuid, bracedGuidString);

        if (!(l = RegOpenKeyExW(HKLM,
                                lpKeyName,
                                0,
                                samDesired,
                                &hClassesKey)))
        {
            if (MachineName != NULL)
                RegCloseKey(HKLM);

            if ((l = RegOpenKeyExW(hClassesKey,
                                   bracedGuidString,
                                   0,
                                   samDesired,
                                   &key)))
            {
                SetLastError(l);
                key = INVALID_HANDLE_VALUE;
            }
            RegCloseKey(hClassesKey);
        }
        else
        {
            if (MachineName != NULL) RegCloseKey(HKLM);
            SetLastError(l);
            key = INVALID_HANDLE_VALUE;
        }
    }

    return key;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceW(
        HDEVINFO DeviceInfoSet,
        PCWSTR DevicePath,
        DWORD OpenFlags,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    struct DeviceInfoSet * list;
    PCWSTR pEnd;
    DWORD dwLength, dwError, dwIndex, dwKeyName, dwSubIndex;
    CLSID ClassId;
    WCHAR Buffer[MAX_PATH + 1];
    WCHAR SymBuffer[MAX_PATH + 1];
    WCHAR InstancePath[MAX_PATH + 1];
    HKEY hKey, hDevKey, hSymKey;
    struct DeviceInfo * deviceInfo;
    struct DeviceInterface *deviceInterface;
    BOOL Ret;
    PLIST_ENTRY ItemList;
    PLIST_ENTRY InterfaceListEntry;

    TRACE("%s(%p %s %08x %p)\n", __FUNCTION__,
        DeviceInfoSet, debugstr_w(DevicePath), OpenFlags, DeviceInterfaceData);


    if (DeviceInterfaceData && DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    list = (struct DeviceInfoSet * )DeviceInfoSet;

    dwLength = wcslen(DevicePath);
    if (dwLength < 39)
    {
        /* path must be at least a guid length + L'\0' */
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }

    if (DevicePath[0] != L'\\' ||
        DevicePath[1] != L'\\' ||
        (DevicePath[2] != L'?' && DevicePath[2] != L'.') ||
        DevicePath[3] != L'\\')
    {
        /* invalid formatted path */
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }

    /* check for reference strings */
    pEnd = wcschr(&DevicePath[4], L'\\');
    if (!pEnd)
    {
        /* no reference string */
        pEnd = DevicePath + dwLength;
    }

    /* copy guid */
    wcscpy(Buffer, pEnd - 37);
    Buffer[36] = L'\0';

    dwError = UuidFromStringW(Buffer, &ClassId);
    if (dwError != NOERROR)
    {
        /* invalid formatted path */
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }

    hKey = SetupDiOpenClassRegKeyExW(&ClassId, KEY_READ, DIOCR_INTERFACE, list->MachineName, NULL);

    if (hKey == INVALID_HANDLE_VALUE)
    {
        /* invalid device class */
        return FALSE;
    }

    ItemList = list->ListHead.Flink;
    while (ItemList != &list->ListHead)
    {
        deviceInfo = CONTAINING_RECORD(ItemList, struct DeviceInfo, ListEntry);
        InterfaceListEntry = deviceInfo->InterfaceListHead.Flink;
        while (InterfaceListEntry != &deviceInfo->InterfaceListHead)
        {
            deviceInterface = CONTAINING_RECORD(InterfaceListEntry, struct DeviceInterface, ListEntry);
            if (!IsEqualIID(&deviceInterface->InterfaceClassGuid, &ClassId))
            {
                InterfaceListEntry = InterfaceListEntry->Flink;
                continue;
            }

            if (!wcsicmp(deviceInterface->SymbolicLink, DevicePath))
            {
                if (DeviceInterfaceData)
                {
                    DeviceInterfaceData->Reserved = (ULONG_PTR)deviceInterface;
                    DeviceInterfaceData->Flags = deviceInterface->Flags;
                    CopyMemory(&DeviceInterfaceData->InterfaceClassGuid, &ClassId, sizeof(GUID));
                }

                return TRUE;
            }

        }
    }


    dwIndex = 0;
    do
    {
        Buffer[0] = 0;
        dwKeyName = sizeof(Buffer) / sizeof(WCHAR);
        dwError = RegEnumKeyExW(hKey, dwIndex, Buffer, &dwKeyName, NULL, NULL, NULL, NULL);

        if (dwError != ERROR_SUCCESS)
            break;

        if (RegOpenKeyExW(hKey, Buffer, 0, KEY_READ, &hDevKey) != ERROR_SUCCESS)
            break;

        dwSubIndex = 0;
        InstancePath[0] = 0;
        dwKeyName = sizeof(InstancePath);

        dwError = RegQueryValueExW(hDevKey, L"DeviceInstance", NULL, NULL, (LPBYTE)InstancePath, &dwKeyName);

        while(TRUE)
        {
            Buffer[0] = 0;
            dwKeyName = sizeof(Buffer) / sizeof(WCHAR);
            dwError = RegEnumKeyExW(hDevKey, dwSubIndex, Buffer, &dwKeyName, NULL, NULL, NULL, NULL);

            if (dwError != ERROR_SUCCESS)
                break;

            dwError = RegOpenKeyExW(hDevKey, Buffer, 0, KEY_READ, &hSymKey);
            if (dwError != ERROR_SUCCESS)
                break;

            /* query for symbolic link */
            dwKeyName = sizeof(SymBuffer);
            SymBuffer[0] = L'\0';
            dwError = RegQueryValueExW(hSymKey, L"SymbolicLink", NULL, NULL, (LPBYTE)SymBuffer, &dwKeyName);

            if (dwError != ERROR_SUCCESS)
            {
                RegCloseKey(hSymKey);
                break;
            }

            if (!wcsicmp(SymBuffer, DevicePath))
            {
                Ret = CreateDeviceInfo(list, InstancePath, &ClassId, &deviceInfo);
                RegCloseKey(hSymKey);
                RegCloseKey(hDevKey);
                RegCloseKey(hKey);

                if (Ret)
                {
                    deviceInterface = HeapAlloc(GetProcessHeap(), 0, sizeof(struct DeviceInterface) + (wcslen(SymBuffer) + 1) * sizeof(WCHAR));
                    if (deviceInterface)
                    {

                        CopyMemory(&deviceInterface->InterfaceClassGuid, &ClassId, sizeof(GUID));
                        deviceInterface->DeviceInfo = deviceInfo;
                        deviceInterface->Flags = SPINT_ACTIVE; //FIXME

                        wcscpy(deviceInterface->SymbolicLink, SymBuffer);

                        InsertTailList(&deviceInfo->InterfaceListHead, &deviceInterface->ListEntry);
                        InsertTailList(&list->ListHead, &deviceInfo->ListEntry);


                        if (DeviceInterfaceData)
                        {
                            DeviceInterfaceData->Reserved = (ULONG_PTR)deviceInterface;
                            DeviceInterfaceData->Flags = deviceInterface->Flags;
                            CopyMemory(&DeviceInterfaceData->InterfaceClassGuid, &ClassId, sizeof(GUID));
                        }
                        else
                        {
                            Ret = FALSE;
                            SetLastError(ERROR_INVALID_USER_BUFFER);
                        }
                    }
                }
                else
                {
                    HeapFree(GetProcessHeap(), 0, deviceInfo);
                    Ret = FALSE;
                }
                return Ret;
        }
        RegCloseKey(hSymKey);
        dwSubIndex++;
    }

    RegCloseKey(hDevKey);
    dwIndex++;
    } while(TRUE);

    RegCloseKey(hKey);
    return FALSE;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceA(
        HDEVINFO DeviceInfoSet,
        PCSTR DevicePath,
        DWORD OpenFlags,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    LPWSTR DevicePathW = NULL;
    BOOL bResult;

    TRACE("%s(%p %s %08lx %p)\n", __FUNCTION__, DeviceInfoSet, debugstr_a(DevicePath), OpenFlags, DeviceInterfaceData);

    DevicePathW = pSetupMultiByteToUnicode(DevicePath, CP_ACP);
    if (DevicePathW == NULL)
        return FALSE;

    bResult = SetupDiOpenDeviceInterfaceW(DeviceInfoSet,
        DevicePathW, OpenFlags, DeviceInterfaceData);

    MyFree(DevicePathW);

    return bResult;
}

/***********************************************************************
 *		SetupDiSetClassInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetClassInstallParamsA(
        HDEVINFO  DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        PSP_CLASSINSTALL_HEADER ClassInstallParams,
        DWORD ClassInstallParamsSize)
{
    FIXME("%p %p %x %u\n",DeviceInfoSet, DeviceInfoData,
          ClassInstallParams->InstallFunction, ClassInstallParamsSize);
    return FALSE;
}

static BOOL WINAPI
IntSetupDiRegisterDeviceInfo(
        IN HDEVINFO DeviceInfoSet,
        IN OUT PSP_DEVINFO_DATA DeviceInfoData)
{
    return SetupDiRegisterDeviceInfo(DeviceInfoSet, DeviceInfoData, 0, NULL, NULL, NULL);
}

/***********************************************************************
 *		SetupDiCallClassInstaller (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCallClassInstaller(
        DI_FUNCTION InstallFunction,
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;

    TRACE("%s(%u %p %p)\n", __FUNCTION__, InstallFunction, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->HKLM != HKEY_LOCAL_MACHINE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        SP_DEVINSTALL_PARAMS_W InstallParams;
#define CLASS_COINSTALLER  0x1
#define DEVICE_COINSTALLER 0x2
#define CLASS_INSTALLER    0x4
        UCHAR CanHandle = 0;
        DEFAULT_CLASS_INSTALL_PROC DefaultHandler = NULL;

        switch (InstallFunction)
        {
            case DIF_ADDPROPERTYPAGE_ADVANCED:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_ALLOW_INSTALL:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_DETECT:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_DESTROYPRIVATEDATA:
                CanHandle = CLASS_INSTALLER;
                break;
            case DIF_INSTALLDEVICE:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiInstallDevice;
                break;
            case DIF_INSTALLDEVICEFILES:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiInstallDriverFiles;
                break;
            case DIF_INSTALLINTERFACES:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiInstallDeviceInterfaces;
                break;
            case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_NEWDEVICEWIZARD_POSTANALYZE:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_NEWDEVICEWIZARD_PREANALYZE:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_NEWDEVICEWIZARD_PRESELECT:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_NEWDEVICEWIZARD_SELECT:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_POWERMESSAGEWAKE:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_PROPERTYCHANGE:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiChangeState;
                break;
            case DIF_REGISTER_COINSTALLERS:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiRegisterCoDeviceInstallers;
                break;
            case DIF_REGISTERDEVICE:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = IntSetupDiRegisterDeviceInfo;
                break;
            case DIF_REMOVE:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiRemoveDevice;
                break;
            case DIF_SELECTBESTCOMPATDRV:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiSelectBestCompatDrv;
                break;
            case DIF_SELECTDEVICE:
                CanHandle = CLASS_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiSelectDevice;
                break;
            case DIF_TROUBLESHOOTER:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                break;
            case DIF_UNREMOVE:
                CanHandle = CLASS_COINSTALLER | DEVICE_COINSTALLER | CLASS_INSTALLER;
                DefaultHandler = SetupDiUnremoveDevice;
                break;
            default:
                ERR("Install function %u not supported\n", InstallFunction);
                SetLastError(ERROR_NOT_SUPPORTED);
        }

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        if (!SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams))
            /* Don't process this call, as a parameter is invalid */
            CanHandle = 0;

        if (CanHandle != 0)
        {
            LIST_ENTRY ClassCoInstallersListHead;
            LIST_ENTRY DeviceCoInstallersListHead;
            HMODULE ClassInstallerLibrary = NULL;
            CLASS_INSTALL_PROC ClassInstaller = NULL;
            COINSTALLER_CONTEXT_DATA Context;
            PLIST_ENTRY ListEntry;
            HKEY hKey;
            DWORD dwRegType, dwLength;
            DWORD rc = NO_ERROR;

            InitializeListHead(&ClassCoInstallersListHead);
            InitializeListHead(&DeviceCoInstallersListHead);

            if (CanHandle & DEVICE_COINSTALLER)
            {
                hKey = SETUPDI_OpenDrvKey(((struct DeviceInfoSet *)DeviceInfoSet)->HKLM, (struct DeviceInfo *)DeviceInfoData->Reserved, KEY_QUERY_VALUE);
                if (hKey != INVALID_HANDLE_VALUE)
                {
                    rc = RegQueryValueExW(hKey, REGSTR_VAL_COINSTALLERS_32, NULL, &dwRegType, NULL, &dwLength);
                    if (rc == ERROR_SUCCESS && dwRegType == REG_MULTI_SZ)
                    {
                        LPWSTR KeyBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
                        if (KeyBuffer != NULL)
                        {
                            rc = RegQueryValueExW(hKey, REGSTR_VAL_COINSTALLERS_32, NULL, NULL, (LPBYTE)KeyBuffer, &dwLength);
                            if (rc == ERROR_SUCCESS)
                            {
                                LPWSTR ptr;
                                for (ptr = KeyBuffer; *ptr; ptr += strlenW(ptr) + 1)
                                {
                                    /* Add coinstaller to DeviceCoInstallersListHead list */
                                    struct CoInstallerElement *coinstaller;
                                    TRACE("Got device coinstaller '%s'\n", debugstr_w(ptr));
                                    coinstaller = HeapAlloc(GetProcessHeap(), 0, sizeof(struct CoInstallerElement));
                                    if (!coinstaller)
                                        continue;
                                    ZeroMemory(coinstaller, sizeof(struct CoInstallerElement));
                                    if (GetFunctionPointer(ptr, &coinstaller->Module, (PVOID*)&coinstaller->Function) == ERROR_SUCCESS)
                                        InsertTailList(&DeviceCoInstallersListHead, &coinstaller->ListEntry);
                                    else
                                        HeapFree(GetProcessHeap(), 0, coinstaller);
                                }
                            }
                            HeapFree(GetProcessHeap(), 0, KeyBuffer);
                        }
                    }
                    RegCloseKey(hKey);
                }
            }
            if (CanHandle & CLASS_COINSTALLER)
            {
                rc = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    REGSTR_PATH_CODEVICEINSTALLERS,
                    0, /* Options */
                    KEY_QUERY_VALUE,
                    &hKey);
                if (rc == ERROR_SUCCESS)
                {
                    WCHAR szGuidString[40];
                    if (pSetupStringFromGuid(&DeviceInfoData->ClassGuid, szGuidString, ARRAYSIZE(szGuidString)) == ERROR_SUCCESS)
                    {
                        rc = RegQueryValueExW(hKey, szGuidString, NULL, &dwRegType, NULL, &dwLength);
                        if (rc == ERROR_SUCCESS && dwRegType == REG_MULTI_SZ)
                        {
                            LPWSTR KeyBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
                            if (KeyBuffer != NULL)
                            {
                                rc = RegQueryValueExW(hKey, szGuidString, NULL, NULL, (LPBYTE)KeyBuffer, &dwLength);
                                if (rc == ERROR_SUCCESS)
                                {
                                    LPWSTR ptr;
                                    for (ptr = KeyBuffer; *ptr; ptr += strlenW(ptr) + 1)
                                    {
                                        /* Add coinstaller to ClassCoInstallersListHead list */
                                        struct CoInstallerElement *coinstaller;
                                        TRACE("Got class coinstaller '%s'\n", debugstr_w(ptr));
                                        coinstaller = HeapAlloc(GetProcessHeap(), 0, sizeof(struct CoInstallerElement));
                                        if (!coinstaller)
                                            continue;
                                        ZeroMemory(coinstaller, sizeof(struct CoInstallerElement));
                                        if (GetFunctionPointer(ptr, &coinstaller->Module, (PVOID*)&coinstaller->Function) == ERROR_SUCCESS)
                                            InsertTailList(&ClassCoInstallersListHead, &coinstaller->ListEntry);
                                        else
                                            HeapFree(GetProcessHeap(), 0, coinstaller);
                                    }
                                }
                                HeapFree(GetProcessHeap(), 0, KeyBuffer);
                            }
                        }
                    }
                    RegCloseKey(hKey);
                }
            }
            if ((CanHandle & CLASS_INSTALLER) && !(InstallParams.FlagsEx & DI_FLAGSEX_CI_FAILED))
            {
                hKey = SetupDiOpenClassRegKey(&DeviceInfoData->ClassGuid, KEY_QUERY_VALUE);
                if (hKey != INVALID_HANDLE_VALUE)
                {
                    rc = RegQueryValueExW(hKey, REGSTR_VAL_INSTALLER_32, NULL, &dwRegType, NULL, &dwLength);
                    if (rc == ERROR_SUCCESS && dwRegType == REG_SZ)
                    {
                        LPWSTR KeyBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
                        if (KeyBuffer != NULL)
                        {
                            rc = RegQueryValueExW(hKey, REGSTR_VAL_INSTALLER_32, NULL, NULL, (LPBYTE)KeyBuffer, &dwLength);
                            if (rc == ERROR_SUCCESS)
                            {
                                /* Get ClassInstaller function pointer */
                                TRACE("Got class installer '%s'\n", debugstr_w(KeyBuffer));
                                if (GetFunctionPointer(KeyBuffer, &ClassInstallerLibrary, (PVOID*)&ClassInstaller) != ERROR_SUCCESS)
                                {
                                    InstallParams.FlagsEx |= DI_FLAGSEX_CI_FAILED;
                                    SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
                                }
                            }
                            HeapFree(GetProcessHeap(), 0, KeyBuffer);
                        }
                    }
                    RegCloseKey(hKey);
                }
            }

            /* Call Class co-installers */
            Context.PostProcessing = FALSE;
            rc = NO_ERROR;
            ListEntry = ClassCoInstallersListHead.Flink;
            while (rc == NO_ERROR && ListEntry != &ClassCoInstallersListHead)
            {
                struct CoInstallerElement *coinstaller;
                coinstaller = CONTAINING_RECORD(ListEntry, struct CoInstallerElement, ListEntry);
                rc = (*coinstaller->Function)(InstallFunction, DeviceInfoSet, DeviceInfoData, &Context);
                coinstaller->PrivateData = Context.PrivateData;
                if (rc == ERROR_DI_POSTPROCESSING_REQUIRED)
                {
                    coinstaller->DoPostProcessing = TRUE;
                    rc = NO_ERROR;
                }
                ListEntry = ListEntry->Flink;
            }

            /* Call Device co-installers */
            ListEntry = DeviceCoInstallersListHead.Flink;
            while (rc == NO_ERROR && ListEntry != &DeviceCoInstallersListHead)
            {
                struct CoInstallerElement *coinstaller;
                coinstaller = CONTAINING_RECORD(ListEntry, struct CoInstallerElement, ListEntry);
                rc = (*coinstaller->Function)(InstallFunction, DeviceInfoSet, DeviceInfoData, &Context);
                coinstaller->PrivateData = Context.PrivateData;
                if (rc == ERROR_DI_POSTPROCESSING_REQUIRED)
                {
                    coinstaller->DoPostProcessing = TRUE;
                    rc = NO_ERROR;
                }
                ListEntry = ListEntry->Flink;
            }

            /* Call Class installer */
            if (ClassInstaller)
            {
                rc = (*ClassInstaller)(InstallFunction, DeviceInfoSet, DeviceInfoData);
                FreeFunctionPointer(ClassInstallerLibrary, ClassInstaller);
            }
            else
                rc = ERROR_DI_DO_DEFAULT;

            /* Call default handler */
            if (rc == ERROR_DI_DO_DEFAULT)
            {
                if (DefaultHandler && !(InstallParams.Flags & DI_NODI_DEFAULTACTION))
                {
                    if ((*DefaultHandler)(DeviceInfoSet, DeviceInfoData))
                        rc = NO_ERROR;
                    else
                        rc = GetLastError();
                }
                else
                    rc = NO_ERROR;
            }

            /* Call Class co-installers that required postprocessing */
            Context.PostProcessing = TRUE;
            ListEntry = ClassCoInstallersListHead.Flink;
            while (ListEntry != &ClassCoInstallersListHead)
            {
                struct CoInstallerElement *coinstaller;
                coinstaller = CONTAINING_RECORD(ListEntry, struct CoInstallerElement, ListEntry);
                if (coinstaller->DoPostProcessing)
                {
                    Context.InstallResult = rc;
                    Context.PrivateData = coinstaller->PrivateData;
                    rc = (*coinstaller->Function)(InstallFunction, DeviceInfoSet, DeviceInfoData, &Context);
                }
                FreeFunctionPointer(coinstaller->Module, coinstaller->Function);
                ListEntry = ListEntry->Flink;
            }

            /* Call Device co-installers that required postprocessing */
            ListEntry = DeviceCoInstallersListHead.Flink;
            while (ListEntry != &DeviceCoInstallersListHead)
            {
                struct CoInstallerElement *coinstaller;
                coinstaller = CONTAINING_RECORD(ListEntry, struct CoInstallerElement, ListEntry);
                if (coinstaller->DoPostProcessing)
                {
                    Context.InstallResult = rc;
                    Context.PrivateData = coinstaller->PrivateData;
                    rc = (*coinstaller->Function)(InstallFunction, DeviceInfoSet, DeviceInfoData, &Context);
                }
                FreeFunctionPointer(coinstaller->Module, coinstaller->Function);
                ListEntry = ListEntry->Flink;
            }

            /* Free allocated memory */
            while (!IsListEmpty(&ClassCoInstallersListHead))
            {
                ListEntry = RemoveHeadList(&ClassCoInstallersListHead);
                HeapFree(GetProcessHeap(), 0, CONTAINING_RECORD(ListEntry, struct CoInstallerElement, ListEntry));
            }
            while (!IsListEmpty(&DeviceCoInstallersListHead))
            {
                ListEntry = RemoveHeadList(&DeviceCoInstallersListHead);
                HeapFree(GetProcessHeap(), 0, CONTAINING_RECORD(ListEntry, struct CoInstallerElement, ListEntry));
            }

            ret = (rc == NO_ERROR);
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstallParamsA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        PSP_DEVINSTALL_PARAMS_A DeviceInstallParams)
{
    SP_DEVINSTALL_PARAMS_W deviceInstallParamsW;
    BOOL ret = FALSE;

    TRACE("%s(%p %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    if (DeviceInstallParams == NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS_A))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        deviceInstallParamsW.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        ret = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &deviceInstallParamsW);

        if (ret)
        {
            /* Do W->A conversion */
            memcpy(
                DeviceInstallParams,
                &deviceInstallParamsW,
                FIELD_OFFSET(SP_DEVINSTALL_PARAMS_W, DriverPath));
            if (WideCharToMultiByte(CP_ACP, 0, deviceInstallParamsW.DriverPath, -1,
                DeviceInstallParams->DriverPath, MAX_PATH, NULL, NULL) == 0)
            {
                DeviceInstallParams->DriverPath[0] = '\0';
                ret = FALSE;
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListClass  (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInfoListClass(
        IN HDEVINFO DeviceInfoSet,
        OUT LPGUID ClassGuid)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, ClassGuid);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (IsEqualIID(&list->ClassGuid, &GUID_NULL))
        SetLastError(ERROR_NO_ASSOCIATED_CLASS);
    else
    {
        *ClassGuid = list->ClassGuid;

        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInstallParamsW(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
        OUT PSP_DEVINSTALL_PARAMS_W DeviceInstallParams)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%s(%p %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!DeviceInstallParams)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        PSP_DEVINSTALL_PARAMS_W Source;

        if (DeviceInfoData)
            Source = &((struct DeviceInfo *)DeviceInfoData->Reserved)->InstallParams;
        else
            Source = &list->InstallParams;

        ret = TRUE;

        _SEH2_TRY
        {
            memcpy(DeviceInstallParams, Source, Source->cbSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
            ret = FALSE;
        }
        _SEH2_END;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static BOOL
CheckDeviceInstallParameters(
        IN PSP_DEVINSTALL_PARAMS_W DeviceInstallParams)
{
    DWORD SupportedFlags =
        DI_NOVCP |                            /* 0x00000008 */
        DI_DIDCOMPAT |                        /* 0x00000010 */
        DI_DIDCLASS |                         /* 0x00000020 */
        DI_NEEDRESTART |                      /* 0x00000080 */
        DI_NEEDREBOOT |                       /* 0x00000100 */
        DI_RESOURCEPAGE_ADDED |               /* 0x00002000 */
        DI_PROPERTIES_CHANGE |                /* 0x00004000 */
        DI_ENUMSINGLEINF |                    /* 0x00010000 */
        DI_DONOTCALLCONFIGMG |                /* 0x00020000 */
        DI_CLASSINSTALLPARAMS |               /* 0x00100000 */
        DI_NODI_DEFAULTACTION |               /* 0x00200000 */
        DI_QUIETINSTALL |                     /* 0x00800000 */
        DI_NOFILECOPY |                       /* 0x01000000 */
        DI_DRIVERPAGE_ADDED;                  /* 0x04000000 */
    DWORD SupportedFlagsEx =
        DI_FLAGSEX_CI_FAILED |                /* 0x00000004 */
        DI_FLAGSEX_DIDINFOLIST |              /* 0x00000010 */
        DI_FLAGSEX_DIDCOMPATINFO |            /* 0x00000020 */
        DI_FLAGSEX_ALLOWEXCLUDEDDRVS |        /* 0x00000800 */
        DI_FLAGSEX_NO_DRVREG_MODIFY |         /* 0x00008000 */
        DI_FLAGSEX_INSTALLEDDRIVER;           /* 0x04000000 */
    BOOL ret = FALSE;

    /* FIXME: add support for more flags */

    /* FIXME: DI_CLASSINSTALLPARAMS flag is not correctly used.
     * It should be checked before accessing to other values
     * of the SP_DEVINSTALL_PARAMS structure */

    if (DeviceInstallParams->Flags & ~SupportedFlags)
    {
        FIXME("Unknown Flags: 0x%08lx\n", DeviceInstallParams->Flags & ~SupportedFlags);
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if (DeviceInstallParams->FlagsEx & ~SupportedFlagsEx)
    {
        FIXME("Unknown FlagsEx: 0x%08lx\n", DeviceInstallParams->FlagsEx & ~SupportedFlagsEx);
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if ((DeviceInstallParams->Flags & DI_NOVCP)
        && (DeviceInstallParams->FileQueue == NULL || DeviceInstallParams->FileQueue == (HSPFILEQ)INVALID_HANDLE_VALUE))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        /* FIXME: check Reserved field */
        ret = TRUE;
    }

    return ret;
}

/***********************************************************************
 *		SetupDiSetDeviceInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetDeviceInstallParamsW(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
        IN PSP_DEVINSTALL_PARAMS_W DeviceInstallParams)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%s(%p %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!DeviceInstallParams)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInstallParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (CheckDeviceInstallParameters(DeviceInstallParams))
    {
        PSP_DEVINSTALL_PARAMS_W Destination;

        if (DeviceInfoData)
            Destination = &((struct DeviceInfo *)DeviceInfoData->Reserved)->InstallParams;
        else
            Destination = &list->InstallParams;
        memcpy(Destination, DeviceInstallParams, DeviceInstallParams->cbSize);
        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiSetDeviceInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetDeviceInstallParamsA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        PSP_DEVINSTALL_PARAMS_A DeviceInstallParams)
{
    SP_DEVINSTALL_PARAMS_W deviceInstallParamsW;
    int len = 0;
    BOOL ret = FALSE;

    TRACE("%s(%p %p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    if (DeviceInstallParams == NULL)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInstallParams->cbSize < sizeof(SP_DEVINSTALL_PARAMS_A))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        memcpy(&deviceInstallParamsW, DeviceInstallParams, FIELD_OFFSET(SP_DEVINSTALL_PARAMS_A, DriverPath));
        deviceInstallParamsW.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        len = MultiByteToWideChar(CP_ACP, 0, DeviceInstallParams->DriverPath, -1, NULL, 0);
        if (!len)
        {
            ERR("DrivePath is NULL\n");
            ret = FALSE;
        }
        else
        {
            MultiByteToWideChar(CP_ACP, 0, DeviceInstallParams->DriverPath, -1, deviceInstallParamsW.DriverPath, len);
            ret = SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &deviceInstallParamsW);
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static HKEY
OpenHardwareProfileKey(
        IN HKEY HKLM,
        IN DWORD HwProfile,
        IN DWORD samDesired)
{
    HKEY hHWProfilesKey = NULL;
    HKEY hHWProfileKey = NULL;
    HKEY ret = INVALID_HANDLE_VALUE;
    LONG rc;

    rc = RegOpenKeyExW(HKLM,
                       REGSTR_PATH_HWPROFILES,
                       0,
                       READ_CONTROL,
                       &hHWProfilesKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    if (HwProfile == 0)
    {
        rc = RegOpenKeyExW(hHWProfilesKey,
                           REGSTR_KEY_CURRENT,
                           0,
                           KEY_CREATE_SUB_KEY,
                           &hHWProfileKey);
    }
    else
    {
        WCHAR subKey[5];
        snprintfW(subKey, 4, InstanceKeyFormat, HwProfile);
        subKey[4] = '\0';
        rc = RegOpenKeyExW(hHWProfilesKey,
                           subKey,
                           0,
                           KEY_CREATE_SUB_KEY,
                           &hHWProfileKey);
    }
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    ret = hHWProfileKey;

cleanup:
    if (hHWProfilesKey != NULL)
        RegCloseKey(hHWProfilesKey);
    if (hHWProfileKey != NULL && hHWProfileKey != ret)
        RegCloseKey(hHWProfileKey);
    return ret;
}

static BOOL
IsDeviceInfoInDeviceInfoSet(
        struct DeviceInfoSet *deviceInfoSet,
        struct DeviceInfo *deviceInfo)
{
    PLIST_ENTRY ListEntry;

    ListEntry = deviceInfoSet->ListHead.Flink;
    while (ListEntry != &deviceInfoSet->ListHead)
    {
        if (deviceInfo == CONTAINING_RECORD(ListEntry, struct DeviceInfo, ListEntry))
            return TRUE;

        ListEntry = ListEntry->Flink;
    }

    return FALSE;
}

/***********************************************************************
 *		SetupDiDeleteDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiDeleteDeviceInfo(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *deviceInfoSet;
    struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData;
    BOOL ret = FALSE;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((deviceInfoSet = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!IsDeviceInfoInDeviceInfoSet(deviceInfoSet, deviceInfo))
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        RemoveEntryList(&deviceInfo->ListEntry);
        DestroyDeviceInfo(deviceInfo);
        ret = TRUE;
    }

    return ret;
}


/***********************************************************************
 *		SetupDiOpenDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiOpenDeviceInfoA(
        IN HDEVINFO DeviceInfoSet,
        IN PCSTR DeviceInstanceId,
        IN HWND hwndParent OPTIONAL,
        IN DWORD OpenFlags,
        OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    LPWSTR DeviceInstanceIdW = NULL;
    BOOL bResult;

    TRACE("%s(%p %s %p %lx %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInstanceId, hwndParent, OpenFlags, DeviceInfoData);

    DeviceInstanceIdW = pSetupMultiByteToUnicode(DeviceInstanceId, CP_ACP);
    if (DeviceInstanceIdW == NULL)
        return FALSE;

    bResult = SetupDiOpenDeviceInfoW(DeviceInfoSet,
        DeviceInstanceIdW, hwndParent, OpenFlags, DeviceInfoData);

    MyFree(DeviceInstanceIdW);

    return bResult;
}


/***********************************************************************
 *		SetupDiOpenDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiOpenDeviceInfoW(
        IN HDEVINFO DeviceInfoSet,
        IN PCWSTR DeviceInstanceId,
        IN HWND hwndParent OPTIONAL,
        IN DWORD OpenFlags,
        OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    struct DeviceInfoSet *list;
    HKEY hEnumKey, hKey = NULL;
    DWORD rc, dwSize;
    BOOL ret = FALSE;

    TRACE("%s(%p %s %p %lx %p)\n", __FUNCTION__,
        DeviceInfoSet, debugstr_w(DeviceInstanceId),
        hwndParent, OpenFlags, DeviceInfoData);

    if (OpenFlags & DIOD_CANCEL_REMOVE)
        FIXME("DIOD_CANCEL_REMOVE flag not implemented\n");

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInstanceId)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (OpenFlags & ~(DIOD_CANCEL_REMOVE | DIOD_INHERIT_CLASSDRVS))
    {
        TRACE("Unknown flags: 0x%08lx\n", OpenFlags & ~(DIOD_CANCEL_REMOVE | DIOD_INHERIT_CLASSDRVS));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        struct DeviceInfo *deviceInfo = NULL;
        /* Search if device already exists in DeviceInfoSet.
         *    If yes, return the existing element
         *    If no, create a new element using information in registry
         */
        PLIST_ENTRY ItemList = list->ListHead.Flink;
        while (ItemList != &list->ListHead)
        {
            deviceInfo = CONTAINING_RECORD(ItemList, struct DeviceInfo, ListEntry);
            if (!wcscmp(deviceInfo->instanceId, DeviceInstanceId))
                break;
            deviceInfo = NULL;
            ItemList = ItemList->Flink;
        }

        if (deviceInfo)
        {
            /* good one found */
            ret = TRUE;
        }
        else
        {
            GUID ClassGUID;
            WCHAR szClassGuid[MAX_GUID_STRING_LEN];

            /* Open supposed registry key */
            rc = RegOpenKeyExW(
                list->HKLM,
                REGSTR_PATH_SYSTEMENUM,
                0, /* Options */
                READ_CONTROL,
                &hEnumKey);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            rc = RegOpenKeyExW(
                hEnumKey,
                DeviceInstanceId,
                0, /* Options */
                KEY_QUERY_VALUE,
                &hKey);
            RegCloseKey(hEnumKey);
            if (rc != ERROR_SUCCESS)
            {
                if (rc == ERROR_FILE_NOT_FOUND)
                    rc = ERROR_NO_SUCH_DEVINST;
                SetLastError(rc);
                goto cleanup;
            }

            ClassGUID = GUID_NULL;
            dwSize = MAX_GUID_STRING_LEN * sizeof(WCHAR);

            if (RegQueryValueExW(hKey,
                                 REGSTR_VAL_CLASSGUID,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szClassGuid,
                                 &dwSize) == ERROR_SUCCESS)
            {
                szClassGuid[MAX_GUID_STRING_LEN - 2] = UNICODE_NULL;

                /* Convert a string to a ClassGuid */
                UuidFromStringW(&szClassGuid[1], &ClassGUID);
            }

            if (!CreateDeviceInfo(list, DeviceInstanceId, &ClassGUID, &deviceInfo))
                goto cleanup;

            InsertTailList(&list->ListHead, &deviceInfo->ListEntry);

            ret = TRUE;
        }

        if (ret && deviceInfo && DeviceInfoData)
        {
            memcpy(&DeviceInfoData->ClassGuid, &deviceInfo->ClassGuid, sizeof(GUID));
            DeviceInfoData->DevInst = deviceInfo->dnDevInst;
            DeviceInfoData->Reserved = (ULONG_PTR)deviceInfo;
        }
    }

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    return ret;
}


/***********************************************************************
 *		SetupDiGetSelectedDevice (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetSelectedDevice(
        IN HDEVINFO DeviceInfoSet,
        OUT PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (list->SelectedDevice == NULL)
        SetLastError(ERROR_NO_DEVICE_SELECTED);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        memcpy(&DeviceInfoData->ClassGuid,
            &list->SelectedDevice->ClassGuid,
            sizeof(GUID));
        DeviceInfoData->DevInst = list->SelectedDevice->dnDevInst;
        DeviceInfoData->Reserved = (ULONG_PTR)list->SelectedDevice;
        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupDiSetSelectedDevice (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetSelectedDevice(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (DeviceInfoData->Reserved == 0)
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        list->SelectedDevice = (struct DeviceInfo *)DeviceInfoData->Reserved;
        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/* Return the current hardware profile id, or -1 if error */
static DWORD
SETUPAPI_GetCurrentHwProfile(
        IN HDEVINFO DeviceInfoSet)
{
    HKEY hKey = NULL;
    DWORD dwRegType, dwLength;
    DWORD hwProfile;
    LONG rc;
    DWORD ret = (DWORD)-1;

    rc = RegOpenKeyExW(((struct DeviceInfoSet *)DeviceInfoSet)->HKLM,
                       REGSTR_PATH_IDCONFIGDB,
                       0, /* Options */
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }

    dwLength = sizeof(DWORD);
    rc = RegQueryValueExW(hKey,
                          REGSTR_VAL_CURRENTCONFIG,
                          NULL,
                          &dwRegType,
                          (LPBYTE)&hwProfile, &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    else if (dwRegType != REG_DWORD || dwLength != sizeof(DWORD))
    {
        SetLastError(ERROR_GEN_FAILURE);
        goto cleanup;
    }

    ret = hwProfile;

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);

    return ret;
}

static BOOL
ResetDevice(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData)
{
#ifndef __WINESRC__
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    CONFIGRET cr;

    cr = CM_Enable_DevNode_Ex(deviceInfo->dnDevInst, 0, set->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(GetErrorCodeFromCrCode(cr));
        return FALSE;
    }

    return TRUE;
#else
    FIXME("Stub: ResetDevice(%p %p)\n", DeviceInfoSet, DeviceInfoData);
    return TRUE;
#endif
}

static BOOL
StopDevice(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData)
{
#ifndef __WINESRC__
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    CONFIGRET cr;

    cr = CM_Disable_DevNode_Ex(deviceInfo->dnDevInst, 0, set->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(GetErrorCodeFromCrCode(cr));
        return FALSE;
    }

    return TRUE;
#else
    FIXME("Stub: StopDevice(%p %p)\n", DeviceInfoSet, DeviceInfoData);
    return TRUE;
#endif
}

/***********************************************************************
 *		SetupDiChangeState (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiChangeState(
        IN HDEVINFO DeviceInfoSet,
        IN OUT PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    PSP_PROPCHANGE_PARAMS PropChange;
    HKEY hRootKey = INVALID_HANDLE_VALUE, hKey = INVALID_HANDLE_VALUE;
    LPCWSTR RegistryValueName;
    DWORD dwConfigFlags, dwLength, dwRegType;
    LONG rc;
    BOOL ret = FALSE;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoData)
        PropChange = ((struct DeviceInfoSet *)DeviceInfoSet)->ClassInstallParams.PropChangeParams;
    else
        PropChange = ((struct DeviceInfo *)DeviceInfoData->Reserved)->ClassInstallParams.PropChangeParams;
    if (!PropChange)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    if (PropChange->Scope == DICS_FLAG_GLOBAL)
        RegistryValueName = REGSTR_VAL_CONFIGFLAGS;
    else
        RegistryValueName = REGSTR_VAL_CSCONFIGFLAGS;

    switch (PropChange->StateChange)
    {
        case DICS_ENABLE:
        case DICS_DISABLE:
        {
            if (PropChange->Scope == DICS_FLAG_GLOBAL)
                hRootKey = set->HKLM;
            else /* PropChange->Scope == DICS_FLAG_CONFIGSPECIFIC */
            {
                hRootKey = OpenHardwareProfileKey(set->HKLM, PropChange->HwProfile, KEY_CREATE_SUB_KEY);
                if (hRootKey == INVALID_HANDLE_VALUE)
                    goto cleanup;
            }

            /* Enable/disable device in registry */
            hKey = SETUPDI_OpenDrvKey(hRootKey, deviceInfo, KEY_QUERY_VALUE | KEY_SET_VALUE);
            if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
                hKey = SETUPDI_CreateDevKey(hRootKey, deviceInfo, KEY_QUERY_VALUE | KEY_SET_VALUE);
            if (hKey == INVALID_HANDLE_VALUE)
                break;
            dwLength = sizeof(DWORD);
            rc = RegQueryValueExW(
                hKey,
                RegistryValueName,
                NULL,
                &dwRegType,
                (LPBYTE)&dwConfigFlags, &dwLength);
            if (rc == ERROR_FILE_NOT_FOUND)
                dwConfigFlags = 0;
            else if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            else if (dwRegType != REG_DWORD || dwLength != sizeof(DWORD))
            {
                SetLastError(ERROR_GEN_FAILURE);
                goto cleanup;
            }
            if (PropChange->StateChange == DICS_ENABLE)
                dwConfigFlags &= ~(PropChange->Scope == DICS_FLAG_GLOBAL ? CONFIGFLAG_DISABLED : CSCONFIGFLAG_DISABLED);
            else
                dwConfigFlags |= (PropChange->Scope == DICS_FLAG_GLOBAL ? CONFIGFLAG_DISABLED : CSCONFIGFLAG_DISABLED);
            rc = RegSetValueExW(
                hKey,
                RegistryValueName,
                0,
                REG_DWORD,
                (LPBYTE)&dwConfigFlags, sizeof(DWORD));
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }

            /* Enable/disable device if needed */
            if (PropChange->Scope == DICS_FLAG_GLOBAL
                || PropChange->HwProfile == 0
                || PropChange->HwProfile == SETUPAPI_GetCurrentHwProfile(DeviceInfoSet))
            {
                if (PropChange->StateChange == DICS_ENABLE)
                    ret = ResetDevice(DeviceInfoSet, DeviceInfoData);
                else
                    ret = StopDevice(DeviceInfoSet, DeviceInfoData);
            }
            else
                ret = TRUE;
            break;
        }
        case DICS_PROPCHANGE:
        {
            ret = ResetDevice(DeviceInfoSet, DeviceInfoData);
            break;
        }
        default:
        {
            ERR("Unknown StateChange 0x%lx\n", PropChange->StateChange);
            SetLastError(ERROR_NOT_SUPPORTED);
        }
    }

cleanup:
    if (hRootKey != INVALID_HANDLE_VALUE && hRootKey != set->HKLM)
        RegCloseKey(hRootKey);

    if (hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiSelectDevice (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSelectDevice(
        IN HDEVINFO DeviceInfoSet,
        IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    FIXME("%p %p\n", DeviceInfoSet, DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		SetupDiRegisterCoDeviceInstallers (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiRegisterCoDeviceInstallers(
        IN HDEVINFO DeviceInfoSet,
        IN PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo;
    BOOL ret = FALSE; /* Return value */

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        SP_DEVINSTALL_PARAMS_W InstallParams;
        struct DriverInfoElement *SelectedDriver;
        BOOL Result;
        DWORD DoAction;
        WCHAR SectionName[MAX_PATH];
        DWORD SectionNameLength = 0;
        HKEY hKey = INVALID_HANDLE_VALUE;
        PVOID Context = NULL;

        InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
        Result = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
        if (!Result)
            goto cleanup;

        SelectedDriver = (struct DriverInfoElement *)InstallParams.ClassInstallReserved;
        if (SelectedDriver == NULL)
        {
            SetLastError(ERROR_NO_DRIVER_SELECTED);
            goto cleanup;
        }

        /* Get .CoInstallers section name */
        Result = SetupDiGetActualSectionToInstallW(
            SelectedDriver->InfFileDetails->hInf,
            SelectedDriver->Details.SectionName,
            SectionName, MAX_PATH, &SectionNameLength, NULL);
        if (!Result || SectionNameLength > MAX_PATH - strlenW(DotCoInstallers) - 1)
            goto cleanup;
        lstrcatW(SectionName, DotCoInstallers);

        deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

        /* Open/Create driver key information */
#if _WIN32_WINNT >= 0x502
        hKey = SETUPDI_OpenDrvKey(set->HKLM, deviceInfo, KEY_READ | KEY_WRITE);
#else
        hKey = SETUPDI_OpenDrvKey(set->HKLM, deviceInfo, KEY_ALL_ACCESS);
#endif
        if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
#if _WIN32_WINNT >= 0x502
            hKey = SETUPDI_CreateDrvKey(set->HKLM, deviceInfo, (UUID*)&DeviceInfoData->ClassGuid, KEY_READ | KEY_WRITE);
#else
            hKey = SETUPDI_CreateDrvKey(set->HKLM, deviceInfo, (UUID*)&DeviceInfoData->ClassGuid, KEY_ALL_ACCESS);
#endif
        if (hKey == INVALID_HANDLE_VALUE)
            goto cleanup;

        /* Install .CoInstallers section */
        DoAction = SPINST_REGISTRY;
        if (!(InstallParams.Flags & DI_NOFILECOPY))
        {
            DoAction |= SPINST_FILES;
            Context = SetupInitDefaultQueueCallback(InstallParams.hwndParent);
            if (!Context)
                goto cleanup;
        }
        Result = SetupInstallFromInfSectionW(InstallParams.hwndParent,
            SelectedDriver->InfFileDetails->hInf, SectionName,
            DoAction, hKey, SelectedDriver->InfFileDetails->DirectoryName, SP_COPY_NEWER,
            SetupDefaultQueueCallbackW, Context,
            DeviceInfoSet, DeviceInfoData);
        if (!Result)
            goto cleanup;

        ret = TRUE;

cleanup:
        if (Context)
            SetupTermDefaultQueueCallback(Context);
        if (hKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hKey);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static BOOL
InfIsFromOEMLocation(
        IN PCWSTR FullName,
        OUT LPBOOL IsOEMLocation)
{
    PWCHAR last;

    last = strrchrW(FullName, '\\');
    if (!last)
    {
        /* No directory specified */
        *IsOEMLocation = FALSE;
    }
    else
    {
        LPWSTR Windir;
        UINT ret;

        Windir = MyMalloc((MAX_PATH + 1 + strlenW(InfDirectory)) * sizeof(WCHAR));
        if (!Windir)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        ret = GetSystemWindowsDirectoryW(Windir, MAX_PATH);
        if (ret == 0 || ret > MAX_PATH)
        {
            MyFree(Windir);
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if (*Windir && Windir[strlenW(Windir) - 1] != '\\')
            strcatW(Windir, BackSlash);
        strcatW(Windir, InfDirectory);

        if (strncmpiW(FullName, Windir, last - FullName) == 0)
        {
            /* The path is %SYSTEMROOT%\Inf */
            *IsOEMLocation = FALSE;
        }
        else
        {
            /* The file is in another place */
            *IsOEMLocation = TRUE;
        }
        MyFree(Windir);
    }
    return TRUE;
}

/***********************************************************************
 *		SetupDiInstallDevice (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiInstallDevice(
        IN HDEVINFO DeviceInfoSet,
        IN OUT PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *deviceInfo;
    SP_DEVINSTALL_PARAMS_W InstallParams;
    struct DriverInfoElement *SelectedDriver;
    SYSTEMTIME DriverDate;
    WCHAR SectionName[MAX_PATH];
    WCHAR Buffer[32];
    DWORD SectionNameLength = 0;
    BOOL Result = FALSE;
    ULONG DoAction;
    DWORD RequiredSize;
    LPWSTR pSectionName = NULL;
    WCHAR ClassName[MAX_CLASS_NAME_LEN];
    GUID ClassGuid;
    LPWSTR lpGuidString = NULL, lpFullGuidString = NULL;
    BOOL RebootRequired = FALSE;
    HKEY hKey = INVALID_HANDLE_VALUE;
    BOOL NeedtoCopyFile;
    LARGE_INTEGER fullVersion;
    LONG rc;
    PVOID Context = NULL;
    BOOL ret = FALSE; /* Return value */

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData && DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
        Result = TRUE;

    if (!Result)
    {
        /* One parameter is bad */
        goto cleanup;
    }

    InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
    Result = SetupDiGetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);
    if (!Result)
        goto cleanup;

    if (InstallParams.FlagsEx & DI_FLAGSEX_SETFAILEDINSTALL)
    {
        /* Set FAILEDINSTALL in ConfigFlags registry value */
        DWORD ConfigFlags, regType;
        Result = SetupDiGetDeviceRegistryPropertyW(
            DeviceInfoSet,
            DeviceInfoData,
            SPDRP_CONFIGFLAGS,
            &regType,
            (PBYTE)&ConfigFlags,
            sizeof(ConfigFlags),
            NULL);
        if (!Result || regType != REG_DWORD)
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }
        ConfigFlags |= CONFIGFLAG_FAILEDINSTALL;
        Result = SetupDiSetDeviceRegistryPropertyW(
            DeviceInfoSet,
            DeviceInfoData,
            SPDRP_CONFIGFLAGS,
            (PBYTE)&ConfigFlags,
            sizeof(ConfigFlags));
        if (!Result)
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }

        ret = TRUE;
        goto cleanup;
    }

    SelectedDriver = (struct DriverInfoElement *)InstallParams.ClassInstallReserved;
    if (SelectedDriver == NULL)
    {
        SetLastError(ERROR_NO_DRIVER_SELECTED);
        goto cleanup;
    }

    FileTimeToSystemTime(&SelectedDriver->Info.DriverDate, &DriverDate);

    Result = SetupDiGetActualSectionToInstallW(
        SelectedDriver->InfFileDetails->hInf,
        SelectedDriver->Details.SectionName,
        SectionName, MAX_PATH, &SectionNameLength, NULL);
    if (!Result || SectionNameLength > MAX_PATH - strlenW(DotServices))
        goto cleanup;
    pSectionName = &SectionName[strlenW(SectionName)];

    /* Get information from [Version] section */
    if (!SetupDiGetINFClassW(SelectedDriver->Details.InfFileName, &ClassGuid, ClassName, MAX_CLASS_NAME_LEN, &RequiredSize))
        goto cleanup;
    /* Format ClassGuid to a string */
    if (UuidToStringW((UUID*)&ClassGuid, &lpGuidString) != RPC_S_OK)
        goto cleanup;
    RequiredSize = lstrlenW(lpGuidString);
    lpFullGuidString = HeapAlloc(GetProcessHeap(), 0, (RequiredSize + 3) * sizeof(WCHAR));
    if (!lpFullGuidString)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    lpFullGuidString[0] = '{';
    memcpy(&lpFullGuidString[1], lpGuidString, RequiredSize * sizeof(WCHAR));
    lpFullGuidString[RequiredSize + 1] = '}';
    lpFullGuidString[RequiredSize + 2] = '\0';

    /* Copy .inf file to Inf\ directory (if needed) */
    Result = InfIsFromOEMLocation(SelectedDriver->Details.InfFileName, &NeedtoCopyFile);
    if (!Result)
        goto cleanup;
    if (NeedtoCopyFile)
    {
        WCHAR NewFileName[MAX_PATH];
        struct InfFileDetails *newInfFileDetails;
        Result = SetupCopyOEMInfW(
            SelectedDriver->Details.InfFileName,
            NULL,
            SPOST_NONE,
            SP_COPY_NOOVERWRITE,
            NewFileName, MAX_PATH,
            NULL,
            NULL);
        if (!Result && GetLastError() != ERROR_FILE_EXISTS)
            goto cleanup;
        /* Create a new struct InfFileDetails, and set it to
         * SelectedDriver->InfFileDetails, to release use of
         * current InfFile */
        newInfFileDetails = CreateInfFileDetails(NewFileName);
        if (!newInfFileDetails)
            goto cleanup;
        DereferenceInfFile(SelectedDriver->InfFileDetails);
        SelectedDriver->InfFileDetails = newInfFileDetails;
        strcpyW(SelectedDriver->Details.InfFileName, NewFileName);
    }

    deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

    /* Open/Create driver key information */
#if _WIN32_WINNT >= 0x502
    hKey = SETUPDI_OpenDrvKey(set->HKLM, deviceInfo, KEY_READ | KEY_WRITE);
#else
    hKey = SETUPDI_OpenDrvKey(set->HKLM, deviceInfo, KEY_ALL_ACCESS);
#endif
    if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
#if _WIN32_WINNT >= 0x502
        hKey = SETUPDI_CreateDrvKey(set->HKLM, deviceInfo, (UUID*)&DeviceInfoData->ClassGuid, KEY_READ | KEY_WRITE);
#else
        hKey = SETUPDI_CreateDrvKey(set->HKLM, deviceInfo, (UUID*)&DeviceInfoData->ClassGuid, KEY_ALL_ACCESS);
#endif
    if (hKey == INVALID_HANDLE_VALUE)
        goto cleanup;

    /* Install main section */
    DoAction = 0;
    if (!(InstallParams.FlagsEx & DI_FLAGSEX_NO_DRVREG_MODIFY))
        DoAction |= SPINST_REGISTRY;
    if (!(InstallParams.Flags & DI_NOFILECOPY))
    {
        DoAction |= SPINST_FILES;
        Context = SetupInitDefaultQueueCallback(InstallParams.hwndParent);
        if (!Context)
            goto cleanup;
    }
    *pSectionName = '\0';
    Result = SetupInstallFromInfSectionW(InstallParams.hwndParent,
        SelectedDriver->InfFileDetails->hInf, SectionName,
        DoAction, hKey, SelectedDriver->InfFileDetails->DirectoryName, SP_COPY_NEWER,
        SetupDefaultQueueCallbackW, Context,
        DeviceInfoSet, DeviceInfoData);
    if (!Result)
        goto cleanup;
    InstallParams.Flags |= DI_NOFILECOPY;
    SetupDiSetDeviceInstallParamsW(DeviceInfoSet, DeviceInfoData, &InstallParams);

    /* Write information to driver key */
    *pSectionName = UNICODE_NULL;
    memcpy(&fullVersion, &SelectedDriver->Info.DriverVersion, sizeof(LARGE_INTEGER));
    TRACE("Write information to driver key\n");
    TRACE("DriverDate      : '%u-%u-%u'\n", DriverDate.wMonth, DriverDate.wDay, DriverDate.wYear);
    TRACE("DriverDesc      : '%s'\n", debugstr_w(SelectedDriver->Info.Description));
    TRACE("DriverVersion   : '%ld.%ld.%lu.%ld'\n", fullVersion.HighPart >> 16, fullVersion.HighPart & 0xffff, fullVersion.LowPart >> 16, fullVersion.LowPart & 0xffff);
    TRACE("InfPath         : '%s'\n", debugstr_w(SelectedDriver->InfFileDetails->FileName));
    TRACE("InfSection      : '%s'\n", debugstr_w(SelectedDriver->Details.SectionName));
    TRACE("InfSectionExt   : '%s'\n", debugstr_w(&SectionName[strlenW(SelectedDriver->Details.SectionName)]));
    TRACE("MatchingDeviceId: '%s'\n", debugstr_w(SelectedDriver->MatchingId));
    TRACE("ProviderName    : '%s'\n", debugstr_w(SelectedDriver->Info.ProviderName));
    sprintfW(Buffer, DateFormat, DriverDate.wMonth, DriverDate.wDay, DriverDate.wYear);
    rc = RegSetValueExW(hKey, REGSTR_DRIVER_DATE, 0, REG_SZ, (const BYTE *)Buffer, (strlenW(Buffer) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_DRIVER_DATE_DATA, 0, REG_BINARY, (const BYTE *)&SelectedDriver->Info.DriverDate, sizeof(FILETIME));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_DRVDESC, 0, REG_SZ, (const BYTE *)SelectedDriver->Info.Description, (strlenW(SelectedDriver->Info.Description) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
    {
        sprintfW(Buffer, VersionFormat, fullVersion.HighPart >> 16, fullVersion.HighPart & 0xffff, fullVersion.LowPart >> 16, fullVersion.LowPart & 0xffff);
        rc = RegSetValueExW(hKey, REGSTR_DRIVER_VERSION, 0, REG_SZ, (const BYTE *)Buffer, (strlenW(Buffer) + 1) * sizeof(WCHAR));
    }
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_INFPATH, 0, REG_SZ, (const BYTE *)SelectedDriver->InfFileDetails->FileName, (strlenW(SelectedDriver->InfFileDetails->FileName) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_INFSECTION, 0, REG_SZ, (const BYTE *)SelectedDriver->Details.SectionName, (strlenW(SelectedDriver->Details.SectionName) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_INFSECTIONEXT, 0, REG_SZ, (const BYTE *)&SectionName[strlenW(SelectedDriver->Details.SectionName)], (strlenW(SectionName) - strlenW(SelectedDriver->Details.SectionName) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_MATCHINGDEVID, 0, REG_SZ, (const BYTE *)SelectedDriver->MatchingId, (strlenW(SelectedDriver->MatchingId) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_PROVIDER_NAME, 0, REG_SZ, (const BYTE *)SelectedDriver->Info.ProviderName, (strlenW(SelectedDriver->Info.ProviderName) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
       SetLastError(rc);
       goto cleanup;
    }
    RegCloseKey(hKey);
    hKey = INVALID_HANDLE_VALUE;

    /* FIXME: Process .LogConfigOverride section */

    /* Install .Services section */
    strcpyW(pSectionName, DotServices);
    Result = SetupInstallServicesFromInfSectionExW(
        SelectedDriver->InfFileDetails->hInf,
        SectionName,
        0,
        DeviceInfoSet,
        DeviceInfoData,
        NULL,
        NULL);
    if (!Result)
    {
        if (GetLastError() != ERROR_SECTION_NOT_FOUND)
            goto cleanup;
        SetLastError(ERROR_SUCCESS);
    }
    if (GetLastError() == ERROR_SUCCESS_REBOOT_REQUIRED)
        RebootRequired = TRUE;

    /* Open device registry key */
    hKey = SETUPDI_OpenDevKey(((struct DeviceInfoSet *)DeviceInfoSet)->HKLM, (struct DeviceInfo *)DeviceInfoData->Reserved, KEY_SET_VALUE);
    if (hKey == INVALID_HANDLE_VALUE)
        goto cleanup;

    /* Install .HW section */
    DoAction = 0;
    if (!(InstallParams.FlagsEx & DI_FLAGSEX_NO_DRVREG_MODIFY))
        DoAction |= SPINST_REGISTRY;
    strcpyW(pSectionName, DotHW);
    Result = SetupInstallFromInfSectionW(InstallParams.hwndParent,
        SelectedDriver->InfFileDetails->hInf, SectionName,
        DoAction, hKey, NULL, 0,
        NULL, NULL,
        DeviceInfoSet, DeviceInfoData);
    if (!Result)
        goto cleanup;

    /* Write information to enum key */
    TRACE("Write information to enum key\n");
    TRACE("Class           : '%s'\n", debugstr_w(ClassName));
    TRACE("ClassGUID       : '%s'\n", debugstr_w(lpFullGuidString));
    TRACE("DeviceDesc      : '%s'\n", debugstr_w(SelectedDriver->Info.Description));
    TRACE("Mfg             : '%s'\n", debugstr_w(SelectedDriver->Info.MfgName));
    rc = RegSetValueExW(hKey, REGSTR_VAL_CLASS, 0, REG_SZ, (const BYTE *)ClassName, (strlenW(ClassName) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_CLASSGUID, 0, REG_SZ, (const BYTE *)lpFullGuidString, (strlenW(lpFullGuidString) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_DEVDESC, 0, REG_SZ, (const BYTE *)SelectedDriver->Info.Description, (strlenW(SelectedDriver->Info.Description) + 1) * sizeof(WCHAR));
    if (rc == ERROR_SUCCESS)
        rc = RegSetValueExW(hKey, REGSTR_VAL_MFG, 0, REG_SZ, (const BYTE *)SelectedDriver->Info.MfgName, (strlenW(SelectedDriver->Info.MfgName) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
       SetLastError(rc);
       goto cleanup;
    }

    /* Start the device */
    if (!RebootRequired && !(InstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT | DI_DONOTCALLCONFIGMG)))
        ret = ResetDevice(DeviceInfoSet, DeviceInfoData);
    else
        ret = TRUE;

cleanup:
    /* End of installation */
    if (hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);
    if (lpGuidString)
        RpcStringFreeW(&lpGuidString);
    HeapFree(GetProcessHeap(), 0, lpFullGuidString);
    if (Context)
        SetupTermDefaultQueueCallback(Context);
    TRACE("Returning %d\n", ret);
    return ret;
}

HKEY SETUPDI_CreateDevKey(HKEY RootKey, struct DeviceInfo *devInfo, REGSAM samDesired)
{
    HKEY enumKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    l = RegCreateKeyExW(RootKey, REGSTR_PATH_SYSTEMENUM, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &enumKey, NULL);
    if (!l)
    {
        l = RegCreateKeyExW(enumKey, devInfo->instanceId, 0, NULL, REG_OPTION_NON_VOLATILE, samDesired, NULL, &key, NULL);
        RegCloseKey(enumKey);
    }
    if (l)
        SetLastError(l);
    return key;
}

HKEY SETUPDI_CreateDrvKey(HKEY RootKey, struct DeviceInfo *devInfo, UUID *ClassGuid, REGSAM samDesired)
{
    HKEY key = INVALID_HANDLE_VALUE;
    LPWSTR lpGuidString = NULL;
    LPWSTR DriverKey = NULL; /* {GUID}\Index */
    LPWSTR pDeviceInstance; /* Points into DriverKey, on the Index field */
    DWORD Index; /* Index used in the DriverKey name */
    DWORD dwSize;
    DWORD Disposition;
    DWORD rc;
    HKEY hHWProfileKey = INVALID_HANDLE_VALUE;
    HKEY hEnumKey = NULL;
    HKEY hClassKey = NULL;
    HKEY hDeviceKey = INVALID_HANDLE_VALUE;
    HKEY hKey = NULL;

    /* Open device key, to read Driver value */
    hDeviceKey = SETUPDI_OpenDevKey(RootKey, devInfo, KEY_QUERY_VALUE | KEY_SET_VALUE);
    if (hDeviceKey == INVALID_HANDLE_VALUE)
        goto cleanup;

    rc = RegOpenKeyExW(RootKey, REGSTR_PATH_CLASS_NT, 0, KEY_CREATE_SUB_KEY, &hClassKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }

    rc = RegQueryValueExW(hDeviceKey, REGSTR_VAL_DRIVER, NULL, NULL, NULL, &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        /* Create a new driver key */

        if (UuidToStringW(ClassGuid, &lpGuidString) != RPC_S_OK)
            goto cleanup;

        /* The driver key is in \System\CurrentControlSet\Control\Class\{GUID}\Index */
        DriverKey = HeapAlloc(GetProcessHeap(), 0, (strlenW(lpGuidString) + 7) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
        if (!DriverKey)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }

        DriverKey[0] = '{';
        strcpyW(&DriverKey[1], lpGuidString);
        pDeviceInstance = &DriverKey[strlenW(DriverKey)];
        *pDeviceInstance++ = '}';
        *pDeviceInstance++ = '\\';

        /* Try all values for Index between 0 and 9999 */
        Index = 0;
        while (Index <= 9999)
        {
            sprintfW(pDeviceInstance, InstanceKeyFormat, Index);
            rc = RegCreateKeyExW(hClassKey,
                DriverKey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
#if _WIN32_WINNT >= 0x502
                KEY_READ | KEY_WRITE,
#else
                KEY_ALL_ACCESS,
#endif
                NULL,
                &hKey,
                &Disposition);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            if (Disposition == REG_CREATED_NEW_KEY)
                break;
            RegCloseKey(hKey);
            hKey = NULL;
            Index++;
        }

        if (Index > 9999)
        {
            /* Unable to create more than 9999 devices within the same class */
            SetLastError(ERROR_GEN_FAILURE);
            goto cleanup;
        }

        /* Write the new Driver value */
        rc = RegSetValueExW(hDeviceKey, REGSTR_VAL_DRIVER, 0, REG_SZ, (const BYTE *)DriverKey, (strlenW(DriverKey) + 1) * sizeof(WCHAR));
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
    }
    else
    {
        /* Open the existing driver key */

        DriverKey = HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (!DriverKey)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }

        rc = RegQueryValueExW(hDeviceKey, REGSTR_VAL_DRIVER, NULL, NULL, (LPBYTE)DriverKey, &dwSize);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }

        rc = RegCreateKeyExW(hClassKey,
            DriverKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
#if _WIN32_WINNT >= 0x502
            KEY_READ | KEY_WRITE,
#else
            KEY_ALL_ACCESS,
#endif
            NULL,
            &hKey,
            &Disposition);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
    }

    key = hKey;

cleanup:
        if (lpGuidString)
            RpcStringFreeW(&lpGuidString);
        HeapFree(GetProcessHeap(), 0, DriverKey);
        if (hHWProfileKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hHWProfileKey);
        if (hEnumKey != NULL)
            RegCloseKey(hEnumKey);
        if (hClassKey != NULL)
            RegCloseKey(hClassKey);
        if (hDeviceKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hDeviceKey);
        if (hKey != NULL && hKey != key)
            RegCloseKey(hKey);

    TRACE("Returning 0x%p\n", hKey);
    return hKey;
}

HKEY SETUPDI_OpenDevKey(HKEY RootKey, struct DeviceInfo *devInfo, REGSAM samDesired)
{
    HKEY enumKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    l = RegOpenKeyExW(RootKey, REGSTR_PATH_SYSTEMENUM, 0, READ_CONTROL, &enumKey);
    if (!l)
    {
        l = RegOpenKeyExW(enumKey, devInfo->instanceId, 0, samDesired, &key);
        RegCloseKey(enumKey);
    }
    if (l)
        SetLastError(l);
    return key;
}

HKEY SETUPDI_OpenDrvKey(HKEY RootKey, struct DeviceInfo *devInfo, REGSAM samDesired)
{
    LPWSTR DriverKey = NULL;
    DWORD dwLength = 0;
    DWORD dwRegType;
    DWORD rc;
    HKEY hEnumKey = NULL;
    HKEY hKey = NULL;
    HKEY key = INVALID_HANDLE_VALUE;

    hKey = SETUPDI_OpenDevKey(RootKey, devInfo, KEY_QUERY_VALUE);
    if (hKey == INVALID_HANDLE_VALUE)
        goto cleanup;
    /* Read the 'Driver' key */
    rc = RegQueryValueExW(hKey, REGSTR_VAL_DRIVER, NULL, &dwRegType, NULL, &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    else if (dwRegType != REG_SZ)
    {
        SetLastError(ERROR_GEN_FAILURE);
        goto cleanup;
    }
    DriverKey = HeapAlloc(GetProcessHeap(), 0, dwLength);
    if (!DriverKey)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    rc = RegQueryValueExW(hKey, REGSTR_VAL_DRIVER, NULL, &dwRegType, (LPBYTE)DriverKey, &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    RegCloseKey(hKey);
    hKey = NULL;
    /* Need to open the driver key */
    rc = RegOpenKeyExW(
        RootKey,
        REGSTR_PATH_CLASS_NT,
        0, /* Options */
        READ_CONTROL,
        &hEnumKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    rc = RegOpenKeyExW(
        hEnumKey,
        DriverKey,
        0, /* Options */
        samDesired,
        &hKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    key = hKey;

cleanup:
    if (hEnumKey != NULL)
        RegCloseKey(hEnumKey);
    if (hKey != NULL && hKey != key)
        RegCloseKey(hKey);
    if (DriverKey)
        HeapFree(GetProcessHeap(), 0, DriverKey);
    return key;
}

/***********************************************************************
 *		SetupDiOpenDevRegKey (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenDevRegKey(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Scope,
        DWORD HwProfile,
        DWORD KeyType,
        REGSAM samDesired)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct DeviceInfo *devInfo;
    HKEY key = INVALID_HANDLE_VALUE;
    HKEY RootKey;

    TRACE("%s(%p %p %d %d %d %x)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData,
          Scope, HwProfile, KeyType, samDesired);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    if (KeyType != DIREG_DEV && KeyType != DIREG_DRV)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
    {
        RootKey = OpenHardwareProfileKey(set->HKLM, HwProfile, 0);
        if (RootKey == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;
    }
    else
        RootKey = set->HKLM;
    switch (KeyType)
    {
        case DIREG_DEV:
            key = SETUPDI_OpenDevKey(RootKey, devInfo, samDesired);
            if (Scope == DICS_FLAG_GLOBAL)
            {
                LONG rc;
                HKEY hTempKey = key;
                rc = RegOpenKeyExW(hTempKey,
                                   L"Device Parameters",
                                   0,
                                   samDesired,
                                   &key);
                if (rc == ERROR_SUCCESS)
                    RegCloseKey(hTempKey);
            }
            break;
        case DIREG_DRV:
            key = SETUPDI_OpenDrvKey(RootKey, devInfo, samDesired);
            break;
        default:
            WARN("unknown KeyType %d\n", KeyType);
    }
    if (RootKey != set->HKLM)
        RegCloseKey(RootKey);
    return key;
}

static BOOL SETUPDI_DeleteDevKey(HKEY RootKey, struct DeviceInfo *devInfo)
{
    FIXME("\n");
    return FALSE;
}

static BOOL SETUPDI_DeleteDrvKey(HKEY RootKey, struct DeviceInfo *devInfo)
{
    FIXME("\n");
    return FALSE;
}

/***********************************************************************
 *		SetupDiDeleteDevRegKey (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDevRegKey(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Scope,
        DWORD HwProfile,
        DWORD KeyType)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;
    BOOL ret = FALSE;
    HKEY RootKey;

    TRACE("%s(%p %p %d %d %d)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData, Scope, HwProfile,
            KeyType);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }
    if (KeyType != DIREG_DEV && KeyType != DIREG_DRV && KeyType != DIREG_BOTH)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
    {
        RootKey = OpenHardwareProfileKey(set->HKLM, HwProfile, 0);
        if (RootKey == INVALID_HANDLE_VALUE)
            return FALSE;
    }
    else
        RootKey = set->HKLM;
    switch (KeyType)
    {
        case DIREG_DEV:
            ret = SETUPDI_DeleteDevKey(RootKey, devInfo);
            break;
        case DIREG_DRV:
            ret = SETUPDI_DeleteDrvKey(RootKey, devInfo);
            break;
        case DIREG_BOTH:
            ret = SETUPDI_DeleteDevKey(RootKey, devInfo);
            if (ret)
                ret = SETUPDI_DeleteDrvKey(RootKey, devInfo);
            break;
        default:
            WARN("unknown KeyType %d\n", KeyType);
    }
    if (RootKey != set->HKLM)
        RegCloseKey(RootKey);
    return ret;
}

/***********************************************************************
 *		SetupDiRestartDevices (SETUPAPI.@)
 */
BOOL
WINAPI
SetupDiRestartDevices(
    HDEVINFO DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;
    CONFIGRET cr;

    TRACE("%s(%p %p)\n", __FUNCTION__, DeviceInfoSet, DeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

    cr = CM_Enable_DevNode_Ex(devInfo->dnDevInst, 0, set->hMachine);
    if (cr != CR_SUCCESS)
    {
        SetLastError(GetErrorCodeFromCrCode(cr));
        return FALSE;
    }

    return TRUE;
}
