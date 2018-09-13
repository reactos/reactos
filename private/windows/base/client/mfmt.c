/*++

Module Name:

    mfmt.c

Abstract:

    This program is designed to show how to access a physical floppy
    disk using the Win32 API set.

    This program has two major functions.

        - It can be used to display the geometry of a disk

            mfmt -g a:

        - It can be used to produce a disk image, or to write a disk
          image to a floppy.

            mfmt -c a: bootdisk         - produce a disk image of a:

            mfmt -c bootdisk a:         - make a: identical to bootdisk image

    This program is very very simple. Minimal error checking is done. It is
    meant to provide an example of how to:

        - Open a physical disk

        - Read a disk's geometry

        - Perform a low level format operation

        - read and write physical sectors

Author:

    Some Guy in the NT group (sgitng) 05-Oct-1992

Revision History:

--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <string.h>
#include <memory.h>

DISK_GEOMETRY SupportedGeometry[20];
DWORD SupportedGeometryCount;

BOOL
GetDiskGeometry(
    HANDLE hDisk,
    PDISK_GEOMETRY lpGeometry
    )

{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                lpGeometry,
                sizeof(*lpGeometry),
                &ReturnedByteCount,
                NULL
                );
}

DWORD
GetSupportedGeometrys(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;
    BOOL b;
    DWORD NumberSupported;

    b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_MEDIA_TYPES,
                NULL,
                0,
                SupportedGeometry,
                sizeof(SupportedGeometry),
                &ReturnedByteCount,
                NULL
                );
    if ( b ) {
        NumberSupported = ReturnedByteCount / sizeof(DISK_GEOMETRY);
        }
    else {
        NumberSupported = 0;
        }
    SupportedGeometryCount = NumberSupported;

    return NumberSupported;
}

VOID
PrintGeometry(
    LPSTR lpDriveName,
    PDISK_GEOMETRY lpGeometry
    )
{
    LPSTR MediaType;

    if (lpDriveName) {
        printf("Geometry for Drive %s\n",lpDriveName);
        }

    switch ( lpGeometry->MediaType ) {
        case F5_1Pt2_512:  MediaType = "5.25, 1.2MB,  512 bytes/sector";break;
        case F3_1Pt44_512: MediaType = "3.5,  1.44MB, 512 bytes/sector";break;
        case F3_2Pt88_512: MediaType = "3.5,  2.88MB, 512 bytes/sector";break;
        case F3_20Pt8_512: MediaType = "3.5,  20.8MB, 512 bytes/sector";break;
        case F3_720_512:   MediaType = "3.5,  720KB,  512 bytes/sector";break;
        case F5_360_512:   MediaType = "5.25, 360KB,  512 bytes/sector";break;
        case F5_320_512:   MediaType = "5.25, 320KB,  512 bytes/sector";break;
        case F5_320_1024:  MediaType = "5.25, 320KB,  1024 bytes/sector";break;
        case F5_180_512:   MediaType = "5.25, 180KB,  512 bytes/sector";break;
        case F5_160_512:   MediaType = "5.25, 160KB,  512 bytes/sector";break;
        case RemovableMedia: MediaType = "Removable media other than floppy";break;
        case FixedMedia:   MediaType = "Fixed hard disk media";break;
        default:           MediaType = "Unknown";break;
    }
    printf("    Media Type %s\n",MediaType);
    printf("    Cylinders %d Tracks/Cylinder %d Sectors/Track %d\n",
        lpGeometry->Cylinders.LowPart,
        lpGeometry->TracksPerCylinder,
        lpGeometry->SectorsPerTrack
        );
}

BOOL
LowLevelFormat(
    HANDLE hDisk,
    PDISK_GEOMETRY lpGeometry
    )
{
    FORMAT_PARAMETERS FormatParameters;
    PBAD_TRACK_NUMBER lpBadTrack;
    UINT i;
    BOOL b;
    DWORD ReturnedByteCount;

    FormatParameters.MediaType = lpGeometry->MediaType;
    FormatParameters.StartHeadNumber = 0;
    FormatParameters.EndHeadNumber = lpGeometry->TracksPerCylinder - 1;
    lpBadTrack = (PBAD_TRACK_NUMBER) LocalAlloc(LMEM_ZEROINIT,lpGeometry->TracksPerCylinder*sizeof(*lpBadTrack));

    for (i = 0; i < lpGeometry->Cylinders.LowPart; i++) {

        FormatParameters.StartCylinderNumber = i;
        FormatParameters.EndCylinderNumber = i;

        b = DeviceIoControl(
                hDisk,
                IOCTL_DISK_FORMAT_TRACKS,
                &FormatParameters,
                sizeof(FormatParameters),
                lpBadTrack,
                lpGeometry->TracksPerCylinder*sizeof(*lpBadTrack),
                &ReturnedByteCount,
                NULL
                );

        if (!b ) {
            LocalFree(lpBadTrack);
            return b;
            }
        }

    LocalFree(lpBadTrack);

    return TRUE;
}

BOOL
LockVolume(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                FSCTL_LOCK_VOLUME,
                NULL,
                0,
                NULL,
                0,
                &ReturnedByteCount,
                NULL
                );
}

BOOL
UnlockVolume(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                FSCTL_UNLOCK_VOLUME,
                NULL,
                0,
                NULL,
                0,
                &ReturnedByteCount,
                NULL
                );
}

BOOL
DismountVolume(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                FSCTL_DISMOUNT_VOLUME,
                NULL,
                0,
                NULL,
                0,
                &ReturnedByteCount,
                NULL
                );
}

DWORD
_cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char Drive[MAX_PATH];
    HANDLE hDrive, hDiskImage;
    DISK_GEOMETRY Geometry;
    UINT i;
    char c, *p;
    LPSTR DriveName;
    BOOL fUsage = TRUE;
    BOOL fShowGeometry = FALSE;
    BOOL fDiskImage = FALSE;
    BOOL SourceIsDrive;
    LPSTR Source, Destination, DiskImage;

    if ( argc > 1 ) {
        fUsage = FALSE;
        while (--argc) {
            p = *++argv;
            if (*p == '/' || *p == '-') {
                while (c = *++p)
                switch (toupper( c )) {
                case '?':
                    fUsage = TRUE;
                    break;

                case 'C':
                    fDiskImage = TRUE;
                    argc--, argv++;
                    Source = *argv;
                    argc--, argv++;
                    Destination = *argv;
                    break;

                case 'G':
                    fShowGeometry = TRUE;
                    argc--, argv++;
                    DriveName = *argv;
                    break;

                default:
                    printf("MFMT: Invalid switch - /%c\n", c );
                    fUsage = TRUE;
                    break;
                    }
                }
            }
        }

    if ( fUsage ) {
        printf("usage: MFMT switches \n" );
        printf("            [-?] display this message\n" );
        printf("            [-g drive] shows disk geometry\n" );
        printf("            [-c source destination] produce diskimage\n" );
        ExitProcess(1);
        }

    if ( fShowGeometry ) {
        sprintf(Drive,"\\\\.\\%s",DriveName);
        hDrive = CreateFile(
                    Drive,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
        if ( hDrive == INVALID_HANDLE_VALUE ) {
            printf("MFMT: Open %s failed %d\n",DriveName,GetLastError());
            ExitProcess(1);
            }

        LockVolume(hDrive);

        if ( !GetDiskGeometry(hDrive,&Geometry) ) {
            printf("MFMT: GetDiskGeometry %s failed %d\n",DriveName,GetLastError());
            ExitProcess(1);
            }
        PrintGeometry(DriveName,&Geometry);

        if ( !GetSupportedGeometrys(hDrive) ) {
            printf("MFMT: GetSupportedGeometrys %s failed %d\n",DriveName,GetLastError());
            ExitProcess(1);
            }
        printf("\nDrive %s supports the following disk geometries\n",DriveName);

        for(i=0;i<SupportedGeometryCount;i++) {
            printf("\n");
            PrintGeometry(NULL,&SupportedGeometry[i]);
            }

        printf("\n");
        ExitProcess(0);
        }

    if ( fDiskImage ) {
        SourceIsDrive = FALSE;
        if ( Source[strlen(Source)-1] == ':' ) {
            SourceIsDrive = TRUE;
            sprintf(Drive,"\\\\.\\%s",Source);
            DiskImage = Destination;
            }
        if ( Destination[strlen(Destination)-1] == ':' ) {
            if ( SourceIsDrive ) {
                printf("MFMT: Source and Destination cannot both be drives\n");
                ExitProcess(1);
                }
            SourceIsDrive = FALSE;
            sprintf(Drive,"\\\\.\\%s",Destination);
            DiskImage = Source;
            }
        else {
            if ( !SourceIsDrive ) {
                printf("MFMT: Either Source or Destination must be a drive\n");
                ExitProcess(1);
                }
            }

        //
        // Open and Lock the drive
        //

        hDrive = CreateFile(
                    Drive,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
        if ( hDrive == INVALID_HANDLE_VALUE ) {
            printf("MFMT: Open %s failed %d\n",DriveName,GetLastError());
            ExitProcess(1);
            }
        LockVolume(hDrive);

        if ( !GetDiskGeometry(hDrive,&Geometry) ) {
            printf("MFMT: GetDiskGeometry %s failed %d\n",DriveName,GetLastError());
            ExitProcess(1);
            }

        if ( !GetSupportedGeometrys(hDrive) ) {
            printf("MFMT: GetSupportedGeometrys %s failed %d\n",DriveName,GetLastError());
            ExitProcess(1);
            }

        //
        // Open the disk image file
        //

        hDiskImage = CreateFile(
                        DiskImage,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        SourceIsDrive ? CREATE_ALWAYS : OPEN_EXISTING,
                        0,
                        NULL
                        );
        if ( hDiskImage == INVALID_HANDLE_VALUE ) {
            printf("MFMT: Open %s failed %d\n",DiskImage,GetLastError());
            ExitProcess(1);
            }

        //
        // Now do the copy
        //
        {
            LPVOID IoBuffer;
            BOOL b;
            DWORD BytesRead, BytesWritten;
            DWORD FileSize;
            DWORD GeometrySize;

            //
            // If we are copying from floppy to file, just do the copy
            // Otherwise, we might have to format the floppy first
            //

            if ( SourceIsDrive ) {

                //
                // Device reads must be sector aligned. VirtualAlloc will
                // garuntee alignment
                //

                GeometrySize = Geometry.Cylinders.LowPart *
                               Geometry.TracksPerCylinder *
                               Geometry.SectorsPerTrack *
                               Geometry.BytesPerSector;

                IoBuffer = VirtualAlloc(NULL,GeometrySize,MEM_COMMIT,PAGE_READWRITE);

                if ( !IoBuffer ) {
                    printf("MFMT: Buffer Allocation Failed\n");
                    ExitProcess(1);
                    }

                b = ReadFile(hDrive,IoBuffer, GeometrySize, &BytesRead, NULL);
                if (b && BytesRead){
                    b = WriteFile(hDiskImage,IoBuffer, BytesRead, &BytesWritten, NULL);
                    if ( !b || ( BytesRead != BytesWritten ) ) {
                        printf("MFMT: Fatal Write Error %d\n",GetLastError());
                        ExitProcess(1);
                        }
                    }
                else {
                    printf("MFMT: Fatal Read Error %d\n",GetLastError());
                    ExitProcess(1);
                    }
                }
            else {

                //
                // Check to see if the image will fit on the floppy. If it
                // will, then LowLevelFormat the floppy and press on
                //

                FileSize = GetFileSize(hDiskImage,NULL);

                b = FALSE;
                for(i=0;i<SupportedGeometryCount;i++) {
                    GeometrySize = SupportedGeometry[i].Cylinders.LowPart *
                                   SupportedGeometry[i].TracksPerCylinder *
                                   SupportedGeometry[i].SectorsPerTrack *
                                   SupportedGeometry[i].BytesPerSector;
                    if ( GeometrySize >= FileSize ) {

                        IoBuffer = VirtualAlloc(NULL,GeometrySize,MEM_COMMIT,PAGE_READWRITE);

                        if ( !IoBuffer ) {
                            printf("MFMT: Buffer Allocation Failed\n");
                            ExitProcess(1);
                            }

                        //
                        // Format the floppy
                        //

                        LowLevelFormat(hDrive,&SupportedGeometry[i]);

                        b = ReadFile(hDiskImage,IoBuffer, GeometrySize, &BytesRead, NULL);
                        if (b && BytesRead){
                            b = WriteFile(hDrive,IoBuffer, BytesRead, &BytesWritten, NULL);
                            if ( !b || ( BytesRead != BytesWritten ) ) {
                                printf("MFMT: Fatal Write Error %d\n",GetLastError());
                                ExitProcess(1);
                                }
                            }
                        else {
                            printf("MFMT: Fatal Read Error %d\n",GetLastError());
                            ExitProcess(1);
                            }
                        b = TRUE;
                        break;
                        }
                    }

                if ( !b ) {
                    printf("MFMT: FileSize %d is not supported on drive %s\n",FileSize,DriveName);
                    ExitProcess(1);
                    }
                }
        }

        //
        // Dismounting forces the filesystem to re-evaluate the media id
        // and geometry. This is the same as popping the floppy in and out
        // of the disk drive
        //

        DismountVolume(hDrive);
        UnlockVolume(hDrive);

        ExitProcess(0);
        }
}
