/*
 * DDK information for DVD
 *
 * Copyright (C) 2004 Uwe Bonnes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __NTDDCDVD_H
#define __NTDDCDVD_H

/* definitions taken from libdvdcss, modified to reflect Windows names and data types in places */

#define IOCTL_DVD_BASE                 FILE_DEVICE_DVD

#define IOCTL_DVD_START_SESSION     CTL_CODE(IOCTL_DVD_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_READ_KEY          CTL_CODE(IOCTL_DVD_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY          CTL_CODE(IOCTL_DVD_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_END_SESSION       CTL_CODE(IOCTL_DVD_BASE, 0x0403, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SET_READ_AHEAD    CTL_CODE(IOCTL_DVD_BASE, 0x0404, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_GET_REGION        CTL_CODE(IOCTL_DVD_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DVD_SEND_KEY2         CTL_CODE(IOCTL_DVD_BASE, 0x0406, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DVD_READ_STRUCTURE    CTL_CODE(IOCTL_DVD_BASE, 0x0450, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef enum {
    DvdChallengeKey = 0x01,
    DvdBusKey1,
    DvdBusKey2,
    DvdTitleKey,
    DvdAsf,
    DvdSetRpcKey = 0x6,
    DvdGetRpcKey = 0x8,
    DvdDiskKey = 0x80,
    DvdInvalidateAGID = 0x3f
} DVD_KEY_TYPE;

typedef ULONG DVD_SESSION_ID, *PDVD_SESSION_ID;

#include <pshpack1.h>

typedef struct _DVD_COPY_PROTECT_KEY {
    ULONG KeyLength;
    DVD_SESSION_ID SessionId;
    DVD_KEY_TYPE KeyType;
    ULONG KeyFlags;
    union {
        struct {
            ULONG FileHandle;
            ULONG Reserved;   /* used for NT alignment */
        } s;
        LARGE_INTEGER TitleOffset;
    } Parameters;
    UCHAR KeyData[1];
} DVD_COPY_PROTECT_KEY, *PDVD_COPY_PROTECT_KEY;

typedef struct _DVD_RPC_KEY {
    UCHAR UserResetsAvailable:3;
    UCHAR ManufacturerResetsAvailable:3;
    UCHAR TypeCode:2;
    UCHAR RegionMask;
    UCHAR RpcScheme;
    UCHAR Reserved2[1];
} DVD_RPC_KEY, * PDVD_RPC_KEY;

typedef struct _DVD_ASF {
    UCHAR Reserved0[3];
    UCHAR SuccessFlag:1;
    UCHAR Reserved1:7;
} DVD_ASF, * PDVD_ASF;

typedef struct _DVD_REGION
{
        UCHAR CopySystem;
        UCHAR RegionData;              /* current media region (not playable when set) */
        UCHAR SystemRegion;            /* current drive region (playable when set) */
        UCHAR ResetCount;              /* number of resets available */
} DVD_REGION, * PDVD_REGION;

typedef enum _DVD_STRUCTURE_FORMAT
{
	DvdPhysicalDescriptor,
	DvdCopyrightDescriptor,
	DvdDiskKeyDescriptor,
	DvdBCADescriptor,
	DvdManufacturerDescriptor,
	DvdMaxDescriptor
} DVD_STRUCTURE_FORMAT, *PDVD_STRUCTURE_FORMAT;

typedef struct DVD_READ_STRUCTURE {
        /* Contains an offset to the logical block address of the descriptor to be retrieved. */
        LARGE_INTEGER BlockByteOffset;

        /* 0:Physical descriptor, 1:Copyright descriptor, 2:Disk key descriptor
           3:BCA descriptor, 4:Manufacturer descriptor, 5:Max descriptor
         */
        DVD_STRUCTURE_FORMAT Format;

        /* Session ID, that is obtained by IOCTL_DVD_START_SESSION */
        DVD_SESSION_ID SessionId;

        /* From 0 to 4 */
        UCHAR LayerNumber;
} DVD_READ_STRUCTURE, *PDVD_READ_STRUCTURE;

typedef struct _DVD_DESCRIPTOR_HEADER {
    USHORT Length;
    UCHAR Reserved[2];
} DVD_DESCRIPTOR_HEADER, *PDVD_DESCRIPTOR_HEADER;
C_ASSERT(sizeof(DVD_DESCRIPTOR_HEADER) == 4);

typedef struct _DVD_LAYER_DESCRIPTOR
{
    UCHAR BookVersion : 4;

    /* 0:DVD-ROM, 1:DVD-RAM, 2:DVD-R, 3:DVD-RW, 9:DVD-RW */
    UCHAR BookType : 4;

    UCHAR MinimumRate : 4;

    /* The physical size of the media. 0:120 mm, 1:80 mm. */
    UCHAR DiskSize : 4;

    /* 1:Read-only layer, 2:Recordable layer, 4:Rewritable layer */
    UCHAR LayerType : 4;

    /* 0:parallel track path, 1:opposite track path */
    UCHAR TrackPath : 1;

    /* 0:one layers, 1:two layers, and so on */
    UCHAR NumberOfLayers : 2;

    UCHAR Reserved1 : 1;

    /* 0:0.74 µm/track, 1:0.80 µm/track, 2:0.615 µm/track */
    UCHAR TrackDensity : 4;

    /* 0:0.267 µm/bit, 1:0.293 µm/bit, 2:0.409 to 0.435 µm/bit, 4:0.280 to 0.291 µm/bit, 8:0.353 µm/bit */
    UCHAR LinearDensity : 4;

    /* Must be either 0x30000:DVD-ROM or DVD-R/-RW or 0x31000:DVD-RAM or DVD+RW */
    ULONG StartingDataSector;

    ULONG EndDataSector;
    ULONG EndLayerZeroSector;
    UCHAR Reserved5 : 7;

    /* 0 indicates no BCA data */
    UCHAR BCAFlag : 1;
}DVD_LAYER_DESCRIPTOR, * PDVD_LAYER_DESCRIPTOR;
C_ASSERT(sizeof(DVD_LAYER_DESCRIPTOR) == 17);

typedef struct _DVD_COPYRIGHT_DESCRIPTOR
{
    UCHAR CopyrightProtectionType;
    UCHAR RegionManagementInformation;
    USHORT Reserved;
}DVD_COPYRIGHT_DESCRIPTOR, * PDVD_COPYRIGHT_DESCRIPTOR;

typedef struct _DVD_DISK_KEY_DESCRIPTOR
{
    UCHAR DiskKeyData[2048];
}DVD_DISK_KEY_DESCRIPTOR, * PDVD_DISK_KEY_DESCRIPTOR;

typedef struct _DVD_BCA_DESCRIPTOR
{
    UCHAR BCAInformation[1];
}DVD_BCA_DESCRIPTOR, * PDVD_BCA_DESCRIPTOR;

typedef struct _DVD_MANUFACTURER_DESCRIPTOR
{
        UCHAR ManufacturingInformation[2048];
}DVD_MANUFACTURER_DESCRIPTOR, * PDVD_MANUFACTURER_DESCRIPTOR;

#define DVD_CHALLENGE_KEY_LENGTH    (12 + sizeof(DVD_COPY_PROTECT_KEY) - sizeof(UCHAR))

#define DVD_DISK_KEY_LENGTH         (2048 + sizeof(DVD_COPY_PROTECT_KEY) - sizeof(UCHAR))

#define DVD_KEY_SIZE 5
#define DVD_CHALLENGE_SIZE 10
#define DVD_DISCKEY_SIZE 2048
#define DVD_SECTOR_PROTECTED            0x00000020

#include <poppack.h>

#endif /* __NTDDCDVD_H */
