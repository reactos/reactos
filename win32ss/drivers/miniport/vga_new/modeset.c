/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/miniport/vga_new/modeset.c
 * PURPOSE:         Handles switching to Standard VGA Modes for compatible cards
 * PROGRAMMERS:     Copyright (c) 1992  Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */

#include "vga.h"

VP_STATUS
NTAPI
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
NTAPI
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize,
// eVb: 2.1 [SET MODE] - Add new output parameter for framebuffer update functionality
    PULONG PhysPtrChange
// eVb: 2.1 [END]
    );

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

VOID
NTAPI
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaInterpretCmdStream)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#endif

//---------------------------------------------------------------------------
VP_STATUS
NTAPI
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    )

/*++

Routine Description:

    Interprets the appropriate command array to set up VGA registers for the
    requested mode. Typically used to set the VGA into a particular mode by
    programming all of the registers

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    pusCmdStream - array of commands to be interpreted.

Return Value:

    The status of the operation (can only fail on a bad command); TRUE for
    success, FALSE for failure.

--*/

{
    ULONG ulCmd;
    ULONG ulPort;
    UCHAR jValue;
    USHORT usValue;
    ULONG culCount;
    ULONG ulIndex;
    ULONG ulBase;

    if (pusCmdStream == NULL) {

        VideoDebugPrint((1, "VgaInterpretCmdStream - Invalid pusCmdStream\n"));
        return TRUE;
    }

    ulBase = (ULONG)HwDeviceExtension->IOAddress;

    //
    // Now set the adapter to the desired mode.
    //

    while ((ulCmd = *pusCmdStream++) != EOD) {

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

            //
            // Basic input/output command
            //

            case INOUT:

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)(ulBase+ulPort),
                                    jValue);

                        } else {

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            VideoPortWritePortUshort((PUSHORT)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // String byte outs. Do in a loop; can't use
                            // VideoPortWritePortBufferUchar because the data
                            // is in USHORT form
                            //

                            ulPort = ulBase + *pusCmdStream++;
                            culCount = *pusCmdStream++;

                            while (culCount--) {
                                jValue = (UCHAR) *pusCmdStream++;
                                VideoPortWritePortUchar((PUCHAR)ulPort,
                                        jValue);

                            }

                        } else {

                            //
                            // String word outs
                            //

                            ulPort = *pusCmdStream++;
                            culCount = *pusCmdStream++;
                            VideoPortWritePortBufferUshort((PUSHORT)
                                    (ulBase + ulPort), pusCmdStream, culCount);
                            pusCmdStream += culCount;

                        }
                    }

                } else {

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);

                    } else {

                        //
                        // Single word in
                        //

                        ulPort = *pusCmdStream++;
                        usValue = VideoPortReadPortUshort((PUSHORT)
                                (ulBase+ulPort));

                    }

                }

                break;

            //
            // Higher-level input/output commands
            //

            case METAOUT:

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            usValue = (USHORT) (ulIndex +
                                      (((ULONG)(*pusCmdStream++)) << 8));
                            VideoPortWritePortUshort((PUSHORT)ulPort, usValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // Masked out (read, AND, XOR, write)
                    //

                    case MASKOUT:

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);
                        jValue &= *pusCmdStream++;
                        jValue ^= *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)ulBase + ulPort,
                                jValue);
                        break;

                    //
                    // Attribute Controller out
                    //

                    case ATCOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            // Write Attribute Controller index
                            VideoPortWritePortUchar((PUCHAR)ulPort,
                                    (UCHAR)ulIndex);

                            // Write Attribute Controller data
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)ulPort, jValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // None of the above; error
                    //
                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:

                break;

            //
            // Unknown command; error
            //

            default:

                return FALSE;

        }

    }

    return TRUE;

} // end VgaInterpretCmdStream()

VP_STATUS
NTAPI
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize,
// eVb: 2.2 [SET MODE] - Add new output parameter for framebuffer update functionality
    PULONG PhysPtrChange
// eVb: 2.2 [END]
    )

/*++

Routine Description:

    This routine sets the vga into the requested mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    Mode - Pointer to the structure containing the information about the
        font to be set.

    ModeSize - Length of the input buffer supplied by the user.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    ERROR_INVALID_PARAMETER if the mode number is invalid.

    NO_ERROR if the operation completed successfully.

--*/

{
    PVIDEOMODE pRequestedMode;
    VP_STATUS status;
    ULONG RequestedModeNum;
// eVb: 2.3 [SET MODE] - Add new output parameter for framebuffer update functionality
    *PhysPtrChange = FALSE;
// eVb: 2.3 [END]
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Extract the clear memory, and map linear bits.
    //

    RequestedModeNum = Mode->RequestedMode &
        ~(VIDEO_MODE_NO_ZERO_MEMORY | VIDEO_MODE_MAP_MEM_LINEAR);


    if (!(Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY))
    {
#if defined(_X86_)
        VgaZeroVideoMemory(HwDeviceExtension);
#endif
    }

    //
    // Check to see if we are requesting a valid mode
    //
// eVb: 2.4 [CIRRUS] - Remove Cirrus-specific check for valid mode
    if ( (RequestedModeNum >= NumVideoModes) )
// eVb: 2.4 [END]
    {
        VideoDebugPrint((0, "Invalide Mode Number = %d!\n", RequestedModeNum));

        return ERROR_INVALID_PARAMETER;
    }

    VideoDebugPrint((2, "Attempting to set mode %d\n",
                        RequestedModeNum));
// eVb: 2.5 [VBE] - Use dynamic VBE mode list instead of hard-coded VGA list
    pRequestedMode = &VgaModeList[RequestedModeNum];
// eVb: 2.5 [END]
    VideoDebugPrint((2, "Info on Requested Mode:\n"
                        "\tResolution: %dx%d\n",
                        pRequestedMode->hres,
                        pRequestedMode->vres ));

    //
    // VESA BIOS mode switch
    //
// eVb: 2.6 [VBE] - VBE Mode Switch Support 
    status = VbeSetMode(HwDeviceExtension, pRequestedMode, PhysPtrChange);
    if (status == ERROR_INVALID_FUNCTION)
    {
        //
        // VGA mode switch
        //

        if (!pRequestedMode->CmdStream) return ERROR_INVALID_FUNCTION;
        if (!VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStream)) return ERROR_INVALID_FUNCTION;
        goto Cleanup;
    }
    else if (status != NO_ERROR) return status;
// eVb: 2.6 [END]
// eVb: 2.7 [MODE-X] - Windows VGA Miniport Supports Mode-X, we should too
    //
    // ModeX check
    //
    
    if (pRequestedMode->hres == 320)
    {
        VideoDebugPrint((0, "ModeX not support!!!\n"));
        return ERROR_INVALID_PARAMETER;
    }
// eVb: 2.7 [END]
    //
    // Text mode check
    //
    
    if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS))
    {
// eVb: 2.8 [TODO] - This code path is not implemented yet
        VideoDebugPrint((0, "Text-mode not support!!!\n"));
        return ERROR_INVALID_PARAMETER;
// eVb: 2.8 [END]
    }

Cleanup:
    //
    // Update the location of the physical frame buffer within video memory.
    //
// eVb: 2.9 [VBE] - Linear and banked support is unified in VGA, unlike Cirrus
    HwDeviceExtension->PhysicalVideoMemoryBase.LowPart = pRequestedMode->PhysBase;
    HwDeviceExtension->PhysicalVideoMemoryLength = pRequestedMode->PhysSize;

    HwDeviceExtension->PhysicalFrameLength =
            pRequestedMode->FrameBufferSize;

    HwDeviceExtension->PhysicalFrameOffset.LowPart =
            pRequestedMode->FrameBufferBase;
// eVb: 2.9 [END]

    //
    // Store the new mode value.
    //

    HwDeviceExtension->CurrentMode = pRequestedMode;
    HwDeviceExtension->ModeIndex = Mode->RequestedMode;

    return NO_ERROR;

} //end VgaSetMode()

VP_STATUS
NTAPI
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the list of all available available modes on the
    card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the list of all valid modes is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    PVIDEO_MODE_INFORMATION videoModes = ModeInformation;
    ULONG i;

    //
    // Find out the size of the data to be put in the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize =
// eVb: 2.10 [VBE] - We store VBE/VGA mode count in this global, not in DevExt like Cirrus
            NumVideoModes *
// eVb: 2.10 [END]
            sizeof(VIDEO_MODE_INFORMATION)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //

    for (i = 0; i < NumVideoModes; i++)
    {
        videoModes->Length = sizeof(VIDEO_MODE_INFORMATION);
        videoModes->ModeIndex  = i;
// eVb: 2.11 [VBE] - Use dynamic VBE mode list instead of hard-coded VGA list
        videoModes->VisScreenWidth = VgaModeList[i].hres;
        videoModes->ScreenStride = VgaModeList[i].wbytes;
        videoModes->VisScreenHeight = VgaModeList[i].vres;
        videoModes->NumberOfPlanes = VgaModeList[i].numPlanes;
        videoModes->BitsPerPlane = VgaModeList[i].bitsPerPlane;
        videoModes->Frequency = VgaModeList[i].Frequency;
        videoModes->XMillimeter = 320;        // temporary hardcoded constant
        videoModes->YMillimeter = 240;        // temporary hardcoded constant
        videoModes->AttributeFlags = VgaModeList[i].fbType;
// eVb: 2.11 [END]

        if ((VgaModeList[i].bitsPerPlane == 32) ||
            (VgaModeList[i].bitsPerPlane == 24))
        {

            videoModes->NumberRedBits = 8;
            videoModes->NumberGreenBits = 8;
            videoModes->NumberBlueBits = 8;
            videoModes->RedMask = 0xff0000;
            videoModes->GreenMask = 0x00ff00;
            videoModes->BlueMask = 0x0000ff;

        }
        else if (VgaModeList[i].bitsPerPlane == 16)
        {

            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;
            videoModes->RedMask = 0x1F << 11;
            videoModes->GreenMask = 0x3F << 5;
            videoModes->BlueMask = 0x1F;

        }
// eVb: 2.12 [VGA] - Add support for 15bpp modes, which Cirrus doesn't support
        else if (VgaModeList[i].bitsPerPlane == 15)
        {

            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;
            videoModes->RedMask = 0x3E << 9;
            videoModes->GreenMask = 0x1F << 5;
            videoModes->BlueMask = 0x1F;
        }
// eVb: 2.12 [END]
        else
        {

            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;
            videoModes->RedMask = 0;
            videoModes->GreenMask = 0;
            videoModes->BlueMask = 0;
        }

// eVb: 2.13 [VGA] - All modes are palette managed/driven, unlike Cirrus
        videoModes->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
             VIDEO_MODE_MANAGED_PALETTE;
// eVb: 2.13 [END]
        videoModes++;

    }

    return NO_ERROR;

} // end VgaGetAvailableModes()

VP_STATUS
NTAPI
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the number of available modes for this particular
    video card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    NumModes - Pointer to the output buffer supplied by the user. This is
        where the number of modes is stored.

    NumModesSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (NumModesSize < (*OutputSize = sizeof(VIDEO_NUM_MODES)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the number of modes into the buffer.
    //

// eVb: 2.14 [VBE] - We store VBE/VGA mode count in this global, not in DevExt like Cirrus
    NumModes->NumModes = NumVideoModes;
// eVb: 2.14 [END]
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    return NO_ERROR;

} // end VgaGetNumberOfAvailableModes()

VP_STATUS
NTAPI
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns a description of the current video mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the current mode information is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL ) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize = sizeof(VIDEO_MODE_INFORMATION))) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the characteristics of the current mode into the buffer.
    //

    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex = HwDeviceExtension->ModeIndex;
    ModeInformation->VisScreenWidth = HwDeviceExtension->CurrentMode->hres;
    ModeInformation->ScreenStride = HwDeviceExtension->CurrentMode->wbytes;
    ModeInformation->VisScreenHeight = HwDeviceExtension->CurrentMode->vres;
    ModeInformation->NumberOfPlanes = HwDeviceExtension->CurrentMode->numPlanes;
    ModeInformation->BitsPerPlane = HwDeviceExtension->CurrentMode->bitsPerPlane;
    ModeInformation->Frequency = HwDeviceExtension->CurrentMode->Frequency;
    ModeInformation->XMillimeter = 320;        // temporary hardcoded constant
    ModeInformation->YMillimeter = 240;        // temporary hardcoded constant

    ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType;

    if ((ModeInformation->BitsPerPlane == 32) ||
        (ModeInformation->BitsPerPlane == 24))
    {

        ModeInformation->NumberRedBits = 8;
        ModeInformation->NumberGreenBits = 8;
        ModeInformation->NumberBlueBits = 8;
        ModeInformation->RedMask = 0xff0000;
        ModeInformation->GreenMask = 0x00ff00;
        ModeInformation->BlueMask = 0x0000ff;

    }
    else if (ModeInformation->BitsPerPlane == 16)
    {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0x1F << 11;
        ModeInformation->GreenMask = 0x3F << 5;
        ModeInformation->BlueMask = 0x1F;

    }
// eVb: 2.12 [VGA] - Add support for 15bpp modes, which Cirrus doesn't support
    else if (ModeInformation->BitsPerPlane == 15)
    {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0x3E << 9;
        ModeInformation->GreenMask = 0x1F << 5;
        ModeInformation->BlueMask = 0x1F;
    }
// eVb: 2.12 [END]
    else
    {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0;
        ModeInformation->GreenMask = 0;
        ModeInformation->BlueMask = 0;
    }

// eVb: 2.13 [VGA] - All modes are palette managed/driven, unlike Cirrus
    ModeInformation->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
         VIDEO_MODE_MANAGED_PALETTE;
// eVb: 2.13 [END]

    return NO_ERROR;

} // end VgaQueryCurrentMode()

VOID
NTAPI
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine zeros the first 256K on the VGA.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.


Return Value:

    None.

--*/
{
    UCHAR temp;

    //
    // Map font buffer at A0000
    //

    VgaInterpretCmdStream(HwDeviceExtension, EnableA000Data);

    //
    // Enable all planes.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
            IND_MAP_MASK);

    temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT) | (UCHAR)0x0F;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
            temp);

    VideoPortZeroDeviceMemory(HwDeviceExtension->VideoMemoryAddress, 0xFFFF);

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

}
