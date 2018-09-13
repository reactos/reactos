/////////////////////////////////////////////////////////////////////////////
// BSC.H
//
// Definition of CBindStatusCallback and CScreenSaverBSC
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     07/15/97    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __BSC_H__
#define __BSC_H__

class CBindStatusCallback;
class CTimeoutBSC;
class CSSNavigateBSC;

#include "sswnd.h"

/////////////////////////////////////////////////////////////////////////////
// CBindStatusCallback
/////////////////////////////////////////////////////////////////////////////
class CBindStatusCallback : public IBindStatusCallback
{
// Construction/Destruction
public:
    CBindStatusCallback();
    virtual ~CBindStatusCallback();

// Data
protected:
    int         m_cRef;
    IBinding *  m_pBinding;

// Interfaces
public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)   (THIS);
    STDMETHOD_(ULONG, Release)  (THIS);
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj);

    // IBindStatusCallback
    STDMETHOD(OnStartBinding)   (THIS_ DWORD dwReserved, IBinding * pib);
    STDMETHOD(GetPriority)      (THIS_ LONG * pnPriority);
    STDMETHOD(OnLowResource)    (THIS_ DWORD reserved);
    STDMETHOD(OnProgress)       (THIS_ ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding)    (THIS_ HRESULT hrResult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)      (THIS_ DWORD * grfBINDF, BINDINFO * pbindinfo);
    STDMETHOD(OnDataAvailable)  (THIS_ DWORD grfBSCF, DWORD dwSize, FORMATETC * pformatetc, STGMEDIUM * pstgmed);
    STDMETHOD(OnObjectAvailable)(THIS_ REFIID riid, IUnknown * punk);
};

/////////////////////////////////////////////////////////////////////////////
// CTimeoutBSC
/////////////////////////////////////////////////////////////////////////////
class CTimeoutBSC : public CBindStatusCallback
{
// Construction/Destruction
public:
    CTimeoutBSC(DWORD dwTimeoutMS);
    virtual ~CTimeoutBSC();

// Data
protected:
    DWORD       m_dwTimeoutMS;
    MMRESULT    m_idTimeoutEvent;

// Class methods
public:
    virtual void    StopTimeoutTimer();
    virtual HRESULT Abort();

// Interfaces
public:
    STDMETHOD(OnStartBinding)   (THIS_ DWORD dwReserved, IBinding * pib);
    STDMETHOD(OnProgress)       (THIS_ ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding)    (THIS_ HRESULT hrResult, LPCWSTR szError);
};

/////////////////////////////////////////////////////////////////////////////
// CSSNavigateBSC
/////////////////////////////////////////////////////////////////////////////
class CSSNavigateBSC : public CTimeoutBSC
{
// Construction/Destruction
public:
    CSSNavigateBSC(CScreenSaverWindow * pScreenSaverWnd, DWORD dwTimeoutMS);
    virtual ~CSSNavigateBSC();

// Data
protected:
    CScreenSaverWindow * m_pScreenSaverWnd;

// Interfaces
public:
    STDMETHOD(OnStopBinding)(THIS_ HRESULT hrResult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)  (THIS_ DWORD * grfBINDF, BINDINFO * pbindinfo);
};

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
void CALLBACK BSCTimeoutTimerProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

#endif  // __BSC_H__
