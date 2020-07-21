/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PC/AT hardware header file
 * COPYRIGHT:   ...
 */

#pragma once

/* CMOS Registers and Ports */
#define CMOS_CONTROL_PORT       (PUCHAR)0x70
#define CMOS_DATA_PORT          (PUCHAR)0x71
#define RTC_REGISTER_A          0x0A
#define   RTC_REG_A_UIP         0x80
#define RTC_REGISTER_B          0x0B
#define   RTC_REG_B_PI          0x40
#define RTC_REGISTER_C          0x0C
#define   RTC_REG_C_IRQ         0x80
#define RTC_REGISTER_D          0x0D
#define RTC_REGISTER_CENTURY    0x32

//
// BIOS Interrupts
//
#define VIDEO_SERVICES   0x10

//
// Operations for INT 10h (in AH)
//
#define SET_VIDEO_MODE   0x00

//
// Video Modes for INT10h AH=00 (in AL)
//
#define GRAPHICS_MODE_12 0x12           /* 80x30  8x16  640x480  16/256K */

#if defined(SARCH_XBOX)
//
// For some unknown reason the PIT of the Xbox is fixed at 1.125000 MHz,
// which is ~5.7% lower than on the PC.
//
#define PIT_FREQUENCY 1125000
#else
//
// Commonly stated as being 1.19318MHz
//
// See ISA System Architecture 3rd Edition (Tom Shanley, Don Anderson, John Swindle)
// p. 471
//
// However, the true value is closer to 1.19318181[...]81MHz since this is 1/3rd
// of the NTSC color subcarrier frequency which runs at 3.57954545[...]45MHz.
//
// Note that Windows uses 1.193167MHz which seems to have no basis. However, if
// one takes the NTSC color subcarrier frequency as being 3.579545 (trimming the
// infinite series) and divides it by three, one obtains 1.19318167.
//
// It may be that the original NT HAL source code introduced a typo and turned
// 119318167 into 1193167 by ommitting the "18". This is very plausible as the
// number is quite long.
//
#define PIT_FREQUENCY 1193182
#endif

//
// These ports are controlled by the i8254 Programmable Interrupt Timer (PIT)
//
#define TIMER_CHANNEL0_DATA_PORT 0x40
#define TIMER_CHANNEL1_DATA_PORT 0x41
#define TIMER_CHANNEL2_DATA_PORT 0x42
#define TIMER_CONTROL_PORT       0x43

//
// Mode 0 - Interrupt On Terminal Count
// Mode 1 - Hardware Re-triggerable One-Shot
// Mode 2 - Rate Generator
// Mode 3 - Square Wave Generator
// Mode 4 - Software Triggered Strobe
// Mode 5 - Hardware Triggered Strobe
//
typedef enum _TIMER_OPERATING_MODES
{
    PitOperatingMode0,
    PitOperatingMode1,
    PitOperatingMode2,
    PitOperatingMode3,
    PitOperatingMode4,
    PitOperatingMode5,
    PitOperatingMode2Reserved,
    PitOperatingMode5Reserved
} TIMER_OPERATING_MODES;

typedef enum _TIMER_ACCESS_MODES
{
    PitAccessModeCounterLatch,
    PitAccessModeLow,
    PitAccessModeHigh,
    PitAccessModeLowHigh
} TIMER_ACCESS_MODES;

typedef enum _TIMER_CHANNELS
{
    PitChannel0,
    PitChannel1,
    PitChannel2,
    PitReadBack
} TIMER_CHANNELS;

typedef union _TIMER_CONTROL_PORT_REGISTER
{
    struct
    {
        UCHAR BcdMode:1;
        UCHAR OperatingMode:3;
        UCHAR AccessMode:2;
        UCHAR Channel:2;
    };
    UCHAR Bits;
} TIMER_CONTROL_PORT_REGISTER, *PTIMER_CONTROL_PORT_REGISTER;

//
// See ISA System Architecture 3rd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 400
//
// This port is controled by the i8255 Programmable Peripheral Interface (PPI)
//
#define SYSTEM_CONTROL_PORT_A   0x92
#define SYSTEM_CONTROL_PORT_B   0x61
typedef union _SYSTEM_CONTROL_PORT_B_REGISTER
{
    struct
    {
        UCHAR Timer2GateToSpeaker:1;
        UCHAR SpeakerDataEnable:1;
        UCHAR ParityCheckEnable:1;
        UCHAR ChannelCheckEnable:1;
        UCHAR RefreshRequest:1;
        UCHAR Timer2Output:1;
        UCHAR ChannelCheck:1;
        UCHAR ParityCheck:1;
    };
    UCHAR Bits;
} SYSTEM_CONTROL_PORT_B_REGISTER, *PSYSTEM_CONTROL_PORT_B_REGISTER;

//
// See ISA System Architecture 3rd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 396, 397
//
// These ports are controlled by the i8259 Programmable Interrupt Controller (PIC)
//
#define PIC1_CONTROL_PORT      0x20
#define PIC1_DATA_PORT         0x21
#define PIC2_CONTROL_PORT      0xA0
#define PIC2_DATA_PORT         0xA1

#define PIC_TIMER_IRQ      0
#define PIC_CASCADE_IRQ    2
#define PIC_RTC_IRQ        8

//
// Definitions for ICW/OCW Bits
//
typedef enum _I8259_ICW1_OPERATING_MODE
{
    Cascade,
    Single
} I8259_ICW1_OPERATING_MODE;

typedef enum _I8259_ICW1_INTERRUPT_MODE
{
    EdgeTriggered,
    LevelTriggered
} I8259_ICW1_INTERRUPT_MODE;

typedef enum _I8259_ICW1_INTERVAL
{
    Interval8,
    Interval4
} I8259_ICW1_INTERVAL;

typedef enum _I8259_ICW4_SYSTEM_MODE
{
    Mcs8085Mode,
    New8086Mode
} I8259_ICW4_SYSTEM_MODE;

typedef enum _I8259_ICW4_EOI_MODE
{
    NormalEoi,
    AutomaticEoi
} I8259_ICW4_EOI_MODE;

typedef enum _I8259_ICW4_BUFFERED_MODE
{
    NonBuffered,
    NonBuffered2,
    BufferedSlave,
    BufferedMaster
} I8259_ICW4_BUFFERED_MODE;

typedef enum _I8259_READ_REQUEST
{
    InvalidRequest,
    InvalidRequest2,
    ReadIdr,
    ReadIsr
} I8259_READ_REQUEST;

typedef enum _I8259_EOI_MODE
{
    RotateAutoEoiClear,
    NonSpecificEoi,
    InvalidEoiMode,
    SpecificEoi,
    RotateAutoEoiSet,
    RotateNonSpecific,
    SetPriority,
    RotateSpecific
} I8259_EOI_MODE;

//
// Definitions for ICW Registers
//
typedef union _I8259_ICW1
{
    struct
    {
        UCHAR NeedIcw4:1;
        UCHAR OperatingMode:1;
        UCHAR Interval:1;
        UCHAR InterruptMode:1;
        UCHAR Init:1;
        UCHAR InterruptVectorAddress:3;
    };
    UCHAR Bits;
} I8259_ICW1, *PI8259_ICW1;

typedef union _I8259_ICW2
{
    struct
    {
        UCHAR Sbz:3;
        UCHAR InterruptVector:5;
    };
    UCHAR Bits;
} I8259_ICW2, *PI8259_ICW2;

typedef union _I8259_ICW3
{
    union
    {
        struct
        {
            UCHAR SlaveIrq0:1;
            UCHAR SlaveIrq1:1;
            UCHAR SlaveIrq2:1;
            UCHAR SlaveIrq3:1;
            UCHAR SlaveIrq4:1;
            UCHAR SlaveIrq5:1;
            UCHAR SlaveIrq6:1;
            UCHAR SlaveIrq7:1;
        };
        struct
        {
            UCHAR SlaveId:3;
            UCHAR Reserved:5;
        };
    };
    UCHAR Bits;
} I8259_ICW3, *PI8259_ICW3;

typedef union _I8259_ICW4
{
    struct
    {
        UCHAR SystemMode:1;
        UCHAR EoiMode:1;
        UCHAR BufferedMode:2;
        UCHAR SpecialFullyNestedMode:1;
        UCHAR Reserved:3;
    };
    UCHAR Bits;
} I8259_ICW4, *PI8259_ICW4;

typedef union _I8259_OCW2
{
    struct
    {
        UCHAR IrqNumber:3;
        UCHAR Sbz:2;
        UCHAR EoiMode:3;
    };
    UCHAR Bits;
} I8259_OCW2, *PI8259_OCW2;

typedef union _I8259_OCW3
{
    struct
    {
        UCHAR ReadRequest:2;
        UCHAR PollCommand:1;
        UCHAR Sbo:1;
        UCHAR Sbz:1;
        UCHAR SpecialMaskMode:2;
        UCHAR Reserved:1;
    };
    UCHAR Bits;
} I8259_OCW3, *PI8259_OCW3;

typedef union _I8259_ISR
{
    struct
    {
        UCHAR Irq0:1;
        UCHAR Irq1:1;
        UCHAR Irq2:1;
        UCHAR Irq3:1;
        UCHAR Irq4:1;
        UCHAR Irq5:1;
        UCHAR Irq6:1;
        UCHAR Irq7:1;
    };
    UCHAR Bits;
} I8259_ISR, *PI8259_ISR;

typedef I8259_ISR I8259_IDR, *PI8259_IDR;

//
// See EISA System Architecture 2nd Edition (Tom Shanley, Don Anderson, John Swindle)
// P. 34, 35
//
// These ports are controlled by the i8259A Programmable Interrupt Controller (PIC)
//
#define EISA_ELCR_MASTER       0x4D0
#define EISA_ELCR_SLAVE        0x4D1

typedef union _EISA_ELCR
{
    struct
    {
        struct
        {
            UCHAR Irq0Level:1;
            UCHAR Irq1Level:1;
            UCHAR Irq2Level:1;
            UCHAR Irq3Level:1;
            UCHAR Irq4Level:1;
            UCHAR Irq5Level:1;
            UCHAR Irq6Level:1;
            UCHAR Irq7Level:1;
        } Master;
        struct
        {
            UCHAR Irq8Level:1;
            UCHAR Irq9Level:1;
            UCHAR Irq10Level:1;
            UCHAR Irq11Level:1;
            UCHAR Irq12Level:1;
            UCHAR Irq13Level:1;
            UCHAR Irq14Level:1;
            UCHAR Irq15Level:1;
        } Slave;
    };
    USHORT Bits;
} EISA_ELCR, *PEISA_ELCR;

typedef union _PIC_MASK
{
    struct
    {
        UCHAR Master;
        UCHAR Slave;
    };
    USHORT Both;
} PIC_MASK, *PPIC_MASK;
