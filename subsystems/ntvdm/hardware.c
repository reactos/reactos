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

typedef struct _PIT_CHANNEL
{
    BOOLEAN RateGenerator;
    BOOLEAN Pulsed;
    BOOLEAN FlipFlop;
    BYTE AccessMode;
    WORD ReloadValue;
} PIT_CHANNEL, *PPIT_CHANNEL;

static PIC MasterPic, SlavePic;
static PIT_CHANNEL PitChannels[PIT_CHANNELS];

static DWORD WINAPI PitThread(PVOID Parameter)
{
    LARGE_INTEGER Frequency, CurrentTime, LastTickTime;
    LONGLONG Elapsed, Milliseconds, TicksNeeded;
    UNREFERENCED_PARAMETER(Parameter);
    
    /* Get the performance counter frequency */
    if (!QueryPerformanceFrequency(&Frequency)) return EXIT_FAILURE;
    if (!QueryPerformanceCounter(&LastTickTime)) return EXIT_FAILURE;
    
    while (VdmRunning)
    {
        if (!QueryPerformanceCounter(&CurrentTime)) return EXIT_FAILURE;
        
        /* Calculate the elapsed time, in PIT ticks */
        Elapsed = ((CurrentTime.QuadPart - LastTickTime.QuadPart)
                  * PIT_BASE_FREQUENCY)
                  / Frequency.QuadPart;
                  
        /* A reload value of 0 indicates 65536 */
        if (PitChannels[0].ReloadValue) TicksNeeded = PitChannels[0].ReloadValue;
        else TicksNeeded = 65536;

        if (Elapsed < TicksNeeded)
        {
            /* Get the number of milliseconds */
            Milliseconds = (Elapsed * 1000LL) / PIT_BASE_FREQUENCY;
            
            /* If this number is non-zero, put the thread in the waiting state */
            if (Milliseconds > 0LL) Sleep((DWORD)Milliseconds);
            
            continue;
        }
        
        LastTickTime = CurrentTime;
        
        /* Do the IRQ */
        if (PitChannels[0].RateGenerator || !PitChannels[0].Pulsed)
        {
            PitChannels[0].Pulsed = TRUE;
            PicInterruptRequest(0);
        }
    }
    
    return EXIT_SUCCESS;
}

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
    
    /* Set the access mode and reset flip-flop */
    // TODO: Support latch command!
    PitChannels[Channel].AccessMode = (Value >> 4) & 3;
    PitChannels[Channel].FlipFlop = FALSE;
    
    switch (Mode)
    {
        case 0:
        case 4:
        {
            PitChannels[Channel].RateGenerator = FALSE;
            break;
        }
            
        case 2:
        case 3:
        {
            PitChannels[Channel].RateGenerator = TRUE;
            break;
        }
    }
}

VOID PitWriteData(BYTE Channel, BYTE Value)
{
    /* Use the flip-flop for access mode 3 */
    if (PitChannels[Channel].AccessMode == 3)
    {
        PitChannels[Channel].AccessMode = PitChannels[Channel].FlipFlop ? 1 : 2;
        PitChannels[Channel].FlipFlop = !PitChannels[Channel].FlipFlop;
    }
    
    switch (PitChannels[Channel].AccessMode)
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

VOID PitInitialize()
{
    HANDLE ThreadHandle;
    
    /* Set up channel 0 */
    PitChannels[0].ReloadValue = 0;
    PitChannels[0].RateGenerator = TRUE;
    PitChannels[0].Pulsed = FALSE;
    PitChannels[0].AccessMode = 3;
    PitChannels[0].FlipFlop = FALSE;

    /* Create the PIT timer thread */
    ThreadHandle = CreateThread(NULL, 0, PitThread, NULL, 0, NULL);
    
    /* We don't need the handle */
    CloseHandle(ThreadHandle);
}
