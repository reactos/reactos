/*
 * PROJECT:          ReactOS System Libraries
 * LICENSE:          GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:             dll/win32/wzcsapi/wzcsapi.c
 * PURPOSE:          ReactOS Wireless Zero Configuration API
 * COPYRIGHT:        Copyright 2020 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

/* INCLUDES *****************************************************************/

#include <windef.h>
#include <winbase.h>
#include <objbase.h>

#define NDEBUG
#include <debug.h>

#include <wzcsapi.h>

/* FUNCTIONS ****************************************************************/

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD     fdwReason,
        LPVOID    lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

void __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

DTLNODE*
WINAPI
CreateEapcfgNode(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
WINAPI
DestroyEapcfgNode(IN OUT DTLNODE* pNode)
{
    UNIMPLEMENTED;
}

VOID
WINAPI
DtlDestroyList(DTLLIST*     param1,
               PDESTROYNODE param2)
{
    UNIMPLEMENTED;
}

DTLNODE*
WINAPI
EapcfgNodeFromKey(IN DTLLIST* pList,
                  IN DWORD    dwKey)
{
    UNIMPLEMENTED;
    return 0;
}

DTLLIST*
WINAPI
ReadEapcfgList(IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
WINAPI
WZCDeleteIntfObj(IN PINTF_ENTRY pIntf)
{
    UNIMPLEMENTED;
}

DWORD
WINAPI
WZCEapolFreeState(IN EAPOL_INTF_STATE *pIntfState)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolGetCustomAuthData(IN LPWSTR     pSrvAddr, 
                          IN PWCHAR     pwszGuid, 
                          IN DWORD      dwEapTypeId, 
                          IN DWORD      dwSizeOfSSID, 
                          IN BYTE       *pbSSID, 
                          IN OUT PBYTE  pbConnInfo, 
                          IN OUT PDWORD pdwInfoSize)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolGetInterfaceParams(IN LPWSTR                pSrvAddr,
                           IN PWCHAR                pwszGuid,
                           IN DWORD                 dwSizeOfSSID,
                           IN BYTE                  *pbSSID,
                           IN OUT EAPOL_INTF_PARAMS *pIntfParams)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolQueryState(IN LPWSTR               pSrvAddr, 
                   IN PWCHAR               pwszGuid, 
                   IN OUT EAPOL_INTF_STATE *pIntfState)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolReAuthenticate(IN LPWSTR pSrvAddr, 
                       IN PWCHAR pwszGuid)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolSetCustomAuthData(IN LPWSTR pSrvAddr, 
                          IN PWCHAR pwszGuid, 
                          IN DWORD  dwEapTypeId, 
                          IN DWORD  dwSizeOfSSID, 
                          IN BYTE   *pbSSID, 
                          IN PBYTE  pbConnInfo, 
                          IN DWORD  dwInfoSize)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolSetInterfaceParams(IN LPWSTR            pSrvAddr,
                           IN PWCHAR            pwszGuid,
                           IN DWORD             dwSizeOfSSID,
                           IN BYTE              *pbSSID,
                           IN EAPOL_INTF_PARAMS *pIntfParams)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEapolUIResponse(IN LPWSTR               pSrvAddr, 
                   IN EAPOL_EAP_UI_CONTEXT EapolUIContext, 
                   IN EAPOLUI_RESP         EapolUIResp)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCEnumInterfaces(IN  LPWSTR           pSrvAddr,
                  OUT PINTFS_KEY_TABLE pIntfs)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCGetEapUserInfo(IN WCHAR     *pwszGUID, 
                  IN DWORD     dwEapTypeId, 
                  IN DWORD     dwSizOfSSID, 
                  IN BYTE      *pbSSID, 
                  IN OUT PBYTE pbUserInfo, 
                  IN OUT DWORD *pdwInfoSize)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

VOID
WINAPI
WZCPassword2Key(PWZC_WLAN_CONFIG pwzcConfig, 
                LPCSTR           cszPassword)
{
    UNIMPLEMENTED;
}

DWORD
WINAPI
WZCQueryContext(IN  LPWSTR       pSrvAddr, 
                IN  DWORD        dwInFlags, 
                IN  PWZC_CONTEXT pContext, 
                OUT LPDWORD      pdwOutFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCQueryInterface(IN     LPWSTR      pSrvAddr,
                  IN     DWORD       dwInFlags,
                  IN OUT PINTF_ENTRY pIntf,
                  OUT    LPDWORD     pdwOutFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCRefreshInterface(IN  LPWSTR      pSrvAddr,
                    IN  DWORD       dwInFlags,
                    IN  PINTF_ENTRY pIntf,
                    OUT LPDWORD     pdwOutFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCSetContext(IN  LPWSTR       pSrvAddr, 
              IN  DWORD        dwInFlags, 
              IN  PWZC_CONTEXT pContext, 
              OUT LPDWORD      pdwOutFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
WZCSetInterface(IN  LPWSTR      pSrvAddr, 
                IN  DWORD       dwInFlags, 
                IN  PINTF_ENTRY pIntf, 
                OUT LPDWORD     pdwOutFlags)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
