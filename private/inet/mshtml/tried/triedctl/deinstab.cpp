// DEInsTab.cpp : Implementation of CDEInsertTableParam
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
#include "stdafx.h"
#include "DHTMLEd.h"
#include "DEInsTab.h"

/////////////////////////////////////////////////////////////////////////////
// CDEInsertTableParam

static WCHAR k_wszEmpty[] = L"";


CDEInsertTableParam::CDEInsertTableParam()
{
	m_nNumRows = 3;
	m_nNumCols = 3;
	m_bstrTableAttrs = ::SysAllocString(L"border=1 cellPadding=1 cellSpacing=1 width=75%");
	m_bstrCellAttrs = ::SysAllocString(k_wszEmpty);
	m_bstrCaption = ::SysAllocString(k_wszEmpty);
}

CDEInsertTableParam::~CDEInsertTableParam()
{
	::SysFreeString(m_bstrTableAttrs);
	m_bstrTableAttrs = NULL;
	::SysFreeString(m_bstrCellAttrs);
	m_bstrCellAttrs = NULL;
	::SysFreeString(m_bstrCaption);
	m_bstrCaption = NULL;
}

STDMETHODIMP CDEInsertTableParam::get_NumRows(LONG * pVal)
{
	_ASSERTE(pVal);

	*pVal = m_nNumRows;
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::put_NumRows(LONG newVal)
{
	m_nNumRows = newVal;
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::get_NumCols(LONG * pVal)
{
	_ASSERTE(pVal);

	*pVal = m_nNumCols;
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::put_NumCols(LONG newVal)
{
	m_nNumCols = newVal;
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::get_TableAttrs(BSTR * pVal)
{
	*pVal = ::SysAllocString(m_bstrTableAttrs);
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::put_TableAttrs(BSTR newVal)
{
	::SysFreeString(m_bstrTableAttrs);
	if ( NULL == newVal )
	{
		m_bstrTableAttrs = ::SysAllocString(k_wszEmpty);
	}
	else
	{
		m_bstrTableAttrs = ::SysAllocString(newVal);
	}
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::get_CellAttrs(BSTR * pVal)
{
	*pVal = ::SysAllocString(m_bstrCellAttrs);
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::put_CellAttrs(BSTR newVal)
{
	::SysFreeString(m_bstrCellAttrs);
	if ( NULL == newVal )
	{
		m_bstrCellAttrs = ::SysAllocString(k_wszEmpty);
	}
	else
	{
		m_bstrCellAttrs = ::SysAllocString(newVal);
	}
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::get_Caption(BSTR * pVal)
{
	*pVal = ::SysAllocString(m_bstrCaption);
	return S_OK;
}

STDMETHODIMP CDEInsertTableParam::put_Caption(BSTR newVal)
{
	::SysFreeString(m_bstrCaption);
	if ( NULL == newVal )
	{
		m_bstrCaption = ::SysAllocString(k_wszEmpty);
	}
	else
	{
		m_bstrCaption = ::SysAllocString(newVal);
	}
	return S_OK;
}
