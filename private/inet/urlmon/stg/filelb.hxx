//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       FILELB.HXX
//
//  Contents:
//
//  Classes:    Declaration for FileLockBytes class, which derives
//              from the CLockBytes class.
//
//  Functions:
//
//  History:    12-13-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR FileLockBytes : public CLockBytes
{
public:
        FileLockBytes(HANDLE filehandle);

        // *** IUnknown methods ***
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

private:
        HRESULT seekfile(ULARGE_INTEGER offset);

        HANDLE      _hFileHandle;
        CRefCount   _CRefs;
};

