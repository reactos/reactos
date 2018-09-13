#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <wtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <ftapi.h>
#include <mountmgr.h>
#include <wmium.h>
#include <wmiguid.h>
#include <assert.h>

#include "diskutil.h"

#define HEAP_FLAGS  (HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS)
#define INITIAL_MOUNTMGR_BUFFER_SIZE    8192

// sizes are in characters (not bytes)
#define SIZE_OF_DOSDEVICES  12L     // size of "\DosDevices\" string
#define SIZE_OF_DEVICE       8L     // size of "\Device\" string
#define SIZE_OF_HARDDISK     8L     // size of "Harddisk" string

static const LONGLONG   llDosDevicesId  = 0x0073006f0044005c; // "\Dos"
static const LONGLONG   llFloppyName    = 0x0070006f006c0046; // "Flop"
static const LONGLONG   llCdRomName     = 0x006f005200640043; // "CdRo"

BOOL                bUseNT4InstanceNames = FALSE;

DWORD
GetDriveNumberFromDevicePath (
    LPCWSTR szDevicePath,
    LPDWORD pdwDriveId
)
/*
    evaluates the device path and returns the Drive Number 
    if the string is in the following format

        \Device\HarddiskX

    where X is a decimal number (consisting of 1 or more decimal
    digits representing a value between 0 and 65535 inclusive)

    The function returns a value of:
        ERROR_SUCCESS           if successful
        ERROR_INVALID_PARAMETER if the input string is incorrectly formatted
        ERROR_INVALID_DATA      if the volume number is too big
        
*/
{
    PWCHAR  pNumberChar;
    LONG    lValue;
    DWORD   dwDriveAndPartition;
    DWORD   dwReturn;
    DWORD   dwRetry = 4;

    // validate the input arguments
    assert (szDevicePath != NULL);
    assert (*szDevicePath != 0);
    assert (pdwDriveId != NULL);

    // start at the beginning of the string
    pNumberChar = (PWCHAR)szDevicePath;

    // make sure it starts with a backslash. if not then
    // try the next 4 characters to see if there's some garbage
    // in front of the string.
    while (*pNumberChar != L'\\') {
        --dwRetry;
        if (dwRetry) {
            pNumberChar++;
        } else {
            break;
        }
    }

    if (*pNumberChar == L'\\') {
        // and go to the end of the Device string to see 
        // if this is a harddisk path
       pNumberChar += SIZE_OF_DEVICE;
        if (*pNumberChar == L'H') {
            // this is a volume Entry so go to the number
            pNumberChar += SIZE_OF_HARDDISK;
            lValue = _wtol(pNumberChar);
            if (lValue <= (LONG)0x0000FFFF) {
                // load the drive number into the DWORD
                dwDriveAndPartition = (DWORD)lValue;
                *pdwDriveId = dwDriveAndPartition;
                dwReturn = ERROR_SUCCESS;
            } else {
                // drive ID Is out of range
                dwReturn = ERROR_INVALID_DATA;
            }
        } else {
            // not a valid path
            dwReturn = ERROR_INVALID_PARAMETER;
        }
    } else {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

    return dwReturn;
}

DWORD
GetSymbolicLink (
    LPCWSTR szDeviceString,
    LPWSTR  szLinkString,
    LPDWORD pcchLength
)
/*
    this functions opens the device string as a symbolic link
    and returns the corresponding link string
*/
{
    OBJECT_ATTRIBUTES   Attributes;
    UNICODE_STRING      ObjectName;
    UNICODE_STRING      LinkName;
    WORD                wDevStrLen;
    NTSTATUS            ntStatus;
    DWORD               dwRetSize = 0;
    DWORD               dwReturnStatus;
    HANDLE              hObject = NULL;

    // validate arguments
    assert (szDeviceString != NULL);
    assert (*szDeviceString != 0);
    assert (szLinkString != NULL);
    assert (pcchLength != NULL);
    assert (*pcchLength > 0);

    // get the length of the input string
    wDevStrLen = (WORD)lstrlenW(szDeviceString);

    // create the object name UNICODE string structure
    ObjectName.Length = (WORD)(wDevStrLen * sizeof (WCHAR));
    ObjectName.MaximumLength = (WORD)((wDevStrLen + 1) * sizeof (WCHAR));
    ObjectName.Buffer = (LPWSTR)szDeviceString;

    // initialize the object attributes for the open call
    InitializeObjectAttributes( &Attributes,
                            &ObjectName,
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL );

    // open the name as a symbolic link, if this fails, the input
    // name is probably not a link

    ntStatus = NtOpenSymbolicLinkObject(
                            &hObject,
                            SYMBOLIC_LINK_QUERY,
                            &Attributes);

    if (NT_SUCCESS(ntStatus)) {
        // init a Unicode String for the return buffer using the caller's
        // buffer
        LinkName.Length = 0;
        LinkName.MaximumLength = (WORD)(*pcchLength * sizeof (WCHAR));
        LinkName.Buffer = szLinkString;

        // and look up the link
        ntStatus = NtQuerySymbolicLinkObject(
            hObject, &LinkName, &dwRetSize);

        if (NT_SUCCESS(ntStatus)) {
            // buffer is loaded so set the return status and length
            *pcchLength = LinkName.Length / sizeof (WCHAR);
            // make sure the string is 0 terminated
            szLinkString[*pcchLength] = 0;
            dwReturnStatus = ERROR_SUCCESS;
        } else {
            // unable to look up the link so return the error
            dwReturnStatus = RtlNtStatusToDosError(ntStatus);
        }
        
        // close the handle to the link
        NtClose (hObject);
    } else {
        dwReturnStatus = RtlNtStatusToDosError(ntStatus);
    }
    
    return dwReturnStatus;  
}


DWORD
BuildPhysDiskList (
    HANDLE  hDiskPerf,
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries
)
{
    DWORD   status = ERROR_SUCCESS; // return value of the function
    HANDLE  hWmiDiskPerf = NULL;    // local handle value 
    DWORD   dwLocalMaxVolNo = 0;
    DWORD   dwLocalWmiItemCount = 0;

    // WMI Buffer variables
    DWORD   WmiBufSize;
    DWORD   WmiAllocSize = 0x8000;     
    LPBYTE  WmiBuffer = NULL;

    // WMI buffer processing variables
    PWNODE_ALL_DATA     WmiDiskInfo;
    DISK_PERFORMANCE    *pDiskPerformance;    //  Disk driver returns counters here
    DWORD               dwInstanceNameOffset;
    WORD                wNameLen;   // string length is first word in buffer
    LPWSTR              wszInstanceName; // pointer to string in WMI buffer
    
    WCHAR   wszInstName[MAX_PATH];
    DWORD   dwBytesToCopy;

    DWORD   dwListEntry;

    BOOL    bNotDone = TRUE;

    DWORD   dwLocalStatus;
    DWORD   dwLocalDriveId;
    DWORD   dwLocalPartition;
    WCHAR   szDrivePartString[MAX_PATH];
    DWORD   dwSymbLinkLen;
    WCHAR   szSymbLinkString[MAX_PATH];


    HANDLE  hProcessHeap = GetProcessHeap();

    if (hDiskPerf == NULL) {
        // open handle to disk perf device driver
        status = WmiOpenBlock (
            (GUID *)&DiskPerfGuid,
            GENERIC_READ,
            &hWmiDiskPerf);
    } else {
        // use caller's handle
        hWmiDiskPerf = hDiskPerf;
    }

    assert (pList != NULL);
    assert (pdwNumEntries != NULL);

    DebugPrint((3, "BuildPhysDisk: dwEntries is %d\n", *pdwNumEntries));
    dwListEntry = 0;

    if (status == ERROR_SUCCESS) {
        // allocate a buffer to send to WMI to get the diskperf data
        WmiBufSize = WmiAllocSize;
        WmiBuffer = (LPBYTE)HeapAlloc (hProcessHeap, 
            HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
            WmiBufSize);

        if (WmiBuffer  == NULL) {
            status = ERROR_OUTOFMEMORY;
        } else {
#if DBG
            HeapUsed += WmiBufSize;
            DebugPrint((4,"\tWmiBuffer add %d to %d\n", WmiBufSize, HeapUsed));
#endif
            status = WmiQueryAllDataW (
                hWmiDiskPerf,
                &WmiBufSize,
                WmiBuffer);

            DebugPrint((2,
                "BuildPhysDisk: WmiQueryAllDataW status1=%d\n",
                status));
#if DBG
            if (!HeapValidate(hProcessHeap, 0, WmiBuffer)) {
                DebugPrint((2,
                    "BuildPhysDisk: WmiQueryAllDataW corrupted WmiBuffer\n"));
                DbgBreakPoint();
            }
#endif

            // if buffer size attempted is too big or too small, resize
            if ((WmiBufSize > 0) && (WmiBufSize != WmiAllocSize)) {
                WmiBuffer = (LPBYTE)HeapReAlloc (hProcessHeap,
                    HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
                    WmiBuffer, WmiBufSize);

                if (WmiBuffer == NULL) {
                    // reallocation failed so bail out
                    status = ERROR_OUTOFMEMORY;
                } else {
#if DBG
                    HeapUsed += (WmiBufSize - WmiAllocSize);
                    DebugPrint((4, "\tRealloc WmiBuffer old %d new %d to %d\n",
                        WmiAllocSize, WmiBufSize, HeapUsed));
                    WmiAllocSize = WmiBufSize;
                    if (!HeapValidate(hProcessHeap, 0, WmiBuffer)) {
                        DebugPrint((2, "\tHeapReAlloc is corrupted!\n"));
                        DbgBreakPoint();
                    }
#endif
                }
            }

            if (status == ERROR_INSUFFICIENT_BUFFER) {
                // if it didn't work because it was too small the first time
                // try one more time
                status = WmiQueryAllDataW (
                    hWmiDiskPerf,
                    &WmiBufSize,
                    WmiBuffer);
            
#if DBG
                if (!HeapValidate(hProcessHeap, 0, WmiBuffer)) {
                    DebugPrint((2,
                        "BuildPhysDisk: WmiQueryAllDataW2 corrupted WmiBuffer\n"));
                    DbgBreakPoint();
                }
#endif
                DebugPrint((2,
                    "BuildPhysDisk: WmiQueryAllDataW status2=%d\n",
                    status));

            } else {
                // it either worked the fisrt time or it failed because of 
                // something other than a buffer size problem
            }
        }

        if (status == ERROR_SUCCESS) {
            WmiDiskInfo = (PWNODE_ALL_DATA)WmiBuffer;
            // go through returned names and add to the buffer
            while (bNotDone) {
#if DBG
                if ((PCHAR) WmiDiskInfo > (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: WmiDiskInfo %d exceeded %d + %d\n",
                        WmiDiskInfo, WmiBuffer, WmiAllocSize));
                }
#endif
                pDiskPerformance = (PDISK_PERFORMANCE)(
                    (PUCHAR)WmiDiskInfo +  WmiDiskInfo->DataBlockOffset);

#if DBG
                if ((PCHAR) pDiskPerformance > (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: pDiskPerformance %d exceeded %d + %d\n",
                        pDiskPerformance, WmiBuffer, WmiAllocSize));
                }
#endif
        
                dwInstanceNameOffset = WmiDiskInfo->DataBlockOffset + 
                                      ((sizeof(DISK_PERFORMANCE) + 1) & ~1) ;

#if DBG
                if ((dwInstanceNameOffset+(PCHAR)WmiDiskInfo) > (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: dwInstanceNameOffset %d exceeded %d + %d\n",
                        dwInstanceNameOffset, WmiBuffer, WmiAllocSize));
                }
#endif
                // get length of string (it's a counted string) length is in chars
                wNameLen = *(LPWORD)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset);

#if DBG
                if ((wNameLen + (PCHAR)WmiDiskInfo + dwInstanceNameOffset) >
                         (PCHAR) WmiBuffer + WmiAllocSize) {
                    DebugPrint((2,
                        "BuildPhysDisk: wNameLen %d exceeded %d + %d\n",
                        wNameLen, WmiBuffer, WmiAllocSize));
                }
#endif
                if (wNameLen > 0) {
                    // just a sanity check here
                    assert (wNameLen < MAX_PATH);
                    // get pointer to string text
                    wszInstanceName = (LPWSTR)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset + sizeof(WORD));

                    // truncate to last characters if name is larger than the buffer in the table
                    if (wNameLen >= DVE_DEV_NAME_LEN) {
                        // copy the last DVE_DEV_NAME_LEN chars
                        wszInstanceName += (wNameLen  - DVE_DEV_NAME_LEN) + 1;
                        dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
                        wNameLen = DVE_DEV_NAME_LEN - 1;
                    } else {
                        dwBytesToCopy = wNameLen;
                    }
                    // copy it to the buffer to make it a SZ string
                    memcpy (wszInstName, &wszInstanceName[0], dwBytesToCopy);
                    // zero terminate it
                    wszInstName[wNameLen/sizeof(WCHAR)] = 0;

                    DebugPrint((2, "Checking PhysDisk: '%ws'\n",
                        wszInstName));

                    if (IsPhysicalDrive(pDiskPerformance)) {
                        // enum partitions
                        dwLocalStatus = GetDriveNumberFromDevicePath (wszInstName, &dwLocalDriveId);
                        if (dwLocalStatus == ERROR_SUCCESS) {
                            // then take the drive ID and find all the matching partitions with logical
                            // drives
                            for (dwLocalPartition = 0; 
                                dwLocalPartition <= 0xFFFF;
                                dwLocalPartition++) {
                                swprintf (szDrivePartString, L"\\Device\\Harddisk%d\\Partition%d",
                                    dwLocalDriveId, dwLocalPartition);
                                dwSymbLinkLen = sizeof (szSymbLinkString) / sizeof(szSymbLinkString[0]);
                                dwLocalStatus = GetSymbolicLink (szDrivePartString, 
                                    szSymbLinkString, &dwSymbLinkLen);
                                if (dwLocalStatus == ERROR_SUCCESS) {
                                    if (dwListEntry < *pdwNumEntries) {
                                        DebugPrint((2,
                                            "Adding Partition: '%ws' as '%ws'\n",
                                            szDrivePartString, szSymbLinkString));
                                        pList[dwListEntry].wPartNo = (WORD)dwLocalPartition;
                                        pList[dwListEntry].wDriveNo = (WORD)dwLocalDriveId;
                                        pList[dwListEntry].wcDriveLetter = 0;
                                        pList[dwListEntry].wReserved = 0;
                                        memcpy (&pList[dwListEntry].szVolumeManager, 
                                            pDiskPerformance->StorageManagerName,
                                            sizeof(pDiskPerformance->StorageManagerName));
                                        pList[dwListEntry].dwVolumeNumber = pDiskPerformance->StorageDeviceNumber;
                                        pList[dwListEntry].hVolume = NULL;
                                        memset (&pList[dwListEntry].wszInstanceName[0],
                                            0, (DVE_DEV_NAME_LEN * sizeof(WCHAR)));
                                        if (dwSymbLinkLen < DVE_DEV_NAME_LEN) {
                                            lstrcpyW (&pList[dwListEntry].wszInstanceName[0],
                                                szSymbLinkString);
                                        } else {
                                            memcpy (&pList[dwListEntry].wszInstanceName[0],
                                                szSymbLinkString, DVE_DEV_NAME_LEN * sizeof(WCHAR));
                                            pList[dwListEntry].wszInstanceName[DVE_DEV_NAME_LEN-1] = 0;
                                        }                                            
                                    } else {
                                        status = ERROR_INSUFFICIENT_BUFFER;
                                    }
                                    dwListEntry++;
                                } else {
                                    // that's it for this disk
                                    break;
                                }
                            }  // end of partition search
                        } // else unable to get the harddisk number from the path
                    } else {
                        // not a physical drive so ignore
                    }
                    // count the number of entries
                    dwLocalWmiItemCount++;
                } else {
                    // no string to examine (length == 0)
                }

                // bump pointers inside WMI data block
                if (WmiDiskInfo->WnodeHeader.Linkage != 0) {
                    // continue
                    WmiDiskInfo = (PWNODE_ALL_DATA) (
                        (LPBYTE)WmiDiskInfo + WmiDiskInfo->WnodeHeader.Linkage);
                } else {
                    bNotDone = FALSE;
                }
            } // end while looking through the WMI data block
        }

        if (hDiskPerf == NULL) {
            // then the disk perf handle is local so close it
            status = WmiCloseBlock (hWmiDiskPerf);
        }
    }

    if (WmiBuffer != NULL) {
        HeapFree (RtlProcessHeap(), 0, WmiBuffer);
#if DBG
        HeapUsed -= WmiBufSize;
        DebugPrint((4, "\tFreed WmiBuffer %d to %d\n", WmiBufSize, HeapUsed));
#endif
    }

    *pdwNumEntries = dwListEntry;
    DebugPrint((3,"BuildPhysDisk: Returning dwNumEntries=%d\n",*pdwNumEntries));

    return status;
}

DWORD
BuildVolumeList (
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries
)
/*

  Using the Mount manager, this function builds a list of all mounted 
  hard drive volumes (CD, Floppy & other types of disks are ignored).

  The calling function must pass in a buffer and indicate the maximum
  number of entries in the buffer. If successful, the buffer contains 
  one entry for each disk volume found and the number of entries used
  is returned

    pList           IN: pointer to a buffer that will receive the entries
                    OUT: buffer containing disk entries

    pdwNumEntries   IN: pointer to DWORD that specifies the max # of entries 
                        in the buffer referenced by pList
                    OUT: pointer to DWORD that contains the number of entries
                        written into the buffer referenced by pList
    pdwMaxVolume    IN: ignored
                    OUT: the max volume ID returned by the mount manager

  The function can return one of the following return values:

    ERROR_SUCCESS   if successful

    If unsuccessful:
        an error returned by 
*/
{
    DWORD       dwReturnValue = ERROR_SUCCESS;  // return value of function

    HANDLE      hMountMgr;      // handle to mount manger service
 
    // mount manager function variables
    PMOUNTMGR_MOUNT_POINTS  pMountPoints = NULL;
    MOUNTMGR_MOUNT_POINT    mountPoint;
    DWORD                   dwBufferSize = 0;
    DWORD                   dwReturnSize;
    BOOL                    bStatus;

    // processing loop functions
    DWORD                   dwListEntry;    // entry in caller's buffer
    DWORD                   dwBufEntry;     // entry in mount manager buffer
    PMOUNTMGR_MOUNT_POINT   point;          // the current entry 
    PWCHAR                  pDriveLetter;
    DWORD                   dwDone;

    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK   status_block;
    NTSTATUS          status;
    UNICODE_STRING    usVolume;
    WCHAR             szTempPath[MAX_PATH];
    LPWSTR            pThisChar;
    LPWSTR            szDeviceName;
    DWORD             dwBytesToCopy;
    BOOL              bNeedMoreData = TRUE;
    DWORD             dwRetryCount = 100;

    UINT              dwOrigErrorMode;

    HANDLE             hProcessHeap = GetProcessHeap();

    BOOL            bIsHardDisk;
    
    // pList can be NULL for size queries
    assert (pdwNumEntries != NULL);

    DebugPrint((3, "BuildVolumeList: Building %d entries\n", *pdwNumEntries));

    hMountMgr = CreateFile(MOUNTMGR_DOS_DEVICE_NAME, 0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);

    if (hMountMgr == INVALID_HANDLE_VALUE) {
        dwReturnValue = GetLastError();
        DebugPrint((2,
            "VolumeList: Mount Manager connection returned %d\n",
            dwReturnValue));
        goto BVL_ERROR_EXIT;
    }

    while ((bNeedMoreData) && (dwRetryCount)) {
        dwBufferSize += INITIAL_MOUNTMGR_BUFFER_SIZE;
        if (pMountPoints != NULL) {
            HeapFree (hProcessHeap, HEAP_FLAGS, pMountPoints);
            pMountPoints = NULL;
#if DBG
            HeapUsed -= dwBufferSize;
            DebugPrint((4,
                "\tFreed MountPoints %d to %d\n", dwBufferSize, HeapUsed));
#endif
        }
        pMountPoints = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc (
            hProcessHeap, HEAP_FLAGS, dwBufferSize);
        if (pMountPoints == NULL) {
            dwReturnValue = ERROR_OUTOFMEMORY;
            DebugPrint((2, "VolumeList: Buffer Alloc failed\n"));
            goto BVL_ERROR_EXIT;
        }

#if DBG
        HeapUsed += dwBufferSize;
        DebugPrint((4,
            "\tAdded MountPoints %d to %d\n", dwBufferSize, HeapUsed));
#endif
        dwReturnSize = 0;
        memset(&mountPoint, 0, sizeof(MOUNTMGR_MOUNT_POINT));
        bStatus = DeviceIoControl(hMountMgr,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    &mountPoint, sizeof(MOUNTMGR_MOUNT_POINT),
                    pMountPoints, dwBufferSize,
                    &dwReturnSize, NULL); 

        if (!bStatus) {
            dwReturnValue = GetLastError();
            if (dwReturnValue != ERROR_MORE_DATA) {
                DebugPrint((2,
                    "VolumeList: Mount Manager IOCTL returned %d\n",
                    dwReturnValue));
                goto BVL_ERROR_EXIT;
            } else {
                // we need a bigger buffer so try again
                dwReturnValue = ERROR_SUCCESS;
            }
            dwRetryCount--;
        } else {
            // everything worked so leave the loop
            bNeedMoreData = FALSE;
        }
    }

    if (!dwRetryCount)  {
        // then we gave up trying to get a big enough buffer so return an error
        dwReturnValue = ERROR_MORE_DATA;
    } else {
        // see if there's room in the caller's buffer for this data
        // **note that even though not all mounted drives will be returned
        // this is an easy and fast, if overstated, check
        // load size for caller to know required buffer size
        DebugPrint((2,
           "VolumeList: Mount Manager returned %d Volume entries\n",
           pMountPoints->NumberOfMountPoints));

        if (pMountPoints->NumberOfMountPoints > *pdwNumEntries) {
            *pdwNumEntries = (DWORD)pMountPoints->NumberOfMountPoints;
            if (pList != NULL) {
                // they passed in a buffer that wasn't big enough
                dwReturnValue = ERROR_INSUFFICIENT_BUFFER;
            } else {
                // they just wanted to know the size
                dwReturnValue = ERROR_SUCCESS;
            }
            goto BVL_ERROR_EXIT;
        }

        // assume there's room in the buffer now
        // load the caller's buffer
        
        dwOrigErrorMode = SetErrorMode (
            SEM_FAILCRITICALERRORS      |
            SEM_NOALIGNMENTFAULTEXCEPT  |
            SEM_NOGPFAULTERRORBOX       |
            SEM_NOOPENFILEERRORBOX);

        for (dwBufEntry=0, dwListEntry = 0; 
                dwBufEntry < pMountPoints->NumberOfMountPoints; 
                dwBufEntry++) {
            point = &pMountPoints->MountPoints[dwBufEntry];
            // there are 2 steps to complete to know this is a good
            // entry for the caller. so set the count to 2 and decrement
            // it as the steps are successful.
            dwDone = 2; 
            bIsHardDisk = TRUE;

            if (point->DeviceNameLength) {
                LONGLONG    *pSig;

                // device name is in bytes
                pList[dwListEntry].dwVolumeNumber = 0;
                szDeviceName = (LPWSTR)((PCHAR) pMountPoints + point->DeviceNameOffset);
                if ((DWORD)point->DeviceNameLength >= (DVE_DEV_NAME_LEN * sizeof(WCHAR))) {
                    // copy the last DVE_DEV_NAME_LEN chars
                    szDeviceName += ((DWORD)point->DeviceNameLength - DVE_DEV_NAME_LEN) + 1;
                    dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
                } else {
                    dwBytesToCopy = (DWORD)point->DeviceNameLength;
                }
                memcpy (pList[dwListEntry].wszInstanceName, szDeviceName, dwBytesToCopy);
                // null terminate
                assert ((dwBytesToCopy / sizeof(WCHAR)) < DVE_DEV_NAME_LEN);
                pList[dwListEntry].wszInstanceName[dwBytesToCopy / sizeof(WCHAR)] = 0;

                pSig = (LONGLONG *)&(pList[dwListEntry].wszInstanceName[SIZE_OF_DEVICE]);
                if ((*pSig == llFloppyName) || (*pSig == llCdRomName)) {
                    // this to avoid opening drives that we won't be collecting data from
                    bIsHardDisk = FALSE;
                }

                dwDone--;
            }

            if (point->SymbolicLinkNameLength) {
                pDriveLetter = (PWCHAR)((PCHAR)pMountPoints + point->SymbolicLinkNameOffset);
                // make sure this is a \DosDevices path
                if (*(LONGLONG *)pDriveLetter == llDosDevicesId) {
                    pDriveLetter += SIZE_OF_DOSDEVICES;
                    if (((*pDriveLetter >= L'A') && (*pDriveLetter <= L'Z')) ||
                        ((*pDriveLetter >= L'a') && (*pDriveLetter <= L'z'))) {
                        pList[dwListEntry].wcDriveLetter = towupper(*pDriveLetter);

                        memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

                        memcpy (szTempPath, 
                            (LPVOID)((PCHAR) pMountPoints + point->SymbolicLinkNameOffset),
                            point->SymbolicLinkNameLength);
                    
                        pThisChar = &szTempPath[point->SymbolicLinkNameLength / sizeof(WCHAR)];
                        *pThisChar++ = L'\\';
                        *pThisChar = 0;

                        // try to open a handle to the volume for free space info

                        if (bIsHardDisk) {

                            usVolume.Buffer = szTempPath;
                            usVolume.Length = (WORD)(point->SymbolicLinkNameLength + (WORD)sizeof(WCHAR));
                            usVolume.MaximumLength = (WORD)(usVolume.Length + (WORD)sizeof(WCHAR));

                            InitializeObjectAttributes(&objectAttributes,
                                       &usVolume,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

                            status = NtOpenFile(&pList[dwListEntry].hVolume,
                                (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                                &objectAttributes,
                                &status_block,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE
                                );
                    
                            if (status != STATUS_SUCCESS) {
                                // no free space available
                                pList[dwListEntry].hVolume = NULL;
                            }
                        } else {
                            status = STATUS_SUCCESS;
                            pList[dwListEntry].hVolume = NULL;
                        }

                        dwDone--;
                    }
                } else {
                    // this is some other type of string so ignore it
                }
            }

            if (dwDone == 0) {
#if _DBG_PRINT_INSTANCES
               WCHAR szDbgBuffer[512];
               swprintf (szDbgBuffer, L"\nPERFDISK:VolumeList -- Added %s as drive %c",
                   pList[dwListEntry].wszInstanceName, pList[dwListEntry].wcDriveLetter);
               OutputDebugStringW (szDbgBuffer);
#endif
                // then the data fields have been satisfied so 
                // this entry is done and we can now go 
                // to the next entry in the caller's buffer
                dwListEntry++;
            }
        }

        SetErrorMode (dwOrigErrorMode);

        // return the number of entries actually used here
        *pdwNumEntries = dwListEntry;
    }

BVL_ERROR_EXIT:
    CloseHandle(hMountMgr);

    if (pMountPoints != NULL) {
        HeapFree (hProcessHeap, 0, pMountPoints);
#if DBG
        DebugPrint((4,
            "\tFreed mountpoints %d to %d\n", dwBufferSize, HeapUsed));
        dwBufferSize = 0;
#endif
    }

    DebugPrint((3, "BuildVolumeList: returning with %d entries\n", *pdwNumEntries));
    return dwReturnValue;
}

DWORD
MapLoadedDisks (
    HANDLE  hDiskPerf,
    PDRIVE_VOLUME_ENTRY pList,
    LPDWORD             pdwNumEntries,
    LPDWORD             pdwMaxVolNo,
    LPDWORD             pdwWmiItemCount
)
/*
    This function maps the hard disk partitions to the corresponding 
    volume and drive letter found in the list of volume entries 
    passed in by the caller.

    This function can use a handle to WMI if the caller has one, or if 
    not, it will try to open it's own.

*/
{
    DWORD   status = ERROR_SUCCESS; // return value of the function
    HANDLE  hWmiDiskPerf = NULL;    // local handle value 
    DWORD   dwLocalMaxVolNo = 0;
    DWORD   dwLocalWmiItemCount = 0;

    // WMI Buffer variables
    DWORD   WmiBufSize;
    DWORD   WmiAllocSize = 0x8000;     
    LPBYTE  WmiBuffer = NULL;

    // WMI buffer processing variables
    PWNODE_ALL_DATA     WmiDiskInfo;
    DISK_PERFORMANCE    *pDiskPerformance;    //  Disk driver returns counters here
    DWORD               dwInstanceNameOffset;
    WORD                wNameLen;   // string length is first word in buffer
    LPWSTR              wszInstanceName; // pointer to string in WMI buffer
    
    WCHAR   wszInstName[MAX_PATH];
    DWORD   dwBytesToCopy;

    DWORD   dwListEntry;

    BOOL    bNotDone = TRUE;

    HANDLE  hProcessHeap = GetProcessHeap();

    if (hDiskPerf == NULL) {
        // open handle to disk perf device driver
        status = WmiOpenBlock (
            (GUID *)&DiskPerfGuid,
            GENERIC_READ,
            &hWmiDiskPerf);
    } else {
        // use caller's handle
        hWmiDiskPerf = hDiskPerf;
    }

    assert (pList != NULL);
    assert (pdwNumEntries != NULL);
    assert (pdwMaxVolNo != NULL);

    DebugPrint((3, "MapLoadedDisks with %d entries %d volumes",
        *pdwNumEntries, *pdwMaxVolNo));
    if (status == ERROR_SUCCESS) {
        // allocate a buffer to send to WMI to get the diskperf data
        WmiBufSize = WmiAllocSize;
        WmiBuffer = (LPBYTE)HeapAlloc (hProcessHeap, 
            HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
            WmiBufSize);

        if (WmiBuffer  == NULL) {
            status = ERROR_OUTOFMEMORY;
        } else {
#if DBG
            HeapUsed += WmiBufSize;
            DebugPrint((4,"\tWmiBuffer add %d to %d\n", WmiBufSize, HeapUsed));
#endif
            status = WmiQueryAllDataW (
                hWmiDiskPerf,
                &WmiBufSize,
                WmiBuffer);

            // if buffer size attempted is too big or too small, resize
            if ((WmiBufSize > 0) && (WmiBufSize != WmiAllocSize)) {
                WmiBuffer = (LPBYTE)HeapReAlloc (hProcessHeap,
                    HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
                    WmiBuffer, WmiBufSize);

                if (WmiBuffer == NULL) {
                    // reallocation failed so bail out
                    status = ERROR_OUTOFMEMORY;
                } else {
#if DBG
                    HeapUsed += (WmiBufSize - WmiAllocSize);
                    DebugPrint((4, "\tRealloc WmiBuffer old %d new %d to %d\n",
                        WmiAllocSize, WmiBufSize, HeapUsed));
#endif
                    WmiAllocSize = WmiBufSize;
                }
            }

            if (status == ERROR_INSUFFICIENT_BUFFER) {
                // if it didn't work because it was too small the first time
                // try one more time
                status = WmiQueryAllDataW (
                    hWmiDiskPerf,
                    &WmiBufSize,
                    WmiBuffer);
            
            } else {
                // it either worked the fisrt time or it failed because of 
                // something other than a buffer size problem
            }
        }

        if (status == ERROR_SUCCESS) {
            WmiDiskInfo = (PWNODE_ALL_DATA)WmiBuffer;
            // go through returned names and add to the buffer
            while (bNotDone) {
                pDiskPerformance = (PDISK_PERFORMANCE)(
                    (PUCHAR)WmiDiskInfo +  WmiDiskInfo->DataBlockOffset);
        
                dwInstanceNameOffset = WmiDiskInfo->DataBlockOffset + 
                                      ((sizeof(DISK_PERFORMANCE) + 1) & ~1) ;

                // get length of string (it's a counted string) length is in chars
                wNameLen = *(LPWORD)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset);

                if (wNameLen > 0) {
                    // just a sanity check here
                    assert (wNameLen < MAX_PATH);
                    // get pointer to string text
                    wszInstanceName = (LPWSTR)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset + sizeof(WORD));

                    // truncate to last characters if name is larger than the buffer in the table
                    if (wNameLen >= DVE_DEV_NAME_LEN) {
                        // copy the last DVE_DEV_NAME_LEN chars
                        wszInstanceName += (wNameLen  - DVE_DEV_NAME_LEN) + 1;
                        dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
                        wNameLen = DVE_DEV_NAME_LEN - 1;
                    } else {
                        dwBytesToCopy = wNameLen;
                    }
                    // copy it to the buffer to make it a SZ string
                    memcpy (wszInstName, &wszInstanceName[0], dwBytesToCopy);
                    // zero terminate it
                    wszInstName[wNameLen/sizeof(WCHAR)] = 0;

                    // find matching entry in list
                    // sent by caller and update
                    // the drive & partition info
                    for (dwListEntry = 0; 
                        dwListEntry < *pdwNumEntries;
                        dwListEntry++) {

                        DebugPrint((6,
                            "MapDrive: Comparing '%ws' to '%ws'\n",
                            wszInstName,
                            pList[dwListEntry].wszInstanceName));

                        if (lstrcmpW(wszInstName, pList[dwListEntry].wszInstanceName) == 0) {
                            // update entry and...
                            pList[dwListEntry].dwVolumeNumber = pDiskPerformance->StorageDeviceNumber;
                            memcpy (&pList[dwListEntry].szVolumeManager, 
                                pDiskPerformance->StorageManagerName,
                                sizeof(pDiskPerformance->StorageManagerName));
                            if (dwLocalMaxVolNo < pList[dwListEntry].dwVolumeNumber) {
                                dwLocalMaxVolNo = pList[dwListEntry].dwVolumeNumber;
                            }
                            DebugPrint ((2,
                                "MapDrive: Mapped %8.8s, %d to drive %c\n",
                                pList[dwListEntry].szVolumeManager,
                                pList[dwListEntry].dwVolumeNumber,
                                pList[dwListEntry].wcDriveLetter));

                            // break out of loop
                            dwListEntry = *pdwNumEntries; 
                        }
                    }
                    // count the number of entries
                    dwLocalWmiItemCount++;
                } else {
                    // no string to examine (length == 0)
                }

                // bump pointers inside WMI data block
                if (WmiDiskInfo->WnodeHeader.Linkage != 0) {
                    // continue
                    WmiDiskInfo = (PWNODE_ALL_DATA) (
                        (LPBYTE)WmiDiskInfo + WmiDiskInfo->WnodeHeader.Linkage);
                } else {
                    bNotDone = FALSE;
                }
            } // end while looking through the WMI data block
        }

        if (hDiskPerf == NULL) {
            // then the disk perf handle is local so close it
            status = WmiCloseBlock (hWmiDiskPerf);
        }

        *pdwMaxVolNo = dwLocalMaxVolNo;
        *pdwWmiItemCount = dwLocalWmiItemCount;
    }

    if (WmiBuffer != NULL) {
        HeapFree (RtlProcessHeap(), 0, WmiBuffer);
#if DBG
        HeapUsed -= WmiBufSize;
        DebugPrint((4, "\tFreed WmiBuffer %d to %d\n", WmiBufSize, HeapUsed));
#endif
    }

    DebugPrint((3, "MapLoadedDisks returning with %d entries %d volumes",
        *pdwNumEntries, *pdwMaxVolNo));
    return status;

}

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
)
/*
    This function will try to look up a disk device referenced by 
    it's Volume Manager Name and ID and return
    either the drive letter that corresponds to this disk as found in 
    the pList buffer or the generic name \HarddiskX\PartitionY if no 
    drive letter can be found.

    szDevicePath    IN: a partition or volume name in the format of
                            \Device\HarddiskX\PartitionY  or
                            \Device\VolumeX

    cchDevicePathSize   IN: length of the device Path in chars.

    pList           IN: pointer to an initialized list of drives, 
                            volumes and partitions

    dwNumEntries    IN: the number of drive letter entries in the pList buffer

    szNameBuffer    IN: pointer to buffer to receive the name of the
                            drive letter or name that corresponds to the
                            device specified by the szDevicePath buffer
                    OUT: pointer to buffer containing the name or drive 
                            letter of disk partition

    pcchNameBufferSize  IN: pointer to DWORD containing the size of the
                            szNameBuffer in characters
                    OUT: pointer to DWORD that contains the size of the
                        string returned in szNameBuffer

  The return value of this function can be one of the following values
    ERROR_SUCCESS   the function succeded and a string was returned in 
                    the buffer referenced by szNameBuffer

    
*/
{
    DWORD   dwReturnStatus = ERROR_SUCCESS;

    WCHAR   szLocalDevicePath[MAX_PATH];
    LPWSTR  szSrcPtr;
    DWORD   dwBytesToCopy;
    DWORD   dwThisEntry;
    DWORD   dwDestSize;

    LONGLONG    *pllVolMgrName;

    // validate the input arguments
    assert (szDevicePath != NULL);
    assert (*szDevicePath != 0);
    assert (cchDevicePathSize > 0);
    assert (cchDevicePathSize <= MAX_PATH);
    assert (pList != NULL);
    assert (dwNumEntries > 0);
    assert (szNameBuffer != NULL);
    assert (pcchNameBufferSize != NULL);
    assert (*pcchNameBufferSize > 0);

    pllVolMgrName = (LONGLONG *)szVolumeManagerName;

    if ((pllVolMgrName[0] == LL_LOGIDISK_0) && 
        (pllVolMgrName[1] == LL_LOGIDISK_1) &&
        (dwVolumeNumber == 0)) {
        // no short cut exists so look up by matching
        // the szDevicePath param to the wszInstanceName field

            assert (DVE_DEV_NAME_LEN < (sizeof(szLocalDevicePath)/sizeof(szLocalDevicePath[0])));
            szSrcPtr = (LPWSTR)szDevicePath;
            dwBytesToCopy = lstrlenW (szSrcPtr); // length is really in chars
            if (dwBytesToCopy >= DVE_DEV_NAME_LEN) {
                // copy the last DVE_DEV_NAME_LEN chars
                szSrcPtr += (dwBytesToCopy - DVE_DEV_NAME_LEN) + 1;
                dwBytesToCopy = (DVE_DEV_NAME_LEN - 1) * sizeof(WCHAR);
            } else {
                dwBytesToCopy *= sizeof(WCHAR);
            }
            // now dwBytesToCopy is in bytes
            memcpy (szLocalDevicePath, szSrcPtr, dwBytesToCopy);
            // null terminate
            assert ((dwBytesToCopy / sizeof(WCHAR)) < DVE_DEV_NAME_LEN);
            szLocalDevicePath[dwBytesToCopy / sizeof(WCHAR)] = 0;

        for (dwThisEntry = 0; dwThisEntry < dwNumEntries; dwThisEntry++) {
            if (lstrcmpW(szLocalDevicePath, pList[dwThisEntry].wszInstanceName) == 0) {
                break;
            }
        }
        // continue to assign letter
    } else {
        // use the faster look up

        for (dwThisEntry = 0; dwThisEntry < dwNumEntries; dwThisEntry++) {
            if (((pList[dwThisEntry].llVolMgr[0] == pllVolMgrName[0]) &&
                 (pList[dwThisEntry].llVolMgr[1] == pllVolMgrName[1])) &&
                 (pList[dwThisEntry].dwVolumeNumber == dwVolumeNumber)) {
                break;
            }
        }
    }

    if (dwThisEntry < dwNumEntries) {
        // then a matching entry was found so copy the drive letter
        //then this is the matching entry
        if (*pcchNameBufferSize > 3) {
            szNameBuffer[0] = pList[dwThisEntry].wcDriveLetter;
            szNameBuffer[1] = L':';
            szNameBuffer[2] = 0;
            if (phVolume != NULL) {
                *phVolume = pList[dwThisEntry].hVolume;
            }
        } else {
            dwReturnStatus = ERROR_INSUFFICIENT_BUFFER;
        }
        *pcchNameBufferSize = 3;
    } else {
        // then this is a valid path, but doesn't match 
        // any assigned drive letters, so remove "\device\"
        // and copy the remainder of the string
        dwDestSize = cchDevicePathSize;
        dwDestSize -= SIZE_OF_DEVICE;   // subtract front of string not copied
        if (dwDestSize < *pcchNameBufferSize) {
            memcpy (szNameBuffer, &szDevicePath[SIZE_OF_DEVICE],
                (dwDestSize * sizeof (WCHAR)));
            szNameBuffer[dwDestSize] = 0;
        } else {
            dwReturnStatus = ERROR_INSUFFICIENT_BUFFER;
        }
        *pcchNameBufferSize = dwDestSize + 1;
    }
    DebugPrint((4, "GetDriveNameString: NameBufSize %d Entries %d\n",
        *pcchNameBufferSize, dwNumEntries));
    return dwReturnStatus;
}

DWORD
MakePhysDiskInstanceNames (
    PDRIVE_VOLUME_ENTRY pPhysDiskList,
    DWORD               dwNumPhysDiskListItems,
    LPDWORD             pdwMaxDriveNo,
    PDRIVE_VOLUME_ENTRY pVolumeList,
    DWORD               dwNumVolumeListItems
)
{
    DWORD   dwPDItem;
    DWORD   dwVLItem;
    WCHAR   szLocalInstanceName[MAX_PATH];
    WCHAR   *pszNextChar;
    DWORD   dwMaxDriveNo = 0;

    // for each HD in the PhysDisk List, 
    // find matching Volumes in the Volume list

    DebugPrint((3, "MakePhysDiskInstanceNames: maxdriveno %d\n",
        *pdwMaxDriveNo));
    for (dwPDItem = 0; dwPDItem < dwNumPhysDiskListItems; dwPDItem++) {
        if (pPhysDiskList[dwPDItem].wPartNo != 0) {
            //only do partitions that might have logical volumes first
            // initialize the instance name for this HD
            for (dwVLItem = 0; dwVLItem < dwNumVolumeListItems; dwVLItem++) {

                DebugPrint((6,
                    "Phys Disk -- Comparing '%ws' to '%ws'\n",
                pPhysDiskList[dwPDItem].wszInstanceName,
                pVolumeList[dwVLItem].wszInstanceName));

                if (lstrcmpiW(pPhysDiskList[dwPDItem].wszInstanceName, 
                    pVolumeList[dwVLItem].wszInstanceName) == 0) {

                   DebugPrint ((4,
                       "Phys Disk: Drive/Part %d/%d (%s) is Logical Drive %c\n",
                       pPhysDiskList[dwPDItem].wDriveNo, 
                       pPhysDiskList[dwPDItem].wPartNo,
                       pPhysDiskList[dwPDItem].wszInstanceName,
                       pVolumeList[dwVLItem].wcDriveLetter));

                    // then this partition matches so copy the volume information
                    pPhysDiskList[dwPDItem].wcDriveLetter = 
                        pVolumeList[dwVLItem].wcDriveLetter;
                    pPhysDiskList[dwPDItem].llVolMgr[0] =
                        pVolumeList[dwVLItem].llVolMgr[0];
                    pPhysDiskList[dwPDItem].llVolMgr[1] =
                        pVolumeList[dwVLItem].llVolMgr[1];
                    pPhysDiskList[dwPDItem].dwVolumeNumber =
                        pVolumeList[dwVLItem].dwVolumeNumber;
                    // there should only one match so bail out and go to the next item
                    break;
                }
            }
        }
    }

    // all the partitions with volumes now have drive letters so build the physical 
    // drive instance strings

    for (dwPDItem = 0; dwPDItem < dwNumPhysDiskListItems; dwPDItem++) {
        if (pPhysDiskList[dwPDItem].wPartNo == 0) {
            // only do the physical partitions
            // save the \Device\HarddiskVolume path here
            lstrcpyW (szLocalInstanceName, pPhysDiskList[dwPDItem].wszInstanceName);
            // initialize the instance name for this HD
            memset(&pPhysDiskList[dwPDItem].wszInstanceName[0], 0, (DVE_DEV_NAME_LEN * sizeof(WCHAR)));
            _ltow ((LONG)pPhysDiskList[dwPDItem].wDriveNo, pPhysDiskList[dwPDItem].wszInstanceName, 10);
            pPhysDiskList[dwPDItem].wReserved = (WORD)(lstrlenW (pPhysDiskList[dwPDItem].wszInstanceName));
            // search the entries that are logical partitions of this drive
            for (dwVLItem = 0; dwVLItem < dwNumPhysDiskListItems; dwVLItem++) {
                if (pPhysDiskList[dwVLItem].wPartNo != 0) {

                        DebugPrint ((6, "Phys Disk: Comparing %d/%d (%s) to %d/%d\n",
                            pPhysDiskList[dwPDItem].wDriveNo,
                            pPhysDiskList[dwPDItem].wPartNo,
                            szLocalInstanceName,
                            pPhysDiskList[dwVLItem].wDriveNo,
                            pPhysDiskList[dwVLItem].wPartNo));

                    if ((pPhysDiskList[dwVLItem].wDriveNo == pPhysDiskList[dwPDItem].wDriveNo) &&
                        (pPhysDiskList[dwVLItem].wcDriveLetter >= L'A')) {  // only allow letters to be added
                        // then this logical drive is on the physical disk
                        pszNextChar = &pPhysDiskList[dwPDItem].wszInstanceName[0];
                        pszNextChar += pPhysDiskList[dwPDItem].wReserved;
                        *pszNextChar++ = L' ';
                        *pszNextChar++ = (WCHAR)(pPhysDiskList[dwVLItem].wcDriveLetter); 
                        *pszNextChar++ = L':';
                        *pszNextChar = L'\0';
                        pPhysDiskList[dwPDItem].wReserved += 3;

                        DebugPrint ((4, " -- Drive %c added.\n",
                            pPhysDiskList[dwVLItem].wcDriveLetter));

                        if ((DWORD)pPhysDiskList[dwPDItem].wDriveNo > dwMaxDriveNo) {
                            dwMaxDriveNo = (DWORD)pPhysDiskList[dwPDItem].wDriveNo;

                            DebugPrint((2,
                                "Phys Disk: Drive count now = %d\n",
                                dwMaxDriveNo));

                        }
                    }
                }
            }

            DebugPrint((2,
                "Mapped Phys Disk: '%ws'\n",
                pPhysDiskList[dwPDItem].wszInstanceName));

        } // else not a physical partition
    } //end of loop    

    // return max drive number
    *pdwMaxDriveNo = dwMaxDriveNo;

    DebugPrint((3, "MakePhysDiskInstanceNames: return maxdriveno %d\n",
        *pdwMaxDriveNo));
    return ERROR_SUCCESS;
}

DWORD
CompressPhysDiskTable (
    PDRIVE_VOLUME_ENTRY     pOrigTable,
    DWORD                   dwOrigCount,
    PDRIVE_VOLUME_ENTRY     pNewTable,
    DWORD                   dwNewCount
)
{
    DWORD   dwPDItem;
    DWORD   dwVLItem;
    DWORD   dwDriveId;

    for (dwPDItem = 0; dwPDItem < dwNewCount; dwPDItem++) {
        // for each drive entry in the new table find the matching 
        // harddisk entry in the original table
        dwDriveId = (WORD)dwPDItem;
        dwDriveId <<= 16;
        dwDriveId &= 0xFFFF0000;

        for (dwVLItem = 0; dwVLItem < dwOrigCount; dwVLItem++) {
            if (pOrigTable[dwVLItem].dwDriveId == dwDriveId) {

               DebugPrint((2,
                   "CompressPhysDiskTable:Phys Disk: phys drive %d is mapped as %s\n",
                   dwPDItem, pOrigTable[dwVLItem].wszInstanceName));

                // copy this entry
                memcpy (&pNewTable[dwPDItem], &pOrigTable[dwVLItem],
                    sizeof(DRIVE_VOLUME_ENTRY));
                break;
            }
        }
    }

    return ERROR_SUCCESS;
}


BOOL
GetPhysicalDriveNameString (
    DWORD                   dwDriveNumber,    
    PDRIVE_VOLUME_ENTRY     pList,
    DWORD                   dwNumEntries,
    LPWSTR                  szNameBuffer
)
{
    LPWSTR  szNewString = NULL;

    // see if the indexed entry matches
    if (dwNumEntries > 0) {
        if ((dwDriveNumber < dwNumEntries) && (!bUseNT4InstanceNames)) {
            if ((DWORD)(pList[dwDriveNumber].wDriveNo) == dwDriveNumber) {
                // this matches so we'll get the address of the instance string
                szNewString = &pList[dwDriveNumber].wszInstanceName[0];
            } else {
                // this drive number doesn't match the one in the table
            }
        } else {
            // this is an unknown drive no or we don't want to use
            // the fancy ones
        }
    } else {
        // no entries to look up
    }

    if (szNewString == NULL) {
        // then we have to make one 
        _ltow ((LONG)dwDriveNumber, szNameBuffer, 10);
    } else {
        lstrcpyW (szNameBuffer, szNewString);
    }

    return TRUE;
}
