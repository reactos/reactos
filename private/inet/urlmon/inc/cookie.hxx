#ifndef _COOKIE_HXX_
#define _COOKIE_HXX_
#ifdef __cplusplus

#ifdef unix
#include "../download/packet.hxx"
#endif /* unix */

// CCookie<TYPE>
// This is a template for implementing a typesafe cookie that will prevent
// re-entrancy by the same thread. Eg. when a thread posts a dialog box it
// can still receive messages and can reenter a msg handler that
// put up the dialog if not protected. This is a case with us putting up
// WinVerifyTrust dialogs from OnStopBinding in code download. We want to
// serialize this across different code downloads on the same thread and
// different dependent piece downloads within the same code download.
// we also use this to serialize the setup phase making sure only one code 
// download per thread is in setup phase

// Rules: 
//  to enter either the setup phase or to verify trust of a component
//  you need to acquire the corresponding cookie in tls.
//  pcookie->Acquire() 
//  returns S_Ok if you grabbed the cookie successfully
//  returns S_FALSE if cookie is busy. You must return from the
//  msg handler and then a packet will get posted to you
//  with (wParam = id passed in, lParam (fixed for cookie obj)
//  when you pCookie->Relinquish() the next client on line will be posted a pkt

template<class TYPE>
class CCookie 
{

public:
    CCookie(DWORD msg);

    BOOL IsFree() const;
    BOOL IsOwner(TYPE id) const;

    HRESULT Acquire(TYPE id);
    HRESULT Relinquish(TYPE id);

    HRESULT JITAcquire();
    HRESULT JITRelinquish();

    HRESULT TickleWaitList();

private:

    TYPE                    m_owner;

    CList<TYPE,TYPE>        m_pWaitList;    // linked list of waiting clients

    DWORD                   m_msg;          // client that gets cookie

                                            // wParam is the id (TYPE)
    DWORD                   m_lParam;

    BOOL                    m_bJITAcquired;

};


/////////////////////////////////////////////////////////////////////////////
// CCookie<TYPE> functions

template<class TYPE>
CCookie<TYPE>::CCookie(DWORD msg):m_owner(NULL),m_lParam(0)
{
    m_msg = msg;

    m_pWaitList.RemoveAll();

    m_bJITAcquired = FALSE;
}

template<class TYPE>
BOOL CCookie<TYPE>::IsOwner(TYPE id) const
    { return (id == m_owner); }

template<class TYPE>
BOOL CCookie<TYPE>::IsFree() const
    { return ((NULL == m_owner) && !m_bJITAcquired); }

//  pcookie->Acquire() 
//  returns S_Ok if you grabbed the cookie successfully
//  returns S_FALSE if cookie is busy. You must return from the
//  msg handler and then a packet will get posted to you
//  with (wParam = id passed in, lParam (fixed for cookie obj)

template<class TYPE>
HRESULT CCookie<TYPE>::Acquire(TYPE id)
{
    HRESULT hr = S_OK;
    Assert(id);

    if (m_bJITAcquired || ((m_owner != NULL) && (m_owner != id))) {

        // add iff not already in list
        if (m_pWaitList.Find(id) == NULL)
            m_pWaitList.AddTail(id);

        hr = S_FALSE;

    } else {
        
        m_owner = id;   // assign cookie
    }

    return hr;
}

// pCookie->Relinquish(): free the cookie
// and the next client on line will get the cookie and be posted a packet
// with (wParam = id passed in, lParam (fixed for cookie obj)
template<class TYPE>
HRESULT CCookie<TYPE>::Relinquish(TYPE id)
{
    HRESULT hr = S_OK;

    Assert(id);
    Assert(m_owner);
    Assert(m_owner == id);

    if ((m_owner == NULL) || (m_owner != id)) {
        return E_UNEXPECTED;
    }

    hr = TickleWaitList();
    Assert(SUCCEEDED(hr));

    return hr;
}

template<class TYPE>
HRESULT CCookie<TYPE>::TickleWaitList()
{
    HRESULT hr = S_OK;

    if (m_pWaitList.GetCount() > 0)
        m_owner = m_pWaitList.RemoveHead();
    else
        m_owner = NULL;

    // if m_owner, post message to this new owner
    if (m_owner) {

        CCDLPacket *pPkt= new CCDLPacket(m_msg, m_owner, (DWORD)m_lParam);

        if (pPkt) {
            hr = pPkt->Post();
        } else {
            hr = E_OUTOFMEMORY;
        }

    }

    Assert(SUCCEEDED(hr));

    return hr;
}

template<class TYPE>
HRESULT CCookie<TYPE>::JITAcquire()
{
    Assert(m_owner == NULL);

    m_bJITAcquired = TRUE;

    return S_OK;
}

template<class TYPE>
HRESULT CCookie<TYPE>::JITRelinquish()
{
    HRESULT hr = S_OK;

    Assert(m_owner == NULL);

    m_bJITAcquired = FALSE;

    hr = TickleWaitList();
    Assert(SUCCEEDED(hr));

    return hr;
}
#endif
#endif
