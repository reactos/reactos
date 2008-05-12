/* Copyright (c) 2003 Juan Lang
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
#ifndef __WINE_NETBIOS_H__
#define __WINE_NETBIOS_H__

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "lm.h"
#include "nb30.h"

/* This file describes the interface WINE's NetBIOS implementation uses to
 * interact with a transport implementation (where a transport might be
 * NetBIOS-over-TCP/IP (aka NetBT, NBT), NetBIOS-over-IPX, etc.)
 */

/**
 * Public functions
 */

void NetBIOSInit(void);
void NetBIOSShutdown(void);

struct _NetBIOSTransport;

/* A transport should register itself during its init function (see below) with
 * a unique id (the transport_id of ACTION_HEADER, for example) and an
 * implementation.  Returns TRUE on success, and FALSE on failure.
 */
BOOL NetBIOSRegisterTransport(ULONG id, struct _NetBIOSTransport *transport);

/* Registers an adapter with the given transport and ifIndex with NetBIOS.
 * ifIndex is an interface index usable by the IpHlpApi.  ifIndex is not
 * required to be unique, but is required so that NetWkstaTransportEnum can use
 * GetIfEntry to get the name and hardware address of the adapter.
 * Returns TRUE on success, FALSE on failure.
 * FIXME: need functions for retrieving the name and hardware index, rather
 * than assuming a correlation with IpHlpApi.
 */
BOOL NetBIOSRegisterAdapter(ULONG transport, DWORD ifIndex, void *adapter);

/* During enumeration, all adapters from your transport are disabled
 * internally.  If an adapter is still valid, reenable it with this function.
 * Adapters you don't enable will have their transport's NetBIOSCleanupAdapter
 * function (see below) called on them, and will be removed from the table.
 * (This is to deal with lack of plug-and-play--sorry.)
 */
void NetBIOSEnableAdapter(UCHAR lana);

/* Gets a quick count of the number of NetBIOS adapters.  Not guaranteed not
 * to change from one call to the next, depending on what's been enumerated
 * lately.  See also NetBIOSEnumAdapters.
 */
UCHAR NetBIOSNumAdapters(void);

typedef struct _NetBIOSAdapterImpl {
    UCHAR lana;
    DWORD ifIndex;
    void *data;
} NetBIOSAdapterImpl;

typedef BOOL (*NetBIOSEnumAdaptersCallback)(UCHAR totalLANAs, UCHAR lanaIndex,
 ULONG transport, const NetBIOSAdapterImpl *data, void *closure);

/* Enumerates all NetBIOS adapters for the transport transport, or for all
 * transports if transport is ALL_TRANSPORTS.  Your callback will be called
 * once for every enumerated adapter, with a count of how many adapters have
 * been enumerated, a 0-based index relative to that count, the adapter's
 * transport, and its ifIndex.
 * Your callback should return FALSE if it no longer wishes to be called.
 */
void NetBIOSEnumAdapters(ULONG transport, NetBIOSEnumAdaptersCallback cb,
 void *closure);

/* Hangs up the session identified in the NCB; the NCB need not be a NCBHANGUP.
 * Will result in the transport's hangup function being called, so release any
 * locks you own before calling to avoid deadlock.
 * This function is intended for use by a transport, if the session is closed
 * by some error in the transport layer.
 */
void NetBIOSHangupSession(const NCB *ncb);

/**
 * Functions a transport implementation must implement
 */

/* This function is called to ask a transport implementation to enumerate any
 * LANAs into the NetBIOS adapter table by:
 * - calling NetBIOSRegisterAdapter for any new adapters
 * - calling NetBIOSEnableAdapter for any existing adapters
 * NetBIOSEnumAdapters (see) may be of use to determine which adapters already
 * exist.
 * A transport can assume no other thread is modifying the NetBIOS adapter
 * table during the lifetime of its NetBIOSEnum function (and, therefore, that
 * this function won't be called reentrantly).
 */
typedef UCHAR (*NetBIOSEnum)(void);

/* A cleanup function for a transport.  This is the last function called on a
 * transport.
 */
typedef void (*NetBIOSCleanup)(void);

/* Adapter functions */

/* Functions with direct mappings to the Netbios interface.  These functions
 * are expected to be synchronous, although the first four bytes of the
 * reserved member of the ncb are a cancel flag.  A long-running function
 * should check whether this is not FALSE from time to time (see the
 * NCB_CANCELLED macro), and return NRC_CMDCAN if it's been cancelled.  (The
 * remainder of the NCB's reserved field is, well, reserved.)
 */

/* Used to see whether the pointer to an NCB has been cancelled.  The NetBIOS
 * interface designates certain functions as non-cancellable functions, but I
 * use this flag for all NCBs.  Support it if you can.
 * FIXME: this isn't enough, need to support an EVENT or some such, because
 * some calls (recv) will block indefinitely, so a reset, shutdown, etc. will
 * never occur.
 */
#define NCB_CANCELLED(pncb) *(const BOOL *)((pncb)->ncb_reserve)

typedef UCHAR (*NetBIOSAstat)(void *adapter, PNCB ncb);
typedef UCHAR (*NetBIOSFindName)(void *adapter, PNCB ncb);

/* Functions to support the session service */

/* Implement to support the NCBCALL command.  If you need data stored for the
 * session, return it in *session.  You can clean it up in your NetBIOSHangup
 * function (see).
 */
typedef UCHAR (*NetBIOSCall)(void *adapter, PNCB ncb, void **session);
typedef UCHAR (*NetBIOSSend)(void *adapter, void *session, PNCB ncb);
typedef UCHAR (*NetBIOSRecv)(void *adapter, void *session, PNCB ncb);
typedef UCHAR (*NetBIOSHangup)(void *adapter, void *session);

/* The last function called on an adapter; it is not called reentrantly, and
 * no new calls will be made on the adapter once this has been entered.  Clean
 * up any resources allocated for the adapter here.
 */
typedef void (*NetBIOSCleanupAdapter)(void *adapter);

typedef struct _NetBIOSTransport
{
    NetBIOSEnum           enumerate;
    NetBIOSAstat          astat;
    NetBIOSFindName       findName;
    NetBIOSCall           call;
    NetBIOSSend           send;
    NetBIOSRecv           recv;
    NetBIOSHangup         hangup;
    NetBIOSCleanupAdapter cleanupAdapter;
    NetBIOSCleanup        cleanup;
} NetBIOSTransport;

/* Transport-specific functions.  When adding a transport, add a call to its
 * init function in netapi32's DllMain.  The transport can do any global
 * initialization it needs here.  It should call NetBIOSRegisterTransport to
 * register itself with NetBIOS.
 */

/* NetBIOS-over-TCP/IP (NetBT) functions */

/* Not defined by MS, so make my own private define: */
#define TRANSPORT_NBT "MNBT"

void NetBTInit(void);

#endif /* ndef __WINE_NETBIOS_H__ */
