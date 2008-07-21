/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/tcpip.h
 * PURPOSE:     TCP/IP protocol driver definitions
 * NOTES:       Spin lock acquire order:
 *                - Net table list lock
 *                - Interface lock
 *                - Interface list lock
 *                - Prefix list lock
 *                - Neighbor cache lock
 *                - Route cache lock
 */
#ifndef __TCPIP_H
#define __TCPIP_H

#ifdef _MSC_VER
#include <basetsd.h>
#include <ntddk.h>
#include <windef.h>
#include <ndis.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#else
#include <ntddk.h>
#include <ndis.h>
#include <tdikrnl.h>
#include <tdiinfo.h>
#endif

#include <debug.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_STRING	TAG('S', 'T', 'R', ' ') /* string */

/* Define _NTTEST_ to make test version. Device names are prefixed with
   'NT' to allow the driver to run side by side with MS TCP/IP driver */
//#define _NTTEST_

/* FIXME: The following should be moved to ntddk.h or tdi headers */
#ifndef _MSC_VER

#ifndef IO_NETWORK_INCREMENT
#define IO_NETWORK_INCREMENT 2
#endif

#endif

#ifdef _MSC_VER
/* EXPORTED is already defined ddk/defines.h */
#define EXPORTED __declspec(dllexport)

#endif

#include <titypes.h>
#include <ticonsts.h>

/* Macros */

#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#define NDIS_BUFFER_TAG FOURCC('n','b','u','f')
#define NDIS_PACKET_TAG FOURCC('n','p','k','t')

#ifdef i386

/* DWORD network to host byte order conversion for i386 */
#define DN2H(dw) \
    ((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* DWORD host to network byte order conversion for i386 */
#define DH2N(dw) \
	((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* WORD network to host order conversion for i386 */
#define WN2H(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

/* WORD host to network byte order conversion for i386 */
#define WH2N(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

#else /* i386 */

/* DWORD network to host byte order conversion for other architectures */
#define DN2H(dw) \
    (dw)

/* DWORD host to network byte order conversion for other architectures */
#define DH2N(dw) \
    (dw)

/* WORD network to host order conversion for other architectures */
#define WN2H(w) \
    (w)

/* WORD host to network byte order conversion for other architectures */
#define WH2N(w) \
    (w)

#endif /* i386 */

/* AF_INET and other things Arty likes to use ;) */
#define AF_INET 2
#define SOCK_STREAM 1

/* Should use TDI structure, but Arty wants to keep BSD style */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
struct in_addr
{
    union
    {
        struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
        struct { u_short s_w1,s_w2; } S_un_w;
        u_long S_addr;
    } S_un;
#define s_addr  S_un.S_addr
#define s_host  S_un.S_un_b.s_b2
#define s_net   S_un.S_un_b.s_b1
#define s_imp   S_un.S_un_w.s_w2
#define s_impno S_un.S_un_b.s_b4
#define s_lh    S_un.S_un_b.s_b3
};
struct sockaddr_in
{
    short sin_family;
    u_short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};
typedef struct sockaddr_in SOCKADDR_IN;
struct sockaddr
{
    u_short sa_family;
    char sa_data[14];
};

typedef TDI_STATUS (*InfoRequest_f)( UINT InfoClass,
				     UINT InfoType,
				     UINT InfoId,
				     PVOID Context,
				     TDIEntityID *id,
				     PNDIS_BUFFER Buffer,
				     PUINT BufferSize );

typedef TDI_STATUS (*InfoSet_f)( UINT InfoClass,
				 UINT InfoType,
				 UINT InfoId,
				 PVOID Context,
				 TDIEntityID *id,
				 PCHAR Buffer,
				 UINT BufferSize );

/* Sufficient information to manage the entity list */
typedef struct {
    UINT tei_entity;
    UINT tei_instance;
    PVOID context;
    InfoRequest_f info_req;
    InfoSet_f info_set;
} TDIEntityInfo;

#ifndef htons
#define htons(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))
#endif

/* Global variable */
extern PDEVICE_OBJECT TCPDeviceObject;
extern PDEVICE_OBJECT UDPDeviceObject;
extern PDEVICE_OBJECT IPDeviceObject;
extern PDEVICE_OBJECT RawIPDeviceObject;
extern LIST_ENTRY InterfaceListHead;
extern KSPIN_LOCK InterfaceListLock;
extern LIST_ENTRY AddressFileListHead;
extern KSPIN_LOCK AddressFileListLock;
extern NDIS_HANDLE GlobalPacketPool;
extern NDIS_HANDLE GlobalBufferPool;
extern KSPIN_LOCK EntityListLock;
extern TDIEntityInfo *EntityList;
extern ULONG EntityCount;
extern ULONG EntityMax;

extern NTSTATUS TiGetProtocolNumber( PUNICODE_STRING FileName,
				     PULONG Protocol );

#endif /* __TCPIP_H */

/* EOF */
