//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#include "stdafx.h"
#include "ShellDetails.h"

HRESULT CShellDetails::GetDetailsOf(LPCITEMIDLIST pidl,UINT iColumn,LPSHELLDETAILS pDetail)
{
	HRESULT hr=E_FAIL;
	if (m_pUnk == NULL)
		return hr;
	IShellFolder2 *pShellFolder2=NULL;
	hr = m_pUnk->QueryInterface(IID_IShellFolder2,(LPVOID*)&pShellFolder2);
	if (SUCCEEDED(hr))
	{
		hr = pShellFolder2->GetDetailsOf(pidl,iColumn,pDetail);
		pShellFolder2->Release();
	}
	else 
	{
		IShellDetails *pShellDetails=NULL;
		hr = m_pUnk->QueryInterface(IID_IShellDetails,(LPVOID*)&pShellDetails);
		if (SUCCEEDED(hr))
		{
			hr = pShellDetails->GetDetailsOf(pidl,iColumn,pDetail);
			pShellDetails->Release();
		}
	}
	return hr;
}

bool CShellDetails::IsValidDetails()
{
	return m_pUnk != NULL;
}

void CShellDetails::SetShellDetails(IUnknown *pUnk)
{
	FreeInterfaces();
	if (pUnk)
	{
		m_pUnk = pUnk;
		m_pUnk->AddRef();
	}
}

CShellDetails::CShellDetails(const CShellDetails &rOther)
 : 	m_pUnk(rOther.m_pUnk)
{
	m_pUnk->AddRef();
}

const CShellDetails &CShellDetails::operator=(const CShellDetails &rOther)
{
	if (this == &rOther)
		return *this;

	FreeInterfaces();
	m_pUnk = rOther.m_pUnk;
	m_pUnk->AddRef();

	return *this;
}

void CShellDetails::FreeInterfaces()
{
	if (m_pUnk)
	{
		m_pUnk->Release();
		m_pUnk = NULL;
	}
}

CShellDetails::CShellDetails()
{
	m_pUnk = NULL;
}

CShellDetails::~CShellDetails()
{
	FreeInterfaces();
}
