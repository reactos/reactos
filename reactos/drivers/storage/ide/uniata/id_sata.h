#ifndef __UNIATA_SATA__H__
#define __UNIATA_SATA__H__

UCHAR
UniataSataConnect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel          // logical channel
    );

UCHAR
UniataSataPhyEnable(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel          // logical channel
    );

#define UNIATA_SATA_DO_CONNECT        TRUE
#define UNIATA_SATA_IGNORE_CONNECT    FALSE

BOOLEAN
UniataSataClearErr(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN BOOLEAN do_connect
    );

#define UNIATA_SATA_EVENT_ATTACH      0x01
#define UNIATA_SATA_EVENT_DETACH      0x02

BOOLEAN
UniataSataEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG Action
    );

#endif __UNIATA_SATA__H__
