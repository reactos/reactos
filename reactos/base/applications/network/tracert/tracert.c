 /*
 * PROJECT:     ReactOS trace route utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/network/tracert/tracert.c
 * PURPOSE:     Trace network paths through networks
 * COPYRIGHT:   Copyright 2006 - 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "tracert.h"

//#define TRACERT_DBG

CHAR cHostname[256];            // target hostname
CHAR cDestIP[18];               // target IP

static VOID
DebugPrint(LPTSTR lpString, ...)
{
#ifdef TRACERT_DBG
    va_list args;
    va_start(args, lpString);
    _vtprintf(lpString, args);
    va_end(args);
#else
    UNREFERENCED_PARAMETER(lpString);
#endif
}

static VOID
Usage(VOID)
{
    _tprintf(_T("\nUsage: tracert [-d] [-h maximum_hops] [-j host-list] [-w timeout] target_name\n\n"
                "Options:\n"
                "    -d                 Do not resolve addresses to hostnames.\n"
                "    -h maximum_hops    Maximum number of hops to search for target.\n"
                "    -j host-list       Loose source route along host-list.\n"
                "    -w timeout         Wait timeout milliseconds for each reply.\n\n"));

    _tprintf(_T("NOTES\n-----\n"
           "- Setting TTL values is not currently supported in ReactOS, so the trace will\n"
           "  jump straight to the destination. This feature will be implemented soon.\n"
           "- Host info is not currently available in ReactOS and will fail with strange\n"
           "  results. Use -d to force it not to resolve IP's.\n"
           "- For testing purposes, all should work as normal in a Windows environment\n\n"));
}

static BOOL
ParseCmdline(int argc,
             LPCTSTR argv[],
             PAPPINFO pInfo)
{
    INT i;

    if (argc < 2)
    {
       Usage();
       return FALSE;
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] == _T('-'))
            {
                switch (argv[i][1])
                {
                    case _T('d'):
                        pInfo->bResolveAddresses = FALSE;
                    break;

                    case _T('h'):
                        _stscanf(argv[i+1], _T("%d"), &pInfo->iMaxHops);
                    break;

                    case _T('j'):
                        _tprintf(_T("-j is not yet implemented.\n"));
                    break;

                    case _T('w'):
                        _stscanf(argv[i+1], _T("%d"), &pInfo->iTimeOut);
                    break;

                    default:
                    {
                        _tprintf(_T("%s is not a valid option.\n"), argv[i]);
                        Usage();
                        return FALSE;
                    }
                }
            }
            else
               /* copy target address */
               _tcsncpy(cHostname, argv[i], 255);
        }
    }

    return TRUE;
}

static WORD
CheckSum(PUSHORT data,
         UINT size)
{
    DWORD dwSum = 0;

    while (size > 1)
    {
        dwSum += *data++;
        size -= sizeof(USHORT);
    }

    if (size)
        dwSum += *(UCHAR*)data;

    dwSum = (dwSum >> 16) + (dwSum & 0xFFFF);
    dwSum += (dwSum >> 16);

    return (USHORT)(~dwSum);
}

static VOID
SetupTimingMethod(PAPPINFO pInfo)
{
    LARGE_INTEGER PerformanceCounterFrequency;

    /* check if performance counters are available */
    pInfo->bUsePerformanceCounter = QueryPerformanceFrequency(&PerformanceCounterFrequency);

    if (pInfo->bUsePerformanceCounter)
    {
        /* restrict execution to first processor on SMP systems */
        if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0)
            pInfo->bUsePerformanceCounter = FALSE;

        pInfo->TicksPerMs.QuadPart  = PerformanceCounterFrequency.QuadPart / 1000;
        pInfo->TicksPerUs.QuadPart  = PerformanceCounterFrequency.QuadPart / 1000000;
    }
    else
    {
        pInfo->TicksPerMs.QuadPart = 1;
        pInfo->TicksPerUs.QuadPart = 1;
    }
}

static BOOL
ResolveHostname(PAPPINFO pInfo)
{
    HOSTENT *hp;
    ULONG addr;

    ZeroMemory(&pInfo->dest, sizeof(pInfo->dest));

    /* if address is not a dotted decimal */
    if ((addr = inet_addr(cHostname))== INADDR_NONE)
    {
       if ((hp = gethostbyname(cHostname)) != 0)
       {
          //CopyMemory(&pInfo->dest.sin_addr, hp->h_addr, hp->h_length);
          pInfo->dest.sin_addr = *((struct in_addr *)hp->h_addr);
          pInfo->dest.sin_family = hp->h_addrtype;
       }
       else
       {
          _tprintf(_T("Unable to resolve target system name %s.\n"), cHostname);
          return FALSE;
       }
    }
    else
    {
        pInfo->dest.sin_addr.s_addr = addr;
        pInfo->dest.sin_family = AF_INET;
    }

    _tcscpy(cDestIP, inet_ntoa(pInfo->dest.sin_addr));

    return TRUE;
}

static LONGLONG
GetTime(PAPPINFO pInfo)
{
    LARGE_INTEGER Time;

    /* Get the system time using preformance counters if available */
    if (pInfo->bUsePerformanceCounter)
    {
        if (QueryPerformanceCounter(&Time))
        {
            return Time.QuadPart;
        }
    }

    /* otherwise fall back to GetTickCount */
    Time.u.LowPart = (DWORD)GetTickCount();
    Time.u.HighPart = 0;

    return (LONGLONG)Time.u.LowPart;
}

static BOOL
SetTTL(SOCKET sock,
       INT iTTL)
{
    if (setsockopt(sock,
                   IPPROTO_IP,
                   IP_TTL,
                   (const char *)&iTTL,
                   sizeof(iTTL)) == SOCKET_ERROR)
    {
       DebugPrint(_T("TTL setsockopt failed : %d. \n"), WSAGetLastError());
       return FALSE;
    }

    return TRUE;
}

static BOOL
CreateSocket(PAPPINFO pInfo)
{
    pInfo->icmpSock = WSASocket(AF_INET,
                                SOCK_RAW,
                                IPPROTO_ICMP,
                                0,
                                0,
                                0);

    if (pInfo->icmpSock == INVALID_SOCKET)
    {
        INT err = WSAGetLastError();
        DebugPrint(_T("Could not create socket : %d.\n"), err);

        if (err == WSAEACCES)
        {
            _tprintf(_T("\n\nYou must have access to raw sockets (admin) to run this program!\n\n"));
        }

        return FALSE;
    }

    return TRUE;
}

static VOID
PreparePacket(PAPPINFO pInfo,
              USHORT iSeqNum)
{
    /* assemble ICMP echo request packet */
    pInfo->SendPacket->icmpheader.type     = ECHO_REQUEST;
    pInfo->SendPacket->icmpheader.code     = 0;
    pInfo->SendPacket->icmpheader.checksum = 0;
    pInfo->SendPacket->icmpheader.id       = (USHORT)GetCurrentProcessId();
    pInfo->SendPacket->icmpheader.seq      = htons((USHORT)iSeqNum);

    /* calculate checksum of packet */
    pInfo->SendPacket->icmpheader.checksum  = CheckSum((PUSHORT)&pInfo->SendPacket->icmpheader,
                                                       sizeof(ICMP_HEADER) + PACKET_SIZE);
}

static INT
SendPacket(PAPPINFO pInfo)
{
    INT iSockRet;

    DebugPrint(_T("\nsending packet of %d bytes... "), PACKET_SIZE);

    /* get time packet was sent */
    pInfo->lTimeStart = GetTime(pInfo);

    iSockRet = sendto(pInfo->icmpSock,              //socket
                      (char *)&pInfo->SendPacket->icmpheader,//buffer
                      sizeof(ICMP_HEADER) + PACKET_SIZE,//size of buffer
                      0,                            //flags
                      (SOCKADDR *)&pInfo->dest,     //destination
                      sizeof(pInfo->dest));         //address length

    if (iSockRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() == WSAEACCES)
        {
            /* FIXME: Is this correct? */
            _tprintf(_T("\n\nYou must be an administrator to run this program!\n\n"));
            WSACleanup();
            HeapFree(GetProcessHeap(), 0, pInfo);
            exit(-1);
        }
        else
        {
            DebugPrint(_T("sendto failed %d\n"), WSAGetLastError());
        }
    }
    else
    {
        DebugPrint(_T("sent %d bytes\n"), iSockRet);
    }

    return iSockRet;
}

static BOOL
ReceivePacket(PAPPINFO pInfo)
{
    TIMEVAL timeVal;
    FD_SET readFDS;
    INT iSockRet = 0, iSelRet;
    INT iFromLen;
    BOOL bRet = FALSE;

    iFromLen = sizeof(pInfo->source);

    DebugPrint(_T("Receiving packet. Available buffer, %d bytes... "), MAX_PING_PACKET_SIZE);

    /* monitor icmpSock for incomming connections */
    FD_ZERO(&readFDS);
    FD_SET(pInfo->icmpSock, &readFDS);

    /* set timeout values */
    timeVal.tv_sec  = pInfo->iTimeOut / 1000;
    timeVal.tv_usec = pInfo->iTimeOut % 1000;

    iSelRet = select(0,
                     &readFDS,
                     NULL,
                     NULL,
                     &timeVal);

    if (iSelRet == SOCKET_ERROR)
    {
        DebugPrint(_T("select() failed in sendPacket() %d\n"), WSAGetLastError());
    }
    else if (iSelRet == 0) /* if socket timed out */
    {
        _tprintf(_T("   *  "));
    }
    else if ((iSelRet != SOCKET_ERROR) && (iSelRet != 0))
    {
        iSockRet = recvfrom(pInfo->icmpSock,            // socket
                           (char *)pInfo->RecvPacket,  // buffer
                           MAX_PING_PACKET_SIZE,        // size of buffer
                           0,                           // flags
                           (SOCKADDR *)&pInfo->source,  // source address
                           &iFromLen);                  // address length

        if (iSockRet != SOCKET_ERROR)
        {
            /* get time packet was received */
            pInfo->lTimeEnd = GetTime(pInfo);
            DebugPrint(_T("reveived %d bytes\n"), iSockRet);
            bRet = TRUE;
        }
        else
        {
            DebugPrint(_T("recvfrom failed: %d\n"), WSAGetLastError());
        }
    }

    return bRet;
}

static INT
DecodeResponse(PAPPINFO pInfo)
{
    unsigned short header_len = pInfo->RecvPacket->h_len * 4;

    /* cast the received packet into an ECHO reply and a TTL Exceed and check the ID*/
    ECHO_REPLY_HEADER *IcmpHdr = (ECHO_REPLY_HEADER *)((char*)pInfo->RecvPacket + header_len);

    /* Make sure the reply is ok */
    if (PACKET_SIZE < header_len + ICMP_MIN_SIZE)
    {
        DebugPrint(_T("too few bytes from %s\n"), inet_ntoa(pInfo->dest.sin_addr));
        return -2;
    }

    switch (IcmpHdr->icmpheader.type)
    {
           case TTL_EXCEEDED :
                _tprintf(_T("%3ld ms"), (ULONG)((pInfo->lTimeEnd - pInfo->lTimeStart) / pInfo->TicksPerMs.QuadPart));
                return 0;

           case ECHO_REPLY :
                if (IcmpHdr->icmpheader.id != (USHORT)GetCurrentProcessId())
                {
                /* FIXME: our network stack shouldn't allow this... */
                /* we've picked up a packet not related to this process probably from another local program. We ignore it */
                    DebugPrint(_T("Rouge packet: header id %d, process id  %d"), IcmpHdr->icmpheader.id, GetCurrentProcessId());
                    return -1;
                }
                _tprintf(_T("%3ld ms"), (ULONG)((pInfo->lTimeEnd - pInfo->lTimeStart) / pInfo->TicksPerMs.QuadPart));
                return 1;

           case DEST_UNREACHABLE :
                _tprintf(_T("  *  "));
                return 2;
    }

    return -3;
}

static BOOL
AllocateBuffers(PAPPINFO pInfo)
{
    pInfo->SendPacket = (PECHO_REPLY_HEADER)HeapAlloc(GetProcessHeap(),
                                                      0,
                                                      sizeof(ECHO_REPLY_HEADER) + PACKET_SIZE);
    if (!pInfo->SendPacket)
        return FALSE;

    pInfo->RecvPacket = (PIPv4_HEADER)HeapAlloc(GetProcessHeap(),
                                                0,
                                                MAX_PING_PACKET_SIZE);
    if (!pInfo->RecvPacket)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 pInfo->SendPacket);

        return FALSE;
    }

    return TRUE;
}

static INT
Driver(PAPPINFO pInfo)
{
    INT iHopCount = 1;              // hop counter. default max is 30
    BOOL bFoundTarget = FALSE;      // Have we reached our destination yet
    INT iReceiveReturn;             // ReceiveReturn return value
    PECHO_REPLY_HEADER icmphdr;
    INT iTTL = 1;

    INT ret = -1;

    //temps for getting host name
    CHAR cHost[256];
    CHAR cServ[256];
    CHAR *ip;

    SetupTimingMethod(pInfo);

    if (AllocateBuffers(pInfo) &&
        ResolveHostname(pInfo) &&
        CreateSocket(pInfo))
    {
        /* print tracing info to screen */
        _tprintf(_T("\nTracing route to %s [%s]\n"), cHostname, cDestIP);
        _tprintf(_T("over a maximum of %d hop"), pInfo->iMaxHops);
        pInfo->iMaxHops > 1 ? _tprintf(_T("s:\n\n")) : _tprintf(_T(":\n\n"));

        /* run until we hit either max hops, or find the target */
        while ((iHopCount <= pInfo->iMaxHops) &&
               (bFoundTarget != TRUE))
        {
            USHORT iSeqNum = 0;
            INT i;

            _tprintf(_T("%3d   "), iHopCount);

            /* run 3 pings for each hop */
            for (i = 0; i < 3; i++)
            {
                if (SetTTL(pInfo->icmpSock, iTTL) != TRUE)
                {
                    DebugPrint(_T("error in Setup()\n"));
                    return ret;
                }

                PreparePacket(pInfo, iSeqNum);

                if (SendPacket(pInfo) != SOCKET_ERROR)
                {
                    BOOL bAwaitPacket = FALSE; // indicates whether we have received a good packet

                    do
                    {
                        /* Receive replies until we get a successful read, or a fatal error */
                        if ((iReceiveReturn = ReceivePacket(pInfo)) < 0)
                        {
                            /* FIXME: consider moving this into ReceivePacket */
                            /* check the seq num in the packet, if it's bad wait for another */
                            WORD hdrLen = pInfo->RecvPacket->h_len * 4;
                            icmphdr = (ECHO_REPLY_HEADER *)((char*)&pInfo->RecvPacket + hdrLen);
                            if (icmphdr->icmpheader.seq != iSeqNum)
                            {
                                _tprintf(_T("bad sequence number!\n"));
                                continue;
                            }
                        }

                        if (iReceiveReturn)
                        {
                            if (DecodeResponse(pInfo) < 0)
                                bAwaitPacket = TRUE;
                        }

                    } while (bAwaitPacket);
                }

                iSeqNum++;
                _tprintf(_T("   "));
            }

            if(pInfo->bResolveAddresses)
            {
                INT iNameInfoRet;               // getnameinfo return value
               /* gethostbyaddr() and getnameinfo() are
                * unimplemented in ROS at present.
                * Alex has advised he will be implementing getnameinfo.
                * I've used that for the time being for testing in Windows*/

                  //ip = inet_addr(inet_ntoa(source.sin_addr));
                  //host = gethostbyaddr((char *)&ip, 4, 0);

                  ip = inet_ntoa(pInfo->source.sin_addr);

                  iNameInfoRet = getnameinfo((SOCKADDR *)&pInfo->source,
                                             sizeof(SOCKADDR),
                                             cHost,
                                             256,
                                             cServ,
                                             256,
                                             NI_NUMERICSERV);
                  if (iNameInfoRet == 0)
                  {
                     /* if IP address resolved to a hostname,
                      * print the IP address after it */
                      if (lstrcmpA(cHost, ip) != 0)
                          _tprintf(_T("%s [%s]"), cHost, ip);
                      else
                          _tprintf(_T("%s"), cHost);
                  }
                  else
                  {
                      DebugPrint(_T("error: %d"), WSAGetLastError());
                      DebugPrint(_T(" getnameinfo failed: %d"), iNameInfoRet);
                      _tprintf(_T("%s"), inet_ntoa(pInfo->source.sin_addr));
                  }

            }
            else
               _tprintf(_T("%s"), inet_ntoa(pInfo->source.sin_addr));

            _tprintf(_T("\n"));

            /* check if we've arrived at the target */
            if (strcmp(cDestIP, inet_ntoa(pInfo->source.sin_addr)) == 0)
                bFoundTarget = TRUE;
            else
            {
                iTTL++;
                iHopCount++;
                Sleep(500);
            }
        }
        _tprintf(_T("\nTrace complete.\n"));
        ret = 0;
    }

    return ret;
}

static VOID
Cleanup(PAPPINFO pInfo)
{
    if (pInfo->icmpSock)
        closesocket(pInfo->icmpSock);

    WSACleanup();

    if (pInfo->SendPacket)
        HeapFree(GetProcessHeap(),
                 0,
                 pInfo->SendPacket);

    if (pInfo->RecvPacket)
        HeapFree(GetProcessHeap(),
                 0,
                 pInfo->RecvPacket);
}

#if defined(_UNICODE) && defined(__GNUC__)
static
#endif
int _tmain(int argc, LPCTSTR argv[])
{
    PAPPINFO pInfo;
    WSADATA wsaData;
    int ret = -1;

    pInfo = (PAPPINFO)HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                sizeof(APPINFO));
    if (pInfo)
    {
        pInfo->bResolveAddresses = TRUE;
        pInfo->iMaxHops = 30;
        pInfo->iTimeOut = 1000;

        if (ParseCmdline(argc, argv, pInfo))
        {
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            {
                DebugPrint(_T("WSAStartup failed.\n"));
            }
            else
            {
                ret = Driver(pInfo);
                Cleanup(pInfo);
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 pInfo);
    }

    return ret;
}

#if defined(_UNICODE) && defined(__GNUC__)
/* HACK - MINGW HAS NO OFFICIAL SUPPORT FOR wmain()!!! */
int main( int argc, char **argv )
{
    WCHAR **argvW;
    int i, j, Ret = 1;

    if ((argvW = malloc(argc * sizeof(WCHAR*))))
    {
        /* convert the arguments */
        for (i = 0, j = 0; i < argc; i++)
        {
            if (!(argvW[i] = malloc((strlen(argv[i]) + 1) * sizeof(WCHAR))))
            {
                j++;
            }
            swprintf(argvW[i], L"%hs", argv[i]);
        }

        if (j == 0)
        {
            /* no error converting the parameters, call wmain() */
            Ret = wmain(argc, (LPCTSTR *)argvW);
        }

        /* free the arguments */
        for (i = 0; i < argc; i++)
        {
            if (argvW[i])
                free(argvW[i]);
        }
        free(argvW);
    }

    return Ret;
}
#endif
