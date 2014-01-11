/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            cmos.c
 * PURPOSE:         CMOS Real Time Clock emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "cmos.h"
#include "io.h"
#include "bios/bios.h"
#include "pic.h"

/* PRIVATE VARIABLES **********************************************************/

static HANDLE hCmosRam = INVALID_HANDLE_VALUE;
static CMOS_MEMORY CmosMemory;

static BOOLEAN NmiEnabled = TRUE;
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
            return READ_CMOS_DATA(CmosMemory, CurrentTime.wSecond);

        case CMOS_REG_ALARM_SEC:
            return READ_CMOS_DATA(CmosMemory, CmosMemory.AlarmSecond);

        case CMOS_REG_MINUTES:
            return READ_CMOS_DATA(CmosMemory, CurrentTime.wMinute);

        case CMOS_REG_ALARM_MIN:
            return READ_CMOS_DATA(CmosMemory, CmosMemory.AlarmMinute);

        case CMOS_REG_HOURS:
        {
            BOOLEAN Afternoon = FALSE;
            BYTE Value = CurrentTime.wHour;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            Value = READ_CMOS_DATA(CmosMemory, Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            return Value;
        }

        case CMOS_REG_ALARM_HRS:
        {
            BOOLEAN Afternoon = FALSE;
            BYTE Value = CmosMemory.AlarmHour;

            if (!(CmosMemory.StatusRegB & CMOS_STB_24HOUR) && (Value >= 12))
            {
                Value -= 12;
                Afternoon = TRUE;
            }

            Value = READ_CMOS_DATA(CmosMemory, Value);

            /* Convert to 12-hour */
            if (Afternoon) Value |= 0x80;

            return Value;
        }

        case CMOS_REG_DAY_OF_WEEK:
            /*
             * The CMOS value is 1-based but the
             * GetLocalTime API value is 0-based.
             * Correct it.
             */
            return READ_CMOS_DATA(CmosMemory, CurrentTime.wDayOfWeek + 1);

        case CMOS_REG_DAY:
            return READ_CMOS_DATA(CmosMemory, CurrentTime.wDay);

        case CMOS_REG_MONTH:
            return READ_CMOS_DATA(CmosMemory, CurrentTime.wMonth);

        case CMOS_REG_YEAR:
            return READ_CMOS_DATA(CmosMemory, CurrentTime.wYear % 100);

        case CMOS_REG_STATUS_C:
        {
            BYTE Value = CmosMemory.StatusRegC;

            /* Clear status register C */
            CmosMemory.StatusRegC = 0x00;

            /* Return the old value */
            return Value;
        }

        case CMOS_REG_BASE_MEMORY_LOW:
            return Bda->MemorySize & 0xFF;

        case CMOS_REG_BASE_MEMORY_HIGH:
            return Bda->MemorySize >> 8;

        case CMOS_REG_EXT_MEMORY_LOW:
            return ((MAX_ADDRESS - 0x100000) / 1024) & 0xFF;

        case CMOS_REG_EXT_MEMORY_HIGH:
            return ((MAX_ADDRESS - 0x100000) / 1024) >> 8;

        case CMOS_REG_STATUS_A:
        case CMOS_REG_STATUS_B:
        case CMOS_REG_STATUS_D:
        case CMOS_REG_DIAGNOSTICS:
        case CMOS_REG_SHUTDOWN_STATUS:
        default:
        {
            // ASSERT(SelectedRegister < CMOS_REG_MAX);
            return CmosMemory.Regs[SelectedRegister];
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

        default:
        {
            CmosMemory.Regs[SelectedRegister] = Value;
        }
    }

    if (ChangeTime) SetLocalTime(&CurrentTime);

    /* Return to Status Register D */
    SelectedRegister = CMOS_REG_STATUS_D;
}

BYTE WINAPI CmosReadPort(ULONG Port)
{
    return CmosReadData();
}

VOID WINAPI CmosWritePort(ULONG Port, BYTE Data)
{
    if (Port == CMOS_ADDRESS_PORT)
        CmosWriteAddress(Data);
    else if (Port == CMOS_DATA_PORT)
        CmosWriteData(Data);
}

DWORD RtcGetTicksPerSecond(VOID)
{
    BYTE RateSelect = CmosMemory.StatusRegB & 0x0F;

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
VOID RtcTimeUpdate(VOID)
{
    SYSTEMTIME CurrentTime;

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

VOID CmosInitialize(VOID)
{
    DWORD CmosSize = sizeof(CmosMemory);

    /* File must not be opened before */
    ASSERT(hCmosRam == INVALID_HANDLE_VALUE);

    /* Clear the CMOS memory */
    ZeroMemory(&CmosMemory, sizeof(CmosMemory));

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
        BOOL Success = FALSE;

        /* Attempt to fill the CMOS memory with the RAM file */
        SetLastError(0); // For debugging purposes
        Success = ReadFile(hCmosRam, &CmosMemory, CmosSize, &CmosSize, NULL);
        if (CmosSize != sizeof(CmosMemory))
        {
            /* Bad CMOS Ram file. Reinitialize the CMOS memory. */
            DPRINT1("Invalid CMOS file, read bytes %u, expected bytes %u\n", CmosSize, sizeof(CmosMemory));
            ZeroMemory(&CmosMemory, sizeof(CmosMemory));
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

    /* Register the I/O Ports */
    RegisterIoPort(CMOS_ADDRESS_PORT, NULL        , CmosWritePort);
    RegisterIoPort(CMOS_DATA_PORT   , CmosReadPort, CmosWritePort);
}

VOID CmosCleanup(VOID)
{
    DWORD CmosSize = sizeof(CmosMemory);

    if (hCmosRam == INVALID_HANDLE_VALUE) return;

    /* Flush the CMOS memory back to the RAM file and close it */
    SetFilePointer(hCmosRam, 0, NULL, FILE_BEGIN);
    WriteFile(hCmosRam, &CmosMemory, CmosSize, &CmosSize, NULL);

    CloseHandle(hCmosRam);
    hCmosRam = INVALID_HANDLE_VALUE;
}

/* EOF */
