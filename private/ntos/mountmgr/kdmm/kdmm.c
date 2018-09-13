/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdmm.c

Abstract:

    Mount mgr driver KD extension - based on Vert's skeleton

Author:

    John Vert (jvert) 6-Aug-1992

Revision History:

--*/

#include "precomp.h"

//
// globals
//

EXT_API_VERSION        ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;

#define TrueOrFalse( _x )  ( _x ? "True" : "False" )

/* forwards */

BOOL
ReadTargetMemory(
    PVOID TargetAddress,
    PVOID LocalBuffer,
    ULONG BytesToRead
    );

__inline PCHAR
ListInUse(
    PLIST_ENTRY
    );

__inline PCHAR
TrueFalse(
    BOOLEAN Value
    );

/* end forwards */

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            SavedMajorVersion == 0x0c ? "Checked" : "Free",
            SavedMinorVersion
            );
}

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

DECLARE_API( dumpdb )
/*
 *   dump the mount mgr database
 */
{
    PDEVICE_EXTENSION TargetExt;
    DEVICE_EXTENSION LocalExt;
    PMOUNTED_DEVICE_INFORMATION TargetDevInfo;
    PMOUNTED_DEVICE_INFORMATION LastDevInfo;
    MOUNTED_DEVICE_INFORMATION LocalDevInfo;
    MOUNTDEV_UNIQUE_ID LocalUniqueId;
    PSYMBOLIC_LINK_NAME_ENTRY TargetSymLink;
    PSYMBOLIC_LINK_NAME_ENTRY LastSymLink;
    SYMBOLIC_LINK_NAME_ENTRY LocalSymLink;
    WCHAR NameBuffer[512];
    UCHAR UniqueIdBuffer[512];
    PUCHAR pUniqueId;

    //
    // convert address of extension in target machine
    //

    TargetExt = (PDEVICE_EXTENSION)GetExpression( args );

    if ( !TargetExt ) {

        dprintf("bad string conversion (%s) \n", args );
        return;
    }

    //
    // read in extension from target machine
    //

    if ( !ReadTargetMemory((PVOID)TargetExt,
                           (PVOID)&LocalExt,
                           sizeof(DEVICE_EXTENSION))) {
        return;
    }

    TargetDevInfo = (PMOUNTED_DEVICE_INFORMATION)LocalExt.MountedDeviceList.Flink;
    LastDevInfo = (PMOUNTED_DEVICE_INFORMATION)&TargetExt->MountedDeviceList.Flink;

    while ( TargetDevInfo != LastDevInfo ) {

        if (CheckControlC()) {
            return;
        }

        if ( !ReadTargetMemory(TargetDevInfo,
                               &LocalDevInfo,
                               sizeof( MOUNTED_DEVICE_INFORMATION )))
        {
            dprintf("Problem reading mounted device info at %08X\n", TargetDevInfo );
            return;
        }

        dprintf( "Mounted Device Info @ %08X\n", TargetDevInfo );

        if ( ReadTargetMemory((PVOID)LocalDevInfo.NotificationName.Buffer,
                              (PVOID)&NameBuffer,
                              LocalDevInfo.NotificationName.Length)) {

            dprintf( "    NotificationName: %.*ws\n",
                     LocalDevInfo.NotificationName.Length/sizeof(WCHAR),
                     NameBuffer);
        } else {
            dprintf( "    NotificationName @ %08X\n", LocalDevInfo.NotificationName.Buffer );
        }

        if ( ReadTargetMemory((PVOID)LocalDevInfo.UniqueId,
                              (PVOID)&LocalUniqueId,
                              sizeof(LocalUniqueId))) {

            dprintf( "    Unique ID Length = %u bytes\n    UniqueId",
                     LocalUniqueId.UniqueIdLength);

            if ( ReadTargetMemory((PVOID)LocalDevInfo.UniqueId->UniqueId,
                                  (PVOID)UniqueIdBuffer,
                                  LocalUniqueId.UniqueIdLength)) {
                dprintf(": ");
                pUniqueId = UniqueIdBuffer;
                while ( LocalUniqueId.UniqueIdLength-- )
                    dprintf( "%02X ", *pUniqueId++ );
            } else {
                dprintf( " @ %08X", LocalDevInfo.UniqueId->UniqueId );
            }
            dprintf( "\n" );

        } else {
            dprintf( "    UniqueId @ %08X\n", LocalDevInfo.UniqueId );
        }

        if ( ReadTargetMemory((PVOID)LocalDevInfo.DeviceName.Buffer,
                              (PVOID)&NameBuffer,
                              LocalDevInfo.DeviceName.Length)) {

            dprintf( "    DeviceName: %.*ws\n",
                     LocalDevInfo.DeviceName.Length/sizeof(WCHAR),
                     NameBuffer);
        } else {
            dprintf( "    DeviceName @ %08X\n", LocalDevInfo.DeviceName.Buffer );
        }

        TargetSymLink = (PSYMBOLIC_LINK_NAME_ENTRY)LocalDevInfo.SymbolicLinkNames.Flink;
        LastSymLink = (PSYMBOLIC_LINK_NAME_ENTRY)&TargetDevInfo->SymbolicLinkNames.Flink;

        while ( TargetSymLink != LastSymLink ) {

            if (CheckControlC()) {
                return;
            }

            if ( !ReadTargetMemory(TargetSymLink,
                                   &LocalSymLink,
                                   sizeof( SYMBOLIC_LINK_NAME_ENTRY )))
            {
                dprintf("Problem reading symlink entry at %08X\n", TargetSymLink );
                return;
            }

            if ( ReadTargetMemory((PVOID)LocalSymLink.SymbolicLinkName.Buffer,
                                  (PVOID)&NameBuffer,
                                  LocalSymLink.SymbolicLinkName.Length)) {

                dprintf( "        SymbolicLinkName: %.*ws\n",
                         LocalSymLink.SymbolicLinkName.Length/sizeof(WCHAR),
                         NameBuffer);
            } else {
                dprintf( "        SymbolicLinkName @ %08X\n", LocalSymLink.SymbolicLinkName.Buffer );
            }

            dprintf( "        IsInDatabase = %s\n", LocalSymLink.IsInDatabase ? "TRUE" : "FALSE" );

            TargetSymLink = (PSYMBOLIC_LINK_NAME_ENTRY)LocalSymLink.ListEntry.Flink;
        }

        TargetDevInfo = (PMOUNTED_DEVICE_INFORMATION)LocalDevInfo.ListEntry.Flink;
    }
}

BOOL
ReadTargetMemory(
    PVOID TargetAddress,
    PVOID LocalBuffer,
    ULONG BytesToRead
    )
{
    BOOL success;
    ULONG BytesRead;

    success = ReadMemory((ULONG)TargetAddress, LocalBuffer, BytesToRead, &BytesRead);

    if (success) {

        if (BytesRead != BytesToRead) {

            dprintf("wrong byte count. expected=%d, read =%d\n", BytesToRead, BytesRead);
        }

    } else {
        dprintf("Problem reading memory at %08X for %u bytes\n",
                TargetAddress, BytesToRead);

        success = FALSE;
    }

    return success;
}

__inline PCHAR
ListInUse(
    PLIST_ENTRY ListToCheck
    )
{
    return ListToCheck->Flink == ListToCheck->Blink ? "(empty)" : "";
}

DECLARE_API( help )
{
    dprintf("Mountmgr kd extensions\n\n");
    dprintf("dumpdb <mntmgr dev extension addr>- dump the mount mgr database\n");
    dprintf( "      use !devobj mountpointmanager to get dev extension addr\n");
}
