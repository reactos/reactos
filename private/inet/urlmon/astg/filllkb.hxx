//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	filllkb.hxx
//
//  Contents:	CFillLockBytes class header
//
//  Classes:	
//
//  Functions:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __FILLLKB_HXX__
#define __FILLLKB_HXX__

#include "ifill.h"

//+---------------------------------------------------------------------------
//
//  Class:	CFillLockBytes
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

class CFillLockBytes: public ILockBytes,
    public IFillLockBytes
{
public:
    CFillLockBytes(ILockBytes *pilb);
    ~CFillLockBytes();

    SCODE Init(void);
    
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

    //From IFillLockBytes
    STDMETHOD(FillAppend)(void const *pv,
                         ULONG cb,
                         ULONG *pcbWritten);
    STDMETHOD(FillAt)(ULARGE_INTEGER ulOffset,
                     void const *pv,
                     ULONG cb,
                     ULONG *pcbWritten);
    STDMETHOD(SetFillSize)(ULARGE_INTEGER ulSize);
    STDMETHOD(Terminate)(BOOL bCanceled);

    SCODE GetFailureInfo(ULONG *pulWaterMark, ULONG *pulFailurePoint);
    HANDLE GetNotificationEvent(void);
    SCODE GetTerminationStatus(DWORD *pdwFlags);
    
private:
    ILockBytes *_pilb;

    ULONG _ulHighWater;
    DWORD _dwTerminate;

    ULONG _ulFailurePoint;

    HANDLE _hNotifyEvent;
};


#endif // #ifndef __FILLLKB_HXX__
