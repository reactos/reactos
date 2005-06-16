/* 
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS traceroute utility
 * FILE:        apps/utils/net/tracert/tracert.c
 * PURPOSE:     trace a packets route through a network
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *   GM 03/05/05 Created
 *
 */


#include <windows.h>
#include <winsock2.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <string.h>
#include <time.h>
#include "tracert.h"

#define WIN32_LEAN_AND_MEAN

#ifdef DBG
#undef DBG
#endif

/*
 * globals
 */
SOCKET icmpSock;                // socket descriptor 
SOCKADDR_IN source, dest;       // source and destination address info
ECHO_REPLY_HEADER sendpacket;   // ICMP echo packet
IPv4_HEADER recvpacket;         // return reveive packet

BOOL bUsePerformanceCounter;    // whether to use the high res performance counter
LARGE_INTEGER TicksPerMs;       // number of millisecs in relation to proc freq
LARGE_INTEGER TicksPerUs;       // number of microsecs in relation to proc freq
LONGLONG lTimeStart;            // send packet, timer start
LONGLONG lTimeEnd;	    	    // receive packet, timer end

CHAR cHostname[256];            // target hostname
CHAR cDestIP[18];               // target IP


/*
 * command line options
 */
BOOL bResolveAddresses = TRUE;  // -d  MS ping defaults to true.
INT iMaxHops = 30;              // -h  Max number of hops before trace ends
INT iHostList;                  // -j  @UNIMPLEMENTED@
INT iTimeOut = 2000;            // -w  time before packet times out




/* 
 *
 * Parse command line parameters and set any options
 *
 */
BOOL ParseCmdline(int argc, char* argv[])
{
    int i;
 
    if (argc < 2) 
    {
       Usage();
       return FALSE;
    }

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
               case 'd': bResolveAddresses = FALSE;  
                         break;
               case 'h': sscanf(argv[i+1], "%d", &iMaxHops); 
                         break;
               case 'l': break; /* @unimplemented@ */
               case 'w': sscanf(argv[i+1], "%d", &iTimeOut); 
                         break;
               default:
                  _tprintf(_T("%s is not a valid option.\n"), argv[i]);
                  Usage();
                  return FALSE;
            }
        } else {
           /* copy target address */
           strncpy(cHostname, argv[i], 255);

        }
    }
 
    return TRUE;
}



/*
 *
 * Driver function, controls the traceroute program
 * 
 */
INT Driver(VOID) {
    
    INT i;
    INT iHopCount = 1;              // hop counter. default max is 30
    INT iSeqNum = 0;                // initialise packet sequence number
    INT iTTL = 1;                   // set initial packet TTL to 1
    BOOL bFoundTarget = FALSE;      // Have we reached our destination yet
    BOOL bAwaitPacket;              // indicates whether we have recieved a good packet
    INT iDecRes;                    // DecodeResponse return value
    INT iRecieveReturn;             // RecieveReturn return value
    INT iNameInfoRet;               // getnameinfo return value
    INT iPacketSize = PACKET_SIZE;  // packet size
    WORD wHeaderLen;                // header length
    PECHO_REPLY_HEADER icmphdr;     
    
    
    //temps for getting host name
    CHAR cHost[256];
    CHAR cServ[256];
    CHAR *ip;
    
    /* setup winsock */
    WSADATA wsaData;

    /* check for winsock 2 */
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
#ifdef DBG
        _tprintf(_T("WSAStartup failed.\n"));
#endif /* DBG */
        exit(1);
    }
    
    SetupTimingMethod();
    
    /* setup target info */
    ResolveHostname();

    /* print standard tracing info to screen */
    _tprintf(_T("\nTracing route to %s [%s]\n"), cHostname, cDestIP);
    _tprintf(_T("over a maximum of %d hop"), iMaxHops);
    iMaxHops > 1 ? _tprintf(_T("s:\n\n")) : _tprintf(_T(":\n\n"));

    /* run until we hit either max hops, or we recieve 3 echo replys */
    while ((iHopCount <= iMaxHops) && (bFoundTarget != TRUE)) {
        _tprintf(_T("%3d   "), iHopCount);
        /* run 3 pings for each hop */
        for (i=0; i<3; i++) {
            if (Setup(iTTL) != TRUE) {
#ifdef DBG
                _tprintf(_T("error in Setup()\n"));
#endif /* DBG */
                WSACleanup();
                exit(1);
            }
            PreparePacket(iPacketSize, iSeqNum);
            if (SendPacket(iPacketSize) != SOCKET_ERROR) {
                /* loop until we get a good packet */
                bAwaitPacket = TRUE;
                while (bAwaitPacket) {
                    /* Receive replies until we either get a successful
                     * read, or a fatal error occurs. */
                    if ((iRecieveReturn = ReceivePacket(iPacketSize)) < 0) {
                        /* check the sequence number in the packet
                         * if it's bad, complain and wait for another packet
                         * , otherwise break */
                        wHeaderLen = recvpacket.h_len * 4;
                        icmphdr = (ECHO_REPLY_HEADER *)((char*)&recvpacket + wHeaderLen);
                        if (icmphdr->icmpheader.seq != iSeqNum) {
                            _tprintf(_T("bad sequence number!\n"));
                            continue;
                        } else {
                            break;
                        }
                    }
                    
                    /* if RecievePacket timed out we don't bother decoding */
                    if (iRecieveReturn != 1) {
                        iDecRes = DecodeResponse(iPacketSize, iSeqNum);
   
                        switch (iDecRes) {
                           case 0 : bAwaitPacket = FALSE;  /* time exceeded */
                                    break;
                           case 1 : bAwaitPacket = FALSE;  /* echo reply */
                                    break; 
                           case 2 : bAwaitPacket = FALSE;  /* destination unreachable */
                                    break;  
#ifdef DBG 
                           case -1 :
                                     _tprintf(_T("recieved foreign packet\n")); 
                                     break;
                           case -2 : 
                                     _tprintf(_T("error in DecodeResponse\n")); 
                                     break;
                           case -3 : 
                                     _tprintf(_T("unknown ICMP packet\n")); 
                                     break;
#endif /* DBG */
                           default : break;
                        }
                    } else {
                        /* packet timed out. Don't wait for it again */
                        bAwaitPacket = FALSE;
                    }
                }   
            }

            iSeqNum++;
            _tprintf(_T("   "));
        }

        if(bResolveAddresses) {
           /* gethostbyaddr() and getnameinfo() are 
            * unimplemented in ROS at present.
            * Alex has advised he will be implementing gethostbyaddr
            * but as it's depricieted and getnameinfo is much nicer,
            * I've used that for the time being for testing in Windows*/
            
              //ip = inet_addr(inet_ntoa(source.sin_addr));
              //host = gethostbyaddr((char *)&ip, 4, 0);
              
              ip = inet_ntoa(source.sin_addr);

              iNameInfoRet = getnameinfo((SOCKADDR *)&source,
                                 sizeof(SOCKADDR),
                                 cHost,
                                 256,
                                 cServ,
                                 256,
                                 NI_NUMERICSERV);
              if (iNameInfoRet == 0) {
                 /* if IP address resolved to a hostname,
                   * print the IP address after it */   
                  if (lstrcmpA(cHost, ip) != 0) {
                      _tprintf(_T("%s [%s]"), cHost, ip);
                  } else {
                      _tprintf(_T("%s"), cHost);
                  }
              } else {
                  _tprintf(_T("error: %d"), WSAGetLastError());    
#ifdef DBG
                  _tprintf(_T(" getnameinfo failed: %d"), iNameInfoRet);
#endif /* DBG */ 
              }

        } else {
           _tprintf(_T("%s"), inet_ntoa(source.sin_addr));
        }
        _tprintf(_T("\n"));

        /* check if we've arrived at the target */
        if (strcmp(cDestIP, inet_ntoa(source.sin_addr)) == 0) {
            bFoundTarget = TRUE;
        } else {
            iTTL++;
            iHopCount++;
            Sleep(500);
        }
    }
    _tprintf(_T("\nTrace complete.\n"));
    WSACleanup();
    
    return 0;
}

/*
 * Establish if performance counters are available and
 * set up timing figures in relation to processor frequency.
 * If performance counters are not available, we'll be using 
 * gettickcount, so set the figures to 1
 *
 */
VOID SetupTimingMethod(VOID)
{
    LARGE_INTEGER PerformanceCounterFrequency;
    
    /* check if performance counters are available */
    bUsePerformanceCounter = QueryPerformanceFrequency(&PerformanceCounterFrequency);
    if (bUsePerformanceCounter) {
        /* restrict execution to first processor on SMP systems */
        if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0) {
            bUsePerformanceCounter = FALSE;
        }
    
        TicksPerMs.QuadPart  = PerformanceCounterFrequency.QuadPart / 1000;
        TicksPerUs.QuadPart  = PerformanceCounterFrequency.QuadPart / 1000000;
    }
    
    if (!bUsePerformanceCounter) {
        TicksPerMs.QuadPart = 1;
        TicksPerUs.QuadPart = 1;
    }    
}


/*
 *
 * Check for a hostname or dotted deciamal for our target.
 * If we have a hostname, resolve to an IP and store it, else
 * just store the target IP address. Also set up other key 
 * SOCKADDR_IN members needed for the connection.
 *
 */
VOID ResolveHostname(VOID)
{
    HOSTENT *hp;
    ULONG addr;
    
    memset(&dest, 0, sizeof(dest));

    addr = inet_addr(cHostname);
    /* if address is not a dotted decimal */
    if (addr == INADDR_NONE) {
       hp = gethostbyname(cHostname);
       if (hp != 0) {
          memcpy(&dest.sin_addr, hp->h_addr, hp->h_length);
          //dest.sin_addr = *((struct in_addr *)hp->h_addr);
          dest.sin_family = hp->h_addrtype;
       } else {
          _tprintf(_T("Unable to resolve target system name %s.\n"), cHostname);
          WSACleanup();
          exit(1);
       }
    } else {
        dest.sin_addr.s_addr = addr;
        dest.sin_family = AF_INET;
    }
    /* copy destination IP address into a string */
    strcpy(cDestIP, inet_ntoa(dest.sin_addr));       
}



/*
 *
 * Create our socket which will be used for sending and recieving,
 * Socket Type is raw, Protocol is ICMP. Also set the TTL value which will be
 * set in the outgoing IP packet.
 *
 */
INT Setup(INT iTTL)
{
    INT iSockRet;

    /* create raw socket */
    icmpSock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, 0, 0, 0);
    if (icmpSock == INVALID_SOCKET) {
       _tprintf(_T("Could not create socket : %d.\n"), WSAGetLastError());
       if (WSAGetLastError() == WSAEACCES) {
            _tprintf(_T("\n\nYou must be an administrator to run this program!\n\n"));
            WSACleanup();
            exit(1);
        }
       return FALSE;
    }
    
    /* setup for TTL */
    iSockRet = setsockopt(icmpSock, IPPROTO_IP, IP_TTL, (const char *)&iTTL, sizeof(iTTL));
    if (iSockRet == SOCKET_ERROR) {
       _tprintf(_T("TTL setsockopt failed : %d. \n"), WSAGetLastError());
       return FALSE;
    }
    
    return TRUE;
}



/*
 * Prepare the ICMP echo request packet for sending.
 * Calculate the packet checksum
 *
 */
VOID PreparePacket(INT iPacketSize, INT iSeqNum) 
{
    /* assemble ICMP echo request packet */
    sendpacket.icmpheader.type      = ECHO_REQUEST;
    sendpacket.icmpheader.code      = 0;
    sendpacket.icmpheader.checksum  = 0;
    sendpacket.icmpheader.id        = (USHORT)GetCurrentProcessId();
    sendpacket.icmpheader.seq       = iSeqNum;

    /* calculate checksum of packet */
    sendpacket.icmpheader.checksum  = CheckSum((PUSHORT)&sendpacket, sizeof(ICMP_HEADER) + iPacketSize);
}



/*
 *
 * Get the system time and send the ICMP packet to the destination
 * address.
 *
 */
INT SendPacket(INT datasize)
{
    INT iSockRet;
    INT iPacketSize;
    
    iPacketSize = sizeof(ECHO_REPLY_HEADER) + datasize;

#ifdef DBG
    _tprintf(_T("\nsending packet of %d bytes\n"), iPacketSize);
#endif /* DBG */

    /* get time packet was sent */
    lTimeStart = GetTime();

    iSockRet = sendto(icmpSock,              //socket
                      (char *)&sendpacket,   //buffer
                      iPacketSize,           //size of buffer
                      0,                     //flags
                      (SOCKADDR *)&dest,     //destination
                      sizeof(dest));         //address length
    
    if (iSockRet == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEACCES) {
            _tprintf(_T("\n\nYou must be an administrator to run this program!\n\n"));
            exit(1);
            WSACleanup();
        } else {
#ifdef DBG
            _tprintf(_T("sendto failed %d\n"), WSAGetLastError());
#endif /* DBG */            
            return FALSE;
        }
    }
#ifdef DBG
    _tprintf(_T("sent %d bytes\n"), iSockRet);
#endif /* DBG */

    /* return number of bytes sent */
    return iSockRet;
}



/*
 *
 * Set up a timeout value and put the socket in a select poll.
 * Wait until we recieve an IPv4 reply packet in reply to the ICMP 
 * echo request packet and get the time the packet was recieved.
 * If we don't recieve a packet, do some checking to establish why.
 *
 */
INT ReceivePacket(INT datasize)
{
    TIMEVAL timeVal;
    FD_SET readFDS;
    int iSockRet = 0, iSelRet;
    int iFromLen;
    int iPacketSize;

    /* allow for a larger recv buffer to store ICMP TTL
     * exceed, IP header and orginal ICMP request */
    iPacketSize = MAX_REC_SIZE + datasize; 

    iFromLen = sizeof(source);

#ifdef DBG
    _tprintf(_T("receiving packet. Available buffer, %d bytes\n"), iPacketSize);
#endif /* DBG */

    /* monitor icmpSock for incomming connections */
    FD_ZERO(&readFDS);
    FD_SET(icmpSock, &readFDS);
    
    /* set timeout values */
    timeVal.tv_sec  = iTimeOut / 1000;
    timeVal.tv_usec = iTimeOut % 1000;
    
    iSelRet = select(0, &readFDS, NULL, NULL, &timeVal);
    
    if ((iSelRet != SOCKET_ERROR) && (iSelRet != 0)) {
        iSockRet = recvfrom(icmpSock,              //socket
                           (char *)&recvpacket,    //buffer
                           iPacketSize,            //size of buffer
                           0,                      //flags
                           (SOCKADDR *)&source,    //source address
                           &iFromLen);             //pointer to address length
        /* get time packet was recieved */
        lTimeEnd = GetTime();  
    /* if socket timed out */                             
    } else if (iSelRet == 0) {
        _tprintf(_T("   *  "));
        return 1;
    } else if (iSelRet == SOCKET_ERROR) {
        _tprintf(_T("select() failed in sendPacket() %d\n"), WSAGetLastError());
	return -1;
    }

    
    if (iSockRet == SOCKET_ERROR) {
        _tprintf(_T("recvfrom failed: %d\n"), WSAGetLastError());
        return -2;
    }
#ifdef DBG
    else {
	_tprintf(_T("reveived %d bytes\n"), iSockRet);
    }
#endif /* DBG */

    return 0;
}



/*
 *
 * Cast the IPv4 packet to an echo reply and to a TTL exceed.
 * Check the 'type' field to establish what was recieved, and 
 * ensure the packet is related to the originating process.
 * It all is well, print the time taken for the round trip.
 *
 */
INT DecodeResponse(INT iPacketSize, INT iSeqNum)
{
    unsigned short header_len = recvpacket.h_len * 4;
    /* cast the recieved packet into an ECHO reply and a TTL Exceed so we can check the ID*/
    ECHO_REPLY_HEADER *IcmpHdr = (ECHO_REPLY_HEADER *)((char*)&recvpacket + header_len);
    TTL_EXCEED_HEADER *TTLExceedHdr = (TTL_EXCEED_HEADER *)((char *)&recvpacket + header_len);

    /* Make sure the reply is ok */
    if (iPacketSize < header_len + ICMP_MIN_SIZE) {
        _tprintf(_T("too few bytes from %s\n"), inet_ntoa(dest.sin_addr));
        return -2;
    }  
    
    switch (IcmpHdr->icmpheader.type) {
           case TTL_EXCEEDED :
                if (TTLExceedHdr->OrigIcmpHeader.id != (USHORT)GetCurrentProcessId()) {
                /* FIXME */
                /* we've picked up a packet not related to this process
                 * probably from another local program. We ignore it */
#ifdef DGB
                    _tprintf(_T("header id,  process id  %d"), TTLExceedHdr->OrigIcmpHeader.id, GetCurrentProcessId());
#endif /* DBG */
                    //_tprintf(_T("oops ");
                    return -1;
                }
                _tprintf(_T("%3Ld ms"), (lTimeEnd - lTimeStart) / TicksPerMs.QuadPart);
                return 0;
           case ECHO_REPLY :
                if (IcmpHdr->icmpheader.id != (USHORT)GetCurrentProcessId()) {
                /* FIXME */
                /* we've picked up a packet not related to this process
                 * probably from another local program. We ignore it */
#ifdef DGB
                    _tprintf(_T("\nPicked up wrong packet. icmpheader.id = %d and process id = %d"), IcmpHdr->icmpheader.id, GetCurrentProcessId());
#endif /* DBG */
                    //_tprintf(_T("oops ");
                    return -1;
                }
                _tprintf(_T("%3Ld ms"), (lTimeEnd - lTimeStart) / TicksPerMs.QuadPart);
                return 1;
           case DEST_UNREACHABLE :
                _tprintf(_T("  *  "));
                return 2;
           default :
                /* unknown ICMP packet */
                return -3;
    } 
}


/*
 *
 * Get the system time using preformance counters if available,
 * otherwise fall back to GetTickCount()
 *
 */

LONG GetTime(VOID) 
{
    LARGE_INTEGER Time;
    
    if (bUsePerformanceCounter) {
        if (QueryPerformanceCounter(&Time) == 0) {
            Time.u.LowPart = (DWORD)GetTickCount();
            Time.u.HighPart = 0;
            return (LONGLONG)Time.u.LowPart;
        } 
    } else {
            Time.u.LowPart = (DWORD)GetTickCount();
            Time.u.HighPart = 0;
            return (LONGLONG)Time.u.LowPart;
    }
    return Time.QuadPart;
}


/*
 *
 * Calculate packet checksum.
 *
 */
WORD CheckSum(PUSHORT data, UINT size)
{
    DWORD dwSum = 0;

    while (size > 1) {
        dwSum += *data++;
        size -= sizeof(USHORT);
    }

    if (size)
        dwSum += *(UCHAR*)data;

    dwSum = (dwSum >> 16) + (dwSum & 0xFFFF);
    dwSum += (dwSum >> 16);

    return (USHORT)(~dwSum);
}


/*
 *
 * print program usage to screen
 *
 */
VOID Usage(VOID)
{
    _tprintf(_T("\nUsage: tracert [-d] [-h maximum_hops] [-j host-list] [-w timeout] target_name\n\n"));
    _tprintf(_T("Options:\n"));
    _tprintf(_T("    -d                 Do not resolve addresses to hostnames.\n"));
    _tprintf(_T("    -h maximum_hops    Maximum number of hops to search for target.\n"));
    _tprintf(_T("    -j host-list       Loose source route along host-list.\n"));
    _tprintf(_T("    -w timeout         Wait timeout milliseconds for each reply.\n\n"));
    
    /* temp notes to stop user questions until getnameinfo/gethostbyaddr and getsockopt are implemented */
    _tprintf(_T("NOTES\n-----\n"
           "- Setting TTL values is not currently supported in ReactOS, so the trace will\n"
           "  jump straight to the destination. This feature will be implemented soon.\n"
           "- Host info is not currently available in ReactOS and will fail with strange\n"
           "  results. Use -d to force it not to resolve IP's.\n"
           "- For testing purposes, all should work as normal in a Windows environment\n\n"));
}



/*
 *
 * Program entry point
 *
 */
int main(int argc, char* argv[])
{
    if (!ParseCmdline(argc, _argv)) return -1;

    Driver();

    return 0;
}
