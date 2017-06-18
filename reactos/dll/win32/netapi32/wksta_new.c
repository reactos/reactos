/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/wksta_new.c
 * PURPOSE:         Workstation service interface code
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include "wkssvc_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


handle_t __RPC_USER
WKSSVC_IDENTIFY_HANDLE_bind(WKSSVC_IDENTIFY_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("WKSSVC_IDENTIFY_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\wkssvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
WKSSVC_IDENTIFY_HANDLE_unbind(WKSSVC_IDENTIFY_HANDLE pszSystemName,
                              handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("WKSSVC_IDENTIFY_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


handle_t __RPC_USER
WKSSVC_IMPERSONATE_HANDLE_bind(WKSSVC_IMPERSONATE_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("WKSSVC_IMPERSONATE_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\wkssvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
WKSSVC_IMPERSONATE_HANDLE_unbind(WKSSVC_IMPERSONATE_HANDLE pszSystemName,
                                 handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("WKSSVC_IMPERSONATE_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
WINAPI
NetAddAlternateComputerName(
    _In_opt_ LPCWSTR Server,
    _In_ LPCWSTR AlternateName,
    _In_opt_ LPCWSTR DomainAccount,
    _In_opt_ LPCWSTR DomainAccountPassword,
    _In_ ULONG Reserved)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetAddAlternateComputerName(%s %s %s %s 0x%lx)\n",
          debugstr_w(Server), debugstr_w(AlternateName), debugstr_w(DomainAccount),
          debugstr_w(DomainAccountPassword), Reserved);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrAddAlternateComputerName(BindingHandle,
                                              (PWSTR)Server,
                                              (PWSTR)AlternateName,
                                              (PWSTR)DomainAccount,
                                              EncryptedPassword,
                                              Reserved);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetEnumerateComputerNames(
    _In_opt_ LPCWSTR Server,
    _In_ NET_COMPUTER_NAME_TYPE NameType,
    _In_ ULONG Reserved,
    _Out_ PDWORD EntryCount,
    _Out_ LPWSTR **ComputerNames)
{
    PNET_COMPUTER_NAME_ARRAY ComputerNameArray = NULL;
    ULONG BufferSize, i;
    PWSTR *NameBuffer = NULL, Ptr;
    NET_API_STATUS status;

    TRACE("NetEnumerateComputerNames(%s %lu %lu %p %p)\n",
          debugstr_w(Server), NameType, Reserved, EntryCount, ComputerNames);

    RpcTryExcept
    {
        status = NetrEnumerateComputerNames((PWSTR)Server,
                                            NameType,
                                            Reserved,
                                            &ComputerNameArray);
        if (status == NERR_Success)
        {
            *EntryCount = ComputerNameArray->EntryCount;

            BufferSize = 0;
            for (i = 0; i < ComputerNameArray->EntryCount; i++)
            {
                BufferSize += ComputerNameArray->ComputerNames[i].Length + sizeof(WCHAR) + sizeof(PWSTR);
            }

            status = NetApiBufferAllocate(BufferSize, (PVOID*)&NameBuffer);
            if (status == NERR_Success)
            {
                ZeroMemory(NameBuffer, BufferSize);

                Ptr = (PWSTR)((ULONG_PTR)NameBuffer + ComputerNameArray->EntryCount * sizeof(PWSTR));
                for (i = 0; i < ComputerNameArray->EntryCount; i++)
                {
                    NameBuffer[i] = Ptr;
                    CopyMemory(Ptr,
                               ComputerNameArray->ComputerNames[i].Buffer,
                               ComputerNameArray->ComputerNames[i].Length);
                    Ptr = (PWSTR)((ULONG_PTR)Ptr + ComputerNameArray->ComputerNames[i].Length + sizeof(WCHAR));
                }

                *ComputerNames = NameBuffer;
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


#if 0
NET_API_STATUS
WINAPI
NetGetJoinInformation(
    LPCWSTR Server,
    LPWSTR *Name,
    PNETSETUP_JOIN_STATUS type)
{
    NET_API_STATUS status;

    TRACE("NetGetJoinInformation(%s %p %p)\n", debugstr_w(Server),
          Name, type);

    if (Name == NULL || type == NULL)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        status = NetrGetJoinInformation((LPWSTR)Server,
                                        Name,
                                        type);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}
#endif


NET_API_STATUS
WINAPI
NetGetJoinableOUs(
    _In_ LPCWSTR lpServer,
    _In_ LPCWSTR lpDomain,
    _In_ LPCWSTR lpAccount,
    _In_ LPCWSTR lpPassword,
    _Out_ DWORD *OUCount,
    _Out_ LPWSTR **OUs)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetGetJoinableOUs(%s %s %s %s %p %p)\n",
          debugstr_w(lpServer), debugstr_w(lpDomain), debugstr_w(lpAccount),
          debugstr_w(lpPassword), OUCount, OUs);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrGetJoinableOUs2(BindingHandle,
                                     (PWSTR)lpServer,
                                     (PWSTR)lpDomain,
                                     (PWSTR)lpAccount,
                                     EncryptedPassword,
                                     OUCount,
                                     OUs);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetJoinDomain(
    _In_ LPCWSTR lpServer,
    _In_ LPCWSTR lpDomain,
    _In_ LPCWSTR lpAccountOU,
    _In_ LPCWSTR lpAccount,
    _In_ LPCWSTR lpPassword,
    _In_ DWORD fJoinOptions)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetJoinDomain(%s %s %s %s 0x%lx)\n",
          debugstr_w(lpServer), debugstr_w(lpDomain), debugstr_w(lpAccountOU),
          debugstr_w(lpAccount), debugstr_w(lpPassword), fJoinOptions);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrJoinDomain2(BindingHandle,
                                 (PWSTR)lpServer,
                                 (PWSTR)lpDomain,
                                 (PWSTR)lpAccountOU,
                                 (PWSTR)lpAccount,
                                 EncryptedPassword,
                                 fJoinOptions);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetRemoveAlternateComputerName(
    _In_opt_ LPCWSTR Server,
    _In_ LPCWSTR AlternateName,
    _In_opt_ LPCWSTR DomainAccount,
    _In_opt_ LPCWSTR DomainAccountPassword,
    _In_ ULONG Reserved)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetRemoveAlternateComputerName(%s %s %s %s 0x%lx)\n",
          debugstr_w(Server), debugstr_w(AlternateName), debugstr_w(DomainAccount),
          debugstr_w(DomainAccountPassword), Reserved);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrRemoveAlternateComputerName(BindingHandle,
                                                 (PWSTR)Server,
                                                 (PWSTR)AlternateName,
                                                 (PWSTR)DomainAccount,
                                                 EncryptedPassword,
                                                 Reserved);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetRenameMachineInDomain(
    _In_ LPCWSTR lpServer,
    _In_ LPCWSTR lpNewMachineName,
    _In_ LPCWSTR lpAccount,
    _In_ LPCWSTR lpPassword,
    _In_ DWORD fRenameOptions)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetRenameMachineInDomain(%s %s %s %s 0x%lx)\n",
          debugstr_w(lpServer), debugstr_w(lpNewMachineName), debugstr_w(lpAccount),
          debugstr_w(lpPassword), fRenameOptions);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrRenameMachineInDomain2(BindingHandle,
                                            (PWSTR)lpServer,
                                            (PWSTR)lpNewMachineName,
                                            (PWSTR)lpAccount,
                                            EncryptedPassword,
                                            fRenameOptions);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetSetPrimaryComputerName(
    _In_opt_ LPCWSTR Server,
    _In_ LPCWSTR PrimaryName,
    _In_opt_ LPCWSTR DomainAccount,
    _In_opt_ LPCWSTR DomainAccountPassword,
    _In_ ULONG Reserved)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetSetPrimaryComputerName(%s %s %s %s %lu)\n",
          debugstr_w(Server), debugstr_w(PrimaryName), debugstr_w(DomainAccount),
          debugstr_w(DomainAccountPassword), Reserved);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrSetPrimaryComputerName(BindingHandle,
                                            (PWSTR)Server,
                                            (PWSTR)PrimaryName,
                                            (PWSTR)DomainAccount,
                                            EncryptedPassword,
                                            Reserved);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetUnjoinDomain(
    _In_ LPCWSTR lpServer,
    _In_ LPCWSTR lpAccount,
    _In_ LPCWSTR lpPassword,
    _In_ DWORD fUnjoinOptions)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetUnjoinDomain(%s %s %s %s 0x%lx)\n",
          debugstr_w(lpServer), debugstr_w(lpAccount),
          debugstr_w(lpPassword), fUnjoinOptions);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrUnjoinDomain2(BindingHandle,
                                   (PWSTR)lpServer,
                                   (PWSTR)lpAccount,
                                   EncryptedPassword,
                                   fUnjoinOptions);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetUseAdd(
    LMSTR UncServerName,
    DWORD Level,
    LPBYTE Buf,
    LPDWORD ParmError)
{
    NET_API_STATUS status;

    TRACE("NetUseAdd(%s %d %p %p)\n", debugstr_w(UncServerName),
          Level, Buf, ParmError);

    RpcTryExcept
    {
        status = NetrUseAdd(UncServerName,
                            Level,
                            (LPUSE_INFO)Buf,
                            ParmError);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetUseDel(
    LMSTR UncServerName,
    LMSTR UseName,
    DWORD ForceCond)
{
    NET_API_STATUS status;

    TRACE("NetUseDel(%s %s %d)\n", debugstr_w(UncServerName),
          debugstr_w(UseName), ForceCond);

    RpcTryExcept
    {
        status = NetrUseDel(UncServerName,
                            UseName,
                            ForceCond);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetUseEnum(
    LMSTR UncServerName,
    DWORD Level,
    LPBYTE *BufPtr,
    DWORD PreferedMaximumSize,
    LPDWORD EntriesRead,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle)
{
    USE_ENUM_STRUCT UseEnumInfo;
    USE_INFO_0_CONTAINER Container0;
    USE_INFO_1_CONTAINER Container1;
    USE_INFO_2_CONTAINER Container2;
    NET_API_STATUS status;

    TRACE("NetUseEnum(%s, %d, %p, %d, %p, %p, %p)\n", debugstr_w(UncServerName),
          Level, BufPtr, PreferedMaximumSize, EntriesRead, TotalEntries, ResumeHandle);

    UseEnumInfo.Level = Level;
    switch (Level)
    {
        case 0:
            UseEnumInfo.UseInfo.Level0 = &Container0;
            Container0.EntriesRead = 0;
            Container0.Buffer = NULL;
            break;

        case 1:
            UseEnumInfo.UseInfo.Level1 = &Container1;
            Container1.EntriesRead = 0;
            Container1.Buffer = NULL;
            break;

        case 2:
            UseEnumInfo.UseInfo.Level2 = &Container2;
            Container2.EntriesRead = 0;
            Container2.Buffer = NULL;
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        status = NetrUseEnum(UncServerName,
                             &UseEnumInfo,
                             PreferedMaximumSize,
                             TotalEntries,
                             ResumeHandle);
        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            switch (Level)
            {
                case 0:
                    *BufPtr = (LPBYTE)UseEnumInfo.UseInfo.Level0->Buffer;
                    *EntriesRead = UseEnumInfo.UseInfo.Level0->EntriesRead;
                    break;

                case 1:
                    *BufPtr = (LPBYTE)UseEnumInfo.UseInfo.Level1->Buffer;
                    *EntriesRead = UseEnumInfo.UseInfo.Level1->EntriesRead;
                    break;

                case 2:
                    *BufPtr = (LPBYTE)UseEnumInfo.UseInfo.Level2->Buffer;
                    *EntriesRead = UseEnumInfo.UseInfo.Level2->EntriesRead;
                    break;
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetValidateName(
    _In_ LPCWSTR lpServer,
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpAccount,
    _In_ LPCWSTR lpPassword,
    _In_ NETSETUP_NAME_TYPE NameType)
{
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedPassword;
    handle_t BindingHandle;
    NET_API_STATUS status;

    TRACE("NetValidateName(%s %s %s %s %u)\n",
          debugstr_w(lpServer), debugstr_w(lpName), debugstr_w(lpAccount),
          debugstr_w(lpPassword), NameType);

    /* FIXME */
    BindingHandle = NULL;
    EncryptedPassword = NULL;

    RpcTryExcept
    {
        status = NetrValidateName2(BindingHandle,
                                   (PWSTR)lpServer,
                                   (PWSTR)lpName,
                                   (PWSTR)lpAccount,
                                   EncryptedPassword,
                                   NameType);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetUseGetInfo(
    LMSTR UncServerName,
    LMSTR UseName,
    DWORD Level,
    LPBYTE *BufPtr)
{
    NET_API_STATUS status;

    TRACE("NetUseGetInfo(%s, %s, %d, %p)\n", debugstr_w(UncServerName),
          debugstr_w(UseName), Level, BufPtr);

    *BufPtr = NULL;

    RpcTryExcept
    {
        status = NetrUseGetInfo(UncServerName,
                                UseName,
                                Level,
                                (LPUSE_INFO)BufPtr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


#if 0
NET_API_STATUS
WINAPI
NetWkstaGetInfo(
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr)
{
    NET_API_STATUS status;

    TRACE("NetWkstaGetInfo(%s, %d, %p)\n", debugstr_w(servername),
          level, bufptr);

    if (bufptr == NULL)
        return ERROR_INVALID_PARAMETER;

    *bufptr = NULL;

    RpcTryExcept
    {
        status = NetrWkstaGetInfo(servername,
                                  level,
                                  (LPWKSTA_INFO)bufptr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}
#endif


NET_API_STATUS
WINAPI
NetWkstaSetInfo(
    LPWSTR servername,
    DWORD level,
    LPBYTE buffer,
    LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetWkstaSetInfo(%s, %d, %p, %p)\n", debugstr_w(servername),
          level, buffer, parm_err);

    RpcTryExcept
    {
        status = NetrWkstaSetInfo(servername,
                                  level,
                                  (LPWKSTA_INFO)buffer,
                                  parm_err);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaTransportAdd(
    LPWSTR servername,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetWkstaTransportAdd(%s, %d, %p, %p)\n", debugstr_w(servername),
          level, buf, parm_err);

    RpcTryExcept
    {
        status = NetrWkstaTransportAdd(servername,
                                       level,
                                       (LPWKSTA_TRANSPORT_INFO_0)buf,
                                       parm_err);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaTransportDel(
    LPWSTR servername,
    LPWSTR transportname,
    DWORD ucond)
{
    NET_API_STATUS status;

    TRACE("NetWkstaTransportDel(%s, %s, %d)\n", debugstr_w(servername),
          debugstr_w(transportname), ucond);

    RpcTryExcept
    {
        status = NetrWkstaTransportDel(servername,
                                       transportname,
                                       ucond);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


#if 0
NET_API_STATUS
WINAPI
NetWkstaTransportEnum(
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    LPDWORD resumehandle)
{
    WKSTA_TRANSPORT_ENUM_STRUCT TransportEnumInfo;
    WKSTA_TRANSPORT_INFO_0_CONTAINER Container0;
    NET_API_STATUS status;

    TRACE("NetWkstaTransportEnum(%s, %d, %p, %d, %p, %p, %p)\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);

    TransportEnumInfo.Level = level;
    switch (level)
    {
        case 0:
            TransportEnumInfo.WkstaTransportInfo.Level0 = &Container0;
            Container0.EntriesRead = 0;
            Container0.Buffer = NULL;
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        status = NetrWkstaTransportEnum(servername,
                                        &TransportEnumInfo,
                                        prefmaxlen,
                                        totalentries,
                                        resumehandle);
        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            switch (level)
            {
                case 0:
                    *bufptr = (LPBYTE)TransportEnumInfo.WkstaTransportInfo.Level0->Buffer;
                    *entriesread = TransportEnumInfo.WkstaTransportInfo.Level0->EntriesRead;
                    break;
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaUserEnum(
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    LPDWORD resumehandle)
{
    WKSTA_USER_ENUM_STRUCT UserEnumInfo;
    WKSTA_USER_INFO_0_CONTAINER Container0;
    WKSTA_USER_INFO_1_CONTAINER Container1;
    NET_API_STATUS status;

    TRACE("NetWkstaUserEnum(%s, %d, %p, %d, %p, %p, %p)\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);

    UserEnumInfo.Level = level;
    switch (level)
    {
        case 0:
            UserEnumInfo.WkstaUserInfo.Level0 = &Container0;
            Container0.EntriesRead = 0;
            Container0.Buffer = NULL;
            break;

        case 1:
            UserEnumInfo.WkstaUserInfo.Level1 = &Container1;
            Container1.EntriesRead = 0;
            Container1.Buffer = NULL;
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        status = NetrWkstaUserEnum(servername,
                                   &UserEnumInfo,
                                   prefmaxlen,
                                   totalentries,
                                   resumehandle);
        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            switch (level)
            {
                case 0:
                    *bufptr = (LPBYTE)UserEnumInfo.WkstaUserInfo.Level0->Buffer;
                    *entriesread = UserEnumInfo.WkstaUserInfo.Level0->EntriesRead;
                    break;

                case 1:
                    *bufptr = (LPBYTE)UserEnumInfo.WkstaUserInfo.Level1->Buffer;
                    *entriesread = UserEnumInfo.WkstaUserInfo.Level1->EntriesRead;
                    break;
            }
        }
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaUserGetInfo(
    LPWSTR reserved,
    DWORD level,
    PBYTE *bufptr)
{
    NET_API_STATUS status;

    TRACE("NetWkstaUserGetInfo(%s, %d, %p)\n", debugstr_w(reserved),
          level, bufptr);

    if (reserved != NULL)
        return ERROR_INVALID_PARAMETER;

    *bufptr = NULL;

    RpcTryExcept
    {
        status = NetrWkstaUserGetInfo(NULL,
                                      level,
                                      (LPWKSTA_USER_INFO)bufptr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}
#endif


NET_API_STATUS
WINAPI
NetWkstaUserSetInfo(
    LPWSTR reserved,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    NET_API_STATUS status;

    TRACE("NetWkstaSetInfo(%s, %d, %p, %p)\n", debugstr_w(reserved),
          level, buf, parm_err);

    if (reserved != NULL)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
       status = NetrWkstaUserSetInfo(NULL,
                                     level,
                                     (LPWKSTA_USER_INFO)&buf,
                                     parm_err);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}

/* EOF */
