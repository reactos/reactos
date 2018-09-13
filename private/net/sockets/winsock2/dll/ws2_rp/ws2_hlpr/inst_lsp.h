//------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  inst_lsp.h
//
//  Installation program for the MS Restricted Process Layered
//  Service Provider.
//
//  Author:
//
//    Edward Reus (edwardr)   11/06/97
//
//  Revision History:
//
//    Edward Reus (edwardr)   12/30/97    Merge into DLL helper code.
//
//------------------------------------------------------------------------

#ifndef _INST_LSP_H_
#define _INST_LSP_H_

//------------------------------------------------------------------------
// Constants/Macros:
//------------------------------------------------------------------------

#define RESTRICTED_LSP_NAME   TEXT("Restricted TCPIP LSP")
                           
#define CHAIN_PREFIX          TEXT("Restricted ")

#define SIMPLE_CHAIN_NAME     TEXT("Restricted TCP/IP")

#define LSP_PATH              TEXT("%SystemRoot%\\system32\\msrlsp.dll")

//------------------------------------------------------------------------
// ec91fa14-5d3e-11d1-8c0f-0000f8754035
//------------------------------------------------------------------------
DEFINE_GUID( RestrictedProviderChainId,
             0xec91fa14,
             0x5d3e,
             0x11d1,
             0x8c, 0x0f, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35 );

//------------------------------------------------------------------------
// a92d64e1-5709-11d1-8c02-0000f8754035
//------------------------------------------------------------------------
DEFINE_GUID( LayeredProviderGuid,
             0xa92d64e1,
             0x5709,
             0x11d1,
             0x8c, 0x02, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35 );

//------------------------------------------------------------------------
// Functions:
//------------------------------------------------------------------------

extern void InstallLSP();

extern void UninstallLSP();

extern DWORD LSPAlreadyInstalled( BOOL *pfIsInstalled );

#endif // _INST_LSP_H_
