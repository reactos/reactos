/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ps2.c
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "ps2.h"

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

#define PS2_DEFAULT_CONFIG  0x45
static BYTE ControllerConfig = PS2_DEFAULT_CONFIG;
static BYTE ControllerCommand = 0x00;

static BYTE StatusRegister = 0x00;
// static BYTE InputBuffer  = 0x00; // PS/2 Input  Buffer
static BYTE OutputBuffer = 0x00; // PS/2 Output Buffer

static PHARDWARE_TIMER IrqTimer = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID PS2SendCommand(PPS2_PORT Port, BYTE Command)
{
    if (!Port->IsEnabled) return;

    /* Call the device command */
    if (Port->DeviceCommand) Port->DeviceCommand(Port->Param, Command);
}

static BYTE WINAPI PS2ReadPort(USHORT Port)
{
    if (Port == PS2_CONTROL_PORT)
    {
        /* Be sure bit 2 is always set */
        StatusRegister |= 1 << 2;

        // FIXME: Should clear bits 6 and 7 because there are
        // no timeouts and no parity errors.

        return StatusRegister;
    }
    else if (Port == PS2_DATA_PORT)
    {
        /*
         * If there is something to read (response byte from the
         * controller or data from a PS/2 device), read it.
         */
        if (StatusRegister &   (1 << 0)) // || StatusRegister &   (1 << 5) for second PS/2 port
            StatusRegister &= ~(1 << 0); //    StatusRegister &= ~(1 << 5);

        // FIXME: We may check there whether there is data latched in
        // PS2 ports 1 or 2 (keyboard or mouse) and retrieve it there...

        /* Always return the available byte stored in the output buffer */
        return OutputBuffer;
    }

    return 0x00;
}

static VOID WINAPI PS2WritePort(USHORT Port, BYTE Data)
{
    if (Port == PS2_CONTROL_PORT)
    {
        switch (Data)
        {
            /* Read configuration byte */
            case 0x20:
            {
                OutputBuffer = ControllerConfig;
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Write configuration byte */
            case 0x60:
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
                StatusRegister |= (1 << 3); // This is a controller command
                break;
            }

            /* Disable second PS/2 port */
            case 0xA7:
            {
                Ports[1].IsEnabled = FALSE;
                break;
            }

            /* Enable second PS/2 port */
            case 0xA8:
            {
                Ports[1].IsEnabled = TRUE;
                break;
            }

            /* Test second PS/2 port */
            case 0xA9:
            {
                OutputBuffer = 0x00; // Success code
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Test PS/2 controller */
            case 0xAA:
            {
                OutputBuffer = 0x55; // Success code
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Test first PS/2 port */
            case 0xAB:
            {
                OutputBuffer = 0x00; // Success code
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Disable first PS/2 port */
            case 0xAD:
            {
                Ports[0].IsEnabled = FALSE;
                break;
            }

            /* Enable first PS/2 port */
            case 0xAE:
            {
                Ports[0].IsEnabled = TRUE;
                break;
            }

            /* Read controller output port */
            case 0xD0:
            {
                /* Bit 0 always set, and bit 1 is the A20 gate state */
                OutputBuffer = (!!EmulatorGetA20() << 1) | 0x01;
                // FIXME: Set the status of IRQ1 and IRQ12

                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* CPU Reset */
            case 0xF0:
            case 0xF2:
            case 0xF4:
            case 0xF6:
            case 0xF8:
            case 0xFA:
            case 0xFC:
            case 0xFE:
            {
                /* Stop the VDM */
                EmulatorTerminate();
                break;
            }
        }
    }
    else if (Port == PS2_DATA_PORT)
    {
        /* Check if the controller is waiting for a response */
        if (StatusRegister & (1 << 3)) // If we have data for the controller
        {
            StatusRegister &= ~(1 << 3);

            /* Check which command it was */
            switch (ControllerCommand)
            {
                /* Write configuration byte */
                case 0x60:
                {
                    ControllerConfig = Data;
                    break;
                }

                /* Write controller output */
                case 0xD1:
                {
                    /* Check if bit 0 is unset */
                    if (!(Data & (1 << 0)))
                    {
                        /* CPU disabled - Stop the VDM */
                        EmulatorTerminate();
                    }

                    /* Update the A20 line setting */
                    EmulatorSetA20(Data & (1 << 1));

                    // FIXME: Add the status of IRQ1 and IRQ12

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
                    PS2SendCommand(&Ports[1], Data);
                    break;
                }
            }

            return;
        }

        /* By default, send a command to the first PS/2 port */
        PS2SendCommand(&Ports[0], Data);
    }
}

static VOID FASTCALL GeneratePS2Irq(ULONGLONG ElapsedTime)
{
    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Generate an IRQ 1 if there is data ready in the output queue */
    if (PS2PortQueueRead(0))
    {
        /* Generate an interrupt if interrupts for the first PS/2 port are enabled */
        if (ControllerConfig & 0x01) PicInterruptRequest(1);
        return;
    }

    /* Generate an IRQ 12 if there is data ready in the output queue */
    if (PS2PortQueueRead(1))
    {
        /* Generate an interrupt if interrupts for the second PS/2 port are enabled */
        if (ControllerConfig & 0x02) PicInterruptRequest(12);
        return;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN PS2PortQueueRead(BYTE PS2Port)
{
    BOOLEAN Result = TRUE;
    PPS2_PORT Port;

    if (PS2Port >= PS2_PORTS) return FALSE;
    Port = &Ports[PS2Port];

    if (!Port->IsEnabled) return FALSE;

    /* Make sure the queue is not empty (fast check) */
    if (Port->QueueEmpty)
    {
        /* Only the keyboard should have its last data latched */
        // FIXME: Alternatively this can be done in PS2ReadPort when
        // we read PS2_DATA_PORT. What is the best solution??
        if (PS2Port == 0)
        {
            OutputBuffer = Port->Queue[(Port->QueueStart - 1) % BUFFER_SIZE];
        }

        return FALSE;
    }

    WaitForSingleObject(Port->QueueMutex, INFINITE);

    /*
     * Recheck whether the queue is not empty (it may
     * have changed after having grabbed the mutex).
     */
    if (Port->QueueEmpty)
    {
        Result = FALSE;
        goto Done;
    }

    /* Get the data */
    OutputBuffer = Port->Queue[Port->QueueStart];
    StatusRegister |= (1 << 0); // There is something to read
    // Sometimes StatusRegister |= (1 << 5); for the second PS/2 port

    /* Remove the value from the queue */
    Port->QueueStart++;
    Port->QueueStart %= BUFFER_SIZE;

    /* Check if the queue is now empty */
    if (Port->QueueStart == Port->QueueEnd)
        Port->QueueEmpty = TRUE;

Done:
    ReleaseMutex(Port->QueueMutex);
    return Result;
}

VOID PS2SetDeviceCmdProc(BYTE PS2Port, LPVOID Param, PS2_DEVICE_CMDPROC DeviceCommand)
{
    if (PS2Port >= PS2_PORTS) return;

    Ports[PS2Port].Param         = Param;
    Ports[PS2Port].DeviceCommand = DeviceCommand;
}

// PS2SendToPort
BOOLEAN PS2QueuePush(BYTE PS2Port, BYTE Data)
{
    BOOLEAN Result = TRUE;
    PPS2_PORT Port;

    if (PS2Port >= PS2_PORTS) return FALSE;
    Port = &Ports[PS2Port];

    if (!Port->IsEnabled) return FALSE;

    WaitForSingleObject(Port->QueueMutex, INFINITE);

    /* Check if the queue is full */
    if (!Port->QueueEmpty && (Port->QueueStart == Port->QueueEnd))
    {
        Result = FALSE;
        goto Done;
    }

    /* Insert the value in the queue */
    Port->Queue[Port->QueueEnd] = Data;
    Port->QueueEnd++;
    Port->QueueEnd %= BUFFER_SIZE;

    /* The queue is not empty anymore */
    Port->QueueEmpty = FALSE;

    /* Schedule the IRQ */
    EnableHardwareTimer(IrqTimer);

Done:
    ReleaseMutex(Port->QueueMutex);
    return Result;
}

BOOLEAN PS2Initialize(VOID)
{
    /* Initialize the PS/2 ports */
    Ports[0].IsEnabled  = TRUE;
    Ports[0].QueueEmpty = TRUE;
    Ports[0].QueueStart = 0;
    Ports[0].QueueEnd   = 0;
    Ports[0].QueueMutex = CreateMutex(NULL, FALSE, NULL);

    Ports[1].IsEnabled  = TRUE;
    Ports[1].QueueEmpty = TRUE;
    Ports[1].QueueStart = 0;
    Ports[1].QueueEnd   = 0;
    Ports[1].QueueMutex = CreateMutex(NULL, FALSE, NULL);

    /* Register the I/O Ports */
    RegisterIoPort(PS2_CONTROL_PORT, PS2ReadPort, PS2WritePort);
    RegisterIoPort(PS2_DATA_PORT   , PS2ReadPort, PS2WritePort);

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
