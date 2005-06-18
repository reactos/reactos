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

/* EXPORTED DATA *************************************************************/
extern NTOSAPI POBJECT_TYPE ObDirectoryType;
extern NTOSAPI struct DEVICE_MAP* ObSystemDeviceMap;

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

#endif
