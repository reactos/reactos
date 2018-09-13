/*****************************************************************************
 *
 *	common.c - Shared stuff that operates on all classes
 *
 *	WARNING!  The Common services work only if you pass in the
 *	"primary object".  This is vacuous if you don't use multiple
 *	inheritance, since there's only one object in the first place.
 *
 *	If you use multiple inheritance, make sure you pass the pointer
 *	to the object that you use as IUnknown.
 *
 *	The exceptions are the Forward_* functions, which work on
 *	pointers to non-primary interfaces.  They forward the call to the
 *	primary interface.
 *
 *****************************************************************************/

#include "fnd.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflCommon

/*****************************************************************************
 *
 *  USAGE FOR OLE OBJECTS
 *
 *	Suppose you want to implement an object called CObj that supports
 *	the interfaces Foo, Bar, and Baz.  Suppose that you opt for
 *	Foo as the primary interface.
 *
 *	>> NAMING CONVENTION <<
 *
 *	    COM objects begin with the letter "C".
 *
 *	(1) Declare the primary and secondary vtbls.
 *
 *		Primary_Interface(CObj, IFoo);
 *		Secondary_Interface(CObj, IBar);
 *		Secondary_Interface(CObj, IBaz);
 *
 *	(3) Declare the object itself.
 *
 *		typedef struct CObj {
 *		    IFoo 	foo;	    // Primary must come first
 *		    IBar	bar;
 *		    IBaz	baz;
 *		    ... other fields ...
 *		} CObj;
 *
 *	(4) Implement the methods.
 *
 *	    You may *not* reimplement the AddRef and Release methods!
 *	    although you can subclass them.
 *
 *	(5) To allocate an object of the appropriate type, write
 *
 *		hres = Common_New(CObj, ppvOut);
 *
 *	    or, if the object is variable-sized,
 *
 *		hres = Common_NewCb(cb, CObj, ppvOut);
 *
 *	    If the object supports multiple interfaces, you also need to
 *	    initialize all the secondary interfaces.
 *
 *		CObj *pco = *ppvOut;
 *		pco->bar = Secondary_Vtbl(CObj, IBar);
 *		pco->baz = Secondary_Vtbl(CObj, IBaz);
 *
 *	(6) Define the vtbls.
 *
 *		#pragma BEGIN_CONST_DATA
 *
 *		// The macros will declare QueryInterface, AddRef and Release
 *		// so don't list them again
 *
 *		Primary_Interface_Begin(CObj, IFoo)
 *		    CObj_FooMethod1,
 *		    CObj_FooMethod2,
 *		    CObj_FooMethod3,
 *		    CObj_FooMethod4,
 *		Primary_Interface_End(Obj, IFoo)
 *
 *		Secondary_Interface_Begin(CObj, IBar, bar)
 *		    CObj_Bar_BarMethod1,
 *		    CObj_Bar_BarMethod2,
 *		Secondary_Interface_Begin(CObj, IBar, bar)
 *
 *		Secondary_Interface_Begin(CObj, IBaz, baz)
 *		    CObj_Baz_BazMethod1,
 *		    CObj_Baz_BazMethod2,
 *		    CObj_Baz_BazMethod3,
 *		Secondary_Interface_Begin(CObj, IBaz, baz)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  USAGE FOR NON-OLE OBJECTS
 *
 *	All objects are COM objects, even if they are never given out.
 *	In the simplest case, it just derives from IUnknown.
 *
 *	Suppose you want to implement an object called Obj which is
 *	used only internally.
 *
 *	(1) Declare the vtbl.
 *
 *		Simple_Interface(Obj);
 *
 *	(3) Declare the object itself.
 *
 *		typedef struct Obj {
 *		    IUnknown unk;
 *		    ... other fields ...
 *		} Obj;
 *
 *	(4) Implement the methods.
 *
 *	    You may *not* override the QueryInterface, AddRef or
 *	    Release methods!
 *
 *	(5) Allocating an object of the appropriate type is the same
 *	    as with OLE objects.
 *
 *	(6) Define the "vtbl".
 *
 *		#pragma BEGIN_CONST_DATA
 *
 *		Simple_Interface_Begin(Obj)
 *		Simple_Interface_End(Obj)
 *
 *	    That's right, nothing goes between the Begin and the End.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *	CommonInfo
 *
 *	Information tracked for all common objects.
 *
 *	A common object looks like this:
 *
 *			  riid
 *              cRef	  FinalizeProc
 *	pFoo -> lpVtbl -> QueryInterface
 *		data	  Common_AddRef
 *		data	  Common_Release
 *		...	  ...
 *
 *	Essentially, we use the otherwise-unused space above the
 *	pointers to record our bookkeeping information.
 *
 *	cRef	     = object reference count
 *	riid	     = object iid
 *	FinalizeProc = Finalization procedure
 *
 *	For secondary interfaces, it looks like this:
 *
 *              	  offset to primary interface
 *	pFoo -> lpVtbl -> Forward_QueryInterface
 *			  Forward_AddRef
 *			  Forward_Release
 *			  ...
 *
 *****************************************************************************/

typedef struct CommonInfoN {
  D(ULONG cin_dwSig;)
    ULONG cin_cRef;
} CommonInfoN, CIN, *PCIN;

typedef struct CommonInfoP {
    PREVTBL *cip_prevtbl;
} CommonInfoP, CIP, *PCIP;

typedef struct CommonInfoP2 {
    PREVTBL2 *cip2_prevtbl2;
} CommonInfoP2, CIP2, *PCIP2;

typedef union CommonInfo {
    CIN cin[1];
    CIP cip[1];
    CIP2 cip2[1];
} CommonInfo, CI, *PCI;

#define ci_dwSig	cin[-1].cin_dwSig
#define ci_cRef		cin[-1].cin_cRef
#define ci_rgfp		cip[0].cip_prevtbl
#define ci_riid		cip[0].cip_prevtbl[-1].riid
#define ci_Finalize	cip[0].cip_prevtbl[-1].FinalizeProc
#define ci_lib		cip2[0].cip2_prevtbl2[-1].lib

#ifdef DEBUG
#define ci_Start	ci_dwSig
#else
#define ci_Start	ci_cRef
#endif

#define ci_dwSignature	0x38162378		/* typed by my cat */

/*****************************************************************************
 *
 *	Common_QueryInterface (from IUnknown)
 *
 *	Use this for objects that support only one interface.
 *
 *****************************************************************************/

STDMETHODIMP
Common_QueryInterface(PV pv, REFIID riid, PPV ppvObj)
{
    PCI pci = pv;
    HRESULT hres;
    EnterProc(Common_QueryInterface, (_ "pG", pv, riid));
    AssertF(pci->ci_dwSig == ci_dwSignature);
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, pci->ci_riid)) {
	*ppvObj = pv;
	Common_AddRef(pv);
	hres = NOERROR;
    } else {
	*ppvObj = NULL;
	hres = ResultFromScode(E_NOINTERFACE);
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	Common_AddRef (from IUnknown)
 *
 *	Increment the object refcount and the dll refcount.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
_Common_AddRef(PV pv)
{
    PCI pci = pv;
    AssertF(pci->ci_dwSig == ci_dwSignature);
    InterlockedIncrement((LPLONG)&g_cRef);
    return ++pci->ci_cRef;
}

/*****************************************************************************
 *
 *	Common_Finalize (from Common_Release)
 *
 *	By default, no finalization is necessary.
 *
 *****************************************************************************/

void EXTERNAL
Common_Finalize(PV pv)
{
    SquirtSqflPtszV(sqfl, TEXT("Common_Finalize(%08x)"), pv);
}

/*****************************************************************************
 *
 *	Common_Release (from IUnknown)
 *
 *	Decrement the object refcount and the dll refcount.
 *
 *	If the object refcount drops to zero, finalize the object
 *	and free it.
 *
 *	The finalization handler lives ahead of the object vtbl.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
_Common_Release(PV pv)
{
    PCI pci = pv;
    ULONG ulRc;
    AssertF(pci->ci_dwSig == ci_dwSignature);
    InterlockedDecrement((LPLONG)&g_cRef);
    ulRc = --pci->ci_cRef;
    if (ulRc == 0) {
	pci->ci_Finalize(pv);
	FreePv(&pci->ci_Start);
    }
    return ulRc;
}

/*****************************************************************************
 *
 *	Forward_QueryInterface (from IUnknown)
 *
 *	Move to the main object and try again.
 *
 *****************************************************************************/

STDMETHODIMP
Forward_QueryInterface(PV pv, REFIID riid, PPV ppvObj)
{
    PCI pci = pv;
    LPUNKNOWN punk = pvAddPvCb(pv, 0 - pci->ci_lib);
    return Common_QueryInterface(punk, riid, ppvObj);
}

/*****************************************************************************
 *
 *	Forward_AddRef (from IUnknown)
 *
 *	Move to the main object and try again.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
Forward_AddRef(PV pv)
{
    PCI pci = pv;
    LPUNKNOWN punk = pvAddPvCb(pv, 0 - pci->ci_lib);
    return Common_AddRef(punk);
}

/*****************************************************************************
 *
 *	Forward_Release (from IUnknown)
 *
 *	Move to the main object and try again.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
Forward_Release(PV pv)
{
    PCI pci = pv;
    LPUNKNOWN punk = pvAddPvCb(pv, 0 - pci->ci_lib);
    return Common_Release(punk);
}

/*****************************************************************************
 *
 *	_Common_New
 *
 *	Create a new object with refcount 1 and the specific vtbl.
 *	All other fields are zero-initialized.
 *
 *****************************************************************************/

STDMETHODIMP
_Common_New(ULONG cb, PV vtbl, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(Common_New, (_ "u", cb));
    SquirtSqflPtszV(sqfl, TEXT("Common_New()"));
    hres = AllocCbPpv(cb + sizeof(CIN), ppvObj);
    if (SUCCEEDED(hres)) {
	PCI pci = pvAddPvCb(*ppvObj, sizeof(CIN));
      D(pci->ci_dwSig = ci_dwSignature);
	pci->ci_rgfp = (PV)vtbl;
	*ppvObj = pci;
	Common_AddRef(pci);
	hres = NOERROR;
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	Invoke_Release
 *
 *	Release the object (if there is one) and wipe out the back-pointer.
 *	Note that we wipe out the value before calling the release, in order
 *	to ameliorate various weird callback conditions.
 *
 *****************************************************************************/

void EXTERNAL
Invoke_Release(PV pv)
{
    LPUNKNOWN punk = pvExchangePpvPv(pv, 0);
    if (punk) {
	punk->lpVtbl->Release(punk);
    }
}
