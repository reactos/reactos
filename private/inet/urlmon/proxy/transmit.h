//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       transmit.h
//
//  Contents:   Function prototypes for STGMEDIUM marshalling.
//
//  Functions:  STGMEDIUM_to_xmit
//              STGMEDIUM_from_xmit
//              STGMEDIUM_free_inst
//
//  History:    May-10-94   ShannonC    Created
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_API STGMEDIUM_to_xmit (STGMEDIUM *pinst, RemSTGMEDIUM **ppxmit);
EXTERN_C void __RPC_API STGMEDIUM_from_xmit (RemSTGMEDIUM __RPC_FAR *pxmit, STGMEDIUM __RPC_FAR *pinst);
EXTERN_C void __RPC_API STGMEDIUM_free_inst(STGMEDIUM *pinst);

