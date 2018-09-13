//---------------------------------------------------------------------------
//
//	File: PROPSEXT.CPP
//
//	Defines the CPropSheetExt object.
//
//---------------------------------------------------------------------------

#ifndef _PROPSEXT_H_
#define _PROPSEXT_H_

#include <windows.h>

#include <prsht.h>
#include <shlobj.h>

//Type for an object-destroyed callback
typedef void (FAR PASCAL *LPFNDESTROYED)(void);


class CPropSheetExt : public IShellPropSheetExt
{
private:
	ULONG           m_cRef;
	LPUNKNOWN       m_pUnkOuter;    //Controlling unknown
	LPFNDESTROYED	m_pfnDestroy;	//Function closure call

public:
	CPropSheetExt( LPUNKNOWN pUnkOuter, LPFNDESTROYED pfnDestroy );
	~CPropSheetExt(void);

	// IUnknown members
	STDMETHODIMP		 QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // *** IShellPropSheetExt methods ***
    STDMETHODIMP		AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam );
    STDMETHODIMP		ReplacePage( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam );
};

#endif //_PROPSEXT_H_
