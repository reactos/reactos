/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios32.c
 * PURPOSE:         VDM 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "bios32.h"

#include "io.h"
#include "hardware/cmos.h"
#include "hardware/pic.h"
#include "hardware/timer.h"

#include "int32.h"

/* PRIVATE VARIABLES **********************************************************/

PBIOS_DATA_AREA Bda;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI BiosException(LPWORD Stack)
{
    /* Get the exception number and call the emulator API */
    BYTE ExceptionNumber = LOBYTE(Stack[STACK_INT_NUM]);
    EmulatorException(ExceptionNumber, Stack);
}

static VOID WINAPI BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    setAX(Bda->EquipmentList);
}

static VOID WINAPI BiosGetMemorySize(LPWORD Stack)
{
    /* Return the conventional memory size in kB, typically 640 kB */
    setAX(Bda->MemorySize);
}

static VOID WINAPI BiosMiscService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Wait */
        case 0x86:
        {
            /*
             * Interval in microseconds in CX:DX
             * See Ralf Brown: http://www.ctyme.com/intr/rb-1525.htm
             * for more information.
             */
            Sleep(MAKELONG(getDX(), getCX()));

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

        default:
        {
            DPRINT1("BIOS Function INT 15h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
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

    RegisterInt32(vector, func);
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

    DPRINT("Master - IrqNumber = 0x%x\n", IrqNumber);

    PicIRQComplete(Stack);
}

static VOID WINAPI BiosHandleSlavePicIRQ(LPWORD Stack)
{
    BYTE IrqNumber;

    IOWriteB(PIC_SLAVE_CMD, PIC_OCW3_READ_ISR /* == 0x0B */);
    IrqNumber = IOReadB(PIC_SLAVE_CMD);

    DPRINT("Slave - IrqNumber = 0x%x\n", IrqNumber);

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
    EmulatorInterrupt(BIOS_SYS_TIMER_INTERRUPT);
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


    /* Initialize PIT Counter 0 */
    IOWriteB(PIT_COMMAND_PORT, 0x34);
    IOWriteB(PIT_DATA_PORT(0), 0x00);
    IOWriteB(PIT_DATA_PORT(0), 0x00);

    /* Initialize PIT Counter 1 */
    IOWriteB(PIT_COMMAND_PORT, 0x74);
    IOWriteB(PIT_DATA_PORT(1), 0x00);
    IOWriteB(PIT_DATA_PORT(1), 0x00);

    /* Initialize PIT Counter 2 */
    IOWriteB(PIT_COMMAND_PORT, 0xB4);
    IOWriteB(PIT_DATA_PORT(2), 0x00);
    IOWriteB(PIT_DATA_PORT(2), 0x00);

    EnableHwIRQ(0, BiosTimerIrq);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * The BIOS POST (Power On-Self Test)
 */
BOOLEAN Bios32Initialize(IN HANDLE ConsoleInput,
                         IN HANDLE ConsoleOutput)
{
    UCHAR Low, High;
    UCHAR i;

    /* Initialize the BDA */
    Bda = (PBIOS_DATA_AREA)SEG_OFF_TO_PTR(BDA_SEGMENT, 0);
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

    /* Initialize the 32-bit Interrupt system */
    InitializeInt32(BIOS_SEGMENT);

    /* Register the BIOS 32-bit Interrupts */

    /* Initialize the exception vector interrupts to a default Exception handler */
    for (i = 0; i < 8; i++)
        RegisterInt32(i, BiosException);

    /* Initialize HW vector interrupts to a default HW handler */
    for (i = BIOS_PIC_MASTER_INT; i < BIOS_PIC_MASTER_INT + 8; i++)
        RegisterInt32(i, BiosHandleMasterPicIRQ);
    for (i = BIOS_PIC_SLAVE_INT ; i < BIOS_PIC_SLAVE_INT  + 8; i++)
        RegisterInt32(i, BiosHandleSlavePicIRQ);

    /* Initialize software vector handlers */
    RegisterInt32(BIOS_EQUIPMENT_INTERRUPT, BiosEquipmentService    );
    RegisterInt32(BIOS_MEMORY_SIZE        , BiosGetMemorySize       );
    RegisterInt32(BIOS_MISC_INTERRUPT     , BiosMiscService         );
    RegisterInt32(BIOS_TIME_INTERRUPT     , BiosTimeService         );
    RegisterInt32(BIOS_SYS_TIMER_INTERRUPT, BiosSystemTimerInterrupt);

    /* Some interrupts are in fact addresses to tables */
    ((PDWORD)BaseAddress)[0x1E] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x41] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x46] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x48] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x49] = (DWORD)NULL;

    /* Initialize platform hardware (PIC/PIT chips, ...) */
    BiosHwSetup();

    /* Initialize the Keyboard BIOS */
    if (!KbdBios32Initialize(ConsoleInput)) return FALSE;

    /* Set the console input mode */
    SetConsoleMode(ConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

    /* Initialize the Video BIOS */
    if (!VidBios32Initialize(ConsoleOutput)) return FALSE;

    return TRUE;
}

VOID Bios32Cleanup(VOID)
{
    VidBios32Cleanup();
    KbdBios32Cleanup();
}

/* EOF */
