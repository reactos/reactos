/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcp.h

Abstract:

    This module defines the DHCP server service definitions and structures.

Author:

    Manny Weiser  (mannyw)  11-Aug-1992

Revision History:

    Madan Appiah (madana) 10-Oct-1993

--*/

#ifndef _DHCP_
#define _DHCP_

#define WS_VERSION_REQUIRED     MAKEWORD( 1, 1)

//
// update dhcpapi.h also if you modify the following three typedefs.
//

typedef DWORD DHCP_IP_ADDRESS, *PDHCP_IP_ADDRESS, *LPDHCP_IP_ADDRESS;
typedef DWORD DHCP_OPTION_ID;

typedef struct _DATE_TIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} DATE_TIME, *LPDATE_TIME;

#define DHCP_DATE_TIME_ZERO_HIGH        0
#define DHCP_DATE_TIME_ZERO_LOW         0

#define DHCP_DATE_TIME_INFINIT_HIGH     0x7FFFFFFF
#define DHCP_DATE_TIME_INFINIT_LOW      0xFFFFFFFF

#define DOT_IP_ADDR_SIZE                16          // XXX.XXX.XXX.XXX + '\0'
#define NO_DHCP_IP_ADDRESS              ((DHCP_IP_ADDRESS)-1)
#define DHCP_IP_KEY_LEN                 32          //arbitary size.

#define INFINIT_TIME                    0x7FFFFFFF  // time_t is int
#define INFINIT_LEASE                   0xFFFFFFFF  // in secs. (unsigned int.)
#define MDHCP_SERVER_IP_ADDRESS         0x0100efef // 239.239.0.1
//
// hardware types.
//
#define HARDWARE_TYPE_NONE              0 // used for non-hardware type client id
#define HARDWARE_TYPE_10MB_EITHERNET    1
#define HARDWARE_TYPE_IEEE_802          6
#define HARDWARE_ARCNET                 7
#define HARDWARE_PPP                    8

//
// Client-server protoocol reserved ports
//

#define DHCP_CLIENT_PORT    68
#define DHCP_SERVR_PORT     67

//
// DHCP BROADCAST flag.
//

#define DHCP_BROADCAST      0x8000
#define DHCP_NO_BROADCAST   0x0000

// MDHCP flag
#define DHCP_MBIT           0x4000
#define IS_MDHCP_MESSAGE( _msg ) ( _I_ntohs((_msg)->Reserved) & DHCP_MBIT ? TRUE : FALSE )
#define MDHCP_MESSAGE( _msg ) ( (_msg)->Reserved |= htons(DHCP_MBIT) )

#define CLASSD_NET_ADDR(a)  ( (a & 0xf0) == 0xe0)
#define CLASSD_HOST_ADDR(a)  ((a & 0xf0000000) == 0xe0000000)

#define DHCP_MESSAGE_SIZE       576
#define DHCP_SEND_MESSAGE_SIZE  548
#define BOOTP_MESSAGE_SIZE      300 // the options field for bootp is 64 bytes.

//
// The amount of time to wait for a DHCP response after a request
// has been sent.
//

#if !DBG
#define WAIT_FOR_RESPONSE_TIME          5
#else
#define WAIT_FOR_RESPONSE_TIME          10
#endif

//
// DHCP Operations
//

#define BOOT_REQUEST   1
#define BOOT_REPLY     2

//
// DHCP Standard Options.
//

#define OPTION_PAD                      0
#define OPTION_SUBNET_MASK              1
#define OPTION_TIME_OFFSET              2
#define OPTION_ROUTER_ADDRESS           3
#define OPTION_TIME_SERVERS             4
#define OPTION_IEN116_NAME_SERVERS      5
#define OPTION_DOMAIN_NAME_SERVERS      6
#define OPTION_LOG_SERVERS              7
#define OPTION_COOKIE_SERVERS           8
#define OPTION_LPR_SERVERS              9
#define OPTION_IMPRESS_SERVERS          10
#define OPTION_RLP_SERVERS              11
#define OPTION_HOST_NAME                12
#define OPTION_BOOT_FILE_SIZE           13
#define OPTION_MERIT_DUMP_FILE          14
#define OPTION_DOMAIN_NAME              15
#define OPTION_SWAP_SERVER              16
#define OPTION_ROOT_DISK                17
#define OPTION_EXTENSIONS_PATH          18

//
// IP layer parameters - per host
//

#define OPTION_BE_A_ROUTER              19
#define OPTION_NON_LOCAL_SOURCE_ROUTING 20
#define OPTION_POLICY_FILTER_FOR_NLSR   21
#define OPTION_MAX_REASSEMBLY_SIZE      22
#define OPTION_DEFAULT_TTL              23
#define OPTION_PMTU_AGING_TIMEOUT       24
#define OPTION_PMTU_PLATEAU_TABLE       25

//
// Link layer parameters - per interface.
//

#define OPTION_MTU                      26
#define OPTION_ALL_SUBNETS_MTU          27
#define OPTION_BROADCAST_ADDRESS        28
#define OPTION_PERFORM_MASK_DISCOVERY   29
#define OPTION_BE_A_MASK_SUPPLIER       30
#define OPTION_PERFORM_ROUTER_DISCOVERY 31
#define OPTION_ROUTER_SOLICITATION_ADDR 32
#define OPTION_STATIC_ROUTES            33
#define OPTION_TRAILERS                 34
#define OPTION_ARP_CACHE_TIMEOUT        35
#define OPTION_ETHERNET_ENCAPSULATION   36

//
// TCP Paramters - per host
//

#define OPTION_TTL                      37
#define OPTION_KEEP_ALIVE_INTERVAL      38
#define OPTION_KEEP_ALIVE_DATA_SIZE     39

//
// Application Layer Parameters
//

#define OPTION_NETWORK_INFO_SERVICE_DOM 40
#define OPTION_NETWORK_INFO_SERVERS     41
#define OPTION_NETWORK_TIME_SERVERS     42

//
// Vender specific information option
//

#define OPTION_VENDOR_SPEC_INFO         43

//
// NetBIOS over TCP/IP Name server option
//

#define OPTION_NETBIOS_NAME_SERVER      44
#define OPTION_NETBIOS_DATAGRAM_SERVER  45
#define OPTION_NETBIOS_NODE_TYPE        46
#define OPTION_NETBIOS_SCOPE_OPTION     47

//
// X Window System Options.
//

#define OPTION_XWINDOW_FONT_SERVER      48
#define OPTION_XWINDOW_DISPLAY_MANAGER  49

//
// Other extensions
//

#define OPTION_REQUESTED_ADDRESS        50
#define OPTION_LEASE_TIME               51
#define OPTION_OK_TO_OVERLAY            52
#define OPTION_MESSAGE_TYPE             53
#define OPTION_SERVER_IDENTIFIER        54
#define OPTION_PARAMETER_REQUEST_LIST   55
#define OPTION_MESSAGE                  56
#define OPTION_MESSAGE_LENGTH           57
#define OPTION_RENEWAL_TIME             58      // T1
#define OPTION_REBIND_TIME              59      // T2
#define OPTION_CLIENT_CLASS_INFO        60
#define OPTION_CLIENT_ID                61

#define OPTION_TFTP_SERVER_NAME         66
#define OPTION_BOOTFILE_NAME            67

//
//  user class id
//
#define OPTION_USER_CLASS               77

//
//  Dynamic DNS Stuff.  Tells if we should do both A+PTR updates?
//
#define OPTION_DYNDNS_BOTH              81

// Multicast options.
#define OPTION_MCAST_SCOPE_ID           101
#define OPTION_MCAST_LEASE_START        102
#define OPTION_MCAST_TTL                103
#define OPTION_CLIENT_PORT              105
#define OPTION_MCAST_SCOPE_LIST         107

// special option to extend options
#define     OPTION_LARGE_OPTION    127

#define OPTION_WPAD_URL                 252 

#define OPTION_END                      255

// default mcast_ttl value.
#define DEFAULT_MCAST_TTL               32

//
// Different option values for the DYNDNS_BOTH option ...
//

#define DYNDNS_REGISTER_AT_CLIENT       0     // Client will do both registrations
#define DYNDNS_REGISTER_AT_SERVER       1     // Server will do registrations
#define DYNDNS_DOWNLEVEL_CLIENT         3     // arbitraty # diff from above

//
// Microsoft-specific options
//
#define OPTION_MSFT_DSDOMAINNAME_REQ    94    // send me your DS Domain name
#define OPTION_MSFT_DSDOMAINNAME_RESP   95    // sending my DS Domain name
#define OPTION_MSFT_CONTINUED           250   // the previous option is being continued..

//
// DHCP Message types
//

#define DHCP_DISCOVER_MESSAGE  1
#define DHCP_OFFER_MESSAGE     2
#define DHCP_REQUEST_MESSAGE   3
#define DHCP_DECLINE_MESSAGE   4
#define DHCP_ACK_MESSAGE       5
#define DHCP_NACK_MESSAGE      6
#define DHCP_RELEASE_MESSAGE   7
#define DHCP_INFORM_MESSAGE    8

#define DHCP_MAGIC_COOKIE_BYTE1     99
#define DHCP_MAGIC_COOKIE_BYTE2     130
#define DHCP_MAGIC_COOKIE_BYTE3     83
#define DHCP_MAGIC_COOKIE_BYTE4     99

#define BOOT_FILE_SIZE          128
#define BOOT_SERVER_SIZE        64
#define BOOT_FILE_SIZE_W        ( BOOT_FILE_SIZE * sizeof( WCHAR ))
#define BOOT_SERVER_SIZE_W      ( BOOT_SERVER_SIZE * sizeof( WCHAR ))

//
// DHCP APP names - used to indentify to the eventlogger.
//

#define DHCP_EVENT_CLIENT     TEXT("Dhcp")
#define DHCP_EVENT_SERVER     TEXT("DhcpServer")


typedef struct _OPTION {
    BYTE OptionType;
    BYTE OptionLength;
    BYTE OptionValue[1];
} OPTION, *POPTION, *LPOPTION;

//
// A DHCP message buffer
//


#pragma pack(1)         /* Assume byte packing */
typedef struct _DHCP_MESSAGE {
    BYTE Operation;
    BYTE HardwareAddressType;
    BYTE HardwareAddressLength;
    BYTE HopCount;
    DWORD TransactionID;
    WORD SecondsSinceBoot;
    WORD Reserved;
    DHCP_IP_ADDRESS ClientIpAddress;
    DHCP_IP_ADDRESS YourIpAddress;
    DHCP_IP_ADDRESS BootstrapServerAddress;
    DHCP_IP_ADDRESS RelayAgentIpAddress;
    BYTE HardwareAddress[16];
    BYTE HostName[ BOOT_SERVER_SIZE ];
    BYTE BootFileName[BOOT_FILE_SIZE];
    OPTION Option;
} DHCP_MESSAGE, *PDHCP_MESSAGE, *LPDHCP_MESSAGE;
#pragma pack()

#define DHCP_MESSAGE_FIXED_PART_SIZE \
            (sizeof(DHCP_MESSAGE) - sizeof(OPTION))

#define DHCP_MIN_SEND_RECV_PK_SIZE \
            (DHCP_MESSAGE_FIXED_PART_SIZE + 64)

//
// JET - DHCP database constants.
//

#define DB_TABLE_SIZE       10      // table size in 4K pages.
#define DB_TABLE_DENSITY    80      // page density
#define DB_LANGID           0x0409  // language id
#define DB_CP               1252    // code page

#if DBG

//
// debug functions.
//

#ifdef CHICAGO // No Tracing available on CHICAGO
#define DhcpPrintTrace
#endif

//#define IF_DEBUG(flag) if (DhcpGlobalDebugFlag & (DEBUG_ ## flag))
#define DhcpPrint(_x_) DEBUG_PRINT(UTIL,INFO,_x_)
#define Trace          DhcpPrintTrace

#ifndef CHICAGO
VOID
DhcpPrintTrace(
    IN LPSTR Format,
    ...
    );

#endif

#if DBG

/*
VOID
//extern "C"
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )
{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

    //DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}
*/
#endif // DBG


#else

//#define IF_DEBUG(flag) if (FALSE)
#define DhcpPrint(_x_)
#define Trace       (void)

#endif // DBG

#define OpenDriver     DhcpOpenDriver

#endif // _DHCP_

