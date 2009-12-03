/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kdbg/amd64/kdb.c
 * PURPOSE:         Kernel Debugger
 * PROGRAMMERS:     Gregor Anich
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern KSPIN_LOCK KdpSerialSpinLock;
STRING KdpPromptString = RTL_CONSTANT_STRING("kdb:> ");

/* GLOBALS *******************************************************************/

ULONG KdbDebugState = 0; /* KDBG Settings (NOECHO, KDSERIAL) */

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdbpGetCommandLineSettings(PCHAR p1)
{
    PCHAR p2;

    while (p1 && (p2 = strchr(p1, ' ')))
    {
        p2++;

        if (!_strnicmp(p2, "KDSERIAL", 8))
        {
            p2 += 8;
            KdbDebugState |= KD_DEBUG_KDSERIAL;
            KdpDebugMode.Serial = TRUE;
        }
        else if (!_strnicmp(p2, "KDNOECHO", 8))
        {
            p2 += 8;
            KdbDebugState |= KD_DEBUG_KDNOECHO;
        }

        p1 = p2;
    }
}

KD_CONTINUE_TYPE
KdbEnterDebuggerException(
   IN PEXCEPTION_RECORD ExceptionRecord  OPTIONAL,
   IN KPROCESSOR_MODE PreviousMode,
   IN PCONTEXT Context,
   IN OUT PKTRAP_FRAME TrapFrame,
   IN BOOLEAN FirstChance)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
KdbpCliModuleLoaded(IN PUNICODE_STRING Name)
{
    UNIMPLEMENTED;
}

VOID
KdbpCliInit()
{
    UNIMPLEMENTED;
}

ULONG
NTAPI
KdpPrompt(IN LPSTR InString,
          IN USHORT InStringLength,
          OUT LPSTR OutString,
          IN USHORT OutStringLength)
{
    USHORT i;
    CHAR Response;
    ULONG DummyScanCode;
    KIRQL OldIrql;

    /* Acquire the printing spinlock without waiting at raised IRQL */
    while (TRUE)
    {
        /* Wait when the spinlock becomes available */
        while (!KeTestSpinLock(&KdpSerialSpinLock));

        /* Spinlock was free, raise IRQL */
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);

        /* Try to get the spinlock */
        if (KeTryToAcquireSpinLockAtDpcLevel(&KdpSerialSpinLock))
            break;

        /* Someone else got the spinlock, lower IRQL back */
        KeLowerIrql(OldIrql);
    }

    /* Loop the string to send */
    for (i = 0; i < InStringLength; i++)
    {
        /* Print it to serial */
        KdPortPutByteEx(&SerialPortInfo, *(PCHAR)(InString + i));
    }

    /* Print a new line for log neatness */
    KdPortPutByteEx(&SerialPortInfo, '\r');
    KdPortPutByteEx(&SerialPortInfo, '\n');

    /* Print the kdb prompt */
    for (i = 0; i < KdpPromptString.Length; i++)
    {
        /* Print it to serial */
        KdPortPutByteEx(&SerialPortInfo,
                        *(KdpPromptString.Buffer + i));
    }

    /* Loop the whole string */
    for (i = 0; i < OutStringLength; i++)
    {
        /* Check if this is serial debugging mode */
        if (KdbDebugState & KD_DEBUG_KDSERIAL)
        {
            /* Get the character from serial */
            do
            {
                Response = KdbpTryGetCharSerial(MAXULONG);
            } while (Response == -1);
        }
        else
        {
            /* Get the response from the keyboard */
            do
            {
                Response = KdbpTryGetCharKeyboard(&DummyScanCode, MAXULONG);
            } while (Response == -1);
        }

        /* Check for return */
        if (Response == '\r')
        {
            /*
             * We might need to discard the next '\n'.
             * Wait a bit to make sure we receive it.
             */
            KeStallExecutionProcessor(100000);

            /* Check the mode */
            if (KdbDebugState & KD_DEBUG_KDSERIAL)
            {
                /* Read and discard the next character, if any */
                KdbpTryGetCharSerial(5);
            }
            else
            {
                /* Read and discard the next character, if any */
                KdbpTryGetCharKeyboard(&DummyScanCode, 5);
            }

            /* 
             * Null terminate the output string -- documentation states that
             * DbgPrompt does not null terminate, but it does
             */
            *(PCHAR)(OutString + i) = 0;

            /* Print a new line */
            KdPortPutByteEx(&SerialPortInfo, '\r');
            KdPortPutByteEx(&SerialPortInfo, '\n');         

            /* Release spinlock */
            KiReleaseSpinLock(&KdpSerialSpinLock);

            /* Lower IRQL back */
            KeLowerIrql(OldIrql);

            /* Return the length  */
            return OutStringLength + 1;
        }

        /* Write it back and print it to the log */
        *(PCHAR)(OutString + i) = Response;
        KdPortPutByteEx(&SerialPortInfo, Response);
    }

    /* Print a new line */
    KdPortPutByteEx(&SerialPortInfo, '\r');
    KdPortPutByteEx(&SerialPortInfo, '\n');

    /* Release spinlock */
    KiReleaseSpinLock(&KdpSerialSpinLock);

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);

    /* Return the length  */
    return OutStringLength;
}
