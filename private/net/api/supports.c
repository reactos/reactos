/*++

Copyright (c) 1991-92 Microsoft Corporation

Module Name:

    Supports.c

Abstract:

    This module determines which optional features that a given remote
    machine supports.  These features are of interest to the RpcXlate
    code, among other people.

Author:

    John Rogers (JohnRo) 28-Mar-1991

Environment:

    Only runs under NT, although the interface is portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    28-Mar-1991 Johnro
        Created.
    02-Apr-1991 JohnRo
        Moved NetpRdrFsControlTree to <netlibnt.h>.
    06-May-1991 JohnRo
        Implement UNICODE.
    26-Jul-1991 JohnRo
        Quiet DLL stub debug output.
    31-Oct-1991 JohnRo
        RAID 3414: allow explicit local server name.
        Also allow use of NetRemoteComputerSupports() for local computer.
        Minor UNICODE work.
    08-May-1992 JohnRo
        Use <prefix.h> equates.
    22-Sep-1992 JohnRo
        RAID 6739: Browser too slow when not logged into browsed domain.

--*/

// These must be included first:

#include <nt.h>         // IN, NULL, etc.
#include <windef.h>     // DWORD, LPDWORD, LPTSTR, TCHAR, etc.
#include <lmcons.h>     // NET_API_STATUS, NET_API_FUNCTION.

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG().
#include <icanon.h>     // NetpIsRemote(), NIRFLAG_ stuff, IS equates.
#include <lmerr.h>      // NERR_Success, etc.
#include <lmremutl.h>   // My prototype, SUPPORTS_ equates.
#include <names.h>      // NetpIsRemoteNameValid().
#include <netdebug.h>   // NetpAssert().
#include <netlib.h>     // NetpMemoryAllocate(), NetpMemoryFree().
#include <netlibnt.h>   // NetpRdrFsControlTree().
#include <ntddnfs.h>    // LMR_TRANSACTION, etc.
#include <prefix.h>     // PREFIX_ equates.
#include <tstring.h>    // STRCAT(), STRCPY(), STRLEN().
#include <lmuse.h>              // USE_IPC

NET_API_STATUS NET_API_FUNCTION
NetRemoteComputerSupports(
    IN LPCWSTR UncServerName OPTIONAL,   // Must start with "\\".
    IN DWORD OptionsWanted,             // Set SUPPORT_ bits wanted.
    OUT LPDWORD OptionsSupported        // Supported features, masked.
    )

#define SHARE_SUFFIX            (LPTSTR) TEXT("\\IPC$")
#define SHARE_SUFFIX_LEN        5

#ifdef UNICODE
#define LOCAL_FLAGS             ( SUPPORTS_REMOTE_ADMIN_PROTOCOL \
                                | SUPPORTS_RPC \
                                | SUPPORTS_SAM_PROTOCOL \
                                | SUPPORTS_UNICODE \
                                | SUPPORTS_LOCAL )
#else // not UNICODE
#define LOCAL_FLAGS             ( SUPPORTS_REMOTE_ADMIN_PROTOCOL \
                                | SUPPORTS_RPC \
                                | SUPPORTS_SAM_PROTOCOL \
                                | SUPPORTS_LOCAL )
#endif // not UNICODE

/*++

Routine Description:

    NetRemoteComputerSupports queries the redirector about a given remote
    system.  This is done to find out which optional features the remote
    system supports.  The features of interest are Unicode, RPC, and the
    Remote Admin Protocol.

    This will establish a connection if one doesn't already exist.

Arguments:

    UncServerName - Gives name of remote server to query.  This must begin
        with "\\".

    OptionsWanted - Gives a set of bits indicating which features the caller is
        interested in.  (At least one bit must be on.)

    OptionsSupported - Points to a DWORD which will be set with set of bits
        indicating which of the features selected by OptionsWanted are actually
        implemented on the computer with UncServerName.  (All other bits in this
        DWORD will be set to 0.)  The value of OptionsSupported is undefined if
        the return value is not NERR_Success.

Return Value:

    NET_API_STATUS.

--*/


{
    NET_API_STATUS Status;
    DWORD TempSupported = 0;

    IF_DEBUG(SUPPORTS) {
        NetpKdPrint(( PREFIX_NETAPI "NetRemoteComputerSupports: input mask is "
                FORMAT_HEX_DWORD ".\n", OptionsWanted));
    }

    // Error check what caller gave us.
    if (OptionsSupported == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (OptionsWanted == 0) {
        // Not what caller really intended, probably.
        return (ERROR_INVALID_PARAMETER);
    }

    //
    // Handle no name given (implies local computer).
    //
    if ( (UncServerName==NULL) || ((*UncServerName) == (TCHAR) '\0') ) {

        TempSupported = LOCAL_FLAGS & OptionsWanted;

    } else {

        TCHAR CanonServerName[MAX_PATH];
        DWORD LocalOrRemote;    // Will be set to ISLOCAL or ISREMOTE.

        //
        // Name was given.  Canonicalize it and check if it's remote.
        //
        Status = NetpIsRemote(
            (LPWSTR)UncServerName,      // input: uncanon name
            & LocalOrRemote,    // output: local or remote flag
            CanonServerName,    // output: canon name
            0);                 // flags: normal
        IF_DEBUG(SUPPORTS) {
            NetpKdPrint(( PREFIX_NETAPI
                    "NetRemoteComputerSupports: canon status is "
                    FORMAT_API_STATUS ", Lcl/rmt=" FORMAT_HEX_DWORD
                    ", canon buf is '" FORMAT_LPTSTR "'.\n",
                    Status, LocalOrRemote, CanonServerName));
        }
        if (Status != NERR_Success) {
            return (Status);
        }

        if (LocalOrRemote == ISLOCAL) {

            //
            // Explicit local name given.
            //
            TempSupported = LOCAL_FLAGS & OptionsWanted;

        } else {

            //
            // Explicit remote name given.
            //

            DWORD RedirCapabilities;
            PLMR_CONNECTION_INFO_2 RedirConnInfo;
            DWORD RedirConnInfoSize = sizeof(LMR_CONNECTION_INFO_2)
                    + ( (MAX_PATH+1 + MAX_PATH+1) * sizeof(TCHAR) );

            PLMR_REQUEST_PACKET RedirRequest;
            DWORD RedirRequestSize = sizeof(LMR_REQUEST_PACKET);

            LPTSTR TreeConnName;

            // Build tree connect name.
            TreeConnName =
                NetpMemoryAllocate(
                    (STRLEN(CanonServerName) + SHARE_SUFFIX_LEN + 1)
                    * sizeof(TCHAR) );
            if (TreeConnName == NULL) {
                return (ERROR_NOT_ENOUGH_MEMORY);
            }
            (void) STRCPY(TreeConnName, CanonServerName);
            (void) STRCAT(TreeConnName, SHARE_SUFFIX);
            NetpAssert(NetpIsRemoteNameValid(TreeConnName));

            // Alloc fsctl buffers.
            RedirConnInfo = NetpMemoryAllocate(RedirConnInfoSize);
            if (RedirConnInfo == NULL) {
                NetpMemoryFree(TreeConnName);
                return (ERROR_NOT_ENOUGH_MEMORY);
            }
            RedirRequest = NetpMemoryAllocate(RedirRequestSize);
            if (RedirRequest == NULL) {
                NetpMemoryFree(RedirConnInfo);
                NetpMemoryFree(TreeConnName);
                return (ERROR_NOT_ENOUGH_MEMORY);
            }

            RedirRequest->Level = 2;
            RedirRequest->Type = GetConnectionInfo;
            RedirRequest->Version = REQUEST_PACKET_VERSION;

#ifndef CDEBUG
            // Open tree conn (which will establish connection with the remote
            // server if one doesn't already exist) and do the FSCTL.
            Status = NetpRdrFsControlTree(
                    TreeConnName,                       // \\server\IPC$
                    NULL,                               // No transport.
                    USE_IPC,                            // Connection type
                    FSCTL_LMR_GET_CONNECTION_INFO,      // fsctl func code
                    NULL,                               // security descriptor
                    RedirRequest,                       // in buffer
                    RedirRequestSize,                   // in buffer size
                    RedirConnInfo,                      // out buffer
                    RedirConnInfoSize,                  // out buffer size
                    FALSE);                     // not a "null session" API.

            IF_DEBUG(SUPPORTS) {
                NetpKdPrint(( PREFIX_NETAPI
                        "NetRemoteComputerSupports: back from fsctl, "
                        "status is " FORMAT_API_STATUS ".\n", Status));
            }

            // Handle remote machine not found.
            if (Status != NERR_Success) {
                NetpMemoryFree(RedirConnInfo);
                NetpMemoryFree(RedirRequest);
                NetpMemoryFree(TreeConnName);
                return (Status);
            }
            RedirCapabilities = RedirConnInfo->Capabilities;

            IF_DEBUG(SUPPORTS) {
                NetpKdPrint(( PREFIX_NETAPI
                        "NetRemoteComputerSupports: redir mask is "
                        FORMAT_HEX_DWORD ".\n", RedirCapabilities));
            }

#else // def CDEBUG

            // BUGBUG: This is just value for testing RpcXlate.
            RedirCapabilities = CAPABILITY_REMOTE_ADMIN_PROTOCOL;

#endif // def CDEBUG

            NetpMemoryFree(RedirConnInfo);
            NetpMemoryFree(RedirRequest);
            NetpMemoryFree(TreeConnName);

            if (OptionsWanted & SUPPORTS_REMOTE_ADMIN_PROTOCOL) {
                if (RedirCapabilities & CAPABILITY_REMOTE_ADMIN_PROTOCOL) {
                    TempSupported |= SUPPORTS_REMOTE_ADMIN_PROTOCOL;
                }
            }
            if (OptionsWanted & SUPPORTS_RPC) {
                if (RedirCapabilities & CAPABILITY_RPC) {
                    TempSupported |= SUPPORTS_RPC;
                }
            }
            if (OptionsWanted & SUPPORTS_SAM_PROTOCOL) {
                if (RedirCapabilities & CAPABILITY_SAM_PROTOCOL) {
                    TempSupported |= SUPPORTS_SAM_PROTOCOL;
                }
            }
            if (OptionsWanted & SUPPORTS_UNICODE) {
                if (RedirCapabilities & CAPABILITY_UNICODE) {
                    TempSupported |= SUPPORTS_UNICODE;
                }
            }

        }
    }

    IF_DEBUG(SUPPORTS) {
        NetpKdPrint(( PREFIX_NETAPI "NetRemoteComputerSupports: output mask is "
                FORMAT_HEX_DWORD ".\n", TempSupported));
    }

    // Make sure we don't tell caller anything he/she didn't want to know.
    NetpAssert( (TempSupported & (~OptionsWanted)) == 0);

    // Tell caller what we know.
    *OptionsSupported = TempSupported;

    return (NERR_Success);

} // NetRemoteComputerSupports
