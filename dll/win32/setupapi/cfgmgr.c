/*
 * Configuration manager functions
 *
 * Copyright 2000 James Hatheway
 * Copyright 2005, 2006 Eric Kohl
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

#include <dbt.h>
#include <pnp_c.h>
#include <winsvc.h>

#include <pseh/pseh2.h>

#include "rpc_private.h"

DWORD
WINAPI
I_ScPnPGetServiceName(IN SERVICE_STATUS_HANDLE hServiceStatus,
                      OUT LPWSTR lpServiceName,
                      IN DWORD cchServiceName);


/* Registry key and value names */
static const WCHAR BackslashOpenBrace[] = {'\\', '{', 0};
static const WCHAR CloseBrace[] = {'}', 0};
static const WCHAR Class[]  = {'C','l','a','s','s',0};

static const WCHAR ControlClass[] = {'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'C','l','a','s','s',0};

static const WCHAR DeviceClasses[] = {'S','y','s','t','e','m','\\',
                                      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                      'C','o','n','t','r','o','l','\\',
                                      'D','e','v','i','c','e','C','l','a','s','s','e','s',0};

typedef struct _MACHINE_INFO
{
    WCHAR szMachineName[SP_MAX_MACHINENAME_LENGTH];
    RPC_BINDING_HANDLE BindingHandle;
    HSTRING_TABLE StringTable;
    BOOL bLocal;
} MACHINE_INFO, *PMACHINE_INFO;


typedef struct _LOG_CONF_INFO
{
    ULONG ulMagic;
    DEVINST dnDevInst;
    ULONG ulType;
    ULONG ulTag;
} LOG_CONF_INFO, *PLOG_CONF_INFO;

#define LOG_CONF_MAGIC 0x464E434C  /* "LCNF" */

typedef struct _RES_DES_INFO
{
    ULONG ulMagic;
    DEVINST dnDevInst;
    ULONG ulLogConfType;
    ULONG ulLogConfTag;
    ULONG ulResDesType;
    ULONG ulResDesTag;
} RES_DES_INFO, *PRES_DES_INFO;

#define RES_DES_MAGIC 0x53445352  /* "RSDS" */

typedef struct _NOTIFY_DATA
{
    ULONG ulMagic;
    PVOID hNotifyHandle;
} NOTIFY_DATA, *PNOTIFY_DATA;

#define NOTIFY_MAGIC 0x44556677


typedef struct _INTERNAL_RANGE
{
    LIST_ENTRY ListEntry;
    struct _INTERNAL_RANGE_LIST *pRangeList;
    DWORDLONG ullStart;
    DWORDLONG ullEnd;
} INTERNAL_RANGE, *PINTERNAL_RANGE;

typedef struct _INTERNAL_RANGE_LIST
{
    ULONG ulMagic;
    HANDLE hMutex;
    LIST_ENTRY ListHead;
} INTERNAL_RANGE_LIST, *PINTERNAL_RANGE_LIST;

#define RANGE_LIST_MAGIC 0x33445566

typedef struct _CONFLICT_DATA
{
    ULONG ulMagic;
    PPNP_CONFLICT_LIST pConflictList;
} CONFLICT_DATA, *PCONFLICT_DATA;

#define CONFLICT_MAGIC 0x11225588


/* FUNCTIONS ****************************************************************/

static
BOOL
GuidToString(
    _In_ LPGUID Guid,
    _Out_ LPWSTR String)
{
    LPWSTR lpString;

    if (UuidToStringW(Guid, &lpString) != RPC_S_OK)
        return FALSE;

    lstrcpyW(&String[1], lpString);

    String[0] = '{';
    String[MAX_GUID_STRING_LEN - 2] = '}';
    String[MAX_GUID_STRING_LEN - 1] = UNICODE_NULL;

    RpcStringFreeW(&lpString);

    return TRUE;
}


static
CONFIGRET
RpcStatusToCmStatus(
    _In_ RPC_STATUS Status)
{
    return CR_FAILURE;
}


static
ULONG
GetRegistryPropertyType(
    _In_ ULONG ulProperty)
{
    switch (ulProperty)
    {
        case CM_DRP_DEVICEDESC:
        case CM_DRP_SERVICE:
        case CM_DRP_CLASS:
        case CM_DRP_CLASSGUID:
        case CM_DRP_DRIVER:
        case CM_DRP_MFG:
        case CM_DRP_FRIENDLYNAME:
        case CM_DRP_LOCATION_INFORMATION:
        case CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME:
        case CM_DRP_ENUMERATOR_NAME:
        case CM_DRP_SECURITY_SDS:
        case CM_DRP_UI_NUMBER_DESC_FORMAT:
            return REG_SZ;

        case CM_DRP_HARDWAREID:
        case CM_DRP_COMPATIBLEIDS:
        case CM_DRP_UPPERFILTERS:
        case CM_DRP_LOWERFILTERS:
            return REG_MULTI_SZ;

        case CM_DRP_CONFIGFLAGS:
        case CM_DRP_CAPABILITIES:
        case CM_DRP_UI_NUMBER:
        case CM_DRP_LEGACYBUSTYPE:
        case CM_DRP_BUSNUMBER:
        case CM_DRP_DEVTYPE:
        case CM_DRP_EXCLUSIVE:
        case CM_DRP_CHARACTERISTICS:
        case CM_DRP_ADDRESS:
        case CM_DRP_REMOVAL_POLICY:
        case CM_DRP_REMOVAL_POLICY_HW_DEFAULT:
        case CM_DRP_REMOVAL_POLICY_OVERRIDE:
        case CM_DRP_INSTALL_STATE:
            return REG_DWORD;

        case CM_DRP_BUSTYPEGUID:
        case CM_DRP_SECURITY:
        case CM_DRP_DEVICE_POWER_DATA:
        default:
            return REG_BINARY;
    }

    return REG_NONE;
}


static
VOID
SplitDeviceInstanceId(
    _In_ PWSTR pszDeviceInstanceId,
    _Out_ PWSTR pszDeviceId,
    _Out_ PWSTR pszInstanceId)
{
    PWCHAR ptr;

    wcscpy(pszDeviceId, pszDeviceInstanceId);

    ptr = wcschr(pszDeviceId, L'\\');
    if (ptr != NULL)
    {
        *ptr = UNICODE_NULL;
        ptr++;

        wcscpy(pszInstanceId, ptr);
    }
    else
    {
        *pszInstanceId = UNICODE_NULL;
    }
}


static
CONFIGRET
GetDeviceInstanceKeyPath(
    _In_ RPC_BINDING_HANDLE BindingHandle,
    _In_ PWSTR pszDeviceInst,
    _Out_ PWSTR pszKeyPath,
    _Out_ PWSTR pszInstancePath,
    _In_ ULONG ulHardwareProfile,
    _In_ ULONG ulFlags)
{
    PWSTR pszBuffer = NULL;
    ULONG ulType = 0;
    ULONG ulTransferLength, ulLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("GetDeviceInstanceKeyPath()\n");

    /* Allocate a buffer for the device id */
    pszBuffer = MyMalloc(300 * sizeof(WCHAR));
    if (pszBuffer == NULL)
    {
        ERR("MyMalloc() failed\n");
        return CR_OUT_OF_MEMORY;
    }

    if (ulFlags & CM_REGISTRY_SOFTWARE)
    {
        /* Software Key Path */

        ulTransferLength = 300 * sizeof(WCHAR);
        ulLength = 300 * sizeof(WCHAR);

        RpcTryExcept
        {
            ret = PNP_GetDeviceRegProp(BindingHandle,
                                       pszDeviceInst,
                                       CM_DRP_DRIVER,
                                       &ulType,
                                       (PVOID)pszBuffer,
                                       &ulTransferLength,
                                       &ulLength,
                                       0);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = RpcStatusToCmStatus(RpcExceptionCode());
        }
        RpcEndExcept;

        if (ret != CR_SUCCESS)
        {
            RpcTryExcept
            {
                ret = PNP_GetClassInstance(BindingHandle,
                                           pszDeviceInst,
                                           (PVOID)pszBuffer,
                                           300);
            }
            RpcExcept(EXCEPTION_EXECUTE_HANDLER)
            {
                ret = RpcStatusToCmStatus(RpcExceptionCode());
            }
            RpcEndExcept;

            if (ret != CR_SUCCESS)
            {
                goto done;
            }
        }

        TRACE("szBuffer: %S\n", pszBuffer);

        SplitDeviceInstanceId(pszBuffer,
                              pszBuffer,
                              pszInstancePath);

        TRACE("szBuffer: %S\n", pszBuffer);

        if (ulFlags & CM_REGISTRY_CONFIG)
        {
            if (ulHardwareProfile == 0)
            {
                wsprintfW(pszKeyPath,
                          L"%s\\%s\\%s\\%s",
                          L"System\\CurrentControlSet\\Hardware Profiles",
                          L"Current",
                          L"System\\CurrentControlSet\\Control\\Class",
                          pszBuffer);
            }
            else
            {
                wsprintfW(pszKeyPath,
                          L"%s\\%04lu\\%s\\%s",
                          L"System\\CurrentControlSet\\Hardware Profiles",
                          ulHardwareProfile,
                          L"System\\CurrentControlSet\\Control\\Class",
                          pszBuffer);
            }
        }
        else
        {
            wsprintfW(pszKeyPath,
                      L"%s\\%s",
                      L"System\\CurrentControlSet\\Control\\Class",
                      pszBuffer);
        }
    }
    else
    {
        /* Hardware Key Path */

        if (ulFlags & CM_REGISTRY_CONFIG)
        {
            SplitDeviceInstanceId(pszDeviceInst,
                                  pszBuffer,
                                  pszInstancePath);

            if (ulHardwareProfile == 0)
            {
                wsprintfW(pszKeyPath,
                          L"%s\\%s\\%s\\%s",
                          L"System\\CurrentControlSet\\Hardware Profiles",
                          L"Current",
                          L"System\\CurrentControlSet\\Enum",
                          pszBuffer);
            }
            else
            {
                wsprintfW(pszKeyPath,
                          L"%s\\%04lu\\%s\\%s",
                          L"System\\CurrentControlSet\\Hardware Profiles",
                          ulHardwareProfile,
                          L"System\\CurrentControlSet\\Enum",
                          pszBuffer);
            }
        }
        else if (ulFlags & CM_REGISTRY_USER)
        {
            wsprintfW(pszKeyPath,
                      L"%s\\%s",
                      L"System\\CurrentControlSet\\Enum",
                      pszDeviceInst);

            wcscpy(pszInstancePath,
                   L"Device Parameters");
        }
        else
        {
            SplitDeviceInstanceId(pszDeviceInst,
                                  pszBuffer,
                                  pszInstancePath);

            wsprintfW(pszKeyPath,
                      L"%s\\%s",
                      L"System\\CurrentControlSet\\Enum",
                      pszBuffer);
        }
    }

done:
    if (pszBuffer != NULL)
        MyFree(pszBuffer);

    return ret;
}


BOOL
IsValidRangeList(
    _In_opt_ PINTERNAL_RANGE_LIST pRangeList)
{
    BOOL bValid = TRUE;

    if (pRangeList == NULL)
        return FALSE;

    _SEH2_TRY
    {
        if (pRangeList->ulMagic != RANGE_LIST_MAGIC)
            bValid = FALSE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }
    _SEH2_END;

    return bValid;
}


BOOL
IsValidLogConf(
    _In_opt_ PLOG_CONF_INFO pLogConfInfo)
{
    BOOL bValid = TRUE;

    if (pLogConfInfo == NULL)
        return FALSE;

    _SEH2_TRY
    {
        if (pLogConfInfo->ulMagic != LOG_CONF_MAGIC)
            bValid = FALSE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }
    _SEH2_END;

    return bValid;
}


BOOL
IsValidResDes(
    _In_opt_ PRES_DES_INFO pResDesInfo)
{
    BOOL bValid = TRUE;

    if (pResDesInfo == NULL)
        return FALSE;

    _SEH2_TRY
    {
        if (pResDesInfo->ulMagic != RES_DES_MAGIC)
            bValid = FALSE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }
    _SEH2_END;

    return bValid;
}


BOOL
IsValidConflictData(
    _In_opt_ PCONFLICT_DATA pConflictData)
{
    BOOL bValid = TRUE;

    if (pConflictData == NULL)
        return FALSE;

    _SEH2_TRY
    {
        if (pConflictData->ulMagic != CONFLICT_MAGIC)
            bValid = FALSE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }
    _SEH2_END;

    return bValid;
}


/***********************************************************************
 * CMP_GetBlockedDriverInfo [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_GetBlockedDriverInfo(
    _Out_opt_ LPWSTR pszNames,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    ULONG ulTransferLength;
    CONFIGRET ret;

    TRACE("CMP_GetBlockedDriverInfo(%p %p %lx %p)\n",
          pszNames, pulLength, ulFlags, hMachine);

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    ulTransferLength = *pulLength;

    RpcTryExcept
    {
        ret = PNP_GetBlockedDriverInfo(BindingHandle,
                                       (PBYTE)pszNames,
                                       &ulTransferLength,
                                       pulLength,
                                       ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CMP_GetServerSideDeviceInstallFlags [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_GetServerSideDeviceInstallFlags(
    _Out_ PULONG pulSSDIFlags,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CMP_GetServerSideDeviceInstallFlags(%p %lx %p)\n",
          pulSSDIFlags, ulFlags, hMachine);

    if (pulSSDIFlags == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_GetServerSideDeviceInstallFlags(BindingHandle,
                                                  pulSSDIFlags,
                                                  ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CMP_Init_Detection [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_Init_Detection(
    _In_ ULONG ulMagic)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CMP_Init_Detection(%lu)\n", ulMagic);

    if (ulMagic != CMP_MAGIC)
        return CR_INVALID_DATA;

    if (!PnpGetLocalHandles(&BindingHandle, NULL))
        return CR_FAILURE;

    RpcTryExcept
    {
        ret = PNP_InitDetection(BindingHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CMP_RegisterNotification [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_RegisterNotification(
    _In_ HANDLE hRecipient,
    _In_ LPVOID lpvNotificationFilter,
    _In_ ULONG ulFlags,
    _Out_ PHDEVNOTIFY phDevNotify)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    PNOTIFY_DATA pNotifyData;
    WCHAR szNameBuffer[256];
    INT nLength;
    DWORD ulUnknown9 = 0;
    DWORD dwError;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CMP_RegisterNotification(%p %p %lu %p)\n",
          hRecipient, lpvNotificationFilter, ulFlags, phDevNotify);

    if ((hRecipient == NULL) ||
        (lpvNotificationFilter == NULL) ||
        (phDevNotify == NULL))
        return CR_INVALID_POINTER;

    if (ulFlags & ~0x7)
        return CR_INVALID_FLAG;

    if (((PDEV_BROADCAST_HDR)lpvNotificationFilter)->dbch_size < sizeof(DEV_BROADCAST_HDR))
        return CR_INVALID_DATA;

    if (!PnpGetLocalHandles(&BindingHandle, NULL))
        return CR_FAILURE;

    pNotifyData = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(NOTIFY_DATA));
    if (pNotifyData == NULL)
        return CR_OUT_OF_MEMORY;

    pNotifyData->ulMagic = NOTIFY_MAGIC;
    pNotifyData->hNotifyHandle = NULL;

    ZeroMemory(szNameBuffer, sizeof(szNameBuffer));

    if ((ulFlags & DEVICE_NOTIFY_SERVICE_HANDLE) == DEVICE_NOTIFY_WINDOW_HANDLE)
    {
        FIXME("Register a window\n");

        nLength = GetWindowTextW((HWND)hRecipient,
                                 szNameBuffer,
                                 ARRAYSIZE(szNameBuffer));
        if (nLength == 0)
        {
            szNameBuffer[0] = UNICODE_NULL;
        }

        FIXME("Register window: %S\n", szNameBuffer);
    }
    else if ((ulFlags & DEVICE_NOTIFY_SERVICE_HANDLE) == DEVICE_NOTIFY_SERVICE_HANDLE)
    {
        FIXME("Register a service\n");

        dwError = I_ScPnPGetServiceName((SERVICE_STATUS_HANDLE)hRecipient,
                                        szNameBuffer,
                                        ARRAYSIZE(szNameBuffer));
        if (dwError != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, pNotifyData);
            return CR_INVALID_DATA;
        }

        FIXME("Register service: %S\n", szNameBuffer);
    }

    RpcTryExcept
    {
        ret = PNP_RegisterNotification(BindingHandle,
                                       (DWORD_PTR)hRecipient,
                                       szNameBuffer,
                                       (BYTE*)lpvNotificationFilter,
                                       ((DEV_BROADCAST_HDR*)lpvNotificationFilter)->dbch_size,
                                       ulFlags,
                                       &pNotifyData->hNotifyHandle,
                                       GetCurrentProcessId(),
                                       &ulUnknown9); /* ??? */
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        TRACE("hNotifyHandle: %p\n", pNotifyData->hNotifyHandle);
        *phDevNotify = (HDEVNOTIFY)pNotifyData;
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, pNotifyData);

        *phDevNotify = (HDEVNOTIFY)NULL;
    }

    return ret;
}


/***********************************************************************
 * CMP_Report_LogOn [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_Report_LogOn(
    _In_ DWORD dwMagic,
    _In_ DWORD dwProcessId)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;
    BOOL bAdmin;
    DWORD i;

    TRACE("CMP_Report_LogOn(%lu %lu)\n", dwMagic, dwProcessId);

    if (dwMagic != CMP_MAGIC)
        return CR_INVALID_DATA;

    if (!PnpGetLocalHandles(&BindingHandle, NULL))
        return CR_FAILURE;

    bAdmin = pSetupIsUserAdmin();

    for (i = 0; i < 30; i++)
    {
        RpcTryExcept
        {
            ret = PNP_ReportLogOn(BindingHandle,
                                  bAdmin,
                                  dwProcessId);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = RpcStatusToCmStatus(RpcExceptionCode());
        }
        RpcEndExcept;

        if (ret == CR_SUCCESS)
            break;

        Sleep(5000);
    }

    return ret;
}


/***********************************************************************
 * CMP_UnregisterNotification [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_UnregisterNotification(
    _In_ HDEVNOTIFY hDevNotify)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    PNOTIFY_DATA pNotifyData;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CMP_UnregisterNotification(%p)\n", hDevNotify);

    pNotifyData = (PNOTIFY_DATA)hDevNotify;

    if ((pNotifyData == NULL) ||
        (pNotifyData->ulMagic != NOTIFY_MAGIC))
        return CR_INVALID_POINTER;

    if (!PnpGetLocalHandles(&BindingHandle, NULL))
        return CR_FAILURE;

    RpcTryExcept
    {
        ret = PNP_UnregisterNotification(BindingHandle,
                                         &pNotifyData->hNotifyHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        pNotifyData->hNotifyHandle = NULL;
        HeapFree(GetProcessHeap(), 0, pNotifyData);
    }

    return ret;
}


/***********************************************************************
 * CMP_WaitNoPendingInstallEvents [SETUPAPI.@]
 */
DWORD
WINAPI
CMP_WaitNoPendingInstallEvents(
    _In_ DWORD dwTimeout)
{
    HANDLE hEvent;
    DWORD ret;

    TRACE("CMP_WaitNoPendingInstallEvents(%lu)\n", dwTimeout);

    hEvent = OpenEventW(SYNCHRONIZE, FALSE, L"Global\\PnP_No_Pending_Install_Events");
    if (hEvent == NULL)
       return WAIT_FAILED;

    ret = WaitForSingleObject(hEvent, dwTimeout);
    CloseHandle(hEvent);
    return ret;
}


/***********************************************************************
 * CMP_WaitServicesAvailable [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_WaitServicesAvailable(
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;
    WORD Version;

    TRACE("CMP_WaitServicesAvailable(%p)\n", hMachine);

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_GetVersion(BindingHandle, &Version);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Add_Empty_Log_Conf [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf(
    _Out_ PLOG_CONF plcLogConf,
    _In_ DEVINST dnDevInst,
    _In_ PRIORITY Priority,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Add_Empty_Log_Conf(%p %p %lu %lx)\n",
          plcLogConf, dnDevInst, Priority, ulFlags);

    return CM_Add_Empty_Log_Conf_Ex(plcLogConf, dnDevInst, Priority,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_Empty_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_Empty_Log_Conf_Ex(
    _Out_ PLOG_CONF plcLogConf,
    _In_ DEVINST dnDevInst,
    _In_ PRIORITY Priority,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    ULONG ulLogConfTag = 0;
    LPWSTR lpDevInst;
    PLOG_CONF_INFO pLogConfInfo;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_Add_Empty_Log_Conf_Ex(%p %p %lu %lx %p)\n",
          plcLogConf, dnDevInst, Priority, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (plcLogConf == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (Priority > 0xFFFF)
        return CR_INVALID_PRIORITY;

    if (ulFlags & ~(LOG_CONF_BITS | PRIORITY_BIT))
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_AddEmptyLogConf(BindingHandle, lpDevInst, Priority,
                                  &ulLogConfTag, ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        pLogConfInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(LOG_CONF_INFO));
        if (pLogConfInfo == NULL)
        {
            ret = CR_OUT_OF_MEMORY;
        }
        else
        {
            pLogConfInfo->ulMagic = LOG_CONF_MAGIC;
            pLogConfInfo->dnDevInst = dnDevInst;
            pLogConfInfo->ulType = ulFlags;
            pLogConfInfo->ulTag = ulLogConfTag;

            *plcLogConf = (LOG_CONF)pLogConfInfo;

            ret = CR_SUCCESS;
        }
    }

    return ret;
}


/***********************************************************************
 * CM_Add_IDA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_IDA(
    _In_ DEVINST dnDevInst,
    _In_ PSTR pszID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Add_IDA(%p %s %lx)\n",
          dnDevInst, debugstr_a(pszID), ulFlags);

    return CM_Add_ID_ExA(dnDevInst, pszID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_IDW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_IDW(
    _In_ DEVINST dnDevInst,
    _In_ PWSTR pszID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Add_IDW(%p %s %lx)\n",
          dnDevInst, debugstr_w(pszID), ulFlags);

    return CM_Add_ID_ExW(dnDevInst, pszID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_ID_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_ID_ExA(
    _In_ DEVINST dnDevInst,
    _In_ PSTR pszID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    PWSTR pszIDW;
    CONFIGRET ret;

    TRACE("CM_Add_ID_ExA(%p %s %lx %p)\n",
          dnDevInst, debugstr_a(pszID), ulFlags, hMachine);

    if (pSetupCaptureAndConvertAnsiArg(pszID, &pszIDW))
        return CR_INVALID_DATA;

    ret = CM_Add_ID_ExW(dnDevInst, pszIDW, ulFlags, hMachine);

    MyFree(pszIDW);

    return ret;
}


/***********************************************************************
 * CM_Add_ID_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_ID_ExW(
    _In_ DEVINST dnDevInst,
    _In_ PWSTR pszID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Add_ID_ExW(%p %s %lx %p)\n",
          dnDevInst, debugstr_w(pszID), ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (pszID == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_ADD_ID_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_AddID(BindingHandle,
                        lpDevInst,
                        pszID,
                        ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Add_Range [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_Range(
    _In_ DWORDLONG ullStartValue,
    _In_ DWORDLONG ullEndValue,
    _In_ RANGE_LIST rlh,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pRangeList;
    PINTERNAL_RANGE pRange;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_Add_Range(%I64u %I64u %p %lx)\n",
          ullStartValue, ullEndValue, rlh, ulFlags);

    pRangeList = (PINTERNAL_RANGE_LIST)rlh;

    if (!IsValidRangeList(pRangeList))
        return CR_INVALID_RANGE_LIST;

    if (ulFlags & ~CM_ADD_RANGE_BITS)
        return CR_INVALID_FLAG;

    if (ullEndValue < ullStartValue)
        return CR_INVALID_RANGE;

    /* Lock the range list */
    WaitForSingleObject(pRangeList->hMutex, INFINITE);

    /* Allocate the new range */
    pRange = HeapAlloc(GetProcessHeap(), 0, sizeof(INTERNAL_RANGE));
    if (pRange == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    pRange->pRangeList = pRangeList;
    pRange->ullStart = ullStartValue;
    pRange->ullEnd = ullEndValue;

    /* Insert the range */
    if (IsListEmpty(&pRangeList->ListHead))
    {
        InsertTailList(&pRangeList->ListHead, &pRange->ListEntry);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, pRange);
        UNIMPLEMENTED;
    }

done:
    /* Unlock the range list */
    ReleaseMutex(pRangeList->hMutex);

    return ret;
}


/***********************************************************************
 * CM_Add_Res_Des [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_Res_Des(
    _Out_opt_ PRES_DES prdResDes,
    _In_ LOG_CONF lcLogConf,
    _In_ RESOURCEID ResourceID,
    _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Add_Res_Des(%p %p %lu %p %lu %lx)\n",
          prdResDes, lcLogConf, ResourceID, ResourceData, ResourceLen, ulFlags);

    return CM_Add_Res_Des_Ex(prdResDes, lcLogConf, ResourceID, ResourceData,
                             ResourceLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_Res_Des_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Add_Res_Des_Ex(
    _Out_opt_ PRES_DES prdResDes,
    _In_ LOG_CONF lcLogConf,
    _In_ RESOURCEID ResourceID,
    _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    FIXME("CM_Add_Res_Des_Ex(%p %p %lu %p %lu %lx %p)\n",
          prdResDes, lcLogConf, ResourceID,
          ResourceData, ResourceLen, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Connect_MachineA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Connect_MachineA(
    _In_opt_ PCSTR UNCServerName,
    _Out_ PHMACHINE phMachine)
{
    PWSTR pServerNameW;
    CONFIGRET ret;

    TRACE("CM_Connect_MachineA(%s %p)\n",
          debugstr_a(UNCServerName), phMachine);

    if (UNCServerName == NULL || *UNCServerName == 0)
        return CM_Connect_MachineW(NULL, phMachine);

    if (pSetupCaptureAndConvertAnsiArg(UNCServerName, &pServerNameW))
        return CR_INVALID_DATA;

    ret = CM_Connect_MachineW(pServerNameW, phMachine);

    MyFree(pServerNameW);

    return ret;
}


/***********************************************************************
 * CM_Connect_MachineW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Connect_MachineW(
    _In_opt_ PCWSTR UNCServerName,
    _Out_ PHMACHINE phMachine)
{
    PMACHINE_INFO pMachine;

    TRACE("CM_Connect_MachineW(%s %p)\n",
          debugstr_w(UNCServerName), phMachine);

    if (phMachine == NULL)
        return CR_INVALID_POINTER;

    *phMachine = NULL;

    pMachine = HeapAlloc(GetProcessHeap(), 0, sizeof(MACHINE_INFO));
    if (pMachine == NULL)
        return CR_OUT_OF_MEMORY;

    if (UNCServerName == NULL || *UNCServerName == 0)
    {
        pMachine->bLocal = TRUE;

        /* FIXME: store the computers name in pMachine->szMachineName */

        if (!PnpGetLocalHandles(&pMachine->BindingHandle,
                                &pMachine->StringTable))
        {
            HeapFree(GetProcessHeap(), 0, pMachine);
            return CR_FAILURE;
        }
    }
    else
    {
        pMachine->bLocal = FALSE;
        if (wcslen(UNCServerName) >= SP_MAX_MACHINENAME_LENGTH - 1)
        {
            HeapFree(GetProcessHeap(), 0, pMachine);
            return CR_INVALID_MACHINENAME;
        }
        lstrcpyW(pMachine->szMachineName, UNCServerName);

        pMachine->StringTable = pSetupStringTableInitialize();
        if (pMachine->StringTable == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pMachine);
            return CR_FAILURE;
        }

        pSetupStringTableAddString(pMachine->StringTable, L"PLT", 1);

        if (!PnpBindRpc(UNCServerName, &pMachine->BindingHandle))
        {
            pSetupStringTableDestroy(pMachine->StringTable);
            HeapFree(GetProcessHeap(), 0, pMachine);
            return CR_INVALID_MACHINENAME;
        }
    }

    *phMachine = (PHMACHINE)pMachine;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Create_DevNodeA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Create_DevNodeA(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINSTID_A pDeviceID,
    _In_ DEVINST dnParent,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Create_DevNodeA(%p %s %p %lx)\n",
          pdnDevInst, debugstr_a(pDeviceID), dnParent, ulFlags);

    return CM_Create_DevNode_ExA(pdnDevInst, pDeviceID, dnParent,
                                 ulFlags, NULL);
}


/***********************************************************************
 * CM_Create_DevNodeW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Create_DevNodeW(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINSTID_W pDeviceID,
    _In_ DEVINST dnParent,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Create_DevNodeW(%p %s %p %lx)\n",
          pdnDevInst, debugstr_w(pDeviceID), dnParent, ulFlags);

    return CM_Create_DevNode_ExW(pdnDevInst, pDeviceID, dnParent,
                                 ulFlags, NULL);
}


/***********************************************************************
 * CM_Create_DevNode_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Create_DevNode_ExA(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINSTID_A pDeviceID,
    _In_ DEVINST dnParent,
    _In_ ULONG ulFlags,
    _In_opt_ HANDLE hMachine)
{
    DEVINSTID_W pDeviceIDW;
    CONFIGRET ret;

    TRACE("CM_Create_DevNode_ExA(%p %s %p %lx %p)\n",
          pdnDevInst, debugstr_a(pDeviceID), dnParent, ulFlags, hMachine);

    if (pSetupCaptureAndConvertAnsiArg(pDeviceID, &pDeviceIDW))
        return CR_INVALID_DATA;

    ret = CM_Create_DevNode_ExW(pdnDevInst, pDeviceIDW, dnParent, ulFlags,
                                hMachine);

    MyFree(pDeviceIDW);

    return ret;
}


/***********************************************************************
 * CM_Create_DevNode_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Create_DevNode_ExW(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINSTID_W pDeviceID,
    _In_ DEVINST dnParent,
    _In_ ULONG ulFlags,
    _In_opt_ HANDLE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpParentDevInst;
    CONFIGRET ret = CR_SUCCESS;
    WCHAR szLocalDeviceID[MAX_DEVICE_ID_LEN];

    TRACE("CM_Create_DevNode_ExW(%p %s %p %lx %p)\n",
          pdnDevInst, debugstr_w(pDeviceID), dnParent, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (pdnDevInst == NULL)
        return CR_INVALID_POINTER;

    if (pDeviceID == NULL || wcslen(pDeviceID) == 0 || wcslen(pDeviceID) >= MAX_DEVICE_ID_LEN)
        return CR_INVALID_DEVICE_ID;

    if (dnParent == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_CREATE_DEVNODE_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpParentDevInst = pSetupStringTableStringFromId(StringTable, dnParent);
    if (lpParentDevInst == NULL)
        return CR_INVALID_DEVNODE;

    wcscpy(szLocalDeviceID, pDeviceID);

    RpcTryExcept
    {
        ret = PNP_CreateDevInst(BindingHandle,
                                szLocalDeviceID,
                                lpParentDevInst,
                                MAX_DEVICE_ID_LEN,
                                ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        /* If CM_CREATE_DEVINST_GENERATE_ID was passed in, PNP_CreateDevInst
         * will return the generated device ID in szLocalDeviceID */
        *pdnDevInst = pSetupStringTableAddString(StringTable, szLocalDeviceID, 1);
        if (*pdnDevInst == 0)
            ret = CR_NO_SUCH_DEVNODE;
    }

    return ret;
}


/***********************************************************************
 * CM_Create_Range_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Create_Range_List(
    _Out_ PRANGE_LIST prlh,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pRangeList;

    FIXME("CM_Create_Range_List(%p %lx)\n",
          prlh, ulFlags);

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (prlh == NULL)
        return CR_INVALID_POINTER;

    /* Allocate the range list */
    pRangeList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(INTERNAL_RANGE_LIST));
    if (pRangeList == NULL)
        return CR_OUT_OF_MEMORY;

    /* Set the magic value */
    pRangeList->ulMagic = RANGE_LIST_MAGIC;

    /* Initialize the mutex for synchonized access */
    pRangeList->hMutex = CreateMutex(NULL, FALSE, NULL);
    if (pRangeList->hMutex == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pRangeList);
        return CR_FAILURE;
    }

    InitializeListHead(&pRangeList->ListHead);

    *prlh = (RANGE_LIST)pRangeList;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Delete_Class_Key [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Delete_Class_Key(
    _In_ LPGUID ClassGuid,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Delete_Class_Key(%p %lx)\n",
          ClassGuid, ulFlags);

    return CM_Delete_Class_Key_Ex(ClassGuid, ulFlags, NULL);
}


/***********************************************************************
 * CM_Delete_Class_Key_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Delete_Class_Key_Ex(
    _In_ LPGUID ClassGuid,
    _In_ ULONG ulFlags,
    _In_opt_ HANDLE hMachine)
{
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Delete_Class_Key_Ex(%p %lx %p)\n",
          ClassGuid, ulFlags, hMachine);

    if (ClassGuid == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_DELETE_CLASS_BITS)
        return CR_INVALID_FLAG;

    if (!GuidToString(ClassGuid, szGuidString))
        return CR_INVALID_DATA;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_DeleteClassKey(BindingHandle,
                                 szGuidString,
                                 ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Delete_DevNode_Key [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Delete_DevNode_Key(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulHardwareProfile,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Delete_DevNode_Key(%p %lu %lx)\n",
          dnDevInst, ulHardwareProfile, ulFlags);

    return CM_Delete_DevNode_Key_Ex(dnDevInst, ulHardwareProfile, ulFlags,
                                    NULL);
}


/***********************************************************************
 * CM_Delete_DevNode_Key_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Delete_DevNode_Key_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulHardwareProfile,
    _In_ ULONG ulFlags,
    _In_opt_ HANDLE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PWSTR pszDevInst, pszKeyPath = NULL, pszInstancePath = NULL;
    CONFIGRET ret;

    FIXME("CM_Delete_DevNode_Key_Ex(%p %lu %lx %p)\n",
          dnDevInst, ulHardwareProfile, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags & ~CM_REGISTRY_BITS)
        return CR_INVALID_FLAG;

    if ((ulFlags & CM_REGISTRY_USER) && (ulFlags & CM_REGISTRY_CONFIG))
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    pszDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (pszDevInst == NULL)
        return CR_INVALID_DEVNODE;

    TRACE("pszDevInst: %S\n", pszDevInst);

    pszKeyPath = MyMalloc(512 * sizeof(WCHAR));
    if (pszKeyPath == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    pszInstancePath = MyMalloc(512 * sizeof(WCHAR));
    if (pszInstancePath == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    ret = GetDeviceInstanceKeyPath(BindingHandle,
                                   pszDevInst,
                                   pszKeyPath,
                                   pszInstancePath,
                                   ulHardwareProfile,
                                   ulFlags);
    if (ret != CR_SUCCESS)
        goto done;

    TRACE("pszKeyPath: %S\n", pszKeyPath);
    TRACE("pszInstancePath: %S\n", pszInstancePath);

    if (ulFlags & CM_REGISTRY_USER)
    {
        FIXME("The CM_REGISTRY_USER flag is not supported yet!\n");
    }
    else
    {
#if 0
        if (!pSetupIsUserAdmin())
        {
            ret = CR_ACCESS_DENIED;
            goto done;
        }
#endif

        if (!(ulFlags & CM_REGISTRY_CONFIG))
            ulHardwareProfile = 0;

        RpcTryExcept
        {
            ret = PNP_DeleteRegistryKey(BindingHandle,
                                        pszDevInst,
                                        pszKeyPath,
                                        pszInstancePath,
                                        ulHardwareProfile);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = RpcStatusToCmStatus(RpcExceptionCode());
        }
        RpcEndExcept;
    }

done:
    if (pszInstancePath != NULL)
        MyFree(pszInstancePath);

    if (pszKeyPath != NULL)
        MyFree(pszKeyPath);

    return ret;
}


/***********************************************************************
 * CM_Delete_Range [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Delete_Range(
    _In_ DWORDLONG ullStartValue,
    _In_ DWORDLONG ullEndValue,
    _In_ RANGE_LIST rlh,
    _In_ ULONG ulFlags)
{
    FIXME("CM_Delete_Range(%I64u %I64u %p %lx)\n",
          ullStartValue, ullEndValue, rlh, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Detect_Resource_Conflict [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict(
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _Out_ PBOOL pbConflictDetected,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Detect_Resource_Conflict(%p %lu %p %lu %p 0x%lx)\n",
          dnDevInst, ResourceID, ResourceData, ResourceLen,
          pbConflictDetected, ulFlags);

    return CM_Detect_Resource_Conflict_Ex(dnDevInst,
                                          ResourceID,
                                          ResourceData,
                                          ResourceLen,
                                          pbConflictDetected,
                                          ulFlags,
                                          NULL);
}


/***********************************************************************
 * CM_Detect_Resource_Conflict_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Detect_Resource_Conflict_Ex(
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _Out_ PBOOL pbConflictDetected,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    FIXME("CM_Detect_Resource_Conflict_Ex(%p %lu %p %lu %p 0x%lx %p)\n",
          dnDevInst, ResourceID, ResourceData, ResourceLen,
          pbConflictDetected, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Disable_DevNode [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Disable_DevNode(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Disable_DevNode(%p %lx)\n",
          dnDevInst, ulFlags);

    return CM_Disable_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Disable_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Disable_DevNode_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Disable_DevNode_Ex(%p %lx %p)\n",
          dnDevInst, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_DisableDevInst(BindingHandle,
                                 lpDevInst,
                                 NULL,
                                 NULL,
                                 0,
                                 ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Disconnect_Machine [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Disconnect_Machine(
    _In_opt_ HMACHINE hMachine)
{
    PMACHINE_INFO pMachine;

    TRACE("CM_Disconnect_Machine(%p)\n", hMachine);

    pMachine = (PMACHINE_INFO)hMachine;
    if (pMachine == NULL)
        return CR_SUCCESS;

    if (pMachine->bLocal == FALSE)
    {
        if (pMachine->StringTable != NULL)
            pSetupStringTableDestroy(pMachine->StringTable);

        if (!PnpUnbindRpc(pMachine->BindingHandle))
            return CR_ACCESS_DENIED;
    }

    HeapFree(GetProcessHeap(), 0, pMachine);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Dup_Range_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Dup_Range_List(
    _In_ RANGE_LIST rlhOld,
    _In_ RANGE_LIST rlhNew,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pOldRangeList, pNewRangeList;
    PINTERNAL_RANGE pOldRange, pNewRange;
    PLIST_ENTRY ListEntry;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_Dup_Range_List(%p %p %lx)\n",
          rlhOld, rlhNew, ulFlags);

    pOldRangeList = (PINTERNAL_RANGE_LIST)rlhOld;
    pNewRangeList = (PINTERNAL_RANGE_LIST)rlhNew;

    if (!IsValidRangeList(pOldRangeList))
        return CR_INVALID_RANGE_LIST;

    if (!IsValidRangeList(pNewRangeList))
        return CR_INVALID_RANGE_LIST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    /* Lock the range lists */
    WaitForSingleObject(pOldRangeList->hMutex, INFINITE);
    WaitForSingleObject(pNewRangeList->hMutex, INFINITE);

    /* Delete the new range list, if ist is not empty */
    while (!IsListEmpty(&pNewRangeList->ListHead))
    {
        ListEntry = RemoveHeadList(&pNewRangeList->ListHead);
        pNewRange = CONTAINING_RECORD(ListEntry, INTERNAL_RANGE, ListEntry);
        HeapFree(GetProcessHeap(), 0, pNewRange);
    }

    /* Copy the old range list into the new range list */
    ListEntry = &pOldRangeList->ListHead;
    while (ListEntry->Flink == &pOldRangeList->ListHead)
    {
        pOldRange = CONTAINING_RECORD(ListEntry, INTERNAL_RANGE, ListEntry);

        pNewRange = HeapAlloc(GetProcessHeap(), 0, sizeof(INTERNAL_RANGE));
        if (pNewRange == NULL)
        {
            ret = CR_OUT_OF_MEMORY;
            goto done;
        }

        pNewRange->pRangeList = pNewRangeList;
        pNewRange->ullStart = pOldRange->ullStart;
        pNewRange->ullEnd = pOldRange->ullEnd;

        InsertTailList(&pNewRangeList->ListHead, &pNewRange->ListEntry);

        ListEntry = ListEntry->Flink;
    }

done:
    /* Unlock the range lists */
    ReleaseMutex(pNewRangeList->hMutex);
    ReleaseMutex(pOldRangeList->hMutex);

    return ret;
}


/***********************************************************************
 * CM_Enable_DevNode [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enable_DevNode(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Enable_DevNode(%p %lx)\n",
          dnDevInst, ulFlags);

    return CM_Enable_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Enable_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enable_DevNode_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Enable_DevNode_Ex(%p %lx %p)\n",
          dnDevInst, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_DeviceInstanceAction(BindingHandle,
                                       PNP_DEVINST_ENABLE,
                                       ulFlags,
                                       lpDevInst,
                                       NULL);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Enumerate_Classes [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enumerate_Classes(
    _In_ ULONG ulClassIndex,
    _Out_ LPGUID ClassGuid,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Enumerate_Classes(%lx %p %lx)\n",
          ulClassIndex, ClassGuid, ulFlags);

    return CM_Enumerate_Classes_Ex(ulClassIndex, ClassGuid, ulFlags, NULL);
}


/***********************************************************************
 * CM_Enumerate_Classes_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enumerate_Classes_Ex(
    _In_ ULONG ulClassIndex,
    _Out_ LPGUID ClassGuid,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_GUID_STRING_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulLength = MAX_GUID_STRING_LEN;

    TRACE("CM_Enumerate_Classes_Ex(%lx %p %lx %p)\n",
          ulClassIndex, ClassGuid, ulFlags, hMachine);

    if (ClassGuid == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_EnumerateSubKeys(BindingHandle,
                                   PNP_CLASS_SUBKEYS,
                                   ulClassIndex,
                                   szBuffer,
                                   MAX_GUID_STRING_LEN,
                                   &ulLength,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        /* Remove the {} */
        szBuffer[MAX_GUID_STRING_LEN - 2] = UNICODE_NULL;

        /* Convert the buffer to a GUID */
        if (UuidFromStringW(&szBuffer[1], ClassGuid) != RPC_S_OK)
            return CR_FAILURE;
    }

    return ret;
}


/***********************************************************************
 * CM_Enumerate_EnumeratorsA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsA(
    _In_ ULONG ulEnumIndex,
    _Out_writes_(*pulLength) PCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Enumerate_EnumeratorsA(%lu %p %p %lx)\n",
          ulEnumIndex, Buffer, pulLength, ulFlags);

    return CM_Enumerate_Enumerators_ExA(ulEnumIndex, Buffer, pulLength,
                                        ulFlags, NULL);
}


/***********************************************************************
 * CM_Enumerate_EnumeratorsW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enumerate_EnumeratorsW(
    _In_ ULONG ulEnumIndex,
    _Out_writes_(*pulLength) PWCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Enumerate_EnumeratorsW(%lu %p %p %lx)\n",
          ulEnumIndex, Buffer, pulLength, ulFlags);

    return CM_Enumerate_Enumerators_ExW(ulEnumIndex, Buffer, pulLength,
                                        ulFlags, NULL);
}


/***********************************************************************
 * CM_Enumerate_Enumerators_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExA(
    _In_ ULONG ulEnumIndex,
    _Out_writes_(*pulLength) PCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_DEVICE_ID_LEN];
    ULONG ulOrigLength;
    ULONG ulLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Enumerate_Enumerators_ExA(%lu %p %p %lx %p)\n",
          ulEnumIndex, Buffer, pulLength, ulFlags, hMachine);

    if (Buffer == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    ulOrigLength = *pulLength;
    *pulLength = 0;

    ulLength = MAX_DEVICE_ID_LEN;
    ret = CM_Enumerate_Enumerators_ExW(ulEnumIndex, szBuffer, &ulLength,
                                       ulFlags, hMachine);
    if (ret == CR_SUCCESS)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                szBuffer,
                                ulLength,
                                Buffer,
                                ulOrigLength,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
        else
            *pulLength = lstrlenA(Buffer) + 1;
    }

    return ret;
}


/***********************************************************************
 * CM_Enumerate_Enumerators_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Enumerate_Enumerators_ExW(
    _In_ ULONG ulEnumIndex,
    _Out_writes_(*pulLength) PWCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Enumerate_Enumerators_ExW(%lu %p %p %lx %p)\n",
          ulEnumIndex, Buffer, pulLength, ulFlags, hMachine);

    if (Buffer == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    *Buffer = UNICODE_NULL;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_EnumerateSubKeys(BindingHandle,
                                   PNP_ENUMERATOR_SUBKEYS,
                                   ulEnumIndex,
                                   Buffer,
                                   *pulLength,
                                   pulLength,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Find_Range [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Find_Range(
    _Out_ PDWORDLONG pullStart,
    _In_ DWORDLONG ullStart,
    _In_ ULONG ulLength,
    _In_ DWORDLONG ullAlignment,
    _In_ DWORDLONG ullEnd,
    _In_ RANGE_LIST rlh,
    _In_ ULONG ulFlags)
{
    FIXME("CM_Find_Range(%p %I64u %lu %I64u %I64u %p %lx)\n",
          pullStart, ullStart, ulLength, ullAlignment, ullEnd, rlh, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_First_Range [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_First_Range(
    _In_ RANGE_LIST rlh,
    _Out_ PDWORDLONG pullStart,
    _Out_ PDWORDLONG pullEnd,
    _Out_ PRANGE_ELEMENT preElement,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pRangeList;
    PINTERNAL_RANGE pRange;
    PLIST_ENTRY ListEntry;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_First_Range(%p %p %p %p %lx)\n",
          rlh, pullStart, pullEnd, preElement, ulFlags);

    pRangeList = (PINTERNAL_RANGE_LIST)rlh;

    if (!IsValidRangeList(pRangeList))
        return CR_INVALID_RANGE_LIST;

    if (pullStart == NULL || pullEnd == NULL || preElement == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    /* Lock the range list */
    WaitForSingleObject(pRangeList->hMutex, INFINITE);

    /* Fail, if the list is empty */
    if (IsListEmpty(&pRangeList->ListHead))
    {
        ret = CR_FAILURE;
        goto done;
    }

    /* Get the first range */
    ListEntry = pRangeList->ListHead.Flink;
    pRange = CONTAINING_RECORD(ListEntry, INTERNAL_RANGE, ListEntry);

    /* Return the range data */
    *pullStart = pRange->ullStart;
    *pullEnd = pRange->ullEnd;
    *preElement = (RANGE_ELEMENT)pRange;

done:
    /* Unlock the range list */
    ReleaseMutex(pRangeList->hMutex);

    return ret;
}


/***********************************************************************
 * CM_Free_Log_Conf [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Log_Conf(
    _In_ LOG_CONF lcLogConfToBeFreed,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Free_Log_Conf(%lx %lx)\n",
          lcLogConfToBeFreed, ulFlags);

    return CM_Free_Log_Conf_Ex(lcLogConfToBeFreed, ulFlags, NULL);
}


/***********************************************************************
 * CM_Free_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Log_Conf_Ex(
    _In_ LOG_CONF lcLogConfToBeFreed,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    PLOG_CONF_INFO pLogConfInfo;
    CONFIGRET ret;

    TRACE("CM_Free_Log_Conf_Ex(%lx %lx %p)\n",
          lcLogConfToBeFreed, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConfToBeFreed;
    if (!IsValidLogConf(pLogConfInfo))
        return CR_INVALID_LOG_CONF;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, pLogConfInfo->dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_FreeLogConf(BindingHandle,
                              lpDevInst,
                              pLogConfInfo->ulType,
                              pLogConfInfo->ulTag,
                              0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Free_Log_Conf_Handle [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Log_Conf_Handle(
    _In_ LOG_CONF lcLogConf)
{
    PLOG_CONF_INFO pLogConfInfo;

    TRACE("CM_Free_Log_Conf_Handle(%lx)\n", lcLogConf);

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConf;
    if (!IsValidLogConf(pLogConfInfo))
        return CR_INVALID_LOG_CONF;

    HeapFree(GetProcessHeap(), 0, pLogConfInfo);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Free_Range_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Range_List(
    _In_ RANGE_LIST RangeList,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pRangeList;
    PINTERNAL_RANGE pRange;
    PLIST_ENTRY ListEntry;

    FIXME("CM_Free_Range_List(%p %lx)\n",
          RangeList, ulFlags);

    pRangeList = (PINTERNAL_RANGE_LIST)RangeList;

    if (!IsValidRangeList(pRangeList))
        return CR_INVALID_RANGE_LIST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    /* Lock the range list */
    WaitForSingleObject(pRangeList->hMutex, INFINITE);

    /* Free the list of ranges */
    while (!IsListEmpty(&pRangeList->ListHead))
    {
        ListEntry = RemoveHeadList(&pRangeList->ListHead);
        pRange = CONTAINING_RECORD(ListEntry, INTERNAL_RANGE, ListEntry);
        HeapFree(GetProcessHeap(), 0, pRange);
    }

    /* Unlock the range list */
    ReleaseMutex(pRangeList->hMutex);

    /* Close the mutex */
    CloseHandle(pRangeList->hMutex);

    /* Free the range list */
    HeapFree(GetProcessHeap(), 0, pRangeList);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Free_Res_Des [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Res_Des(
    _Out_ PRES_DES prdResDes,
    _In_ RES_DES rdResDes,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Free_Res_Des(%p %p %lx)\n",
          prdResDes, rdResDes, ulFlags);

    return CM_Free_Res_Des_Ex(prdResDes, rdResDes, ulFlags, NULL);
}


/***********************************************************************
 * CM_Free_Res_Des_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Res_Des_Ex(
    _Out_ PRES_DES prdResDes,
    _In_ RES_DES rdResDes,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    FIXME("CM_Free_Res_Des_Ex(%p %p %lx %p)\n",
          prdResDes, rdResDes, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Free_Res_Des_Handle [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Res_Des_Handle(
    _In_ RES_DES rdResDes)
{
    PRES_DES_INFO pResDesInfo;

    FIXME("CM_Free_Res_Des_Handle(%p)\n", rdResDes);

    pResDesInfo = (PRES_DES_INFO)rdResDes;
    if (!IsValidResDes(pResDesInfo))
        return CR_INVALID_RES_DES;

    HeapFree(GetProcessHeap(), 0, pResDesInfo);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Free_Resource_Conflict_Handle [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Free_Resource_Conflict_Handle(
    _In_ CONFLICT_LIST clConflictList)
{
    PCONFLICT_DATA pConflictData;

    FIXME("CM_Free_Resource_Conflict_Handle(%p)\n",
          clConflictList);

    pConflictData = (PCONFLICT_DATA)clConflictList;
    if (!IsValidConflictData(pConflictData))
        return CR_INVALID_CONFLICT_LIST;

    if (pConflictData->pConflictList != NULL)
        MyFree(pConflictData->pConflictList);

    MyFree(pConflictData);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Child [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Child(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Child(%p %p %lx)\n",
          pdnDevInst, dnDevInst, ulFlags);

    return CM_Get_Child_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Child_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Child_Ex(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szRelatedDevInst[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    DWORD dwIndex, dwLength = MAX_DEVICE_ID_LEN;
    CONFIGRET ret;

    TRACE("CM_Get_Child_Ex(%p %lx %lx %p)\n",
          pdnDevInst, dnDevInst, ulFlags, hMachine);

    if (pdnDevInst == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    *pdnDevInst = -1;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetRelatedDeviceInstance(BindingHandle,
                                           PNP_GET_CHILD_DEVICE_INSTANCE,
                                           lpDevInst,
                                           szRelatedDevInst,
                                           &dwLength,
                                           0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return ret;

    TRACE("szRelatedDevInst: %s\n", debugstr_w(szRelatedDevInst));

    dwIndex = pSetupStringTableAddString(StringTable, szRelatedDevInst, 1);
    if (dwIndex == -1)
        return CR_FAILURE;

    *pdnDevInst = dwIndex;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Class_Key_NameA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Key_NameA(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) LPSTR pszKeyName,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Class_Key_NameA(%p %p %p %lx)\n",
          ClassGuid, pszKeyName, pulLength, ulFlags);

    return CM_Get_Class_Key_Name_ExA(ClassGuid, pszKeyName, pulLength,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Class_Key_NameW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Key_NameW(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) LPWSTR pszKeyName,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Class_Key_NameW(%p %p %p %lx)\n",
          ClassGuid, pszKeyName, pulLength, ulFlags);

    return CM_Get_Class_Key_Name_ExW(ClassGuid, pszKeyName, pulLength,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Class_Key_Name_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExA(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) LPSTR pszKeyName,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_GUID_STRING_LEN];
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulLength;
    ULONG ulOrigLength;

    TRACE("CM_Get_Class_Key_Name_ExA(%p %p %p %lx %p)\n",
          ClassGuid, pszKeyName, pulLength, ulFlags, hMachine);

    if (ClassGuid == NULL || pszKeyName == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    ulOrigLength = *pulLength;
    *pulLength = 0;

    ulLength = MAX_GUID_STRING_LEN;
    ret = CM_Get_Class_Key_Name_ExW(ClassGuid, szBuffer, &ulLength,
                                    ulFlags, hMachine);
    if (ret == CR_SUCCESS)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                szBuffer,
                                ulLength,
                                pszKeyName,
                                ulOrigLength,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
        else
            *pulLength = lstrlenA(pszKeyName) + 1;
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Class_Key_Name_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Key_Name_ExW(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) LPWSTR pszKeyName,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    TRACE("CM_Get_Class_Key_Name_ExW(%p %p %p %lx %p)\n",
          ClassGuid, pszKeyName, pulLength, ulFlags, hMachine);

    if (ClassGuid == NULL || pszKeyName == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (*pulLength < MAX_GUID_STRING_LEN)
    {
        *pulLength = 0;
        return CR_BUFFER_SMALL;
    }

    if (!GuidToString(ClassGuid, pszKeyName))
        return CR_INVALID_DATA;

    *pulLength = MAX_GUID_STRING_LEN;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Class_NameA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_NameA(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) PCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Class_NameA(%p %p %p %lx)\n",
          ClassGuid, Buffer, pulLength, ulFlags);

    return CM_Get_Class_Name_ExA(ClassGuid, Buffer, pulLength, ulFlags,
                                 NULL);
}


/***********************************************************************
 * CM_Get_Class_NameW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_NameW(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) PWCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Class_NameW(%p %p %p %lx)\n",
          ClassGuid, Buffer, pulLength, ulFlags);

    return CM_Get_Class_Name_ExW(ClassGuid, Buffer, pulLength, ulFlags,
                                 NULL);
}


/***********************************************************************
 * CM_Get_Class_Name_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Name_ExA(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) PCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_CLASS_NAME_LEN];
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulLength;
    ULONG ulOrigLength;

    TRACE("CM_Get_Class_Name_ExA(%p %p %p %lx %p)\n",
          ClassGuid, Buffer, pulLength, ulFlags, hMachine);

    if (ClassGuid == NULL || Buffer == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    ulOrigLength = *pulLength;
    *pulLength = 0;

    ulLength = MAX_CLASS_NAME_LEN;
    ret = CM_Get_Class_Name_ExW(ClassGuid, szBuffer, &ulLength,
                                ulFlags, hMachine);
    if (ret == CR_SUCCESS)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                szBuffer,
                                ulLength,
                                Buffer,
                                ulOrigLength,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
        else
            *pulLength = lstrlenA(Buffer) + 1;
    }

    return ret;
}


/***********************************************************************
 * CM_Get_Class_Name_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Name_ExW(
    _In_ LPGUID ClassGuid,
    _Out_writes_opt_(*pulLength) PWCHAR Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Get_Class_Name_ExW(%p %p %p %lx %p\n",
          ClassGuid, Buffer, pulLength, ulFlags, hMachine);

    if (ClassGuid == NULL || Buffer == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (!GuidToString(ClassGuid, szGuidString))
        return CR_INVALID_DATA;

    TRACE("Guid %s\n", debugstr_w(szGuidString));

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_GetClassName(BindingHandle,
                               szGuidString,
                               Buffer,
                               pulLength,
                               ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Class_Registry_PropertyA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Registry_PropertyA(
    LPGUID ClassGuid,
    ULONG ulProperty,
    PULONG pulRegDataType,
    PVOID Buffer,
    PULONG pulLength,
    ULONG ulFlags,
    HMACHINE hMachine)
{
    PWSTR BufferW;
    ULONG ulLength = 0;
    ULONG ulType;
    CONFIGRET ret;

    TRACE("CM_Get_Class_Registry_PropertyA(%p %lu %p %p %p %lx %p)\n",
          ClassGuid, ulProperty, pulRegDataType, Buffer, pulLength,
          ulFlags, hMachine);

    if (pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulProperty < CM_CRP_MIN || ulProperty > CM_CRP_MAX)
        return CR_INVALID_PROPERTY;

    ulType = GetRegistryPropertyType(ulProperty);
    if (ulType == REG_SZ || ulType == REG_MULTI_SZ)
    {
        /* Get the required buffer size */
        ret = CM_Get_Class_Registry_PropertyW(ClassGuid, ulProperty, pulRegDataType,
                                              NULL, &ulLength, ulFlags, hMachine);
        if (ret != CR_BUFFER_SMALL)
            return ret;

        /* Allocate the unicode buffer */
        BufferW = HeapAlloc(GetProcessHeap(), 0, ulLength);
        if (BufferW == NULL)
            return CR_OUT_OF_MEMORY;

        /* Get the property */
        ret = CM_Get_Class_Registry_PropertyW(ClassGuid, ulProperty, pulRegDataType,
                                              BufferW, &ulLength, ulFlags, hMachine);
        if (ret != CR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, BufferW);
            return ret;
        }

        /* Do W->A conversion */
        *pulLength = WideCharToMultiByte(CP_ACP,
                                         0,
                                         BufferW,
                                         ulLength,
                                         Buffer,
                                         *pulLength,
                                         NULL,
                                         NULL);

        /* Release the unicode buffer */
        HeapFree(GetProcessHeap(), 0, BufferW);

        if (*pulLength == 0)
            ret = CR_FAILURE;
    }
    else
    {
        /* Get the property */
        ret = CM_Get_Class_Registry_PropertyW(ClassGuid, ulProperty, pulRegDataType,
                                              Buffer, pulLength, ulFlags, hMachine);
    }

    return ret;
}


/***********************************************************************
 * CM_Get_Class_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Class_Registry_PropertyW(
    LPGUID ClassGuid,
    ULONG ulProperty,
    PULONG pulRegDataType,
    PVOID Buffer,
    PULONG pulLength,
    ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WCHAR szGuidString[PNP_MAX_GUID_STRING_LEN + 1];
    ULONG ulType = 0;
    ULONG ulTransferLength = 0;
    CONFIGRET ret;

    TRACE("CM_Get_Class_Registry_PropertyW(%p %lu %p %p %p %lx %p)\n",
          ClassGuid, ulProperty, pulRegDataType, Buffer, pulLength,
          ulFlags, hMachine);

    if (ClassGuid == NULL || pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (pSetupStringFromGuid(ClassGuid,
                             szGuidString,
                             PNP_MAX_GUID_STRING_LEN) != 0)
        return CR_INVALID_DATA;

    if (ulProperty < CM_CRP_MIN || ulProperty > CM_CRP_MAX)
        return CR_INVALID_PROPERTY;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    ulTransferLength = *pulLength;

    RpcTryExcept
    {
        ret = PNP_GetClassRegProp(BindingHandle,
                                  szGuidString,
                                  ulProperty,
                                  &ulType,
                                  Buffer,
                                  &ulTransferLength,
                                  pulLength,
                                  ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        if (pulRegDataType != NULL)
            *pulRegDataType = ulType;
    }

    return ret;
}


/***********************************************************************
 * CM_Get_Depth [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Depth(
    _Out_ PULONG pulDepth,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Depth(%p %lx %lx)\n",
          pulDepth, dnDevInst, ulFlags);

    return CM_Get_Depth_Ex(pulDepth, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Depth_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Depth_Ex(
    _Out_ PULONG pulDepth,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Get_Depth_Ex(%p %lx %lx %p)\n",
          pulDepth, dnDevInst, ulFlags, hMachine);

    if (pulDepth == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetDepth(BindingHandle,
                           lpDevInst,
                           pulDepth,
                           ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_DevNode_Custom_PropertyA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyA(
    _In_ DEVINST dnDevInst,
    _In_ PCSTR pszCustomPropertyName,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_DevNode_Custom_PropertyA(%lx %s %p %p %p %lx)\n",
          dnDevInst, pszCustomPropertyName, pulRegDataType,
          Buffer, pulLength, ulFlags);

    return CM_Get_DevNode_Custom_Property_ExA(dnDevInst, pszCustomPropertyName,
                                              pulRegDataType, Buffer,
                                              pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Custom_PropertyW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_PropertyW(
    _In_ DEVINST dnDevInst,
    _In_ PCWSTR pszCustomPropertyName,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_DevNode_Custom_PropertyW(%lx %s %p %p %p %lx)\n",
          dnDevInst, debugstr_w(pszCustomPropertyName), pulRegDataType,
          Buffer, pulLength, ulFlags);

    return CM_Get_DevNode_Custom_Property_ExW(dnDevInst, pszCustomPropertyName,
                                              pulRegDataType, Buffer,
                                              pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Custom_Property_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExA(
    _In_ DEVINST dnDevInst,
    _In_ PCSTR pszCustomPropertyName,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR pszPropertyNameW;
    PVOID BufferW;
    ULONG ulLengthW;
    ULONG ulDataType = REG_NONE;
    CONFIGRET ret;

    TRACE("CM_Get_DevNode_Custom_Property_ExA(%lx %s %p %p %p %lx %p)\n",
          dnDevInst, pszCustomPropertyName, pulRegDataType,
          Buffer, pulLength, ulFlags, hMachine);

    if (!pulLength)
        return CR_INVALID_POINTER;

    ulLengthW = *pulLength * sizeof(WCHAR);
    BufferW = HeapAlloc(GetProcessHeap(), 0, ulLengthW);
    if (!BufferW)
        return CR_OUT_OF_MEMORY;

    pszPropertyNameW = pSetupMultiByteToUnicode(pszCustomPropertyName,
                                                CP_ACP);
    if (pszPropertyNameW == NULL)
    {
        HeapFree(GetProcessHeap(), 0, BufferW);
        return CR_OUT_OF_MEMORY;
    }

    ret = CM_Get_DevNode_Custom_Property_ExW(dnDevInst,
                                             pszPropertyNameW,
                                             &ulDataType,
                                             BufferW,
                                             &ulLengthW,
                                             ulFlags,
                                             hMachine);
    if (ret == CR_SUCCESS)
    {
        if (ulDataType == REG_SZ ||
            ulDataType == REG_EXPAND_SZ ||
            ulDataType == REG_MULTI_SZ)
        {
            /* Do W->A conversion */
            *pulLength = WideCharToMultiByte(CP_ACP,
                                             0,
                                             BufferW,
                                             lstrlenW(BufferW) + 1,
                                             Buffer,
                                             *pulLength,
                                             NULL,
                                             NULL);
            if (*pulLength == 0)
                ret = CR_FAILURE;
        }
        else
        {
            /* Directly copy the value */
            if (ulLengthW <= *pulLength)
                memcpy(Buffer, BufferW, ulLengthW);
            else
            {
                *pulLength = ulLengthW;
                ret = CR_BUFFER_SMALL;
            }
        }
    }

    if (pulRegDataType)
        *pulRegDataType = ulDataType;

    HeapFree(GetProcessHeap(), 0, BufferW);
    MyFree(pszPropertyNameW);

    return ret;
}


/***********************************************************************
 * CM_Get_DevNode_Custom_Property_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Custom_Property_ExW(
    _In_ DEVINST dnDevInst,
    _In_ PCWSTR pszCustomPropertyName,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    ULONG ulDataType = REG_NONE;
    ULONG ulTransferLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_DevNode_Custom_Property_ExW(%lx %s %p %p %p %lx %p)\n",
          dnDevInst, debugstr_w(pszCustomPropertyName), pulRegDataType,
          Buffer, pulLength, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (pszCustomPropertyName == NULL ||
        pulLength == NULL ||
        *pulLength == 0)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_CUSTOMDEVPROP_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ulTransferLength = *pulLength;

    RpcTryExcept
    {
        ret = PNP_GetCustomDevProp(BindingHandle,
                                   lpDevInst,
                                   (LPWSTR)pszCustomPropertyName,
                                   &ulDataType,
                                   Buffer,
                                   &ulTransferLength,
                                   pulLength,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        if (pulRegDataType != NULL)
            *pulRegDataType = ulDataType;
    }

    return ret;
}


/***********************************************************************
 * CM_Get_DevNode_Registry_PropertyA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyA(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_DevNode_Registry_PropertyA(%lx %lu %p %p %p %lx)\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags);

    return CM_Get_DevNode_Registry_Property_ExA(dnDevInst, ulProperty,
                                                pulRegDataType, Buffer,
                                                pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_PropertyW(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_DevNode_Registry_PropertyW(%lx %lu %p %p %p %lx)\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags);

    return CM_Get_DevNode_Registry_Property_ExW(dnDevInst, ulProperty,
                                                pulRegDataType, Buffer,
                                                pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Registry_Property_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExA(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    PVOID BufferW;
    ULONG LengthW;
    ULONG ulDataType = REG_NONE;
    CONFIGRET ret;

    TRACE("CM_Get_DevNode_Registry_Property_ExA(%lx %lu %p %p %p %lx %p)\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength,
          ulFlags, hMachine);

    if (!pulLength)
        return CR_INVALID_POINTER;

    LengthW = *pulLength * sizeof(WCHAR);
    BufferW = HeapAlloc(GetProcessHeap(), 0, LengthW);
    if (!BufferW)
        return CR_OUT_OF_MEMORY;

    ret = CM_Get_DevNode_Registry_Property_ExW(dnDevInst,
                                               ulProperty,
                                               &ulDataType,
                                               BufferW,
                                               &LengthW,
                                               ulFlags,
                                               hMachine);

    if (ret == CR_SUCCESS)
    {
        if (ulDataType == REG_SZ ||
            ulDataType == REG_EXPAND_SZ ||
            ulDataType == REG_MULTI_SZ)
        {
            /* Do W->A conversion */
            *pulLength = WideCharToMultiByte(CP_ACP,
                                             0,
                                             BufferW,
                                             lstrlenW(BufferW) + 1,
                                             Buffer,
                                             *pulLength,
                                             NULL,
                                             NULL);
            if (*pulLength == 0)
                ret = CR_FAILURE;
        }
        else
        {
            /* Directly copy the value */
            if (LengthW <= *pulLength)
                memcpy(Buffer, BufferW, LengthW);
            else
            {
                *pulLength = LengthW;
                ret = CR_BUFFER_SMALL;
            }
        }
    }

    if (pulRegDataType)
        *pulRegDataType = ulDataType;

    HeapFree(GetProcessHeap(), 0, BufferW);

    return ret;
}


/***********************************************************************
 * CM_Get_DevNode_Registry_Property_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Registry_Property_ExW(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _Out_opt_ PULONG pulRegDataType,
    _Out_writes_bytes_opt_(*pulLength) PVOID Buffer,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpDevInst;
    ULONG ulDataType = REG_NONE;
    ULONG ulTransferLength = 0;

    TRACE("CM_Get_DevNode_Registry_Property_ExW(%lx %lu %p %p %p %lx %p)\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength,
          ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulProperty < CM_DRP_MIN || ulProperty > CM_DRP_MAX)
        return CR_INVALID_PROPERTY;

    /* pulRegDataType is optional */

    /* Buffer is optional */

    if (pulLength == NULL)
        return CR_INVALID_POINTER;

    if (*pulLength == 0)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ulTransferLength = *pulLength;

    RpcTryExcept
    {
        ret = PNP_GetDeviceRegProp(BindingHandle,
                                   lpDevInst,
                                   ulProperty,
                                   &ulDataType,
                                   Buffer,
                                   &ulTransferLength,
                                   pulLength,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        if (pulRegDataType != NULL)
            *pulRegDataType = ulDataType;
    }

    return ret;
}


/***********************************************************************
 * CM_Get_DevNode_Status [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Status(
    _Out_ PULONG pulStatus,
    _Out_ PULONG pulProblemNumber,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_DevNode_Status(%p %p %lx %lx)\n",
          pulStatus, pulProblemNumber, dnDevInst, ulFlags);

    return CM_Get_DevNode_Status_Ex(pulStatus, pulProblemNumber, dnDevInst,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Status_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_DevNode_Status_Ex(
    _Out_ PULONG pulStatus,
    _Out_ PULONG pulProblemNumber,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Get_DevNode_Status_Ex(%p %p %lx %lx %p)\n",
          pulStatus, pulProblemNumber, dnDevInst, ulFlags, hMachine);

    if (pulStatus == NULL || pulProblemNumber == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetDeviceStatus(BindingHandle,
                                  lpDevInst,
                                  pulStatus,
                                  pulProblemNumber,
                                  ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Device_IDA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_IDA(
    _In_ DEVINST dnDevInst,
    _Out_writes_(BufferLen) PCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_IDA(%lx %p %lu %lx)\n",
          dnDevInst, Buffer, BufferLen, ulFlags);

    return CM_Get_Device_ID_ExA(dnDevInst, Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_IDW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_IDW(
    _In_ DEVINST dnDevInst,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_IDW(%lx %p %lu %lx)\n",
          dnDevInst, Buffer, BufferLen, ulFlags);

    return CM_Get_Device_ID_ExW(dnDevInst, Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_ExA(
    _In_ DEVINST dnDevInst,
    _Out_writes_(BufferLen) PCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szBufferW[MAX_DEVICE_ID_LEN];
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_ID_ExA(%lx %p %lu %lx %p)\n",
          dnDevInst, Buffer, BufferLen, ulFlags, hMachine);

    if (Buffer == NULL)
        return CR_INVALID_POINTER;

    ret = CM_Get_Device_ID_ExW(dnDevInst,
                               szBufferW,
                               MAX_DEVICE_ID_LEN,
                               ulFlags,
                               hMachine);
    if (ret == CR_SUCCESS)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                szBufferW,
                                lstrlenW(szBufferW) + 1,
                                Buffer,
                                BufferLen,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
    }

    return ret;
}


/***********************************************************************
 * CM_Get_Device_ID_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_ExW(
    _In_ DEVINST dnDevInst,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    HSTRING_TABLE StringTable = NULL;

    TRACE("CM_Get_Device_ID_ExW(%lx %p %lu %lx %p)\n",
          dnDevInst, Buffer, BufferLen, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (Buffer == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(NULL, &StringTable))
            return CR_FAILURE;
    }

    if (!pSetupStringTableStringFromIdEx(StringTable,
                                         dnDevInst,
                                         Buffer,
                                         &BufferLen))
        return CR_FAILURE;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Device_ID_ListA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_ListA(
    _In_ PCSTR pszFilter,
    _Out_writes_(BufferLen) PCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_ID_ListA(%p %p %lu %lx)\n",
          pszFilter, Buffer, BufferLen, ulFlags);

    return CM_Get_Device_ID_List_ExA(pszFilter, Buffer, BufferLen,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_ListW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_ListW(
    _In_ PCWSTR pszFilter,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_ID_ListW(%p %p %lu %lx)\n",
          pszFilter, Buffer, BufferLen, ulFlags);

    return CM_Get_Device_ID_List_ExW(pszFilter, Buffer, BufferLen,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_List_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExA(
    _In_ PCSTR pszFilter,
    _Out_writes_(BufferLen) PCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR BufferW = NULL;
    LPWSTR pszFilterW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_ID_List_ExA(%p %p %lu %lx %p)\n",
          pszFilter, Buffer, BufferLen, ulFlags, hMachine);

    BufferW = MyMalloc(BufferLen * sizeof(WCHAR));
    if (BufferW == NULL)
        return CR_OUT_OF_MEMORY;

    if (pszFilter == NULL)
    {
        ret = CM_Get_Device_ID_List_ExW(NULL,
                                        BufferW,
                                        BufferLen,
                                        ulFlags,
                                        hMachine);
    }
    else
    {
        if (pSetupCaptureAndConvertAnsiArg(pszFilter, &pszFilterW))
        {
            ret = CR_INVALID_DEVICE_ID;
            goto Done;
        }

        ret = CM_Get_Device_ID_List_ExW(pszFilterW,
                                        BufferW,
                                        BufferLen,
                                        ulFlags,
                                        hMachine);

        MyFree(pszFilterW);
    }

    if (WideCharToMultiByte(CP_ACP,
                            0,
                            BufferW,
                            BufferLen,
                            Buffer,
                            BufferLen,
                            NULL,
                            NULL) == 0)
        ret = CR_FAILURE;

Done:
    MyFree(BufferW);

    return ret;
}


/***********************************************************************
 * CM_Get_Device_ID_List_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_List_ExW(
    _In_ PCWSTR pszFilter,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Get_Device_ID_List_ExW(%p %p %lu %lx %p)\n",
          pszFilter, Buffer, BufferLen, ulFlags, hMachine);

    if (Buffer == NULL || BufferLen == 0)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_GETIDLIST_FILTER_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    *Buffer = 0;

    RpcTryExcept
    {
        ret = PNP_GetDeviceList(BindingHandle,
                                (LPWSTR)pszFilter,
                                Buffer,
                                &BufferLen,
                                ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Device_ID_List_SizeA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeA(
    _Out_ PULONG pulLen,
    _In_opt_ PCSTR pszFilter,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_ID_List_SizeA(%p %s %lx)\n",
          pulLen, debugstr_a(pszFilter), ulFlags);

    return CM_Get_Device_ID_List_Size_ExA(pulLen, pszFilter, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_List_SizeW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_List_SizeW(
    _Out_ PULONG pulLen,
    _In_opt_ PCWSTR pszFilter,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_ID_List_SizeW(%p %s %lx)\n",
          pulLen, debugstr_w(pszFilter), ulFlags);

    return CM_Get_Device_ID_List_Size_ExW(pulLen, pszFilter, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_List_Size_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExA(
    _Out_ PULONG pulLen,
    _In_opt_ PCSTR pszFilter,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR pszFilterW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_Get_Device_ID_List_Size_ExA(%p %s %lx %p)\n",
          pulLen, debugstr_a(pszFilter), ulFlags, hMachine);

    if (pszFilter == NULL)
    {
        ret = CM_Get_Device_ID_List_Size_ExW(pulLen,
                                             NULL,
                                             ulFlags,
                                             hMachine);
    }
    else
    {
        if (pSetupCaptureAndConvertAnsiArg(pszFilter, &pszFilterW))
            return CR_INVALID_DEVICE_ID;

        ret = CM_Get_Device_ID_List_Size_ExW(pulLen,
                                             pszFilterW,
                                             ulFlags,
                                             hMachine);

        MyFree(pszFilterW);
    }

    return ret;
}


/***********************************************************************
 * CM_Get_Device_ID_List_Size_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_List_Size_ExW(
    _Out_ PULONG pulLen,
    _In_opt_ PCWSTR pszFilter,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    FIXME("CM_Get_Device_ID_List_Size_ExW(%p %s %lx %p)\n",
          pulLen, debugstr_w(pszFilter), ulFlags, hMachine);

    if (pulLen == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_GETIDLIST_FILTER_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    *pulLen = 0;

    RpcTryExcept
    {
        ret = PNP_GetDeviceListSize(BindingHandle,
                                    (LPWSTR)pszFilter,
                                    pulLen,
                                    ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Device_ID_Size [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_Size(
    _Out_ PULONG pulLen,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_ID_Size(%p %lx %lx)\n",
          pulLen, dnDevInst, ulFlags);

    return CM_Get_Device_ID_Size_Ex(pulLen, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_Size_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_ID_Size_Ex(
    _Out_ PULONG pulLen,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    HSTRING_TABLE StringTable = NULL;
    LPWSTR DeviceId;

    TRACE("CM_Get_Device_ID_Size_Ex(%p %lx %lx %p)\n",
          pulLen, dnDevInst, ulFlags, hMachine);

    if (pulLen == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(NULL, &StringTable))
            return CR_FAILURE;
    }

    DeviceId = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (DeviceId == NULL)
    {
        *pulLen = 0;
        return CR_SUCCESS;
    }

    *pulLen = lstrlenW(DeviceId);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Device_Interface_AliasA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasA(
    _In_ LPCSTR pszDeviceInterface,
    _In_ LPGUID AliasInterfaceGuid,
    _Out_writes_(*pulLength) LPSTR pszAliasDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_Interface_AliasA(%p %p %p %p %lx)\n",
          pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags);

    return CM_Get_Device_Interface_Alias_ExA(pszDeviceInterface,
        AliasInterfaceGuid, pszAliasDeviceInterface, pulLength,
        ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_Interface_AliasW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_AliasW(
    _In_ LPCWSTR pszDeviceInterface,
    _In_ LPGUID AliasInterfaceGuid,
    _Out_writes_(*pulLength) LPWSTR pszAliasDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_Interface_AliasW(%p %p %p %p %lx)\n",
          pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags);

    return CM_Get_Device_Interface_Alias_ExW(pszDeviceInterface,
        AliasInterfaceGuid, pszAliasDeviceInterface, pulLength,
        ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_Interface_Alias_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExA(
    _In_ LPCSTR pszDeviceInterface,
    _In_ LPGUID AliasInterfaceGuid,
    _Out_writes_(*pulLength) LPSTR pszAliasDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    FIXME("CM_Get_Device_Interface_Alias_ExA(%p %p %p %p %lx %p)\n",
          pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Device_Interface_Alias_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_Alias_ExW(
    _In_ LPCWSTR pszDeviceInterface,
    _In_ LPGUID AliasInterfaceGuid,
    _Out_writes_(*pulLength) LPWSTR pszAliasDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    ULONG ulTransferLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_Interface_Alias_ExW(%p %p %p %p %lx %p)\n",
          pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags, hMachine);

    if (pszDeviceInterface == NULL ||
        AliasInterfaceGuid == NULL ||
        pszAliasDeviceInterface == NULL ||
        pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    ulTransferLength = *pulLength;

    RpcTryExcept
    {
        ret = PNP_GetInterfaceDeviceAlias(BindingHandle,
                                          (LPWSTR)pszDeviceInterface,
                                          AliasInterfaceGuid,
                                          pszAliasDeviceInterface,
                                          pulLength,
                                          &ulTransferLength,
                                          0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 *      CM_Get_Device_Interface_ListA (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListA(
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_A pDeviceID,
    _Out_writes_(BufferLen) PCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_Interface_ListA(%s %s %p %lu 0x%08lx)\n",
          debugstr_guid(InterfaceClassGuid), debugstr_a(pDeviceID),
          Buffer, BufferLen, ulFlags);

    return CM_Get_Device_Interface_List_ExA(InterfaceClassGuid, pDeviceID,
                                            Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_ListW (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_ListW(
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_W pDeviceID,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_Interface_ListW(%s %s %p %lu 0x%08lx)\n",
          debugstr_guid(InterfaceClassGuid), debugstr_w(pDeviceID),
          Buffer, BufferLen, ulFlags);

    return CM_Get_Device_Interface_List_ExW(InterfaceClassGuid, pDeviceID,
                                            Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_ExA (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExA(
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_A pDeviceID,
    _Out_writes_(BufferLen) PCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    DEVINSTID_W pDeviceIdW = NULL;
    PWCHAR BufferW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_Interface_List_ExA(%s %s %p %lu 0x%08lx %p)\n",
          debugstr_guid(InterfaceClassGuid), debugstr_a(pDeviceID),
          Buffer, BufferLen, ulFlags, hMachine);

    if (Buffer == NULL ||
        BufferLen == 0)
        return CR_INVALID_POINTER;

    if (pDeviceID != NULL)
    {
        if (!pSetupCaptureAndConvertAnsiArg(pDeviceID, &pDeviceIdW))
            return CR_INVALID_DEVICE_ID;
    }

    BufferW = MyMalloc(BufferLen * sizeof(WCHAR));
    if (BufferW == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto Done;
    }

    ret = CM_Get_Device_Interface_List_ExW(InterfaceClassGuid, pDeviceIdW,
                                           BufferW, BufferLen, ulFlags,
                                           hMachine);
    if (ret != CR_SUCCESS)
        goto Done;

    if (WideCharToMultiByte(CP_ACP,
                            0,
                            BufferW,
                            BufferLen,
                            Buffer,
                            BufferLen,
                            NULL,
                            NULL) == 0)
        ret = CR_FAILURE;

Done:
    if (BufferW != NULL)
        MyFree(BufferW);

    if (pDeviceIdW != NULL)
        MyFree(pDeviceIdW);

    return ret;
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_ExW (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_ExW(
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_W pDeviceID,
    _Out_writes_(BufferLen) PWCHAR Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    PNP_RPC_BUFFER_SIZE BufferSize = 0;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_Interface_List_ExW(%s %s %p %lu 0x%08lx %p)\n",
          debugstr_guid(InterfaceClassGuid), debugstr_w(pDeviceID),
          Buffer, BufferLen, ulFlags, hMachine);

    if (Buffer == NULL ||
        BufferLen == 0)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_GET_DEVICE_INTERFACE_LIST_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    *Buffer = 0;
    BufferSize = BufferLen;

    RpcTryExcept
    {
        ret = PNP_GetInterfaceDeviceList(BindingHandle,
                                         InterfaceClassGuid,
                                         pDeviceID,
                                         (LPBYTE)Buffer,
                                         &BufferSize,
                                         ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeA (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeA(
    _Out_ PULONG pulLen,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_A pDeviceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_Interface_List_SizeA(%p %p %s 0x%08lx)\n",
          pulLen, InterfaceClassGuid, debugstr_a(pDeviceID), ulFlags);

    return CM_Get_Device_Interface_List_Size_ExA(pulLen, InterfaceClassGuid,
                                                 pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeW (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_SizeW(
    _Out_ PULONG pulLen,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_W pDeviceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Device_Interface_List_SizeW(%p %p %s 0x%08lx)\n",
          pulLen, InterfaceClassGuid, debugstr_w(pDeviceID), ulFlags);

    return CM_Get_Device_Interface_List_Size_ExW(pulLen, InterfaceClassGuid,
                                                 pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExA (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExA(
    _Out_ PULONG pulLen,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_A pDeviceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    DEVINSTID_W pDeviceIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_Interface_List_Size_ExA(%p %p %s 0x%08lx %p)\n",
          pulLen, InterfaceClassGuid, debugstr_a(pDeviceID), ulFlags, hMachine);

    if (pulLen == NULL)
        return CR_INVALID_POINTER;

    if (pDeviceID != NULL)
    {
        if (!pSetupCaptureAndConvertAnsiArg(pDeviceID, &pDeviceIdW))
            return CR_INVALID_DEVICE_ID;
    }

    *pulLen = 0;

    ret = CM_Get_Device_Interface_List_Size_ExW(pulLen, InterfaceClassGuid,
                                                pDeviceIdW, ulFlags, hMachine);

    if (pDeviceIdW != NULL)
        MyFree(pDeviceIdW);

    return ret;
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExW (SETUPAPI.@)
 */
CONFIGRET
WINAPI
CM_Get_Device_Interface_List_Size_ExW(
    _Out_ PULONG pulLen,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ DEVINSTID_W pDeviceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_Device_Interface_List_Size_ExW(%p %p %s 0x%08lx %p)\n",
          pulLen, InterfaceClassGuid, debugstr_w(pDeviceID), ulFlags, hMachine);

    if (pulLen == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_GET_DEVICE_INTERFACE_LIST_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    *pulLen = 0;

    RpcTryExcept
    {
        ret = PNP_GetInterfaceDeviceListSize(BindingHandle,
                                             pulLen,
                                             InterfaceClassGuid,
                                             pDeviceID,
                                             ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_First_Log_Conf [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_First_Log_Conf(
    _Out_opt_ PLOG_CONF plcLogConf,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_First_Log_Conf(%p %lx %lx)\n",
          plcLogConf, dnDevInst, ulFlags);

    return CM_Get_First_Log_Conf_Ex(plcLogConf, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_First_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_First_Log_Conf_Ex(
    _Out_opt_ PLOG_CONF plcLogConf,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst = NULL;
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulTag;
    PLOG_CONF_INFO pLogConfInfo;

    FIXME("CM_Get_First_Log_Conf_Ex(%p %lx %lx %p)\n",
          plcLogConf, dnDevInst, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags & ~LOG_CONF_BITS)
        return CR_INVALID_FLAG;

    if (plcLogConf)
        *plcLogConf = 0;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetFirstLogConf(BindingHandle,
                                  lpDevInst,
                                  ulFlags,
                                  &ulTag,
                                  ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return ret;

    if (plcLogConf)
    {
        pLogConfInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(LOG_CONF_INFO));
        if (pLogConfInfo == NULL)
            return CR_OUT_OF_MEMORY;

        pLogConfInfo->ulMagic = LOG_CONF_MAGIC;
        pLogConfInfo->dnDevInst = dnDevInst;
        pLogConfInfo->ulType = ulFlags;
        pLogConfInfo->ulTag = ulTag;

        *plcLogConf = (LOG_CONF)pLogConfInfo;
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Global_State [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Global_State(
    _Out_ PULONG pulState,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Global_State(%p %lx)\n",
          pulState, ulFlags);

    return CM_Get_Global_State_Ex(pulState, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Global_State_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Global_State_Ex(
    _Out_ PULONG pulState,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Get_Global_State_Ex(%p %lx %p)\n",
          pulState, ulFlags, hMachine);

    if (pulState == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_GetGlobalState(BindingHandle, pulState, ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_HW_Prof_FlagsA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsA(
    _In_ DEVINSTID_A szDevInstName,
    _In_ ULONG ulHardwareProfile,
    _Out_ PULONG pulValue,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_HW_Prof_FlagsA(%s %lu %p %lx)\n",
          debugstr_a(szDevInstName), ulHardwareProfile, pulValue, ulFlags);

    return CM_Get_HW_Prof_Flags_ExA(szDevInstName, ulHardwareProfile,
                                    pulValue, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_HW_Prof_FlagsW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_HW_Prof_FlagsW(
    _In_ DEVINSTID_W szDevInstName,
    _In_ ULONG ulHardwareProfile,
    _Out_ PULONG pulValue,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_HW_Prof_FlagsW(%s %lu %p %lx)\n",
          debugstr_w(szDevInstName), ulHardwareProfile, pulValue, ulFlags);

    return CM_Get_HW_Prof_Flags_ExW(szDevInstName, ulHardwareProfile,
                                    pulValue, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_HW_Prof_Flags_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExA(
    _In_ DEVINSTID_A szDevInstName,
    _In_ ULONG ulHardwareProfile,
    _Out_ PULONG pulValue,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    DEVINSTID_W pszDevIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Get_HW_Prof_Flags_ExA(%s %lu %p %lx %p)\n",
          debugstr_a(szDevInstName), ulHardwareProfile, pulValue, ulFlags, hMachine);

    if (szDevInstName != NULL)
    {
       if (pSetupCaptureAndConvertAnsiArg(szDevInstName, &pszDevIdW))
         return CR_INVALID_DEVICE_ID;
    }

    ret = CM_Get_HW_Prof_Flags_ExW(pszDevIdW, ulHardwareProfile,
                                   pulValue, ulFlags, hMachine);

    if (pszDevIdW != NULL)
        MyFree(pszDevIdW);

    return ret;
}


/***********************************************************************
 * CM_Get_HW_Prof_Flags_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_HW_Prof_Flags_ExW(
    _In_ DEVINSTID_W szDevInstName,
    _In_ ULONG ulHardwareProfile,
    _Out_ PULONG pulValue,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    FIXME("CM_Get_HW_Prof_Flags_ExW(%s %lu %p %lx %p)\n",
          debugstr_w(szDevInstName), ulHardwareProfile, pulValue, ulFlags, hMachine);

    if ((szDevInstName == NULL) || (pulValue == NULL))
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    /* FIXME: Check whether szDevInstName is valid */

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_HwProfFlags(BindingHandle, PNP_GET_HWPROFFLAGS, szDevInstName,
                              ulHardwareProfile, pulValue, NULL, NULL, 0, 0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Hardware_Profile_InfoA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoA(
    _In_ ULONG ulIndex,
    _Out_ PHWPROFILEINFO_A pHWProfileInfo,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Hardware_Profile_InfoA(%lu %p %lx)\n",
          ulIndex, pHWProfileInfo, ulFlags);

    return CM_Get_Hardware_Profile_Info_ExA(ulIndex, pHWProfileInfo,
                                            ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Hardware_Profile_InfoW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_InfoW(
    _In_ ULONG ulIndex,
    _Out_ PHWPROFILEINFO_W pHWProfileInfo,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Hardware_Profile_InfoW(%lu %p %lx)\n",
          ulIndex, pHWProfileInfo, ulFlags);

    return CM_Get_Hardware_Profile_Info_ExW(ulIndex, pHWProfileInfo,
                                            ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Hardware_Profile_Info_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExA(
    _In_ ULONG ulIndex,
    _Out_ PHWPROFILEINFO_A pHWProfileInfo,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    HWPROFILEINFO_W LocalProfileInfo;
    CONFIGRET ret;

    TRACE("CM_Get_Hardware_Profile_Info_ExA(%lu %p %lx %p)\n",
          ulIndex, pHWProfileInfo, ulFlags, hMachine);

    if (pHWProfileInfo == NULL)
        return CR_INVALID_POINTER;

    ret = CM_Get_Hardware_Profile_Info_ExW(ulIndex, &LocalProfileInfo,
                                           ulFlags, hMachine);
    if (ret == CR_SUCCESS)
    {
        pHWProfileInfo->HWPI_ulHWProfile = LocalProfileInfo.HWPI_ulHWProfile;
        pHWProfileInfo->HWPI_dwFlags = LocalProfileInfo.HWPI_dwFlags;

        if (WideCharToMultiByte(CP_ACP,
                                0,
                                LocalProfileInfo.HWPI_szFriendlyName,
                                lstrlenW(LocalProfileInfo.HWPI_szFriendlyName) + 1,
                                pHWProfileInfo->HWPI_szFriendlyName,
                                MAX_PROFILE_LEN,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
    }

    return ret;
}


/***********************************************************************
 * CM_Get_Hardware_Profile_Info_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Hardware_Profile_Info_ExW(
    _In_ ULONG ulIndex,
    _Out_ PHWPROFILEINFO_W pHWProfileInfo,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Get_Hardware_Profile_Info_ExW(%lu %p %lx %p)\n",
          ulIndex, pHWProfileInfo, ulFlags, hMachine);

    if (pHWProfileInfo == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_GetHwProfInfo(BindingHandle, ulIndex, pHWProfileInfo,
                                sizeof(HWPROFILEINFO_W), 0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Log_Conf_Priority [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority(
    _In_ LOG_CONF lcLogConf,
    _Out_ PPRIORITY pPriority,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Log_Conf_Priority(%p %p %lx)\n",
          lcLogConf, pPriority, ulFlags);

    return CM_Get_Log_Conf_Priority_Ex(lcLogConf, pPriority, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Log_Conf_Priority_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Log_Conf_Priority_Ex(
    _In_ LOG_CONF lcLogConf,
    _Out_ PPRIORITY pPriority,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PLOG_CONF_INFO pLogConfInfo;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("CM_Get_Log_Conf_Priority_Ex(%p %p %lx %p)\n",
          lcLogConf, pPriority, ulFlags, hMachine);

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConf;
    if (!IsValidLogConf(pLogConfInfo))
        return CR_INVALID_LOG_CONF;

    if (pPriority == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, pLogConfInfo->dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetLogConfPriority(BindingHandle,
                                     lpDevInst,
                                     pLogConfInfo->ulType,
                                     pLogConfInfo->ulTag,
                                     pPriority,
                                     0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Get_Next_Log_Conf [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf(
    _Out_opt_ PLOG_CONF plcLogConf,
    _In_ LOG_CONF lcLogConf,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Next_Log_Conf(%p %p %lx)\n",
          plcLogConf, lcLogConf, ulFlags);

    return CM_Get_Next_Log_Conf_Ex(plcLogConf, lcLogConf, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Next_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Next_Log_Conf_Ex(
    _Out_opt_ PLOG_CONF plcLogConf,
    _In_ LOG_CONF lcLogConf,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PLOG_CONF_INFO pLogConfInfo;
    PLOG_CONF_INFO pNewLogConfInfo;
    ULONG ulNewTag;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("CM_Get_Next_Log_Conf_Ex(%p %p %lx %p)\n",
          plcLogConf, lcLogConf, ulFlags, hMachine);

    if (plcLogConf)
        *plcLogConf = 0;

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConf;
    if (!IsValidLogConf(pLogConfInfo))
        return CR_INVALID_LOG_CONF;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, pLogConfInfo->dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetNextLogConf(BindingHandle,
                                 lpDevInst,
                                 pLogConfInfo->ulType,
                                 pLogConfInfo->ulTag,
                                 &ulNewTag,
                                 0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return ret;

    if (plcLogConf)
    {
        pNewLogConfInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(LOG_CONF_INFO));
        if (pNewLogConfInfo == NULL)
            return CR_OUT_OF_MEMORY;

        pNewLogConfInfo->ulMagic = LOG_CONF_MAGIC;
        pNewLogConfInfo->dnDevInst = pLogConfInfo->dnDevInst;
        pNewLogConfInfo->ulType = pLogConfInfo->ulType;
        pNewLogConfInfo->ulTag = ulNewTag;

        *plcLogConf = (LOG_CONF)pNewLogConfInfo;
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Next_Res_Des [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Next_Res_Des(
    _Out_ PRES_DES prdResDes,
    _In_ RES_DES rdResDes,
    _In_ RESOURCEID ForResource,
    _Out_opt_ PRESOURCEID pResourceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Next_Res_Des(%p %p %lu %p %lx)\n",
          prdResDes, rdResDes, ForResource, pResourceID, ulFlags);

    return CM_Get_Next_Res_Des_Ex(prdResDes, rdResDes, ForResource,
                                  pResourceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Next_Re_Des_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Next_Res_Des_Ex(
    _Out_ PRES_DES prdResDes,
    _In_ RES_DES rdResDes,
    _In_ RESOURCEID ForResource,
    _Out_opt_ PRESOURCEID pResourceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PRES_DES_INFO pNewResDesInfo = NULL;
    ULONG ulLogConfTag, ulLogConfType, ulResDesTag;
    ULONG ulNextResDesType = 0, ulNextResDesTag = 0;
    LPWSTR lpDevInst;
    DEVINST dnDevInst;
    CONFIGRET ret;

    FIXME("CM_Get_Next_Res_Des_Ex(%p %p %lu %p %lx %p)\n",
          prdResDes, rdResDes, ForResource, pResourceID, ulFlags, hMachine);

    if (prdResDes == NULL)
        return CR_INVALID_POINTER;

    if (IsValidLogConf((PLOG_CONF_INFO)rdResDes))
    {
        FIXME("LogConf found!\n");
        dnDevInst = ((PLOG_CONF_INFO)rdResDes)->dnDevInst;
        ulLogConfTag = ((PLOG_CONF_INFO)rdResDes)->ulTag;
        ulLogConfType = ((PLOG_CONF_INFO)rdResDes)->ulType;
        ulResDesTag = (ULONG)-1;
    }
    else if (IsValidResDes((PRES_DES_INFO)rdResDes))
    {
        FIXME("ResDes found!\n");
        dnDevInst = ((PRES_DES_INFO)rdResDes)->dnDevInst;
        ulLogConfTag = ((PRES_DES_INFO)rdResDes)->ulLogConfTag;
        ulLogConfType = ((PRES_DES_INFO)rdResDes)->ulLogConfType;
        ulResDesTag = ((PRES_DES_INFO)rdResDes)->ulResDesTag;
    }
    else
    {
        return CR_INVALID_RES_DES;
    }

    if ((ForResource == ResType_All) && (pResourceID == NULL))
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetNextResDes(BindingHandle,
                                lpDevInst,
                                ulLogConfTag,
                                ulLogConfType,
                                ForResource,
                                ulResDesTag,
                                &ulNextResDesTag,
                                &ulNextResDesType,
                                0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return ret;

    if (ForResource == ResType_All)
        *pResourceID = ulNextResDesType;

    if (prdResDes)
    {
        pNewResDesInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(RES_DES_INFO));
        if (pNewResDesInfo == NULL)
            return CR_OUT_OF_MEMORY;

        pNewResDesInfo->ulMagic = LOG_CONF_MAGIC;
        pNewResDesInfo->dnDevInst = dnDevInst;
        pNewResDesInfo->ulLogConfType = ulLogConfType;
        pNewResDesInfo->ulLogConfTag = ulLogConfTag;
        pNewResDesInfo->ulResDesType = ulNextResDesType;
        pNewResDesInfo->ulResDesTag = ulNextResDesTag;

        *prdResDes = (RES_DES)pNewResDesInfo;
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Parent [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Parent(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Parent(%p %p %lx)\n",
          pdnDevInst, dnDevInst, ulFlags);

    return CM_Get_Parent_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Parent_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Parent_Ex(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szRelatedDevInst[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    DWORD dwIndex, dwLength = MAX_DEVICE_ID_LEN;
    CONFIGRET ret;

    TRACE("CM_Get_Parent_Ex(%p %lx %lx %p)\n",
          pdnDevInst, dnDevInst, ulFlags, hMachine);

    if (pdnDevInst == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    *pdnDevInst = -1;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetRelatedDeviceInstance(BindingHandle,
                                           PNP_GET_PARENT_DEVICE_INSTANCE,
                                           lpDevInst,
                                           szRelatedDevInst,
                                           &dwLength,
                                           0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return ret;

    TRACE("szRelatedDevInst: %s\n", debugstr_w(szRelatedDevInst));

    dwIndex = pSetupStringTableAddString(StringTable, szRelatedDevInst, 1);
    if (dwIndex == -1)
        return CR_FAILURE;

    *pdnDevInst = dwIndex;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Res_Des_Data [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Res_Des_Data(
    _In_ RES_DES rdResDes,
    _Out_writes_bytes_(BufferLen) PVOID Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Res_Des_Data(%p %p %lu %lx)\n",
          rdResDes, Buffer, BufferLen, ulFlags);

    return CM_Get_Res_Des_Data_Ex(rdResDes, Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Res_Des_Data_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Ex(
    _In_ RES_DES rdResDes,
    _Out_writes_bytes_(BufferLen) PVOID Buffer,
    _In_ ULONG BufferLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    FIXME("CM_Get_Res_Des_Data_Ex(%p %p %lu %lx %p)\n",
          rdResDes, Buffer, BufferLen, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Res_Des_Size [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size(
    _Out_ PULONG pulSize,
    _In_ RES_DES rdResDes,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Res_Des_Data_Size(%p %p %lx)\n",
          pulSize, rdResDes, ulFlags);

    return CM_Get_Res_Des_Data_Size_Ex(pulSize, rdResDes, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Res_Des_Size_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Res_Des_Data_Size_Ex(
    _Out_ PULONG pulSize,
    _In_ RES_DES rdResDes,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    TRACE("CM_Get_Res_Des_Data_Size_Ex(%p %p %lx %p)\n",
          pulSize, rdResDes, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Resource_Conflict_Count [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_Count(
    _In_ CONFLICT_LIST clConflictList,
    _Out_ PULONG pulCount)
{
    PCONFLICT_DATA pConflictData;

    FIXME("CM_Get_Resource_Conflict_Count(%p %p)\n",
          clConflictList, pulCount);

    pConflictData = (PCONFLICT_DATA)clConflictList;
    if (!IsValidConflictData(pConflictData))
        return CR_INVALID_CONFLICT_LIST;

    if (pulCount == NULL)
        return CR_INVALID_POINTER;

    *pulCount = pConflictData->pConflictList->ConflictsListed;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Resource_Conflict_DetailsA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsA(
    _In_ CONFLICT_LIST clConflictList,
    _In_ ULONG ulIndex,
    _Inout_ PCONFLICT_DETAILS_A pConflictDetails)
{
    FIXME("CM_Get_Resource_Conflict_CountA(%p %lu %p)\n",
          clConflictList, ulIndex, pConflictDetails);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Resource_Conflict_DetailsW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Resource_Conflict_DetailsW(
    _In_ CONFLICT_LIST clConflictList,
    _In_ ULONG ulIndex,
    _Inout_ PCONFLICT_DETAILS_W pConflictDetails)
{
    FIXME("CM_Get_Resource_Conflict_CountW(%p %lu %p)\n",
          clConflictList, ulIndex, pConflictDetails);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Sibling [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Sibling(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Get_Sibling(%p %p %lx)\n",
          pdnDevInst, dnDevInst, ulFlags);

    return CM_Get_Sibling_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Sibling_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Get_Sibling_Ex(
    _Out_ PDEVINST pdnDevInst,
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szRelatedDevInst[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    DWORD dwIndex, dwLength = MAX_DEVICE_ID_LEN;
    CONFIGRET ret;

    TRACE("CM_Get_Sibling_Ex(%p %lx %lx %p)\n",
          pdnDevInst, dnDevInst, ulFlags, hMachine);

    if (pdnDevInst == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    *pdnDevInst = -1;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_GetRelatedDeviceInstance(BindingHandle,
                                           PNP_GET_SIBLING_DEVICE_INSTANCE,
                                           lpDevInst,
                                           szRelatedDevInst,
                                           &dwLength,
                                           0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return ret;

    TRACE("szRelatedDevInst: %s\n", debugstr_w(szRelatedDevInst));

    dwIndex = pSetupStringTableAddString(StringTable, szRelatedDevInst, 1);
    if (dwIndex == -1)
        return CR_FAILURE;

    *pdnDevInst = dwIndex;

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Version [SETUPAPI.@]
 */
WORD
WINAPI
CM_Get_Version(VOID)
{
    TRACE("CM_Get_Version()\n");

    return CM_Get_Version_Ex(NULL);
}


/***********************************************************************
 * CM_Get_Version_Ex [SETUPAPI.@]
 */
WORD
WINAPI
CM_Get_Version_Ex(
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WORD Version = 0;
    CONFIGRET ret;

    TRACE("CM_Get_Version_Ex(%p)\n", hMachine);

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return 0;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_GetVersion(BindingHandle, &Version);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return 0;

    return Version;
}


/***********************************************************************
 * CM_Intersect_Range_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Intersect_Range_List(
    _In_ RANGE_LIST rlhOld1,
    _In_ RANGE_LIST rlhOld2,
    _In_ RANGE_LIST rlhNew,
    _In_ ULONG ulFlags)
{
    FIXME("CM_Intersect_Range_List(%p %p %p %lx)\n",
          rlhOld1, rlhOld2, rlhNew, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Invert_Range_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Invert_Range_List(
    _In_ RANGE_LIST rlhOld,
    _In_ RANGE_LIST rlhNew,
    _In_ DWORDLONG ullMaxValue,
    _In_ ULONG ulFlags)
{
    FIXME("CM_Invert_Range_List(%p %p %I64u %lx)\n",
          rlhOld, rlhNew, ullMaxValue, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Is_Dock_Station_Present [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present(
    _Out_ PBOOL pbPresent)
{
    TRACE("CM_Is_Dock_Station_Present(%p)\n",
          pbPresent);

    return CM_Is_Dock_Station_Present_Ex(pbPresent, NULL);
}


/***********************************************************************
 * CM_Is_Dock_Station_Present_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Is_Dock_Station_Present_Ex(
    _Out_ PBOOL pbPresent,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Is_Dock_Station_Present_Ex(%p %p)\n",
          pbPresent, hMachine);

    if (pbPresent == NULL)
        return CR_INVALID_POINTER;

    *pbPresent = FALSE;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_IsDockStationPresent(BindingHandle,
                                       pbPresent);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Is_Version_Available_Ex [SETUPAPI.@]
 */
BOOL
WINAPI
CM_Is_Version_Available(
     _In_ WORD wVersion)
{
    TRACE("CM_Is_Version_Available(%hu)\n",
          wVersion);

    return CM_Is_Version_Available_Ex(wVersion, NULL);
}


/***********************************************************************
 * CM_Is_Version_Available_Ex [SETUPAPI.@]
 */
BOOL
WINAPI
CM_Is_Version_Available_Ex(
    _In_ WORD wVersion,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WORD wServerVersion;
    CONFIGRET ret;

    TRACE("CM_Is_Version_Available_Ex(%hu %p)\n",
          wVersion, hMachine);

    if (wVersion <= 0x400)
        return TRUE;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return FALSE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return FALSE;
    }

    RpcTryExcept
    {
        ret = PNP_GetVersion(BindingHandle, &wServerVersion);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        return FALSE;

    return (wServerVersion >= wVersion);
}


/***********************************************************************
 * CM_Locate_DevNodeA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Locate_DevNodeA(
    _Out_ PDEVINST pdnDevInst,
    _In_opt_ DEVINSTID_A pDeviceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Locate_DevNodeA(%p %s %lx)\n",
          pdnDevInst, debugstr_a(pDeviceID), ulFlags);

    return CM_Locate_DevNode_ExA(pdnDevInst, pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Locate_DevNodeW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Locate_DevNodeW(
    _Out_ PDEVINST pdnDevInst,
    _In_opt_ DEVINSTID_W pDeviceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Locate_DevNodeW(%p %s %lx)\n",
          pdnDevInst, debugstr_w(pDeviceID), ulFlags);

    return CM_Locate_DevNode_ExW(pdnDevInst, pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Locate_DevNode_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Locate_DevNode_ExA(
    _Out_ PDEVINST pdnDevInst,
    _In_opt_ DEVINSTID_A pDeviceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    DEVINSTID_W pDevIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Locate_DevNode_ExA(%p %s %lx %p)\n",
          pdnDevInst, debugstr_a(pDeviceID), ulFlags, hMachine);

    if (pDeviceID != NULL)
    {
       if (pSetupCaptureAndConvertAnsiArg(pDeviceID, &pDevIdW))
         return CR_INVALID_DEVICE_ID;
    }

    ret = CM_Locate_DevNode_ExW(pdnDevInst, pDevIdW, ulFlags, hMachine);

    if (pDevIdW != NULL)
        MyFree(pDevIdW);

    return ret;
}


/***********************************************************************
 * CM_Locate_DevNode_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Locate_DevNode_ExW(
    _Out_ PDEVINST pdnDevInst,
    _In_opt_ DEVINSTID_W pDeviceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR DeviceIdBuffer[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Locate_DevNode_ExW(%p %s %lx %p)\n",
          pdnDevInst, debugstr_w(pDeviceID), ulFlags, hMachine);

    if (pdnDevInst == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~CM_LOCATE_DEVNODE_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    if (pDeviceID != NULL && lstrlenW(pDeviceID) != 0)
    {
        lstrcpyW(DeviceIdBuffer, pDeviceID);

        RpcTryExcept
        {
            /* Validate the device ID */
            ret = PNP_ValidateDeviceInstance(BindingHandle,
                                             DeviceIdBuffer,
                                             ulFlags);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = RpcStatusToCmStatus(RpcExceptionCode());
        }
        RpcEndExcept;
    }
    else
    {
        RpcTryExcept
        {
            /* Get the root device ID */
            ret = PNP_GetRootDeviceInstance(BindingHandle,
                                            DeviceIdBuffer,
                                            MAX_DEVICE_ID_LEN);
        }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
        {
            ret = RpcStatusToCmStatus(RpcExceptionCode());
        }
        RpcEndExcept;
    }

    TRACE("DeviceIdBuffer: %s\n", debugstr_w(DeviceIdBuffer));

    if (ret == CR_SUCCESS)
    {
        *pdnDevInst = pSetupStringTableAddString(StringTable, DeviceIdBuffer, 1);
        if (*pdnDevInst == -1)
            ret = CR_FAILURE;
    }

    return ret;
}


/***********************************************************************
 * CM_Merge_Range_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Merge_Range_List(
    _In_ RANGE_LIST rlhOld1,
    _In_ RANGE_LIST rlhOld2,
    _In_ RANGE_LIST rlhNew,
    _In_ ULONG ulFlags)
{
    FIXME("CM_Merge_Range_List(%p %p %p %lx)\n",
          rlhOld1, rlhOld2, rlhNew, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Modify_Res_Des [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Modify_Res_Des(
    _Out_ PRES_DES prdResDes,
    _In_ RES_DES rdResDes,
    _In_ RESOURCEID ResourceID,
    _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Modify_Res_Des(%p %p %lx %p %lu %lx)\n",
          prdResDes, rdResDes, ResourceID, ResourceData,
          ResourceLen, ulFlags);

    return CM_Modify_Res_Des_Ex(prdResDes, rdResDes, ResourceID, ResourceData,
                                ResourceLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Modify_Res_Des_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Modify_Res_Des_Ex(
    _Out_ PRES_DES prdResDes,
    _In_ RES_DES rdResDes,
    _In_ RESOURCEID ResourceID,
    _In_reads_bytes_(ResourceLen) PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    FIXME("CM_Modify_Res_Des_Ex(%p %p %lx %p %lu %lx %p)\n",
          prdResDes, rdResDes, ResourceID, ResourceData,
          ResourceLen, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Move_DevNode [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Move_DevNode(
    _In_ DEVINST dnFromDevInst,
    _In_ DEVINST dnToDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Move_DevNode(%lx %lx %lx)\n",
          dnFromDevInst, dnToDevInst, ulFlags);

    return CM_Move_DevNode_Ex(dnFromDevInst, dnToDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Move_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Move_DevNode_Ex(
    _In_ DEVINST dnFromDevInst,
    _In_ DEVINST dnToDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpFromDevInst;
    LPWSTR lpToDevInst;
    CONFIGRET ret;

    FIXME("CM_Move_DevNode_Ex(%lx %lx %lx %p)\n",
          dnFromDevInst, dnToDevInst, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (dnFromDevInst == 0 || dnToDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpFromDevInst = pSetupStringTableStringFromId(StringTable, dnFromDevInst);
    if (lpFromDevInst == NULL)
        return CR_INVALID_DEVNODE;

    lpToDevInst = pSetupStringTableStringFromId(StringTable, dnToDevInst);
    if (lpToDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_DeviceInstanceAction(BindingHandle,
                                       PNP_DEVINST_MOVE,
                                       ulFlags,
                                       lpFromDevInst,
                                       lpToDevInst);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Next_Range [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Next_Range(
    _Inout_ PRANGE_ELEMENT preElement,
    _Out_ PDWORDLONG pullStart,
    _Out_ PDWORDLONG pullEnd,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pRangeList;
    PINTERNAL_RANGE pRange;
    PLIST_ENTRY ListEntry;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_Next_Range(%p %p %p %lx)\n",
          preElement, pullStart, pullEnd, ulFlags);

    pRange = (PINTERNAL_RANGE)preElement;

    if (pRange == NULL || pRange->pRangeList == NULL)
        return CR_FAILURE;

    if (pullStart == NULL || pullEnd == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    pRangeList = pRange->pRangeList;

    /* Lock the range list */
    WaitForSingleObject(pRangeList->hMutex, INFINITE);

    /* Fail, if we reached the end of the list */
    if (pRange->ListEntry.Flink == &pRangeList->ListHead)
    {
        ret = CR_FAILURE;
        goto done;
    }

    /* Get the next range */
    ListEntry = pRangeList->ListHead.Flink;
    pRange = CONTAINING_RECORD(ListEntry, INTERNAL_RANGE, ListEntry);

    /* Return the range data */
    *pullStart = pRange->ullStart;
    *pullEnd = pRange->ullEnd;
    *preElement = (RANGE_ELEMENT)pRange;

done:
    /* Unlock the range list */
    ReleaseMutex(pRangeList->hMutex);

    return ret;
}


/***********************************************************************
 * CM_Open_Class_KeyA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Open_Class_KeyA(
    _In_opt_ LPGUID pClassGuid,
    _In_opt_ LPCSTR pszClassName,
    _In_ REGSAM samDesired,
    _In_ REGDISPOSITION Disposition,
    _Out_ PHKEY phkClass,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Open_Class_KeyA(%p %s %lx %lx %p %lx)\n",
          debugstr_guid(pClassGuid), debugstr_a(pszClassName),
          samDesired, Disposition, phkClass, ulFlags);

    return CM_Open_Class_Key_ExA(pClassGuid, pszClassName, samDesired,
                                 Disposition, phkClass, ulFlags, NULL);
}


/***********************************************************************
 * CM_Open_Class_KeyW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Open_Class_KeyW(
    _In_opt_ LPGUID pClassGuid,
    _In_opt_ LPCWSTR pszClassName,
    _In_ REGSAM samDesired,
    _In_ REGDISPOSITION Disposition,
    _Out_ PHKEY phkClass,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Open_Class_KeyW(%p %s %lx %lx %p %lx)\n",
          debugstr_guid(pClassGuid), debugstr_w(pszClassName),
          samDesired, Disposition, phkClass, ulFlags);

    return CM_Open_Class_Key_ExW(pClassGuid, pszClassName, samDesired,
                                 Disposition, phkClass, ulFlags, NULL);
}


/***********************************************************************
 * CM_Open_Class_Key_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Open_Class_Key_ExA(
    _In_opt_ LPGUID pClassGuid,
    _In_opt_ LPCSTR pszClassName,
    _In_ REGSAM samDesired,
    _In_ REGDISPOSITION Disposition,
    _Out_ PHKEY phkClass,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR pszClassNameW = NULL;
    CONFIGRET ret;

    TRACE("CM_Open_Class_Key_ExA(%p %s %lx %lx %p %lx %p)\n",
          debugstr_guid(pClassGuid), debugstr_a(pszClassName),
          samDesired, Disposition, phkClass, ulFlags, hMachine);

    if (pszClassName != NULL)
    {
       if (pSetupCaptureAndConvertAnsiArg(pszClassName, &pszClassNameW))
         return CR_INVALID_DATA;
    }

    ret = CM_Open_Class_Key_ExW(pClassGuid, pszClassNameW, samDesired,
                                Disposition, phkClass, ulFlags, hMachine);

    if (pszClassNameW != NULL)
        MyFree(pszClassNameW);

    return ret;
}


/***********************************************************************
 * CM_Open_Class_Key_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Open_Class_Key_ExW(
    _In_opt_ LPGUID pClassGuid,
    _In_opt_ LPCWSTR pszClassName,
    _In_ REGSAM samDesired,
    _In_ REGDISPOSITION Disposition,
    _Out_ PHKEY phkClass,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    WCHAR szKeyName[MAX_PATH];
    LPWSTR lpGuidString;
    DWORD dwDisposition;
    DWORD dwError;
    HKEY hKey;

    TRACE("CM_Open_Class_Key_ExW(%p %s %lx %lx %p %lx %p)\n",
          debugstr_guid(pClassGuid), debugstr_w(pszClassName),
          samDesired, Disposition, phkClass, ulFlags, hMachine);

    /* Check Disposition and ulFlags */
    if ((Disposition & ~RegDisposition_Bits) ||
        (ulFlags & ~CM_OPEN_CLASS_KEY_BITS))
        return CR_INVALID_FLAG;

    /* Check phkClass */
    if (phkClass == NULL)
        return CR_INVALID_POINTER;

    *phkClass = NULL;

    if (ulFlags == CM_OPEN_CLASS_KEY_INTERFACE &&
        pszClassName != NULL)
        return CR_INVALID_DATA;

    if (hMachine == NULL)
    {
        hKey = HKEY_LOCAL_MACHINE;
    }
    else
    {
        if (RegConnectRegistryW(((PMACHINE_INFO)hMachine)->szMachineName,
                                HKEY_LOCAL_MACHINE,
                                &hKey))
            return CR_REGISTRY_ERROR;
    }

    if (ulFlags & CM_OPEN_CLASS_KEY_INTERFACE)
    {
        lstrcpyW(szKeyName, DeviceClasses);
    }
    else
    {
        lstrcpyW(szKeyName, ControlClass);
    }

    if (pClassGuid != NULL)
    {
        if (UuidToStringW((UUID*)pClassGuid, &lpGuidString) != RPC_S_OK)
        {
            RegCloseKey(hKey);
            return CR_INVALID_DATA;
        }

        lstrcatW(szKeyName, BackslashOpenBrace);
        lstrcatW(szKeyName, lpGuidString);
        lstrcatW(szKeyName, CloseBrace);
    }

    if (Disposition == RegDisposition_OpenAlways)
    {
        dwError = RegCreateKeyExW(hKey, szKeyName, 0, NULL, 0, samDesired,
                                  NULL, phkClass, &dwDisposition);
    }
    else
    {
        dwError = RegOpenKeyExW(hKey, szKeyName, 0, samDesired, phkClass);
    }

    RegCloseKey(hKey);

    if (pClassGuid != NULL)
        RpcStringFreeW(&lpGuidString);

    if (dwError != ERROR_SUCCESS)
    {
        *phkClass = NULL;
        return CR_NO_SUCH_REGISTRY_KEY;
    }

    if (pszClassName != NULL)
    {
        RegSetValueExW(*phkClass, Class, 0, REG_SZ, (LPBYTE)pszClassName,
                       (lstrlenW(pszClassName) + 1) * sizeof(WCHAR));
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Open_DevNode_Key [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Open_DevNode_Key(
    _In_ DEVINST dnDevNode,
    _In_ REGSAM samDesired,
    _In_ ULONG ulHardwareProfile,
    _In_ REGDISPOSITION Disposition,
    _Out_ PHKEY phkDevice,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Open_DevNode_Key(%lx %lx %lu %lx %p %lx)\n",
          dnDevNode, samDesired, ulHardwareProfile, Disposition, phkDevice, ulFlags);

    return CM_Open_DevNode_Key_Ex(dnDevNode, samDesired, ulHardwareProfile,
                                  Disposition, phkDevice, ulFlags, NULL);
}


/***********************************************************************
 * CM_Open_DevNode_Key_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Open_DevNode_Key_Ex(
    _In_ DEVINST dnDevNode,
    _In_ REGSAM samDesired,
    _In_ ULONG ulHardwareProfile,
    _In_ REGDISPOSITION Disposition,
    _Out_ PHKEY phkDevice,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR pszDevInst, pszKeyPath = NULL, pszInstancePath = NULL;
    LONG lError;
    DWORD dwDisposition;
    HKEY hRootKey = NULL;
    CONFIGRET ret = CR_CALL_NOT_IMPLEMENTED;

    TRACE("CM_Open_DevNode_Key_Ex(%lx %lx %lu %lx %p %lx %p)\n",
          dnDevNode, samDesired, ulHardwareProfile, Disposition, phkDevice, ulFlags, hMachine);

    if (phkDevice == NULL)
        return CR_INVALID_POINTER;

    *phkDevice = NULL;

    if (dnDevNode == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_REGISTRY_BITS)
        return CR_INVALID_FLAG;

    if (Disposition & ~RegDisposition_Bits)
        return CR_INVALID_DATA;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    pszDevInst = pSetupStringTableStringFromId(StringTable, dnDevNode);
    if (pszDevInst == NULL)
        return CR_INVALID_DEVNODE;

    TRACE("pszDevInst: %S\n", pszDevInst);

    pszKeyPath = MyMalloc(512 * sizeof(WCHAR));
    if (pszKeyPath == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    pszInstancePath = MyMalloc(512 * sizeof(WCHAR));
    if (pszInstancePath == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    ret = GetDeviceInstanceKeyPath(BindingHandle,
                                   pszDevInst,
                                   pszKeyPath,
                                   pszInstancePath,
                                   ulHardwareProfile,
                                   ulFlags);
    if (ret != CR_SUCCESS)
        goto done;

    TRACE("pszKeyPath: %S\n", pszKeyPath);
    TRACE("pszInstancePath: %S\n", pszInstancePath);

    wcscat(pszKeyPath, L"\\");
    wcscat(pszKeyPath, pszInstancePath);

    TRACE("pszKeyPath: %S\n", pszKeyPath);

    if (hMachine == NULL)
    {
        hRootKey = HKEY_LOCAL_MACHINE;
    }
    else
    {
        if (RegConnectRegistryW(((PMACHINE_INFO)hMachine)->szMachineName,
                                HKEY_LOCAL_MACHINE,
                                &hRootKey))
        {
            ret = CR_REGISTRY_ERROR;
            goto done;
        }
    }

    if (Disposition == RegDisposition_OpenAlways)
    {
        lError = RegCreateKeyExW(hRootKey,
                                 pszKeyPath,
                                 0,
                                 NULL,
                                 0,
                                 samDesired,
                                 NULL,
                                 phkDevice,
                                 &dwDisposition);
    }
    else
    {
        lError = RegOpenKeyExW(hRootKey,
                               pszKeyPath,
                               0,
                               samDesired,
                               phkDevice);
    }

    if (lError != ERROR_SUCCESS)
    {
        *phkDevice = NULL;
        ret = CR_NO_SUCH_REGISTRY_KEY;
    }

done:
    if ((hRootKey != NULL) && (hRootKey != HKEY_LOCAL_MACHINE))
        RegCloseKey(hRootKey);

    if (pszInstancePath != NULL)
        MyFree(pszInstancePath);

    if (pszKeyPath != NULL)
        MyFree(pszKeyPath);

    return ret;
}


/***********************************************************************
 * CM_Query_And_Remove_SubTreeA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeA(
    _In_ DEVINST dnAncestor,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Query_And_Remove_SubTreeA(%lx %p %p %lu %lx)\n",
          dnAncestor, pVetoType, pszVetoName, ulNameLength, ulFlags);

    return CM_Query_And_Remove_SubTree_ExA(dnAncestor, pVetoType, pszVetoName,
                                           ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_And_Remove_SubTreeW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTreeW(
    _In_ DEVINST dnAncestor,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Query_And_Remove_SubTreeW(%lx %p %p %lu %lx)\n",
          dnAncestor, pVetoType, pszVetoName, ulNameLength, ulFlags);

    return CM_Query_And_Remove_SubTree_ExW(dnAncestor, pVetoType, pszVetoName,
                                           ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_And_Remove_SubTree_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExA(
    _In_ DEVINST dnAncestor,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR lpLocalVetoName;
    CONFIGRET ret;

    TRACE("CM_Query_And_Remove_SubTree_ExA(%lx %p %p %lu %lx %p)\n",
          dnAncestor, pVetoType, pszVetoName, ulNameLength,
          ulFlags, hMachine);

    if (pszVetoName == NULL && ulNameLength == 0)
        return CR_INVALID_POINTER;

    lpLocalVetoName = HeapAlloc(GetProcessHeap(), 0, ulNameLength * sizeof(WCHAR));
    if (lpLocalVetoName == NULL)
        return CR_OUT_OF_MEMORY;

    ret = CM_Query_And_Remove_SubTree_ExW(dnAncestor, pVetoType, lpLocalVetoName,
                                          ulNameLength, ulFlags, hMachine);
    if (ret == CR_REMOVE_VETOED)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                lpLocalVetoName,
                                ulNameLength,
                                pszVetoName,
                                ulNameLength,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
    }

    HeapFree(GetProcessHeap(), 0, lpLocalVetoName);

    return ret;
}


/***********************************************************************
 * CM_Query_And_Remove_SubTree_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_And_Remove_SubTree_ExW(
    _In_ DEVINST dnAncestor,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Query_And_Remove_SubTree_ExW(%lx %p %p %lu %lx %p)\n",
          dnAncestor, pVetoType, pszVetoName, ulNameLength,
          ulFlags, hMachine);

    if (dnAncestor == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_REMOVE_BITS)
        return CR_INVALID_FLAG;

    if (pszVetoName == NULL && ulNameLength == 0)
        return CR_INVALID_POINTER;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnAncestor);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_QueryRemove(BindingHandle,
                              lpDevInst,
                              pVetoType,
                              pszVetoName,
                              ulNameLength,
                              ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Query_Arbitrator_Free_Data [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data(
    _Out_writes_bytes_(DataLen) PVOID pData,
    _In_ ULONG DataLen,
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Query_Arbitrator_Free_Data(%p %lu %lx %lu 0x%08lx)\n",
          pData, DataLen, dnDevInst, ResourceID, ulFlags);

    return CM_Query_Arbitrator_Free_Data_Ex(pData, DataLen, dnDevInst,
                                            ResourceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_Arbitrator_Free_Data_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Data_Ex(
    _Out_writes_bytes_(DataLen) PVOID pData,
    _In_ ULONG DataLen,
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Query_Arbitrator_Free_Data_Ex(%p %lu %lx %lu 0x%08lx %p)\n",
          pData, DataLen, dnDevInst, ResourceID, ulFlags, hMachine);

    if (pData == NULL || DataLen == 0)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags & ~CM_QUERY_ARBITRATOR_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_QueryArbitratorFreeData(BindingHandle,
                                          pData,
                                          DataLen,
                                          lpDevInst,
                                          ResourceID,
                                          ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Query_Arbitrator_Free_Size [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size(
    _Out_ PULONG pulSize,
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Query_Arbitrator_Free_Size(%p %lu %lx 0x%08lx)\n",
          pulSize, dnDevInst,ResourceID, ulFlags);

    return CM_Query_Arbitrator_Free_Size_Ex(pulSize, dnDevInst, ResourceID,
                                            ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_Arbitrator_Free_Size_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_Arbitrator_Free_Size_Ex(
    _Out_ PULONG pulSize,
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Query_Arbitrator_Free_Size_Ex(%p %lu %lx 0x%08lx %p)\n",
          pulSize, dnDevInst,ResourceID, ulFlags, hMachine);

    if (pulSize == NULL)
        return CR_INVALID_POINTER;

    if (dnDevInst == 0)
        return CR_INVALID_DEVINST;

    if (ulFlags & ~CM_QUERY_ARBITRATOR_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_QueryArbitratorFreeSize(BindingHandle,
                                          pulSize,
                                          lpDevInst,
                                          ResourceID,
                                          ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Query_Remove_SubTree [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET
WINAPI
CM_Query_Remove_SubTree(
    _In_ DEVINST dnAncestor,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Query_Remove_SubTree(%lx %lx)\n",
          dnAncestor, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Query_Remove_SubTree_Ex [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET
WINAPI
CM_Query_Remove_SubTree_Ex(
    _In_ DEVINST dnAncestor,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    TRACE("CM_Query_Remove_SubTree_Ex(%lx %lx %p)\n",
          dnAncestor, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Query_Resource_Conflict_List [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Query_Resource_Conflict_List(
    _Out_ PCONFLICT_LIST pclConflictList,
    _In_ DEVINST dnDevInst,
    _In_ RESOURCEID ResourceID,
    _In_ PCVOID ResourceData,
    _In_ ULONG ResourceLen,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PPNP_CONFLICT_LIST pConflictBuffer = NULL;
    PCONFLICT_DATA pConflictData = NULL;
    ULONG ulBufferLength;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("CM_Query_Resource_Conflict_List(%p %lx %lu %p %lu %lx %p)\n",
          pclConflictList, dnDevInst, ResourceID, ResourceData,
          ResourceLen, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_RESDES_WIDTH_BITS)
        return CR_INVALID_FLAG;

    if (pclConflictList == NULL ||
        ResourceData == NULL ||
        ResourceLen == 0)
        return CR_INVALID_POINTER;

    if (ResourceID == 0)
        return CR_INVALID_RESOURCEID;

    *pclConflictList = 0;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    pConflictData = MyMalloc(sizeof(CONFLICT_DATA));
    if (pConflictData == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    ulBufferLength = sizeof(PNP_CONFLICT_LIST) +
                     sizeof(PNP_CONFLICT_STRINGS) +
                     (sizeof(wchar_t) * 200);
    pConflictBuffer = MyMalloc(ulBufferLength);
    if (pConflictBuffer == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto done;
    }

    RpcTryExcept
    {
        ret = PNP_QueryResConfList(BindingHandle,
                                   lpDevInst,
                                   ResourceID,
                                   (PBYTE)ResourceData,
                                   ResourceLen,
                                   (PBYTE)pConflictBuffer,
                                   ulBufferLength,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret != CR_SUCCESS)
        goto done;

    pConflictData->ulMagic = CONFLICT_MAGIC;
    pConflictData->pConflictList = pConflictBuffer;

    *pclConflictList = (CONFLICT_LIST)pConflictData;

done:
    if (ret != CR_SUCCESS)
    {
        if (pConflictBuffer != NULL)
            MyFree(pConflictBuffer);

        if (pConflictData != NULL)
            MyFree(pConflictData);
    }

    return ret;
}


/***********************************************************************
 * CM_Reenumerate_DevNode [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Reenumerate_DevNode(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Reenumerate_DevNode(%lx %lx)\n",
          dnDevInst, ulFlags);

    return CM_Reenumerate_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Reenumerate_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI
CM_Reenumerate_DevNode_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("CM_Reenumerate_DevNode_Ex(%lx %lx %p)\n",
          dnDevInst, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_REENUMERATE_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_DeviceInstanceAction(BindingHandle,
                                       PNP_DEVINST_REENUMERATE,
                                       ulFlags,
                                       lpDevInst,
                                       NULL);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Register_Device_Driver [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Register_Device_Driver(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Register_Device_Driver(%lx 0x%08lx)\n",
          dnDevInst, ulFlags);

    return CM_Register_Device_Driver_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Register_Device_Driver_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Register_Device_Driver_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Register_Device_Driver_Ex(%lx 0x%08lx %p)\n",
          dnDevInst, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_REGISTER_DEVICE_DRIVER_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_RegisterDriver(BindingHandle,
                                 lpDevInst,
                                 ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Register_Device_InterfaceA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Register_Device_InterfaceA(
    _In_ DEVINST dnDevInst,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ LPCSTR pszReference,
    _Out_writes_(*pulLength) LPSTR pszDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Register_Device_InterfaceA(%lx %s %s %p %p %lx)\n",
          dnDevInst, debugstr_guid(InterfaceClassGuid),
          pszReference, pszDeviceInterface, pulLength, ulFlags);

    return CM_Register_Device_Interface_ExA(dnDevInst, InterfaceClassGuid,
                                            pszReference, pszDeviceInterface,
                                            pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Register_Device_InterfaceW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Register_Device_InterfaceW(
    _In_ DEVINST dnDevInst,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ LPCWSTR pszReference,
    _Out_writes_(*pulLength) LPWSTR pszDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Register_Device_InterfaceW(%lx %s %s %p %p %lx)\n",
          dnDevInst, debugstr_guid(InterfaceClassGuid),
          debugstr_w(pszReference), pszDeviceInterface, pulLength, ulFlags);

    return CM_Register_Device_Interface_ExW(dnDevInst, InterfaceClassGuid,
                                            pszReference, pszDeviceInterface,
                                            pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Register_Device_Interface_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExA(
    _In_ DEVINST dnDevInst,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ LPCSTR pszReference,
    _Out_writes_(*pulLength) LPSTR pszDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR pszReferenceW = NULL;
    LPWSTR pszDeviceInterfaceW;
    ULONG ulLength;
    CONFIGRET ret;

    TRACE("CM_Register_Device_Interface_ExA(%lx %s %s %p %p %lx %p)\n",
          dnDevInst, debugstr_guid(InterfaceClassGuid), debugstr_a(pszReference),
          pszDeviceInterface, pulLength, ulFlags, hMachine);

    if (pulLength == NULL || pszDeviceInterface == NULL)
        return CR_INVALID_POINTER;

    if (pszReference != NULL)
    {
        if (pSetupCaptureAndConvertAnsiArg(pszReference, &pszReferenceW))
            return CR_INVALID_DATA;
    }

    ulLength = *pulLength;

    pszDeviceInterfaceW = HeapAlloc(GetProcessHeap(), 0, ulLength * sizeof(WCHAR));
    if (pszDeviceInterfaceW == NULL)
    {
        ret = CR_OUT_OF_MEMORY;
        goto Done;
    }

    ret = CM_Register_Device_Interface_ExW(dnDevInst,
                                           InterfaceClassGuid,
                                           pszReferenceW,
                                           pszDeviceInterfaceW,
                                           &ulLength,
                                           ulFlags,
                                           hMachine);
    if (ret == CR_SUCCESS)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                pszDeviceInterfaceW,
                                ulLength,
                                pszDeviceInterface,
                                *pulLength,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
    }

    *pulLength = ulLength;

Done:
    if (pszDeviceInterfaceW != NULL)
        HeapFree(GetProcessHeap(), 0, pszDeviceInterfaceW);

    if (pszReferenceW != NULL)
        MyFree(pszReferenceW);

    return ret;
}


/***********************************************************************
 * CM_Register_Device_Interface_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Register_Device_Interface_ExW(
    _In_ DEVINST dnDevInst,
    _In_ LPGUID InterfaceClassGuid,
    _In_opt_ LPCWSTR pszReference,
    _Out_writes_(*pulLength) LPWSTR pszDeviceInterface,
    _Inout_ PULONG pulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    ULONG ulTransferLength;
    CONFIGRET ret;

    TRACE("CM_Register_Device_Interface_ExW(%lx %s %s %p %p %lx %p)\n",
          dnDevInst, debugstr_guid(InterfaceClassGuid), debugstr_w(pszReference),
          pszDeviceInterface, pulLength, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (InterfaceClassGuid == NULL ||
        pszDeviceInterface == NULL ||
        pulLength == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ulTransferLength = *pulLength;

    RpcTryExcept
    {
        ret = PNP_RegisterDeviceClassAssociation(BindingHandle,
                                                 lpDevInst,
                                                 InterfaceClassGuid,
                                                 (LPWSTR)pszReference,
                                                 pszDeviceInterface,
                                                 pulLength,
                                                 &ulTransferLength,
                                                 0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Remove_SubTree [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET
WINAPI
CM_Remove_SubTree(
    _In_ DEVINST dnAncestor,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Remove_SubTree(%lx %lx)\n",
          dnAncestor, ulFlags);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Remove_SubTree_Ex [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET
WINAPI
CM_Remove_SubTree_Ex(
    _In_ DEVINST dnAncestor,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    TRACE("CM_Remove_SubTree_Ex(%lx %lx %p)\n",
          dnAncestor, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Request_Device_EjectA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Request_Device_EjectA(
    _In_ DEVINST dnDevInst,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Request_Device_EjectA(%lx %p %p %lu %lx)\n",
          dnDevInst, pVetoType, pszVetoName, ulNameLength, ulFlags);

    return CM_Request_Device_Eject_ExA(dnDevInst, pVetoType, pszVetoName,
                                       ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Request_Device_EjectW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Request_Device_EjectW(
    _In_ DEVINST dnDevInst,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Request_Device_EjectW(%lx %p %p %lu %lx)\n",
          dnDevInst, pVetoType, pszVetoName, ulNameLength, ulFlags);

    return CM_Request_Device_Eject_ExW(dnDevInst, pVetoType, pszVetoName,
                                       ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Request_Device_Eject_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExA(
    _In_ DEVINST dnDevInst,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR lpLocalVetoName = NULL;
    CONFIGRET ret;

    TRACE("CM_Request_Device_Eject_ExA(%lx %p %p %lu %lx %p)\n",
          dnDevInst, pVetoType, pszVetoName, ulNameLength, ulFlags, hMachine);

    if (ulNameLength != 0)
    {
        if (pszVetoName == NULL)
            return CR_INVALID_POINTER;

        lpLocalVetoName = HeapAlloc(GetProcessHeap(), 0, ulNameLength * sizeof(WCHAR));
        if (lpLocalVetoName == NULL)
            return CR_OUT_OF_MEMORY;
    }

    ret = CM_Request_Device_Eject_ExW(dnDevInst, pVetoType, lpLocalVetoName,
                                      ulNameLength, ulFlags, hMachine);
    if (ret == CR_REMOVE_VETOED && ulNameLength != 0)
    {
        if (WideCharToMultiByte(CP_ACP,
                                0,
                                lpLocalVetoName,
                                ulNameLength,
                                pszVetoName,
                                ulNameLength,
                                NULL,
                                NULL) == 0)
            ret = CR_FAILURE;
    }

    HeapFree(GetProcessHeap(), 0, lpLocalVetoName);

    return ret;
}


/***********************************************************************
 * CM_Request_Device_Eject_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Request_Device_Eject_ExW(
    _In_ DEVINST dnDevInst,
    _Out_opt_ PPNP_VETO_TYPE pVetoType,
    _Out_writes_opt_(ulNameLength) LPWSTR pszVetoName,
    _In_ ULONG ulNameLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Request_Device_Eject_ExW(%lx %p %p %lu %lx %p)\n",
          dnDevInst, pVetoType, pszVetoName, ulNameLength, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (pszVetoName == NULL && ulNameLength != 0)
        return CR_INVALID_POINTER;

    /* Windows 2003 SP2 ignores pszVetoName when ulNameLength is zero
     * and behaves like when pszVetoName is NULL */
    if (ulNameLength == 0)
        pszVetoName = NULL;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_RequestDeviceEject(BindingHandle,
                                     lpDevInst,
                                     pVetoType,
                                     pszVetoName,
                                     ulNameLength,
                                     ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Request_Eject_PC [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Request_Eject_PC(VOID)
{
    TRACE("CM_Request_Eject_PC()\n");

    return CM_Request_Eject_PC_Ex(NULL);
}


/***********************************************************************
 * CM_Request_Eject_PC_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Request_Eject_PC_Ex(
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Request_Eject_PC_Ex(%p)\n", hMachine);

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_RequestEjectPC(BindingHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Run_Detection [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Run_Detection(
    _In_ ULONG ulFlags)
{
    TRACE("CM_Run_Detection(%lx)\n", ulFlags);

    return CM_Run_Detection_Ex(ulFlags, NULL);
}


/***********************************************************************
 * CM_Run_Detection_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Run_Detection_Ex(
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Run_Detection_Ex(%lx %p)\n",
          ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (ulFlags & ~CM_DETECT_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_RunDetection(BindingHandle,
                               ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Set_Class_Registry_PropertyA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_Class_Registry_PropertyA(
    _In_ LPGUID ClassGuid,
    _In_ ULONG ulProperty,
    _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
    _In_ ULONG ulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR lpBuffer;
    ULONG ulType;
    CONFIGRET ret;

    TRACE("CM_Set_Class_Registry_PropertyA(%p %lx %p %lu %lx %p)\n",
          ClassGuid, ulProperty, Buffer, ulLength, ulFlags, hMachine);

    if (ClassGuid == NULL)
        return CR_INVALID_POINTER;

    if ((Buffer == NULL) && (ulLength != 0))
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (Buffer == NULL)
    {
        ret = CM_Set_Class_Registry_PropertyW(ClassGuid,
                                              ulProperty,
                                              Buffer,
                                              ulLength,
                                              ulFlags,
                                              hMachine);
    }
    else
    {
        /* Get property type */
        ulType = GetRegistryPropertyType(ulProperty);

        /* Allocate buffer if needed */
        if ((ulType == REG_SZ) || (ulType == REG_MULTI_SZ))
        {
            lpBuffer = MyMalloc(ulLength * sizeof(WCHAR));
            if (lpBuffer == NULL)
            {
                ret = CR_OUT_OF_MEMORY;
            }
            else
            {
                if (!MultiByteToWideChar(CP_ACP, 0, Buffer,
                                         ulLength, lpBuffer, ulLength))
                {
                    MyFree(lpBuffer);
                    ret = CR_FAILURE;
                }
                else
                {
                    ret = CM_Set_Class_Registry_PropertyW(ClassGuid,
                                                          ulProperty,
                                                          lpBuffer,
                                                          ulLength * sizeof(WCHAR),
                                                          ulFlags,
                                                          hMachine);
                    MyFree(lpBuffer);
                }
            }
        }
        else
        {
            ret = CM_Set_Class_Registry_PropertyW(ClassGuid,
                                                  ulProperty,
                                                  Buffer,
                                                  ulLength,
                                                  ulFlags,
                                                  hMachine);
        }

    }

    return ret;
}


/***********************************************************************
 * CM_Set_Class_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_Class_Registry_PropertyW(
    _In_ LPGUID ClassGuid,
    _In_ ULONG ulProperty,
    _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
    _In_ ULONG ulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WCHAR szGuidString[PNP_MAX_GUID_STRING_LEN + 1];
    ULONG ulType = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    ULONG SecurityDescriptorSize = 0;
    CONFIGRET ret;

    TRACE("CM_Set_Class_Registry_PropertyW(%p %lx %p %lu %lx %p)\n",
          ClassGuid, ulProperty, Buffer, ulLength, ulFlags, hMachine);

    if (ClassGuid == NULL)
        return CR_INVALID_POINTER;

    if ((Buffer == NULL) && (ulLength != 0))
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (pSetupStringFromGuid(ClassGuid,
                             szGuidString,
                             PNP_MAX_GUID_STRING_LEN) != 0)
        return CR_INVALID_DATA;

    if ((ulProperty < CM_CRP_MIN) || (ulProperty > CM_CRP_MAX))
        return CR_INVALID_PROPERTY;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    ulType = GetRegistryPropertyType(ulProperty);
    if ((ulType == REG_DWORD) && (ulLength != sizeof(DWORD)))
        return CR_INVALID_DATA;

    if (ulProperty == CM_CRP_SECURITY_SDS)
    {
        if (ulLength != 0)
        {
            if (!ConvertStringSecurityDescriptorToSecurityDescriptorW((LPCWSTR)Buffer,
                                                                      SDDL_REVISION_1,
                                                                      &pSecurityDescriptor,
                                                                      &SecurityDescriptorSize))
            {
                ERR("ConvertStringSecurityDescriptorToSecurityDescriptorW() failed (Error %lu)\n", GetLastError());
                return CR_INVALID_DATA;
            }

            Buffer = (PCVOID)pSecurityDescriptor;
            ulLength = SecurityDescriptorSize;
        }

        ulProperty = CM_CRP_SECURITY;
        ulType = REG_BINARY;
    }

    RpcTryExcept
    {
        ret = PNP_SetClassRegProp(BindingHandle,
                                  szGuidString,
                                  ulProperty,
                                  ulType,
                                  (LPBYTE)Buffer,
                                  ulLength,
                                  ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (pSecurityDescriptor)
        LocalFree(pSecurityDescriptor);

    return ret;
}


/***********************************************************************
 * CM_Set_DevNode_Problem [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_DevNode_Problem(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProblem,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Set_DevNode_Problem(%lx %lx %lx)\n",
          dnDevInst, ulProblem, ulFlags);

    return CM_Set_DevNode_Problem_Ex(dnDevInst, ulProblem, ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_DevNode_Problem_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_DevNode_Problem_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProblem,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Set_DevNode_Problem_Ex(%lx %lx %lx %p)\n",
          dnDevInst, ulProblem, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_SET_DEVNODE_PROBLEM_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_SetDeviceProblem(BindingHandle,
                                   lpDevInst,
                                   ulProblem,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Set_DevNode_Registry_PropertyA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyA(
  _In_ DEVINST dnDevInst,
  _In_ ULONG ulProperty,
  _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
  _In_ ULONG ulLength,
  _In_ ULONG ulFlags)
{
    TRACE("CM_Set_DevNode_Registry_PropertyA(%lx %lu %p %lx %lx)\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags);

    return CM_Set_DevNode_Registry_Property_ExA(dnDevInst, ulProperty,
                                                Buffer, ulLength,
                                                ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_DevNode_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_PropertyW(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
    _In_ ULONG ulLength,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Set_DevNode_Registry_PropertyW(%lx %lu %p %lx %lx)\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags);

    return CM_Set_DevNode_Registry_Property_ExW(dnDevInst, ulProperty,
                                                Buffer, ulLength,
                                                ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_DevNode_Registry_Property_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExA(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
    _In_ ULONG ulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpBuffer;
    ULONG ulType;

    FIXME("CM_Set_DevNode_Registry_Property_ExA(%lx %lu %p %lx %lx %p)\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags, hMachine);

    if (Buffer == NULL && ulLength != 0)
        return CR_INVALID_POINTER;

    if (ulProperty < CM_DRP_MIN || ulProperty > CM_DRP_MAX)
        return CR_INVALID_PROPERTY;

    if (Buffer == NULL)
    {
        ret = CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                   ulProperty,
                                                   NULL,
                                                   0,
                                                   ulFlags,
                                                   hMachine);
    }
    else
    {
        /* Get property type */
        ulType = GetRegistryPropertyType(ulProperty);

        /* Allocate buffer if needed */
        if (ulType == REG_SZ ||
            ulType == REG_MULTI_SZ)
        {
            lpBuffer = MyMalloc(ulLength * sizeof(WCHAR));
            if (lpBuffer == NULL)
            {
                ret = CR_OUT_OF_MEMORY;
            }
            else
            {
                if (!MultiByteToWideChar(CP_ACP, 0, Buffer,
                                         ulLength, lpBuffer, ulLength))
                {
                    MyFree(lpBuffer);
                    ret = CR_FAILURE;
                }
                else
                {
                    ret = CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                               ulProperty,
                                                               lpBuffer,
                                                               ulLength * sizeof(WCHAR),
                                                               ulFlags,
                                                               hMachine);
                    MyFree(lpBuffer);
                }
            }
        }
        else
        {
            ret = CM_Set_DevNode_Registry_Property_ExW(dnDevInst,
                                                       ulProperty,
                                                       Buffer,
                                                       ulLength,
                                                       ulFlags,
                                                       hMachine);
        }

        ret = CR_CALL_NOT_IMPLEMENTED;
    }

    return ret;
}


/***********************************************************************
 * CM_Set_DevNode_Registry_Property_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_DevNode_Registry_Property_ExW(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulProperty,
    _In_reads_bytes_opt_(ulLength) PCVOID Buffer,
    _In_ ULONG ulLength,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    ULONG ulType;
    CONFIGRET ret;

    TRACE("CM_Set_DevNode_Registry_Property_ExW(%lx %lu %p %lx %lx %p)\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulProperty <  CM_DRP_MIN || ulProperty > CM_DRP_MAX)
        return CR_INVALID_PROPERTY;

    if (Buffer != NULL && ulLength == 0)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    /* Get property type */
    ulType = GetRegistryPropertyType(ulProperty);

    RpcTryExcept
    {
        ret = PNP_SetDeviceRegProp(BindingHandle,
                                   lpDevInst,
                                   ulProperty,
                                   ulType,
                                   (BYTE *)Buffer,
                                   ulLength,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Set_HW_Prof [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_HW_Prof(
    _In_ ULONG ulHardwareProfile,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Set_HW_Prof(%lu %lx)\n",
          ulHardwareProfile, ulFlags);

    return CM_Set_HW_Prof_Ex(ulHardwareProfile, ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_HW_Prof_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_HW_Prof_Ex(
    _In_ ULONG ulHardwareProfile,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Set_HW_Prof_Ex(%lu %lx %p)\n",
          ulHardwareProfile, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_SetHwProf(BindingHandle, ulHardwareProfile, ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Set_HW_Prof_FlagsA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsA(
    _In_ DEVINSTID_A szDevInstName,
    _In_ ULONG ulConfig,
    _In_ ULONG ulValue,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Set_HW_Prof_FlagsA(%s %lu %lu %lx)\n",
          debugstr_a(szDevInstName), ulConfig, ulValue, ulFlags);

    return CM_Set_HW_Prof_Flags_ExA(szDevInstName, ulConfig, ulValue,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_HW_Prof_FlagsW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_HW_Prof_FlagsW(
    _In_ DEVINSTID_W szDevInstName,
    _In_ ULONG ulConfig,
    _In_ ULONG ulValue,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Set_HW_Prof_FlagsW(%s %lu %lu %lx)\n",
          debugstr_w(szDevInstName), ulConfig, ulValue, ulFlags);

    return CM_Set_HW_Prof_Flags_ExW(szDevInstName, ulConfig, ulValue,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_HW_Prof_Flags_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExA(
    _In_ DEVINSTID_A szDevInstName,
    _In_ ULONG ulConfig,
    _In_ ULONG ulValue,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    DEVINSTID_W pszDevIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CM_Set_HW_Prof_Flags_ExA(%s %lu %lu %lx %p)\n",
          debugstr_a(szDevInstName), ulConfig, ulValue, ulFlags, hMachine);

    if (szDevInstName != NULL)
    {
       if (pSetupCaptureAndConvertAnsiArg(szDevInstName, &pszDevIdW))
         return CR_INVALID_DEVICE_ID;
    }

    ret = CM_Set_HW_Prof_Flags_ExW(pszDevIdW, ulConfig, ulValue,
                                   ulFlags, hMachine);

    if (pszDevIdW != NULL)
        MyFree(pszDevIdW);

    return ret;
}


/***********************************************************************
 * CM_Set_HW_Prof_Flags_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Set_HW_Prof_Flags_ExW(
    _In_ DEVINSTID_W szDevInstName,
    _In_ ULONG ulConfig,
    _In_ ULONG ulValue,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    FIXME("CM_Set_HW_Prof_Flags_ExW(%s %lu %lu %lx %p)\n",
          debugstr_w(szDevInstName), ulConfig, ulValue, ulFlags, hMachine);

    if (szDevInstName == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags & ~ CM_SET_HW_PROF_FLAGS_BITS)
        return CR_INVALID_FLAG;

    /* FIXME: Check whether szDevInstName is valid */

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_HwProfFlags(BindingHandle, PNP_SET_HWPROFFLAGS, szDevInstName,
                              ulConfig, &ulValue, NULL, NULL, 0, 0);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Setup_DevNode [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Setup_DevNode(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Setup_DevNode(%lx %lx)\n",
          dnDevInst, ulFlags);

    return CM_Setup_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Setup_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Setup_DevNode_Ex(
    _In_ DEVINST dnDevInst,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("CM_Setup_DevNode_Ex(%lx %lx %p)\n",
          dnDevInst, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags & ~CM_SETUP_BITS)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_DeviceInstanceAction(BindingHandle,
                                       PNP_DEVINST_SETUP,
                                       ulFlags,
                                       lpDevInst,
                                       NULL);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Test_Range_Available [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Test_Range_Available(
    _In_ DWORDLONG ullStartValue,
    _In_ DWORDLONG ullEndValue,
    _In_ RANGE_LIST rlh,
    _In_ ULONG ulFlags)
{
    PINTERNAL_RANGE_LIST pRangeList;
    PINTERNAL_RANGE pRange;
    PLIST_ENTRY ListEntry;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("CM_Test_Range_Available(%I64u %I64u %p %lx)\n",
          ullStartValue, ullEndValue, rlh, ulFlags);

    pRangeList = (PINTERNAL_RANGE_LIST)rlh;

    if (!IsValidRangeList(pRangeList))
        return CR_INVALID_RANGE_LIST;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (ullStartValue > ullEndValue)
        return CR_INVALID_RANGE;

    /* Lock the range list */
    WaitForSingleObject(pRangeList->hMutex, INFINITE);

    /* Check the ranges */
    ListEntry = &pRangeList->ListHead;
    while (ListEntry->Flink == &pRangeList->ListHead)
    {
        pRange = CONTAINING_RECORD(ListEntry, INTERNAL_RANGE, ListEntry);

        /* Check if the start value is within the current range */
        if ((ullStartValue >= pRange->ullStart) && (ullStartValue <= pRange->ullEnd))
        {
            ret = CR_FAILURE;
            break;
        }

        /* Check if the end value is within the current range */
        if ((ullEndValue >= pRange->ullStart) && (ullEndValue <= pRange->ullEnd))
        {
            ret = CR_FAILURE;
            break;
        }

        /* Check if the current range lies inside of the start-end interval */
        if ((ullStartValue <= pRange->ullStart) && (ullEndValue >= pRange->ullEnd))
        {
            ret = CR_FAILURE;
            break;
        }

        ListEntry = ListEntry->Flink;
    }

    /* Unlock the range list */
    ReleaseMutex(pRangeList->hMutex);

    return ret;
}


/***********************************************************************
 * CM_Uninstall_DevNode [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Uninstall_DevNode(
    _In_ DEVINST dnPhantom,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Uninstall_DevNode(%lx %lx)\n",
          dnPhantom, ulFlags);

    return CM_Uninstall_DevNode_Ex(dnPhantom, ulFlags, NULL);
}


/***********************************************************************
 * CM_Uninstall_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Uninstall_DevNode_Ex(
    _In_ DEVINST dnPhantom,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("CM_Uninstall_DevNode_Ex(%lx %lx %p)\n",
          dnPhantom, ulFlags, hMachine);

    if (dnPhantom == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;

        StringTable = ((PMACHINE_INFO)hMachine)->StringTable;
        if (StringTable == 0)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, &StringTable))
            return CR_FAILURE;
    }

    lpDevInst = pSetupStringTableStringFromId(StringTable, dnPhantom);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    RpcTryExcept
    {
        ret = PNP_UninstallDevInst(BindingHandle,
                                   lpDevInst,
                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}


/***********************************************************************
 * CM_Unregister_Device_InterfaceA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceA(
    _In_ LPCSTR pszDeviceInterface,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Unregister_Device_InterfaceA(%s %lx)\n",
          debugstr_a(pszDeviceInterface), ulFlags);

    return CM_Unregister_Device_Interface_ExA(pszDeviceInterface,
                                              ulFlags, NULL);
}


/***********************************************************************
 * CM_Unregister_Device_InterfaceW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Unregister_Device_InterfaceW(
    _In_ LPCWSTR pszDeviceInterface,
    _In_ ULONG ulFlags)
{
    TRACE("CM_Unregister_Device_InterfaceW(%s %lx)\n",
          debugstr_w(pszDeviceInterface), ulFlags);

    return CM_Unregister_Device_Interface_ExW(pszDeviceInterface,
                                              ulFlags, NULL);
}


/***********************************************************************
 * CM_Unregister_Device_Interface_ExA [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExA(
    _In_ LPCSTR pszDeviceInterface,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    LPWSTR pszDeviceInterfaceW = NULL;
    CONFIGRET ret;

    TRACE("CM_Unregister_Device_Interface_ExA(%s %lx %p)\n",
          debugstr_a(pszDeviceInterface), ulFlags, hMachine);

    if (pszDeviceInterface == NULL)
        return CR_INVALID_POINTER;

    if (pSetupCaptureAndConvertAnsiArg(pszDeviceInterface, &pszDeviceInterfaceW))
        return CR_INVALID_DATA;

    ret = CM_Unregister_Device_Interface_ExW(pszDeviceInterfaceW,
                                             ulFlags, hMachine);

    if (pszDeviceInterfaceW != NULL)
        MyFree(pszDeviceInterfaceW);

    return ret;
}


/***********************************************************************
 * CM_Unregister_Device_Interface_ExW [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CM_Unregister_Device_Interface_ExW(
    _In_ LPCWSTR pszDeviceInterface,
    _In_ ULONG ulFlags,
    _In_opt_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("CM_Unregister_Device_Interface_ExW(%s %lx %p)\n",
          debugstr_w(pszDeviceInterface), ulFlags, hMachine);

    if (pszDeviceInterface == NULL)
        return CR_INVALID_POINTER;

    if (ulFlags != 0)
        return CR_INVALID_FLAG;

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return CR_FAILURE;
    }
    else
    {
        if (!PnpGetLocalHandles(&BindingHandle, NULL))
            return CR_FAILURE;
    }

    RpcTryExcept
    {
        ret = PNP_UnregisterDeviceClassAssociation(BindingHandle,
                                                   (LPWSTR)pszDeviceInterface,
                                                   ulFlags);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    return ret;
}
