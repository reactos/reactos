/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    protocol.cxx

Abstract:

    This module contains the server to client protocol for DHCP.

Author:

    Manny Weiser (mannyw)  21-Oct-1992

Environment:

    User Mode - Win32

Revision History:

    Madan Appiah (madana)  21-Oct-1993

    Arthur Bierer (arthurbi) 15-July-1998
        hacked up to use with Wininet's auto-proxy detection code

--*/

#include <wininetp.h>
#include "aproxp.h"

#include "apdetect.h"

#ifndef VXD
// ping routines.. ICMP
#include <ipexport.h>
//#include <icmpif.h>
#include <icmpapi.h>
#endif

#ifdef NEWNT
extern BOOL DhcpGlobalIsService;
#endif // NEWNT

DWORD                                             // Time in seconds
DhcpCalculateWaitTime(                            // how much time to wait
    IN      DWORD                  RoundNum,      // which round is this
    OUT     DWORD                 *WaitMilliSecs  // if needed the # in milli seconds
);


POPTION
FormatDhcpInform(
    PDHCP_CONTEXT DhcpContext
);

DWORD
SendDhcpInform(
    PDHCP_CONTEXT DhcpContext,
    PDWORD TransactionId
);

DWORD                                             // status
SendInformAndGetReplies(                          // send an inform packet and collect replies
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send out of
    IN      DWORD                  nInformsToSend,// how many informs to send?
    IN      DWORD                  MaxAcksToWait, // how many acks to wait for
    OUT     DHCP_EXPECTED_OPTIONS *pExpectedOptions // list of things parsed out of request
);

VOID
DhcpExtractFullOrLiteOptions(                     // Extract some important options alone or ALL
    IN      PDHCP_CONTEXT          DhcpContext,
    IN      LPBYTE                 OptStart,      // start of the options stuff
    IN      DWORD                  MessageSize,   // # of bytes of options
    IN      BOOL                   LiteOnly,      // next struc is EXPECTED_OPTIONS and not FULL_OPTIONS
    OUT     LPVOID                 DhcpOptions,   // this is where the options would be stored
    IN OUT  PLIST_ENTRY            RecdOptions,   // if !LiteOnly this gets filled with all incoming options
    IN OUT  DWORD                 *LeaseExpiry,   // if !LiteOnly input expiry time, else output expiry time
    IN      LPBYTE                 ClassName,     // if !LiteOnly this is used to add to the option above
    IN      DWORD                  ClassLen       // if !LiteOnly this gives the # of bytes of classname
);

DWORD
SendDhcpMessage(
    PDHCP_CONTEXT DhcpContext,
    DWORD MessageLength,
    PDWORD TransactionId
    );

DWORD
OpenDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
GetSpecifiedDhcpMessage(
    PDHCP_CONTEXT DhcpContext,
    PDWORD BufferLength,
    DWORD TransactionId,
    DWORD TimeToWait
    );

DWORD
CloseDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    );

//
// functions
//


DWORD                                             // Time in seconds
DhcpCalculateWaitTime(                            // how much time to wait
    IN      DWORD                  RoundNum,      // which round is this
    OUT     DWORD                 *WaitMilliSecs  // if needed the # in milli seconds
) {
    DWORD                          MilliSecs;
    //DWORD                          WaitTimes[4] = { 4000, 8000, 16000, 32000 };
    DWORD                          WaitTimes[4] = { 2000, 4000, 8000, 16000 };

    if( WaitMilliSecs ) *WaitMilliSecs = 0;
    if( RoundNum >= sizeof(WaitTimes)/sizeof(WaitTimes[0]) )
        return 0;

    MilliSecs = WaitTimes[RoundNum] - 1000 + ((rand()*((DWORD) 2000))/RAND_MAX);
    if( WaitMilliSecs ) *WaitMilliSecs = MilliSecs;

    return (MilliSecs + 501)/1000;
}


VOID        _inline
ConcatOption(
    IN OUT  LPBYTE                *Buf,           // input buffer to re-alloc
    IN OUT  ULONG                 *BufSize,       // input buffer size
    IN      BYTE UNALIGNED        *Data,          // data to append
    IN      ULONG                  DataSize       // how many bytes to add?
)
{
    LPBYTE                         NewBuf;
    ULONG                          NewSize;

    NewSize = (*BufSize) + DataSize;
    NewBuf = (LPBYTE) DhcpAllocateMemory(NewSize);
    if( NULL == NewBuf ) {                        // could not alloc memory?
        return;                                   // can't do much
    }

    memcpy(NewBuf, *Buf, *BufSize);               // copy existing part
    memcpy(NewBuf + *BufSize, Data, DataSize);    // copy new stuff

    if( NULL != *Buf ) DhcpFreeMemory(*Buf);      // if we alloc'ed mem, free it now
    *Buf = NewBuf;
    *BufSize = NewSize;                           // fill in new values..
}

VOID
DhcpExtractFullOrLiteOptions(                     // Extract some important options alone or ALL
    IN      PDHCP_CONTEXT          DhcpContext,   // input context
    IN      LPBYTE                 OptStart,      // start of the options stuff
    IN      DWORD                  MessageSize,   // # of bytes of options
    IN      BOOL                   LiteOnly,      // next struc is EXPECTED_OPTIONS and not FULL_OPTIONS
    OUT     LPVOID                 DhcpOptions,   // this is where the options would be stored
    IN OUT  PLIST_ENTRY            RecdOptions,   // if !LiteOnly this gets filled with all incoming options
    IN OUT  DWORD                 *LeaseExpiry,   // if !LiteOnly input expiry time, else output expiry time
    IN      LPBYTE                 ClassName,     // if !LiteOnly this is used to add to the option above
    IN      DWORD                  ClassLen       // if !LiteOnly this gives the # of bytes of classname
) {
    BYTE    UNALIGNED*             ThisOpt;
    BYTE    UNALIGNED*             NextOpt;
    BYTE    UNALIGNED*             EndOpt;
    BYTE    UNALIGNED*             MagicCookie;
    DWORD                          Error;
    DWORD                          Size, ThisSize, UClassSize = 0;
    LPBYTE                         UClass= NULL;  // concatenation of all OPTION_USER_CLASS options
    PDHCP_EXPECTED_OPTIONS         ExpOptions;
    PDHCP_FULL_OPTIONS             FullOptions;
    BYTE                           ReqdCookie[] = {
        (BYTE)DHCP_MAGIC_COOKIE_BYTE1,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE2,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE3,
        (BYTE)DHCP_MAGIC_COOKIE_BYTE4
    };


    EndOpt = OptStart + MessageSize;              // all options should be < EndOpt;
    ExpOptions = (PDHCP_EXPECTED_OPTIONS)DhcpOptions;
    FullOptions = (PDHCP_FULL_OPTIONS)DhcpOptions;
    RtlZeroMemory((LPBYTE)DhcpOptions, LiteOnly?sizeof(*ExpOptions):sizeof(*FullOptions));
    // if(!LiteOnly) InitializeListHead(RecdOptions); -- clear off this list for getting ALL options
    // dont clear off options... just accumulate over..

    MagicCookie = OptStart;
    if( 0 == MessageSize ) goto DropPkt;          // nothing to do in this case
    if( 0 != memcmp(MagicCookie, ReqdCookie, sizeof(ReqdCookie)) )
        goto DropPkt;                             // oops, cant handle this packet

    NextOpt = &MagicCookie[sizeof(ReqdCookie)];
    while( NextOpt < EndOpt && OPTION_END != *NextOpt ) {
        if( OPTION_PAD == *NextOpt ) {            // handle pads right away
            NextOpt++;
            continue;
        }

        ThisOpt = NextOpt;                        // take a good look at this option
        if( NextOpt + 2 >  EndOpt ) {             // goes over boundary?
            break;
        }

        NextOpt += 2 + (unsigned)ThisOpt[1];      // Option[1] holds the size of this option
        Size = ThisOpt[1];

        if( NextOpt > EndOpt ) {                  // illegal option that goes over boundary!
            break;                                // ignore the error, but dont take this option
        }

        if(!LiteOnly) do {                        // look for any OPTION_MSFT_CONTINUED ..
            if( NextOpt >= EndOpt ) break;        // no more options
            if( OPTION_MSFT_CONTINUED != NextOpt[0] ) break;
            if( NextOpt + 1 + NextOpt[1] > EndOpt ) {
                NextOpt = NULL;                   // do this so that we know to quit at the end..
                break;
            }

            NextOpt++;                            // skip opt code
            ThisSize = NextOpt[0];                // # of bytes to shift back..
            memcpy(ThisOpt+2+Size, NextOpt+1,ThisSize);
            NextOpt += ThisSize+1;
            Size += ThisSize;
        } while(1);                               // keep stringing up any "continued" options..

        if( NULL == NextOpt ) {                   // err parsing OPTION_MSFT_CONTINUED ..
            break;
        }

        if( LiteOnly ) {                          // handle the small subnet of options
            switch( ThisOpt[0] ) {                // ThisOpt[0] is OptionId, ThisOpt[1] is size
            case OPTION_MESSAGE_TYPE:
                if( ThisOpt[1] != 1 ) goto DropPkt;
                ExpOptions->MessageType = &ThisOpt[2];
                continue;
            case OPTION_SUBNET_MASK:
                if( ThisOpt[1] != sizeof(DWORD) ) goto DropPkt;
                ExpOptions->SubnetMask = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                continue;
            case OPTION_LEASE_TIME:
                if( ThisOpt[1] != sizeof(DWORD) ) goto DropPkt;
                ExpOptions->LeaseTime = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                continue;
            case OPTION_SERVER_IDENTIFIER:
                if( ThisOpt[1] != sizeof(DWORD) ) goto DropPkt;
                ExpOptions->ServerIdentifier = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                continue;
            case OPTION_DOMAIN_NAME:
                if( ThisOpt[1] == 0 ) goto DropPkt;
                ExpOptions->DomainName = (BYTE UNALIGNED *)&ThisOpt[2];
                ExpOptions->DomainNameSize = ThisOpt[1];
                break;
            case OPTION_WPAD_URL:
                if( ThisOpt[1] == 0 ) goto DropPkt;
                ExpOptions->WpadUrl = (BYTE UNALIGNED *)&ThisOpt[2];
                ExpOptions->WpadUrlSize = ThisOpt[1];
                break;

            default:
                continue;
            }
        } else {                                  // Handle the full set of options
            switch( ThisOpt[0] ) {
            case OPTION_MESSAGE_TYPE:
                if( Size != 1 ) goto DropPkt;
                FullOptions->MessageType = &ThisOpt[2];
                break;
            case OPTION_SUBNET_MASK:
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->SubnetMask = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_LEASE_TIME:
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->LeaseTime = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_SERVER_IDENTIFIER:
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->ServerIdentifier = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_RENEWAL_TIME:             // T1Time
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->T1Time = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_REBIND_TIME:              // T2Time
                if( Size != sizeof(DWORD) ) goto DropPkt;
                FullOptions->T2Time = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_ROUTER_ADDRESS:
                if( Size < sizeof(DWORD) || (Size % sizeof(DWORD) ) )
                    goto DropPkt;                 // There can be many router addresses
                FullOptions->GatewayAddresses = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                FullOptions->nGateways = Size / sizeof(DWORD);
                break;
            case OPTION_STATIC_ROUTES:
                if( Size < 2*sizeof(DWORD) || (Size % (2*sizeof(DWORD))) )
                    goto DropPkt;                 // the static routes come in pairs
                FullOptions->StaticRouteAddresses = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                FullOptions->nStaticRoutes = Size/(2*sizeof(DWORD));
                break;
            case OPTION_DYNDNS_BOTH:
                if( Size < 3 ) goto DropPkt;
                FullOptions->DnsFlags = (BYTE UNALIGNED *)&ThisOpt[2];
                FullOptions->DnsRcode1 = (BYTE UNALIGNED *)&ThisOpt[3];
                FullOptions->DnsRcode2 = (BYTE UNALIGNED *)&ThisOpt[3];
                break;
            case OPTION_DOMAIN_NAME:
                if( Size == 0 ) goto DropPkt;
                FullOptions->DomainName = (BYTE UNALIGNED *)&ThisOpt[2];
                FullOptions->DomainNameSize = Size;
                break;
            case OPTION_WPAD_URL:
                if( Size == 0 ) goto DropPkt;
                FullOptions->WpadUrl = (BYTE UNALIGNED *)&ThisOpt[2];
                FullOptions->WpadUrlSize = Size;
                break;
            case OPTION_DOMAIN_NAME_SERVERS:
                if( Size < sizeof(DWORD) || (Size % sizeof(DWORD) ))
                    goto DropPkt;
                FullOptions->DnsServerList = (DHCP_IP_ADDRESS UNALIGNED *)&ThisOpt[2];
                FullOptions->nDnsServers = Size / sizeof(DWORD);
                break;
            case OPTION_MESSAGE:
                if( Size == 0 ) break;      // ignore zero sized packets
                FullOptions->ServerMessage = &ThisOpt[2];
                FullOptions->ServerMessageLength = ThisOpt[1];
                break;
            case OPTION_MCAST_LEASE_START:
                if ( Size != sizeof(DATE_TIME) ) goto DropPkt;
                FullOptions->MCastLeaseStartTime = (DWORD UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_MCAST_TTL:
                if ( Size != 1 ) goto DropPkt;
                FullOptions->MCastTTL = (BYTE UNALIGNED *)&ThisOpt[2];
                break;
            case OPTION_USER_CLASS:
                if( Size <= 6) goto DropPkt;
                ConcatOption(&UClass, &UClassSize, &ThisOpt[2], Size);
                continue;                         // don't add this option yet...

            default:
                // unknowm message, nothing to do.. especially dont log this
                break;
            }

        } // if LiteOnly then else
    } // while NextOpt < EndOpt

    if( LiteOnly && LeaseExpiry ) {               // If asked to calculate lease expiration time..
        DWORD    LeaseTime;
        time_t   TimeNow, ExpirationTime;

        // BBUGBUGBUG [arthurbi] broken intensionlly, dead code.
        //if( ExpOptions->LeaseTime ) LeaseTime = _I_ntohl(*ExpOptions->LeaseTime);
        if( ExpOptions->LeaseTime ) LeaseTime = 0;
        else LeaseTime = DHCP_MINIMUM_LEASE;
        ExpirationTime = (TimeNow = time(NULL)) + (time_t)LeaseTime;
        if( ExpirationTime < TimeNow ) {
            ExpirationTime = INFINIT_TIME;
        }

        *LeaseExpiry = (DWORD)ExpirationTime ;
    }

    if( !LiteOnly && NULL != UClass ) {           // we have a user class list to pass on..
        DhcpAssert(UClassSize != 0 );             // we better have something here..
        DhcpFreeMemory(UClass); UClass = NULL;
    }

    return;

  DropPkt:
    RtlZeroMemory(DhcpOptions, LiteOnly?sizeof(ExpOptions):sizeof(FullOptions));
    if( LiteOnly && LeaseExpiry ) *LeaseExpiry = (DWORD) time(NULL) + DHCP_MINIMUM_LEASE;
    //if(!LiteOnly) DhcpFreeAllOptions(RecdOptions);// ok undo the options that we just added
    if(!LiteOnly && NULL != UClass ) DhcpFreeMemory(UClass);
}

POPTION                                           // ptr to add additional options
FormatDhcpInform(                                 // format the packet for an INFORM
    IN      PDHCP_CONTEXT          DhcpContext    // format for this context
) {
    LPOPTION option;
    LPBYTE OptionEnd;

    BYTE value;
    PDHCP_MESSAGE dhcpMessage;


    dhcpMessage = DhcpContext->MessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

    //
    // BUGBUG [arthurbi] - 
    // For RAS client, use broadcast bit, otherwise the router will try
    // to send as unicast to made-up RAS client hardware address, which
    // will not work.  So will this work without it?
    //

    //
    // Transaction ID is filled in during send
    //

    dhcpMessage->Operation             = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType   = DhcpContext->HardwareAddressType;
    dhcpMessage->SecondsSinceBoot      = (WORD) DhcpContext->SecondsSinceBoot;
    memcpy(dhcpMessage->HardwareAddress,DhcpContext->HardwareAddress,DhcpContext->HardwareAddressLength);
    dhcpMessage->HardwareAddressLength = (BYTE)DhcpContext->HardwareAddressLength;
    dhcpMessage->ClientIpAddress       = DhcpContext->IpAddress;
    //dhcpMessage->Reserved = 0;
    //dhcpMessage->Reserved = _I_htons(DHCP_BROADCAST);
    //if ( IS_MDHCP_CTX(DhcpContext ) ) MDHCP_MESSAGE( dhcpMessage );

    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;

    //
    // always add magic cookie first
    //

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value = DHCP_INFORM_MESSAGE;
    option = DhcpAppendOption(
        option,
        OPTION_MESSAGE_TYPE,
        &value,
        1,
        OptionEnd
    );

    //
    // BUGBUG [arthurbi], shouldn't we uncomment this?
    //

    // un comment later on
    /*option = DhcpAppendClassIdOption(
        DhcpContext,
        (LPBYTE)option,
        OptionEnd
    );*/

    return( option );
}


DWORD                                             // status
SendDhcpInform(                                   // send an inform packet after filling required options
    IN      PDHCP_CONTEXT          DhcpContext,   // sned out for this context
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    DWORD                          Error;
    POPTION                        option;
    LPBYTE                         OptionEnd;
    BYTE                           SentOpt[OPTION_END+1];
    BYTE                           SentVOpt[OPTION_END+1];
    BYTE                           VendorOpt[OPTION_END+1];
    DWORD                          VendorOptSize;

    RtlZeroMemory(SentOpt, sizeof(SentOpt));      // initialize boolean arrays
    RtlZeroMemory(SentVOpt, sizeof(SentVOpt));    // so that no option is presumed sent
    VendorOptSize = 0;                            // encapsulated vendor option is empty
    option = FormatDhcpInform( DhcpContext );     // core format

    OptionEnd = (LPBYTE)(DhcpContext->MessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    if( DhcpContext->ClientIdentifier.fSpecified) // client id specified in registy
        option = DhcpAppendClientIDOption(        // ==> use this client id as option
            option,
            DhcpContext->ClientIdentifier.bType,
            DhcpContext->ClientIdentifier.pbID,
            (BYTE)DhcpContext->ClientIdentifier.cbID,
            OptionEnd
        );
    else                                          // client id was not specified
        option = DhcpAppendClientIDOption(        // ==> use hw addr as client id
            option,
            DhcpContext->HardwareAddressType,
            DhcpContext->HardwareAddress,
            (BYTE)DhcpContext->HardwareAddressLength,
            OptionEnd
        );

    {   // add hostname and comment options
        char szHostName[255];

        if ( _I_gethostname(szHostName, ARRAY_ELEMENTS(szHostName)) != SOCKET_ERROR  ) 
        {
            option = DhcpAppendOption(
                option,
                OPTION_HOST_NAME,
                (LPBYTE)szHostName,
                (BYTE)((strlen(szHostName) + 1) * sizeof(CHAR)),
                OptionEnd
            );
        }
    }

    if( NULL != DhcpGlobalClientClassInfo ) {     // if we have any info on client class..
        option = DhcpAppendOption(
            option,
            OPTION_CLIENT_CLASS_INFO,
            (LPBYTE)DhcpGlobalClientClassInfo,
            strlen(DhcpGlobalClientClassInfo),
            OptionEnd
        );
    }

    SentOpt[OPTION_MESSAGE_TYPE] = TRUE;          // these must have been added by now
    if(DhcpContext->ClassIdLength) SentOpt[OPTION_USER_CLASS] = TRUE;
    SentOpt[OPTION_CLIENT_CLASS_INFO] = TRUE;
    SentOpt[OPTION_CLIENT_ID] = TRUE;
    SentOpt[OPTION_REQUESTED_ADDRESS] = TRUE;
    SentOpt[OPTION_HOST_NAME] = TRUE;

    option = DhcpAppendSendOptions(               // append all other options we need to send
        DhcpContext,                              // for this context
        &DhcpContext->SendOptionsList,            // this is the list of options to send out
        DhcpContext->ClassId,                     // which class.
        DhcpContext->ClassIdLength,               // how many bytes are there in the class id
        (LPBYTE)option,                           // start of the buffer to add the options
        (LPBYTE)OptionEnd,                        // end of the buffer up to which we can add options
        SentOpt,                                  // this is the boolean array that marks what opt were sent
        SentVOpt,                                 // this is for vendor spec options
        VendorOpt,                                // this would contain some vendor specific options
        &VendorOptSize                            // the # of bytes of vendor options added to VendorOpt param
    );

    if( !SentOpt[OPTION_VENDOR_SPEC_INFO] && VendorOptSize && VendorOptSize <= OPTION_END )
        option = DhcpAppendOption(                // add vendor specific options if we havent already sent it
            option,
            OPTION_VENDOR_SPEC_INFO,
            VendorOpt,
            (BYTE)VendorOptSize,
            OptionEnd
        );

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MessageBuffer);

    return  SendDhcpMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD
InitializeDhcpSocket(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function initializes and binds a socket to the specified IP address.

Arguments:

    Socket - Returns a pointer to the initialized socket.

    IpAddress - The IP address to bind the socket to.  It is legitimate
        to bind a socket to 0.0.0.0 if the card has no current IP address.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    DWORD closeError;
    DWORD value;
    struct sockaddr_in socketName;
    DWORD i;
    SOCKET sock;

    //
    // Sockets initialization
    //

    sock = _I_socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( sock == INVALID_SOCKET ) {
        error = _I_WSAGetLastError();
        DhcpPrint(("socket failed, error = %ld\n", error ));
        return( error );
    }

    //
    // Make the socket share-able
    //

    value = 1;

    error = _I_setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char FAR *)&value, sizeof(value) );
    if ( error != 0 ) {
        error = _I_WSAGetLastError();
        DhcpPrint(("setsockopt failed, err = %ld\n", error ));

        closeError = _I_closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint(("closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

    error = _I_setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char FAR *)&value, sizeof(value) );
    if ( error != 0 ) {
        error = _I_WSAGetLastError();
        DhcpPrint(("setsockopt failed, err = %ld\n", error ));

        closeError = _I_closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint(("closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

    //
    // If the IpAddress is zero, set the special socket option to make
    // stack work with zero address.
    //

    if( IpAddress == 0 ) {
        value = 1234;
        error = _I_setsockopt( sock, SOL_SOCKET, 0x8000, (char FAR *)&value, sizeof(value) );
        if ( error != 0 ) {
            error = _I_WSAGetLastError();
            DhcpPrint(("setsockopt failed, err = %ld\n", error ));

            closeError = _I_closesocket( sock );
            if ( closeError != 0 ) {
                DhcpPrint(("closesocket failed, err = %d\n", closeError ));
            }
            return( error );
        }
    }

    socketName.sin_family = PF_INET;
    socketName.sin_port = _I_htons( DHCP_CLIENT_PORT );
    socketName.sin_addr.s_addr = IpAddress;

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    //
    // Bind this socket to the DHCP server port
    //

    error = _I_bind(
               sock,
               (struct sockaddr FAR *)&socketName,
               sizeof( socketName )
               );

    if ( error != 0 ) {
        error = _I_WSAGetLastError();
        DhcpPrint(("bind failed (address 0x%lx), err = %ld\n", IpAddress, error ));
        closeError = _I_closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint(("closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

    *Socket = sock;
    return( NO_ERROR );
}


DWORD                                             // status
SendInformAndGetReplies(                          // send an inform packet and collect replies
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send out of
    IN      DWORD                  nInformsToSend,// how many informs to send?
    IN      DWORD                  MaxAcksToWait, // how many acks to wait for
    OUT     DHCP_EXPECTED_OPTIONS *pExpectedOptions // list of things parsed out of request
) {
    time_t                         StartTime;
    time_t                         TimeNow;
    DWORD                          TimeToWait;
    DWORD                          Error;
    DWORD                          Xid;
    DWORD                          MessageSize;
    DWORD                          RoundNum;
    DWORD                          MessageCount;
    DWORD                          LeaseExpirationTime;
    DHCP_FULL_OPTIONS              FullOptions;

    DhcpPrint(("SendInformAndGetReplies entered\n"));

    if((Error = OpenDhcpSocket(DhcpContext)) != ERROR_SUCCESS) {
        DhcpPrint(("Could not open socket for this interface! (%ld)\n", Error));
        return Error;
    }

    Xid                           = 0;            // Will be generated by first SendDhcpPacket
    MessageCount                  = 0;            // total # of messages we have got

    DhcpContext->SecondsSinceBoot = 0;            // start at zero..
    for( RoundNum = 0; RoundNum < nInformsToSend;  RoundNum ++ ) {
        Error = SendDhcpInform(DhcpContext, &Xid);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint(("SendDhcpInform: %ld\n", Error));
            goto Cleanup;
        } else {
            DhcpPrint(("Sent DhcpInform\n"));
        }

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        DhcpContext->SecondsSinceBoot += TimeToWait; // do this so that next time thru it can go thru relays..
        StartTime  = time(NULL);
        while ( TRUE ) {                          // wiat for the specified wait time
            MessageSize =  DHCP_MESSAGE_SIZE;

            DhcpPrint(("Waiting for ACK[Xid=%x]: %ld seconds\n",Xid, TimeToWait));
            Error = GetSpecifiedDhcpMessage(      // try to receive an ACK
                DhcpContext,
                &MessageSize,
                Xid,
                (DWORD)TimeToWait
            );
            if ( Error == ERROR_SEM_TIMEOUT ) break;
            if( Error != ERROR_SUCCESS ) {
                DhcpPrint(("GetSpecifiedDhcpMessage: %ld\n", Error));
                goto Cleanup;
            }

            DhcpExtractFullOrLiteOptions(         // Need to see if this is an ACK
                DhcpContext,
                (LPBYTE)&DhcpContext->MessageBuffer->Option,
                MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                TRUE,                             // do lite extract only
                pExpectedOptions,                 // check for only expected options
                NULL,                             // unused
                &LeaseExpirationTime,
                NULL,                             // unused
                0                                 // unused
            );

            if( NULL == pExpectedOptions->MessageType ) {
                DhcpPrint(("Received no message type!\n"));
            } else if( DHCP_ACK_MESSAGE != *(pExpectedOptions->MessageType) ) {
                DhcpPrint(("Received unexpected message type: %ld\n", *(pExpectedOptions->MessageType)));
            } else if( NULL == pExpectedOptions->ServerIdentifier ) {
                DhcpPrint(("Received no server identifier, dropping inform ACK\n"));
            } else {
                MessageCount ++;
                DhcpPrint(("Received %ld ACKS so far\n", MessageCount));
                DhcpExtractFullOrLiteOptions(     // do FULL options..
                    DhcpContext,
                    (LPBYTE)&DhcpContext->MessageBuffer->Option,
                    MessageSize - DHCP_MESSAGE_FIXED_PART_SIZE,
                    FALSE,
                    &FullOptions,
                    &(DhcpContext->RecdOptionsList),
                    &LeaseExpirationTime,
                    DhcpContext->ClassId,
                    DhcpContext->ClassIdLength
                );
                if( MessageCount >= MaxAcksToWait ) goto Cleanup;
            } // if( it is an ACK and ServerId present )

            TimeNow     = time(NULL);             // Reset the time values to reflect new time
            if( TimeToWait < (DWORD) (TimeNow - StartTime) ) {
                break;                            // no more time left to wait..
            }
            TimeToWait -= (DWORD)(TimeNow - StartTime);  // recalculate time now
            StartTime   = TimeNow;                // reset start time also
        } // end of while ( TimeToWait > 0)
    } // for (RoundNum = 0; RoundNum < nInformsToSend ; RoundNum ++ )

  Cleanup:
    CloseDhcpSocket(DhcpContext);
    if( MessageCount ) Error = ERROR_SUCCESS;
    DhcpPrint(("SendInformAndGetReplies: got %d ACKS (returning %ld)\n", MessageCount,Error));
    return Error;
}

//--------------------------------------------------------------------------------
//  This function gets the options from the server using DHCP_INFORM message.
//  It picks the first ACK and then processes it.
//  It ignores any errors caused by TIME_OUTS as that only means there is no
//  server, or the server does not have this functionality.  No point giving up
//  because of that.
//--------------------------------------------------------------------------------
BOOL                                              // win32 status
DhcpDoInform(                                     // send an inform packet if necessary
    IN      CAdapterInterface *    pAdapterInterface,
    IN      BOOL                   fBroadcast,    // Do we broadcast this inform, or unicast to server?
    OUT     LPSTR                  lpszAutoProxyUrl,
    IN      DWORD                  dwAutoProxyUrlLength
) {
    DHCP_CONTEXT                   StackDhcpContext;   // input context to do inform on
    PDHCP_CONTEXT                  DhcpContext = &StackDhcpContext;
    DWORD                          Error;
    DWORD                          LocalError;
    BOOL                           WasPlumbedBefore;
    time_t                         OldT2Time;
    DHCP_EXPECTED_OPTIONS          ExpectedOptions;

    *lpszAutoProxyUrl = '\0';
           
    if ( ! pAdapterInterface->IsDhcp() ) {
        return FALSE;
    }

    if (! pAdapterInterface->CopyAdapterInfoToDhcpContext(DhcpContext) ) {
        return FALSE;
    }
   
    // mdhcp uses INADDR_ANY so it does not have to have an ipaddress.
    if( 0 == DhcpContext->IpAddress && !IS_MDHCP_CTX( DhcpContext) ) {
        DhcpPrint(("Cannot do DhcpInform on an adapter without ip address!\n"));
        return FALSE;
    }

    // Open the socket ahead... so that things work. Tricky, else does not work!!!
    if((Error = OpenDhcpSocket(DhcpContext)) != ERROR_SUCCESS ) {
        DhcpPrint(("Could not open socket (%ld)\n", Error));
        return FALSE;
    }

    // If you always need to broadcast this message, the KLUDGE is to
    // set pContext->T2Time = 0; and pContext->fFlags &= ~DHCP_CONTEXT_FLAGS_PLUMBED
    // and that should do the trick! Safe to change the struct as it was cloned.
    OldT2Time = DhcpContext->T2Time;
    WasPlumbedBefore = IS_ADDRESS_PLUMBED(DhcpContext);
    if(fBroadcast) {
        DhcpContext->T2Time = 0; // !!!! KLUDGE.. look at SendDhcpMessage to understand this ..
        ADDRESS_UNPLUMBED(DhcpContext);
        CONNECTION_BROADCAST(DhcpContext);
    } else {
        DhcpContext->T2Time = (-1);
    }

    memset((void *) &ExpectedOptions, 0, sizeof(DHCP_EXPECTED_OPTIONS));

    Error = SendInformAndGetReplies(              // get replies on this
        DhcpContext,                              // context to send on
        2,                                        // send atmost 2 informs
        1,                                        // wait for as many as 4 packets..
        &ExpectedOptions
    );
    DhcpContext->LastInformSent = time(NULL);     // record when the last inform was sent
    DhcpContext->T2Time = OldT2Time;
    if( WasPlumbedBefore ) ADDRESS_PLUMBED(DhcpContext);

    LocalError = CloseDhcpSocket(DhcpContext);
    DhcpAssert(ERROR_SUCCESS == LocalError);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint(("DhcpDoInform:return [0x%lx]\n", Error));
    }
    else
    {
        //
        // Did we actually get a response with an URL that can be used ? 
        //

        if ( ExpectedOptions.WpadUrl && 
             ExpectedOptions.WpadUrlSize > 0 &&
             dwAutoProxyUrlLength > ExpectedOptions.WpadUrlSize )
        {
            memcpy(lpszAutoProxyUrl, ExpectedOptions.WpadUrl, ExpectedOptions.WpadUrlSize );
            return TRUE;
        }            
    }

    return FALSE;
}


DWORD
SendDhcpMessage(
    PDHCP_CONTEXT DhcpContext,
    DWORD MessageLength,
    PDWORD TransactionId
    )
/*++

Routine Description:

    This function sends a UDP message to the DHCP server specified
    in the DhcpContext.

Arguments:

    DhcpContext - A pointer to a DHCP context block.

    MessageLength - The length of the message to send.

    TransactionID - The transaction ID for this message.  If 0, the
        function generates a random ID, and returns it.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    int i;
    struct sockaddr_in socketName;
    time_t TimeNow;
    BOOL   LockedInterface = FALSE;

    if ( *TransactionId == 0 ) {
        *TransactionId = (rand() << 16) + rand();
    }

    DhcpContext->MessageBuffer->TransactionID = *TransactionId;

    //
    // Initialize the outgoing address.
    //

    socketName.sin_family = PF_INET;
    socketName.sin_port = _I_htons( DHCP_SERVR_PORT );

    if ( IS_MDHCP_CTX(DhcpContext) ) {
        socketName.sin_addr.s_addr = DhcpContext->DhcpServerAddress;
        if ( CLASSD_NET_ADDR( DhcpContext->DhcpServerAddress ) ) {
            int   TTL = 16;
            //
            // Set TTL
            // MBUG: we need to read this from the registry.
            //
            if (_I_setsockopt(
                  DhcpContext->Socket,
                  IPPROTO_IP,
                  IP_MULTICAST_TTL,
                  (char *)&TTL,
                  sizeof((int)TTL)) == SOCKET_ERROR){

                 error = _I_WSAGetLastError();
                 DhcpPrint(("could not set MCast TTL %ld\n",error ));
                 return error;
            }

        }
    } else if( IS_ADDRESS_PLUMBED(DhcpContext) &&
               !IS_MEDIA_RECONNECTED(DhcpContext) &&    // media reconnect - braodcast
               !IS_POWER_RESUMED(DhcpContext) ) {       // power resumed - broadcast

        //
        // If we are past T2, use the broadcast address; otherwise,
        // direct this to the server.
        //

        TimeNow = time( NULL );

        // BUGBUG why did we broadcast here before ?
        if ( TimeNow > DhcpContext->T2Time && IS_CONNECTION_BROADCAST(DhcpContext)) {
            socketName.sin_addr.s_addr = (DHCP_IP_ADDRESS)(INADDR_BROADCAST);
        } else {
            socketName.sin_addr.s_addr = DhcpContext->DhcpServerAddress;
        }
    }
    else {
        socketName.sin_addr.s_addr = (DHCP_IP_ADDRESS)(INADDR_BROADCAST);
        INET_ASSERT(FALSE);
    }

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    if( socketName.sin_addr.s_addr ==
            (DHCP_IP_ADDRESS)(INADDR_BROADCAST) ) {

        DWORD Error = ERROR_SUCCESS;
        DWORD InterfaceId;

        //
        // BUGBUG TODO [arthurbi] This code below is needed for 
        //  Broadcasts to work.  We need to make some fancy driver
        //  calls to work...
        //

        //
        // if we broadcast a message, inform IP stack - the adapter we
        // like to send this broadcast on, otherwise it will pick up the
        // first uninitialized adapter.
        //

//        InterfaceId = DhcpContext->IpInterfaceContext;            
//
//        if( !IPSetInterface( InterfaceId ) ) {
//            // DhcpAssert( FALSE );
//            Error = ERROR_GEN_FAILURE;
//        }

//        InterfaceId = ((PLOCAL_CONTEXT_INFO)
//            DhcpContext->LocalInformation)->IpInterfaceContext;
//
//        LOCK_INTERFACE();
//        LockedInterface = TRUE;
//        Error = IPSetInterface( InterfaceId );
        // DhcpAssert( Error == ERROR_SUCCESS );

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint(("IPSetInterface failed with %lx error\n", Error));
            UNLOCK_INTERFACE();
            return Error;
        }
    }

    //
    // send minimum DHCP_MIN_SEND_RECV_PK_SIZE (300) bytes, otherwise
    // bootp relay agents don't like the packet.
    //

    MessageLength = (MessageLength > DHCP_MIN_SEND_RECV_PK_SIZE) ?
                        MessageLength : DHCP_MIN_SEND_RECV_PK_SIZE;
    error = _I_sendto(
                DhcpContext->Socket,
                (PCHAR)DhcpContext->MessageBuffer,
                MessageLength,
                0,
                (struct sockaddr *)&socketName,
                sizeof( struct sockaddr )
                );

#ifndef VXD
    if( LockedInterface ) { UNLOCK_INTERFACE(); }
#endif  VXD

    if ( error == SOCKET_ERROR ) {
        error = _I_WSAGetLastError();
        DhcpPrint(("Send failed, error = %ld\n", error ));
    } else {
        IF_DEBUG( PROTOCOL ) {
            DhcpPrint(("Sent message to %s: \n", _I_inet_ntoa( socketName.sin_addr )));
        }

        DhcpDumpMessage( DEBUG_PROTOCOL_DUMP, DhcpContext->MessageBuffer );
        error = NO_ERROR;
    }

    return( error );
}

DWORD
OpenDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    )
{

    DWORD Error;
    PLOCAL_CONTEXT_INFO localInfo;

    if ( DhcpContext->Socket != INVALID_SOCKET ) {
        return ( ERROR_SUCCESS );
    }

    //
    // create a socket for the dhcp protocol.  it's important to bind the
    // socket to the correct ip address.  There are currently three cases:
    //
    // 1.  If the interface has been autoconfigured, it already has an address,
    //     say, IP1.  If the client receives a unicast offer from a dhcp server
    //     the offer will be addressed to IP2, which is the client's new dhcp
    //     address.  If we bind the dhcp socket to IP1, the client won't be able
    //     to receive unicast responses.  So, we bind the socket to 0.0.0.0.
    //     This will allow the socket to receive a unicast datagram addressed to
    //     any address.
    //
    // 2.  If the interface in not plumbed (i.e. doesn't have an address) bind
    //     the socket to 0.0.0.0
    //
    // 3.  If the interface has been plumbed has in *not* autoconfigured, then
    //     bind to the current address.


    Error =  InitializeDhcpSocket(
                 &DhcpContext->Socket,
                 DhcpContext->IpAddress 
                 );

    if( Error != ERROR_SUCCESS ) {
        DhcpContext->Socket = INVALID_SOCKET;
        DhcpPrint((" Socket Open failed, %ld\n", Error ));
    }

    return(Error);
}

DWORD
CloseDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    )
{

    DWORD Error = ERROR_SUCCESS;

    if( DhcpContext->Socket != INVALID_SOCKET ) {

        BOOL Bool;

        Error = _I_closesocket( DhcpContext->Socket );

        if( Error != ERROR_SUCCESS ) {
            DhcpPrint((" Socket close failed, %ld\n", Error ));
        }

        DhcpContext->Socket = INVALID_SOCKET;

        //
        // Reset the IP stack to send DHCP broadcast to first
        // uninitialized stack.
        //

        //Bool = IPResetInterface();
        //DhcpAssert( Bool == TRUE );
    }

    return( Error );
}


typedef     struct  /* anonymous */ {             // structure to hold waiting recvfroms
    LIST_ENTRY                     RecvList;      // other elements in this list
    PDHCP_CONTEXT                  Ctxt;          // which context is this wait for?
    DWORD                          InBufLen;      // what was the buffer size to recv in?
    PDWORD                         BufLen;        // how many bytes did we recvd?
    DWORD                          Xid;           // what xid is this wait for?
    time_t                         ExpTime;       // wait until what time?
    HANDLE                         WaitEvent;     // event for waiting on..
    BOOL                           Recd;          // was a packet received..?
} RECV_CTXT, *PRECV_CTXT;                         // ctxt used to recv on..

VOID
InsertInPriorityList(                             // insert in priority list according to Secs
    IN OUT  PRECV_CTXT             Ctxt,          // Secs field changed to hold offset
    IN      PLIST_ENTRY            List,
    OUT     PBOOL                  First          // adding in first location?
)
{
    PRECV_CTXT                     ThisCtxt;
    PLIST_ENTRY                    InitList;      // "List" param at function entry


    if( IsListEmpty(List) ) {                     // no element in list? add this and quit
        *First = TRUE;                            // adding at head
    } else {
        *First = FALSE;                           // adding at tail..
    }

    InsertTailList( List, &Ctxt->RecvList);       // insert element..
    //LeaveCriticalSection( &DhcpGlobalRecvFromCritSect );
}

DWORD
TryReceive(                                       // try to recv pkt on 0.0.0.0 socket
    IN      SOCKET                 Socket,        // socket to recv on
    IN      LPBYTE                 Buffer,        // buffer to fill
    OUT     PDWORD                 BufLen,        // # of bytes filled in buffer
    OUT     PDWORD                 Xid,           // Xid of recd pkt
    IN      DWORD                  Secs           // # of secs to spend waiting?
)
{
    DWORD                          Error;
    struct timeval                 timeout;
    fd_set                         SockSet;
    struct sockaddr                SockName;
    int                            SockNameSize;

    FD_ZERO(&SockSet);
    FD_SET(Socket,&SockSet);

    SockNameSize = sizeof( SockName );

    timeout.tv_sec = Secs;
    timeout.tv_usec = 0;

    DhcpPrint(("Select: waiting for: %ld seconds\n", Secs));
    Error = _I_select( 0, &SockSet, NULL, NULL, &timeout );
    if( ERROR_SUCCESS == Error ) {            // timed out..
        DhcpPrint(("Recv timed out..\n"));
        return ERROR_SEM_TIMEOUT;
    }

    Error = _I_recvfrom(Socket,(char *)Buffer,*BufLen, 0, &SockName, &SockNameSize);
    if( SOCKET_ERROR == Error ) {
        Error = _I_WSAGetLastError();
        DhcpPrint(("Recv failed 0x%lx\n",Error));
    } else {
        *BufLen = Error;
        Error = ERROR_SUCCESS;
        *Xid = ((PDHCP_MESSAGE)Buffer)->TransactionID;
        DhcpPrint(("Recd msg XID: 0x%lx [Mdhcp? %s]\n", *Xid,
                   IS_MDHCP_MESSAGE(((PDHCP_MESSAGE)Buffer))?"yes":"no" ));

    }

    return Error;
}

VOID
DispatchPkt(                                      // find out any takers for Xid
    IN OUT  PRECV_CTXT             Ctxt,          // ctxt that has buffer and buflen
    IN      DWORD                  Xid            // recd Xid
)
{
    do {                                          // not a loop, just for ease of use
        LPBYTE                     Tmp;
        PLIST_ENTRY                Entry;
        PRECV_CTXT                 ThisCtxt;

        Entry = DhcpGlobalRecvFromList.Flink;
        while(Entry != &DhcpGlobalRecvFromList ) {
            ThisCtxt = CONTAINING_RECORD(Entry, RECV_CTXT, RecvList);
            Entry = Entry->Flink;

            if(Xid != ThisCtxt->Xid ) continue;   // mismatch.. nothing more todo

            // now check for same type of message and ctxt...
            if( (unsigned)IS_MDHCP_MESSAGE((Ctxt->Ctxt->MessageBuffer))
                !=
                IS_MDHCP_CTX( (ThisCtxt->Ctxt) )
            ) {
                //
                // The contexts dont match.. give up
                //
                continue;
            }

            //
            // check for same hardware address..
            //

            if( ThisCtxt->Ctxt->HardwareAddressLength != Ctxt->Ctxt->MessageBuffer->HardwareAddressLength ) {
                continue;
            }

            if( 0 != memcmp(ThisCtxt->Ctxt->HardwareAddress,
                            Ctxt->Ctxt->MessageBuffer->HardwareAddress,
                            ThisCtxt->Ctxt->HardwareAddressLength
            ) ) {
                continue;
            }

            // matched.. switch buffers to give this guy this due..

            DhcpDumpMessage(DEBUG_PROTOCOL_DUMP, (PDHCP_MESSAGE)(Ctxt->Ctxt->MessageBuffer) );

            *(ThisCtxt->BufLen) = *(Ctxt->BufLen);
            Tmp = (LPBYTE)(Ctxt->Ctxt)->MessageBuffer;
            (Ctxt->Ctxt)->MessageBuffer = (ThisCtxt->Ctxt)->MessageBuffer;
            (ThisCtxt->Ctxt)->MessageBuffer = (PDHCP_MESSAGE)Tmp;

            RemoveEntryList(&ThisCtxt->RecvList);
            InitializeListHead(&ThisCtxt->RecvList);
            DhcpAssert(FALSE == ThisCtxt->Recd);
            ThisCtxt->Recd = TRUE;
            if( 0 == SetEvent(ThisCtxt->WaitEvent) ) {
                DhcpAssert(FALSE);
            }

            break;
        }
    } while (FALSE);
    //LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);
}

DWORD
ProcessRecvFromSocket(                            // wait using select and process incoming pkts
    IN OUT  PRECV_CTXT             Ctxt           // ctxt to use
)
{
    time_t                         TimeNow;
    SOCKET                         Socket;
    LPBYTE                         Buffer;
    DWORD                          Xid;
    DWORD                          Error;
    PLIST_ENTRY                    Entry;

    Socket = (Ctxt->Ctxt)->Socket;
    TimeNow = time(NULL);

    Error = ERROR_SEM_TIMEOUT;
    while(TimeNow <= Ctxt->ExpTime ) {            // while required to wait
        Buffer = (LPBYTE)((Ctxt->Ctxt)->MessageBuffer);
        *(Ctxt->BufLen) = Ctxt->InBufLen;
        Error = TryReceive(Socket, Buffer, Ctxt->BufLen, &Xid, (DWORD)(Ctxt->ExpTime - TimeNow));
        if( ERROR_SUCCESS != Error ) {            // did not recv?
            if( WSAECONNRESET != Error ) break;   // ignore possibly spurious conn-resets..
            else {  TimeNow = time(NULL); continue; }
        }

        if( Xid == Ctxt->Xid ) break;             // this was destined for this ctxt only..

        DispatchPkt(Ctxt, Xid);
        TimeNow = time(NULL);
    }

    if( TimeNow > Ctxt->ExpTime ) {               // we timed out.
        Error = ERROR_SEM_TIMEOUT;
    }

    // now done.. so we must remove this ctxt from the list and signal first guy
    //EnterCriticalSection(&DhcpGlobalRecvFromCritSect);
    RemoveEntryList(&Ctxt->RecvList);
    CloseHandle(Ctxt->WaitEvent);
    if( !IsListEmpty(&DhcpGlobalRecvFromList)) {  // ok got an elt.. signal this.
        Entry = DhcpGlobalRecvFromList.Flink;
        Ctxt = CONTAINING_RECORD(Entry, RECV_CTXT, RecvList);
        if( 0 == SetEvent(Ctxt->WaitEvent) ) {
            DhcpAssert(FALSE);
        }
    }
    //LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);

    return Error;
}

//================================================================================
//  get dhcp message with requested transaction id, but also make sure only one
//  socket is used at any given time (one socket bound to 0.0.0.0), and also
//  re-distribute message for some other thread if that is also required..
//================================================================================
DWORD
GetSpecifiedDhcpMessageEx(
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // which context to recv for
    OUT     PDWORD                 BufferLength,  // how big a buffer was read?
    IN      DWORD                  Xid,           // which xid to look for?
    IN      DWORD                  TimeToWait     // how many seconds to sleep?
)
{
    RECV_CTXT                      Ctxt;          // element in list for this call to getspe..
    BOOL                           First;         // is this the first element in list?
    DWORD                          Result;

    Ctxt.Ctxt = DhcpContext;                      // fill in the context
    Ctxt.InBufLen = *BufferLength;
    Ctxt.BufLen = BufferLength;
    Ctxt.Xid = Xid;
    Ctxt.ExpTime = time(NULL) + TimeToWait;
    Ctxt.WaitEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    Ctxt.Recd = FALSE;
    if( NULL == Ctxt.WaitEvent ) {
        DhcpAssert(NULL != Ctxt.WaitEvent);
        return GetLastError();
    }

    First = FALSE;
    InsertInPriorityList(&Ctxt, &DhcpGlobalRecvFromList, &First);

    if( First ) {                                 // this *is* the first call to GetSpec..
        Result = ProcessRecvFromSocket(&Ctxt);
    } else {                                      // we wait for other calls to go thru..
        Result = WaitForSingleObject(Ctxt.WaitEvent, TimeToWait * 1000);
        //EnterCriticalSection(&DhcpGlobalRecvFromCritSect);
        if( Ctxt.Recd || WAIT_FAILED == Result || WAIT_TIMEOUT == Result ) {
            if( WAIT_FAILED == Result ) Result = GetLastError();
            else if (WAIT_TIMEOUT == Result ) Result = ERROR_SEM_TIMEOUT;
            else Result = ERROR_SUCCESS;

            RemoveEntryList(&Ctxt.RecvList);      // remove it from list
            //LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);
            CloseHandle(Ctxt.WaitEvent);
            return Result;
        } else {
            DhcpAssert(WAIT_OBJECT_0 == Result && Ctxt.Recd == FALSE );
            // have not received a packet but have been woken up? must be first in line now..
            //LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);
            Result = ProcessRecvFromSocket(&Ctxt);
        }
    }

    return Result;
}


#define RATIO 1
DWORD
GetSpecifiedDhcpMessage(
    PDHCP_CONTEXT DhcpContext,
    PDWORD BufferLength,
    DWORD TransactionId,
    DWORD TimeToWait
    )
/*++

Routine Description:

    This function waits TimeToWait seconds to receives the specified
    DHCP response.

Arguments:

    DhcpContext - A pointer to a DHCP context block.

    BufferLength - Returns the size of the input buffer.

    TransactionID - A filter.  Wait for a message with this TID.

    TimeToWait - Time, in milli seconds, to wait for the message.

Return Value:

    The status of the operation.  If the specified message has been
    been returned, the status is ERROR_TIMEOUT.

--*/
{
    struct sockaddr socketName;
    int socketNameSize = sizeof( socketName );
    struct timeval timeout;
    time_t startTime, now;
    DWORD error;
    DWORD actualTimeToWait;
    SOCKET clientSocket;
    fd_set readSocketSet;

    if( !IS_ADDRESS_PLUMBED(DhcpContext) ) {
        //
        // For RAS server Lease API this call won't happen as we don't have to do this nonsense
        //
        error = GetSpecifiedDhcpMessageEx(
            DhcpContext,
            BufferLength,
            TransactionId,
            TimeToWait
        );
	if( ERROR_SUCCESS == error ) {
	    // received a message frm the dhcp server..
            SERVER_REACHED(DhcpContext);
	}
	return error;
    }

    startTime = time( NULL );
    actualTimeToWait = TimeToWait;

    //
    // Setup the file descriptor set for select.
    //

    clientSocket = DhcpContext->Socket;

    FD_ZERO( &readSocketSet );
    FD_SET( clientSocket, &readSocketSet );

    while ( 1 ) {

        timeout.tv_sec  = actualTimeToWait / RATIO;
        timeout.tv_usec = actualTimeToWait % RATIO;
        DhcpPrint(("Select: waiting for: %ld seconds\n", actualTimeToWait));
        error = _I_select( 0, &readSocketSet, NULL, NULL, &timeout );

        if ( error == 0 ) {

            //
            // Timeout before read data is available.
            //

            DhcpPrint(("Recv timed out\n", 0 ));
            error = ERROR_SEM_TIMEOUT;
            break;
        }

        error = _I_recvfrom(
                    clientSocket,
                    (PCHAR)DhcpContext->MessageBuffer,
                    *BufferLength,
                    0,
                    &socketName,
                    &socketNameSize
                    );

        if ( error == SOCKET_ERROR ) {
            error = _I_WSAGetLastError();
            DhcpPrint(("Recv failed, error = %ld\n", error ));

            if( WSAECONNRESET != error ) break;

            //
            // ignore connreset -- this could be caused by someone sending random ICMP port unreachable.
            //

        } else if (DhcpContext->MessageBuffer->TransactionID == TransactionId ) {
             
            DhcpPrint((  "Received Message, XID = %lx, MDhcp = %d.\n",
                            TransactionId,
                            IS_MDHCP_MESSAGE( DhcpContext->MessageBuffer) ));

            if (((unsigned)IS_MDHCP_MESSAGE( DhcpContext->MessageBuffer) == IS_MDHCP_CTX( DhcpContext))) {
                DhcpDumpMessage(DEBUG_PROTOCOL_DUMP, DhcpContext->MessageBuffer );

                *BufferLength = error;
                error = NO_ERROR;

                if( DhcpContext->MessageBuffer->HardwareAddressLength == DhcpContext->HardwareAddressLength
                    && 0 == memcmp(DhcpContext->MessageBuffer->HardwareAddress,
                                   DhcpContext->HardwareAddress,
                                   DhcpContext->HardwareAddressLength
                    )) {

                    //
                    // Transction IDs match, same type (MDHCP/DHCP), Hardware addresses match!
                    //

                    break;
                }
            }
        } else {            
            DhcpPrint(( "Received a buffer with unknown XID = %lx\n",
                         DhcpContext->MessageBuffer->TransactionID ));
        }

        //
        // We received a message, but not the one we're interested in.
        // Reset the timeout to reflect elapsed time, and wait for
        // another message.
        //
        now = time( NULL );
        actualTimeToWait = (DWORD)(TimeToWait - RATIO * (now - startTime));
        if ( (LONG)actualTimeToWait < 0 ) {
            error = ERROR_SEM_TIMEOUT;
            break;
        }
    }

    if ( ERROR_SEM_TIMEOUT != error )
    {
        //
        // a message was received from a DHCP server.  disable IP autoconfiguration.
        //

        SERVER_REACHED(DhcpContext);
    }

    return( error );
}


DWORD 
QueryWellKnownDnsName(
    IN OUT LPSTR lpszAutoProxyUrl,
    IN     DWORD dwAutoProxyUrlLength
    )
/*++

Routine Description:

    This function walks a list of standard DNS names trying to find
     an entry for "wpad.some-domain-here.org"  If it does, it constructs
     an URL that is suitable for use in auto-proxy.

Arguments:

    lpszAutoProxyUrl - Url used to return a successful auto-proxy discover

    dwAutoProxyUrlLength - length of buffer passed in above

Return Value:
        
    ERROR_SUCCESS - if we found a URL/DNS name

    ERROR_NOT_FOUND - on error

revised: joshco 7-oct-1998
  if we dont get a valid domain back, be sure and try
  the netbios name ("wpad") no trailing dot.

  revised: joshco 7-oct-1998
  	use the define PROXY_AUTO_DETECT_PATH instead
  	of hardcoding "wpad.dat"
  	
--*/

{
#define WORK_BUFFER_SIZE 356

    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "QueryWellKnownDnsName",
                 "%x, %u",
                 lpszAutoProxyUrl,
                 dwAutoProxyUrlLength                 
                 ));

    char szHostDomain[WORK_BUFFER_SIZE];
    char * pszTemp = szHostDomain ;
    char *pszDot1 = NULL;
    char *pszDot2 = NULL;
    DWORD error = ERROR_NOT_FOUND;
    DWORD dwMinDomain = 2;  //  By default, assume domain is of the form: .domain-name.org

    lstrcpy(szHostDomain, "wpad.");
    pszTemp += (sizeof("wpad.") - 1);

    if ( SockGetSingleValue(CONFIG_DOMAIN,
                            (LPBYTE)pszTemp,
                            WORK_BUFFER_SIZE - sizeof("wpad.")
                            ) != ERROR_SUCCESS )
    {
        lstrcpy(szHostDomain, "wpad.");
        pszTemp = szHostDomain ;
        pszTemp += (sizeof("wpad.") - 1);
    }

    if ( (GetProxyDetectType() & PROXY_AUTO_DETECT_TYPE_NO_DOMAIN ) ||
         *pszTemp == '\0' ) 
    {
        // if the debug setting for no domain (netbios) or 
        // we didnt get back a valid domain, then just do the
        // netbios name.  
        // XXBUG sockgetsinglevalue returns true even if there is no domain

        INET_ASSERT(*(pszTemp  - 1 ) == '.');

        *(pszTemp - 1) = '\0';
    }

    // Now determine which form the domain name follows:
    //     domain-name.org
    //     domain-name.co.uk
    pszDot1 = &szHostDomain[lstrlen(szHostDomain)-1];

    while (pszDot1 >= szHostDomain && *pszDot1 != '.')
        pszDot1--;

    // Only check .?? endings
    if (pszDot1 >= szHostDomain && (pszDot1 + 3 == &szHostDomain[lstrlen(szHostDomain)]) )
    {
        pszDot2 = pszDot1 - 1;

        while (pszDot2 >= szHostDomain && *pszDot2 != '.')
           pszDot2--;
   
        if (pszDot2 >= szHostDomain && pszDot2 + 3 >= pszDot1)
        {
           // Domain ended in something of the form: .co.uk
           // This requires at least 3 pieces then to be considered a domain
           dwMinDomain = 3;
        }
        else if ((pszDot2 + 4) == pszDot1)
        {
            // Check domain endings of the form ending in .com.uk
            // These special 3-letter pieces also need 3 dots to be classified
            // as a domain.  Unfortunately, we can't leverage the equivalent
            // code used by cookies because there, the strings are reversed.
            static const char *s_pachSpecialDomains[] = {"COM", "EDU", "NET", "ORG", "GOV", "MIL", "INT" };

            for (int i=0; i < ARRAY_ELEMENTS(s_pachSpecialDomains); i++)
            {
                if (StrCmpNIC(pszDot2+1, s_pachSpecialDomains[i], 3) == 0)
                {
                    dwMinDomain = 3;
                    break;
                }
            }
        }
    }

    while (TRUE)
    {
        PHOSTENT lpHostent = _I_gethostbyname(szHostDomain);

        if ( lpHostent != NULL )
        {
            //
            // Found a host, extract the IP address and form an URL to use.
            //

            char *pszAddressStr;
            LPBYTE * addressList;
            struct  in_addr sin_addr;
            DWORD dwIPAddressSize;

            addressList         = (LPBYTE *)lpHostent->h_addr_list;
            *(LPDWORD)&sin_addr = *(LPDWORD)addressList[0] ;

            pszAddressStr = _I_inet_ntoa (sin_addr);

            INET_ASSERT(pszAddressStr);

            dwIPAddressSize = lstrlen(pszAddressStr);

            if ( dwAutoProxyUrlLength < (dwIPAddressSize + 
              sizeof("http:///") + sizeof(PROXY_AUTO_DETECT_PATH) )  )
            {
                error = ERROR_INSUFFICIENT_BUFFER;
                goto quit;
            }

            wsprintf(lpszAutoProxyUrl, "http://%s/%s", pszAddressStr, PROXY_AUTO_DETECT_PATH );
            error = ERROR_SUCCESS;
            goto quit;
        }
        else
        {
            //
            // Did not find anything yet, reduce the domain level, 
            //  and if we're in the root domain stop and return error
            //

            DWORD dwPeriodCnt = 0, dwNewEndLength = 0;
            LPSTR lpszPeriod1 = NULL, lpszPeriod2 = NULL;

            for (pszTemp = szHostDomain; *pszTemp; pszTemp++ )
            {
                if ( *pszTemp == '.' ) {
                    dwPeriodCnt ++;
                    if ( lpszPeriod1 == NULL ) {
                        lpszPeriod1 = pszTemp;
                    }
                    else if ( lpszPeriod2 == NULL ) {
                        lpszPeriod2 = pszTemp;
                    }
                }
            }

            if ( dwPeriodCnt <= dwMinDomain) 
            {
                error = ERROR_NOT_FOUND;
                goto quit;
            }

            dwNewEndLength = lstrlen(lpszPeriod2);
            MoveMemory(lpszPeriod1, lpszPeriod2, dwNewEndLength);
            *(lpszPeriod1 + dwNewEndLength) = '\0';
        }
    }
quit:

    DEBUG_LEAVE(error);

    return error;    
}


//================================================================================
// End of file
//================================================================================


