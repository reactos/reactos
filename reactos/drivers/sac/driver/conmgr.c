/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/conmgr.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

DEFINE_GUID(PRIMARY_SAC_CHANNEL_APPLICATION_GUID,
            0x63D02270,
            0x8AA4,
            0x11D5,
            0xBC, 0xCF, 0x80, 0x6D, 0x61, 0x72, 0x69, 0x6F);

LONG CurrentChannelRefCount;
KMUTEX CurrentChannelLock;

PSAC_CHANNEL CurrentChannel;
PSAC_CHANNEL SacChannel;

ULONG ExecutePostConsumerCommand;
PSAC_CHANNEL ExecutePostConsumerCommandData;

BOOLEAN InputInEscape, InputInEscTab, ConMgrLastCharWasCR;
CHAR InputBuffer[80];

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
SacPutString(IN PWCHAR String)
{
    NTSTATUS Status;

    /* Write the string on the main SAC channel */
    Status = ChannelOWrite(SacChannel,
                           (PCHAR)String,
                           wcslen(String) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        SAC_DBG(SAC_DBG_INIT, "SAC XmlMgrSacPutString: OWrite failed\n");
    }
}

BOOLEAN
NTAPI
SacPutSimpleMessage(IN ULONG MessageIndex)
{
    PWCHAR MessageBuffer;
    BOOLEAN Result;

    /* Get the message */
    MessageBuffer = GetMessage(MessageIndex);
    if (MessageBuffer)
    {
        /* Output it */
        SacPutString(MessageBuffer);
        Result = TRUE;
    }
    else
    {
        Result = FALSE;
    }

    /* All done */
    return Result;
}

NTSTATUS
NTAPI
ConMgrDisplayCurrentChannel(VOID)
{
    NTSTATUS Status;
    BOOLEAN HasRedraw;

    /* Make sure the lock is held */
    SacAssertMutexLockHeld();

    /* Check if we can redraw */
    Status = ChannelHasRedrawEvent(CurrentChannel, &HasRedraw);
    if (NT_SUCCESS(Status))
    {
        /* Enable writes */
        _InterlockedExchange(&CurrentChannel->WriteEnabled, 1);
        if (HasRedraw)
        {
            /* If we can redraw, set the event */
            ChannelSetRedrawEvent(CurrentChannel);
        }

        /* Flush the output */
        Status = ChannelOFlush(CurrentChannel);
    }

    /* All done, return the status */
    return Status;
}

NTSTATUS
NTAPI
ConMgrWriteData(IN PSAC_CHANNEL Channel,
                IN PVOID Buffer,
                IN ULONG BufferLength)
{
    ULONG i;
    NTSTATUS Status;
    LARGE_INTEGER Interval;

    /* Loop up to 32 times */
    for (i = 0; i < 32; i++)
    {
        /* Attempt sending the data */
        Status = HeadlessDispatch(HeadlessCmdPutData, Buffer, BufferLength, NULL, NULL);
        if (Status != STATUS_UNSUCCESSFUL) break;

        /* Sending the data on the port failed, wait a second... */
        Interval.HighPart = -1;
        Interval.LowPart = -100000;
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }

    /* After 32 attempts it should really have worked... */
    ASSERT(NT_SUCCESS(Status));
    return Status;
}

NTSTATUS
NTAPI
ConMgrFlushData(IN PSAC_CHANNEL Channel)
{
    /* Nothing to do */
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
ConMgrIsSacChannel(IN PSAC_CHANNEL Channel)
{
    /* Check which channel is active */
    return Channel == SacChannel;
}

BOOLEAN
NTAPI
ConMgrIsWriteEnabled(IN PSAC_CHANNEL Channel)
{
    /* If the current channel is active, allow writes */
    return ChannelIsEqual(Channel, &CurrentChannel->ChannelId);
}

NTSTATUS
NTAPI
ConMgrInitialize(VOID)
{
    PWCHAR pcwch;
    PSAC_CHANNEL FoundChannel;
    SAC_CHANNEL_ATTRIBUTES SacChannelAttributes;
    NTSTATUS Status;

    /* Initialize the connection manager lock */
    SacInitializeMutexLock();
    SacAcquireMutexLock();

    /* Setup the attributes for the raw SAC channel */
    RtlZeroMemory(&SacChannelAttributes, sizeof(SacChannelAttributes));
    SacChannelAttributes.ChannelType = Raw;

    /* Get the right name for it */
    pcwch = GetMessage(SAC_CHANNEL_NAME);
    ASSERT(pcwch);
    wcsncpy(SacChannelAttributes.NameBuffer, pcwch, SAC_CHANNEL_NAME_SIZE);
    SacChannelAttributes.NameBuffer[SAC_CHANNEL_NAME_SIZE] = ANSI_NULL;

    /* Get the right description for it */
    pcwch = GetMessage(SAC_CHANNEL_DESCRIPTION);
    ASSERT(pcwch);
    wcsncpy(SacChannelAttributes.DescriptionBuffer, pcwch, SAC_CHANNEL_DESCRIPTION_SIZE);
    SacChannelAttributes.DescriptionBuffer[SAC_CHANNEL_DESCRIPTION_SIZE] = ANSI_NULL;

    /* Set all the right flags */
    SacChannelAttributes.Flag = SAC_CHANNEL_FLAG_APPLICATION | SAC_CHANNEL_FLAG_INTERNAL;
    SacChannelAttributes.CloseEvent = NULL;
    SacChannelAttributes.HasNewDataEvent = NULL;
    SacChannelAttributes.LockEvent = NULL;
    SacChannelAttributes.RedrawEvent = NULL;
    SacChannelAttributes.ChannelId = PRIMARY_SAC_CHANNEL_APPLICATION_GUID;

    /* Now create it */
    Status = ChanMgrCreateChannel(&SacChannel, &SacChannelAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Try to get it back */
        Status = ChanMgrGetByHandle(SacChannel->ChannelId, &FoundChannel);
        if (NT_SUCCESS(Status))
        {
            /* Set it as the current and SAC channel */
            SacChannel = CurrentChannel = FoundChannel;

            /* Diasable writes for now and clear the display */
            _InterlockedExchange(&FoundChannel->WriteEnabled, FALSE);
            Status = HeadlessDispatch(HeadlessCmdClearDisplay, NULL, 0, NULL, NULL);
            if (!NT_SUCCESS(Status))
            {
                SAC_DBG(SAC_DBG_INIT, "SAC ConMgrInitialize: Failed dispatch\n");
            }

            /* Display the initial prompt */
            SacPutSimpleMessage(SAC_NEWLINE);
            SacPutSimpleMessage(SAC_INIT_STATUS);
            SacPutSimpleMessage(SAC_NEWLINE);
            SacPutSimpleMessage(SAC_PROMPT);

            /* Display the current channel */
            ConMgrDisplayCurrentChannel();
        }
    }

    /* Release the channel lock */
    SacReleaseMutexLock();
    return STATUS_SUCCESS;
}

VOID
NTAPI
ConMgrEventMessage(IN PWCHAR EventMessage,
                   IN BOOLEAN LockHeld)
{
    /* Acquire the current channel lock if needed */
    if (!LockHeld) SacAcquireMutexLock();

    /* Send out the event message */
    SacPutSimpleMessage(2);
    SacPutString(EventMessage);
    SacPutSimpleMessage(3);

    /* Release the current channel lock if needed */
    if (!LockHeld) SacReleaseMutexLock();
}

BOOLEAN
NTAPI
ConMgrSimpleEventMessage(IN ULONG MessageIndex,
                         IN BOOLEAN LockHeld)
{
    PWCHAR MessageBuffer;
    BOOLEAN Result;

    /* Get the message to send out */
    MessageBuffer = GetMessage(MessageIndex);
    if (MessageBuffer)
    {
        /* Send it */
        ConMgrEventMessage(MessageBuffer, LockHeld);
        Result = TRUE;
    }
    else
    {
        /* It doesn't exist, fail */
        Result = FALSE;
    }

    /* Return if the message was sent or not */
    return Result;
}

NTSTATUS
NTAPI
ConMgrDisplayFastChannelSwitchingInterface(IN PSAC_CHANNEL Channel)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
ConMgrSetCurrentChannel(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    BOOLEAN HasRedrawEvent;

    /* Make sure the lock is held */
    SacAssertMutexLockHeld();

    /* Check if we have a redraw event */
    Status = ChannelHasRedrawEvent(CurrentChannel, &HasRedrawEvent);
    if (!NT_SUCCESS(Status)) return Status;

    /* Clear it */
    if (HasRedrawEvent) ChannelClearRedrawEvent(CurrentChannel);

    /* Disable writes on the current channel */
    _InterlockedExchange(&CurrentChannel->WriteEnabled, 0);

    /* Release the current channel */
    Status = ChanMgrReleaseChannel(CurrentChannel);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set the new channel and also disable writes on it */
    CurrentChannel = Channel;
    _InterlockedExchange(&Channel->WriteEnabled, 0);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ConMgrResetCurrentChannel(IN BOOLEAN KeepChannel)
{
    NTSTATUS Status;
    PSAC_CHANNEL Channel;

    /* Make sure the lock is held */
    SacAssertMutexLockHeld();

    /* Get the current SAC channel */
    Status = ChanMgrGetByHandle(SacChannel->ChannelId, &Channel);
    if (NT_SUCCESS(Status))
    {
        /* Set this as the current SAC channel*/
        SacChannel = Channel;
        Status = ConMgrSetCurrentChannel(Channel);
        if (NT_SUCCESS(Status))
        {
            /* Check if the caller wants to switch or not */
            if (KeepChannel)
            {
                /* Nope, keep the same channel */
                Status = ConMgrDisplayCurrentChannel();
            }
            else
            {
                /* Yep, show the switching interface */
                Status = ConMgrDisplayFastChannelSwitchingInterface(CurrentChannel);
            }
        }
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
ConMgrChannelClose(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check if we're in the right channel */
    if (ConMgrIsWriteEnabled(Channel))
    {
        /* Yep, reset it */
        Status = ConMgrResetCurrentChannel(FALSE);
        ASSERT(NT_SUCCESS(Status));
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
ConMgrShutdown(VOID)
{
    NTSTATUS Status;

    /* Check if we have a SAC channel */
    if (SacChannel)
    {
        /* Close it */
        Status = ChannelClose(SacChannel);
        if (!NT_SUCCESS(Status))
        {
            SAC_DBG(SAC_DBG_INIT, "SAC ConMgrShutdown: failed closing SAC channel.\n");
        }

        /* No longer have one */
        SacChannel = NULL;
    }

    /* Check if we have a current channel */
    if (CurrentChannel)
    {
        /* Release it */
        Status = ChanMgrReleaseChannel(CurrentChannel);
        if (!NT_SUCCESS(Status))
        {
            SAC_DBG(SAC_DBG_INIT, "SAC ConMgrShutdown: failed releasing current channel\n");
        }

        /* No longer have one */
        CurrentChannel = NULL;
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ConMgrAdvanceCurrentChannel(VOID)
{
    NTSTATUS Status;
    ULONG Index;
    PSAC_CHANNEL Channel;

    /* Should always be called with the lock held */
    SacAssertMutexLockHeld();

    /* Get the next active channel */
    Status = ChanMgrGetNextActiveChannel(CurrentChannel, &Index, &Channel);
    if (NT_SUCCESS(Status))
    {
        /* Set it as the new channel */
        Status = ConMgrSetCurrentChannel(Channel);
        if (NT_SUCCESS(Status))
        {
            /* Let the user switch to it */
            Status = ConMgrDisplayFastChannelSwitchingInterface(Channel);
        }
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
ConMgrChannelOWrite(IN PSAC_CHANNEL Channel,
                    IN PVOID WriteBuffer)
{
    NTSTATUS Status;

    /* Do the write with the lock held */
    SacAcquireMutexLock();
    Status = STATUS_NOT_IMPLEMENTED;// ChannelOWrite(Channel, WriteBuffer + 24, *(WriteBuffer + 20));
    SacReleaseMutexLock();

    /* Return back to the caller */
    ASSERT(NT_SUCCESS(Status) || Status == STATUS_NOT_FOUND);
    return Status;
}

VOID
NTAPI
ConMgrProcessInputLine(VOID)
{
    ASSERT(FALSE);
}

#define Nothing 0

VOID
NTAPI
ConMgrSerialPortConsumer(VOID)
{
    NTSTATUS Status;
    CHAR Char, LastChar;
    CHAR WriteBuffer[2], ReadBuffer[2];
    ULONG ReadBufferSize, i;
    WCHAR StringBuffer[2];
    SAC_DBG(SAC_DBG_MACHINE, "SAC TimerDpcRoutine: Entering.\n"); //bug

    /* Acquire the manager lock and make sure a channel is selected */
    SacAcquireMutexLock();
    ASSERT(CurrentChannel);

    /* Read whatever came off the serial port */
    for (Status = SerialBufferGetChar(&Char);
         NT_SUCCESS(Status);
         Status = SerialBufferGetChar(&Char))
    {
        /* If nothing came through, bail out */
        if (Status == STATUS_NO_DATA_DETECTED) break;

        /* Check if ESC was pressed */
        if (Char == '\x1B')
        {
            /* Was it already pressed? */
            if (!InputInEscape)
            {
                /* First time ESC is pressed! Remember and reset TAB state */
                InputInEscTab = FALSE;
                InputInEscape = TRUE;
                continue;
            }
        }
        else if (Char == '\t')
        {
            /* TAB was pressed, is it following ESC (VT-100 sequence)? */
            if (InputInEscape)
            {
                /* Yes! This must be the only ESC-TAB we see in once moment */
                ASSERT(InputInEscTab == FALSE);

                /* No longer treat us as being in ESC */
                InputInEscape = FALSE;

                /* ESC-TAB is the sequence for changing channels */
                Status = ConMgrAdvanceCurrentChannel();
                if (!NT_SUCCESS(Status)) break;

                /* Remember ESC-TAB was pressed */
                InputInEscTab = TRUE;
                continue;
            }
        }
        else if ((Char == '0') && (InputInEscTab))
        {
            /* It this ESC-TAB-0? */
            ASSERT(InputInEscape == FALSE);
            InputInEscTab = FALSE;

            /* If writes are already enabled, don't do this */
            if (!CurrentChannel->WriteEnabled)
            {
                /* Reset the channel, this is our special sequence */
                Status = ConMgrResetCurrentChannel(FALSE);
                if (!NT_SUCCESS(Status)) break;
            }

            continue;
        }
        else
        {
            /* This is ESC-TAB-something else */
            InputInEscTab = FALSE;

            /* If writes are already enabled, don't do this */
            if (!CurrentChannel->WriteEnabled)
            {
                /* Display the current channel */
                InputInEscape = FALSE;
                Status = ConMgrDisplayCurrentChannel();
                if (!NT_SUCCESS(Status)) break;
                continue;
            }
        }

        /* Check if an ESC-sequence was being typed into a command channel */
        if ((InputInEscape) && (CurrentChannel != SacChannel))
        {
            /* Store the ESC in the current channel buffer */
            WriteBuffer[0] = '\x1B';
            ChannelIWrite(CurrentChannel, WriteBuffer, sizeof(CHAR));
        }

        /* Check if we are no longer pressing ESC and exit the mode if so */
        if (Char != '\x1B') InputInEscape = FALSE;

        /* Whatever was typed in, save it int eh current channel */
        ChannelIWrite(CurrentChannel, &Char, sizeof(Char));

        /* If this is a command channel, we're done, nothing to process */
        if (CurrentChannel != SacChannel) continue;

        /* Check for line feed right after a carriage return */
        if ((ConMgrLastCharWasCR) && (Char == '\n'))
        {
            /* Ignore the line feed, but clear the carriage return */
            ChannelIReadLast(CurrentChannel);
            ConMgrLastCharWasCR = 0;
            continue;
        }

        /* Check if the user did a carriage return */
        ConMgrLastCharWasCR = (Char == '\n');

        /* If the user did an "ENTER", we need to run the command */
        if ((Char == '\n') || (Char == '\r'))
        {
            /* Echo back to the terminal */
            SacPutString(L"\r\n");

DoLineParsing:
            /* Inhibit the character (either CR or LF) */
            ChannelIReadLast(CurrentChannel);

            /* NULL-terminate the channel's input buffer */
            WriteBuffer[0] = ANSI_NULL;
            ChannelIWrite(CurrentChannel, WriteBuffer, sizeof(CHAR));

            /* Loop over every last character */
            do
            {
                /* Read every character in the channel, and strip whitespace */
                LastChar = ChannelIReadLast(CurrentChannel);
                WriteBuffer[0] = LastChar;
            } while ((!(LastChar) ||
                       (LastChar == ' ') ||
                       (LastChar == '\t')) &&
                     (ChannelIBufferLength(CurrentChannel)));

            /* Write back into the channel the last character */
            ChannelIWrite(CurrentChannel, WriteBuffer, sizeof(CHAR));

            /* NULL-terminate the input buffer */
            WriteBuffer[0] = ANSI_NULL;
            ChannelIWrite(CurrentChannel, WriteBuffer, sizeof(WCHAR));

            /* Now loop over every first character */
            do
            {
                /* Read every character in the channel, and strip whitespace */
                ChannelIRead(CurrentChannel,
                             ReadBuffer,
                             sizeof(ReadBuffer),
                             &ReadBufferSize);
                WriteBuffer[0] = ReadBuffer[0];
            } while ((ReadBufferSize) &&
                     ((ReadBuffer[0] != ' ') || (ReadBuffer[0] != '\t')));

            /* We read one more than we should, so treat that as our first one */
            InputBuffer[0] = ReadBuffer[0];
            i = 1;

            /* And now loop reading all the others */
            do
            {
                /* Read each character -- there should be max 80 */
                ChannelIRead(CurrentChannel,
                             ReadBuffer,
                             sizeof(ReadBuffer),
                             &ReadBufferSize);
                ASSERT(i < SAC_VTUTF8_COL_WIDTH);
                InputBuffer[i++] = ReadBuffer[0];
            } while (ReadBufferSize);

            /* Now go over the entire input stream */
            for (i = 0; InputBuffer[i]; i++)
            {
                /* Again it should be less than 80 characters */
                ASSERT(i < SAC_VTUTF8_COL_WIDTH);

                /* And upcase each character */
                Char = InputBuffer[i];
                if ((Char >= 'A') && (Char <= 'Z')) InputBuffer[i] = Char + ' ';
            }

            /* Ok, at this point, no pending command should exist */
            ASSERT(ExecutePostConsumerCommand == Nothing);

            /* Go and process the input, then show the prompt again */
            ConMgrProcessInputLine();
            SacPutSimpleMessage(SAC_PROMPT);

            /* If the user typed a valid command, get out of here */
            if (ExecutePostConsumerCommand != Nothing) break;

            /* Keep going */
            continue;
        }

        /* Check if the user typed backspace or delete */
        if ((Char == '\b') || (Char == '\x7F'))
        {
            /* Omit the last character, which should be the DEL/BS itself */
            if (ChannelIBufferLength(CurrentChannel))
            {
                ChannelIReadLast(CurrentChannel);
            }

            /* Omit the before-last character, which is the one to delete */
            if (ChannelIBufferLength(CurrentChannel))
            {
                /* Also send two backspaces back to the console */
                SacPutString(L"\b \b");
                ChannelIReadLast(CurrentChannel);
            }

            /* Keep going */
            continue;
        }

        /* If the user pressed CTRL-C at this point, treat it like ENTER */
        if (Char == '\x03') goto DoLineParsing;

        /* Check if the user pressed TAB */
        if (Char == '\t')
        {
            /* Omit it, send a BELL, and keep going. We ignore TABs */
            ChannelIReadLast(CurrentChannel);
            SacPutString(L"\a");
            continue;
        }

        /* Check if the user is getting close to the end of the screen */
        if (ChannelIBufferLength(CurrentChannel) == (SAC_VTUTF8_COL_WIDTH - 2))
        {
            /* Delete the last character, replacing it with this one instead */
            swprintf(StringBuffer, L"\b%c", Char);
            SacPutString(StringBuffer);

            /* Omit the last two characters from the buffer */
            ChannelIReadLast(CurrentChannel);
            ChannelIReadLast(CurrentChannel);

            /* NULL-terminate it */
            WriteBuffer[0] = Char;
            ChannelIWrite(CurrentChannel, WriteBuffer, sizeof(CHAR));
            continue;
        }

        /* Nothing of interest happened, just write the character back */
        swprintf(StringBuffer, L"%c", Char);
        SacPutString(StringBuffer);
    }

    /* We're done, release the lock */
    SacReleaseMutexLock();
    SAC_DBG(SAC_DBG_MACHINE, "SAC TimerDpcRoutine: Exiting.\n"); //bug
}

VOID
NTAPI
ConMgrWorkerProcessEvents(IN PSAC_DEVICE_EXTENSION DeviceExtension)
{
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC WorkerProcessEvents: Entering.\n");

    /* Enter the main loop */
    while (TRUE)
    {
        /* Wait for something to do */
        KeWaitForSingleObject(&DeviceExtension->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        /* Consume data off the serial port */
        ConMgrSerialPortConsumer();
        switch (ExecutePostConsumerCommand)
        {
            case 1:
                /* A reboot was sent, do it  */
                DoRebootCommand(FALSE);
                break;

            case 2:
                /* A close was sent, do it */
                ChanMgrCloseChannel(ExecutePostConsumerCommandData);
                ChanMgrReleaseChannel(ExecutePostConsumerCommandData);
                break;

            case 3:
                /* A shutdown was sent, do it */
                DoRebootCommand(TRUE);
                break;
        }

        /* Clear the serial port consumer state */
        ExecutePostConsumerCommand = 0;
        ExecutePostConsumerCommandData = NULL;
    }
}

NTSTATUS
NTAPI
ConMgrGetChannelCloseMessage(IN PSAC_CHANNEL Channel,
                             IN NTSTATUS CloseStatus,
                             OUT PWCHAR OutputBuffer)
{
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
ConMgrHandleEvent(IN ULONG EventCode,
                  IN PSAC_CHANNEL Channel,
                  OUT PVOID Data)
{
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

