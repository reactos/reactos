/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        include/afd/shared.h
 * PURPOSE:     Shared definitions for AFD.SYS and MSAFD.DLL
 */
#ifndef __AFD_SHARED_H
#define __AFD_SHARED_H

#define AfdSocket           "AfdSocket"
#define AFD_SOCKET_LENGTH   (sizeof(AfdSocket) - 1)

typedef struct _AFD_SOCKET_INFORMATION {
  BOOL CommandChannel;
  INT AddressFamily;
  INT SocketType;
  INT Protocol;
  PVOID HelperContext;
  DWORD NotificationEvents;
  UNICODE_STRING TdiDeviceName;
  SOCKADDR Name;
} __attribute__((packed)) AFD_SOCKET_INFORMATION, *PAFD_SOCKET_INFORMATION;


/* AFD IOCTL code definitions */

#define FSCTL_AFD_BASE     FILE_DEVICE_NAMED_PIPE // ???

#define AFD_CTL_CODE(Function, Method, Access) \
  CTL_CODE(FSCTL_AFD_BASE, Function, Method, Access)

#define IOCTL_AFD_BIND \
  AFD_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_LISTEN \
  AFD_CTL_CODE(1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_SENDTO \
  AFD_CTL_CODE(2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_RECVFROM \
  AFD_CTL_CODE(3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_SELECT \
  AFD_CTL_CODE(4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_EVENTSELECT \
  AFD_CTL_CODE(5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_ENUMNETWORKEVENTS \
  AFD_CTL_CODE(6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_RECV \
  AFD_CTL_CODE(7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_SEND \
  AFD_CTL_CODE(8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_ACCEPT \
  AFD_CTL_CODE(9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_CONNECT \
  AFD_CTL_CODE(10, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _FILE_REQUEST_BIND {
  SOCKADDR Name;
} __attribute__((packed)) FILE_REQUEST_BIND, *PFILE_REQUEST_BIND;

typedef struct _FILE_REPLY_BIND {
  INT Status;
  HANDLE TdiAddressObjectHandle;
  HANDLE TdiConnectionObjectHandle;
} __attribute__((packed)) FILE_REPLY_BIND, *PFILE_REPLY_BIND;

typedef struct _FILE_REQUEST_LISTEN {
  INT Backlog;
} __attribute__((packed)) FILE_REQUEST_LISTEN, *PFILE_REQUEST_LISTEN;

typedef struct _FILE_REPLY_LISTEN {
  INT Status;
} __attribute__((packed)) FILE_REPLY_LISTEN, *PFILE_REPLY_LISTEN;


typedef struct _FILE_REQUEST_SENDTO {
  LPWSABUF Buffers;
  DWORD BufferCount;
  DWORD Flags;
  SOCKADDR To;
  INT ToLen;
} __attribute__((packed)) FILE_REQUEST_SENDTO, *PFILE_REQUEST_SENDTO;

typedef struct _FILE_REPLY_SENDTO {
  INT Status;
  DWORD NumberOfBytesSent;
} __attribute__((packed)) FILE_REPLY_SENDTO, *PFILE_REPLY_SENDTO;


typedef struct _FILE_REQUEST_RECVFROM {
  LPWSABUF Buffers;
  DWORD BufferCount;
  LPDWORD Flags;
  LPSOCKADDR From;
  LPINT FromLen;
} __attribute__((packed)) FILE_REQUEST_RECVFROM, *PFILE_REQUEST_RECVFROM;

typedef struct _FILE_REPLY_RECVFROM {
  INT Status;
  DWORD NumberOfBytesRecvd;
} __attribute__((packed)) FILE_REPLY_RECVFROM, *PFILE_REPLY_RECVFROM;


typedef struct _FILE_REQUEST_SELECT {
  LPFD_SET ReadFDSet;
  LPFD_SET WriteFDSet;
  LPFD_SET ExceptFDSet;
  TIMEVAL Timeout;
} __attribute__((packed)) FILE_REQUEST_SELECT, *PFILE_REQUEST_SELECT;

typedef struct _FILE_REPLY_SELECT {
  INT Status;
  DWORD SocketCount;
} __attribute__((packed)) FILE_REPLY_SELECT, *PFILE_REPLY_SELECT;


typedef struct _FILE_REQUEST_EVENTSELECT {
  WSAEVENT hEventObject;
  LONG lNetworkEvents;
} __attribute__((packed)) FILE_REQUEST_EVENTSELECT, *PFILE_REQUEST_EVENTSELECT;

typedef struct _FILE_REPLY_EVENTSELECT {
  INT Status;
} __attribute__((packed)) FILE_REPLY_EVENTSELECT, *PFILE_REPLY_EVENTSELECT;


typedef struct _FILE_REQUEST_ENUMNETWORKEVENTS {
  WSAEVENT hEventObject;
} __attribute__((packed)) FILE_REQUEST_ENUMNETWORKEVENTS, *PFILE_REQUEST_ENUMNETWORKEVENTS;

typedef struct _FILE_REPLY_ENUMNETWORKEVENTS {
  INT Status;
  WSANETWORKEVENTS NetworkEvents;
} __attribute__((packed)) FILE_REPLY_ENUMNETWORKEVENTS, *PFILE_REPLY_ENUMNETWORKEVENTS;


typedef struct _FILE_REQUEST_RECV {
  LPWSABUF Buffers;
  DWORD BufferCount;
  LPDWORD Flags;
} __attribute__((packed)) FILE_REQUEST_RECV, *PFILE_REQUEST_RECV;

typedef struct _FILE_REPLY_RECV {
  INT Status;
  DWORD NumberOfBytesRecvd;
} __attribute__((packed)) FILE_REPLY_RECV, *PFILE_REPLY_RECV;


typedef struct _FILE_REQUEST_SEND {
  LPWSABUF Buffers;
  DWORD BufferCount;
  DWORD Flags;
} __attribute__((packed)) FILE_REQUEST_SEND, *PFILE_REQUEST_SEND;

typedef struct _FILE_REPLY_SEND {
  INT Status;
  DWORD NumberOfBytesSent;
} __attribute__((packed)) FILE_REPLY_SEND, *PFILE_REPLY_SEND;


typedef struct _FILE_REQUEST_ACCEPT {
  LPSOCKADDR addr;
  INT addrlen;
  LPCONDITIONPROC lpfnCondition;
  DWORD dwCallbackData;
} __attribute__((packed)) FILE_REQUEST_ACCEPT, *PFILE_REQUEST_ACCEPT;

typedef struct _FILE_REPLY_ACCEPT {
  INT Status;
  INT addrlen;
  SOCKET Socket;
} __attribute__((packed)) FILE_REPLY_ACCEPT, *PFILE_REPLY_ACCEPT;


typedef struct _FILE_REQUEST_CONNECT {
  LPSOCKADDR name;
  INT namelen;
  LPWSABUF lpCallerData;
  LPWSABUF lpCalleeData;
  LPQOS lpSQOS;
  LPQOS lpGQOS;
} __attribute__((packed)) FILE_REQUEST_CONNECT, *PFILE_REQUEST_CONNECT;

typedef struct _FILE_REPLY_CONNECT {
  INT Status;
} __attribute__((packed)) FILE_REPLY_CONNECT, *PFILE_REPLY_CONNECT;

#endif /*__AFD_SHARED_H */

/* EOF */
