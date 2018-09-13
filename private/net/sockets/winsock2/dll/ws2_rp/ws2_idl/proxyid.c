//------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Right Reserved
//
// proxyid.c
//
//------------------------------------------------------------------

#define INITGUIDS

#define INC_OLE2
#include <ole2.h>
#include <ole2ver.h>
#include <ws2_if.h>

#ifdef INITGUIDS
#include <initguid.h>
#endif //INITGUIDS

//--------------------------------------------------------------------
// Restricted Process WinSock Helpers
//
//    Object:    {570da105-3c30-11d1-8bf1-0000f8754035}
//    Interface: {3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
//    IDL Proxy: {3f7ec550-80a3-11d1-b222-00a0c90c91fe}
//--------------------------------------------------------------------

DEFINE_GUID(CLSID_RestrictedProcessProxy,
            0x3f7ec550, 
            0x80a3, 0x11d1, 0xb2, 0x22, 0x00, 0xa0, 0xc9, 0x0c, 0x91, 0xfe);

