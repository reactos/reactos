/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            timer.c
 * PURPOSE:         Programmable Interval Timer emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "timer.h"
#include "pic.h"

/* PRIVATE VARIABLES **********************************************************/

static PIT_CHANNEL PitChannels[PIT_CHANNELS];

/* PUBLIC FUNCTIONS ***********************************************************/

VOID PitWriteCommand(BYTE Value)
{
    BYTE Channel = Value >> 6;
    BYTE Mode = (Value >> 1) & 0x07;

    /* Check if this is a counter latch command */
    if (((Value >> 4) & 3) == 0)
    {
        PitChannels[Channel].LatchSet = TRUE;
        PitChannels[Channel].LatchedValue = PitChannels[Channel].CurrentValue;
        return;
    }

    /* Set the access mode and reset flip-flops */
    PitChannels[Channel].AccessMode = (Value >> 4) & 3;
    PitChannels[Channel].Pulsed = FALSE;
    PitChannels[Channel].LatchSet = FALSE;
    PitChannels[Channel].InputFlipFlop = FALSE;
    PitChannels[Channel].OutputFlipFlop = FALSE;

    switch (Mode)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        {
            PitChannels[Channel].Mode = Mode;
            break;
        }

        case 6:
        {
            PitChannels[Channel].Mode = PIT_MODE_RATE_GENERATOR;
            break;
        }

        case 7:
        {
            PitChannels[Channel].Mode = PIT_MODE_SQUARE_WAVE;
            break;
        }
    }
}

BYTE PitReadData(BYTE Channel)
{
    WORD CurrentValue = PitChannels[Channel].CurrentValue;
    BYTE AccessMode = PitChannels[Channel].AccessMode;

    /* Check if the value was latched */
    if (PitChannels[Channel].LatchSet)
    {
        CurrentValue = PitChannels[Channel].LatchedValue;

        if (AccessMode == 1 || AccessMode == 2)
        {
            /* The latched value was read as one byte */
            PitChannels[Channel].LatchSet = FALSE;
        }
    }

    /* Use the flip-flop for access mode 3 */
    if (AccessMode == 3)
    {
        AccessMode = PitChannels[Channel].InputFlipFlop ? 1 : 2;
        PitChannels[Channel].InputFlipFlop = !PitChannels[Channel].InputFlipFlop;

        /* Check if this was the last read for the latched value */
        if (!PitChannels[Channel].InputFlipFlop)
        {
            /* Yes, the latch value was read as two bytes */
            PitChannels[Channel].LatchSet = FALSE;
        }
    }

    switch (AccessMode)
    {
        case 1:
        {
            /* Low byte */
            return CurrentValue & 0x00FF;
        }

        case 2:
        {
            /* High byte */
            return CurrentValue >> 8;
        }
    }

    /* Shouldn't get here */
    return 0;
}

VOID PitWriteData(BYTE Channel, BYTE Value)
{
    BYTE AccessMode = PitChannels[Channel].AccessMode;

    /* Use the flip-flop for access mode 3 */
    if (PitChannels[Channel].AccessMode == 3)
    {
        AccessMode = PitChannels[Channel].InputFlipFlop ? 1 : 2;
        PitChannels[Channel].InputFlipFlop = !PitChannels[Channel].InputFlipFlop;
    }

    switch (AccessMode)
    {
        case 1:
        {
            /* Low byte */
            PitChannels[Channel].ReloadValue &= 0xFF00;
            PitChannels[Channel].ReloadValue |= Value;
            break;
        }

        case 2:
        {
            /* High byte */
            PitChannels[Channel].ReloadValue &= 0x00FF;
            PitChannels[Channel].ReloadValue |= Value << 8;
        }
    }
}

VOID PitDecrementCount(DWORD Count)
{
    INT i;

    for (i = 0; i < PIT_CHANNELS; i++)
    {
        switch (PitChannels[i].Mode)
        {
            case PIT_MODE_INT_ON_TERMINAL_COUNT:
            {
                /* Decrement the value */
                if (Count > PitChannels[i].CurrentValue)
                {
                    /* The value does not reload in this case */
                    PitChannels[i].CurrentValue = 0;
                }
                else PitChannels[i].CurrentValue -= Count;

                /* Did it fall to the terminal count? */
                if (PitChannels[i].CurrentValue == 0 && !PitChannels[i].Pulsed)
                {
                    /* Yes, raise the output line */
                    if (i == 0) PicInterruptRequest(0);
                    PitChannels[i].Pulsed = TRUE;
                }
                break;
            }

            case PIT_MODE_RATE_GENERATOR:
            {
                BOOLEAN Reloaded = FALSE;

                while (Count)
                {
                    if ((Count > PitChannels[i].CurrentValue)
                        && (PitChannels[i].CurrentValue != 0))
                    {
                        /* Decrease the count */
                        Count -= PitChannels[i].CurrentValue;

                        /* Reload the value */
                        PitChannels[i].CurrentValue = PitChannels[i].ReloadValue;

                        /* Set the flag */
                        Reloaded = TRUE;
                    }
                    else
                    {
                        /* Decrease the value */
                        PitChannels[i].CurrentValue -= Count;

                        /* Clear the count */
                        Count = 0;

                        /* Did it fall to zero? */
                        if (PitChannels[i].CurrentValue == 0)
                        {
                            PitChannels[i].CurrentValue = PitChannels[i].ReloadValue;
                            Reloaded = TRUE;
                        }
                    }
                }

                /* If there was a reload on channel 0, raise IRQ 0 */
                if ((i == 0) && Reloaded) PicInterruptRequest(0);

                break;
            }

            case PIT_MODE_SQUARE_WAVE:
            {
                INT ReloadCount = 0;
                WORD ReloadValue = PitChannels[i].ReloadValue;

                /* The reload value must be even */
                ReloadValue &= ~1;

                while (Count)
                {
                    if (((Count * 2) > PitChannels[i].CurrentValue)
                        && (PitChannels[i].CurrentValue != 0))
                    {
                        /* Decrease the count */
                        Count -= PitChannels[i].CurrentValue / 2;

                        /* Reload the value */
                        PitChannels[i].CurrentValue = ReloadValue;

                        /* Increment the reload count */
                        ReloadCount++;
                    }
                    else
                    {
                        /* Clear the count */
                        Count = 0;

                        /* Decrease the value */
                        PitChannels[i].CurrentValue -= Count * 2;

                        /* Did it fall to zero? */
                        if (PitChannels[i].CurrentValue == 0)
                        {
                            /* Reload the value */
                            PitChannels[i].CurrentValue = ReloadValue;

                            /* Increment the reload count */
                            ReloadCount++;
                        }
                    }
                }

                if (ReloadCount == 0) break;

                /* Toggle the flip-flop if the number of reloads was odd */
                if (ReloadCount & 1)
                {
                    PitChannels[i].OutputFlipFlop = !PitChannels[i].OutputFlipFlop;
                }

                /* Was there any rising edge on channel 0 ? */
                if ((PitChannels[i].OutputFlipFlop || ReloadCount) && (i == 0))
                {
                    /* Yes, IRQ 0 */
                    PicInterruptRequest(0);
                }

                break;
            }

            case PIT_MODE_SOFTWARE_STROBE:
            {
                // TODO: NOT IMPLEMENTED
                break;
            }

            case PIT_MODE_HARDWARE_ONE_SHOT:
            case PIT_MODE_HARDWARE_STROBE:
            {
                /* These modes do not work on x86 PCs */
                break;
            }
        }
    }
}

DWORD PitGetResolution(VOID)
{
    INT i;
    DWORD MinReloadValue = 65536;

    for (i = 0; i < PIT_CHANNELS; i++)
    {
        DWORD ReloadValue = PitChannels[i].ReloadValue;

        /* 0 means 65536 */
        if (ReloadValue == 0) ReloadValue = 65536;

        if (ReloadValue < MinReloadValue) MinReloadValue = ReloadValue;
    }

    /* Return the frequency resolution */
    return PIT_BASE_FREQUENCY / MinReloadValue;
}

/* EOF */
