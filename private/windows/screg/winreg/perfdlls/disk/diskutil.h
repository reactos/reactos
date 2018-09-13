#ifndef __DISKUTIL_H_
#define __DISKUTIL_H_

#ifndef _DBG_PRINT_INSTANCES
#define _DBG_PRINT_INSTANCES 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <ntdddisk.h>

extern BOOL                 bUseNT4InstanceNames;

#define DU_MAX_VOLUMES      ((WORD)0xFFFF)
#define DU_MAX_DRIVES       ((WORD)0xFFFF)
#define DU_MAX_PARTITIONS   ((WORD)0xFFFF)

#define DVE_DEV_NAME_LEN    ((DWORD)368)

typedef struct _DRIVE_VOLUME_ENTRY {
    union {
        DWORD   dwDriveId;      // 00-03
        struct {
            WORD    wPartNo;
            WORD    wDriveNo;
        };
    };                  
    WORD    wcDriveLetter;      // 04-05
    WORD    wReserved;          // 06-07
    union {
        WCHAR   szVolumeManager[8];
        LONGLONG    llVolMgr[2];
    };                          // 08-17
    DWORD   dwVolumeNumber;     // 18-1B
    HANDLE  hVolume;            // 1C-17
    WCHAR   wszInstanceName[DVE_DEV_NAME_LEN];  // 20-1FF
} DRIVE_VOLUME_ENTRY, *PDRIVE_VOLUME_ENTRY;

#define LL_PARTMGR_0    ((LONGLONG)0x0074007200610050) //"Part"
#define LL_PARTMGR_1    ((LONGLONG)0x002000720067006D) //"mgr "

#define LL_PHYSDISK_0   ((LONGLONG)0x0073007900680050) //"Phys"
#define LL_PHYSDISK_1   ((LONGLONG)0x006B007300690044) //"Disk"

#define LL_LOGIDISK_0   ((LONGLONG)0x00690067006F004C) //"Logi"
#define LL_LOGIDISK_1   ((LONGLONG)0x006B007300690044) //"Disk"

__inline
BOOL
IsPhysicalDrive (PDISK_PERFORMANCE pPerfInf) {
    if (((*(LONGLONG *)&(pPerfInf->StorageManagerName[0]) == LL_PARTMGR_0) &&
         (*(LONGLONG *)&(pPerfInf->StorageManagerName[4]) == LL_PARTMGR_1)) ||
        ((*(LONGLONG *)&(pPerfInf->StorageManagerName[0]) == LL_PHYSDISK_0) &&
         (*(LONGLONG *)&(pPerfInf->StorageManagerName[4]) == LL_PHYSDISK_1))) {
        return TRUE;
    } else {
        return FALSE;
    }
}

DWORD
BuildPhysDiskList (
    HANDLE  hDiskPerf,
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries
);

DWORD
MakePhysDiskInstanceNames (
    PDRIVE_VOLUME_ENTRY pPhysDiskList,
    DWORD               dwNumPhysDiskListItems,
    LPDWORD             pdwMaxDriveNo,
    PDRIVE_VOLUME_ENTRY pVolumeList,
    DWORD               dwNumVolumeListItems
);

DWORD
BuildVolumeList (
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries
);

DWORD
MapLoadedDisks (
    HANDLE  hDiskPerf,
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries,
    LPDWORD             pdwMaxVolumeNo,
    LPDWORD             pdwWmiItemCount
);

DWORD
GetDriveNameString (
    LPCWSTR             szDevicePath,
    DWORD               cchDevicePathSize,
    PDRIVE_VOLUME_ENTRY pList,
    DWORD               dwNumEntries,
    LPWSTR              szNameBuffer,
    LPDWORD             pcchNameBufferSize,
    LPCWSTR             szVolumeManagerName,
    DWORD               dwVolumeNumber,
    HANDLE              *phVolume
);

DWORD
CompressPhysDiskTable (
    PDRIVE_VOLUME_ENTRY     pOrigTable,
    DWORD                   dwOrigCount,
    PDRIVE_VOLUME_ENTRY     pNewTable,
    DWORD                   dwNewCount
);

BOOL
GetPhysicalDriveNameString (
    DWORD                   dwDriveNumber,    
    PDRIVE_VOLUME_ENTRY     pList,
    DWORD                   dwNumEntries,
    LPWSTR                  szNameBuffer
);

#if DBG

extern ULONG_PTR HeapUsed;

VOID
PerfDiskDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x)   PerfDiskDebugPrint x

#else

#define DebugPrint(x)
#endif // DBG

#ifdef __cplusplus
}
#endif
#endif // __DISKUTIL_H_
