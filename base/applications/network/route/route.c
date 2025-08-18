/* Poor man's route
 *
 * Supported commands:
 *
 * "print"
 * "add" target ["mask" mask] gw ["metric" metric]
 * "delete" target gw
 *
 * Goals:
 *
 * Flexible, simple
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <malloc.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <iphlpapi.h>
#include <conutils.h>

#include "resource.h"

#define IPBUF 17
#define IN_ADDR_OF(x) *((struct in_addr *)&(x))

#define FLUSH_FLAG        0x1
#define PERSISTENT_FLAG   0x2

static
BOOL
MatchWildcard(
    _In_ PWSTR Text,
    _In_ PWSTR Pattern)
{
    size_t TextLength, PatternLength, TextIndex = 0, PatternIndex = 0;
    size_t StartIndex = -1, MatchIndex = 0;

    TextLength = wcslen(Text);
    PatternLength = wcslen(Pattern);

    while (TextIndex < TextLength)
    {
        if ((PatternIndex < PatternLength) &&
            ((Pattern[PatternIndex] == L'?') ||
             (towlower(Pattern[PatternIndex]) == towlower(Text[TextIndex]))))
        {
            TextIndex++;
            PatternIndex++;
        }
        else if ((PatternIndex < PatternLength) &&
                 (Pattern[PatternIndex] == L'*'))
        {
            StartIndex = PatternIndex;
            MatchIndex = TextIndex;
            PatternIndex++;
        }
        else if (StartIndex != -1)
        {
            PatternIndex = StartIndex + 1;
            MatchIndex++;
            TextIndex = MatchIndex;
        }
        else
        {
            return FALSE;
        }
    }

    while ((PatternIndex < PatternLength) &&
           (Pattern[PatternIndex] == L'*'))
    {
        PatternIndex++;
    }

    return (PatternIndex == PatternLength);
}

static
VOID
PrintMacAddress(
    _In_ PBYTE Mac,
    _In_ PWSTR Buffer)
{
    swprintf(Buffer, L"%02X %02X %02X %02X %02X %02X ",
        Mac[0], Mac[1], Mac[2], Mac[3], Mac[4],  Mac[5]);
}

static
int
PrintRoutes(
    _In_ int argc,
    _In_ WCHAR **argv,
    _In_ int start,
    _In_ int flags)
{
    PMIB_IPFORWARDTABLE IpForwardTable = NULL;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL;
    ULONG Size = 0;
    DWORD Error = ERROR_SUCCESS;
    ULONG adaptOutBufLen = 15000;
    WCHAR Destination[IPBUF], Gateway[IPBUF], Netmask[IPBUF];
    unsigned int i;
    BOOL EntriesFound;
    ULONG Flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                  GAA_FLAG_SKIP_DNS_SERVER;
    PWSTR Filter = NULL;

    if (argc > start)
        Filter = argv[start];

    /* set required buffer size */
    pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(adaptOutBufLen);
    if (pAdapterAddresses == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    if (GetAdaptersAddresses(AF_INET, Flags, NULL, pAdapterAddresses, &adaptOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
       free(pAdapterAddresses);
       pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(adaptOutBufLen);
       if (pAdapterAddresses == NULL)
       {
           Error = ERROR_NOT_ENOUGH_MEMORY;
           goto Error;
       }
    }

    if ((GetIpForwardTable(NULL, &Size, TRUE)) == ERROR_INSUFFICIENT_BUFFER)
    {
        if (!(IpForwardTable = malloc(Size)))
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (((Error = GetAdaptersAddresses(AF_INET, Flags, NULL, pAdapterAddresses, &adaptOutBufLen)) == NO_ERROR) &&
        ((Error = GetIpForwardTable(IpForwardTable, &Size, TRUE)) == NO_ERROR))
    {
        ConResPrintf(StdOut, IDS_SEPARATOR);
        ConResPrintf(StdOut, IDS_INTERFACE_LIST);
        /* FIXME - sort by the index! */
        while (pAdapterAddresses)
        {
            if (pAdapterAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
            {
                WCHAR PhysicalAddress[20];
                PrintMacAddress(pAdapterAddresses->PhysicalAddress, PhysicalAddress);
                ConResPrintf(StdOut, IDS_ETHERNET_ENTRY, pAdapterAddresses->IfIndex, PhysicalAddress, pAdapterAddresses->Description);
            }
            else
            {
                ConResPrintf(StdOut, IDS_INTERFACE_ENTRY, pAdapterAddresses->IfIndex, pAdapterAddresses->Description);
            }
            pAdapterAddresses = pAdapterAddresses->Next;
        }
        ConResPrintf(StdOut, IDS_SEPARATOR);

        ConResPrintf(StdOut, IDS_IPV4_ROUTE_TABLE);
        ConResPrintf(StdOut, IDS_SEPARATOR);
        ConResPrintf(StdOut, IDS_ACTIVE_ROUTES);
        EntriesFound = FALSE;
        for (i = 0; i < IpForwardTable->dwNumEntries; i++)
        {
            mbstowcs(Destination, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardDest)), IPBUF);
            mbstowcs(Netmask, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardMask)), IPBUF);
            mbstowcs(Gateway, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardNextHop)), IPBUF);

            if ((Filter == NULL) || MatchWildcard(Destination, Filter))
            {
                if (EntriesFound == FALSE)
                    ConResPrintf(StdOut, IDS_ROUTES_HEADER);
                ConResPrintf(StdOut, IDS_ROUTES_ENTRY,
                             Destination,
                             Netmask,
                             Gateway,
                             IpForwardTable->table[i].dwForwardIfIndex,
                             IpForwardTable->table[i].dwForwardMetric1);
                EntriesFound = TRUE;
            }
        }

        if (Filter == NULL)
        {
            for (i = 0; i < IpForwardTable->dwNumEntries; i++)
            {
                if (IpForwardTable->table[i].dwForwardDest == 0)
                {
                    mbstowcs(Gateway, inet_ntoa(IN_ADDR_OF(IpForwardTable->table[i].dwForwardNextHop)), IPBUF);
                    ConResPrintf(StdOut, IDS_DEFAULT_GATEWAY, Gateway);
                }
            }
        }
        else if (EntriesFound == FALSE)
            ConResPrintf(StdOut, IDS_NONE);
        ConResPrintf(StdOut, IDS_SEPARATOR);

        ConResPrintf(StdOut, IDS_PERSISTENT_ROUTES);
        ConResPrintf(StdOut, IDS_NONE);
    }

Error:
    if (pAdapterAddresses)
        free(pAdapterAddresses);
    if (IpForwardTable)
        free(IpForwardTable);

    if (Error != ERROR_SUCCESS)
        ConResPrintf(StdErr, IDS_ROUTE_ENUM_ERROR);

    return (Error == ERROR_SUCCESS) ? 0 : 2;
}

static int
ConvertAddCmdLine(
    _Out_ PMIB_IPFORWARDROW RowToAdd,
    _In_ int argc,
    _In_ WCHAR **argv,
    _In_ int start)
{
    int i;
    char addr[16];

    if (argc > start)
    {
        wcstombs(addr, argv[start], 16);
        RowToAdd->dwForwardDest = inet_addr(addr);
    }
    else
        return FALSE;

    for (i = start + 1; i < argc; i++)
    {
        if (!_wcsicmp(argv[i], L"mask"))
        {
            i++;
            if (i >= argc)
                return FALSE;
            wcstombs(addr, argv[i], 16);
            RowToAdd->dwForwardMask = inet_addr(addr);
        }
        else if (!_wcsicmp(argv[i], L"metric"))
        {
            i++;
            if (i >= argc) 
                return FALSE;
            RowToAdd->dwForwardMetric1 = _wtoi(argv[i]);
        }
        else
        {
            wcstombs(addr, argv[i], 16);
            RowToAdd->dwForwardNextHop = inet_addr(addr);
        }
    }

    return TRUE;
}

static
int
AddRoute(
    _In_ int argc,
    _In_ WCHAR **argv,
    _In_ int start,
    _In_ int flags)
{
    MIB_IPFORWARDROW RowToAdd = { 0 };
    DWORD Error;

    if ((argc <= start) || !ConvertAddCmdLine(&RowToAdd, argc, argv, start))
        return 1;

    Error = CreateIpForwardEntry(&RowToAdd);
    if (Error != ERROR_SUCCESS)
    {
        ConResPrintf(StdErr, IDS_ROUTE_ADD_ERROR);
        return 2;
    }

    return 0;
}

static int
DeleteRoute(
    _In_ int argc,
    _In_ WCHAR **argv,
    _In_ int start,
    _In_ int flags)
{
    MIB_IPFORWARDROW RowToDel = { 0 };
    DWORD Error;

    if ((argc < start + 2) || !ConvertAddCmdLine(&RowToDel, argc, argv, start))
        return 1;

    Error = DeleteIpForwardEntry(&RowToDel);
    if (Error != ERROR_SUCCESS)
    {
        ConResPrintf(StdErr, IDS_ROUTE_DEL_ERROR);
        return 2;
    }

    return 0;
}

int
wmain(
    _In_ int argc,
    _In_ WCHAR **argv)
{
    int i, flags = 0, ret = 0;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == L'-' || argv[i][0] == L'/')
        {
            if (!_wcsicmp(&argv[i][1], L"f"))
            {
                flags |= FLUSH_FLAG;
            }
            else if (!_wcsicmp(&argv[i][1], L"p"))
            {
                flags |= PERSISTENT_FLAG;
            }
            else
            {
                ret = 1;
                break;
            }
        }
        else
        {
            if (!_wcsicmp(argv[i], L"print"))
                ret = PrintRoutes(argc, argv, i + 1, flags);
            else if (!_wcsicmp(argv[i], L"add"))
                ret = AddRoute(argc, argv, i + 1, flags);
            else if (!_wcsicmp(argv[i], L"delete"))
                ret = DeleteRoute(argc, argv, i + 1, flags);
            else
                ret = 1;

            break;
        }
    }

    if (argc == 1)
        ret = 1;

    if (ret == 1)
    {
        ConResPrintf(StdErr, IDS_USAGE1);
//        ConResPrintf(StdErr, IDS_USAGE2);
        ConResPrintf(StdErr, IDS_USAGE3);
    }

    return (ret == 0) ? 0 : 1;
}
