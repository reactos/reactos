/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Intel 8255A PPI header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define PPI_IO_o_PORT_C     0x35

#define PPI_IO_o_CONTROL    0x37
    #define PPI_TIMER_1_GATE_TO_SPEAKER      0x06
    #define PPI_TIMER_1_UNGATE_TO_SPEAKER    0x07
    #define PPI_SHUTDOWN_1_ENABLE            0x0B
    #define PPI_SHUTDOWN_0_ENABLE            0x0F

#define PPI_IO_i_PORT_A     0x31
#define PPI_IO_i_PORT_B     0x33
#define PPI_IO_i_PORT_C     0x35

typedef union _SYSTEM_CONTROL_PORT_A_REGISTER
{
    struct
    {
        UCHAR DipSw2_1:1;
        UCHAR DipSw2_2:1;
        UCHAR DipSw2_3:1;
        UCHAR DipSw2_4:1;
        UCHAR DipSw2_5:1;
        UCHAR DipSw2_6:1;
        UCHAR DipSw2_7:1;
        UCHAR DipSw2_8:1;
    };
    UCHAR Bits;
} SYSTEM_CONTROL_PORT_A_REGISTER, *PSYSTEM_CONTROL_PORT_A_REGISTER;

typedef union _SYSTEM_CONTROL_PORT_B_REGISTER
{
    struct
    {
        UCHAR RtcData:1;

        /* NMI */
        UCHAR ExtendedMemoryParityCheck:1;
        UCHAR MemoryParityCheck:1;

        UCHAR HighResolution:1;
        UCHAR Int3:1;
        UCHAR DataCarrierDetect:1;
        UCHAR ClearToSend:1;
        UCHAR RingIndicator:1;
    };
    UCHAR Bits;
} SYSTEM_CONTROL_PORT_B_REGISTER, *PSYSTEM_CONTROL_PORT_B_REGISTER;

typedef union _SYSTEM_CONTROL_PORT_C_REGISTER
{
    struct
    {
        UCHAR InterruptEnableRxReady:1;
        UCHAR InterruptEnableTxEmpty:1;
        UCHAR InterruptEnableTxReady:1;
        UCHAR Timer1GateToSpeaker:1;
        UCHAR Mcke:1;
        UCHAR Shut1:1;
        UCHAR PrinterStrobeSignal:1;
        UCHAR Shut0:1;
    };
    UCHAR Bits;
} SYSTEM_CONTROL_PORT_C_REGISTER, *PSYSTEM_CONTROL_PORT_C_REGISTER;
