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
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    PVOID HelperContext;
    DWORD NotificationEvents;
    UNICODE_STRING TdiDeviceName;
    SOCKADDR Name;
} AFD_SOCKET_INFORMATION, *PAFD_SOCKET_INFORMATION;


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

typedef struct _FILE_REQUEST_BIND {
    SOCKADDR Name;
} FILE_REQUEST_BIND, *PFILE_REQUEST_BIND;

typedef struct _FILE_REPLY_BIND {
    INT Status;
    HANDLE TdiAddressObjectHandle;
    HANDLE TdiConnectionObjectHandle;
} FILE_REPLY_BIND, *PFILE_REPLY_BIND;

typedef struct _FILE_REQUEST_LISTEN {
    INT Backlog;
} FILE_REQUEST_LISTEN, *PFILE_REQUEST_LISTEN;

typedef struct _FILE_REPLY_LISTEN {
    INT Status;
} FILE_REPLY_LISTEN, *PFILE_REPLY_LISTEN;


typedef struct _FILE_REQUEST_SENDTO {
    LPWSABUF Buffers;
    DWORD BufferCount;
    DWORD Flags;
    SOCKADDR To;
    INT ToLen;
} FILE_REQUEST_SENDTO, *PFILE_REQUEST_SENDTO;

typedef struct _FILE_REPLY_SENDTO {
    INT Status;
    DWORD NumberOfBytesSent;
} FILE_REPLY_SENDTO, *PFILE_REPLY_SENDTO;


typedef struct _FILE_REQUEST_RECVFROM {
    LPWSABUF Buffers;
    DWORD BufferCount;
    LPDWORD Flags;
    LPSOCKADDR From;
    LPINT FromLen;
} FILE_REQUEST_RECVFROM, *PFILE_REQUEST_RECVFROM;

typedef struct _FILE_REPLY_RECVFROM {
    INT Status;
    DWORD NumberOfBytesRecvd;
} FILE_REPLY_RECVFROM, *PFILE_REPLY_RECVFROM;

#endif /*__AFD_SHARED_H */

/* EOF */
