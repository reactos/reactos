//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	filelkb.hxx
//
//  Contents:	File ILockBytes class for async docfile
//
//  Classes:	CFileLockBytes
//
//  Functions:	
//
//  History:	19-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __FILELKB_HXX__
#define __FILELKB_HXX__


//+---------------------------------------------------------------------------
//
//  Class:	CFileLockBytes
//
//  Purpose:	
//
//  Interface:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------


class CFileLockBytes: public ILockBytes
{
public:
    CFileLockBytes();
    ~CFileLockBytes();

    SCODE Init(OLECHAR const *pwcsName);
    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // ILockBytes
    STDMETHOD(ReadAt)(ULARGE_INTEGER ulOffset,
		     VOID HUGEP *pv,
		     ULONG cb,
		     ULONG *pcbRead);
    STDMETHOD(WriteAt)(ULARGE_INTEGER ulOffset,
		      VOID const HUGEP *pv,
		      ULONG cb,
		      ULONG *pcbWritten);
    STDMETHOD(Flush)(void);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
			 ULARGE_INTEGER cb,
			 DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
			   ULARGE_INTEGER cb,
			    DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
private:
    LONG _cReferences;

    CRITICAL_SECTION _cs;
    HANDLE _h;

    OLECHAR _acName[MAX_PATH + 1];
};


SCODE Win32ErrorToScode(DWORD dwErr);

#endif // #ifndef __FILELKB_HXX__
