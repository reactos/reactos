/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            cmos.c
 * PURPOSE:         CMOS Real Time Clock emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "cmos.h"
#include "pic.h"

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN NmiEnabled = TRUE;
static BYTE StatusRegA = CMOS_DEFAULT_STA;
static BYTE StatusRegB = CMOS_DEFAULT_STB;
static BYTE StatusRegC = 0;
static BYTE AlarmHour, AlarmMinute, AlarmSecond;
static CMOS_REGISTERS SelectedRegister = CMOS_REG_STATUS_D;

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN IsNmiEnabled(VOID)
{
    return NmiEnabled;
}

VOID CmosWriteAddress(BYTE Value)
{
    /* Update the NMI enabled flag */
    NmiEnabled = !(Value & CMOS_DISABLE_NMI);
    
    /* Get the register number */
    Value &= ~CMOS_DISABLE_NMI;

    if (Value < CMOS_REG_MAX)
    {
        /* Select the new register */
        SelectedRegister = Value;
    }
    else
    {
        /* Default to Status Register D */
        SelectedRegister = CMOS_REG_STATUS_D;
    }
}

BYTE CmosReadData(VOID)
{
    SYSTEMTIME CurrentTime;

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    switch (SelectedRegister)
    {
        case CMOS_REG_SECONDS:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? CurrentTime.wSecond
                   : BINARY_TO_BCD(CurrentTime.wSecond);
        }

        case CMOS_REG_ALARM_SEC:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? AlarmSecond
                   : BINARY_TO_BCD(AlarmSecond);
        }

        case CMOS_REG_MINUTES:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? CurrentTime.wMinute
                   : BINARY_TO_BCD(CurrentTime.wMinute);
        }

        case CMOS_REG_ALARM_MIN:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? AlarmMinute
                   : BINARY_TO_BCD(AlarmMinute);
        }

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;
            BYTE Value = CurrentTime.wHour;

            if (!(StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            if (!(StatusRegB & CMOS_STB_BINARY)) Value = BINARY_TO_BCD(Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            return Value;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;
            BYTE Value = AlarmHour;

            if (!(StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            if (!(StatusRegB & CMOS_STB_BINARY)) Value = BINARY_TO_BCD(Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            return Value;
        }

        case CMOS_REG_DAY_OF_WEEK:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? CurrentTime.wDayOfWeek
                   : BINARY_TO_BCD(CurrentTime.wDayOfWeek);
        }

        case CMOS_REG_DAY:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? CurrentTime.wDay
                   :BINARY_TO_BCD(CurrentTime.wDay);
        }

        case CMOS_REG_MONTH:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? CurrentTime.wMonth
                   : BINARY_TO_BCD(CurrentTime.wMonth);
        }

        case CMOS_REG_YEAR:
        {
            return (StatusRegB & CMOS_STB_BINARY)
                   ? (CurrentTime.wYear % 100)
                   : BINARY_TO_BCD(CurrentTime.wYear % 100);
        }

        case CMOS_REG_STATUS_A:
        {
            return StatusRegA;
        }

        case CMOS_REG_STATUS_B:
        {
            return StatusRegB;
        }

        case CMOS_REG_STATUS_C:
        {
            BYTE Value = StatusRegC;

            /* Clear status register C */
            StatusRegC = 0;

            /* Return the old value */
            return Value;
        }

        case CMOS_REG_STATUS_D:
        {
            /* Our CMOS battery works perfectly forever */
            return CMOS_BATTERY_OK;
        }

        case CMOS_REG_DIAGNOSTICS:
        {
            /* Diagnostics found no errors */
            return 0;
        }

        default:
        {
            /* Read ignored */
            return 0;
        }
    }

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;
}

VOID CmosWriteData(BYTE Value)
{
    BOOLEAN ChangeTime = FALSE;
    SYSTEMTIME CurrentTime;

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    switch (SelectedRegister)
    {
        case CMOS_REG_SECONDS:
        {
            ChangeTime = TRUE;
            CurrentTime.wSecond = (StatusRegB & CMOS_STB_BINARY)
                                  ? Value
                                  : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_ALARM_SEC:
        {
            AlarmSecond = (StatusRegB & CMOS_STB_BINARY)
                          ? Value
                          : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_MINUTES:
        {
            ChangeTime = TRUE;
            CurrentTime.wMinute = (StatusRegB & CMOS_STB_BINARY)
                                  ? Value
                                  : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_ALARM_MIN:
        {
            AlarmMinute = (StatusRegB & CMOS_STB_BINARY)
                          ? Value
                          : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;

            ChangeTime = TRUE;

            if (!(StatusRegB & CMOS_STB_24HOUR) && (Value & 0x80))
            {
                Value &= ~0x80;
                Afternoon = TRUE;
            }

            CurrentTime.wHour = (StatusRegB & CMOS_STB_BINARY)
                                ? Value
                                : BCD_TO_BINARY(Value);

            /* Convert to 24-hour format */
            if (Afternoon) CurrentTime.wHour += 12;

            break;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;

            if (!(StatusRegB & CMOS_STB_24HOUR) && (Value & 0x80))
            {
                Value &= ~0x80;
                Afternoon = TRUE;
            }

            AlarmHour = (StatusRegB & CMOS_STB_BINARY)
                        ? Value
                        : BCD_TO_BINARY(Value);

            /* Convert to 24-hour format */
            if (Afternoon) AlarmHour += 12;

            break;
        }

        case CMOS_REG_DAY_OF_WEEK:
        {
            ChangeTime = TRUE;
            CurrentTime.wDayOfWeek = (StatusRegB & CMOS_STB_BINARY)
                                     ? Value
                                     : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_DAY:
        {
            ChangeTime = TRUE;
            CurrentTime.wDay = (StatusRegB & CMOS_STB_BINARY)
                               ? Value
                               : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_MONTH:
        {
            ChangeTime = TRUE;
            CurrentTime.wMonth = (StatusRegB & CMOS_STB_BINARY)
                                 ? Value
                                 : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_YEAR:
        {
            ChangeTime = TRUE;

            /* Clear everything except the century */
            CurrentTime.wYear = (CurrentTime.wYear / 100) * 100;

            CurrentTime.wYear += (StatusRegB & CMOS_STB_BINARY)
                                 ? Value
                                 : BCD_TO_BINARY(Value);

            break;
        }

        case CMOS_REG_STATUS_A:
        {
            StatusRegA = Value;
            break;
        }

        case CMOS_REG_STATUS_B:
        {
            StatusRegB = Value;
            break;
        }

        default:
        {
            /* Write ignored */
        }
    }

    if (ChangeTime) SetLocalTime(&CurrentTime);

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;
}

DWORD RtcGetTicksPerSecond(VOID)
{
    BYTE RateSelect = StatusRegB & 0x0F;

    if (RateSelect == 0)
    {
        /* No periodic interrupt */
        return 0;
    }

    /* 1 and 2 act like 8 and 9 */
    if (RateSelect <= 2) RateSelect += 7;

    return 1 << (16 - RateSelect);
}

VOID RtcPeriodicTick(VOID)
{
    /* Set PF */
    StatusRegC |= CMOS_STC_PF;

    /* Check if there should be an interrupt on a periodic timer tick */
    if (StatusRegB & CMOS_STB_INT_PERIODIC)
    {
        StatusRegC |= CMOS_STC_IRQF;

        /* Interrupt! */
        PicInterruptRequest(RTC_IRQ_NUMBER);
    }
}

/* Should be called every second */
VOID RtcTimeUpdate(VOID)
{
    SYSTEMTIME CurrentTime;

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    /* Set UF */
    StatusRegC |= CMOS_STC_UF;

    /* Check if the time matches the alarm time */
    if ((CurrentTime.wHour == AlarmHour)
        && (CurrentTime.wMinute == AlarmMinute)
        && (CurrentTime.wSecond == AlarmSecond))
    {
        /* Set the alarm flag */
        StatusRegC |= CMOS_STC_AF;

        /* Set IRQF if there should be an interrupt */
        if (StatusRegB & CMOS_STB_INT_ON_ALARM) StatusRegC |= CMOS_STC_IRQF;
    }

    /* Check if there should be an interrupt on update */
    if (StatusRegB & CMOS_STB_INT_ON_UPDATE) StatusRegC |= CMOS_STC_IRQF;

    if (StatusRegC & CMOS_STC_IRQF)
    {
        /* Interrupt! */
        PicInterruptRequest(RTC_IRQ_NUMBER);
    }
}
