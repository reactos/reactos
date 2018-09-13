//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       tls.h
//
//  Contents:   Manage thread local storage for UrlMon.
//              The non-inline routines are in ..\mon\tls.cxx
//  Classes:
//
//  Functions:
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _TLS_H_
#define _TLS_H_

#include "clist.hxx"
#include "cookie.hxx"

class CTransactionMgr;
class CCDLPacketMgr;
class CCodeDownload;
class CDownload;
// notification/scheduler

//+---------------------------------------------------------------------------
//
// forward declarations (in order to avoid type casting when accessing
// data members of the SOleTlsData structure).
//
//+---------------------------------------------------------------------------

extern DWORD  gTlsIndex;                    // global Index for TLS

//+---------------------------------------------------------------------------
//
//  Enum:       URLMKTLSFLAGS
//
//  Synopsys:   bit values for dwFlags field of SUrlMkTlsData. If you just want
//              to store a BOOL in TLS, use this enum and the dwFlag field.
//
//+---------------------------------------------------------------------------
typedef enum tagURLMKTLSFLAGS
{
    URLMKTLS_LOCALTID        = 1,  // whether TID is in current process or not
    URLMKTLS_UUIDINITIALIZED = 2,  // whether logical thread was init'd
    URLMKTLS_INTHREADDETACH  = 4,  // Whether we are in thread detach. Needed
                                   // due to NT's special thread detach rules.
}  URLMKTLSFLAGS;


//+---------------------------------------------------------------------------
//
//  Structure:  SUrlMkTlsData
//
//  Synopsis:   structure holding per thread state needed by UrlMon
//
//+---------------------------------------------------------------------------
typedef struct tagSUrlMkTlsData
{
    DWORD               dwApartmentID;      // Per thread "process ID"
    DWORD               dwFlags;            // see URLMKTLSFLAGS above
    HWND                hwndUrlMkNotify;    // notification window
    LONG                cDispatchLevel;     // dispatch nesting level

    CTransactionMgr    *pCTransMgr;         // transaction manager

#ifdef PER_THREAD
    CMediaTypeHolder   *pCMediaHolder;      // media types register per apartment
#endif //PER_THREAD

    CList<CCodeDownload *,CCodeDownload *>*
                        pCodeDownloadList;  // linked list of pointers to
                                            // CCodeDownload objects ongoing on
                                            // this thread
    CCookie<CDownload*> *pTrustCookie;

                                            // only the cookie owner can do
                                            // setup/winverifytrust
                                            // Others wait for the
                                            // cookie to enter these phases
                                            // Cookie availabilty is posted
                                            // as a msg. We can't uses any
                                            // regular sync apis as this
                                            // is protecting execution by the
                                            // same thread in a diff msg.
    CCookie<CCodeDownload*>
                        *pSetupCookie;


    CCDLPacketMgr       *pCDLPacketMgr;     // per thread packet manager
                                            // A packet is a unit
                                            // of work that takes time eg.
                                            // trust verifcation of a piece
                                            // setup of a piece or INF
                                            // processing of one piece.
                                            // To be able to have the
                                            // client be responsive with UI
                                            // and abort capabilty we need
                                            // to split out work into as
                                            // small units as possible
                                            // and queue up these packets
                                            // Packets get run on a timer per
                                            // thread.
    CList<LPCWSTR ,LPCWSTR >*
                        pRejectedFeaturesList;
                                            // linked list of pointers to
                                            // features or code downloads that
                                            // the use has explicitly rejected
                                            // on this thread

#if DBG==1
    LONG                cTraceNestingLevel; // call nesting level for UrlMonTRACE
#endif

} SUrlMkTlsData;

//+---------------------------------------------------------------------------
//
//  class       CUrlMkTls
//
//  Synopsis:   class to abstract thread-local-storage in UrlMon.
//
//  Notes:      To use Tls in UrlMon, functions should define an instance of
//              this class on their stack, then use the -> operator on the
//              instance to access fields of the SOleTls structure.
//
//              There are two instances of the ctor. One just Assert's that
//              the SUrlMkTlsData has already been allocated for this thread. Most
//              internal code should use this ctor, since we can assert that if
//              the thread made it this far into our code, tls has already been
//              checked.
//
//              The other ctor will check if SUrlMkTlsData exists, and attempt to
//              allocate and initialize it if it does not. This ctor will
//              return an HRESULT. Functions that are entry points to UrlMon
//              should use this version.
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//+---------------------------------------------------------------------------
class CUrlMkTls
{
public:
    CUrlMkTls(HRESULT &hr);

    // to get direct access to the data structure
    SUrlMkTlsData * operator->(void);

private:

    HRESULT      TLSAllocData(); // allocates an SUrlMkTlsData structure

    SUrlMkTlsData * _pData;       // ptr to UrlMon TLS data
};

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMkTls::CUrlMkTls
//
//  Synopsis:   ctor for UrlMon Tls object.
//
//  Notes:      Peripheral UrlMon code that can not assume that some outer-layer
//              function has already verified the existence of the SUrlMkTlsData
//              structure for the current thread should use this version of
//              the ctor.
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//+---------------------------------------------------------------------------
inline CUrlMkTls::CUrlMkTls(HRESULT &hr)
{
    _pData = (SUrlMkTlsData *) TlsGetValue(gTlsIndex);
    if (_pData)
        hr = S_OK;
    else
        hr = TLSAllocData();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUrlMkTls::operator->(void)
//
//  Synopsis:   returns ptr to the data structure
//
//  History:    12-02-95   JohannP (Johann Posch)   Created
//
//+---------------------------------------------------------------------------
inline SUrlMkTlsData * CUrlMkTls::operator->(void)
{
    return _pData;
}



typedef struct URLMON_TS
{
    DWORD           _dwTID;
    HWND            _hwndNotify;
    URLMON_TS*      _pNext;
} URLMON_TS, *LPURLMON_TS;

#endif // _TLS_H_


