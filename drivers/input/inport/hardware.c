/*
 * PROJECT:     ReactOS InPort (Bus) Mouse Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware support code
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* Note: Some code was taken from Linux */

/* INCLUDES *******************************************************************/

#include "inport.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InPortInitializeMouse)
#endif

#define READ_MOUSE(DeviceExtension, Port) \
    READ_PORT_UCHAR((DeviceExtension)->IoBase + (Port))

#define WRITE_MOUSE(DeviceExtension, Port, Data) \
    WRITE_PORT_UCHAR((DeviceExtension)->IoBase + (Port), (Data))

/*
 * NEC
 */
#define NEC_BM_DATA          0x00

#define NEC_BM_CONTROL       0x04
    #define NEC_INT_ENABLE       0x00
    #define NEC_INT_DISABLE      0x10

    #define NEC_READ_X_LOW       0x00
    #define NEC_READ_X_HIGH      0x20
    #define NEC_READ_Y_LOW       0x40
    #define NEC_READ_Y_HIGH      0x60

    #define NEC_INPUT_CAPTURE    0x00
    #define NEC_INPUT_HOLD       0x80

#define NEC_BM_CONFIG        0x06
    #define NEC_PPI_INT_ENABLE   0x08
    #define NEC_PPI_INT_DISABLE  0x09
    #define NEC_PPI_HC_NO_CLEAR  0x0E
    #define NEC_PPI_HC_CLEAR     0x0F
    #define NEC_PPI_DEFAULT_MODE 0x93

#define NEC_BM_INT_RATE      0x4002
    #define NEC_RATE_120_HZ      0x00
    #define NEC_RATE_60_HZ       0x01
    #define NEC_RATE_30_HZ       0x02
    #define NEC_RATE_15_HZ       0x03

#define NEC_BM_HIRESO_BASE   (PUCHAR)0x61

#define NEC_BUTTON_RIGHT    0x20
#define NEC_BUTTON_LEFT     0x80

/*
 * Microsoft InPort
 */
#define MS_INPORT_CONTROL    0x00
    #define INPORT_REG_BTNS    0x00
    #define INPORT_REG_X       0x01
    #define INPORT_REG_Y       0x02
    #define INPORT_REG_MODE    0x07
    #define INPORT_RESET       0x80

#define MS_INPORT_DATA       0x01
    #define INPORT_MODE_IRQ    0x01
    #define INPORT_MODE_BASE   0x10
    #define INPORT_MODE_HOLD   0x20

#define MS_INPORT_SIGNATURE  0x02

#define MS_BUTTON_MIDDLE   0x01
#define MS_BUTTON_LEFT     0x02
#define MS_BUTTON_RIGHT    0x04

/*
 * Logitech
 */
#define LOG_BM_DATA          0x00

#define LOG_BM_SIGNATURE     0x01
    #define LOG_SIGNATURE_BYTE 0xA5

#define LOG_BM_CONTROL       0x02
    #define LOG_ENABLE_IRQ     0x00
    #define LOG_DISABLE_IRQ    0x10

    #define LOG_READ_X_LOW     0x80
    #define LOG_READ_X_HIGH    0xA0
    #define LOG_READ_Y_LOW     0xC0
    #define LOG_READ_Y_HIGH    0xE0

#define LOG_BM_CONFIG        0x03
    #define LOG_DEFAULT_MODE   0x90
    #define LOG_CONFIG_BYTE    0x91

#define LOG_BUTTON_RIGHT     0x20
#define LOG_BUTTON_MIDDLE    0x40
#define LOG_BUTTON_LEFT      0x80

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
InPortDpcForIsr(
    _In_ PKDPC Dpc,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_opt_ PVOID Context)
{
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL OldIrql;
    ULONG DummyInputDataConsumed;
    INPORT_RAW_DATA RawData;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    /* Copy raw data */
    OldIrql = KeAcquireInterruptSpinLock(DeviceExtension->InterruptObject);
    RawData = DeviceExtension->RawData;
    KeReleaseInterruptSpinLock(DeviceExtension->InterruptObject, OldIrql);

    /* Fill out fields */
    DeviceExtension->MouseInputData.LastX = RawData.DeltaX;
    DeviceExtension->MouseInputData.LastY = RawData.DeltaY;
    DeviceExtension->MouseInputData.ButtonFlags = 0;
    if (RawData.ButtonDiff != 0)
    {
        switch (DeviceExtension->MouseType)
        {
            case NecBusMouse:
            {
                if (RawData.ButtonDiff & NEC_BUTTON_LEFT)
                {
                    if (RawData.Buttons & NEC_BUTTON_LEFT)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_UP;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
                }
                if (RawData.ButtonDiff & NEC_BUTTON_RIGHT)
                {
                    if (RawData.Buttons & NEC_BUTTON_RIGHT)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_UP;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_DOWN;
                }

                break;
            }

            case MsInPortMouse:
            {
                /* Button flags have to be inverted */
                if (RawData.ButtonDiff & MS_BUTTON_LEFT)
                {
                    if (RawData.Buttons & MS_BUTTON_LEFT)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_UP;
                }
                if (RawData.ButtonDiff & MS_BUTTON_RIGHT)
                {
                    if (RawData.Buttons & MS_BUTTON_RIGHT)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_DOWN;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_UP;
                }
                if (RawData.ButtonDiff & MS_BUTTON_MIDDLE)
                {
                    if (RawData.Buttons & MS_BUTTON_MIDDLE)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_MIDDLE_BUTTON_DOWN;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_MIDDLE_BUTTON_UP;
                }

                break;
            }

            case LogitechBusMouse:
            {
                if (RawData.ButtonDiff & LOG_BUTTON_LEFT)
                {
                    if (RawData.Buttons & LOG_BUTTON_LEFT)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_UP;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
                }
                if (RawData.ButtonDiff & LOG_BUTTON_RIGHT)
                {
                    if (RawData.Buttons & LOG_BUTTON_RIGHT)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_UP;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_RIGHT_BUTTON_DOWN;
                }
                if (RawData.ButtonDiff & LOG_BUTTON_MIDDLE)
                {
                    if (RawData.Buttons & LOG_BUTTON_MIDDLE)
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_MIDDLE_BUTTON_UP;
                    else
                        DeviceExtension->MouseInputData.ButtonFlags |= MOUSE_MIDDLE_BUTTON_DOWN;
                }

                break;
            }
        }
    }

    /* Send mouse packet */
    (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassService)(
        DeviceExtension->ClassDeviceObject,
        &DeviceExtension->MouseInputData,
        &DeviceExtension->MouseInputData + 1,
        &DummyInputDataConsumed);
}

BOOLEAN
NTAPI
InPortIsr(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID Context)
{
    UCHAR Buttons;
    ULONG ButtonDiff;
    CHAR DeltaX, DeltaY;
    PINPORT_DEVICE_EXTENSION DeviceExtension = Context;

    UNREFERENCED_PARAMETER(Interrupt);

    switch (DeviceExtension->MouseType)
    {
        case NecBusMouse:
        {
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONTROL,
                        NEC_INPUT_CAPTURE | NEC_INT_DISABLE);

            WRITE_MOUSE(DeviceExtension, NEC_BM_CONTROL,
                        NEC_INPUT_HOLD | NEC_INT_DISABLE | NEC_READ_X_LOW);
            DeltaX = READ_MOUSE(DeviceExtension, NEC_BM_DATA) & 0x0F;

            WRITE_MOUSE(DeviceExtension, NEC_BM_CONTROL,
                        NEC_INPUT_HOLD | NEC_INT_DISABLE | NEC_READ_X_HIGH);
            DeltaX |= READ_MOUSE(DeviceExtension, NEC_BM_DATA) << 4;

            WRITE_MOUSE(DeviceExtension, NEC_BM_CONTROL,
                        NEC_INPUT_HOLD | NEC_INT_DISABLE | NEC_READ_Y_LOW);
            DeltaY = READ_MOUSE(DeviceExtension, NEC_BM_DATA) & 0x0F;

            WRITE_MOUSE(DeviceExtension, NEC_BM_CONTROL,
                        NEC_INPUT_HOLD | NEC_INT_DISABLE | NEC_READ_Y_HIGH);
            Buttons = READ_MOUSE(DeviceExtension, NEC_BM_DATA);
            DeltaY |= Buttons << 4;
            Buttons &= (NEC_BUTTON_LEFT | NEC_BUTTON_RIGHT);

            WRITE_MOUSE(DeviceExtension, NEC_BM_CONTROL,
                        NEC_INPUT_HOLD | NEC_INT_ENABLE);

            break;
        }

        case MsInPortMouse:
        {
            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_MODE);
            WRITE_MOUSE(DeviceExtension, MS_INPORT_DATA,
                        INPORT_MODE_HOLD | INPORT_MODE_IRQ | INPORT_MODE_BASE);

            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_X);
            DeltaX = READ_MOUSE(DeviceExtension, MS_INPORT_DATA);

            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_Y);
            DeltaY = READ_MOUSE(DeviceExtension, MS_INPORT_DATA);

            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_BTNS);
            Buttons = READ_MOUSE(DeviceExtension, MS_INPORT_DATA);
            Buttons &= (MS_BUTTON_MIDDLE | MS_BUTTON_LEFT | MS_BUTTON_RIGHT);

            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_MODE);
            WRITE_MOUSE(DeviceExtension, MS_INPORT_DATA,
                        INPORT_MODE_IRQ | INPORT_MODE_BASE);

            break;
        }

        case LogitechBusMouse:
        {
            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_READ_X_LOW);
            DeltaX = READ_MOUSE(DeviceExtension, LOG_BM_DATA) & 0x0F;

            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_READ_X_HIGH);
            DeltaX |= READ_MOUSE(DeviceExtension, LOG_BM_DATA) << 4;

            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_READ_Y_LOW);
            DeltaY = READ_MOUSE(DeviceExtension, LOG_BM_DATA) & 0x0F;

            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_READ_Y_HIGH);
            Buttons = READ_MOUSE(DeviceExtension, LOG_BM_DATA);
            DeltaY |= Buttons << 4;
            Buttons &= (LOG_BUTTON_RIGHT | LOG_BUTTON_MIDDLE | LOG_BUTTON_LEFT);

            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_ENABLE_IRQ);

            break;
        }
    }

    ButtonDiff = DeviceExtension->MouseButtonState ^ Buttons;
    DeviceExtension->MouseButtonState = Buttons;

    /*
     * Bus mouse devices don't have a status register to check
     * whether this interrupt is indeed for us.
     */
    if ((DeltaX == 0) && (DeltaY == 0) && (ButtonDiff == 0))
    {
        /* We just pretend that the interrupt is not ours */
        return FALSE;
    }
    else
    {
        DeviceExtension->RawData.DeltaX = DeltaX;
        DeviceExtension->RawData.DeltaY = DeltaY;
        DeviceExtension->RawData.Buttons = Buttons;
        DeviceExtension->RawData.ButtonDiff = ButtonDiff;

        IoRequestDpc(DeviceExtension->Self, NULL, NULL);

        return TRUE;
    }
}

VOID
NTAPI
InPortInitializeMouse(
    _In_ PINPORT_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    /* Initialize mouse and disable interrupts */
    switch (DeviceExtension->MouseType)
    {
        case NecBusMouse:
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_DEFAULT_MODE);

            /* Setup interrupt rate (unavailable on hireso machines) */
            if (DeviceExtension->IoBase != NEC_BM_HIRESO_BASE)
                WRITE_MOUSE(DeviceExtension, NEC_BM_INT_RATE, NEC_RATE_60_HZ);

            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_INT_DISABLE);
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_HC_NO_CLEAR);
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_HC_CLEAR);
            break;

        case MsInPortMouse:
            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_RESET);
            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_MODE);
            WRITE_MOUSE(DeviceExtension, MS_INPORT_DATA, INPORT_MODE_BASE);
            break;

        case LogitechBusMouse:
            WRITE_MOUSE(DeviceExtension, LOG_BM_CONFIG, LOG_DEFAULT_MODE);
            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_DISABLE_IRQ);
            break;
    }
}

BOOLEAN
NTAPI
InPortStartMouse(
    _In_ PVOID SynchronizeContext)
{
    PINPORT_DEVICE_EXTENSION DeviceExtension = SynchronizeContext;

    /* Enable interrupts */
    switch (DeviceExtension->MouseType)
    {
        case NecBusMouse:
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_INT_ENABLE);
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_HC_NO_CLEAR);
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_HC_CLEAR);
            break;

        case MsInPortMouse:
            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_MODE);
            WRITE_MOUSE(DeviceExtension, MS_INPORT_DATA,
                        INPORT_MODE_IRQ | INPORT_MODE_BASE);
            break;

        case LogitechBusMouse:
            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_ENABLE_IRQ);
            break;
    }

    return TRUE;
}

BOOLEAN
NTAPI
InPortStopMouse(
    _In_ PVOID SynchronizeContext)
{
    PINPORT_DEVICE_EXTENSION DeviceExtension = SynchronizeContext;

    /* Disable interrupts */
    switch (DeviceExtension->MouseType)
    {
        case NecBusMouse:
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_INT_DISABLE);
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_HC_NO_CLEAR);
            WRITE_MOUSE(DeviceExtension, NEC_BM_CONFIG, NEC_PPI_HC_CLEAR);
            break;

        case MsInPortMouse:
            WRITE_MOUSE(DeviceExtension, MS_INPORT_CONTROL, INPORT_REG_MODE);
            WRITE_MOUSE(DeviceExtension, MS_INPORT_DATA, INPORT_MODE_BASE);
            break;

        case LogitechBusMouse:
            WRITE_MOUSE(DeviceExtension, LOG_BM_CONTROL, LOG_DISABLE_IRQ);
            break;
    }

    return TRUE;
}
