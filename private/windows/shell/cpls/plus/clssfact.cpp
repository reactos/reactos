//---------------------------------------------------------------------------
//
//	File: CLSSFACT.CPP
//
//	Implements a class factory object that creates CPropSheetExt objects.
//
//---------------------------------------------------------------------------

#include "clssfact.h"


extern ULONG g_cObj;	// See PLUSTAB.CPP
extern ULONG g_cLock;	// See PLUSTAB.CPP


//---------------------------------------------------------------------------
//	ObjectDestroyed()
//
//	Function for the CPropSheetExt object to call when it is destroyed.
//	Because we're in a DLL, we only track the number of objects here,
//	letting DllCanUnloadNow take care of the rest.
//---------------------------------------------------------------------------
void FAR PASCAL ObjectDestroyed( void )
{
	g_cObj--;
	return;
}


//---------------------------------------------------------------------------
//	Class Member functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	Constructor
//---------------------------------------------------------------------------
CClassFactory::CClassFactory()
{
	m_cRef = 0L;
	return;
}

//---------------------------------------------------------------------------
//	Destructor
//---------------------------------------------------------------------------
CClassFactory::~CClassFactory( void )
{
	return;
}

//---------------------------------------------------------------------------
//	QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    //Any interface on this object is the object pointer.
    if( IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_IClassFactory ) )
	{
        *ppv = (LPVOID)this;
		++m_cRef;
		return NOERROR;
	}
	return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//	AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
	return ++m_cRef;
}

//---------------------------------------------------------------------------
//	Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::Release()
{
ULONG cRefT;

	cRefT = --m_cRef;

	if( 0L == m_cRef )
		delete this;

	return cRefT;
}

//---------------------------------------------------------------------------
//	CreateInstance()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR *ppvObj )
{
CPropSheetExt*	pObj;
HRESULT		hr = E_OUTOFMEMORY;

	*ppvObj = NULL;

    // We don't support aggregation at all.
	if( pUnkOuter )
	{
		return CLASS_E_NOAGGREGATION;
	}

	//Verify that a controlling unknown asks for IShellPropSheetExt
	if( IsEqualIID( riid, IID_IShellPropSheetExt ) )
	{
		//Create the object, passing function to notify on destruction
		pObj = new CPropSheetExt( pUnkOuter, ObjectDestroyed );

		if( NULL == pObj )
		{
			return hr;
		}
		hr = pObj->QueryInterface( riid, ppvObj );

		//Kill the object if initial creation or FInit failed.
		if( FAILED(hr) )
		{
			delete pObj;
		}
		else
		{
			g_cObj++;
		}
		return hr;
	}
	return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//	LockServer()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer( BOOL fLock )
{
	if( fLock )
	{
		g_cLock++;
	}
	else
	{
		g_cLock--;
	}
	return NOERROR;
}
