/********************************************************************************
/	This is the base file to the Microsoft JScript Proxy Configuration 
/	This file implements the code to provide the script site and the JSProxy psuedo
/	object for the script engine to call against.
/
/	Created		11/27/96	larrysu
/
/
/
/
/
/
/
/
/
*/
#include "jsproxy.h"


/*******************************************************************************
*	JSProxy functions.
********************************************************************************/
CJSProxy::CJSProxy()
{
	m_refCount = 1;
	m_fDestroyable = FALSE;
	m_fInitialized = FALSE;
	m_pCallout = NULL;
}

CJSProxy::~CJSProxy()
{
	if(m_fInitialized)
		DeInit();
}

STDMETHODIMP CJSProxy::Init(AUTO_PROXY_HELPER_APIS* pAPHA)
{

	m_strings[0] = L"isPlainHostName";
	m_strings[1] = L"dnsDomainIs";
	m_strings[2] = L"localHostOrDomainIs";
	m_strings[3] = L"isResolvable";
	m_strings[4] = L"isInNet";
	m_strings[5] = L"dnsResolve";
	m_strings[6] = L"myIpAddress";
	m_strings[7] = L"dnsDomainLevels";
	m_strings[8] = L"shExpMatch";
	m_strings[9] = L"weekdayRange";
	m_strings[10] = L"dateRange";
	m_strings[11] = L"timeRange";
	m_strings[12] = L"alert";
	
	m_pCallout = pAPHA;
	m_fInitialized = TRUE;
	return S_OK;
}

STDMETHODIMP CJSProxy::DeInit()
{
	m_pCallout = NULL;
	m_fInitialized = FALSE;
	return S_OK;
}
//IDispatch functions for JSProxy.  I want these to be light and fast.
STDMETHODIMP CJSProxy::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames,UINT cNames, LCID lcid, DISPID FAR* rgdispid)
{
	// Use addition of first 4 chars to make quick cheesy hash of which function wants to be called.
	// These are values are #defined in JSProxy.h
	HRESULT		hr = S_OK;
	long		strval = 0;
	unsigned long		nameindex = 0;
	OLECHAR*	currentName = NULL;

	if (!*rgszNames)
		return E_POINTER;
	if (cNames < 1)
		return E_INVALIDARG;

	while (nameindex < cNames)
	{
		currentName = rgszNames[nameindex];
		if (currentName == NULL)
			break;
		
		strval = currentName[0]+currentName[1]+currentName[2]+currentName[3]+currentName[4];

		switch (strval)
		{

			case VAL_myIpAddress :
					if (StrCmpW(m_strings[6],currentName) == 0)
						rgdispid[nameindex] = DISPID_myIpAddress;
					else
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;

			case VAL_isInNet :
					if (StrCmpW(m_strings[4],currentName) == 0)
						rgdispid[nameindex] = DISPID_isInNet;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_dateRange :
					if (StrCmpW(m_strings[10],currentName) == 0)
						rgdispid[nameindex] = DISPID_dateRange;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_dnsDomainIs : // This is also VAL_dnsDomainLevels check for both strings.
					if (StrCmpW(m_strings[7],currentName) == 0)
						rgdispid[nameindex] = DISPID_dnsDomainLevels;
					else 
					{
						if (StrCmpW(m_strings[1],currentName) == 0)
							rgdispid[nameindex] = DISPID_dnsDomainIs;
						else 
						{
							rgdispid[nameindex] = DISPID_UNKNOWN;
							hr = DISP_E_UNKNOWNNAME;
						}
					}
					break;
			
			case VAL_isPlainHostName :
					if (StrCmpW(m_strings[0],currentName) == 0)
						rgdispid[nameindex] = DISPID_isPlainHostName;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_dnsResolve :
					if (StrCmpW(m_strings[5],currentName) == 0)
						rgdispid[nameindex] = DISPID_dnsResolve;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_timeRange :
					if (StrCmpW(m_strings[11],currentName) == 0)
						rgdispid[nameindex] = DISPID_timeRange;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_isResolvable :
					if (StrCmpW(m_strings[3],currentName) == 0)
						rgdispid[nameindex] = DISPID_isResolvable;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_shExpMatch :
					if (StrCmpW(m_strings[8],currentName) == 0)
						rgdispid[nameindex] = DISPID_shExpMatch;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_localHostOrDomainIs :
					if (StrCmpW(m_strings[2],currentName) == 0)
						rgdispid[nameindex] = DISPID_localHostOrDomainIs;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_weekdayRange :
					if (StrCmpW(m_strings[9],currentName) == 0)
						rgdispid[nameindex] = DISPID_weekdayRange;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			case VAL_alert :
					if (StrCmpW(m_strings[12],currentName) == 0)
						rgdispid[nameindex] = DISPID_alert;
					else 
					{
						rgdispid[nameindex] = DISPID_UNKNOWN;
						hr = DISP_E_UNKNOWNNAME;
					}
					break;
			
			default :
					rgdispid[nameindex] = DISPID_UNKNOWN;
					hr = DISP_E_UNKNOWNNAME;
					break;

		}
		nameindex++;
	}
	return hr;
	
}

STDMETHODIMP CJSProxy::Invoke(
				DISPID dispidMember,
				REFIID riid,
				LCID lcid,
				WORD wFlags,
				DISPPARAMS* pdispparams,
				VARIANT* pvarResult,
				EXCEPINFO* pexcepinfo,
				UINT* puArgErr)
{
	HRESULT hr = S_OK;
	
	if (dispidMember > 0x000d)
		return DISP_E_MEMBERNOTFOUND;

	if (!(wFlags & (DISPATCH_PROPERTYGET|DISPATCH_METHOD)))
	{
		return DISP_E_MEMBERNOTFOUND;
	}

	// The big switch based on DISPID!
	switch (dispidMember)
	{
/*****************************************************************************
	Calling isPlainHostName 
*****************************************************************************/
	case DISPID_isPlainHostName :
		{
			// look in the DISPARAMS to make sure the signiture is correct for this function.
			if (pdispparams->cArgs != 1)
				hr = DISP_E_BADPARAMCOUNT;
			if (pdispparams->cNamedArgs > 0)
				hr = DISP_E_NONAMEDARGS;

			if (FAILED(hr))
				break;
			
			VARIANT arg1;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call isPlainHostName.
			hr = isPlainHostName(arg1.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling dnsDomainIs
*****************************************************************************/
	case DISPID_dnsDomainIs :
		{
			if (pdispparams->cArgs != 2)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT arg1;
			VARIANT arg2;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg2 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg2,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			if (pdispparams->rgvarg[1].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[1];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[1]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call dnsDomainIs
			hr = dnsDomainIs(arg1.bstrVal,arg2.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling localHostOrDomainIs
*****************************************************************************/
	case DISPID_localHostOrDomainIs :
		{
			if (pdispparams->cArgs != 2)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT arg1;
			VARIANT arg2;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg2 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg2,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			if (pdispparams->rgvarg[1].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[1];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[1]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call localHostOrDomainIs
			hr = localHostOrDomainIs(arg1.bstrVal,arg2.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling isResolvable
*****************************************************************************/
	case DISPID_isResolvable :
		{
			if (pdispparams->cArgs != 1)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT arg1;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call isResolvable
			hr = isResolvable(arg1.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling isInNet
*****************************************************************************/
	case DISPID_isInNet :
		{
			int x;

			if (pdispparams->cArgs != 3)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT args[3];
			
			for (x=0;x<3;x++)
			{
				// check the type of the variant in the disparams and if it is a bstr use it
				if (pdispparams->rgvarg[x].vt == VT_BSTR)
					args[x] = pdispparams->rgvarg[x];
				// otherwise change it into one!  if this fails the return an error.
				else
				{
					hr = VariantChangeType(&args[x],&(pdispparams->rgvarg[x]),NULL,VT_BSTR);
					if (FAILED(hr))
					{
						hr = DISP_E_TYPEMISMATCH;
						break;
					}
				}
			}
			if (FAILED(hr))
				break;
			// call isInNet.  Args need to be reversed
			hr = isInNet(args[2].bstrVal,args[1].bstrVal,args[0].bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling dnsResolve
*****************************************************************************/
	case DISPID_dnsResolve :
		{
			if (pdispparams->cArgs != 1)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT arg1;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call dnsResolve
			hr = dnsResolve(arg1.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling myIpAddress
*****************************************************************************/
	case DISPID_myIpAddress :
		// Should have no args and 1 named arg and the name should be DISPATCH_PROPERTYGET!
/*		if (pdispparams->cNamedArgs != 1)
		{
			hr = DISP_E_BADPARAMCOUNT;
			break;
		}
*/
		// call myIpAddress
		hr = myIpAddress(pvarResult);
		break;
/*****************************************************************************
	Calling dnsDomainLevels
*****************************************************************************/
	case DISPID_dnsDomainLevels :
		{
			if (pdispparams->cArgs != 1)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT arg1;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call dnsDomainLevels
			hr = dnsDomainLevels(arg1.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling shExpMatch
*****************************************************************************/
	case DISPID_shExpMatch :
		{
			if (pdispparams->cArgs != 2)
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT arg1;
			VARIANT arg2;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg2 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg2,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			if (pdispparams->rgvarg[1].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[1];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[1]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call isPlainHostName.
			hr = shExpMatch(arg1.bstrVal,arg2.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Calling weekdayRange
*****************************************************************************/
	case DISPID_weekdayRange :
		{
			unsigned int x;

			if ((pdispparams->cArgs > 3) || (pdispparams->cArgs < 1))
			{
				hr = DISP_E_BADPARAMCOUNT;
				break;
			}
			if (pdispparams->cNamedArgs > 0)
			{
				hr = DISP_E_NONAMEDARGS;
				break;
			}
			
			VARIANT* args[3] = {NULL,NULL,NULL};
			
			for (x=0;x<pdispparams->cArgs;x++)
			{
				args[x] = new(VARIANT);
                if( !(args[x]) )
                {
                       hr = E_OUTOFMEMORY;
                       break;
                }

				// check the type of the variant in the disparams and if it is a bstr use it
				if (pdispparams->rgvarg[x].vt == VT_BSTR)
					*args[x] = pdispparams->rgvarg[x];
				// otherwise change it into one!  if this fails the return an error.
				else
				{
					hr = VariantChangeType(args[x],&(pdispparams->rgvarg[x]),NULL,VT_BSTR);
					if (FAILED(hr))
					{
						hr = DISP_E_TYPEMISMATCH;
						break;
					}
				}
			}
			if (FAILED(hr))
				break;
			// call isInNet.  Args need to be reversed
			switch (pdispparams->cArgs)
			{
			case 1:
				hr = weekdayRange(args[0]->bstrVal,NULL,NULL,pvarResult);
				break;
			case 2:
				if ((args[0]->bstrVal[0] == 'G') || (args[0]->bstrVal[0] == 'g'))
					hr = weekdayRange(args[1]->bstrVal,NULL,args[0]->bstrVal,pvarResult);
				else
					hr = weekdayRange(args[1]->bstrVal,args[0]->bstrVal,NULL,pvarResult);
				break;
			case 3:
				hr = weekdayRange(args[2]->bstrVal,args[1]->bstrVal,args[0]->bstrVal,pvarResult);
				break;
			}
			break;
		}
/*****************************************************************************
	Calling dateRange
*****************************************************************************/
	case DISPID_dateRange :
		break;
/*****************************************************************************
	Calling timeRange
*****************************************************************************/
	case DISPID_timeRange :
		break;
/*****************************************************************************
	Calling alert 
*****************************************************************************/
	case DISPID_alert :
		{
			// look in the DISPARAMS to make sure the signiture is correct for this function.
			if (pdispparams->cArgs != 1)
				hr = DISP_E_BADPARAMCOUNT;
			if (pdispparams->cNamedArgs > 0)
				hr = DISP_E_NONAMEDARGS;

			if (FAILED(hr))
				break;
			
			VARIANT arg1;
			
			// check the type of the variant in the disparams and if it is a bstr use it
			if (pdispparams->rgvarg[0].vt == VT_BSTR)
				arg1 = pdispparams->rgvarg[0];
			// otherwise change it into one!  if this fails the return an error.
			else
			{
				hr = VariantChangeType(&arg1,&(pdispparams->rgvarg[0]),NULL,VT_BSTR);
				if (FAILED(hr))
				{
					hr = DISP_E_TYPEMISMATCH;
					break;
				}
			}
			// call alert.
			hr = alert(arg1.bstrVal,pvarResult);
			break;
		}
/*****************************************************************************
	Default returning error code
*****************************************************************************/
	default:
		hr = DISP_E_MEMBERNOTFOUND;
	}

	return hr;
}


//  JScript Auto-Proxy config functions.
STDMETHODIMP CJSProxy::isPlainHostName(BSTR host, VARIANT* retval)
{
	WCHAR	*currentch;
	BOOL	bfound = FALSE;

	if (!host || !retval)
		return E_POINTER;

	retval->vt = VT_BOOL;

	// check to detemine whether this is a plain host name!
	currentch = host;
	while ((*currentch != '\0') && !bfound)
	{
		if (*currentch == '.')
			bfound = TRUE;
		else
			currentch++;
	}

	if (bfound)
		retval->boolVal = VARIANT_FALSE;
	else
		retval->boolVal = VARIANT_TRUE;

	return S_OK;
}

STDMETHODIMP CJSProxy::dnsDomainIs(BSTR host,BSTR domain, VARIANT* retval)
{
	WCHAR *result = NULL;

	if (!host || !domain || !retval)
		return E_POINTER;
	
	result = StrStrW(host,domain);
	retval->vt = VT_BOOL;
	if (result)
		retval->boolVal = VARIANT_TRUE;
	else
		retval->boolVal = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CJSProxy::localHostOrDomainIs(BSTR host,BSTR hostdom, VARIANT* retval)
{
	HRESULT	hr = S_OK;

	if (!host || !hostdom || !retval)
		return E_POINTER;

	// check to see if it is a local host
	hr = isPlainHostName(host,retval);
	if (SUCCEEDED(hr))
	{
		if (retval->boolVal != VARIANT_TRUE)
        {
            //
            // this is a strange function, if its not a local hostname
            //  we do a strait compare against the passed in domain
            //  string.  If its not a direct match, then its FALSE,
            //  even if the root of the domain/hostname are the same.
            //  Blame Netscape for this, we are just following their
            //  behavior and docs.
            //

            if ( StrCmpIW(host, hostdom) == 0 )
            {
                retval->boolVal = VARIANT_TRUE;
            }
            else
            {
                retval->boolVal = VARIANT_FALSE;
            }

        }
	}

	return hr;
}

// Functions that need to call back on wininet.
STDMETHODIMP CJSProxy::isResolvable(BSTR host, VARIANT* retval)
{
	
	if (!host || !retval)
		return E_POINTER;
	// call into wininet provided functions!
	retval->vt = VT_BOOL;
	if (m_pCallout)
	{
		MAKE_ANSIPTR_FROMWIDE(szhost,host);
		if (m_pCallout->IsResolvable(szhost)) 
			retval->boolVal = VARIANT_TRUE;
		else
			retval->boolVal = VARIANT_FALSE;
	}
	else
		retval->boolVal = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CJSProxy::isInNet(BSTR host, BSTR pattern, BSTR mask, VARIANT* retval)
{
	VARIANT	myretval;
	HRESULT	hr = S_OK;
	
	// call into wininet provided functions!
	if (!host || !pattern || !mask || !retval)
		return E_POINTER;
	// call into wininet provided functions!
	retval->vt = VT_BOOL;
	VariantInit(&myretval);

	if (m_pCallout)
	{
		hr = dnsResolve(host,&myretval);
		if (SUCCEEDED(hr))
		{
			if (myretval.vt != VT_BSTR)
			{	
				VariantClear(&myretval);
				retval->boolVal = VARIANT_FALSE;		
				return hr;
			}
		}
		else
		{
			VariantClear(&myretval);
			retval->boolVal = VARIANT_FALSE;
			return hr;	
		}

		// Fallthrough to code to check IP/pattern and mask!
	
		MAKE_ANSIPTR_FROMWIDE(szhost,myretval.bstrVal);
		MAKE_ANSIPTR_FROMWIDE(szpattern,pattern);
		MAKE_ANSIPTR_FROMWIDE(szmask,mask);

		//  Check to see if IP address from dnsResolve matches the pattern/mask!
        if ( m_pCallout->IsInNet(szhost, szpattern, szmask ) ) 
			retval->boolVal = VARIANT_TRUE;
		else
			retval->boolVal = VARIANT_FALSE;
	}
	else
		retval->boolVal = VARIANT_FALSE;
	
	VariantClear(&myretval);
	return S_OK;
}

STDMETHODIMP CJSProxy::dnsResolve(BSTR host, VARIANT* retval)
{
	char ipaddress[16];
	DWORD dwretval;
	DWORD dwipsize = 16;

	if (!host || !retval)
		return E_POINTER;
	// call into wininet provided functions!

	if (m_pCallout)
	{
		MAKE_ANSIPTR_FROMWIDE(szhost,host);
		dwretval = m_pCallout->ResolveHostName(szhost,ipaddress,&dwipsize); 
		if (dwretval == ERROR_SUCCESS)
		{
			retval->vt = VT_BSTR;
			retval->bstrVal = MakeWideStrFromAnsi((LPSTR)ipaddress,STR_BSTR);
		}
		else
		{
			retval->vt = VT_BOOL;
			retval->boolVal = VARIANT_FALSE;
		}
	}
	else
	{	
		retval->vt = VT_BOOL;
		retval->boolVal = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP CJSProxy::myIpAddress(VARIANT* retval)
{
	char ipaddress[16];
	DWORD dwretval;
	DWORD dwipsize = 16;

	if (!retval)
		return E_POINTER;
	// call into wininet provided functions!

	if (m_pCallout)
	{
		dwretval = m_pCallout->GetIPAddress(ipaddress,&dwipsize);
		if (dwretval == ERROR_SUCCESS)
		{
			retval->vt = VT_BSTR;
			retval->bstrVal = MakeWideStrFromAnsi((LPSTR)ipaddress,STR_BSTR);
		}
		else
		{
			retval->vt = VT_BOOL;
			retval->boolVal = VARIANT_FALSE;
		}
	}
	else
	{	
		retval->vt = VT_BOOL;
		retval->boolVal = VARIANT_FALSE;
	}

	return S_OK;
}

// Back to functions implemented here.
STDMETHODIMP CJSProxy::dnsDomainLevels(BSTR host, VARIANT* retval)
{
	WCHAR	*currentch;
	DWORD	dwlevels = 0;

	if (!host || !retval)
		return E_POINTER;

	retval->vt = VT_I4;

	// check to detemine whether this is a plain host name!
	currentch = host;
	while (*currentch != L'\0')
	{
		if (*currentch == L'.')
			dwlevels++;

		currentch++;
	}

	retval->lVal = dwlevels;

	return S_OK;
}

STDMETHODIMP CJSProxy::shExpMatch(BSTR str, BSTR shexp, VARIANT* retval)
{

	if (!str || !shexp || !retval)
		return E_POINTER;

	retval->vt = VT_BOOL;
	// convert BSTR to ansi - these macros allocate memory that is freed when they
	// go out of scope!  No need to free!
	MAKE_ANSIPTR_FROMWIDE(szstr, str);
	MAKE_ANSIPTR_FROMWIDE(szshexp, shexp);
	// Call into the regular expression matching code.
	if (match(szstr,szshexp))
		retval->boolVal = VARIANT_TRUE;
	else
		retval->boolVal = VARIANT_FALSE;

	return S_OK;
}

// These are to do last!!!.
STDMETHODIMP CJSProxy::weekdayRange(BSTR wd1, BSTR wd2, BSTR gmt, VARIANT* retval)
{
	SYSTEMTIME	systime;
	SYSTEMTIME	loctime;
	char		szday[4];
	int			today = -1;
	int			day1 = -1; // days are as follows SUN = 0; MON = 1; ...;SAT = 6.
	int			day2 = -1;  
	BOOL		bIsInRange = FALSE;

	if (!wd1)
		return E_POINTER;
	if (gmt)
		GetSystemTime(&systime);

	GetDateFormat(//LOCALE_SYSTEM_DEFAULT,
					MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT),
					NULL,
					gmt? &systime:NULL,
					"ddd",
					szday,
					4);

	if (szday)
	{
		int lcv;
		//convert all chars to upper if lowercase (don't use runtimes)
		for (lcv=0;lcv<3;lcv++)
		{
			if ((short)szday[lcv] > 90)
				szday[lcv]-=32;
		}

		today = ConvertAnsiDayToInt(szday);
	}
	
	if (today == -1)
		return E_FAIL;
	
	// compare day ranges!
	if (wd2)
	{
		// These are by definition in ALL CAPS
		MAKE_ANSIPTR_FROMWIDE(szwd1, wd1);
		MAKE_ANSIPTR_FROMWIDE(szwd2, wd2);
		if (szwd1 && szwd2)
		{
			day1 = ConvertAnsiDayToInt(szwd1);
			day2 = ConvertAnsiDayToInt(szwd2);
		}

		if ((day1 == -1) || (day2 == -1))
			return E_INVALIDARG;

		if (day1 < day2)
		{
			if ((today >= day1) && (today <= day2))
				bIsInRange = TRUE;
			else
				bIsInRange = FALSE;
		}
        else if ( day1 == day2 )
        {
            if (today == day1)
            {
                bIsInRange = TRUE;
            }
            else
            {
                bIsInRange = FALSE;
            }
        }
		else
		{
			if ((today >= day1) || (today <= day2))
                bIsInRange = TRUE;
			else
				bIsInRange = FALSE;
		}

	}
	else // only one day to check!
	{
		MAKE_ANSIPTR_FROMWIDE(szwd1, wd1);
		if (lstrcmp(szday,szwd1) == 0)
			bIsInRange = TRUE;
		else
			bIsInRange = FALSE;
	}

	if (bIsInRange)
	{
		retval->vt = VT_BOOL;
		retval->boolVal = VARIANT_TRUE;
	}
	else
	{
		retval->vt = VT_BOOL;
		retval->boolVal = VARIANT_FALSE;
	}

	return S_OK;
}

STDMETHODIMP CJSProxy::dateRange(long day, BSTR month, BSTR gmt, VARIANT* retval)
{
	return S_OK;
}
STDMETHODIMP CJSProxy::timeRange(long hour, long min, long sec, BSTR gmt, VARIANT* retval)
{
	return S_OK;
}

STDMETHODIMP CJSProxy::alert(BSTR message, VARIANT* retval)
{
    if (!message)
        return E_POINTER;

    // Return true if available...not needed?
    if (retval)
    {
	    retval->vt = VT_BOOL;
        retval->vt = VARIANT_TRUE;
    }

    MAKE_ANSIPTR_FROMWIDE(szMessage,message);

    // Display the alert which isn't truly modal to the browser.
    // Getting the appropriate window handle will be quite a chore from here.
    MessageBox(
        NULL,
        szMessage,
        TEXT("Microsoft Internet Explorer"),
        MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST | MB_TASKMODAL
        );

    return S_OK;
}

// don't yet know what to do with ProxyConfig.bindings.
//ProxyConfig.bindings 
