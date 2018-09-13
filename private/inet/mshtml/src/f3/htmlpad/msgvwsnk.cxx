/* copyright (c) 1995 Microsoft Corporation -- All rights reserved */

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#pragma warning(disable:4514) // Unreferened inline function has been removed

CViewNotifier::CViewNotifier (void)
{
    Initialize();
}

CViewNotifier::~CViewNotifier ()
{
}

BOOL CViewNotifier::Initialize (void)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i)
    {
        m_aryAdviseSink [i] = NULL;
    }

    return TRUE;
}

HRESULT CViewNotifier::Advise(LPMAPIVIEWADVISESINK pAdvise,
                              ULONG * pulConnection)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i)
    {
        if (NULL == m_aryAdviseSink [i] )
        {
            m_aryAdviseSink [i] = pAdvise;
            m_aryAdviseSink [i] ->AddRef();
            *pulConnection = ++i;
            // connection == index + 1
            return S_OK;
        }
    }

    return ResultFromScode(E_OUTOFMEMORY);
}

HRESULT CViewNotifier::Unadvise (ULONG ulConnection)
{
    if (--ulConnection < MAXSINKS)
    {   // connection == index + 1
        m_aryAdviseSink [ulConnection] ->Release();
        m_aryAdviseSink [ulConnection] = NULL;
        return S_OK;
    }
    return ResultFromScode(E_INVALIDARG);
}

void CViewNotifier::OnShutdown (void)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i)
    {
        if (NULL != m_aryAdviseSink [i] )
        {
            m_aryAdviseSink [i] -> OnShutdown ();
        }
    }
}

void CViewNotifier::OnNewMessage (void)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i)
    {
        if (NULL != m_aryAdviseSink [i] )
        {
            m_aryAdviseSink [i] ->OnNewMessage();
        }
    }
}

HRESULT CViewNotifier::OnPrint(ULONG ulPageNumber, HRESULT hrStatus)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i)
    {
        if (NULL != m_aryAdviseSink [i] )
        {
            if(MAPI_E_USER_CANCEL == GetScode(m_aryAdviseSink [i] ->OnPrint(ulPageNumber, hrStatus)))
                return MAPI_E_USER_CANCEL;
        }
    }
    
    return hrSuccess;
}

void CViewNotifier::OnSubmitted (void)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i) {
        if (NULL != m_aryAdviseSink [i] ) {
            m_aryAdviseSink [i] ->OnSubmitted();
        }
    }
}

void CViewNotifier::OnSaved (void)
{
    ULONG   i;

    for (i = 0; i < MAXSINKS; ++i) {
        if (NULL != m_aryAdviseSink [i] ) {
            m_aryAdviseSink [i] ->OnSaved();
        }
    }
}
