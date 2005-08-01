/*
 * Configuration manager functions
 *
 * Copyright 2000 James Hatheway
 * Copyright 2005 Eric Kohl
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "setupapi_private.h"

#include "rpc.h"
#include "rpc_private.h"

#include "pnp_c.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Registry key and value names */
static const WCHAR ControlClass[] = {'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'C','l','a','s','s',0};


typedef struct _MACHINE_INFO
{
    WCHAR szMachineName[MAX_PATH];
    RPC_BINDING_HANDLE BindingHandle;
    HSTRING_TABLE StringTable;
} MACHINE_INFO, *PMACHINE_INFO;


/***********************************************************************
 * CM_Connect_MachineA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Connect_MachineA(PCSTR UNCServerName, PHMACHINE phMachine)
{
    PWSTR pServerNameW;
    CONFIGRET ret;

    TRACE("%s %p\n", UNCServerName, phMachine);

    if (UNCServerName == NULL || *UNCServerName == 0)
        return CM_Connect_MachineW(NULL, phMachine);

    if (CaptureAndConvertAnsiArg(UNCServerName, &pServerNameW))
        return CR_INVALID_DATA;

    ret = CM_Connect_MachineW(pServerNameW, phMachine);

    MyFree(pServerNameW);

    return ret;
}


/***********************************************************************
 * CM_Connect_MachineW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Connect_MachineW(PCWSTR UNCServerName, PHMACHINE phMachine)
{
    PMACHINE_INFO pMachine;

    TRACE("%s %p\n", debugstr_w(UNCServerName), phMachine);

    pMachine = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MACHINE_INFO));
    if (pMachine == NULL)
        return CR_OUT_OF_MEMORY;

    lstrcpyW(pMachine->szMachineName, UNCServerName);

    pMachine->StringTable = StringTableInitialize();
    if (pMachine->StringTable == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pMachine);
        return CR_FAILURE;
    }

    StringTableAddString(pMachine->StringTable, L"PLT", 1);

    if (!PnpBindRpc(UNCServerName, &pMachine->BindingHandle))
    {
        StringTableDestroy(pMachine->StringTable);
        HeapFree(GetProcessHeap(), 0, pMachine);
        return CR_INVALID_MACHINENAME;
    }

    phMachine = (PHMACHINE)pMachine;

    return CR_SUCCESS;
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

    if (pMachine->StringTable != NULL)
        StringTableDestroy(pMachine->StringTable);

    if (!PnpUnbindRpc(pMachine->BindingHandle))
        return CR_ACCESS_DENIED;

    HeapFree(GetProcessHeap(), 0, pMachine);

    return CR_SUCCESS;
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


static CONFIGRET GetCmCodeFromErrorCode(DWORD ErrorCode)
{
    switch (ErrorCode)
    {
        case ERROR_SUCCESS:
            return CR_SUCCESS;

        case ERROR_ACCESS_DENIED:
            return CR_ACCESS_DENIED;

        case ERROR_INSUFFICIENT_BUFFER:
            return CR_BUFFER_SMALL;

        case ERROR_INVALID_DATA:
            return CR_INVALID_DATA;

        case ERROR_INVALID_PARAMETER:
            return CR_INVALID_DATA;

        case ERROR_NO_MORE_ITEMS:
            return CR_NO_SUCH_VALUE;

        case ERROR_NO_SYSTEM_RESOURCES:
            return CR_OUT_OF_MEMORY;

        default:
            return CR_FAILURE;
    }
}


/***********************************************************************
 * CM_Enumerate_Classes_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Enumerate_Classes_Ex(
    ULONG ulClassIndex, LPGUID ClassGuid, ULONG ulFlags, HMACHINE hMachine)
{
    HKEY hRelativeKey, hKey;
    DWORD rc;
    WCHAR Buffer[39];

    TRACE("%lx %p %lx %p\n", ulClassIndex, ClassGuid, ulFlags, hMachine);

    if (hMachine != NULL)
    {
        FIXME("hMachine argument ignored\n");
        hRelativeKey = HKEY_LOCAL_MACHINE; /* FIXME: use here a field in hMachine */
    }
    else
        hRelativeKey = HKEY_LOCAL_MACHINE;

    rc = RegOpenKeyExW(
        hRelativeKey,
        ControlClass,
        0, /* options */
        KEY_ENUMERATE_SUB_KEYS,
        &hKey);
    if (rc != ERROR_SUCCESS)
        return GetCmCodeFromErrorCode(rc);

    rc = RegEnumKeyW(
        hKey,
        ulClassIndex,
        Buffer,
        sizeof(Buffer) / sizeof(WCHAR));

    RegCloseKey(hKey);

    if (rc == ERROR_SUCCESS)
    {
        /* Remove the {} */
        Buffer[37] = UNICODE_NULL;
        /* Convert the buffer to a GUID */
        if (UuidFromStringW(&Buffer[1], ClassGuid) != RPC_S_OK)
            return CR_FAILURE;
    }

    return GetCmCodeFromErrorCode(rc);
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
    DWORD dwIndex;
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ret = PNP_GetRelatedDeviceInstance(BindingHandle,
                                       PNP_DEVICE_CHILD,
                                       lpDevInst,
                                       szRelatedDevInst,
                                       MAX_DEVICE_ID_LEN,
                                       0);
    if (ret != CR_SUCCESS)
        return ret;

    TRACE("szRelatedDevInst: %s\n", debugstr_w(szRelatedDevInst));

    dwIndex = StringTableAddString(StringTable, szRelatedDevInst, 1);
    if (dwIndex == -1)
        return CR_FAILURE;

    *pdnDevInst = dwIndex;

    return CR_SUCCESS;
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    return PNP_GetDepth(BindingHandle,
                        lpDevInst,
                        pulDepth,
                        ulFlags);
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
    FIXME("%lx %lu %p %p %p %lx %lx\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength,
          ulFlags, hMachine);

    return CR_CALL_NOT_IMPLEMENTED;
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
    ULONG ulDataType = 0;
    ULONG ulTransferLength = 0;

    FIXME("%lx %lu %p %p %p %lx %lx\n",
          dnDevInst, ulProperty, pulRegDataType, Buffer, pulLength,
          ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulProperty < 1 /* CM_DRP_MIN */ || ulProperty > 0x17 /* CM_DRP_MAX */)
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ret = PNP_GetDeviceRegProp(BindingHandle,
                               lpDevInst,
                               ulProperty,
                               &ulDataType,
                               Buffer,
                               &ulTransferLength,
                               pulLength,
                               ulFlags);
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    return PNP_GetDeviceStatus(BindingHandle,
                               lpDevInst,
                               pulStatus,
                               pulProblemNumber,
                               ulFlags);
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

    FIXME("%lx %p %ld %ld %lx\n",
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

    if (!StringTableStringFromIdEx(StringTable,
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

    FIXME("%p %p %ld %ld %lx\n",
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
        if (CaptureAndConvertAnsiArg(pszFilter, &pszFilterW))
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
    FIXME("%p %p %ld %ld %lx\n",
          pszFilter, Buffer, BufferLen, ulFlags, hMachine);
    memset(Buffer,0,2);
    return CR_SUCCESS;
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
        if (CaptureAndConvertAnsiArg(pszFilter, &pszFilterW))
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
    FIXME("%p %s %ld %lx\n", pulLen, debugstr_w(pszFilter), ulFlags, hMachine);
    *pulLen = 2;
    return CR_SUCCESS;
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

    DeviceId = StringTableStringFromId(StringTable, dnDevInst);
    if (DeviceId == NULL)
    {
        *pulLen = 0;
        return CR_SUCCESS;
    }

    *pulLen = lstrlenW(DeviceId);

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

    return PNP_GetGlobalState(BindingHandle, pulState, ulFlags);
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
    DWORD dwIndex;
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ret = PNP_GetRelatedDeviceInstance(BindingHandle,
                                       PNP_DEVICE_PARENT,
                                       lpDevInst,
                                       szRelatedDevInst,
                                       MAX_DEVICE_ID_LEN,
                                       0);
    if (ret != CR_SUCCESS)
        return ret;

    TRACE("szRelatedDevInst: %s\n", debugstr_w(szRelatedDevInst));

    dwIndex = StringTableAddString(StringTable, szRelatedDevInst, 1);
    if (dwIndex == -1)
        return CR_FAILURE;

    *pdnDevInst = dwIndex;

    return CR_SUCCESS;
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
    DWORD dwIndex;
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ret = PNP_GetRelatedDeviceInstance(BindingHandle,
                                       PNP_DEVICE_SIBLING,
                                       lpDevInst,
                                       szRelatedDevInst,
                                       MAX_DEVICE_ID_LEN,
                                       0);
    if (ret != CR_SUCCESS)
        return ret;

    TRACE("szRelatedDevInst: %s\n", debugstr_w(szRelatedDevInst));

    dwIndex = StringTableAddString(StringTable, szRelatedDevInst, 1);
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

    if (PNP_GetVersion(BindingHandle, &Version) != CR_SUCCESS)
        return 0;

    return Version;
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
       if (CaptureAndConvertAnsiArg(pDeviceID, &pDevIdW))
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
        /* Get the root device ID */
        ret = PNP_GetRootDeviceInstance(BindingHandle,
                                        DeviceIdBuffer,
                                        MAX_DEVICE_ID_LEN);
        if (ret != CR_SUCCESS)
            return CR_FAILURE;
    }
    TRACE("DeviceIdBuffer: %s\n", debugstr_w(DeviceIdBuffer));

    /* Validate the device ID */
    ret = PNP_ValidateDeviceInstance(BindingHandle,
                                     DeviceIdBuffer,
                                     ulFlags);
    if (ret == CR_SUCCESS)
    {
        *pdnDevInst = StringTableAddString(StringTable, DeviceIdBuffer, 1);
        if (*pdnDevInst == -1)
            ret = CR_FAILURE;
    }

    return ret;
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    return PNP_SetDeviceProblem(BindingHandle,
                                lpDevInst,
                                ulProblem,
                                ulFlags);
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
    FIXME("%lx %lu %p %lx %lx %lx\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags, hMachine);
    return CR_CALL_NOT_IMPLEMENTED;
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

    FIXME("%lx %lu %p %lx %lx %lx\n",
          dnDevInst, ulProperty, Buffer, ulLength, ulFlags, hMachine);

    if (dnDevInst == 0)
        return CR_INVALID_DEVNODE;

    if (ulProperty < 1 /* CM_DRP_MIN */ || ulProperty > 0x17 /* CM_DRP_MAX */)
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

    lpDevInst = StringTableStringFromId(StringTable, dnDevInst);
    if (lpDevInst == NULL)
        return CR_INVALID_DEVNODE;

    ulType = REG_SZ; /* FIXME */

    return PNP_SetDeviceRegProp(BindingHandle,
                                lpDevInst,
                                ulProperty,
                                ulType,
                                (char *)Buffer,
                                ulLength,
                                ulFlags);
}
