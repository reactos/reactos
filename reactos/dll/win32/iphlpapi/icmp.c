/*
 * ICMP
 *
 * Francois Gouget, 1999, based on the work of
 *   RW Hall, 1999, based on public domain code PING.C by Mike Muus (1983)
 *   and later works (c) 1989 Regents of Univ. of California - see copyright
 *   notice at end of source-code.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Future work:
 * - Systems like FreeBSD don't seem to support the IP_TTL option and maybe others.
 *   But using IP_HDRINCL and building the IP header by hand might work.
 * - Not all IP options are supported.
 * - Are ICMP handles real handles, i.e. inheritable and all? There might be some
 *   more work to do here, including server side stuff with synchronization.
 * - Is it correct to use malloc for the internal buffer, for allocating the
 *   handle's structure?
 * - This API should probably be thread safe. Is it really?
 * - Using the winsock functions has not been tested.
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
# include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdarg.h>
//#include <string.h>
//#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include <windef.h>
#include <winbase.h>
//#include "winerror.h"
#include <ipexport.h>
//#include <ws2tcpip.h>
//#include "icmpapi.h"
#include <wine/debug.h>

/* Set up endiannes macros for the ip and ip_icmp BSD headers */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN       4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN    1234
#endif
#ifndef BYTE_ORDER
#ifdef WORDS_BIGENDIAN
#define BYTE_ORDER       BIG_ENDIAN
#else
#define BYTE_ORDER       LITTLE_ENDIAN
#endif
#endif /* BYTE_ORDER */

#define u_int16_t  WORD
#define u_int32_t  DWORD

/* These are BSD headers. We use these here because they are needed on
 * libc5 Linux systems. On other platforms they are usually simply more
 * complete than the native stuff, and cause less portability problems
 * so we use them anyway.
 */
#include "ip.h"
#include "ip_icmp.h"


WINE_DEFAULT_DEBUG_CHANNEL(icmp);


typedef struct {
    int sid;
    IP_OPTION_INFORMATION default_opts;
} icmp_t;

#define IP_OPTS_UNKNOWN     0
#define IP_OPTS_DEFAULT     1
#define IP_OPTS_CUSTOM      2

/* The sequence number is unique process wide, so that all threads
 * have a distinct sequence number.
 */
static LONG icmp_sequence=0;

static int in_cksum(u_short *addr, int len)
{
    int nleft=len;
    u_short *w = addr;
    int sum = 0;
    u_short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(u_char *)(&answer) = *(u_char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum  += (sum >> 16);
    answer = ~sum;
    return(answer);
}



/*
 * Exported Routines.
 */

/***********************************************************************
 *		IcmpCreateFile (ICMP.@)
 */
HANDLE WINAPI IcmpCreateFile(VOID)
{
    icmp_t* icp;

    int sid=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if (sid < 0) {
        MESSAGE("WARNING: Trying to use ICMP (network ping) will fail unless running as root\n");
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    icp=HeapAlloc(GetProcessHeap(), 0, sizeof(*icp));
    if (icp==NULL) {
        closesocket(sid);
        SetLastError(IP_NO_RESOURCES);
        return INVALID_HANDLE_VALUE;
    }
    icp->sid=sid;
    icp->default_opts.OptionsSize=IP_OPTS_UNKNOWN;
    return (HANDLE)icp;
}


/***********************************************************************
 *		IcmpCloseHandle (ICMP.@)
 */
BOOL WINAPI IcmpCloseHandle(HANDLE  IcmpHandle)
{
    icmp_t* icp=(icmp_t*)IcmpHandle;
    if (IcmpHandle==INVALID_HANDLE_VALUE) {
        /* FIXME: in fact win98 seems to ignore the handle value !!! */
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    shutdown(icp->sid,2);
    HeapFree(GetProcessHeap (), 0, icp);
    return TRUE;
}


/***********************************************************************
 *		IcmpSendEcho (ICMP.@)
 */
DWORD WINAPI IcmpSendEcho(
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    icmp_t* icp=(icmp_t*)IcmpHandle;
    unsigned char* reqbuf;
    int reqsize;

    struct icmp_echo_reply* ier;
    struct ip* ip_header;
    struct icmp* icmp_header;
    char* endbuf;
    int ip_header_len;
    int maxlen;
    fd_set fdr;
    struct timeval timeout;
    DWORD send_time,recv_time;
    struct sockaddr_in addr;
    unsigned int addrlen;
    unsigned short id,seq,cksum;
    int res;

    if (IcmpHandle==INVALID_HANDLE_VALUE) {
        /* FIXME: in fact win98 seems to ignore the handle value !!! */
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (ReplySize<sizeof(ICMP_ECHO_REPLY)+ICMP_MINLEN) {
        SetLastError(IP_BUF_TOO_SMALL);
        return 0;
    }
    /* check the request size against SO_MAX_MSG_SIZE using getsockopt */

    /* Prepare the request */
    id=GetCurrentProcessId() & 0xFFFF;
    seq=InterlockedIncrement(&icmp_sequence) & 0xFFFF;

    reqsize=ICMP_MINLEN+RequestSize;
    reqbuf=HeapAlloc(GetProcessHeap(), 0, reqsize);
    if (reqbuf==NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    icmp_header=(struct icmp*)reqbuf;
    icmp_header->icmp_type=ICMP_ECHO;
    icmp_header->icmp_code=0;
    icmp_header->icmp_cksum=0;
    icmp_header->icmp_id=id;
    icmp_header->icmp_seq=seq;
    memcpy(reqbuf+ICMP_MINLEN, RequestData, RequestSize);
    icmp_header->icmp_cksum=cksum=in_cksum((u_short*)reqbuf,reqsize);

    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=DestinationAddress;
    addr.sin_port=0;

    if (RequestOptions!=NULL) {
        int val;
        if (icp->default_opts.OptionsSize==IP_OPTS_UNKNOWN) {
            int len;
            /* Before we mess with the options, get the default values */
            len=sizeof(val);
            getsockopt(icp->sid,IPPROTO_IP,IP_TTL,(char *)&val,&len);
            icp->default_opts.Ttl=val;

            len=sizeof(val);
            getsockopt(icp->sid,IPPROTO_IP,IP_TOS,(char *)&val,&len);
            icp->default_opts.Tos=val;
            /* FIXME: missing: handling of IP 'flags', and all the other options */
        }

        val=RequestOptions->Ttl;
        setsockopt(icp->sid,IPPROTO_IP,IP_TTL,(char *)&val,sizeof(val));
        val=RequestOptions->Tos;
        setsockopt(icp->sid,IPPROTO_IP,IP_TOS,(char *)&val,sizeof(val));
        /* FIXME:  missing: handling of IP 'flags', and all the other options */

        icp->default_opts.OptionsSize=IP_OPTS_CUSTOM;
    } else if (icp->default_opts.OptionsSize==IP_OPTS_CUSTOM) {
        int val;

        /* Restore the default options */
        val=icp->default_opts.Ttl;
        setsockopt(icp->sid,IPPROTO_IP,IP_TTL,(char *)&val,sizeof(val));
        val=icp->default_opts.Tos;
        setsockopt(icp->sid,IPPROTO_IP,IP_TOS,(char *)&val,sizeof(val));
        /* FIXME: missing: handling of IP 'flags', and all the other options */

        icp->default_opts.OptionsSize=IP_OPTS_DEFAULT;
    }

    /* Get ready for receiving the reply
     * Do it before we send the request to minimize the risk of introducing delays
     */
    FD_ZERO(&fdr);
    FD_SET(icp->sid,&fdr);
    timeout.tv_sec=Timeout/1000;
    timeout.tv_usec=(Timeout % 1000)*1000;
    addrlen=sizeof(addr);
    ier=ReplyBuffer;
    ip_header=(struct ip *) ((char *) ReplyBuffer+sizeof(ICMP_ECHO_REPLY));
    endbuf=(char *) ReplyBuffer+ReplySize;
    maxlen=ReplySize-sizeof(ICMP_ECHO_REPLY);

    /* Send the packet */
    TRACE("Sending %d bytes (RequestSize=%d) to %s\n", reqsize, RequestSize, inet_ntoa(addr.sin_addr));
#if 0
    if (TRACE_ON(icmp)){
        unsigned char* buf=(unsigned char*)reqbuf;
        int i;
        printf("Output buffer:\n");
        for (i=0;i<reqsize;i++)
            printf("%2x,", buf[i]);
        printf("\n");
    }
#endif

    send_time = GetTickCount();
    res=sendto(icp->sid, (const char*)reqbuf, reqsize, 0, (struct sockaddr*)&addr, sizeof(addr));
    HeapFree(GetProcessHeap (), 0, reqbuf);
    if (res<0) {
        if (WSAGetLastError()==WSAEMSGSIZE)
            SetLastError(IP_PACKET_TOO_BIG);
        else {
            switch (WSAGetLastError()) {
            case WSAENETUNREACH:
                SetLastError(IP_DEST_NET_UNREACHABLE);
                break;
            case WSAEHOSTUNREACH:
                SetLastError(IP_DEST_HOST_UNREACHABLE);
                break;
            default:
                TRACE("unknown error: errno=%d\n",WSAGetLastError());
                SetLastError(IP_GENERAL_FAILURE);
            }
        }
        return 0;
    }

    /* Get the reply */
    ip_header_len=0; /* because gcc was complaining */
    while ((res=select(icp->sid+1,&fdr,NULL,NULL,&timeout))>0) {
        recv_time = GetTickCount();
        res=recvfrom(icp->sid, (char*)ip_header, maxlen, 0, (struct sockaddr*)&addr,(int*)&addrlen);
        TRACE("received %d bytes from %s\n",res, inet_ntoa(addr.sin_addr));
        ier->Status=IP_REQ_TIMED_OUT;

        /* Check whether we should ignore this packet */
        if ((ip_header->ip_p==IPPROTO_ICMP) && (res>=sizeof(struct ip)+ICMP_MINLEN)) {
            ip_header_len=ip_header->ip_hl << 2;
            icmp_header=(struct icmp*)(((char*)ip_header)+ip_header_len);
            TRACE("received an ICMP packet of type,code=%d,%d\n",icmp_header->icmp_type,icmp_header->icmp_code);
            if (icmp_header->icmp_type==ICMP_ECHOREPLY) {
                if ((icmp_header->icmp_id==id) && (icmp_header->icmp_seq==seq))
                    ier->Status=IP_SUCCESS;
            } else {
                switch (icmp_header->icmp_type) {
                case ICMP_UNREACH:
                    switch (icmp_header->icmp_code) {
                    case ICMP_UNREACH_HOST:
#ifdef ICMP_UNREACH_HOST_UNKNOWN
                    case ICMP_UNREACH_HOST_UNKNOWN:
#endif
#ifdef ICMP_UNREACH_ISOLATED
                    case ICMP_UNREACH_ISOLATED:
#endif
#ifdef ICMP_UNREACH_HOST_PROHIB
              	    case ICMP_UNREACH_HOST_PROHIB:
#endif
#ifdef ICMP_UNREACH_TOSHOST
                    case ICMP_UNREACH_TOSHOST:
#endif
                        ier->Status=IP_DEST_HOST_UNREACHABLE;
                        break;
                    case ICMP_UNREACH_PORT:
                        ier->Status=IP_DEST_PORT_UNREACHABLE;
                        break;
                    case ICMP_UNREACH_PROTOCOL:
                        ier->Status=IP_DEST_PROT_UNREACHABLE;
                        break;
                    case ICMP_UNREACH_SRCFAIL:
                        ier->Status=IP_BAD_ROUTE;
                        break;
                    default:
                        ier->Status=IP_DEST_NET_UNREACHABLE;
                    }
                    break;
                case ICMP_TIMXCEED:
                    if (icmp_header->icmp_code==ICMP_TIMXCEED_REASS)
                        ier->Status=IP_TTL_EXPIRED_REASSEM;
                    else
                        ier->Status=IP_TTL_EXPIRED_TRANSIT;
                    break;
                case ICMP_PARAMPROB:
                    ier->Status=IP_PARAM_PROBLEM;
                    break;
                case ICMP_SOURCEQUENCH:
                    ier->Status=IP_SOURCE_QUENCH;
                    break;
                }
                if (ier->Status!=IP_REQ_TIMED_OUT) {
                    struct ip* rep_ip_header;
                    struct icmp* rep_icmp_header;
                    /* The ICMP header size of all the packets we accept is the same */
                    rep_ip_header=(struct ip*)(((char*)icmp_header)+ICMP_MINLEN);
                    rep_icmp_header=(struct icmp*)(((char*)rep_ip_header)+(rep_ip_header->ip_hl << 2));

		    /* Make sure that this is really a reply to our packet */
                    if (ip_header_len+ICMP_MINLEN+(rep_ip_header->ip_hl << 2)+ICMP_MINLEN>ip_header->ip_len) {
			ier->Status=IP_REQ_TIMED_OUT;
                    } else if ((rep_icmp_header->icmp_type!=ICMP_ECHO) ||
                        (rep_icmp_header->icmp_code!=0) ||
                        (rep_icmp_header->icmp_id!=id) ||
                        /* windows doesn't check this checksum, else tracert */
                        /* behind a Linux 2.2 masquerading firewall would fail*/
                        /* (rep_icmp_header->icmp_cksum!=cksum) || */
                        (rep_icmp_header->icmp_seq!=seq)) {
                        /* This was not a reply to one of our packets after all */
                        TRACE("skipping type,code=%d,%d id,seq=%d,%d cksum=%d\n",
                            rep_icmp_header->icmp_type,rep_icmp_header->icmp_code,
                            rep_icmp_header->icmp_id,rep_icmp_header->icmp_seq,
                            rep_icmp_header->icmp_cksum);
                        TRACE("expected type,code=8,0 id,seq=%d,%d cksum=%d\n",
                            id,seq,
                            cksum);
			ier->Status=IP_REQ_TIMED_OUT;
		    }
                }
	    }
	}

        if (ier->Status==IP_REQ_TIMED_OUT) {
            /* This packet was not for us.
             * Decrease the timeout so that we don't enter an endless loop even
             * if we get flooded with ICMP packets that are not for us.
             */
            int t = Timeout - (recv_time - send_time);
            if (t < 0) t = 0;
            timeout.tv_sec = t / 1000;
            timeout.tv_usec = (t % 1000) * 1000;
            continue;
        } else {
            /* This is a reply to our packet */
            memcpy(&ier->Address,&ip_header->ip_src,sizeof(IPAddr));
            /* Status is already set */
            ier->RoundTripTime= recv_time - send_time;
            ier->DataSize=res-ip_header_len-ICMP_MINLEN;
            ier->Reserved=0;
            ier->Data=endbuf-ier->DataSize;
            memmove(ier->Data,((char*)ip_header)+ip_header_len+ICMP_MINLEN,ier->DataSize);
            ier->Options.Ttl=ip_header->ip_ttl;
            ier->Options.Tos=ip_header->ip_tos;
            ier->Options.Flags=ip_header->ip_off >> 13;
            ier->Options.OptionsSize=ip_header_len-sizeof(struct ip);
            if (ier->Options.OptionsSize!=0) {
                ier->Options.OptionsData=(unsigned char *) ier->Data-ier->Options.OptionsSize;
                /* FIXME: We are supposed to rearrange the option's 'source route' data */
                memmove(ier->Options.OptionsData,((char*)ip_header)+ip_header_len,ier->Options.OptionsSize);
                endbuf=(char*)ier->Options.OptionsData;
            } else {
                ier->Options.OptionsData=NULL;
                endbuf=ier->Data;
            }

            /* Prepare for the next packet */
            ier++;
            ip_header=(struct ip*)(((char*)ip_header)+sizeof(ICMP_ECHO_REPLY));
            maxlen=endbuf-(char*)ip_header;

            /* Check out whether there is more but don't wait this time */
            timeout.tv_sec=0;
            timeout.tv_usec=0;
        }
        FD_ZERO(&fdr);
        FD_SET(icp->sid,&fdr);
    }
    res=ier-(ICMP_ECHO_REPLY*)ReplyBuffer;
    if (res==0)
        SetLastError(IP_REQ_TIMED_OUT);
    TRACE("received %d replies\n",res);
    return res;
}

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
