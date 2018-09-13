//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CLOCKBYT.HXX
//
//  Contents:
//
//  Classes:    Declaration for ILockBytes base class
//
//  Functions:
//
//  History:    12-01-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR CLockBytes : public ILockBytes
{
public:

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);

        // *** ILockBytes methods ***
        STDMETHOD(Flush) (THIS);
        STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
            DWORD dwLockType);
        STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(Stat) (THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag);

private:
        CRefCount   _CRefs;
};

