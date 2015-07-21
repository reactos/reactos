/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vbe.c
 * PURPOSE:         VDM VESA BIOS Extensions (for the Cirrus CL-GD5434 emulated card)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/cpu.h"

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
    { 0x14, 0xFFFF, NULL /* TODO */, NULL /* TODO */ },
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
    { 0x72, 0xFFFF, NULL /* TODO */, NULL /* TODO */ },
    { 0x73, 0xFFFF, NULL /* TODO */, NULL /* TODO */ },
    { 0x74, 0x117,  NULL /* TODO */, NULL /* TODO */ },
    { 0x75, 0x11A,  NULL /* TODO */, NULL /* TODO */ },
    { 0x76, 0xFFFF, NULL /* TODO */, NULL /* TODO */ },
    { 0x78, 0x115,  NULL /* TODO */, NULL /* TODO */ },
    { 0x79, 0x118,  NULL /* TODO */, NULL /* TODO */ },
};

/* PUBLIC FUNCTIONS ***********************************************************/

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
            WORD ModeNumber = getCX() & 0x1FF;
            PWORD Data = NULL;

            /* Function recognized */
            setAL(0x4F);

            /* Find the mode */
            for (i = 0; i < VBE_MODE_COUNT; i++)
            {
                if ((!(ModeNumber & 0x100) && (ModeNumber == Modes[i].Number))
                    || ((ModeNumber & 0x100) && (ModeNumber == Modes[i].VesaNumber)))
                {
                    Data = (PWORD)Modes[i].Info;

                    if (Data == NULL)
                    {
                        DPRINT1("WARNING: The mode information for mode %02X (%03X) is missing!\n",
                                Modes[i].Number,
                                Modes[i].VesaNumber);
                    }
                }
            }

            if (Data == NULL)
            {
                /* Mode not found */
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
