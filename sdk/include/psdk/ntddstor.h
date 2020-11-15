/*
 * ntddstor.h
 *
 * Storage class IOCTL interface.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _NTDDSTOR_H_
#define _NTDDSTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DEFINE_GUID)

DEFINE_GUID(GUID_DEVINTERFACE_DISK,
  0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_CDROM,
  0x53f56308L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_PARTITION,
  0x53f5630aL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_TAPE,
  0x53f5630bL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_WRITEONCEDISK,
  0x53f5630cL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_VOLUME,
  0x53f5630dL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_MEDIUMCHANGER,
  0x53f56310L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_FLOPPY,
  0x53f56311L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_CDCHANGER,
  0x53f56312L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_STORAGEPORT,
  0x2accfe60L, 0xc130, 0x11d2, 0xb0, 0x82, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

DEFINE_GUID(GUID_DEVINTERFACE_HIDDEN_VOLUME,
  0x7f108a28L, 0x9833, 0x4b3b, 0xb7, 0x80, 0x2c, 0x6b, 0x5f, 0xa5, 0xc0, 0x62);

#define WDI_STORAGE_PREDICT_FAILURE_DPS_GUID \
  {0xe9f2d03aL, 0x747c, 0x41c2, {0xbb, 0x9a, 0x02, 0xc6, 0x2b, 0x6d, 0x5f, 0xcb}};

/* Aliases for storage guids */
#define DiskClassGuid               GUID_DEVINTERFACE_DISK
#define CdRomClassGuid              GUID_DEVINTERFACE_CDROM
#define PartitionClassGuid          GUID_DEVINTERFACE_PARTITION
#define TapeClassGuid               GUID_DEVINTERFACE_TAPE
#define WriteOnceDiskClassGuid      GUID_DEVINTERFACE_WRITEONCEDISK
#define VolumeClassGuid             GUID_DEVINTERFACE_VOLUME
#define MediumChangerClassGuid      GUID_DEVINTERFACE_MEDIUMCHANGER
#define FloppyClassGuid             GUID_DEVINTERFACE_FLOPPY
#define CdChangerClassGuid          GUID_DEVINTERFACE_CDCHANGER
#define StoragePortClassGuid        GUID_DEVINTERFACE_STORAGEPORT
#define HiddenVolumeClassGuid       GUID_DEVINTERFACE_HIDDEN_VOLUME

#endif /* defined(DEFINE_GUID) */

#if defined(DEFINE_DEVPROPKEY)
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Portable,           0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 2);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Removable_Media,    0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 3);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_System_Critical,    0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 4);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Disk_Number,        0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 5);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Partition_Number,   0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 6);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Mbr_Type,           0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 7);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Gpt_Type,           0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 8);
DEFINE_DEVPROPKEY(DEVPKEY_Storage_Gpt_Name,           0x4d1ebee8, 0x803, 0x4774, 0x98, 0x42, 0xb7, 0x7d, 0xb5, 0x2, 0x65, 0xe9, 9);
#endif

#ifndef _WINIOCTL_

#define IOCTL_STORAGE_BASE                FILE_DEVICE_MASS_STORAGE

#define IOCTL_STORAGE_CHECK_VERIFY \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_CHECK_VERIFY2 \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_MEDIA_REMOVAL \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_EJECT_MEDIA \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_LOAD_MEDIA \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_LOAD_MEDIA2 \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_RESERVE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_RELEASE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_FIND_NEW_DEVICES \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_EJECTION_CONTROL \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0250, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_MCN_CONTROL \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0251, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_MEDIA_TYPES \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_MEDIA_TYPES_EX \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_RESET_BUS \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_RESET_DEVICE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_GET_DEVICE_NUMBER \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_PREDICT_FAILURE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0440, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif /* _WINIOCTL_ */

#define IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0304, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_HOTPLUG_INFO \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0305, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_SET_HOTPLUG_INFO \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0306, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define OBSOLETE_IOCTL_STORAGE_RESET_BUS \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define OBSOLETE_IOCTL_STORAGE_RESET_DEVICE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_BREAK_RESERVATION \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_PERSISTENT_RESERVE_IN \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0406, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_PERSISTENT_RESERVE_OUT \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0407, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_READ_CAPACITY \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0450, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_QUERY_PROPERTY \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0501, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_GET_LB_PROVISIONING_MAP_RESOURCES \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0502, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_GET_BC_PROPERTIES \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0600, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_STORAGE_ALLOCATE_BC_STREAM \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0601, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_FREE_BC_STREAM \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0602, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_CHECK_PRIORITY_HINT_SUPPORT \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0620, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_START_DATA_INTEGRITY_CHECK \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0621, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_STOP_DATA_INTEGRITY_CHECK  \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0622, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_FIRMWARE_GET_INFO \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0700, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_FIRMWARE_DOWNLOAD \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0701, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_FIRMWARE_ACTIVATE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0702, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_ENABLE_IDLE_POWER \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0720, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_IDLE_POWERUP_REASON \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0721, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_POWER_ACTIVE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0722, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_POWER_IDLE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0723, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_EVENT_NOTIFICATION \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0724, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_DEVICE_POWER_CAP \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0725, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_RPMB_COMMAND \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0726, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_ATTRIBUTE_MANAGEMENT \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0727, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_STORAGE_DIAGNOSTIC \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0728, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_PHYSICAL_ELEMENT_STATUS \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0729, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_REMOVE_ELEMENT_AND_TRUNCATE \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0730, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_GET_DEVICE_INTERNAL_LOG \
  CTL_CODE(IOCTL_STORAGE_BASE, 0x0731, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define RECOVERED_WRITES_VALID         0x00000001
#define UNRECOVERED_WRITES_VALID       0x00000002
#define RECOVERED_READS_VALID          0x00000004
#define UNRECOVERED_READS_VALID        0x00000008
#define WRITE_COMPRESSION_INFO_VALID   0x00000010
#define READ_COMPRESSION_INFO_VALID    0x00000020

#define TAPE_RETURN_STATISTICS         0L
#define TAPE_RETURN_ENV_INFO           1L
#define TAPE_RESET_STATISTICS          2L

/* DEVICE_MEDIA_INFO.DeviceSpecific.DiskInfo.MediaCharacteristics constants */
#define MEDIA_ERASEABLE                   0x00000001
#define MEDIA_WRITE_ONCE                  0x00000002
#define MEDIA_READ_ONLY                   0x00000004
#define MEDIA_READ_WRITE                  0x00000008
#define MEDIA_WRITE_PROTECTED             0x00000100
#define MEDIA_CURRENTLY_MOUNTED           0x80000000

#define StorageIdTypeNAA StorageIdTypeFCPHName

#define DeviceDsmActionFlag_NonDestructive  0x80000000

#define IsDsmActionNonDestructive(_Action) ((BOOLEAN)((_Action & DeviceDsmActionFlag_NonDestructive) != 0))

#define DeviceDsmAction_None                    0x0u
#define DeviceDsmAction_Trim                    0x1u
#define DeviceDsmAction_Notification            (0x00000002u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_OffloadRead             (0x00000003u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_OffloadWrite            (0x00000004u)
#define DeviceDsmAction_Allocation              (0x00000005u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_Repair                  (0x00000006u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_Scrub                   (0x00000007u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_DrtQuery                (0x00000008u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_DrtClear                (0x00000009u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_DrtDisable              (0x0000000Au | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_TieringQuery            (0x0000000Bu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_Map                     (0x0000000Cu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_RegenerateParity        (0x0000000Du | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_NvCache_Change_Priority (0x0000000Eu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_NvCache_Evict           (0x0000000Fu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_TopologyIdQuery         (0x00000010u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_GetPhysicalAddresses    (0x00000011u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_ScopeRegen              (0x00000012u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_ReportZones             (0x00000013u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_OpenZone                (0x00000014u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_FinishZone              (0x00000015u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_CloseZone               (0x00000016u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_ResetWritePointer       (0x00000017u)
#define DeviceDsmAction_GetRangeErrorInfo       (0x00000018u | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_WriteZeroes             (0x00000019u)
#define DeviceDsmAction_LostQuery               (0x0000001Au | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_GetFreeSpace            (0x0000001Bu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_ConversionQuery         (0x0000001Cu | DeviceDsmActionFlag_NonDestructive)
#define DeviceDsmAction_VdtSet                  (0x0000001Du)

#define DEVICE_DSM_FLAG_ENTIRE_DATA_SET_RANGE    0x00000001

#define DEVICE_DSM_NOTIFY_FLAG_BEGIN             0x00000001
#define DEVICE_DSM_NOTIFY_FLAG_END               0x00000002

#define IOCTL_STORAGE_BC_VERSION                 1

#define STORAGE_PRIORITY_HINT_SUPPORTED          0x0001

typedef struct _STORAGE_HOTPLUG_INFO {
  ULONG Size;
  BOOLEAN MediaRemovable;
  BOOLEAN MediaHotplug;
  BOOLEAN DeviceHotplug;
  BOOLEAN WriteCacheEnableOverride;
} STORAGE_HOTPLUG_INFO, *PSTORAGE_HOTPLUG_INFO;

typedef struct _STORAGE_DEVICE_NUMBER {
  DEVICE_TYPE DeviceType;
  ULONG DeviceNumber;
  ULONG PartitionNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;

typedef struct _STORAGE_BUS_RESET_REQUEST {
  UCHAR PathId;
} STORAGE_BUS_RESET_REQUEST, *PSTORAGE_BUS_RESET_REQUEST;

typedef struct _STORAGE_BREAK_RESERVATION_REQUEST {
  ULONG Length;
  UCHAR _unused;
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
} STORAGE_BREAK_RESERVATION_REQUEST, *PSTORAGE_BREAK_RESERVATION_REQUEST;

#ifndef _WINIOCTL_
typedef struct _PREVENT_MEDIA_REMOVAL {
  BOOLEAN PreventMediaRemoval;
} PREVENT_MEDIA_REMOVAL, *PPREVENT_MEDIA_REMOVAL;
#endif

typedef struct _CLASS_MEDIA_CHANGE_CONTEXT {
  ULONG MediaChangeCount;
  ULONG NewState;
} CLASS_MEDIA_CHANGE_CONTEXT, *PCLASS_MEDIA_CHANGE_CONTEXT;

typedef struct _TAPE_STATISTICS {
  ULONG Version;
  ULONG Flags;
  LARGE_INTEGER RecoveredWrites;
  LARGE_INTEGER UnrecoveredWrites;
  LARGE_INTEGER RecoveredReads;
  LARGE_INTEGER UnrecoveredReads;
  UCHAR CompressionRatioReads;
  UCHAR CompressionRatioWrites;
} TAPE_STATISTICS, *PTAPE_STATISTICS;

typedef struct _TAPE_GET_STATISTICS {
  ULONG Operation;
} TAPE_GET_STATISTICS, *PTAPE_GET_STATISTICS;

typedef enum _STORAGE_MEDIA_TYPE {
  DDS_4mm = 0x20,
  MiniQic,
  Travan,
  QIC,
  MP_8mm,
  AME_8mm,
  AIT1_8mm,
  DLT,
  NCTP,
  IBM_3480,
  IBM_3490E,
  IBM_Magstar_3590,
  IBM_Magstar_MP,
  STK_DATA_D3,
  SONY_DTF,
  DV_6mm,
  DMI,
  SONY_D2,
  CLEANER_CARTRIDGE,
  CD_ROM,
  CD_R,
  CD_RW,
  DVD_ROM,
  DVD_R,
  DVD_RW,
  MO_3_RW,
  MO_5_WO,
  MO_5_RW,
  MO_5_LIMDOW,
  PC_5_WO,
  PC_5_RW,
  PD_5_RW,
  ABL_5_WO,
  PINNACLE_APEX_5_RW,
  SONY_12_WO,
  PHILIPS_12_WO,
  HITACHI_12_WO,
  CYGNET_12_WO,
  KODAK_14_WO,
  MO_NFR_525,
  NIKON_12_RW,
  IOMEGA_ZIP,
  IOMEGA_JAZ,
  SYQUEST_EZ135,
  SYQUEST_EZFLYER,
  SYQUEST_SYJET,
  AVATAR_F2,
  MP2_8mm,
  DST_S,
  DST_M,
  DST_L,
  VXATape_1,
  VXATape_2,
#if (NTDDI_VERSION < NTDDI_WINXP)
  STK_EAGLE,
#else
  STK_9840,
#endif
  LTO_Ultrium,
  LTO_Accelis,
  DVD_RAM,
  AIT_8mm,
  ADR_1,
  ADR_2,
  STK_9940,
  SAIT,
  VXATape
} STORAGE_MEDIA_TYPE, *PSTORAGE_MEDIA_TYPE;

typedef enum _STORAGE_BUS_TYPE {
  BusTypeUnknown = 0x00,
  BusTypeScsi,
  BusTypeAtapi,
  BusTypeAta,
  BusType1394,
  BusTypeSsa,
  BusTypeFibre,
  BusTypeUsb,
  BusTypeRAID,
  BusTypeiScsi,
  BusTypeSas,
  BusTypeSata,
  BusTypeSd,
  BusTypeMmc,
  BusTypeVirtual,
  BusTypeFileBackedVirtual,
  BusTypeMax,
  BusTypeMaxReserved = 0x7F
} STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

typedef struct _DEVICE_MEDIA_INFO {
  union {
    struct {
      LARGE_INTEGER Cylinders;
      STORAGE_MEDIA_TYPE MediaType;
      ULONG TracksPerCylinder;
      ULONG SectorsPerTrack;
      ULONG BytesPerSector;
      ULONG NumberMediaSides;
      ULONG MediaCharacteristics;
    } DiskInfo;
    struct {
      LARGE_INTEGER Cylinders;
      STORAGE_MEDIA_TYPE MediaType;
      ULONG TracksPerCylinder;
      ULONG SectorsPerTrack;
      ULONG BytesPerSector;
      ULONG NumberMediaSides;
      ULONG MediaCharacteristics;
    } RemovableDiskInfo;
    struct {
      STORAGE_MEDIA_TYPE MediaType;
      ULONG MediaCharacteristics;
      ULONG CurrentBlockSize;
      STORAGE_BUS_TYPE BusType;
      union {
        struct {
          UCHAR MediumType;
          UCHAR DensityCode;
        } ScsiInformation;
      } BusSpecificData;
    } TapeInfo;
  } DeviceSpecific;
} DEVICE_MEDIA_INFO, *PDEVICE_MEDIA_INFO;

typedef struct _GET_MEDIA_TYPES {
  ULONG DeviceType;
  ULONG MediaInfoCount;
  DEVICE_MEDIA_INFO MediaInfo[1];
} GET_MEDIA_TYPES, *PGET_MEDIA_TYPES;

typedef struct _STORAGE_PREDICT_FAILURE {
  ULONG PredictFailure;
  UCHAR VendorSpecific[512];
} STORAGE_PREDICT_FAILURE, *PSTORAGE_PREDICT_FAILURE;

typedef enum _STORAGE_QUERY_TYPE {
  PropertyStandardQuery = 0,
  PropertyExistsQuery,
  PropertyMaskQuery,
  PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef enum _STORAGE_PROPERTY_ID {
  StorageDeviceProperty = 0,
  StorageAdapterProperty,
  StorageDeviceIdProperty,
  StorageDeviceUniqueIdProperty,
  StorageDeviceWriteCacheProperty,
  StorageMiniportProperty,
  StorageAccessAlignmentProperty,
  StorageDeviceSeekPenaltyProperty,
  StorageDeviceTrimProperty,
  StorageDeviceWriteAggregationProperty,
  StorageDeviceDeviceTelemetryProperty,
  StorageDeviceLBProvisioningProperty,
  StorageDevicePowerProperty,
  StorageDeviceCopyOffloadProperty,
  StorageDeviceResiliencyProperty,
  StorageDeviceMediumProductType,
  StorageAdapterRpmbProperty,
  StorageAdapterCryptoProperty,
  StorageDeviceTieringProperty,
  StorageDeviceFaultDomainProperty,
  StorageDeviceClusportProperty,
  StorageDeviceDependantDevicesProperty,
  StorageDeviceIoCapabilityProperty = 48,
  StorageAdapterProtocolSpecificProperty,
  StorageDeviceProtocolSpecificProperty,
  StorageAdapterTemperatureProperty,
  StorageDeviceTemperatureProperty,
  StorageAdapterPhysicalTopologyProperty,
  StorageDevicePhysicalTopologyProperty,
  StorageDeviceAttributesProperty,
  StorageDeviceManagementStatus,
  StorageAdapterSerialNumberProperty,
  StorageDeviceLocationProperty,
  StorageDeviceNumaProperty,
  StorageDeviceZonedDeviceProperty,
  StorageDeviceUnsafeShutdownCount,
  StorageDeviceEnduranceProperty,
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef struct _STORAGE_PROPERTY_QUERY {
  STORAGE_PROPERTY_ID PropertyId;
  STORAGE_QUERY_TYPE QueryType;
  UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef _Struct_size_bytes_(Size) struct _STORAGE_DESCRIPTOR_HEADER {
  ULONG Version;
  ULONG Size;
} STORAGE_DESCRIPTOR_HEADER, *PSTORAGE_DESCRIPTOR_HEADER;

typedef _Struct_size_bytes_(Size) struct _STORAGE_DEVICE_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  UCHAR DeviceType;
  UCHAR DeviceTypeModifier;
  BOOLEAN RemovableMedia;
  BOOLEAN CommandQueueing;
  ULONG VendorIdOffset;
  ULONG ProductIdOffset;
  ULONG ProductRevisionOffset;
  ULONG SerialNumberOffset;
  STORAGE_BUS_TYPE BusType;
  ULONG RawPropertiesLength;
  UCHAR RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

typedef _Struct_size_bytes_(Size) struct _STORAGE_ADAPTER_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  ULONG MaximumTransferLength;
  ULONG MaximumPhysicalPages;
  ULONG AlignmentMask;
  BOOLEAN AdapterUsesPio;
  BOOLEAN AdapterScansDown;
  BOOLEAN CommandQueueing;
  BOOLEAN AcceleratedTransfer;
#if (NTDDI_VERSION < NTDDI_WINXP)
  BOOLEAN BusType;
#else
  UCHAR BusType;
#endif
  USHORT BusMajorVersion;
  USHORT BusMinorVersion;
#if (NTDDI_VERSION >= NTDDI_WIN8)
  UCHAR SrbType;
  UCHAR AddressType;
#endif
} STORAGE_ADAPTER_DESCRIPTOR, *PSTORAGE_ADAPTER_DESCRIPTOR;

typedef _Struct_size_bytes_(Size) struct _STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  ULONG BytesPerCacheLine;
  ULONG BytesOffsetForCacheAlignment;
  ULONG BytesPerLogicalSector;
  ULONG BytesPerPhysicalSector;
  ULONG BytesOffsetForSectorAlignment;
} STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR, *PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR;

typedef _Struct_size_bytes_(Size) struct _STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  ULONG MediumProductType;
} STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR, *PSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR;

typedef enum _STORAGE_PORT_CODE_SET {
  StoragePortCodeSetReserved = 0,
  StoragePortCodeSetStorport = 1,
  StoragePortCodeSetSCSIport = 2,
  StoragePortCodeSetSpaceport= 3,
  StoragePortCodeSetATAport  = 4,
  StoragePortCodeSetUSBport  = 5,
  StoragePortCodeSetSBP2port = 6,
  StoragePortCodeSetSDport   = 7
} STORAGE_PORT_CODE_SET, *PSTORAGE_PORT_CODE_SET;

typedef struct _STORAGE_MINIPORT_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  STORAGE_PORT_CODE_SET Portdriver;
  BOOLEAN LUNResetSupported;
  BOOLEAN TargetResetSupported;
#if (NTDDI_VERSION >= NTDDI_WIN8)
  USHORT IoTimeoutValue;
#endif
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
  BOOLEAN ExtraIoInfoSupported;
  UCHAR Reserved0[3];
  ULONG Reserved1;
#endif
} STORAGE_MINIPORT_DESCRIPTOR, *PSTORAGE_MINIPORT_DESCRIPTOR;

typedef struct _DEVICE_LB_PROVISIONING_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  UCHAR ThinProvisioningEnabled:1;
  UCHAR ThinProvisioningReadZeros:1;
  UCHAR AnchorSupported:3;
  UCHAR UnmapGranularityAlignmentValid:1;
  UCHAR Reserved0:2;
  UCHAR Reserved1[7];
  ULONGLONG OptimalUnmapGranularity;
  ULONGLONG UnmapGranularityAlignment;
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
  ULONG MaxUnmapLbaCount;
  ULONG MaxUnmapBlockDescriptorCount;
#endif
} DEVICE_LB_PROVISIONING_DESCRIPTOR, *PDEVICE_LB_PROVISIONING_DESCRIPTOR;

#define DEVICE_LB_PROVISIONING_DESCRIPTOR_V1_SIZE RTL_SIZEOF_THROUGH_FIELD(DEVICE_LB_PROVISIONING_DESCRIPTOR, UnmapGranularityAlignment)

typedef struct _DEVICE_POWER_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  BOOLEAN DeviceAttentionSupported;
  BOOLEAN AsynchronousNotificationSupported;
  BOOLEAN IdlePowerManagementEnabled;
  BOOLEAN D3ColdEnabled;
  BOOLEAN D3ColdSupported;
  BOOLEAN NoVerifyDuringIdlePower;
  UCHAR Reserved[2];
  ULONG IdleTimeoutInMS;
} DEVICE_POWER_DESCRIPTOR, *PDEVICE_POWER_DESCRIPTOR;

typedef struct _DEVICE_COPY_OFFLOAD_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  ULONG MaximumTokenLifetime;
  ULONG DefaultTokenLifetime;
  ULONGLONG MaximumTransferSize;
  ULONGLONG OptimalTransferCount;
  ULONG MaximumDataDescriptors;
  ULONG MaximumTransferLengthPerDescriptor;
  ULONG OptimalTransferLengthPerDescriptor;
  USHORT OptimalTransferLengthGranularity;
  UCHAR Reserved[2];
} DEVICE_COPY_OFFLOAD_DESCRIPTOR, *PDEVICE_COPY_OFFLOAD_DESCRIPTOR;

typedef _Struct_size_bytes_(Size) struct _STORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  ULONG LunMaxIoCount;
  ULONG AdapterMaxIoCount;
} STORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR, *PSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR;

typedef enum _STORAGE_IDENTIFIER_CODE_SET {
  StorageIdCodeSetReserved = 0,
  StorageIdCodeSetBinary = 1,
  StorageIdCodeSetAscii = 2,
  StorageIdCodeSetUtf8 = 3
} STORAGE_IDENTIFIER_CODE_SET, *PSTORAGE_IDENTIFIER_CODE_SET;

typedef enum _STORAGE_IDENTIFIER_TYPE {
  StorageIdTypeVendorSpecific = 0,
  StorageIdTypeVendorId = 1,
  StorageIdTypeEUI64 = 2,
  StorageIdTypeFCPHName = 3,
  StorageIdTypePortRelative = 4,
  StorageIdTypeTargetPortGroup = 5,
  StorageIdTypeLogicalUnitGroup = 6,
  StorageIdTypeMD5LogicalUnitIdentifier = 7,
  StorageIdTypeScsiNameString = 8
} STORAGE_IDENTIFIER_TYPE, *PSTORAGE_IDENTIFIER_TYPE;

typedef enum _STORAGE_ID_NAA_FORMAT {
  StorageIdNAAFormatIEEEExtended = 2,
  StorageIdNAAFormatIEEERegistered = 3,
  StorageIdNAAFormatIEEEERegisteredExtended = 5
} STORAGE_ID_NAA_FORMAT, *PSTORAGE_ID_NAA_FORMAT;

typedef enum _STORAGE_ASSOCIATION_TYPE {
  StorageIdAssocDevice = 0,
  StorageIdAssocPort = 1,
  StorageIdAssocTarget = 2
} STORAGE_ASSOCIATION_TYPE, *PSTORAGE_ASSOCIATION_TYPE;

typedef struct _STORAGE_IDENTIFIER {
  STORAGE_IDENTIFIER_CODE_SET CodeSet;
  STORAGE_IDENTIFIER_TYPE Type;
  USHORT IdentifierSize;
  USHORT NextOffset;
  STORAGE_ASSOCIATION_TYPE Association;
  UCHAR Identifier[1];
} STORAGE_IDENTIFIER, *PSTORAGE_IDENTIFIER;

typedef _Struct_size_bytes_(Size) struct _STORAGE_DEVICE_ID_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  ULONG NumberOfIdentifiers;
  UCHAR Identifiers[1];
} STORAGE_DEVICE_ID_DESCRIPTOR, *PSTORAGE_DEVICE_ID_DESCRIPTOR;

typedef struct _DEVICE_SEEK_PENALTY_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  BOOLEAN IncursSeekPenalty;
} DEVICE_SEEK_PENALTY_DESCRIPTOR, *PDEVICE_SEEK_PENALTY_DESCRIPTOR;

typedef struct _DEVICE_WRITE_AGGREGATION_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  BOOLEAN BenefitsFromWriteAggregation;
} DEVICE_WRITE_AGGREGATION_DESCRIPTOR, *PDEVICE_WRITE_AGGREGATION_DESCRIPTOR;

typedef struct _DEVICE_TRIM_DESCRIPTOR {
  ULONG Version;
  ULONG Size;
  BOOLEAN TrimEnabled;
} DEVICE_TRIM_DESCRIPTOR, *PDEVICE_TRIM_DESCRIPTOR;

typedef ULONG DEVICE_DATA_MANAGEMENT_SET_ACTION;

#define DEVICE_DSM_FLAG_TRIM_NOT_FS_ALLOCATED 0x80000000

typedef struct _DEVICE_DATA_SET_RANGE {
  LONGLONG StartingOffset;
  ULONGLONG LengthInBytes;
} DEVICE_DATA_SET_RANGE, *PDEVICE_DATA_SET_RANGE;

typedef struct _DEVICE_MANAGE_DATA_SET_ATTRIBUTES {
  ULONG Size;
  DEVICE_DATA_MANAGEMENT_SET_ACTION Action;
  ULONG Flags;
  ULONG ParameterBlockOffset;
  ULONG ParameterBlockLength;
  ULONG DataSetRangesOffset;
  ULONG DataSetRangesLength;
} DEVICE_MANAGE_DATA_SET_ATTRIBUTES, *PDEVICE_MANAGE_DATA_SET_ATTRIBUTES;

typedef struct _DEVICE_DSM_NOTIFICATION_PARAMETERS {
  ULONG Size;
  ULONG Flags;
  ULONG NumFileTypeIDs;
  GUID FileTypeID[1];
} DEVICE_DSM_NOTIFICATION_PARAMETERS, *PDEVICE_DSM_NOTIFICATION_PARAMETERS;

typedef struct _STORAGE_GET_BC_PROPERTIES_OUTPUT {
  ULONG MaximumRequestsPerPeriod;
  ULONG MinimumPeriod;
  ULONGLONG MaximumRequestSize;
  ULONG EstimatedTimePerRequest;
  ULONG NumOutStandingRequests;
  ULONGLONG RequestSize;
} STORAGE_GET_BC_PROPERTIES_OUTPUT, *PSTORAGE_GET_BC_PROPERTIES_OUTPUT;

typedef struct _STORAGE_ALLOCATE_BC_STREAM_INPUT {
  ULONG Version;
  ULONG RequestsPerPeriod;
  ULONG Period;
  BOOLEAN RetryFailures;
  BOOLEAN Discardable;
  BOOLEAN Reserved1[2];
  ULONG AccessType;
  ULONG AccessMode;
} STORAGE_ALLOCATE_BC_STREAM_INPUT, *PSTORAGE_ALLOCATE_BC_STREAM_INPUT;

typedef struct _STORAGE_ALLOCATE_BC_STREAM_OUTPUT {
  ULONGLONG RequestSize;
  ULONG NumOutStandingRequests;
} STORAGE_ALLOCATE_BC_STREAM_OUTPUT, *PSTORAGE_ALLOCATE_BC_STREAM_OUTPUT;

typedef struct _STORAGE_PRIORITY_HINT_SUPPORT {
  ULONG SupportFlags;
} STORAGE_PRIORITY_HINT_SUPPORT, *PSTORAGE_PRIORITY_HINT_SUPPORT;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)
#endif

// #if defined(_MSC_EXTENSIONS)

typedef struct _STORAGE_MEDIA_SERIAL_NUMBER_DATA {
  USHORT Reserved;
  USHORT SerialNumberLength;
  UCHAR SerialNumber[0];
} STORAGE_MEDIA_SERIAL_NUMBER_DATA, *PSTORAGE_MEDIA_SERIAL_NUMBER_DATA;

typedef struct _PERSISTENT_RESERVE_COMMAND {
  ULONG Version;
  ULONG Size;
  __MINGW_EXTENSION union {
    struct {
      UCHAR ServiceAction:5;
      UCHAR Reserved1:3;
      USHORT AllocationLength;
    } PR_IN;
    struct {
      UCHAR ServiceAction:5;
      UCHAR Reserved1:3;
      UCHAR Type:4;
      UCHAR Scope:4;
      UCHAR ParameterList[0];
    } PR_OUT;
  } DUMMYUNIONNAME;
} PERSISTENT_RESERVE_COMMAND, *PPERSISTENT_RESERVE_COMMAND;

// #endif /* defined(_MSC_EXTENSIONS) */

#ifdef _MSC_VER
#pragma warning(pop) /* disable:4200 */
#endif

typedef _Struct_size_bytes_(Size) struct _STORAGE_READ_CAPACITY {
  ULONG Version;
  ULONG Size;
  ULONG BlockLength;
  LARGE_INTEGER NumberOfBlocks;
  LARGE_INTEGER DiskLength;
} STORAGE_READ_CAPACITY, *PSTORAGE_READ_CAPACITY;

typedef enum _WRITE_CACHE_TYPE {
  WriteCacheTypeUnknown,
  WriteCacheTypeNone,
  WriteCacheTypeWriteBack,
  WriteCacheTypeWriteThrough
} WRITE_CACHE_TYPE;

typedef enum _WRITE_CACHE_ENABLE {
  WriteCacheEnableUnknown,
  WriteCacheDisabled,
  WriteCacheEnabled
} WRITE_CACHE_ENABLE;

typedef enum _WRITE_CACHE_CHANGE {
  WriteCacheChangeUnknown,
  WriteCacheNotChangeable,
  WriteCacheChangeable
} WRITE_CACHE_CHANGE;

typedef enum _WRITE_THROUGH {
  WriteThroughUnknown,
  WriteThroughNotSupported,
  WriteThroughSupported
} WRITE_THROUGH;

typedef _Struct_size_bytes_(Size) struct _STORAGE_WRITE_CACHE_PROPERTY {
  ULONG Version;
  ULONG Size;
  WRITE_CACHE_TYPE WriteCacheType;
  WRITE_CACHE_ENABLE WriteCacheEnabled;
  WRITE_CACHE_CHANGE WriteCacheChangeable;
  WRITE_THROUGH WriteThroughSupported;
  BOOLEAN FlushCacheSupported;
  BOOLEAN UserDefinedPowerProtection;
  BOOLEAN NVCacheEnabled;
} STORAGE_WRITE_CACHE_PROPERTY, *PSTORAGE_WRITE_CACHE_PROPERTY;

typedef struct _STORAGE_LB_PROVISIONING_MAP_RESOURCES {
  ULONG Size;
  ULONG Version;
  UCHAR AvailableMappingResourcesValid:1;
  UCHAR UsedMappingResourcesValid:1;
  UCHAR Reserved0:6;
  UCHAR Reserved1[3];
  UCHAR AvailableMappingResourcesScope:2;
  UCHAR UsedMappingResourcesScope:2;
  UCHAR Reserved2:4;
  UCHAR Reserved3[3];
  ULONGLONG AvailableMappingResources;
  ULONGLONG UsedMappingResources;
} STORAGE_LB_PROVISIONING_MAP_RESOURCES, *PSTORAGE_LB_PROVISIONING_MAP_RESOURCES;

typedef struct _DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT {
  ULONG Size;
  DEVICE_DATA_MANAGEMENT_SET_ACTION Action;
  ULONG Flags;
  ULONG OperationStatus;
  ULONG ExtendedError;
  ULONG TargetDetailedError;
  ULONG ReservedStatus;
  ULONG OutputBlockOffset;
  ULONG OutputBlockLength;
} DEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT, *PDEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT;

typedef struct _DEVICE_DATA_SET_LB_PROVISIONING_STATE {
  ULONG Size;
  ULONG Version;
  ULONGLONG SlabSizeInBytes;
  ULONG SlabOffsetDeltaInBytes;
  ULONG SlabAllocationBitMapBitCount;
  ULONG SlabAllocationBitMapLength;
  ULONG SlabAllocationBitMap[ANYSIZE_ARRAY];
} DEVICE_DATA_SET_LB_PROVISIONING_STATE, *PDEVICE_DATA_SET_LB_PROVISIONING_STATE,
  DEVICE_DSM_ALLOCATION_OUTPUT, *PDEVICE_DSM_ALLOCATION_OUTPUT;

#define DEVICE_DSM_ALLOCATION_OUTPUT_V1 (sizeof(DEVICE_DSM_ALLOCATION_OUTPUT))
#define DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V1 DEVICE_DSM_ALLOCATION_OUTPUT_V1

typedef struct _DEVICE_DATA_SET_LB_PROVISIONING_STATE_V2 {
  ULONG Size;
  ULONG Version;
  ULONGLONG SlabSizeInBytes;
  ULONGLONG SlabOffsetDeltaInBytes;
  ULONG SlabAllocationBitMapBitCount;
  ULONG SlabAllocationBitMapLength;
  ULONG SlabAllocationBitMap[ANYSIZE_ARRAY];
} DEVICE_DATA_SET_LB_PROVISIONING_STATE_V2, *PDEVICE_DATA_SET_LB_PROVISIONING_STATE_V2,
  DEVICE_DSM_ALLOCATION_OUTPUT2, *PDEVICE_DSM_ALLOCATION_OUTPUT2;

#define DEVICE_DSM_ALLOCATION_OUTPUT_V2 (sizeof(DEVICE_DSM_ALLOCATION_OUTPUT2))
#define DEVICE_DATA_SET_LB_PROVISIONING_STATE_VERSION_V2 DEVICE_DSM_ALLOCATION_OUTPUT_V2

#define DeviceDsmDefinition_Allocation {DeviceDsmAction_Allocation,                  \
                                        TRUE,                                        \
                                        __alignof(DEVICE_DSM_ALLOCATION_PARAMETERS), \
                                        sizeof(DEVICE_DSM_ALLOCATION_PARAMETERS),    \
                                        TRUE,                                        \
                                        __alignof(DEVICE_DSM_ALLOCATION_OUTPUT2),    \
                                        sizeof(DEVICE_DSM_ALLOCATION_OUTPUT2)}

#define DEVICE_DSM_FLAG_ALLOCATION_CONSOLIDATEABLE_ONLY 0x40000000

typedef struct _DEVICE_DATA_SET_LBP_STATE_PARAMETERS {
  ULONG Version;
  ULONG Size;
  ULONG Flags;
  ULONG OutputVersion;
} DEVICE_DATA_SET_LBP_STATE_PARAMETERS, *PDEVICE_DATA_SET_LBP_STATE_PARAMETERS,
  DEVICE_DSM_ALLOCATION_PARAMETERS, *PDEVICE_DSM_ALLOCATION_PARAMETERS;

typedef struct _STORAGE_EVENT_NOTIFICATION {
  ULONG Version;
  ULONG Size;
  ULONGLONG Events;
} STORAGE_EVENT_NOTIFICATION, *PSTORAGE_EVENT_NOTIFICATION;

#define STORAGE_EVENT_NOTIFICATION_VERSION_V1 1

#define STORAGE_EVENT_MEDIA_STATUS     0x0000000000000001
#define STORAGE_EVENT_DEVICE_STATUS    0x0000000000000002
#define STORAGE_EVENT_DEVICE_OPERATION 0x0000000000000004
#define STORAGE_EVENT_ALL (STORAGE_EVENT_MEDIA_STATUS | STORAGE_EVENT_DEVICE_STATUS | STORAGE_EVENT_DEVICE_OPERATION)

#define STORAGE_OFFLOAD_MAX_TOKEN_LENGTH        512
#define STORAGE_OFFLOAD_TOKEN_ID_LENGTH         0x1F8
#define STORAGE_OFFLOAD_TOKEN_TYPE_ZERO_DATA    0xFFFF0001

typedef struct _STORAGE_OFFLOAD_TOKEN {
  UCHAR TokenType[4];
  UCHAR Reserved[2];
  UCHAR TokenIdLength[2];
  union {
      struct {
          UCHAR Reserved2[STORAGE_OFFLOAD_TOKEN_ID_LENGTH];
      } StorageOffloadZeroDataToken;
      UCHAR Token[STORAGE_OFFLOAD_TOKEN_ID_LENGTH];
  } DUMMYUNIONNAME;
} STORAGE_OFFLOAD_TOKEN, *PSTORAGE_OFFLOAD_TOKEN;

#define MAKE_ZERO_TOKEN(T) (                                  \
    ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[0] = 0xFF,         \
    ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[1] = 0xFF,         \
    ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[2] = 0x00,         \
    ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[3] = 0x01,         \
    ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenIdLength[0] = 0x01,     \
    ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenIdLength[1] = 0xF8      \
)

#define IS_ZERO_TOKEN(T) (                                    \
    (((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[0] == 0xFF     && \
     ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[1] == 0xFF     && \
     ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[2] == 0x00     && \
     ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenType[3] == 0x01     && \
     ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenIdLength[0] == 0x01 && \
     ((PSTORAGE_OFFLOAD_TOKEN)T)->TokenIdLength[1] == 0xF8)   \
)

typedef struct _DEVICE_DSM_OFFLOAD_READ_PARAMETERS {
  ULONG Flags;
  ULONG TimeToLive;
  ULONG Reserved[2];
} DEVICE_DSM_OFFLOAD_READ_PARAMETERS, *PDEVICE_DSM_OFFLOAD_READ_PARAMETERS;

#define STORAGE_OFFLOAD_READ_RANGE_TRUNCATED 0x00000001

typedef struct _STORAGE_OFFLOAD_READ_OUTPUT {
  ULONG OffloadReadFlags;
  ULONG Reserved;
  ULONGLONG LengthProtected;
  ULONG TokenLength;
  STORAGE_OFFLOAD_TOKEN Token;
} STORAGE_OFFLOAD_READ_OUTPUT, *PSTORAGE_OFFLOAD_READ_OUTPUT;

#define DeviceDsmDefinition_OffloadRead {DeviceDsmAction_OffloadRead,                   \
                                         FALSE,                                         \
                                         __alignof(DEVICE_DSM_OFFLOAD_READ_PARAMETERS), \
                                         sizeof(DEVICE_DSM_OFFLOAD_READ_PARAMETERS),    \
                                         FALSE,                                         \
                                         __alignof(STORAGE_OFFLOAD_READ_OUTPUT),        \
                                         sizeof(STORAGE_OFFLOAD_READ_OUTPUT)}

typedef struct _DEVICE_DSM_OFFLOAD_WRITE_PARAMETERS {
  ULONG Flags;
  ULONG Reserved;
  ULONGLONG TokenOffset;
  STORAGE_OFFLOAD_TOKEN Token;
} DEVICE_DSM_OFFLOAD_WRITE_PARAMETERS, *PDEVICE_DSM_OFFLOAD_WRITE_PARAMETERS;

#define STORAGE_OFFLOAD_WRITE_RANGE_TRUNCATED   0x0001
#define STORAGE_OFFLOAD_TOKEN_INVALID           0x0002

typedef struct _STORAGE_OFFLOAD_WRITE_OUTPUT {
  ULONG OffloadWriteFlags;
  ULONG Reserved;
  ULONGLONG LengthCopied;
} STORAGE_OFFLOAD_WRITE_OUTPUT, *PSTORAGE_OFFLOAD_WRITE_OUTPUT;

#define DeviceDsmDefinition_OffloadWrite {DeviceDsmAction_OffloadWrite,                   \
                                          FALSE,                                          \
                                          __alignof(DEVICE_DSM_OFFLOAD_WRITE_PARAMETERS), \
                                          sizeof(DEVICE_DSM_OFFLOAD_WRITE_PARAMETERS),    \
                                          FALSE,                                          \
                                          __alignof(STORAGE_OFFLOAD_WRITE_OUTPUT),        \
                                          sizeof(STORAGE_OFFLOAD_WRITE_OUTPUT)}


#define READ_COPY_NUMBER_KEY                    0x52434e00  //'RCN'
#define READ_COPY_NUMBER_BYPASS_CACHE_FLAG      0x00000100

#define IsKeyReadCopyNumber(_k)                 (((_k) & 0xFFFFFE00) == READ_COPY_NUMBER_KEY)

#define IsKeyReadCopyNumberBypassCache(_k)      ((_k) & READ_COPY_NUMBER_BYPASS_CACHE_FLAG)
#define SetReadCopyNumberBypassCacheToKey(_k)   ((_k) |= READ_COPY_NUMBER_BYPASS_CACHE_FLAG)

#define ReadCopyNumberToKey(_c)                 (READ_COPY_NUMBER_KEY | (UCHAR)(_c))
#define ReadCopyNumberFromKey(_k)               (UCHAR)((_k) & 0x000000FF)

typedef struct _STORAGE_IDLE_POWER {
  ULONG Version;
  ULONG Size;
  ULONG WakeCapableHint:1;
  ULONG D3ColdSupported:1;
  ULONG Reserved:30;
  ULONG D3IdleTimeout;
} STORAGE_IDLE_POWER, *PSTORAGE_IDLE_POWER;


// for IOCTL_STORAGE_GET_IDLE_POWERUP_REASON

typedef enum _STORAGE_POWERUP_REASON_TYPE {
  StoragePowerupUnknown = 0,
  StoragePowerupIO,
  StoragePowerupDeviceAttention
} STORAGE_POWERUP_REASON_TYPE, *PSTORAGE_POWERUP_REASON_TYPE;

typedef struct _STORAGE_IDLE_POWERUP_REASON
{
  ULONG Version;
  ULONG Size;
  STORAGE_POWERUP_REASON_TYPE PowerupReason;
} STORAGE_IDLE_POWERUP_REASON, *PSTORAGE_IDLE_POWERUP_REASON;

#define STORAGE_IDLE_POWERUP_REASON_VERSION_V1 1

#ifdef __cplusplus
}
#endif

#endif /* _NTDDSTOR_H_ */
