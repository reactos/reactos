/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PL011 serial port for the kernel debugger on ARM64
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <cportlib/uartinfo.h>
#define NDEBUG
#include <debug.h>

/* PL011 registers */
#define UART_DR             0x00
#define UART_FR             0x18
#define UART_CR             0x30

#define UART_FR_RXFE        0x10
#define UART_FR_TXFF        0x20

#define UART_CR_UARTEN      0x001
#define UART_CR_TXE         0x100
#define UART_CR_RXE         0x200

/* FUNCTIONS *****************************************************************/

/**
 * @brief Maps the UART at the top of the HAL range.
 *
 * The loader only maps the port in the user half, which goes away with the
 * first address space switch. The HAL hands out its mappings from the bottom
 * of the same range.
 */
static
PUCHAR
KdpMapUart(
    _In_ ULONG_PTR PhysicalAddress)
{
    PMMPTE PointerPte;
    MMPTE TempPte;

    for (PointerPte = MiAddressToPte(MM_HAL_VA_END);
         PointerPte >= MiAddressToPte(MM_HAL_VA_START);
         PointerPte--)
    {
        if (PointerPte->u.Long == 0)
            break;
    }

    if (PointerPte < MiAddressToPte(MM_HAL_VA_START))
        return NULL;

    TempPte.u.Long = 0;
    TempPte.u.Hard.Valid = 1;
    TempPte.u.Hard.NotLargePage = 1;
    TempPte.u.Hard.Accessed = 1;
    TempPte.u.Hard.CacheType = MI_CACHE_TYPE_DEVICE;
    TempPte.u.Hard.PrivilegedNoExecute = 1;
    TempPte.u.Hard.UserNoExecute = 1;
    TempPte.u.Hard.Writable = 1;
    TempPte.u.Hard.PageFrameNumber = PhysicalAddress >> PAGE_SHIFT;
    *PointerPte = TempPte;

    __dsb(_ARM64_BARRIER_ISHST);
    __isb(_ARM64_BARRIER_SY);

    return (PUCHAR)MiPteToAddress(PointerPte) + BYTE_OFFSET(PhysicalAddress);
}

BOOLEAN
NTAPI
KdPortInitializeEx(
    _In_ PCPPORT PortInformation,
    _In_ ULONG ComPortNumber)
{
    ULONG_PTR PhysicalAddress;
    PUCHAR Base;

    if (PortInformation->Address)
    {
        PhysicalAddress = (ULONG_PTR)PortInformation->Address;
    }
    else
    {
        if ((ComPortNumber == 0) || (ComPortNumber >= RTL_NUMBER_OF(BaseArray)))
            ComPortNumber = 1;
        PhysicalAddress = BaseArray[ComPortNumber];
    }

    Base = KdpMapUart(PhysicalAddress);
    if (!Base)
        return FALSE;

    /* The firmware already set the line up, only make sure it runs */
    WRITE_REGISTER_ULONG((PULONG)(Base + UART_CR),
                         READ_REGISTER_ULONG((PULONG)(Base + UART_CR)) |
                         UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);

    PortInformation->Address = Base;
    return TRUE;
}

BOOLEAN
NTAPI
KdPortGetByteEx(
    _In_ PCPPORT PortInformation,
    _Out_ PUCHAR ByteReceived)
{
    PUCHAR Base = PortInformation->Address;

    if (READ_REGISTER_ULONG((PULONG)(Base + UART_FR)) & UART_FR_RXFE)
        return FALSE;

    *ByteReceived = (UCHAR)READ_REGISTER_ULONG((PULONG)(Base + UART_DR));
    return TRUE;
}

VOID
NTAPI
KdPortPutByteEx(
    _In_ PCPPORT PortInformation,
    _In_ UCHAR ByteToSend)
{
    PUCHAR Base = PortInformation->Address;

    while (READ_REGISTER_ULONG((PULONG)(Base + UART_FR)) & UART_FR_TXFF)
        YieldProcessor();

    WRITE_REGISTER_ULONG((PULONG)(Base + UART_DR), ByteToSend);
}
