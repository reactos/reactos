#include "Status.h"
#include "..\\inc\\urlmon.hxx"

#define TIMEOUT 600000 // 10 minutes

/******************************************************************************

    Constructor and destructor and helper functions

******************************************************************************/

CSilentCodeDLSink::CSilentCodeDLSink()
{
    m_cRef = 1;
    m_pBinding = NULL;
    m_hOnStopBindingEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_fAbort = FALSE;
}

CSilentCodeDLSink::~CSilentCodeDLSink()
{
    if(m_hOnStopBindingEvt)
		CloseHandle(m_hOnStopBindingEvt);	
}

VOID CSilentCodeDLSink::Abort()
{
    m_fAbort = TRUE;
}

HRESULT CSilentCodeDLSink::WaitTillNotified()
{
    if (m_hOnStopBindingEvt == NULL)
        return E_FAIL;

    HRESULT hr = E_FAIL;
    DWORD dwResult = 0;
    const DWORD MORE_INPUT = WAIT_OBJECT_0 + 1;
    MSG msg;

    // Test state of event
    dwResult = WaitForSingleObject(m_hOnStopBindingEvt, 0);
    if (dwResult == WAIT_FAILED)
        return HRESULT_FROM_WIN32(GetLastError());

    // Note that MsgWaitForMultipleObjects doesn't return 
    // if there was previously unread input of the specified 
    // type in the queue. It only wakes up when input arrives. 

    for (dwResult = MORE_INPUT; dwResult == MORE_INPUT; ) 
    {
        if (dwResult == MORE_INPUT)
        {
            // more input in queue
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else if (dwResult == WAIT_OBJECT_0)
        {
            // event has been signaled
            hr = S_OK;
        }
        else if (dwResult == 0xFFFFFFFF)
        {
            // error has occurred
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            // timeout or wait abondaned
            hr = E_FAIL;
        }
        
        dwResult = MsgWaitForMultipleObjects(
                                  1, &m_hOnStopBindingEvt, 
                                  FALSE, TIMEOUT, QS_ALLINPUT);
    }

    if (FAILED(hr))
        Abort();

    return hr;
}

/******************************************************************************

    IUnknown Methods

******************************************************************************/

STDMETHODIMP CSilentCodeDLSink::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
	{
        *ppv = (IBindStatusCallback*)this;
	}
	else if (riid == IID_ICodeInstall)
	{
		*ppv = (ICodeInstall*)this;
	}
	else if (riid == IID_IWindowForBindingUI)
	{
		*ppv = (IWindowForBindingUI*)this;
	}

	if (*ppv != NULL)
	{
	    ((IUnknown*)*ppv)->AddRef();
    	return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSilentCodeDLSink::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSilentCodeDLSink::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0;   
}

/******************************************************************************

    IBindStatusCallback Methods

******************************************************************************/

STDMETHODIMP CSilentCodeDLSink::OnStartBinding(DWORD grfBSCOption, IBinding *pib)
{
	if (m_pBinding != NULL)
        m_pBinding->Release();

    m_pBinding = pib;
	if (m_pBinding != NULL)
        m_pBinding->AddRef();

	if (m_fAbort)
		m_pBinding->Abort();

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::GetPriority(LONG *pnPriority)
{
	if (m_fAbort)
		m_pBinding->Abort();

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::OnLowResource(DWORD reserved)
{
	if (m_fAbort)
		m_pBinding->Abort();

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::OnProgress(
                                    ULONG ulProgress,
                                    ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText)
{
	if (m_fAbort)
		m_pBinding->Abort();

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::OnStopBinding(
                                    HRESULT hresult, 
                                    LPCWSTR szError)
{
	if (m_pBinding)
    {
        m_pBinding->Release();
	    m_pBinding = NULL;
    }

    if(m_hOnStopBindingEvt)
		SetEvent(m_hOnStopBindingEvt);

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::GetBindInfo(
                                    DWORD* pgrfBINDF, 
                                    BINDINFO* pbindInfo)
{
    if (!pgrfBINDF || !pbindInfo || !pbindInfo->cbSize)
        return E_INVALIDARG;

    *pgrfBINDF = BINDF_ASYNCHRONOUS|BINDF_SILENTOPERATION;

    // clear BINDINFO but keep its size
    DWORD cbSize = pbindInfo->cbSize;
    ZeroMemory( pbindInfo, cbSize );
    pbindInfo->cbSize = cbSize;

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::OnDataAvailable(
                                        DWORD grfBSCF, 
                                        DWORD dwSize,
                                        FORMATETC *pformatetc,
                                        STGMEDIUM *pstgmed)
{
	if (m_fAbort)
		m_pBinding->Abort();

    return S_OK;
}

STDMETHODIMP CSilentCodeDLSink::OnObjectAvailable(
                                            REFIID riid,
                                            IUnknown *punk)
{
    return S_OK;
}

/******************************************************************************

    ICodeInstall Methods

******************************************************************************/

STDMETHODIMP CSilentCodeDLSink::GetWindow(
                                    REFGUID rguidReason, 
                                    HWND *phwnd)
{
    *phwnd = (HWND)INVALID_HANDLE_VALUE;

	if (m_fAbort)
		m_pBinding->Abort();

    return S_FALSE;
}

STDMETHODIMP CSilentCodeDLSink::OnCodeInstallProblem(
                                           ULONG ulStatusCode, 
                                           LPCWSTR szDestination, 
                                           LPCWSTR szSource, 
                                           DWORD dwReserved)
{
	switch (ulStatusCode)
	{
		case CIP_ACCESS_DENIED:
		case CIP_DISK_FULL:
			return E_ABORT;

		case CIP_OLDER_VERSION_EXISTS:
			return S_OK; // always update

		case CIP_NEWER_VERSION_EXISTS:
			return S_FALSE; // don't update

		case CIP_NAME_CONFLICT:
			return E_ABORT;

		case CIP_EXE_SELF_REGISTERATION_TIMEOUT:
			return S_OK;

		case CIP_TRUST_VERIFICATION_COMPONENT_MISSING:
			return E_ABORT; 

		case CIP_UNSAFE_TO_ABORT:
			return S_OK;

		default:
			return E_ABORT;
    }

	return S_OK;
}

