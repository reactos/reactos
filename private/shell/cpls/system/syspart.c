/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    syspart.c

Abstract:

    Routines to determine the system partition on x86 machines.

Author:

    Ted Miller (tedm) 30-June-1994

Revision History:

    24-Apr-1997 scotthal
        re-horked for system applet

--*/

#include "sysdm.h"
#include <ntdddisk.h>
#include <tchar.h>

#define ISNT() TRUE
#define MALLOC(s) LocalAlloc(LPTR, (s))
#define REALLOC(p, s) LocalReAlloc((HLOCAL) (p), (s), 0L)
#define FREE(p) LocalFree((HLOCAL) (p))

BOOL g_fInitialized = FALSE;

//
// NT-specific routines we use from ntdll.dll
//
//NTSYSAPI
NTSTATUS
(NTAPI *NtOpenSymLinkRoutine)(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtQuerSymLinkRoutine)(
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtQuerDirRoutine) (
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtOpenDirRoutine) (
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

BOOL
MatchNTSymbolicPaths(
    PCTSTR lpDeviceName,
    PCTSTR lpSysPart,
    PCTSTR lpMatchName
    );


IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}

BOOL
GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )

/*++

Routine Description:

    Fill in a PARTITION_INFORMATION structure with information about
    a particular drive.

    This routine is meaningful only when run on NT -- it always fails
    on Win95.

Arguments:

    Drive - supplies drive letter whose partition info is desired.

    PartitionInfo - upon success, receives partition info for Drive.

Return Value:

    Boolean value indicating whether PartitionInfo has been filled in.

--*/
{
    TCHAR DriveName[] = TEXT("\\\\.\\?:");
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    if(!ISNT()) {
        return(FALSE);
    }

    DriveName[4] = Drive;

    hDisk = CreateFile(
                DriveName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &DataSize,
            NULL
            );

    CloseHandle(hDisk);

    return(b);
}

UINT
MyGetDriveType(
    IN TCHAR Drive
    )

/*++

Routine Description:

    Same as GetDriveType() Win32 API except on NT returns
    DRIVE_FIXED for removeable hard drives.

Arguments:

    Drive - supplies drive letter whose type is desired.

Return Value:

    Same as GetDriveType().

--*/
{
    TCHAR DriveNameNt[] = TEXT("\\\\.\\?:");
    TCHAR DriveName[] = TEXT("?:\\");
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type. If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    //
    DriveName[0] = Drive;
    rc = GetDriveType(DriveName);

#ifdef _X86_ //NEC98
    if((rc != DRIVE_REMOVABLE) || !ISNT() || (!IsNEC98() && (Drive < L'C'))) {
        return(rc);
    }
#else //NEC98
    if((rc != DRIVE_REMOVABLE) || !ISNT() || (Drive < L'C')) {
        return(rc);
    }
#endif

    //
    // DRIVE_REMOVABLE on NT.
    //

    //
    // Disallow use of removable media (e.g. Jazz, Zip, ...).
    //
#if 0

    DriveNameNt[4] = Drive;

    hDisk = CreateFile(
                DriveNameNt,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk != INVALID_HANDLE_VALUE) {

        b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                &MediaInfo,
                sizeof(MediaInfo),
                &DataSize,
                NULL
                );

        //
        // It's really a hard disk if the media type is removable.
        //
        if(b && (MediaInfo.MediaType == RemovableMedia)) {
            rc = DRIVE_FIXED;
        }

        CloseHandle(hDisk);
    }
#endif

    return(rc);
}

BOOL
ArcPathToNtPath(
    IN  LPCTSTR ArcPath,
    OUT LPTSTR  NtPath,
    IN  UINT    NtPathBufferLen
    )
{
    WCHAR arcPath[256];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ObjectHandle;
    NTSTATUS Status;
    WCHAR Buffer[512];

    PWSTR ntPath;

    lstrcpyW(arcPath,L"\\ArcName\\");
#ifdef UNICODE
    lstrcpynW(arcPath+9,ArcPath,sizeof(arcPath)/sizeof(WCHAR));
#else
    MultiByteToWideChar(
        CP_ACP,
        0,
        ArcPath,
        -1,
        arcPath+9,
        (sizeof(arcPath)/sizeof(WCHAR))-9
        );
#endif

    UnicodeString.Buffer = arcPath;
    UnicodeString.Length = (USHORT) (lstrlenW(arcPath)*sizeof(WCHAR));
    UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

    InitializeObjectAttributes(
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = (*NtOpenSymLinkRoutine)(
                &ObjectHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Obja
                );

    if(NT_SUCCESS(Status)) {
        //
        // Query the object to get the link target.
        //
        UnicodeString.Buffer = Buffer;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = sizeof(Buffer)-sizeof(WCHAR);

        Status = (*NtQuerSymLinkRoutine)(ObjectHandle,&UnicodeString,NULL);

        CloseHandle(ObjectHandle);

        if(NT_SUCCESS(Status)) {

            Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

#ifdef UNICODE
            lstrcpyn(NtPath,Buffer,NtPathBufferLen);
#else
            WideCharToMultiByte(CP_ACP,0,Buffer,-1,NtPath,NtPathBufferLen,NULL,NULL);
#endif

            return(TRUE);
        }
    }

    return(FALSE);
}


BOOL
AppearsToBeSysPart(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout,
    IN TCHAR                     Drive
    )
{
    PARTITION_INFORMATION PartitionInfo,*p;
    BOOL IsPrimary;
    UINT i;
    DWORD d;

    LPTSTR BootFiles[] = { TEXT("BOOT.INI"),
                           TEXT("NTLDR"),
                           TEXT("NTDETECT.COM"),
                           NULL
                         };

    TCHAR FileName[64];

    //
    // Get partition information for this partition.
    //
    if(!GetPartitionInfo(Drive,&PartitionInfo)) {
        return(FALSE);
    }

    //
    // See if the drive is a primary partition.
    //
    IsPrimary = FALSE;
    for(i=0; i<min(DriveLayout->PartitionCount,4); i++) {

        p = &DriveLayout->PartitionEntry[i];

        if((p->PartitionType != PARTITION_ENTRY_UNUSED)
        && (p->StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
        && (p->PartitionLength.QuadPart == PartitionInfo.PartitionLength.QuadPart)) {

            IsPrimary = TRUE;
            break;
        }
    }

    if(!IsPrimary) {
        return(FALSE);
    }

    //
    // Don't rely on the active partition flag.  This could easily not be accurate
    // (like user is using os/2 boot manager, for example).
    //

    //
    // See whether all NT boot files are present on this drive.
    //
    for(i=0; BootFiles[i]; i++) {

        wsprintf(FileName,TEXT("%c:\\%s"),Drive,BootFiles[i]);

        d = GetFileAttributes(FileName);
        if(d == (DWORD)(-1)) {
            return(FALSE);
        }
    }

    return(TRUE);
}


DWORD
QueryHardDiskNumber(
    IN  TCHAR   DriveLetter
    )

{
    TCHAR                   driveName[10];
    HANDLE                  h;
    BOOL                    b;
    STORAGE_DEVICE_NUMBER   number;
    DWORD                   bytes;

    driveName[0] = '\\';
    driveName[1] = '\\';
    driveName[2] = '.';
    driveName[3] = '\\';
    driveName[4] = DriveLetter;
    driveName[5] = ':';
    driveName[6] = 0;

    h = CreateFile(driveName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return (DWORD) -1;
    }

    b = DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                        &number, sizeof(number), &bytes, NULL);
    CloseHandle(h);

    if (!b) {
        return (DWORD) -1;
    }

    return number.DeviceNumber;
}

TCHAR
x86DetermineSystemPartition(
    IN  HWND   ParentWindow
    )

/*++

Routine Description:

    Determine the system partition on x86 machines.

    On Win95, we always return C:. For NT, read on.

    The system partition is the primary partition on the boot disk.
    Usually this is the active partition on disk 0 and usually it's C:.
    However the user could have remapped drive letters and generally
    determining the system partition with 100% accuracy is not possible.

    The one thing we can be sure of is that the system partition is on
    the physical hard disk with the arc path multi(0)disk(0)rdisk(0).
    We can be sure of this because by definition this is the arc path
    for bios drive 0x80.

    This routine determines which drive letters represent drives on
    that physical hard drive, and checks each for the nt boot files.
    The first drive found with those files is assumed to be the system
    partition.

    If for some reason we cannot determine the system partition by the above
    method, we simply assume it's C:.

Arguments:

    ParentWindow - supplies window handle for window to be the parent for
        any dialogs, etc.

    SysPartDrive - if successful, receives drive letter of system partition.

Return Value:

    Boolean value indicating whether SysPartDrive has been filled in.
    If FALSE, the user will have been infomred about why.

--*/

{
    TCHAR DriveName[3];
    BOOL  GotIt;
    TCHAR NtDevicePath[256];
    DWORD NtDevicePathLen;
    LPTSTR p;
    DWORD PhysicalDriveNumber;
    TCHAR Buffer[512];
    TCHAR FoundSystemPartition[20], temp[5];
    HANDLE hDisk;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    DWORD DriveLayoutSize;
    TCHAR Drive;
    BOOL b;
    DWORD DataSize;
    DWORD BootPartitionNumber, cnt;
    PPARTITION_INFORMATION Start, i;

    if (!g_fInitialized) {
        GotIt = FALSE;
        goto c0;
    }

    if(IsNEC98()) {
        GetWindowsDirectory(Buffer,sizeof(Buffer)/sizeof(TCHAR));
        return(Buffer[0]);
    }

    if(!ISNT()) {
       return(TEXT('C'));
    }

    DriveName[1] = TEXT(':');
    DriveName[2] = 0;

    //
    // The system partition must be on multi(0)disk(0)rdisk(0)
    // If we can't translate that ARC path then something is really wrong.
    // We assume C: because we don't know what else to do.
    //
    b = ArcPathToNtPath(
            TEXT("multi(0)disk(0)rdisk(0)"),
            NtDevicePath,
            sizeof(NtDevicePath)/sizeof(TCHAR)
            );

    if(!b) {

        //
        // Missed.  Try scsi(0) in case the user is using ntbootdd.sys
        //
        b = ArcPathToNtPath(
                TEXT("scsi(0)disk(0)rdisk(0)"),
                NtDevicePath,
                sizeof(NtDevicePath)/sizeof(TCHAR)
                );
        if(!b) {
            GotIt = FALSE;
            goto c0;
        }
    }

    //
    // The arc path for a disk device is usually linked
    // to partition0. Get rid of the partition part of the name.
    //
    CharLower(NtDevicePath);
    if(p = _tcsstr(NtDevicePath,TEXT("\\partition"))) {
        *p = 0;
    }

    NtDevicePathLen = lstrlen(NtDevicePath);

    //
    // Determine the physical drive number of this drive.
    // If the name is not of the form \device\harddiskx then
    // something is very wrong. Just assume we don't understand
    // this device type, and return C:.
    //
    if(memcmp(NtDevicePath,TEXT("\\device\\harddisk"),16*sizeof(TCHAR))) {
        Drive = TEXT('C');
        GotIt = TRUE;
        goto c0;
    }

    PhysicalDriveNumber = _tcstoul(NtDevicePath+16,NULL,10);
    wsprintf(Buffer,TEXT("\\\\.\\PhysicalDrive%u"),PhysicalDriveNumber);

    //
    // Get drive layout info for this physical disk.
    // If we can't do this something is very wrong.
    //
    hDisk = CreateFile(
                Buffer,
                FILE_READ_ATTRIBUTES | FILE_READ_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk == INVALID_HANDLE_VALUE) {
        GotIt = FALSE;
        goto c0;
    }

    //
    // Get partition information.
    //
    DriveLayout = MALLOC(1024);
    DriveLayoutSize = 1024;
    if(!DriveLayout) {
        GotIt = FALSE;
        goto c1;
    }

    _tcscpy( FoundSystemPartition, TEXT("Partition") );

    retry:

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT,
            NULL,
            0,
            (PVOID)DriveLayout,
            DriveLayoutSize,
            &DataSize,
            NULL
            );

    if(!b) {

        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            DriveLayoutSize += 1024;
            if(p = REALLOC((PVOID)DriveLayout,DriveLayoutSize)) {
                (PVOID)DriveLayout = p;
            } else {
                GotIt = FALSE;
                goto c2;
            }
            goto retry;
        } else {
            GotIt = FALSE;
            goto c2;
        }
    }else{
        // Now walk the Drive_Layout to find the boot partition
        
        Start = DriveLayout->PartitionEntry;
        cnt = 0;

        for( i = Start; cnt < DriveLayout->PartitionCount; i++ ){
            cnt++;
            if( i->BootIndicator == TRUE ){
                BootPartitionNumber = i->PartitionNumber;
                _ultot( BootPartitionNumber, temp, 10 );
                _tcscat( FoundSystemPartition, temp );
                break;
            }
        }

    }

    //
    // The system partition can only be a drive that is on
    // this disk.  We make this determination by looking at NT drive names
    // for each drive letter and seeing if the NT equivalent of
    // multi(0)disk(0)rdisk(0) is a prefix.
    //
    GotIt = FALSE;
    for(Drive=TEXT('A'); Drive<=TEXT('Z'); Drive++) {

        if(MyGetDriveType(Drive) == DRIVE_FIXED) {

            DriveName[0] = Drive;

            if(QueryDosDevice(DriveName,Buffer,sizeof(Buffer)/sizeof(TCHAR))) {

                if( MatchNTSymbolicPaths(NtDevicePath,FoundSystemPartition,Buffer)) {
                    //
                    // Now look to see whether there's an nt boot sector and
                    // boot files on this drive.
                    //
                    if(AppearsToBeSysPart(DriveLayout,Drive)) {
                        GotIt = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if(!GotIt) {
        //
        // Strange case, just assume C:
        //
        GotIt = TRUE;
        Drive = TEXT('C');
    }

c2:
    if (DriveLayout) {
        FREE(DriveLayout);
    }
c1:
    CloseHandle(hDisk);
c0:
    if(GotIt) {
        return(Drive);
    }
    else {
        return(TEXT('C'));
    }
}

BOOL
InitializeArcStuff(
    )
{
    HMODULE NtdllLib;

    if(ISNT()) {
        //
        // On NT ntdll.dll had better be already loaded.
        //
        NtdllLib = LoadLibrary(TEXT("NTDLL"));
        if(!NtdllLib) {

            return(FALSE);

        }

        (FARPROC)NtOpenSymLinkRoutine = GetProcAddress(NtdllLib,"NtOpenSymbolicLinkObject");
        (FARPROC)NtQuerSymLinkRoutine = GetProcAddress(NtdllLib,"NtQuerySymbolicLinkObject");
        (FARPROC)NtOpenDirRoutine = GetProcAddress(NtdllLib,"NtOpenDirectoryObject");
        (FARPROC)NtQuerDirRoutine = GetProcAddress(NtdllLib,"NtQueryDirectoryObject");


        if(!NtOpenSymLinkRoutine || !NtQuerSymLinkRoutine || !NtOpenDirRoutine || !NtQuerDirRoutine) {

            FreeLibrary(NtdllLib);

            return(FALSE);
        }

        //
        // We don't need the extraneous handle any more.
        //
        FreeLibrary(NtdllLib);
    }

    return(g_fInitialized = TRUE);
}


BOOL
MatchNTSymbolicPaths(
    PCTSTR lpDeviceName,
    PCTSTR lpSysPart,
    PCTSTR lpMatchName
    )
/*
    
    Introduced this routine to mend the old way of finding if we determined the right system partition.
   
   Arguments:
    lpDeviceName    -  This should be the symbolic link (\Device\HarddiskX) name for the arcpath 
                       multi/scsi(0)disk(0)rdisk(0) which is the arcpath for bios drive 0x80.  
                       Remember that we strip the PartitionX part to get just \Device\HarddiskX.
                       
    lpSysPart       -  When we traverse the lpDeviceName directory we compare the symbolic link corresponding to
                       lpSysPart and see if it matches lpMatchName
                       
    lpMatchName     -  This is the symbolic name that a drive letter translates to (got by calling 
                       QueryDosDevice() ). 
                       
   So it boils down to us trying to match a drive letter to the system partition on bios drive 0x80.


*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle, SymLinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN RestartScan, ret;
    UCHAR DirInfoBuffer[ 512 ];
    WCHAR Buffer[1024];
    WCHAR pDevice[512], pMatch[512], pSysPart[20];
    ULONG Context = 0;
    ULONG ReturnedLength;
    


#ifdef UNICODE
    lstrcpyW( pDevice,lpDeviceName);
    lstrcpyW( pMatch,lpMatchName);
    lstrcpyW( pSysPart,lpSysPart);
#else
    MultiByteToWideChar(
        CP_ACP,
        0,
        lpDeviceName,
        -1,
        pDevice,
        (sizeof(pDevice)/sizeof(WCHAR))
        );
    MultiByteToWideChar(
        CP_ACP,
        0,
        lpMatchName,
        -1,
        pMatch,
        (sizeof(pMatch)/sizeof(WCHAR))
        );
    MultiByteToWideChar(
        CP_ACP,
        0,
        lpSysPart,
        -1,
        pSysPart,
        (sizeof(pSysPart)/sizeof(WCHAR))
        );
#endif


    UnicodeString.Buffer = pDevice;
    UnicodeString.Length = (USHORT)(lstrlenW(pDevice)*sizeof(WCHAR));
    UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = (*NtOpenDirRoutine)( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (!NT_SUCCESS( Status ))
        return(FALSE);
        

    RestartScan = TRUE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
    ret = FALSE;
    while (TRUE) {
        Status = (*NtQuerDirRoutine)( DirectoryHandle,
                                         (PVOID)DirInfo,
                                         sizeof( DirInfoBuffer ),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &ReturnedLength
                                       );

        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
                }

            break;
            }

        if (!wcsncmp( DirInfo->TypeName.Buffer, L"SymbolicLink", DirInfo->TypeName.Length ) && 
            !_wcsnicmp( DirInfo->Name.Buffer, pSysPart, DirInfo->Name.Length ) ) {


            UnicodeString.Buffer = DirInfo->Name.Buffer;
            UnicodeString.Length = (USHORT)(lstrlenW(DirInfo->Name.Buffer)*sizeof(WCHAR));
            UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);
            InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                DirectoryHandle,
                                NULL
                              );


            Status = (*NtOpenSymLinkRoutine)(
                &SymLinkHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Attributes
                );

            if(NT_SUCCESS(Status)) {
                //
                // Query the object to get the link target.
                //
                UnicodeString.Buffer = Buffer;
                UnicodeString.Length = 0;
                UnicodeString.MaximumLength = sizeof(Buffer)-sizeof(WCHAR);
        
                Status = (*NtQuerSymLinkRoutine)(SymLinkHandle,&UnicodeString,NULL);
        
                CloseHandle(SymLinkHandle);
                
                if( NT_SUCCESS(Status)){
            
                    if (!_wcsnicmp(UnicodeString.Buffer, pMatch, (UnicodeString.Length/sizeof(WCHAR)))) {
                        ret = TRUE;
                        break;
                    }
                }
            
            }

        }

        RestartScan = FALSE;
    }
    CloseHandle( DirectoryHandle );

    return( ret );
}





