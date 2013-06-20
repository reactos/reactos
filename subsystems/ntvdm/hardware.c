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

static PIC MasterPic, SlavePic;

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
    if (Number >= 0 && Number < 8)
    {
        /* Check if the interrupt is busy or in a cascade */
        if (MasterPic.CascadeRegister & (1 << Number)
            || MasterPic.InServiceRegister & (1 << Number))
        {
            return;
        }
        
        MasterPic.InServiceRegister |= 1 << Number;
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

        /* Check the if the slave PIC is busy */
        if (MasterPic.InServiceRegister & (1 << 2)) return;

        /* Set the IRQ 2 bit in the master ISR */
        MasterPic.InServiceRegister |= 1 << 2;
        
        /* Check if the interrupt is busy or in a cascade */
        if (SlavePic.CascadeRegister & (1 << Number)
            || SlavePic.InServiceRegister & (1 << Number))
        {
            return;
        }
        
        SlavePic.InServiceRegister |= 1 << Number;
        EmulatorInterrupt(SlavePic.IntOffset + Number);
    }
}
