/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios32/vbe.c
 * PURPOSE:         VDM VESA BIOS Extensions (for the Cirrus CL-GD5434 emulated card)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "cpu/cpu.h"
#include "bios32p.h"
#include "hardware/video/svga.h"

#include "vbe.h"

#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

static const VBE_MODE Modes[VBE_MODE_COUNT] = {
    { 0x00, 0x00,   NULL /* TODO */, NULL /* VGA */  },
    { 0x01, 0x01,   NULL /* TODO */, NULL /* VGA */  },
    { 0x02, 0x02,   NULL /* TODO */, NULL /* VGA */  },
    { 0x03, 0x03,   NULL /* TODO */, NULL /* VGA */  },
    { 0x04, 0x04,   NULL /* TODO */, NULL /* VGA */  },
    { 0x05, 0x05,   NULL /* TODO */, NULL /* VGA */  },
    { 0x06, 0x06,   NULL /* TODO */, NULL /* VGA */  },
    { 0x07, 0x07,   NULL /* TODO */, NULL /* VGA */  },
    { 0x0D, 0x0D,   NULL /* TODO */, NULL /* VGA */  },
    { 0x0E, 0x0E,   NULL /* TODO */, NULL /* VGA */  },
    { 0x0F, 0x0F,   NULL /* TODO */, NULL /* VGA */  },
    { 0x10, 0x10,   NULL /* TODO */, NULL /* VGA */  },
    { 0x11, 0x11,   NULL /* TODO */, NULL /* VGA */  },
    { 0x12, 0x12,   NULL /* TODO */, NULL /* VGA */  },
    { 0x13, 0x13,   NULL /* TODO */, NULL /* VGA */  },
    { 0x14, 0xFFFF, NULL /* TODO */, NULL            },
    { 0x54, 0x10A,  NULL /* TODO */, NULL /* TODO */ },
    { 0x55, 0x109,  NULL /* TODO */, NULL /* TODO */ },
    { 0x58, 0x102,  NULL /* TODO */, NULL /* TODO */ },
    { 0x5C, 0x103,  NULL /* TODO */, NULL /* TODO */ },
    { 0x5D, 0x104,  NULL /* TODO */, NULL /* TODO */ },
    { 0x5E, 0x100,  NULL /* TODO */, NULL /* TODO */ },
    { 0x5F, 0x101,  NULL /* TODO */, NULL /* TODO */ },
    { 0x60, 0x105,  NULL /* TODO */, NULL /* TODO */ },
    { 0x64, 0x111,  NULL /* TODO */, NULL /* TODO */ },
    { 0x65, 0x114,  NULL /* TODO */, NULL /* TODO */ },
    { 0x66, 0x110,  NULL /* TODO */, NULL /* TODO */ },
    { 0x67, 0x113,  NULL /* TODO */, NULL /* TODO */ },
    { 0x68, 0x116,  NULL /* TODO */, NULL /* TODO */ },
    { 0x69, 0x119,  NULL /* TODO */, NULL /* TODO */ },
    { 0x6C, 0x106,  NULL /* TODO */, NULL /* TODO */ },
    { 0x6D, 0x107,  NULL /* TODO */, NULL /* TODO */ },
    { 0x71, 0x112,  NULL /* TODO */, NULL /* TODO */ },
    { 0x72, 0xFFFF, NULL /* TODO */, NULL            },
    { 0x73, 0xFFFF, NULL /* TODO */, NULL            },
    { 0x74, 0x117,  NULL /* TODO */, NULL /* TODO */ },
    { 0x75, 0x11A,  NULL /* TODO */, NULL /* TODO */ },
    { 0x76, 0xFFFF, NULL /* TODO */, NULL            },
    { 0x78, 0x115,  NULL /* TODO */, NULL /* TODO */ },
    { 0x79, 0x118,  NULL /* TODO */, NULL /* TODO */ },
};

/* PRIVATE FUNCTIONS **********************************************************/

PCVBE_MODE VbeGetModeByNumber(WORD Number)
{
    INT i;

    Number &= 0x1FF;

    /* Find the mode */
    for (i = 0; i < VBE_MODE_COUNT; i++)
    {
        if ((!(Number & 0x100) && (Number == Modes[i].Number))
            || ((Number & 0x100) && (Number== Modes[i].VesaNumber)))
        {
            return &Modes[i];
        }
    }

    return NULL;
}

/* This function is based on VgaSetRegisters in vidbios.c */
static VOID VbeSetExtendedRegisters(PSVGA_REGISTERS Registers)
{
    UINT i;

    /* Disable interrupts */
    BOOLEAN Interrupts = getIF();
    setIF(0);

    /*
     * Set the CRT base address according to the selected mode,
     * monochrome or color. The following macros:
     * VGA_INSTAT1_READ, VGA_CRTC_INDEX and VGA_CRTC_DATA are then
     * used to access the correct VGA I/O ports.
     */
    Bda->CrtBasePort = (Registers->Misc & 0x01) ? VGA_CRTC_INDEX_COLOR
                                                : VGA_CRTC_INDEX_MONO;
    /* Bit 1 indicates whether display is color (0) or monochrome (1) */
    Bda->VGAOptions     = (Bda->VGAOptions     & 0xFD) | (!(Registers->Misc & 0x01) << 1);
    Bda->CrtModeControl = (Bda->CrtModeControl & 0xFB) | (!(Registers->Misc & 0x01) << 1);

    /* Update blink bit in BDA */
    if (Registers->Attribute[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_BLINK)
        Bda->CrtModeControl |= (1 << 5);
    else
        Bda->CrtModeControl &= ~(1 << 5);

    /* Turn the video off */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_CLOCK_REG);
    IOWriteB(VGA_SEQ_DATA , IOReadB(VGA_SEQ_DATA) | VGA_SEQ_CLOCK_SD);

    /* Write the misc register */
    IOWriteB(VGA_MISC_WRITE, Registers->Misc);

    /* Synchronous reset on */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_RESET_REG);
    IOWriteB(VGA_SEQ_DATA , VGA_SEQ_RESET_AR );

    /* Write the sequencer registers */
    for (i = 1; i < SVGA_SEQ_MAX_REG; i++)
    {
        if (i != VGA_SEQ_MAX_REG && i != SVGA_SEQ_UNLOCK_REG)
        {
            IOWriteB(VGA_SEQ_INDEX, i);
            IOWriteB(VGA_SEQ_DATA , Registers->Sequencer[i]);
        }
    }

    /* Synchronous reset off */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_RESET_REG);
    IOWriteB(VGA_SEQ_DATA , VGA_SEQ_RESET_SR | VGA_SEQ_RESET_AR);

    /* Unlock CRTC registers 0-7 */
    IOWriteB(VGA_CRTC_INDEX, VGA_CRTC_END_HORZ_BLANKING_REG);
    IOWriteB(VGA_CRTC_DATA , IOReadB(VGA_CRTC_DATA) | 0x80);
    IOWriteB(VGA_CRTC_INDEX, VGA_CRTC_VERT_RETRACE_END_REG);
    IOWriteB(VGA_CRTC_DATA , IOReadB(VGA_CRTC_DATA) & ~0x80);
    // Make sure they remain unlocked
    Registers->CRT[VGA_CRTC_END_HORZ_BLANKING_REG] |= 0x80;
    Registers->CRT[VGA_CRTC_VERT_RETRACE_END_REG] &= ~0x80;

    /* Write the CRTC registers */
    for (i = 0; i < SVGA_CRTC_MAX_REG; i++)
    {
        if ((i < SVGA_CRTC_UNUSED0_REG || i > SVGA_CRTC_UNUSED6_REG) && i != SVGA_CRTC_UNUSED7_REG)
        {
            IOWriteB(VGA_CRTC_INDEX, i);
            IOWriteB(VGA_CRTC_DATA , Registers->CRT[i]);
        }
    }

    /* Write the GC registers */
    for (i = 0; i < SVGA_GC_MAX_REG; i++)
    {
        if (i != SVGA_GC_UNUSED0_REG && i != SVGA_GC_UNUSED11_REG
            && (i < SVGA_GC_UNUSED1_REG || i > SVGA_GC_UNUSED10_REG))
        {
            IOWriteB(VGA_GC_INDEX, i);
            IOWriteB(VGA_GC_DATA , Registers->Graphics[i]);
        }
    }

    /* Write the AC registers */
    for (i = 0; i < VGA_AC_MAX_REG; i++)
    {
        /* Write the index */
        IOReadB(VGA_INSTAT1_READ); // Put the AC register into index state
        IOWriteB(VGA_AC_INDEX, i);

        /* Write the data */
        IOWriteB(VGA_AC_WRITE, Registers->Attribute[i]);
    }

    /* Perform 4 dummy reads from the DAC mask to access the hidden register */
    for (i = 0; i < 4; i++) IOReadB(VGA_DAC_MASK);

    /* Set the hidden register */
    IOWriteB(VGA_DAC_MASK, Registers->Hidden);

    /* Set the PEL mask */
    IOWriteB(VGA_DAC_MASK, 0xFF);

    /* Enable screen and disable palette access */
    IOReadB(VGA_INSTAT1_READ); // Put the AC register into index state
    IOWriteB(VGA_AC_INDEX, 0x20);

    /* Turn the video on */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_CLOCK_REG);
    IOWriteB(VGA_SEQ_DATA , IOReadB(VGA_SEQ_DATA) & ~VGA_SEQ_CLOCK_SD);

    /* Restore interrupts */
    setIF(Interrupts);
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN WINAPI VbeSetExtendedVideoMode(BYTE ModeNumber)
{
    PCVBE_MODE Mode = VbeGetModeByNumber(ModeNumber);

    /* At this point, Mode->Registers shouldn't be NULL unless the mode is unimplemented */
    if (Mode->Registers == NULL)
    {
        DPRINT1("Extended video mode %02X still UNIMPLEMENTED.\n", ModeNumber);
        return FALSE;
    }

    /* Set the registers */
    VbeSetExtendedRegisters(Mode->Registers);

    /* Update the current video mode in the BDA */
    Bda->VideoMode = ModeNumber;

    return TRUE;
}

VOID WINAPI VbeResetExtendedRegisters(VOID)
{
    BYTE i;

    /* Disable interrupts */
    BOOLEAN Interrupts = getIF();
    setIF(0);

    /* Reset the extended sequencer registers */
    for (i = SVGA_SEQ_EXT_MODE_REG; i < SVGA_SEQ_MAX_REG; i++)
    {
        if (i != VGA_SEQ_MAX_REG && i != SVGA_SEQ_UNLOCK_REG)
        {
            IOWriteB(VGA_SEQ_INDEX, i);
            IOWriteB(VGA_SEQ_DATA, 0x00);
        }
    }

    /* Reset the extended CRTC registers */
    for (i = SVGA_CRTC_INTERLACE_END_REG; i < SVGA_CRTC_MAX_REG; i++)
    {
        if ((i < SVGA_CRTC_UNUSED0_REG || i > SVGA_CRTC_UNUSED6_REG) && i != SVGA_CRTC_UNUSED7_REG)
        {
            IOWriteB(VGA_CRTC_INDEX, i);
            IOWriteB(VGA_CRTC_DATA, 0x00);
        }
    }

    /* Reset the extended GC registers */
    for (i = SVGA_GC_OFFSET_0_REG; i < SVGA_GC_MAX_REG; i++)
    {
        if (i != SVGA_GC_UNUSED0_REG && i != SVGA_GC_UNUSED11_REG
            && (i < SVGA_GC_UNUSED1_REG || i > SVGA_GC_UNUSED10_REG))
        {
            IOWriteB(VGA_GC_INDEX, i);
            IOWriteB(VGA_GC_DATA, 0x00);
        }
    }

    /*
     * And finally, reset the hidden register. This requires 4 dummy reads from
     * the DAC mask register.
     */
    for (i = 0; i < 4; i++) IOReadB(VGA_DAC_MASK);
    IOWriteB(VGA_DAC_MASK, 0x00);

    /* Restore interrupts */
    setIF(Interrupts);
}

VOID WINAPI VbeService(LPWORD Stack)
{
    INT i;

    switch (getAL())
    {
        /* Get VBE Information */
        case 0x00:
        {
            VBE_INFORMATION Info;
            PWORD Data = (PWORD)&Info;

            /* Function recognized */
            setAL(0x4F);

            ZeroMemory(&Info, sizeof(VBE_INFORMATION));
            Info.Signature = 'ASEV';
            Info.Version = 0x0102;
            Info.OemName = OEM_NAME_PTR;
            Info.Capabilities = 0;
            Info.ModeList = MAKELONG(LOWORD(getDI()
                                     + FIELD_OFFSET(VBE_INFORMATION, ModeListBuffer)),
                                     getES());
            Info.VideoMemory = HIWORD(SVGA_BANK_SIZE * VGA_NUM_BANKS);

            /* Fill the mode list */
            for (i = 0; i < VBE_MODE_COUNT; i++)
            {
                /* Some modes don't have VESA numbers */
                if (Modes[i].VesaNumber != 0xFFFF)
                {
                    Info.ModeListBuffer[i] = Modes[i].VesaNumber;
                }
            }

            Info.ModeListBuffer[VBE_MODE_COUNT] = 0xFFFF;

            /* Copy the data to the caller */
            for (i = 0; i < sizeof(VBE_INFORMATION) / sizeof(WORD); i++)
            {
                *(PWORD)SEG_OFF_TO_PTR(getES(), LOWORD(getDI() + i * 2)) = Data[i];
            }

            setAH(0);
            break;
        }

        /* Get VBE Mode Information */
        case 0x01:
        {
            PCVBE_MODE Mode = VbeGetModeByNumber(getCX());
            PWORD Data = NULL;

            /* Function recognized */
            setAL(0x4F);

            if (Mode == NULL)
            {
                /* Mode not found */
                setAH(1);
                break;
            }

            Data = (PWORD)Mode->Info;
            if (Data == NULL)
            {
                DPRINT1("WARNING: The mode information for mode %02X (%03X) is missing!\n",
                        Mode->Number,
                        Mode->VesaNumber);

                setAH(1);
                break;
            }

            /* Clear the buffer */
            for (i = 0; i < 128; i++)
            {
                *(PWORD)SEG_OFF_TO_PTR(getES(), LOWORD(getDI() + i * 2)) = 0;
            }

            /* Copy the data to the caller */
            for (i = 0; i < sizeof(VBE_MODE_INFO) / sizeof(WORD); i++)
            {
                *(PWORD)SEG_OFF_TO_PTR(getES(), LOWORD(getDI() + i * 2)) = Data[i];
            }

            setAH(0);
            break;
        }

        /* Set VBE Mode */
        case 0x02:
        {
            PCVBE_MODE Mode = VbeGetModeByNumber(getBX());

            if (Mode->Registers == NULL)
            {
                /* Call the VGA BIOS */
                setAH(0x00);
                setAL(Mode->Number);
                Int32Call(&BiosContext, BIOS_VIDEO_INTERRUPT);

                setAL(0x4F);
                setAH(Bda->VideoMode == Mode->Number);
            }
            else
            {
                /* This is an extended video mode */
                setAL(0x4F);
                setAH(VbeSetExtendedVideoMode(Mode->Number));
            }

            break;
        }

        /* Get Current VBE Mode */
        case 0x03:
        {
            PCVBE_MODE Mode = VbeGetModeByNumber(Bda->VideoMode);

            setAL(0x4F);

            if (Mode)
            {
                setBX(Mode->VesaNumber != 0xFFFF
                      ? Mode->VesaNumber : Mode->Number);
                setAH(0);
            }
            else
            {
                setAH(1);
            }

            break;
        }

        default:
        {
            DPRINT1("VESA BIOS Extensions function %02Xh NOT IMPLEMENTED!\n", getAL());
            break;
        }
    }
}

BOOLEAN VbeInitialize(VOID)
{
    BOOLEAN Success;
    BYTE SeqIndex = IOReadB(VGA_SEQ_INDEX);

    /* Store the OEM name */
    strcpy(FAR_POINTER(OEM_NAME_PTR), OEM_NAME);

    /* Unlock SVGA extensions on the card */
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_UNLOCK_REG);
    IOWriteB(VGA_SEQ_DATA, SVGA_SEQ_UNLOCKED);

    /* Check if it worked */
    Success = IOReadB(VGA_SEQ_DATA) == SVGA_SEQ_UNLOCKED;

    IOWriteB(VGA_SEQ_INDEX, SeqIndex);
    return Success;
}
