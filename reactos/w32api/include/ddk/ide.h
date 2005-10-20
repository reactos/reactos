/*
 * ide.h
 *
 * IDE driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Hervé Poussineau <hpoussin@reactos.org>
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

#ifndef __IDE_H
#define __IDE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_IDE_CHANNEL   2
#define MAX_IDE_DEVICE    2

#include <pshpack1.h>
typedef struct _IDE_DRIVE_IDENTIFY
{
  USHORT GeneralConfiguration;
  USHORT NumberOfCylinders;
  USHORT Reserved1;
  USHORT NumberOfHeads;
  USHORT UnformattedBytesPerTrack;
  USHORT UnformattedBytesPerSector;
  USHORT SectorsPerTrack;
  USHORT VendorUnique1[3];
  BYTE   SerialNumber[20];
  USHORT BufferType;
  USHORT BufferSectorSize;
  USHORT NumberOfEccBytes;
  BYTE   FirmwareRevision[8];
  BYTE   ModelNumber[40];
  BYTE   MaximumBlockTransfer;
  BYTE   VendorUnique2;
  USHORT DoubleWordIo;
  USHORT Capabilities;
  USHORT Reserved2;
  BYTE   VendorUnique3;
  BYTE   PioCycleTimingMode;
  BYTE   VendorUnique4;
  BYTE   DmaCycleTimingMode;
  USHORT TranslationFieldsValid:3;
  USHORT Reserved3:13;
  USHORT NumberOfCurrentCylinders;
  USHORT NumberOfCurrentHeads;
  USHORT CurrentSectorsPerTrack;
  ULONG  CurrentSectorCapacity;
  USHORT CurrentMultiSectorSetting;
  ULONG  UserAddressableSectors;
  USHORT SingleWordDMASupport : 8;
  USHORT SingleWordDMAActive : 8;
  USHORT MultiWordDMASupport : 8;
  USHORT MultiWordDMAActive : 8;
  USHORT AdvancedPIOModes : 8;
  USHORT Reserved4 : 8;
  USHORT MinimumMWXferCycleTime;
  USHORT RecommendedMWXferCycleTime;
  USHORT MinimumPIOCycleTime;
  USHORT MinimumPIOCycleTimeIORDY;
  USHORT Reserved5[11];
  USHORT MajorRevision;
  USHORT MinorRevision;
  USHORT Reserved6[6];
  USHORT UltraDMASupport : 8;
  USHORT UltraDMAActive : 8;
  USHORT Reserved7[37];
  USHORT LastLun:3;
  USHORT Reserved8:13;
  USHORT MediaStatusNotification:2;
  USHORT Reserved9:6;
  USHORT DeviceWriteProtect:1;
  USHORT Reserved10:7;
  USHORT Reserved11[128];
} IDE_DRIVE_IDENTIFY, *PIDE_DRIVE_IDENTIFY;
#include <poppack.h>

typedef struct _PCIIDE_TRANSFER_MODE_SELECT
{
  ULONG Channel;
  BOOLEAN DevicePresent[MAX_IDE_DEVICE];
  BOOLEAN FixedDisk[MAX_IDE_DEVICE];
  BOOLEAN IoReadySupported[MAX_IDE_DEVICE];
  ULONG DeviceTransferModeSupported[MAX_IDE_DEVICE];
  ULONG BestPioCycleTime[MAX_IDE_DEVICE];
  ULONG BestSwDmaCycleTime[MAX_IDE_DEVICE];
  ULONG BestMwDmaCycleTime[MAX_IDE_DEVICE];
  ULONG BestUDmaCycleTime[MAX_IDE_DEVICE];
  ULONG DeviceTransferModeCurrent[MAX_IDE_DEVICE];
  ULONG DeviceTransferModeSelected[MAX_IDE_DEVICE];
} PCIIDE_TRANSFER_MODE_SELECT, *PPCIIDE_TRANSFER_MODE_SELECT;

typedef enum
{
  ChannelDisabled,
  ChannelEnabled,
  ChannelStateUnknown
} IDE_CHANNEL_STATE;

typedef IDE_CHANNEL_STATE
(NTAPI *PCIIDE_CHANNEL_ENABLED)(
  IN PVOID DeviceExtension,
  IN ULONG Channel);

typedef BOOLEAN
(NTAPI *PCIIDE_SYNC_ACCESS_REQUIRED)(
  IN PVOID DeviceExtension);

typedef NTSTATUS
(NTAPI *PCIIDE_TRANSFER_MODE_SELECT_FUNC)(
  IN PVOID DeviceExtension,
  IN OUT PPCIIDE_TRANSFER_MODE_SELECT XferMode);

typedef BOOLEAN
(NTAPI *PCIIDE_USEDMA_FUNC)(
  IN PVOID DeviceExtension,
  IN PUCHAR CdbCommand,
  IN PUCHAR Slave);

typedef NTSTATUS
(NTAPI *PCIIDE_UDMA_MODES_SUPPORTED)(
  IN IDE_DRIVE_IDENTIFY IdentifyData,
  OUT PULONG BestXferMode,
  OUT PULONG CurrentXferMode);

typedef struct _IDE_CONTROLLER_PROPERTIES
{
  ULONG Size;
  ULONG ExtensionSize;
  ULONG SupportedTransferMode[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];
  PCIIDE_CHANNEL_ENABLED PciIdeChannelEnabled;
  PCIIDE_SYNC_ACCESS_REQUIRED PciIdeSyncAccessRequired;
  PCIIDE_TRANSFER_MODE_SELECT_FUNC PciIdeTransferModeSelect;
  BOOLEAN IgnoreActiveBitForAtaDevice;
  BOOLEAN AlwaysClearBusMasterInterrupt;
  PCIIDE_USEDMA_FUNC PciIdeUseDma;
  ULONG AlignmentRequirement;
  ULONG DefaultPIO;
  PCIIDE_UDMA_MODES_SUPPORTED PciIdeUdmaModesSupported;
} IDE_CONTROLLER_PROPERTIES, *PIDE_CONTROLLER_PROPERTIES;

typedef NTSTATUS
(NTAPI *PCONTROLLER_PROPERTIES)(
  IN PVOID DeviceExtension,
  IN PIDE_CONTROLLER_PROPERTIES ControllerProperties);

NTSTATUS NTAPI
PciIdeXInitialize(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath,
  IN PCONTROLLER_PROPERTIES HwGetControllerProperties,
  IN ULONG ExtensionSize);

NTSTATUS NTAPI
PciIdeXGetBusData(
  IN PVOID DeviceExtension,
  IN PVOID Buffer,
  IN ULONG ConfigDataOffset,
  IN ULONG BufferLength);

NTSTATUS NTAPI
PciIdeXSetBusData(
  IN PVOID DeviceExtension,
  IN PVOID Buffer,
  IN PVOID DataMask,
  IN ULONG ConfigDataOffset,
  IN ULONG BufferLength);

/* Bit field values for 
 * PCIIDE_TRANSFER_MODE_SELECT.DeviceTransferModeSupported and
 * IDE_CONTROLLER_PROPERTIES.SupportedTransferMode
 */
// PIO Modes
#define PIO_MODE0   (1 << 0)
#define PIO_MODE1   (1 << 1)
#define PIO_MODE2   (1 << 2)
#define PIO_MODE3   (1 << 3)
#define PIO_MODE4   (1 << 4)
// Single-word DMA Modes
#define SWDMA_MODE0 (1 << 5)
#define SWDMA_MODE1 (1 << 6)
#define SWDMA_MODE2 (1 << 7)
// Multi-word DMA Modes
#define MWDMA_MODE0 (1 << 8)
#define MWDMA_MODE1 (1 << 9)
#define MWDMA_MODE2 (1 << 10)
// Ultra DMA Modes
#define UDMA_MODE0  (1 << 11)
#define UDMA_MODE1  (1 << 12)
#define UDMA_MODE2  (1 << 13)
#define UDMA_MODE3  (1 << 14)
#define UDMA_MODE4  (1 << 15)

#ifdef __cplusplus
}
#endif

#endif /* __IDE_H */
