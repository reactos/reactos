/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios32.c
 * PURPOSE:         VDM 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

/* For BIOS Version number */
#include <reactos/buildno.h>

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/cpu.h" // for EMULATOR_FLAG_CF
#include "cpu/bop.h"
#include "int32.h"

#include <bios/bios.h>
#include <bios/rom.h>
#include "bios32.h"
#include "bios32p.h"
#include "kbdbios32.h"
#include "vidbios32.h"
#include "moubios32.h"

#include "io.h"
#include "hardware/cmos.h"
#include "hardware/pic.h"
#include "hardware/pit.h"

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

/*
 * WARNING! For compatibility purposes the string "IBM" should be at F000:E00E .
 * Some programs alternatively look at "COPR. IBM" that is at F000:E008 .
 */
static const CHAR BiosCopyright[] = "0000000 NTVDM IBM Compatible 486 32-bit BIOS Copyright (C) ReactOS Team 1996-2014";
static const CHAR BiosVersion[]   = "ReactOS NTVDM 32-bit BIOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")";
static const CHAR BiosDate[]      = "06/17/13";

C_ASSERT(sizeof(BiosCopyright)-1 <= 0x5B); // Ensures that we won't overflow on the POST Code starting at F000:E05B
C_ASSERT(sizeof(BiosDate)-1      == 0x08);

/* 16-bit bootstrap code at F000:FFF0 */
static BYTE Bootstrap[] =
{
    0xEA,                   // jmp far ptr
    0x5B, 0xE0, 0x00, 0xF0, // F000:E05B
};

/*
 * Normally at F000:E05B there is the POST that finally calls the bootstrap
 * interrupt. It should also check the value of Bda->SoftReset. Since we do
 * all the POST in 32 bit from the start, we just place there the bootstrap
 * interrupt call.
 */
static BYTE PostCode[] =
{
    LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_RESET,  // Call BIOS POST
    0xCD, 0x19,                                             // INT 0x19, the bootstrap loader interrupt
//  LOBYTE(EMULATOR_BOP), HIBYTE(EMULATOR_BOP), BOP_UNSIMULATE
};


/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI BiosException(LPWORD Stack)
{
    /* Get the exception number and call the emulator API */
    BYTE ExceptionNumber = LOBYTE(Stack[STACK_INT_NUM]);
    EmulatorException(ExceptionNumber, Stack);
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
            LARGE_INTEGER TimeOut;
            TimeOut.QuadPart = MAKELONG(getDX(), getCX()) * -10LL;

            // HACK: For now, use the NT API (time in hundreds of nanoseconds).
            NtDelayExecution(FALSE, &TimeOut);

            /* Clear CF */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

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
            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_ACTUAL_EXT_MEMORY_LOW);
            Low  = IOReadB(CMOS_DATA_PORT);
            IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_ACTUAL_EXT_MEMORY_HIGH);
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
            break;
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
            // Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            // setES(???);

            UNIMPLEMENTED;

            /* We do not support EBDA yet */
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

            break;
        }

        /* Pointing Device BIOS Interface (PS) */
        case 0xC2:
        {
            BiosMousePs2Interface(Stack);
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 15h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosRomBasic(LPWORD Stack)
{
    /* ROM Basic is unsupported, display a message to the user */
    DisplayMessage(L"NTVDM doesn't support ROM Basic. The VDM is closing.");

    /* Stop the VDM */
    EmulatorTerminate();
    return;
}


VOID DosBootsectorInitialize(VOID);

static VOID WINAPI BiosBootstrapLoader(LPWORD Stack)
{
    /*
     * In real BIOSes one loads the bootsector read from a diskette
     * or from a disk, copy it to 0000:7C00 and then boot it.
     * Since we are 32-bit VM and we hardcode our DOS at the moment,
     * just call the DOS 32-bit initialization code.
     */

    DPRINT("BiosBootstrapLoader -->\n");

    /* Load DOS */
    DosBootsectorInitialize();

    /*
     * Position CPU to 0000:7C00 to boot the OS.
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

        case 0x01:
        {
            /* Set the tick count to CX:DX */
            Bda->TickCounter = MAKELONG(getDX(), getCX());

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

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


VOID PicIRQComplete(LPWORD Stack)
{
    /* Get the interrupt number */
    BYTE IntNum = LOBYTE(Stack[STACK_INT_NUM]);

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

    PicIRQComplete(Stack);
}

static VOID WINAPI BiosHandleSlavePicIRQ(LPWORD Stack)
{
    BYTE IrqNumber;

    IOWriteB(PIC_SLAVE_CMD, PIC_OCW3_READ_ISR /* == 0x0B */);
    IrqNumber = IOReadB(PIC_SLAVE_CMD);

    DPRINT("Slave - IrqNumber = 0x%02X\n", IrqNumber);

    PicIRQComplete(Stack);
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
    Int32Call(&BiosContext, BIOS_SYS_TIMER_INTERRUPT);
    // BiosSystemTimerInterrupt(Stack);
    PicIRQComplete(Stack);
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
    // DRAM refresh every 15ms: http://www.cs.dartmouth.edu/~spl/Academic/Organization/docs/PC%20Timer%208253.html
    IOWriteB(PIT_DATA_PORT(1),   18);

    /* Initialize PIT Counter 2 - Mode 3, 16bit binary count */
    IOWriteB(PIT_COMMAND_PORT, 0xB6);
    // Count for 440Hz
    IOWriteB(PIT_DATA_PORT(2), 0x97);
    IOWriteB(PIT_DATA_PORT(2), 0x0A);

    EnableHwIRQ(0, BiosTimerIrq);
}

static VOID InitializeBiosInt32(VOID)
{
    USHORT i;

    /* Initialize the callback context */
    InitializeContext(&BiosContext, BIOS_SEGMENT, 0x0000);

    /* Register the default BIOS interrupt vectors */

    /* Zero out all of the IVT (0x00 -- 0xFF) */
    RtlZeroMemory(BaseAddress, 0x0100 * sizeof(ULONG));

#ifdef ADVANCED_DEBUGGING
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
    // BIOS_DISK_INTERRUPT   : 0x13 -- UNIMPLEMENTED
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
    RegisterBiosInt32(0x13, NULL);
    RegisterBiosInt32(0x14, NULL);
    RegisterBiosInt32(0x17, NULL);
    RegisterBiosInt32(0x1B, NULL);
    RegisterBiosInt32(0x4A, NULL); // User Alarm Handler
    RegisterBiosInt32(0x6D, NULL); // Video BIOS Entry Point

    /* Relocated services by the BIOS (when needed) */
    RegisterBiosInt32(0x40, NULL); // ROM BIOS Diskette Handler relocated by Hard Disk BIOS
    RegisterBiosInt32(0x42, NULL); // Relocated Default INT 10h Video Services

    /* Miscellaneous unimplemented vector handlers that should better have a default one */
    RegisterBiosInt32(0x4B, NULL); // Virtual DMA Specification Services
    RegisterBiosInt32(0x5C, NULL); // NetBIOS

    // ROM-BASIC interrupts span from 0x80 up to 0xEF.
    // They don't have any default handler at the moment.

    /* Some vectors are in fact addresses to tables */
    ((PULONG)BaseAddress)[0x1D] = (ULONG)NULL; // Video Parameter Tables
    ((PULONG)BaseAddress)[0x1E] = (ULONG)NULL; // Diskette Parameters
    ((PULONG)BaseAddress)[0x1F] = (ULONG)NULL; // 8x8 Graphics Font
    ((PULONG)BaseAddress)[0x41] = (ULONG)NULL; // Hard Disk 0 Parameter Table Address
    ((PULONG)BaseAddress)[0x43] = (ULONG)NULL; // Character Table (EGA, MCGA, VGA)
    ((PULONG)BaseAddress)[0x46] = (ULONG)NULL; // Hard Disk 1 Drive Parameter Table Address
    /* Tables that are always uninitialized */
    ((PULONG)BaseAddress)[0x44] = (ULONG)NULL; // ROM BIOS Character Font, Characters 00h-7Fh (PCjr)
    ((PULONG)BaseAddress)[0x48] = (ULONG)NULL; // Cordless Keyboard Translation (PCjr)
    ((PULONG)BaseAddress)[0x49] = (ULONG)NULL; // Non-Keyboard Scan-code Translation Table (PCJr)
}

static VOID InitializeBiosData(VOID)
{
    UCHAR Low, High;

    /* Initialize the BDA contents */
    RtlZeroMemory(Bda, sizeof(*Bda));
    Bda->EquipmentList = BIOS_EQUIPMENT_LIST;

    /*
     * Retrieve the conventional memory size
     * in kB from CMOS, typically 640 kB.
     */
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_BASE_MEMORY_LOW);
    Low  = IOReadB(CMOS_DATA_PORT);
    IOWriteB(CMOS_ADDRESS_PORT, CMOS_REG_BASE_MEMORY_HIGH);
    High = IOReadB(CMOS_DATA_PORT);
    Bda->MemorySize = MAKEWORD(Low, High);
}

static VOID InitializeBiosInfo(VOID)
{
    RtlZeroMemory(Bct, sizeof(*Bct));

    Bct->Length     = sizeof(*Bct);
    Bct->Model      = BIOS_MODEL;
    Bct->SubModel   = BIOS_SUBMODEL;
    Bct->Revision   = BIOS_REVISION;
    Bct->Feature[0] = 0x70; // At the moment we don't support "wait for external event (INT 15/AH=41h)", we also don't have any "extended BIOS area allocated (usually at top of RAM)"; see http://www.ctyme.com/intr/rb-1594.htm#Table510
    Bct->Feature[1] = 0x00; // We don't support anything from here; see http://www.ctyme.com/intr/rb-1594.htm#Table511
    Bct->Feature[2] = 0x00;
    Bct->Feature[3] = 0x00;
    Bct->Feature[4] = 0x00;
}



/*
 * The BIOS POST (Power On-Self Test)
 */
VOID
Bios32Post(VOID)
{
#if 0
    BOOLEAN Success;
#endif

    DPRINT("Bios32Post\n");

    /* Initialize the stack */
    // That's what says IBM... (stack at 30:00FF going downwards)
    // setSS(0x0000);
    // setSP(0x0400);
    setSS(0x0050);  // Stack at 50:0400, going downwards
    setSP(0x0400);

    /* Set data segment */
    setDS(BDA_SEGMENT);

    /*
     * Check the word at 0040h:0072h (Bda->SoftReset) and do one of the
     * following actions:
     * - if the word is 0000h, perform a cold reboot (aka. Reset). Everything gets initialized.
     * - if the word is 1234h, perform a warm reboot (aka. Ctrl-Alt-Del). Some stuff is skipped.
     * In case we do a warm reboot, we need to check for the CMOS shutdown flag
     * and take a suitable action.
     */

    // FIXME: This is a debug temporary check:
    // Since NTVDM memory is by default initialized with 0xCC, it is also
    // the case for the BDA. So at first boot we get SoftReset == 0xCCCC.
    // After we zero out the BDA and put valid values in it.
    // If for some reason an app calls the BIOS initialization code,
    // SoftReset is normally zero (unless the app puts a non-null value in SoftReset)
    // and we can detect that. With the current state of NTVDM, apps calling
    // by hand the BIOS init code is a sign of a bug, e.g. see MSD.EXE version 2+.
    if (Bda->SoftReset != 0xCCCC)
    {
        DisplayMessage(L"NTVDM is performing a COLD reboot! The program you are currently testing seems to not behave correctly! The VDM is stopping...");
        EmulatorTerminate();
        return;
    }

    /* Initialize the BDA and the BIOS ROM Information */
    InitializeBiosData();
    InitializeBiosInfo();

    /*
     * Initialize IVT and hardware
     */

    /* Register the BIOS 32-bit Interrupts */
    InitializeBiosInt32();

    /* Initialize platform hardware (PIC/PIT chips, ...) */
    BiosHwSetup();

    /* Initialize the Keyboard, Video and Mouse BIOS */
    if (!KbdBios32Initialize() || !VidBios32Initialize() || !MouseBios32Initialize())
    {
        // return FALSE;

        /* Stop the VDM */
        EmulatorTerminate();
        return;
    }

#if 0
    /* Initialize the Keyboard and Video BIOS */
    if (!KbdBiosInitialize() || !VidBiosInitialize())
    {
        /* Stop the VDM */
        EmulatorTerminate();
        return;
    }
#endif

    ///////////// MUST BE DONE AFTER IVT INITIALIZATION !! /////////////////////

#if 0
    /* Load some ROMs */
    Success = LoadRom("boot.bin", (PVOID)0xE0000, NULL);
    DPRINT1("Test ROM loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());
#endif

    SearchAndInitRoms(&BiosContext);

    /*
     * End of the 32-bit POST portion. We then fall back into 16-bit where
     * the rest of the POST code is executed, typically calling INT 19h
     * to boot up the OS.
     */
}

static VOID WINAPI Bios32ResetBop(LPWORD Stack)
{
    DPRINT("Bios32ResetBop\n");

    /* Disable interrupts */
    setIF(0);

    /* Do the POST */
    Bios32Post();

    /* Enable interrupts */
    setIF(1);
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN Bios32Initialize(VOID)
{
    /*
     * Initialize BIOS32 static data
     */

    /* Bootstrap code */
    RtlCopyMemory(SEG_OFF_TO_PTR(0xF000, 0xE05B), PostCode , sizeof(PostCode ));
    RtlCopyMemory(SEG_OFF_TO_PTR(0xF000, 0xFFF0), Bootstrap, sizeof(Bootstrap));

    /* System BIOS Copyright */
    RtlCopyMemory(SEG_OFF_TO_PTR(0xF000, 0xE000), BiosCopyright, sizeof(BiosCopyright)-1);

    /* System BIOS Version */
    RtlCopyMemory(SEG_OFF_TO_PTR(0xF000, 0xE080), BiosVersion, sizeof(BiosVersion)-1);
    // FIXME: or E061, or E100 ??

    /* System BIOS Date */
    RtlCopyMemory(SEG_OFF_TO_PTR(0xF000, 0xFFF5), BiosDate, sizeof(BiosDate)-1);

    /* System BIOS Model (same as Bct->Model) */
    *(PBYTE)(SEG_OFF_TO_PTR(0xF000, 0xFFFE)) = BIOS_MODEL;

    /* Redefine our POST function */
    RegisterBop(BOP_RESET, Bios32ResetBop);

    /* We are done */
    return TRUE;
}

VOID Bios32Cleanup(VOID)
{
    MouseBios32Cleanup();
    VidBios32Cleanup();
    KbdBios32Cleanup();
}

/* EOF */
