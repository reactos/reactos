//--------------------------------------------------------------------------------
//
//	File:	CLSSFACT.H
//
//	Defines the CClassFactory object.
//
//--------------------------------------------------------------------------------

#ifndef _CLSSFACT_H_
#define _CLSSFACT_H_

#include "propsext.h"

void FAR PASCAL ObjectDestroyed();

//This class factory object creates CPropSheetExt objects.
class CClassFactory : public IClassFactory
{
protected:
	ULONG	m_cRef;

public:
	CClassFactory();
	~CClassFactory();

	//IUnknown members
	STDMETHODIMP		 QueryInterface( REFIID, LPVOID* );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance( LPUNKNOWN, REFIID, LPVOID* );
	STDMETHODIMP		LockServer( BOOL );
};

#endif //_CLSSFACT_H_
