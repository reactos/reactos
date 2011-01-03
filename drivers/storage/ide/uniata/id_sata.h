#ifndef __UNIATA_SATA__H__
#define __UNIATA_SATA__H__

UCHAR
NTAPI
UniataSataConnect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port = 0 /* for port multipliers */
    );

UCHAR
NTAPI
UniataSataPhyEnable(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port = 0 /* for port multipliers */
    );

#define UNIATA_SATA_DO_CONNECT        TRUE
#define UNIATA_SATA_IGNORE_CONNECT    FALSE

BOOLEAN
NTAPI
UniataSataClearErr(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN BOOLEAN do_connect,
    IN ULONG pm_port = 0 /* for port multipliers */
    );

#define UNIATA_SATA_EVENT_ATTACH      0x01
#define UNIATA_SATA_EVENT_DETACH      0x02

BOOLEAN
NTAPI
UniataSataEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG Action,
    IN ULONG pm_port = 0 /* for port multipliers */
    );

#define UniataIsSATARangeAvailable(deviceExtension, lChannel) \
    ((deviceExtension->BaseIoAddressSATA_0.Addr || \
      deviceExtension->BaseIoAHCI_0.Addr) && \
        (deviceExtension->chan[lChannel].RegTranslation[IDX_SATA_SStatus].Addr))

ULONG
NTAPI
UniataSataReadPort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG pm_port=0 /* for port multipliers */
    );

VOID
NTAPI
UniataSataWritePort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG data,
    IN ULONG pm_port=0 /* for port multipliers */
    );

BOOLEAN
NTAPI
UniataAhciInit(
    IN PVOID HwDeviceExtension
    );

UCHAR
NTAPI
UniataAhciStatus(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber
    );

ULONG
NTAPI
UniataAhciSetupFIS(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
   OUT PUCHAR fis,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature,
    IN ULONG flags
    );

#endif //__UNIATA_SATA__H__
