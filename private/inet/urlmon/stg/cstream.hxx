//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CSTREAM.HXX
//
//  Contents:
//
//  Classes:    Declaration for IStream class
//
//  Functions:
//
//  History:    12-01-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR CStream : public IStream
{
public:
        CStream(IStream *pStr);

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** IStream methods ***
        STDMETHOD(Read) (THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead);
        STDMETHOD(Write) (THIS_ VOID const HUGEP *pv, ULONG cb,
            ULONG FAR *pcbWritten);
        STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition);
        STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize);
        STDMETHOD(CopyTo) (THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten);
        STDMETHOD(Commit) (THIS_ DWORD dwCommitFlags);
        STDMETHOD(Revert) (THIS);
        STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(Stat) (THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag);
        STDMETHOD(Clone) (THIS_ LPSTREAM FAR *ppStm);

private:
        CRefCount   _CRefs;
        IStream     *_pStream;
};

