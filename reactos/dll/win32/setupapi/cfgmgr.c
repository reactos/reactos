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

#include "rpc_private.h"

/* Registry key and value names */
static const WCHAR Backslash[] = {'\\', 0};
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
    ULONG ulFlags;
    ULONG ulTag;
} LOG_CONF_INFO, *PLOG_CONF_INFO;

#define LOG_CONF_MAGIC 0x464E434C  /* "LCNF" */


typedef struct _NOTIFY_DATA
{
    ULONG ulMagic;
    ULONG ulNotifyData;
} NOTIFY_DATA, *PNOTIFY_DATA;

#define NOTIFY_MAGIC 0x44556677


static BOOL GuidToString(LPGUID Guid, LPWSTR String)
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


static CONFIGRET
RpcStatusToCmStatus(RPC_STATUS Status)
{
    return CR_FAILURE;
}


static
ULONG
GetRegistryPropertyType(
    ULONG ulProperty)
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


/***********************************************************************
 * CMP_GetServerSideDeviceInstallFlags [SETUPAPI.@]
 */
CONFIGRET
WINAPI
CMP_GetServerSideDeviceInstallFlags(
    _Out_ PULONG pulSSDIFlags,
    _In_ ULONG ulFlags,
    _In_ HMACHINE hMachine)
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
    PNOTIFY_DATA pNotifyData = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("CMP_RegisterNotification(%p %p %lu %p)\n",
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

/*
    if (dwFlags & DEVICE_NOTIFY_SERVICE_HANDLE == DEVICE_NOTYFY_WINDOW_HANDLE)
    {

    }
    else if (dwFlags & DEVICE_NOTIFY_SERVICE_HANDLE == DEVICE_NOTYFY_SERVICE_HANDLE)
    {

    }
*/

    RpcTryExcept
    {
        ret = PNP_RegisterNotification(BindingHandle,
                                       ulFlags,
                                       &pNotifyData->ulNotifyData);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
    {
        *phDevNotify = (HDEVNOTIFY)pNotifyData;
    }
    else
    {
        if (pNotifyData != NULL)
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
                                         pNotifyData->ulNotifyData);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = RpcStatusToCmStatus(RpcExceptionCode());
    }
    RpcEndExcept;

    if (ret == CR_SUCCESS)
        HeapFree(GetProcessHeap(), 0, pNotifyData);

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
    _In_ HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;
    WORD Version;

    TRACE("CMP_WaitServicesAvailable(%p)\n", hMachine);

    if (!PnpGetLocalHandles(&BindingHandle, NULL))
        return CR_FAILURE;

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
CONFIGRET WINAPI CM_Add_Empty_Log_Conf(
    PLOG_CONF plcLogConf, DEVINST dnDevInst, PRIORITY Priority,
    ULONG ulFlags)
{
    TRACE("%p %p %lu %lx\n", plcLogConf, dnDevInst, Priority, ulFlags);
    return CM_Add_Empty_Log_Conf_Ex(plcLogConf, dnDevInst, Priority,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_Empty_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Add_Empty_Log_Conf_Ex(
    PLOG_CONF plcLogConf, DEVINST dnDevInst, PRIORITY Priority,
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    ULONG ulLogConfTag = 0;
    LPWSTR lpDevInst;
    PLOG_CONF_INFO pLogConfInfo;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("%p %p %lu %lx %p\n",
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
            pLogConfInfo->ulFlags = ulFlags;
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
CONFIGRET WINAPI CM_Add_IDA(
    DEVINST dnDevInst, PSTR pszID, ULONG ulFlags)
{
    TRACE("%p %s %lx\n", dnDevInst, pszID, ulFlags);
    return CM_Add_ID_ExA(dnDevInst, pszID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_IDW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Add_IDW(
    DEVINST dnDevInst, PWSTR pszID, ULONG ulFlags)
{
    TRACE("%p %s %lx\n", dnDevInst, debugstr_w(pszID), ulFlags);
    return CM_Add_ID_ExW(dnDevInst, pszID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_ID_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Add_ID_ExA(
    DEVINST dnDevInst, PSTR pszID, ULONG ulFlags, HMACHINE hMachine)
{
    PWSTR pszIDW;
    CONFIGRET ret;

    TRACE("%p %s %lx %p\n", dnDevInst, pszID, ulFlags, hMachine);

    if (pSetupCaptureAndConvertAnsiArg(pszID, &pszIDW))
        return CR_INVALID_DATA;

    ret = CM_Add_ID_ExW(dnDevInst, pszIDW, ulFlags, hMachine);

    MyFree(pszIDW);

    return ret;
}


/***********************************************************************
 * CM_Add_ID_ExW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Add_ID_ExW(
    DEVINST dnDevInst, PWSTR pszID, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%p %s %lx %p\n", dnDevInst, debugstr_w(pszID), ulFlags, hMachine);

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
 * CM_Add_Res_Des [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Add_Res_Des(
    PRES_DES prdResDes, LOG_CONF lcLogConf, RESOURCEID ResourceID,
    PCVOID ResourceData, ULONG ResourceLen, ULONG ulFlags)
{
    TRACE("%p %p %lu %p %lu %lx\n", prdResDes, lcLogConf, ResourceID,
          ResourceData, ResourceLen, ulFlags);
    return CM_Add_Res_Des_Ex(prdResDes, lcLogConf, ResourceID, ResourceData,
                             ResourceLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Add_Res_Des_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Add_Res_Des_Ex(
    PRES_DES prdResDes, LOG_CONF lcLogConf, RESOURCEID ResourceID,
    PCVOID ResourceData, ULONG ResourceLen, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %p %lu %p %lu %lx %p\n", prdResDes, lcLogConf, ResourceID,
          ResourceData, ResourceLen, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Connect_MachineA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Connect_MachineA(
    PCSTR UNCServerName, PHMACHINE phMachine)
{
    PWSTR pServerNameW;
    CONFIGRET ret;

    TRACE("%s %p\n", UNCServerName, phMachine);

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
CONFIGRET WINAPI CM_Connect_MachineW(
    PCWSTR UNCServerName, PHMACHINE phMachine)
{
    PMACHINE_INFO pMachine;

    TRACE("%s %p\n", debugstr_w(UNCServerName), phMachine);

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
CONFIGRET WINAPI CM_Create_DevNodeA(
    PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, DEVINST dnParent,
    ULONG ulFlags)
{
    TRACE("%p %s %p %lx\n",
          pdnDevInst, debugstr_a(pDeviceID), dnParent, ulFlags);
    return CM_Create_DevNode_ExA(pdnDevInst, pDeviceID, dnParent,
                                 ulFlags, NULL);
}


/***********************************************************************
 * CM_Create_DevNodeW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Create_DevNodeW(
    PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, DEVINST dnParent,
    ULONG ulFlags)
{
    TRACE("%p %s %p %lx\n",
          pdnDevInst, debugstr_w(pDeviceID), dnParent, ulFlags);
    return CM_Create_DevNode_ExW(pdnDevInst, pDeviceID, dnParent,
                                 ulFlags, NULL);
}


/***********************************************************************
 * CM_Create_DevNode_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Create_DevNode_ExA(
    PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, DEVINST dnParent,
    ULONG ulFlags, HANDLE hMachine)
{
    DEVINSTID_W pDeviceIDW;
    CONFIGRET ret;

    TRACE("%p %s %p %lx %p\n",
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
CONFIGRET WINAPI CM_Create_DevNode_ExW(
    PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, DEVINST dnParent,
    ULONG ulFlags, HANDLE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpParentDevInst;
    CONFIGRET ret = CR_SUCCESS;
    WCHAR szLocalDeviceID[MAX_DEVICE_ID_LEN];

    TRACE("%p %s %p %lx %p\n",
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
 * CM_Delete_Class_Key [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Delete_Class_Key(
    LPGUID ClassGuid, ULONG ulFlags)
{
    TRACE("%p %lx\n", ClassGuid, ulFlags);
    return CM_Delete_Class_Key_Ex(ClassGuid, ulFlags, NULL);
}


/***********************************************************************
 * CM_Delete_Class_Key_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Delete_Class_Key_Ex(
    LPGUID ClassGuid, ULONG ulFlags, HANDLE hMachine)
{
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%p %lx %lx\n", ClassGuid, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Delete_DevNode_Key(
    DEVNODE dnDevNode, ULONG ulHardwareProfile, ULONG ulFlags)
{
    TRACE("%p %lu %lx\n", dnDevNode, ulHardwareProfile, ulFlags);
    return CM_Delete_DevNode_Key_Ex(dnDevNode, ulHardwareProfile, ulFlags,
                                    NULL);
}


/***********************************************************************
 * CM_Delete_DevNode_Key_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Delete_DevNode_Key_Ex(
    DEVNODE dnDevNode, ULONG ulHardwareProfile, ULONG ulFlags,
    HANDLE hMachine)
{
    FIXME("%p %lu %lx %p\n",
          dnDevNode, ulHardwareProfile, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Disable_DevNode [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Disable_DevNode(
    DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %lx\n", dnDevInst, ulFlags);
    return CM_Disable_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Disable_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Disable_DevNode_Ex(
    DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("%p %lx %p\n", dnDevInst, ulFlags, hMachine);

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
                                       PNP_DEVINST_DISABLE,
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
 * CM_Disconnect_Machine [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Disconnect_Machine(HMACHINE hMachine)
{
    PMACHINE_INFO pMachine;

    TRACE("%lx\n", hMachine);

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
 * CM_Enable_DevNode [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Enable_DevNode(
    DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %lx\n", dnDevInst, ulFlags);
    return CM_Enable_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Enable_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Enable_DevNode_Ex(
    DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%p %lx %p\n", dnDevInst, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Enumerate_Classes(
    ULONG ulClassIndex, LPGUID ClassGuid, ULONG ulFlags)
{
    TRACE("%lx %p %lx\n", ulClassIndex, ClassGuid, ulFlags);
    return CM_Enumerate_Classes_Ex(ulClassIndex, ClassGuid, ulFlags, NULL);
}


/***********************************************************************
 * CM_Enumerate_Classes_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Enumerate_Classes_Ex(
    ULONG ulClassIndex, LPGUID ClassGuid, ULONG ulFlags, HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_GUID_STRING_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulLength = MAX_GUID_STRING_LEN;

    TRACE("%lx %p %lx %p\n", ulClassIndex, ClassGuid, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Enumerate_EnumeratorsA(
    ULONG ulEnumIndex, PCHAR Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lu %p %p %lx\n", ulEnumIndex, Buffer, pulLength, ulFlags);
    return CM_Enumerate_Enumerators_ExA(ulEnumIndex, Buffer, pulLength,
                                        ulFlags, NULL);
}


/***********************************************************************
 * CM_Enumerate_EnumeratorsW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Enumerate_EnumeratorsW(
    ULONG ulEnumIndex, PWCHAR Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lu %p %p %lx\n", ulEnumIndex, Buffer, pulLength, ulFlags);
    return CM_Enumerate_Enumerators_ExW(ulEnumIndex, Buffer, pulLength,
                                        ulFlags, NULL);
}


/***********************************************************************
 * CM_Enumerate_Enumerators_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Enumerate_Enumerators_ExA(
    ULONG ulEnumIndex, PCHAR Buffer, PULONG pulLength, ULONG ulFlags,
    HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_DEVICE_ID_LEN];
    ULONG ulOrigLength;
    ULONG ulLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%lu %p %p %lx %lx\n", ulEnumIndex, Buffer, pulLength, ulFlags,
          hMachine);

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
CONFIGRET WINAPI CM_Enumerate_Enumerators_ExW(
    ULONG ulEnumIndex, PWCHAR Buffer, PULONG pulLength, ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%lu %p %p %lx %lx\n", ulEnumIndex, Buffer, pulLength, ulFlags,
          hMachine);

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
 * CM_Free_Log_Conf [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Free_Log_Conf(
    LOG_CONF lcLogConfToBeFreed, ULONG ulFlags)
{
    TRACE("%lx %lx\n", lcLogConfToBeFreed, ulFlags);
    return CM_Free_Log_Conf_Ex(lcLogConfToBeFreed, ulFlags, NULL);
}


/***********************************************************************
 * CM_Free_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Free_Log_Conf_Ex(
    LOG_CONF lcLogConfToBeFreed, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    PLOG_CONF_INFO pLogConfInfo;
    CONFIGRET ret;

    TRACE("%lx %lx %lx\n", lcLogConfToBeFreed, ulFlags, hMachine);

    if (!pSetupIsUserAdmin())
        return CR_ACCESS_DENIED;

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConfToBeFreed;
    if (pLogConfInfo == NULL || pLogConfInfo->ulMagic != LOG_CONF_MAGIC)
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
        ret = PNP_FreeLogConf(BindingHandle, lpDevInst, pLogConfInfo->ulFlags,
                              pLogConfInfo->ulTag, 0);
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
CONFIGRET WINAPI CM_Free_Log_Conf_Handle(
    LOG_CONF lcLogConf)
{
    PLOG_CONF_INFO pLogConfInfo;

    TRACE("%lx\n", lcLogConf);

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConf;
    if (pLogConfInfo == NULL || pLogConfInfo->ulMagic != LOG_CONF_MAGIC)
        return CR_INVALID_LOG_CONF;

    HeapFree(GetProcessHeap(), 0, pLogConfInfo);

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Free_Res_Des [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Free_Res_Des(
   PRES_DES prdResDes, RES_DES rdResDes, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", prdResDes, rdResDes, ulFlags);
    return CM_Free_Res_Des_Ex(prdResDes, rdResDes, ulFlags, NULL);
}


/***********************************************************************
 * CM_Free_Res_Des_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Free_Res_Des_Ex(
    PRES_DES prdResDes, RES_DES rdResDes, ULONG ulFlags,
    HMACHINE hMachine)
{
    FIXME("%p %p %lx %lx\n", prdResDes, rdResDes, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Free_Res_Des_Handle [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Free_Res_Des_Handle(
    RES_DES rdResDes)
{
    FIXME("%p\n", rdResDes);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Child [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Child(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", pdnDevInst, dnDevInst, ulFlags);
    return CM_Get_Child_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Child_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Child_Ex(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    WCHAR szRelatedDevInst[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    DWORD dwIndex, dwLength = MAX_DEVICE_ID_LEN;
    CONFIGRET ret;

    TRACE("%p %lx %lx %lx\n", pdnDevInst, dnDevInst, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Class_Key_NameA(
    LPGUID ClassGuid, LPSTR pszKeyName, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%p %p %p %lx\n",
          ClassGuid, pszKeyName, pulLength, ulFlags);
    return CM_Get_Class_Key_Name_ExA(ClassGuid, pszKeyName, pulLength,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Class_Key_NameW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Class_Key_NameW(
    LPGUID ClassGuid, LPWSTR pszKeyName, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%p %p %p %lx\n",
          ClassGuid, pszKeyName, pulLength, ulFlags);
    return CM_Get_Class_Key_Name_ExW(ClassGuid, pszKeyName, pulLength,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Class_Key_Name_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Class_Key_Name_ExA(
    LPGUID ClassGuid, LPSTR pszKeyName, PULONG pulLength, ULONG ulFlags,
    HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_GUID_STRING_LEN];
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulLength;
    ULONG ulOrigLength;

    TRACE("%p %p %p %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_Class_Key_Name_ExW(
    LPGUID ClassGuid, LPWSTR pszKeyName, PULONG pulLength, ULONG ulFlags,
    HMACHINE hMachine)
{
    TRACE("%p %p %p %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_Class_NameA(
    LPGUID ClassGuid, PCHAR Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%p %p %p %lx\n", ClassGuid, Buffer, pulLength, ulFlags);
    return CM_Get_Class_Name_ExA(ClassGuid, Buffer, pulLength, ulFlags,
                                 NULL);
}


/***********************************************************************
 * CM_Get_Class_NameW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Class_NameW(
    LPGUID ClassGuid, PWCHAR Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%p %p %p %lx\n", ClassGuid, Buffer, pulLength, ulFlags);
    return CM_Get_Class_Name_ExW(ClassGuid, Buffer, pulLength, ulFlags,
                                 NULL);
}


/***********************************************************************
 * CM_Get_Class_Name_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Class_Name_ExA(
    LPGUID ClassGuid, PCHAR Buffer, PULONG pulLength, ULONG ulFlags,
    HMACHINE hMachine)
{
    WCHAR szBuffer[MAX_CLASS_NAME_LEN];
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulLength;
    ULONG ulOrigLength;

    TRACE("%p %p %p %lx %lx\n",
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
CONFIGRET WINAPI
CM_Get_Class_Name_ExW(
    LPGUID ClassGuid, PWCHAR Buffer, PULONG pulLength, ULONG ulFlags,
    HMACHINE hMachine)
{
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%p %p %p %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyA(
    LPGUID ClassGuid, ULONG ulProperty, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    PWSTR BufferW = NULL;
    ULONG ulLength = 0;
    ULONG ulType;
    CONFIGRET ret;

    TRACE("%p %lu %p %p %p %lx %lx\n",
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
                                         lstrlenW(BufferW) + 1,
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
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyW(
    LPGUID ClassGuid, ULONG ulProperty, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WCHAR szGuidString[PNP_MAX_GUID_STRING_LEN + 1];
    ULONG ulType = 0;
    ULONG ulTransferLength = 0;
    CONFIGRET ret;

    TRACE("%p %lu %p %p %p %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_Depth(
    PULONG pulDepth, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %lx %lx\n", pulDepth, dnDevInst, ulFlags);
    return CM_Get_Depth_Ex(pulDepth, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Depth_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Depth_Ex(
    PULONG pulDepth, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%p %lx %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_DevNode_Custom_PropertyA(
    DEVINST dnDevInst, PCSTR pszCustomPropertyName, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lx %s %p %p %p %lx\n", dnDevInst, pszCustomPropertyName,
          pulRegDataType, Buffer, pulLength, ulFlags);
    return CM_Get_DevNode_Custom_Property_ExA(dnDevInst, pszCustomPropertyName,
                                              pulRegDataType, Buffer,
                                              pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Custom_PropertyW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_DevNode_Custom_PropertyW(
    DEVINST dnDevInst, PCWSTR pszCustomPropertyName, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lx %s %p %p %p %lx\n", dnDevInst, debugstr_w(pszCustomPropertyName),
          pulRegDataType, Buffer, pulLength, ulFlags);
    return CM_Get_DevNode_Custom_Property_ExW(dnDevInst, pszCustomPropertyName,
                                              pulRegDataType, Buffer,
                                              pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Custom_Property_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_DevNode_Custom_Property_ExA(
    DEVINST dnDevInst, PCSTR pszCustomPropertyName, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    LPWSTR pszPropertyNameW = NULL;
    PVOID BufferW;
    ULONG ulLengthW;
    ULONG ulDataType = REG_NONE;
    CONFIGRET ret;

    TRACE("%lx %s %p %p %p %lx %p\n", dnDevInst, pszCustomPropertyName,
          pulRegDataType, Buffer, pulLength, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_DevNode_Custom_Property_ExW(
    DEVINST dnDevInst, PCWSTR pszCustomPropertyName, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    ULONG ulDataType = REG_NONE;
    ULONG ulTransferLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%lx %s %p %p %p %lx %p\n", dnDevInst,
          debugstr_w(pszCustomPropertyName), pulRegDataType, Buffer,
          pulLength, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyA(
    DEVINST dnDevInst, ULONG ulProperty, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lx %lu %p %p %p %lx\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags);

    return CM_Get_DevNode_Registry_Property_ExA(dnDevInst, ulProperty,
                                                pulRegDataType, Buffer,
                                                pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyW(
    DEVINST dnDevInst, ULONG ulProperty, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lx %lu %p %p %p %lx\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength, ulFlags);

    return CM_Get_DevNode_Registry_Property_ExW(dnDevInst, ulProperty,
                                                pulRegDataType, Buffer,
                                                pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Registry_Property_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExA(
    DEVINST dnDevInst, ULONG ulProperty, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    PVOID BufferW;
    ULONG LengthW;
    ULONG ulDataType = REG_NONE;
    CONFIGRET ret;

    TRACE("%lx %lu %p %p %p %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExW(
    DEVINST dnDevInst, ULONG ulProperty, PULONG pulRegDataType,
    PVOID Buffer, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpDevInst;
    ULONG ulDataType = REG_NONE;
    ULONG ulTransferLength = 0;

    TRACE("%lx %lu %p %p %p %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_DevNode_Status(
    PULONG pulStatus, PULONG pulProblemNumber, DEVINST dnDevInst,
    ULONG ulFlags)
{
    TRACE("%p %p %lx %lx\n",
          pulStatus, pulProblemNumber, dnDevInst, ulFlags);
    return CM_Get_DevNode_Status_Ex(pulStatus, pulProblemNumber, dnDevInst,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_DevNode_Status_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI
CM_Get_DevNode_Status_Ex(
    PULONG pulStatus, PULONG pulProblemNumber, DEVINST dnDevInst,
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%p %p %lx %lx %lx\n",
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
CONFIGRET WINAPI CM_Get_Device_IDA(
    DEVINST dnDevInst, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%lx %p %ld %ld\n",
          dnDevInst, Buffer, BufferLen, ulFlags);
    return CM_Get_Device_ID_ExA(dnDevInst, Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_IDW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_IDW(
    DEVINST dnDevInst, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%lx %p %ld %ld\n",
          dnDevInst, Buffer, BufferLen, ulFlags);
    return CM_Get_Device_ID_ExW(dnDevInst, Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_ExA(
    DEVINST dnDevInst, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags,
    HMACHINE hMachine)
{
    WCHAR szBufferW[MAX_DEVICE_ID_LEN];
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%lx %p %ld %ld %lx\n",
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
CONFIGRET WINAPI CM_Get_Device_ID_ExW(
    DEVINST dnDevInst, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags,
    HMACHINE hMachine)
{
    HSTRING_TABLE StringTable = NULL;

    TRACE("%lx %p %ld %ld %lx\n",
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
CONFIGRET WINAPI CM_Get_Device_ID_ListA(
    PCSTR pszFilter, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%p %p %ld %ld\n", pszFilter, Buffer, BufferLen, ulFlags);
    return CM_Get_Device_ID_List_ExA(pszFilter, Buffer, BufferLen,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_ListW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListW(
    PCWSTR pszFilter, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%p %p %ld %ld\n", pszFilter, Buffer, BufferLen, ulFlags);
    return CM_Get_Device_ID_List_ExW(pszFilter, Buffer, BufferLen,
                                     ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_List_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_ExA(
    PCSTR pszFilter, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags,
    HMACHINE hMachine)
{
    LPWSTR BufferW = NULL;
    LPWSTR pszFilterW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%p %p %ld %ld %lx\n",
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
                            lstrlenW(BufferW) + 1,
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
CONFIGRET WINAPI CM_Get_Device_ID_List_ExW(
    PCWSTR pszFilter, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%p %p %ld %ld %lx\n",
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
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeA(
    PULONG pulLen, PCSTR pszFilter, ULONG ulFlags)
{
    TRACE("%p %s %ld\n", pulLen, pszFilter, ulFlags);
    return CM_Get_Device_ID_List_Size_ExA(pulLen, pszFilter, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_List_SizeW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeW(
    PULONG pulLen, PCWSTR pszFilter, ULONG ulFlags)
{
    TRACE("%p %s %ld\n", pulLen, debugstr_w(pszFilter), ulFlags);
    return CM_Get_Device_ID_List_Size_ExW(pulLen, pszFilter, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_List_Size_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExA(
    PULONG pulLen, PCSTR pszFilter, ULONG ulFlags, HMACHINE hMachine)
{
    LPWSTR pszFilterW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    FIXME("%p %s %lx %lx\n", pulLen, pszFilter, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExW(
    PULONG pulLen, PCWSTR pszFilter, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    FIXME("%p %s %ld %lx\n", pulLen, debugstr_w(pszFilter), ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Device_ID_Size(
    PULONG pulLen, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %lx %lx\n", pulLen, dnDevInst, ulFlags);
    return CM_Get_Device_ID_Size_Ex(pulLen, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_ID_Size_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_Size_Ex(
    PULONG pulLen, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    HSTRING_TABLE StringTable = NULL;
    LPWSTR DeviceId;

    TRACE("%p %lx %lx %lx\n", pulLen, dnDevInst, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Device_Interface_AliasA(
    LPCSTR pszDeviceInterface, LPGUID AliasInterfaceGuid,
    LPSTR pszAliasDeviceInterface, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%p %p %p %p %lu\n", pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags);

    return CM_Get_Device_Interface_Alias_ExA(pszDeviceInterface,
        AliasInterfaceGuid, pszAliasDeviceInterface, pulLength,
        ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_Interface_AliasW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_Interface_AliasW(
    LPCWSTR pszDeviceInterface, LPGUID AliasInterfaceGuid,
    LPWSTR pszAliasDeviceInterface, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%p %p %p %p %lu\n", pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags);

    return CM_Get_Device_Interface_Alias_ExW(pszDeviceInterface,
        AliasInterfaceGuid, pszAliasDeviceInterface, pulLength,
        ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Device_Interface_Alias_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_Interface_Alias_ExA(
    LPCSTR pszDeviceInterface, LPGUID AliasInterfaceGuid, LPSTR pszAliasDeviceInterface,
    PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %p %p %p %lu %lx\n", pszDeviceInterface, AliasInterfaceGuid,
          pszAliasDeviceInterface, pulLength, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Device_Interface_Alias_ExW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_Interface_Alias_ExW(
    LPCWSTR pszDeviceInterface, LPGUID AliasInterfaceGuid, LPWSTR pszAliasDeviceInterface,
    PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    ULONG ulTransferLength;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%p %p %p %p %lu %lx\n", pszDeviceInterface, AliasInterfaceGuid,
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
CONFIGRET WINAPI CM_Get_Device_Interface_ListA(
    LPGUID InterfaceClassGuid, DEVINSTID_A pDeviceID, PCHAR Buffer,
    ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%s %s %p %lu 0x%08lx\n", debugstr_guid(InterfaceClassGuid),
          pDeviceID, Buffer, BufferLen, ulFlags);

    return CM_Get_Device_Interface_List_ExA(InterfaceClassGuid, pDeviceID,
                                            Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_ListW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_ListW(
    LPGUID InterfaceClassGuid, DEVINSTID_W pDeviceID, PWCHAR Buffer,
    ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%s %s %p %lu 0x%08lx\n", debugstr_guid(InterfaceClassGuid),
          debugstr_w(pDeviceID), Buffer, BufferLen, ulFlags);

    return CM_Get_Device_Interface_List_ExW(InterfaceClassGuid, pDeviceID,
                                            Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_ExA(
    LPGUID InterfaceClassGuid, DEVINSTID_A pDeviceID, PCHAR Buffer,
    ULONG BufferLen, ULONG ulFlags, HMACHINE hMachine)
{
    DEVINSTID_W pDeviceIdW = NULL;
    PWCHAR BufferW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%s %s %p %lu 0x%08lx %p\n", debugstr_guid(InterfaceClassGuid),
          pDeviceID, Buffer, BufferLen, ulFlags, hMachine);

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
                            lstrlenW(BufferW) + 1,
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
CONFIGRET WINAPI CM_Get_Device_Interface_List_ExW(
    LPGUID InterfaceClassGuid, DEVINSTID_W pDeviceID, PWCHAR Buffer,
    ULONG BufferLen, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    PNP_RPC_BUFFER_SIZE BufferSize = 0;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%s %s %p %lu 0x%08lx %p\n", debugstr_guid(InterfaceClassGuid),
          debugstr_w(pDeviceID), Buffer, BufferLen, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeA(
    PULONG pulLen, LPGUID InterfaceClassGuid, DEVINSTID_A pDeviceId,
    ULONG ulFlags)
{
    TRACE("%p %p %s 0x%08lx\n", pulLen, InterfaceClassGuid,
          pDeviceId, ulFlags);

    return CM_Get_Device_Interface_List_Size_ExA(pulLen, InterfaceClassGuid,
                                                 pDeviceId, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeW(
    PULONG pulLen, LPGUID InterfaceClassGuid, DEVINSTID_W pDeviceId,
    ULONG ulFlags)
{
    TRACE("%p %p %s 0x%08lx\n", pulLen, InterfaceClassGuid,
          debugstr_w(pDeviceId), ulFlags);

    return CM_Get_Device_Interface_List_Size_ExW(pulLen, InterfaceClassGuid,
                                                 pDeviceId, ulFlags, NULL);
}


/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExA(
    PULONG pulLen, LPGUID InterfaceClassGuid, DEVINSTID_A pDeviceId,
    ULONG ulFlags, HMACHINE hMachine)
{
    DEVINSTID_W pDeviceIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%p %p %s 0x%08lx %p\n", pulLen, InterfaceClassGuid,
          pDeviceId, ulFlags, hMachine);

    if (pulLen == NULL)
        return CR_INVALID_POINTER;

    if (pDeviceId != NULL)
    {
        if (!pSetupCaptureAndConvertAnsiArg(pDeviceId, &pDeviceIdW))
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
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExW(
    PULONG pulLen, LPGUID InterfaceClassGuid, DEVINSTID_W pDeviceId,
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%p %p %s 0x%08lx %p\n", pulLen, InterfaceClassGuid,
          debugstr_w(pDeviceId), ulFlags, hMachine);

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
                                             pDeviceId,
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
CONFIGRET WINAPI CM_Get_First_Log_Conf(
    PLOG_CONF plcLogConf, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %lx %lx\n", plcLogConf, dnDevInst, ulFlags);
    return CM_Get_First_Log_Conf_Ex(plcLogConf, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_First_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_First_Log_Conf_Ex(
    PLOG_CONF plcLogConf, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst = NULL;
    CONFIGRET ret = CR_SUCCESS;
    ULONG ulTag;
    PLOG_CONF_INFO pLogConfInfo;

    FIXME("%p %lx %lx %lx\n", plcLogConf, dnDevInst, ulFlags, hMachine);

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
        pLogConfInfo->ulFlags = ulFlags;
        pLogConfInfo->ulTag = ulTag;

        *plcLogConf = (LOG_CONF)pLogConfInfo;
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Global_State [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Global_State(
    PULONG pulState, ULONG ulFlags)
{
    TRACE("%p %lx\n", pulState, ulFlags);
    return CM_Get_Global_State_Ex(pulState, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Global_State_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Global_State_Ex(
    PULONG pulState, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%p %lx %lx\n", pulState, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_HW_Prof_FlagsA(
    DEVINSTID_A szDevInstName, ULONG ulHardwareProfile, PULONG pulValue,
    ULONG ulFlags)
{
    TRACE("%s %lu %p %lx\n", szDevInstName,
          ulHardwareProfile, pulValue, ulFlags);

    return CM_Get_HW_Prof_Flags_ExA(szDevInstName, ulHardwareProfile,
                                    pulValue, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_HW_Prof_FlagsW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_HW_Prof_FlagsW(
    DEVINSTID_W szDevInstName, ULONG ulHardwareProfile, PULONG pulValue,
    ULONG ulFlags)
{
    TRACE("%s %lu %p %lx\n", debugstr_w(szDevInstName),
          ulHardwareProfile, pulValue, ulFlags);

    return CM_Get_HW_Prof_Flags_ExW(szDevInstName, ulHardwareProfile,
                                    pulValue, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_HW_Prof_Flags_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_HW_Prof_Flags_ExA(
    DEVINSTID_A szDevInstName, ULONG ulHardwareProfile, PULONG pulValue,
    ULONG ulFlags, HMACHINE hMachine)
{
    DEVINSTID_W pszDevIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%s %lu %p %lx %lx\n", szDevInstName,
          ulHardwareProfile, pulValue, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_HW_Prof_Flags_ExW(
    DEVINSTID_W szDevInstName, ULONG ulHardwareProfile, PULONG pulValue,
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    FIXME("%s %lu %p %lx %lx\n", debugstr_w(szDevInstName),
          ulHardwareProfile, pulValue, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Hardware_Profile_InfoA(
    ULONG ulIndex, PHWPROFILEINFO_A pHWProfileInfo, ULONG ulFlags)
{
    TRACE("%lu %p %lx\n", ulIndex, pHWProfileInfo, ulFlags);

    return CM_Get_Hardware_Profile_Info_ExA(ulIndex, pHWProfileInfo,
                                            ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Hardware_Profile_InfoW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Hardware_Profile_InfoW(
    ULONG ulIndex, PHWPROFILEINFO_W pHWProfileInfo, ULONG ulFlags)
{
    TRACE("%lu %p %lx\n", ulIndex, pHWProfileInfo, ulFlags);

    return CM_Get_Hardware_Profile_Info_ExW(ulIndex, pHWProfileInfo,
                                            ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Hardware_Profile_Info_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Hardware_Profile_Info_ExA(
    ULONG ulIndex, PHWPROFILEINFO_A pHWProfileInfo, ULONG ulFlags,
    HMACHINE hMachine)
{
    HWPROFILEINFO_W LocalProfileInfo;
    CONFIGRET ret;

    TRACE("%lu %p %lx %lx\n", ulIndex, pHWProfileInfo, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Hardware_Profile_Info_ExW(
    ULONG ulIndex, PHWPROFILEINFO_W pHWProfileInfo, ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%lu %p %lx %lx\n", ulIndex, pHWProfileInfo, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Log_Conf_Priority(
    LOG_CONF lcLogConf, PPRIORITY pPriority, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", lcLogConf, pPriority, ulFlags);
    return CM_Get_Log_Conf_Priority_Ex(lcLogConf, pPriority, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Log_Conf_Priority_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Log_Conf_Priority_Ex(
    LOG_CONF lcLogConf, PPRIORITY pPriority, ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PLOG_CONF_INFO pLogConfInfo;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("%p %p %lx %lx\n", lcLogConf, pPriority, ulFlags, hMachine);

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConf;
    if (pLogConfInfo == NULL || pLogConfInfo->ulMagic != LOG_CONF_MAGIC)
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
                                     pLogConfInfo->ulFlags,
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
CONFIGRET WINAPI CM_Get_Next_Log_Conf(
    PLOG_CONF plcLogConf, LOG_CONF lcLogConf, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", plcLogConf, lcLogConf, ulFlags);
    return CM_Get_Next_Log_Conf_Ex(plcLogConf, lcLogConf, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Next_Log_Conf_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Next_Log_Conf_Ex(
    PLOG_CONF plcLogConf, LOG_CONF lcLogConf, ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    PLOG_CONF_INFO pLogConfInfo;
    PLOG_CONF_INFO pNewLogConfInfo;
    ULONG ulNewTag;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("%p %p %lx %lx\n", plcLogConf, lcLogConf, ulFlags, hMachine);

    if (plcLogConf)
        *plcLogConf = 0;

    pLogConfInfo = (PLOG_CONF_INFO)lcLogConf;
    if (pLogConfInfo == NULL || pLogConfInfo->ulMagic != LOG_CONF_MAGIC)
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
                                 pLogConfInfo->ulFlags,
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
        pNewLogConfInfo->ulFlags = pLogConfInfo->ulFlags;
        pNewLogConfInfo->ulTag = ulNewTag;

        *plcLogConf = (LOG_CONF)pNewLogConfInfo;
    }

    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Next_Re_Des [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Next_Res_Des(
    PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ForResource,
    PRESOURCEID pResourceID, ULONG ulFlags)
{
    TRACE("%p %p %lu %p %lx\n", prdResDes, rdResDes, ForResource,
          pResourceID, ulFlags);
    return CM_Get_Next_Res_Des_Ex(prdResDes, rdResDes, ForResource,
                                  pResourceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Next_Re_Des_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Next_Res_Des_Ex(
    PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ForResource,
    PRESOURCEID pResourceID, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %p %lu %p %lx %lx\n", prdResDes, rdResDes, ForResource,
          pResourceID, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Parent [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Parent(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", pdnDevInst, dnDevInst, ulFlags);
    return CM_Get_Parent_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Parent_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Parent_Ex(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    WCHAR szRelatedDevInst[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    DWORD dwIndex, dwLength = MAX_DEVICE_ID_LEN;
    CONFIGRET ret;

    TRACE("%p %lx %lx %lx\n", pdnDevInst, dnDevInst, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Get_Res_Des_Data(
    RES_DES rdResDes, PVOID Buffer, ULONG BufferLen, ULONG ulFlags)
{
    TRACE("%p %p %l %lx\n", rdResDes, Buffer, BufferLen, ulFlags);
    return CM_Get_Res_Des_Data_Ex(rdResDes, Buffer, BufferLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Res_Des_Data_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Res_Des_Data_Ex(
    RES_DES rdResDes, PVOID Buffer, ULONG BufferLen, ULONG ulFlags,
    HMACHINE hMachine)
{
    FIXME("%p %p %l %lx %lx\n", rdResDes, Buffer, BufferLen, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Res_Des_Size [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Res_Des_Data_Size(
    PULONG pulSize, RES_DES rdResDes, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", pulSize, rdResDes, ulFlags);
    return CM_Get_Res_Des_Data_Size_Ex(pulSize, rdResDes, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Res_Des_Size_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Res_Des_Data_Size_Ex(
    PULONG pulSize, RES_DES rdResDes, ULONG ulFlags, HMACHINE hMachine)
{
    TRACE("%p %p %lx %lx\n", pulSize, rdResDes, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Get_Sibling [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Sibling(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", pdnDevInst, dnDevInst, ulFlags);
    return CM_Get_Sibling_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Sibling_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Sibling_Ex(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    WCHAR szRelatedDevInst[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    DWORD dwIndex, dwLength = MAX_DEVICE_ID_LEN;
    CONFIGRET ret;

    TRACE("%p %lx %lx %lx\n", pdnDevInst, dnDevInst, ulFlags, hMachine);

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
WORD WINAPI CM_Get_Version(VOID)
{
    TRACE("\n");
    return CM_Get_Version_Ex(NULL);
}


/***********************************************************************
 * CM_Get_Version_Ex [SETUPAPI.@]
 */
WORD WINAPI CM_Get_Version_Ex(HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WORD Version = 0;
    CONFIGRET ret;

    TRACE("%lx\n", hMachine);

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
 * CM_Is_Dock_Station_Present [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Is_Dock_Station_Present(
    PBOOL pbPresent)
{
    TRACE("%p\n", pbPresent);
    return CM_Is_Dock_Station_Present_Ex(pbPresent, NULL);
}


/***********************************************************************
 * CM_Is_Dock_Station_Present_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Is_Dock_Station_Present_Ex(
    PBOOL pbPresent, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%p %lx\n", pbPresent, hMachine);

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
BOOL WINAPI CM_Is_Version_Available(
     WORD wVersion)
{
    TRACE("%hu\n", wVersion);
    return CM_Is_Version_Available_Ex(wVersion, NULL);
}


/***********************************************************************
 * CM_Is_Version_Available_Ex [SETUPAPI.@]
 */
BOOL WINAPI CM_Is_Version_Available_Ex(
    WORD wVersion, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    WORD wServerVersion;
    CONFIGRET ret;

    TRACE("%hu %lx\n", wVersion, hMachine);

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
CONFIGRET WINAPI CM_Locate_DevNodeA(
    PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags)
{
    TRACE("%p %s %lu\n", pdnDevInst, pDeviceID, ulFlags);
    return CM_Locate_DevNode_ExA(pdnDevInst, pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Locate_DevNodeW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Locate_DevNodeW(
    PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags)
{
    TRACE("%p %s %lu\n", pdnDevInst, debugstr_w(pDeviceID), ulFlags);
    return CM_Locate_DevNode_ExW(pdnDevInst, pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Locate_DevNode_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExA(
    PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
    DEVINSTID_W pDevIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%p %s %lu %lx\n", pdnDevInst, pDeviceID, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Locate_DevNode_ExW(
    PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
    WCHAR DeviceIdBuffer[MAX_DEVICE_ID_LEN];
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%p %s %lu %lx\n", pdnDevInst, debugstr_w(pDeviceID), ulFlags, hMachine);

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

        if (ret != CR_SUCCESS)
            return CR_FAILURE;
    }
    TRACE("DeviceIdBuffer: %s\n", debugstr_w(DeviceIdBuffer));

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

    if (ret == CR_SUCCESS)
    {
        *pdnDevInst = pSetupStringTableAddString(StringTable, DeviceIdBuffer, 1);
        if (*pdnDevInst == -1)
            ret = CR_FAILURE;
    }

    return ret;
}


/***********************************************************************
 * CM_Modify_Res_Des [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Modify_Res_Des(
    PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ResourceID,
    PCVOID ResourceData, ULONG ResourceLen, ULONG ulFlags)
{
    TRACE("%p %p %lx %p %lu %lx", prdResDes, rdResDes, ResourceID, ResourceData,
          ResourceLen, ulFlags);
    return CM_Modify_Res_Des_Ex(prdResDes, rdResDes, ResourceID, ResourceData,
                                ResourceLen, ulFlags, NULL);
}


/***********************************************************************
 * CM_Modify_Res_Des_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Modify_Res_Des_Ex(
    PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ResourceID, PCVOID ResourceData,
    ULONG ResourceLen, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %p %lx %p %lu %lx %lx", prdResDes, rdResDes, ResourceID, ResourceData,
          ResourceLen, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Move_DevNode [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Move_DevNode(
    DEVINST dnFromDevInst, DEVINST dnToDevInst, ULONG ulFlags)
{
    TRACE("%lx %lx %lx\n", dnFromDevInst, dnToDevInst, ulFlags);
    return CM_Move_DevNode_Ex(dnFromDevInst, dnToDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Move_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Move_DevNode_Ex(
    DEVINST dnFromDevInst, DEVINST dnToDevInst, ULONG ulFlags,
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpFromDevInst;
    LPWSTR lpToDevInst;
    CONFIGRET ret;

    FIXME("%lx %lx %lx %lx\n",
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
 * CM_Open_Class_KeyA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Open_Class_KeyA(
    LPGUID pClassGuid, LPCSTR pszClassName, REGSAM samDesired,
    REGDISPOSITION Disposition, PHKEY phkClass, ULONG ulFlags)
{
    TRACE("%p %s %lx %lx %p %lx\n",
          debugstr_guid(pClassGuid), pszClassName,
          samDesired, Disposition, phkClass, ulFlags);

    return CM_Open_Class_Key_ExA(pClassGuid, pszClassName, samDesired,
                                 Disposition, phkClass, ulFlags, NULL);
}


/***********************************************************************
 * CM_Open_Class_KeyW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Open_Class_KeyW(
    LPGUID pClassGuid, LPCWSTR pszClassName, REGSAM samDesired,
    REGDISPOSITION Disposition, PHKEY phkClass, ULONG ulFlags)
{
    TRACE("%p %s %lx %lx %p %lx\n",
          debugstr_guid(pClassGuid), debugstr_w(pszClassName),
          samDesired, Disposition, phkClass, ulFlags);

    return CM_Open_Class_Key_ExW(pClassGuid, pszClassName, samDesired,
                                 Disposition, phkClass, ulFlags, NULL);
}


/***********************************************************************
 * CM_Open_Class_Key_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Open_Class_Key_ExA(
    LPGUID pClassGuid, LPCSTR pszClassName, REGSAM samDesired,
    REGDISPOSITION Disposition, PHKEY phkClass, ULONG ulFlags,
    HMACHINE hMachine)
{
    LPWSTR pszClassNameW = NULL;
    CONFIGRET ret;

    TRACE("%p %s %lx %lx %p %lx %lx\n",
          debugstr_guid(pClassGuid), pszClassName,
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
CONFIGRET WINAPI CM_Open_Class_Key_ExW(
    LPGUID pClassGuid, LPCWSTR pszClassName, REGSAM samDesired,
    REGDISPOSITION Disposition, PHKEY phkClass, ULONG ulFlags,
    HMACHINE hMachine)
{
    WCHAR szKeyName[MAX_PATH];
    LPWSTR lpGuidString;
    DWORD dwDisposition;
    DWORD dwError;
    HKEY hKey;

    TRACE("%p %s %lx %lx %p %lx %lx\n",
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
                               HKEY_LOCAL_MACHINE, &hKey))
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

        lstrcatW(szKeyName, Backslash);
        lstrcatW(szKeyName, lpGuidString);
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
CONFIGRET WINAPI CM_Open_DevNode_Key(
    DEVINST dnDevNode, REGSAM samDesired, ULONG ulHardwareProfile,
    REGDISPOSITION Disposition, PHKEY phkDevice, ULONG ulFlags)
{
    TRACE("%lx %lx %lu %lx %p %lx\n", dnDevNode, samDesired,
          ulHardwareProfile, Disposition, phkDevice, ulFlags);
    return CM_Open_DevNode_Key_Ex(dnDevNode, samDesired, ulHardwareProfile,
                                  Disposition, phkDevice, ulFlags, NULL);
}


/***********************************************************************
 * CM_Open_DevNode_Key_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Open_DevNode_Key_Ex(
    DEVINST dnDevNode, REGSAM samDesired, ULONG ulHardwareProfile,
    REGDISPOSITION Disposition, PHKEY phkDevice, ULONG ulFlags,
    HMACHINE hMachine)
{
    FIXME("%lx %lx %lu %lx %p %lx %lx\n", dnDevNode, samDesired,
          ulHardwareProfile, Disposition, phkDevice, ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Query_And_Remove_SubTreeA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Query_And_Remove_SubTreeA(
    DEVINST dnAncestor, PPNP_VETO_TYPE pVetoType, LPSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags)
{
    TRACE("%lx %p %s %lu %lx\n", dnAncestor, pVetoType, pszVetoName,
          ulNameLength, ulFlags);

    return CM_Query_And_Remove_SubTree_ExA(dnAncestor, pVetoType, pszVetoName,
                                           ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_And_Remove_SubTreeW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Query_And_Remove_SubTreeW(
    DEVINST dnAncestor, PPNP_VETO_TYPE pVetoType, LPWSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags)
{
    TRACE("%lx %p %s %lu %lx\n", dnAncestor, pVetoType,
          debugstr_w(pszVetoName), ulNameLength, ulFlags);

    return CM_Query_And_Remove_SubTree_ExW(dnAncestor, pVetoType, pszVetoName,
                                           ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_And_Remove_SubTree_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Query_And_Remove_SubTree_ExA(
    DEVINST dnAncestor, PPNP_VETO_TYPE pVetoType, LPSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags, HMACHINE hMachine)
{
    LPWSTR lpLocalVetoName;
    CONFIGRET ret;

    TRACE("%lx %p %s %lu %lx %lx\n", dnAncestor, pVetoType, pszVetoName,
          ulNameLength, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Query_And_Remove_SubTree_ExW(
    DEVINST dnAncestor, PPNP_VETO_TYPE pVetoType, LPWSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%lx %p %s %lu %lx %lx\n", dnAncestor, pVetoType,
          debugstr_w(pszVetoName), ulNameLength, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Query_Arbitrator_Free_Data(
    PVOID pData, ULONG DataLen, DEVINST dnDevInst, RESOURCEID ResourceID,
    ULONG ulFlags)
{
    TRACE("%p %lu %lx %lu 0x%08lx\n", pData, DataLen, dnDevInst,
          ResourceID, ulFlags);

    return CM_Query_Arbitrator_Free_Data_Ex(pData, DataLen, dnDevInst,
                                            ResourceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_Arbitrator_Free_Data_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Query_Arbitrator_Free_Data_Ex(
  OUT PVOID pData,
  IN ULONG DataLen,
  IN DEVINST dnDevInst,
  IN RESOURCEID ResourceID,
  IN ULONG ulFlags,
  IN HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%p %lu %lx %lu 0x%08lx %p\n", pData, DataLen, dnDevInst,
          ResourceID, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Query_Arbitrator_Free_Size(
    PULONG pulSize, DEVINST dnDevInst, RESOURCEID ResourceID, ULONG ulFlags)
{
    TRACE("%p %lu %lx 0x%08lx\n", pulSize, dnDevInst,ResourceID, ulFlags);

    return CM_Query_Arbitrator_Free_Size_Ex(pulSize, dnDevInst, ResourceID,
                                            ulFlags, NULL);
}


/***********************************************************************
 * CM_Query_Arbitrator_Free_Size_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Query_Arbitrator_Free_Size_Ex(
      PULONG pulSize, DEVINST dnDevInst, RESOURCEID ResourceID,
      ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%p %lu %lx 0x%08lx %p\n", pulSize, dnDevInst,ResourceID, ulFlags,
          hMachine);

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
CONFIGRET WINAPI CM_Query_Remove_SubTree(
    DEVINST dnAncestor, ULONG ulFlags)
{
    TRACE("%lx %lx\n", dnAncestor, ulFlags);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Query_Remove_SubTree_Ex [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET WINAPI CM_Query_Remove_SubTree_Ex(
    DEVINST dnAncestor, ULONG ulFlags, HMACHINE hMachine)
{
    TRACE("%lx %lx %lx\n", dnAncestor, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Reenumerate_DevNode [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Reenumerate_DevNode(
    DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%lx %lx\n", dnDevInst, ulFlags);
    return CM_Reenumerate_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Reenumerate_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI
CM_Reenumerate_DevNode_Ex(
    DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("%lx %lx %lx\n", dnDevInst, ulFlags, hMachine);

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
 * CM_Register_Device_InterfaceA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Register_Device_InterfaceA(
    DEVINST dnDevInst, LPGUID InterfaceClassGuid, LPCSTR pszReference,
    LPSTR pszDeviceInterface, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lx %s %s %p %p %lx\n", dnDevInst, debugstr_guid(InterfaceClassGuid),
          pszReference, pszDeviceInterface, pulLength, ulFlags);

    return CM_Register_Device_Interface_ExA(dnDevInst, InterfaceClassGuid,
                                            pszReference, pszDeviceInterface,
                                            pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Register_Device_InterfaceW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Register_Device_InterfaceW(
    DEVINST dnDevInst, LPGUID InterfaceClassGuid, LPCWSTR pszReference,
    LPWSTR pszDeviceInterface, PULONG pulLength, ULONG ulFlags)
{
    TRACE("%lx %s %s %p %p %lx\n", dnDevInst, debugstr_guid(InterfaceClassGuid),
          debugstr_w(pszReference), pszDeviceInterface, pulLength, ulFlags);

    return CM_Register_Device_Interface_ExW(dnDevInst, InterfaceClassGuid,
                                            pszReference, pszDeviceInterface,
                                            pulLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Register_Device_Interface_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Register_Device_Interface_ExA(
    DEVINST dnDevInst, LPGUID InterfaceClassGuid, LPCSTR pszReference,
    LPSTR pszDeviceInterface, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    LPWSTR pszReferenceW = NULL;
    LPWSTR pszDeviceInterfaceW = NULL;
    ULONG ulLength;
    CONFIGRET ret;

    TRACE("%lx %s %s %p %p %lx %lx\n", dnDevInst, debugstr_guid(InterfaceClassGuid),
          pszReference, pszDeviceInterface, pulLength, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Register_Device_Interface_ExW(
    DEVINST dnDevInst, LPGUID InterfaceClassGuid, LPCWSTR pszReference,
    LPWSTR pszDeviceInterface, PULONG pulLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    ULONG ulTransferLength;
    CONFIGRET ret;

    TRACE("%lx %s %s %p %p %lx %lx\n", dnDevInst, debugstr_guid(InterfaceClassGuid),
          debugstr_w(pszReference), pszDeviceInterface, pulLength, ulFlags, hMachine);

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
 * CM_Register_Device_Driver [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Register_Device_Driver(
    DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%lx 0x%08lx\n", dnDevInst, ulFlags);
    return CM_Register_Device_Driver_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Register_Device_Driver [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Register_Device_Driver_Ex(
    DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%lx 0x%08lx %p\n", dnDevInst, ulFlags, hMachine);

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
 * CM_Remove_SubTree [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET WINAPI CM_Remove_SubTree(
    DEVINST dnAncestor, ULONG ulFlags)
{
    TRACE("%lx %lx\n", dnAncestor, ulFlags);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Remove_SubTree_Ex [SETUPAPI.@]
 *
 * This function is obsolete in Windows XP and above.
 */
CONFIGRET WINAPI CM_Remove_SubTree_Ex(
    DEVINST dnAncestor, ULONG ulFlags, HMACHINE hMachine)
{
    TRACE("%lx %lx %lx\n", dnAncestor, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Request_Device_EjectA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Request_Device_EjectA(
    DEVINST dnDevInst, PPNP_VETO_TYPE pVetoType, LPSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags)
{
    TRACE("%lx %p %s %lu %lx\n", dnDevInst, pVetoType, pszVetoName,
          ulNameLength, ulFlags);
    return CM_Request_Device_Eject_ExA(dnDevInst, pVetoType, pszVetoName,
                                       ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Request_Device_EjectW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Request_Device_EjectW(
    DEVINST dnDevInst, PPNP_VETO_TYPE pVetoType, LPWSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags)
{
    TRACE("%lx %p %s %lu %lx\n", dnDevInst, pVetoType, debugstr_w(pszVetoName),
          ulNameLength, ulFlags);
    return CM_Request_Device_Eject_ExW(dnDevInst, pVetoType, pszVetoName,
                                       ulNameLength, ulFlags, NULL);
}


/***********************************************************************
 * CM_Request_Device_Eject_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Request_Device_Eject_ExA(
    DEVINST dnDevInst, PPNP_VETO_TYPE pVetoType, LPSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags, HMACHINE hMachine)
{
    LPWSTR lpLocalVetoName;
    CONFIGRET ret;

    TRACE("%lx %p %s %lu %lx %lx\n", dnDevInst, pVetoType, pszVetoName,
          ulNameLength, ulFlags, hMachine);

    if (pszVetoName == NULL && ulNameLength == 0)
        return CR_INVALID_POINTER;

    lpLocalVetoName = HeapAlloc(GetProcessHeap(), 0, ulNameLength * sizeof(WCHAR));
    if (lpLocalVetoName == NULL)
        return CR_OUT_OF_MEMORY;

    ret = CM_Request_Device_Eject_ExW(dnDevInst, pVetoType, lpLocalVetoName,
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
 * CM_Request_Device_Eject_ExW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Request_Device_Eject_ExW(
    DEVINST dnDevInst, PPNP_VETO_TYPE pVetoType, LPWSTR pszVetoName,
    ULONG ulNameLength, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%lx %p %s %lu %lx %lx\n", dnDevInst, pVetoType,
          debugstr_w(pszVetoName), ulNameLength, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulFlags != 0)
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
CONFIGRET WINAPI CM_Request_Eject_PC(VOID)
{
    TRACE("\n");
    return CM_Request_Eject_PC_Ex(NULL);
}


/***********************************************************************
 * CM_Request_Eject_PC_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Request_Eject_PC_Ex(
    HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%lx\n", hMachine);

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
CONFIGRET WINAPI CM_Run_Detection(
    ULONG ulFlags)
{
    TRACE("%lx\n", ulFlags);
    return CM_Run_Detection_Ex(ulFlags, NULL);
}


/***********************************************************************
 * CM_Run_Detection_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Run_Detection_Ex(
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%lx %lx\n", ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Set_Class_Registry_PropertyA(
    LPGUID ClassGuid, ULONG ulProperty, PCVOID Buffer, ULONG ulLength,
    ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %lx %p %lu %lx %p\n",
          ClassGuid, ulProperty, Buffer, ulLength, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Set_Class_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_Class_Registry_PropertyW(
    LPGUID ClassGuid, ULONG ulProperty, PCVOID Buffer, ULONG ulLength,
    ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %lx %p %lu %lx %p\n",
          ClassGuid, ulProperty, Buffer, ulLength, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
}


/***********************************************************************
 * CM_Set_DevNode_Problem [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_DevNode_Problem(
    DEVINST dnDevInst, ULONG ulProblem, ULONG ulFlags)
{
    TRACE("%lx %lx %lx\n", dnDevInst, ulProblem, ulFlags);
    return CM_Set_DevNode_Problem_Ex(dnDevInst, ulProblem, ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_DevNode_Problem_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_DevNode_Problem_Ex(
    DEVINST dnDevInst, ULONG ulProblem, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%lx %lx %lx %lx\n", dnDevInst, ulProblem, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Set_DevNode_Registry_PropertyA(
    DEVINST dnDevInst, ULONG ulProperty, PCVOID Buffer, ULONG ulLength,
    ULONG ulFlags)
{
    TRACE("%lx %lu %p %lx %lx\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags);
    return CM_Set_DevNode_Registry_Property_ExA(dnDevInst, ulProperty,
                                                Buffer, ulLength,
                                                ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_DevNode_Registry_PropertyW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_DevNode_Registry_PropertyW(
    DEVINST dnDevInst, ULONG ulProperty, PCVOID Buffer, ULONG ulLength,
    ULONG ulFlags)
{
    TRACE("%lx %lu %p %lx %lx\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags);
    return CM_Set_DevNode_Registry_Property_ExW(dnDevInst, ulProperty,
                                                Buffer, ulLength,
                                                ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_DevNode_Registry_Property_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_DevNode_Registry_Property_ExA(
    DEVINST dnDevInst, ULONG ulProperty, PCVOID Buffer, ULONG ulLength,
    ULONG ulFlags, HMACHINE hMachine)
{
    CONFIGRET ret = CR_SUCCESS;
    LPWSTR lpBuffer;
    ULONG ulType;

    FIXME("%lx %lu %p %lx %lx %lx\n",
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
CONFIGRET WINAPI CM_Set_DevNode_Registry_Property_ExW(
    DEVINST dnDevInst, ULONG ulProperty, PCVOID Buffer, ULONG ulLength,
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    ULONG ulType;
    CONFIGRET ret;

    TRACE("%lx %lu %p %lx %lx %lx\n",
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
CONFIGRET WINAPI CM_Set_HW_Prof(
    ULONG ulHardwareProfile, ULONG ulFlags)
{
    TRACE("%lu %lu\n", ulHardwareProfile, ulFlags);
    return CM_Set_HW_Prof_Ex(ulHardwareProfile, ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_HW_Prof_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_HW_Prof_Ex(
    ULONG ulHardwareProfile, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%lu %lu %lx\n", ulHardwareProfile, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Set_HW_Prof_FlagsA(
    DEVINSTID_A szDevInstName, ULONG ulConfig, ULONG ulValue,
    ULONG ulFlags)
{
    TRACE("%s %lu %lu %lx\n", szDevInstName,
          ulConfig, ulValue, ulFlags);
    return CM_Set_HW_Prof_Flags_ExA(szDevInstName, ulConfig, ulValue,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_HW_Prof_FlagsW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_HW_Prof_FlagsW(
    DEVINSTID_W szDevInstName, ULONG ulConfig, ULONG ulValue,
    ULONG ulFlags)
{
    TRACE("%s %lu %lu %lx\n", debugstr_w(szDevInstName),
          ulConfig, ulValue, ulFlags);
    return CM_Set_HW_Prof_Flags_ExW(szDevInstName, ulConfig, ulValue,
                                    ulFlags, NULL);
}


/***********************************************************************
 * CM_Set_HW_Prof_Flags_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Set_HW_Prof_Flags_ExA(
    DEVINSTID_A szDevInstName, ULONG ulConfig, ULONG ulValue,
    ULONG ulFlags, HMACHINE hMachine)
{
    DEVINSTID_W pszDevIdW = NULL;
    CONFIGRET ret = CR_SUCCESS;

    TRACE("%s %lu %lu %lx %lx\n", szDevInstName,
          ulConfig, ulValue, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Set_HW_Prof_Flags_ExW(
    DEVINSTID_W szDevInstName, ULONG ulConfig, ULONG ulValue,
    ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    FIXME("%s %lu %lu %lx %lx\n", debugstr_w(szDevInstName),
          ulConfig, ulValue, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Setup_DevNode(
    DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%lx %lx\n", dnDevInst, ulFlags);
    return CM_Setup_DevNode_Ex(dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Setup_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Setup_DevNode_Ex(
    DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    FIXME("%lx %lx %lx\n", dnDevInst, ulFlags, hMachine);

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
 * CM_Uninstall_DevNode [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Uninstall_DevNode(
    DEVINST dnPhantom, ULONG ulFlags)
{
    TRACE("%lx %lx\n", dnPhantom, ulFlags);
    return CM_Uninstall_DevNode_Ex(dnPhantom, ulFlags, NULL);
}


/***********************************************************************
 * CM_Uninstall_DevNode_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Uninstall_DevNode_Ex(
    DEVINST dnPhantom, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    HSTRING_TABLE StringTable = NULL;
    LPWSTR lpDevInst;
    CONFIGRET ret;

    TRACE("%lx %lx %lx\n", dnPhantom, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Unregister_Device_InterfaceA(
    LPCSTR pszDeviceInterface, ULONG ulFlags)
{
    TRACE("%s %lx\n", pszDeviceInterface, ulFlags);

    return CM_Unregister_Device_Interface_ExA(pszDeviceInterface,
                                              ulFlags, NULL);
}


/***********************************************************************
 * CM_Unregister_Device_InterfaceW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Unregister_Device_InterfaceW(
    LPCWSTR pszDeviceInterface, ULONG ulFlags)
{
    TRACE("%s %lx\n", debugstr_w(pszDeviceInterface), ulFlags);

    return CM_Unregister_Device_Interface_ExW(pszDeviceInterface,
                                              ulFlags, NULL);
}


/***********************************************************************
 * CM_Unregister_Device_Interface_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Unregister_Device_Interface_ExA(
    LPCSTR pszDeviceInterface, ULONG ulFlags, HMACHINE hMachine)
{
    LPWSTR pszDeviceInterfaceW = NULL;
    CONFIGRET ret;

    TRACE("%s %lx %lx\n", pszDeviceInterface, ulFlags, hMachine);

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
CONFIGRET WINAPI CM_Unregister_Device_Interface_ExW(
    LPCWSTR pszDeviceInterface, ULONG ulFlags, HMACHINE hMachine)
{
    RPC_BINDING_HANDLE BindingHandle = NULL;
    CONFIGRET ret;

    TRACE("%s %lx %lx\n", debugstr_w(pszDeviceInterface), ulFlags, hMachine);

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
