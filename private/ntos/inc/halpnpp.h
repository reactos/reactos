/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1998  Microsoft Corporation

Module Name:

    halpnpp.h

Abstract:

    Private interface from 'legacy' hal to 'PnP' class drivers that support
    new functionality

Author:

    Mike Gallop (mikeg) April, 1998

Revision History:

--*/


#define ISA_FTYPE_DMA_INTERFACE_VERSION 1
#define ISA_DMA_CHANNELS 8

typedef
NTSTATUS
(*PISA_CLAIM_FTYPE_CHANNEL)(
    IN PVOID Context,
    IN ULONG Channel,
    OUT PULONG ChannelInfo
    );

typedef
NTSTATUS
(*PISA_RELEASE_FTYPE_CHANNEL)(
    IN PVOID Context,
    IN ULONG Channel
    );

/*++


Routine Description:

    This returns information about children to be enumerated by a multifunction
    driver.

Arguments:

    Context - Context from the ISA_FTYPE_DMA_INTERFACE

    Channel - Channel to try and set to F-Type DMA

    ChannelInfo - Result of the set. Returns the mask of channels set to F-Type

Return Value:

    Status code that indicates whether or not the function was successful.

    STATUS_NO_MORE_ENTRIES indicates that the are no more children to enumerate

--*/

typedef struct _ISA_FTYPE_DMA_INTERFACE {

    //
    // Generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    //
    //
    PISA_CLAIM_FTYPE_CHANNEL IsaSetFTypeChannel;
    PISA_RELEASE_FTYPE_CHANNEL IsaReleaseFTypeChannel;

} ISA_FTYPE_DMA_INTERFACE, *PISA_FTYPE_DMA_INTERFACE;



DEFINE_GUID(GUID_ISA_FDMA_INTERFACE,
            0xEFF58E88L, 0xCE6B, 0x11D1, 0x8B, 0xA8, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xD0);

DEFINE_GUID( GUID_FDMA_INTERFACE_PRIVATE,
            0x60526D5EL, 0xCF34, 0x11D1, 0x8B, 0xA8, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xD0 );


