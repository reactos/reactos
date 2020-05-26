#include "precomp.h"

/* PRIVATE FUNCTIONS *********************************************************/

static BOOLEAN
NTAPI
VgaInterpretCmdStream(
    _In_ PUSHORT CmdStream)
{
    USHORT Cmd;
    UCHAR Major, Minor;
    USHORT Port;
    USHORT Count;
    UCHAR Index;
    UCHAR Value;
    USHORT ShortValue;

    /* First make sure that we have a Command Stream */
    if (!CmdStream) return TRUE;

    /* Loop as long as we have commands */
    while (*CmdStream != EOD)
    {
        /* Get the next command and its Major and Minor functions */
        Cmd = *CmdStream++;
        Major = Cmd & 0xF0;
        Minor = Cmd & 0x0F;

        /* Check which major function this is */
        if (Major == INOUT)
        {
            /* Check the minor function */
            if (Minor & IO /* CMD_STREAM_READ */)
            {
                /* Check the sub-type */
                if (Minor & BW /* CMD_STREAM_USHORT */)
                {
                    /* Get the port and read an USHORT from it */
                    Port = *CmdStream++;
                    ShortValue = __inpw(Port);
                }
                else // if (Minor & CMD_STREAM_WRITE)
                {
                    /* Get the port and read an UCHAR from it */
                    Port = *CmdStream++;
                    Value = __inpb(Port);
                }
            }
            else if (Minor & MULTI /* CMD_STREAM_WRITE_ARRAY */)
            {
                /* Check the sub-type */
                if (Minor & BW /* CMD_STREAM_USHORT */)
                {
                    /* Get the port and the count of elements */
                    Port = *CmdStream++;
                    Count = *CmdStream++;

                    /* Write the USHORT to the port; the buffer is what's in the command stream */
                    WRITE_PORT_BUFFER_USHORT((PUSHORT)(VgaRegisterBase + Port), CmdStream, Count);

                    /* Move past the buffer in the command stream */
                    CmdStream += Count;
                }
                else // if (Minor & CMD_STREAM_WRITE)
                {
                    /* Get the port and the count of elements */
                    Port = *CmdStream++;
                    Count = *CmdStream++;

                    /* Loop the command array */
                    for (; Count; --Count, ++CmdStream)
                    {
                        /* Get the UCHAR and write it to the port */
                        Value = (UCHAR)*CmdStream;
                        __outpb(Port, Value);
                    }
                }
            }
            else if (Minor & BW /* CMD_STREAM_USHORT */)
            {
                /* Get the port */
                Port = *CmdStream++;

                /* Get the USHORT and write it to the port */
                ShortValue = *CmdStream++;
                __outpw(Port, ShortValue);
            }
            else // if (Minor & CMD_STREAM_WRITE)
            {
                /* Get the port */
                Port = *CmdStream++;

                /* Get the UCHAR and write it to the port */
                Value = (UCHAR)*CmdStream++;
                __outpb(Port, Value);
            }
        }
        else if (Major == METAOUT)
        {
            /* Check the minor function. Note these are not flags. */
            switch (Minor)
            {
                case INDXOUT:
                {
                    /* Get the port, the count of elements and the start index */
                    Port = *CmdStream++;
                    Count = *CmdStream++;
                    Index = (UCHAR)*CmdStream++;

                    /* Loop the command array */
                    for (; Count; --Count, ++Index, ++CmdStream)
                    {
                        /* Get the USHORT and write it to the port */
                        ShortValue = (USHORT)Index + ((*CmdStream) << 8);
                        __outpw(Port, ShortValue);
                    }
                    break;
                }

                case ATCOUT:
                {
                    /* Get the port, the count of elements and the start index */
                    Port = *CmdStream++;
                    Count = *CmdStream++;
                    Index = (UCHAR)*CmdStream++;

                    /* Loop the command array */
                    for (; Count; --Count, ++Index, ++CmdStream)
                    {
                        /* Write the index */
                        __outpb(Port, Index);

                        /* Get the UCHAR and write it to the port */
                        Value = (UCHAR)*CmdStream;
                        __outpb(Port, Value);
                    }
                    break;
                }

                case MASKOUT:
                {
                    /* Get the port */
                    Port = *CmdStream++;

                    /* Read the current value and add the stream data */
                    Value = __inpb(Port);
                    Value &= *CmdStream++;
                    Value ^= *CmdStream++;

                    /* Write the value */
                    __outpb(Port, Value);
                    break;
                }

                default:
                    /* Unknown command, fail */
                    return FALSE;
            }
        }
        else if (Major != NCMD)
        {
            /* Unknown major function, fail */
            return FALSE;
        }
    }

    /* If we got here, return success */
    return TRUE;
}

static BOOLEAN
NTAPI
VgaIsPresent(VOID)
{
    UCHAR OrgGCAddr, OrgReadMap, OrgBitMask;
    UCHAR OrgSCAddr, OrgMemMode;
    UCHAR i;

    /* Remember the original state of the Graphics Controller Address register */
    OrgGCAddr = __inpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT);

    /*
     * Write the Read Map register with a known state so we can verify
     * that it isn't changed after we fool with the Bit Mask. This ensures
     * that we're dealing with indexed registers, since both the Read Map and
     * the Bit Mask are addressed at GRAPH_DATA_PORT.
     */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);

    /*
     * If we can't read back the Graphics Address register setting we just
     * performed, it's not readable and this isn't a VGA.
     */
    if ((__inpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_READ_MAP)
        return FALSE;

    /*
     * Set the Read Map register to a known state.
     */
    OrgReadMap = __inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_TEST_SETTING);

    /* Read it back... it should be the same */
    if (__inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING)
    {
        /*
         * The Read Map setting we just performed can't be read back; not a
         * VGA. Restore the default Read Map state and fail.
         */
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        return FALSE;
    }

    /* Remember the original setting of the Bit Mask register */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);

    /* Read it back... it should be the same */
    if ((__inpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT) & GRAPH_ADDR_MASK) != IND_BIT_MASK)
    {
        /*
         * The Graphics Address register setting we just made can't be read
         * back; not a VGA. Restore the default Read Map state and fail.
         */
        __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        return FALSE;
    }

    /* Read the VGA Data Register */
    OrgBitMask = __inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT);

    /*
     * Set up the initial test mask we'll write to and read from the Bit Mask,
     * and loop on the bitmasks.
     */
    for (i = 0xBB; i; i >>= 1)
    {
        /* Write the test mask to the Bit Mask */
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, i);

        /* Read it back... it should be the same */
        if (__inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) != i)
        {
            /*
             * The Bit Mask is not properly writable and readable; not a VGA.
             * Restore the Bit Mask and Read Map to their default states and fail.
             */
            __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
            __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);
            __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
            return FALSE;
        }
    }

    /*
     * There's something readable at GRAPH_DATA_PORT; now switch back and
     * make sure that the Read Map register hasn't changed, to verify that
     * we're dealing with indexed registers.
     */
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_READ_MAP);

    /* Read it back */
    if (__inpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT) != READ_MAP_TEST_SETTING)
    {
        /*
         * The Read Map is not properly writable and readable; not a VGA.
         * Restore the Bit Mask and Read Map to their default states, in case
         * this is an EGA, so subsequent writes to the screen aren't garbled.
         * Then fail.
         */
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, READ_MAP_DEFAULT);
        __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
        __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, BIT_MASK_DEFAULT);
        return FALSE;
    }

    /*
     * We've pretty surely verified the existence of the Bit Mask register.
     * Put the Graphics Controller back to the original state.
     */
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, OrgReadMap);
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, IND_BIT_MASK);
    __outpb(VGA_BASE_IO_PORT + GRAPH_DATA_PORT, OrgBitMask);
    __outpb(VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT, OrgGCAddr);

    /*
     * Now, check for the existence of the Chain4 bit.
     */

    /*
     * Remember the original states of the Sequencer Address and Memory Mode
     * registers.
     */
    OrgSCAddr = __inpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT);
    __outpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, IND_MEMORY_MODE);

    /* Read it back... it should be the same */
    if ((__inpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT) & SEQ_ADDR_MASK) != IND_MEMORY_MODE)
    {
        /*
         * Couldn't read back the Sequencer Address register setting
         * we just performed, fail.
         */
        return FALSE;
    }

    /* Read sequencer Data */
    OrgMemMode = __inpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT);

    /*
     * Toggle the Chain4 bit and read back the result. This must be done during
     * sync reset, since we're changing the chaining state.
     */

    /* Begin sync reset */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (IND_SYNC_RESET + (START_SYNC_RESET_VALUE << 8)));

    /* Toggle the Chain4 bit */
    __outpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, IND_MEMORY_MODE);
    __outpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT, OrgMemMode ^ CHAIN4_MASK);

    /* Read it back... it should be the same */
    if (__inpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT) != (OrgMemMode ^ CHAIN4_MASK))
    {
        /*
         * Chain4 bit is not there, not a VGA.
         * Set text mode default for Memory Mode register.
         */
        __outpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT, MEMORY_MODE_TEXT_DEFAULT);

        /* End sync reset */
        __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

        /* Fail */
        return FALSE;
    }

    /*
     * It's a VGA.
     */

    /* Restore the original Memory Mode setting */
    __outpb(VGA_BASE_IO_PORT + SEQ_DATA_PORT, OrgMemMode);

    /* End sync reset */
    __outpw(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, (IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8)));

    /* Restore the original Sequencer Address setting */
    __outpb(VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, OrgSCAddr);

    /* VGA is present! */
    return TRUE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
VidInitialize(
    _In_ BOOLEAN SetMode)
{
    ULONG_PTR Context = 0;
    PHYSICAL_ADDRESS TranslatedAddress;
    PHYSICAL_ADDRESS NullAddress = {{0, 0}}, VgaAddress;
    ULONG AddressSpace;
    BOOLEAN Result;
    ULONG_PTR Base;

    /* Make sure that we have a bus translation function */
    if (!HalFindBusAddressTranslation) return FALSE;

    /* Loop trying to find possible VGA base addresses */
    while (TRUE)
    {
        /* Get the VGA Register address */
        AddressSpace = 1;
        Result = HalFindBusAddressTranslation(NullAddress,
                                              &AddressSpace,
                                              &TranslatedAddress,
                                              &Context,
                                              TRUE);
        if (!Result) return FALSE;

        /* See if this is I/O Space, which we need to map */
        if (!AddressSpace)
        {
            /* Map it */
            Base = (ULONG_PTR)MmMapIoSpace(TranslatedAddress, 0x400, MmNonCached);
        }
        else
        {
            /* The base is the translated address, no need to map I/O space */
            Base = TranslatedAddress.LowPart;
        }

        /* Try to see if this is VGA */
        VgaRegisterBase = Base;
        if (VgaIsPresent())
        {
            /* Translate the VGA Memory Address */
            VgaAddress.LowPart = MEM_VGA;
            VgaAddress.HighPart = 0;
            AddressSpace = 0;
            Result = HalFindBusAddressTranslation(VgaAddress,
                                                  &AddressSpace,
                                                  &TranslatedAddress,
                                                  &Context,
                                                  FALSE);
            if (Result) break;
        }
        else
        {
            /* It's not, so unmap the I/O space if we mapped it */
            if (!AddressSpace) MmUnmapIoSpace((PVOID)VgaRegisterBase, 0x400);
        }

        /* Continue trying to see if there's any other address */
    }

    /* Success! See if this is I/O Space, which we need to map */
    if (!AddressSpace)
    {
        /* Map it */
        Base = (ULONG_PTR)MmMapIoSpace(TranslatedAddress,
                                       MEM_VGA_SIZE,
                                       MmNonCached);
    }
    else
    {
        /* The base is the translated address, no need to map I/O space */
        Base = TranslatedAddress.LowPart;
    }

    /* Set the VGA Memory Base */
    VgaBase = Base;

    /* Now check if we have to set the mode */
    if (SetMode)
    {
        /* Clear the current position */
        VidpCurrentX = 0;
        VidpCurrentY = 0;

        /* Reset the display and initialize it */
        if (HalResetDisplay())
        {
            /* The HAL handled the display, re-initialize only the AC registers */
            VgaInterpretCmdStream(AT_Initialization);
        }
        else
        {
            /* The HAL didn't handle the display, fully re-initialize the VGA */
            VgaInterpretCmdStream(VGA_640x480);
        }
    }

    /* VGA is ready */
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
VidResetDisplay(
    _In_ BOOLEAN HalReset)
{
    /* Clear the current position */
    VidpCurrentX = 0;
    VidpCurrentY = 0;

    /* Clear the screen with HAL if we were asked to */
    if (HalReset)
    {
        if (!HalResetDisplay())
        {
            /* The HAL didn't handle the display, fully re-initialize the VGA */
            VgaInterpretCmdStream(VGA_640x480);
        }
    }

    /* Always re-initialize the AC registers */
    VgaInterpretCmdStream(AT_Initialization);

    /* Re-initialize the palette and fill the screen black */
    InitializePalette();
    VidSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLACK);
}
