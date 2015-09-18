/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios32/moubios32.c
 * PURPOSE:         VDM 32-bit PS/2 Mouse BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE: Based from VirtualBox OSE ROM BIOS, and SeaBIOS.
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cpu/cpu.h" // for EMULATOR_FLAG_CF

#include "moubios32.h"
#include "bios32p.h"

#include "io.h"
#include "hardware/mouse.h"
#include "hardware/ps2.h"

/* PRIVATE VARIABLES **********************************************************/

#define MOUSE_IRQ_INT   0x74

static BOOLEAN MouseEnabled = FALSE;
static DWORD OldIrqHandler;

/*
 * Far pointer to a device handler. In compatible PS/2, it is stored in the EBDA.
 *
 * See Ralf Brown: http://www.ctyme.com/intr/rb-1603.htm
 * for more information. In particular:
 * when the subroutine is called, it is given 4 WORD values on the stack;
 * the handler should return with a FAR return without popping the stack.
 */
static ULONG DeviceHandler = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID DisableMouseInt(VOID)
{
    BYTE ControllerConfig;

    /* Clear the mouse queue */
    while (PS2PortQueueRead(1)) ; // NOTE: Should be a IOReadB! But see r67231

    /* Disable mouse interrupt and events */
    IOWriteB(PS2_CONTROL_PORT, 0x20);
    ControllerConfig = IOReadB(PS2_DATA_PORT);
    ControllerConfig &= ~0x02; // Turn off IRQ12
    ControllerConfig |=  0x20; // Disable mouse clock line
    IOWriteB(PS2_CONTROL_PORT, 0x60);
    IOWriteB(PS2_DATA_PORT, ControllerConfig);
}

static VOID EnableMouseInt(VOID)
{
    BYTE ControllerConfig;

    /* Clear the mouse queue */
    while (PS2PortQueueRead(1)) ; // NOTE: Should be a IOReadB! But see r67231

    /* Enable mouse interrupt and events */
    IOWriteB(PS2_CONTROL_PORT, 0x20);
    ControllerConfig = IOReadB(PS2_DATA_PORT);
    ControllerConfig |=  0x02; // Turn on IRQ12
    ControllerConfig &= ~0x20; // Enable mouse clock line
    IOWriteB(PS2_CONTROL_PORT, 0x60);
    IOWriteB(PS2_DATA_PORT, ControllerConfig);
}

static inline
VOID SendMouseCommand(UCHAR Command)
{
    /* Clear the mouse queue */
    while (PS2PortQueueRead(1)) ; // NOTE: Should be a IOReadB! But see r67231

    /* Send the command */
    IOWriteB(PS2_CONTROL_PORT, 0xD4);
    IOWriteB(PS2_DATA_PORT, Command);
}

static inline
UCHAR ReadMouseData(VOID)
{
    PS2PortQueueRead(1); // NOTE: Should be a IOReadB! But see r67231
    return IOReadB(PS2_DATA_PORT);
}


static
VOID BiosMouseEnable(VOID)
{
    if (MouseEnabled) return;

    MouseEnabled = TRUE;

    /* Get the old IRQ handler */
    OldIrqHandler = ((PDWORD)BaseAddress)[MOUSE_IRQ_INT];

    /* Set the IRQ handler */
    //RegisterInt32(MAKELONG(FIELD_OFFSET(MOUSE_DRIVER, MouseIrqInt16Stub), MouseDataSegment),
    //              MOUSE_IRQ_INT, DosMouseIrq, NULL);
}

static
VOID BiosMouseDisable(VOID)
{
    if (!MouseEnabled) return;

    /* Restore the old IRQ handler */
    // ((PDWORD)BaseAddress)[MOUSE_IRQ_INT] = OldIrqHandler;

    MouseEnabled = FALSE;
}


// Mouse IRQ 12
static VOID WINAPI BiosMouseIrq(LPWORD Stack)
{
    DPRINT1("PS/2 Mouse IRQ! DeviceHandler = 0x%04X:0x%04X\n",
            HIWORD(DeviceHandler), LOWORD(DeviceHandler));

    if (DeviceHandler != 0)
    {
        /*
         * Prepare the stack for the mouse device handler:
         * push Status, X and Y data, and a zero word.
         */
        setSP(getSP() - sizeof(WORD));
        *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = 0; // Status
        setSP(getSP() - sizeof(WORD));
        *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = 0; // X data (high byte = 0)
        setSP(getSP() - sizeof(WORD));
        *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = 0; // Y data (high byte = 0)
        setSP(getSP() - sizeof(WORD));
        *((LPWORD)SEG_OFF_TO_PTR(getSS(), getSP())) = 0; // Zero

        /* Call the device handler */
        RunCallback16(&BiosContext, DeviceHandler);

        /* Pop the stack */
        setSP(getSP() + 4*sizeof(WORD));
    }

    PicIRQComplete(LOBYTE(Stack[STACK_INT_NUM]));
}

VOID BiosMousePs2Interface(LPWORD Stack)
{
    /* Disable mouse interrupt and events */
    DisableMouseInt();

    switch (getAL())
    {
        /* Enable / Disable */
        case 0x00:
        {
            UCHAR State = getBH();

            if (State > 2)
            {
                /* Invalid function */
                setAH(0x01);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            if (State == 0x00)
            {
                BiosMouseDisable();

                /* Disable packet reporting */
                SendMouseCommand(0xF5);
            }
            else // if (State == 0x01)
            {
                /* Check for the presence of the device handler */
                if (DeviceHandler == 0)
                {
                    /* No device handler installed */
                    setAH(0x05);
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    break;
                }

                BiosMouseEnable();

                /* Enable packet reporting */
                SendMouseCommand(0xF4);
            }

            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Initialize */
        case 0x05:
        {
            // Fall through
        }

        /* Reset */
        case 0x01:
        {
            UCHAR Answer;

            SendMouseCommand(0xFF);
            Answer = ReadMouseData();
            /* A "Resend" signal (0xFE) is sent if no mouse is attached */
            if (Answer == 0xFE)
            {
                /* Resend */
                setAH(0x04);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }
            else if (Answer != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            setBL(ReadMouseData()); // Should be MOUSE_BAT_SUCCESS
            setBH(ReadMouseData()); // Mouse ID

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Set Sampling Rate */
        case 0x02:
        {
            UCHAR SampleRate = 0;

            switch (getBH())
            {
                case 0x00: SampleRate =  10; break; //  10 reports/sec
                case 0x01: SampleRate =  20; break; //  20    "     "
                case 0x02: SampleRate =  40; break; //  40    "     "
                case 0x03: SampleRate =  60; break; //  60    "     "
                case 0x04: SampleRate =  80; break; //  80    "     "
                case 0x05: SampleRate = 100; break; // 100    "     "
                case 0x06: SampleRate = 200; break; // 200    "     "
                default:   SampleRate =   0;
            }

            if (SampleRate == 0)
            {
                /* Invalid input */
                setAH(0x02);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            SendMouseCommand(0xF3);
            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            SendMouseCommand(SampleRate);
            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Set Resolution */
        case 0x03:
        {
            UCHAR Resolution = getBH();

            /*
             * 0:  25 dpi, 1 count  per millimeter
             * 1:  50 dpi, 2 counts per millimeter
             * 2: 100 dpi, 4 counts per millimeter
             * 3: 200 dpi, 8 counts per millimeter
             */
            if (Resolution > 3)
            {
                /* Invalid input */
                setAH(0x02);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            SendMouseCommand(0xE8);
            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            SendMouseCommand(Resolution);
            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Get Type */
        case 0x04:
        {
            SendMouseCommand(0xF2);
            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            setBH(ReadMouseData());

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Extended Commands (Return Status and Set Scaling Factor) */
        case 0x06:
        {
            UCHAR Command = getBH();

            switch (Command)
            {
                /* Return Status */
                case 0x00:
                {
                    SendMouseCommand(0xE9);
                    if (ReadMouseData() != MOUSE_ACK)
                    {
                        /* Failure */
                        setAH(0x03);
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        break;
                    }

                    setBL(ReadMouseData()); // Status
                    setCL(ReadMouseData()); // Resolution
                    setDL(ReadMouseData()); // Sample rate

                    /* Success */
                    setAH(0x00);
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    break;
                }

                /* Set Scaling Factor to 1:1 */
                case 0x01:
                /* Set Scaling Factor to 2:1 */
                case 0x02:
                {
                    SendMouseCommand(Command == 0x01 ? 0xE6 : 0xE7);
                    if (ReadMouseData() != MOUSE_ACK)
                    {
                        /* Failure */
                        setAH(0x03);
                        Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                        break;
                    }

                    /* Success */
                    setAH(0x00);
                    Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
                    break;
                }

                default:
                {
                    /* Invalid function */
                    setAH(0x01);
                    Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                    break;
                }
            }

            break;
        }

        /* Set Device Handler Address */
        case 0x07:
        {
            /* ES:BX == 0000h:0000h removes the device handler */
            DeviceHandler = MAKELONG(getBX(), getES());

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Write to Pointer Port */
        case 0x08:
        {
            SendMouseCommand(getBL());
            if (ReadMouseData() != MOUSE_ACK)
            {
                /* Failure */
                setAH(0x03);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
                break;
            }

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Read from Pointer Port */
        case 0x09:
        {
            setBL(ReadMouseData());
            setCL(ReadMouseData());
            setDL(ReadMouseData());

            /* Success */
            setAH(0x00);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        default:
        {
            DPRINT1("INT 15h, AH = C2h, AL = %02Xh NOT IMPLEMENTED\n",
                    getAL());

            /* Unknown function */
            setAH(0x01);
            Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;
        }
    }

    /* Reenable mouse interrupt and events */
    EnableMouseInt();
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID MouseBios32Post(VOID)
{
    UCHAR Answer;

    /* Initialize PS/2 mouse port */
    // Enable the port
    IOWriteB(PS2_CONTROL_PORT, 0xA8);

    /* Detect mouse presence by attempting a reset */
    SendMouseCommand(0xFF);
    Answer = ReadMouseData();
    /* A "Resend" signal (0xFE) is sent if no mouse is attached */
    if (Answer == 0xFE)
    {
        DPRINT1("No mouse present!\n");
    }
    else if (Answer != MOUSE_ACK)
    {
        DPRINT1("Mouse reset failure!\n");
    }
    else
    {
        /* Mouse present, try to completely enable it */

        // FIXME: The following is temporary until
        // this is moved into the mouse driver!!

        /* Enable packet reporting */
        SendMouseCommand(0xF4);
        if (ReadMouseData() != MOUSE_ACK)
        {
            DPRINT1("Failed to enable mouse!\n");
        }
        else
        {
            /* Enable mouse interrupt and events */
            EnableMouseInt();
        }
    }

    /* No mouse driver available so far */
    RegisterBiosInt32(0x33, NULL);

    /* Set up the HW vector interrupts */
    EnableHwIRQ(12, BiosMouseIrq);
}

BOOLEAN MouseBiosInitialize(VOID)
{
    return TRUE;
}

VOID MouseBios32Cleanup(VOID)
{
}
