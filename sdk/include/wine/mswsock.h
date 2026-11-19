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
#ifndef _MSWSOCK_
#define _MSWSOCK_

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef USE_WS_PREFIX
#define WS(x)    WS_##x
#else
#define WS(x)    x
#endif

#ifndef USE_WS_PREFIX
#define SO_CONNDATA        0x7000
#define SO_CONNOPT         0x7001
#define SO_DISCDATA        0x7002
#define SO_DISCOPT         0x7003
#define SO_CONNDATALEN     0x7004
#define SO_CONNOPTLEN      0x7005
#define SO_DISCDATALEN     0x7006
#define SO_DISCOPTLEN      0x7007
#else
#define WS_SO_CONNDATA     0x7000
#define WS_SO_CONNOPT      0x7001
#define WS_SO_DISCDATA     0x7002
#define WS_SO_DISCOPT      0x7003
#define WS_SO_CONNDATALEN  0x7004
#define WS_SO_CONNOPTLEN   0x7005
#define WS_SO_DISCDATALEN  0x7006
#define WS_SO_DISCOPTLEN   0x7007
#endif

#ifndef USE_WS_PREFIX
#define SO_OPENTYPE     0x7008
#else
#define WS_SO_OPENTYPE  0x7008
#endif

#ifndef USE_WS_PREFIX
#define SO_SYNCHRONOUS_ALERT       0x10
#define SO_SYNCHRONOUS_NONALERT    0x20
#else
#define WS_SO_SYNCHRONOUS_ALERT    0x10
#define WS_SO_SYNCHRONOUS_NONALERT 0x20
#endif

#ifndef USE_WS_PREFIX
#define SO_MAXDG                      0x7009
#define SO_MAXPATHDG                  0x700A
#define SO_UPDATE_ACCEPT_CONTEXT      0x700B
#define SO_CONNECT_TIME               0x700C
#define SO_UPDATE_CONNECT_CONTEXT     0x7010
#else
#define WS_SO_MAXDG                   0x7009
#define WS_SO_MAXPATHDG               0x700A
#define WS_SO_UPDATE_ACCEPT_CONTEXT   0x700B
#define WS_SO_CONNECT_TIME            0x700C
#define WS_SO_UPDATE_CONNECT_CONTEXT  0x7010
#endif

#ifndef USE_WS_PREFIX
#define TCP_BSDURGENT              0x7000
#else
#define WS_TCP_BSDURGENT              0x7000
#endif

#ifndef USE_WS_PREFIX
#define SIO_UDP_CONNRESET               _WSAIOW(IOC_VENDOR, 12)
#define SIO_SET_COMPATIBILITY_MODE      _WSAIOW(IOC_VENDOR, 300)
#define SIO_BASE_HANDLE                 _WSAIOR(IOC_WS2, 34)
#else
#define WS_SIO_UDP_CONNRESET            _WSAIOW(WS_IOC_VENDOR, 12)
#define WS_SIO_SET_COMPATIBILITY_MODE   _WSAIOW(WS_IOC_VENDOR, 300)
#define WS_SIO_BASE_HANDLE              _WSAIOR(WS_IOC_WS2, 34)
#endif

#define DE_REUSE_SOCKET TF_REUSE_SOCKET

#ifndef USE_WS_PREFIX
#define MSG_TRUNC   0x0100
#define MSG_CTRUNC  0x0200
#define MSG_BCAST   0x0400
#define MSG_MCAST   0x0800
#else
#define WS_MSG_TRUNC   0x0100
#define WS_MSG_CTRUNC  0x0200
#define WS_MSG_BCAST   0x0400
#define WS_MSG_MCAST   0x0800
#endif

#define TF_DISCONNECT          0x01
#define TF_REUSE_SOCKET        0x02
#define TF_WRITE_BEHIND        0x04
#define TF_USE_DEFAULT_WORKER  0x00
#define TF_USE_SYSTEM_THREAD   0x10
#define TF_USE_KERNEL_APC      0x20

#define TP_DISCONNECT           TF_DISCONNECT
#define TP_REUSE_SOCKET         TF_REUSE_SOCKET
#define TP_USE_DEFAULT_WORKER   TF_USE_DEFAULT_WORKER
#define TP_USE_SYSTEM_THREAD    TF_USE_SYSTEM_THREAD
#define TP_USE_KERNEL_APC       TF_USE_KERNEL_APC

#define TP_ELEMENT_MEMORY   1
#define TP_ELEMENT_FILE     2
#define TP_ELEMENT_EOP      4

#define WSAID_ACCEPTEX \
	{0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_CONNECTEX \
	{0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
#define WSAID_DISCONNECTEX \
	{0x7fda2e11,0x8630,0x436f,{0xa0,0x31,0xf5,0x36,0xa6,0xee,0xc1,0x57}}
#define WSAID_GETACCEPTEXSOCKADDRS \
	{0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_TRANSMITFILE \
	{0xb5367df0,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_TRANSMITPACKETS \
	{0xd9689da0,0x1f90,0x11d3,{0x99,0x71,0x00,0xc0,0x4f,0x68,0xc8,0x76}}
#define WSAID_WSARECVMSG \
	{0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22}}
#define WSAID_WSASENDMSG \
	{0xa441e712,0x754f,0x43ca,{0x84,0xa7,0x0d,0xee,0x44,0xcf,0x60,0x6d}}

typedef struct _TRANSMIT_FILE_BUFFERS {
    LPVOID  Head;
    DWORD   HeadLength;
    LPVOID  Tail;
    DWORD   TailLength;
} TRANSMIT_FILE_BUFFERS, *PTRANSMIT_FILE_BUFFERS, *LPTRANSMIT_FILE_BUFFERS;

typedef struct _TRANSMIT_PACKETS_ELEMENT {
    ULONG  dwElFlags;
    ULONG  cLength;
    union {
      struct {
	LARGE_INTEGER  nFileOffset;
	HANDLE         hFile;
      } DUMMYSTRUCTNAME;
      PVOID  pBuffer;
    } DUMMYUNIONNAME;
} TRANSMIT_PACKETS_ELEMENT, *PTRANSMIT_PACKETS_ELEMENT, *LPTRANSMIT_PACKETS_ELEMENT;

typedef struct _WSACMSGHDR {
    SIZE_T      cmsg_len;
    INT         cmsg_level;
    INT         cmsg_type;
    /* followed by UCHAR cmsg_data[] */
} WSACMSGHDR, *PWSACMSGHDR, *LPWSACMSGHDR;

typedef WSACMSGHDR CMSGHDR, *PCMSGHDR;

typedef enum _NLA_BLOB_DATA_TYPE {
    NLA_RAW_DATA,
    NLA_INTERFACE,       /* interface name, type and speed */
    NLA_802_1X_LOCATION, /* wireless network info */
    NLA_CONNECTIVITY,    /* status on network connectivity */
    NLA_ICS              /* internet connection sharing */
} NLA_BLOB_DATA_TYPE;

typedef enum _NLA_CONNECTIVITY_TYPE {
    NLA_NETWORK_AD_HOC,  /* private network */
    NLA_NETWORK_MANAGED, /* network managed by domain */
    NLA_NETWORK_UNMANAGED,
    NLA_NETWORK_UNKNOWN
} NLA_CONNECTIVITY_TYPE;

typedef enum _NLA_INTERNET {
    NLA_INTERNET_UNKNOWN, /* can't determine if connected or not */
    NLA_INTERNET_NO,      /* not connected to internet */
    NLA_INTERNET_YES      /* connected to internet */
} NLA_INTERNET;

/* this structure is returned in the lpBlob field during calls to WSALookupServiceNext */
typedef struct _NLA_BLOB {
    /* the header defines the size of the current record and if there is a next record */
    struct {
        NLA_BLOB_DATA_TYPE type;
        DWORD dwSize;
        DWORD nextOffset; /* if it's zero there are no more blobs */
    } header;

    /* the following union interpretation depends on the header.type value
     * from the struct above.
     * the header.dwSize will be the size of all data, specially useful when
     * the last struct field is size [1] */
    union {
        /* NLA_RAW_DATA */
        CHAR rawData[1];

        /* NLA_INTERFACE */
        struct {
            DWORD dwType;
            DWORD dwSpeed;
            CHAR adapterName[1];
        } interfaceData;

        /* NLA_802_1X_LOCATION */
        struct {
            CHAR information[1];
        } locationData;

        /* NLA_CONNECTIVITY */
        struct {
            NLA_CONNECTIVITY_TYPE type;
            NLA_INTERNET internet;
        } connectivity;

        /* NLA_ICS */
        struct {
            struct {
                DWORD speed;
                DWORD type;
                DWORD state;
                WCHAR machineName[256];
                WCHAR sharedAdapterName[256];
            } remote;
        } ICS;
    } data;
} NLA_BLOB, *PNLA_BLOB;

typedef BOOL (WINAPI * LPFN_ACCEPTEX)(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI * LPFN_CONNECTEX)(SOCKET, const struct WS(sockaddr) *, int, PVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI * LPFN_DISCONNECTEX)(SOCKET, LPOVERLAPPED, DWORD, DWORD);
typedef VOID (WINAPI * LPFN_GETACCEPTEXSOCKADDRS)(PVOID, DWORD, DWORD, DWORD, struct WS(sockaddr) **, LPINT, struct WS(sockaddr) **, LPINT);
typedef BOOL (WINAPI * LPFN_TRANSMITFILE)(SOCKET, HANDLE, DWORD, DWORD, LPOVERLAPPED, LPTRANSMIT_FILE_BUFFERS, DWORD);
typedef BOOL (WINAPI * LPFN_TRANSMITPACKETS)(SOCKET, LPTRANSMIT_PACKETS_ELEMENT, DWORD, DWORD, LPOVERLAPPED, DWORD);
typedef INT  (WINAPI * LPFN_WSARECVMSG)(SOCKET, LPWSAMSG, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef INT  (WINAPI * LPFN_WSASENDMSG)(SOCKET, LPWSAMSG, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

BOOL WINAPI AcceptEx(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
VOID WINAPI GetAcceptExSockaddrs(PVOID, DWORD, DWORD, DWORD, struct WS(sockaddr) **, LPINT, struct WS(sockaddr) **, LPINT);
BOOL WINAPI TransmitFile(SOCKET, HANDLE, DWORD, DWORD, LPOVERLAPPED, LPTRANSMIT_FILE_BUFFERS, DWORD);
INT  WINAPI WSARecvEx(SOCKET, char *, INT, INT *);

#ifdef __cplusplus
}
#endif

#undef WS

#endif /* _MSWSOCK_ */
