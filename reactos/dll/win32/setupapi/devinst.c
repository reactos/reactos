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

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR BackSlash[] = {'\\',0};
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR Class[]  = {'C','l','a','s','s',0};
static const WCHAR DateFormat[]  = {'%','u','-','%','u','-','%','u',0};
static const WCHAR DotCoInstallers[]  = {'.','C','o','I','n','s','t','a','l','l','e','r','s',0};
static const WCHAR DotHW[]  = {'.','H','W',0};
static const WCHAR DotServices[]  = {'.','S','e','r','v','i','c','e','s',0};
static const WCHAR InfDirectory[] = {'i','n','f','\\',0};
static const WCHAR InstanceKeyFormat[] = {'%','0','4','l','u',0};
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


/***********************************************************************
 *		SetupDiCreateDeviceInfoList (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoList(
    IN CONST GUID *ClassGuid OPTIONAL,
    IN HWND hwndParent OPTIONAL)
{
    return SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent, NULL, NULL);
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExA (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExA(
    IN CONST GUID *ClassGuid OPTIONAL,
    IN HWND hwndParent OPTIONAL,
    IN PCSTR MachineName OPTIONAL,
    IN PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    HDEVINFO hDevInfo;

    TRACE("%s %p %s %p\n", debugstr_guid(ClassGuid), hwndParent,
      debugstr_a(MachineName), Reserved);

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return (HDEVINFO)INVALID_HANDLE_VALUE;
    }

    hDevInfo = SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent,
                                              MachineNameW, Reserved);

    MyFree(MachineNameW);

    return hDevInfo;
}

static DWORD
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
    case CR_SUCCESS:              return ERROR_SUCCESS;
    default:                      return ERROR_GEN_FAILURE;
  }

  /* Does not happen */
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExW (SETUPAPI.@)
 *
 * Create an empty DeviceInfoSet list.
 *
 * PARAMS
 *   ClassGuid [I] if not NULL only devices with GUID ClcassGuid are associated
 *                 with this list.
 *   hwndParent [I] hwnd needed for interface related actions.
 *   MachineName [I] name of machine to create emtpy DeviceInfoSet list, if NULL
 *                   local regestry will be used.
 *   Reserved [I] must be NULL
 *
 * RETURNS
 *   Success: empty list.
 *   Failure: INVALID_HANDLE_VALUE.
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExW(
    IN CONST GUID *ClassGuid OPTIONAL,
    IN HWND hwndParent OPTIONAL,
    IN PCWSTR MachineName OPTIONAL,
    IN PVOID Reserved)
{
    struct DeviceInfoSet *list = NULL;
    DWORD size;
    DWORD rc;
    CONFIGRET cr;
    HDEVINFO ret = (HDEVINFO)INVALID_HANDLE_VALUE;;

    TRACE("%s %p %s %p\n", debugstr_guid(ClassGuid), hwndParent,
        debugstr_w(MachineName), Reserved);

    if (Reserved != NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    size = FIELD_OFFSET(struct DeviceInfoSet, szData);
    if (MachineName)
        size += (strlenW(MachineName) + 3) * sizeof(WCHAR);
    list = MyMalloc(size);
    if (!list)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    ZeroMemory(list, FIELD_OFFSET(struct DeviceInfoSet, szData));

    list->magic = SETUP_DEVICE_INFO_SET_MAGIC;
    memcpy(
        &list->ClassGuid,
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

    ret = (HDEVINFO)list;

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
 *		SetupDiEnumDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiEnumDeviceInfo(
    IN HDEVINFO DeviceInfoSet,
    IN DWORD MemberIndex,
    OUT PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;

    TRACE("%p, 0x%08lx, %p\n", DeviceInfoSet, MemberIndex, DeviceInfoData);
    if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoSet && DeviceInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)DeviceInfoSet;

        if (list->magic != SETUP_DEVICE_INFO_SET_MAGIC)
            SetLastError(ERROR_INVALID_HANDLE);
        else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
            SetLastError(ERROR_INVALID_USER_BUFFER);
        else
        {
            PLIST_ENTRY ItemList = list->ListHead.Flink;
            while (ItemList != &list->ListHead && MemberIndex-- > 0)
                ItemList = ItemList->Flink;
            if (ItemList == &list->ListHead)
                SetLastError(ERROR_NO_MORE_ITEMS);
            else
            {
                struct DeviceInfo *DevInfo = CONTAINING_RECORD(ItemList, struct DeviceInfo, ListEntry);
                memcpy(&DeviceInfoData->ClassGuid,
                    &DevInfo->ClassGuid,
                    sizeof(GUID));
                DeviceInfoData->DevInst = DevInfo->dnDevInst;
                DeviceInfoData->Reserved = (ULONG_PTR)DevInfo;
                ret = TRUE;
            }
        }
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetActualSectionToInstallA(
    IN HINF InfHandle,
    IN PCSTR InfSectionName,
    OUT PSTR InfSectionWithExt OPTIONAL,
    IN DWORD InfSectionWithExtSize,
    OUT PDWORD RequiredSize OPTIONAL,
    OUT PSTR *Extension OPTIONAL)
{
    return SetupDiGetActualSectionToInstallExA(InfHandle, InfSectionName,
        NULL, InfSectionWithExt, InfSectionWithExtSize, RequiredSize,
        Extension, NULL);
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetActualSectionToInstallW(
    IN HINF InfHandle,
    IN PCWSTR InfSectionName,
    OUT PWSTR InfSectionWithExt OPTIONAL,
    IN DWORD InfSectionWithExtSize,
    OUT PDWORD RequiredSize OPTIONAL,
    OUT PWSTR *Extension OPTIONAL)
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

    TRACE("\n");

    if (InfSectionName)
    {
        InfSectionNameW = MultiByteToUnicode(InfSectionName, CP_ACP);
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
    LPCWSTR pExtensionPlatform, pExtensionArchitecture;
    LPWSTR Fields[6];
    DWORD i;
    BOOL ret = FALSE;

    static const WCHAR ExtensionPlatformNone[]  = {'.',0};
    static const WCHAR ExtensionPlatformNT[]  = {'.','N','T',0};
    static const WCHAR ExtensionPlatformWindows[]  = {'.','W','i','n',0};

    static const WCHAR ExtensionArchitectureNone[]  = {0};
    static const WCHAR ExtensionArchitecturealpha[]  = {'a','l','p','h','a',0};
    static const WCHAR ExtensionArchitectureamd64[]  = {'A','M','D','6','4',0};
    static const WCHAR ExtensionArchitectureia64[]  = {'I','A','6','4',0};
    static const WCHAR ExtensionArchitecturemips[]  = {'m','i','p','s',0};
    static const WCHAR ExtensionArchitectureppc[]  = {'p','p','c',0};
    static const WCHAR ExtensionArchitecturex86[]  = {'x','8','6',0};

    TRACE("%s %p 0x%x 0x%x\n",
        debugstr_w(SectionName), PlatformInfo, ProductType, SuiteMask);

    *ScorePlatform = *ScoreMajorVersion = *ScoreMinorVersion = *ScoreProductType = *ScoreSuiteMask = 0;

    Section = DuplicateString(SectionName);
    if (!Section)
    {
        TRACE("DuplicateString() failed\n");
        goto cleanup;
    }

    /* Set various extensions values */
    switch (PlatformInfo->Platform)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            pExtensionPlatform = ExtensionPlatformWindows;
            break;
        case VER_PLATFORM_WIN32_NT:
            pExtensionPlatform = ExtensionPlatformNT;
            break;
        default:
            ERR("Unkown platform 0x%lx\n", PlatformInfo->Platform);
            pExtensionPlatform = ExtensionPlatformNone;
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
     * Remark: lastests fields may be NULL if the information is not provided
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

    TRACE("%p %s %p %p %lu %p %p %p\n", InfHandle, debugstr_w(InfSectionName),
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
                OSVERSIONINFOEX VersionInfo;
                SYSTEM_INFO SystemInfo;
                VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
                ret = GetVersionExW((OSVERSIONINFO*)&VersionInfo);
                if (!ret)
                    goto done;
                GetSystemInfo(&SystemInfo);
                CurrentPlatform.cbSize = sizeof(SP_ALTPLATFORM_INFO);
                CurrentPlatform.Platform = VersionInfo.dwPlatformId;
                CurrentPlatform.MajorVersion = VersionInfo.dwMajorVersion;
                CurrentPlatform.MinorVersion = VersionInfo.dwMinorVersion;
                CurrentPlatform.ProcessorArchitecture = SystemInfo.wProcessorArchitecture;
                CurrentPlatform.Reserved = 0;
                CurrentProductType = VersionInfo.wProductType;
                CurrentSuiteMask = VersionInfo.wSuiteMask;
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
        if (!EnumerateSectionsStartingWith(
            InfHandle,
            InfSectionName,
            GetSectionCallback,
            &CallbackInfo))
        {
            SetLastError(ERROR_GEN_FAILURE);
            goto done;
        }

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
    deviceInfo->DeviceName = deviceInfo->Data;
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
    return HeapFree(GetProcessHeap(), 0, list);
}

/***********************************************************************
 *		SetupDiDestroyDeviceInfoList (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiDestroyDeviceInfoList(
    IN HDEVINFO DeviceInfoSet)
{
    BOOL ret = FALSE;

    TRACE("%p\n", DeviceInfoSet);
    if (DeviceInfoSet && DeviceInfoSet != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)DeviceInfoSet;

        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
            ret = DestroyDeviceInfoSet(list);
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceRegistryPropertyA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    OUT PDWORD PropertyRegDataType OPTIONAL,
    OUT PBYTE PropertyBuffer OPTIONAL,
    IN DWORD PropertyBufferSize,
    OUT PDWORD  RequiredSize OPTIONAL)
{
    BOOL bResult;
    BOOL bIsStringProperty;
    DWORD RegType;
    DWORD RequiredSizeA, RequiredSizeW;
    DWORD PropertyBufferSizeW = 0;
    PBYTE PropertyBufferW = NULL;

    TRACE("%p %p %ld %p %p %ld %p\n", DeviceInfoSet, DeviceInfoData,
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

    bResult = SetupDiGetDeviceRegistryPropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        Property,
        &RegType,
        PropertyBufferW,
        PropertyBufferSizeW,
        &RequiredSizeW);

    if (bResult || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
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

    if (!bResult)
    {
        HeapFree(GetProcessHeap(), 0, PropertyBufferW);
        return bResult;
    }

    if (RequiredSizeA <= PropertyBufferSize)
    {
        if (bIsStringProperty && PropertyBufferSize > 0)
        {
            if (WideCharToMultiByte(CP_ACP, 0, (LPWSTR)PropertyBufferW, RequiredSizeW / sizeof(WCHAR), (LPSTR)PropertyBuffer, PropertyBufferSize, NULL, NULL) == 0)
            {
                /* Last error is already set by WideCharToMultiByte */
                bResult = FALSE;
            }
        }
        else
            memcpy(PropertyBuffer, PropertyBufferW, RequiredSizeA);
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        bResult = FALSE;
    }

    HeapFree(GetProcessHeap(), 0, PropertyBufferW);
    return bResult;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceRegistryPropertyW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    OUT PDWORD PropertyRegDataType OPTIONAL,
    OUT PBYTE PropertyBuffer OPTIONAL,
    IN DWORD PropertyBufferSize,
    OUT PDWORD  RequiredSize OPTIONAL)
{
    HKEY hEnumKey, hKey;
    DWORD rc;
    BOOL ret = FALSE;

    TRACE("%p %p %ld %p %p %ld %p\n", DeviceInfoSet, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);

    if (!DeviceInfoSet || DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (Property >= SPDRP_MAXIMUM_PROPERTY)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)DeviceInfoSet;
        struct DeviceInfo *DevInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

        switch (Property)
        {
            case SPDRP_CAPABILITIES:
            case SPDRP_CLASS:
            case SPDRP_CLASSGUID:
            case SPDRP_COMPATIBLEIDS:
            case SPDRP_CONFIGFLAGS:
            case SPDRP_DEVICEDESC:
            case SPDRP_DRIVER:
            case SPDRP_FRIENDLYNAME:
            case SPDRP_HARDWAREID:
            case SPDRP_LOCATION_INFORMATION:
            case SPDRP_LOWERFILTERS:
            case SPDRP_MFG:
            case SPDRP_SECURITY:
            case SPDRP_SERVICE:
            case SPDRP_UI_NUMBER:
            case SPDRP_UI_NUMBER_DESC_FORMAT:
            case SPDRP_UPPERFILTERS:
            {
                LPCWSTR RegistryPropertyName;
                DWORD BufferSize;

                switch (Property)
                {
                    case SPDRP_CAPABILITIES:
                        RegistryPropertyName = REGSTR_VAL_CAPABILITIES; break;
                    case SPDRP_CLASS:
                        RegistryPropertyName = REGSTR_VAL_CLASS; break;
                    case SPDRP_CLASSGUID:
                        RegistryPropertyName = REGSTR_VAL_CLASSGUID; break;
                    case SPDRP_COMPATIBLEIDS:
                        RegistryPropertyName = REGSTR_VAL_COMPATIBLEIDS; break;
                    case SPDRP_CONFIGFLAGS:
                        RegistryPropertyName = REGSTR_VAL_CONFIGFLAGS; break;
                    case SPDRP_DEVICEDESC:
                        RegistryPropertyName = REGSTR_VAL_DEVDESC; break;
                    case SPDRP_DRIVER:
                        RegistryPropertyName = REGSTR_VAL_DRIVER; break;
                    case SPDRP_FRIENDLYNAME:
                        RegistryPropertyName = REGSTR_VAL_FRIENDLYNAME; break;
                    case SPDRP_HARDWAREID:
                        RegistryPropertyName = REGSTR_VAL_HARDWAREID; break;
                    case SPDRP_LOCATION_INFORMATION:
                        RegistryPropertyName = REGSTR_VAL_LOCATION_INFORMATION; break;
                    case SPDRP_LOWERFILTERS:
                        RegistryPropertyName = REGSTR_VAL_LOWERFILTERS; break;
                    case SPDRP_MFG:
                        RegistryPropertyName = REGSTR_VAL_MFG; break;
                    case SPDRP_SECURITY:
                        RegistryPropertyName = REGSTR_SECURITY; break;
                    case SPDRP_SERVICE:
                        RegistryPropertyName = REGSTR_VAL_SERVICE; break;
                    case SPDRP_UI_NUMBER:
                        RegistryPropertyName = REGSTR_VAL_UI_NUMBER; break;
                    case SPDRP_UI_NUMBER_DESC_FORMAT:
                        RegistryPropertyName = REGSTR_UI_NUMBER_DESC_FORMAT; break;
                    case SPDRP_UPPERFILTERS:
                        RegistryPropertyName = REGSTR_VAL_UPPERFILTERS; break;
                    default:
                        /* Should not happen */
                        RegistryPropertyName = NULL; break;
                }

                /* Open registry key name */
                rc = RegOpenKeyExW(
                    list->HKLM,
                    REGSTR_PATH_SYSTEMENUM,
                    0, /* Options */
                    0,
                    &hEnumKey);
                if (rc != ERROR_SUCCESS)
                {
                    SetLastError(rc);
                    break;
                }
                rc = RegOpenKeyExW(
                    hEnumKey,
                    DevInfo->Data,
                    0, /* Options */
                    KEY_QUERY_VALUE,
                    &hKey);
                RegCloseKey(hEnumKey);
                if (rc != ERROR_SUCCESS)
                {
                    SetLastError(rc);
                    break;
                }
                /* Read registry entry */
                BufferSize = PropertyBufferSize;
                rc = RegQueryValueExW(
                    hKey,
                    RegistryPropertyName,
                    NULL, /* Reserved */
                    PropertyRegDataType,
                    PropertyBuffer,
                    &BufferSize);
                if (RequiredSize)
                    *RequiredSize = BufferSize;
                switch(rc) {
                    case ERROR_SUCCESS:
                        if (PropertyBuffer != NULL || BufferSize == 0)
                            ret = TRUE;
                        else
                            SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        break;
                    case ERROR_MORE_DATA:
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        break;
                    default:
                        SetLastError(rc);
                }
                RegCloseKey(hKey);
                break;
            }

            case SPDRP_PHYSICAL_DEVICE_OBJECT_NAME:
            {
                DWORD required = (strlenW(DevInfo->Data) + 1) * sizeof(WCHAR);

                if (PropertyRegDataType)
                    *PropertyRegDataType = REG_SZ;
                if (RequiredSize)
                    *RequiredSize = required;
                if (PropertyBufferSize >= required)
                {
                    strcpyW((LPWSTR)PropertyBuffer, DevInfo->Data);
                    ret = TRUE;
                }
                else
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                break;
            }

            /*case SPDRP_BUSTYPEGUID:
            case SPDRP_LEGACYBUSTYPE:
            case SPDRP_BUSNUMBER:
            case SPDRP_ENUMERATOR_NAME:
            case SPDRP_SECURITY_SDS:
            case SPDRP_DEVTYPE:
            case SPDRP_EXCLUSIVE:
            case SPDRP_CHARACTERISTICS:
            case SPDRP_ADDRESS:
            case SPDRP_DEVICE_POWER_DATA:*/
#if (WINVER >= 0x501)
            /*case SPDRP_REMOVAL_POLICY:
            case SPDRP_REMOVAL_POLICY_HW_DEFAULT:
            case SPDRP_REMOVAL_POLICY_OVERRIDE:
            case SPDRP_INSTALL_STATE:*/
#endif

            default:
            {
                ERR("Property 0x%lx not implemented\n", Property);
                SetLastError(ERROR_NOT_SUPPORTED);
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiSetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetDeviceRegistryPropertyA(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    IN CONST BYTE *PropertyBuffer OPTIONAL,
    IN DWORD PropertyBufferSize)
{
    FIXME("%p %p 0x%lx %p 0x%lx\n", DeviceInfoSet, DeviceInfoData,
        Property, PropertyBuffer, PropertyBufferSize);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		SetupDiSetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiSetDeviceRegistryPropertyW(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    IN CONST BYTE *PropertyBuffer OPTIONAL,
    IN DWORD PropertyBufferSize)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %p 0x%lx %p 0x%lx\n", DeviceInfoSet, DeviceInfoData,
        Property, PropertyBuffer, PropertyBufferSize);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        switch (Property)
        {
            case SPDRP_COMPATIBLEIDS:
            case SPDRP_CONFIGFLAGS:
            case SPDRP_FRIENDLYNAME:
            case SPDRP_HARDWAREID:
            case SPDRP_LOCATION_INFORMATION:
            case SPDRP_LOWERFILTERS:
            case SPDRP_SECURITY:
            case SPDRP_SERVICE:
            case SPDRP_UI_NUMBER_DESC_FORMAT:
            case SPDRP_UPPERFILTERS:
            {
                LPCWSTR RegistryPropertyName;
                DWORD RegistryDataType;
                HKEY hKey;
                LONG rc;

                switch (Property)
                {
                    case SPDRP_COMPATIBLEIDS:
                        RegistryPropertyName = REGSTR_VAL_COMPATIBLEIDS;
                        RegistryDataType = REG_MULTI_SZ;
                        break;
                    case SPDRP_CONFIGFLAGS:
                        RegistryPropertyName = REGSTR_VAL_CONFIGFLAGS;
                        RegistryDataType = REG_DWORD;
                        break;
                    case SPDRP_FRIENDLYNAME:
                        RegistryPropertyName = REGSTR_VAL_FRIENDLYNAME;
                        RegistryDataType = REG_SZ;
                        break;
                    case SPDRP_HARDWAREID:
                        RegistryPropertyName = REGSTR_VAL_HARDWAREID;
                        RegistryDataType = REG_MULTI_SZ;
                        break;
                    case SPDRP_LOCATION_INFORMATION:
                        RegistryPropertyName = REGSTR_VAL_LOCATION_INFORMATION;
                        RegistryDataType = REG_SZ;
                        break;
                    case SPDRP_LOWERFILTERS:
                        RegistryPropertyName = REGSTR_VAL_LOWERFILTERS;
                        RegistryDataType = REG_MULTI_SZ;
                        break;
                    case SPDRP_SECURITY:
                        RegistryPropertyName = REGSTR_SECURITY;
                        RegistryDataType = REG_BINARY;
                        break;
                    case SPDRP_SERVICE:
                        RegistryPropertyName = REGSTR_VAL_SERVICE;
                        RegistryDataType = REG_SZ;
                        break;
                    case SPDRP_UI_NUMBER_DESC_FORMAT:
                        RegistryPropertyName = REGSTR_UI_NUMBER_DESC_FORMAT;
                        RegistryDataType = REG_SZ;
                        break;
                    case SPDRP_UPPERFILTERS:
                        RegistryPropertyName = REGSTR_VAL_UPPERFILTERS;
                        RegistryDataType = REG_MULTI_SZ;
                        break;
                    default:
                        /* Should not happen */
                        RegistryPropertyName = NULL;
                        RegistryDataType = REG_BINARY;
                        break;
                }
                /* Open device registry key */
                hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_SET_VALUE);
                if (hKey != INVALID_HANDLE_VALUE)
                {
                    /* Write new data */
                    rc = RegSetValueExW(
                        hKey,
                        RegistryPropertyName,
                        0, /* Reserved */
                        RegistryDataType,
                        PropertyBuffer,
                        PropertyBufferSize);
                    if (rc == ERROR_SUCCESS)
                        ret = TRUE;
                    else
                        SetLastError(rc);
                    RegCloseKey(hKey);
                }
                break;
            }

            /*case SPDRP_CHARACTERISTICS:
            case SPDRP_DEVTYPE:
            case SPDRP_EXCLUSIVE:*/
#if (WINVER >= 0x501)
            //case SPDRP_REMOVAL_POLICY_OVERRIDE:
#endif
            //case SPDRP_SECURITY_SDS:

            default:
            {
                ERR("Property 0x%lx not implemented\n", Property);
                SetLastError(ERROR_NOT_SUPPORTED);
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
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
BOOL WINAPI
SetupDiCallClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%u %p %p\n", InstallFunction, DeviceInfoSet, DeviceInfoData);

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
                hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_QUERY_VALUE);
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
                    LPWSTR lpGuidString;
                    if (UuidToStringW((UUID*)&DeviceInfoData->ClassGuid, &lpGuidString) == RPC_S_OK)
                    {
                        rc = RegQueryValueExW(hKey, lpGuidString, NULL, &dwRegType, NULL, &dwLength);
                        if (rc == ERROR_SUCCESS && dwRegType == REG_MULTI_SZ)
                        {
                            LPWSTR KeyBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
                            if (KeyBuffer != NULL)
                            {
                                rc = RegQueryValueExW(hKey, lpGuidString, NULL, NULL, (LPBYTE)KeyBuffer, &dwLength);
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
                        RpcStringFreeW(&lpGuidString);
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
 *		SetupDiGetDeviceInfoListClass  (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInfoListClass(
    IN HDEVINFO DeviceInfoSet,
    OUT LPGUID ClassGuid)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %p\n", DeviceInfoSet, ClassGuid);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (IsEqualIID(&list->ClassGuid, &GUID_NULL))
        SetLastError(ERROR_NO_ASSOCIATED_CLASS);
    else
    {
        memcpy(&ClassGuid, &list->ClassGuid, sizeof(GUID));

        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailW  (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInfoListDetailW(
    IN HDEVINFO DeviceInfoSet,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA_W DeviceInfoListDetailData)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoListDetailData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoListDetailData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoListDetailData->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA_W))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else
    {
        memcpy(
            &DeviceInfoListDetailData->ClassGuid,
            &list->ClassGuid,
            sizeof(GUID));
        DeviceInfoListDetailData->RemoteMachineHandle = list->hMachine;
        if (list->MachineName)
            strcpyW(DeviceInfoListDetailData->RemoteMachineName, list->MachineName + 2);
        else
            DeviceInfoListDetailData->RemoteMachineName[0] = 0;

        ret = TRUE;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInstallParamsA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    OUT PSP_DEVINSTALL_PARAMS_A DeviceInstallParams)
{
    SP_DEVINSTALL_PARAMS_W deviceInstallParamsW;
    BOOL ret = FALSE;

    TRACE("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

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

    TRACE("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

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
        memcpy(DeviceInstallParams, Source, Source->cbSize);
        ret = TRUE;
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

    TRACE("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

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
 *		SetupDiGetDeviceInstanceIdA(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInstanceIdA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    OUT PSTR DeviceInstanceId OPTIONAL,
    IN DWORD DeviceInstanceIdSize,
    OUT PDWORD RequiredSize OPTIONAL)
{
    PWSTR DeviceInstanceIdW = NULL;
    BOOL ret = FALSE;

    TRACE("%p %p %p %lu %p\n", DeviceInfoSet, DeviceInfoData,
          DeviceInstanceId, DeviceInstanceIdSize, RequiredSize);

    if (!DeviceInstanceId && DeviceInstanceIdSize > 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        if (DeviceInstanceIdSize != 0)
        {
            DeviceInstanceIdW = MyMalloc(DeviceInstanceIdSize * sizeof(WCHAR));
            if (DeviceInstanceIdW == NULL)
                return FALSE;
        }

        ret = SetupDiGetDeviceInstanceIdW(DeviceInfoSet, DeviceInfoData,
                                          DeviceInstanceIdW, DeviceInstanceIdSize,
                                          RequiredSize);

        if (ret && DeviceInstanceIdW != NULL)
        {
            if (WideCharToMultiByte(CP_ACP, 0, DeviceInstanceIdW, -1,
                DeviceInstanceId, DeviceInstanceIdSize, NULL, NULL) == 0)
            {
                DeviceInstanceId[0] = '\0';
                ret = FALSE;
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdW(SETUPAPI.@)
 */
BOOL WINAPI
SetupDiGetDeviceInstanceIdW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    OUT PWSTR DeviceInstanceId OPTIONAL,
    IN DWORD DeviceInstanceIdSize,
    OUT PDWORD RequiredSize OPTIONAL)
{
    BOOL ret = FALSE;

    TRACE("%p %p %p %lu %p\n", DeviceInfoSet, DeviceInfoData,
          DeviceInstanceId, DeviceInstanceIdSize, RequiredSize);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (((struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (!DeviceInstanceId && DeviceInstanceIdSize > 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInstanceId && DeviceInstanceIdSize == 0)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct DeviceInfo *DevInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
        DWORD required;

        required = (strlenW(DevInfo->DeviceName) + 1) * sizeof(WCHAR);
        if (RequiredSize)
            *RequiredSize = required;

        if (required <= DeviceInstanceIdSize)
        {
            strcpyW(DeviceInstanceId, DevInfo->DeviceName);
            ret = TRUE;
        }
        else
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupDiCreateDevRegKeyA (SETUPAPI.@)
 */
HKEY WINAPI
SetupDiCreateDevRegKeyA(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Scope,
    IN DWORD HwProfile,
    IN DWORD KeyType,
    IN HINF InfHandle OPTIONAL,
    IN PCSTR InfSectionName OPTIONAL)
{
    PCWSTR InfSectionNameW = NULL;
    HKEY ret = INVALID_HANDLE_VALUE;

    if (InfSectionName)
    {
        InfSectionNameW = MultiByteToUnicode(InfSectionName, CP_ACP);
        if (InfSectionNameW == NULL) return INVALID_HANDLE_VALUE;
    }

    ret = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                  DeviceInfoData,
                                  Scope,
                                  HwProfile,
                                  KeyType,
                                  InfHandle,
                                  InfSectionNameW);

    if (InfSectionNameW != NULL)
        MyFree((PVOID)InfSectionNameW);

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
        0,
        &hHWProfilesKey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        goto cleanup;
    }
    if (HwProfile == 0)
    {
        rc = RegOpenKeyExW(
            hHWProfilesKey,
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
        rc = RegOpenKeyExW(
            hHWProfilesKey,
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

/***********************************************************************
 *		SetupDiCreateDevRegKeyW (SETUPAPI.@)
 */
HKEY WINAPI
SetupDiCreateDevRegKeyW(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Scope,
    IN DWORD HwProfile,
    IN DWORD KeyType,
    IN HINF InfHandle OPTIONAL,
    IN PCWSTR InfSectionName OPTIONAL)
{
    struct DeviceInfoSet *list;
    HKEY ret = INVALID_HANDLE_VALUE;

    TRACE("%p %p %lu %lu %lu %p %s\n", DeviceInfoSet, DeviceInfoData,
          Scope, HwProfile, KeyType, InfHandle, debugstr_w(InfSectionName));

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (KeyType != DIREG_DEV && KeyType != DIREG_DRV)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (InfHandle && !InfSectionName)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!InfHandle && InfSectionName)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        LPWSTR lpGuidString = NULL;
        LPWSTR DriverKey = NULL; /* {GUID}\Index */
        LPWSTR pDeviceInstance; /* Points into DriverKey, on the Index field */
        DWORD Index; /* Index used in the DriverKey name */
        DWORD rc;
        HKEY hHWProfileKey = INVALID_HANDLE_VALUE;
        HKEY hEnumKey = NULL;
        HKEY hClassKey = NULL;
        HKEY hDeviceKey = INVALID_HANDLE_VALUE;
        HKEY hKey = NULL;
        HKEY RootKey;

        if (Scope == DICS_FLAG_GLOBAL)
            RootKey = list->HKLM;
        else /* Scope == DICS_FLAG_CONFIGSPECIFIC */
        {
            hHWProfileKey = OpenHardwareProfileKey(list->HKLM, HwProfile, KEY_CREATE_SUB_KEY);
            if (hHWProfileKey == INVALID_HANDLE_VALUE)
                goto cleanup;
            RootKey = hHWProfileKey;
        }

        if (KeyType == DIREG_DEV)
        {
            struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;

            rc = RegCreateKeyExW(
                RootKey,
                REGSTR_PATH_SYSTEMENUM,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_CREATE_SUB_KEY,
                NULL,
                &hEnumKey,
                NULL);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
            rc = RegCreateKeyExW(
                hEnumKey,
                deviceInfo->DeviceName,
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
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
        }
        else /* KeyType == DIREG_DRV */
        {
            if (UuidToStringW((UUID*)&DeviceInfoData->ClassGuid, &lpGuidString) != RPC_S_OK)
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
            rc = RegOpenKeyExW(RootKey,
                REGSTR_PATH_CLASS_NT,
                0,
                KEY_CREATE_SUB_KEY,
                &hClassKey);
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }

            /* Try all values for Index between 0 and 9999 */
            Index = 0;
            while (Index <= 9999)
            {
                DWORD Disposition;
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

            /* Open device key, to write Driver value */
            hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, Scope, HwProfile, DIREG_DEV, KEY_SET_VALUE);
            if (hDeviceKey == INVALID_HANDLE_VALUE)
                goto cleanup;
            rc = RegSetValueExW(hDeviceKey, REGSTR_VAL_DRIVER, 0, REG_SZ, (const BYTE *)DriverKey, (strlenW(DriverKey) + 1) * sizeof(WCHAR));
            if (rc != ERROR_SUCCESS)
            {
                SetLastError(rc);
                goto cleanup;
            }
        }

        /* Do installation of the specified section */
        if (InfHandle)
        {
            FIXME("Need to install section %s in file %p\n",
                debugstr_w(InfSectionName), InfHandle);
        }
        ret = hKey;

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
        if (hKey != NULL && hKey != ret)
            RegCloseKey(hKey);
    }

    TRACE("Returning 0x%p\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiOpenDevRegKey (SETUPAPI.@)
 */
HKEY WINAPI
SetupDiOpenDevRegKey(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Scope,
    IN DWORD HwProfile,
    IN DWORD KeyType,
    IN REGSAM samDesired)
{
    struct DeviceInfoSet *list;
    HKEY ret = INVALID_HANDLE_VALUE;

    TRACE("%p %p %lu %lu %lu 0x%lx\n", DeviceInfoSet, DeviceInfoData,
          Scope, HwProfile, KeyType, samDesired);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!DeviceInfoData)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
        SetLastError(ERROR_INVALID_USER_BUFFER);
    else if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (KeyType != DIREG_DEV && KeyType != DIREG_DRV)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
        LPWSTR DriverKey = NULL;
        DWORD dwLength = 0;
        DWORD dwRegType;
        DWORD rc;
        HKEY hHWProfileKey = INVALID_HANDLE_VALUE;
        HKEY hEnumKey = NULL;
        HKEY hKey = NULL;
        HKEY RootKey;

        if (Scope == DICS_FLAG_GLOBAL)
            RootKey = list->HKLM;
        else /* Scope == DICS_FLAG_CONFIGSPECIFIC */
        {
            hHWProfileKey = OpenHardwareProfileKey(list->HKLM, HwProfile, 0);
            if (hHWProfileKey == INVALID_HANDLE_VALUE)
                goto cleanup;
            RootKey = hHWProfileKey;
        }

        rc = RegOpenKeyExW(
            RootKey,
            REGSTR_PATH_SYSTEMENUM,
            0, /* Options */
            0,
            &hEnumKey);
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        rc = RegOpenKeyExW(
            hEnumKey,
            deviceInfo->DeviceName,
            0, /* Options */
            KeyType == DIREG_DEV ? samDesired : KEY_QUERY_VALUE,
            &hKey);
        RegCloseKey(hEnumKey);
        hEnumKey = NULL;
        if (rc != ERROR_SUCCESS)
        {
            SetLastError(rc);
            goto cleanup;
        }
        if (KeyType == DIREG_DEV)
        {
            /* We're done. Just return the hKey handle */
            ret = hKey;
            goto cleanup;
        }
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
            0,
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
        ret = hKey;

cleanup:
        if (hHWProfileKey != INVALID_HANDLE_VALUE)
            RegCloseKey(hHWProfileKey);
        if (hEnumKey != NULL)
            RegCloseKey(hEnumKey);
        if (hKey != NULL && hKey != ret)
            RegCloseKey(hKey);
    }

    TRACE("Returning 0x%p\n", ret);
    return ret;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiCreateDeviceInfoA(
    IN HDEVINFO DeviceInfoSet,
    IN PCSTR DeviceName,
    IN CONST GUID *ClassGuid,
    IN PCSTR DeviceDescription OPTIONAL,
    IN HWND hwndParent OPTIONAL,
    IN DWORD CreationFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    LPWSTR DeviceNameW = NULL;
    LPWSTR DeviceDescriptionW = NULL;
    BOOL bResult;

    TRACE("\n");

    if (DeviceName)
    {
        DeviceNameW = MultiByteToUnicode(DeviceName, CP_ACP);
        if (DeviceNameW == NULL) return FALSE;
    }
    if (DeviceDescription)
    {
        DeviceDescriptionW = MultiByteToUnicode(DeviceDescription, CP_ACP);
        if (DeviceDescriptionW == NULL)
        {
            if (DeviceNameW) MyFree(DeviceNameW);
            return FALSE;
        }
    }

    bResult = SetupDiCreateDeviceInfoW(DeviceInfoSet, DeviceNameW,
                                       ClassGuid, DeviceDescriptionW,
                                       hwndParent, CreationFlags,
                                       DeviceInfoData);

    if (DeviceNameW) MyFree(DeviceNameW);
    if (DeviceDescriptionW) MyFree(DeviceDescriptionW);

    return bResult;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiCreateDeviceInfoW(
    IN HDEVINFO DeviceInfoSet,
    IN PCWSTR DeviceName,
    IN CONST GUID *ClassGuid,
    IN PCWSTR DeviceDescription OPTIONAL,
    IN HWND hwndParent OPTIONAL,
    IN DWORD CreationFlags,
    OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    struct DeviceInfoSet *list;
    BOOL ret = FALSE;

    TRACE("%p %s %s %s %p %lx %p\n", DeviceInfoSet, debugstr_w(DeviceName),
        debugstr_guid(ClassGuid), debugstr_w(DeviceDescription),
        hwndParent, CreationFlags, DeviceInfoData);

    if (!DeviceInfoSet)
        SetLastError(ERROR_INVALID_HANDLE);
    else if ((list = (struct DeviceInfoSet *)DeviceInfoSet)->magic != SETUP_DEVICE_INFO_SET_MAGIC)
        SetLastError(ERROR_INVALID_HANDLE);
    else if (!ClassGuid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (!IsEqualIID(&list->ClassGuid, &GUID_NULL) && !IsEqualIID(&list->ClassGuid, ClassGuid))
        SetLastError(ERROR_CLASS_MISMATCH);
    else if (CreationFlags & ~(DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS))
    {
        TRACE("Unknown flags: 0x%08lx\n", CreationFlags & ~(DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS));
        SetLastError(ERROR_INVALID_FLAGS);
    }
    else
    {
        SP_DEVINFO_DATA DevInfo;

        if (CreationFlags & DICD_GENERATE_ID)
        {
            /* Generate a new unique ID for this device */
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            FIXME("not implemented\n");
        }
        else
        {
            /* Device name is fully qualified. Try to open it */
            BOOL rc;

            DevInfo.cbSize = sizeof(SP_DEVINFO_DATA);
            rc = SetupDiOpenDeviceInfoW(
                DeviceInfoSet,
                DeviceName,
                NULL, /* hwndParent */
                CreationFlags & DICD_INHERIT_CLASSDRVS ? DIOD_INHERIT_CLASSDRVS : 0,
                &DevInfo);

            if (rc)
            {
                /* SetupDiOpenDeviceInfoW has already added
                 * the device info to the device info set
                 */
                SetLastError(ERROR_DEVINST_ALREADY_EXISTS);
            }
            else if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                struct DeviceInfo *deviceInfo;

                if (CreateDeviceInfo(list, DeviceName, ClassGuid, &deviceInfo))
                {
                    InsertTailList(&list->ListHead, &deviceInfo->ListEntry);

                    if (!DeviceInfoData)
                        ret = TRUE;
                    else
                    {
                        if (DeviceInfoData->cbSize != sizeof(PSP_DEVINFO_DATA))
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
            }
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 *		SetupDiDeleteDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiDeleteDeviceInfo(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

    FIXME("not implemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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

    TRACE("%p %s %p %lx %p\n", DeviceInfoSet, DeviceInstanceId, hwndParent, OpenFlags, DeviceInfoData);

    DeviceInstanceIdW = MultiByteToUnicode(DeviceInstanceId, CP_ACP);
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

    TRACE("%p %s %p %lx %p\n",
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
            // TODO
            //if (good one)
            //    break;
            FIXME("not implemented\n");
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
                0,
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

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

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

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

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

    rc = RegOpenKeyExW(
        ((struct DeviceInfoSet *)DeviceInfoSet)->HKLM,
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
    rc = RegQueryValueExW(
        hKey,
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

    return hwProfile;
}

static BOOL
ResetDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
#ifndef __WINESRC__
    PLUGPLAY_CONTROL_RESET_DEVICE_DATA ResetDeviceData;
    struct DeviceInfo *deviceInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    NTSTATUS Status;

    if (((struct DeviceInfoSet *)DeviceInfoSet)->HKLM != HKEY_LOCAL_MACHINE)
    {
        /* At the moment, I only know how to start local devices */
        SetLastError(ERROR_INVALID_COMPUTERNAME);
        return FALSE;
    }

    RtlInitUnicodeString(&ResetDeviceData.DeviceInstance, deviceInfo->DeviceName);
    Status = NtPlugPlayControl(PlugPlayControlResetDevice, &ResetDeviceData, sizeof(PLUGPLAY_CONTROL_RESET_DEVICE_DATA));
    SetLastError(RtlNtStatusToDosError(Status));
    return NT_SUCCESS(Status);
#else
    FIXME("Stub: ResetDevice(%p %p)\n", DeviceInfoSet, DeviceInfoData);
    return TRUE;
#endif
}

static BOOL StopDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("Stub: StopDevice(%p %p)\n", DeviceInfoSet, DeviceInfoData);
    return TRUE;
}

/***********************************************************************
 *		SetupDiChangeState (SETUPAPI.@)
 */
BOOL WINAPI
SetupDiChangeState(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData)
{
    PSP_PROPCHANGE_PARAMS PropChange;
    HKEY hKey = INVALID_HANDLE_VALUE;
    LPCWSTR RegistryValueName;
    DWORD dwConfigFlags, dwLength, dwRegType;
    LONG rc;
    BOOL ret = FALSE;

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

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
            /* Enable/disable device in registry */
            hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, PropChange->Scope, PropChange->HwProfile, DIREG_DEV, KEY_QUERY_VALUE | KEY_SET_VALUE);
            if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
                hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, PropChange->Scope, PropChange->HwProfile, DIREG_DEV, NULL, NULL);
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
    BOOL ret = FALSE; /* Return value */

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

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

        SelectedDriver = (struct DriverInfoElement *)InstallParams.Reserved;
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

        /* Open/Create driver key information */
#if _WIN32_WINNT >= 0x502
        hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
#else
        hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS);
#endif
        if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
            hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
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

    TRACE("%p %p\n", DeviceInfoSet, DeviceInfoData);

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
        ConfigFlags |= DNF_DISABLED;
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

    SelectedDriver = (struct DriverInfoElement *)InstallParams.Reserved;
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
        if (!Result)
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

    /* Open/Create driver key information */
#if _WIN32_WINNT >= 0x502
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
#else
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS);
#endif
    if (hKey == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
        hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
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
        goto cleanup;
    if (GetLastError() == ERROR_SUCCESS_REBOOT_REQUIRED)
        RebootRequired = TRUE;

    /* Open device registry key */
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet, DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_SET_VALUE);
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
