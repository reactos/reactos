/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            pic.c
 * PURPOSE:         Programmable Interrupt Controller emulation
 *                  (Interrupt Controller Adapter (ICA) in Windows terminology)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "pic.h"

#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

static PIC MasterPic, SlavePic;

/* PRIVATE FUNCTIONS **********************************************************/

static BYTE PicReadCommand(BYTE Port)
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
        /* Read the interrupt request register */
        return Pic->IntRequestRegister;
    }
}

static VOID PicWriteCommand(BYTE Port, BYTE Value)
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

        if (MasterPic.IntRequestRegister || SlavePic.IntRequestRegister)
        {
            /* Signal the next IRQ */
            EmulatorInterruptSignal();
        }
    }
}

static BYTE PicReadData(BYTE Port)
{
    /* Read the mask register */
    if (Port == PIC_MASTER_DATA) return MasterPic.MaskRegister;
    else return SlavePic.MaskRegister;
}

static VOID PicWriteData(BYTE Port, BYTE Value)
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

static BYTE WINAPI PicReadPort(USHORT Port)
{
    switch (Port)
    {
        case PIC_MASTER_CMD:
        case PIC_SLAVE_CMD:
        {
            return PicReadCommand(Port);
        }

        case PIC_MASTER_DATA:
        case PIC_SLAVE_DATA:
        {
            return PicReadData(Port);
        }
    }

    return 0;
}

static VOID WINAPI PicWritePort(USHORT Port, BYTE Data)
{
    switch (Port)
    {
        case PIC_MASTER_CMD:
        case PIC_SLAVE_CMD:
        {
            PicWriteCommand(Port, Data);
            break;
        }

        case PIC_MASTER_DATA:
        case PIC_SLAVE_DATA:
        {
            PicWriteData(Port, Data);
            break;
        }
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID PicInterruptRequest(BYTE Number)
{
    BYTE i;

    if (/* Number >= 0 && */ Number < 8)
    {
        /* Check if any of the higher-priority interrupts are busy */
        for (i = 0; i <= Number; i++)
        {
            if (MasterPic.InServiceRegister & (1 << Number)) return;
        }

        /* Check if the interrupt is masked */
        if (MasterPic.MaskRegister & (1 << Number)) return;

        /* Set the appropriate bit in the IRR and interrupt the CPU */
        MasterPic.IntRequestRegister |= 1 << Number;
        EmulatorInterruptSignal();
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

        /* Check if any of the higher-priority interrupts are busy */
        if (MasterPic.InServiceRegister != 0) return;
        for (i = 0; i <= Number; i++)
        {
            if (SlavePic.InServiceRegister & (1 << Number)) return;
        }

        /* Check if the interrupt is masked */
        if (SlavePic.MaskRegister & (1 << Number)) return;

        /* Set the IRQ 2 bit in the master ISR */
        if (!MasterPic.AutoEoi) MasterPic.InServiceRegister |= (1 << 2);

        /* Set the appropriate bit in the IRR and interrupt the CPU */
        SlavePic.IntRequestRegister |= 1 << Number;
        EmulatorInterruptSignal();
    }
}

BYTE PicGetInterrupt(VOID)
{
    INT i;

    /* Search the master PIC interrupts by priority */
    for (i = 0; i < 8; i++)
    {
        if (MasterPic.IntRequestRegister & (1 << i))
        {
            /* Clear the IRR flag */
            MasterPic.IntRequestRegister &= ~(1 << i);

            /* Set the ISR flag, unless AEOI is enabled */
            if (!MasterPic.AutoEoi) MasterPic.InServiceRegister |= (1 << i);

            /* Return the interrupt number */
            return MasterPic.IntOffset + i;
        }
    }

    /* Search the slave PIC interrupts by priority */
    for (i = 0; i < 8; i++)
    {
        if (SlavePic.IntRequestRegister & (1 << i))
        {
            /* Clear the IRR flag */
            SlavePic.IntRequestRegister &= ~(1 << i);

            if ((i == 1) && SlavePic.CascadeRegisterSet)
            {
                /* This interrupt is routed to the master PIC */
                return MasterPic.IntOffset + SlavePic.CascadeRegister;
            }
            else
            {
                /* Set the ISR flag, unless AEOI is enabled */
                if (!SlavePic.AutoEoi) SlavePic.InServiceRegister |= (1 << i);

                /* Return the interrupt number */
                return SlavePic.IntOffset + i;
            }
        }
    }
    
    /* Spurious interrupt */
    if (MasterPic.InServiceRegister & (1 << 2)) return SlavePic.IntOffset + 7;
    else return MasterPic.IntOffset + 7;
}

VOID PicInitialize(VOID)
{
    /* Register the I/O Ports */
    RegisterIoPort(PIC_MASTER_CMD , PicReadPort, PicWritePort);
    RegisterIoPort(PIC_SLAVE_CMD  , PicReadPort, PicWritePort);
    RegisterIoPort(PIC_MASTER_DATA, PicReadPort, PicWritePort);
    RegisterIoPort(PIC_SLAVE_DATA , PicReadPort, PicWritePort);
}



VOID
WINAPI
call_ica_hw_interrupt(INT  ms,
                      BYTE line,
                      INT  count)
{
    BYTE InterruptNumber = line;

    /* Check for PIC validity */
    if (ms != ICA_MASTER && ms != ICA_SLAVE) return;

    /*
     * Adjust the interrupt request number according to the parameters,
     * by adding an offset == 8 to the interrupt number.
     * 
     * Indeed VDDs calling this function usually subtracts 8 so that they give:
     *
     *      ms     |  line  | corresponding interrupt number
     * ------------+--------+--------------------------------
     *  ICA_MASTER | 0 -- 7 |            0 -- 7
     *  ICA_SLAVE  | 0 -- 7 |            8 -- 15
     *
     * and PicInterruptRequest subtracts again 8 to the interrupt number
     * if it is greater or equal than 8 (so that it determines which PIC
     * to use via the interrupt number).
     */
    if (ms == ICA_SLAVE) InterruptNumber += 8;

    /* Send the specified number of interrupt requests */
    while (count-- > 0)
    {
        PicInterruptRequest(InterruptNumber);
    }
}

WORD
WINAPI
VDDReserveIrqLine(IN HANDLE hVdd,
                  IN WORD   IrqLine)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0xFFFF;
}

BOOL
WINAPI
VDDReleaseIrqLine(IN HANDLE hVdd,
                  IN WORD   IrqLine)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/* EOF */
