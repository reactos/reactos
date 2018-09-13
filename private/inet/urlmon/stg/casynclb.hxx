//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CASYNCLB.HXX
//
//  Contents:
//
//  Classes:    Declaration for IAsyncLockBytes class
//
//  Functions:
//
//  History:    12-13-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR CAsyncLockBytes : public ILockBytes, public IFillLockBytes
{
public:
        CAsyncLockBytes(ILockBytes *pLB);

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** ILockBytes methods ***
        STDMETHOD(ReadAt) (THIS_ ULARGE_INTEGER ulOffset, VOID HUGEP *pv,
            ULONG cb, ULONG FAR *pcbRead);
        STDMETHOD(WriteAt) (THIS_ ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
            ULONG cb, ULONG FAR *pcbWritten);
        STDMETHOD(Flush) (THIS);
        STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER cb);
        STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
            DWORD dwLockType);
        STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(Stat) (THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag);

        // *** IFillLockBytes methods ***
        STDMETHOD(FillAppend) (void const *pv, ULONG cb, ULONG *pcbWritten);
        STDMETHOD(Terminate) (BOOL bCanceled);

private:
        CRefCount       _CRefs;
        ILockBytes      *_pLBchain;
        ULARGE_INTEGER  _cbFillOffset;
        BOOL            _fFillDone;
};

