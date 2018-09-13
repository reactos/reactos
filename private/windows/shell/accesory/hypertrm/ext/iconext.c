/*	File: D:\WACKER\ext\iconext.c (Created: 11-Mar-1994)
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

#include <term\res.h>
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
BOOL WINAPI TDllEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

//
// Global variables
//
UINT g_cRefThisDll = 0; 	// Reference count of this DLL.
HINSTANCE hInstanceDll;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	IconEntry
 *
 * DESCRIPTION:
 *	Currently, just initializes the C-Runtime library but may be used
 *	for other things later.
 *
 * ARGUMENTS:
 *	hInstDll	- Instance of this DLL
 *	fdwReason	- Why this entry point is called
 *	lpReserved	- reserved
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL WINAPI IconEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
	{
	hInstanceDll = hInstDll;

	// You need to initialize the C runtime if you use any C-Runtime
	// functions.  Currently this is not the case execpt for memcmp
	// used in IsEqualGUID().  However, if we're compiling for release
	// we get the inline version of memcmp and so we don't need the
	// C-Runtime.

	#if defined(NDEBUG)
	return TRUE;
	#else
	return _CRT_INIT(hInstDll, fdwReason, lpReserved);
	#endif
	}

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
	// This DLL has only one class (CLSID_SampleIconExt). If a DLL supports
    // multiple classes, it should have either multiple if-statements or
    // efficient table lookup code.
	//

	if (IsEqualIID(rclsid, &CLSID_SampleIconExt))
		{
		//
		// We are supposed return the class object for this class. Instead
		// of fully implementing it in this DLL, we just call a helper
		// function in the shell DLL which creates a default class factory
		// object for us. When its CreateInstance member is called, it
		// will call back our create instance function (IconExt_CreateInstance).
		//
		return SHCreateDefClassObject(
			riid,
			ppvOut,
		    IconExt_CreateInstance, // callback function
			&g_cRefThisDll, 		// reference count of this DLL
		    &IID_IPersistFile	    // init interface
		    );
		}

    return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
	}

//---------------------------------------------------------------------------
//
// CSampleIconExt class
//
// In C++:
//  class CSampleIconExt : protected IExtractIcon, protected IPersistFile
//  {
//  protected:
//      int          _cRef;
//      LPDATAOBJECT _pdtobj;
//	HKEY	     _hkeyProgID;
//  public:
//      CSampleIconExt() _cRef(1), _pdtobj(NULL), _hkeyProgID(NULL) {};
//      ...
//  };
//
//---------------------------------------------------------------------------
typedef struct _CSampleIconExt	// smx
	{
	IExtractIcon	_ctm;			// 1st base class
	IPersistFile	_sxi;			// 2nd base class
	int 			_cRef;			// reference count
    char	    _szFile[MAX_PATH];	//
	} CSampleIconExt, * PSAMPLEICONEXT;

#define SMX_OFFSETOF(x)	        ((UINT_PTR)(&((PSAMPLEICONEXT)0)->x))
#define PVOID2PSMX(pv,offset)   ((PSAMPLEICONEXT)(((LPBYTE)pv)-offset))
#define PCTM2PSMX(pctm)	        PVOID2PSMX(pctm, SMX_OFFSETOF(_ctm))
#define PSXI2PSMX(psxi)	        PVOID2PSMX(psxi, SMX_OFFSETOF(_sxi))

//
// Vtable prototype
//
extern IExtractIconVtbl     c_SampleIconExt_CTMVtbl;
extern IPersistFileVtbl 	c_SampleIconExt_SXIVtbl;

//---------------------------------------------------------------------------
//
// IconExt_CreateInstance
//
//  This function is called back from within IClassFactory::CreateInstance()
// of the default class factory object, which is created by SHCreateClassObject.
//
//---------------------------------------------------------------------------

HRESULT CALLBACK IconExt_CreateInstance(LPUNKNOWN punkOuter,
				        REFIID riid, LPVOID FAR* ppvOut)
	{
    HRESULT hres;
    PSAMPLEICONEXT psmx;

    //
    // Shell extentions typically does not support aggregation.
    //
	if (punkOuter)
		return ResultFromScode(CLASS_E_NOAGGREGATION);

    //
    // in C++:
    //  psmx = new CSampleIconExt();
    //
	psmx = LocalAlloc(LPTR, sizeof(CSampleIconExt));

	if (!psmx)
		return ResultFromScode(E_OUTOFMEMORY);

    psmx->_ctm.lpVtbl = &c_SampleIconExt_CTMVtbl;
    psmx->_sxi.lpVtbl = &c_SampleIconExt_SXIVtbl;
    psmx->_cRef = 1;
    g_cRefThisDll++;

    //
    // in C++:
    //  hres = psmx->QueryInterface(riid, ppvOut);
    //  psmx->Release();
    //
    // Note that the Release member will free the object, if QueryInterface
    // failed.
    //
    hres = c_SampleIconExt_CTMVtbl.QueryInterface(&psmx->_ctm, riid, ppvOut);
    c_SampleIconExt_CTMVtbl.Release(&psmx->_ctm);

    return hres;	// S_OK or E_NOINTERFACE
	}

//---------------------------------------------------------------------------
// CSampleIconExt::Load (IPersistFile override)
//---------------------------------------------------------------------------
STDMETHODIMP IconExt_GetClassID(LPPERSISTFILE pPersistFile, LPCLSID lpClassID)
	{
    return ResultFromScode(E_FAIL);
	}

STDMETHODIMP IconExt_IsDirty(LPPERSISTFILE pPersistFile)
	{
    return ResultFromScode(E_FAIL);
	}

STDMETHODIMP IconExt_Load(LPPERSISTFILE pPersistFile, LPCOLESTR lpszFileName, DWORD grfMode)
	{
	PSAMPLEICONEXT this = PSXI2PSMX(pPersistFile);
	int iRet = 0;

#if 1
	iRet = WideCharToMultiByte(
			CP_ACP, 			// CodePage
			0,					// dwFlags
			lpszFileName,		// lpWideCharStr
			-1, 				// cchWideChar
			this->_szFile,		// lpMultiByteStr
			sizeof(this->_szFile),	// cchMultiByte,
			NULL,				// lpDefaultChar,
			NULL				// lpUsedDefaultChar
			);
#endif
//
// BUGBUG: WideCharToMultiByte does not work on build 84.
//
#if 1
    if (iRet==0)
    {
	LPSTR psz=this->_szFile;
	while(*psz++ = (char)*lpszFileName++);
    }
#endif
    return NOERROR;
	}

STDMETHODIMP IconExt_Save(LPPERSISTFILE pPersistFile, LPCOLESTR lpszFileName, BOOL fRemember)
	{
    return ResultFromScode(E_FAIL);
	}

STDMETHODIMP IconExt_SaveCompleted(LPPERSISTFILE pPersistFile, LPCOLESTR lpszFileName)
	{
    return ResultFromScode(E_FAIL);
	}

STDMETHODIMP IconExt_GetCurFile(LPPERSISTFILE pPersistFile, LPOLESTR FAR* lplpszFileName)
	{
    return ResultFromScode(E_FAIL);
	}


/*
 * At the time I write this, the only known documentation for these next two
 * functions is in SHLOBJ.H.  Please take the time to read it.
 */

STDMETHODIMP IconExt_GetIconLocation(LPEXTRACTICON pexic,
		     UINT   uFlags,
		     LPSTR  szIconFile,
		     UINT   cchMax,
		     int  FAR * piIndex,
		     UINT FAR * pwFlags)
	{
	PSAMPLEICONEXT this = PCTM2PSMX(pexic);

    if (this->_szFile[0])
		{
		HANDLE hFile;
		DWORD dw;
		DWORD dwSize;
		DWORD dwIdx;
		int nIndex = 0;
		int nRet = 0;

		GetModuleFileName(hInstanceDll, szIconFile, cchMax);

		hFile = CreateFile(this->_szFile, GENERIC_READ, FILE_SHARE_READ,
			0, OPEN_EXISTING, 0, 0);

		if ( hFile != INVALID_HANDLE_VALUE ) //mpt:4-29-98 we weren't checking for failure here
			{
			// Skip past header.  First ID will be the icon number.
			// (IDs are shorts).  Size field follows (DWORD)

			if (SetFilePointer(hFile, 256+sizeof(SHORT), 0,
					FILE_BEGIN) != (DWORD)-1)
				{
				if (ReadFile(hFile, &dwSize, sizeof(DWORD), &dw, 0))
					{
					dwIdx = 0;

					if (ReadFile(hFile, &dwIdx, min(sizeof(DWORD), dwSize),
							&dw, 0))
						{
						nIndex = (int)dwIdx;
						nIndex -= IDI_PROG;
						}
					}
				}

			CloseHandle(hFile);
			}

		*piIndex = nIndex;
		}

    *pwFlags = 0;
    return NOERROR;
	}

STDMETHODIMP IconExt_Extract(LPEXTRACTICON pexic,
			   LPCSTR pszFile,
		       UINT	  nIconIndex,
		       HICON  FAR *phiconLarge,
		       HICON  FAR *phiconSmall,
		       UINT   nIcons)
	{
    // Force default extraction.
	return ResultFromScode(S_FALSE);
	}

//---------------------------------------------------------------------------
// CSampleIconExt::AddRef (IExtractIcon override)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) IconExt_CTM_AddRef(LPEXTRACTICON pctm)
	{
    PSAMPLEICONEXT this = PCTM2PSMX(pctm);
    return ++this->_cRef;
	}


//---------------------------------------------------------------------------
// CSampleIconExt::AddRef (IPersistFile override)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) IconExt_SXI_AddRef(LPPERSISTFILE psxi)
	{
    PSAMPLEICONEXT this = PSXI2PSMX(psxi);
    return ++this->_cRef;
	}

//---------------------------------------------------------------------------
// CSampleIconExt::Release (IExtractIcon override)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) IconExt_CTM_Release(LPEXTRACTICON pctm)
	{
	PSAMPLEICONEXT this = PCTM2PSMX(pctm);

	if (InterlockedDecrement(&this->_cRef))
		return this->_cRef;

    LocalFree((HLOCAL)this);
    g_cRefThisDll--;

    return 0;
	}

//---------------------------------------------------------------------------
// CSampleIconExt::Release (IPersistFile thunk)
//---------------------------------------------------------------------------

STDMETHODIMP_(UINT) IconExt_SXI_Release(LPPERSISTFILE psxi)
	{
    PSAMPLEICONEXT this = PSXI2PSMX(psxi);
    return IconExt_CTM_Release(&this->_ctm);
	}

//---------------------------------------------------------------------------
// CSampleIconExt::QueryInterface (IExtractIcon override)
//---------------------------------------------------------------------------

STDMETHODIMP IconExt_CTM_QueryInterface(LPEXTRACTICON pctm, REFIID riid, LPVOID FAR* ppvOut)
	{
	PSAMPLEICONEXT this = PCTM2PSMX(pctm);

	if (IsEqualIID(riid, &IID_IExtractIcon) ||
			IsEqualIID(riid, &IID_IUnknown))
		{
        (LPEXTRACTICON)*ppvOut=pctm;
        this->_cRef++;
        return NOERROR;
		}

    if (IsEqualIID(riid, &IID_IPersistFile))
		{
        (LPPERSISTFILE)*ppvOut=&this->_sxi;
        this->_cRef++;
        return NOERROR;
		}

    return ResultFromScode(E_NOINTERFACE);
	}

//---------------------------------------------------------------------------
// CSampleIconExt::QueryInterface (IPersistFile thunk)
//---------------------------------------------------------------------------

STDMETHODIMP IconExt_SXI_QueryInterface(LPPERSISTFILE psxi, REFIID riid, LPVOID FAR* ppv)
	{
    PSAMPLEICONEXT this = PSXI2PSMX(psxi);
    return IconExt_CTM_QueryInterface(&this->_ctm, riid, ppv);
	}

//---------------------------------------------------------------------------
// CSampleIconExt class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(".text")
IExtractIconVtbl c_SampleIconExt_CTMVtbl =
	{
    IconExt_CTM_QueryInterface,
    IconExt_CTM_AddRef,
    IconExt_CTM_Release,
    IconExt_GetIconLocation,
	IconExt_Extract,
	};

IPersistFileVtbl c_SampleIconExt_SXIVtbl =
	{
    IconExt_SXI_QueryInterface,
    IconExt_SXI_AddRef,
    IconExt_SXI_Release,
    IconExt_GetClassID,
    IconExt_IsDirty,
    IconExt_Load,
    IconExt_Save,
    IconExt_SaveCompleted,
    IconExt_GetCurFile
	};
#pragma data_seg()
