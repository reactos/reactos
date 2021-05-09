/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     RTC and NVRAM access routines
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/*
 * The PC-98 hardware maps data from the NVRAM directly into the text video
 * memory address space. Every fourth byte is a "writable data".
 *
 * |0x2FE2|0x2FE3|0x2FE4|0x2FE5|0x2FE6|0x2FE7| .... |0x2FFD|0x2FFE|
 * |  D   |      |      |      |   D  |      | .... |      |   D  |
 *
 * Most of these bits of the NVRAM are already used. There are some reserved
 * bits in the 0x3FE6 and 0x3FFE that we can use.
 */
#define NVRAM_START         0x3FE2
#define NVRAM_SIZE          0x1C
#define NVRAM_UNUSED_REG    0x14
#define NVRAM_UNUSED_BIT    0x80

static ULONG_PTR MappedNvram;

/* PRIVATE FUNCTIONS *********************************************************/

/* Avoid double calls */
#undef BCD_INT
static UCHAR
BCD_INT(
    _In_ UCHAR Bcd)
{
    return ((Bcd & 0xF0) >> 4) * 10 + (Bcd & 0x0F);
}

static UCHAR
NTAPI
HalpReadNvram(
    _In_ UCHAR Register)
{
    return READ_REGISTER_UCHAR((PUCHAR)(MappedNvram + Register));
}

_Requires_lock_held_(HalpSystemHardwareLock)
static VOID
NTAPI
HalpWriteNvram(
    _In_ UCHAR Register,
    _In_ UCHAR Value)
{
    __outbyte(GDC1_IO_o_MODE_FLIPFLOP1, GDC1_NVRAM_UNPROTECT);
    WRITE_REGISTER_UCHAR((PUCHAR)(MappedNvram + Register), Value);
    __outbyte(GDC1_IO_o_MODE_FLIPFLOP1, GDC1_NVRAM_PROTECT);
}

_Requires_lock_held_(HalpSystemHardwareLock)
static UCHAR
NTAPI
HalpRtcReadByte(VOID)
{
    UCHAR i;
    UCHAR Byte = 0;

    /* Read byte from single wire bus */
    for (i = 0; i < 8; i++)
    {
        Byte |= (__inbyte(PPI_IO_i_PORT_B) & 1) << i;

        __outbyte(RTC_IO_o_DATA, RTC_CLOCK | RTC_CMD_SERIAL_TRANSFER_MODE);
        KeStallExecutionProcessor(1);

        __outbyte(RTC_IO_o_DATA, RTC_CMD_SERIAL_TRANSFER_MODE);
        KeStallExecutionProcessor(1);
    }

    return Byte;
}

_Requires_lock_held_(HalpSystemHardwareLock)
static VOID
NTAPI
HalpRtcWriteBit(
    _In_ UCHAR Bit)
{
    Bit = (Bit & 1) << 5;

    __outbyte(RTC_IO_o_DATA, Bit | RTC_CMD_SERIAL_TRANSFER_MODE);
    KeStallExecutionProcessor(1);

    __outbyte(RTC_IO_o_DATA, Bit | RTC_CLOCK | RTC_CMD_SERIAL_TRANSFER_MODE);
    KeStallExecutionProcessor(1);
}

_Requires_lock_held_(HalpSystemHardwareLock)
static VOID
NTAPI
HalpRtcWriteCommand(
    _In_ UCHAR Command)
{
    UCHAR i;

    for (i = 0; i < 4; i++)
        HalpRtcWriteBit(Command >> i);

    __outbyte(RTC_IO_o_DATA, RTC_STROBE | RTC_CMD_SERIAL_TRANSFER_MODE);
    KeStallExecutionProcessor(1);

    __outbyte(RTC_IO_o_DATA, RTC_CMD_SERIAL_TRANSFER_MODE);
    KeStallExecutionProcessor(1);
}

UCHAR
NTAPI
HalpReadCmos(
    _In_ UCHAR Reg)
{
    /* Not supported by hardware */
    return 0;
}

VOID
NTAPI
HalpWriteCmos(
    _In_ UCHAR Reg,
    _In_ UCHAR Value)
{
    /* Not supported by hardware */
    NOTHING;
}

ULONG
NTAPI
HalpGetCmosData(
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    /* Not supported by hardware */
    return 0;
}

ULONG
NTAPI
HalpSetCmosData(
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    /* Not supported by hardware */
    return 0;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitializeCmos(VOID)
{
    PHYSICAL_ADDRESS PhysicalAddress;

    /* TODO: Detect TVRAM address */
    if (TRUE)
        PhysicalAddress.QuadPart = VRAM_NORMAL_TEXT + NVRAM_START;
    else
        PhysicalAddress.QuadPart = VRAM_HI_RESO_TEXT + NVRAM_START;
    MappedNvram = (ULONG_PTR)HalpMapPhysicalMemory64(PhysicalAddress, BYTES_TO_PAGES(NVRAM_SIZE));
}

/* PUBLIC FUNCTIONS **********************************************************/

ARC_STATUS
NTAPI
HalGetEnvironmentVariable(
    _In_ PCH Name,
    _In_ USHORT ValueLength,
    _Out_writes_z_(ValueLength) PCH Value)
{
    UCHAR Val;

    /* Only variable supported on x86 */
    if (_stricmp(Name, "LastKnownGood"))
        return ENOENT;

    if (!MappedNvram)
        return ENOENT;

    HalpAcquireCmosSpinLock();

    Val = HalpReadNvram(NVRAM_UNUSED_REG) & NVRAM_UNUSED_BIT;

    HalpReleaseCmosSpinLock();

    /* Check the flag */
    if (Val)
        strncpy(Value, "FALSE", ValueLength);
    else
        strncpy(Value, "TRUE", ValueLength);

    return ESUCCESS;
}

ARC_STATUS
NTAPI
HalSetEnvironmentVariable(
    _In_ PCH Name,
    _In_ PCH Value)
{
    UCHAR Val;

    /* Only variable supported on x86 */
    if (_stricmp(Name, "LastKnownGood"))
        return ENOMEM;

    if (!MappedNvram)
        return ENOMEM;

    /* Check if this is true or false */
    if (!_stricmp(Value, "TRUE"))
    {
        HalpAcquireCmosSpinLock();

        Val = HalpReadNvram(NVRAM_UNUSED_REG) | NVRAM_UNUSED_BIT;
    }
    else if (!_stricmp(Value, "FALSE"))
    {
        HalpAcquireCmosSpinLock();

        Val = HalpReadNvram(NVRAM_UNUSED_REG) & ~NVRAM_UNUSED_BIT;
    }
    else
    {
        /* Fail */
        return ENOMEM;
    }

    HalpWriteNvram(NVRAM_UNUSED_REG, Val);

    HalpReleaseCmosSpinLock();

    return ESUCCESS;
}

BOOLEAN
NTAPI
HalQueryRealTimeClock(
    _Out_ PTIME_FIELDS Time)
{
    UCHAR Temp;

    HalpAcquireCmosSpinLock();

    HalpRtcWriteCommand(RTC_CMD_TIME_READ);
    HalpRtcWriteCommand(RTC_CMD_REGISTER_SHIFT);
    KeStallExecutionProcessor(19);

    /* Set the time data */
    Time->Second = BCD_INT(HalpRtcReadByte());
    Time->Minute = BCD_INT(HalpRtcReadByte());
    Time->Hour = BCD_INT(HalpRtcReadByte());
    Time->Day = BCD_INT(HalpRtcReadByte());
    Temp = HalpRtcReadByte();
    Time->Weekday = Temp & 0x0F;
    Time->Month = Temp >> 4;
    Time->Year = BCD_INT(HalpRtcReadByte());
    Time->Milliseconds = 0;

    Time->Year += (Time->Year >= 80) ? 1900 : 2000;

    HalpRtcWriteCommand(RTC_CMD_REGISTER_HOLD);

    HalpReleaseCmosSpinLock();

    return TRUE;
}

BOOLEAN
NTAPI
HalSetRealTimeClock(
    _In_ PTIME_FIELDS Time)
{
    UCHAR i, j;
    UCHAR SysTime[6];

    HalpAcquireCmosSpinLock();

    HalpRtcWriteCommand(RTC_CMD_REGISTER_SHIFT);

    SysTime[0] = INT_BCD(Time->Second);
    SysTime[1] = INT_BCD(Time->Minute);
    SysTime[2] = INT_BCD(Time->Hour);
    SysTime[3] = INT_BCD(Time->Day);
    SysTime[4] = (Time->Month << 4) | (Time->Weekday & 0x0F);
    SysTime[5] = INT_BCD(Time->Year % 100);

    /* Write time fields to RTC */
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 8; j++)
            HalpRtcWriteBit(SysTime[i] >> j);
    }

    HalpRtcWriteCommand(RTC_CMD_TIME_SET_COUNTER_HOLD);
    HalpRtcWriteCommand(RTC_CMD_REGISTER_HOLD);

    HalpReleaseCmosSpinLock();

    return TRUE;
}
