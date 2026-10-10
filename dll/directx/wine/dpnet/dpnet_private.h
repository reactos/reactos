/*
 * DirectPlay8 (dpnet) private include file
 *
 * Copyright 2004 Raphael Junqueira
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

#ifndef __WINE_DPNET_PRIVATE_H
#define __WINE_DPNET_PRIVATE_H

#include <wine/list.h>
#include "winsock2.h"

#include "dplay8.h"
#include "dplobby8.h"
/*
 *#include "dplay8sp.h"
 */

/* DirectPlay8 Interfaces: */
typedef struct IDirectPlay8ClientImpl IDirectPlay8ClientImpl;
typedef struct IDirectPlay8AddressImpl IDirectPlay8AddressImpl;
typedef struct IDirectPlay8LobbiedApplicationImpl IDirectPlay8LobbiedApplicationImpl;
typedef struct IDirectPlay8ThreadPoolImpl IDirectPlay8ThreadPoolImpl;

/* ------------------ */
/* IDirectPlay8Client */
/* ------------------ */

/*****************************************************************************
 * IDirectPlay8Client implementation structure
 */
struct IDirectPlay8ClientImpl
{
    IDirectPlay8Client IDirectPlay8Client_iface;
    LONG ref;

    /* IDirectPlay8Client fields */
    PFNDPNMESSAGEHANDLER msghandler;
    DWORD flags;
    void *usercontext;
    WCHAR *username;
    void  *data;
    DWORD datasize;

    DPN_SP_CAPS spcaps;
};

/* ------------------- */
/* IDirectPlay8Address */
/* ------------------- */
struct component
{
    struct list entry;

    WCHAR *name;
    DWORD type;
    DWORD size;

    union
    {
        DWORD value;            /* DPNA_DATATYPE_DWORD       */
        GUID guid;              /* DPNA_DATATYPE_GUID        */
        WCHAR *string;          /* DPNA_DATATYPE_STRING      */
        char *ansi;             /* DPNA_DATATYPE_STRING_ANSI */
        void *binary;           /* DPNA_DATATYPE_BINARY      */
    } data;
};

/*****************************************************************************
 * IDirectPlay8Address implementation structure
 */
struct IDirectPlay8AddressImpl
{
    IDirectPlay8Address IDirectPlay8Address_iface;
    LONG ref;
    /* IDirectPlay8Address fields */
    GUID SP_guid;
    BOOL init;

    struct component **components;
    DWORD comp_count;
    DWORD comp_array_size;
};

/*****************************************************************************
 * IDirectPlay8LobbiedApplication implementation structure
 */
struct IDirectPlay8LobbiedApplicationImpl
{
    IDirectPlay8LobbiedApplication IDirectPlay8LobbiedApplication_iface;
    LONG ref;

    PFNDPNMESSAGEHANDLER msghandler;
    DWORD flags;
    void *usercontext;
    DPNHANDLE *connection;
};

/*****************************************************************************
 * IDirectPlay8ThreadPool implementation structure
 */
struct IDirectPlay8ThreadPoolImpl
{
    IDirectPlay8ThreadPool IDirectPlay8ThreadPool_iface;
    LONG ref;
};

/**
 * factories
 */
extern HRESULT DPNET_CreateDirectPlay8Client(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8Server(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8Peer(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8Address(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8LobbiedApp(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8ThreadPool(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8LobbyClient(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj);

extern void init_dpn_sp_caps(DPN_SP_CAPS *dpnspcaps);
extern void init_winsock(void);
extern HRESULT enum_services_providers(const GUID * const service, DPN_SERVICE_PROVIDER_INFO * const info_buffer,
        DWORD * const buf_size, DWORD * const returned);

/* used for generic dumping (copied from ddraw) */
typedef struct {
    DWORD val;
    const char* name;
} flag_info;

typedef struct {
    const GUID *guid;
    const char* name;
} guid_info;

#define FE(x) { x, #x }	
#define GE(x) { &x, #x }

#endif
