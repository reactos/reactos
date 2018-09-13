//=--------------------------------------------------------------------------=
// AutoObj.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// all of our objects will inherit from this class to share as much of the same
// code as possible.  this super-class contains the unknown, dispatch and
// error info implementations for them.
//
#include "IPServer.H"
#include "LocalSrv.H"

#include "AutoObj.H"
#include "Globals.H"
#include "Util.H"


// for ASSERT and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// CAutomationObject::CAutomationObject
//=--------------------------------------------------------------------------=
// create the object and initialize the refcount
//
// Parameters:
//    IUnknown *      - [in] controlling Unknown
//    int             - [in] the object type that we are
//    void *          - [in] the VTable of of the object we really are.
//
// Notes:
//
CAutomationObject::CAutomationObject 
(
    IUnknown *pUnkOuter,
    int   ObjType,
    void *pVTable,
	BOOL fExpandoEnabled
)
: CUnknownObject(pUnkOuter, pVTable), m_ObjectType (ObjType)
{
    m_fLoadedTypeInfo = FALSE;
	m_fExpandoEnabled = (BYTE)fExpandoEnabled;
	m_pexpando = NULL;
}


//=--------------------------------------------------------------------------=
// CAutomationObject::~CAutomationObject
//=--------------------------------------------------------------------------=
// "I have a rendezvous with Death, At some disputed barricade"
// - Alan Seeger (1888-1916)
//
// Notes:
//
CAutomationObject::~CAutomationObject ()
{
    // if we loaded up a type info, release our count on the globally stashed
    // type infos, and release if it becomes zero.
    //
    if (m_fLoadedTypeInfo) {

        // we have to crit sect this since it's possible to have more than
        // one thread partying with this object.
        //
        EnterCriticalSection(&g_CriticalSection);
        ASSERT(CTYPEINFOOFOBJECT(m_ObjectType), "Bogus ref counting on the Type Infos");
        CTYPEINFOOFOBJECT(m_ObjectType)--;

        // if we're the last one, free that sucker!
        //
        if (!CTYPEINFOOFOBJECT(m_ObjectType)) {
            PTYPEINFOOFOBJECT(m_ObjectType)->Release();
            PTYPEINFOOFOBJECT(m_ObjectType) = NULL;
        }
        LeaveCriticalSection(&g_CriticalSection);
    }

	if (m_pexpando)
	{
		delete m_pexpando;
	}
    return;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::InternalQueryInterface
//=--------------------------------------------------------------------------=
// the controlling unknown will call this for us in the case where they're
// looking for a specific interface.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CAutomationObject::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    ASSERT(ppvObjOut, "controlling Unknown should be checking this!");

    // start looking for the guids we support, namely IDispatch, and 
    // IDispatchEx

    if (DO_GUIDS_MATCH(riid, IID_IDispatch)) {
		// If expando functionality is enabled, attempt to allocate an
		// expando object and return that for the IDispatch interface.
		// If the allocation fails, we will fall back on using the regular
		// IDispatch from m_pvInterface;
		if (m_fExpandoEnabled)
		{
			if (!m_pexpando)
				m_pexpando = new CExpandoObject(m_pUnkOuter, (IDispatch*) m_pvInterface);  

			if (m_pexpando)
			{
				*ppvObjOut = (void*)(IDispatch*) m_pexpando;
				((IUnknown *)(*ppvObjOut))->AddRef();
				return S_OK;
			}
		}

        *ppvObjOut = (void*) (IDispatch*) m_pvInterface;
        ((IUnknown *)(*ppvObjOut))->AddRef();
        return S_OK;
    }
    else if (DO_GUIDS_MATCH(riid, IID_IDispatchEx) && m_fExpandoEnabled) {
		// Allocate the expando object if it hasn't been allocated already
		if (!m_pexpando)
			m_pexpando = new CExpandoObject(m_pUnkOuter, (IDispatch*) m_pvInterface);  

		// If the allocation succeeded, return the IDispatchEx interface from
		// the expando.  Otherwise fall through to CUnknownObject::InternalQueryInterface,
		// (which will most likely fail)
		if (m_pexpando)
		{
			 *ppvObjOut = (void *)(IDispatchEx *) m_pexpando;
			((IUnknown *)(*ppvObjOut))->AddRef();
			return S_OK;
		}
    }

    // just get our parent class to process it from here on out.
    //
    return CUnknownObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
// CAutomationObject::GetTypeInfoCount
//=--------------------------------------------------------------------------=
// returns the number of type information interfaces that the object provides
//
// Parameters:
//    UINT *            - [out] the number of interfaces supported.
//
// Output:
//    HRESULT           - S_OK, E_NOTIMPL, E_INVALIDARG
//
// Notes:
//
STDMETHODIMP CAutomationObject::GetTypeInfoCount
(
    UINT *pctinfo
)
{
    // arg checking
    //
    if (!pctinfo)
        return E_INVALIDARG;

    // we support GetTypeInfo, so we need to return the count here.
    //
    *pctinfo = 1;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::GetTypeInfo
//=--------------------------------------------------------------------------=
// Retrieves a type information object, which can be used to get the type
// information for an interface.
//
// Parameters:
//    UINT              - [in]  the type information they'll want returned
//    LCID              - [in]  the LCID of the type info we want
//    ITypeInfo **      - [out] the new type info object.
//
// Output:
//    HRESULT           - S_OK, E_INVALIDARG, etc.
//
// Notes:
//
STDMETHODIMP CAutomationObject::GetTypeInfo
(
    UINT        itinfo,
    LCID        lcid,
    ITypeInfo **ppTypeInfoOut
)
{
    DWORD       dwPathLen;
    char        szDllPath[MAX_PATH];
    HRESULT     hr;
    ITypeLib   *pTypeLib;
    ITypeInfo **ppTypeInfo =NULL;

    // arg checking
    //
    if (itinfo != 0)
        return DISP_E_BADINDEX;

    if (!ppTypeInfoOut)
        return E_POINTER;

    *ppTypeInfoOut = NULL;

    // ppTypeInfo will point to our global holder for this particular
    // type info.  if it's null, then we have to load it up. if it's not
    // NULL, then it's already loaded, and we're happy.
    // crit sect this entire nightmare so we're okay with multiple
    // threads trying to use this object.
    //
    EnterCriticalSection(&g_CriticalSection);
    ppTypeInfo = PPTYPEINFOOFOBJECT(m_ObjectType);

    if (*ppTypeInfo == NULL) {

        ITypeInfo *pTypeInfoTmp;
        HREFTYPE   hrefType;

        // we don't have the type info around, so go load the sucker.
        //
        hr = LoadRegTypeLib(*g_pLibid, (USHORT)VERSIONOFOBJECT(m_ObjectType), 0,
                            LANG_NEUTRAL, &pTypeLib);

        // if, for some reason, we failed to load the type library this
        // way, we're going to try and load the type library directly out of
        // our resources.  this has the advantage of going and re-setting all
        // the registry information again for us.
        //
        if (FAILED(hr)) {

            dwPathLen = GetModuleFileName(g_hInstance, szDllPath, MAX_PATH);
            if (!dwPathLen) {
                hr = E_FAIL;
                goto CleanUp;
            }

            MAKE_WIDEPTR_FROMANSI(pwsz, szDllPath);
            hr = LoadTypeLib(pwsz, &pTypeLib);
            CLEANUP_ON_FAILURE(hr);
        }

        // we've got the Type Library now, so get the type info for the interface
        // we're interested in.
        //
        hr = pTypeLib->GetTypeInfoOfGuid((REFIID)INTERFACEOFOBJECT(m_ObjectType), &pTypeInfoTmp);
        pTypeLib->Release();
        CLEANUP_ON_FAILURE(hr);

        // the following couple of lines of code are to dereference the dual
        // interface stuff and take us right to the dispatch portion of the
        // interfaces.
        //
        hr = pTypeInfoTmp->GetRefTypeOfImplType(0xffffffff, &hrefType);
        if (FAILED(hr)) {
            pTypeInfoTmp->Release();
            goto CleanUp;
        }

        hr = pTypeInfoTmp->GetRefTypeInfo(hrefType, ppTypeInfo);
        pTypeInfoTmp->Release();
        CLEANUP_ON_FAILURE(hr);

        // add an extra reference to this object.  if it ever becomes zero, then
        // we need to release it ourselves.  crit sect this since more than
        // one thread can party on this object.
        //
        CTYPEINFOOFOBJECT(m_ObjectType)++;
        m_fLoadedTypeInfo = TRUE;
    }


    // we still have to go and addref the Type info object, however, so that
    // the people using it can release it.
    //
    (*ppTypeInfo)->AddRef();
    *ppTypeInfoOut = *ppTypeInfo;
    hr = S_OK;

  CleanUp:
    LeaveCriticalSection(&g_CriticalSection);
    return hr;
}



//=--------------------------------------------------------------------------=
// CAutomationObject::GetIDsOfNames
//=--------------------------------------------------------------------------=
// Maps a single member and an optional set of argument names to a
// corresponding set of integer DISPIDs
//
// Parameters:
//    REFIID            - [in]  must be IID_NULL
//    OLECHAR **        - [in]  array of names to map.
//    UINT              - [in]  count of names in the array.
//    LCID              - [in]  LCID on which to operate
//    DISPID *          - [in]  place to put the corresponding DISPIDs.
//
// Output:
//    HRESULT           - S_OK, E_OUTOFMEMORY, DISP_E_UNKNOWNNAME,
//                        DISP_E_UNKNOWNLCID
//
// Notes:
//    - we're just going to use DispGetIDsOfNames to save us a lot of hassle,
//      and to let this superclass handle it.
//
STDMETHODIMP CAutomationObject::GetIDsOfNames
(
    REFIID    riid,
    OLECHAR **rgszNames,
    UINT      cNames,
    LCID      lcid,
    DISPID   *rgdispid
)
{
    HRESULT     hr;
    ITypeInfo  *pTypeInfo;

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return E_INVALIDARG;

    // get the type info for this dude!
    //
    hr = GetTypeInfo(0, lcid, &pTypeInfo);
    RETURN_ON_FAILURE(hr);

    // use the standard provided routines to do all the work for us.
    //
    hr = pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
    pTypeInfo->Release();

    return hr;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::Invoke
//=--------------------------------------------------------------------------=
// provides access to the properties and methods on this object.
//
// Parameters:
//    DISPID            - [in]  identifies the member we're working with.
//    REFIID            - [in]  must be IID_NULL.
//    LCID              - [in]  language we're working under
//    USHORT            - [in]  flags, propput, get, method, etc ...
//    DISPPARAMS *      - [in]  array of arguments.
//    VARIANT *         - [out] where to put result, or NULL if they don't care.
//    EXCEPINFO *       - [out] filled in in case of exception
//    UINT *            - [out] where the first argument with an error is.
//
// Output:
//    HRESULT           - tonnes of them.
//
// Notes:
//    
STDMETHODIMP CAutomationObject::Invoke
(
    DISPID      dispid,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *puArgErr
)
{
    HRESULT    hr;
    ITypeInfo *pTypeInfo;

    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return E_INVALIDARG;

    // get our typeinfo first!
    //
    hr = GetTypeInfo(0, lcid, &pTypeInfo);
    RETURN_ON_FAILURE(hr);

    // Clear exceptions
    //
    SetErrorInfo(0L, NULL);

    // This is exactly what DispInvoke does--so skip the overhead.
    //
    hr = pTypeInfo->Invoke(m_pvInterface, dispid, wFlags,
                           pdispparams, pvarResult,
                           pexcepinfo, puArgErr);
    pTypeInfo->Release();

	return hr;

}

//=--------------------------------------------------------------------------=
// CAutomationObject::Exception
//=--------------------------------------------------------------------------=
// fills in the rich error info object so that both our vtable bound interfaces
// and calls through ITypeInfo::Invoke get the right error informaiton.
//
// Parameters:
//    HRESULT          - [in] the SCODE that should be associated with this err
//    WORD             - [in] the RESOURCE ID of the error message.
//    DWORD            - [in] helpcontextid for the error
//
// Output:
//    HRESULT          - the HRESULT that was passed in.
//
// Notes:
//
HRESULT CAutomationObject::Exception
(
    HRESULT hrExcep,
    WORD    idException,
    DWORD   dwHelpContextID
)
{
    ICreateErrorInfo *pCreateErrorInfo;
    IErrorInfo *pErrorInfo;
    WCHAR   wszTmp[256];
    char    szTmp[256];
    HRESULT hr;


    // first get the createerrorinfo object.
    //
    hr = CreateErrorInfo(&pCreateErrorInfo);
    if (FAILED(hr)) return hrExcep;

    MAKE_WIDEPTR_FROMANSI(wszHelpFile, HELPFILEOFOBJECT(m_ObjectType));

    // set up some default information on it.
    //
    pCreateErrorInfo->SetGUID((REFIID)INTERFACEOFOBJECT(m_ObjectType));
    pCreateErrorInfo->SetHelpFile(wszHelpFile);
    pCreateErrorInfo->SetHelpContext(dwHelpContextID);

    // load in the actual error string value.  max of 256.
    //
    LoadString(GetResourceHandle(), idException, szTmp, 256);
    MultiByteToWideChar(CP_ACP, 0, szTmp, -1, wszTmp, 256);
    pCreateErrorInfo->SetDescription(wszTmp);

    // load in the source
    //
    MultiByteToWideChar(CP_ACP, 0, NAMEOFOBJECT(m_ObjectType), -1, wszTmp, 256);
    pCreateErrorInfo->SetSource(wszTmp);

    // now set the Error info up with the system
    //
    hr = pCreateErrorInfo->QueryInterface(IID_IErrorInfo, (void **)&pErrorInfo);
    CLEANUP_ON_FAILURE(hr);

    SetErrorInfo(0, pErrorInfo);
    pErrorInfo->Release();

  CleanUp:
    pCreateErrorInfo->Release();
    return hrExcep;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
// indicates whether or not the given interface supports rich error information
//
// Parameters:
//    REFIID        - [in] the interface we want the answer for.
//
// Output:
//    HRESULT       - S_OK = Yes, S_FALSE = No.
//
// Notes:
//
HRESULT CAutomationObject::InterfaceSupportsErrorInfo
(
    REFIID riid
)
{
    // see if it's the interface for the type of object that we are.
    //
    if (riid == (REFIID)INTERFACEOFOBJECT(m_ObjectType))
        return S_OK;

    return S_FALSE;
}

//=--------------------------------------------------------------------------=
// CAutomationObject::GetResourceHandle    [helper]
//=--------------------------------------------------------------------------=
// virtual routine to get the resource handle.  virtual, so that inheriting
// objects, such as COleControl can use theirs instead, which goes and gets
// the Host's version ...
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE CAutomationObject::GetResourceHandle
(
    void
)
{
    return ::GetResourceHandle();
}


