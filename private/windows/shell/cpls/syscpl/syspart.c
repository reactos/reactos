/** FILE: syspart.c ******** Module Header ********************************
 *
 *  Control panel applet for System configuration.  This file holds
 *  everything to do with finding the system partition where the
 *  boot.ini file for x86 systems resides.
 *
 * History:
 *  08 Sept 1994  -by- Steve Cathcart [stevecat]
 *        Took base routines from TedM's SETUP code
 *  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update - SUR release NT v4.0
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#include "stdio.h"
#include "stdlib.h"

// For NT apis
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntdddisk.h"

// For Windows apis
#include "windows.h"


//==========================================================================
//                            Local Definitions
//==========================================================================

#define MALLOC( size )            Malloc( size )
#define REALLOC( block, size )    Realloc( (block), (size) )
#define FREE( block )             Free( &(block) )


//==========================================================================
//                            Typedefs and Structs
//==========================================================================


//==========================================================================
//                            External Declarations
//==========================================================================
/* Functions */
extern void  ErrMemDlg( HWND hParent );


//==========================================================================
//                     Local Data Declarations
//==========================================================================


//==========================================================================
//                      Local Function Prototypes
//==========================================================================


#ifdef _X86_


///////////////////////////////////////////////////////////////////////////////
//
//  Routine Description:
//
//    Allocates memory and fatal errors if none is available.
//
//  Arguments:
//
//    Size - number of bytes to allocate
//
//  Return Value:
//
//    Pointer to memory.
//
///////////////////////////////////////////////////////////////////////////////

PVOID Malloc( IN DWORD Size )
{
    PVOID p;

    if( ( p = (PVOID) LocalAlloc( LPTR, Size ) ) == NULL )
    {
        ErrMemDlg( NULL );
    }

    return( p );
}



///////////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//    Free a block of memory previously allocated with Malloc().
//
// Arguments:
//
//    Block - supplies pointer to block to free.
//
// Return Value:
//
//    None.
//
///////////////////////////////////////////////////////////////////////////////

VOID Free( IN OUT PVOID *Block )
{
    LocalFree( (HLOCAL) *Block );
    *Block = NULL;
}



///////////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//    Reallocates a block of memory previously allocated with Malloc();
//    fatal errors if none is available.
//
// Arguments:
//
//    Block - supplies pointer to block to resize
//
//    Size - number of bytes to allocate
//
// Return Value:
//
//    Pointer to memory.
//
///////////////////////////////////////////////////////////////////////////////

PVOID Realloc( IN PVOID Block, IN DWORD Size )
{
    PVOID p;

    if( ( p = LocalReAlloc( (HLOCAL) Block, Size, 0 ) ) == NULL )
    {
        ErrMemDlg( NULL );
    }

    return( p );
}


BOOL GetPartitionInfo( IN  TCHAR                  Drive,
                       OUT PPARTITION_INFORMATION PartitionInfo )
{
    TCHAR DriveName[] = TEXT("\\\\.\\?:");
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    DriveName[4] = Drive;

    hDisk = CreateFile( DriveName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if( hDisk == INVALID_HANDLE_VALUE )
    {
        return( FALSE );
    }

    b = DeviceIoControl( hDisk,
                         IOCTL_DISK_GET_PARTITION_INFO,
                         NULL,
                         0,
                         PartitionInfo,
                         sizeof( PARTITION_INFORMATION ),
                         &DataSize,
                         NULL );

    CloseHandle( hDisk );

    return( b );
}


UINT MyGetDriveType( IN TCHAR Drive )
{
    TCHAR DriveNameNt[] = TEXT("\\\\.\\?:");
    TCHAR DriveName[] = TEXT("?:\\");
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk.  Otherwise
    // just believe the api.
    //
    //

    DriveName[0] = Drive;

    if( (rc = GetDriveType( DriveName ) ) == DRIVE_REMOVABLE )
    {
        DriveNameNt[ 4 ] = Drive;

        hDisk = CreateFile ( DriveNameNt,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL );

        if( hDisk != INVALID_HANDLE_VALUE )
        {
            b = DeviceIoControl( hDisk,
                                 IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                 NULL,
                                 0,
                                 &MediaInfo,
                                 sizeof( MediaInfo ),
                                 &DataSize,
                                 NULL );

            //
            // It's really a hard disk if the media type is removable.
            //

            if( b && ( MediaInfo.MediaType == RemovableMedia ) )
            {
                rc = DRIVE_FIXED;
            }

            CloseHandle( hDisk );
        }
    }

    return( rc );
}


PWSTR ArcPathToNtPath( IN PWSTR ArcPath )
{
    NTSTATUS Status;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    UCHAR Buffer[ 1024 ];
    PWSTR arcPath;
    PWSTR ntPath;

    //
    // Assume failure
    //

    ntPath = NULL;

    arcPath = MALLOC( ( (wcslen( ArcPath ) + 1 ) * sizeof( WCHAR ) )
                       + sizeof( TEXT( "\\ArcName" ) ) );

    wcscpy( arcPath, TEXT( "\\ArcName\\" ) );
    wcscat( arcPath, ArcPath );

    RtlInitUnicodeString( &UnicodeString, arcPath );

    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenSymbolicLinkObject( &ObjectHandle,
                                       READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                       &Obja );

    if( NT_SUCCESS( Status ) )
    {
        //
        // Query the object to get the link target.
        //

        UnicodeString.Buffer = (PWSTR) Buffer;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = sizeof( Buffer );

        Status = NtQuerySymbolicLinkObject ( ObjectHandle,
                                             &UnicodeString,
                                             NULL );

        if( NT_SUCCESS( Status ) )
        {
            ntPath = MALLOC( UnicodeString.Length + sizeof( WCHAR ) );

            CopyMemory( ntPath, UnicodeString.Buffer, UnicodeString.Length );

            ntPath[ UnicodeString.Length / sizeof( WCHAR ) ] = 0;
        }

        NtClose( ObjectHandle );
    }

    FREE( arcPath );

    return( ntPath );
}


BOOL AppearsToBeSysPart( IN PDRIVE_LAYOUT_INFORMATION DriveLayout,
                         IN WCHAR                     Drive )
{
    PARTITION_INFORMATION PartitionInfo,*p;
    BOOL IsPrimary;
    unsigned i;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    PTSTR BootFiles[] = { TEXT( "BOOT.INI" ),
                          TEXT( "NTLDR" ),
                          TEXT( "NTDETECT.COM" ),
                          NULL
                        };

    TCHAR FileName[ 64 ];

    //
    // Get partition information for this partition.
    //

    if( !GetPartitionInfo( (TCHAR) Drive, &PartitionInfo ) )
    {
        return( FALSE );
    }

    //
    // See if the drive is a primary partition.
    //

    IsPrimary = FALSE;

    for( i = 0; i < min( DriveLayout->PartitionCount , 4 ); i++ )
    {
        p = &DriveLayout->PartitionEntry[ i ];

        if((p->PartitionType != PARTITION_ENTRY_UNUSED)
          && (p->StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
          && (p->PartitionLength.QuadPart == PartitionInfo.PartitionLength.QuadPart))
        {
            IsPrimary = TRUE;
            break;
        }
    }

    if( !IsPrimary )
    {
        return( FALSE );
    }

    //
    // Don't rely on the active partition flag.  This could easily not be
    // accurate (like user is using os/2 boot manager, for example).
    //

    //
    // See whether an nt boot files are present on this drive.
    //

    for( i = 0; BootFiles[ i ]; i++ )
    {
        wsprintf( FileName, TEXT( "%wc:\\%s" ), Drive, BootFiles[ i ] );

        FindHandle = FindFirstFile( FileName, &FindData );

        if( FindHandle == INVALID_HANDLE_VALUE )
        {
            return( FALSE );
        }
        else
        {
            FindClose( FindHandle );
        }
    }

    return( TRUE );
}



///////////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//    Determine the system partition on x86 machines.
//
//    The system partition is the primary partition on the boot disk.
//    Usually this is the active partition on disk 0 and usually it's C:.
//    However the user could have remapped drive letters and generally
//    determining the system partition with 100% accuracy is not possible.
//
//    The one thing we can be sure of is that the system partition is on
//    the physical hard disk with the arc path multi(0)disk(0)rdisk(0).
//    We can be sure of this because by definition this is the arc path
//    for bios drive 0x80.
//
//    This routine determines which drive letters represent drives on
//    that physical hard drive, and checks each for the nt boot files.
//    The first drive found with those files is assumed to be the system
//    partition.
//
//    If for some reason we cannot determine the system partition by the above
//    method, we simply assume it's C:.
//
// Arguments:
//
//    hdlg - Handle of topmost window currently being displayed. (unused)
//
// Return Value:
//
//    Drive letter of system partition.
//
///////////////////////////////////////////////////////////////////////////////

TCHAR x86DetermineSystemPartition( IN HWND hdlg )
{
    BOOL  GotIt;
    PWSTR NtDevicePath;
    WCHAR Drive;
    WCHAR DriveName[ 3 ];
    WCHAR Buffer[ 512 ];
    DWORD NtDevicePathLen;
    PWSTR p;
    DWORD PhysicalDriveNumber;
    HANDLE hDisk;
    BOOL  b;
    DWORD DataSize;
    PVOID DriveLayout;
    DWORD DriveLayoutSize;

    DriveName[1] = TEXT( ':' );
    DriveName[2] = 0;

    GotIt = FALSE;

    //
    // The system partition must be on multi(0)disk(0)rdisk(0)
    //

    if( NtDevicePath = ArcPathToNtPath( TEXT( "multi(0)disk(0)rdisk(0)" ) ) )
    {
        //
        // The arc path for a disk device is usually linked
        // to partition0.  Get rid of the partition part of the name.
        //

        CharLowerW( NtDevicePath );

        if( p = wcsstr( NtDevicePath, TEXT( "\\partition" ) ) )
        {
            *p = 0;
        }

        NtDevicePathLen = lstrlenW( NtDevicePath );

        //
        // Determine the physical drive number of this drive.
        // If the name is not of the form \device\harddiskx then
        // something is very wrong.
        //

        if( !wcsncmp( NtDevicePath, TEXT( "\\device\\harddisk" ), 16 ) )
        {
            PhysicalDriveNumber = wcstoul( NtDevicePath+16, NULL, 10 );

            wsprintfW( Buffer, TEXT( "\\\\.\\PhysicalDrive%u" ), PhysicalDriveNumber );

            //
            // Get drive layout info for this physical disk.
            //

            hDisk = CreateFileW( Buffer,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );

            if( hDisk != INVALID_HANDLE_VALUE )
            {
                //
                // Get partition information.
                //

                DriveLayout = MALLOC( 1024 );
                DriveLayoutSize = 1024;

                retry:

                b = DeviceIoControl( hDisk,
                                     IOCTL_DISK_GET_DRIVE_LAYOUT,
                                     NULL,
                                     0,
                                     DriveLayout,
                                     DriveLayoutSize,
                                     &DataSize,
                                     NULL );

                if( !b && ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) )
                {
                    DriveLayoutSize += 1024;
                    DriveLayout = REALLOC( DriveLayout, DriveLayoutSize );
                    goto retry;
                }

                CloseHandle( hDisk );

                if( b )
                {
                    //
                    // The system partition can only be a drive that is on
                    // this disk.  We make this determination by looking at NT drive names
                    // for each drive letter and seeing if the nt equivalent of
                    // multi(0)disk(0)rdisk(0) is a prefix.
                    //

                    for( Drive = TEXT( 'C' ); Drive <= TEXT( 'Z' ); Drive++)
                    {
                        if (MyGetDriveType( (TCHAR) Drive ) == DRIVE_FIXED )
                        {
                            DriveName[0] = Drive;

                            if( QueryDosDeviceW( DriveName,
                                                 Buffer,
                                                 sizeof( Buffer ) / sizeof( WCHAR ) ) )
                            {
                                if( !_wcsnicmp( NtDevicePath,
                                               Buffer,
                                               NtDevicePathLen ) )
                                {
                                    //
                                    // Now look to see whether there's
                                    // an nt boot sector and
                                    // boot files on this drive.
                                    //
                                    if( AppearsToBeSysPart( DriveLayout, Drive ) )
                                    {
                                        GotIt = TRUE;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                FREE( DriveLayout );
            }
        }

        FREE( NtDevicePath );
    }


    return( GotIt ? (TCHAR) Drive : TEXT( 'C' ) );
}

#endif  // _x86_
