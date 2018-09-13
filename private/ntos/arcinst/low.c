#include "precomp.h"
#pragma hdrstop
#include <bootmbr.h>

ARC_STATUS
LowOpenDisk(
    IN  PCHAR   DevicePath,
    OUT PULONG  DiskId
    )
/*++

Routine Description:

    This routine opens the supplied device for DASD access.

Arguments:

    DevicePath  - Supplies the device path to be opened.
    DiskId      - Returns the disk id.

Return Value:

    ArcOpen

--*/
{
   char buffer[256];

   sprintf(buffer,"%spartition(0)",DevicePath);

   return ArcOpen(buffer, ArcOpenReadWrite, DiskId);
}


ARC_STATUS
LowCloseDisk(
    IN  ULONG    DiskId
    )
/*++

Routine Description:

    This routine closes the supplied device.

Arguments:

    DiskId      - Supplies the disk id.

Return Value:

    ArcClose

--*/
{
    return ArcClose(DiskId);
}

ARC_STATUS
LowGetDriveGeometry(
    IN  PCHAR   DevicePath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    )
{
    char Buffer[256];

    sprintf(Buffer,"%spartition(0)",DevicePath);
    return(LowGetPartitionGeometry(Buffer,TotalSectorCount,SectorSize,SectorsPerTrack,Heads));
}



ARC_STATUS
LowGetPartitionGeometry(
    IN  PCHAR   PartitionPath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    )
/*++

Routine Description:

    This routine computes the drive geometry for the given partition or
    physical disk.

Arguments:

    PartitionPath   - Supplies a path to the partition or physical disk.
    NumberOfSectors - Returns the number of sectors.
    SectorSize      - Returns the sector size.
    SectorsPerTrack - Returns the number of sectors per track.
    Heads           - Returns the number of heads.

Return Value:

    ArcOpen, ArcGetFileInformation, ArcClose, E2BIG, ESUCCESS

--*/
{
    FILE_INFORMATION    file_info;
    ARC_STATUS          r;
    ULONG               fileid;
    LARGE_INTEGER       l;
    CM_DISK_GEOMETRY_DEVICE_DATA    *DiskGeometry;
    CONFIGURATION_COMPONENT         *DiskComponent;
    CM_PARTIAL_RESOURCE_LIST        *DiskConfiguration;
    CHAR                            DataBuffer[sizeof(CM_PARTIAL_RESOURCE_LIST) +
                                               sizeof(CM_DISK_GEOMETRY_DEVICE_DATA)];
    CM_PARTIAL_RESOURCE_DESCRIPTOR  *DiskData;

    // Always assume 512 bytes per sector.

    *SectorSize      = 512;

    // Assume the SCSI default values for number of heads and sectors per track

    *SectorsPerTrack = 32;
    *Heads           = 64;

    // See if there is device specific data describing the geometry of
    // the drive.  If there is none, then just use the default SCSI
    // values.

    DiskComponent = ArcGetComponent(PartitionPath);

    if (DiskComponent == NULL) {
        return EINVAL;
    }

    //
    // See if the ConfigurationDataLength is correct
    // It should contain one DeviceSpecific resource descriptor
    //

    if (DiskComponent->ConfigurationDataLength == sizeof(CM_PARTIAL_RESOURCE_LIST) +
                                                  sizeof(CM_DISK_GEOMETRY_DEVICE_DATA)  ) {

        DiskConfiguration = (CM_PARTIAL_RESOURCE_LIST *)DataBuffer;

        r = ArcGetConfigurationData(DiskConfiguration,DiskComponent);

        if (r == ESUCCESS) {

            //
            // See if the Configuration Data has ARC version 1.3 or greater
            //

            if  ( (DiskConfiguration->Version == 1 && DiskConfiguration->Revision >=3 ) ||
                  (DiskConfiguration->Version >  1)                                        ) {

                DiskData = &(DiskConfiguration->PartialDescriptors[DiskConfiguration->Count-1]);

                if (DiskData->Type == CmResourceTypeDeviceSpecific) {

                    if (DiskData->u.DeviceSpecificData.DataSize == sizeof(CM_DISK_GEOMETRY_DEVICE_DATA)) {
                        DiskGeometry     = (CM_DISK_GEOMETRY_DEVICE_DATA *)
                                           &(DiskConfiguration->PartialDescriptors[DiskConfiguration->Count]);
                        *SectorsPerTrack = DiskGeometry->SectorsPerTrack;
                        *Heads           = DiskGeometry->NumberOfHeads;
                        *SectorSize      = DiskGeometry->BytesPerSector;
                    }
                }
            }
        }
    }

//    PrintError("SectorSize = %08x",*SectorSize);
//    PrintError("SectorsPerTrack = %08x",*SectorsPerTrack);
//    PrintError("Heads = %08x",*Heads);

    r = ArcOpen(PartitionPath, ArcOpenReadOnly, &fileid);

    if (r != ESUCCESS) {
        return r;
    }

    r = ArcGetFileInformation(fileid, &file_info);

    if (r != ESUCCESS) {
        return r;
    }

    r = ArcClose(fileid);

    if (r != ESUCCESS) {
        return r;
    }

    l.QuadPart = file_info.EndingAddress.QuadPart -
                 file_info.StartingAddress.QuadPart;

    l.QuadPart = ((ULONGLONG)l.QuadPart) / ((ULONGLONG)(*SectorSize));

    if (l.HighPart) {
        return E2BIG;
    }

    *TotalSectorCount = l.LowPart;

    return ESUCCESS;
}


#define MAX_TRANSFER    65536


ARC_STATUS
LowReadSectors(
    IN  ULONG   VolumeId,
    IN  ULONG   SectorSize,
    IN  ULONG   StartingSector,
    IN  ULONG   NumberOfSectors,
    OUT PVOID   Buffer
    )
/*++

Routine Description:

    This routine reads 'NumberOfSectors' sectors starting at sector
    'StartingSector' on the volume with ID 'VolumeId'.

Arguments:

    VolumeId        - Supplies the ID for the volume.
    SectorSize      - Supplies the number of bytes per sector.
    StartingSector  - Supplies the starting sector for the read.
    NumberOfSectors - Supplies the number of sectors to read.
    Buffer          - Returns the read in sectors.

Return Value:

    ArcSeek, ArcRead, EIO, ESUCCESS

--*/
{
    ARC_STATUS    r;
    ULONG         c;
    LARGE_INTEGER l;
    ULONG         i;
    ULONG         transfer;
    PCHAR         buf;
    ULONG         total;


    l.QuadPart = UInt32x32To64(StartingSector,SectorSize);

    buf = (PCHAR) Buffer;

    r = ArcSeek(VolumeId, &l, SeekAbsolute);

    if (r != ESUCCESS) {
        return r;
    }

    total = SectorSize*NumberOfSectors;

    for (i = 0; i < total; i += MAX_TRANSFER) {

        transfer = min(MAX_TRANSFER, total - i);

        r = ArcRead(VolumeId, &buf[i], transfer, &c);

        if (r != ESUCCESS) {
            return r;
        }

        if (c != transfer) {
            return EIO;
        }
    }

    return ESUCCESS;
}


ARC_STATUS
LowWriteSectors(
    IN  ULONG   VolumeId,
    IN  ULONG   SectorSize,
    IN  ULONG   StartingSector,
    IN  ULONG   NumberOfSectors,
    IN  PVOID   Buffer
    )
/*++

Routine Description:

    This routine write 'NumberOfSectors' sectors starting at sector
    'StartingSector' on the volume with ID 'VolumeId'.

Arguments:

    VolumeId        - Supplies the ID for the volume.
    SectorSize      - Supplies the number of bytes per sector.
    StartingSector  - Supplies the starting sector for the write.
    NumberOfSectors - Supplies the number of sectors to write.
    Buffer          - Supplies the sectors to write.

Return Value:

    ArcSeek, ArcWrite, EIO, ESUCCESS

--*/
{
    ARC_STATUS    r;
    ULONG         c;
    LARGE_INTEGER l;
    ULONG         i;
    ULONG         transfer;
    PCHAR         buf;
    ULONG         total;

    l.QuadPart = UInt32x32To64(StartingSector,SectorSize);

    buf = (PCHAR) Buffer;

    r = ArcSeek(VolumeId, &l, SeekAbsolute);

    if (r != ESUCCESS) {
        return r;
    }

    total = SectorSize*NumberOfSectors;

    for (i = 0; i < total; i += MAX_TRANSFER) {

        transfer = min(MAX_TRANSFER, total - i);

        r = ArcWrite(VolumeId, &buf[i], transfer, &c);

        if (r != ESUCCESS) {
            return r;
        }

        if (c != transfer) {
            return EIO;
        }
    }

    return ESUCCESS;
}


/******************** DAVIDRO CODE *************************************/
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alpath.c

Abstract:

    This module provides ARC pathname functions.

Author:

    David M. Robinson (davidro) 13-November-1991

Revision History:

--*/

//
// Define the ARC pathname mnemonics.
//

PCHAR MnemonicTable[] = {
    "arc",
    "cpu",
    "fpu",
    "pic",
    "pdc",
    "sic",
    "sdc",
    "sc",
    "eisa",
    "tc",
    "scsi",
    "dti",
    "multi",
    "disk",
    "tape",
    "cdrom",
    "worm",
    "serial",
    "net",
    "video",
    "par",
    "point",
    "key",
    "audio",
    "other",
    "rdisk",
    "fdisk",
    "tape",
    "modem",
    "monitor",
    "print",
    "pointer",
    "keyboard",
    "term",
    "other",
    "line",
    "netper",
    "memory"
    };

//
// Static storage for the pathname return value.
//

CHAR Pathname[256];


PCHAR
AlGetPathnameFromComponent (
    IN PCONFIGURATION_COMPONENT Component
    )

/*++

Routine Description:

    This function builds an ARC pathname for the specified component.

Arguments:

    Component - Supplies a pointer to a configuration component.

Return Value:

    Returns a pointer to a string which contains the ARC pathname for the
    component.  NOTE: The string is stored in static storage, and must be
    copied by the user before another call to this routine.

--*/
{
    PCONFIGURATION_COMPONENT ParentComponent;
    CHAR NewSegment[16];
    CHAR Tempname[256];

    Pathname[0] = 0;

    //
    // Loop while not at the root component.
    //

    while ((ParentComponent = ArcGetParent(Component)) != NULL) {

        //
        // Build new pathname segment from the Component type and key.
        //

        sprintf(NewSegment,
                "%s(%d)",
                MnemonicTable[Component->Type],
                Component->Key);

        //
        // Add the new segment as a prefix of the current pathname.
        //

        strcpy(Tempname, Pathname);
        strcpy(Pathname, NewSegment);
        strcat(Pathname, Tempname);

        //
        // Move to the parent component.
        //

        Component = ParentComponent;
    }

    return Pathname;
}
/******************* END OF DAVIDRO CODE *********************************/


ARC_STATUS
LowQueryPathFromComponent(
    IN  PCONFIGURATION_COMPONENT    Component,
    OUT PCHAR*                      Path
    )
/*++

Routine Description:

    This routine computes a path from a component.  The resulting
    path is allocated on the heap.

Arguments:

    Component   - Supplies a component.
    Path        - Returns the path corresponding to that component.

Return Value:

    ENOMEM, ESUCCESS

--*/
{
    PCHAR   p;
    PCHAR   path;

    p = AlGetPathnameFromComponent(Component);

    path = AllocateMemory(strlen(p) + 1);

    if (!path) {
        return ENOMEM;
    }

    strcpy(path, p);

    *Path = path;

    return ESUCCESS;
}


ARC_STATUS
LowTraverseChildren(
    IN      PCONFIGURATION_COMPONENT    Parent,
    IN      CONFIGURATION_CLASS*        ConfigClass OPTIONAL,
    IN      CONFIGURATION_TYPE*         ConfigType OPTIONAL,
    IN OUT  PCONFIGURATION_COMPONENT*   MatchingArray OPTIONAL,
    IN OUT  PULONG                      CurrentLength
    )
/*++

Routine Description:

    This routine traverses the trees whose parent is 'Parent'.
    If the Matching array is provided then it will be filled
    with pointers to all of the components whose type and class
    match 'ConfigType' and 'ConfigClass'.  Also, the 'CurrentLength'
    is incremented by the number of nodes that match.

Arguments:

    Parent          - Supplies the root of the tree.
    ConfigClass     - Supplies the class to search for.
    ConfigType      - Supplies the type to search for.
    CurrentLength   - Supplies the current count.

Return Value:

    ESUCCESS

--*/
{
    PCONFIGURATION_COMPONENT    pc;
    ARC_STATUS                  r;

    if (!(pc = ArcGetChild(Parent))) {
        return ESUCCESS;
    }

    for (;;) {

        if ((!ConfigClass || pc->Class == *ConfigClass) &&
            (!ConfigType  || pc->Type  == *ConfigType)) {

            if (MatchingArray) {

                MatchingArray[*CurrentLength] = pc;
            }

            (*CurrentLength)++;
        }

        r = LowTraverseChildren(pc, ConfigClass, ConfigType,
                                MatchingArray, CurrentLength);

        if (r != ESUCCESS) {
            return r;
        }

        if (!(pc = ArcGetPeer(pc))) {
            break;
        }
    }

    return ESUCCESS;
}


ARC_STATUS
LowQueryComponentList(
    IN  CONFIGURATION_CLASS*        ConfigClass OPTIONAL,
    IN  CONFIGURATION_TYPE*             ConfigType OPTIONAL,
    OUT PCONFIGURATION_COMPONENT**  ComponentList,
    OUT PULONG                      ListLength
    )
/*++

Routine Description:

    This routine returns an array of components whose class and
    type match the ones given.  (Since each parameter is optional,
    you can do type-only and class-only searches.)

    The array is allocated on the heap and contains pointers to
    the actual components (NOT copies).

Arguments:

    ConfigClass     - Supplies the configuation class to search for.
    ConfigType      - Supplies the configuration type to search for.
    ComponentList   - Returns a list of pointers to components whose
                        class and type match 'ConfigClass' and
                        'ConfigType'.
    ListLength      - Returns the number of components in the list.

Return Value:

    LowTraverseChildren, ENOMEM

--*/
{
    ARC_STATUS  r;

    *ListLength = 0;

    r = LowTraverseChildren(NULL, ConfigClass, ConfigType, NULL, ListLength);

    if (r != ESUCCESS) {
        return r;
    }

    if (!(*ComponentList = (PCONFIGURATION_COMPONENT*) AllocateMemory(
            (*ListLength)*sizeof(PCONFIGURATION_COMPONENT)))) {

        return ENOMEM;
    }

    *ListLength = 0;

    return LowTraverseChildren(NULL, ConfigClass, ConfigType,
                               *ComponentList, ListLength);
}


ARC_STATUS
LowQueryPathList(
    IN  CONFIGURATION_CLASS*        ConfigClass OPTIONAL,
    IN  CONFIGURATION_TYPE*             ConfigType OPTIONAL,
    OUT PCHAR**                     PathList,
    OUT PULONG                      ListLength
    )
/*++

Routine Description:

    This routine returns a list of paths to the components that are
    of class ConfigClass and of type ConfigType.

Arguments:

    ConfigClass     - Supplies the configuation class to search for.
    ConfigType      - Supplies the configuration type to search for.
    PathList        - Returns a list of paths to the components.
    ListLength      - Returns the number of components in the list.

Return Value:

    LowQueryComponentList, LowQueryPathFromComponent

--*/
{
    PCONFIGURATION_COMPONENT*   component_list;
    ULONG                       list_length;
    ARC_STATUS                  r;
    ULONG                       i;
    PCHAR*                      path_list;

    r = LowQueryComponentList(ConfigClass, ConfigType,
                              &component_list, &list_length);

    if (r != ESUCCESS) {
        return r;
    }

    if (!(path_list = (PCHAR*) AllocateMemory(list_length*sizeof(PCHAR)))) {
        FreeMemory(component_list);
        return ENOMEM;
    }


    for (i = 0; i < list_length; i++) {
        path_list[i] = NULL;
    }

    for (i = 0; i < list_length; i++) {

        r = LowQueryPathFromComponent(component_list[i], &path_list[i]);

        if (r != ESUCCESS) {
            FreeMemory(component_list);
            LowFreePathList(path_list, list_length);
            return r;
        }
    }

    FreeMemory(component_list);

    *PathList = path_list;
    *ListLength = list_length;

    return ESUCCESS;
}


ARC_STATUS
LowFreePathList(
    IN  PCHAR*  PathList,
    IN  ULONG   ListLength
    )
/*++

Routine Description:

    This routine frees up the space taken by the path lists.

Arguments:

    PathList    - Supplies the paths.
    ListLength  - Supplies the number of paths.

Return Value:

    ESUCCESS

--*/
{
    ULONG i;

    for (i = 0; i < ListLength; i++) {
        if (PathList[i]) {
            FreeMemory(PathList[i]);
        }
    }
    FreeMemory(PathList);

    return ESUCCESS;
}


ARC_STATUS
LowQueryFdiskPathList(
    OUT PCHAR** PathList,
    OUT PULONG  ListLength
    )
/*++

Routine Description:

    This routine returns a list of paths to all the devices of interest
    to FDISK.

Arguments:

    PathList    - Returns a list of paths.
    ListLength  - Returns the length of the list.

Return Value:

    LowQueryComponentList, LowQueryPathFromComponent, ESUCCESS

--*/
{
    CONFIGURATION_TYPE  config_type;

    config_type = DiskPeripheral;
    return LowQueryPathList(NULL, &config_type, PathList, ListLength);
}


ARC_STATUS
LowFreeFdiskPathList(
    IN OUT  PCHAR*  PathList,
    IN      ULONG   ListLength
    )
/*++

Routine Description:

    This routine frees up the space taken by the path lists.

Arguments:

    PathList    - Supplies the paths.
    ListLength  - Supplies the number of paths.

Return Value:

    ESUCCESS

--*/
{
    return LowFreePathList(PathList, ListLength);
}


ARC_STATUS
LowGetDiskLayout(
    IN  PCHAR                      Path,
    OUT PDRIVE_LAYOUT_INFORMATION* DriveLayout
    )
{
    ARC_STATUS                status;
    ULONG                     Handle;
    ULONG                     i,ExtendedStart,BootSector,Entry;
    ULONG                     dummy,bps;
    BOOLEAN                   Link,mbr;
    PDRIVE_LAYOUT_INFORMATION DriveInfo;
    PPARTITION_INFORMATION    p;
    PCHAR                     SectorBuffer;
    PPARTITION_DESCRIPTOR     ptable;


#define PSTART(p)  (                                \
        (ULONG) ((p)->StartingSectorLsb0) +           \
        (ULONG) ((p)->StartingSectorLsb1 << 8) +      \
        (ULONG) ((p)->StartingSectorMsb0 << 16) +     \
        (ULONG) ((p)->StartingSectorMsb1 << 24) )

#define PLENGTH(p)   (                              \
        (ULONG) ((p)->PartitionLengthLsb0) +          \
        (ULONG) ((p)->PartitionLengthLsb1 << 8) +     \
        (ULONG) ((p)->PartitionLengthMsb0 << 16) +    \
        (ULONG) ((p)->PartitionLengthMsb1 << 24) )



    if((DriveInfo = AlAllocateHeap(sizeof(DRIVE_LAYOUT_INFORMATION) + (500*sizeof(PARTITION_INFORMATION)))) == NULL) {
        return(ENOMEM);
    }
    p = &DriveInfo->PartitionEntry[0];

    if((status = LowGetDriveGeometry(Path,&dummy,&bps,&dummy,&dummy)) != ESUCCESS) {
        AlDeallocateHeap(DriveInfo);
        return(status);
    }

    if((SectorBuffer = AlAllocateHeap(bps)) == NULL) {
        AlDeallocateHeap(DriveInfo);
        return(ENOMEM);
    }

    ptable = (PPARTITION_DESCRIPTOR)(SectorBuffer + (2*PARTITION_TABLE_OFFSET));

    if((status = LowOpenDisk(Path,&Handle)) != ESUCCESS) {
        AlDeallocateHeap(SectorBuffer);
        AlDeallocateHeap(DriveInfo);
        return(status);
    }

    mbr = TRUE;
    Entry = 0;
    BootSector = 0;
    ExtendedStart = 0;
    status = ESUCCESS;

    do {

        if((status = LowReadSectors(Handle,bps,BootSector,1,SectorBuffer)) != ESUCCESS) {
            break;
        }

        // This is to catch the case where there is no MBR yet.

        if(((PUSHORT)SectorBuffer)[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE) {
            break;
        }

        Link = FALSE;

        for(i=0; i<NUM_PARTITION_TABLE_ENTRIES; i++) {

            if(ptable[i].PartitionType == SYSID_UNUSED) {

                // set as unused.

                p[Entry].PartitionType    = SYSID_UNUSED;
                p[Entry].BootIndicator    = FALSE;
                p[Entry].RewritePartition = FALSE;
                p[Entry].PartitionLength.QuadPart  = 0;
                p[Entry].HiddenSectors    = 0;
                p[Entry].StartingOffset.QuadPart   = 0;
                p[Entry].RecognizedPartition = FALSE;

            } else {

                LARGE_INTEGER   Result1, Result2;

                p[Entry].PartitionType    = ptable[i].PartitionType;
                p[Entry].BootIndicator    = ptable[i].ActiveFlag;
                p[Entry].RewritePartition = FALSE;
                p[Entry].PartitionLength.QuadPart  = UInt32x32To64(PLENGTH(ptable + i),bps);

                // BUGBUG (tedm) the following are not correct for link
                //               entries in the extended partition.
                //               But fdisk does not use these values in
                //               this case so blow it off.  As of
                //               11/13/91, IoReadPartitionTable has the
                //               same bug.

                p[Entry].HiddenSectors    = PSTART(ptable + i);
                Result1.QuadPart = UInt32x32To64(PSTART(ptable + i),bps);
                Result2.QuadPart = UInt32x32To64(BootSector,bps);
                p[Entry].StartingOffset.QuadPart = Result1.QuadPart + Result2.QuadPart;

                p[Entry].RecognizedPartition = TRUE;     // BUGBUG this is broken

                if(p[Entry].PartitionType == SYSID_EXTENDED) {

                    Link = TRUE;

                    if(mbr) {
                        mbr = FALSE;
                        BootSector = PSTART(ptable + i);
                        ExtendedStart = BootSector;
                    } else {
                        BootSector = ExtendedStart + PSTART(ptable + i);
                    }
                }
            }
            Entry++;
        }
    } while(Link);

    LowCloseDisk(Handle);

    AlDeallocateHeap(SectorBuffer);

    if(status != ESUCCESS) {

        AlDeallocateHeap(DriveInfo);
        return(status);
    }

    // reallocate DriveInfo, set PartitionCount field.
    // DriveInfo is shrinking.

    DriveInfo = AlReallocateHeap(DriveInfo,
                                   sizeof(DRIVE_LAYOUT_INFORMATION)
                                 + ((Entry - 1) * sizeof(PARTITION_INFORMATION))
                                );

    DriveInfo->PartitionCount = Entry;

    *DriveLayout = DriveInfo;

    return(ESUCCESS);
}


//
// cylinder/head/sector stuff placed in partition table
//

typedef struct _tagCHS {
    USHORT StartCylinder;
    UCHAR  StartHead;
    UCHAR  StartSector;
    USHORT EndCylinder;
    UCHAR  EndHead;
    UCHAR  EndSector;
} CHS, *PCHS;


VOID
CalculateCHSVals(
    IN  ULONG Start,
    IN  ULONG Size,
    IN  ULONG spt,
    IN  ULONG h,
    OUT PCHS  chs
    )
{
    ULONG spc = spt * h;        // sectors per cylinder
    ULONG r;
    ULONG End = Start+Size-1;

    chs->StartCylinder = (USHORT)(Start/spc);
    r = Start % spc;
    chs->StartHead = (UCHAR)(r / spt);
    chs->StartSector = (UCHAR)((r % spt) + 1);      // sector is 1-based

    chs->EndCylinder = (USHORT)(End/spc);
    r = End % spc;
    chs->EndHead = (UCHAR)(r / spt);
    chs->EndSector = (UCHAR)((r % spt) + 1);        // sector is 1-based
}


VOID
SetPartitionTableEntry(
    IN OUT PPARTITION_DESCRIPTOR p,
    IN     UCHAR                 Active,
    IN     UCHAR                 SysID,
    IN     ULONG                 RelativeSector,
    IN     ULONG                 SectorCount,
    IN     PCHS                  chs
    )
{
    // first get the easy ones out of the way.

    p->ActiveFlag    = Active;
    p->PartitionType = SysID;

    if(chs) {
        p->StartingTrack = chs->StartHead;
        p->EndingTrack   = chs->EndHead;
    } else {
        p->StartingTrack = 0;
        p->EndingTrack   = 0;
    }

    if(chs) {

        // pack sector/cyl values

        p->StartingCylinderLsb = (chs->StartSector & 0x3f) | ((chs->StartCylinder >> 2) & 0xc0);
        p->StartingCylinderMsb = (UCHAR)chs->StartCylinder;

        p->EndingCylinderLsb   = (chs->EndSector   & 0x3f) | ((chs->EndCylinder   >> 2) & 0xc0);
        p->EndingCylinderMsb   = (UCHAR)chs->EndCylinder;

    } else {

        p->StartingCylinderLsb = 0;
        p->StartingCylinderMsb = 0;

        p->EndingCylinderLsb   = 0;
        p->EndingCylinderMsb   = 0;
    }

    // now handle the relative and total sector counts

    p->StartingSectorLsb0  = (UCHAR)(RelativeSector >> 0 );
    p->StartingSectorLsb1  = (UCHAR)(RelativeSector >> 8 );
    p->StartingSectorMsb0  = (UCHAR)(RelativeSector >> 16);
    p->StartingSectorMsb1  = (UCHAR)(RelativeSector >> 24);

    p->PartitionLengthLsb0 = (UCHAR)(SectorCount    >> 0 );
    p->PartitionLengthLsb1 = (UCHAR)(SectorCount    >> 8 );
    p->PartitionLengthMsb0 = (UCHAR)(SectorCount    >> 16);
    p->PartitionLengthMsb1 = (UCHAR)(SectorCount    >> 24);
}


#define ZeroPartitionTableEntry(p) SetPartitionTableEntry(p,0,SYSID_UNUSED,0,0,NULL);

VOID
ZeroPartitionTable(
    PPARTITION_DESCRIPTOR PartitionTable
    )
{
    ULONG i;

    for(i=0; i<ENTRIES_PER_BOOTSECTOR; i++) {
        ZeroPartitionTableEntry(PartitionTable+i);
    }
}


ARC_STATUS
LowSetDiskLayout(
    IN PCHAR                     Path,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    )
{
    ARC_STATUS             status;
    ULONG                   dummy,bps,spt,h;
    PCHAR                   SectorBuffer;
    ULONG                   Handle;
    PPARTITION_DESCRIPTOR   PartitionTable;
    PPARTITION_INFORMATION  p;
    BOOLEAN                 mbr = TRUE,Update;
    ULONG                   BootSector = 0,ExtendedPartitionStart = 0,
                            NextBootSector;
    ULONG                   i,j,UsedCount;
    CHS                     chs;


#define SECCNT(l) ((ULONG)(((ULONGLONG)l.QuadPart)/((ULONGLONG)bps)))


    ASRT(DriveLayout->PartitionCount);

    if((status = LowGetDriveGeometry(Path,&dummy,&bps,&spt,&h)) != ESUCCESS) {
        return(status);
    }

    // allocate a buffer for sector I/O

    if((SectorBuffer = AlAllocateHeap(bps)) == NULL) {
        return(ENOMEM);
    }

    //
    // Use x86 bootcode as a template so the disk will boot an x86
    // if it is moved to disk0 on an x86 machine.
    //
    RtlMoveMemory(SectorBuffer,x86BootCode,min(X86BOOTCODE_SIZE,bps));

    ((PUSHORT)SectorBuffer)[BOOT_SIGNATURE_OFFSET] = BOOT_RECORD_SIGNATURE;
    PartitionTable = (PPARTITION_DESCRIPTOR)(&(((PUSHORT)SectorBuffer)[PARTITION_TABLE_OFFSET]));

    if((status = LowOpenDisk(Path,&Handle)) != ESUCCESS) {
        AlDeallocateHeap(SectorBuffer);
        return(status);
    }

    ASRT(!(DriveLayout->PartitionCount % ENTRIES_PER_BOOTSECTOR));
    for(i=0; i<DriveLayout->PartitionCount; i+=ENTRIES_PER_BOOTSECTOR) {

        Update = FALSE;
        UsedCount = 0;

        ZeroPartitionTable(PartitionTable);

        for(j=0; j<ENTRIES_PER_BOOTSECTOR; j++) {

            p = &DriveLayout->PartitionEntry[i+j];

            switch(p->PartitionType) {
            case SYSID_UNUSED:
                ZeroPartitionTableEntry(PartitionTable+j);
                break;

            case SYSID_EXTENDED:
                NextBootSector = SECCNT(p->StartingOffset);
                CalculateCHSVals(NextBootSector,SECCNT(p->PartitionLength),spt,h,&chs);
                SetPartitionTableEntry(PartitionTable+j,
                                       p->BootIndicator,
                                       SYSID_EXTENDED,
                                       SECCNT(p->StartingOffset) - ExtendedPartitionStart,
                                       SECCNT(p->PartitionLength),
                                       &chs
                                      );
                if(mbr) {
                    mbr = FALSE;
                    ExtendedPartitionStart = NextBootSector;
                }
                break;

            default:
                CalculateCHSVals(SECCNT(p->StartingOffset),SECCNT(p->PartitionLength),spt,h,&chs);
                SetPartitionTableEntry(PartitionTable+j,
                                       p->BootIndicator,
                                       p->PartitionType,
                                       SECCNT(p->StartingOffset) - BootSector,
                                       SECCNT(p->PartitionLength),
                                       &chs
                                      );
                break;
            }
            Update = Update || p->RewritePartition;

            if(p->PartitionType != SYSID_UNUSED) {
                UsedCount++;
            }
        }
        if(Update || !UsedCount) {
            if((status = LowWriteSectors(Handle,bps,BootSector,1,SectorBuffer)) != ESUCCESS) {
                LowCloseDisk(Handle);
                AlDeallocateHeap(SectorBuffer);
                return(status);
            }
        }
        BootSector = NextBootSector;
    }

    LowCloseDisk(Handle);
    AlDeallocateHeap(SectorBuffer);
    return(ESUCCESS);
}
