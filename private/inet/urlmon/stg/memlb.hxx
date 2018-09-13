//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       MEMLB.HXX
//
//  Contents:
//
//  Classes:    Declaration for MemLockBytes class, which derives
//              from the CLockBytes class.
//
//  Functions:
//
//  History:    12-13-95    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

class FAR MemLockBytes : public CLockBytes
{
public:

        // *** IUnknown methods ***
        STDMETHOD_(ULONG,Release) (THIS);

        // *** ILockBytes methods ***
        STDMETHOD(ReadAt) (THIS_ ULARGE_INTEGER ulOffset, VOID HUGEP *pv,
            ULONG cb, ULONG FAR *pcbRead);
        STDMETHOD(WriteAt) (THIS_ ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,
            ULONG cb, ULONG FAR *pcbWritten);
        STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER cb);

private:
        CRefCount   _CRefs;
        HGLOBAL     memhandle;
};

