#include "cscpsite.h"


/*******************************************************************************
*	CScriptSite Functions
********************************************************************************/
CScriptSite::CScriptSite()
{
	m_refCount = 1;
	m_pios = NULL;
	m_pasp = NULL; 
	m_pScriptDispatch = NULL;
	m_Scriptdispid = -1;
	m_fInitialized = FALSE;

}
CScriptSite::~CScriptSite()
{
	if (m_fInitialized)
		DeInit();
}

STDMETHODIMP CScriptSite::Init(AUTO_PROXY_HELPER_APIS* pAPHA, LPCSTR szScript)
{
	CHAR szClassId[64];	
	CLSID clsid;
	HRESULT hr = S_OK;
	BSTR	bstrClsID = NULL;
	BSTR	bstrScriptText = NULL;
	BSTR	rgbstrNames[1] = {L"FindProxyForURL"};
	EXCEPINFO	exceptinfo;

	// pAPHA can be null - it is checked in the autoproxy object!
	if (!szScript)
		return E_POINTER;

	if (m_fInitialized)
		return hr;
	// CoCreateInstance the JScript engine.

	// Get the class id of the desired language engine
	hr = GetScriptEngineClassIDFromName(
		"JavaScript",
		szClassId,
		sizeof(szClassId)
		);
	if (FAILED(hr)) {
		return E_FAIL;
	}
	//convert CLSID string to clsid

	bstrClsID = BSTRFROMANSI(szClassId);
	if (!bstrClsID)
		goto exit;
	hr = CLSIDFromString(bstrClsID, &clsid);
	SysFreeString(bstrClsID);
	if (FAILED(hr))
		goto exit;

	// Instantiate the script engine
	hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void**)&m_pios);
	if (FAILED(hr))
		goto exit;

	// Get the IActiveScriptParse interface, if any
	hr = m_pios->QueryInterface(IID_IActiveScriptParse, (void**) &m_pasp);
	if (FAILED(hr))
		goto exit;

	hr = m_pasp->InitNew();
	if (FAILED(hr))
		goto exit;

	// SetScriptSite to this
	hr = m_pios->SetScriptSite((IActiveScriptSite *)this);
	if (FAILED(hr))
		goto exit;
	hr = m_pios->SetScriptState(SCRIPTSTATE_INITIALIZED);

	// AddNamedItem for pUnk and set m_punkJSProxy to pUnk.
	// If we added JSProxy to the name space the store away the JSProxy objects punk.
	m_punkJSProxy = new CJSProxy;
	if( !m_punkJSProxy )
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}
	m_punkJSProxy->Init(pAPHA);
	hr = m_pios->AddNamedItem(L"JSProxy",SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS);
	if (FAILED(hr))
		goto exit;
	
	// Convert the ANSI script text to a bstr.
	bstrScriptText = BSTRFROMANSI(szScript);
	if (!bstrScriptText)
		goto exit;
	// Add the script text to the parser
	hr = m_pasp->ParseScriptText(
							 bstrScriptText,
							 NULL,
							 NULL,
							 NULL,
							 0,
							 0,
							 SCRIPTTEXT_ISEXPRESSION|SCRIPTTEXT_ISVISIBLE,
							 NULL,
							 &exceptinfo);
	
	SysFreeString(bstrScriptText);
	if (FAILED(hr))
		goto exit;

	hr = m_pios->SetScriptState(SCRIPTSTATE_STARTED);
	if (FAILED(hr))
		goto exit;
	// Now get the script dispatch and find the DISPID for the method just added.  since this is a single use dll
	// I can do this otherwise this would be bad.
	hr = m_pios->GetScriptDispatch(NULL,&m_pScriptDispatch);
	if (FAILED(hr))
		goto exit;
	hr = m_pScriptDispatch->GetIDsOfNames(IID_NULL,rgbstrNames,1,LOCALE_SYSTEM_DEFAULT,&m_Scriptdispid);
	if (FAILED(hr))
		goto exit;

	m_fInitialized = TRUE;

	return hr;

exit: // we come here if something fails  -  release everything and set to null.
	if (m_pios)
	{
		m_pios->Close();
		m_pios->Release();
	}
	if (m_pasp)
		m_pasp->Release();
	if (m_pScriptDispatch)
		m_pScriptDispatch->Release();
	m_pios = NULL;
	m_pasp = NULL;
	m_pScriptDispatch = NULL;
	m_Scriptdispid = -1;
	
	return hr;
}

STDMETHODIMP CScriptSite::DeInit()
{
	HRESULT hr = S_OK;

	if (m_pios)
	{
		hr = m_pios->Close();
		m_pios->Release();
	}
	if (m_pasp)
		m_pasp->Release();
	if (m_pScriptDispatch)
		m_pScriptDispatch->Release();
	m_pios = NULL;
	m_pasp = NULL;
	m_pScriptDispatch = NULL;
	m_Scriptdispid = -1;
	m_fInitialized = FALSE;

	return hr;
}

STDMETHODIMP CScriptSite::RunScript(LPCSTR szURL, LPCSTR szHost, LPSTR* result)
{
	HRESULT		hr = S_OK;
	UINT		puArgErr = 0;
	EXCEPINFO	excep;
	VARIANT		varresult;
	DISPPARAMS	dispparams;
	VARIANT		args[2]; // We always call with 2 args!

	
	if (!szURL || !szHost || !result)
		return E_POINTER;

	VariantInit(&varresult);

	dispparams.cArgs = 2;
	VariantInit(&args[0]);
	VariantInit(&args[1]);

	args[0].vt = VT_BSTR;
	args[1].vt = VT_BSTR;

	args[0].bstrVal = BSTRFROMANSI(szHost);
	args[1].bstrVal = BSTRFROMANSI(szURL);

	dispparams.rgvarg = args;

	dispparams.cNamedArgs = 0;	
	dispparams.rgdispidNamedArgs = NULL; 

	// Call invoke on the stored dispid
	hr = m_pScriptDispatch->Invoke(m_Scriptdispid,
								   IID_NULL,LOCALE_SYSTEM_DEFAULT,
								   DISPATCH_METHOD,
								   &dispparams,
								   &varresult,
								   &excep,
								   &puArgErr);

	// convert result into bstr and return ansi version of the string!
	if (varresult.vt == VT_BSTR)
	{
		MAKE_ANSIPTR_FROMWIDE(rescpy, varresult.bstrVal);
		*result = (LPSTR) GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,lstrlen(rescpy)+1);
		if (!*result)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}
		lstrcpy(*result,rescpy);

	}
	else
	{
		VARIANT	resvar;

		VariantInit(&resvar);
		hr = VariantChangeType(&resvar,&varresult,NULL,VT_BSTR);
		if (SUCCEEDED(hr))
		{
			MAKE_ANSIPTR_FROMWIDE(rescpy, resvar.bstrVal);
			*result = (LPSTR) GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,lstrlen(rescpy)+1);
			if (!*result)
			{
				hr = E_OUTOFMEMORY;
				VariantClear(&resvar);
				goto Cleanup;
			}
			lstrcpy(*result,rescpy);
		}
		else
			*result = NULL;
		VariantClear(&resvar);
	}

Cleanup:
    VariantClear(&varresult);
	VariantClear(&args[0]);
	VariantClear(&args[1]);
	
	return hr;
}

STDMETHODIMP CScriptSite::GetLCID(LCID *plcid)
{
	return E_NOTIMPL;
}
STDMETHODIMP CScriptSite::GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppunkItem, ITypeInfo **ppTypeInfo)
{
	if (!pstrName || !ppunkItem)
		return E_POINTER;

	if ((StrCmpW(L"JSProxy",pstrName) == 0) && (dwReturnMask == SCRIPTINFO_IUNKNOWN))
	{
		*ppunkItem = (LPUNKNOWN)(IDispatch*)(CJSProxy*)m_punkJSProxy;
		(*ppunkItem)->AddRef();
		return S_OK;
	}
	else
		return TYPE_E_ELEMENTNOTFOUND;
}
STDMETHODIMP CScriptSite::GetDocVersionString(BSTR *pstrVersionString)
{
	return E_NOTIMPL;
}

// I am not interested it the transitioning of state or the status of where we are in
// the executing of the script.
STDMETHODIMP CScriptSite::OnScriptTerminate(const VARIANT *pvarResult,const EXCEPINFO *pexcepinfo)
{
	return S_OK;
}
STDMETHODIMP CScriptSite::OnStateChange(SCRIPTSTATE ssScriptState)
{
	return S_OK;
}
STDMETHODIMP CScriptSite::OnScriptError(IActiveScriptError *pase)
{
	return S_OK;
}
STDMETHODIMP CScriptSite::OnEnterScript()
{
	return S_OK;
}
STDMETHODIMP CScriptSite::OnLeaveScript()
{
	return S_OK;
}
