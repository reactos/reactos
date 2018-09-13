#ifndef _PERFDISK_H_
#define _PERFDISK_H_
#include "diskutil.h"

//
//  Definition of handle table for disks
//

//  Information for collecting disk driver statistics

extern  HANDLE  hEventLog;       // handle to event log
extern  HANDLE  hLibHeap;       // handle to DLL heap            
extern  LPWSTR  wszTotal;

extern  BOOL    bShownDiskPerfMessage;  // flag to reduce eventlog noise
extern  BOOL    bShownDiskVolumeMessage;

extern  const   WCHAR  szTotalValue[];
extern  const   WCHAR  szDefaultTotalString[];

extern  WMIHANDLE   hWmiDiskPerf;
extern  LPBYTE  WmiBuffer;

extern PDRIVE_VOLUME_ENTRY  pPhysDiskList;
extern DWORD                dwNumPhysDiskListEntries;
extern PDRIVE_VOLUME_ENTRY  pVolumeList;
extern DWORD                dwNumVolumeListEntries;
extern BOOL                 bRemapDriveLetters;
extern DWORD                dwMaxVolumeNumber;
extern DWORD                dwWmiDriveCount;

extern DOUBLE               dSysTickTo100Ns;

DWORD APIENTRY MapDriveLetters();   // function to map drive letters to volume  or device name

//  logidisk.c
PM_LOCAL_COLLECT_PROC CollectLDiskObjectData;

//  Physdisk.c
PM_LOCAL_COLLECT_PROC CollectPDiskObjectData;

#endif // _PERFDISK_H_
