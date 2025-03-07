/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ex/hdlsterm.c
 * PURPOSE:         Headless Terminal Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* GLOBALS ********************************************************************/

PHEADLESS_GLOBALS HeadlessGlobals;

/* FUNCTIONS ******************************************************************/

FORCEINLINE
KIRQL
HdlspAcquireGlobalLock(VOID)
{
    KIRQL OldIrql;

    /* Don't acquire the lock if we are bugchecking */
    if (!HeadlessGlobals->InBugCheck)
    {
        KeAcquireSpinLock(&HeadlessGlobals->SpinLock, &OldIrql);
    }
    else
    {
        OldIrql = 0xFF;
    }

    return OldIrql;
}

FORCEINLINE
VOID
HdlspReleaseGlobalLock(IN KIRQL OldIrql)
{
    /* Only release the lock if we aren't bugchecking */
    if (OldIrql != 0xFF)
    {
        KeReleaseSpinLock(&HeadlessGlobals->SpinLock, OldIrql);
    }
    else
    {
        ASSERT(HeadlessGlobals->InBugCheck == TRUE);
    }
}

VOID
NTAPI
HdlspSendStringAtBaud(IN PUCHAR String)
{
    /* Send every byte */
    while (*String != ANSI_NULL)
    {
        InbvPortPutByte(HeadlessGlobals->TerminalPort, *String++);
    }
}

VOID
NTAPI
HdlspPutData(IN PUCHAR Data,
             IN ULONG DataSize)
{
    ULONG i;
    for (i = 0; i < DataSize; i++)
    {
        InbvPortPutByte(HeadlessGlobals->TerminalPort, Data[i]);
    }
}

VOID
NTAPI
HdlspPutString(IN PUCHAR String)
{
    PUCHAR Dest = HeadlessGlobals->TmpBuffer;
    UCHAR Char = 0;

    /* Scan each character */
    while (*String != ANSI_NULL)
    {
        /* Check for rotate, send existing buffer and restart from where we are */
        if (Dest >= &HeadlessGlobals->TmpBuffer[79])
        {
            HeadlessGlobals->TmpBuffer[79] = ANSI_NULL;
            HdlspSendStringAtBaud(HeadlessGlobals->TmpBuffer);
            Dest = HeadlessGlobals->TmpBuffer;
        }
        else
        {
            /* Get the current character and check for special graphical chars */
            Char = *String;
            if (Char & 0x80)
            {
                switch (Char)
                {
                    case 0xB0: case 0xB3: case 0xBA:
                        Char = '|';
                        break;
                    case 0xB1: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
                        Char = '%';
                        break;
                    case 0xB2: case 0xDB:
                        Char = '#';
                        break;
                    case 0xA9: case 0xAA: case 0xBB: case 0xBC: case 0xBF:
                    case 0xC0: case 0xC8: case 0xC9: case 0xD9: case 0xDA:
                        Char = '+';
                        break;
                    case 0xC4:
                        Char = '-';
                        break;
                    case 0xCD:
                        Char = '=';
                        break;
                }
            }

            /* Anything else must be Unicode */
            if (Char & 0x80)
            {
                /* Can't do Unicode yet */
                UNIMPLEMENTED;
            }
            else
            {
                /* Add the modified char to the temporary buffer */
                *Dest++ = Char;
            }

            /* Check the next char */
            String++;
        }
    }

    /* Finish and send */
    *Dest = ANSI_NULL;
    HdlspSendStringAtBaud(HeadlessGlobals->TmpBuffer);
}

NTSTATUS
NTAPI
HdlspEnableTerminal(IN BOOLEAN Enable)
{
    /* Enable if requested, as long as this isn't a PCI serial port crashing */
    if ((Enable) &&
        !(HeadlessGlobals->TerminalEnabled) &&
        !((HeadlessGlobals->IsMMIODevice) && (HeadlessGlobals->InBugCheck)))
    {
        /* Initialize the COM port with cportlib */
        HeadlessGlobals->TerminalEnabled = InbvPortInitialize(HeadlessGlobals->TerminalBaudRate,
                                                              HeadlessGlobals->TerminalPortNumber,
                                                              HeadlessGlobals->TerminalPortAddress,
                                                              &HeadlessGlobals->TerminalPort,
                                                              HeadlessGlobals->IsMMIODevice);
        if (!HeadlessGlobals->TerminalEnabled)
        {
            DPRINT1("Failed to initialize port through cportlib\n");
            return STATUS_UNSUCCESSFUL;
        }

        /* Cleanup the screen and reset the cursor */
        HdlspSendStringAtBaud((PUCHAR)"\x1B[2J");
        HdlspSendStringAtBaud((PUCHAR)"\x1B[H");

        /* Enable FIFO */
        InbvPortEnableFifo(HeadlessGlobals->TerminalPort, TRUE);
    }
    else if (!Enable)
    {
        /* Specific case when headless is being disabled */
        InbvPortTerminate(HeadlessGlobals->TerminalPort);
        HeadlessGlobals->TerminalPort = 0;
        HeadlessGlobals->TerminalEnabled = FALSE;
    }

    /* All done */
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
VOID
NTAPI
HeadlessInit(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PHEADLESS_LOADER_BLOCK HeadlessBlock;

    /* Only initialize further if the loader found EMS enabled */
    HeadlessBlock = LoaderBlock->Extension->HeadlessLoaderBlock;
    if (!HeadlessBlock) return;

    /* Ignore invalid EMS settings */
    if ((HeadlessBlock->PortNumber > 4) && (HeadlessBlock->UsedBiosSettings)) return;

    /* Allocate the global headless data */
    HeadlessGlobals = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(*HeadlessGlobals),
                                            'sldH');
    if (!HeadlessGlobals) return;

    /* Zero and copy loader data */
    RtlZeroMemory(HeadlessGlobals, sizeof(*HeadlessGlobals));
    HeadlessGlobals->TerminalPortNumber = HeadlessBlock->PortNumber;
    HeadlessGlobals->TerminalPortAddress = HeadlessBlock->PortAddress;
    HeadlessGlobals->TerminalBaudRate = HeadlessBlock->BaudRate;
    HeadlessGlobals->TerminalParity = HeadlessBlock->Parity;
    HeadlessGlobals->TerminalStopBits = HeadlessBlock->StopBits;
    HeadlessGlobals->UsedBiosSettings = HeadlessBlock->UsedBiosSettings;
    HeadlessGlobals->IsMMIODevice = HeadlessBlock->IsMMIODevice;
    HeadlessGlobals->TerminalType = HeadlessBlock->TerminalType;
    HeadlessGlobals->SystemGUID = HeadlessBlock->SystemGUID;
    DPRINT1("EMS on Port %lu (0x%p) at %lu bps\n",
             HeadlessGlobals->TerminalPortNumber,
             HeadlessGlobals->TerminalPortAddress,
             HeadlessGlobals->TerminalBaudRate);

    /* These two are opposites of each other */
    if (HeadlessGlobals->IsMMIODevice) HeadlessGlobals->IsNonLegacyDevice = TRUE;

    /* Check for a PCI device, warn that this isn't supported */
    if (HeadlessBlock->PciDeviceId != PCI_INVALID_VENDORID)
    {
        DPRINT1("PCI Serial Ports not supported\n");
    }

    /* Log entries are not yet supported */
    DPRINT1("FIXME: No Headless logging support\n");

    /* Allocate temporary buffer */
    HeadlessGlobals->TmpBuffer = ExAllocatePoolWithTag(NonPagedPool, 80, 'sldH');
    if (!HeadlessGlobals->TmpBuffer) return;

    /* Windows seems to apply some special hacks for 9600 bps */
    if (HeadlessGlobals->TerminalBaudRate == 9600)
    {
        DPRINT1("Please use other baud rate than 9600bps for now\n");
    }

    /* Enable the terminal */
    HdlspEnableTerminal(TRUE);
}

NTSTATUS
NTAPI
HdlspDispatch(IN HEADLESS_CMD Command,
              IN PVOID InputBuffer,
              IN SIZE_T InputBufferSize,
              OUT PVOID OutputBuffer,
              OUT PSIZE_T OutputBufferSize)
{
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PHEADLESS_RSP_QUERY_INFO HeadlessInfo;
    PHEADLESS_CMD_PUT_STRING PutString;
    PHEADLESS_CMD_ENABLE_TERMINAL EnableTerminal;
    PHEADLESS_CMD_SET_COLOR SetColor;
    PHEADLESS_CMD_POSITION_CURSOR CursorPos;
    PHEADLESS_RSP_GET_BYTE GetByte;
    UCHAR DataBuffer[80];

    ASSERT(HeadlessGlobals != NULL);
    // ASSERT(HeadlessGlobals->PageLockHandle != NULL);

    /* Ignore non-reentrant commands */
    if ((Command != HeadlessCmdAddLogEntry) &&
        (Command != HeadlessCmdStartBugCheck) &&
        (Command != HeadlessCmdSendBlueScreenData) &&
        (Command != HeadlessCmdDoBugCheckProcessing))
    {
        OldIrql = HdlspAcquireGlobalLock();

        if (HeadlessGlobals->ProcessingCmd)
        {
            HdlspReleaseGlobalLock(OldIrql);
            return STATUS_UNSUCCESSFUL;
        }

        /* Don't allow these commands next time */
        HeadlessGlobals->ProcessingCmd = TRUE;
        HdlspReleaseGlobalLock(OldIrql);
    }

    /* Handle each command */
    switch (Command)
    {
        case HeadlessCmdEnableTerminal:
        {
            /* Make sure the caller passed valid data */
            if (!(InputBuffer) ||
                (InputBufferSize != sizeof(*EnableTerminal)))
            {
                DPRINT1("Invalid buffer\n");
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Go and enable it */
            EnableTerminal = InputBuffer;
            Status = HdlspEnableTerminal(EnableTerminal->Enable);
            break;
        }

        case HeadlessCmdCheckForReboot:
            break;

        case HeadlessCmdPutString:
        {
            /* Validate the existence of an input buffer */
            if (!InputBuffer)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Terminal should be on */
            if (HeadlessGlobals->TerminalEnabled)
            {
                /* Print each byte in the string making sure VT100 chars are used */
                PutString = InputBuffer;
                HdlspPutString(PutString->String);
            }

            /* Return success either way */
            Status = STATUS_SUCCESS;
            break;
        }

        case HeadlessCmdClearDisplay:
        case HeadlessCmdClearToEndOfDisplay:
        case HeadlessCmdClearToEndOfLine:
        case HeadlessCmdDisplayAttributesOff:
        case HeadlessCmdDisplayInverseVideo:
        case HeadlessCmdSetColor:
        case HeadlessCmdPositionCursor:
        {
            /* By default return success */
            Status = STATUS_SUCCESS;

            /* Send the VT100 commands only if the terminal is enabled */
            if (HeadlessGlobals->TerminalEnabled)
            {
                PUCHAR CommandStr = NULL;

                if (Command == HeadlessCmdClearDisplay)
                    CommandStr = (PUCHAR)"\x1B[2J";
                else if (Command == HeadlessCmdClearToEndOfDisplay)
                    CommandStr = (PUCHAR)"\x1B[0J";
                else if (Command == HeadlessCmdClearToEndOfLine)
                    CommandStr = (PUCHAR)"\x1B[0K";
                else if (Command == HeadlessCmdDisplayAttributesOff)
                    CommandStr = (PUCHAR)"\x1B[0m";
                else if (Command == HeadlessCmdDisplayInverseVideo)
                    CommandStr = (PUCHAR)"\x1B[7m";
                else if (Command == HeadlessCmdSetColor)
                {
                    /* Make sure the caller passed valid data */
                    if (!InputBuffer ||
                        (InputBufferSize != sizeof(*SetColor)))
                    {
                        DPRINT1("Invalid buffer\n");
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    SetColor = InputBuffer;
                    Status = RtlStringCbPrintfA((PCHAR)DataBuffer, sizeof(DataBuffer),
                                                "\x1B[%d;%dm",
                                                SetColor->BkgdColor,
                                                SetColor->TextColor);
                    if (!NT_SUCCESS(Status)) break;

                    CommandStr = DataBuffer;
                }
                else // if (Command == HeadlessCmdPositionCursor)
                {
                    /* Make sure the caller passed valid data */
                    if (!InputBuffer ||
                        (InputBufferSize != sizeof(*CursorPos)))
                    {
                        DPRINT1("Invalid buffer\n");
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    CursorPos = InputBuffer;
                    /* Cursor position is 1-based */
                    Status = RtlStringCbPrintfA((PCHAR)DataBuffer, sizeof(DataBuffer),
                                                "\x1B[%d;%dH",
                                                CursorPos->CursorRow + 1,
                                                CursorPos->CursorCol + 1);
                    if (!NT_SUCCESS(Status)) break;

                    CommandStr = DataBuffer;
                }

                /* Send the command */
                HdlspSendStringAtBaud(CommandStr);
            }

            break;
        }

        case HeadlessCmdTerminalPoll:
            break;

        case HeadlessCmdGetByte:
        {
            /* Make sure the caller passed valid data */
            if (!(OutputBuffer) ||
                !(OutputBufferSize) ||
                (*OutputBufferSize < sizeof(*GetByte)))
            {
                DPRINT1("Invalid buffer\n");
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Make sure the terminal is enabled */
            GetByte = OutputBuffer;
            if (HeadlessGlobals->TerminalEnabled)
            {
                /* Poll if something is on the wire */
                if (InbvPortPollOnly(HeadlessGlobals->TerminalPort))
                {
                    /* If so, read it */
                    InbvPortGetByte(HeadlessGlobals->TerminalPort,
                                    &GetByte->Value);
                }
                else
                {
                    /* Nothing is there, return 0 */
                    GetByte->Value = 0;
                }
            }
            else
            {
                /* Otherwise return nothing */
                GetByte->Value = 0;
            }

            /* Return success either way */
            Status = STATUS_SUCCESS;
            break;
        }

        case HeadlessCmdGetLine:
            break;

        case HeadlessCmdStartBugCheck:
        {
            HeadlessGlobals->InBugCheck = TRUE;
            HeadlessGlobals->ProcessingCmd = FALSE;
            Status = STATUS_SUCCESS;
            break;
        }

        case HeadlessCmdDoBugCheckProcessing:
            break;

        case HeadlessCmdQueryInformation:
        {
            /* Make sure the caller passed valid data */
            if (!(OutputBuffer) ||
                !(OutputBufferSize) ||
                (*OutputBufferSize < sizeof(*HeadlessInfo)))
            {
                DPRINT1("Invalid buffer\n");
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* If we got here, headless is enabled -- we know this much */
            HeadlessInfo = OutputBuffer;
            HeadlessInfo->PortType = HeadlessSerialPort;
            HeadlessInfo->Serial.TerminalAttached = TRUE;
            HeadlessInfo->Serial.UsedBiosSettings = HeadlessGlobals->UsedBiosSettings != 0;
            HeadlessInfo->Serial.TerminalBaudRate = HeadlessGlobals->TerminalBaudRate;
            HeadlessInfo->Serial.TerminalType = HeadlessGlobals->TerminalType;

            /* Now check on what port/baud it's enabled on */
            if ((HeadlessGlobals->TerminalPortNumber >= 1) ||
                (HeadlessGlobals->UsedBiosSettings))
            {
                /* Get the EMS information */
                HeadlessInfo->Serial.TerminalPort = HeadlessGlobals->
                                                    TerminalPortNumber;
                HeadlessInfo->Serial.TerminalPortBaseAddress = HeadlessGlobals->
                                                               TerminalPortAddress;
            }
            else
            {
                /* We don't know for sure */
                HeadlessInfo->Serial.TerminalPort = SerialPortUndefined;
                HeadlessInfo->Serial.TerminalPortBaseAddress = 0;
            }

            /* All done */
            Status = STATUS_SUCCESS;
            break;
        }

        case HeadlessCmdAddLogEntry:
            break;
        case HeadlessCmdDisplayLog:
            break;

        case HeadlessCmdSetBlueScreenData:
        {
            /* Validate the existence of an input buffer */
            if (!InputBuffer)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Lie so that we can get Hdl bringup a little bit further */
            UNIMPLEMENTED;
            Status = STATUS_SUCCESS;
            break;
        }

        case HeadlessCmdSendBlueScreenData:
            // TODO: Send XML description of bugcheck.
            // InputBuffer points to the BugCheckCode.
            break;

        case HeadlessCmdQueryGUID:
            break;

        case HeadlessCmdPutData:
        {
            /* Validate the existence of an input buffer */
            if (!(InputBuffer) || !(InputBufferSize))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Terminal should be on */
            if (HeadlessGlobals->TerminalEnabled)
            {
                /* Print each byte in the string making sure VT100 chars are used */
                PutString = InputBuffer;
                HdlspPutData(PutString->String, InputBufferSize);
            }

            /* Return success either way */
            Status = STATUS_SUCCESS;
            break;
        }

        default:
            break;
    }

    /* Unset processing state */
    if ((Command != HeadlessCmdAddLogEntry) &&
        (Command != HeadlessCmdStartBugCheck) &&
        (Command != HeadlessCmdSendBlueScreenData) &&
        (Command != HeadlessCmdDoBugCheckProcessing))
    {
        ASSERT(HeadlessGlobals->ProcessingCmd == TRUE);
        HeadlessGlobals->ProcessingCmd = FALSE;
    }

    /* All done */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
HeadlessDispatch(IN HEADLESS_CMD Command,
                 IN PVOID InputBuffer,
                 IN SIZE_T InputBufferSize,
                 OUT PVOID OutputBuffer,
                 OUT PSIZE_T OutputBufferSize)
{
    /* Check for stubs that will expect something even with headless off */
    if (!HeadlessGlobals)
    {
        /* Don't allow the SAC to connect */
        if (Command == HeadlessCmdEnableTerminal) return STATUS_UNSUCCESSFUL;

        /* Send bogus reply */
        if ((Command == HeadlessCmdQueryInformation) ||
            (Command == HeadlessCmdGetByte) ||
            (Command == HeadlessCmdGetLine) ||
            (Command == HeadlessCmdCheckForReboot) ||
            (Command == HeadlessCmdTerminalPoll))
        {
            if (!(OutputBuffer) || !(OutputBufferSize))
            {
                return STATUS_INVALID_PARAMETER;
            }

            RtlZeroMemory(OutputBuffer, *OutputBufferSize);
        }

        return STATUS_SUCCESS;
    }

    /* Do the real work */
    return HdlspDispatch(Command,
                         InputBuffer,
                         InputBufferSize,
                         OutputBuffer,
                         OutputBufferSize);
}

/* EOF */
