// comtest.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "comtest.h"


ComTestBase::ComTestBase()
{
	OutputDebugStringW(L"ComTestBase");
}

ComTestBase::~ComTestBase()
{
	OutputDebugStringW(L"~ComTestBase");
}

STDMETHODIMP ComTestBase::test(void)
{
	return S_OK;
}

