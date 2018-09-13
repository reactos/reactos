#include "mslocusr.h"
#include "msluglob.h"

#include <ole2.h>

CUserSettings::CUserSettings(void)
	: m_cRef(1),
	  m_clsid(GUID_NULL),
	  m_nlsName(NULL),
	  m_hkey(NULL)
{
	// nothing else
}


CUserSettings::~CUserSettings(void)
{
	if (m_hkey != NULL) {
		::RegCloseKey(m_hkey);
#ifdef DEBUG
		m_hkey = NULL;
#endif
	}
}


STDMETHODIMP CUserSettings::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	if (!IsEqualIID(riid, IID_IUnknown) &&
		!IsEqualIID(riid, IID_IUserSettings)) {
		return ResultFromScode(E_NOINTERFACE);
	}

	*ppvObj = this;
	AddRef();
	return NOERROR;
}


STDMETHODIMP_(ULONG) CUserSettings::AddRef(void)
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) CUserSettings::Release(void)
{
	ULONG cRef;

	cRef = --m_cRef;

	if (0L == m_cRef) {
		delete this;
	}

	return cRef;
}


STDMETHODIMP CUserSettings::GetCLSID(CLSID *pclsidOut)
{
	return E_NOTIMPL;
}


STDMETHODIMP CUserSettings::GetName(LPSTR pbBuffer, LPDWORD pcbBuffer)
{
	if (m_nlsName.QueryError())
		return ResultFromScode(E_OUTOFMEMORY);

	UINT err = NPSCopyNLS(&m_nlsName, pbBuffer, pcbBuffer);

	return HRESULT_FROM_WIN32(err);
}


STDMETHODIMP CUserSettings::GetDisplayName(LPSTR pbBuffer, LPDWORD pcbBuffer)
{
	return E_NOTIMPL;
}


STDMETHODIMP CUserSettings::QueryKey(HKEY *phkeyOut)
{
	return E_NOTIMPL;
}

