#ifndef KMIXER_H__
#define KMIXER_H__

#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#define YDEBUG
#include <debug.h>

#include <samplerate.h>
#include <float_cast.h>

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;


}KMIXER_DEVICE_EXT, *PKMIXER_DEVICE_EXT;

NTSTATUS
NTAPI
KMixAllocateDeviceHeader(
    IN PKMIXER_DEVICE_EXT DeviceExtension);


#endif
