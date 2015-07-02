#pragma once

#define YDEBUG
#include <ntddk.h>
#include <debug.h>
#include <initguid.h>
#include <hdaudio.h>
#include <stdio.h>

#define TAG_HDA 'bADH'


// include Haiku headers
#include "driver.h"

#define MAKE_RATE(base, multiply, divide) \
	((base == 44100 ? FORMAT_44_1_BASE_RATE : 0) \
		| ((multiply - 1) << FORMAT_MULTIPLY_RATE_SHIFT) \
		| ((divide - 1) << FORMAT_DIVIDE_RATE_SHIFT))

#define HDAC_INPUT_STREAM_OFFSET(index) \
	((index) * HDAC_STREAM_SIZE)
#define HDAC_OUTPUT_STREAM_OFFSET(num_input_streams, index) \
	((num_input_streams + (index)) * HDAC_STREAM_SIZE)
#define HDAC_BIDIR_STREAM_OFFSET(num_input_streams, num_output_streams, index) \
	((num_input_streams + num_output_streams \
		+ (index)) * HDAC_STREAM_SIZE)

#define ALIGN(size, align)	(((size) + align - 1) & ~(align - 1))


typedef struct {
	ULONG response;
	ULONG flags;
}RIRB_RESPONSE, *PRIRB_RESPONSE;

typedef struct
{
	PDEVICE_OBJECT ChildPDO;
	ULONG FunctionGroup;
	ULONG NodeId;
}HDA_CODEC_AUDIO_GROUP, *PHDA_CODEC_AUDIO_GROUP;

typedef struct
{
	USHORT		VendorId;
	USHORT		ProductId;
	UCHAR		Major;
	UCHAR		Minor;
	UCHAR		Revision;
	UCHAR		Stepping;
	UCHAR		Addr;

	ULONG Responses[MAX_CODEC_RESPONSES];
	ULONG ResponseCount;

	PHDA_CODEC_AUDIO_GROUP AudioGroups[HDA_MAX_AUDIO_GROUPS];
	ULONG AudioGroupCount;

}HDA_CODEC_ENTRY, *PHDA_CODEC_ENTRY;


typedef struct
{
	BOOLEAN IsFDO;
	PDEVICE_OBJECT LowerDevice;
	
	PUCHAR RegBase;
	PKINTERRUPT Interrupt;

	ULONG CorbLength;
	PULONG CorbBase;
	ULONG RirbLength;
	PRIRB_RESPONSE RirbBase;
	ULONG RirbReadPos;
	ULONG CorbWritePos;
	PVOID StreamPositions;

	PHDA_CODEC_ENTRY Codecs[HDA_MAX_CODECS + 1];

}HDA_FDO_DEVICE_EXTENSION, *PHDA_FDO_DEVICE_EXTENSION;

typedef struct
{
	BOOLEAN IsFDO;
	PHDA_CODEC_ENTRY Codec;
	PHDA_CODEC_AUDIO_GROUP AudioGroup;
	PDEVICE_OBJECT FDO;
}HDA_PDO_DEVICE_EXTENSION, *PHDA_PDO_DEVICE_EXTENSION;


typedef struct {
	ULONG device : 16;
	ULONG vendor : 16;
	ULONG stepping : 8;
	ULONG revision : 8;
	ULONG minor : 4;
	ULONG major : 4;
	ULONG _reserved0 : 8;
	ULONG count : 8;
	ULONG _reserved1 : 8;
	ULONG start : 8;
	ULONG _reserved2 : 8;
}CODEC_RESPONSE, *PCODEC_RESPONSE;


PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

VOID
FreeItem(
    IN PVOID Item);

/* fdo.cpp */
BOOLEAN
NTAPI
HDA_InterruptService(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext);

NTSTATUS
NTAPI
HDA_FDOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
HDA_FDOQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
HDA_SendVerbs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHDA_CODEC_ENTRY Codec,
    IN PULONG Verbs,
    OUT PULONG Responses,
    IN ULONG Count);

/* pdo.cpp*/

NTSTATUS
HDA_PDOQueryBusInformation(
    IN PIRP Irp);

NTSTATUS
NTAPI
HDA_PDOQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
HDA_PDOHandleQueryDeviceText(
    IN PIRP Irp);

NTSTATUS
HDA_PDOQueryBusDeviceCapabilities(
    IN PIRP Irp);

NTSTATUS
HDA_PDOQueryBusDevicePnpState(
    IN PIRP Irp);

/* businterface.cpp */

NTSTATUS
HDA_PDOHandleQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* hdaudbus.cpp*/

NTSTATUS
NTAPI
HDA_SyncForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

