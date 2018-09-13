/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpdef.h

Abstract:

    This module contains data type definitions for the DHCP client.

Author:

    Madan Appiah (madana) 31-Oct-1993

Environment:

    User Mode - Win32

Revision History:

--*/
//
// init.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//

#ifndef _DHCPDEF_
#define _DHCPDEF_

#ifdef  GLOBAL_DATA_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

//
// the registry key is of different type between NT and Memphis.
//
#ifdef VXD
typedef VMMHKEY   DHCPKEY;
#else  //  NT
typedef HKEY      DHCPKEY;
#endif


#ifndef VXD
#define RUNNING_IN_RAS_CONTEXT()     (!DhcpGlobalIsService)
#else
#define RUNNING_IN_RAS_CONTEXT()     FALSE
#endif


//
// The amount of time to wait for a retry if we have no IP address
//

#define ADDRESS_ALLOCATION_RETRY        300 //  5 minutes
#define EASYNET_ALLOCATION_RETRY        300 //  5 minutes

//
// The amount of time to wait for a retry if we have an IP address,
// but the renewal on startup failed.
//

#if !DBG
#define RENEWAL_RETRY                   600 // 10 minutes
#else
#define RENEWAL_RETRY                   60  // 1 minute
#endif

//
// The number of times to send a request before giving up waiting
// for a response.
//

#define DHCP_MAX_RETRIES                4
#define DHCP_ACCEPT_RETRIES             2
#define DHCP_MAX_RENEW_RETRIES          2


//
// amount of time required between consequtive send_informs..
//

#define DHCP_DEFAULT_INFORM_SEPARATION_INTERVAL   60 // one minute

//
// amount of time to wait after an address conflict is detected
//

#define ADDRESS_CONFLICT_RETRY          10 // 10 seconds

//
//
// Expoenential backoff delay.
//

#define DHCP_EXPO_DELAY                  4

//
// The maximum total amount of time to spend trying to obtain an
// initial address.
//
// This delay is computed as below:
//
// DHCP_MAX_RETRIES - n
// DHCP_EXPO_DELAY - m
// WAIT_FOR_RESPONSE_TIME - w
// MAX_STARTUP_DELAY - t
//
// Binary Exponential backup Algorithm.
//
// t > m * (n*(n+1)/2) + n + w*n
//     -------------------   ---
//        random wait      + response wait
//

#define MAX_STARTUP_DELAY \
    DHCP_EXPO_DELAY * \
        (( DHCP_MAX_RETRIES * (DHCP_MAX_RETRIES + 1)) / 2) + \
            DHCP_MAX_RETRIES + DHCP_MAX_RETRIES * WAIT_FOR_RESPONSE_TIME

#define MAX_RENEW_DELAY \
    DHCP_EXPO_DELAY * \
        (( DHCP_MAX_RENEW_RETRIES * (DHCP_MAX_RENEW_RETRIES + 1)) / 2) + \
            DHCP_MAX_RENEW_RETRIES + DHCP_MAX_RENEW_RETRIES * \
                WAIT_FOR_RESPONSE_TIME

//
// The maximum amount of time to wait between renewal retries, if the
// lease period is between T1 and T2.
//

#define MAX_RETRY_TIME                  3600    // 1 hour

//
// Minimum time to sleep between retries.
//

#if DBG
#define MIN_SLEEP_TIME                  1 * 60      // 1 min.
#else
#define MIN_SLEEP_TIME                  5 * 60      // 5 min.
#endif

//
// Minimum lease time.
//

#define DHCP_MINIMUM_LEASE              60*60   // 24 hours.

#ifdef __DHCP_DYNDNS_ENABLED__

#define DHCP_DNS_TTL                    0       // let the DNS api decide..

#endif


//
// IP Autoconfiguration defaults
//

#define DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET  "169.254.0.0"
#define DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK    "255.255.0.0"

// define the reserved range of autonet addresses..

#define DHCP_RESERVED_AUTOCFG_SUBNET             "169.254.255.0"
#define DHCP_RESERVED_AUTOCFG_MASK               "255.255.255.0"

// will dhcp pick any reserved autonet addr? NO!
#define DHCP_RESERVED_AUTOCFG_FLAG                (1)

// self default route (0,0,<self>) will have a metric of (3)
#define DHCP_SELF_DEFAULT_METRIC                  (3)

//
// General purpose macros
//

#define MIN(a,b)                        ((a) < (b) ? (a) : (b))
#define MAX(a,b)                        ((a) > (b) ? (a) : (b))

#if DBG
#define STATIC
#else
#define STATIC static
#endif

/*#define LOCK_RENEW_LIST()       EnterCriticalSection(&DhcpGlobalRenewListCritSect)
#define UNLOCK_RENEW_LIST()     LeaveCriticalSection(&DhcpGlobalRenewListCritSect)

#define LOCK_INTERFACE()        EnterCriticalSection(&DhcpGlobalSetInterfaceCritSect)
#define UNLOCK_INTERFACE()      LeaveCriticalSection(&DhcpGlobalSetInterfaceCritSect)

#define LOCK_OPTIONS_LIST()     EnterCriticalSection(&DhcpGlobalOptionsListCritSect)
#define UNLOCK_OPTIONS_LIST()   LeaveCriticalSection(&DhcpGlobalOptionsListCritSect)
*/
#define LOCK_RENEW_LIST()       
#define UNLOCK_RENEW_LIST()     

#define LOCK_INTERFACE()        
#define UNLOCK_INTERFACE()      

#define LOCK_OPTIONS_LIST()     
#define UNLOCK_OPTIONS_LIST()   


#define ZERO_TIME                       0x0         // in secs.

//
// length of the time string returned by ctime.
// actually it is 26.
//

#define TIME_STRING_LEN                 32

//
// String size when a long converted to printable string.
// 2^32 = 4294967295 (10 digits) + termination char.
//

#define LONG_STRING_SIZE                12

//
// A renewal function.
//

typedef
DWORD
(*PRENEWAL_FUNCTION) (
    IN PVOID Context,
    LPDWORD Sleep
    );

//
// DHCP Client-Identifier (option 61)
//
typedef struct _DHCP_CLIENT_IDENTIFIER
{
    BYTE  *pbID;
    DWORD  cbID;
    BYTE   bType;
    BOOL   fSpecified;
} DHCP_CLIENT_IDENTIFIER;


//
// state information for IP autoconfiguration
//

typedef struct _DHCP_IPAUTOCONFIGURATION_CONTEXT
{
    DHCP_IP_ADDRESS   Address;
    DHCP_IP_ADDRESS   Subnet;
    DHCP_IP_ADDRESS   Mask;
    DWORD             Seed;
} DHCP_IPAUTOCONFIGURATION_CONTEXT;

//
// A DHCP context block.  One block is maintained per NIC (network
// interface Card).
//

typedef struct _DHCP_CONTEXT {

        // list of adapters.
    //LIST_ENTRY NicListEntry;

        // hardware type.
    BYTE HardwareAddressType;
        // HW address, just follows this context structure.
    LPBYTE HardwareAddress;
        // Length of HW address.
    DWORD HardwareAddressLength;

        // Selected IpAddress, NetworkOrder.
    DHCP_IP_ADDRESS IpAddress;
        // Selected subnet mask. NetworkOrder.
    //DHCP_IP_ADDRESS SubnetMask;
        // Selected DHCP server address. Network Order.
    DHCP_IP_ADDRESS DhcpServerAddress;
        // Desired IpAddress the client request in next discover.
    //DHCP_IP_ADDRESS DesiredIpAddress;
        // The ip address that was used just before losing this..
    //DHCP_IP_ADDRESS LastKnownGoodAddress; // ONLY DNS uses this..
        // the domain name that was used with last registration..
    //WCHAR LastUsedDomainName[257]; // dns domain name is atmost 255 bytes.
        // current domain name for this adapter.
    //BYTE  DomainName[257];

        // IP Autoconfiguration state
    //DHCP_IPAUTOCONFIGURATION_CONTEXT IPAutoconfigurationContext;

    DHCP_CLIENT_IDENTIFIER ClientIdentifier;

        // Lease time in seconds.
    //DWORD Lease;
        // Time the lease was obtained.
    //time_t LeaseObtained;
        // Time the client should start renew its address.
    //time_t T1Time;
        // Time the client should start broadcast to renew address.
    time_t T2Time;
        // Time the lease expires. The clinet should stop using the
        // IpAddress.
        // LeaseObtained  < T1Time < T2Time < LeaseExpires
    //time_t LeaseExpires;
        // when was the last time an inform was sent?
    time_t LastInformSent;
        // how many seconds between consecutive informs?
    //DWORD  InformSeparationInterval;
        // # of gateways and the currently plumbed gateways are stored here
    //DWORD  nGateways;
    //DHCP_IP_ADDRESS *GatewayAddresses;

        // # of static routes and the actual static routes are stored here
    //DWORD  nStaticRoutes;
    //DHCP_IP_ADDRESS *StaticRouteAddresses;

        // to place in renewal list.
    //LIST_ENTRY RenewalListEntry;
        // Time for next renewal state.
    //time_t RunTime;

        // seconds passed since boot.
    DWORD SecondsSinceBoot;

        // should we ping the g/w or always assume g/w is NOT present?
    //BOOL  DontPingGatewayFlag;

        // can we use DHCP_INFORM packets or should we use DHCP_REQUEST instead?
    //BOOL  UseInformFlag;

    //WORD  ClientPort;

        // what to function at next renewal state.
    //PRENEWAL_FUNCTION RenewalFunction;

    	// A semaphore for synchronization to this structure
    //HANDLE RenewHandle;

        // the list of options to send and the list of options received
    LIST_ENTRY  SendOptionsList;
    LIST_ENTRY  RecdOptionsList;

        // the opened key to the adapter info storage location
    //DHCPKEY AdapterInfoKey;

        // the class this adapter belongs to
    LPBYTE ClassId;
    DWORD  ClassIdLength;

        // Message buffer to send and receive DHCP message.
    PDHCP_MESSAGE MessageBuffer;

        // state information for this interface. see below for manifests
    struct /* anonymous */ {
        unsigned Plumbed       : 1 ;    // is this interface plumbed
        unsigned ServerReached : 1 ;    // Did we reach the server ever
        unsigned AutonetEnabled: 1 ;    // Autonet enabled?
        unsigned HasBeenLooked : 1 ;    // Has this context been looked at?
        unsigned DhcpEnabled   : 1 ;    // Is this context dhcp enabled?
        unsigned AutoMode      : 1 ;    // Currently in autonet mode?
        unsigned MediaState    : 2 ;    // One of connected, disconnected, reconnected
        unsigned MDhcp         : 1 ;    // Is this context created for Mdhcp?
        unsigned PowerResumed  : 1 ;    // Was power just resumed on this interface?
        unsigned Broadcast     : 1 ;
    }   State;

	    // machine specific information
    //PVOID LocalInformation;
     
//    DWORD  IpInterfaceInstance;  // needed for BringUpInterface
    LPTSTR AdapterName;
//    LPWSTR DeviceName;
//    LPWSTR NetBTDeviceName;
//    LPWSTR RegistryKey;
    SOCKET Socket;
    DWORD  IpInterfaceContext;
//    BOOL DefaultGatewaysSet;

    CHAR szMessageBuffer[DHCP_MESSAGE_SIZE];
} DHCP_CONTEXT, *PDHCP_CONTEXT;

#define ADDRESS_PLUMBED(Ctxt)        ((Ctxt)->State.Plumbed = 1)
#define ADDRESS_UNPLUMBED(Ctxt)      ((Ctxt)->State.Plumbed = 0)
#define IS_ADDRESS_PLUMBED(Ctxt)     ((Ctxt)->State.Plumbed)
#define IS_ADDRESS_UNPLUMBED(Ctxt)   (!(Ctxt)->State.Plumbed)

#define CONNECTION_BROADCAST(Ctxt)        ((Ctxt)->State.Broadcast = 1)
#define CONNECTION_NO_BROADCAST(Ctxt)      ((Ctxt)->State.Broadcast = 0)
#define IS_CONNECTION_BROADCAST(Ctxt)     ((Ctxt)->State.Broadcast)
#define IS_CONNECTION_NOBROADCAST(Ctxt)   (!(Ctxt)->State.Broadcast)

#define SERVER_REACHED(Ctxt)         ((Ctxt)->State.ServerReached = 1)
#define SERVER_UNREACHED(Ctxt)       ((Ctxt)->State.ServerReached = 0)
#define IS_SERVER_REACHABLE(Ctxt)    ((Ctxt)->State.ServerReached)
#define IS_SERVER_UNREACHABLE(Ctxt)  (!(Ctxt)->State.ServerReached)

#define AUTONET_ENABLED(Ctxt)        ((Ctxt)->State.AutonetEnabled = 1)
#define AUTONET_DISABLED(Ctxt)       ((Ctxt)->State.AutonetEnabled = 0)
#define IS_AUTONET_ENABLED(Ctxt)     ((Ctxt)->State.AutonetEnabled)
#define IS_AUTONET_DISABLED(Ctxt)    (!(Ctxt)->State.AutonetEnabled)

#define CTXT_WAS_LOOKED(Ctxt)        ((Ctxt)->State.HasBeenLooked = 1)
#define CTXT_WAS_NOT_LOOKED(Ctxt)    ((Ctxt)->State.HasBeenLooked = 0)
#define WAS_CTXT_LOOKED(Ctxt)        ((Ctxt)->State.HasBeenLooked)
#define WAS_CTXT_NOT_LOOKED(Ctxt)    (!(Ctxt)->State.HasBeenLooked)

#define DHCP_ENABLED(Ctxt)           ((Ctxt)->State.DhcpEnabled = 1)
#define DHCP_DISABLED(Ctxt)          ((Ctxt)->State.DhcpEnabled = 0)
#define IS_DHCP_ENABLED(Ctxt)        ((Ctxt)->State.DhcpEnabled )
#define IS_DHCP_DISABLED(Ctxt)       (!(Ctxt)->State.DhcpEnabled )

#define ADDRESS_TYPE_AUTO            1
#define ADDRESS_TYPE_DHCP            0

#define ACQUIRED_DHCP_ADDRESS(Ctxt)  ((Ctxt)->State.AutoMode = 0 )
#define ACQUIRED_AUTO_ADDRESS(Ctxt)  ((Ctxt)->State.AutoMode = 1 )
#define IS_ADDRESS_DHCP(Ctxt)        (!(Ctxt)->State.AutoMode)
#define IS_ADDRESS_AUTO(Ctxt)        ((Ctxt)->State.AutoMode)

#define MEDIA_CONNECTED(Ctxt)        ((Ctxt)->State.MediaState = 0)
#define MEDIA_RECONNECTED(Ctxt)      ((Ctxt)->State.MediaState = 1)
#define MEDIA_DISCONNECTED(Ctxt)     ((Ctxt)->State.MediaState = 2)
#define IS_MEDIA_CONNECTED(Ctxt)     ((Ctxt)->State.MediaState == 0)
#define IS_MEDIA_RECONNECTED(Ctxt)   ((Ctxt)->State.MediaState == 1)
#define IS_MEDIA_DISCONNECTED(Ctxt)  ((Ctxt)->State.MediaState == 2)

#define _INIT_STATE1(Ctxt)           do{(Ctxt)->State.Plumbed = 0; (Ctxt)->State.AutonetEnabled=0;}while(0)
#define _INIT_STATE2(Ctxt)           do{(Ctxt)->State.HasBeenLooked = 0; (Ctxt)->State.DhcpEnabled=1;}while(0)
#define _INIT_STATE3(Ctxt)           do{(Ctxt)->State.AutoMode = 0; (Ctxt)->State.MediaState = 0;}while(0)
#define INIT_STATE(Ctxt)             do{_INIT_STATE1(Ctxt);_INIT_STATE2(Ctxt);_INIT_STATE3(Ctxt);}while(0)

#define MDHCP_CTX(Ctxt)           ((Ctxt)->State.MDhcp = 1)
#define NONMDHCP_CTX(Ctxt)          ((Ctxt)->State.MDhcp = 0)
#define IS_MDHCP_CTX(Ctxt)        ((Ctxt)->State.MDhcp )
#define SET_MDHCP_STATE( Ctxt ) { \
    ADDRESS_PLUMBED( Ctxt ), MDHCP_CTX( Ctxt ); \
}

#define POWER_RESUMED(Ctxt)           ((Ctxt)->State.PowerResumed = 1)
#define POWER_NOT_RESUMED(Ctxt)       ((Ctxt)->State.PowerResumed = 0)
#define IS_POWER_RESUMED(Ctxt)        ((Ctxt)->State.PowerResumed )


/*LPSTR _inline                        //  the string'ed version of state (same as Buffer)
ConvertStateToString(                //  convert from bits to string
    IN PDHCP_CONTEXT   Ctxt,         //  The context to print state for
    IN LPBYTE          Buffer        //  The input buffer to write state into
) {
    strcpy(Buffer, IS_DHCP_ENABLED(Ctxt)?"DhcpEnabled ":"DhcpDisabled ");
    strcat(Buffer, IS_AUTONET_ENABLED(Ctxt)?"AutonetEnabled ":"AutonetDisabled ");
    strcat(Buffer, IS_ADDRESS_DHCP(Ctxt)?"DhcpMode ":"AutoMode ");
    strcat(Buffer, IS_ADDRESS_PLUMBED(Ctxt)?"Plumbed ":"UnPlumbed ");
    strcat(Buffer, IS_SERVER_REACHABLE(Ctxt)?"(server-present) ":"(server-absent) ");
    strcat(Buffer, WAS_CTXT_LOOKED(Ctxt)? "(seen) ":"(not-seen) ");

    if(IS_MEDIA_CONNECTED(Ctxt) ) strcat(Buffer, "MediaConnected\n");
    else if(IS_MEDIA_RECONNECTED(Ctxt)) strcat(Buffer, "MediaReConnected\n");
    else if(IS_MEDIA_DISCONNECTED(Ctxt)) strcat(Buffer, "MediaDisConnected\n");
    else strcat(Buffer, "MediaUnknownState\n");

    strcat(Buffer, IS_MDHCP_CTX(Ctxt)? "(MDhcp) ":"");
    strcat(Buffer, IS_POWER_RESUMED(Ctxt)? "Pwr Resumed ":"");

    return Buffer;
}
*/

//
// The types of machines.. laptop would have aggressive EASYNET behaviour.
//

#define MACHINE_NONE   0
#define MACHINE_LAPTOP 1

//
//  Here is the set of expected options by the client -- If they are absent, not much can be done
//

typedef struct _DHCP_EXPECTED_OPTIONS {
    BYTE            UNALIGNED*     MessageType;
    DHCP_IP_ADDRESS UNALIGNED*     SubnetMask;
    DHCP_IP_ADDRESS UNALIGNED*     LeaseTime;
    DHCP_IP_ADDRESS UNALIGNED*     ServerIdentifier;
    BYTE            UNALIGNED*     DomainName;
    DWORD                          DomainNameSize;
    // Wpad Auto-Proxy Url
    BYTE            UNALIGNED*     WpadUrl;
    DWORD                          WpadUrlSize;
} DHCP_EXPECTED_OPTIONS, *PDHCP_EXPECTED_OPTIONS, *LPDHCP_EXPECTED_OPTIONS;

//
//  Here is the set of options understood by the client
//
typedef struct _DHCP_FULL_OPTIONS {
    BYTE            UNALIGNED*     MessageType;   // What kind of message is this?

    // Basic IP Parameters

    DHCP_IP_ADDRESS UNALIGNED*     SubnetMask;
    DHCP_IP_ADDRESS UNALIGNED*     LeaseTime;
    DHCP_IP_ADDRESS UNALIGNED*     T1Time;
    DHCP_IP_ADDRESS UNALIGNED*     T2Time;
    DHCP_IP_ADDRESS UNALIGNED*     GatewayAddresses;
    DWORD                          nGateways;
    DHCP_IP_ADDRESS UNALIGNED*     StaticRouteAddresses;
    DWORD                          nStaticRoutes;

    DHCP_IP_ADDRESS UNALIGNED*     ServerIdentifier;

    // DNS parameters

    BYTE            UNALIGNED*     DnsFlags;
    BYTE            UNALIGNED*     DnsRcode1;
    BYTE            UNALIGNED*     DnsRcode2;
    BYTE            UNALIGNED*     DomainName;
    DWORD                          DomainNameSize;
    DHCP_IP_ADDRESS UNALIGNED*     DnsServerList;
    DWORD                          nDnsServers;

    // Multicast options.
    DWORD           UNALIGNED*     MCastLeaseStartTime;
    BYTE            UNALIGNED     *MCastTTL;

    // Server message is something that the server may inform us of

    BYTE            UNALIGNED*     ServerMessage;
    DWORD                          ServerMessageLength;

    // Wpad Auto-Proxy Url
    BYTE            UNALIGNED*     WpadUrl;
    DWORD                          WpadUrlSize;

} DHCP_FULL_OPTIONS, *PDHCP_FULL_OPTIONS, *LPDHCP_FULL_OPTIONS;

typedef DHCP_FULL_OPTIONS DHCP_OPTIONS, *PDHCP_OPTIONS;

//
// structure for a list of messages
//

typedef struct _MSG_LIST {
    LIST_ENTRY     MessageListEntry;
    DWORD          ServerIdentifier;
    DWORD          MessageSize;
    DWORD          LeaseExpirationTime;
    DHCP_MESSAGE   Message;
} MSGLIST, *PMSGLIST, *LPMSGLIST;


//
// DHCP Global data.
//

extern BOOL DhcpGlobalServiceRunning;   // initialized global.

EXTERN LPSTR DhcpGlobalHostName;
EXTERN LPWSTR DhcpGlobalHostNameW;
EXTERN LPSTR DhcpGlobalHostComment;

//
// NIC List.
//

EXTERN LIST_ENTRY DhcpGlobalNICList;
EXTERN LIST_ENTRY DhcpGlobalRenewList;

//
// Synchronization variables.
//

EXTERN CRITICAL_SECTION DhcpGlobalRenewListCritSect;
EXTERN CRITICAL_SECTION DhcpGlobalSetInterfaceCritSect;
EXTERN CRITICAL_SECTION DhcpGlobalOptionsListCritSect;
EXTERN HANDLE DhcpGlobalRecomputeTimerEvent;
EXTERN HANDLE DhcpGlobalResumePowerEvent;

// waitable timer
EXTERN HANDLE DhcpGlobalWaitableTimerHandle;

//
// to display success message.
//

EXTERN BOOL DhcpGlobalProtocolFailed;

//
// This varible tells if we are going to provide the DynDns api support to external clients
// and if we are going to use the corresponding DnsApi.  The define below gives the default
// value.
//

EXTERN DWORD UseMHAsyncDns;
#define DEFAULT_USEMHASYNCDNS             1

//
// This flag tells if we need to use inform or request packets
//
EXTERN DWORD DhcpGlobalUseInformFlag;

//
// This flag tells if pinging the g/w is disabled. (in this case the g/w is always NOT present)
//
EXTERN DWORD DhcpGlobalDontPingGatewayFlag;

//
// The # of seconds before retrying according to AUTONET... default is EASYNET_ALLOCATION_RETRY
//

EXTERN DWORD AutonetRetriesSeconds;

//
// Not used on NT.  Just here for memphis.
//

EXTERN DWORD DhcpGlobalMachineType;

//
// Do we need to do a global refresh?
//

EXTERN ULONG DhcpGlobalDoRefresh;


//
// options related lists
//

EXTERN LIST_ENTRY DhcpGlobalClassesList;
EXTERN LIST_ENTRY DhcpGlobalOptionDefList;


//
// dhcpmsg.c.. list for doing parallel recv on..
//

EXTERN LIST_ENTRY DhcpGlobalRecvFromList;
EXTERN CRITICAL_SECTION DhcpGlobalRecvFromCritSect;

//
// the client vendor name ( "MSFT 5.0" or something like that )
//

EXTERN LPSTR   DhcpGlobalClientClassInfo;

//
// The following global keys are used to avoid re-opening each time
//
EXTERN DHCPKEY DhcpGlobalParametersKey;
EXTERN DHCPKEY DhcpGlobalTcpipParametersKey;
EXTERN DHCPKEY DhcpGlobalClientOptionKey;
EXTERN DHCPKEY DhcpGlobalServicesKey;

//
// debug variables.
//

#if DBG
EXTERN DWORD DhcpGlobalDebugFlag;
#endif

#endif // _DHCPDEF_


