/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             subsys/system/dhcp/pipe.c
 * PURPOSE:          DHCP client pipe
 * PROGRAMMER:       arty
 */

#include <rosdhcp.h>

#define NDEBUG
#include <reactos/debug.h>

#define COMM_PIPE_OUTPUT_BUFFER sizeof(COMM_DHCP_REQ)
#define COMM_PIPE_INPUT_BUFFER sizeof(COMM_DHCP_REPLY)
#define COMM_PIPE_DEFAULT_TIMEOUT 1000

DWORD PipeSend( HANDLE CommPipe, COMM_DHCP_REPLY *Reply ) {
    DWORD Written = 0;
    OVERLAPPED Overlapped = {0};
    BOOL Success =
        WriteFile( CommPipe,
                   Reply,
                   sizeof(*Reply),
                   &Written,
                   &Overlapped);
    if (!Success)
    {
        WaitForSingleObject(CommPipe, INFINITE);
        Success = GetOverlappedResult(CommPipe,
                                      &Overlapped,
                                      &Written,
                                      TRUE);
    }

    return Success ? Written : -1;
}

/**
 * @brief
 * Creates a security descriptor for the DHCP pipe
 * service.
 *
 * @param[out] SecurityDescriptor
 * A pointer to an allocated security descriptor
 * for the DHCP pipe.
 *
 * @return
 * ERROR_SUCCESS is returned if the function has
 * successfully created the descriptor otherwise
 * a Win32 error code is returned.
 *
 * @remarks
 * Both admins and local system are given full power
 * over the DHCP pipe whereas authenticated users
 * and network operators can only read over this pipe.
 * They can also execute it.
 */
DWORD CreateDhcpPipeSecurity( PSECURITY_DESCRIPTOR *SecurityDescriptor ) {
    DWORD ErrCode;
    PACL Dacl;
    ULONG DaclSize, RelSDSize = 0;
    PSECURITY_DESCRIPTOR AbsSD = NULL, RelSD = NULL;
    PSID AuthenticatedUsersSid = NULL, NetworkOpsSid = NULL, AdminsSid = NULL, SystemSid = NULL;
    static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_AUTHENTICATED_USER_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &AuthenticatedUsersSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to create Authenticated Users SID (error code %d)\n", GetLastError());
        return GetLastError();
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS,
                                  0, 0, 0, 0, 0, 0,
                                  &NetworkOpsSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to create Network Ops SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to create Admins SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &SystemSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to create Local System SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    AbsSD = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SECURITY_DESCRIPTOR));
    if (!AbsSD)
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to allocate absolute security descriptor!\n");
        ErrCode = ERROR_OUTOFMEMORY;
        goto Quit;
    }

    if (!InitializeSecurityDescriptor(AbsSD, SECURITY_DESCRIPTOR_REVISION))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to initialize absolute security descriptor (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(AuthenticatedUsersSid) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(NetworkOpsSid) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(SystemSid);

    Dacl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DaclSize);
    if (!Dacl)
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to allocate DACL!\n");
        ErrCode = ERROR_OUTOFMEMORY;
        goto Quit;
    }

    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to initialize DACL (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_READ | GENERIC_EXECUTE,
                             AuthenticatedUsersSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up ACE for Authenticated Users SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_READ | GENERIC_EXECUTE,
                             NetworkOpsSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up ACE for Network Ops SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             AdminsSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up ACE for Admins SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             SystemSid))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up ACE for Local System SID (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!SetSecurityDescriptorDacl(AbsSD, TRUE, Dacl, FALSE))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up DACL to absolute security descriptor (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!SetSecurityDescriptorOwner(AbsSD, AdminsSid, FALSE))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up owner to absolute security descriptor (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!SetSecurityDescriptorGroup(AbsSD, SystemSid, FALSE))
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to set up group to absolute security descriptor (error code %d)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    if (!MakeSelfRelativeSD(AbsSD, NULL, &RelSDSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        DPRINT1("CreateDhcpPipeSecurity(): Unexpected error code (error code %d -- must be ERROR_INSUFFICIENT_BUFFER)\n", GetLastError());
        ErrCode = GetLastError();
        goto Quit;
    }

    RelSD = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RelSDSize);
    if (RelSD == NULL)
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to allocate relative SD!\n");
        ErrCode = ERROR_OUTOFMEMORY;
        goto Quit;
    }

    if (!MakeSelfRelativeSD(AbsSD, RelSD, &RelSDSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        DPRINT1("CreateDhcpPipeSecurity(): Failed to allocate relative SD, buffer too smal (expected size %lu)\n", RelSDSize);
        ErrCode = ERROR_INSUFFICIENT_BUFFER;
        goto Quit;
    }

    *SecurityDescriptor = RelSD;
    ErrCode = ERROR_SUCCESS;

Quit:
    if (ErrCode != ERROR_SUCCESS)
    {
        if (RelSD)
        {
            HeapFree(GetProcessHeap(), 0, RelSD);
        }
    }

    if (AuthenticatedUsersSid)
    {
        FreeSid(AuthenticatedUsersSid);
    }

    if (NetworkOpsSid)
    {
        FreeSid(NetworkOpsSid);
    }

    if (AdminsSid)
    {
        FreeSid(AdminsSid);
    }

    if (SystemSid)
    {
        FreeSid(SystemSid);
    }

    if (Dacl)
    {
        HeapFree(GetProcessHeap(), 0, Dacl);
    }

    if (AbsSD)
    {
        HeapFree(GetProcessHeap(), 0, AbsSD);
    }

    return ErrCode;
}

DWORD WINAPI PipeThreadProc( LPVOID Parameter ) {
    DWORD BytesRead;
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    BOOL Result, Connected;
    HANDLE Events[2];
    HANDLE CommPipe;
    OVERLAPPED Overlapped = {0};
    DWORD dwError;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_DESCRIPTOR DhcpPipeSD = NULL;

    DPRINT("PipeThreadProc(%p)\n", Parameter);

    dwError = CreateDhcpPipeSecurity(&DhcpPipeSD);
    if (dwError != ERROR_SUCCESS)
    {
        DbgPrint("DHCP: Could not create security descriptor for pipe\n");
        return FALSE;
    }

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = DhcpPipeSD;
    SecurityAttributes.bInheritHandle = FALSE;

    CommPipe = CreateNamedPipeW
        ( DHCP_PIPE_NAME,
          PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
          1,
          COMM_PIPE_OUTPUT_BUFFER,
          COMM_PIPE_INPUT_BUFFER,
          COMM_PIPE_DEFAULT_TIMEOUT,
          &SecurityAttributes );
    HeapFree(GetProcessHeap(), 0, DhcpPipeSD);
    if (CommPipe == INVALID_HANDLE_VALUE)
    {
        DbgPrint("DHCP: Could not create named pipe\n");
        return FALSE;
    }

    Events[0] = (HANDLE)Parameter;
    Events[1] = CommPipe;

    while( TRUE )
    {
        Connected = ConnectNamedPipe(CommPipe, &Overlapped);
        if (!Connected)
        {
            dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING)
            {
                dwError = WaitForMultipleObjects(2, Events, FALSE, INFINITE);
                DPRINT("WaitForMultipleObjects() returned %lu\n", dwError);
                if (dwError == WAIT_OBJECT_0 + 1)
                {
                    Connected = GetOverlappedResult(CommPipe,
                                                    &Overlapped,
                                                    &BytesRead,
                                                    TRUE);
                }
                else if (dwError == WAIT_OBJECT_0)
                {
                    CancelIo(CommPipe);
                    CloseHandle(CommPipe);
                    CommPipe = INVALID_HANDLE_VALUE;
                    break;
                }
            }
        }

        if (!Connected) {
            DbgPrint("DHCP: Could not connect named pipe\n");
            CloseHandle( CommPipe );
            CommPipe = INVALID_HANDLE_VALUE;
            break;
        }

        Result = ReadFile(CommPipe, &Req, sizeof(Req), &BytesRead, &Overlapped);
        if (!Result)
        {
            dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING)
            {
                dwError = WaitForMultipleObjects(2, Events, FALSE, INFINITE);
                DPRINT("WaitForMultipleObjects() returned %lu\n", dwError);
                if (dwError == WAIT_OBJECT_0 + 1)
                {
                    Result = GetOverlappedResult(CommPipe,
                                                 &Overlapped,
                                                 &BytesRead,
                                                 TRUE);
                }
                else if (dwError == WAIT_OBJECT_0)
                {
                    CancelIo(CommPipe);
                    DisconnectNamedPipe( CommPipe );
                    CloseHandle(CommPipe);
                    CommPipe = INVALID_HANDLE_VALUE;
                    break;
                }
            }
        }

        if( Result ) {
            switch( Req.Type ) {
            case DhcpReqQueryHWInfo:
                DSQueryHWInfo( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqLeaseIpAddress:
                DSLeaseIpAddress( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqReleaseIpAddress:
                DSReleaseIpAddressLease( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqRenewIpAddress:
                DSRenewIpAddressLease( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqStaticRefreshParams:
                DSStaticRefreshParams( PipeSend, CommPipe, &Req );
                break;

            case DhcpReqGetAdapterInfo:
                DSGetAdapterInfo( PipeSend, CommPipe, &Req );
                break;

            default:
                DPRINT1("Unrecognized request type %d\n", Req.Type);
                ZeroMemory( &Reply, sizeof( COMM_DHCP_REPLY ) );
                Reply.Reply = 0;
                PipeSend(CommPipe, &Reply );
                break;
            }
        }
        DisconnectNamedPipe( CommPipe );
    }

    DPRINT("Pipe thread stopped!\n");

    return TRUE;
}

HANDLE PipeInit(HANDLE hStopEvent)
{
    return CreateThread( NULL, 0, PipeThreadProc, (LPVOID)hStopEvent, 0, NULL);
}
