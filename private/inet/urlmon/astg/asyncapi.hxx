//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	asyncapi.hxx
//
//  Contents:	API definitions for async storage
//
//  Classes:	
//
//  Functions:	
//
//  History:	05-Jan-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __ASYNCAPI_HXX__
#define __ASYNCAPI_HXX__

interface IFillLockBytes;

HRESULT StgOpenAsyncDocfileOnIFillLockBytes( IFillLockBytes *pflb,
                                             DWORD grfMode,
                                             DWORD asyncFlags,
                                             IStorage **ppstgOpen);

HRESULT StgGetIFillLockBytesOnILockBytes( ILockBytes *pilb,
                                         IFillLockBytes **ppflb);

HRESULT StgGetIFillLockBytesOnFile(OLECHAR const *pwcsName,
                                  IFillLockBytes **ppflb);

#if DBG == 1
HRESULT StgGetDebugFileLockBytes(OLECHAR const *pwcsName, ILockBytes **ppilb);
#endif

#endif // #ifndef __ASYNCAPI_HXX__
