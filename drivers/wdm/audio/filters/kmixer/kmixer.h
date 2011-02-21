#pragma once

#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <swenum.h>
#define YDEBUG
#include <debug.h>

#include <samplerate.h>
#include <float_cast.h>

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;


}KMIXER_DEVICE_EXT, *PKMIXER_DEVICE_EXT;

typedef struct
{
    KSPIN_LOCK Lock;


}SUM_NODE_CONTEXT, *PSUM_NODE_CONTEXT;


NTSTATUS
NTAPI
KMixAllocateDeviceHeader(
    IN PKMIXER_DEVICE_EXT DeviceExtension);

NTSTATUS
CreatePin(
    IN PIRP Irp);
