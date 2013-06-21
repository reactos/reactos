/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            hardware.c
 * PURPOSE:         Minimal hardware emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

typedef struct _PIC
{
    BOOLEAN Initialization;
    BYTE MaskRegister;
    BYTE InServiceRegister;
    BYTE IntOffset;
    BYTE ConfigRegister;
    BYTE CascadeRegister;
    BOOLEAN CascadeRegisterSet;
    BOOLEAN AutoEoi;
    BOOLEAN Slave;
    BOOLEAN ReadIsr;
} PIC, *PPIC;

enum
{
    PIT_MODE_INT_ON_TERMINAL_COUNT,
    PIT_MODE_HARDWARE_ONE_SHOT,
    PIT_MODE_RATE_GENERATOR,
    PIT_MODE_SQUARE_WAVE,
    PIT_MODE_SOFTWARE_STROBE,
    PIT_MODE_HARDWARE_STROBE
};

typedef struct _PIT_CHANNEL
{
    WORD ReloadValue;
    WORD CurrentValue;
    WORD LatchedValue;
    INT Mode;
    BOOLEAN Pulsed;
    BOOLEAN LatchSet;
    BOOLEAN InputFlipFlop;
    BOOLEAN OutputFlipFlop;
    BYTE AccessMode;
} PIT_CHANNEL, *PPIT_CHANNEL;

static PIC MasterPic, SlavePic;
static PIT_CHANNEL PitChannels[PIT_CHANNELS];

/* PUBLIC FUNCTIONS ***********************************************************/

BYTE PicReadCommand(BYTE Port)
{
    PPIC Pic;
    
    /* Which PIC are we accessing? */
    if (Port == PIC_MASTER_CMD) Pic = &MasterPic;
    else Pic = &SlavePic;
    
    if (Pic->ReadIsr)
    {
        /* Read the in-service register */
        Pic->ReadIsr = FALSE;
        return Pic->InServiceRegister;
    }
    else
    {
        /* The IRR is always 0, as the emulated CPU receives the interrupt instantly */
        return 0;
    }
}

VOID PicWriteCommand(BYTE Port, BYTE Value)
{
    PPIC Pic;
    
    /* Which PIC are we accessing? */
    if (Port == PIC_MASTER_CMD) Pic = &MasterPic;
    else Pic = &SlavePic;
    
    if (Value & PIC_ICW1)
    {
        /* Start initialization */
        Pic->Initialization = TRUE;
        Pic->IntOffset = 0xFF;
        Pic->CascadeRegisterSet = FALSE;
        Pic->ConfigRegister = Value;
        return;
    }
    
    if (Value & PIC_OCW3)
    {
        /* This is an OCR3 */
        if (Value == PIC_OCW3_READ_ISR)
        {
            /* Return the ISR on next read from command port */
            Pic->ReadIsr = TRUE;
        }
        
        return;
    }
    
    /* This is an OCW2 */
    if (Value & PIC_OCW2_EOI)
    {
        if (Value & PIC_OCW2_SL)
        {
            /* If the SL bit is set, clear a specific IRQ */
            Pic->InServiceRegister &= ~(1 << (Value & PIC_OCW2_NUM_MASK));
        }
        else
        {
            /* Otherwise, clear all of them */
            Pic->InServiceRegister = 0;
        }
    }
}

BYTE PicReadData(BYTE Port)
{
    /* Read the mask register */
    if (Port == PIC_MASTER_DATA) return MasterPic.MaskRegister;
    else return SlavePic.MaskRegister;
}

VOID PicWriteData(BYTE Port, BYTE Value)
{
    PPIC Pic;
    
    /* Which PIC are we accessing? */
    if (Port == PIC_MASTER_DATA) Pic = &MasterPic;
    else Pic = &SlavePic;
    
    /* Is the PIC ready? */
    if (!Pic->Initialization)
    {
        /* Yes, this is an OCW1 */
        Pic->MaskRegister = Value;
        return;
    }
    
    /* Has the interrupt offset been set? */
    if (Pic->IntOffset == 0xFF)
    {
        /* This is an ICW2, set the offset (last three bits always zero) */
        Pic->IntOffset = Value & 0xF8;
        
        /* Check if we are in single mode and don't need an ICW4 */
        if ((Pic->ConfigRegister & PIC_ICW1_SINGLE)
            && !(Pic->ConfigRegister & PIC_ICW1_ICW4))
        {
            /* Yes, done initializing */
            Pic->Initialization = FALSE;
        }
        return;
    }
    
    /* Check if we are in cascade mode and the cascade register was not set */
    if (!(Pic->ConfigRegister & PIC_ICW1_SINGLE) && !Pic->CascadeRegisterSet)
    {
        /* This is an ICW3 */
        Pic->CascadeRegister = Value;
        Pic->CascadeRegisterSet = TRUE;
        
        /* Check if we need an ICW4 */
        if (!(Pic->ConfigRegister & PIC_ICW1_ICW4))
        {
            /* No, done initializing */
            Pic->Initialization = FALSE;
        }
        return;
    }
    
    /* This must be an ICW4, we will ignore the 8086 bit (assume always set) */
    if (Value & PIC_ICW4_AEOI)
    {
        /* Use automatic end-of-interrupt */
        Pic->AutoEoi = TRUE;
    }
    
    /* Done initializing */
    Pic->Initialization = FALSE;
}

VOID PicInterruptRequest(BYTE Number)
{    
    BYTE i;
    
    if (Number >= 0 && Number < 8)
    {
        /* Check if any of the higher-priorirty interrupts are busy */
        for (i = 0; i <= Number ; i++)
        {
            if (MasterPic.InServiceRegister & (1 << Number)) return;
        }
        
        /* Check if the interrupt is masked */
        if (MasterPic.MaskRegister & (1 << Number)) return;
        
        /* Set the appropriate bit in the ISR and interrupt the CPU */
        if (!MasterPic.AutoEoi) MasterPic.InServiceRegister |= 1 << Number;
        EmulatorInterrupt(MasterPic.IntOffset + Number);
    }
    else if (Number >= 8 && Number < 16)
    {
        Number -= 8;
        
        /*
         * The slave PIC is connected to IRQ 2, always! If the master PIC
         * was misconfigured, don't do anything.
         */
        if (!(MasterPic.CascadeRegister & (1 << 2))
            || SlavePic.CascadeRegister != 2)
        {
            return;
        }
        
        /* Check if any of the higher-priorirty interrupts are busy */
        if (MasterPic.InServiceRegister != 0) return;
        for (i = 0; i <= Number ; i++)
        {
            if (SlavePic.InServiceRegister & (1 << Number)) return;
        }
        
        /* Check if the interrupt is masked */
        if (SlavePic.MaskRegister & (1 << Number)) return;

        /* Set the IRQ 2 bit in the master ISR */
        if (!MasterPic.AutoEoi) MasterPic.InServiceRegister |= 1 << 2;
    
        /* Set the appropriate bit in the ISR and interrupt the CPU */
        if (!SlavePic.AutoEoi) SlavePic.InServiceRegister |= 1 << Number;
        EmulatorInterrupt(SlavePic.IntOffset + Number);
    }
}

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

VOID PitDecrementCount()
{
    INT i;
    
    for (i = 0; i < PIT_CHANNELS; i++)
    {
        switch (PitChannels[i].Mode)
        {
            case PIT_MODE_INT_ON_TERMINAL_COUNT:
            {
                /* Decrement the value */
                PitChannels[i].CurrentValue--;
                
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
                /* Decrement the value */
                PitChannels[i].CurrentValue--;
                
                /* Did it fall to zero? */
                if (PitChannels[i].CurrentValue != 0) break;

                /* Yes, raise the output line and reload */
                if (i == 0) PicInterruptRequest(0);
                PitChannels[i].CurrentValue = PitChannels[i].ReloadValue;
                
                break;
            }
            
            case PIT_MODE_SQUARE_WAVE:
            {
                /* Decrement the value by 2 */
                PitChannels[i].CurrentValue -= 2;
                
                /* Did it fall to zero? */
                if (PitChannels[i].CurrentValue != 0) break;
                
                /* Yes, toggle the flip-flop */
                PitChannels[i].OutputFlipFlop = !PitChannels[i].OutputFlipFlop;
                    
                /* Did this create a rising edge in the signal? */
                if (PitChannels[i].OutputFlipFlop)
                {
                    /* Yes, IRQ 0 if this is channel 0 */
                    if (i == 0) PicInterruptRequest(0);
                }
                    
                /* Reload the value, but make sure it's even */
                if (PitChannels[i].ReloadValue % 2)
                {
                    /* It's odd, reduce it by 1 */
                    PitChannels[i].CurrentValue = PitChannels[i].ReloadValue - 1;
                }
                else
                {
                    /* It was even */
                    PitChannels[i].CurrentValue = PitChannels[i].ReloadValue;
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

