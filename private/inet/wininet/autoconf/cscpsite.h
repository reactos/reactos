#ifndef __CSCPSITE_H__
#define __CSCPSITE_H__

#include <windows.h>
#include <olectl.h>
#include <activscp.h>
#include "jsproxy.h"
#include "utils.h"


/********************************************************************************************/
// ScriptSite Class
//
//
//
class CScriptSite : public IActiveScriptSite
{

public:
	CScriptSite();
	~CScriptSite();
	// IUnknown Interface methods.
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObject)
	{
		if (riid == IID_IUnknown)
		{            
			*ppvObject = (LPVOID)(LPUNKNOWN)this;
            AddRef();
			return S_OK;
		}
		else
		{
			if (riid == IID_IActiveScriptSite)
			{
				*ppvObject = (LPVOID)(IDispatch*)this;
                AddRef();
				return S_OK;
			}
			else
			{
				*ppvObject = 0;
				return E_NOINTERFACE;
			}
		}

	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return ++m_refCount;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		if (--m_refCount)
			return m_refCount;

		delete this;
		return 0;
	}

	STDMETHODIMP GetLCID(LCID *plcid);
	STDMETHODIMP GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppunkItem, ITypeInfo **ppTypeInfo);
	STDMETHODIMP GetDocVersionString(BSTR *pstrVersionString);
	STDMETHODIMP OnScriptTerminate(const VARIANT *pvarResult,const EXCEPINFO *pexcepinfo);
	STDMETHODIMP OnStateChange(SCRIPTSTATE ssScriptState);
	STDMETHODIMP OnScriptError(IActiveScriptError *pase);
	STDMETHODIMP OnEnterScript();
	STDMETHODIMP OnLeaveScript();

	STDMETHODIMP Init(AUTO_PROXY_HELPER_APIS* pAPHA, LPCSTR szScript);
	STDMETHODIMP DeInit();
	STDMETHODIMP RunScript(LPCSTR szURL, LPCSTR szHost, LPSTR* result);

private:
	BOOL				m_fInitialized;
	long				m_refCount;
	IActiveScript		*m_pios;
	IActiveScriptParse	*m_pasp;
	CJSProxy			*m_punkJSProxy;
	IDispatch			*m_pScriptDispatch; // Stored dispatch for script
	DISPID				m_Scriptdispid; // DISPID for stored script to facilitate quicker invoke.

};


#endif