/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            cmos.c
 * PURPOSE:         CMOS Real Time Clock emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "cmos.h"

#include "io.h"
#include "pic.h"
#include "clock.h"

/* PRIVATE VARIABLES **********************************************************/

static HANDLE hCmosRam = INVALID_HANDLE_VALUE;
static CMOS_MEMORY CmosMemory;

static BOOLEAN NmiEnabled = TRUE;
static CMOS_REGISTERS SelectedRegister = CMOS_REG_STATUS_D;

static PHARDWARE_TIMER ClockTimer;
static PHARDWARE_TIMER PeriodicTimer;

/* PRIVATE FUNCTIONS **********************************************************/

static VOID RtcUpdatePeriodicTimer(VOID)
{
    BYTE RateSelect = CmosMemory.StatusRegA & 0x0F;

    if (RateSelect == 0)
    {
        /* No periodic interrupt */
        DisableHardwareTimer(PeriodicTimer);
        return;
    }

    /* 1 and 2 act like 8 and 9 */
    if (RateSelect <= 2) RateSelect += 7;

    SetHardwareTimerDelay(PeriodicTimer, HZ_TO_NS(1 << (16 - RateSelect)));
    EnableHardwareTimer(PeriodicTimer);
}

static VOID FASTCALL RtcPeriodicTick(ULONGLONG ElapsedTime)
{
    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Set PF */
    CmosMemory.StatusRegC |= CMOS_STC_PF;

    /* Check if there should be an interrupt on a periodic timer tick */
    if (CmosMemory.StatusRegB & CMOS_STB_INT_PERIODIC)
    {
        CmosMemory.StatusRegC |= CMOS_STC_IRQF;

        /* Interrupt! */
        PicInterruptRequest(RTC_IRQ_NUMBER);
    }
}

/* Should be called every second */
static VOID FASTCALL RtcTimeUpdate(ULONGLONG ElapsedTime)
{
    SYSTEMTIME CurrentTime;

    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    /* Set UF */
    CmosMemory.StatusRegC |= CMOS_STC_UF;

    /* Check if the time matches the alarm time */
    if ((CurrentTime.wHour   == CmosMemory.AlarmHour  ) &&
        (CurrentTime.wMinute == CmosMemory.AlarmMinute) &&
        (CurrentTime.wSecond == CmosMemory.AlarmSecond))
    {
        /* Set the alarm flag */
        CmosMemory.StatusRegC |= CMOS_STC_AF;

        /* Set IRQF if there should be an interrupt */
        if (CmosMemory.StatusRegB & CMOS_STB_INT_ON_ALARM) CmosMemory.StatusRegC |= CMOS_STC_IRQF;
    }

    /* Check if there should be an interrupt on update */
    if (CmosMemory.StatusRegB & CMOS_STB_INT_ON_UPDATE) CmosMemory.StatusRegC |= CMOS_STC_IRQF;

    if (CmosMemory.StatusRegC & CMOS_STC_IRQF)
    {
        /* Interrupt! */
        PicInterruptRequest(RTC_IRQ_NUMBER);
    }
}

static VOID CmosWriteAddress(BYTE Value)
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

static BYTE CmosReadData(VOID)
{
    BYTE Value;
    SYSTEMTIME CurrentTime;

    /* Get the current time */
    GetLocalTime(&CurrentTime);

    switch (SelectedRegister)
    {
        case CMOS_REG_SECONDS:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wSecond);
            break;
        }

        case CMOS_REG_ALARM_SEC:
        {
            Value = READ_CMOS_DATA(CmosMemory, CmosMemory.AlarmSecond);
            break;
        }

        case CMOS_REG_MINUTES:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wMinute);
            break;
        }

        case CMOS_REG_ALARM_MIN:
        {
            Value = READ_CMOS_DATA(CmosMemory, CmosMemory.AlarmMinute);
            break;
        }

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;
            Value = CurrentTime.wHour;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            Value = READ_CMOS_DATA(CmosMemory, Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            break;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;
            Value = CmosMemory.AlarmHour;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            Value = READ_CMOS_DATA(CmosMemory, Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            break;
        }

        case CMOS_REG_DAY_OF_WEEK:
        {
            /*
             * The CMOS value is 1-based but the
             * GetLocalTime API value is 0-based.
             * Correct it.
             */
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wDayOfWeek + 1);
            break;
        }

        case CMOS_REG_DAY:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wDay);
            break;
        }

        case CMOS_REG_MONTH:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wMonth);
            break;
        }

        case CMOS_REG_YEAR:
        {
            Value = READ_CMOS_DATA(CmosMemory, CurrentTime.wYear % 100);
            break;
        }

        case CMOS_REG_STATUS_C:
        {
            /* Return the old value */
            Value = CmosMemory.StatusRegC;

            /* Clear status register C */
            CmosMemory.StatusRegC = 0x00;

            break;
        }

        case CMOS_REG_STATUS_A:
        case CMOS_REG_STATUS_B:
        case CMOS_REG_STATUS_D:
        case CMOS_REG_DIAGNOSTICS:
        case CMOS_REG_SHUTDOWN_STATUS:
        default:
        {
            // ASSERT(SelectedRegister < CMOS_REG_MAX);
            Value = CmosMemory.Regs[SelectedRegister];
        }
    }

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;

    return Value;
}

static VOID CmosWriteData(BYTE Value)
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
            CurrentTime.wSecond = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_ALARM_SEC:
        {
            CmosMemory.AlarmSecond = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_MINUTES:
        {
            ChangeTime = TRUE;
            CurrentTime.wMinute = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_ALARM_MIN:
        {
            CmosMemory.AlarmMinute = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;

            ChangeTime = TRUE;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value & 0x80))
            {
                Value &= ~0x80;
                Afternoon = TRUE;
            }

            CurrentTime.wHour = WRITE_CMOS_DATA(CmosMemory, Value);

            /* Convert to 24-hour format */
            if (Afternoon) CurrentTime.wHour += 12;

            break;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value & 0x80))
            {
                Value &= ~0x80;
                Afternoon = TRUE;
            }

            CmosMemory.AlarmHour = WRITE_CMOS_DATA(CmosMemory, Value);

            /* Convert to 24-hour format */
            if (Afternoon) CmosMemory.AlarmHour += 12;

            break;
        }

        case CMOS_REG_DAY_OF_WEEK:
        {
            ChangeTime = TRUE;
            /*
             * The CMOS value is 1-based but the
             * SetLocalTime API value is 0-based.
             * Correct it.
             */
            Value -= 1;
            CurrentTime.wDayOfWeek = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_DAY:
        {
            ChangeTime = TRUE;
            CurrentTime.wDay = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_MONTH:
        {
            ChangeTime = TRUE;
            CurrentTime.wMonth = WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_YEAR:
        {
            ChangeTime = TRUE;

            /* Clear everything except the century */
            CurrentTime.wYear = (CurrentTime.wYear / 100) * 100;

            CurrentTime.wYear += WRITE_CMOS_DATA(CmosMemory, Value);
            break;
        }

        case CMOS_REG_STATUS_A:
        {
            CmosMemory.StatusRegA = Value & 0x7F; // Bit 7 is read-only
            RtcUpdatePeriodicTimer();
            break;
        }

        case CMOS_REG_STATUS_B:
        {
            CmosMemory.StatusRegB = Value;
            break;
        }

        case CMOS_REG_STATUS_C:
        case CMOS_REG_STATUS_D:
            // Status registers C and D are read-only
            break;

        /* Is the following correct? */
        case CMOS_REG_EXT_MEMORY_LOW:
        case CMOS_REG_ACTUAL_EXT_MEMORY_LOW:
        {
            /* Sync EMS and UMS */
            CmosMemory.Regs[CMOS_REG_EXT_MEMORY_LOW]        =
            CmosMemory.Regs[CMOS_REG_ACTUAL_EXT_MEMORY_LOW] = Value;
            break;
        }

        /* Is the following correct? */
        case CMOS_REG_EXT_MEMORY_HIGH:
        case CMOS_REG_ACTUAL_EXT_MEMORY_HIGH:
        {
            /* Sync EMS and UMS */
            CmosMemory.Regs[CMOS_REG_EXT_MEMORY_HIGH]        =
            CmosMemory.Regs[CMOS_REG_ACTUAL_EXT_MEMORY_HIGH] = Value;
            break;
        }

        default:
        {
            CmosMemory.Regs[SelectedRegister] = Value;
        }
    }

    if (ChangeTime) SetLocalTime(&CurrentTime);

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;
}

static BYTE WINAPI CmosReadPort(USHORT Port)
{
    ASSERT(Port == CMOS_DATA_PORT);
    return CmosReadData();
}

static VOID WINAPI CmosWritePort(USHORT Port, BYTE Data)
{
    if (Port == CMOS_ADDRESS_PORT)
        CmosWriteAddress(Data);
    else if (Port == CMOS_DATA_PORT)
        CmosWriteData(Data);
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN IsNmiEnabled(VOID)
{
    return NmiEnabled;
}

VOID CmosInitialize(VOID)
{
    DWORD CmosSize = sizeof(CmosMemory);

    /* File must not be opened before */
    ASSERT(hCmosRam == INVALID_HANDLE_VALUE);

    /* Clear the CMOS memory */
    RtlZeroMemory(&CmosMemory, sizeof(CmosMemory));

    /* Always open (and if needed, create) a RAM file with shared access */
    SetLastError(0); // For debugging purposes
    hCmosRam = CreateFileW(L"cmos.ram",
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    DPRINT1("CMOS opening %s ; GetLastError() = %u\n", hCmosRam != INVALID_HANDLE_VALUE ? "succeeded" : "failed", GetLastError());

    if (hCmosRam != INVALID_HANDLE_VALUE)
    {
        BOOL Success;

        /* Attempt to fill the CMOS memory with the RAM file */
        SetLastError(0); // For debugging purposes
        Success = ReadFile(hCmosRam, &CmosMemory, CmosSize, &CmosSize, NULL);
        if (CmosSize != sizeof(CmosMemory))
        {
            /* Bad CMOS Ram file. Reinitialize the CMOS memory. */
            DPRINT1("Invalid CMOS file, read bytes %u, expected bytes %u\n", CmosSize, sizeof(CmosMemory));
            RtlZeroMemory(&CmosMemory, sizeof(CmosMemory));
        }
        DPRINT1("CMOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());
        SetFilePointer(hCmosRam, 0, NULL, FILE_BEGIN);
    }

    /* Overwrite some registers with default values */
    CmosMemory.StatusRegA     = CMOS_DEFAULT_STA;
    CmosMemory.StatusRegB     = CMOS_DEFAULT_STB;
    CmosMemory.StatusRegC     = 0x00;
    CmosMemory.StatusRegD     = CMOS_BATTERY_OK; // Our CMOS battery works perfectly forever.
    CmosMemory.Diagnostics    = 0x00;            // Diagnostics must not find any errors.
    CmosMemory.ShutdownStatus = 0x00;

    /* Memory settings */

    /*
     * Conventional memory size is 640 kB,
     * see: http://webpages.charter.net/danrollins/techhelp/0184.HTM
     * and see Ralf Brown: http://www.ctyme.com/intr/rb-0598.htm
     * for more information.
     */
    CmosMemory.Regs[CMOS_REG_BASE_MEMORY_LOW ] = LOBYTE(0x0280);
    CmosMemory.Regs[CMOS_REG_BASE_MEMORY_HIGH] = HIBYTE(0x0280);

    CmosMemory.Regs[CMOS_REG_EXT_MEMORY_LOW]         =
    CmosMemory.Regs[CMOS_REG_ACTUAL_EXT_MEMORY_LOW]  = LOBYTE((MAX_ADDRESS - 0x100000) / 1024);

    CmosMemory.Regs[CMOS_REG_EXT_MEMORY_HIGH]        =
    CmosMemory.Regs[CMOS_REG_ACTUAL_EXT_MEMORY_HIGH] = HIBYTE((MAX_ADDRESS - 0x100000) / 1024);

    /* Register the I/O Ports */
    RegisterIoPort(CMOS_ADDRESS_PORT, NULL        , CmosWritePort);
    RegisterIoPort(CMOS_DATA_PORT   , CmosReadPort, CmosWritePort);

    ClockTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED,
                                     HZ_TO_NS(1),
                                     RtcTimeUpdate);
    PeriodicTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED | HARDWARE_TIMER_PRECISE,
                                        HZ_TO_NS(1000),
                                        RtcPeriodicTick);
}

VOID CmosCleanup(VOID)
{
    DWORD CmosSize = sizeof(CmosMemory);

    if (hCmosRam == INVALID_HANDLE_VALUE) return;

    DestroyHardwareTimer(PeriodicTimer);
    DestroyHardwareTimer(ClockTimer);

    /* Flush the CMOS memory back to the RAM file and close it */
    SetFilePointer(hCmosRam, 0, NULL, FILE_BEGIN);
    WriteFile(hCmosRam, &CmosMemory, CmosSize, &CmosSize, NULL);

    CloseHandle(hCmosRam);
    hCmosRam = INVALID_HANDLE_VALUE;
}

/* EOF */
