/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for IOCTL_TCP_QUERY_INFORMATION_EX
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <apitest.h>

#include <stdio.h>

#include <winioctl.h>
#include <tcpioctl.h>
#include <tdiinfo.h>
#include <iptypes.h>
#include <winsock.h>

/* FIXME */
#define AT_MIB_ADDRXLAT_INFO_ID 1
#define AT_MIB_ADDRXLAT_ENTRY_ID 0x101

/* Route info */
typedef struct IPRouteEntry {
    unsigned long ire_dest;
    unsigned long ire_index;
    unsigned long ire_metric1;
    unsigned long ire_metric2;
    unsigned long ire_metric3;
    unsigned long ire_metric4;
    unsigned long ire_nexthop;
    unsigned long ire_type;
    unsigned long ire_proto;
    unsigned long ire_age;
    unsigned long ire_mask;
    unsigned long ire_metric5;
    unsigned long ire_context;
} IPRouteEntry;

/* Present in headers for Vista+, but there in WinXP/2k3 ntdll */
NTSYSAPI
PSTR
NTAPI
RtlIpv4AddressToStringA(
  _In_ const struct in_addr *Addr,
  _Out_writes_(16) PSTR S);


static HANDLE TcpFileHandle;

static ULONG IndentationLevel = 0;

static
char*
dbg_print_physaddr(const unsigned char* addr, unsigned long addr_len)
{
    static char buffer[24];

    char* dest = buffer;
    *dest = '\0';

    while (addr_len--)
    {
        dest += sprintf(dest, "%02x", *addr);
        addr++;
        if (addr_len)
            *dest++ = ':';
    }

    return buffer;
}

static
int
__cdecl
indent_printf(const char* format, ...)
{
    ULONG Indent = IndentationLevel;
    int ret;
    va_list args;

    while(Indent--)
        printf("\t");

    va_start(args, format);
    ret = vprintf(format, args);
    va_end(args);

    ret += IndentationLevel;

    return ret;
}

static
void
test_IF_MIB_STATS(
    TDIEntityID Id,
    ULONG EntityType)
{
    IFEntry* IfEntry;
    TCP_REQUEST_QUERY_INFORMATION_EX Request;
    ULONG BufferSize = sizeof(IFEntry) + MAX_ADAPTER_DESCRIPTION_LENGTH + 1;
    BOOL Result;

    /* Not valid for other entity types */
    if (EntityType != IF_MIB)
        return;

    IfEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
    ok(IfEntry != NULL, "\n");

    ZeroMemory(&Request, sizeof(Request));
    Request.ID.toi_entity = Id;
    Request.ID.toi_class = INFO_CLASS_PROTOCOL;
    Request.ID.toi_type = INFO_TYPE_PROVIDER;
    Request.ID.toi_id = IF_MIB_STATS_ID;

    Result = DeviceIoControl(
        TcpFileHandle,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &Request,
        sizeof(Request),
        IfEntry,
        BufferSize,
        &BufferSize,
        NULL);
    ok(Result, "DeviceIoControl failed.\n");

    /* Dump it */
    indent_printf("IF_MIB Statistics:\n");
    IndentationLevel++;
    indent_printf("if_index:           %lu\n", IfEntry->if_index);
    indent_printf("if_type:            %lu\n", IfEntry->if_type);
    indent_printf("if_mtu:             %lu\n", IfEntry->if_mtu);
    indent_printf("if_speed:           %lu\n", IfEntry->if_speed);
    indent_printf("if_physaddr:        %s\n",  dbg_print_physaddr(IfEntry->if_physaddr, IfEntry->if_physaddrlen));
    indent_printf("if_adminstatus:     %lu\n", IfEntry->if_adminstatus);
    indent_printf("if_operstatus:      %lu\n", IfEntry->if_operstatus);
    indent_printf("if_lastchange:      %lu\n", IfEntry->if_lastchange);
    indent_printf("if_inoctets:        %lu\n", IfEntry->if_inoctets);
    indent_printf("if_inucastpkts:     %lu\n", IfEntry->if_inucastpkts);
    indent_printf("if_innucastpkts:    %lu\n", IfEntry->if_innucastpkts);
    indent_printf("if_indiscards:      %lu\n", IfEntry->if_indiscards);
    indent_printf("if_inerrors:        %lu\n", IfEntry->if_inerrors);
    indent_printf("if_inunknownprotos: %lu\n", IfEntry->if_inunknownprotos);
    indent_printf("if_outoctets:       %lu\n", IfEntry->if_outoctets);
    indent_printf("if_outucastpkts:    %lu\n", IfEntry->if_outucastpkts);
    indent_printf("if_outnucastpkts:   %lu\n", IfEntry->if_outnucastpkts);
    indent_printf("if_outdiscards:     %lu\n", IfEntry->if_outdiscards);
    indent_printf("if_outerrors:       %lu\n", IfEntry->if_outerrors);
    indent_printf("if_outqlen:         %lu\n", IfEntry->if_outqlen);
    indent_printf("if_descr:           %*s\n", IfEntry->if_descrlen, IfEntry->if_descr);
    IndentationLevel--;

    HeapFree(GetProcessHeap(), 0, IfEntry);
}

static
void
test_IP_MIB_STATS(
    TDIEntityID Id,
    ULONG EntityType)
{
    IPSNMPInfo IpSnmpInfo;
    TCP_REQUEST_QUERY_INFORMATION_EX Request;
    ULONG BufferSize = 0;
    BOOL Result;

    /* Not valid for other entity types */
    if (EntityType != CL_NL_IP)
        return;

    ZeroMemory(&IpSnmpInfo, sizeof(IpSnmpInfo));

    ZeroMemory(&Request, sizeof(Request));
    Request.ID.toi_entity = Id;
    Request.ID.toi_class = INFO_CLASS_PROTOCOL;
    Request.ID.toi_type = INFO_TYPE_PROVIDER;
    Request.ID.toi_id = IP_MIB_STATS_ID;

    Result = DeviceIoControl(
        TcpFileHandle,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &Request,
        sizeof(Request),
        &IpSnmpInfo,
        sizeof(IpSnmpInfo),
        &BufferSize,
        NULL);
    ok(Result, "DeviceIoControl failed.\n");

    /* Dump it */
    indent_printf("IP_MIB Statistics:\n");
    IndentationLevel++;
    indent_printf("ipsi_forwarding:      %lu\n", IpSnmpInfo.ipsi_forwarding);
    indent_printf("ipsi_defaultttl:      %lu\n", IpSnmpInfo.ipsi_defaultttl);
    indent_printf("ipsi_inreceives:      %lu\n", IpSnmpInfo.ipsi_inreceives);
    indent_printf("ipsi_inhdrerrors:     %lu\n", IpSnmpInfo.ipsi_inhdrerrors);
    indent_printf("ipsi_inaddrerrors:    %lu\n", IpSnmpInfo.ipsi_inaddrerrors);
    indent_printf("ipsi_forwdatagrams:   %lu\n", IpSnmpInfo.ipsi_forwdatagrams);
    indent_printf("ipsi_inunknownprotos: %lu\n", IpSnmpInfo.ipsi_inunknownprotos);
    indent_printf("ipsi_indiscards:      %lu\n", IpSnmpInfo.ipsi_indiscards);
    indent_printf("ipsi_indelivers:      %lu\n", IpSnmpInfo.ipsi_indelivers);
    indent_printf("ipsi_outrequests:     %lu\n", IpSnmpInfo.ipsi_outrequests);
    indent_printf("ipsi_routingdiscards: %lu\n", IpSnmpInfo.ipsi_routingdiscards);
    indent_printf("ipsi_outdiscards:     %lu\n", IpSnmpInfo.ipsi_outdiscards);
    indent_printf("ipsi_outnoroutes:     %lu\n", IpSnmpInfo.ipsi_outnoroutes);
    indent_printf("ipsi_reasmtimeout:    %lu\n", IpSnmpInfo.ipsi_reasmtimeout);
    indent_printf("ipsi_reasmreqds:      %lu\n", IpSnmpInfo.ipsi_reasmreqds);
    indent_printf("ipsi_reasmoks:        %lu\n", IpSnmpInfo.ipsi_reasmoks);
    indent_printf("ipsi_reasmfails:      %lu\n", IpSnmpInfo.ipsi_reasmfails);
    indent_printf("ipsi_fragoks:         %lu\n", IpSnmpInfo.ipsi_fragoks);
    indent_printf("ipsi_fragfails:       %lu\n", IpSnmpInfo.ipsi_fragfails);
    indent_printf("ipsi_fragcreates:     %lu\n", IpSnmpInfo.ipsi_fragcreates);
    indent_printf("ipsi_numif:           %lu\n", IpSnmpInfo.ipsi_numif);
    indent_printf("ipsi_numaddr:         %lu\n", IpSnmpInfo.ipsi_numaddr);
    indent_printf("ipsi_numroutes:       %lu\n", IpSnmpInfo.ipsi_numroutes);

    if (IpSnmpInfo.ipsi_numaddr != 0)
    {
        IPAddrEntry* AddrEntries;
        ULONG i;

        AddrEntries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, IpSnmpInfo.ipsi_numaddr * sizeof(AddrEntries[0]));
        ok(AddrEntries != NULL, "\n");

        ZeroMemory(&Request, sizeof(Request));
        Request.ID.toi_entity = Id;
        Request.ID.toi_class = INFO_CLASS_PROTOCOL;
        Request.ID.toi_type = INFO_TYPE_PROVIDER;
        Request.ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

        Result = DeviceIoControl(
            TcpFileHandle,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &Request,
            sizeof(Request),
            AddrEntries,
            IpSnmpInfo.ipsi_numaddr * sizeof(AddrEntries[0]),
            &BufferSize,
            NULL);
        ok(Result, "DeviceIoControl failed.\n");
        ok_long(BufferSize, IpSnmpInfo.ipsi_numaddr * sizeof(AddrEntries[0]));

        for(i = 0; i < IpSnmpInfo.ipsi_numaddr; i++)
        {
            CHAR AddressString[16];
            struct in_addr Addr;

            Addr.S_un.S_addr = AddrEntries[i].iae_addr;
            RtlIpv4AddressToStringA(&Addr, AddressString);

            indent_printf("Address %lu: %s\n", i, AddressString);

            IndentationLevel++;

            indent_printf("iae_addr:      %lx\n", AddrEntries[i].iae_addr);
            indent_printf("iae_index:     %lu\n", AddrEntries[i].iae_index);
            Addr.S_un.S_addr = AddrEntries[i].iae_mask;
            RtlIpv4AddressToStringA(&Addr, AddressString);
            indent_printf("iae_mask:      %lx (%s)\n", AddrEntries[i].iae_mask, AddressString);
            indent_printf("iae_bcastaddr: %lu\n", AddrEntries[i].iae_bcastaddr);
            indent_printf("iae_reasmsize: %lu\n", AddrEntries[i].iae_reasmsize);
            indent_printf("iae_context:   %u\n",  AddrEntries[i].iae_context);

            {
                IPInterfaceInfo* InterfaceInfo;

                /* Get the interface info */
                BufferSize = sizeof(IPInterfaceInfo) + MAX_PHYSADDR_SIZE;
                InterfaceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
                ok(InterfaceInfo != NULL, "\n");

                Request.ID.toi_id = IP_INTFC_INFO_ID;
                Request.Context[0] = AddrEntries[i].iae_addr;
                Result = DeviceIoControl(
                    TcpFileHandle,
                    IOCTL_TCP_QUERY_INFORMATION_EX,
                    &Request,
                    sizeof(Request),
                    InterfaceInfo,
                    BufferSize,
                    &BufferSize,
                    NULL);
                ok(Result, "DeviceIoControl failed.\n");

                indent_printf("Interface info:\n");
                IndentationLevel++;

                indent_printf("iii_flags:    %lu\n", InterfaceInfo->iii_flags);
                indent_printf("iii_mtu  :    %lu\n", InterfaceInfo->iii_mtu);
                indent_printf("iii_speed:    %lu\n", InterfaceInfo->iii_speed);
                indent_printf("iii_physaddr: %s\n",  dbg_print_physaddr(InterfaceInfo->iii_addr, InterfaceInfo->iii_addrlength));

                IndentationLevel--;
            }

            IndentationLevel--;
        }

        HeapFree(GetProcessHeap(), 0, AddrEntries);
    }

    /* See for the routes */
    if (IpSnmpInfo.ipsi_numroutes)
    {
        IPRouteEntry* RouteEntries;
        ULONG i;

        RouteEntries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, IpSnmpInfo.ipsi_numroutes * sizeof(RouteEntries[0]));
        ok(RouteEntries != NULL, "\n");

        ZeroMemory(&Request, sizeof(Request));
        Request.ID.toi_entity = Id;
        Request.ID.toi_class = INFO_CLASS_PROTOCOL;
        Request.ID.toi_type = INFO_TYPE_PROVIDER;
        Request.ID.toi_id = IP_MIB_ARPTABLE_ENTRY_ID;

        Result = DeviceIoControl(
            TcpFileHandle,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &Request,
            sizeof(Request),
            RouteEntries,
            IpSnmpInfo.ipsi_numroutes * sizeof(RouteEntries[0]),
            &BufferSize,
            NULL);
        ok(Result, "DeviceIoControl failed.\n");
        ok_long(BufferSize, IpSnmpInfo.ipsi_numroutes * sizeof(RouteEntries[0]));

        for (i = 0; i < IpSnmpInfo.ipsi_numroutes; i++)
        {
            CHAR AddressString[16];
            struct in_addr Addr;

            Addr.S_un.S_addr = RouteEntries[i].ire_dest;
            RtlIpv4AddressToStringA(&Addr, AddressString);

            indent_printf("Route %lu:\n", i);

            IndentationLevel++;

            indent_printf("ire_dest:    %s (%lx)\n", AddressString, RouteEntries[i].ire_dest);
            indent_printf("ire_index:   %lu\n", RouteEntries[i].ire_index);
            indent_printf("ire_metric1: %#lx\n", RouteEntries[i].ire_metric1);
            indent_printf("ire_metric2: %#lx\n", RouteEntries[i].ire_metric2);
            indent_printf("ire_metric3: %#lx\n", RouteEntries[i].ire_metric3);
            indent_printf("ire_metric4: %#lx\n", RouteEntries[i].ire_metric4);
            Addr.S_un.S_addr = RouteEntries[i].ire_nexthop;
            RtlIpv4AddressToStringA(&Addr, AddressString);
            indent_printf("ire_nexthop: %s (%lx)\n", AddressString, RouteEntries[i].ire_nexthop);
            indent_printf("ire_type:    %lu\n", RouteEntries[i].ire_type);
            indent_printf("ire_proto:   %lu\n", RouteEntries[i].ire_proto);
            indent_printf("ire_age:     %lu\n", RouteEntries[i].ire_age);
            Addr.S_un.S_addr = RouteEntries[i].ire_mask;
            RtlIpv4AddressToStringA(&Addr, AddressString);
            indent_printf("ire_mask:    %s (%lx)\n", AddressString, RouteEntries[i].ire_mask);
            indent_printf("ire_metric5: %lx\n", RouteEntries[i].ire_metric5);
            indent_printf("ire_context: %lx\n", RouteEntries[i].ire_context);

            IndentationLevel--;
        }
    }

    IndentationLevel--;
}

typedef struct ARPInfo
{
    unsigned long ai_numroutes;
    unsigned long ai_unknown;
} ARPInfo;

typedef struct ARPEntry
{
    unsigned long ae_index;
    unsigned long ae_physaddrlen;
    unsigned char ae_physaddr[MAX_PHYSADDR_SIZE];
    unsigned long ae_address;
    unsigned long ae_unknown;
} ARPEntry;

static
void
test_AT_ARP_STATS(
    TDIEntityID Id,
    ULONG EntityType)
{
    ARPInfo ArpInfo;
    TCP_REQUEST_QUERY_INFORMATION_EX Request;
    ULONG BufferSize = 0;
    BOOL Result;

    /* Not valid for other entity types */
    if (EntityType != AT_ARP)
        return;

    ZeroMemory(&Request, sizeof(Request));
    Request.ID.toi_entity = Id;
    Request.ID.toi_class = INFO_CLASS_PROTOCOL;
    Request.ID.toi_type = INFO_TYPE_PROVIDER;
    Request.ID.toi_id = AT_MIB_ADDRXLAT_INFO_ID;

    Result = DeviceIoControl(
        TcpFileHandle,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &Request,
        sizeof(Request),
        &ArpInfo,
        sizeof(ArpInfo),
        &BufferSize,
        NULL);
    ok(Result, "DeviceIoControl failed.\n");
    ok_long(BufferSize, sizeof(ArpInfo));

    indent_printf("ARP Info:\n");
    IndentationLevel++;

    indent_printf("ai_numroutes: %lu\n", ArpInfo.ai_numroutes);
    indent_printf("ai_unknown:   %lx\n", ArpInfo.ai_unknown);

    if (ArpInfo.ai_numroutes)
    {
        ARPEntry* ArpEntries;
        ULONG i;

        Request.ID.toi_id = AT_MIB_ADDRXLAT_ENTRY_ID;

        ArpEntries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ArpInfo.ai_numroutes * sizeof(ArpEntries[0]));
        ok(ArpEntries != NULL, "\n");

        Result = DeviceIoControl(
            TcpFileHandle,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &Request,
            sizeof(Request),
            ArpEntries,
            ArpInfo.ai_numroutes * sizeof(ArpEntries[0]),
            &BufferSize,
            NULL);
        ok(Result, "DeviceIoControl failed.\n");
        ok_long(BufferSize, ArpInfo.ai_numroutes * sizeof(ArpEntries[0]));

        for (i = 0; i < ArpInfo.ai_numroutes; i++)
        {
            CHAR AddressString[16];
            struct in_addr Addr;

            Addr.S_un.S_addr = ArpEntries[i].ae_address;
            RtlIpv4AddressToStringA(&Addr, AddressString);

            indent_printf("ARP Entry %lu:\n", i);

            IndentationLevel++;

            indent_printf("ae_index:    %lu\n", ArpEntries[i].ae_index);
            indent_printf("ae_physaddr: %s\n", dbg_print_physaddr(ArpEntries[i].ae_physaddr, ArpEntries[i].ae_physaddrlen));
            indent_printf("ae_address:  %lx (%s)\n", ArpEntries[i].ae_address, AddressString);
            indent_printf("ae_unknown:  %lu.\n", ArpEntries[i].ae_unknown);

            IndentationLevel--;
        }

        HeapFree(GetProcessHeap(), 0, ArpEntries);
    }

    IndentationLevel--;
}

START_TEST(tcp_info)
{
    TDIEntityID* Entities;
    DWORD BufferSize;
    BOOL Result;
    ULONG i, EntityCount;
    TCP_REQUEST_QUERY_INFORMATION_EX Request;

    /* Open a control channel file for TCP */
    TcpFileHandle = CreateFileW(
        L"\\\\.\\Tcp",
        FILE_READ_DATA | FILE_WRITE_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    ok(TcpFileHandle != INVALID_HANDLE_VALUE, "CreateFile failed, GLE %lu\n", GetLastError());

    /* Try the IOCTL */
    BufferSize = 0;
    Result = DeviceIoControl(
        TcpFileHandle,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        NULL,
        0,
        NULL,
        0,
        &BufferSize,
        NULL);
    ok(!Result, "DeviceIoControl succeeded.\n");
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);
    ok_long(BufferSize, 0);

    ZeroMemory(&Request, sizeof(Request));
    Request.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    Request.ID.toi_entity.tei_instance = 0;
    Request.ID.toi_class = INFO_CLASS_GENERIC;
    Request.ID.toi_type = INFO_TYPE_PROVIDER;
    Request.ID.toi_id = ENTITY_LIST_ID;

    BufferSize = 0;
    Result = DeviceIoControl(
        TcpFileHandle,
        IOCTL_TCP_QUERY_INFORMATION_EX,
        &Request,
        sizeof(Request),
        NULL,
        0,
        &BufferSize,
        NULL);
    ok(!Result, "DeviceIoControl succeeded.\n");
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);
    ok_long(BufferSize, 0);

    BufferSize = 4 * sizeof(Entities[0]);
    Entities = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
    ok(Entities != NULL, "\n");

    while (TRUE)
    {
        Result = DeviceIoControl(
            TcpFileHandle,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &Request,
            sizeof(Request),
            Entities,
            BufferSize,
            &BufferSize,
            NULL);

        if (Result)
            break;

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            break;

        BufferSize *= 2;
        Entities = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Entities, BufferSize);
        ok(Entities != NULL, "\n");
    }

    ok(Result, "DeviceIoControl failed!\n");
    EntityCount = BufferSize / sizeof(Entities[0]);
    trace("Got %lu entities.\n", EntityCount);

    for (i = 0; i < EntityCount; i++)
    {
        ULONG EntityType;

        /* Get the type */
        Request.ID.toi_entity = Entities[i];
        Request.ID.toi_class = INFO_CLASS_GENERIC;
        Request.ID.toi_type = INFO_TYPE_PROVIDER;
        Request.ID.toi_id = ENTITY_TYPE_ID;

        Result = DeviceIoControl(
            TcpFileHandle,
            IOCTL_TCP_QUERY_INFORMATION_EX,
            &Request,
            sizeof(Request),
            &EntityType,
            sizeof(EntityType),
            &BufferSize,
            NULL);
        ok(Result, "DeviceIoControl failed.\n");

        printf("Entity %lu: %#lx, %#lx, type %#lx\n", i, Entities[i].tei_entity, Entities[i].tei_instance, EntityType);
        test_IF_MIB_STATS(Entities[i], EntityType);
        test_IP_MIB_STATS(Entities[i], EntityType);
        test_AT_ARP_STATS(Entities[i], EntityType);
    }

    HeapFree(GetProcessHeap(), 0, Entities);
    CloseHandle(TcpFileHandle);
}
