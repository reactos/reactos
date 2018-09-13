/*
 * @(#)ScriptSite.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL ScriptSite object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "processor.hxx"
#include "scriptsite.hxx"
#include "securitymanager.hxx"

/* allow developer to set registry to allow debugging of xsl script, default is off */
static const TCHAR s_RegKeyMSXML[] = TEXT("Software\\Microsoft\\MSXML");
static const TCHAR s_XSLScriptDebug[] = TEXT("AllowXSLScriptDebug");
static bool fDebugXSLScript = false;
static bool fCheckRegistry = true;
static bool DoDebugXSLScript()
{
    HKEY hKey;
    DWORD dwResult, dwType, dwSize, dwValue;

    if (fCheckRegistry)
    {
        fCheckRegistry = false;
        dwType = REG_DWORD;
        dwSize = sizeof(dwValue);
        dwValue = 0xFFFF;

        dwResult = RegOpenKeyEx(
            HKEY_CURRENT_USER, 
            s_RegKeyMSXML,
            0,
            KEY_QUERY_VALUE,
            &hKey);

        if (ERROR_SUCCESS == dwResult)
        {
            dwResult = RegQueryValueEx(
                           hKey, 
                           s_XSLScriptDebug,
                           NULL,
                           &dwType,
                           (LPBYTE) &dwValue,
                           &dwSize);
            RegCloseKey(hKey);
        }
        fDebugXSLScript = (dwValue == 1);
    }
    return fDebugXSLScript;
}

DEFINE_CLASS_MEMBERS(ScriptSite, _T("ScriptSite"), Base);

ScriptSite *
ScriptSite::newScriptSite(Processor * xtl)
{
    return new ScriptSite(xtl);  
}


ScriptSite::ScriptSite(Processor * xtl)
{
    _xtl = xtl;
}


void 
ScriptSite::finalize()
{
    _xtl = null;
    super::finalize();
}


/**
 * IActiveScriptSite method.
 */
LCID 
ScriptSite::getLCID()
{
    return ::GetSystemDefaultLCID();
}

/**
 * IActiveScriptSite method.
 */
void 
ScriptSite::getItemInfo(String * pstrName, DWORD dwReturnMask, IUnknown ** ppunkItem, ITypeInfo ** ppTI)
{
	if (ppunkItem != null)
	{
		if (pstrName->equals(_T("Context")))
		{
            *ppunkItem = _xtl->getRuntimeObject();
            (*ppunkItem)->AddRef();
		}            
        else
        {
            *ppunkItem = null;
        }
	}
}

/**
 * IActiveScriptSite method.
 */
String * 
ScriptSite::getDocVersionString()
{
    // BUGBUG - Why this number???
	return String::newString(_T("Version 0.7"));
}

/**
 * IActiveScriptSite method.
 */
void 
ScriptSite::onScriptTerminate(Variant * pvarResult, const EXCEPINFO * pexcepinfo)
{
}

/**
 * IActiveScriptSite method.
 */
void 
ScriptSite::onStateChange(SCRIPTSTATE ssScriptState)
{
}

/**
 * IActiveScriptSite method.
 */
void 
ScriptSite::onScriptError(ActiveScriptError * pscripterror)
{
	TRY
	{
		EXCEPINFO excepinfo;
        DWORD   dw;
        ULONG   uline;
        LONG    col;
        String * source;
        StringBuffer * sb;

		pscripterror->getExceptionInfo(&excepinfo);
        pscripterror->getSourcePosition(&dw, &uline, &col);

        TRY
        {
            source = pscripterror->getSourceLineText();
        }
        CATCHE
        {
            source = null;
        }
        ENDTRY

        // BUGBUG - Should errors go to the output document.  For now this allows minimal script debugging.

        sb = StringBuffer::newStringBuffer(256);
        sb->append(String::newLineString());
        if (excepinfo.bstrSource)
        {
            sb->append(excepinfo.bstrSource);
            sb->append(String::newLineString());
        }

        if (excepinfo.bstrDescription)
        {
            sb->append(excepinfo.bstrDescription);
            sb->append(String::newLineString());
        }

        sb->append(Resources::FormatMessage(XSL_PROCESSOR_SCRIPTERROR_LINE, 
            String::newString((int) uline), 
            String::newString((int)col),
            null));

        if (source != null)
        {
            sb->append(source);
        } else
        {
            sb->append(Resources::FormatMessage(XSL_PROCESSOR_METHODERROR,null));
        }
        sb->append(String::newLineString());

        _lastError = sb->toString();

        _xtl->print(_lastError);

		SysFreeString(excepinfo.bstrDescription);
		SysFreeString(excepinfo.bstrSource);
		SysFreeString(excepinfo.bstrHelpFile);
	}
	CATCH
	{
	}
    ENDTRY
}

/**
 * IActiveScriptSite method.
 */
void 
ScriptSite::onEnterScript()
{
}

/**
 * IActiveScriptSite method.
 */
void 
ScriptSite::onLeaveScript()
{
}


/**
 * IUnknown::QueryInterface over-ride
 */
HRESULT 
ScriptSite::QueryInterface(REFIID iid, void ** ppvObject)
{
    HRESULT hr;

    if (NULL == ppvObject)
        return E_POINTER;

    if (iid == IID_IServiceProvider)
    {
        *ppvObject = (IServiceProvider *) this;
        AddRef();
        hr =  S_OK;
    }
    else if (iid == IID_IBindHost)
    {
        *ppvObject = (IBindHost *) this;
        AddRef();
        hr = S_OK;
    }
    else if (iid == IID_IActiveScriptSiteDebug && !DoDebugXSLScript())
    {
        *ppvObject = (IActiveScriptSiteDebug *) this;
        AddRef();
        hr =  S_OK;
    }
    else
        hr = super::QueryInterface(iid, ppvObject);

    return hr;
}


/**
 * IServiceProvider method
 */
HRESULT 
ScriptSite::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr;
    String *sURL;
    Document *pDoc;
    IUnknown *pUnkSite = NULL;
    IServiceProvider *pSPSite = NULL;

    if (guidService == SID_SInternetHostSecurityManager)
    {
        if (riid == IID_IXMLDOMDocument || riid == IID_IHTMLDocument2)
        {
            TRY
            {
                // this is on behalf of our SafeControl, in case the XSL Script 
                // instantiates an XML Document Object.  In trying to make that
                // object secure, it queries up to the host attempting to
                // set it's base URL.   
                
                // The xsl script might be contained in an XSL file, or it could
                // live as a snippet in an html page.  The CSafeControl will try
                // both, so we just pass the call to the xsl file if we have it, 
                // otherwise we pass it to the host of the script (usually an
                // html page)
                
                pDoc = _xtl->getStyleElement()->getDocument();
                Assert(pDoc != NULL);
                
                sURL = pDoc->getURL();
                if (sURL)
                {
                    hr = _xtl->getStyleElement()->getDocument()->QueryInterface(riid, ppv);
                }
                else
                {
                    pDoc->getSite(IID_IServiceProvider, (void **)&pSPSite);
                    if (!pSPSite)
                        hr = E_FAIL;
                    else
                        hr = pSPSite->QueryService(guidService, riid, ppv);
                }
            }
            CATCH
            {
                hr = ERESULTINFO;
                *ppv = NULL;
            }
            ENDTRY
        }
        else
        {
            SecurityManager *pSecMan = new_ne SecurityManager();
            if (pSecMan)
            {
                TRY
                {
                    pDoc = _xtl->getStyleElement()->getDocument();
                    Assert(pDoc != NULL);

                    // get security options and a site
                    DWORD dwSafetySupported, dwSafetyEnabled;
                    pDoc->getInterfaceSafetyOptions(IID_IUnknown, &dwSafetySupported, &dwSafetyEnabled);
                    pDoc->getSite(IID_IUnknown, (void **)&pUnkSite);

                    // now find out the URL where this xsl script is contained
                    // first see if there is an xsl document
                    sURL = pDoc->getURL();

                    // if not, try the host (e.g, such as the html page)
                    // since we're using this for security checking, use the SecureBaseURL, not the baseURL
                    if (sURL == NULL)
                        sURL = pDoc->GetSecureBaseURL();

                    // initialize the security manager
                    hr = pSecMan->Init(dwSafetyEnabled, pUnkSite, sURL);
                    if (SUCCEEDED(hr))
                        hr = pSecMan->QueryInterface(riid, ppv);

                    // This release is necessary to either 1) get the refcount to 1 
                    // which is where it should be, or to destroy the object if the 
                    // desired interface is not supported
                    pSecMan->Release();
                }
                CATCH
                {
                    pSecMan->Release();
                    hr = ERESULTINFO;
                }
                ENDTRY
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else if (guidService == SID_SBindHost)
    {
        hr = QueryInterface(riid, ppv);
    }
    else
    {
        // The Win32 SDK docs mention SVC_E_UNKNOWNSERVICE as the correct return
        // code here.  However, I couldn't find it in any of the header files.
        // Trident uses E_NOINTERFACE, so we will too.  simonb (10-01-1998)

        hr = E_NOINTERFACE;
        
    }

Cleanup:
    if (pUnkSite)
        pUnkSite->Release();
    if (pSPSite)
        pSPSite->Release();
    return hr;
}


/**
* IActiveScriptSiteDebug method.
*  Allows a smart host to control the handling of runtime errors 
*/
HRESULT STDMETHODCALLTYPE 
ScriptSite::OnScriptErrorDebug( 
    /* [in] */ IActiveScriptErrorDebug __RPC_FAR * pErrorDebug,
    /* [out] */ BOOL __RPC_FAR * pfEnterDebugger, 
    /* [out] */ BOOL __RPC_FAR * pfCallOnScriptErrorWhenContinuing)
{
    if (pfEnterDebugger)
        *pfEnterDebugger = DoDebugXSLScript() ? TRUE : FALSE;
    if (pfCallOnScriptErrorWhenContinuing)
        *pfCallOnScriptErrorWhenContinuing = TRUE;
    return S_OK;
}


/**
** IBindHost method.
** 
** This is the standard method by which ActiveX objects instantiated a script engine
** find out about the URL of the host document.  Objects will use this to make
** themselves secure and set their base URL in case they do relative URL processing.
*/
HRESULT STDMETHODCALLTYPE 
ScriptSite::CreateMoniker( 
        /* [in] */ LPOLESTR szName,
        /* [in] */ IBindCtx __RPC_FAR *pBC,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
        /* [in] */ DWORD dwReserved)
{
    HRESULT hr = S_OK;
    BSTR bstrURL = NULL;
    IMoniker *pmkBase = NULL;
    String *sURL;

    TRY
    {
        // null ptr is bogus but "" is valid
        if (szName == NULL) 
            return E_INVALIDARG;
     
        // Now find out the URL where this xsl script is contained.
        // We want to pass this as the base URL to objects that
        // the script has itself instantiated.
        // 
        // We look at the URL of the xsl document first BEFORE going
        // the BaseURL for the document.  This makes sense because
        // the script that executes lives in this xsl document, not
        // in the site for the document.  We would want references
        // in that xsl file to be relative to where it lives.
        //
        //
        // Failing that, go for the BaseURL of the xsl document itself.
        // The script could live in an HTML page that has BASE tags.
        sURL = _xtl->getStyleElement()->getDocument()->getURL();
        if (sURL == NULL)
            sURL = _xtl->getStyleElement()->getDocument()->GetBaseURL();
        
        // now grab the base url if any
        if (sURL)
        {
            bstrURL = sURL->getBSTR();
            if (!bstrURL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        
        // if the base url is non-existent, then just create the moniker from the supplied arg
        if (!bstrURL || *bstrURL == L'\0')
        {
            if (PathIsURLW(szName))
                hr = CreateURLMoniker(NULL, szName, ppmk);
            else
                hr = E_INVALIDARG;
        }
        
        // else relative moniker computation. We first create a moniker for the the base, 
        // then combine it with the supplied path.  We use this technique rather
        // than CoInternetCombineURL to support pluggable protocols
        else
        {
            hr = CreateURLMoniker(NULL, bstrURL, &pmkBase);
            if (FAILED(hr))
                goto Cleanup;
            
            hr = CreateURLMoniker(pmkBase, szName, ppmk);
        }
    }
    CATCH
    {
        hr = E_FAIL;
    }
    ENDTRY

Cleanup:
    ::SysFreeString(bstrURL);
    if (pmkBase)
        pmkBase->Release();
    return hr;
}
