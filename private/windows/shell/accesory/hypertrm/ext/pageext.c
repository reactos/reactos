/*	File: D:\wacker\ext\pageext.c (Created: 01-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:20p $
 */

#define _INC_OLE		// WIN32, get ole2 from windows.h
#define CONST_VTABLE
#define INITGUID

#include <windows.h>
#pragma hdrstop
//
// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
//
#pragma data_seg(".text")
#include <objbase.h>
#include <initguid.h>
//#include <coguid.h>
//#include <oleguid.h>
#include <shlguid.h>
#include <shlobj.h>
#include "pageext.hh"
#pragma data_seg()

//
// Function prototypes
//
HRESULT CALLBACK PageExt_CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR*);

//
// Global variables
//
UINT g_cRefThisDll = 0;		// Reference count of this DLL.

//---------------------------------------------------------------------------
// DllCanUnloadNow
//---------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
	{
    return ResultFromScode((g_cRefThisDll==0) ? S_OK : S_FALSE);
	}

//---------------------------------------------------------------------------
//
// DllGetClassObject
//
//  This is the entry of this DLL, which all the In-Proc server DLLs should
// export. See the description of "DllGetClassObject" of OLE 2.0 reference
// manual for detail.
//
//---------------------------------------------------------------------------

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppvOut)
	{
    //
    //  This DLL has only one class (CLSID_SamplePageExt). If a DLL supports
    // multiple classes, it should have either multiple if-statements or
    // efficient table lookup code.
	//

// We need to put the icon handler in a separate DLL so when CAB32.EXE
// calls it we don't implicitly link to TAPI and other system DLL's.
// This is mostly for speed and to work around a bug in the Chicago
// beta 1.	Of course, we want to keep one source.	So the main DLL
// will link to the icon DLL to get icons, and the SHCreateDefClassObject(),
// etc.  Sorry for the complexity but its in the interest of system
// performance. - mrw

    if (IsEqualIID(rclsid, &CLSID_SamplePageExt))
		{
	//
	// We are supposed return the class object for this class. Instead
	// of fully implementing it in this DLL, we just call a helper
	// function in the shell DLL which creates a default class factory
	// object for us. When its CreateInstance member is called, it
	// will call back our create instance function (PageExt_CreateInstance).
	//
	return SHCreateDefClassObject(
			riid,
			ppvOut,
		    PageExt_CreateInstance, // callback function
			&g_cRefThisDll, 		// reference count of this DLL
		    &IID_IShellExtInit	    // init interface
			);

		}

    return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
	}


//---------------------------------------------------------------------------
//
// CSamplePageExt class
//
// In C++:
//  class CSamplePageExt : protected IShellPropSheetExt, protected IShellExtInit
//  {
//  protected:
//      UINT         _cRef;
//      LPDATAOBJECT _pdtobj;
//	HKEY	     _hkeyProgID;
//  public:
//      CSamplePageExt() _cRef(1), _pdtobj(NULL), _hkeyProgID(NULL) {};
//      ...
//  };
//
//---------------------------------------------------------------------------
typedef struct _CSamplePageExt	// smx
	{
    IShellPropSheetExt	_spx;           // 1st base class
    IShellExtInit   	_sxi;	    	// 2nd base class
    UINT            	_cRef;          // reference count
	LPDATAOBJECT		_pdtobj;		// data object
	HKEY			_hkeyProgID;		// reg. database key to ProgID
	} CSamplePageExt, * PSAMPLEPAGEEXT;

//
// Useful macros, which casts interface pointers to class pointers.
//
#define SMX_OFFSETOF(x)	        ((UINT_PTR)(&((PSAMPLEPAGEEXT)0)->x))
#define PVOID2PSMX(pv,offset)   ((PSAMPLEPAGEEXT)(((LPBYTE)pv)-offset))
#define PSPX2PSMX(pspx)	        PVOID2PSMX(pspx, SMX_OFFSETOF(_spx))
#define PSXI2PSMX(psxi)	        PVOID2PSMX(psxi, SMX_OFFSETOF(_sxi))

//
// Vtable prototype
//
extern IShellPropSheetExtVtbl   c_SamplePageExt_SPXVtbl;
extern IShellExtInitVtbl    	c_SamplePageExt_SXIVtbl;

//---------------------------------------------------------------------------
//
// PageExt_CreateInstance
//
//  This function is called back from within IClassFactory::CreateInstance()
// of the default class factory object, which is created by Shell_CreateClassObject.
//
//---------------------------------------------------------------------------

HRESULT CALLBACK PageExt_CreateInstance(LPUNKNOWN punkOuter,
				        REFIID riid, LPVOID FAR* ppvOut)
	{
    HRESULT hres;
    PSAMPLEPAGEEXT psmx;

    //
    // Shell extentions typically does not support aggregation.
    //
	if (punkOuter)
		return ResultFromScode(CLASS_E_NOAGGREGATION);

    //
    // in C++:
    //  psmx = new CSamplePageExt();
    //
	psmx = LocalAlloc(LPTR, sizeof(CSamplePageExt));

	if (!psmx)
		return ResultFromScode(E_OUTOFMEMORY);

    psmx->_spx.lpVtbl = &c_SamplePageExt_SPXVtbl;
    psmx->_sxi.lpVtbl = &c_SamplePageExt_SXIVtbl;
    psmx->_cRef = 1;
    psmx->_pdtobj = NULL;
    psmx->_hkeyProgID = NULL;
    g_cRefThisDll++;

    //
    // in C++:
    //  hres = psmx->QueryInterface(riid, ppvOut);
    //  psmx->Release();
    //
    // Note that the Release member will free the object, if QueryInterface
    // failed.
    //
    hres = c_SamplePageExt_SPXVtbl.QueryInterface(&psmx->_spx, riid, ppvOut);
    c_SamplePageExt_SPXVtbl.Release(&psmx->_spx);

    return hres;	// S_OK or E_NOINTERFACE
	}


//---------------------------------------------------------------------------
// CSamplePageExt::Initialize (IShellExtInit override)
//
//  The shell always calls this member function to initialize this object
// immediately after creating it (by calling CoCreateInstance).
//
// Arguments:
//  pdtobj -- Specifies one or more objects for which the shell is about to
//           open the property sheet. Typically, they are selected objects
//           in the explorer. If they are file system objects, it supports
//           CF_FILELIST; if they are network resource objects, it supports
//	     "Net Resource" clipboard format.
//  hkeyProgID -- Specifies the program ID of the primary object (typically
//	     the object which has the focus in the explorer's content pane).
//
// Comments:
//   The extension should "duplicate" the parameters if it needs them later.
//  Call AddRef() member function for the pdtobj and RegOpenKey() API for the
//  hkeyProgID.
//---------------------------------------------------------------------------
STDMETHODIMP PageExt_Initialize(LPSHELLEXTINIT psxi,
				LPCITEMIDLIST pidlFolder,
				LPDATAOBJECT pdtobj, HKEY hkeyProgID)
	{
    PSAMPLEPAGEEXT this = PSXI2PSMX(psxi);

    //
    // Initialize can be called more than once.
    //
	if (this->_pdtobj)
		{
		this->_pdtobj->lpVtbl->Release(this->_pdtobj);
		this->_pdtobj = NULL;
		}

	if (this->_hkeyProgID)
		{
		RegCloseKey(this->_hkeyProgID);
		this->_hkeyProgID = NULL;
		}

    //
    // Duplicate the pdtobj pointer
    //
	if (pdtobj)
		{
		this->_pdtobj = pdtobj;
		pdtobj->lpVtbl->AddRef(pdtobj);
		}

    //
    // Duplicate the handle (althogh we don't use it in this sample)
    //
	if (hkeyProgID)
		RegOpenKeyEx(hkeyProgID, 0, 0, KEY_READ, &this->_hkeyProgID);

    return NOERROR;
	}


//---------------------------------------------------------------------------
// CSamplePageExt::AddPages (IShellPropSheetExt override)
//---------------------------------------------------------------------------
STDMETHODIMP PageExt_AddPages(LPSHELLPROPSHEETEXT pspx,
				  LPFNADDPROPSHEETPAGE lpfnAddPage,
			      LPARAM lParam)
	{
    PSAMPLEPAGEEXT this = PSPX2PSMX(pspx);
    //
    //  This is the place where this extension may add pages to the property
    // sheet the shell is about to create. In this example, we add the
    // "FSPage" if the selected objects are file system objects and add
    // the "NETPage" if the selected objects are file system objects.
    //
    //  Typically, a shell extension is registered either file system object
    // class or network resource classe, and does not need to deal with two
    // different kinds of objects.
    //
    FSPage_AddPages(this->_pdtobj, lpfnAddPage, lParam);
	//NETPage_AddPages(this->_pdtobj, lpfnAddPage, lParam);

    return NOERROR;
	}

//---------------------------------------------------------------------------
// CSamplePageExt::AddRef (IShellPropSheetExt override)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) PageExt_SPX_AddRef(LPSHELLPROPSHEETEXT pspx)
	{
    PSAMPLEPAGEEXT this = PSPX2PSMX(pspx);
    return ++this->_cRef;
	}


//---------------------------------------------------------------------------
// CSamplePageExt::AddRef (IShellExtInit override)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) PageExt_SXI_AddRef(LPSHELLEXTINIT psxi)
	{
    PSAMPLEPAGEEXT this = PSXI2PSMX(psxi);
    return ++this->_cRef;
	}

//---------------------------------------------------------------------------
// CSamplePageExt::Release (IShellPropSheetExt override)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) PageExt_SPX_Release(LPSHELLPROPSHEETEXT pspx)
	{
	PSAMPLEPAGEEXT this = PSPX2PSMX(pspx);

	if (--this->_cRef)
		return this->_cRef;

	if (this->_pdtobj)
		this->_pdtobj->lpVtbl->Release(this->_pdtobj);

	if (this->_hkeyProgID)
		RegCloseKey(this->_hkeyProgID);

    LocalFree((HLOCAL)this);
    g_cRefThisDll--;

    return 0;
	}

//---------------------------------------------------------------------------
// CSamplePageExt::Release (IShellExtInit thunk)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) PageExt_SXI_Release(LPSHELLEXTINIT psxi)
	{
    PSAMPLEPAGEEXT this = PSXI2PSMX(psxi);
    return PageExt_SPX_Release(&this->_spx);
	}

//---------------------------------------------------------------------------
// CSamplePageExt::QueryInterface (IShellPropSheetExt override)
//---------------------------------------------------------------------------

STDMETHODIMP PageExt_SPX_QueryInterface(LPSHELLPROPSHEETEXT pspx, REFIID riid, LPVOID FAR* ppvOut)
	{
    PSAMPLEPAGEEXT this = PSPX2PSMX(pspx);

	if (IsEqualIID(riid, &IID_IShellPropSheetExt) ||
			IsEqualIID(riid, &IID_IUnknown))
		{
        (LPSHELLPROPSHEETEXT)*ppvOut=pspx;
        this->_cRef++;
        return NOERROR;
		}

    if (IsEqualIID(riid, &IID_IShellExtInit))
		{
        (LPSHELLEXTINIT)*ppvOut=&this->_sxi;
        this->_cRef++;
        return NOERROR;
		}

    *ppvOut=NULL;
    return ResultFromScode(E_NOINTERFACE);
	}


//---------------------------------------------------------------------------
// CSamplePageExt::QueryInterface (IShellExtInit thunk)
//---------------------------------------------------------------------------

STDMETHODIMP PageExt_SXI_QueryInterface(LPSHELLEXTINIT psxi, REFIID riid, LPVOID FAR* ppv)
	{
    PSAMPLEPAGEEXT this = PSXI2PSMX(psxi);
    return PageExt_SPX_QueryInterface(&this->_spx, riid, ppv);
	}


//---------------------------------------------------------------------------
// CSamplePageExt class : Vtables
//
//  VTables should be placed in the read only section unless we need to alter
// them at runtime.
//---------------------------------------------------------------------------

#pragma data_seg(".text")
IShellPropSheetExtVtbl c_SamplePageExt_SPXVtbl =
	{
    PageExt_SPX_QueryInterface,
    PageExt_SPX_AddRef,
    PageExt_SPX_Release,
    PageExt_AddPages
	};

IShellExtInitVtbl c_SamplePageExt_SXIVtbl =
	{
    PageExt_SXI_QueryInterface,
    PageExt_SXI_AddRef,
    PageExt_SXI_Release,
    PageExt_Initialize
	};
#pragma data_seg()
