/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            cmos.h
 * PURPOSE:         Real Time Clock emulation (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _CMOS_H_
#define _CMOS_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define RTC_IRQ_NUMBER      8
#define CMOS_ADDRESS_PORT   0x70
#define CMOS_DATA_PORT      0x71
#define CMOS_DISABLE_NMI    (1 << 7)
#define CMOS_BATTERY_OK     0x80

/* Status Register B flags */
#define CMOS_STB_24HOUR         (1 << 1)
#define CMOS_STB_BINARY         (1 << 2)
#define CMOS_STB_SQUARE_WAVE    (1 << 3)
#define CMOS_STB_INT_ON_UPDATE  (1 << 4)
#define CMOS_STB_INT_ON_ALARM   (1 << 5)
#define CMOS_STB_INT_PERIODIC   (1 << 6)

/* Status Register C flags */
#define CMOS_STC_UF     CMOS_STB_INT_ON_UPDATE
#define CMOS_STC_AF     CMOS_STB_INT_ON_ALARM
#define CMOS_STC_PF     CMOS_STB_INT_PERIODIC
#define CMOS_STC_IRQF   (1 << 7)

/* Default register values */
#define CMOS_DEFAULT_STA 0x26
#define CMOS_DEFAULT_STB CMOS_STB_24HOUR

/* BCD-Binary conversion */
#define BINARY_TO_BCD(x) (((x / 10) << 4) | (x % 10))
#define BCD_TO_BINARY(x) (((x >> 4) * 10) + (x & 0x0F))

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
    CMOS_REG_MAX
} CMOS_REGISTERS, *PCMOS_REGISTERS;

BOOLEAN IsNmiEnabled(VOID);
VOID CmosWriteAddress(BYTE Value);
BYTE CmosReadData(VOID);
VOID CmosWriteData(BYTE Value);
DWORD RtcGetTicksPerSecond(VOID);
VOID RtcPeriodicTick(VOID);
VOID RtcTimeUpdate(VOID);
    
#endif // _CMOS_H_

/* EOF */
