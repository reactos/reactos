//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ROSTMDIR.HXX
//
//  Contents:
//
//  Classes:    Declaration for CReadOnlyStreamDirect class
//
//  Functions:
//
//  History:    12-21-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR CReadOnlyStreamDirect : public IStream
{
public:
        CReadOnlyStreamDirect(CTransData *pCTransData, DWORD grfBindF);
        virtual ~CReadOnlyStreamDirect(void);

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
        LPWSTR GetFileName();

private:
        CRefCount    _CRefs;
        CTransData  *_pCTransData;
        DWORD        _grfBindF;
};


