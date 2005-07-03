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

#include "pnp_c.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

typedef struct _MACHINE_INFO
{
  RPC_BINDING_HANDLE BindingHandle;
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

    pMachine = (PMACHINE_INFO)GlobalAlloc(GPTR, sizeof(MACHINE_INFO));
    if (pMachine == NULL)
        return CR_OUT_OF_MEMORY;

    if (!PnpBindRpc(UNCServerName, &pMachine->BindingHandle))
    {
        GlobalFree(pMachine);
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

    if (hMachine == NULL)
        return CR_SUCCESS;

    pMachine = (PMACHINE_INFO)hMachine;
    if (!PnpUnbindRpc(pMachine->BindingHandle))
        return CR_ACCESS_DENIED;

    GlobalFree(pMachine);

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
		case ERROR_SUCCESS:             return CR_SUCCESS;
		case ERROR_ACCESS_DENIED:       return CR_ACCESS_DENIED;
		case ERROR_INSUFFICIENT_BUFFER: return CR_BUFFER_SMALL;
		case ERROR_INVALID_DATA:        return CR_INVALID_DATA;
		case ERROR_INVALID_PARAMETER:   return CR_INVALID_DATA;
		case ERROR_NO_MORE_ITEMS:       return CR_NO_SUCH_VALUE;
		case ERROR_NO_SYSTEM_RESOURCES: return CR_OUT_OF_MEMORY;
		default:                        return CR_FAILURE;
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
        L"System\\CurrentControlSet\\Control\\Class",
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
    return CM_Get_Sibling_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Child_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Child_Ex(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    TRACE("%p %lx %lx %lx\n", pdnDevInst, dnDevInst, ulFlags, hMachine);
    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Device_ID_ListA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListA(
    PCSTR pszFilter, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
    FIXME("%p %p %ld %ld\n", pszFilter, Buffer, BufferLen, ulFlags);
    memset(Buffer,0,2);
    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Device_ID_ListW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListW(
    PCWSTR pszFilter, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
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
    HMACHINE hMachine )
{
    FIXME("%p %p %ld %ld %lx\n",
          pszFilter, Buffer, BufferLen, ulFlags, hMachine);
    memset(Buffer,0,2);
    return CR_SUCCESS;
}


/***********************************************************************
 * CM_Get_Device_ID_List_ExW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_ExW(
    PCWSTR pszFilter, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags,
    HMACHINE hMachine )
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
    FIXME("%p %s %ld\n", pulLen, pszFilter, ulFlags);
    *pulLen = 2;
    return CR_SUCCESS;
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
    FIXME("%p %s %ld %lx\n", pulLen, pszFilter, ulFlags, hMachine);
    *pulLen = 2;
    return CR_SUCCESS;
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
 * CM_Get_Parent [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Parent(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    TRACE("%p %p %lx\n", pdnDevInst, dnDevInst, ulFlags);
    return CM_Get_Sibling_Ex(pdnDevInst, dnDevInst, ulFlags, NULL);
}


/***********************************************************************
 * CM_Get_Parent_Ex [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Get_Parent_Ex(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    TRACE("%p %lx %lx %lx\n", pdnDevInst, dnDevInst, ulFlags, hMachine);
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
    TRACE("%p %lx %lx %lx\n", pdnDevInst, dnDevInst, ulFlags, hMachine);
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
    RPC_STATUS Status;

    FIXME("%lx\n", hMachine);

    if (hMachine != NULL)
    {
        BindingHandle = ((PMACHINE_INFO)hMachine)->BindingHandle;
        if (BindingHandle == NULL)
            return 0;
    }
    else
    {
        Status = PnpGetLocalBindingHandle(&BindingHandle);
        if (Status != RPC_S_OK)
            return 0;
    }

    return PNP_GetVersion(BindingHandle);
}


/***********************************************************************
 * CM_Locate_DevNodeA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Locate_DevNodeA(
    PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags)
{
  FIXME("%p %p %lu\n", pdnDevInst, pDeviceID, ulFlags);
  return CR_SUCCESS;
}


/***********************************************************************
 * CM_Locate_DevNodeW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Locate_DevNodeW(
    PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags)
{
  TRACE("%p %p %lu\n", pdnDevInst, pDeviceID, ulFlags);
  return CM_Locate_DevNode_ExW(pdnDevInst, pDeviceID, ulFlags, NULL);
}


/***********************************************************************
 * CM_Locate_DevNode_ExA [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExA(
    PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
  FIXME("%p %p %lu %lx\n", pdnDevInst, pDeviceID, ulFlags, hMachine);
  return CR_SUCCESS;
}


/***********************************************************************
 * CM_Locate_DevNode_ExW [SETUPAPI.@]
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExW(
    PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
  FIXME("%p %p %lu %lx\n", pdnDevInst, pDeviceID, ulFlags, hMachine);
  return CR_SUCCESS;
}
