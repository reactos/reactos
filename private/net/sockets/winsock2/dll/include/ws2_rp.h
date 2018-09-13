//--------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Rights Reserved
//
// ws2_rp.h
//
//--------------------------------------------------------------------


#ifndef _WS2_RP_H_
#define _WS2_RP_H_

// If this flag is defined, then WinSock2 will always act as if it is
// running in a restricted process:
#ifdef DBG
// #define DBG_ALWAYS_RESTRICTED
#endif

// #define LOCAL_HANDLES
#define LOCAL_EVENT_HANDLES

//--------------------------------------------------------------------
// This is the Provider ID for the chain that a restricted process 
// should follow.
//
// UUID: ec91fa14-5d3e-11d1-8c0f-0000f8754035
//--------------------------------------------------------------------

DEFINE_GUID( RestrictedProviderId,
             0xec91fa14,
             0x5d3e,
             0x11d1,
             0x8c, 0x0f, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35 );

//------------------------------------------------------------------------
// This is the Provider ID for the LSP entry itself.
//
// UUID: a92d64e1-5709-11d1-8c02-0000f8754035
//------------------------------------------------------------------------
DEFINE_GUID( RestrictedLayeredProviderGuid,
             0xa92d64e1,
             0x5709,
             0x11d1,
             0x8c, 0x02, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35 );

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

extern DWORD RP_Init();

extern BOOL  RP_IsRestrictedProcess();

#endif //_WS2_RP_H_
