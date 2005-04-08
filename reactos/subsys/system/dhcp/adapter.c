#include "rosdhcp.h"

static LIST_ENTRY AdapterList;
static WSADATA wsd;
extern struct interface_info *ifi;

DWORD GetAddress( PDHCP_ADAPTER Adapter ) {
    PMIB_IPADDRTABLE AddressTable = NULL;
    ULONG i, Size = 0, NumAddressRows;
    DWORD Error = GetIpAddrTable( AddressTable, &Size, FALSE );

    while( Error == ERROR_INSUFFICIENT_BUFFER ) {
        free( AddressTable );
        AddressTable = malloc( Size );
        if( AddressTable ) 
            Error = GetIpAddrTable( AddressTable, &Size, FALSE );
    }
    if( Error != ERROR_SUCCESS ) {
        free( AddressTable );
        return Error;
    }

    NumAddressRows = Size / sizeof(MIB_IPADDRTABLE);
    for( i = 0; i < AddressTable->dwNumEntries; i++ ) {
        DH_DbgPrint(MID_TRACE,
                    ("Finding address for adapter %d: (%d -> %x)\n", 
                     Adapter->IfMib.dwIndex, 
                     AddressTable->table[i].dwIndex,
                     AddressTable->table[i].dwAddr));
        if( Adapter->IfMib.dwIndex == AddressTable->table[i].dwIndex ) {
            memcpy( &Adapter->IfAddr, &AddressTable->table[i],
                    sizeof( MIB_IPADDRROW ) );
        }
    }
}

void AdapterInit() {
    PMIB_IFTABLE Table = malloc(sizeof(MIB_IFTABLE));
    DWORD Error, Size, i;
    PDHCP_ADAPTER Adapter = NULL;

    WSAStartup(0x0101,&wsd);

    InitializeListHead( &AdapterList );

    DH_DbgPrint(MID_TRACE,("Getting Adapter List...\n"));

    while( (Error = GetIfTable(Table, &Size, 0 )) == 
           ERROR_INSUFFICIENT_BUFFER ) {
        DH_DbgPrint(MID_TRACE,("Error %d, New Buffer Size: %d\n", Error, Size));
        free( Table );
        Table = malloc( Size );
    }

    if( Error != NO_ERROR ) goto term;

    DH_DbgPrint(MID_TRACE,("Got Adapter List (%d entries)\n", Table->dwNumEntries));

    for( i = 0; i < Table->dwNumEntries; i++ ) {
        DH_DbgPrint(MID_TRACE,("Getting adapter %d attributes\n", i));
        Adapter = calloc( sizeof( DHCP_ADAPTER ) + Table->table[i].dwMtu, 1 );
        
        if( Adapter ) {
            memcpy( &Adapter->IfMib, &Table->table[i], 
                    sizeof(Adapter->IfMib) );
            GetAddress( Adapter );
            InsertTailList( &AdapterList, &Adapter->ListEntry );
            Adapter->DhclientInfo.next = ifi;
            Adapter->DhclientInfo.client = &Adapter->DhclientState;
            Adapter->DhclientInfo.rbuf = Adapter->recv_buf;
            Adapter->DhclientInfo.rbuf_max = Table->table[i].dwMtu;
            Adapter->DhclientInfo.rbuf_len = 
                Adapter->DhclientInfo.rbuf_offset = 0;
            Adapter->DhclientInfo.rfdesc = 
                Adapter->DhclientInfo.wfdesc =
                socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
            Adapter->ListenAddr.sin_family = AF_INET;
            Adapter->ListenAddr.sin_port = htons(LOCAL_PORT);
            Adapter->BindStatus = 
                (bind( Adapter->DhclientInfo.rfdesc,
                       (struct sockaddr *)&Adapter->ListenAddr,
                       sizeof(Adapter->ListenAddr) ) == 0) ?
                0 : WSAGetLastError();
            Adapter->DhclientState.config = &Adapter->DhclientConfig;
            Adapter->DhclientConfig.initial_interval = DHCP_DISCOVER_INTERVAL;
            Adapter->DhclientConfig.retry_interval = DHCP_DISCOVER_INTERVAL;
            Adapter->DhclientConfig.select_interval = 1;
            Adapter->DhclientConfig.reboot_timeout = DHCP_REBOOT_TIMEOUT;
            Adapter->DhclientConfig.backoff_cutoff = DHCP_BACKOFF_MAX;
            Adapter->DhclientState.interval = 
                Adapter->DhclientConfig.retry_interval;
            strncpy(Adapter->DhclientInfo.name, Adapter->IfMib.bDescr,
                    sizeof(Adapter->DhclientInfo.name));
            DH_DbgPrint(MID_TRACE,("Adapter Name: [%s] (Bind Status %x)\n", 
                                   Adapter->DhclientInfo.name,
                                   Adapter->BindStatus));
            ifi = &Adapter->DhclientInfo;
        }
    }

    DH_DbgPrint(MID_TRACE,("done with AdapterInit\n"));

term:
    if( Table ) free( Table );
}

void AdapterStop() {
    PLIST_ENTRY ListEntry;
    PDHCP_ADAPTER Adapter;
    while( !IsListEmpty( &AdapterList ) ) {
        ListEntry = (PLIST_ENTRY)RemoveHeadList( &AdapterList );
        Adapter = CONTAINING_RECORD( ListEntry, DHCP_ADAPTER, ListEntry );
        free( Adapter );
    }
    WSACleanup();
}

PDHCP_ADAPTER AdapterFindIndex( unsigned int indx ) {
    PDHCP_ADAPTER Adapter;
    PLIST_ENTRY ListEntry;

    for( ListEntry = AdapterList.Flink;
         ListEntry != &AdapterList;
         ListEntry = ListEntry->Flink ) {
        Adapter = CONTAINING_RECORD( ListEntry, DHCP_ADAPTER, ListEntry );
        if( Adapter->IfMib.dwIndex == indx ) return Adapter;
    }

    return NULL;
}

PDHCP_ADAPTER AdapterFindName( const WCHAR *name ) {
    PDHCP_ADAPTER Adapter;
    PLIST_ENTRY ListEntry;

    for( ListEntry = AdapterList.Flink;
         ListEntry != &AdapterList;
         ListEntry = ListEntry->Flink ) {
        Adapter = CONTAINING_RECORD( ListEntry, DHCP_ADAPTER, ListEntry );
        if( !wcsicmp( Adapter->IfMib.wszName, name ) ) return Adapter;
    }

    return NULL;
}

PDHCP_ADAPTER AdapterGetFirst() {
    if( IsListEmpty( &AdapterList ) ) return NULL; else {
        return CONTAINING_RECORD
            ( AdapterList.Flink, DHCP_ADAPTER, ListEntry );
    }
}

PDHCP_ADAPTER AdapterGetNext( PDHCP_ADAPTER This ) 
{ 
    if( This->ListEntry.Flink == &AdapterList ) return NULL;
    return CONTAINING_RECORD
        ( This->ListEntry.Flink, DHCP_ADAPTER, ListEntry );
}

void if_register_send(struct interface_info *ip) {
    
}

void if_register_receive(struct interface_info *ip) {
}
