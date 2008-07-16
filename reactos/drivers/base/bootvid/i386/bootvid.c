#include "precomp.h"

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
VgaInterpretCmdStream(IN PUSHORT CmdStream)
{
    PUCHAR Base = (PUCHAR)VgaRegisterBase;
    USHORT Cmd;
    UCHAR Major, Minor;
    USHORT Count;
    UCHAR Index;
    PUSHORT Buffer;
    PUSHORT ShortPort;
    PUCHAR Port;
    UCHAR Value;
    USHORT ShortValue;

    /* First make sure that we have a Command Stream */
    if (!CmdStream) return TRUE;

    /* Loop as long as we have commands */
    while (*CmdStream)
    {
        /* Get the Major and Minor Function */
        Cmd = *CmdStream;
        Major = Cmd & 0xF0;
        Minor = Cmd & 0x0F;

        /* Move to the next command */
        CmdStream++;

        /* Check which major function this was */
        if (Major == 0x10)
        {
            /* Now let's see the minor function */
            if (Minor & CMD_STREAM_READ)
            {
                /* Now check the sub-type */
                if (Minor & CMD_STREAM_USHORT)
                {
                    /* The port is what is in the stream right now */
                    ShortPort = UlongToPtr((ULONG)*CmdStream);

                    /* Move to the next command */
                    CmdStream++;

                    /* Read USHORT from the port */
                    READ_PORT_USHORT(PtrToUlong(Base) + ShortPort);
                }
                else
                {
                    /* The port is what is in the stream right now */
                    Port = UlongToPtr((ULONG)*CmdStream);

                    /* Move to the next command */
                    CmdStream++;

                    /* Read UCHAR from the port */
                    READ_PORT_UCHAR(PtrToUlong(Base) + Port);
                }
            }
            else if (Minor & CMD_STREAM_WRITE_ARRAY)
            {
                /* Now check the sub-type */
                if (Minor & CMD_STREAM_USHORT)
                {
                    /* The port is what is in the stream right now */
                    ShortPort = UlongToPtr(Cmd);

                    /* Move to the next command and get the count */
                    Count = *(CmdStream++);

                    /* The buffer is what's next in the command stream */
                    Buffer = CmdStream++;

                    /* Write USHORT to the port */
                    WRITE_PORT_BUFFER_USHORT(PtrToUshort(Base) + ShortPort, Buffer, Count);

                    /* Move past the buffer in the command stream */
                    CmdStream += Count;
                }
                else
                {
                    /* The port is what is in the stream right now */
                    Port = UlongToPtr(Cmd);

                    /* Move to the next command and get the count */
                    Count = *(CmdStream++);

                    /* Add the base to the port */
                    Port = PtrToUlong(Port) + Base;

                    /* Move to next command */
                    CmdStream++;

                    /* Loop the cmd array */
                    for (; Count; Count--, CmdStream++)
                    {
                        /* Get the byte we're writing */
                        Value = (UCHAR)*CmdStream;

                        /* Write UCHAR to the port */
                        WRITE_PORT_UCHAR(Port, Value);
                    }
                }
            }
            else if (Minor & CMD_STREAM_USHORT)
            {
                /* Get the ushort we're writing and advance in the stream */
                ShortValue = *CmdStream;
                CmdStream++;

                /* Write USHORT to the port (which is in cmd) */
                WRITE_PORT_USHORT((PUSHORT)Base + Cmd, ShortValue);
            }
            else
            {
                /* The port is what is in the stream right now */
                Port = UlongToPtr((ULONG)*CmdStream);

                /* Get the uchar we're writing */
                Value = (UCHAR)*++CmdStream;

                /* Move to the next command */
                CmdStream++;

                /* Write UCHAR to the port (which is in cmd) */
                WRITE_PORT_UCHAR(PtrToUlong(Base) + Port, Value);
            }
        }
        else if (Major == 0x20)
        {
            /* Check the minor function. Note these are not flags anymore. */
            switch (Minor)
            {
                case 0:
                    /* The port is what is in the stream right now */
                    ShortPort = UlongToPtr(*CmdStream);

                    /* Move to the next command and get the count */
                    Count = *(CmdStream++);

                    /* Move to the next command and get the value to write */
                    ShortValue = *(CmdStream++);

                    /* Add the base to the port */
                    ShortPort = PtrToUlong(ShortPort) + (PUSHORT)Base;

                    /* Move to next command */
                    CmdStream++;

                    /* Make sure we have data */
                    if (!ShortValue) continue;

                    /* Loop the cmd array */
                    for (; Count; Count--, CmdStream++, Value++)
                    {
                        /* Get the byte we're writing */
                        ShortValue += (*CmdStream) << 8;

                        /* Write USHORT to the port */
                        WRITE_PORT_USHORT(ShortPort, ShortValue);
                    }
                    break;
                case 1:
                    /* The port is what is in the stream right now. Add the base too */
                    Port = *CmdStream + Base;

                    /* Move to the next command and get the count */
                    Count = *++CmdStream;

                    /* Move to the next command and get the index to write */
                    Index = (UCHAR)*++CmdStream;

                    /* Move to next command */
                    CmdStream++;

                    /* Loop the cmd array */
                    for (; Count; Count--, Index++)
                    {
                        /* Write the index */
                        WRITE_PORT_UCHAR(Port, Index);

                        /* Get the byte we're writing */
                        Value = (UCHAR)*CmdStream;

                        /* Move to next command */
                        CmdStream++;

                        /* Write UCHAR value to the port */
                        WRITE_PORT_UCHAR(Port, Value);
                    }
                    break;
                case 2:
                    /* The port is what is in the stream right now. Add the base too */
                    Port = *CmdStream + Base;

                    /* Read the current value and add the stream data */
                    Value = READ_PORT_UCHAR(Port);
                    Value &= *CmdStream++;
                    Value ^= *CmdStream++;

                    /* Write the value */
                    WRITE_PORT_UCHAR(Port, Value);
                    break;
                default:
                    /* Unknown command, fail */
                    return FALSE;
            }
        }
        else if (Major != 0xF0)
        {
            /* Unknown major function, fail */
            return FALSE;
        }

        /* Get the next command */
        Cmd = *CmdStream;
    }

    /* If we got here, return success */
    return TRUE;
}

BOOLEAN
NTAPI
VgaIsPresent(VOID)
{
    UCHAR VgaReg, VgaReg2, VgaReg3;
    UCHAR SeqReg, SeqReg2;
    UCHAR i;

    /* Read the VGA Address Register */
    VgaReg = READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE);

    /* Select Read Map Select Register */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 4);

    /* Read it back...it should be 4 */
    if (((READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE)) & 0xF) != 4) return FALSE;

    /* Read the VGA Data Register */
    VgaReg2 = READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF);

    /* Enable all planes */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 3);

    /* Read it back...it should be 3 */
    if (READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF) != 0x3)
    {
        /* Reset the registers and fail */
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 0);
        return FALSE;
    }

    /* Select Bit Mask Register */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 8);

    /* Read it back...it should be 8 */
    if (((READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE)) & 0xF) != 8)
    {
        /* Reset the registers and fail */
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 4);
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 0);
        return FALSE;
    }

    /* Read the VGA Data Register */
    VgaReg3 = READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF);

    /* Loop bitmasks */
    for (i = 0xBB; i; i >>= 1)
    {
        /*  Set bitmask */
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, i);

        /* Read it back...it should be the same */
        if (READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF) != i)
        {
            /* Reset the registers and fail */
            WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 0xFF);
            WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 4);
            WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 0);
            return FALSE;
        }
    }

    /* Select Read Map Select Register */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 4);

    /* Read it back...it should be 3 */
    if (READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF) != 3)
    {
        /* Reset the registers and fail */
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 0);
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 8);
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, 0xFF);
        return FALSE;
    }

    /* Write the registers we read earlier */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, VgaReg2);
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, 8);
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CF, VgaReg3);
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3CE, VgaReg);

    /* Read sequencer address */
    SeqReg = READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C4);

    /* Select memory mode register */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C4, 4);

    /* Read it back...it should still be 4 */
    if (((READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C4)) & 7) != 4)
    {
        /*  Fail */
        return FALSE;
    }

    /* Read sequencer Data */
    SeqReg2 = READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C5);

    /* Write null plane */
    WRITE_PORT_USHORT((PUSHORT)VgaRegisterBase + 0x3C4, 0x100);

    /* Write sequencer flag */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C5, SeqReg2 ^ 8);

    /* Read it back */
    if ((READ_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C5)) != (SeqReg2 ^ 8))
    {
        /* Not the same value...restore registers and fail */
        WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C5, 2);
        WRITE_PORT_USHORT((PUSHORT)VgaRegisterBase + 0x3C4, 0x300);
        return FALSE;
    }

    /* Now write the registers we read */
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C5, SeqReg2);
    WRITE_PORT_USHORT((PUSHORT)VgaRegisterBase + 0x3C4, 0x300);
    WRITE_PORT_UCHAR((PUCHAR)VgaRegisterBase + 0x3C4, SeqReg);

    /* VGA is present! */
    return TRUE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
VidInitialize(IN BOOLEAN SetMode)
{
    ULONG Context = 0;
    PHYSICAL_ADDRESS TranslatedAddress;
    PHYSICAL_ADDRESS NullAddress = {{0}};
    ULONG AddressSpace = 1;
    BOOLEAN Result;
    ULONG_PTR Base;

    /* Make sure that we have a bus translation function */
    if (!HalFindBusAddressTranslation) return FALSE;

    /* Get the VGA Register address */
    Result = HalFindBusAddressTranslation(NullAddress,
                                          &AddressSpace,
                                          &TranslatedAddress,
                                          &Context,
                                          TRUE);
    if (!Result) return FALSE;

    /* See if this is I/O Space, which we need to map */
TryAgain:
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

    /* Set the VGA Register base and now check if we have a VGA device */
    VgaRegisterBase = Base;
    if (VgaIsPresent())
    {
        /* Translate the VGA Memory Address */
        NullAddress.LowPart = 0xA0000;
        AddressSpace = 0;
        Result = HalFindBusAddressTranslation(NullAddress,
                                              &AddressSpace,
                                              &TranslatedAddress,
                                              &Context,
                                              FALSE);
        if (Result)
        {
            /* Success! See if this is I/O Space, which we need to map */
            if (!AddressSpace)
            {
                /* Map it */
                Base = (ULONG_PTR)MmMapIoSpace(TranslatedAddress,
                                               0x20000,
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
                /* Reset the display */
                HalResetDisplay();
                curr_x = 0;
                curr_y = 0;

                /* Initialize it */
                VgaInterpretCmdStream(AT_Initialization);
                return TRUE;
            }
        }
    }
    else
    {
        /* It's not, so unmap the I/O space if we mapped it */
        if (!AddressSpace) MmUnmapIoSpace((PVOID)VgaRegisterBase, 0x400);
    }

    /* If we got here, then we failed...let's try again */
    Result = HalFindBusAddressTranslation(NullAddress,
                                          &AddressSpace,
                                          &TranslatedAddress,
                                          &Context,
                                          TRUE);
    if (Result) goto TryAgain;

    /* If we got here, then we failed even past our re-try... */
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
VidResetDisplay(IN BOOLEAN HalReset)
{
    /* Clear the current position */
    curr_x = 0;
    curr_y = 0;

    /* Clear the screen with HAL if we were asked to */
    if (HalReset) HalResetDisplay();

    /* Re-initialize the VGA Display */
    VgaInterpretCmdStream(AT_Initialization);

    /* Re-initialize the palette and fill the screen black */
    InitializePalette();
    VidSolidColorFill(0, 0, 639, 479, 0);
}

