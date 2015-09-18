/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/cmos.h
 * PURPOSE:         CMOS Real Time Clock emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _CMOS_H_
#define _CMOS_H_

/* DEFINES ********************************************************************/

#define RTC_IRQ_NUMBER      8
#define CMOS_ADDRESS_PORT   0x70
#define CMOS_DATA_PORT      0x71
#define CMOS_DISABLE_NMI    (1 << 7)
#define CMOS_BATTERY_OK     0x80

/* Status Register B flags */
#define CMOS_STB_DST            (1 << 0)
#define CMOS_STB_24HOUR         (1 << 1)
#define CMOS_STB_BINARY         (1 << 2)
#define CMOS_STB_SQUARE_WAVE    (1 << 3)
#define CMOS_STB_INT_ON_UPDATE  (1 << 4)
#define CMOS_STB_INT_ON_ALARM   (1 << 5)
#define CMOS_STB_INT_PERIODIC   (1 << 6)
#define CMOS_STB_UPDATE_CYCLE   (1 << 7)

/* Status Register C flags */
#define CMOS_STC_UF     (1 << 4)
#define CMOS_STC_AF     (1 << 5)
#define CMOS_STC_PF     (1 << 6)
#define CMOS_STC_IRQF   (1 << 7)

/* Default register values */
#define CMOS_DEFAULT_STA    0x26
#define CMOS_DEFAULT_STB    CMOS_STB_24HOUR

#define WRITE_CMOS_DATA(Cmos, Value)    \
    ((Cmos).StatusRegB & CMOS_STB_BINARY) ? (Value) : BCD_TO_BINARY(Value)

#define READ_CMOS_DATA(Cmos, Value)     \
    ((Cmos).StatusRegB & CMOS_STB_BINARY) ? (Value) : BINARY_TO_BCD(Value)

typedef enum _CMOS_REGISTERS
{
    CMOS_REG_SECONDS,
    CMOS_REG_ALARM_SEC,
    CMOS_REG_MINUTES,
    CMOS_REG_ALARM_MIN,
    CMOS_REG_HOURS,
    CMOS_REG_ALARM_HRS,
    CMOS_REG_DAY_OF_WEEK,
    CMOS_REG_DAY,
    CMOS_REG_MONTH,
    CMOS_REG_YEAR,
    CMOS_REG_STATUS_A,
    CMOS_REG_STATUS_B,
    CMOS_REG_STATUS_C,
    CMOS_REG_STATUS_D,
    CMOS_REG_DIAGNOSTICS,
    CMOS_REG_SHUTDOWN_STATUS,
    CMOS_REG_BASE_MEMORY_LOW        = 0x15,
    CMOS_REG_BASE_MEMORY_HIGH       = 0x16,
    CMOS_REG_EXT_MEMORY_LOW         = 0x17,
    CMOS_REG_EXT_MEMORY_HIGH        = 0x18,
    CMOS_REG_ACTUAL_EXT_MEMORY_LOW  = 0x30,
    CMOS_REG_ACTUAL_EXT_MEMORY_HIGH = 0x31,
    CMOS_REG_CENTURY                = 0x32,
    CMOS_REG_MAX                    = 0x40
} CMOS_REGISTERS, *PCMOS_REGISTERS;

/*
 * CMOS Memory Map
 *
 * See the following documentation for more information:
 * http://www.intel-assembler.it/portale/5/cmos-memory-map-123/cmos-memory-map-123.asp
 * http://wiki.osdev.org/CMOS
 * http://www.walshcomptech.com/ohlandl/config/cmos_registers.html
 * http://www.fysnet.net/cmosinfo.htm
 * http://www.bioscentral.com/misc/cmosmap.htm
 */
#pragma pack(push, 1)
typedef struct
{
    BYTE Second;        // 0x00
    BYTE AlarmSecond;   // 0x01
    BYTE Minute;        // 0x02
    BYTE AlarmMinute;   // 0x03
    BYTE Hour;          // 0x04
    BYTE AlarmHour;     // 0x05
    BYTE DayOfWeek;     // 0x06
    BYTE Day;           // 0x07
    BYTE Month;         // 0x08
    BYTE Year;          // 0x09

    BYTE StatusRegA;    // 0x0a
    BYTE StatusRegB;    // 0x0b
} CMOS_CLOCK, *PCMOS_CLOCK;

typedef struct
{
    union
    {
        struct
        {
            CMOS_CLOCK;             // 0x00 - 0x0b
            BYTE StatusRegC;        // 0x0c
            BYTE StatusRegD;        // 0x0d
            BYTE Diagnostics;       // 0x0e
            BYTE ShutdownStatus;    // 0x0f
            BYTE Padding[0x22];     // 0x10
            BYTE Century;           // 0x32
        };
        BYTE Regs1[0x10];           // 0x00 - 0x0f
        BYTE Regs [0x40];           // 0x00 - 0x3f
    };

    /*
     * Extended information 0x40 - 0x7f
     */
} CMOS_MEMORY, *PCMOS_MEMORY;
#pragma pack(pop)

C_ASSERT(sizeof(CMOS_MEMORY) == 0x40);

/* FUNCTIONS ******************************************************************/

BOOLEAN IsNmiEnabled(VOID);
DWORD RtcGetTicksPerSecond(VOID);

VOID CmosInitialize(VOID);
VOID CmosCleanup(VOID);

#endif // _CMOS_H_

/* EOF */
