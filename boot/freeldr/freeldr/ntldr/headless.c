/*
 * PROJECT:     ReactOS Boot Loader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Provides support for Windows Emergency Management Services
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group
 *              Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <cportlib/cportlib.h>
#include <cportlib/uartinfo.h>
#include "ntldropts.h"

#include <debug.h> // For _WARN()

/* Note: Move these to some smbios.h header */
#define SYSID_TYPE_UUID "_UUID_"
#define SYSID_UUID_DATA_SIZE 16
#include <pshpack1.h>
typedef struct _SYSID_UUID_ENTRY
{
    UCHAR Type[6];
    UCHAR Checksum;
    USHORT Length;
    UCHAR UUID[SYSID_UUID_DATA_SIZE];
} SYSID_UUID_ENTRY, *PSYSID_UUID_ENTRY;
#include <poppack.h>

/* GLOBALS ********************************************************************/

HEADLESS_LOADER_BLOCK LoaderRedirectionInformation;
BOOLEAN WinLdrTerminalConnected;
ULONG WinLdrTerminalDeviceId;
ULONG WinLdrTerminalDelay;

CPPORT Port[4] =
{
    {NULL, 0, TRUE},
    {NULL, 0, TRUE},
    {NULL, 0, TRUE},
    {NULL, 0, TRUE}
};

/* FUNCTIONS ******************************************************************/

VOID
WinLdrLoadGUID(
    _Out_ PGUID SystemGuid)
{
#if (defined(_M_IX86) || defined(_M_AMD64)) && !defined(UEFIBOOT)
    PSYSID_UUID_ENTRY CurrentAddress;

    CurrentAddress = (PSYSID_UUID_ENTRY)0xE0000;
    while (CurrentAddress < (PSYSID_UUID_ENTRY)0x100000)
    {
        if (RtlCompareMemory(&CurrentAddress->Type, SYSID_TYPE_UUID, 6) == 6)
        {
            RtlCopyMemory(SystemGuid, &CurrentAddress->UUID, SYSID_UUID_DATA_SIZE);
            return;
        }
        CurrentAddress = (PSYSID_UUID_ENTRY)((ULONG_PTR)CurrentAddress + 1);
    }
#else
    _WARN("WinLdrLoadGUID needs SMBIOS table reading implementation on this platform!");
#endif

    RtlZeroMemory(SystemGuid, SYSID_UUID_DATA_SIZE);
}

/* NOTE: This function mirrors nt!inbvport.c:InbvPortInitialize() */
BOOLEAN
WinLdrPortInitialize(
    _In_ ULONG BaudRate,
    _In_ ULONG PortNumber,
    _In_ PUCHAR PortAddress,
    _In_ BOOLEAN TerminalConnected,
    _Out_ PULONG PortId)
{
    /* Set the default baud rate */
    if (BaudRate == 0)
        BaudRate = DEFAULT_BAUD_RATE;

    /* Check if the port or address is given */
    if (PortNumber)
    {
        /* Pick correct address for port */
        if (!PortAddress)
        {
            if (PortNumber < 1 || PortNumber > MAX_COM_PORTS)
                PortNumber = MAX_COM_PORTS;
            PortAddress = UlongToPtr(BaseArray[PortNumber]);
        }
    }
    else
    {
        /* Pick correct port for address */
#if defined(SARCH_PC98)
        static const ULONG TestPorts[] = {1, 2};
#else
        static const ULONG TestPorts[] = {2, 1};
#endif
        PortAddress = UlongToPtr(BaseArray[TestPorts[0]]);
        if (CpDoesPortExist(PortAddress))
        {
            PortNumber = TestPorts[0];
        }
        else
        {
            PortAddress = UlongToPtr(BaseArray[TestPorts[1]]);
            if (!CpDoesPortExist(PortAddress))
                return FALSE;
            PortNumber = TestPorts[1];
        }
    }

    /* Not yet supported */
    ASSERT(LoaderRedirectionInformation.IsMMIODevice == FALSE);

    /* Check if port exists */
    if (CpDoesPortExist(PortAddress))
    {
        /* Initialize port for the first time, or re-initialize if specified */
        if (!!TerminalConnected == (Port[PortNumber - 1].Address != NULL))
        {
            /* Initialize the port and return it */
            CpInitialize(&Port[PortNumber - 1], PortAddress, BaudRate);
            *PortId = PortNumber - 1;
            return TRUE;
        }
    }

    return FALSE;
}

VOID
WinLdrPortPutByte(
    _In_ ULONG PortId,
    _In_ UCHAR Byte)
{
    CpPutByte(&Port[PortId], Byte);
}

BOOLEAN
WinLdrPortGetByte(
    _In_ ULONG PortId,
    _Out_ PUCHAR Byte)
{
    return CpGetByte(&Port[PortId], Byte, TRUE, FALSE) == CP_GET_SUCCESS;
}

BOOLEAN
WinLdrPortPollOnly(
    _In_ ULONG PortId)
{
    UCHAR Dummy;

    return CpGetByte(&Port[PortId], &Dummy, FALSE, TRUE) == CP_GET_SUCCESS;
}

VOID
WinLdrEnableFifo(
    _In_ ULONG PortId,
    _In_ BOOLEAN Enable)
{
    CpEnableFifo(Port[PortId].Address, Enable);
}

VOID
WinLdrInitializeHeadlessPort(VOID)
{
    ULONG PortNumber, BaudRate;
    PUCHAR PortAddress;
    PCSTR AnsiReset = "\x1B[m";
    ULONG i;

    PortNumber = LoaderRedirectionInformation.PortNumber;
    PortAddress = LoaderRedirectionInformation.PortAddress;
    BaudRate = LoaderRedirectionInformation.BaudRate;

    /* Pick a port address */
    if (PortNumber)
    {
        if (!PortAddress)
        {
            if (PortNumber < 1 || PortNumber > MAX_COM_PORTS)
                PortNumber = 1;
            LoaderRedirectionInformation.PortAddress = UlongToPtr(BaseArray[PortNumber]);
        }
    }
    else
    {
        /* No number, so no EMS */
        WinLdrTerminalConnected = FALSE;
        return;
    }

    /* Call arch code to initialize the port */
    PortAddress = LoaderRedirectionInformation.PortAddress;
    WinLdrTerminalConnected = WinLdrPortInitialize(BaudRate,
                                                   PortNumber,
                                                   PortAddress,
                                                   WinLdrTerminalConnected,
                                                   &WinLdrTerminalDeviceId);
    if (WinLdrTerminalConnected)
    {
        /* Port seems usable, set it up and get the BIOS GUID */
        WinLdrEnableFifo(WinLdrTerminalDeviceId, TRUE);

        WinLdrLoadGUID(&LoaderRedirectionInformation.SystemGUID);

        /* Calculate delay in us based on the baud, assume 9600 if none given */
        if (!BaudRate)
        {
            BaudRate = 9600;
            LoaderRedirectionInformation.BaudRate = BaudRate;
        }

        WinLdrTerminalDelay = (10 * 1000 * 1000) / (BaudRate / 10) / 6;

        /* Send an ANSI reset sequence to get the terminal up and running */
        for (i = 0; i < strlen(AnsiReset); i++)
        {
            WinLdrPortPutByte(WinLdrTerminalDeviceId, AnsiReset[i]);
            StallExecutionProcessor(WinLdrTerminalDelay);
        }
    }
}

VOID
WinLdrSetupEms(
    _In_ PCSTR BootOptions)
{
    PCSTR Option;

    /* Start fresh */
    RtlZeroMemory(&LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK));
    LoaderRedirectionInformation.PciDeviceId = PCI_INVALID_VENDORID;

    /* Use a direction port if one was given, or use ACPI to detect one instead */
    Option = NtLdrGetOption(BootOptions, "redirect=");
    if (Option)
    {
        Option += 9;
        if (_strnicmp(Option, "com", 3) == 0)
        {
            Option += 3;
            LoaderRedirectionInformation.PortNumber = atoi(Option);
            LoaderRedirectionInformation.TerminalType = 1; // VT100+
        }
        else if (_strnicmp(Option, "usebiossettings", 15) == 0)
        {
            // FIXME: TODO!
            UiDrawStatusText("ACPI SRT/SPCR Table Not Supported...");
            return;
        }
        else
        {
#ifdef _WIN64
#define strtoulptr strtoull
#else
#define strtoulptr strtoul
#endif
            LoaderRedirectionInformation.PortAddress = (PUCHAR)strtoulptr(Option, 0, 16);
            if (LoaderRedirectionInformation.PortAddress)
                LoaderRedirectionInformation.PortNumber = 3;
        }
    }

    /* Use a direction baudrate if one was given */
    Option = NtLdrGetOption(BootOptions, "redirectbaudrate=");
    if (Option)
    {
        Option += 17;
        // LoaderRedirectionInformation.BaudRate = atoi(Option);
        if (strncmp(Option, "115200", 6) == 0)
        {
            LoaderRedirectionInformation.BaudRate = 115200;
        }
        else if (strncmp(Option, "57600", 5) == 0)
        {
            LoaderRedirectionInformation.BaudRate = 57600;
        }
        else if (strncmp(Option, "19200", 5) == 0)
        {
            LoaderRedirectionInformation.BaudRate = 19200;
        }
        else
        {
            LoaderRedirectionInformation.BaudRate = 9600;
        }
    }

    /* Enable headless support if parameters were found */
    if (LoaderRedirectionInformation.PortNumber)
    {
        if (!LoaderRedirectionInformation.BaudRate)
        {
            LoaderRedirectionInformation.BaudRate = 9600;
        }

        WinLdrInitializeHeadlessPort();
    }
}

/* EOF */
