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

static const VBE_MODE_INFO VbeMode_640x480x256_Info =
{
    /* Attributes */
    VBE_MODE_SUPPORTED
    | VBE_MODE_OPTIONAL_INFO
    // | VBE_MODE_BIOS_SUPPORT
    | VBE_MODE_COLOR
    | VBE_MODE_GRAPHICS,

    /* Window A attributes */
    VBE_WINDOW_EXISTS | VBE_WINDOW_READABLE | VBE_WINDOW_WRITABLE,
    /* Window B attributes */
    0,

    16,                   /* Window granularity, in KB */
    64,                   /* Window size, in KB */
    0xA000,               /* Window A segment, or zero if not supported */
    0x0000,               /* Window B segment, or zero if not supported */
    0x00000000,           /* Window position function pointer */
    640,                  /* Bytes per scanline */
    640,                  /* Width */
    480,                  /* Height */
    8,                    /* Character cell width */
    16,                   /* Character cell height */
    1,                    /* Number of memory planes */
    8,                    /* Bits per pixel */
    1,                    /* Number of banks */
    VBE_MODEL_PACKED,     /* Memory model */
    0,                    /* Bank size */
    11,                   /* Number of image pages */
    0,                    /* Reserved field */
    0,                    /* Red mask size */
    0,                    /* Red field position */
    0,                    /* Green mask size */
    0,                    /* Green field position */
    0,                    /* Blue mask size */
    0,                    /* Blue field position */
    0,                    /* Reserved mask size */
    0,                    /* Reserved field position */
    0,                    /* Direct color info */
};

static SVGA_REGISTERS VbeMode_640x480x256_Registers =
{
    /* Miscellaneous Register */
    0x63,

    /* Hidden Register */
    0x00,

    /* Sequencer Registers */
    {
        0x03, 0x21, 0x0F, 0x00, 0x0E, 0x00, 0x12, 0x11, 0x00, 0x00, 0x18, 0x58,
        0x58, 0x58, 0x58, 0x98, 0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x2D
    },

    /* CRTC Registers */
    {
        0x5F, 0x4F, 0x4F, 0x80, 0x52, 0x1E, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xEA, 0x2C, 0xDF, 0x50, 0x40, 0xDF, 0x0B, 0xC3,
        0xFF, 0x00, 0x00, 0x22, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
        0x80, 0x00, 0x20, 0xB8
    },

    /* GC Registers */
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF, 0x00, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },

    /* AC Registers */
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00
    }
};

static const VBE_MODE_INFO VbeMode_800x600x256_Info =
{
    /* Attributes */
    VBE_MODE_SUPPORTED
    | VBE_MODE_OPTIONAL_INFO
    // | VBE_MODE_BIOS_SUPPORT
    | VBE_MODE_COLOR
    | VBE_MODE_GRAPHICS,

    /* Window A attributes */
    VBE_WINDOW_EXISTS | VBE_WINDOW_READABLE | VBE_WINDOW_WRITABLE,
    /* Window B attributes */
    0,

    16,                   /* Window granularity, in KB */
    64,                   /* Window size, in KB */
    0xA000,               /* Window A segment, or zero if not supported */
    0x0000,               /* Window B segment, or zero if not supported */
    0x00000000,           /* Window position function pointer */
    800,                  /* Bytes per scanline */
    800,                  /* Width */
    600,                  /* Height */
    8,                    /* Character cell width */
    16,                   /* Character cell height */
    1,                    /* Number of memory planes */
    8,                    /* Bits per pixel */
    1,                    /* Number of banks */
    VBE_MODEL_PACKED,     /* Memory model */
    0,                    /* Bank size */
    7,                    /* Number of image pages */
    0,                    /* Reserved field */
    0,                    /* Red mask size */
    0,                    /* Red field position */
    0,                    /* Green mask size */
    0,                    /* Green field position */
    0,                    /* Blue mask size */
    0,                    /* Blue field position */
    0,                    /* Reserved mask size */
    0,                    /* Reserved field position */
    0,                    /* Direct color info */
};

static SVGA_REGISTERS VbeMode_800x600x256_Registers =
{
    /* Miscellaneous Register */
    0x63,

    /* Hidden Register */
    0x00,

    /* Sequencer Registers */
    {
        0x03, 0x21, 0x0F, 0x00, 0x0E, 0x00, 0x12, 0x11, 0x00, 0x00, 0x18, 0x23,
        0x23, 0x23, 0x23, 0x98, 0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x14, 0x2D
    },

    /* CRTC Registers */
    {
        0x7D, 0x63, 0x63, 0x80, 0x6B, 0x1A, 0x98, 0xF0, 0x00, 0x60, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x7D, 0x23, 0x57, 0x64, 0x40, 0x57, 0x98, 0xC3,
        0xFF, 0x00, 0x00, 0x22, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
        0x80, 0x00, 0x20, 0xB8
    },

    /* GC Registers */
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF, 0x00, 0x00, 0x20,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },

    /* AC Registers */
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00
    }
};

static const VBE_MODE Modes[VBE_MODE_COUNT] =
{
    { 0x14, 0xFFFF, NULL                     , NULL /* TODO */                },
    { 0x54, 0x10A , NULL /* TODO */          , NULL /* TODO */                },
    { 0x55, 0x109 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x58, 0x102 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x5C, 0x103 , &VbeMode_800x600x256_Info, &VbeMode_800x600x256_Registers },
    { 0x5D, 0x104 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x5E, 0x100 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x5F, 0x101 , &VbeMode_640x480x256_Info, &VbeMode_640x480x256_Registers },
    { 0x60, 0x105 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x64, 0x111 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x65, 0x114 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x66, 0x110 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x67, 0x113 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x68, 0x116 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x69, 0x119 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x6C, 0x106 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x6D, 0x107 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x71, 0x112 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x72, 0xFFFF, NULL                     , NULL /* TODO */                },
    { 0x73, 0xFFFF, NULL                     , NULL /* TODO */                },
    { 0x74, 0x117 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x75, 0x11A , NULL /* TODO */          , NULL /* TODO */                },
    { 0x76, 0xFFFF, NULL                     , NULL /* TODO */                },
    { 0x78, 0x115 , NULL /* TODO */          , NULL /* TODO */                },
    { 0x79, 0x118 , NULL /* TODO */          , NULL /* TODO */                },
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
    IOWriteB(VGA_CRTC_INDEX, VGA_CRTC_END_VERT_RETRACE_REG);
    IOWriteB(VGA_CRTC_DATA , IOReadB(VGA_CRTC_DATA) & ~0x80);
    // Make sure they remain unlocked
    Registers->CRT[VGA_CRTC_END_HORZ_BLANKING_REG] |= 0x80;
    Registers->CRT[VGA_CRTC_END_VERT_RETRACE_REG] &= ~0x80;

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
    if (Mode == NULL) return FALSE;

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

    /* Clear the screen */
    VgaClearMemory();

    return TRUE;
}

VOID WINAPI VbeResetExtendedRegisters(VOID)
{
    BYTE i;

    /* Disable interrupts */
    BOOLEAN Interrupts = getIF();
    setIF(0);

    /* Turn the video off */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_CLOCK_REG);
    IOWriteB(VGA_SEQ_DATA , IOReadB(VGA_SEQ_DATA) | VGA_SEQ_CLOCK_SD);

    /* Synchronous reset on */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_RESET_REG);
    IOWriteB(VGA_SEQ_DATA , VGA_SEQ_RESET_AR );

    /* Clear the extended sequencer registers, except for the VCLKs and MCLK */
    for (i = SVGA_SEQ_EXT_MODE_REG; i < SVGA_SEQ_VCLK0_DENOMINATOR_REG; i++)
    {
        if (i != VGA_SEQ_MAX_REG && i != SVGA_SEQ_UNLOCK_REG
            && (i < SVGA_SEQ_VCLK0_NUMERATOR_REG || i > SVGA_SEQ_VCLK3_NUMERATOR_REG))
        {
            IOWriteB(VGA_SEQ_INDEX, i);
            IOWriteB(VGA_SEQ_DATA, 0x00);
        }
    }

    /* Reset the VCLKs */
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK0_NUMERATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x66);
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK0_DENOMINATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x3B);

    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK1_NUMERATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x5B);
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK1_DENOMINATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x2F);

    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK2_NUMERATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x45);
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK2_DENOMINATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x30);

    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK3_NUMERATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x7E);
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_VCLK3_DENOMINATOR_REG);
    IOWriteB(VGA_SEQ_DATA, 0x33);

    /* Reset the MCLK */
    IOWriteB(VGA_SEQ_INDEX, SVGA_SEQ_MCLK_REG);
    IOWriteB(VGA_SEQ_DATA, 0x1C);

    /* Synchronous reset off */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_RESET_REG);
    IOWriteB(VGA_SEQ_DATA , VGA_SEQ_RESET_SR | VGA_SEQ_RESET_AR);

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

    /* Turn the video on */
    IOWriteB(VGA_SEQ_INDEX, VGA_SEQ_CLOCK_REG);
    IOWriteB(VGA_SEQ_DATA , IOReadB(VGA_SEQ_DATA) & ~VGA_SEQ_CLOCK_SD);

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
            WORD VesaNumber = getBX();
            setAL(0x4F);

            if (VesaNumber <= BIOS_MAX_VIDEO_MODE)
            {
                /* Call the VGA BIOS */
                setAH(0x00);
                setAL(VesaNumber);
                Int32Call(&BiosContext, BIOS_VIDEO_INTERRUPT);

                setAH(Bda->VideoMode != VesaNumber);
            }
            else
            {
                /* This is an extended video mode */
                PCVBE_MODE Mode = VbeGetModeByNumber(VesaNumber);

                if (Mode) setAH(!VbeSetExtendedVideoMode(Mode->Number));
                else setAH(1);
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

        /* CPU Video Memory Control */
        case 0x05:
        {
            BYTE Window = getBL();
            BYTE OldGcIndex = IOReadB(VGA_GC_INDEX);

            switch (getBH())
            {
                /* Select Memory Window */
                case 0:
                {
                    setAL(0x4F);

                    if (getDH() != 0)
                    {
                        /* Offset too high */
                        setAH(1);
                        break;
                    }

                    IOWriteB(VGA_GC_INDEX, (Window == 0) ? SVGA_GC_OFFSET_0_REG : SVGA_GC_OFFSET_1_REG);
                    IOWriteB(VGA_GC_DATA, getDL());

                    setAH(0);
                    break;
                }

                /* Return Memory Window */
                case 1:
                {
                    IOWriteB(VGA_GC_INDEX, (Window == 0) ? SVGA_GC_OFFSET_0_REG : SVGA_GC_OFFSET_1_REG);
                    setDX(IOReadB(VGA_GC_DATA));

                    setAX(0x004F);
                    break;
                }

                default:
                {
                    DPRINT("VESA INT 0x10, AL = 0x05, Unknown subfunction: %02X", getBH());
                }
            }

            IOWriteB(VGA_GC_INDEX, OldGcIndex);
            break;
        }

        /* Get/Set Display Start */
        case 0x07:
        {
            DWORD StartAddress;
            BYTE Value;
            PCVBE_MODE Mode = VbeGetModeByNumber(Bda->VideoMode);
            BYTE OldCrtcIndex = IOReadB(VGA_CRTC_INDEX_COLOR);

            if (getBL() & 0x80)
            {
                /* Wait for a vertical retrace */
                if (!(IOReadB(VGA_INSTAT1_READ_COLOR) & VGA_STAT_VRETRACE))
                {
                    setCF(1);
                    break;
                }

                setCF(0);
            }

            switch (getBL() & 0x7F)
            {
                /* Set Display Start */
                case 0x00:
                {
                    setAL(0x4F);

                    if (Mode == NULL || Mode->Info == NULL)
                    {
                        /* This is not a VBE mode */
                        // TODO: Support anyway, perhaps? It can be done.
                        setAH(0x01);
                        break;
                    }

                    StartAddress = getCX() + getDX() * Mode->Info->BytesPerScanline;

                    IOWriteB(VGA_CRTC_INDEX_COLOR, SVGA_CRTC_OVERLAY_REG);
                    Value = IOReadB(VGA_CRTC_DATA_COLOR);
                    Value &= ~SVGA_CRTC_EXT_ADDR_BIT19;
                    Value |= (StartAddress >> 12) & SVGA_CRTC_EXT_ADDR_BIT19;
                    IOWriteB(VGA_CRTC_DATA_COLOR, Value);

                    IOWriteB(VGA_CRTC_INDEX_COLOR, SVGA_CRTC_EXT_DISPLAY_REG);
                    Value = IOReadB(VGA_CRTC_DATA_COLOR);
                    Value &= ~(SVGA_CRTC_EXT_ADDR_BIT16 | SVGA_CRTC_EXT_ADDR_BITS1718);
                    Value |= (StartAddress >> 16) & SVGA_CRTC_EXT_ADDR_BIT16;
                    Value |= (StartAddress >> 15) & SVGA_CRTC_EXT_ADDR_BITS1718;
                    IOWriteB(VGA_CRTC_DATA_COLOR, Value);

                    IOWriteB(VGA_CRTC_INDEX_COLOR, VGA_CRTC_START_ADDR_HIGH_REG);
                    IOWriteB(VGA_CRTC_DATA_COLOR, (StartAddress >> 8) & 0xFF);
                    IOWriteB(VGA_CRTC_INDEX_COLOR, VGA_CRTC_START_ADDR_LOW_REG);
                    IOWriteB(VGA_CRTC_DATA_COLOR, StartAddress & 0xFF);

                    setAH(0);
                    break;
                }

                /* Get Display Start */
                case 0x01:
                {
                    setAL(0x4F);
                    StartAddress = 0;

                    if (Mode == NULL || Mode->Info == NULL)
                    {
                        /* This is not a VBE mode */
                        // TODO: Support anyway, perhaps? It can be done.
                        setAH(0x01);
                        break;
                    }

                    IOWriteB(VGA_CRTC_INDEX_COLOR, SVGA_CRTC_OVERLAY_REG);
                    StartAddress = (IOReadB(VGA_CRTC_DATA_COLOR) & SVGA_CRTC_EXT_ADDR_BIT19) << 12;

                    IOWriteB(VGA_CRTC_INDEX_COLOR, SVGA_CRTC_EXT_DISPLAY_REG);
                    Value = IOReadB(VGA_CRTC_DATA_COLOR);
                    StartAddress |= (Value & SVGA_CRTC_EXT_ADDR_BIT16) << 16;
                    StartAddress |= (Value & SVGA_CRTC_EXT_ADDR_BITS1718) << 15;

                    IOWriteB(VGA_CRTC_INDEX_COLOR, VGA_CRTC_START_ADDR_HIGH_REG);
                    StartAddress |= IOReadB(VGA_CRTC_DATA_COLOR) << 8;
                    IOWriteB(VGA_CRTC_INDEX_COLOR, VGA_CRTC_START_ADDR_LOW_REG);
                    StartAddress |= IOReadB(VGA_CRTC_DATA_COLOR);

                    setCX(StartAddress % Mode->Info->BytesPerScanline);
                    setDX(StartAddress / Mode->Info->BytesPerScanline);

                    setAH(0);
                    break;
                }
            }

            IOWriteB(VGA_CRTC_INDEX_COLOR, OldCrtcIndex);
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
