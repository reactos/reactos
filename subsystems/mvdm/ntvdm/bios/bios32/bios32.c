/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios32/bios32.c
 * PURPOSE:         VDM 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* BIOS Version number and Copyright */
#include <reactos/buildno.h>
#include <reactos/version.h>

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "cpu/cpu.h" // for EMULATOR_FLAG_CF
#include "cpu/bop.h"
#include "int32.h"
#include <isvbop.h>

#include <bios/bios.h>
#include <bios/rom.h>
#include "bios32.h"
#include "bios32p.h"
#include "dskbios32.h"
#include "kbdbios32.h"
#include "vidbios32.h"
#include "moubios32.h"

#include "memory.h"
#include "io.h"
#include "hardware/cmos.h"
#include "hardware/pic.h"
#include "hardware/pit.h"
#include "hardware/ps2.h"

/* PRIVATE VARIABLES **********************************************************/

CALLBACK16 BiosContext;

/*

Bochs BIOS, see rombios.h
=========================

// model byte 0xFC = AT
#define SYS_MODEL_ID     0xFC
#define SYS_SUBMODEL_ID  0x00
#define BIOS_REVISION    1
#define BIOS_CONFIG_TABLE 0xe6f5

#ifndef BIOS_BUILD_DATE
#  define BIOS_BUILD_DATE "06/23/99"
#endif

// 1K of base memory used for Extended Bios Data Area (EBDA)
// EBDA is used for PS/2 mouse support, and IDE BIOS, etc.
#define EBDA_SEG           0x9FC0
#define EBDA_SIZE          1              // In KiB
#define BASE_MEM_IN_K   (640 - EBDA_SIZE)


See rombios.c
=============

ROM BIOS compatibility entry points:
===================================
$e05b ; POST Entry Point
$e2c3 ; NMI Handler Entry Point
$e3fe ; INT 13h Fixed Disk Services Entry Point
$e401 ; Fixed Disk Parameter Table
$e6f2 ; INT 19h Boot Load Service Entry Point
$e6f5 ; Configuration Data Table
$e729 ; Baud Rate Generator Table
$e739 ; INT 14h Serial Communications Service Entry Point
$e82e ; INT 16h Keyboard Service Entry Point
$e987 ; INT 09h Keyboard Service Entry Point
$ec59 ; INT 13h Diskette Service Entry Point
$ef57 ; INT 0Eh Diskette Hardware ISR Entry Point
$efc7 ; Diskette Controller Parameter Table
$efd2 ; INT 17h Printer Service Entry Point
$f045 ; INT 10 Functions 0-Fh Entry Point
$f065 ; INT 10h Video Support Service Entry Point
$f0a4 ; MDA/CGA Video Parameter Table (INT 1Dh)
$f841 ; INT 12h Memory Size Service Entry Point
$f84d ; INT 11h Equipment List Service Entry Point
$f859 ; INT 15h System Services Entry Point
$fa6e ; Character Font for 320x200 & 640x200 Graphics (lower 128 characters)
$fe6e ; INT 1Ah Time-of-day Service Entry Point
$fea5 ; INT 08h System Timer ISR Entry Point
$fef3 ; Initial Interrupt Vector Offsets Loaded by POST
$ff53 ; IRET Instruction for Dummy Interrupt Handler
$ff54 ; INT 05h Print Screen Service Entry Point
$fff0 ; Power-up Entry Point
$fff5 ; ASCII Date ROM was built - 8 characters in MM/DD/YY
$fffe ; System Model ID

*/

/*
 * See Ralf Brown: http://www.ctyme.com/intr/rb-1594.htm#Table515
 * for more information.
 */
#define BIOS_MODEL      0xFC // PC-AT
#define BIOS_SUBMODEL   0x01 // AT models 319,339 8 MHz, Enh Keyb, 3.5"
#define BIOS_REVISION   0x00
// FIXME: Find a nice PS/2 486 + 487 BIOS combination!

static const BIOS_CONFIG_TABLE BiosConfigTable =
{
    sizeof(BIOS_CONFIG_TABLE) - sizeof(((BIOS_CONFIG_TABLE*)0)->Length),    // Length: Number of bytes following

    BIOS_MODEL,     // BIOS Model
    BIOS_SUBMODEL,  // BIOS Sub-Model
    BIOS_REVISION,  // BIOS Revision

    // Feature bytes
    {
        0x78,       // At the moment we don't have any Extended BIOS Area; see http://www.ctyme.com/intr/rb-1594.htm#Table510
        0x00,       // We don't support anything from here; see http://www.ctyme.com/intr/rb-1594.htm#Table511
        0x10,       // Bit 4: POST supports ROM-to-RAM enable/disable
        0x00,
        0x00
    }
};


/*
 * WARNING! For compatibility purposes the string "IBM" should be at F000:E00E .
 * Some programs otherwise look for "COPR. IBM" at F000:E008 .
 */
static const CHAR BiosCopyright[] = "0000000 NTVDM IBM COMPATIBLE 486 BIOS COPYRIGHT (C) ReactOS Team 1996-"COPYRIGHT_YEAR;
static const CHAR BiosVersion[]   = "ReactOS NTVDM 32-bit BIOS Version "KERNEL_VERSION_STR"\0"
                                    "BIOS32 Version "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")";
static const CHAR BiosDate[]      = "06/17/13";

C_ASSERT(sizeof(BiosCopyright)-1 <= 0x5B); // Ensures that we won't overflow on the POST Code starting at F000:E05B
C_ASSERT(sizeof(BiosDate)-1      == 0x08);

/* 16-bit bootstrap code at F000:FFF0 */
static const BYTE Bootstrap[] =
{
    0xEA,                   // jmp far ptr
    0x5B, 0xE0, 0x00, 0xF0, // F000:E05B
};

/*
 * POST code at F000:E05B. All the POST is done in 32 bit
 * and only at the end it calls the bootstrap interrupt.
 */
static const BYTE PostCode[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_RESET,  // Call BIOS POST
    0xCD, BIOS_BOOTSTRAP_LOADER,                            // INT 0x19
    0xCD, BIOS_ROM_BASIC,                                   // INT 0x18
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_UNSIMULATE
};


/* PRIVATE FUNCTIONS **********************************************************/

static VOID BiosCharPrint(CHAR Character)
{
    /* Save AX and BX */
    USHORT AX = getAX();
    USHORT BX = getBX();

    /*
     * Set the parameters:
     * AL contains the character to print,
     * BL contains the character attribute,
     * BH contains the video page to use.
     */
    setAL(Character);
    setBL(DEFAULT_ATTRIBUTE);
    setBH(Bda->VideoPage);

    /* Call the BIOS INT 10h, AH=0Eh "Teletype Output" */
    setAH(0x0E);
    Int32Call(&BiosContext, BIOS_VIDEO_INTERRUPT);

    /* Restore AX and BX */
    setBX(BX);
    setAX(AX);
}

static VOID WINAPI BiosException(LPWORD Stack)
{
    /* Get the exception number and call the emulator API */
    BYTE ExceptionNumber = LOBYTE(Stack[STACK_INT_NUM]);
    EmulatorException(ExceptionNumber, Stack);
}

VOID WINAPI BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    setAX(Bda->EquipmentList);
}

VOID WINAPI BiosGetMemorySize(LPWORD Stack)
{
    /* Return the conventional memory size in kB, typically 640 kB */
    setAX(Bda->MemorySize);
}

static VOID WINAPI BiosMiscService(LPWORD Stack)
{
    switch (getAH())
    {
        /* OS Hooks for Multitasking */
        case 0x80:  // Device Open
        case 0x81:  // Device Close
        case 0x82:  // Program Termination
        case 0x90:  // Device Busy
        case 0x91:  // Device POST
        {
            /* Return success by default */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Wait on External Event */
        case 0x41:
        {
            BYTE Value;
            BOOLEAN Return;
            static DWORD StartingCount;

            /* Check if this is the first time this BOP occurred */
            if (!getCF())
            {
                /* Set the starting count */
                StartingCount = Bda->TickCounter;
            }

            if (getBL() != 0 && (Bda->TickCounter - StartingCount) >= getBL())
            {
                /* Timeout expired */
                setCF(0);
                break;
            }

            if (getAL() & (1 << 4))
            {
                /* Read from the I/O port */
                Value = IOReadB(getDX());
            }
            else
            {
                /* Read from the memory */
                Value = *(LPBYTE)SEG_OFF_TO_PTR(getES(), getDI());
            }

            switch (getAL() & 7)
            {
                /* Any external event */
                case 0:
                {
                    /* Return if this is not the first time the BOP occurred */
                    Return = getCF();
                    break;
                }

                /* Compare and return if equal */
                case 1:
                {
                    Return = Value == getBH();
                    break;
                }

                /* Compare and return if not equal */
                case 2:
                {
                    Return = Value != getBH();
                    break;
                }

                /* Test and return if not zero */
                case 3:
                {
                    Return = (Value & getBH()) != 0;
                    break;
                }

                /* Test and return if zero */
                case 4:
                {
                    Return = (Value & getBH()) == 0;
                    break;
                }

                default:
                {
                    DPRINT1("INT 15h, AH = 41h - Unknown condition type: %u\n", getAL() & 7);
                    Return = TRUE;
                    break;
                }
            }

            /* Repeat the BOP if we shouldn't return */
            setCF(!Return);
            break;
        }

        /* Keyboard intercept */
        case 0x4F:
        {
            /* CF should be set but let's just set it again just in case */
            /* Do not modify AL (the hardware scan code), but set CF to continue processing */
            // setCF(1);
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            break;
        }

        /* Wait */
        case 0x86:
        {
            /*
             * Interval in microseconds in CX:DX
             * See Ralf Brown: http://www.ctyme.com/intr/rb-1525.htm
             * for more information.
             */

            static ULONG CompletionTime = 0;

            /* Check if we're already looping */
            if (getCF())
            {
                if (GetTickCount() >= CompletionTime)
                {
                    /* Stop looping */
                    setCF(0);

                    /* Clear the CF on the stack too */
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                }
            }
            else
            {
                /* Set the CF on the stack */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                /* Set the completion time and start looping */
                CompletionTime = GetTickCount() + (MAKELONG(getDX(), getCX()) / 1000);
                setCF(1);
            }

            break;
        }

        /* Copy Extended Memory */
        case 0x87:
        {
            DWORD Count = (DWORD)getCX() * 2;
            PFAST486_GDT_ENTRY Gdt = (PFAST486_GDT_ENTRY)SEG_OFF_TO_PTR(getES(), getSI());
            DWORD SourceBase = Gdt[2].Base + (Gdt[2].BaseMid << 16) + (Gdt[2].BaseHigh << 24);
            DWORD SourceLimit = Gdt[2].Limit + (Gdt[2].LimitHigh << 16);
            DWORD DestBase = Gdt[3].Base + (Gdt[3].BaseMid << 16) + (Gdt[3].BaseHigh << 24);
            DWORD DestLimit = Gdt[3].Limit + (Gdt[3].LimitHigh << 16);

            /* Check for flags */
            if (Gdt[2].Granularity) SourceLimit = (SourceLimit << 12) | 0xFFF;
            if (Gdt[3].Granularity) DestLimit = (DestLimit << 12) | 0xFFF;

            if ((Count > SourceLimit) || (Count > DestLimit))
            {
                setAX(0x80);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Copy */
            RtlMoveMemory((PVOID)((ULONG_PTR)BaseAddress + DestBase),
                          (PVOID)((ULONG_PTR)BaseAddress + SourceBase),
                          Count);

            setAX(ERROR_SUCCESS);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Get Extended Memory Size */
        case 0x88:
        {
            UCHAR Low, High;

            /*
             * Return the (usable) extended memory (after 1 MB)
             * size in kB from CMOS.
             */
            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_ACTUAL_EXT_MEMORY_LOW  | CMOS_DISABLE_NMI);
            Low  = IOReadB(CMOS_DATA_PORT);
            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_ACTUAL_EXT_MEMORY_HIGH | CMOS_DISABLE_NMI);
            High = IOReadB(CMOS_DATA_PORT);
            setAX(MAKEWORD(Low, High));

            /* Clear CF */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Switch to Protected Mode */
        case 0x89:
        {
            DPRINT1("BIOS INT 15h, AH=89h \"Switch to Protected Mode\" is UNIMPLEMENTED");

            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            goto Default;
        }

        /* Get Configuration */
        case 0xC0:
        {
            /* Return the BIOS ROM Configuration Table address in ES:BX */
            // The BCT is found at F000:E6F5 for 100% compatible BIOSes.
            setES(BIOS_SEGMENT);
            setBX(0xE6F5);

            /* Call successful; clear CF */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Return Extended-Bios Data-Area Segment Address (PS) */
        case 0xC1:
        {
            /* We do not support EBDA yet */
            UNIMPLEMENTED;
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
            goto Default;
        }

        /* Pointing Device BIOS Interface (PS) */
        case 0xC2:
        {
            // FIXME: Reenable this call when we understand why
            // our included mouse driver doesn't correctly reenable
            // mouse reporting!
            // BiosMousePs2Interface(Stack);
            // break;
            goto Default;
        }

        /* Get CPU Type and Mask Revision */
        case 0xC9:
        {
            /*
             * We can see this function as a CPUID replacement.
             * See Ralf Brown: http://www.ctyme.com/intr/rb-1613.htm
             * for more information.
             */

            /*
             * Fast486 is a 486DX with FPU included,
             * but old enough to not support CPUID.
             */
            setCX(0x0400);

            /* Call successful; clear CF */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Get System Memory Map */
        case 0xE8:
        {
            if (getAL() == 0x01)
            {
                /* The amount of memory between 1M and 16M, in kilobytes */
                ULONG Above1M = (min(MAX_ADDRESS, 0x01000000) - 0x00100000) >> 10;

                /* The amount of memory above 16M, in 64K blocks */
                ULONG Above16M = (MAX_ADDRESS > 0x01000000) ? ((MAX_ADDRESS - 0x01000000) >> 16) : 0;

                setAX(Above1M);
                setBX(Above16M);
                setCX(Above1M);
                setDX(Above16M);

                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            }
            else if (getAL() == 0x20 && getEDX() == 'SMAP')
            {
                ULONG Offset = getEBX();
                ULONG Length;
                ULONG BytesWritten = 0;
                BOOLEAN Hooked;
                PBIOS_MEMORY_MAP Map = (PBIOS_MEMORY_MAP)SEG_OFF_TO_PTR(getES(), getDI());

                /* Assume the buffer won't be large enough */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

                while (BytesWritten < getECX() && (ULONG_PTR)Map < (MAX_ADDRESS - sizeof(BIOS_MEMORY_MAP)))
                {
                    /* Let's ask our memory controller */
                    if (!MemQueryMemoryZone(Offset, &Length, &Hooked))
                    {
                        /* No more memory blocks */
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        break;
                    }

                    Map->BaseAddress = (ULONGLONG)Offset;
                    Map->Length = (ULONGLONG)Length;
                    Map->Type = Hooked ? BIOS_MEMORY_RESERVED : BIOS_MEMORY_AVAILABLE;

                    /* Go to the next record */
                    Map++;
                    Offset += Length;
                    BytesWritten += sizeof(BIOS_MEMORY_MAP);
                }

                setEAX('SMAP');
                setEBX(Offset);
                setECX(BytesWritten);
            }
            else
            {
                DPRINT1("BIOS Function INT 15h, AH = 0xE8 - unexpected AL = %02X, EDX = %08X\n",
                        getAL(), getEDX());
            }

            break;
        }

        default: Default:
        {
            DPRINT1("BIOS Function INT 15h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());

            /*
             * The original signification of the error code 0x86 is that
             * no PC Cassette is present. The CF is also set in this case.
             * To keep backward compatibility, newer BIOSes use this value
             * to indicate an unimplemented call in INT 15h.
             */
            setAH(0x86);
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }
}

static VOID WINAPI BiosRomBasic(LPWORD Stack)
{
    PrintMessageAnsi(BiosCharPrint, "FATAL: INT18: BOOT FAILURE.");

    /* ROM Basic is unsupported, display a message to the user */
    DisplayMessage(L"NTVDM doesn't support ROM Basic. The VDM is closing.");

    /* Stop the VDM */
    EmulatorTerminate();
}


extern VOID DosBootsectorInitialize(VOID);
extern VOID WINAPI BiosDiskService(LPWORD Stack);

static VOID WINAPI BiosBootstrapLoader(LPWORD Stack)
{
    USHORT BootOrder;

    USHORT AX, BX, CX, DX, ES;
    AX = getAX();
    BX = getBX();
    CX = getCX();
    DX = getDX();
    ES = getES();

    /*
     * Read the boot sequence order from the CMOS, old behaviour AMI-style.
     *
     * For more information, see:
     * http://www.virtualbox.org/svn/vbox/trunk/src/VBox/Devices/PC/BIOS/orgs.asm
     * http://www.virtualbox.org/svn/vbox/trunk/src/VBox/Devices/PC/BIOS/boot.c
     * https://web.archive.org/web/20150813024016/http://bochs.sourceforge.net/cgi-bin/lxr/source/iodev/cmos.cc
     * https://web.archive.org/web/20111209041013/http://www-ivs.cs.uni-magdeburg.de/~zbrog/asm/cmos.html
     * https://web.archive.org/web/20240119203005/http://www.bioscentral.com/misc/cmosmap.htm
     */
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_SYSOP);
    BootOrder = (IOReadB(CMOS_DATA_PORT) & 0x20) >> 5;

    /*
     * BootOrder =
     * 0: Hard Disk, then Floppy Disk
     * 1: Floppy Disk, then Hard Disk
     * In all cases, if booting from those devices failed,
     * ROM DOS-32 is started. If it fails, INT 18h is called.
     */

    DPRINT("BiosBootstrapLoader (BootOrder = 0x%02X) -->\n", BootOrder);

    /*
     * Format of the BootOrder command:
     * 2 bytes. Each half-byte contains the ID of the drive to boot.
     * Currently defined:
     * 0x0: 1st Floppy disk
     * 0x1: 1st Hard disk
     * Other, or 0xF: Stop boot sequence.
     */
    BootOrder = 0xFF00 | ((1 << (4 * BootOrder)) & 0xFF);

Retry:
    switch (BootOrder & 0x0F)
    {
        /* Boot from 1st floppy drive */
        case 0:
        {
            setAH(0x02); // Read sectors
            setAL(0x01); // Number of sectors
            setDH(0x00); // Head 0
            setCH(0x00); // Cylinder 0
            setCL(0x01); // Sector 1
            setDL(0x00); // First diskette drive (used by loader code, so should not be cleared)
            setES(0x0000);  // Write data in 0000:7C00
            setBX(0x7C00);
            BiosDiskService(Stack);
            if (!(Stack[STACK_FLAGS] & EMULATOR_FLAG_CF)) goto Quit;
#ifdef ADVANCED_DEBUGGING
            DPRINT1("An error happened while loading the bootsector from floppy 0, error = %d\n", getAH());
#endif

            break;
        }

        /* Boot from 1st HDD drive */
        case 1:
        {
            setAH(0x02); // Read sectors
            setAL(0x01); // Number of sectors
            setDH(0x00); // Head 0
            setCH(0x00); // Cylinder 0
            setCL(0x01); // Sector 1
            setDL(0x80); // First HDD drive (used by loader code, so should not be cleared)
            setES(0x0000);  // Write data in 0000:7C00
            setBX(0x7C00);
            BiosDiskService(Stack);
            if (!(Stack[STACK_FLAGS] & EMULATOR_FLAG_CF)) goto Quit;
#ifdef ADVANCED_DEBUGGING
            DPRINT1("An error happened while loading the bootsector from HDD 0, error = %d\n", getAH());
#endif

            break;
        }

        default:
            goto StartDos;
    }

    /* Go to next drive and invalidate the last half-byte. */
    BootOrder = (BootOrder >> 4) | 0xF000;
    goto Retry;

StartDos:
    /* Clear everything, we are going to load DOS32 */
    setAX(AX);
    setBX(BX);
    setCX(CX);
    setDX(DX);
    setES(ES);
    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

    /* Load our DOS */
    DosBootsectorInitialize();

Quit:
    /*
     * Jump to 0000:7C00 to boot the OS.
     *
     * Since we are called via the INT32 mechanism, we need to correctly set
     * CS:IP, not by changing the current one (otherwise the interrupt could
     * not be clean up and return properly), but by changing the CS:IP in the
     * stack, so that when the interrupt returns, the modified CS:IP is popped
     * off the stack and the CPU is correctly repositioned.
     */
    Stack[STACK_CS] = 0x0000;
    Stack[STACK_IP] = 0x7C00;

    DPRINT("<-- BiosBootstrapLoader\n");
}

static VOID WINAPI BiosTimeService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Get System Time */
        case 0x00:
        {
            /* Set AL to 1 if midnight had passed, 0 otherwise */
            setAL(Bda->MidnightPassed ? 0x01 : 0x00);

            /* Return the tick count in CX:DX */
            setCX(HIWORD(Bda->TickCounter));
            setDX(LOWORD(Bda->TickCounter));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        /* Set System Time */
        case 0x01:
        {
            /* Set the tick count to CX:DX */
            Bda->TickCounter = MAKELONG(getDX(), getCX());

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        /* Get Real-Time Clock Time */
        case 0x02:
        {
            UCHAR StatusB;

            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_HOURS);
            setCH(IOReadB(CMOS_DATA_PORT));

            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_MINUTES);
            setCL(IOReadB(CMOS_DATA_PORT));

            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_SECONDS);
            setDH(IOReadB(CMOS_DATA_PORT));

            /* Daylight Savings Time */
            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_STATUS_B);
            StatusB = IOReadB(CMOS_DATA_PORT);
            setDL(StatusB & 0x01);

            /* Clear CF */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        // /* Set Real-Time Clock Time */
        // case 0x03:
        // {
        //     break;
        // }

        /* Get Real-Time Clock Date */
        case 0x04:
        {
            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_CENTURY);
            setCH(IOReadB(CMOS_DATA_PORT));

            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_YEAR);
            setCL(IOReadB(CMOS_DATA_PORT));

            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_MONTH);
            setDH(IOReadB(CMOS_DATA_PORT));

            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_DAY);
            setDL(IOReadB(CMOS_DATA_PORT));

            /* Clear CF */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        // /* Set Real-Time Clock Date */
        // case 0x05:
        // {
        //     break;
        // }

        default:
        {
            DPRINT1("BIOS Function INT 1Ah, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosSystemTimerInterrupt(LPWORD Stack)
{
    /* Increase the system tick count */
    Bda->TickCounter++;
}


// From SeaBIOS
static VOID PicSetIRQMask(USHORT off, USHORT on)
{
    UCHAR pic1off = off, pic1on = on, pic2off = off>>8, pic2on = on>>8;
    IOWriteB(PIC_MASTER_DATA, (IOReadB(PIC_MASTER_DATA) & ~pic1off) | pic1on);
    IOWriteB(PIC_SLAVE_DATA , (IOReadB(PIC_SLAVE_DATA ) & ~pic2off) | pic2on);
}

// From SeaBIOS
VOID EnableHwIRQ(UCHAR hwirq, EMULATOR_INT32_PROC func)
{
    UCHAR vector;

    PicSetIRQMask(1 << hwirq, 0);
    if (hwirq < 8)
        vector = BIOS_PIC_MASTER_INT + hwirq;
    else
        vector = BIOS_PIC_SLAVE_INT  + hwirq - 8;

    RegisterBiosInt32(vector, func);
}


VOID PicIRQComplete(BYTE IntNum)
{
    /*
     * If this was a PIC IRQ, send an End-of-Interrupt to the PIC.
     */
    if (IntNum >= BIOS_PIC_MASTER_INT && IntNum < BIOS_PIC_MASTER_INT + 8)
    {
        /* It was an IRQ from the master PIC */
        IOWriteB(PIC_MASTER_CMD, PIC_OCW2_EOI);
    }
    else if (IntNum >= BIOS_PIC_SLAVE_INT && IntNum < BIOS_PIC_SLAVE_INT + 8)
    {
        /* It was an IRQ from the slave PIC */
        IOWriteB(PIC_SLAVE_CMD , PIC_OCW2_EOI);
        IOWriteB(PIC_MASTER_CMD, PIC_OCW2_EOI);
    }
}

static VOID WINAPI BiosHandleMasterPicIRQ(LPWORD Stack)
{
    BYTE IrqNumber;

    IOWriteB(PIC_MASTER_CMD, PIC_OCW3_READ_ISR /* == 0x0B */);
    IrqNumber = IOReadB(PIC_MASTER_CMD);

    DPRINT("Master - IrqNumber = 0x%02X\n", IrqNumber);

    PicIRQComplete(LOBYTE(Stack[STACK_INT_NUM]));
}

static VOID WINAPI BiosHandleSlavePicIRQ(LPWORD Stack)
{
    BYTE IrqNumber;

    IOWriteB(PIC_SLAVE_CMD, PIC_OCW3_READ_ISR /* == 0x0B */);
    IrqNumber = IOReadB(PIC_SLAVE_CMD);

    DPRINT("Slave - IrqNumber = 0x%02X\n", IrqNumber);

    PicIRQComplete(LOBYTE(Stack[STACK_INT_NUM]));
}

// Timer IRQ 0
static VOID WINAPI BiosTimerIrq(LPWORD Stack)
{
    /*
     * Perform the system timer interrupt.
     *
     * Do not call directly BiosSystemTimerInterrupt(Stack);
     * because some programs may hook only BIOS_SYS_TIMER_INTERRUPT
     * for their purpose...
     */

    WORD AX = getAX();
    WORD CX = getCX();
    WORD DX = getDX();
    WORD BX = getBX();
    WORD BP = getBP();
    WORD SI = getSI();
    WORD DI = getDI();
    WORD DS = getDS();
    WORD ES = getES();

    Int32Call(&BiosContext, BIOS_SYS_TIMER_INTERRUPT);

    setAX(AX);
    setCX(CX);
    setDX(DX);
    setBX(BX);
    setBP(BP);
    setSI(SI);
    setDI(DI);
    setDS(DS);
    setES(ES);
    setCF(0);

    // BiosSystemTimerInterrupt(Stack);
    PicIRQComplete(LOBYTE(Stack[STACK_INT_NUM]));
}


static VOID BiosHwSetup(VOID)
{
    /* Initialize the master and the slave PICs (cascade mode) */
    IOWriteB(PIC_MASTER_CMD, PIC_ICW1 | PIC_ICW1_ICW4);
    IOWriteB(PIC_SLAVE_CMD , PIC_ICW1 | PIC_ICW1_ICW4);

    /*
     * Set the interrupt vector offsets for each PIC
     * (base IRQs: 0x08-0x0F for IRQ 0-7, 0x70-0x77 for IRQ 8-15)
     */
    IOWriteB(PIC_MASTER_DATA, BIOS_PIC_MASTER_INT);
    IOWriteB(PIC_SLAVE_DATA , BIOS_PIC_SLAVE_INT );

    /* Tell the master PIC that there is a slave PIC at IRQ 2 */
    IOWriteB(PIC_MASTER_DATA, 1 << 2);
    /* Tell the slave PIC its cascade identity */
    IOWriteB(PIC_SLAVE_DATA , 2);

    /* Make sure both PICs are in 8086 mode */
    IOWriteB(PIC_MASTER_DATA, PIC_ICW4_8086);
    IOWriteB(PIC_SLAVE_DATA , PIC_ICW4_8086);

    /* Clear the masks for both PICs */
    // IOWriteB(PIC_MASTER_DATA, 0x00);
    // IOWriteB(PIC_SLAVE_DATA , 0x00);
    /* Disable all IRQs */
    IOWriteB(PIC_MASTER_DATA, 0xFF);
    IOWriteB(PIC_SLAVE_DATA , 0xFF);


    /* Initialize PIT Counter 0 - Mode 2, 16bit binary count */
    // NOTE: Some BIOSes set it to Mode 3 instead.
    IOWriteB(PIT_COMMAND_PORT, 0x34);
    // 18.2Hz refresh rate
    IOWriteB(PIT_DATA_PORT(0), 0x00);
    IOWriteB(PIT_DATA_PORT(0), 0x00);

    /* Initialize PIT Counter 1 - Mode 2, 8bit binary count */
    IOWriteB(PIT_COMMAND_PORT, 0x54);
    // DRAM refresh every 15ms: https://web.archive.org/web/20180723173420/http://www.cs.dartmouth.edu/~spl/Academic/Organization/docs/PC%20Timer%208253.html
    IOWriteB(PIT_DATA_PORT(1),   18);

    /* Initialize PIT Counter 2 - Mode 3, 16bit binary count */
    IOWriteB(PIT_COMMAND_PORT, 0xB6);
    // Count for 440Hz
    IOWriteB(PIT_DATA_PORT(2), 0x97);
    IOWriteB(PIT_DATA_PORT(2), 0x0A);


    /* Initialize PS/2 keyboard port */
    // Enable the port
    IOWriteB(PS2_CONTROL_PORT, 0xAE);
    // Port interrupts and clock enabled,
    // enable keyboard scancode translation.
    // POST passed, force keyboard unlocking.
    IOWriteB(PS2_CONTROL_PORT, 0x60);
    IOWriteB(PS2_DATA_PORT   , 0x6D);
    // Enable data reporting
    IOWriteB(PS2_DATA_PORT   , 0xF4);

    EnableHwIRQ(0, BiosTimerIrq);
}

static VOID InitializeBiosInt32(VOID)
{
    USHORT i;

    /* Initialize the callback context */
    InitializeContext(&BiosContext, BIOS_SEGMENT, 0x0000);

    /* Register the default BIOS interrupt vectors */

    /*
     * Zero out all of the IVT (0x00 -- 0xFF). Some applications
     * indeed expect to have free vectors at the end of the IVT.
     */
    RtlZeroMemory(BaseAddress, 0x0100 * sizeof(ULONG));

#if defined(ADVANCED_DEBUGGING) && (ADVANCED_DEBUGGING_LEVEL >= 3)
    // Initialize all the interrupt vectors to the default one.
    for (i = 0x00; i <= 0xFF; i++)
        RegisterBiosInt32(i, NULL);
#endif

    /* Initialize the exception interrupt vectors to a default Exception handler */
    for (i = 0x00; i <= 0x07; i++)
        RegisterBiosInt32(i, BiosException);

    /* Initialize HW interrupt vectors to a default HW handler */
    for (i = BIOS_PIC_MASTER_INT; i < BIOS_PIC_MASTER_INT + 8; i++) // 0x08 -- 0x0F
        RegisterBiosInt32(i, BiosHandleMasterPicIRQ);
    for (i = BIOS_PIC_SLAVE_INT ; i < BIOS_PIC_SLAVE_INT  + 8; i++) // 0x70 -- 0x77
        RegisterBiosInt32(i, BiosHandleSlavePicIRQ);

    /* Initialize software vector handlers */
    // BIOS_VIDEO_INTERRUPT  : 0x10 (vidbios32.c)
    RegisterBiosInt32(BIOS_EQUIPMENT_INTERRUPT, BiosEquipmentService    );
    RegisterBiosInt32(BIOS_MEMORY_SIZE        , BiosGetMemorySize       );
    // BIOS_DISK_INTERRUPT   : 0x13 (dskbios32.c)
    // BIOS_SERIAL_INTERRUPT : 0x14 -- UNIMPLEMENTED
    RegisterBiosInt32(BIOS_MISC_INTERRUPT     , BiosMiscService         );
    // BIOS_KBD_INTERRUPT    : 0x16 (kbdbios32.c)
    // BIOS_PRINTER_INTERRUPT: 0x17 -- UNIMPLEMENTED
    RegisterBiosInt32(BIOS_ROM_BASIC          , BiosRomBasic            );
    RegisterBiosInt32(BIOS_BOOTSTRAP_LOADER   , BiosBootstrapLoader     );
    RegisterBiosInt32(BIOS_TIME_INTERRUPT     , BiosTimeService         );
    // BIOS_KBD_CTRL_BREAK_INTERRUPT: 0x1B -- UNIMPLEMENTED
    RegisterBiosInt32(BIOS_SYS_TIMER_INTERRUPT, BiosSystemTimerInterrupt);

    /* Vectors that should be implemented (see above) */
    RegisterBiosInt32(0x14, NULL);
    RegisterBiosInt32(0x17, NULL);
    RegisterBiosInt32(0x1B, NULL);
    RegisterBiosInt32(0x4A, NULL); // User Alarm Handler

    /* Relocated services by the BIOS (when needed) */
    RegisterBiosInt32(0x40, NULL); // ROM BIOS Diskette Handler relocated by Hard Disk BIOS
    RegisterBiosInt32(0x42, NULL); // Relocated Default INT 10h Video Services

    /* Miscellaneous unimplemented vector handlers that should better have a default one */
    RegisterBiosInt32(0x4B, NULL); // Virtual DMA Specification Services
    RegisterBiosInt32(0x5C, NULL); // NetBIOS

    // ROM-BASIC interrupts span from 0x80 up to 0xEF.
    // They don't have any default handler at the moment.

    /* Some vectors are in fact addresses to tables */
    ((PULONG)BaseAddress)[0x1D] = NULL32; // Video Parameter Tables
    ((PULONG)BaseAddress)[0x1E] = NULL32; // Diskette Parameters
    ((PULONG)BaseAddress)[0x1F] = NULL32; // 8x8 Graphics Font
    ((PULONG)BaseAddress)[0x41] = NULL32; // Hard Disk 0 Parameter Table Address
    ((PULONG)BaseAddress)[0x43] = NULL32; // Character Table (EGA, MCGA, VGA)
    ((PULONG)BaseAddress)[0x46] = NULL32; // Hard Disk 1 Drive Parameter Table Address
    /* Tables that are always uninitialized */
    ((PULONG)BaseAddress)[0x44] = NULL32; // ROM BIOS Character Font, Characters 00h-7Fh (PCjr)
    ((PULONG)BaseAddress)[0x48] = NULL32; // Cordless Keyboard Translation (PCjr)
    ((PULONG)BaseAddress)[0x49] = NULL32; // Non-Keyboard Scan-code Translation Table (PCJr)
}

static VOID InitializeBiosData(VOID)
{
    UCHAR Low, High;

    /* Initialize the BDA contents */
    RtlZeroMemory(Bda, sizeof(*Bda));

    /*
     * Retrieve the basic equipment list from the CMOS
     */
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_EQUIPMENT_LIST | CMOS_DISABLE_NMI);
    Bda->EquipmentList = IOReadB(CMOS_DATA_PORT);
    // TODO: Update it if required.
    Bda->EquipmentList &= 0x00FF; // High byte cleared for now...

    /*
     * Retrieve the conventional memory size
     * in kB from the CMOS, typically 640 kB.
     */
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_BASE_MEMORY_LOW  | CMOS_DISABLE_NMI);
    Low  = IOReadB(CMOS_DATA_PORT);
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_BASE_MEMORY_HIGH | CMOS_DISABLE_NMI);
    High = IOReadB(CMOS_DATA_PORT);
    Bda->MemorySize = MAKEWORD(Low, High);
}


/*
 * The BIOS POST (Power On-Self Test)
 */
/*static*/ VOID
WINAPI
Bios32Post(LPWORD Stack)
{
    static BOOLEAN FirstBoot = TRUE;
    BYTE ShutdownStatus;

    /*
     * Initialize BIOS/Keyboard/Video RAM dynamic data
     */

    DPRINT("Bios32Post\n");

    /* Disable interrupts */
    setIF(0);

    /* Set the data segment */
    setDS(BDA_SEGMENT);

    /* Initialize the stack */
    // Temporary stack for POST (to be used only before initializing the INT vectors)
    // setSS(0x0000);
    // setSP(0x0400);
    //
    // Stack to be used after the initialization of the INT vectors
    setSS(0x0000);  // Stack at 00:8000, going downwards
    setSP(0x8000);

    /*
     * Perform early CMOS shutdown status checks
     */

    /* Read the CMOS shutdown status byte and reset it */
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_SHUTDOWN_STATUS | CMOS_DISABLE_NMI);
    ShutdownStatus = IOReadB(CMOS_DATA_PORT);
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_SHUTDOWN_STATUS | CMOS_DISABLE_NMI);
    IOWriteB(CMOS_DATA_PORT, 0x00);

    DPRINT1("Bda->SoftReset = 0x%04X ; ShutdownStatus = 0x%02X\n",
            Bda->SoftReset, ShutdownStatus);

    switch (ShutdownStatus)
    {
        /* Shutdown after Memory Tests (unsupported) */
        case 0x01: case 0x02: case 0x03:
        /* Shutdown after Protected Mode Tests (unsupported) */
        case 0x06: case 0x07: case 0x08:
        /* Shutdown after Block Move Test (unsupported) */
        case 0x09:
        {
            DisplayMessage(L"Unsupported CMOS Shutdown Status value 0x%02X. The VDM will shut down.", ShutdownStatus);
            EmulatorTerminate();
            return;
        }

        /* Shutdown to Boot Loader */
        case 0x04:
        {
            DPRINT1("Fast restart to Bootstrap Loader...\n");
            goto Quit; // Reenable interrupts and exit.
        }

        /* Flush keyboard, issue an EOI... */
        case 0x05:
        {
            IOReadB(PS2_DATA_PORT);

            /* Send EOI */
            IOWriteB(PIC_SLAVE_CMD , PIC_OCW2_EOI);
            IOWriteB(PIC_MASTER_CMD, PIC_OCW2_EOI);

            // Fall back
        }

        /*
         * ... and far JMP to user-specified location at 0040:0067
         * (Bda->ResumeEntryPoint) with interrupts and NMI disabled.
         */
        case 0x0A:
        {
            DPRINT1("Bda->ResumeEntryPoint = %04X:%04X\n",
                    HIWORD(Bda->ResumeEntryPoint),
                    LOWORD(Bda->ResumeEntryPoint));

            /* Position execution pointers and return with interrupts disabled */
            setCS(HIWORD(Bda->ResumeEntryPoint));
            setIP(LOWORD(Bda->ResumeEntryPoint));
            return;
        }

        /* Soft reset or unexpected shutdown... */
        case 0x00:
        /* ... or other possible shutdown codes: just continue the POST */
        default:
            break;
    }

    /*
     * FIXME: UNIMPLEMENTED!
     * Check the word at 0040h:0072h (Bda->SoftReset) and do one of the
     * following actions:
     * - if the word is 0000h, perform a cold reboot (aka. Reset). Everything gets initialized.
     * - if the word is 1234h, perform a warm reboot (aka. Ctrl-Alt-Del). Some stuff is skipped.
     */
    switch (Bda->SoftReset)
    {
        case 0x0000:
        {
            if (!FirstBoot)
            {
                DisplayMessage(L"NTVDM is performing a COLD reboot! The program you are currently testing does not seem to behave correctly! The VDM will shut down...");
                EmulatorTerminate();
                return;
            }
            break;
        }

        case 0x1234:
        {
            DisplayMessage(L"NTVDM is performing a WARM reboot! This is not supported at the moment. The VDM will shut down...");
            EmulatorTerminate();
            return;
        }

        default:
            break;
    }

    FirstBoot = FALSE;

    /* Initialize the BDA */
    InitializeBiosData();

    /* Initialize the User Data Area at 0050:XXXX */
    RtlZeroMemory(SEG_OFF_TO_PTR(0x50, 0x0000), sizeof(USER_DATA_AREA));



    //////// NOTE: This is more or less bios-specific ////////

    /*
     * Initialize IVT and hardware
     */

    // WriteUnProtectRom(...);

    /* Register the BIOS 32-bit Interrupts */
    InitializeBiosInt32();

    /* Initialize platform hardware (PIC/PIT chips, ...) */
    BiosHwSetup();

    /* Initialize the Keyboard, Video and Mouse BIOS */
    KbdBios32Post();
    VidBiosPost();
    MouseBios32Post();
    DiskBios32Post();

    // WriteProtectRom(...);

    //////// End of more or less bios-specific section ///////



    SearchAndInitRoms(&BiosContext);

    /*
     * End of the 32-bit POST portion. We then fall back into 16-bit where
     * the rest of the POST code is executed, typically calling INT 19h
     * to boot up the OS.
     */

Quit:
    /* Enable interrupts */
    setIF(1);
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN Bios32Initialize(VOID)
{
    /*
     * Initialize BIOS/Keyboard/Video ROM static data
     */

    /* System BIOS Copyright */
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xE000), BiosCopyright, sizeof(BiosCopyright)-1);

    /* System BIOS Version */
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xE070), BiosVersion, sizeof(BiosVersion)-1);

    /* System BIOS Date */
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xFFF5), BiosDate, sizeof(BiosDate)-1);

    /* Bootstrap code */
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xE05B), PostCode , sizeof(PostCode ));
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xFFF0), Bootstrap, sizeof(Bootstrap));

    /* BIOS ROM Information */
    RtlCopyMemory(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xE6F5), &BiosConfigTable, sizeof(BiosConfigTable));

    /* System BIOS Model (same as Bct->Model) */
    *(PBYTE)(SEG_OFF_TO_PTR(BIOS_SEGMENT, 0xFFFE)) = BIOS_MODEL;

    /* Initialize the Keyboard and Video BIOS */
    if (!KbdBiosInitialize() || !VidBiosInitialize() || !MouseBiosInitialize() || !DiskBios32Initialize())
    {
        /* Stop the VDM */
        EmulatorTerminate();
        return FALSE;
    }

    /* Redefine our POST function */
    RegisterBop(BOP_RESET, Bios32Post);

    WriteProtectRom((PVOID)TO_LINEAR(BIOS_SEGMENT, 0x0000),
                    ROM_AREA_END - TO_LINEAR(BIOS_SEGMENT, 0x0000) + 1);

    /* We are done */
    return TRUE;
}

VOID Bios32Cleanup(VOID)
{
    DiskBios32Cleanup();
    MouseBios32Cleanup();
    VidBios32Cleanup();
    KbdBiosCleanup();
}

/* EOF */
