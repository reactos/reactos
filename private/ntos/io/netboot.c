/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netboot.c

Abstract:

    This module contains the code to initialize network boot.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Environment:

    Kernel mode, system initialization code

Revision History:

    Colin Watson (colinw) November 1997 Add CSC support

--*/

#include "iop.h"
#pragma hdrstop

#include <ntddip.h>
#include <nbtioctl.h>
#include <ntddnfs.h>
#include <ntddbrow.h>
#include <ntddtcp.h>
#include <setupblk.h>
#include <remboot.h>
#include <oscpkt.h>
#include <windef.h>
#include <tdiinfo.h>
#if defined(REMOTE_BOOT)
#include <ntddndis.h>
#include <ipsec.h>
#endif // defined(REMOTE_BOOT)

#ifndef NT
#define NT
#include <ipinfo.h>
#undef NT
#else
#include <ipinfo.h>
#endif

extern BOOLEAN ExpInTextModeSetup;

//
// TCP/IP definitions
//

#define DEFAULT_DEST                    0
#define DEFAULT_DEST_MASK               0
#define DEFAULT_METRIC                  1

NTSTATUS
IopWriteIpAddressToRegistry(
        HANDLE handle,
        PWCHAR regkey,
        PUCHAR value
        );

NTSTATUS
IopTCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
    IN TDIObjectID FAR       *ID,
    OUT void FAR             *Buffer,
    IN OUT DWORD FAR         *BufferSize,
    IN OUT BYTE FAR          *Context
    );

NTSTATUS
IopTCPSetInformationEx(
    IN HANDLE             TCPHandle,
    IN TDIObjectID FAR   *ID,
    IN void FAR          *Buffer,
    IN DWORD FAR          BufferSize
    );

NTSTATUS
IopSetDefaultGateway(
    IN ULONG GatewayAddress
    );

NTSTATUS
IopCacheNetbiosNameForIpAddress(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
IopAssignNetworkDriveLetter (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#if defined(REMOTE_BOOT)

#if DBG
BOOLEAN DebugReset = FALSE;
#endif

//
// The following variable is used by IoStartCscForTextmodeSetup.
//

BOOLEAN TextmodeSetupHasStartedCsc = FALSE;

//
// CSC definitions
//

typedef struct _FILETIME {
    ULONG dwLowDateTime;
    ULONG dwHighDateTime;
} FILETIME, *LPFILETIME;

typedef struct _WIN32_FIND_DATAA {
    ULONG dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    ULONG nFileSizeHigh;
    ULONG nFileSizeLow;
    ULONG dwReserved0;
    ULONG dwReserved1;
    CHAR   cFileName[ MAX_PATH ];
    CHAR   cAlternateFileName[ 14 ];
} WIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    WCHAR  cFileName[ MAX_PATH ];
    WCHAR  cAlternateFileName[ 14 ];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;


LIST_ENTRY DirectoryList;

HANDLE RdrHandle = NULL;
PKPROCESS RdrHandleProcess = NULL;

typedef enum _START_CSC {
    INIT_CSC,
    FLUSH_CSC
} START_CSC;

START_CSC StartCsc;

typedef enum _CSC_CONTROL {
    READ_CSC,
    SET_FLUSH_CSC,
    SET_FLUSHED_CSC
} CSC_CONTROL;

#include <shdcom.h>

//
// Structure for holding discovered directories until we can process them.
//

typedef struct _DIRENT {
    LIST_ENTRY Next;
    UNICODE_STRING Directory;
    PWCHAR LastToken;
    HSHADOW CSCHandle;
    WCHAR Name[1];
} DIRENT, *PDIRENT;

NTSTATUS
IopInitCsc(
    IN PUCHAR CscPath
    );

NTSTATUS
IopResetCsc(
    IN PUCHAR CscPath
    );

PWSTR
IopBreakPath(
    IN PWSTR* Path
    );

NTSTATUS
IopMarkRoot(
    IN PUNICODE_STRING FileName,
    OUT PHSHADOW RootHandle
    );

VOID
IopSetFlushCSC(
    IN CSC_CONTROL Command
    );

NTSTATUS
IopWalkDirectoryHelper (
    IN PUNICODE_STRING Directory,
    IN HSHADOW CSCHandle,
    IN PUCHAR Buffer,
    IN ULONG BufferSize
    );

NTSTATUS
IopWalkDirectoryTree (
    IN PUNICODE_STRING Directory,
    IN HSHADOW CSCHandle
    );

NTSTATUS
IopGetShadowExW(
    HSHADOW hDir,
    LPWIN32_FIND_DATAW lpFind32,
    LPSHADOWINFO lpSI
    );

NTSTATUS
IopAddHintFromInode(
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG *lpulHintPri,
    ULONG *lpulHintFlags,
    ULONG HintFlagsToSet
    );

NTSTATUS
IopSetShadowInfoW(
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG uStatus,
    ULONG uOp
    );

NTSTATUS
IopCreateShadowW(
    HSHADOW hDir,
    LPWIN32_FIND_DATAW lpFind32,
    ULONG uStatus,
    PHSHADOW lphShadow
    );

NTSTATUS
IopCopyServerAcl(
    HANDLE ParentHandle,
    PWCHAR FileName,
    HSHADOW ShadowHandle,
    BOOLEAN IsDirectory,
    PUCHAR Buffer,
    ULONG BufferSize
    );

NTSTATUS
IopGetShadowPathW(
    HSHADOW hShadow,
    PWCHAR lpLocalPath,
    LPCOPYPARAMSW lpCP
    );

VOID
IopEnableRemoteBootSecurity (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
IopGetHarddiskInfo(
    OUT PWSTR NetHDCSCPartition
    );

#endif // defined(REMOTE_BOOT)

//
// The following allows the I/O system's initialization routines to be
// paged out of memory.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IopAddRemoteBootValuesToRegistry)
#pragma alloc_text(INIT,IopStartNetworkForRemoteBoot)
#pragma alloc_text(INIT,IopStartTcpIpForRemoteBoot)
#pragma alloc_text(INIT,IopIsRemoteBootCard)
#pragma alloc_text(INIT,IopSetupRemoteBootCard)
#if defined(REMOTE_BOOT)
#pragma alloc_text(INIT,IopAssignNetworkDriveLetter)
#pragma alloc_text(INIT,IopInitCsc)
#pragma alloc_text(INIT,IopResetCsc)
#pragma alloc_text(INIT,IopBreakPath)
#pragma alloc_text(INIT,IopMarkRoot)
#pragma alloc_text(INIT,IopSetFlushCSC)
#pragma alloc_text(INIT,IopWalkDirectoryHelper)
#pragma alloc_text(INIT,IopWalkDirectoryTree)
#pragma alloc_text(INIT,IopGetShadowExW)
#pragma alloc_text(INIT,IopAddHintFromInode)
#pragma alloc_text(INIT,IopCreateShadowW)
#pragma alloc_text(INIT,IopCopyServerAcl)
#pragma alloc_text(INIT,IopGetShadowPathW)
#pragma alloc_text(INIT,IopGetHarddiskInfo)
#endif // defined(REMOTE_BOOT)
#endif


NTSTATUS
IopAddRemoteBootValuesToRegistry (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE handle;
    HANDLE serviceHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING string;
    CHAR addressA[16];
    WCHAR addressW[16];
    STRING addressStringA;
    UNICODE_STRING addressStringW;
    PUCHAR addressPointer;
    PUCHAR p;
    PUCHAR q;
    UCHAR ntName[128];
    WCHAR imagePath[128];
    STRING ansiString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING dnsNameString;
    UNICODE_STRING netbiosNameString;
    ULONG tmpValue;

    if (LoaderBlock->SetupLoaderBlock->ComputerName[0] != 0) {

        //
        // Convert the name to a Netbios name.
        //

        _wcsupr( LoaderBlock->SetupLoaderBlock->ComputerName );

        RtlInitUnicodeString( &dnsNameString, LoaderBlock->SetupLoaderBlock->ComputerName );

        status = RtlDnsHostNameToComputerName(
                     &netbiosNameString,
                     &dnsNameString,
                     TRUE);            // allocate netbiosNameString

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Failed RtlDnsHostNameToComputerName: %x\n", status ));
            goto cleanup;
        }

        //
        // Add a value for the computername.
        //

        RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName" );

        InitializeObjectAttributes(
            &objectAttributes,
            &string,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open ComputerName key: %x\n", status ));
            RtlFreeUnicodeString( &netbiosNameString );
            goto cleanup;
        }

        RtlInitUnicodeString( &string, L"ComputerName" );

        status = NtSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_SZ,
                    netbiosNameString.Buffer,
                    netbiosNameString.Length + sizeof(WCHAR)
                    );
        NtClose( handle );
        RtlFreeUnicodeString( &netbiosNameString );

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set ComputerName value: %x\n", status ));
            goto cleanup;
        }

        //
        // Add a value for the host name.
        //

        RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters" );

        InitializeObjectAttributes(
            &objectAttributes,
            &string,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open Tcpip\\Parameters key: %x\n", status ));
            goto cleanup;
        }

        _wcslwr( LoaderBlock->SetupLoaderBlock->ComputerName );

        RtlInitUnicodeString( &string, L"Hostname" );

        status = NtSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_SZ,
                    LoaderBlock->SetupLoaderBlock->ComputerName,
                    (wcslen(LoaderBlock->SetupLoaderBlock->ComputerName) + 1) * sizeof(WCHAR)
                    );
        NtClose( handle );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set Hostname value: %x\n", status ));
            goto cleanup;
        }
    }

    //
    //  If the UNC path to the system files is supplied then store it in the registry.
    //

    ASSERT( _stricmp(LoaderBlock->ArcBootDeviceName,"net(0)") == 0 );

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open Control key: %x\n", status ));
        goto skiproot;
    }

    p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
    if ( (p != NULL) && (*(p+1) == 0) ) {

        //
        // NtBootPathName ends with a backslash, so we need to back up
        // to the previous backslash.
        //

        q = p;
        *q = 0;
        p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
        *q = '\\';
    }
    if ( p == NULL ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: malformed NtBootPathName: %s\n", LoaderBlock->NtBootPathName ));
        NtClose( handle );
        goto skiproot;
    }
    *p = 0;                                 // terminate \server\share\images\machine

#if defined(REMOTE_BOOT)
    //
    // Store the server path in the shared user data area. Note that we need
    // to add an extra \ at the beginning of this path to make it a UNC name.
    //

    SharedUserData->RemoteBootServerPath[0] = L'\\';
    RtlInitAnsiString( &ansiString, LoaderBlock->NtBootPathName );
    unicodeString.MaximumLength = sizeof(SharedUserData->RemoteBootServerPath) - (2 * sizeof(WCHAR));
    unicodeString.Buffer = &SharedUserData->RemoteBootServerPath[1];
    RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, FALSE );
    SharedUserData->RemoteBootServerPath[1 + (unicodeString.Length/sizeof(WCHAR))] = 0;
#endif // defined(REMOTE_BOOT)

    strcpy( ntName, "\\Device\\LanmanRedirector");
    strcat( ntName, LoaderBlock->NtBootPathName );  // append \server\share\images\machine
    *p = '\\';

    RtlInitAnsiString( &ansiString, ntName );
    RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

    RtlInitUnicodeString( &string, L"RemoteBootRoot" );

    status = NtSetValueKey(
                handle,
                &string,
                0,
                REG_SZ,
                unicodeString.Buffer,
                unicodeString.Length + sizeof(WCHAR)
                );

    RtlFreeUnicodeString( &unicodeString );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set RemoteBootRoot value: %x\n", status ));
    }

    if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_TEXTMODE) != 0) {

        strcpy( ntName, "\\Device\\LanmanRedirector");
        strcat( ntName, LoaderBlock->SetupLoaderBlock->MachineDirectoryPath );
        RtlInitAnsiString( &ansiString, ntName );
        RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

        RtlInitUnicodeString( &string, L"RemoteBootMachineDirectory" );

        status = NtSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_SZ,
                    unicodeString.Buffer,
                    unicodeString.Length + sizeof(WCHAR)
                    );

        RtlFreeUnicodeString( &unicodeString );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set RemoteBootMachineDirectory value: %x\n", status ));
        }
    }

    NtClose( handle );

skiproot:

#if defined(REMOTE_BOOT)
    StartCsc = INIT_CSC;
    IopSetFlushCSC(READ_CSC);

    if ( (StartCsc != FLUSH_CSC) &&
         ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_REPIN) != 0) ) {
        StartCsc = FLUSH_CSC;
        IopSetFlushCSC(SET_FLUSH_CSC);
    }

#if 0
    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CSCSettings" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );

    if ( NT_SUCCESS(status) ) {

        PKEY_VALUE_PARTIAL_INFORMATION keyValue;
        UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(DWORD)];
        ULONG length;
        DWORD disabled;

#define ONE_BOOT 2

        RtlInitUnicodeString( &string, L"DisableAgent" );

        if ( (LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_DISABLE_CSC) == 0 ) {

            //
            // Disable CSC for this boot.
            //

            disabled = ONE_BOOT;
            status = NtSetValueKey(
                        handle,
                        &string,
                        0,
                        REG_DWORD,
                        &disabled,
                        sizeof(DWORD));

        } else {

            keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
            status = NtQueryValueKey(
                         handle,
                         &string,
                         KeyValuePartialInformation,
                         keyValue,
                         sizeof(buffer),
                         &length);
            if (NT_SUCCESS(status)) {
                disabled = *((DWORD *)(&keyValue->Data[0]));

                if (disabled == ONE_BOOT) {
                    //  Only disabled for last boot so re-enable now.
                    status = NtDeleteValueKey( handle, &string);
                    //  BUGBUG should we repin?
                }
            }
        }

        NtClose( handle );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to set CSCSettings: %x\n", status ));
            goto cleanup;
        }
    }
#endif
#endif // defined(REMOTE_BOOT)

    //
    // Add registry values for the IP address and subnet mask received
    // from DHCP. These are stored under the Tcpip service key and are
    // read by both Tcpip and Netbt. The adapter name used is the known
    // GUID for the NetbootCard.
    //

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\{54C7D140-09EF-11D1-B25A-F5FE627ED95E}" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open Tcpip\\Parameters\\Interfaces\\{54C7D140-09EF-11D1-B25A-F5FE627ED95E} key: %x\n", status ));
        goto cleanup;
    }

    status = IopWriteIpAddressToRegistry(handle,
                                         L"DhcpIPAddress",
                                         (PUCHAR)&(LoaderBlock->SetupLoaderBlock->IpAddress)
                                        );

    if ( !NT_SUCCESS(status)) {
        NtClose(handle);
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write DhcpIPAddress: %x\n", status ));
        goto cleanup;
    }

    status = IopWriteIpAddressToRegistry(handle,
                                         L"DhcpSubnetMask",
                                         (PUCHAR)&(LoaderBlock->SetupLoaderBlock->SubnetMask)
                                        );

    if ( !NT_SUCCESS(status)) {
        NtClose(handle);
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write DhcpSubnetMask: %x\n", status ));
        goto cleanup;
    }

    status = IopWriteIpAddressToRegistry(handle,
                                         L"DhcpDefaultGateway",
                                         (PUCHAR)&(LoaderBlock->SetupLoaderBlock->DefaultRouter)
                                        );

    NtClose(handle);

    if ( !NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write DhcpDefaultGateway: %x\n", status ));
        goto cleanup;
    }

    //
    // Create the service key for the netboot card. We need to have
    // the Type value there or the card won't be initialized.
    //

    status = IopOpenRegistryKey(&handle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetServices,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open CurrentControlSet\\Services: %x\n", status ));
        goto cleanup;
    }

    RtlInitUnicodeString(&string, LoaderBlock->SetupLoaderBlock->NetbootCardServiceName);

    InitializeObjectAttributes(&objectAttributes,
                               &string,
                               OBJ_CASE_INSENSITIVE,
                               handle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );

    status = ZwCreateKey(&serviceHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &tmpValue     // disposition
                         );

    ZwClose(handle);

    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to open/create netboot card service key: %x\n", status ));
        goto cleanup;
    }

    //
    // Store the image path.
    //

    PiWstrToUnicodeString(&string, L"ImagePath");
    wcscpy(imagePath, L"system32\\drivers\\");
    wcscat(imagePath, LoaderBlock->SetupLoaderBlock->NetbootCardDriverName);

    status = ZwSetValueKey(serviceHandle,
                           &string,
                           TITLE_INDEX_VALUE,
                           REG_SZ,
                           imagePath,
                           (wcslen(imagePath) + 1) * sizeof(WCHAR)
                           );

    if (!NT_SUCCESS(status)) {
        NtClose(serviceHandle);
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write ImagePath: %x\n", status ));
        goto cleanup;
    }

    //
    // Store the type.
    //

    PiWstrToUnicodeString(&string, L"Type");
    tmpValue = 1;

    ZwSetValueKey(serviceHandle,
                  &string,
                  TITLE_INDEX_VALUE,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );

    NtClose(serviceHandle);

    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAddRemoteBootValuesToRegistry: Unable to write Type: %x\n", status ));
    }

cleanup:

    return status;
}

NTSTATUS
IopStartNetworkForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    NTSTATUS status;
    HANDLE dgHandle;
    HANDLE keyHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING string;
    UNICODE_STRING computerName;
    UNICODE_STRING domainName;
    PUCHAR buffer;
    ULONG bufferLength;
    PLMR_REQUEST_PACKET rrp;
    PLMDR_REQUEST_PACKET drrp;
    WKSTA_INFO_502 wkstaConfig;
    WKSTA_TRANSPORT_INFO_0 wkstaTransportInfo;
    LARGE_INTEGER interval;
    ULONG length;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    BOOLEAN startDatagramReceiver;
    ULONG enumerateAttempts;
#if defined(REMOTE_BOOT)
    PWSTR NetHDCSCPartition;
    BOOLEAN leaveRdrHandleOpen;
    BOOLEAN pinNetDriver;
#else
    HANDLE RdrHandle;
#endif // defined(REMOTE_BOOT)

    //
    // Initialize for cleanup.
    //

    buffer = NULL;
    computerName.Buffer = NULL;
    domainName.Buffer = NULL;
    dgHandle = NULL;
    RdrHandle = NULL;
#if defined(REMOTE_BOOT)
    NetHDCSCPartition = NULL;
    leaveRdrHandleOpen = FALSE;
    pinNetDriver = FALSE;
#endif // defined(REMOTE_BOOT)

    //
    // Allocate a temporary buffer. It has to be big enough for all the
    // various FSCTLs we send down.
    //

    bufferLength = max(sizeof(LMR_REQUEST_PACKET) + (MAX_PATH + 1) * sizeof(WCHAR) +
                                                 (DNLEN + 1) * sizeof(WCHAR),
                       max(sizeof(LMDR_REQUEST_PACKET),
                           FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + MAX_PATH));
    bufferLength = max(bufferLength, sizeof(LMMR_RI_INITIALIZE_SECRET));

#if defined(REMOTE_BOOT)
    NetHDCSCPartition = ExAllocatePoolWithTag(
                            NonPagedPool,
                            (80 * sizeof(WCHAR)) + bufferLength,
                            'bRoI'
                            );
    if (NetHDCSCPartition == NULL) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to allocate buffer\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    buffer = (PUCHAR)(NetHDCSCPartition + 80);
#else
    buffer = ExAllocatePoolWithTag( NonPagedPool, bufferLength, 'bRoI' );
    if (buffer == NULL) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to allocate buffer\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
#endif // defined(REMOTE_BOOT)

    rrp = (PLMR_REQUEST_PACKET)buffer;
    drrp = (PLMDR_REQUEST_PACKET)buffer;

    //
    // Open the redirector and the datagram receiver.
    //

    RtlInitUnicodeString( &string, L"\\Device\\LanmanRedirector" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                &RdrHandle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open redirector: %x\n", status ));
        goto cleanup;
    }

#if defined(REMOTE_BOOT)
    RdrHandleProcess = &IoGetCurrentProcess()->Pcb;
#endif // defined(REMOTE_BOOT)

    RtlInitUnicodeString( &string, DD_BROWSER_DEVICE_NAME_U );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                &dgHandle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open datagram receiver: %x\n", status ));
        goto cleanup;
    }

    //
    // If the setup loader block has a disk secret in it provided by the
    // loader, pass this down to the redirector (do this before sending
    // the LMR_START, since that uses this information).
    //

#if defined(REMOTE_BOOT)
    if (LoaderBlock->SetupLoaderBlock->NetBootSecret)
#endif // defined(REMOTE_BOOT)
    {
        PLMMR_RI_INITIALIZE_SECRET RbInit = (PLMMR_RI_INITIALIZE_SECRET)buffer;

        ASSERT(LoaderBlock->SetupLoaderBlock->NetBootSecret != NULL);
        RtlCopyMemory(
            &RbInit->Secret,
            LoaderBlock->SetupLoaderBlock->NetBootSecret,
            sizeof(RI_SECRET));
#if defined(REMOTE_BOOT)
        RbInit->UsePassword2 = LoaderBlock->SetupLoaderBlock->NetBootUsePassword2;
#endif // defined(REMOTE_BOOT)

        status = NtFsControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_LMMR_RI_INITIALIZE_SECRET,
                    buffer,
                    sizeof(LMMR_RI_INITIALIZE_SECRET),
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(RB initialize) redirector: %x\n", status ));
            goto cleanup;
        }
    }

    //
    // Read the computer name and domain name from the registry so we
    // can give them to the datagram receiver. During textmode setup
    // the domain name will not be there, so we won't start the datagram
    // receiver, which is fine.
    //
    // BUGBUG: Figure out the correct location to read the domain name
    // from -- this one is a special hack in winnt.sif just for this.
    //

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &keyHandle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open ComputerName key: %x\n", status ));
        goto cleanup;
    }

    RtlInitUnicodeString( &string, L"ComputerName" );

    keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    RtlZeroMemory(buffer, bufferLength);

    status = NtQueryValueKey(
                 keyHandle,
                 &string,
                 KeyValuePartialInformation,
                 keyValue,
                 bufferLength,
                 &length);

    NtClose( keyHandle );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to query ComputerName value: %x\n", status ));
        goto cleanup;
    }

    if ( !RtlCreateUnicodeString(&computerName, (PWSTR)keyValue->Data) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to create ComputerName string\n" ));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    domainName.Length = 0;

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\DomainName" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenKey( &keyHandle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open DomainName key: %x\n", status ));
        startDatagramReceiver = FALSE;
    } else {

        RtlInitUnicodeString( &string, L"DomainName" );

        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        RtlZeroMemory(buffer, bufferLength);

        status = NtQueryValueKey(
                     keyHandle,
                     &string,
                     KeyValuePartialInformation,
                     keyValue,
                     bufferLength,
                     &length);

        NtClose( keyHandle );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to query Domain value: %x\n", status ));
            startDatagramReceiver = FALSE;
        } else {
            if ( !RtlCreateUnicodeString(&domainName, (PWSTR)keyValue->Data) ) {
                KdPrint(( "IopStartNetworkForRemoteBoot: Unable to create DomainName string\n" ));
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            startDatagramReceiver = TRUE;
        }
    }

    //
    // Tell the redir to start.
    //

    rrp->Type = ConfigInformation;
    rrp->Version = REQUEST_PACKET_VERSION;

    rrp->Parameters.Start.RedirectorNameLength = computerName.Length;
    RtlCopyMemory(rrp->Parameters.Start.RedirectorName,
                  computerName.Buffer,
                  computerName.Length);

    rrp->Parameters.Start.DomainNameLength = domainName.Length;
    RtlCopyMemory(((PUCHAR)rrp->Parameters.Start.RedirectorName) + computerName.Length,
                  domainName.Buffer,
                  domainName.Length);

    RtlFreeUnicodeString(&computerName);
    RtlFreeUnicodeString(&domainName);

    wkstaConfig.wki502_char_wait = 3600;
    wkstaConfig.wki502_maximum_collection_count = 16;
    wkstaConfig.wki502_collection_time = 250;
    wkstaConfig.wki502_keep_conn = 600;
    wkstaConfig.wki502_max_cmds = 5;
    wkstaConfig.wki502_sess_timeout = 45;
    wkstaConfig.wki502_siz_char_buf = 512;
    wkstaConfig.wki502_max_threads = 17;
    wkstaConfig.wki502_lock_quota = 6144;
    wkstaConfig.wki502_lock_increment = 10;
    wkstaConfig.wki502_lock_maximum = 500;
    wkstaConfig.wki502_pipe_increment = 10;
    wkstaConfig.wki502_pipe_maximum = 500;
    wkstaConfig.wki502_cache_file_timeout = 40;
    wkstaConfig.wki502_dormant_file_limit = 45;
    wkstaConfig.wki502_read_ahead_throughput = MAXULONG;
    wkstaConfig.wki502_num_mailslot_buffers = 3;
    wkstaConfig.wki502_num_srv_announce_buffers = 20;
    wkstaConfig.wki502_max_illegal_datagram_events = 5;
    wkstaConfig.wki502_illegal_datagram_event_reset_frequency = 60;
    wkstaConfig.wki502_log_election_packets = FALSE;
    wkstaConfig.wki502_use_opportunistic_locking = TRUE;
    wkstaConfig.wki502_use_unlock_behind = TRUE;
    wkstaConfig.wki502_use_close_behind = TRUE;
    wkstaConfig.wki502_buf_named_pipes = TRUE;
    wkstaConfig.wki502_use_lock_read_unlock = TRUE;
    wkstaConfig.wki502_utilize_nt_caching = TRUE;
    wkstaConfig.wki502_use_raw_read = TRUE;
    wkstaConfig.wki502_use_raw_write = TRUE;
    wkstaConfig.wki502_use_write_raw_data = TRUE;
    wkstaConfig.wki502_use_encryption = TRUE;
    wkstaConfig.wki502_buf_files_deny_write = TRUE;
    wkstaConfig.wki502_buf_read_only_files = TRUE;
    wkstaConfig.wki502_force_core_create_mode = TRUE;
    wkstaConfig.wki502_use_512_byte_max_transfer = FALSE;

    status = NtFsControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_LMR_START | 0x80000000,
                rrp,
                sizeof(LMR_REQUEST_PACKET) +
                    rrp->Parameters.Start.RedirectorNameLength +
                    rrp->Parameters.Start.DomainNameLength,
                &wkstaConfig,
                sizeof(wkstaConfig)
                );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(start) redirector: %x\n", status ));
        goto cleanup;
    }

    if (startDatagramReceiver) {

        //
        // Tell the datagram receiver to start.
        //

        drrp->Version = LMDR_REQUEST_PACKET_VERSION;

        drrp->Parameters.Start.NumberOfMailslotBuffers = 16;
        drrp->Parameters.Start.NumberOfServerAnnounceBuffers = 20;
        drrp->Parameters.Start.IllegalDatagramThreshold = 5;
        drrp->Parameters.Start.EventLogResetFrequency = 60;
        drrp->Parameters.Start.LogElectionPackets = FALSE;

        drrp->Parameters.Start.IsLanManNt = FALSE;

        status = NtDeviceIoControlFile(
                    dgHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    IOCTL_LMDR_START,
                    drrp,
                    sizeof(LMDR_REQUEST_PACKET),
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        NtClose( dgHandle );
        dgHandle = NULL;

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to IOCTL(start) datagram receiver: %x\n", status ));
            goto cleanup;
        }

    } else {

        NtClose( dgHandle );
        dgHandle = NULL;

        //
        // Tell the redir to bind to the transports.
        //
        // Note: In the current redirector implementation, this call just
        // tells the redirector to register for TDI PnP notifications.
        // Starting the datagram receiver also does this, so we only issue
        // this FSCTL if we're not starting the datagram receiver.
        //

        status = NtFsControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_LMR_BIND_TO_TRANSPORT | 0x80000000,
                    NULL,
                    0,
                    NULL,
                    0
                    );

        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(bind) redirector: %x\n", status ));
            goto cleanup;
        }
    }

#if defined(REMOTE_BOOT)
    if (((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_TEXTMODE) == 0) &&
        ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_FORMAT_NEEDED) == 0)) {

        //
        // Get the path to the boot partition on the disk for CSC and redirection
        // Note: On failure, this defaults to \Device\Harddisk0\Partition1
        //

        IopGetHarddiskInfo(NetHDCSCPartition);

        //
        // Tell the redirector to initialize remote boot redirection (back
        // to the local disk). Note that we only do this if we're NOT doing
        // textmode setup. During textmode, we let Setup do this so that it
        // can do so AFTER it has reformatted the local disk.
        //

        status = NtFsControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_LMR_START_RBR,
                    NetHDCSCPartition,
                    wcslen(NetHDCSCPartition) * sizeof(WCHAR),
                    NULL,
                    0
                    );

        if (NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(RBR) redirector: %x\n", status ));
        }
    }

    if ( (LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_DISCONNECTED) == 0 )
#endif // defined(REMOTE_BOOT)
    {

        //
        // Loop until the redirector is bound to the transport. It may take a
        // while because TDI defers notification of binding to a worker thread.
        // We start with a half a second wait and double it each time, trying
        // five times total.
        //

        interval.QuadPart = -500 * 1000 * 10;    // 1/2 second, relative
        enumerateAttempts = 0;

        while (TRUE) {

            KeDelayExecutionThread(KernelMode, FALSE, &interval);

            RtlZeroMemory(rrp, sizeof(LMR_REQUEST_PACKET));

            rrp->Type = EnumerateTransports;
            rrp->Version = REQUEST_PACKET_VERSION;

            status = NtFsControlFile(
                        RdrHandle,
                        NULL,
                        NULL,
                        NULL,
                        &ioStatusBlock,
                        FSCTL_LMR_ENUMERATE_TRANSPORTS,
                        rrp,
                        sizeof(LMR_REQUEST_PACKET),
                        &wkstaTransportInfo,
                        sizeof(wkstaTransportInfo)
                        );

            if ( NT_SUCCESS(status) ) {
                status = ioStatusBlock.Status;
            }
            if ( !NT_SUCCESS(status) ) {
                //KdPrint(( "IopStartNetworkForRemoteBoot: Unable to FSCTL(enumerate) redirector: %x\n", status ));
            } else if (rrp->Parameters.Get.TotalBytesNeeded == 0) {
                //KdPrint(( "IopStartNetworkForRemoteBoot: FSCTL(enumerate) returned 0 entries\n" ));
            } else {
                break;
            }

            ++enumerateAttempts;

            if (enumerateAttempts == 5) {
                KdPrint(( "IopStartNetworkForRemoteBoot: Redirector didn't start\n" ));
                status = STATUS_REDIRECTOR_NOT_STARTED;
                goto cleanup;
            }

            interval.QuadPart *= 2;

        }
    }

    //
    // Prime the transport.
    //

#if defined(REMOTE_BOOT)
    IopEnableRemoteBootSecurity(LoaderBlock);
#endif // defined(REMOTE_BOOT)
    IopSetDefaultGateway(LoaderBlock->SetupLoaderBlock->DefaultRouter);
    IopCacheNetbiosNameForIpAddress(LoaderBlock);

#if defined(REMOTE_BOOT)
    //
    // CSC needs to be initialized after binding to the transport because ResetCSC
    // (if needed) will try to enumerate the files and directories on the server.
    //

    if (((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IS_TEXTMODE) == 0) &&
        ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_FORMAT_NEEDED) == 0)
#if 0
        && ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_DISABLE_CSC) != 0)
#endif
        ) {

        wcstombs(buffer, NetHDCSCPartition, wcslen(NetHDCSCPartition) + 1);
        strcat(buffer, REMOTE_BOOT_IMIRROR_PATH_A REMOTE_BOOT_CSC_SUBDIR_A);

        status = IopInitCsc( buffer );

        //
        //  If we are connected and either we are part way through a reset of
        //  the csc or the init failed then reset the csc.
        //

        if ( NT_SUCCESS(status) ) {

            if (StartCsc == FLUSH_CSC ) {

                if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_DISCONNECTED) == 0) {

                    //
                    // CSC may have lost the pin information.
                    //

                    status = IopResetCsc( buffer );

                    if ( !NT_SUCCESS(status) ) {
                        KdPrint(("IopStartNetworkForRemoteBoot: reset of Csc failed %x\n", status));
                    }
                } else {
                    IoCscInitializationFailed = TRUE;
                }
            } else if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_PIN_NET_DRIVER) &&
                       (LoaderBlock->SetupLoaderBlock->NetbootCardDriverName[0] != L'\0')) {

                //
                // if we are connected and we're not repinning all files and
                // we have a new net card driver to pin, then pin it below
                // after we've created called IopAssignNetworkDriveLetter
                //

                pinNetDriver = TRUE;
            }
        }

        if ( !NT_SUCCESS(status) ) {
            KdPrint(("IopStartNetworkForRemoteBoot: initialization of Csc failed %x\n", status));
            IoCscInitializationFailed = TRUE;
            SharedUserData->SystemFlags |= SYSTEM_FLAG_DISKLESS_CLIENT;
        }

    } else {
        IoCscInitializationFailed = TRUE;
        SharedUserData->SystemFlags |= SYSTEM_FLAG_DISKLESS_CLIENT;
    }
#endif // defined(REMOTE_BOOT)

    IopAssignNetworkDriveLetter(LoaderBlock);

#if defined(REMOTE_BOOT)
    if (pinNetDriver) {

        //
        //  Pin the new net card driver simply by opening it.  if it
        //  fails, it's not fatal, as we'll eventually pin it since
        //  the directory it's in is marked as system/inherit.
        //

        HANDLE driverHandle = NULL;
        PWCHAR fullDriverName;

        fullDriverName = (PWCHAR) ExAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof( L"\\SystemRoot\\System32\\Drivers\\" ) +
                            sizeof( LoaderBlock->SetupLoaderBlock->NetbootCardDriverName ),
                            'bRoI'
                            );

        if (fullDriverName != NULL) {

            wcscpy(fullDriverName, L"\\SystemRoot\\System32\\Drivers\\");
            wcscat(fullDriverName, LoaderBlock->SetupLoaderBlock->NetbootCardDriverName);

            RtlInitUnicodeString( &string, fullDriverName );

            InitializeObjectAttributes(
                &objectAttributes,
                &string,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

            status = NtCreateFile(
                        &driverHandle,
                        GENERIC_READ,
                        &objectAttributes,
                        &ioStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ,
                        FILE_OPEN,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0
                        );
            if ( !NT_SUCCESS(status) ) {

                //
                //  this is not a fatal error, the redir should pin it eventually
                //

                KdPrint(( "IopStartNetworkForRemoteBoot: Unable to open new net driver: 0x%x\n", status ));
            }
            if (driverHandle != NULL) {
                NtClose( driverHandle );
            }

            ExFreePool( fullDriverName );

        } else {

            //
            //  this is not a fatal error, the redir should pin it eventually
            //

            KdPrint(( "IopStartNetworkForRemoteBoot: Unable to allocate buffer to pin netcard driver\n" ));
        }
    }

    leaveRdrHandleOpen = TRUE;
#endif // defined(REMOTE_BOOT)

cleanup:

    RtlFreeUnicodeString( &computerName );
    RtlFreeUnicodeString( &domainName );
#if defined(REMOTE_BOOT)
    if ( NetHDCSCPartition != NULL ) {
        ExFreePool( NetHDCSCPartition );
    }
#else
    if ( buffer != NULL ) {
        ExFreePool( buffer );
    }
#endif // defined(REMOTE_BOOT)

    if ( dgHandle != NULL ) {
        NtClose( dgHandle );
    }

#if defined(REMOTE_BOOT)
    //
    // If requested, exit with RdrHandle still set so that we can close CSC quickly.
    //

    if ( !leaveRdrHandleOpen && (RdrHandle != NULL) ) {
        NtClose( RdrHandle );
        RdrHandle = NULL;
    }
#endif // defined(REMOTE_BOOT)

    return status;
}


#if defined(REMOTE_BOOT)
VOID
IopShutdownCsc(
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    SHADOWINFO shadowInfo;

    if ( RdrHandle != NULL ) {

        KeAttachProcess( RdrHandleProcess );

        RtlZeroMemory( &shadowInfo, sizeof(shadowInfo) );
        shadowInfo.uStatus = 0x0001; // SHADOW_SWITCH_SHADOWING
        shadowInfo.uOp = 1; // SHADOW_SWITCH_OFF

        status = NtDeviceIoControlFile(
                    RdrHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    IOCTL_SWITCHES,
                    &shadowInfo,
                    0,
                    NULL,
                    0
                    );
#if DBG
        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopShutdownCsc: %x\n", status ));
        }
#endif
        NtClose( RdrHandle );
        RdrHandle = NULL;

        KeDetachProcess();
    }

    return;
}
#endif // defined(REMOTE_BOOT)


VOID
IopAssignNetworkDriveLetter (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    PUCHAR p;
    PUCHAR q;
    UCHAR ntName[128];
    STRING ansiString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING unicodeString2;
    NTSTATUS status;

    //
    // Create the symbolic link of X: to the redirector. We do this
    // after the redirector has loaded, but before AssignDriveLetters
    // is called the first time in textmode setup (once that has
    // happened, the drive letters will stick).
    //
    // Note that we use X: for the textmode setup phase of a remote
    // installation. But for a true remote boot, we use C:.
    //

    if ((LoaderBlock->SetupLoaderBlock->Flags & (SETUPBLK_FLAGS_REMOTE_INSTALL |
                                                 SETUPBLK_FLAGS_SYSPREP_INSTALL)) != 0) {
        RtlInitUnicodeString( &unicodeString2, L"\\DosDevices\\X:");
    } else {
        RtlInitUnicodeString( &unicodeString2, L"\\DosDevices\\C:");
    }

    //
    // If this is a remote boot setup boot, NtBootPathName is of the
    // form \<server>\<share>\setup\<install-directory>\<platform>.
    // We want the root of the X: drive to be the root of the install
    // directory.
    //
    // If this is a normal remote boot, NtBootPathName is of the form
    // \<server>\<share>\images\<machine>\winnt. We want the root of
    // the X: drive to be the root of the machine directory.
    //
    // Thus in either case, we need to remove the last element of the
    // path.
    //

    p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
    if ( (p != NULL) && (*(p+1) == 0) ) {

        //
        // NtBootPathName ends with a backslash, so we need to back up
        // to the previous backslash.
        //

        q = p;
        *q = 0;
        p = strrchr( LoaderBlock->NtBootPathName, '\\' );   // find last separator
        *q = '\\';
    }
    if ( p == NULL ) {
        KdPrint(( "IopAssignNetworkDriveLetter: malformed NtBootPathName: %s\n", LoaderBlock->NtBootPathName ));
        KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );
    }
    *p = 0;                                 // terminate \server\share\images\machine

    strcpy( ntName, "\\Device\\LanmanRedirector");
    strcat( ntName, LoaderBlock->NtBootPathName );  // append \server\share\images\machine

    RtlInitAnsiString( &ansiString, ntName );
    RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );

    status = IoCreateSymbolicLink(&unicodeString2, &unicodeString);
    if (!NT_SUCCESS(status)) {
        KdPrint(( "IopAssignNetworkDriveLetter: unable to create DOS link for redirected boot drive: %x\n", status ));
        KeBugCheck( ASSIGN_DRIVE_LETTERS_FAILED );
    }
    // DbgPrint("IopAssignNetworkDriveLetter: assigned %wZ to %wZ\n", &unicodeString2, &unicodeString);

    RtlFreeUnicodeString( &unicodeString );

    *p = '\\';                              // restore string

    return;
}


#if defined(REMOTE_BOOT)
VOID
IopEnableRemoteBootSecurity (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    UNICODE_STRING IpsecString;
    NTSTATUS status;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UCHAR policyBuffer[sizeof(IPSEC_SET_POLICY) + sizeof(IPSEC_POLICY_INFO)];
    PIPSEC_SET_POLICY setPolicy = (PIPSEC_SET_POLICY)policyBuffer;
    IPSEC_FILTER outboundFilter;
    IPSEC_FILTER inboundFilter;
    IPSEC_SET_SPI setSpi;
    UCHAR saBuffer[sizeof(IPSEC_ADD_UPDATE_SA) + (6 * sizeof(ULONG))];
    PIPSEC_ADD_UPDATE_SA addUpdateSa;


    if ((LoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IPSEC_ENABLED) == 0) {
        return;
    }

    RtlInitUnicodeString( &IpsecString, DD_IPSEC_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &IpsecString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                &handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopEnableRemoteBootSecurity: Unable to open IPSEC: %x\n", status ));
        return;
    }

    //
    // Set the policy. We need two filters, one for outbound and
    // one for inbound.
    //

    RtlZeroMemory(&outboundFilter, sizeof(IPSEC_FILTER));
    RtlZeroMemory(&inboundFilter, sizeof(IPSEC_FILTER));

    outboundFilter.SrcAddr = LoaderBlock->SetupLoaderBlock->IpAddress;
    outboundFilter.SrcMask = 0xffffffff;
    outboundFilter.DestAddr = LoaderBlock->SetupLoaderBlock->ServerIpAddress;
    outboundFilter.DestMask = 0xffffffff;
    // outboundFilter.DestPort = 0x8B;  // netbios session port
    outboundFilter.Protocol = 0x6;   // TCP

    inboundFilter.SrcAddr = LoaderBlock->SetupLoaderBlock->ServerIpAddress;
    inboundFilter.SrcMask = 0xffffffff;
    // inboundFilter.SrcPort = 0x8B;  // netbios session port
    inboundFilter.DestAddr = LoaderBlock->SetupLoaderBlock->IpAddress;
    inboundFilter.DestMask = 0xffffffff;
    inboundFilter.Protocol = 0x6;   // TCP

    RtlZeroMemory(setPolicy, sizeof(policyBuffer));

    setPolicy->NumEntries = 2;
    setPolicy->pInfo[0].Index = 1;
    setPolicy->pInfo[0].AssociatedFilter = outboundFilter;
    setPolicy->pInfo[1].Index = 2;
    setPolicy->pInfo[1].AssociatedFilter = inboundFilter;

    status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_IPSEC_SET_POLICY,
                setPolicy,
                sizeof(policyBuffer),
                NULL,
                0
                );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopEnableRemoteBootSecurity: Unable to IOCTL_IPSEC_SET_POLICY: %x\n", status ));
        return;
    }

    //
    // Now set the SPI we made up in the loader.
    //

    setSpi.Context = 0;
    setSpi.InstantiatedFilter = inboundFilter;
    setSpi.SPI = LoaderBlock->SetupLoaderBlock->IpsecInboundSpi;

    status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_IPSEC_SET_SPI,
                &setSpi,
                sizeof(IPSEC_SET_SPI),
                &setSpi,
                sizeof(IPSEC_SET_SPI)
                );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopEnableRemoteBootSecurity: Unable to IOCTL_IPSEC_SET_SPI: %x\n", status ));
        return;
    }

    //
    // Set up the security association for the outbound
    // connection.
    //

    addUpdateSa = (PIPSEC_ADD_UPDATE_SA)saBuffer;
    memset(addUpdateSa, 0, sizeof(saBuffer));

    addUpdateSa->SAInfo.Context = setSpi.Context;
    addUpdateSa->SAInfo.NumSAs = 1;
    addUpdateSa->SAInfo.InstantiatedFilter = outboundFilter;
    addUpdateSa->SAInfo.SecAssoc[0].Operation = Encrypt;
    addUpdateSa->SAInfo.SecAssoc[0].SPI = LoaderBlock->SetupLoaderBlock->IpsecOutboundSpi;
    addUpdateSa->SAInfo.SecAssoc[0].IntegrityAlgo.algoIdentifier = IPSEC_AH_MD5;
    addUpdateSa->SAInfo.SecAssoc[0].IntegrityAlgo.algoKeylen = 4 * sizeof(ULONG);
    addUpdateSa->SAInfo.SecAssoc[0].ConfAlgo.algoIdentifier = IPSEC_ESP_DES;
    addUpdateSa->SAInfo.SecAssoc[0].ConfAlgo.algoKeylen = 2 * sizeof(ULONG);

    addUpdateSa->SAInfo.KeyLen = 6 * sizeof(ULONG);
    memcpy(addUpdateSa->SAInfo.KeyMat, &LoaderBlock->SetupLoaderBlock->IpsecSessionKey, sizeof(ULONG));
    memcpy(addUpdateSa->SAInfo.KeyMat+sizeof(ULONG), &LoaderBlock->SetupLoaderBlock->IpsecSessionKey, sizeof(ULONG));
    memcpy(addUpdateSa->SAInfo.KeyMat+(2*sizeof(ULONG)), &LoaderBlock->SetupLoaderBlock->IpsecSessionKey, sizeof(ULONG));
    memcpy(addUpdateSa->SAInfo.KeyMat+(3*sizeof(ULONG)), &LoaderBlock->SetupLoaderBlock->IpsecSessionKey, sizeof(ULONG));
    memcpy(addUpdateSa->SAInfo.KeyMat+(4*sizeof(ULONG)), &LoaderBlock->SetupLoaderBlock->IpsecSessionKey, sizeof(ULONG));
    memcpy(addUpdateSa->SAInfo.KeyMat+(5*sizeof(ULONG)), &LoaderBlock->SetupLoaderBlock->IpsecSessionKey, sizeof(ULONG));

    status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_IPSEC_ADD_SA,
                addUpdateSa,
                FIELD_OFFSET(IPSEC_ADD_UPDATE_SA, SAInfo.KeyMat[0]) + addUpdateSa->SAInfo.KeyLen,
                NULL,
                0
                );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopEnableRemoteBootSecurity: Unable to IOCTL_IPSEC_ADD_SA: %x\n", status ));
        return;
    }

    //
    // Set up the security association for the inbound connection.
    // If our Operation is "None", IPSEC does this for us.
    //

    if (addUpdateSa->SAInfo.SecAssoc[0].Operation != None) {

        addUpdateSa->SAInfo.SecAssoc[0].SPI = LoaderBlock->SetupLoaderBlock->IpsecInboundSpi;
        addUpdateSa->SAInfo.InstantiatedFilter = inboundFilter;

        status = NtDeviceIoControlFile(
                    handle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    IOCTL_IPSEC_UPDATE_SA,
                    addUpdateSa,
                    FIELD_OFFSET(IPSEC_ADD_UPDATE_SA, SAInfo.KeyMat[0]) + addUpdateSa->SAInfo.KeyLen,
                    NULL,
                    0
                    );

        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopEnableRemoteBootSecurity: Unable to IOCTL_IPSEC_UPDATE_SA: %x\n", status ));
            return;
        }
    }

    NtClose( handle );

}
#endif // defined(REMOTE_BOOT)

NTSTATUS
IopStartTcpIpForRemoteBoot (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    UNICODE_STRING IpString;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    IP_SET_ADDRESS_REQUEST IpRequest;

    RtlInitUnicodeString( &IpString, DD_IP_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &IpString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    IpRequest.Context = (USHORT)2;
    IpRequest.Address = LoaderBlock->SetupLoaderBlock->IpAddress;
    IpRequest.SubnetMask = LoaderBlock->SetupLoaderBlock->SubnetMask;

    status = NtCreateFile(
                &handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartTcpIpForRemoteBoot: Unable to open IP: %x\n", status ));
        goto cleanup;
    }

    status = NtDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_IP_SET_ADDRESS_DUP,
                &IpRequest,
                sizeof(IP_SET_ADDRESS_REQUEST),
                NULL,
                0
                );

    NtClose( handle );

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopStartTcpIpForRemoteBoot: Unable to IOCTL IP: %x\n", status ));
        goto cleanup;
    }

cleanup:

    return status;
}

BOOLEAN
IopIsRemoteBootCard(
    IN PDEVICE_NODE DeviceNode,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PWCHAR HwIds
    )

/*++

Routine Description:

    This function determines if the card described by the hwIds is the
    remote boot network card. It checks against the hardware ID for the
    card that is stored in the setup loader block.

    THIS ASSUMES THAT IOREMOTEBOOTCLIENT IS TRUE AND THAT LOADERBLOCK
    IS VALID.

Arguments:

    DeviceNode - Device node for the card in question.

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

    HwIds - The hardware IDs for the device in question.

Return Value:

    TRUE or FALSE.

--*/

{
    PSETUP_LOADER_BLOCK setupLoaderBlock;
    PWCHAR curHwId;

    //
    // setupLoaderBlock will always be non-NULL if we are
    // remote booting, even if we are not in setup.
    //

    setupLoaderBlock = LoaderBlock->SetupLoaderBlock;

    //
    // Scan through the HwIds for a match.
    //

    curHwId = HwIds;

    while (*curHwId != L'\0') {
        if (wcscmp(curHwId, setupLoaderBlock->NetbootCardHardwareId) == 0) {

            ULONG BusNumber, SlotNumber;

            BusNumber = (ULONG)((((PNET_CARD_INFO)setupLoaderBlock->NetbootCardInfo)->pci.BusDevFunc) >> 8);
            SlotNumber = (ULONG)(((((PNET_CARD_INFO)setupLoaderBlock->NetbootCardInfo)->pci.BusDevFunc) & 0xf8) >> 3);
            
            KdPrint(("IopIsRemoteBootCard: FOUND %ws\n", setupLoaderBlock->NetbootCardHardwareId));
            if ((DeviceNode->ResourceRequirements->BusNumber != BusNumber) ||
                (DeviceNode->ResourceRequirements->SlotNumber != SlotNumber)) {
                KdPrint(("IopIsRemoteBootCard: ignoring non-matching card:\n"));
                KdPrint(("  devnode bus %d, busdevfunc bus %d\n",
                    DeviceNode->ResourceRequirements->BusNumber,
                    BusNumber));
                KdPrint(("  devnode slot %d, busdevfunc slot %d\n",
                    DeviceNode->ResourceRequirements->SlotNumber,
                    SlotNumber));
                return FALSE;
            } else {
                return TRUE;
            }
        }
        curHwId += (wcslen(curHwId) + 1);
    }

    return FALSE;
}

NTSTATUS
IopSetupRemoteBootCard(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HANDLE UniqueIdHandle,
    IN PUNICODE_STRING UnicodeDeviceInstance
    )

/*++

Routine Description:

    This function modifies the registry to set up the netboot card.
    We must do this here since the card is needed to boot, we can't
    wait for the class installer to run.

    THIS ASSUMES THAT IOREMOTEBOOTCLIENT IS TRUE.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

    UniqueIdHandle - A handle to the device's unique node under the
        Enum key.

    UnicodeDeviceInstance - The device instance assigned to the device.

Return Value:

    Status of the operation.

--*/

{
    PSETUP_LOADER_BLOCK setupLoaderBlock;
    UNICODE_STRING unicodeName, pnpInstanceId, keyName;
    HANDLE tmpHandle;
    HANDLE parametersHandle = NULL;
    HANDLE currentControlSetHandle = NULL;
    HANDLE remoteBootHandle = NULL;
    HANDLE instanceHandle = NULL;
    PWCHAR componentIdBuffer, curComponentIdLoc;
    PCHAR registryList;
    ULONG componentIdLength;
    WCHAR tempNameBuffer[32];
    WCHAR tempValueBuffer[128];
    NTSTATUS status;
    ULONG tmpValue, length;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    PKEY_VALUE_BASIC_INFORMATION keyValueBasic;
    UCHAR dataBuffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + 128];
    ULONG enumerateIndex;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    //
    // If we already think we have initialized a remote boot card, then
    // exit (should not really happen once we identify cards using the
    // bus/slot.
    //

    if (IopRemoteBootCardInitialized) {
        return STATUS_SUCCESS;
    }

    //
    // setupLoaderBlock will always be non-NULL if we are
    // remote booting, even if we are not in setup.
    //

    setupLoaderBlock = LoaderBlock->SetupLoaderBlock;

    //
    // Open the current control set.
    //

    status = IopOpenRegistryKey(&currentControlSetHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSet,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open the Control\RemoteBoot key, which may not exist.
    //

    PiWstrToUnicodeString(&unicodeName, L"Control\\RemoteBoot");

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeName,
                               OBJ_CASE_INSENSITIVE,
                               currentControlSetHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );

    status = ZwCreateKey(&remoteBootHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &disposition
                         );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Open the key where the netui code stores information about the cards.
    // During textmode setup this will fail because the Control\Network
    // key is not there. After that it should work, although we may need
    // to create the last node in the path.
    //

    PiWstrToUnicodeString(&unicodeName, L"Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\{54C7D140-09EF-11D1-B25A-F5FE627ED95E}");

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeName,
                               OBJ_CASE_INSENSITIVE,
                               currentControlSetHandle,
                               (PSECURITY_DESCRIPTOR)NULL
                               );

    status = ZwCreateKey(&instanceHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING)NULL,
                         0,
                         &disposition
                         );

    if (NT_SUCCESS(status)) {

        //
        // If the PnpInstanceID of the first netboot card matches the one
        // for this device node, and the NET_CARD_INFO that the loader
        // found is the same as the one we saved, then this is the same
        // card with the same instance ID as before, so we don't need to
        // do anything.
        //

        PiWstrToUnicodeString(&unicodeName, L"PnPInstanceID");
        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)dataBuffer;
        RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

        status = ZwQueryValueKey(
                     instanceHandle,
                     &unicodeName,
                     KeyValuePartialInformation,
                     keyValue,
                     sizeof(dataBuffer),
                     &length);

        //
        // Check that it matches. We can init the string because we zeroed
        // the dataBuffer before reading the key, so even if the
        // registry value had no NULL at the end that is OK.
        //

        if ((NT_SUCCESS(status)) &&
            (keyValue->Type == REG_SZ)) {

            RtlInitUnicodeString(&pnpInstanceId, (PWSTR)(keyValue->Data));

            if (RtlEqualUnicodeString(UnicodeDeviceInstance, &pnpInstanceId, TRUE)) {

                //
                // Instance ID matched, see if the NET_CARD_INFO matches.
                //

                PiWstrToUnicodeString(&unicodeName, L"NetCardInfo");
                RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

                status = ZwQueryValueKey(
                             remoteBootHandle,
                             &unicodeName,
                             KeyValuePartialInformation,
                             keyValue,
                             sizeof(dataBuffer),
                             &length);

                if ((NT_SUCCESS(status)) &&
                    (keyValue->Type == REG_BINARY) &&
                    (keyValue->DataLength == sizeof(NET_CARD_INFO)) &&
                    (memcmp(keyValue->Data, setupLoaderBlock->NetbootCardInfo, sizeof(NET_CARD_INFO)) == 0)) {

                    //
                    // Everything matched, so no need to do any setup.
                    //

                    status = STATUS_SUCCESS;
                    goto cleanup;

                }
            }
        }
    }


    //
    // We come through here if the saved registry data was missing or
    // not correct. Write all the relevant values to the registry.
    //


    //
    // Service name is in the loader block.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_SERVICE);
    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardServiceName,
                  (wcslen(setupLoaderBlock->NetbootCardServiceName) + 1) * sizeof(WCHAR)
                  );

    //
    // ClassGUID is the known net card GUID.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_CLASSGUID);
    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  L"{4D36E972-E325-11CE-BFC1-08002BE10318}",
                  sizeof(L"{4D36E972-E325-11CE-BFC1-08002BE10318}")
                  );

    //
    // Driver is the first net card.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_DRIVER);
    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  L"{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000",
                  sizeof(L"{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000")
                  );

#ifdef REMOTE_BOOT
    //
    // Identify this as the netboot card so the network class
    // installer knows to assign the reserved GUID.
    //

    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_CONFIG_FLAGS);

    keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)dataBuffer;
    RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

    status = ZwQueryValueKey(UniqueIdHandle,
                             &unicodeName,
                             KeyValuePartialInformation,
                             keyValue,
                             sizeof(dataBuffer),
                             &length);

    if ((NT_SUCCESS(status)) &&
        (keyValue->Type == REG_DWORD)) {
        //
        // The ConfigFlags value exists in the registry
        //
        tmpValue = (*(PULONG)keyValue->Data) | CONFIGFLAG_NETBOOT_CARD;
    } else {
        tmpValue = CONFIGFLAG_NETBOOT_CARD;
    }

    ZwSetValueKey(UniqueIdHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );
#endif


    //
    // Open a handle for card parameters. We write RemoteBootCard plus
    // whatever the BINL server told us to write.
    //

    status = IopOpenRegistryKey(&tmpHandle,
                                NULL,
                                &CmRegistryMachineSystemCurrentControlSetControlClass,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    PiWstrToUnicodeString(&unicodeName, L"{4D36E972-E325-11CE-BFC1-08002BE10318}\\0000");

    status = IopOpenRegistryKey(&parametersHandle,
                                tmpHandle,
                                &unicodeName,
                                KEY_ALL_ACCESS,
                                FALSE
                                );

    ZwClose(tmpHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // We know that this is a different NIC, so remove all the old parameters.
    //

    keyValueBasic = (PKEY_VALUE_BASIC_INFORMATION)dataBuffer;
    enumerateIndex = 0;

    while (TRUE) {

        RtlZeroMemory(dataBuffer, sizeof(dataBuffer));

        status = ZwEnumerateValueKey(
                    parametersHandle,
                    enumerateIndex,
                    KeyValueBasicInformation,
                    keyValueBasic,
                    sizeof(dataBuffer),
                    &length
                    );
        if (status == STATUS_NO_MORE_ENTRIES) {
            status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        //
        // We don't delete "NetCfgInstanceID", it won't change and
        // its presence signifies to the net class installer that
        // this is a replacement not a clean install.
        //

        if (_wcsicmp(keyValueBasic->Name, L"NetCfgInstanceID") != 0) {

            RtlInitUnicodeString(&keyName, keyValueBasic->Name);
            status = ZwDeleteValueKey(
                        parametersHandle,
                        &keyName
                        );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

        } else {

            enumerateIndex = 1;   // leave NetCfgInstanceID at index 0
        }

    }

    //
    // Write a parameter called RemoteBootCard set to TRUE, this
    // is primarily so NDIS can recognize this as such.
    //

    PiWstrToUnicodeString(&unicodeName, L"RemoteBootCard");
    tmpValue = 1;
    ZwSetValueKey(parametersHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );


    //
    // Store any other parameters sent from the server.
    //

    registryList = setupLoaderBlock->NetbootCardRegistry;

    if (registryList != NULL) {

        STRING aString;
        UNICODE_STRING uString, uString2;

        //
        // The registry list is a series of name\0type\0value\0, with
        // a final \0 at the end. It is in ANSI, not UNICODE.
        //
        // All values are stored under parametersHandle. Type is 1 for
        // DWORD and 2 for SZ.
        //

        uString.Buffer = tempNameBuffer;
        uString.MaximumLength = sizeof(tempNameBuffer);

        while (*registryList != '\0') {

            //
            // First the name.
            //

            RtlInitString(&aString, registryList);
            RtlAnsiStringToUnicodeString(&uString, &aString, FALSE);

            //
            // Now the type.
            //

            registryList += (strlen(registryList) + 1);

            if (*registryList == '1') {

                //
                // A DWORD, parse it.
                //

                registryList += 2;   // skip "1\0"
                tmpValue = 0;

                while (*registryList != '\0') {
                    tmpValue = (tmpValue * 10) + (*registryList - '0');
                    ++registryList;
                }

                ZwSetValueKey(parametersHandle,
                              &uString,
                              TITLE_INDEX_VALUE,
                              REG_DWORD,
                              &tmpValue,
                              sizeof(tmpValue)
                              );

                registryList += (strlen(registryList) + 1);

            } else if (*registryList == '2') {

                //
                // An SZ, convert to Unicode.
                //

                registryList += 2;   // skip "2\0"

                uString2.Buffer = tempValueBuffer;
                uString2.MaximumLength = sizeof(tempValueBuffer);
                RtlInitAnsiString(&aString, registryList);
                RtlAnsiStringToUnicodeString(&uString2, &aString, FALSE);

                ZwSetValueKey(parametersHandle,
                              &uString,
                              TITLE_INDEX_VALUE,
                              REG_SZ,
                              uString2.Buffer,
                              uString2.Length + sizeof(WCHAR)
                              );

                registryList += (strlen(registryList) + 1);

            } else {

                //
                // Not "1" or "2", so stop processing registryList.
                //

                break;

            }

        }

    }

    //
    // Save the NET_CARD_INFO so we can check it next time.
    //

    PiWstrToUnicodeString(&unicodeName, L"NetCardInfo");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_BINARY,
                  setupLoaderBlock->NetbootCardInfo,
                  sizeof(NET_CARD_INFO)
                  );


    //
    // Save the hardware ID, driver name, and service name,
    // so the loader can read  those if the server is down
    // on subsequent boots.
    //

    PiWstrToUnicodeString(&unicodeName, L"HardwareId");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardHardwareId,
                  (wcslen(setupLoaderBlock->NetbootCardHardwareId) + 1) * sizeof(WCHAR)
                  );

    PiWstrToUnicodeString(&unicodeName, L"DriverName");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardDriverName,
                  (wcslen(setupLoaderBlock->NetbootCardDriverName) + 1) * sizeof(WCHAR)
                  );

    PiWstrToUnicodeString(&unicodeName, L"ServiceName");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  setupLoaderBlock->NetbootCardServiceName,
                  (wcslen(setupLoaderBlock->NetbootCardServiceName) + 1) * sizeof(WCHAR)
                  );

    //
    // Save the device instance, in case we need to ID the card later.
    //

    PiWstrToUnicodeString(&unicodeName, L"DeviceInstance");

    ZwSetValueKey(remoteBootHandle,
                  &unicodeName,
                  TITLE_INDEX_VALUE,
                  REG_SZ,
                  UnicodeDeviceInstance->Buffer,
                  UnicodeDeviceInstance->Length + sizeof(WCHAR)
                  );

    //
    // Make sure we only pick one card to setup this way!
    //

    IopRemoteBootCardInitialized = TRUE;


cleanup:
    if (instanceHandle != NULL) {
        ZwClose(instanceHandle);
    }
    if (remoteBootHandle != NULL) {
        ZwClose(remoteBootHandle);
    }
    if (parametersHandle != NULL) {
        ZwClose(parametersHandle);
    }
    if (currentControlSetHandle != NULL) {
        ZwClose(currentControlSetHandle);
    }

    return status;

}

#if defined(REMOTE_BOOT)
NTSTATUS
IopInitCsc(
    IN PUCHAR CscPath
    )
/*++

Routine Description:

    This function informs the Rdr2 Client Side Cache where to put the CSC database.
    If the database is not there or is corrupt then an error is returned.

Arguments:

    CscPath - supplies the Nt path with no trailing backslash to the csc directory.

Return Value:

    Status of the operation. STATUS_MEDIA_CHECK if database may have lost pin information.

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    SHADOWINFO shadowInfo;
    WIN32_FIND_DATAA * find32;

    find32 = ExAllocatePoolWithTag(NonPagedPool, sizeof(WIN32_FIND_DATAA), 'bRoI');
    if (find32 == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( find32, sizeof(WIN32_FIND_DATAA) );
    find32->nFileSizeHigh = 0;
    find32->nFileSizeLow = 0xccab348;
    find32->dwReserved1 = 0x8000;
    ASSERT(strlen(CscPath) < MAX_PATH);
    strcpy( find32->cFileName, CscPath );

    RtlZeroMemory( &shadowInfo, sizeof(shadowInfo) );
    shadowInfo.uStatus = 0x0001; // SHADOW_SWITCH_SHADOWING
    shadowInfo.uOp = 2; // SHADOW_SWITCH_ON
    shadowInfo.lpFind32 = (WIN32_FIND_DATAW *)find32;

    status = ZwDeviceIoControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_SWITCHES,
                &shadowInfo,
                0,
                NULL,
                0
                );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if ( NT_SUCCESS(status) && shadowInfo.uOp ) {

        //
        // Database was recreated.
        //

        StartCsc = FLUSH_CSC;
        IopSetFlushCSC(SET_FLUSH_CSC);
    }

    ExFreePool(find32);

    return status;
}

NTSTATUS
IopResetCsc(
    IN PUCHAR CscPath
    )
/*++

Routine Description:

    This function walks the servers directories creating CSC database entries for each
    file and directory on the server. The entries are marked as SPARSE so that the local
    copies of all files are discarded.

    There is a potential problem when a file is in the CSC database and is deleted on the
    server by an admin local to the server. Should this happen, go to the workstation with
    the CSC and delete the file or directory.

Arguments:

    CscPath - supplies the Nt path with no trailing backslash to the csc directory.

Return Value:

    Status of the operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING root;
    HSHADOW rootHandle;

    PUCHAR nameBuffer;
    ULONG nameBufferLength;
    ULONG length;
    HANDLE handle;
    UNICODE_STRING string;
    OBJECT_ATTRIBUTES objectAttributes;
    PKEY_VALUE_PARTIAL_INFORMATION keyValue;

    KdPrint(( "IopResetCsc Start\n" ));

#if 0
   //  We don't think we want to re-initialize the database since this will
    //  delete the information for user pinned files. If this functionality
    //  is required then CSC probably needs to implement it for all CSC machines.

    IO_STATUS_BLOCK ioStatusBlock;
    SHADOWINFO shadowInfo;
    WIN32_FIND_DATAA   sFind32;

    memset(&shadowInfo, 0, sizeof(SHADOWINFO));
    memset(&sFind32, 0, sizeof(sFind32));
    sFind32.nFileSizeHigh = 0;
    sFind32.nFileSizeLow = 0xccab348;
    sFind32.dwReserved1 = 0x8000;

    ASSERT(strlen(CscPath) < MAX_PATH);
    strcpy(sFind32.cFileName, CscPath);

    shadowInfo.uOp = 9; // SHADOW_REINIT_DATABASE;
    shadowInfo.lpFind32 = (WIN32_FIND_DATAW *)&sFind32;

    status = ZwDeviceIoControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                IOCTL_DO_SHADOW_MAINTENANCE,
                &shadowInfo,
                0,
                NULL,
                0
                );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

#endif

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );
    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopResetCsc: Unable to open Control key: %x\n", status ));
        return status;
    }

    if ( !ExpInTextModeSetup ) {
        RtlInitUnicodeString( &string, L"RemoteBootRoot" );
    } else {
        RtlInitUnicodeString( &string, L"RemoteBootMachineDirectory" );
    }

    nameBufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + (128 * sizeof(WCHAR));
    nameBuffer = ExAllocatePoolWithTag(NonPagedPool, nameBufferLength, 'bRoI');
    if (nameBuffer == NULL) {
        KdPrint(( "IopResetCsc: Unable to allocate nameBuffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)nameBuffer;
    RtlZeroMemory(nameBuffer, nameBufferLength);

    status = ZwQueryValueKey(
                 handle,
                 &string,
                 KeyValuePartialInformation,
                 keyValue,
                 nameBufferLength,
                 &length);

    ZwClose(handle);

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopResetCsc: Unable to read RemoteBootRoot value: %x\n", status ));
        KdPrint(( "IopResetCsc: returning: %x\n", status ));
        ExFreePool(nameBuffer);
        return status;
    }
    RtlInitUnicodeString(&root, (PWSTR)keyValue->Data);
    status = IopMarkRoot( &root, &rootHandle );
    if (NT_SUCCESS(status)) {
        status = IopWalkDirectoryTree( &root, rootHandle );
    }

    KdPrint(( "IopResetCsc: returning: %x\n", status ));

    ExFreePool(nameBuffer);
    return status;
}

PWSTR
IopBreakPath(
    IN PWSTR* Path
    )

/*++

Routine Description:

    Break Path by skipping any leading backslashes, skip over filename and then
    null out the nextbackslash or null.

    NOTE the Path must not end in a backslash.


Arguments:

    Path - Supplies the start of the next token. On return supplies the terminator
    of the current token or NULL if we return the last element in the Path.

Return Value:

    Start of the current token skipping any leading backslashes.

--*/
{
    PWSTR start = *Path;

    //  skip leading backslashes
    while (*start == L'\\') {
        start++;
    }

    //  Scan along token to find the terminator.

    *Path = start;
    while ((**Path != L'\\') && (**Path) ) {
        (*Path)++;
    }

    if (**Path) {
        //  Found backslash, NULL terminate token
        **Path = L'\0';
    } else {
        *Path = NULL;    //  Flag end of token string.
    }

    return start;
}

NTSTATUS
IopMarkRoot(
    IN PUNICODE_STRING Root,
    OUT PHSHADOW RootHandle
    )

/*++

Routine Description:

    Tells CSC to pin all files created beneath the Root.

    RootHandle - Supplies where to put the CSC handle corresponding to Root.

Arguments:

    Root - Supplies the NT fully qualified name to the root. e.g.:
            L"\Device\LanmanRedirector\colinw3\remoteboot\images\cwcompaq"

Return Value:

    NTSTATUS - status of the operation.

--*/

{

    NTSTATUS Status;
    PWSTR  current, next;
    HSHADOW hDir=0;
    HSHADOW hShadow;
    BOOLEAN first = TRUE;
    BOOLEAN done;

    PWIN32_FIND_DATAW sFind32;
    SHADOWINFO sSI;

    sFind32 = ExAllocatePoolWithTag(NonPagedPool, sizeof(WIN32_FIND_DATAW), 'bRoI');
    if (sFind32 == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //  Walk the CSC datastructures down to the Root directory


    if (Root->Length >= (MAX_PATH * sizeof(WCHAR))) {
        //  Simplifies some checking filling in FIND32 structures later.
        KdPrint(("IopMarkRoot: %ws is invalid\n", Root->Buffer));
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    //  Process the server component of the name. This is automatically in the
    //  database, magically created by CSC.
    //

    next = Root->Buffer + (sizeof(L"\\Device\\LanmanRedirector")/sizeof(WCHAR));;
    current = IopBreakPath(&next);

    RtlZeroMemory(sFind32, sizeof(WIN32_FIND_DATAW));
    sFind32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    //  Build \\Server\Share
    wcscpy(sFind32->cFileName, L"\\\\");
    wcscat(sFind32->cFileName, current);

    //  Restore separator that IopBreakPath removed
    if (next) {
        *next = L'\\';
    } else {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }
    current = IopBreakPath(&next);

    wcscat(sFind32->cFileName, L"\\");
    wcscat(sFind32->cFileName, current);

    Status = IopGetShadowExW(hDir, sFind32, &sSI);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("IopMarkRoot: IopGetShadowExW (%ws) failed %lx\n", sFind32->cFileName, Status));
        goto cleanup;
    }

    if (!sSI.hShadow) {
        //
        //  Directory is not already known to CSC. Make it so.
        //
        Status = IopCreateShadowW( hDir, sFind32, SHADOW_SPARSE, &hShadow );

        if (!NT_SUCCESS(Status)) {
            KdPrint(("IopMarkRoot: IopCreateShadowW (%ws) failed %lx\n", sFind32->cFileName, Status));
            goto cleanup;
        }
    } else {
        hShadow = sSI.hShadow;
    }

    //  Pinning the share means that the files are available without a LAN connection.
    Status = IopAddHintFromInode(
                hDir,
                hShadow,
                &sSI.ulHintPri,
                &sSI.ulHintFlags,
                FLAG_CSC_HINT_PIN_SYSTEM);

#if DBG
    if (Status != STATUS_PENDING) {
        if (DebugReset) KdPrint(( "IopAddHintFromInode: modified hint flags for %ws\n", next ));
    }
#endif

    //  Restore separator that IopBreakPath removed
    if (next) {
        *next = L'\\';
    } else {
        Status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    do {
        current = IopBreakPath(&next);

        RtlZeroMemory(sFind32, sizeof(WIN32_FIND_DATAW));
        sFind32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        wcscpy(sFind32->cFileName, current);

        hDir = hShadow;

        Status = IopGetShadowExW(hDir, sFind32, &sSI);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("IopMarkRoot: IopGetShadowExW (%ws) failed %lx\n", sFind32->cFileName, Status));
            goto cleanup;
        }


        if (!sSI.hShadow) {
            //
            //  Directory is not already known to CSC. Make it so.
            //
            Status = IopCreateShadowW( hDir, sFind32, SHADOW_SPARSE, &hShadow );

            if (!NT_SUCCESS(Status)) {
                KdPrint(("IopMarkRoot: IopCreateShadowW (%ws) failed %lx\n", sFind32->cFileName, Status));
                goto cleanup;
            }

        } else {
            hShadow = sSI.hShadow;
        }

        if (next) {

            //  Pinning the share means that the files are available without a LAN connection.
            Status = IopAddHintFromInode(
                        hDir,
                        hShadow,
                        &sSI.ulHintPri,
                        &sSI.ulHintFlags,
                        FLAG_CSC_HINT_PIN_SYSTEM);

#if DBG
            if (Status != STATUS_PENDING) {
                if (DebugReset) KdPrint(( "IopAddHintFromInode: modified hint flags for %ws\n", next ));
            }
#endif

            //  Restore separator that IopBreakPath removed
            *next = L'\\';

        } else {

            //
            //  WE HAVE FOUND THE ROOT. MARK IT
            //
            Status = IopAddHintFromInode(
                        hDir,
                        hShadow,
                        &sSI.ulHintPri,
                        &sSI.ulHintFlags,
                        (FLAG_CSC_HINT_PIN_SYSTEM | FLAG_CSC_HINT_PIN_INHERIT_SYSTEM));

#if DBG
            if (Status != STATUS_PENDING) {
                if (DebugReset) KdPrint(( "IopAddHintFromInode: modified hint flags for %ws\n", next ));
            }
#endif

            *RootHandle = hShadow;

            break;
        }

    } while ( TRUE );

cleanup:

    ExFreePool(sFind32);

    return Status;
}

VOID
IopSetFlushCSC(
    IN CSC_CONTROL Command
    )
/*++

Routine Description:

    Accesses the registry key that controls when to walk the server directory,
    discard all local copies of files and repin.

Arguments:

    Command - Supplies the intentions of the caller.

Return Value:

    TRUE if repin is required.

--*/

{

    NTSTATUS status;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING string;
    DWORD dword = Command;

    PKEY_VALUE_PARTIAL_INFORMATION keyValue;
    UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(DWORD)];
    ULONG length;

    RtlInitUnicodeString( &string, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );

    InitializeObjectAttributes(
        &objectAttributes,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey( &handle, KEY_ALL_ACCESS, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        return;
    }

    RtlInitUnicodeString( &string, L"ControlCSC" );

    if (Command == READ_CSC) {

        keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        status = ZwQueryValueKey(
                     handle,
                     &string,
                     KeyValuePartialInformation,
                     keyValue,
                     sizeof(buffer),
                     &length);

        dword = *((DWORD *)(&keyValue->Data[0]));

        if (NT_SUCCESS(status) && (dword == SET_FLUSH_CSC)) {
            StartCsc = FLUSH_CSC;
        }

    } else {
        status = ZwSetValueKey(
                    handle,
                    &string,
                    0,
                    REG_DWORD,
                    &dword,
                    sizeof(DWORD)
                    );
        if ( !NT_SUCCESS(status) ) {
            KdPrint(( "IopSetFlushCSC: Unable to set ControlCSC value: %x\n", status ));
        }
    }

    ZwClose( handle );

    return;
}

NTSTATUS
IopWalkDirectoryHelper(
    IN PUNICODE_STRING Directory,
    IN HSHADOW CSCHandle,
    IN PUCHAR Buffer,
    IN ULONG BufferSize
    )

/*++

Routine Description:

    This is a helper function for IopWalkDirectoryTree. Each file found is
    pinned. Each directory is added to the list for later processing.

Arguments:

    Directory - Supplies the NT Path to the directory to walk.

    CSCHandle - Supplies the handle passed to CSC that describes
                the Directory when pinning individual files.

    Buffer    - A scratch buffer to use.

    BufferSize - The length of Buffer.

Return Value:

    NTSTATUS - status of the operation.

--*/

{

    NTSTATUS Status;

    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    PUCHAR CopyAclBuffer;

    NTSTATUS NtStatus;

    PFILE_BOTH_DIR_INFORMATION FileInfo;
    ULONG i;
    PWIN32_FIND_DATAW sFind32;

    PWCHAR SkipNames[] = {L".",
                            L"..",
                            L"pagefile.sys",
                            L"csc",
                            L"RECYCLER",
                            L"TEMP",
                            L"TMP",
                            L""};
    BOOLEAN skip;


    sFind32 = ExAllocatePoolWithTag(NonPagedPool, sizeof(WIN32_FIND_DATAW), 'bRoI');
    if (sFind32 == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CopyAclBuffer = ExAllocatePoolWithTag(NonPagedPool, 512, 'bRoI');
    if (CopyAclBuffer == NULL) {
        ExFreePool(sFind32);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Open the file for list directory access
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        Directory,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(Status = ZwOpenFile( &FileHandle,
                               FILE_LIST_DIRECTORY | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatus,
                               FILE_SHARE_READ,
                               FILE_DIRECTORY_FILE ))) {
        goto cleanup;
    }

    //
    //  Do the directory loop
    //

    for (NtStatus = ZwQueryDirectoryFile( FileHandle,
                                          (HANDLE)NULL,
                                          (PIO_APC_ROUTINE)NULL,
                                          (PVOID)NULL,
                                          &IoStatus,
                                          Buffer,
                                          BufferSize,
                                          FileBothDirectoryInformation,
                                          FALSE,
                                          (PUNICODE_STRING)NULL,
                                          TRUE);
         NT_SUCCESS(NtStatus);
         NtStatus = ZwQueryDirectoryFile( FileHandle,
                                          (HANDLE)NULL,
                                          (PIO_APC_ROUTINE)NULL,
                                          (PVOID)NULL,
                                          &IoStatus,
                                          Buffer,
                                          BufferSize,
                                          FileBothDirectoryInformation,
                                          FALSE,
                                          (PUNICODE_STRING)NULL,
                                          FALSE) ) {

        if (!NT_SUCCESS(Status = ZwWaitForSingleObject(FileHandle, TRUE, NULL))) {
            goto cleanup;
        }

        //
        //  Check the Irp for success
        //

        if (!NT_SUCCESS(IoStatus.Status)) {
            break;
        }

        //
        //  For every record in the buffer type out the directory information
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise IoStatus would have been No More Files
        //

        FileInfo = (PFILE_BOTH_DIR_INFORMATION)&Buffer[0];

        while (TRUE) {

            {
                WCHAR Saved;
                DWORD Index;

                Saved = FileInfo->FileName[FileInfo->FileNameLength/2];
                FileInfo->FileName[FileInfo->FileNameLength/2] = 0;

                Index = 0;
                skip = FALSE;
                while (SkipNames[Index][0] != L'\0') {
                    if (_wcsicmp( FileInfo->FileName,SkipNames[Index])) {
                        Index++;
                    } else {
                        skip = TRUE;
                        break;
                    }
                }

                if (!skip) {
                    UNICODE_STRING Entry;
                    SHADOWINFO sSI;
                    PDIRENT pDE;

                    RtlInitUnicodeString(&Entry,NULL);

                    if (FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        //
                        //  Allocate an entry in the directory list. We will process
                        //  the directory at a later time.
                        //
                        UNICODE_STRING DirName;
                        USHORT NewNameLength;

                        RtlInitUnicodeString(&DirName, FileInfo->FileName);

                        NewNameLength =
                            (Directory->Length + DirName.Length + 1) * sizeof(WCHAR);

                        pDE = ExAllocatePoolWithTag(PagedPool, sizeof(DIRENT) + NewNameLength, 'bRoI');

                        if (pDE == NULL) {
                            KdPrint(("Couldn't allocate space for directory\n"));
                            KdPrint(("\\%ws\\%ws\n", Directory->Buffer, FileInfo->FileName));
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto cleanup;
                        }

                        InsertHeadList( &DirectoryList , &pDE->Next );
                        pDE->Directory.Length=0;
                        pDE->Directory.MaximumLength=NewNameLength;
                        pDE->Directory.Buffer = &pDE->Name[0];
                        RtlCopyUnicodeString(&pDE->Directory, Directory);
                        RtlAppendUnicodeToString(&pDE->Directory, L"\\");
                        pDE->LastToken = pDE->Directory.Buffer + (pDE->Directory.Length/sizeof(WCHAR));
                        RtlAppendUnicodeStringToString(&pDE->Directory, &DirName);
                    }

                    RtlZeroMemory(sFind32, sizeof(WIN32_FIND_DATAW));
                    wcscpy(sFind32->cFileName, FileInfo->FileName);

                    Status = IopGetShadowExW(CSCHandle, sFind32, &sSI);
                    if (!NT_SUCCESS(Status)) {
                        KdPrint(( "IopGetShadowExW: returned: %x for %wZ\\%ws\n", Status, Directory, FileInfo->FileName ));
                        goto cleanup;
                    }

                    if (!sSI.hShadow) {
                        //  Need to create CSC database entry.
                        sFind32->dwFileAttributes = FileInfo->FileAttributes;
                        sFind32->ftCreationTime = *(LPFILETIME)&FileInfo->CreationTime;
                        sFind32->ftLastAccessTime = *(LPFILETIME)&FileInfo->LastAccessTime;
                        sFind32->ftLastWriteTime = *(LPFILETIME)&FileInfo->LastWriteTime;
                        sFind32->nFileSizeLow = FileInfo->EndOfFile.LowPart;
                        sFind32->nFileSizeHigh = FileInfo->EndOfFile.HighPart;
                        memcpy(
                            sFind32->cAlternateFileName,
                            FileInfo->ShortName,
                            FileInfo->ShortNameLength );
                        Status = IopCreateShadowW( CSCHandle, sFind32, SHADOW_SPARSE, &sSI.hShadow);

                        if (!NT_SUCCESS(Status)) {
                            KdPrint(( "IopCreateShadowW: returned: %x for %wZ\\%ws\n", Status, Directory, FileInfo->FileName ));
                            goto cleanup;
                        }
                        // Copy the ACL from the server.
                        Status = IopCopyServerAcl(
                                     FileHandle,         // parent directory
                                     FileInfo->FileName, // this directory
                                     sSI.hShadow,        // shadow handle
                                     TRUE,               // it is a directory
                                     CopyAclBuffer,      // scratch buffer
                                     512);               // buffer size
#if 0
                        //
                        // We may get ACCESS_DENIED errors from dirs that
                        // have been created on the server by hand, so don't
                        // treat this as fatal.
                        //
                        if (!NT_SUCCESS(Status)) {
                            goto cleanup;
                        }
#endif
                    }
                    ASSERT(CSCHandle == sSI.hDir);

                    Status = IopAddHintFromInode(
                                sSI.hDir,
                                sSI.hShadow,
                                &sSI.ulHintPri,
                                &sSI.ulHintFlags,
                                (FLAG_CSC_HINT_PIN_SYSTEM | FLAG_CSC_HINT_PIN_INHERIT_SYSTEM));

#if DBG
                    if (Status != STATUS_PENDING) {
                        if (DebugReset) KdPrint(( "IopAddHintFromInode: modified hint flags for %wZ\\%ws\n", Directory, FileInfo->FileName ));
                    }
#endif
                    if (!NT_SUCCESS(Status)) {
                        KdPrint(( "IopAddHintFromInode: returned: %x for %wZ\\%ws\n", Status, Directory, FileInfo->FileName ));
                        goto cleanup;
                    }

                    if (FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        pDE->CSCHandle = sSI.hShadow;
                    } else {
                        if ((sSI.uStatus & SHADOW_SPARSE) == 0) {
                            Status = IopSetShadowInfoW(
                                        sSI.hDir,
                                        sSI.hShadow,
                                        SHADOW_SPARSE,
                                        SHADOW_FLAGS_ASSIGN | SHADOW_FLAGS_TRUNCATE_DATA);
#if DBG
                            if (DebugReset) KdPrint(( "IopSetShadowInfoW: truncated %wZ\\%ws\n", Directory, FileInfo->FileName ));
#endif
                            if (!NT_SUCCESS(Status)) {
                                KdPrint(( "IopSetShadowInfoW: returned: %x for %wZ\\%ws\n", Status, Directory, FileInfo->FileName ));
                                goto cleanup;
                            }
                        }
                    }
                }

                FileInfo->FileName[FileInfo->FileNameLength/2] = Saved;
            }


            //
            //  Check if there is another record, if there isn't then we
            //  simply get out of this loop
            //

            if (FileInfo->NextEntryOffset == 0) {
                break;
            }

            //
            //  There is another record so advance FileInfo to the next
            //  record
            //


            FileInfo = (PFILE_BOTH_DIR_INFORMATION)(((PUCHAR)FileInfo) + FileInfo->NextEntryOffset);

        }
    }

    ZwClose( FileHandle );

    Status = STATUS_SUCCESS;

cleanup:

    ExFreePool(sFind32);
    ExFreePool(CopyAclBuffer);

    return Status;

} // IopWalkDirectoryHelper

NTSTATUS
IopWalkDirectoryTree(
    IN PUNICODE_STRING Directory,
    IN HSHADOW CSCHandle
    )

/*++

Routine Description:

Arguments:

    Directory - Supplies the NT Path to the directory to walk.

    CSCHandle - Supplies the handle passed to CSC that describes
                the Directory when pinning individual files.

Return Value:

    NTSTATUS - status of the operation.

--*/

{

    PDIRENT Next;
    NTSTATUS status;

    #define BUFFERSIZE 1024
    PUCHAR Buffer;


    // Make 2 bytes larger so that when we null terminate filename we can't run off the end.
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BUFFERSIZE+2, 'bRoI');

    if (Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&DirectoryList);

    status = IopWalkDirectoryHelper(Directory, CSCHandle, Buffer, BUFFERSIZE); // Process first level

    if (!NT_SUCCESS(status)) {
        while (!IsListEmpty(&DirectoryList)) {
            Next = (PDIRENT)RemoveHeadList(&DirectoryList);
            ExFreePool(Next);
        }
        ExFreePool(Buffer);
        return status;
    }

    //
    //  Each directory that WalkDirectory finds gets added to the list.
    //  process the list until we have no more directories.
    //

    while (!IsListEmpty(&DirectoryList)) {

        Next = (PDIRENT)RemoveHeadList(&DirectoryList);

        status = IopWalkDirectoryHelper(&Next->Directory, Next->CSCHandle, Buffer, BUFFERSIZE);

        ExFreePool(Next);

        if (!NT_SUCCESS(status)) {
            while (!IsListEmpty(&DirectoryList)) {
                Next = (PDIRENT)RemoveHeadList(&DirectoryList);
                ExFreePool(Next);
            }
            //  We will try to repin next reboot
            ExFreePool(Buffer);
            return status;
        }
    }

    IopSetFlushCSC(SET_FLUSHED_CSC);
    ExFreePool(Buffer);
    return status;

} // IopWalkDirectoryTree

/*****************************************************************************
 *      Given an hDir and filename, get SHADOWINFO if it exists
 */
NTSTATUS
IopGetShadowExW(
    HSHADOW hDir,
    LPWIN32_FIND_DATAW lpFind32,
    LPSHADOWINFO lpSI
    )
{
    IO_STATUS_BLOCK Iosb;
    NTSTATUS status;

    RtlZeroMemory((PUCHAR)lpSI, sizeof(SHADOWINFO));
    lpSI->hDir = hDir;
    lpSI->lpFind32 = lpFind32;

    status = ZwDeviceIoControlFile(
                RdrHandle,
                NULL,
                NULL,               // APC routine
                NULL,               // APC Context
                &Iosb,
                IOCTL_GETSHADOW,    // IoControlCode
                (LPVOID)(lpSI),     // Buffer for data to the FS
                0,
                NULL,               // OutputBuffer for data from the FS
                0                   // OutputBuffer Length
                );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = Iosb.Status;
    }

    return status;
}

/*****************************************************************************
 *      Call down to the VxD to add a hint on the inode.
 *  This ioctl does the right thing for user and system hints
 *  If successful, there is an additional pincount on the inode entry
 *  and the flags that are passed in are ORed with the original entry
 */
NTSTATUS
IopAddHintFromInode(
    HSHADOW  hDir,
    HSHADOW  hShadow,
    ULONG    *lpulHintPri,
    ULONG    *lpulHintFlags,
    ULONG    HintFlagsToSet
    )
{

    IO_STATUS_BLOCK Iosb;
    NTSTATUS status;
    SHADOWINFO      sSI;

    HintFlagsToSet |= FLAG_CSC_HINT_CONSERVE_BANDWIDTH;

    if ((*lpulHintFlags & HintFlagsToSet) != HintFlagsToSet) {

        RtlZeroMemory(&sSI, sizeof(SHADOWINFO));
        sSI.hDir = hDir;
        sSI.hShadow = hShadow;
        sSI.ulHintFlags = *lpulHintFlags | HintFlagsToSet;
        sSI.uOp = SHADOW_ADDHINT_FROM_INODE;

        status = ZwDeviceIoControlFile(
                    RdrHandle,
                    NULL,
                    NULL,               // APC routine
                    NULL,               // APC Context
                    &Iosb,
                    IOCTL_DO_SHADOW_MAINTENANCE,    // IoControlCode
                    (LPVOID)(&sSI),     // Buffer for data to the FS
                    0,
                    NULL,               // OutputBuffer for data from the FS
                    0                   // OutputBuffer Length
                    );

        ASSERT( status != STATUS_PENDING );
        if ( NT_SUCCESS(status) ) {
            status = Iosb.Status;
        }

        if (NT_SUCCESS(status)) {
            *lpulHintFlags = sSI.ulHintFlags;
            *lpulHintPri = sSI.ulHintPri;
        }
    } else {
        status = STATUS_PENDING;
    }

    return status;
}

/*****************************************************************************
 *  given an hDir and hShadow, set uStatus about the file.
 *  Operation depends on uOp given.
 */
NTSTATUS
IopSetShadowInfoW(
    HSHADOW hDir,
    HSHADOW hShadow,
    ULONG uStatus,
    ULONG uOp
    )
{

    IO_STATUS_BLOCK Iosb;
    NTSTATUS status;
    SHADOWINFO      sSI;

    RtlZeroMemory(&sSI, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.uStatus = uStatus;
    sSI.uOp = uOp;

    status = ZwDeviceIoControlFile(
                RdrHandle,
                NULL,
                NULL,               // APC routine
                NULL,               // APC Context
                &Iosb,
                IOCTL_SHADOW_SET_SHADOW_INFO,    // IoControlCode
                (LPVOID)(&sSI),     // Buffer for data to the FS
                0,
                NULL,               // OutputBuffer for data from the FS
                0                   // OutputBuffer Length
                );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = Iosb.Status;
    }

    return status;
}

/*****************************************************************************
 *      Call down to the VxD to add a file to the shadow.
 *      lphShadow is filled in with the new HSHADOW.
 *      Set uStatus as necessary (ie: SPARSE or whatever...)
 */
NTSTATUS
IopCreateShadowW(
    HSHADOW hDir,
    LPWIN32_FIND_DATAW lpFind32,
    ULONG uStatus,
    PHSHADOW lphShadow
    )
{
    IO_STATUS_BLOCK Iosb;
    NTSTATUS status;
    SHADOWINFO sSI;

    RtlZeroMemory(&sSI, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.uStatus = uStatus;
    sSI.lpFind32 = lpFind32;


    status = ZwDeviceIoControlFile(
                RdrHandle,
                NULL,
                NULL,               // APC routine
                NULL,               // APC Context
                &Iosb,
                IOCTL_SHADOW_CREATE,    // IoControlCode
                (LPVOID)(&sSI),     // Buffer for data to the FS
                0,
                NULL,               // OutputBuffer for data from the FS
                0                   // OutputBuffer Length
                );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = Iosb.Status;
    }

    if (NT_SUCCESS(status)) {
        *lphShadow = sSI.hShadow;
    }

    return status;
}

NTSTATUS
IopCopyServerAcl(
    HANDLE ParentHandle,
    PWCHAR FileName,
    HSHADOW ShadowHandle,
    BOOLEAN IsDirectory,
    PUCHAR Buffer,
    ULONG BufferSize
    )

/*++

Routine Description:

    This routines takes a file/directory on the server and a shadow
    file handle and copies the security descriptor from the server
    to the shadow file.

Arguments:

    ParentHandle - The open handle to the parent of the file on the server.

    FileName - The name of the file/directory on the server.

    ShadowHandle - A CSC handle to the shadow file.

    IsDirectory - Does FileName refer to a directory.

    Buffer - A temporary buffer used to query security descriptors.

    BufferSize - The number of bytes of Buffer.

Return Value:

    NTSTATUS - status of the operation.

--*/

{
    PWCHAR ShadowFileName = NULL;
    UNICODE_STRING ShadowFileString;
    UNICODE_STRING FileNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    COPYPARAMSW CopyParams;
    HANDLE ShadowFileHandle;
    HANDLE ServerFileHandle;
    ULONG LengthNeeded;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOLEAN AllocatedSecurityDescriptor = FALSE;
    PACL Dacl;
    BOOLEAN DaclPresent, DaclDefaulted;
    NTSTATUS Status;

    //
    // Open the server file with "read contol" permission.
    //

    RtlInitUnicodeString(&FileNameString, FileName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileNameString,
        OBJ_CASE_INSENSITIVE,
        ParentHandle,
        NULL);

    Status = ZwOpenFile(
                 &ServerFileHandle,
                 READ_CONTROL,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 0);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatusBlock.Status)) {
        KdPrint(("IopCopyServerAcl: ZwOpenFile on %wZ failed %lx %lx\n", &FileNameString, Status, IoStatusBlock.Status));
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
        }
        goto cleanup;
    }

    //
    // Now read the DACL from the server file.
    //

    Status = ZwQuerySecurityObject(
                 ServerFileHandle,
                 DACL_SECURITY_INFORMATION,
                 Buffer,
                 BufferSize,
                 &LengthNeeded);

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_BUFFER_TOO_SMALL) {

            //
            // Allocate a buffer and query again.
            //

            SecurityDescriptor = ExAllocatePoolWithTag(PagedPool, LengthNeeded, 'bRoI');
            if (SecurityDescriptor == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                AllocatedSecurityDescriptor = TRUE;

                Status = ZwQuerySecurityObject(
                             ServerFileHandle,
                             DACL_SECURITY_INFORMATION,
                             SecurityDescriptor,
                             LengthNeeded,
                             &LengthNeeded);

            }
        }
        if (!NT_SUCCESS(Status)) {
            KdPrint(("IopCopyServerAcl: Could not ZwQuerySecurityObject on %wZ %lx\n", &FileNameString, Status));
            ZwClose(ServerFileHandle);
            goto cleanup;
        }

    } else {

        SecurityDescriptor = (PSECURITY_DESCRIPTOR)Buffer;
    }

    ZwClose(ServerFileHandle);   // don't need this any more.


    //
    // If there is no DACL, return.
    //

    Status = RtlGetDaclSecurityDescriptor(
                 SecurityDescriptor,
                 &DaclPresent,
                 &Dacl,
                 &DaclDefaulted);

    if (!NT_SUCCESS(Status) || !DaclPresent || (Dacl == NULL)) {
        // DbgPrint("IopCopyServerAcl: No DACL for %wZ\n", &FileNameString);
        goto cleanup;
    }

    //
    // Get the shadow file name.
    //

    ShadowFileName = ExAllocatePoolWithTag(NonPagedPool, sizeof(WCHAR) * MAX_PATH, 'bRoI');
    if (ShadowFileName == NULL) {
        KdPrint(("IopCopyServerAcl: Could not allocate ShadowFileName\n"));
        goto cleanup;
    }

    Status = IopGetShadowPathW(
                 ShadowHandle,
                 ShadowFileName,
                 &CopyParams);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("IopCopyServerAcl: IopGetShadowPathW failed on %wZ %lx\n", &FileNameString, Status));
        goto cleanup;
    }

    //
    // Open the file with "write DACL" permission.
    //

    RtlInitUnicodeString(&ShadowFileString, ShadowFileName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &ShadowFileString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = ZwOpenFile(
                 &ShadowFileHandle,
                 WRITE_DAC,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 0);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatusBlock.Status)) {
        KdPrint(("IopCopyServerAcl: ZwOpenFile on %wZ (%wZ) failed %lx %lx\n", &FileNameString, &ShadowFileString, Status, IoStatusBlock.Status));
        if (NT_SUCCESS(Status)) {
            Status = IoStatusBlock.Status;
        }
        goto cleanup;
    }

    //
    // Write the security descriptor to the shadow file.
    //

    Status = ZwSetSecurityObject(
                 ShadowFileHandle,
                 DACL_SECURITY_INFORMATION,
                 SecurityDescriptor);

    ZwClose(ShadowFileHandle);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("IopCopyServerAcl: Could not ZwSetSecurityObject on %wZ %lx\n", &FileNameString, Status));
    }

cleanup:

    if (AllocatedSecurityDescriptor) {
        ExFreePool(SecurityDescriptor);
    }
    if (ShadowFileName != NULL) {
        ExFreePool(ShadowFileName);
    }

    return Status;

}


/*****************************************************************************
 *      Given an hShadow, get the shadow path if it exists
 */
NTSTATUS
IopGetShadowPathW(
    HSHADOW hShadow,
    PWCHAR lpLocalPath,
    LPCOPYPARAMSW lpCP
    )
{
    IO_STATUS_BLOCK Iosb;
    NTSTATUS status;

    RtlZeroMemory((PUCHAR)lpCP, sizeof(COPYPARAMSW));
    lpCP->hShadow = hShadow;
    lpCP->lpLocalPath = lpLocalPath;
    lpCP->uOp = 1;     // return full \Device\... name

    status = ZwDeviceIoControlFile(
                RdrHandle,
                NULL,
                NULL,               // APC routine
                NULL,               // APC Context
                &Iosb,
                IOCTL_SHADOW_GET_UNC_PATH, // IoControlCode
                (LPVOID)(lpCP),     // Buffer for data to the FS
                0,
                NULL,               // OutputBuffer for data from the FS
                0                   // OutputBuffer Length
                );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = Iosb.Status;
    }

    return status;
}
#endif // defined(REMOTE_BOOT)

NTSTATUS
IopWriteIpAddressToRegistry(
        HANDLE handle,
        PWCHAR regkey,
        PUCHAR value
        )
{
    NTSTATUS status;
    UNICODE_STRING string;
    CHAR addressA[16];
    WCHAR addressW[16];
    STRING addressStringA;
    UNICODE_STRING addressStringW;

    RtlInitUnicodeString( &string, regkey );

    sprintf(addressA, "%d.%d.%d.%d",
             value[0],
             value[1],
             value[2],
             value[3]);

    RtlInitAnsiString(&addressStringA, addressA);
    addressStringW.Buffer = addressW;
    addressStringW.MaximumLength = sizeof(addressW);

    RtlAnsiStringToUnicodeString(&addressStringW, &addressStringA, FALSE);

    status = NtSetValueKey(
                handle,
                &string,
                0,
                REG_MULTI_SZ,
                addressW,
                addressStringW.Length + sizeof(WCHAR)
                );
    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IopWriteIpAddressToRegistry: Unable to set %ws value: %x\n", regkey, status ));
    }
    return status;
}


NTSTATUS
IopSetDefaultGateway(
    IN ULONG GatewayAddress
    )
/*++

Routine Description:

    This function adds a default gateway entry from the router table.

Arguments:

    GatewayAddress - Address of the default gateway.

Return Value:

    Error Code.

--*/
{
    NTSTATUS Status;

    HANDLE Handle = NULL;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    DWORD Size;
    IPSNMPInfo IPStats;
    IPAddrEntry *AddrTable = NULL;
    DWORD NumReturned;
    DWORD Type;
    DWORD i;
    DWORD MatchIndex;
    IPRouteEntry RouteEntry;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK ioStatusBlock;

    if (GatewayAddress == 0) {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString( &NameString, DD_TCP_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &NameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &Handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(Status) ) {
        KdPrint(( "IopSetDefaultGateway: Unable to open TCPIP: %x\n", Status ));
        return Status;
    }

    //
    // Get the NetAddr info, to find an interface index for the gateway.
    //

    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IP_MIB_STATS_ID;

    Size = sizeof(IPStats);
    memset(&IPStats, 0x0, Size);
    memset(Context, 0x0, CONTEXT_SIZE);

    Status = IopTCPQueryInformationEx(
                Handle,
                &ID,
                &IPStats,
                &Size,
                Context);

    if (!NT_SUCCESS(Status)) {
        KdPrint(( "IopSetDefaultGateway: Unable to query TCPIP(1): %x\n", Status ));
        goto Cleanup;
    }

    Size = IPStats.ipsi_numaddr * sizeof(IPAddrEntry);
    AddrTable = ExAllocatePoolWithTag(PagedPool, Size, 'bRoI');

    if (AddrTable == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    memset(Context, 0x0, CONTEXT_SIZE);

    Status = IopTCPQueryInformationEx(
                Handle,
                &ID,
                AddrTable,
                &Size,
                Context);

    if (!NT_SUCCESS(Status)) {
        KdPrint(( "IopSetDefaultGateway: Unable to query TCPIP(2): %x\n", Status ));
        goto Cleanup;
    }

    NumReturned = Size/sizeof(IPAddrEntry);

    //
    // We've got the address table. Loop through it. If we find an exact
    // match for the gateway, then we're adding or deleting a direct route
    // and we're done. Otherwise try to find a match on the subnet mask,
    // and remember the first one we find.
    //

    Type = IRE_TYPE_INDIRECT;
    for (i = 0, MatchIndex = 0xffff; i < NumReturned; i++) {

        if( AddrTable[i].iae_addr == GatewayAddress ) {

            //
            // Found an exact match.
            //

            MatchIndex = i;
            Type = IRE_TYPE_DIRECT;
            break;
        }

        //
        // The next hop is on the same subnet as this address. If
        // we haven't already found a match, remember this one.
        //

        if ( (MatchIndex == 0xffff) &&
             (AddrTable[i].iae_addr != 0) &&
             (AddrTable[i].iae_mask != 0) &&
             ((AddrTable[i].iae_addr & AddrTable[i].iae_mask) ==
                (GatewayAddress  & AddrTable[i].iae_mask)) ) {

            MatchIndex = i;
        }
    }

    //
    // We've looked at all of the entries. See if we found a match.
    //

    if (MatchIndex == 0xffff) {
        //
        // Didn't find a match.
        //

        Status = STATUS_UNSUCCESSFUL;
        KdPrint(( "IopSetDefaultGateway: Unable to find match for gateway\n" ));
        goto Cleanup;
    }

    //
    // We've found a match. Fill in the route entry, and call the
    // Set API.
    //

    RouteEntry.ire_dest = DEFAULT_DEST;
    RouteEntry.ire_index = AddrTable[MatchIndex].iae_index;
    RouteEntry.ire_metric1 = DEFAULT_METRIC;
    RouteEntry.ire_metric2 = (DWORD)(-1);
    RouteEntry.ire_metric3 = (DWORD)(-1);
    RouteEntry.ire_metric4 = (DWORD)(-1);
    RouteEntry.ire_nexthop = GatewayAddress;
    RouteEntry.ire_type = Type;
    RouteEntry.ire_proto = IRE_PROTO_LOCAL;
    RouteEntry.ire_age = 0;
    RouteEntry.ire_mask = DEFAULT_DEST_MASK;
    RouteEntry.ire_metric5 = (DWORD)(-1);
    RouteEntry.ire_context = NULL;

    Size = sizeof(RouteEntry);

    ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

    Status = IopTCPSetInformationEx(
                Handle,
                &ID,
                &RouteEntry,
                Size );

    if (!NT_SUCCESS(Status)) {
        KdPrint(( "IopSetDefaultGateway: Unable to set default gateway: %x\n", Status ));
    }

    NtClose(Handle);

    Handle = NULL;

Cleanup:

    if (Handle != NULL) {
        NtClose(Handle);
    }

    if( AddrTable != NULL ) {
        ExFreePool( AddrTable );
    }

    return Status;
}


__inline long
htonl(long x)
{
        return((((x) >> 24) & 0x000000FFL) |
           (((x) >>  8) & 0x0000FF00L) |
           (((x) <<  8) & 0x00FF0000L) |
           (((x) << 24) & 0xFF000000L));
}

NTSTATUS
IopCacheNetbiosNameForIpAddress(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function takes an IP address, and submits it to NetBt for name resolution.

Arguments:

    IpAddress - Address to resolve

Return Value:

    Error Code.

--*/
{
    NTSTATUS Status;
    HANDLE Handle = NULL;
    BYTE Context[CONTEXT_SIZE];
    DWORD Size;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING NameString;
    IO_STATUS_BLOCK ioStatusBlock;
    tREMOTE_CACHE cacheInfo;
    PCHAR serverName;
    PCHAR endOfServerName;

    //
    // Open NetBT.
    //

    RtlInitUnicodeString(
        &NameString,
        L"\\Device\\NetBT_Tcpip_{54C7D140-09EF-11D1-B25A-F5FE627ED95E}"
        );

    InitializeObjectAttributes(
        &objectAttributes,
        &NameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &Handle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );
    if ( !NT_SUCCESS(Status) ) {
        KdPrint(( "IopCacheNetbiosNameForIpAddress: Unable to open NETBT: %x\n", Status ));
        return Status;
    }

    //
    // Get the server's name.
    //
    // If this is a remote boot setup boot, NtBootPathName is of the
    // form \<server>\<share>\setup\<install-directory>\<platform>.
    // If this is a normal remote boot, NtBootPathName is of the form
    // \<server>\<share>\images\<machine>\winnt.
    //
    // Thus in either case, we need to isolate the first element of the
    // path.
    //

    serverName = LoaderBlock->NtBootPathName;
    if ( *serverName == '\\' ) {
        serverName++;
    }
    endOfServerName = strchr( serverName, '\\' );
    if ( endOfServerName == NULL ) {
        endOfServerName = strchr( serverName, '\0' );
    }

    //
    // Fill in the tREMOTE_CACHE structure.
    //

    memset(&cacheInfo, 0x0, sizeof(cacheInfo));

    memset(cacheInfo.name, ' ', NETBIOS_NAMESIZE);
    memcpy(cacheInfo.name, serverName, (ULONG)(endOfServerName - serverName));
    cacheInfo.IpAddress = htonl(LoaderBlock->SetupLoaderBlock->ServerIpAddress);
    cacheInfo.Ttl = MAXULONG;

    //
    // Submit the IOCTL.
    //

    Status = NtDeviceIoControlFile(
               Handle,
               NULL,
               NULL,
               NULL,
               &ioStatusBlock,
               IOCTL_NETBT_ADD_TO_REMOTE_TABLE,
               &cacheInfo,
               sizeof(cacheInfo),
               Context,
               sizeof(Context)
               );

    ASSERT( Status != STATUS_PENDING );
    if ( NT_SUCCESS(Status) ) {
        Status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(Status) ) {
        KdPrint(( "IopCacheNetbiosNameForIpAddress: Adapter status failed: %x\n", Status ));
    }

    NtClose(Handle);

    return Status;
}


NTSTATUS
IopTCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
    IN TDIObjectID FAR       *ID,
    OUT void FAR             *Buffer,
    IN OUT DWORD FAR         *BufferSize,
    IN OUT BYTE FAR          *Context
    )
/*++

Routine Description:

    This routine provides the interface to the TDI QueryInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to query
    Buffer        - Data buffer to contain the query results
    BufferSize    - Pointer to the size of the results buffer. Filled in
                    with the amount of results data on return.
    Context       - Context value for the query. Should be zeroed for a
                    new query. It will be filled with context
                    information for linked enumeration queries.

Return Value:

    An NTSTATUS value.

--*/

{
    TCP_REQUEST_QUERY_INFORMATION_EX   queryBuffer;
    DWORD                              queryBufferSize;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;


    if (TCPHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    queryBufferSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    memcpy(&(queryBuffer.ID), ID, sizeof(TDIObjectID));
    memcpy(&(queryBuffer.Context), Context, CONTEXT_SIZE);

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_QUERY_INFORMATION_EX,  // Control code
                 &queryBuffer,                    // Input buffer
                 queryBufferSize,                 // Input buffer size
                 Buffer,                          // Output buffer
                 *BufferSize                      // Output buffer size
                 );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if (status == STATUS_SUCCESS) {
        //
        // Copy the return context to the caller's context buffer
        //
        memcpy(Context, &(queryBuffer.Context), CONTEXT_SIZE);
        *BufferSize = (ULONG)ioStatusBlock.Information;
        status = ioStatusBlock.Status;
    } else {
        *BufferSize = 0;
    }

    return(status);
}


NTSTATUS
IopTCPSetInformationEx(
    IN HANDLE             TCPHandle,
    IN TDIObjectID FAR   *ID,
    IN void FAR          *Buffer,
    IN DWORD FAR          BufferSize
    )
/*++

Routine Description:

    This routine provides the interface to the TDI SetInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to set
    Buffer        - Data buffer containing the information to be set
    BufferSize    - The size of the set data buffer.

Return Value:

    An NTSTATUS value.

--*/

{
    PTCP_REQUEST_SET_INFORMATION_EX    setBuffer;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;
    DWORD                              setBufferSize;


    if (TCPHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    setBufferSize = FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) + BufferSize;

    setBuffer = ExAllocatePoolWithTag(PagedPool, setBufferSize, 'bRoI');

    if (setBuffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    setBuffer->BufferSize = BufferSize;

    memcpy(&(setBuffer->ID), ID, sizeof(TDIObjectID));

    memcpy(&(setBuffer->Buffer[0]), Buffer, BufferSize);

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_SET_INFORMATION_EX,    // Control code
                 setBuffer,                       // Input buffer
                 setBufferSize,                   // Input buffer size
                 NULL,                            // Output buffer
                 0                                // Output buffer size
                 );

    ASSERT( status != STATUS_PENDING );
    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    ExFreePool(setBuffer);

    return(status);
}

#if defined(REMOTE_BOOT)
VOID
IopGetHarddiskInfo(
    OUT PWSTR NetHDCSCPartition
    )
/*++

Routine Description:

    This routine searches the each partition on each hard disk for
    one which has the active bit set, returning the first one encountered.

Arguments:

    LoaderBlock - The loader parameter block.

    NetHDCSCPartition - Either an arcname or string of type \Device\HarddiskX\PartitionY.
        Defaults to \Device\Harddisk0\Partition1

Return Value:

    None

--*/

{
    PARTITION_INFORMATION PartitionInfo;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    PWSTR NameBuffer;
    PWSTR DiskNameBuffer;
    PWSTR NetHDBootPartition;
    ULONG DiskNumber;
    ULONG PartitionNumber;
    ULONG DestPartitionNumber;
    HANDLE Handle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    LARGE_INTEGER LargestFreeSpace;
    LARGE_INTEGER FreeSpace;

    wcscpy(NetHDCSCPartition, L"\\Device\\Harddisk0\\Partition1");

    NameBuffer = ExAllocatePoolWithTag(NonPagedPool, 80 * 3 * sizeof(WCHAR), 'bRoI');
    if ( NameBuffer == NULL ) {
        KdPrint(( "IopGetHarddiskInfo: unable to allocate buffer\n" ));
        return;
    }

    DiskNameBuffer = NameBuffer + 80;
    NetHDBootPartition = DiskNameBuffer + 80;

    wcscpy(DiskNameBuffer, L"\\Device\\Harddisk0");
    wcscpy(NetHDBootPartition, DiskNameBuffer);
    wcscpy(NetHDBootPartition, L"\\Partition1");
    wcscpy(NetHDCSCPartition, NetHDBootPartition);

    //
    // We find the first bootable harddisk-slash-partition and call that the boot partition.
    //

    DiskNumber = 0;
    while (TRUE) {

        //
        // First check if there is any disk there at all by opening partition 0
        //
        PartitionNumber = 0;

        swprintf(NameBuffer, L"\\Device\\Harddisk%d\\Partition%d", DiskNumber, PartitionNumber);

        RtlInitUnicodeString(&UnicodeString, NameBuffer);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwCreateFile( &Handle,
                               (ACCESS_MASK)FILE_GENERIC_READ,
                               &ObjectAttributes,
                               &IoStatus,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0
                             );

        if (!NT_SUCCESS(Status)) {
            ExFreePool(NameBuffer);
            return;
        }

        ZwClose(Handle);

        //
        // Now, for each partition, check if it is marked 'active'
        //
        while (TRUE) {

            PartitionNumber++;

            swprintf(NameBuffer, L"\\Device\\Harddisk%d\\Partition%d", DiskNumber, PartitionNumber);

            RtlInitUnicodeString(&UnicodeString, NameBuffer);

            InitializeObjectAttributes(
                &ObjectAttributes,
                &UnicodeString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );


            Status = ZwCreateFile( &Handle,
                                   (ACCESS_MASK)FILE_GENERIC_READ,
                                   &ObjectAttributes,
                                   &IoStatus,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ,
                                   FILE_OPEN,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   NULL,
                                   0
                                 );

            if (!NT_SUCCESS(Status)) {
                break;
            }

            Status = ZwDeviceIoControlFile(Handle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatus,
                                           IOCTL_DISK_GET_PARTITION_INFO,
                                           NULL,
                                           0,
                                           &PartitionInfo,
                                           sizeof(PARTITION_INFORMATION)
                                          );

            ZwClose(Handle);

            if (!NT_SUCCESS(Status)) {
                break;
            }

            if (PartitionInfo.BootIndicator) {
                wcscpy(NetHDBootPartition, NameBuffer);
                swprintf(DiskNameBuffer, L"\\Device\\Harddisk%d", DiskNumber);
                goto FoundDisk;
            }

        }

        DiskNumber++;
    }

    ASSERT(0); // We only exit the loop with a return (no boot disk found), or jump to the label

FoundDisk:

    //
    // Check each partition for the CSC directory, and find largest free space partition in
    // case no CSC directory exists.
    //

    wcscpy(NetHDCSCPartition, NetHDBootPartition);

    PartitionNumber = 0;
    DestPartitionNumber = 0;
    LargestFreeSpace = RtlConvertUlongToLargeInteger(0);

    while (TRUE) {

        PartitionNumber++;

        //
        // Check for CSC directory first
        //
        swprintf(NameBuffer,
                 L"%ws\\Partition%d%ws",
                 DiskNameBuffer,
                 PartitionNumber,
                 REMOTE_BOOT_IMIRROR_PATH_W REMOTE_BOOT_CSC_SUBDIR_W);

        RtlInitUnicodeString(&UnicodeString, NameBuffer);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );


        Status = ZwOpenFile( &Handle,
                             FILE_GENERIC_READ,
                             &ObjectAttributes,
                             &IoStatus,
                             FILE_SHARE_READ,
                             FILE_DIRECTORY_FILE
                           );

        if (NT_SUCCESS(Status)) {
            ZwClose(Handle);
            DestPartitionNumber = PartitionNumber;
            break;
        }

        if (Status == STATUS_UNRECOGNIZED_VOLUME) {
            //
            // Skip this one
            //
            continue;
        }

        //
        // Now get this partition's free space, if it is an NTFS partition.
        //
        swprintf(NameBuffer, L"%ws\\Partition%d", DiskNameBuffer, PartitionNumber);

        RtlInitUnicodeString(&UnicodeString, NameBuffer);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = ZwCreateFile( &Handle,
                               FILE_GENERIC_READ,
                               &ObjectAttributes,
                               &IoStatus,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_FREE_SPACE_QUERY,
                               NULL,
                               0
                             );

        if (!NT_SUCCESS(Status)) {
            break;
        }

        Status = ZwDeviceIoControlFile(Handle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatus,
                                       IOCTL_DISK_GET_PARTITION_INFO,
                                       NULL,
                                       0,
                                       &PartitionInfo,
                                       sizeof(PARTITION_INFORMATION)
                                      );

        if (!NT_SUCCESS(Status) || (PartitionInfo.PartitionType != PARTITION_IFS)) {
            ZwClose(Handle);
            continue;
        }


        Status = ZwQueryVolumeInformationFile(
                    Handle,
                    &IoStatus,
                    &SizeInfo,
                    sizeof(SizeInfo),
                    FileFsSizeInformation
                    );

        ZwClose(Handle);

        if(NT_SUCCESS(Status)) {

            //
            // Calculate the amount of free space on the drive.
            //
            FreeSpace = RtlExtendedIntegerMultiply(
                            SizeInfo.AvailableAllocationUnits,
                            SizeInfo.SectorsPerAllocationUnit * SizeInfo.BytesPerSector
                            );

            if (RtlLargeIntegerGreaterThan(FreeSpace, LargestFreeSpace)) {
                LargestFreeSpace = FreeSpace;
                DestPartitionNumber = PartitionNumber;
            }

        }

    }

    if (DestPartitionNumber != 0) {
        swprintf(NetHDCSCPartition, L"%ws\\Partition%d", DiskNameBuffer, DestPartitionNumber);
    }

    ExFreePool(NameBuffer);

    return;
}

VOID
IoStartCscForTextmodeSetup (
    IN BOOLEAN Upgrade
    )
{
    NTSTATUS status;
    PWSTR NetHDCSCPartition;
    PUCHAR buffer;
    IO_STATUS_BLOCK ioStatusBlock;

    if (RdrHandle == NULL) {
        ASSERT(FALSE);
        return;
    }
    if (!ExpInTextModeSetup) {
        ASSERT(FALSE);
        return;
    }
    if (TextmodeSetupHasStartedCsc) {
        ASSERT(FALSE);
        return;
    }

    NetHDCSCPartition = ExAllocatePoolWithTag(
                            NonPagedPool,
                            (80 + MAX_PATH) * sizeof(WCHAR),
                            'bRoI'
                            );
    if (NetHDCSCPartition == NULL) {
        KdPrint(( "IoStartCscForTextmodeSetup: Unable to allocate buffer\n"));
        return;
    }
    buffer = (PUCHAR)(NetHDCSCPartition + 80);

    //
    // Get the path to the boot partition on the disk for CSC and redirection
    // Note: On failure, this defaults to \Device\Harddisk0\Partition1
    //

    IopGetHarddiskInfo(NetHDCSCPartition);

    //
    // Tell the redirector to initialize remote boot redirection (back
    // to the local disk).
    //

    KeAttachProcess( RdrHandleProcess );

    status = ZwFsControlFile(
                RdrHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_LMR_START_RBR,
                NetHDCSCPartition,
                wcslen(NetHDCSCPartition) * sizeof(WCHAR),
                NULL,
                0
                );

    if (NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        KdPrint(( "IoStartCscForTextmodeSetup: Unable to FSCTL(RBR) redirector: %x\n", status ));
    }

    wcstombs(buffer, NetHDCSCPartition, wcslen(NetHDCSCPartition) + 1);
    strcat(buffer, REMOTE_BOOT_IMIRROR_PATH_A REMOTE_BOOT_CSC_SUBDIR_A);

    StartCsc = INIT_CSC;
    status = IopInitCsc( buffer );

    //
    // If this is not an upgrade, or if initialization of CSC failed,
    // reset the cache.
    //

    if ( NT_SUCCESS(status) && (!Upgrade || (StartCsc == FLUSH_CSC)) ) {

        status = IopResetCsc( buffer );

        if ( !NT_SUCCESS(status) ) {
            KdPrint(("IoStartCscForTextmodeSetup: reset of Csc failed %x\n", status));
        }
    }

    if ( !NT_SUCCESS(status) ) {
        KdPrint(("IoStartCscForTextmodeSetup: initialization of Csc failed %x\n", status));
        IoCscInitializationFailed = TRUE;
        SharedUserData->SystemFlags |= SYSTEM_FLAG_DISKLESS_CLIENT;
    } else {
        IoCscInitializationFailed = FALSE;
        SharedUserData->SystemFlags &= ~SYSTEM_FLAG_DISKLESS_CLIENT;
    }

    KeDetachProcess();

    ExFreePool( NetHDCSCPartition );

    return;

} // IoStartCscForTextModeSetup
#endif // defined(REMOTE_BOOT)

