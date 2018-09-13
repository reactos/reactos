#include <ntos.h>
#include <ntdddisk.h>
#include <arc.h>
#include <arccodes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//
// Define special character values.
//
#define ASCI_NUL   0x00
#define ASCI_BEL   0x07
#define ASCI_BS    0x08
#define ASCI_HT    0x09
#define ASCI_LF    0x0A
#define ASCI_VT    0x0B
#define ASCI_FF    0x0C
#define ASCI_CR    0x0D
#define ASCI_CSI   0x9B
#define ASCI_ESC   0x1B
#define ASCI_SYSRQ 0x80

//
// Locally defined error codes
//
#define EBADSYNTAX   EMAXIMUM

#define MAX_COMPONENTS 20

ARC_STATUS
AlGetEnvVarComponents (
    IN  PCHAR  EnvValue,
    OUT PCHAR  **EnvVarComponents,
    OUT PULONG PNumComponents
    );

ARC_STATUS
AlFreeEnvVarComponents (
    IN PCHAR *EnvVarComponents
    );

BOOLEAN
AlFindNextMatchComponent(
    IN PCHAR EnvValue,
    IN PCHAR MatchValue,
    IN ULONG StartComponent,
    OUT PULONG MatchComponent OPTIONAL
    );

ARC_STATUS
AlAddSystemPartition(
    IN PCHAR NewSystemPartition
    );

ARC_STATUS
AlMemoryInitialize (
    ULONG StackPages,
    ULONG HeapPages
    );

PVOID
AlAllocateHeap (
    IN ULONG Size
    );

PVOID
AlDeallocateHeap (
    IN PVOID HeapAddress
    );

PVOID
AlReallocateHeap (
    IN PVOID HeapAddress,
    IN ULONG NewSize
    );

BOOLEAN
AlValidateHeap(
    IN BOOLEAN DumpHeap
    );

BOOLEAN
AlInitializeMenuPackage(
    VOID
    );

BOOLEAN
AlNewMenu(
    PVOID *MenuID
    );

VOID
AlFreeMenu(
    PVOID MenuID
    );

BOOLEAN                     // fails if OOM
AlAddMenuItem(
    PVOID MenuID,
    PCHAR Text,
    ULONG AssociatedData,
    ULONG Attributes        // currently unused
    );

BOOLEAN                     // fails if OOM
AlAddMenuItems(
    PVOID MenuID,
    PCHAR Text[],           // NOTE: associated data for each item is its
    ULONG ItemCount         // index in the array.
    );

BOOLEAN                     // FALSE if user escaped
AlDisplayMenu(
    PVOID   MenuID,
    BOOLEAN PrintOnly,
    ULONG   AssociatedDataOfDefaultChoice,
    ULONG  *AssociatedDataOfChoice,
    ULONG   Row,
    PCHAR   MenuName        // may be NULL
    );

ULONG
AlGetMenuNumberItems(
    PVOID MenuID
    );

ULONG
AlGetMenuAssociatedData(
    PVOID   MenuID,
    ULONG   n
    );

ARC_STATUS
AlGetMenuSelection(
    IN  PCHAR   szTitle,
    IN  PCHAR   *rgszSelections,
    IN  ULONG   crgsz,
    IN  ULONG   crow,
    IN  ULONG   irgszDefault,
    OUT PULONG  pirgsz,
    OUT PCHAR   *pszSelection
    );

VOID
AlWaitKey(
    PCHAR Prompt            // uses default if NULL
    );

VOID
vAlStatusMsg(
    IN ULONG   Row,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    IN va_list ArgumentList
    );

VOID
AlStatusMsg(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    );

VOID
AlStatusMsgNoWait(
    IN ULONG   TopRow,
    IN ULONG   BottomRow,
    IN BOOLEAN Error,
    IN PCHAR   FormatString,
    ...
    );

VOID
AlClearStatusArea(
    IN ULONG TopRow,
    IN ULONG BottomRow
    );

PCHAR
AlStrDup(
    IN  PCHAR   szString
    );

PCHAR
AlCombinePaths (
    IN  PCHAR   szPath1,
    IN  PCHAR   szPath2
    );

VOID
AlFreeArray (
    IN  BOOLEAN fFreeArray,
    IN  PCHAR   *rgsz,
    IN  ULONG   csz
    );

ARC_STATUS
AlGetBase (
    IN  PCHAR   szPath,
    OUT PCHAR   *pszBase
    );

//
// Define types of adapters that can be booted from.
//
typedef enum _ADAPTER_TYPES {
    AdapterEisa,
    AdapterScsi,
    AdapterMulti,
    AdapterMaximum
} ADAPTER_TYPES;

//
// Define type of controllers that can be booted from.
//
typedef enum _CONTROLLER_TYPES {
    ControllerDisk,
    ControllerCdrom,
    ControllerMaximum
} CONTROLLER_TYPES;

//
// Define type of peripheral that can be booted from.
//
typedef enum _PERIPHERAL_TYPES {
    PeripheralRigidDisk,
    PeripheralFloppyDisk,
    PeripheralMaximum
} PERIPHERAL_TYPES;

//
// Define type of token we are referring to
//
typedef enum _TOKEN_TYPES {
    AdaptType,
    ControllerType,
    PeripheralType
} TOKEN_TYPES;

//
// Define error codes
//
#define INVALID_TOKEN_TYPE  ~0L
#define INVALID_TOKEN_VALUE ~1L

//
// token string is searched for next name(unit)
//
PCHAR
AlGetNextArcNameToken (
    IN PCHAR TokenString,
    OUT PCHAR OutputToken,
    OUT PULONG UnitNumber
    );

//
// If invalid tokentype or tokenvalue passed in the error codes defined
// above are returned, else the enumeration of the token type is returned
//
ULONG
AlMatchArcNameToken (
    IN PCHAR TokenValue,
    IN TOKEN_TYPES TokenType
    );

ARC_STATUS
FdiskInitialize(
    VOID
    );

VOID
FdiskCleanUp(
    VOID
    );

VOID
ConfigureSystemPartitions(
    VOID
    );

VOID
ConfigureOSPartitions(
    VOID
    );

ULONG
AlPrint (
    PCHAR Format,
    ...
    );

extern char MSGMARGIN[];

#define AlClearScreen() \
    AlPrint("%c2J", ASCI_CSI)

#define AlClearLine() \
    AlPrint("%c2K", ASCI_CSI)

#define AlSetScreenColor(FgColor, BgColor) \
    AlPrint("%c3%dm", ASCI_CSI, (UCHAR)FgColor); \
    AlPrint("%c4%dm", ASCI_CSI, (UCHAR)BgColor)

#define AlSetScreenAttributes( HighIntensity, Underscored, ReverseVideo ) \
    AlPrint("%c0m", ASCI_CSI); \
    if (HighIntensity) { \
    AlPrint("%c1m", ASCI_CSI); \
    } \
    if (Underscored) { \
    AlPrint("%c4m", ASCI_CSI); \
    } \
    if (ReverseVideo) { \
    AlPrint("%c7m", ASCI_CSI); \
    }

#define AlSetPosition( Row, Column ) \
    AlPrint("%c%d;%dH", ASCI_CSI, Row, Column)

BOOLEAN             // false if user escaped.
AlGetString(
    OUT PCHAR String,
    IN  ULONG StringLength
    );

#define     AllocateMemory(size)            AlAllocateHeap(size)
#define     ReallocateMemory(block,size)    AlReallocateHeap(block,size)
#define     FreeMemory(block)               AlDeallocateHeap(block)

#define     OK_STATUS                       ESUCCESS
#define     RETURN_OUT_OF_MEMORY            return(ENOMEM)

#define LOWPART(x)      ((x).LowPart)

#define ONE_MEG         (1024*1024)

ULONG
SIZEMB(
    IN LARGE_INTEGER ByteCount
    );

#define ENTRIES_PER_BOOTSECTOR          4

/*
    This structure is used to hold the information returned by the
    get drive geometry call.
*/
typedef struct _tagDISKGEOM {
    LARGE_INTEGER   Cylinders;
    ULONG           Heads;
    ULONG           SectorsPerTrack;
    ULONG           BytesPerSector;
    // These two are not part of drive geometry info, but calculated from it.
    ULONG           BytesPerCylinder;
    ULONG           BytesPerTrack;
} DISKGEOM,*PDISKGEOM;


/*
    These structures are used in doubly-linked per disk lists that
    describe the layout of the disk.

    Free spaces are indicated by entries with a SysID of 0 (note that
    these entries don't actually appear anywhere on-disk!)

    The partition number is the number the system will assign to
    the partition in naming it.  For free spaces, this is the number
    that the system WOULD assign to it if it was a partition.
    The number is good only for one transaction (create or delete),
    after which partitions must be renumbered.

*/

typedef struct _tagPARTITION {
    struct _tagPARTITION  *Next;
    struct _tagPARTITION  *Prev;
    LARGE_INTEGER          Offset;
    LARGE_INTEGER          Length;
    ULONG                  Disk;
    ULONG                  OriginalPartitionNumber;
    ULONG                  PartitionNumber;
    ULONG                  PersistentData;
    BOOLEAN                Update;
    BOOLEAN                Active;
    BOOLEAN                Recognized;
    UCHAR                  SysID;
} PARTITION,*PPARTITION;

typedef struct _tagREGION_DATA {
    PPARTITION      Partition;
    LARGE_INTEGER   AlignedRegionOffset;
    LARGE_INTEGER   AlignedRegionSize;
} REGION_DATA,*PREGION_DATA;


#if DBG

#define ASRT(x)   if(!(x)) { char c; ULONG n;                                                      \
                             AlPrint("\r\nAssertion failure in %s, line %u\r\n",__FILE__,__LINE__);\
                             AlPrint("Press return to exit\r\n");                                  \
                             ArcRead(ARC_CONSOLE_INPUT,&c,1,&n);                                   \
                             ArcEnterInteractiveMode();                                                  \
                           }

#else

#define ASRT(x)

#endif

ARC_STATUS
FmtIsFatPartition(
    IN  ULONG    PartitionId,
    IN  ULONG       SectorSize,
    OUT PBOOLEAN    IsFatPartition
    );

ARC_STATUS
FmtIsFat(
    IN  PCHAR       PartitionPath,
    OUT PBOOLEAN    IsFatPartition
    );

ARC_STATUS
FmtFatFormat(
    IN  PCHAR   PartitionPath,
    IN  ULONG   HiddenSectorCount
    );

ARC_STATUS
FmtQueryFatPartitionList(
    OUT PCHAR** FatPartitionList,
    OUT PULONG  ListLength
    );

ARC_STATUS
FmtFreeFatPartitionList(
    IN OUT  PCHAR*  FatPartitionList,
    IN      ULONG   ListLength
    );

ARC_STATUS
LowOpenDisk(
    IN  PCHAR       DevicePath,
    OUT PULONG   DiskId
    );

ARC_STATUS
LowCloseDisk(
    IN  ULONG    DiskId
    );

ARC_STATUS
LowGetDriveGeometry(
    IN  PCHAR   DevicePath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    );

ARC_STATUS
LowGetPartitionGeometry(
    IN  PCHAR   PartitionPath,
    OUT PULONG  TotalSectorCount,
    OUT PULONG  SectorSize,
    OUT PULONG  SectorsPerTrack,
    OUT PULONG  Heads
    );

ARC_STATUS
LowReadSectors(
    IN  ULONG    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    OUT PVOID       Buffer
    );

ARC_STATUS
LowWriteSectors(
    IN  ULONG    VolumeId,
    IN  ULONG       SectorSize,
    IN  ULONG       StartingSector,
    IN  ULONG       NumberOfSectors,
    IN  PVOID       Buffer
    );

ARC_STATUS
LowQueryPathFromComponent(
    IN  PCONFIGURATION_COMPONENT    Component,
    OUT PCHAR*                      Path
    );

ARC_STATUS
LowQueryComponentList(
    IN  CONFIGURATION_CLASS*        ConfigClass OPTIONAL,
    IN  CONFIGURATION_TYPE*         ConfigType OPTIONAL,
    OUT PCONFIGURATION_COMPONENT**  ComponentList,
    OUT PULONG                      ListLength
    );

ARC_STATUS
LowQueryPathList(
    IN  CONFIGURATION_CLASS*        ConfigClass OPTIONAL,
    IN  CONFIGURATION_TYPE*         ConfigType OPTIONAL,
    OUT PCHAR**                     PathList,
    OUT PULONG                      ListLength
    );

ARC_STATUS
LowFreePathList(
    IN  PCHAR*  PathList,
    IN  ULONG   ListLength
    );

ARC_STATUS
LowQueryFdiskPathList(
    OUT PCHAR** PathList,
    OUT PULONG  ListLength
    );

ARC_STATUS
LowFreeFdiskPathList(
    IN OUT  PCHAR*  PathList,
    IN      ULONG   ListLength
    );

ARC_STATUS
LowGetDiskLayout(
    IN  PCHAR                      Path,
    OUT PDRIVE_LAYOUT_INFORMATION *DriveLayout
    );

ARC_STATUS
LowSetDiskLayout(
    IN PCHAR                     Path,
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    );

typedef enum { REGION_PRIMARY,
               REGION_EXTENDED,
               REGION_LOGICAL
             } REGION_TYPE;

enum {
        SYSID_UNUSED     = 0,
        SYSID_EXTENDED   = 5,
        SYSID_BIGFAT     = 6,
        SYSID_IFS        = 7
     };

typedef struct _tagREGION_DESCRIPTOR {
    ULONG           PersistentData;
    ULONG           Disk;
    ULONG           SizeMB;
    ULONG           PartitionNumber;
    ULONG           OriginalPartitionNumber;
    REGION_TYPE     RegionType;
    BOOLEAN         Active;
    BOOLEAN         Recognized;
    UCHAR           SysID;
    PVOID           Reserved;
} REGION_DESCRIPTOR,*PREGION_DESCRIPTOR;

ULONG
GetDiskCount(
    VOID
    );

PCHAR
GetDiskName(
    ULONG Disk
    );

ULONG
DiskSizeMB(
    IN ULONG Disk
    );

ARC_STATUS
GetDiskRegions(
    IN  ULONG               Disk,
    IN  BOOLEAN             WantUsedRegions,
    IN  BOOLEAN             WantFreeRegions,
    IN  BOOLEAN             WantPrimaryRegions,
    IN  BOOLEAN             WantLogicalRegions,
    OUT PREGION_DESCRIPTOR *Region,
    OUT ULONG              *RegionCount
    );

#define GetAllDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,TRUE,TRUE,TRUE,regions,count)

#define GetFreeDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,FALSE,TRUE,TRUE,TRUE,regions,count)

#define GetUsedDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,FALSE,TRUE,TRUE,regions,count)

#define GetPrimaryDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,TRUE,TRUE,FALSE,regions,count)

#define GetLogicalDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,TRUE,FALSE,TRUE,regions,count)

#define GetUsedPrimaryDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,FALSE,TRUE,FALSE,regions,count)

#define GetUsedLogicalDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,TRUE,FALSE,FALSE,TRUE,regions,count)

#define GetFreePrimaryDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,FALSE,TRUE,TRUE,FALSE,regions,count)

#define GetFreeLogicalDiskRegions(disk,regions,count) \
        GetDiskRegions(disk,FALSE,TRUE,FALSE,TRUE,regions,count)

VOID
FreeRegionArray(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              RegionCount
    );

ARC_STATUS
IsAnyCreationAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT PBOOLEAN AnyAllowed,
    OUT PBOOLEAN PrimaryAllowed,
    OUT PBOOLEAN ExtendedAllowed,
    OUT PBOOLEAN LogicalAllowed
    );

ARC_STATUS
IsCreationOfPrimaryAllowed(
    IN  ULONG    Disk,
    IN  BOOLEAN  AllowMultiplePrimaries,
    OUT PBOOLEAN Allowed
    );

ARC_STATUS
IsCreationOfExtendedAllowed(
    IN  ULONG    Disk,
    OUT PBOOLEAN Allowed
    );

ARC_STATUS
IsCreationOfLogicalAllowed(
    IN  ULONG    Disk,
    OUT PBOOLEAN Allowed
    );

ARC_STATUS
DoesAnyPartitionExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN AnyExists,
    OUT PBOOLEAN PrimaryExists,
    OUT PBOOLEAN ExtendedExists,
    OUT PBOOLEAN LogicalExists
    );

ARC_STATUS
DoesAnyPrimaryExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN Exists
    );

ARC_STATUS
DoesExtendedExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN Exists
    );

ARC_STATUS
DoesAnyLogicalExist(
    IN  ULONG    Disk,
    OUT PBOOLEAN Exists
    );

BOOLEAN
IsExtended(
    IN UCHAR SysID
    );

VOID
SetPartitionActiveFlag(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              value
    );

ARC_STATUS
CreatePartition(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type
    );

ARC_STATUS
CreatePartitionEx(
    IN PREGION_DESCRIPTOR Region,
    IN LARGE_INTEGER      MinimumSize,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        Type,
    IN UCHAR              SysId
    );

ARC_STATUS
DeletePartition(
    IN PREGION_DESCRIPTOR Region
    );

ULONG
GetHiddenSectorCount(
    ULONG Disk,
    ULONG Partition
    );

VOID
SetSysID(
    IN ULONG Disk,
    IN ULONG Partition,
    IN UCHAR SysID
    );

VOID
SetSysID2(
    IN PREGION_DESCRIPTOR Region,
    IN UCHAR              SysID
    );

PCHAR
GetSysIDName(
    UCHAR SysID
    );

ARC_STATUS
CommitPartitionChanges(
    IN ULONG Disk
    );

BOOLEAN
HavePartitionsBeenChanged(
    IN ULONG Disk
    );

VOID
FdMarkDiskDirty(
    IN ULONG Disk
    );

VOID
FdSetPersistentData(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              Data
    );

ULONG
FdGetMinimumSizeMB(
    IN ULONG Disk
    );

ULONG
FdGetMaximumSizeMB(
    IN PREGION_DESCRIPTOR Region,
    IN REGION_TYPE        CreationType
    );

LARGE_INTEGER
FdGetExactSize(
    IN PREGION_DESCRIPTOR Region,
    IN BOOLEAN            ForExtended
    );

LARGE_INTEGER
FdGetExactOffset(
    IN PREGION_DESCRIPTOR Region
    );

BOOLEAN
FdCrosses1024Cylinder(
    IN PREGION_DESCRIPTOR Region,
    IN ULONG              CreationSizeMB,
    IN REGION_TYPE        RegionType
    );

ULONG
FdGetDiskSignature(
    IN ULONG Disk
    );

VOID
FdSetDiskSignature(
    IN ULONG Disk,
    IN ULONG Signature
    );

BOOLEAN
IsDiskOffLine(
    IN ULONG Disk
    );



typedef enum _BOOT_VARIABLES {
    LoadIdentifierVariable,
    SystemPartitionVariable,
    OsLoaderVariable,
    OsLoadPartitionVariable,
    OsLoadFilenameVariable,
    OsLoadOptionsVariable,
    MaximumBootVariable
} BOOT_VARIABLE;

extern PCHAR BootString[];

VOID
JzDeleteVariableSegment (
    PCHAR VariableName,
    ULONG Selection
    );

#define MAXIMUM_ENVIRONMENT_VALUE 256
