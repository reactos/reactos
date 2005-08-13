/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/obtypes.h
 * PURPOSE:         Defintions for Object Manager Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _OBTYPES_H
#define _OBTYPES_H

/* DEPENDENCIES **************************************************************/

/* CONSTANTS *****************************************************************/

/* Values for DosDeviceDriveType */
#define DOSDEVICE_DRIVE_UNKNOWN		0
#define DOSDEVICE_DRIVE_CALCULATE	1
#define DOSDEVICE_DRIVE_REMOVABLE	2
#define DOSDEVICE_DRIVE_FIXED		3
#define DOSDEVICE_DRIVE_REMOTE		4
#define DOSDEVICE_DRIVE_CDROM		5
#define DOSDEVICE_DRIVE_RAMDISK		6

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _OBJECT_BASIC_INFORMATION 
{
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG Reserved[3];
    ULONG NameInformationLength;
    ULONG TypeInformationLength;
    ULONG SecurityDescriptorLength;
    LARGE_INTEGER CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

/* FIXME: Add Object Structures Here */
/*
 * FIXME: These will eventually become centerfold in the compliant Ob Manager
 * For now, they are only here so Device Map is properly defined before the header
 * changes
 */
typedef struct _OBJECT_DIRECTORY_ENTRY
{
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
    ULONG HashValue;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

#define NUMBER_HASH_BUCKETS 37
typedef struct _OBJECT_DIRECTORY
{
    struct _OBJECT_DIRECTORY_ENTRY *HashBuckets[NUMBER_HASH_BUCKETS];
    struct _EX_PUSH_LOCK *Lock;
    struct _DEVICE_MAP *DeviceMap;
    ULONG SessionId;
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

typedef struct _DEVICE_MAP
{
    POBJECT_DIRECTORY   DosDevicesDirectory;
    POBJECT_DIRECTORY   GlobalDosDevicesDirectory;
    ULONG               ReferenceCount;
    ULONG               DriveMap;
    UCHAR               DriveType[32];
} DEVICE_MAP, *PDEVICE_MAP; 

/* EXPORTED DATA *************************************************************/

extern NTOSAPI POBJECT_TYPE ObDirectoryType;
extern NTOSAPI PDEVICE_MAP ObSystemDeviceMap;

#endif
