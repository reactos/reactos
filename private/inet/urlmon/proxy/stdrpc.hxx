//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: stdrpc.hxx
//
//  Contents: Private header file for building interface proxies and stubs.
//
//  Classes:	CStreamOnMessage
//
//  Functions: 	
//
//  History:	 4-Jul-93 ShannonC	Created
//		 3-Aug-93 ShannonC      Changes for NT511 and IDispatch support.
//		10-Oct-93 ShannonC      Changed to new IRpcChannelBuffer interface.
//		22-Sep-94 MikeSe	Moved from CINC and simplified.
//
//--------------------------------------------------------------------------
#ifndef __STDRPC_HXX__
#define __STDRPC_HXX__

#define _OLE2ANAC_H_
#include <windows.h>

class CStreamOnMessage : public IStream
{

  public:
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE Read(VOID HUGEP *pv, ULONG cb, ULONG *pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(VOID const HUGEP *pv,
                                  ULONG cb,
                                  ULONG *pcbWritten) ;
    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,
                                  DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition) ;
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) ;
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm,
                                  ULARGE_INTEGER cb,
                                  ULARGE_INTEGER *pcbRead,
                                  ULARGE_INTEGER *pcbWritten) ;
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) ;
    virtual HRESULT STDMETHODCALLTYPE Revert();
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType) ;
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,
                                  ULARGE_INTEGER cb,
                                  DWORD dwLockType) ;
    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag) ;
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream * *ppstm) ;

    CStreamOnMessage(unsigned char **ppMessageBuffer);
	CStreamOnMessage(unsigned char **ppMessageBuffer, unsigned long cbMax);

    unsigned char *pStartOfStream;
    unsigned char **ppBuffer;
	unsigned long cbMaxStreamLength;
    ULONG               ref_count;
};

#endif //__STDRPC_HXX__

