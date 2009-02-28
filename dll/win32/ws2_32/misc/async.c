/* Async WINSOCK DNS services
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 1999 Marcus Meissner
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTE: If you make any changes to fix a particular app, make sure
 * they don't break something else like Netscape or telnet and ftp
 * clients and servers (www.winsite.com got a lot of those).
 *
 * FIXME:
 *	- Add WSACancel* and correct handle management. (works rather well for
 *	  now without it.)
 *	- Verify & Check all calls for correctness
 *	  (currently only WSAGetHostByName*, WSAGetServByPort* calls)
 *	- Check error returns.
 *	- mirc/mirc32 Finger @linux.kernel.org sometimes fails in threaded mode.
 *	  (not sure why)
 *	- This implementation did ignore the "NOTE:" section above (since the
 *	  whole stuff did not work anyway to other changes).
 */

#include <windows.h>
#include <winsock.h>

#define WS_FD_SETSIZE FD_SETSIZE
#define HAVE_GETPROTOBYNAME
#define HAVE_GETPROTOBYNUMBER
#define HAVE_GETSERVBYPORT
typedef struct hostent WS_hostent;
typedef struct servent WS_servent;
typedef struct protoent WS_protoent;

#include "wine/config.h"
#include "wine/port.h"

#ifndef __REACTOS__
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#if defined(__svr4__)
#include <sys/ioccom.h>
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#endif

#if defined(__EMX__)
# include <sys/so_ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#endif

#define CALLBACK __stdcall

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winsock2.h"
#include "ws2spi.h"
#include "wownt32.h"
#include "wine/winsock16.h"
#include "winnt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);


/* protoptypes of some functions in socket.c
 */

#define AQ_WIN16	0x00
#define AQ_WIN32	0x04
#define HB_WIN32(hb) (hb->flags & AQ_WIN32)
#define AQ_NUMBER	0x00
#define AQ_NAME		0x08
#define AQ_COPYPTR1	0x10
#define AQ_DUPLOWPTR1	0x20
#define AQ_MASKPTR1	0x30
#define AQ_COPYPTR2	0x40
#define AQ_DUPLOWPTR2	0x80
#define AQ_MASKPTR2	0xC0

#define AQ_GETHOST	0
#define AQ_GETPROTO	1
#define AQ_GETSERV	2
#define AQ_GETMASK	3

/* The handles used are pseudo-handles that can be simply casted. */
/* 16-bit values are used internally (to be sure handle comparison works right in 16-bit apps). */
#define WSA_H32(h16) ((HANDLE)(ULONG_PTR)(h16))

/* ----------------------------------- helper functions - */

static int list_size(char** l, int item_size)
{
  int i,j = 0;
  if(l)
  { for(i=0;l[i];i++)
	j += (item_size) ? item_size : strlen(l[i]) + 1;
    j += (i + 1) * sizeof(char*); }
  return j;
}

static int list_dup(char** l_src, char* ref, char* base, int item_size)
{
   /* base is either either equal to ref or 0 or SEGPTR */

   char*		p = ref;
   char**		l_to = (char**)ref;
   int			i,j,k;

   for(j=0;l_src[j];j++) ;
   p += (j + 1) * sizeof(char*);
   for(i=0;i<j;i++)
   { l_to[i] = base + (p - ref);
     k = ( item_size ) ? item_size : strlen(l_src[i]) + 1;
     memcpy(p, l_src[i], k); p += k; }
   l_to[i] = NULL;
   return (p - ref);
}

/* ----- hostent */

static int hostent_size(struct hostent* p_he)
{
  int size = 0;
  if( p_he )
  { size  = sizeof(struct hostent);
    size += strlen(p_he->h_name) + 1;
    size += list_size(p_he->h_aliases, 0);
    size += list_size(p_he->h_addr_list, p_he->h_length ); }
  return size;
}

/* Copy hostent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_he(char *p_to,char *p_base,int t_size,struct hostent* p_he, int flag)
{
	char* p_name,*p_aliases,*p_addr,*p;
	struct ws_hostent16 *p_to16 = (struct ws_hostent16*)p_to;
	WS_hostent *p_to32 = (WS_hostent*)p_to;
	int	size = hostent_size(p_he) +
		(
		(flag & AQ_WIN16) ? sizeof(struct ws_hostent16) : sizeof(WS_hostent)
		- sizeof(struct hostent)
		);

	if (t_size < size)
		return -size;
	p = p_to;
	p += (flag & AQ_WIN16) ?
		sizeof(struct ws_hostent16) : sizeof(WS_hostent);
	p_name = p;
	strcpy(p, p_he->h_name); p += strlen(p) + 1;
	p_aliases = p;
	p += list_dup(p_he->h_aliases, p, p_base + (p - (char*)p_to), 0);
	p_addr = p;
	list_dup(p_he->h_addr_list, p, p_base + (p - (char*)p_to), p_he->h_length);

	if (flag & AQ_WIN16)
	{
	    p_to16->h_addrtype = (INT16)p_he->h_addrtype;
	    p_to16->h_length = (INT16)p_he->h_length;
	    p_to16->h_name = (SEGPTR)(p_base + (p_name - p_to));
	    p_to16->h_aliases = (SEGPTR)(p_base + (p_aliases - p_to));
	    p_to16->h_addr_list = (SEGPTR)(p_base + (p_addr - p_to));
	}
	else
	{
	    p_to32->h_addrtype = p_he->h_addrtype;
	    p_to32->h_length = p_he->h_length;
	    p_to32->h_name = (p_base + (p_name - p_to));
	    p_to32->h_aliases = (char **)(p_base + (p_aliases - p_to));
	    p_to32->h_addr_list = (char **)(p_base + (p_addr - p_to));
	}

	return size;
}

/* ----- protoent */

static int protoent_size(struct protoent* p_pe)
{
  int size = 0;
  if( p_pe )
  { size  = sizeof(struct protoent);
    size += strlen(p_pe->p_name) + 1;
    size += list_size(p_pe->p_aliases, 0); }
  return size;
}

/* Copy protoent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_pe(char *p_to,char *p_base,int t_size,struct protoent* p_pe, int flag)
{
	char* p_name,*p_aliases,*p;
	struct ws_protoent16 *p_to16 = (struct ws_protoent16*)p_to;
	WS_protoent *p_to32 = (WS_protoent*)p_to;
	int	size = protoent_size(p_pe) +
		(
		(flag & AQ_WIN16) ? sizeof(struct ws_protoent16) : sizeof(WS_protoent)
		- sizeof(struct protoent)
		);

	if (t_size < size)
		return -size;
	p = p_to;
	p += (flag & AQ_WIN16) ?
		sizeof(struct ws_protoent16) : sizeof(WS_protoent);
	p_name = p;
	strcpy(p, p_pe->p_name); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_pe->p_aliases, p, p_base + (p - (char*)p_to), 0);

	if (flag & AQ_WIN16)
	{
	    p_to16->p_proto = (INT16)p_pe->p_proto;
	    p_to16->p_name = (SEGPTR)(p_base) + (p_name - p_to);
	    p_to16->p_aliases = (SEGPTR)((p_base) + (p_aliases - p_to));
	}
	else
	{
	    p_to32->p_proto = p_pe->p_proto;
	    p_to32->p_name = (p_base) + (p_name - p_to);
	    p_to32->p_aliases = (char **)((p_base) + (p_aliases - p_to));
	}

	return size;
}

/* ----- servent */

static int servent_size(struct servent* p_se)
{
	int size = 0;
	if( p_se ) {
		size += sizeof(struct servent);
		size += strlen(p_se->s_proto) + strlen(p_se->s_name) + 2;
		size += list_size(p_se->s_aliases, 0);
	}
	return size;
}

/* Copy servent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 * Take care of different Win16/Win32 servent structs (packing !)
 */
static int WS_copy_se(char *p_to,char *p_base,int t_size,struct servent* p_se, int flag)
{
	char* p_name,*p_aliases,*p_proto,*p;
	struct ws_servent16 *p_to16 = (struct ws_servent16*)p_to;
	WS_servent *p_to32 = (WS_servent*)p_to;
	int	size = servent_size(p_se) +
		(
		(flag & AQ_WIN16) ? sizeof(struct ws_servent16) : sizeof(WS_servent)
		- sizeof(struct servent)
		);

	if (t_size < size)
		return -size;
	p = p_to;
	p += (flag & AQ_WIN16) ?
		sizeof(struct ws_servent16) : sizeof(WS_servent);
	p_name = p;
	strcpy(p, p_se->s_name); p += strlen(p) + 1;
	p_proto = p;
	strcpy(p, p_se->s_proto); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_se->s_aliases, p, p_base + (p - p_to), 0);

	if (flag & AQ_WIN16)
	{
	    p_to16->s_port = (INT16)p_se->s_port;
	    p_to16->s_name = (SEGPTR)(p_base + (p_name - p_to));
	    p_to16->s_proto = (SEGPTR)(p_base + (p_proto - p_to));
	    p_to16->s_aliases = (SEGPTR)(p_base + (p_aliases - p_to));
	}
	else
	{
	    p_to32->s_port = p_se->s_port;
	    p_to32->s_name = (p_base + (p_name - p_to));
	    p_to32->s_proto = (p_base + (p_proto - p_to));
	    p_to32->s_aliases = (char **)(p_base + (p_aliases - p_to));
	}

	return size;
}

static HANDLE16 __ws_async_handle = 0xdead;

/* Generic async query struct. we use symbolic names for the different queries
 * for readability.
 */
typedef struct _async_query {
	HWND16		hWnd;
	UINT16		uMsg;
	LPCSTR		ptr1;
#define host_name	ptr1
#define host_addr	ptr1
#define serv_name	ptr1
#define proto_name	ptr1
	LPCSTR		ptr2;
#define serv_proto	ptr2
	int		int1;
#define host_len	int1
#define proto_number	int1
#define serv_port	int1
	int		int2;
#define host_type	int2
	SEGPTR		sbuf;
	INT16		sbuflen;

	HANDLE16	async_handle;
	int		flags;
	int		qt;
    char xbuf[1];
} async_query;


/****************************************************************************
 * The async query function.
 *
 * It is either called as a thread startup routine or directly. It has
 * to free the passed arg from the process heap and PostMessageA the async
 * result or the error code.
 *
 * FIXME:
 *	- errorhandling not verified.
 */
static DWORD WINAPI _async_queryfun(LPVOID arg) {
	async_query	*aq = (async_query*)arg;
	int		size = 0;
	WORD		fail = 0;
	char		*targetptr = (HB_WIN32(aq)?(char*)aq->sbuf:0/*(char*)MapSL(aq->sbuf)*/);

	switch (aq->flags & AQ_GETMASK) {
	case AQ_GETHOST: {
			struct hostent *he;
			char *copy_hostent = targetptr;
                        char buf[100];
                        if( !(aq->host_name)) {
                            aq->host_name = buf;
                            if( gethostname( buf, 100) == -1) {
                                fail = WSAENOBUFS; /* appropriate ? */
                                break;
                            }
                        }
			he = (aq->flags & AQ_NAME) ?
				gethostbyname(aq->host_name):
				gethostbyaddr(aq->host_addr,aq->host_len,aq->host_type);
                        if (!he) fail = WSAGetLastError();
			if (he) {
				size = WS_copy_he(copy_hostent,(char*)aq->sbuf,aq->sbuflen,he,aq->flags);
				if (size < 0) {
					fail = WSAENOBUFS;
					size = -size;
				}
			}
		}
		break;
	case AQ_GETPROTO: {
#if defined(HAVE_GETPROTOBYNAME) && defined(HAVE_GETPROTOBYNUMBER)
			struct protoent *pe;
			char *copy_protoent = targetptr;
			pe = (aq->flags & AQ_NAME)?
				getprotobyname(aq->proto_name) :
				getprotobynumber(aq->proto_number);
			if (pe) {
				size = WS_copy_pe(copy_protoent,(char*)aq->sbuf,aq->sbuflen,pe,aq->flags);
				if (size < 0) {
					fail = WSAENOBUFS;
					size = -size;
				}
			} else {
                            if (aq->flags & AQ_NAME)
                                MESSAGE("protocol %s not found; You might want to add "
                                        "this to /etc/protocols\n", debugstr_a(aq->proto_name) );
                            else
                                MESSAGE("protocol number %d not found; You might want to add "
                                        "this to /etc/protocols\n", aq->proto_number );
                            fail = WSANO_DATA;
			}
#else
                        fail = WSANO_DATA;
#endif
		}
		break;
	case AQ_GETSERV: {
			struct servent	*se;
			char *copy_servent = targetptr;
			se = (aq->flags & AQ_NAME)?
				getservbyname(aq->serv_name,aq->serv_proto) :
#ifdef HAVE_GETSERVBYPORT
				getservbyport(aq->serv_port,aq->serv_proto);
#else
                                NULL;
#endif
			if (se) {
				size = WS_copy_se(copy_servent,(char*)aq->sbuf,aq->sbuflen,se,aq->flags);
				if (size < 0) {
					fail = WSAENOBUFS;
					size = -size;
				}
			} else {
                            if (aq->flags & AQ_NAME)
                                MESSAGE("service %s protocol %s not found; You might want to add "
                                        "this to /etc/services\n", debugstr_a(aq->serv_name) ,
                                        aq->serv_proto ? debugstr_a(aq->serv_proto ):"*");
                            else
                                MESSAGE("service on port %d protocol %s not found; You might want to add "
                                        "this to /etc/services\n", aq->serv_port,
                                        aq->serv_proto ? debugstr_a(aq->serv_proto ):"*");
                            fail = WSANO_DATA;
			}
		}
		break;
	}
	PostMessageA(HWND_32(aq->hWnd),aq->uMsg,(WPARAM) aq->async_handle,size|(fail<<16));
	HeapFree(GetProcessHeap(),0,arg);
	return 0;
}

/****************************************************************************
 * The main async help function.
 *
 * It either starts a thread or just calls the function directly for platforms
 * with no thread support. This relies on the fact that PostMessage() does
 * not actually call the windowproc before the function returns.
 */
static HANDLE16	__WSAsyncDBQuery(
	HWND hWnd, UINT uMsg,INT int1,LPCSTR ptr1, INT int2, LPCSTR ptr2,
	void *sbuf, INT sbuflen, UINT flags
)
{
        async_query*	aq;
	char*		pto;
	LPCSTR		pfm;
	int		xbuflen = 0;

	/* allocate buffer to copy protocol- and service name to */
	/* note: this is done in the calling thread so we can return */
	/* a decent error code if the Alloc fails */

	switch (flags & AQ_MASKPTR1) {
	case 0:							break;
	case AQ_COPYPTR1:	xbuflen += int1;		break;
	case AQ_DUPLOWPTR1:	xbuflen += strlen(ptr1) + 1;	break;
	}

	switch (flags & AQ_MASKPTR2) {
	case 0:							break;
	case AQ_COPYPTR2:	xbuflen += int2;		break;
	case AQ_DUPLOWPTR2:	xbuflen += strlen(ptr2) + 1;	break;
	}

	if(!(aq = HeapAlloc(GetProcessHeap(),0,sizeof(async_query) + xbuflen))) {
	        SetLastError(WSAEWOULDBLOCK); /* insufficient resources */
		return 0;
	}

	pto = aq->xbuf;
	if (ptr1) switch (flags & AQ_MASKPTR1) {
	case 0:											break;
	case AQ_COPYPTR1:   memcpy(pto, ptr1, int1); ptr1 = pto; pto += int1; 			break;
	case AQ_DUPLOWPTR1: pfm = ptr1; ptr1 = pto; do *pto++ = tolower(*pfm); while (*pfm++);	break;
	}
	if (ptr2) switch (flags & AQ_MASKPTR2) {
	case 0:											break;
	case AQ_COPYPTR2:   memcpy(pto, ptr2, int2); ptr2 = pto; pto += int2;			break;
	case AQ_DUPLOWPTR2: pfm = ptr2; ptr2 = pto; do *pto++ = tolower(*pfm); while (*pfm++);	break;
	}

	aq->hWnd	= HWND_16(hWnd);
	aq->uMsg	= uMsg;
	aq->int1	= int1;
	aq->ptr1	= ptr1;
	aq->int2	= int2;
	aq->ptr2	= ptr2;
	/* avoid async_handle = 0 */
	aq->async_handle = (++__ws_async_handle ? __ws_async_handle : ++__ws_async_handle);
	aq->flags	= flags;
	aq->sbuf	= (SEGPTR)sbuf;
	aq->sbuflen	= sbuflen;

#if 1
	if (CreateThread(NULL,0,_async_queryfun,aq,0,NULL) == INVALID_HANDLE_VALUE)
#endif
		_async_queryfun(aq);
	return __ws_async_handle;
}


/***********************************************************************
 *       WSAAsyncGetHostByAddr	(WINSOCK.102)
 */
HANDLE16 WINAPI WSAAsyncGetHostByAddr16(HWND16 hWnd, UINT16 uMsg, LPCSTR addr,
                               INT16 len, INT16 type, SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, addr %08x[%i]\n",
	       hWnd, uMsg, (unsigned)addr , len );
	return __WSAsyncDBQuery(HWND_32(hWnd),uMsg,len,addr,type,NULL,
				(void*)sbuf,buflen,
				AQ_NUMBER|AQ_COPYPTR1|AQ_WIN16|AQ_GETHOST);
}

/***********************************************************************
 *       WSAAsyncGetHostByAddr        (WS2_32.102)
 */
HANDLE WINAPI WSAAsyncGetHostByAddr(HWND hWnd, UINT uMsg, LPCSTR addr,
                               INT len, INT type, LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %p, msg %04x, addr %08x[%i]\n",
	       hWnd, uMsg, (unsigned)addr , len );
	return WSA_H32( __WSAsyncDBQuery(hWnd,uMsg,len,addr,type,NULL,sbuf,buflen,
				AQ_NUMBER|AQ_COPYPTR1|AQ_WIN32|AQ_GETHOST));
}

/***********************************************************************
 *       WSAAsyncGetHostByName	(WINSOCK.103)
 */
HANDLE16 WINAPI WSAAsyncGetHostByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name,
                                      SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, host %s, buffer %i\n",
	      hWnd, uMsg, (name)?name:"<null>", (int)buflen );
	return __WSAsyncDBQuery(HWND_32(hWnd),uMsg,0,name,0,NULL,
				(void*)sbuf,buflen,
				AQ_NAME|AQ_DUPLOWPTR1|AQ_WIN16|AQ_GETHOST);
}

/***********************************************************************
 *       WSAAsyncGetHostByName	(WS2_32.103)
 */
HANDLE WINAPI WSAAsyncGetHostByName(HWND hWnd, UINT uMsg, LPCSTR name,
					LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %p, msg %08x, host %s, buffer %i\n",
	       hWnd, uMsg, (name)?name:"<null>", (int)buflen );
	return WSA_H32( __WSAsyncDBQuery(hWnd,uMsg,0,name,0,NULL,sbuf,buflen,
				AQ_NAME|AQ_DUPLOWPTR1|AQ_WIN32|AQ_GETHOST));
}

/***********************************************************************
 *       WSAAsyncGetProtoByName	(WINSOCK.105)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name,
                                         SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %08x, protocol %s\n",
	       hWnd, uMsg, (name)?name:"<null>" );
	return __WSAsyncDBQuery(HWND_32(hWnd),uMsg,0,name,0,NULL,
				(void*)sbuf,buflen,
				AQ_NAME|AQ_DUPLOWPTR1|AQ_WIN16|AQ_GETPROTO);
}

/***********************************************************************
 *       WSAAsyncGetProtoByName       (WS2_32.105)
 */
HANDLE WINAPI WSAAsyncGetProtoByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                         LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %p, msg %08x, protocol %s\n",
	       hWnd, uMsg, (name)?name:"<null>" );
	return WSA_H32( __WSAsyncDBQuery(hWnd,uMsg,0,name,0,NULL,sbuf,buflen,
				AQ_NAME|AQ_DUPLOWPTR1|AQ_WIN32|AQ_GETPROTO));
}


/***********************************************************************
 *       WSAAsyncGetProtoByNumber	(WINSOCK.104)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByNumber16(HWND16 hWnd,UINT16 uMsg,INT16 number,
                                           SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, num %i\n", hWnd, uMsg, number );
	return __WSAsyncDBQuery(HWND_32(hWnd),uMsg,number,NULL,0,NULL,
				(void*)sbuf,buflen,
				AQ_GETPROTO|AQ_NUMBER|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetProtoByNumber     (WS2_32.104)
 */
HANDLE WINAPI WSAAsyncGetProtoByNumber(HWND hWnd, UINT uMsg, INT number,
                                           LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %p, msg %04x, num %i\n", hWnd, uMsg, number );
	return WSA_H32( __WSAsyncDBQuery(hWnd,uMsg,number,NULL,0,NULL,sbuf,buflen,
				AQ_GETPROTO|AQ_NUMBER|AQ_WIN32));
}

/***********************************************************************
 *       WSAAsyncGetServByName	(WINSOCK.107)
 */
HANDLE16 WINAPI WSAAsyncGetServByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name,
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, name %s, proto %s\n",
	       hWnd, uMsg, (name)?name:"<null>", (proto)?proto:"<null>");
	return __WSAsyncDBQuery(HWND_32(hWnd),uMsg,0,name,0,proto,
				(void*)sbuf,buflen,
				AQ_GETSERV|AQ_NAME|AQ_DUPLOWPTR1|AQ_DUPLOWPTR2|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetServByName        (WS2_32.107)
 */
HANDLE WINAPI WSAAsyncGetServByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %p, msg %04x, name %s, proto %s\n",
	       hWnd, uMsg, (name)?name:"<null>", (proto)?proto:"<null>");
	return WSA_H32( __WSAsyncDBQuery(hWnd,uMsg,0,name,0,proto,sbuf,buflen,
				AQ_GETSERV|AQ_NAME|AQ_DUPLOWPTR1|AQ_DUPLOWPTR2|AQ_WIN32));
}

/***********************************************************************
 *       WSAAsyncGetServByPort	(WINSOCK.106)
 */
HANDLE16 WINAPI WSAAsyncGetServByPort16(HWND16 hWnd, UINT16 uMsg, INT16 port,
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, port %i, proto %s\n",
	       hWnd, uMsg, port, (proto)?proto:"<null>" );
	return __WSAsyncDBQuery(HWND_32(hWnd),uMsg,port,NULL,0,proto,
				(void*)sbuf,buflen,
				AQ_GETSERV|AQ_NUMBER|AQ_DUPLOWPTR2|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetServByPort        (WS2_32.106)
 */
HANDLE WINAPI WSAAsyncGetServByPort(HWND hWnd, UINT uMsg, INT port,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %p, msg %04x, port %i, proto %s\n",
	       hWnd, uMsg, port, (proto)?proto:"<null>" );
	return WSA_H32( __WSAsyncDBQuery(hWnd,uMsg,port,NULL,0,proto,sbuf,buflen,
				AQ_GETSERV|AQ_NUMBER|AQ_DUPLOWPTR2|AQ_WIN32));
}
