/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/miniport/vga_new/vga.c
 * PURPOSE:         Main Standard VGA-compatible Minport Handling Code
 * PROGRAMMERS:     Copyright (c) 1992  Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */

//---------------------------------------------------------------------------

#include "vga.h"

#include <devioctl.h>

//---------------------------------------------------------------------------
//
// Function declarations
//
// Functions that start with 'VGA' are entry points for the OS port driver.
//

VP_STATUS
NTAPI
VgaFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

BOOLEAN
NTAPI
VgaInitialize(
    PVOID HwDeviceExtension
    );

BOOLEAN
NTAPI
VgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

//
// Private function prototypes.
//

VP_STATUS
NTAPI
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
NTAPI
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    );

VP_STATUS
NTAPI
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
NTAPI
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize,
// eVb: 1.1 [SET MODE] - Add new output parameter for framebuffer update functionality
    PULONG PhysPtrChange
// eVb: 1.1 [END]
    );

BOOLEAN
NTAPI
VgaIsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
NTAPI
VgaInterpretCmdStream(
    PVOID HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
NTAPI
VgaSetPaletteReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_PALETTE_DATA PaletteBuffer,
    ULONG PaletteBufferSize
    );

VP_STATUS
NTAPI
VgaSetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    );

VP_STATUS
NTAPI
GetDeviceDataCallback(
   PVOID HwDeviceExtension,
   PVOID Context,
   VIDEO_DEVICE_DATA_TYPE DeviceDataType,
   PVOID Identifier,
   ULONG IdentifierLength,
   PVOID ConfigurationData,
   ULONG ConfigurationDataLength,
   PVOID ComponentInformation,
   ULONG ComponentInformationLength
   );

// eVb: 1.2 [RESOURCE] - Add new function for acquiring VGA resources (I/O, memory)
VP_STATUS
NTAPI
VgaAcquireResources(
    PHW_DEVICE_EXTENSION DeviceExtension
    );
// eVb: 1.2 [END]

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,VgaFindAdapter)
#pragma alloc_text(PAGE,VgaInitialize)
#pragma alloc_text(PAGE,VgaStartIO)
#pragma alloc_text(PAGE,VgaIsPresent)
#pragma alloc_text(PAGE,VgaSetColorLookup)
#endif

//---------------------------------------------------------------------------
ULONG
// eVb: 1.3 [GCC] - Add NTAPI for GCC support
NTAPI
// eVb: 1.3 [END]
DriverEntry(
    PVOID Context1,
    PVOID Context2
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    Context1 - First context value passed by the operating system. This is
        the value with which the miniport driver calls VideoPortInitialize().

    Context2 - Second context value passed by the operating system. This is
        the value with which the miniport driver calls 3VideoPortInitialize().

Return Value:

    Status from VideoPortInitialize()

--*/

{

    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    ULONG status;
    ULONG initializationStatus = (ULONG) -1;

    //
    // Zero out structure.
    //

    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    //
    // Specify sizes of structure and extension.
    //

    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitData.HwFindAdapter = VgaFindAdapter;
    hwInitData.HwInitialize = VgaInitialize;
    hwInitData.HwInterrupt = NULL;
    hwInitData.HwStartIO = VgaStartIO;

    //
    // Determine the size we require for the device extension.
    //

    hwInitData.HwDeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

    //
    // Always start with parameters for device0 in this case.
    // We can leave it like this since we know we will only ever find one
    // VGA type adapter in a machine.
    //

    // hwInitData.StartingDeviceNumber = 0;

    //
    // Once all the relevant information has been stored, call the video
    // port driver to do the initialization.
    // For this device we will repeat this call three times, for ISA, EISA
    // and PCI.
    // We will return the minimum of all return values.
    //

    //
    // We will try the PCI bus first so that our ISA detection does'nt claim
    // PCI cards (since it is impossible to differentiate between the two
    // by looking at the registers).
    //

    //
    // NOTE: since this driver only supports one adapter, we will return
    // as soon as we find a device, without going on to the following buses.
    // Normally one would call for each bus type and return the smallest
    // value.
    //

#if !defined(_ALPHA_)

    //
    // Before we can enable this on ALPHA we need to find a way to map a
    // sparse view of a 4MB region successfully.
    //

    hwInitData.AdapterInterfaceType = PCIBus;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }

#endif

    hwInitData.AdapterInterfaceType = MicroChannel;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    //
    // Return immediately instead of checkin for smallest return code.
    //

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }


    hwInitData.AdapterInterfaceType = Internal;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }


    hwInitData.AdapterInterfaceType = Isa;

    initializationStatus = VideoPortInitialize(Context1,
                                               Context2,
                                               &hwInitData,
                                               NULL);

    if (initializationStatus == NO_ERROR)
    {
        return initializationStatus;
    }



    hwInitData.AdapterInterfaceType = Eisa;

    status = VideoPortInitialize(Context1,
                                 Context2,
                                 &hwInitData,
                                 NULL);

    if (initializationStatus > status) {
        initializationStatus = status;
    }

    return initializationStatus;

} // end DriverEntry()

//---------------------------------------------------------------------------
VP_STATUS
NTAPI
VgaFindAdapter(
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    )

/*++

Routine Description:

    This routine is called to determine if the adapter for this driver
    is present in the system.
    If it is present, the function fills out some information describing
    the adapter.

Arguments:

    HwDeviceExtension - Supplies the miniport driver's adapter storage. This
        storage is initialized to zero before this call.

    HwContext - Supplies the context value which was passed to
        VideoPortInitialize().

    ArgumentString - Supplies a NULL terminated ASCII string. This string
        originates from the user.

    ConfigInfo - Returns the configuration information structure which is
        filled by the miniport driver. This structure is initialized with
        any known configuration information (such as SystemIoBusNumber) by
        the port driver. Where possible, drivers should have one set of
        defaults which do not require any supplied configuration information.

    Again - Indicates if the miniport driver wants the port driver to call
        its VIDEO_HW_FIND_ADAPTER function again with a new device extension
        and the same config info. This is used by the miniport drivers which
        can search for several adapters on a bus.

Return Value:

    This routine must return:

    NO_ERROR - Indicates a host adapter was found and the
        configuration information was successfully determined.

    ERROR_INVALID_PARAMETER - Indicates an adapter was found but there was an
        error obtaining the configuration information. If possible an error
        should be logged.

    ERROR_DEV_NOT_EXIST - Indicates no host adapter was found for the
        supplied configuration information.

--*/

{

    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    //
    // Make sure the size of the structure is at least as large as what we
    // are expecting (check version of the config info structure).
    //

    if (ConfigInfo->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {

        return ERROR_INVALID_PARAMETER;

    }
// eVb: 1.4 [CIRRUS] - Remove CIRRUS-specific support
    //
    // Check internal VGA (MIPS and ARM systems)
    //

    if ((ConfigInfo->AdapterInterfaceType == Internal) &&
        (VideoPortGetDeviceData(HwDeviceExtension,
                                VpControllerData,
                                &GetDeviceDataCallback,
                                VgaAccessRange) != NO_ERROR))
    {
        return ERROR_INVALID_PARAMETER;
    }
// eVb: 1.4 [END]
    //
    // No interrupt information is necessary.
    //

    //
    // Check to see if there is a hardware resource conflict.
    //
// eVb: 1.5 [RESOURCE] - Use new function for acquiring VGA resources (I/O, memory)
    if (VgaAcquireResources(hwDeviceExtension) != NO_ERROR) return ERROR_INVALID_PARAMETER;
// eVb: 1.5 [END]
    //
    // Get logical IO port addresses.
    //

    if ((hwDeviceExtension->IOAddress =
         VideoPortGetDeviceBase(hwDeviceExtension,
         VgaAccessRange->RangeStart,
         VGA_MAX_IO_PORT - VGA_BASE_IO_PORT + 1,
         VgaAccessRange->RangeInIoSpace)) == NULL)
    {
        VideoDebugPrint((0, "VgaFindAdapter - Fail to get io address\n"));

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Determine whether a VGA is present.
    //

    if (!VgaIsPresent(hwDeviceExtension)) {

        VideoDebugPrint((0, "VgaFindAdapter - VGA Failed\n"));
        return ERROR_DEV_NOT_EXIST;
    }

    //
    // Minimum size of the buffer required to store the hardware state
    // information returned by IOCTL_VIDEO_SAVE_HARDWARE_STATE.
    //

    ConfigInfo->HardwareStateSize = VGA_TOTAL_STATE_SIZE;

    //
    // Pass a pointer to the emulator range we are using.
    //
// eVb: 1.6 [VDM] - Disable VDM for now
    ConfigInfo->NumEmulatorAccessEntries = 0;
    ConfigInfo->EmulatorAccessEntries = NULL;
    ConfigInfo->EmulatorAccessEntriesContext = 0;
// eVb: 1.6 [END]
    //
    // BUGBUG
    //
    // There is really no reason to have the frame buffer mapped. On an
    // x86 we use if for save/restore (supposedly) but even then we
    // would only need to map a 64K window, not all 16 Meg!
    //

#ifdef _X86_

    //
    // Map the video memory into the system virtual address space so we can
    // clear it out and use it for save and restore.
    //

    if ( (hwDeviceExtension->VideoMemoryAddress =
              VideoPortGetDeviceBase(hwDeviceExtension,
                                     VgaAccessRange[2].RangeStart,
                                     VgaAccessRange[2].RangeLength,
                                     FALSE)) == NULL)
    {
        VideoDebugPrint((0, "VgaFindAdapter - Fail to get memory address\n"));

        return ERROR_INVALID_PARAMETER;
    }

    VideoDebugPrint((0, "vga mapped at %x\n", hwDeviceExtension->VideoMemoryAddress));
#endif
// eVb: 1.7 [VDM] - Disable VDM for now
    ConfigInfo->VdmPhysicalVideoMemoryAddress.QuadPart = 0;
    ConfigInfo->VdmPhysicalVideoMemoryLength = 0;
// eVb: 1.7 [END]
    //
    // Indicate we do not wish to be called again for another initialization.
    //

    *Again = 0;

    //
    // Indicate a successful completion status.
    //

    return NO_ERROR;


} // VgaFindAdapter()

//---------------------------------------------------------------------------
BOOLEAN
NTAPI
VgaInitialize(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine does one time initialization of the device.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    //
    // set up the default cursor position and type.
    //

    hwDeviceExtension->CursorPosition.Column = 0;
    hwDeviceExtension->CursorPosition.Row = 0;
    hwDeviceExtension->CursorTopScanLine = 0;
    hwDeviceExtension->CursorBottomScanLine = 31;
    hwDeviceExtension->CursorEnable = TRUE;

// eVb: 1.8 [VBE] - Initialize VBE modes
    InitializeModeTable(hwDeviceExtension);
// eVb: 1.8 [END]
    return TRUE;

} // VgaInitialize()

//---------------------------------------------------------------------------
BOOLEAN
NTAPI
VgaStartIO(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    )

/*++

Routine Description:

    This routine is the main execution routine for the miniport driver. It
    accepts a Video Request Packet, performs the request, and then returns
    with the appropriate status.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

    RequestPacket - Pointer to the video request packet. This structure
        contains all the parameters passed to the VideoIoControl function.

Return Value:

    This routine will return error codes from the various support routines
    and will also return ERROR_INSUFFICIENT_BUFFER for incorrectly sized
    buffers and ERROR_INVALID_FUNCTION for unsupported functions.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    VP_STATUS status;
    VIDEO_MODE videoMode;
    PVIDEO_MEMORY_INFORMATION memoryInformation;
    ULONG inIoSpace;
    ULONG Result;

    //
    // Switch on the IoContolCode in the RequestPacket. It indicates which
    // function must be performed by the driver.
    //
// eVb: 1.9 [IOCTL] - Remove IOCTLs not needed yet
    switch (RequestPacket->IoControlCode)
    {
    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY:

        VideoDebugPrint((2, "VgaStartIO - ShareVideoMemory\n"));

         status = ERROR_INVALID_FUNCTION;

         break;

    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY:

        VideoDebugPrint((2, "VgaStartIO - UnshareVideoMemory\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_MAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "VgaStartIO - MapVideoMemory\n"));

        if ( (RequestPacket->OutputBufferLength <
              (RequestPacket->StatusBlock->Information =
                                     sizeof(VIDEO_MEMORY_INFORMATION))) ||
             (RequestPacket->InputBufferLength < sizeof(VIDEO_MEMORY)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
        }

        memoryInformation = RequestPacket->OutputBuffer;

        memoryInformation->VideoRamBase = ((PVIDEO_MEMORY)
                (RequestPacket->InputBuffer))->RequestedVirtualAddress;

        //
        // We reserved 16 meg for the frame buffer, however, it makes
        // no sense to map more memory than there is on the card.  So
        // only map the amount of memory we have on the card.
        //
// eVb: 1.10 [CIRRUS] - On VGA, we have VRAM size since boot, use it
        memoryInformation->VideoRamLength =
                hwDeviceExtension->PhysicalVideoMemoryLength;
// eVb: 1.10 [END]
        //
        // If you change to using a dense space frame buffer, make this
        // value a 4 for the ALPHA.
        //

        inIoSpace = 0;

        status = VideoPortMapMemory(hwDeviceExtension,
                                    hwDeviceExtension->PhysicalVideoMemoryBase,
// eVb: 1.11 [CIRRUS] - On VGA, we have VRAM size since boot, use it
                                    &memoryInformation->VideoRamLength,
// eVb: 1.11 [END]
                                    &inIoSpace,
                                    &(memoryInformation->VideoRamBase));

        if (status != NO_ERROR) {
            VideoDebugPrint((0, "VgaStartIO - IOCTL_VIDEO_MAP_VIDEO_MEMORY failed VideoPortMapMemory (%x)\n", status));
        }

        memoryInformation->FrameBufferBase =
            ((PUCHAR) (memoryInformation->VideoRamBase)) +
            hwDeviceExtension->PhysicalFrameOffset.LowPart;

        memoryInformation->FrameBufferLength =
            hwDeviceExtension->PhysicalFrameLength ?
            hwDeviceExtension->PhysicalFrameLength :
            memoryInformation->VideoRamLength;


        VideoDebugPrint((2, "physical VideoMemoryBase %08lx\n", hwDeviceExtension->PhysicalVideoMemoryBase));
        VideoDebugPrint((2, "physical VideoMemoryLength %08lx\n", hwDeviceExtension->PhysicalVideoMemoryLength));
        VideoDebugPrint((2, "VideoMemoryBase %08lx\n", memoryInformation->VideoRamBase));
        VideoDebugPrint((2, "VideoMemoryLength %08lx\n", memoryInformation->VideoRamLength));

        VideoDebugPrint((2, "physical framebuf offset %08lx\n", hwDeviceExtension->PhysicalFrameOffset.LowPart));
        VideoDebugPrint((2, "framebuf base %08lx\n", memoryInformation->FrameBufferBase));
        VideoDebugPrint((2, "physical framebuf len %08lx\n", hwDeviceExtension->PhysicalFrameLength));
        VideoDebugPrint((2, "framebuf length %08lx\n", memoryInformation->FrameBufferLength));

        break;

    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY:

        VideoDebugPrint((2, "VgaStartIO - UnMapVideoMemory\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_QUERY_AVAIL_MODES:

        VideoDebugPrint((2, "VgaStartIO - QueryAvailableModes\n"));

        status = VgaQueryAvailableModes(HwDeviceExtension,
                                        (PVIDEO_MODE_INFORMATION)
                                            RequestPacket->OutputBuffer,
                                        RequestPacket->OutputBufferLength,
                                        &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES:

        VideoDebugPrint((2, "VgaStartIO - QueryNumAvailableModes\n"));

        status = VgaQueryNumberOfAvailableModes(HwDeviceExtension,
                                                (PVIDEO_NUM_MODES)
                                                    RequestPacket->OutputBuffer,
                                                RequestPacket->OutputBufferLength,
                                                &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_QUERY_CURRENT_MODE:

        VideoDebugPrint((2, "VgaStartIO - QueryCurrentMode\n"));

        status = VgaQueryCurrentMode(HwDeviceExtension,
                                     (PVIDEO_MODE_INFORMATION) RequestPacket->OutputBuffer,
                                     RequestPacket->OutputBufferLength,
                                     &RequestPacket->StatusBlock->Information);

        break;


    case IOCTL_VIDEO_SET_CURRENT_MODE:

        VideoDebugPrint((2, "VgaStartIO - SetCurrentModes\n"));

        status = VgaSetMode(HwDeviceExtension,
                              (PVIDEO_MODE) RequestPacket->InputBuffer,
                              RequestPacket->InputBufferLength,
// eVb: 1.12 [SET MODE] - Use new output parameter for framebuffer update functionality
                              &Result);
// eVb: 1.12 [END]

        break;


    case IOCTL_VIDEO_RESET_DEVICE:

        VideoDebugPrint((2, "VgaStartIO - Reset Device\n"));

        videoMode.RequestedMode = 0;

        VgaSetMode(HwDeviceExtension,
                        (PVIDEO_MODE) &videoMode,
                        sizeof(videoMode),
// eVb: 1.13 [SET MODE] - Use new output parameter for framebuffer update functionality
                        &Result);
// eVb: 1.13 [END]

        //
        // Always return succcess since settings the text mode will fail on
        // non-x86.
        //
        // Also, failiure to set the text mode is not fatal in any way, since
        // this operation must be followed by another set mode operation.
        //

        status = NO_ERROR;

        break;


    case IOCTL_VIDEO_LOAD_AND_SET_FONT:

        VideoDebugPrint((2, "VgaStartIO - LoadAndSetFont\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_POSITION:

        VideoDebugPrint((2, "VgaStartIO - QueryCursorPosition\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_SET_CURSOR_POSITION:

        VideoDebugPrint((2, "VgaStartIO - SetCursorPosition\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_QUERY_CURSOR_ATTR:

        VideoDebugPrint((2, "VgaStartIO - QueryCursorAttributes\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_SET_CURSOR_ATTR:

        VideoDebugPrint((2, "VgaStartIO - SetCursorAttributes\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_SET_PALETTE_REGISTERS:

        VideoDebugPrint((2, "VgaStartIO - SetPaletteRegs\n"));

        status = VgaSetPaletteReg(HwDeviceExtension,
                                  (PVIDEO_PALETTE_DATA) RequestPacket->InputBuffer,
                                  RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_SET_COLOR_REGISTERS:

        VideoDebugPrint((2, "VgaStartIO - SetColorRegs\n"));

        status = VgaSetColorLookup(HwDeviceExtension,
                                   (PVIDEO_CLUT) RequestPacket->InputBuffer,
                                   RequestPacket->InputBufferLength);

        break;


    case IOCTL_VIDEO_ENABLE_VDM:

        VideoDebugPrint((2, "VgaStartIO - EnableVDM\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_RESTORE_HARDWARE_STATE:

        VideoDebugPrint((2, "VgaStartIO - RestoreHardwareState\n"));

        status = ERROR_INVALID_FUNCTION;

        break;


    case IOCTL_VIDEO_SAVE_HARDWARE_STATE:

        VideoDebugPrint((2, "VgaStartIO - SaveHardwareState\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    case IOCTL_VIDEO_GET_BANK_SELECT_CODE:

        VideoDebugPrint((2, "VgaStartIO - GetBankSelectCode\n"));

        status = ERROR_INVALID_FUNCTION;
        break;

    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:
        {
            PVIDEO_PUBLIC_ACCESS_RANGES portAccess;
            ULONG physicalPortLength;

            VideoDebugPrint((2, "VgaStartIO - Query Public Address Ranges\n"));

            if (RequestPacket->OutputBufferLength <
                sizeof(VIDEO_PUBLIC_ACCESS_RANGES))
            {
                status = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            RequestPacket->StatusBlock->Information =
                sizeof(VIDEO_PUBLIC_ACCESS_RANGES);

            portAccess = RequestPacket->OutputBuffer;

            //
            // The first public access range is the IO ports.
            //

            portAccess->VirtualAddress  = (PVOID) NULL;
            portAccess->InIoSpace       = TRUE;
            portAccess->MappedInIoSpace = portAccess->InIoSpace;
            physicalPortLength = VGA_MAX_IO_PORT - VGA_BASE_IO_PORT + 1;

            status =  VideoPortMapMemory(hwDeviceExtension,
                                         VgaAccessRange->RangeStart,
                                         &physicalPortLength,
                                         &(portAccess->MappedInIoSpace),
                                         &(portAccess->VirtualAddress));
// eVb: 1.17 [GCG] - Fix lvalue error
            portAccess->VirtualAddress = (PVOID)((ULONG_PTR)portAccess->VirtualAddress - VGA_BASE_IO_PORT);
// eVb: 1.17 [END]
            VideoDebugPrint((2, "VgaStartIO - mapping ports to (%x)\n", portAccess->VirtualAddress));
        }

        break;

    case IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES:

        VideoDebugPrint((2, "VgaStartIO - Free Public Access Ranges\n"));

        status = ERROR_INVALID_FUNCTION;
        break;

    //
    // if we get here, an invalid IoControlCode was specified.
    //

    default:

        VideoDebugPrint((0, "Fell through vga startIO routine - invalid command\n"));

        status = ERROR_INVALID_FUNCTION;

        break;

    }
// eVb: 1.9 [END]
    RequestPacket->StatusBlock->Status = status;

    return TRUE;

} // VgaStartIO()

//---------------------------------------------------------------------------
//
// private routines
//

//---------------------------------------------------------------------------
BOOLEAN
NTAPI
VgaIsPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine returns TRUE if a VGA is present. Determining whether a VGA
    is present is a two-step process. First, this routine walks bits through
    the Bit Mask register, to establish that there are readable indexed
    registers (EGAs normally don't have readable registers, and other adapters
    are unlikely to have indexed registers). This test is done first because
    it's a non-destructive EGA rejection test (correctly rejects EGAs, but
    doesn't potentially mess up the screen or the accessibility of display
    memory). Normally, this would be an adequate test, but some EGAs have
    readable registers, so next, we check for the existence of the Chain4 bit
    in the Memory Mode register; this bit doesn't exist in EGAs. It's
    conceivable that there are EGAs with readable registers and a register bit
    where Chain4 is stored, although I don't know of any; if a better test yet
    is needed, memory could be written to in Chain4 mode, and then examined
    plane by plane in non-Chain4 mode to make sure the Chain4 bit did what it's
    supposed to do. However, the current test should be adequate to eliminate
    just about all EGAs, and 100% of everything else.

    If this function fails to find a VGA, it attempts to undo any damage it
    may have inadvertently done while testing. The underlying assumption for
    the damage control is that if there's any non-VGA adapter at the tested
    ports, it's an EGA or an enhanced EGA, because: a) I don't know of any
    other adapters that use 3C4/5 or 3CE/F, and b), if there are other
    adapters, I certainly don't know how to restore their original states. So
    all error recovery is oriented toward putting an EGA back in a writable
    state, so that error messages are visible. The EGA's state on entry is
    assumed to be text mode, so the Memory Mode register is restored to the
    default state for text mode.

    If a VGA is found, the VGA is returned to its original state after
    testing is finished.

Arguments:

    None.

Return Value:

    TRUE if a VGA is present, FALSE if not.

--*/

{
    UCHAR originalGCAddr;
    UCHAR originalSCAddr;
    UCHAR originalBitMask;
    UCHAR originalReadMap;
    UCHAR originalMemoryMode;
    UCHAR testMask;
    BOOLEAN returnStatus;

    //
    // Remember the original state of the Graphics Controller Address register.
    //

    originalGCAddr = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT);

    //
    // Write the Read Map register with a known state so we can verify
    // that it isn't changed after we fool with the Bit Mask. This ensures
    // that we're dealing with indexed registers, since both the Read Map and
    // the Bit Mask are addressed at GRAPH_DATA_PORT.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);

    //
    // If we can't read back the Graphics Address register setting we just
    // performed, it's not readable and this isn't a VGA.
    //

    if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
        GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_READ_MAP) {

        return FALSE;
    }

    //
    // Set the Read Map register to a known state.
    //

    originalReadMap = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, READ_MAP_TEST_SETTING);

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING) {

        //
        // The Read Map setting we just performed can't be read back; not a
        // VGA. Restore the default Read Map state.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);

        return FALSE;
    }

    //
    // Remember the original setting of the Bit Mask register.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_BIT_MASK) {

        //
        // The Graphics Address register setting we just made can't be read
        // back; not a VGA. Restore the default Read Map state.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, IND_READ_MAP);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);

        return FALSE;
    }

    originalBitMask = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT);

    //
    // Set up the initial test mask we'll write to and read from the Bit Mask.
    //

    testMask = 0xBB;

    do {

        //
        // Write the test mask to the Bit Mask.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, testMask);

        //
        // Make sure the Bit Mask remembered the value.
        //

        if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT) != testMask) {

            //
            // The Bit Mask is not properly writable and readable; not a VGA.
            // Restore the Bit Mask and Read Map to their default states.
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_ADDRESS_PORT, IND_READ_MAP);
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    GRAPH_DATA_PORT, READ_MAP_DEFAULT);

            return FALSE;
        }

        //
        // Cycle the mask for next time.
        //

        testMask >>= 1;

    } while (testMask != 0);

    //
    // There's something readable at GRAPH_DATA_PORT; now switch back and
    // make sure that the Read Map register hasn't changed, to verify that
    // we're dealing with indexed registers.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_READ_MAP);
    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING) {

        //
        // The Read Map is not properly writable and readable; not a VGA.
        // Restore the Bit Mask and Read Map to their default states, in case
        // this is an EGA, so subsequent writes to the screen aren't garbled.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, IND_BIT_MASK);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, BIT_MASK_DEFAULT);

        return FALSE;
    }

    //
    // We've pretty surely verified the existence of the Bit Mask register.
    // Put the Graphics Controller back to the original state.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, originalReadMap);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_DATA_PORT, originalBitMask);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            GRAPH_ADDRESS_PORT, originalGCAddr);

    //
    // Now, check for the existence of the Chain4 bit.
    //

    //
    // Remember the original states of the Sequencer Address and Memory Mode
    // registers.
    //

    originalSCAddr = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT) & SEQ_ADDR_MASK) != IND_MEMORY_MODE) {

        //
        // Couldn't read back the Sequencer Address register setting we just
        // performed.
        //

        return FALSE;
    }
    originalMemoryMode = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT);

    //
    // Toggle the Chain4 bit and read back the result. This must be done during
    // sync reset, since we're changing the chaining state.
    //

    //
    // Begin sync reset.
    //

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
             SEQ_ADDRESS_PORT),
             (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    //
    // Toggle the Chain4 bit.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT, (UCHAR)(originalMemoryMode ^ CHAIN4_MASK));

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT) != (UCHAR) (originalMemoryMode ^ CHAIN4_MASK)) {

        //
        // Chain4 bit not there; not a VGA.
        // Set text mode default for Memory Mode register.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, MEMORY_MODE_TEXT_DEFAULT);
        //
        // End sync reset.
        //

        VideoPortWritePortUshort((PUSHORT) (HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT),
                (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        returnStatus = FALSE;

    } else {

        //
        // It's a VGA.
        //

        //
        // Restore the original Memory Mode setting.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_DATA_PORT, originalMemoryMode);

        //
        // End sync reset.
        //

        VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT),
                (USHORT)(IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        //
        // Restore the original Sequencer Address setting.
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                SEQ_ADDRESS_PORT, originalSCAddr);

        returnStatus = TRUE;
    }

    return returnStatus;

} // VgaIsPresent()

//---------------------------------------------------------------------------
VP_STATUS
NTAPI
VgaSetPaletteReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_PALETTE_DATA PaletteBuffer,
    ULONG PaletteBufferSize
    )

/*++

Routine Description:

    This routine sets a specified portion of the EGA (not DAC) palette
    registers.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    PaletteBuffer - Pointer to the structure containing the palette data.

    PaletteBufferSize - Length of the input buffer supplied by the user.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data.

    ERROR_INVALID_PARAMETER - invalid palette size.

--*/

{
    USHORT i;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ((PaletteBufferSize) < (sizeof(VIDEO_PALETTE_DATA)) ||
        (PaletteBufferSize < (sizeof(VIDEO_PALETTE_DATA) +
                (sizeof(USHORT) * (PaletteBuffer->NumEntries -1)) ))) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (PaletteBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER ) ||
         (PaletteBuffer->NumEntries == 0) ||
         (PaletteBuffer->FirstEntry + PaletteBuffer->NumEntries >
             VIDEO_MAX_PALETTE_REGISTER + 1 ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Reset ATC to index mode
    //

    VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                           ATT_INITIALIZE_PORT_COLOR);

    //
    // Blast out our palette values.
    //

    for (i = 0; i < PaletteBuffer->NumEntries; i++) {

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress + ATT_ADDRESS_PORT,
                                (UCHAR)(i+PaletteBuffer->FirstEntry));

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    ATT_DATA_WRITE_PORT,
                                (UCHAR)PaletteBuffer->Colors[i]);
    }

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + ATT_ADDRESS_PORT,
                            VIDEO_ENABLE);

    return NO_ERROR;

} // end VgaSetPaletteReg()

//---------------------------------------------------------------------------
VP_STATUS
NTAPI
VgaSetColorLookup(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_CLUT ClutBuffer,
    ULONG ClutBufferSize
    )

/*++

Routine Description:

    This routine sets a specified portion of the DAC color lookup table
    settings.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ClutBufferSize - Length of the input buffer supplied by the user.

    ClutBuffer - Pointer to the structure containing the color lookup table.

Return Value:

    NO_ERROR - information returned successfully

    ERROR_INSUFFICIENT_BUFFER - input buffer not large enough for input data.

    ERROR_INVALID_PARAMETER - invalid clut size.

--*/

{
    PVIDEOMODE CurrentMode = HwDeviceExtension->CurrentMode;
    USHORT i;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if ( (ClutBufferSize < sizeof(VIDEO_CLUT) - sizeof(ULONG)) ||
         (ClutBufferSize < sizeof(VIDEO_CLUT) +
                     (sizeof(ULONG) * (ClutBuffer->NumEntries - 1)) ) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Check to see if the parameters are valid.
    //

    if ( (ClutBuffer->NumEntries == 0) ||
         (ClutBuffer->FirstEntry > VIDEO_MAX_COLOR_REGISTER) ||
         (ClutBuffer->FirstEntry + ClutBuffer->NumEntries >
             VIDEO_MAX_COLOR_REGISTER + 1) ) {

        return ERROR_INVALID_PARAMETER;

    }
// eVb: 1.14 [VBE] - Add VBE color support
    //
    // Check SVGA mode
    //

    if (CurrentMode->bitsPerPlane >= 8) return VbeSetColorLookup(HwDeviceExtension, ClutBuffer);
// eVb: 1.14 [END]
    //
    // Path for VGA mode
    //
// eVb: 1.15 [VBE] - Add VBE support for non-VGA-compatible detected modes
    if (!CurrentMode->NonVgaMode)
    {
// eVb: 1.15 [END]
        //
        //  Set CLUT registers directly on the hardware
        //

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                DAC_ADDRESS_WRITE_PORT, (UCHAR) ClutBuffer->FirstEntry);

        for (i = 0; i < ClutBuffer->NumEntries; i++) {
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_ADDRESS_WRITE_PORT,
                                    (UCHAR)(i + ClutBuffer->FirstEntry));

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_DATA_REG_PORT,
                                    ClutBuffer->LookupTable[i].RgbArray.Red);

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_DATA_REG_PORT,
                                    ClutBuffer->LookupTable[i].RgbArray.Green);

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    DAC_DATA_REG_PORT,
                                    ClutBuffer->LookupTable[i].RgbArray.Blue);
        }
        return NO_ERROR;
    }

    return ERROR_INVALID_PARAMETER;

} // end VgaSetColorLookup()

VP_STATUS
NTAPI
GetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    )

/*++

Routine Description:

    Callback routine for the VideoPortGetDeviceData function.

Arguments:

    HwDeviceExtension - Pointer to the miniport drivers device extension.

    Context - Context value passed to the VideoPortGetDeviceData function.

    DeviceDataType - The type of data that was requested in
        VideoPortGetDeviceData.

    Identifier - Pointer to a string that contains the name of the device,
        as setup by the ROM or ntdetect.

    IdentifierLength - Length of the Identifier string.

    ConfigurationData - Pointer to the configuration data for the device or
        BUS.

    ConfigurationDataLength - Length of the data in the configurationData
        field.

    ComponentInformation - Undefined.

    ComponentInformationLength - Undefined.

Return Value:

    Returns NO_ERROR if the function completed properly.
    Returns ERROR_DEV_NOT_EXIST if we did not find the device.
    Returns ERROR_INVALID_PARAMETER otherwise.

--*/

{
    VideoDebugPrint((Error, "Detected internal VGA chip on embedded board, todo\n"));
    while (TRUE);
    return NO_ERROR;

} //end GetDeviceDataCallback()

// eVb: 1.16 [RESOURCE] - Add new function for acquiring VGA resources (I/O, memory)
VP_STATUS
NTAPI
VgaAcquireResources(
    PHW_DEVICE_EXTENSION DeviceExtension
    )
{
    VP_STATUS Status = NO_ERROR;
    ULONG Ranges, i;

    //
    // Try exclusive ranges (vga + ati)
    //

    Ranges = NUM_VGA_ACCESS_RANGES;
    for (i = 0; i < Ranges; i++) VgaAccessRange[i].RangeShareable = FALSE;
    if (VideoPortVerifyAccessRanges(DeviceExtension, Ranges, VgaAccessRange) != NO_ERROR)
    {
        //
        // Not worked, try vga only
        //

        Ranges = 3;
        if (VideoPortVerifyAccessRanges(DeviceExtension, Ranges, VgaAccessRange) != NO_ERROR)
        {
            //
            // Still not, try shared ranges
            //

            for (i = 0; i < Ranges; i++) VgaAccessRange[i].RangeShareable = TRUE;
            Status = VideoPortVerifyAccessRanges(DeviceExtension, Ranges, VgaAccessRange);
            if (Status == NO_ERROR)
            {
                //
                // It did work
                //

                VideoPortVerifyAccessRanges(DeviceExtension, 0, 0);
                Status = NO_ERROR;
            }
        }
    }

    if (Status == NO_ERROR)
    {
        //
        // Worked with exclusive, also try shared
        //

        for (i = 0; i < Ranges; i++) VgaAccessRange[i].RangeShareable = TRUE;
        Status = VideoPortVerifyAccessRanges(DeviceExtension, Ranges, VgaAccessRange);
    }

    return Status;
}
// eVb: 1.16 [END]
