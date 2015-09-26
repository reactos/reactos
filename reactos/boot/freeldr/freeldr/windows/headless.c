/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/windows/headless.c
 * PURPOSE:         Provides support for Windows Emergency Management Services
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <cportlib/cportlib.h>

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
WinLdrLoadGUID(OUT PGUID SystemGuid)
{
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

    RtlZeroMemory(SystemGuid, SYSID_UUID_DATA_SIZE);
}

BOOLEAN
WinLdrPortInitialize(IN ULONG BaudRate,
                     IN ULONG PortNumber,
                     IN PUCHAR PortAddress,
                     IN BOOLEAN TerminalConnected,
                     OUT PULONG PortId)
{
    /* Set default baud rate */
    if (BaudRate == 0) BaudRate = 19200;

    /* Check if port or address given */
    if (PortNumber)
    {
        /* Pick correct address for port */
       if (!PortAddress)
        {
            switch (PortNumber)
            {
                case 1:
                    PortAddress = (PUCHAR)0x3F8;
                    break;

                case 2:
                    PortAddress = (PUCHAR)0x2F8;
                    break;

                case 3:
                    PortAddress = (PUCHAR)0x3E8;
                    break;

                default:
                    PortNumber = 4;
                    PortAddress = (PUCHAR)0x2E8;
            }
        }
    }
    else
    {
        /* Pick correct port for address */
        PortAddress = (PUCHAR)0x2F8;
        if (CpDoesPortExist(PortAddress))
        {
            PortNumber = 2;
        }
        else
        {
            PortAddress = (PUCHAR)0x3F8;
            if (!CpDoesPortExist(PortAddress)) return FALSE;
            PortNumber = 1;
         }
    }

    /* Not yet supported */
    ASSERT(LoaderRedirectionInformation.IsMMIODevice == FALSE);

    /* Check if port exists */
    if ((CpDoesPortExist(PortAddress)) || (CpDoesPortExist(PortAddress)))
    {
        /* Initialize port for first time, or re-initialize if specified */
        if (((TerminalConnected) && (Port[PortNumber - 1].Address)) ||
            !(Port[PortNumber - 1].Address))
        {
            /* Initialize the port, return it */
            CpInitialize(&Port[PortNumber - 1], PortAddress, BaudRate);
            *PortId = PortNumber - 1;
            return TRUE;
        }
    }

    return FALSE;
}

VOID
WinLdrPortPutByte(IN ULONG PortId,
                  IN UCHAR Byte)
{
    CpPutByte(&Port[PortId], Byte);
}

BOOLEAN
WinLdrPortGetByte(IN  ULONG  PortId,
                  OUT PUCHAR Byte)
{
    return CpGetByte(&Port[PortId], Byte, TRUE, FALSE) == CP_GET_SUCCESS;
}

BOOLEAN
WinLdrPortPollOnly(IN ULONG PortId)
{
    UCHAR Dummy;

    return CpGetByte(&Port[PortId], &Dummy, FALSE, TRUE) == CP_GET_SUCCESS;
}

VOID
WinLdrEnableFifo(IN ULONG PortId,
                 IN BOOLEAN Enable)
{
    CpEnableFifo(Port[PortId].Address, Enable);
}

VOID
WinLdrInitializeHeadlessPort(VOID)
{
    ULONG PortNumber, BaudRate;
    PUCHAR PortAddress;
    PCHAR AnsiReset = "\x1B[m";
    ULONG i;

    PortNumber = LoaderRedirectionInformation.PortNumber;
    PortAddress = LoaderRedirectionInformation.PortAddress;
    BaudRate = LoaderRedirectionInformation.BaudRate;

    /* Pick a port address */
    if (PortNumber)
    {
        if (!PortAddress)
        {
            switch (PortNumber)
            {
                case 2:
                    LoaderRedirectionInformation.PortAddress = (PUCHAR)0x2F8;
                    break;

                case 3:
                    LoaderRedirectionInformation.PortAddress = (PUCHAR)0x3E8;
                    break;

                case 4:
                    LoaderRedirectionInformation.PortAddress = (PUCHAR)0x2E8;
                    break;

                default:
                    LoaderRedirectionInformation.PortAddress = (PUCHAR)0x3F8;
                    break;
            }
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
    WinLdrTerminalConnected = WinLdrPortInitialize(
        BaudRate,
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

        /* Sent an ANSI reset sequence to get the terminal up and running */
        for (i = 0; i < strlen(AnsiReset); i++)
        {
            WinLdrPortPutByte(WinLdrTerminalDeviceId, AnsiReset[i]);
            StallExecutionProcessor(WinLdrTerminalDelay);
        }
    }
}

VOID
WinLdrSetupEms(IN PCHAR BootOptions)
{
    PCHAR Settings, RedirectPort;

    /* Start fresh */
    RtlZeroMemory(&LoaderRedirectionInformation, sizeof(HEADLESS_LOADER_BLOCK));
    LoaderRedirectionInformation.PciDeviceId = PCI_INVALID_VENDORID;

    /* Use a direction port if one was given, or use ACPI to detect one instead */
    Settings = strstr(BootOptions, "/redirect=");
    if (Settings)
    {
        RedirectPort = strstr(Settings, "com");
        if (RedirectPort)
        {
            RedirectPort += sizeof("com") - 1;
            LoaderRedirectionInformation.PortNumber = atoi(RedirectPort);
            LoaderRedirectionInformation.TerminalType = 1; // HeadlessSerialPort
        }
        else
        {
            RedirectPort = strstr(Settings, "usebiossettings");
            if (RedirectPort)
            {
                UiDrawStatusText("ACPI SRT Table Not Supported...");
                return;
            }
            else
            {
                LoaderRedirectionInformation.PortAddress = (PUCHAR)strtoul(Settings, 0, 16);
                if (LoaderRedirectionInformation.PortAddress)
                {
                    LoaderRedirectionInformation.PortNumber = 3;
                }
            }
        }
    }

    /* Use a direction baudrate if one was given */
    Settings = strstr(BootOptions, "/redirectbaudrate=");
    if (Settings)
    {
        if (strstr(Settings, "115200"))
        {
            LoaderRedirectionInformation.BaudRate = 115200;
        }
        else if (strstr(Settings, "57600"))
        {
            LoaderRedirectionInformation.BaudRate = 57600;
        }
        else if (strstr(Settings, "19200"))
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
