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
#include <ddk/ntddk.h>
#include <net/ndis.h>
#include <net/tdikrnl.h>
#include <net/tdiinfo.h>
#endif

#include <debug.h>


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
#include <udp.h>


/* Macros */

#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)


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
extern TDIEntityID *EntityList;
extern ULONG EntityCount;
extern UDP_STATISTICS UDPStats;

#endif /* __TCPIP_H */

/* EOF */
