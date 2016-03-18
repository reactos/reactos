/*
 * Copyright (C) the Wine project
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
 */

#ifndef __WINE_WINE_WINSOCK16_H
#define __WINE_WINE_WINSOCK16_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <pshpack1.h>

typedef UINT16 SOCKET16;

typedef struct
{
        UINT16    fd_count;               /* how many are SET? */
        SOCKET16  fd_array[WS_FD_SETSIZE];   /* an array of SOCKETs */
} ws_fd_set16;

/* ws_hostent16, ws_protoent16, ws_servent16, ws_netent16
 * are 1-byte aligned here ! */
typedef struct ws_hostent16
{
        SEGPTR  h_name;         /* official name of host */
        SEGPTR  h_aliases;      /* alias list */
        INT16   h_addrtype;     /* host address type */
        INT16   h_length;       /* length of address */
        SEGPTR  h_addr_list;    /* list of addresses from name server */
} _ws_hostent16;

typedef struct ws_protoent16
{
        SEGPTR  p_name;         /* official protocol name */
        SEGPTR  p_aliases;      /* alias list */
        INT16   p_proto;        /* protocol # */
} _ws_protoent16;

typedef struct ws_servent16
{
        SEGPTR  s_name;         /* official service name */
        SEGPTR  s_aliases;      /* alias list */
        INT16   s_port;         /* port # */
        SEGPTR  s_proto;        /* protocol to use */
} _ws_servent16;

typedef struct ws_netent16
{
        SEGPTR  n_name;         /* official name of net */
        SEGPTR  n_aliases;      /* alias list */
        INT16   n_addrtype;     /* net address type */
        INT     n_net;          /* network # */
} _ws_netent16;

typedef struct WSAData16
{
    WORD                    wVersion;
    WORD                    wHighVersion;
    char                    szDescription[WSADESCRIPTION_LEN+1];
    char                    szSystemStatus[WSASYS_STATUS_LEN+1];
    WORD                    iMaxSockets;
    WORD                    iMaxUdpDg;
    SEGPTR                  lpVendorInfo;
} WSADATA16, *LPWSADATA16;

#include <poppack.h>

#define WS_FD_CLR16(fd, set)   __WS_FD_CLR((fd),(set), ws_fd_set16)
#define WS_FD_SET16(fd, set)   __WS_FD_SET1((fd),(set), ws_fd_set16)
#define WS_FD_ZERO16(set)      (((ws_fd_set16*)(set))->fd_count=0)
#define WS_FD_ISSET16(fd, set) __WSAFDIsSet16((SOCKET16)(fd), (ws_fd_set16*)(set))

#define INVALID_SOCKET16  ((SOCKET16)(~0))

INT16     WINAPI __WSAFDIsSet16( SOCKET16, ws_fd_set16 * );
INT16     WINAPI WSAStartup16(UINT16 wVersionRequired, LPWSADATA16 lpWSAData);
void      WINAPI WSASetLastError16(INT16 iError);
INT16     WINAPI WSAUnhookBlockingHook16(void);
FARPROC16 WINAPI WSASetBlockingHook16(FARPROC16 lpBlockFunc);
HANDLE16  WINAPI WSAAsyncGetServByName16(HWND16 hWnd, UINT16 wMsg, LPCSTR name, LPCSTR proto,
                                         SEGPTR buf, INT16 buflen);
HANDLE16  WINAPI WSAAsyncGetServByPort16(HWND16 hWnd, UINT16 wMsg, INT16 port,
                                         LPCSTR proto, SEGPTR buf, INT16 buflen);
HANDLE16  WINAPI WSAAsyncGetProtoByName16(HWND16 hWnd, UINT16 wMsg,
                                          LPCSTR name, SEGPTR buf, INT16 buflen);
HANDLE16  WINAPI WSAAsyncGetProtoByNumber16(HWND16 hWnd, UINT16 wMsg,
                                            INT16 number, SEGPTR buf, INT16 buflen);
HANDLE16  WINAPI WSAAsyncGetHostByName16(HWND16 hWnd, UINT16 wMsg,
                                         LPCSTR name, SEGPTR buf, INT16 buflen);
HANDLE16  WINAPI WSAAsyncGetHostByAddr16(HWND16 hWnd, UINT16 wMsg, LPCSTR addr,
                              INT16 len, INT16 type, SEGPTR buf, INT16 buflen);
INT16     WINAPI WSACancelAsyncRequest16(HANDLE16 hAsyncTaskHandle);
INT16     WINAPI WSAAsyncSelect16(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, LONG lEvent);
INT16     WINAPI WSARecvEx16(SOCKET16 s, char *buf, INT16 len, INT16 *flags);

#endif /* __WINE_WINE_WINSOCK16_H */
