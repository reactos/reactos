////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*

Module Name:

    udfpubl.h

Abstract:

    This file defines the interface between UDFFS driver & user
    applications | drivers.

Environment:

    NT kernel mode or Win32 app
 */

#ifndef __UDF_PUBL_H__
#define __UDF_PUBL_H__

#pragma pack(push, 8)

#ifndef CTL_CODE
#include "winioctl.h"
#endif

#ifndef IOCTL_UDFFS_BASE
#define IOCTL_UDFFS_BASE        0x00000911
#endif

#ifndef IOCTL_UDF_DISABLE_DRIVER
#define IOCTL_UDF_DISABLE_DRIVER                CTL_CODE(IOCTL_UDFFS_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_ENABLE_DRIVER                 CTL_CODE(IOCTL_UDFFS_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_INVALIDATE_VOLUMES            CTL_CODE(IOCTL_UDFFS_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_GET_RETRIEVAL_POINTERS        CTL_CODE(IOCTL_UDFFS_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_GET_FILE_ALLOCATION_MODE      CTL_CODE(IOCTL_UDFFS_BASE, 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_SET_FILE_ALLOCATION_MODE      CTL_CODE(IOCTL_UDFFS_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_LOCK_VOLUME_BY_PID            CTL_CODE(IOCTL_UDFFS_BASE, 0x0007, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_UNLOCK_VOLUME_BY_PID          CTL_CODE(IOCTL_UDFFS_BASE, 0x0008, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_SEND_LICENSE_KEY              CTL_CODE(IOCTL_UDFFS_BASE, 0x0009, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_GET_SPEC_RETRIEVAL_POINTERS   CTL_CODE(IOCTL_UDFFS_BASE, 0x000a, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_UDF_GET_VERSION                   CTL_CODE(IOCTL_UDFFS_BASE, 0x000b, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UDF_SET_NOTIFICATION_EVENT        CTL_CODE(IOCTL_UDFFS_BASE, 0x000c, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UDF_IS_VOLUME_JUST_MOUNTED        CTL_CODE(IOCTL_UDFFS_BASE, 0x000d, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UDF_REGISTER_AUTOFORMAT           CTL_CODE(IOCTL_UDFFS_BASE, 0x000e, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UDF_SET_OPTIONS                   CTL_CODE(IOCTL_UDFFS_BASE, 0x000f, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _UDF_GET_FILE_ALLOCATION_MODE_OUT {

    CHAR  AllocMode; // see definition of ICB_FLAG_AD_XXX in Ecma_167.h

} UDF_GET_FILE_ALLOCATION_MODE_OUT, *PUDF_GET_FILE_ALLOCATION_MODE_OUT;

// setting default AllocMode
//#define ICB_FLAG_AD_DEFAULT_ALLOC_MODE     (UCHAR)(0xff)

typedef UDF_GET_FILE_ALLOCATION_MODE_OUT  UDF_SET_FILE_ALLOCATION_MODE_IN;
typedef PUDF_GET_FILE_ALLOCATION_MODE_OUT PUDF_SET_FILE_ALLOCATION_MODE_IN;

/*typedef struct _RETRIEVAL_POINTERS_BUFFER {
   int a;
} RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;
*/
typedef struct _UDF_LOCK_VOLUME_BY_PID_IN {
    ULONG  PID; // -1 for current process
} UDF_LOCK_VOLUME_BY_PID_IN, *PUDF_LOCK_VOLUME_BY_PID_IN;

typedef UDF_LOCK_VOLUME_BY_PID_IN  UDF_UNLOCK_VOLUME_BY_PID_IN;
typedef PUDF_LOCK_VOLUME_BY_PID_IN PUDF_UNLOCK_VOLUME_BY_PID_IN;

#define UDF_DOS_FS_NAME L"\\DosDevices\\DwUdf"
#define UDF_WIN_FS_NAME "\\\\.\\DwUdf"

#define UDF_ISO_STREAM_NAME   "UdfIsoBridgeStructure"
#define UDF_ISO_STREAM_NAME_W L"UdfIsoBridgeStructure"

#define UDF_CONFIG_STREAM_NAME   "DvdWriteNow.cfg"
#define UDF_CONFIG_STREAM_NAME_W L"DvdWriteNow.cfg"

#ifdef FSCTL_GET_RETRIEVAL_POINTERS

typedef struct _UDF_GET_SPEC_RETRIEVAL_POINTERS_IN {
    STARTING_VCN_INPUT_BUFFER Standard;
    ULONG                     Special;
} UDF_GET_SPEC_RETRIEVAL_POINTERS_IN, *PUDF_GET_SPEC_RETRIEVAL_POINTERS_IN;

#endif //FSCTL_GET_RETRIEVAL_POINTERS

typedef struct _UDF_GET_VERSION_OUT {
    struct {
        ULONG                     Length;
        ULONG                     DriverVersionMj;
        ULONG                     DriverVersionMn;
        ULONG                     DriverVersionBuild;
    } header;
    ULONG                     FSVersionMj;
    ULONG                     FSVersionMn;
    ULONG                     FSFlags;
    ULONG                     FSCompatFlags;
    ULONG                     FSCfgVersion;
} UDF_GET_VERSION_OUT, *PUDF_GET_VERSION_OUT;

#define UDF_USER_FS_FLAGS_RO             0x0001
#define UDF_USER_FS_FLAGS_RAW            0x0002
#define UDF_USER_FS_FLAGS_OUR_DRIVER     0x0004
#define UDF_USER_FS_FLAGS_FP             0x0008
#define UDF_USER_FS_FLAGS_MEDIA_RO       0x0010
#define UDF_USER_FS_FLAGS_SOFT_RO        0x0020
#define UDF_USER_FS_FLAGS_HW_RO          0x0040
#define UDF_USER_FS_FLAGS_MEDIA_DEFECT_RO 0x0080
#define UDF_USER_FS_FLAGS_PART_RO        0x0100     // partition is r/o
#define UDF_USER_FS_FLAGS_NEW_FS_RO      0x0200

#endif  //IOCTL_UDF_DISABLE_DRIVER

#define         UDF_PART_DAMAGED_RW                 (0x00)
#define         UDF_PART_DAMAGED_RO                 (0x01)
#define         UDF_PART_DAMAGED_NO                 (0x02)

typedef struct _UDF_SET_OPTIONS_IN {
    struct {
        ULONG       HdrLength;
        ULONG       Flags;
    } header;
    UCHAR       Data[1];
} UDF_SET_OPTIONS_IN, *PUDF_SET_OPTIONS_IN;

#define  UDF_SET_OPTIONS_FLAG_TEMPORARY    0x00
#define  UDF_SET_OPTIONS_FLAG_DISK         0x01
#define  UDF_SET_OPTIONS_FLAG_DRIVE        0x02
#define  UDF_SET_OPTIONS_FLAG_GLOBAL       0x03
#define  UDF_SET_OPTIONS_FLAG_MASK         0x03

#pragma pack(pop)

#endif //__UDF_PUBL_H__
