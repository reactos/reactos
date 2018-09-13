// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"
#include "dispexa.h"


HRESULT
CDispExArray::HrGetLength(ULONG* pLength)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;
	VARIANT var;
	BSTR bstrName = NULL;
	WCHAR* oleStr = NULL;
	DISPID dispid;
	DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

	if (!m_piDispEx)
		return E_UNEXPECTED;

	_ASSERTE(pLength);
	if (NULL == pLength)
		return E_INVALIDARG;

	VariantInit(&var);
	oleStr = T2OLE(_T("length"));
	bstrName = ::SysAllocString(oleStr);

	if (FAILED(hr = m_piDispEx->GetDispID(bstrName, fdexNameCaseSensitive, &dispid)))
		goto cleanup;

	::SysFreeString(bstrName);

	hr = m_piDispEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, 
				DISPATCH_PROPERTYGET, &dispparamsNoArgs, 
				&var, NULL, NULL);

	if (FAILED(hr))
		goto cleanup;


	if (V_VT(&var) != VT_I4)
		goto cleanup;

	*pLength = V_I4(&var);

cleanup:
	if (FAILED(hr))
	{
		if (bstrName)
			::SysFreeString(bstrName);
	}

	return hr;
}


HRESULT 
CDispExArray::HrGetElement(ULONG index, LPVARIANT pVar)
{

	USES_CONVERSION;
	HRESULT hr = S_OK;
	VARIANT var;
	BSTR bstrName = NULL;
	WCHAR* oleStr = NULL;
	DISPID dispid = 0;
	DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
	char buffer[32];

	if (!m_piDispEx)
		return E_UNEXPECTED;

	_ASSERTE(pVar);
	if (NULL == pVar)
		return E_INVALIDARG;

	VariantInit(&var);
	oleStr = A2OLE(_itoa(index, buffer, 10));
	bstrName = ::SysAllocString(oleStr);

	if (FAILED(hr = m_piDispEx->GetDispID(bstrName, fdexNameCaseSensitive, &dispid)))
		goto cleanup;

	SysFreeString(bstrName);

	hr = m_piDispEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, 
				DISPATCH_PROPERTYGET, &dispparamsNoArgs, 
				pVar, NULL, NULL);

	if (FAILED(hr))
		goto cleanup;

cleanup:

	if (FAILED(hr))
	{
		if (bstrName)
			::SysFreeString(bstrName);
	}

	return hr;
}
