/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/ps2.c
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * DOCUMENTATION:   IBM Personal System/2 Hardware Interface Technical Reference, May 1988 (Section 10)
 *                  https://wiki.osdev.org/%228042%22_PS/2_Controller
 *                  http://www.computer-engineering.org/ps2keyboard/ (DEAD_LINK)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "ps2.h"

#include "memory.h"
#include "io.h"
#include "pic.h"
#include "clock.h"

/* PRIVATE VARIABLES **********************************************************/

#define BUFFER_SIZE 32

typedef struct _PS2_PORT
{
    BOOLEAN IsEnabled;

    BOOLEAN QueueEmpty;
    BYTE    Queue[BUFFER_SIZE];
    UINT    QueueStart;
    UINT    QueueEnd;
    HANDLE  QueueMutex;

    LPVOID             Param;
    PS2_DEVICE_CMDPROC DeviceCommand;
} PS2_PORT, *PPS2_PORT;

/*
 * Port 1: Keyboard
 * Port 2: Mouse
 */
#define PS2_PORTS  2
static PS2_PORT Ports[PS2_PORTS];

static BYTE Memory[0x20]; // PS/2 Controller internal memory
#define ControllerConfig Memory[0] // The configuration byte is byte 0

static BYTE StatusRegister = 0x00; // Controller status register
// static BYTE InputBuffer  = 0x00; // PS/2 Input  Buffer
static BYTE OutputBuffer = 0x00; // PS/2 Output Buffer

static BYTE ControllerCommand = 0x00;

static PHARDWARE_TIMER IrqTimer = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID PS2SendCommand(BYTE PS2Port, BYTE Command)
{
    PPS2_PORT Port;

    ASSERT(PS2Port < PS2_PORTS);
    Port = &Ports[PS2Port];

    /*
     * According to http://www.win.tue.nl/~aeb/linux/kbd/scancodes-11.html#kccad
     * any PS/2 command sent reenables the corresponding port.
     */
    if (PS2Port == 0)
        ControllerConfig &= ~PS2_CONFIG_KBD_DISABLE;
    else // if (PS2Port == 1)
        ControllerConfig &= ~PS2_CONFIG_AUX_DISABLE;

    Port->IsEnabled = TRUE;

    /* Call the device command */
    if (Port->DeviceCommand) Port->DeviceCommand(Port->Param, Command);
}


static BYTE WINAPI PS2ReadControl(USHORT Port)
{
    UNREFERENCED_PARAMETER(Port);

    /*
     * Be sure the "Keyboard enable" flag is always set.
     * On IBM PC-ATs this is the state of the hardware keyboard
     * lock mechanism. It is not widely used, but some programs
     * still use it, see for example:
     * https://www.os2museum.com/wp/the-dos-4-0-shell-mouse-mystery/
     */
    StatusRegister |= PS2_STAT_KBD_ENABLE;

    /* We do not have any timeouts nor parity errors */
    StatusRegister &= ~(PS2_STAT_PARITY_ERROR | PS2_STAT_GEN_TIMEOUT);

    return StatusRegister;
}

static BYTE WINAPI PS2ReadData(USHORT Port)
{
    UNREFERENCED_PARAMETER(Port);

    /*
     * If there is something to read (response byte from the
     * controller or data from a PS/2 device), read it.
     */
    StatusRegister &= ~PS2_STAT_OUT_BUF_FULL;

    // Keep the state of the "Auxiliary output buffer full" flag
    // in order to remember from where the data came from.
    // StatusRegister &= ~PS2_STAT_AUX_OUT_BUF_FULL;

    // FIXME: We may check there whether there is data latched in
    // PS2 ports 1 or 2 (keyboard or mouse) and retrieve it there...

    /* Always return the available byte stored in the output buffer */
    return OutputBuffer;
}

static VOID WINAPI PS2WriteControl(USHORT Port, BYTE Data)
{
    UNREFERENCED_PARAMETER(Port);

    switch (Data)
    {
        /* Read configuration byte (byte 0 from internal memory) */
        case 0x20:
        /* Read byte N from internal memory */
                   case 0x21: case 0x22: case 0x23:
        case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x2C: case 0x2D: case 0x2E: case 0x2F:
        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
        {
            OutputBuffer = Memory[Data & 0x1F];
            StatusRegister |= PS2_STAT_OUT_BUF_FULL;
            break;
        }

        /* Write configuration byte (byte 0 from internal memory) */
        case 0x60:
        /* Write to byte N of internal memory */
                   case 0x61: case 0x62: case 0x63:
        case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6A: case 0x6B:
        case 0x6C: case 0x6D: case 0x6E: case 0x6F:
        case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B:
        case 0x7C: case 0x7D: case 0x7E: case 0x7F:

        /* Write controller output port */
        case 0xD1:
        /* Write to the first PS/2 port output buffer */
        case 0xD2:
        /* Write to the second PS/2 port output buffer */
        case 0xD3:
        /* Write to the second PS/2 port input buffer */
        case 0xD4:
        {
            /* These commands require a response */
            ControllerCommand = Data;
            StatusRegister |= PS2_STAT_COMMAND;
            break;
        }

        /* Disable second PS/2 port */
        case 0xA7:
        {
            ControllerConfig |= PS2_CONFIG_AUX_DISABLE;
            Ports[1].IsEnabled = FALSE;
            break;
        }

        /* Enable second PS/2 port */
        case 0xA8:
        {
            ControllerConfig &= ~PS2_CONFIG_AUX_DISABLE;
            Ports[1].IsEnabled = TRUE;
            break;
        }

        /* Test second PS/2 port */
        case 0xA9:
        {
            OutputBuffer = 0x00; // Success code
            StatusRegister |= PS2_STAT_OUT_BUF_FULL;
            break;
        }

        /* Test PS/2 controller */
        case 0xAA:
        {
            OutputBuffer = 0x55; // Success code
            StatusRegister |= PS2_STAT_OUT_BUF_FULL;
            break;
        }

        /* Test first PS/2 port */
        case 0xAB:
        {
            OutputBuffer = 0x00; // Success code
            StatusRegister |= PS2_STAT_OUT_BUF_FULL;
            break;
        }

        /* Disable first PS/2 port */
        case 0xAD:
        {
            ControllerConfig |= PS2_CONFIG_KBD_DISABLE;
            Ports[0].IsEnabled = FALSE;
            break;
        }

        /* Enable first PS/2 port */
        case 0xAE:
        {
            ControllerConfig &= ~PS2_CONFIG_KBD_DISABLE;
            Ports[0].IsEnabled = TRUE;
            break;
        }

        /* Read controller output port */
        case 0xD0:
        {
            /* Bit 0 always set, and bit 1 is the A20 gate state */
            OutputBuffer = (!!EmulatorGetA20() << 1) | PS2_OUT_CPU_NO_RESET;

            /* Set IRQ1 state */
            if (ControllerConfig & PS2_CONFIG_KBD_INT)
                OutputBuffer |=  PS2_OUT_IRQ01;
            else
                OutputBuffer &= ~PS2_OUT_IRQ01;

            /* Set IRQ12 state */
            if (ControllerConfig & PS2_CONFIG_AUX_INT)
                OutputBuffer |=  PS2_OUT_IRQ12;
            else
                OutputBuffer &= ~PS2_OUT_IRQ12;

            /* Check whether data is already present */
            if (StatusRegister & PS2_STAT_OUT_BUF_FULL)
            {
                if (StatusRegister & PS2_STAT_AUX_OUT_BUF_FULL)
                    OutputBuffer |= PS2_OUT_AUX_DATA;
                else
                    OutputBuffer |= PS2_OUT_KBD_DATA;
            }

            StatusRegister |= PS2_STAT_OUT_BUF_FULL;
            break;
        }

        /* CPU Reset */
        case 0xF0: case 0xF2: case 0xF4: case 0xF6:
        case 0xF8: case 0xFA: case 0xFC: case 0xFE:
        {
            /* Stop the VDM */
            EmulatorTerminate();
            return;
        }
    }
}

static VOID WINAPI PS2WriteData(USHORT Port, BYTE Data)
{
    /* Check if the controller is waiting for a response */
    if (StatusRegister & PS2_STAT_COMMAND)
    {
        StatusRegister &= ~PS2_STAT_COMMAND;

        /* Check which command it was */
        switch (ControllerCommand)
        {
            /* Write configuration byte (byte 0 from internal memory) */
            case 0x60:
            {
                ControllerConfig = Data;

                /*
                 * Synchronize the enable state of the PS/2 ports
                 * with the flags in the configuration byte.
                 */
                Ports[0].IsEnabled = !(ControllerConfig & PS2_CONFIG_KBD_DISABLE);
                Ports[1].IsEnabled = !(ControllerConfig & PS2_CONFIG_AUX_DISABLE);

                /*
                 * Update the "System enabled" flag of the status register
                 * with bit 2 of the controller configuration byte.
                 * See: https://aeb.win.tue.nl/linux/kbd/scancodes-11.html#kccb2
                 * for more details.
                 */
                if (ControllerConfig & PS2_CONFIG_SYSTEM)
                    StatusRegister |=  PS2_STAT_SYSTEM;
                else
                    StatusRegister &= ~PS2_STAT_SYSTEM;

                /*
                 * Update the "Keyboard enable" flag of the status register
                 * with the "Ignore keyboard lock" flag of the controller
                 * configuration byte (if set), then reset the latter one.
                 * See: https://aeb.win.tue.nl/linux/kbd/scancodes-11.html#kccb3
                 * for more details.
                 */
                if (ControllerConfig & PS2_CONFIG_NO_KEYLOCK)
                {
                    ControllerConfig &= ~PS2_CONFIG_NO_KEYLOCK;
                    StatusRegister   |=  PS2_STAT_KBD_ENABLE;
                }

                break;
            }

            /* Write to byte N of internal memory */
                       case 0x61: case 0x62: case 0x63:
            case 0x64: case 0x65: case 0x66: case 0x67:
            case 0x68: case 0x69: case 0x6A: case 0x6B:
            case 0x6C: case 0x6D: case 0x6E: case 0x6F:
            case 0x70: case 0x71: case 0x72: case 0x73:
            case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x78: case 0x79: case 0x7A: case 0x7B:
            case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            {
                Memory[ControllerCommand & 0x1F] = Data;
                break;
            }

            /* Write controller output */
            case 0xD1:
            {
                /* Check if bit 0 is unset */
                if (!(Data & PS2_OUT_CPU_NO_RESET))
                {
                    /* CPU disabled - Stop the VDM */
                    EmulatorTerminate();
                    return;
                }

                /* Update the A20 line setting */
                EmulatorSetA20(Data & PS2_OUT_A20_SET);

                // FIXME: Should we need to add the status of IRQ1 and IRQ12??

                break;
            }

            /* Push the data byte into the first PS/2 port queue */
            case 0xD2:
            {
                PS2QueuePush(0, Data);
                break;
            }

            /* Push the data byte into the second PS/2 port queue */
            case 0xD3:
            {
                PS2QueuePush(1, Data);
                break;
            }

            /*
             * Send a command to the second PS/2 port (by default
             * it is a command for the first PS/2 port)
             */
            case 0xD4:
            {
                PS2SendCommand(1, Data);
                break;
            }
        }

        return;
    }

    /* By default, send a command to the first PS/2 port */
    PS2SendCommand(0, Data);
}

static VOID FASTCALL GeneratePS2Irq(ULONGLONG ElapsedTime)
{
    UNREFERENCED_PARAMETER(ElapsedTime);

    /*
     * Pop out fresh new data from the PS/2 port output queues, and only
     * in case there is data ready, generate an IRQ1 or IRQ12 depending
     * on whether interrupts are enabled for the given port.
     *
     * NOTE: The first PS/2 port (keyboard) has priority over the second one (mouse).
     */
    if (PS2PortQueueRead(0))
    {
        if (ControllerConfig & PS2_CONFIG_KBD_INT) PicInterruptRequest(1);
    }
    else if (PS2PortQueueRead(1))
    {
        if (ControllerConfig & PS2_CONFIG_AUX_INT) PicInterruptRequest(12);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN PS2PortQueueRead(BYTE PS2Port)
{
    BOOLEAN Result = FALSE;
    PPS2_PORT Port;

    // NOTE: The first PS/2 port (keyboard) has priority over the second one (mouse).

    Port = &Ports[PS2Port];

    if (!Port->IsEnabled) return FALSE;

    /* Make sure the queue is not empty (fast check) */
    if (Port->QueueEmpty)
    {
        /* Only the keyboard should have its last data latched */
        // FIXME: Alternatively this can be done in PS2ReadData when
        // we read PS2_DATA_PORT. What is the best solution??
        if (PS2Port == 0)
        {
            OutputBuffer = Port->Queue[(Port->QueueStart - 1) % BUFFER_SIZE];
            StatusRegister &= ~PS2_STAT_AUX_OUT_BUF_FULL; // Clear flag: keyboard data
        }

        return FALSE;
    }

    WaitForSingleObject(Port->QueueMutex, INFINITE);

    /*
     * Recheck whether the queue is not empty (it may
     * have changed after having grabbed the mutex).
     */
    if (Port->QueueEmpty) goto Done;

    /* Get the data */
    OutputBuffer = Port->Queue[Port->QueueStart];

    // StatusRegister &= ~(PS2_STAT_AUX_OUT_BUF_FULL | PS2_STAT_OUT_BUF_FULL);

    /* Always set the "Output buffer full" flag */
    StatusRegister |= PS2_STAT_OUT_BUF_FULL;

    /* Set the "Auxiliary output buffer full" flag according to where the data came from */
    if (PS2Port == 0)
        StatusRegister &= ~PS2_STAT_AUX_OUT_BUF_FULL; // Clear flag: keyboard data
    else // if (PS2Port == 1)
        StatusRegister |=  PS2_STAT_AUX_OUT_BUF_FULL; //   Set flag: mouse data

    /* Remove the value from the queue */
    Port->QueueStart++;
    Port->QueueStart %= BUFFER_SIZE;

    /* Check if the queue is now empty */
    if (Port->QueueStart == Port->QueueEnd)
        Port->QueueEmpty = TRUE;

    Result = TRUE;

Done:
    ReleaseMutex(Port->QueueMutex);
    return Result;
}

VOID PS2SetDeviceCmdProc(BYTE PS2Port, LPVOID Param, PS2_DEVICE_CMDPROC DeviceCommand)
{
    ASSERT(PS2Port < PS2_PORTS);
    Ports[PS2Port].Param         = Param;
    Ports[PS2Port].DeviceCommand = DeviceCommand;
}

// PS2SendToPort
BOOLEAN PS2QueuePush(BYTE PS2Port, BYTE Data)
{
    BOOLEAN Result = FALSE;
    PPS2_PORT Port;

    ASSERT(PS2Port < PS2_PORTS);
    Port = &Ports[PS2Port];

    if (!Port->IsEnabled) return FALSE;

    WaitForSingleObject(Port->QueueMutex, INFINITE);

    /* Check if the queue is full */
    if (!Port->QueueEmpty && (Port->QueueStart == Port->QueueEnd))
        goto Done;

    /* Insert the value in the queue */
    Port->Queue[Port->QueueEnd] = Data;
    Port->QueueEnd++;
    Port->QueueEnd %= BUFFER_SIZE;

    /* The queue is not empty anymore */
    Port->QueueEmpty = FALSE;

    /* Schedule the IRQ */
    EnableHardwareTimer(IrqTimer);

    Result = TRUE;

Done:
    ReleaseMutex(Port->QueueMutex);
    return Result;
}

BOOLEAN PS2Initialize(VOID)
{
    /* Initialize the PS/2 ports */
    Ports[0].IsEnabled  = FALSE;
    Ports[0].QueueEmpty = TRUE;
    Ports[0].QueueStart = 0;
    Ports[0].QueueEnd   = 0;
    Ports[0].QueueMutex = CreateMutex(NULL, FALSE, NULL);

    Ports[1].IsEnabled  = FALSE;
    Ports[1].QueueEmpty = TRUE;
    Ports[1].QueueStart = 0;
    Ports[1].QueueEnd   = 0;
    Ports[1].QueueMutex = CreateMutex(NULL, FALSE, NULL);

    /* Register the I/O Ports */
    RegisterIoPort(PS2_CONTROL_PORT, PS2ReadControl, PS2WriteControl);
    RegisterIoPort(PS2_DATA_PORT   , PS2ReadData   , PS2WriteData   );

    IrqTimer = CreateHardwareTimer(HARDWARE_TIMER_ONESHOT,
                                   HZ_TO_NS(100),
                                   GeneratePS2Irq);

    return TRUE;
}

VOID PS2Cleanup(VOID)
{
    DestroyHardwareTimer(IrqTimer);

    CloseHandle(Ports[1].QueueMutex);
    CloseHandle(Ports[0].QueueMutex);
}

/* EOF */
