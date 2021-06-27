/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/filters/splitter/splitter.c
 * PURPOSE:         Splitter entry point
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

const GUID PIN_VIDEO_CAPTURE = {0xfb6c4281, 0x0353, 0x11d1, {0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba}};
const GUID KSPROPSETID_Audio = {0x45FFAAA0, 0x6E1B, 0x11D0, {0xBC, 0xF2, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID KSCATEGORY_AUDIO_SPLITTER = {0x9EA331FA, 0xB91B, 0x45F8, {0x92, 0x85, 0xBD, 0x2B, 0xC7, 0x7A, 0xFC, 0xDE}};
const GUID KSNAME_Filter     = {0x9b365890, 0x165f, 0x11d0, {0xa1, 0x95, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4}};
const GUID GUID_NULL = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


KSPROPERTY_ITEM
PinPropertyTable[] =
{
    {
        KSPROPERTY_AUDIO_POSITION,
        {AudioPositionPropertyHandler},
        sizeof(KSPROPERTY),
        sizeof(KSAUDIO_POSITION),
        {AudioPositionPropertyHandler},
        NULL,
        0,
        NULL,
        NULL,
        0
    }
};

const
KSPROPERTY_SET
PinPropertySet[] =
{
    {
        &KSPROPSETID_Audio,
        1,
        PinPropertyTable,
        0,
        NULL
    }
};


const
KSAUTOMATION_TABLE
PinAutomation =
{
    1,
    sizeof(PinPropertySet) / sizeof(KSPROPERTY_SET),
    (const KSPROPERTY_SET*)&PinPropertySet,
    0,
    sizeof(KSMETHOD_SET),
    NULL,
    0,
    sizeof(KSEVENT_SET),
    NULL
};

const
KSPIN_DISPATCH
PinDispatch =
{
    PinCreate,
    PinClose,
    NULL, /* filter centric processing */
    PinReset,
    NULL,
    PinState,
    NULL,
    NULL,
    NULL,
    NULL
};


const
KSFILTER_DISPATCH
FilterDispatch =
{
    NULL,
    NULL,
    FilterProcess,
    NULL
};

KSDATARANGE
PinDataFormatRange =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        {STATIC_KSDATAFORMAT_TYPE_AUDIO},
        {STATIC_GUID_NULL},
        {STATIC_GUID_NULL}
    }
};

const
PKSDATARANGE
PinDataFormatRanges[] =
{
    &PinDataFormatRange
};

#if 0
const
KSALLOCATOR_FRAMING_EX
AllocatorFraming =
{
    1,
    0,
    {
        1,
        1,
        0
    },
    0,
    {
        {STATIC_KSMEMORY_TYPE_KERNEL_PAGED},
        {STATIC_GUID_NULL},
        KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE,
        0,
        0, //KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY | KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO | KSALLOCATOR_OPTIONF_VALID
        8,
        FILE_64_BYTE_ALIGNMENT,
        0,
        {
            0,
            MAXULONG,
            1
        },
        {
            {
                1536, // (48000khz * 2 channels * 16bit / 1000 ) * 8 frames
                1536,
                1
            },
            0,
            0
        }
    }
};
#endif

const
KSPIN_DESCRIPTOR_EX
PinDescriptors[] =
{
    {
        &PinDispatch,
        &PinAutomation,
        {
            0, //no interfaces
            NULL,
            0, // no mediums
            NULL,
            sizeof(PinDataFormatRange) / sizeof(PKSDATARANGE),
            (const PKSDATARANGE*)&PinDataFormatRange,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,
            (const GUID*)&PIN_VIDEO_CAPTURE,
            {
                0,
            }
        },
        KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING | KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL |
        KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING | KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE,
        MAXULONG,
        1,
        NULL, //&AllocatorFraming,
        PinIntersectHandler
    },
    {
        &PinDispatch,
        &PinAutomation,
        {
            0, //no interfaces
            NULL,
            0, // no mediums
            NULL,
            1,
            (const PKSDATARANGE*)&PinDataFormatRange,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,
            NULL,
            {
                0
            }
        },
        KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING | KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL | KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE,
        1,
        1,
        NULL, //&AllocatorFraming,
        PinIntersectHandler
    }
};
const
GUID
Categories[] =
{
    {STATIC_KSCATEGORY_AUDIO},
    {STATIC_KSCATEGORY_AUDIO_SPLITTER}
};


const
KSNODE_DESCRIPTOR
NodeDescriptor[] =
{
    {
        NULL, //automation table
        &KSCATEGORY_AUDIO_SPLITTER, //type
        NULL // name
    }
};


const
KSFILTER_DESCRIPTOR
FilterDescriptor =
{
    &FilterDispatch,
    NULL,
    KSFILTER_DESCRIPTOR_VERSION,
    KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING,
    &KSNAME_Filter,
    2,
    sizeof(KSPIN_DESCRIPTOR_EX),
    (const KSPIN_DESCRIPTOR_EX*)&PinDescriptors,
    sizeof(Categories) / sizeof(GUID),
    (const GUID* )&Categories,
    sizeof(NodeDescriptor) / sizeof(KSNODE_DESCRIPTOR),
    sizeof(KSNODE_DESCRIPTOR),
    (const KSNODE_DESCRIPTOR*)&NodeDescriptor,
    DEFINE_KSFILTER_DEFAULT_CONNECTIONS,
    NULL
};

const
KSFILTER_DESCRIPTOR *
FilterDescriptors =
{
    (const KSFILTER_DESCRIPTOR *)&FilterDescriptor
};

const
KSDEVICE_DESCRIPTOR
DeviceDescriptor =
{
    NULL, //no pnp notifications needed
    1, // filter descriptor count
    (const KSFILTER_DESCRIPTOR * const *)&FilterDescriptors,
    0, // pre KSDEVICE_DESCRIPTOR_VERSION
    0
};


NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPathName)
{
    return KsInitializeDriver(DriverObject, RegistryPathName, &DeviceDescriptor);
}
