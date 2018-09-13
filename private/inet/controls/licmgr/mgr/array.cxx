//+----------------------------------------------------------------------------
//  File:       array.cxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <mgr.hxx>
#include <factory.hxx>


//+----------------------------------------------------------------------------
//
//  Member:     AddClass
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
HRESULT
CLicenseManager::AddClass(
    REFCLSID    rclsid,
    int *       piLic)
{
    IClassFactory2 *    pcf2 = NULL;
    LICINFO             licinfo;
    BSTR                bstrLic;
    int                 iLic;
    HRESULT             hr;

    Assert(piLic);
    Assert(!FindClass(rclsid, &iLic));

    // Get the class factory for the CLSID
    hr = ::CoGetClassObject(rclsid,
                            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                            NULL,
                            IID_IClassFactory2, (void **)&pcf2);
    if (hr)
        goto Cleanup;
    Assert(pcf2);

    // Determine if the object supports the creation of runtime licenses
    licinfo.cbLicInfo = sizeof(LICINFO);
    hr = pcf2->GetLicInfo(&licinfo);
    if (hr)
        goto Cleanup;

    if (!licinfo.fRuntimeKeyAvail ||
        !licinfo.fLicVerified)
    {
        hr = CLASS_E_NOTLICENSED;
        goto Cleanup;
    }

    // Obtain the object's runtime license
    hr = pcf2->RequestLicKey(0, &bstrLic);
    if (hr)
        goto Cleanup;
    Assert(bstrLic);

    // Add the object and its runtime license to the array of CLSID-License pairs
    // (The class is added in ascending order based upon the first DWORD of the CLSID)
    hr = _aryLic.SetSize(_aryLic.Size()+1);
    if (hr)
        goto Cleanup;

    for (iLic = 0; iLic < (_aryLic.Size()-1); iLic++)
    {
        if (rclsid.Data1 < _aryLic[iLic].clsid.Data1)
            break;
    }

    if (iLic < (_aryLic.Size()-1))
    {
        ::memmove(&_aryLic[iLic+1], &_aryLic[iLic], sizeof(_aryLic[0])*(_aryLic.Size()-iLic-1));
    }

    _aryLic[iLic].clsid = rclsid;
    _aryLic[iLic].bstrLic = bstrLic;
    _aryLic[iLic].pcf2 = pcf2;
    pcf2 = NULL;
    *piLic = iLic;
    _fDirty = TRUE;

Cleanup:
    ::SRelease(pcf2);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     FindClass
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
BOOL
CLicenseManager::FindClass(
    REFCLSID    rclsid,
    int *       piLic)
{
    int iLic;

    Assert(piLic);

    // BUGBUG: Consider using a more efficient search if the number of classes is large
    for (iLic=0; iLic < _aryLic.Size(); iLic++)
    {
        if (_aryLic[iLic].clsid.Data1 == rclsid.Data1 &&
            _aryLic[iLic].clsid == rclsid)
            break;
    }

    if (iLic < _aryLic.Size())
    {
        *piLic = iLic;
    }
    return (iLic < _aryLic.Size());
}


//+----------------------------------------------------------------------------
//
//  Member:     OnChangeInRequiredClasses
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::OnChangeInRequiredClasses(
    IRequireClasses *   pRequireClasses)
{
    ULONG   cClasses;
    ULONG   iClass;
    int     cLic;
    int     iLic;
    CLSID   clsid;
    BOOL    fClassUsed;
    BOOL    fClassNotLicensed = FALSE;
    HRESULT hr;

    if (!pRequireClasses)
        return E_INVALIDARG;

    // Determine the number of required classes
    hr = pRequireClasses->CountRequiredClasses(&cClasses);
    if (hr)
        goto Cleanup;

    // Add new classes to the array of required classes
    // NOTE: During this pass, all required classes are also marked as "in use"
    //       Because of this, the second loop must also alway run, even when errors occur,
    //       to remove these marks; that is, this loop cannot "goto Cleanup"
    for (iClass = 0; iClass < cClasses; iClass++)
    {
        // Get the CLSID of the required class
        hr = pRequireClasses->GetRequiredClasses(iClass, &clsid);
        if (hr)
            break;

        // Check if the class is already known; if not, add it
        // (Ignore "false" errors which occur during adding the class and treat it as unlicensed)
        fClassUsed = TRUE;                      // Assume the class will be used
        if (!FindClass(clsid, &iLic))
        {
            hr = AddClass(clsid, &iLic);
            if (hr)
            {
                if (hr == E_OUTOFMEMORY)
                    break;
                fClassUsed = FALSE;             // Class was not found nor added
                fClassNotLicensed = TRUE;
                hr = S_OK;
            }
        }

        // Mark the class as "in use" by setting the high-order bit of the factory address
        if (fClassUsed)
        {
            Assert((ULONG)(_aryLic[iLic].pcf2) < (ULONG_PTR)ADDRESS_TAG_BIT);
            _aryLic[iLic].pcf2 = (IClassFactory2 *)((ULONG_PTR)(_aryLic[iLic].pcf2) | ADDRESS_TAG_BIT);
        }
    }

    // Remove from the array classes no longer required
    // NOTE: If hr is not S_OK, then this loop should still execute, but only to clear
    //       the mark bits on the IClassFactory2 interface pointers, no other changes
    //       should occur
    //       Also, early exits from this loop (using "break" for example) must not occur
    for (cLic = iLic = 0; iLic < _aryLic.Size(); iLic++)
    {
        // If the class is "in use", clear the mark bit
        if ((ULONG_PTR)(_aryLic[iLic].pcf2) & ADDRESS_TAG_BIT)
        {
            _aryLic[iLic].pcf2 = (IClassFactory2 *)((ULONG_PTR)(_aryLic[iLic].pcf2) & (ADDRESS_TAG_BIT-1));

            // If classes have been removed, shift this class down to the first open slot
            if (!hr && iLic > cLic)
            {
                _aryLic[cLic] = _aryLic[iLic];
                ::memset(&(_aryLic[iLic]), 0, sizeof(_aryLic[iLic]));
            }
        }

        // Otherwise, free the class and remove it from the array
        else if (!hr)
        {
            ::SysFreeString(_aryLic[iLic].bstrLic);
            ::SRelease(_aryLic[iLic].pcf2);
            ::memset(&(_aryLic[iLic]), 0, sizeof(_aryLic[iLic]));
            _fDirty = TRUE;
        }

        // As long as it points at a valid class, increment the class counter
        if (_aryLic[cLic].clsid != CLSID_NULL)
        {
            cLic++;
        }
    }
    Implies(hr, cLic == _aryLic.Size());
    Implies(!hr, (ULONG)cLic <= cClasses);
    Verify(SUCCEEDED(_aryLic.SetSize(cLic)));

Cleanup:
    // If a real error occurred, return it
    // Otherwise return CLASS_E_NOTLICENSED if any un-licensed objects were encountered
    return (hr
                ? hr
                : (fClassNotLicensed
                        ? CLASS_E_NOTLICENSED
                        : S_OK));
}


//+----------------------------------------------------------------------------
//
//  Member:     CreateInstance
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::CreateInstance(
    CLSID       clsid,
    IUnknown *  pUnkOuter,
    REFIID      riid,
    DWORD       dwClsCtx,
    void **     ppvObj)
{
    int     iLic;
    HRESULT hr;

    // If there is a runtime license for the class, create it using IClassFactory2
    if (FindClass(clsid, &iLic))
    {
        if (!_aryLic[iLic].pcf2)
        {
            //
            // The following code calls CoGetClassObject for an IClassFactory
            // then QIs for an IClassFactory2. This is because of an apparent
            // bug in ole32.dll.  On a win95 system if the call to
            // CoGetClassObject is remoted and you ask for IClassFactory2 the
            // process hangs.
            //

		    IClassFactory *pIClassFactory;

            hr = ::CoGetClassObject(clsid, dwClsCtx, NULL,
                                    IID_IClassFactory, (void **)&(pIClassFactory));

            if (SUCCEEDED(hr)) {

                hr = pIClassFactory->QueryInterface(IID_IClassFactory2,
                                                    (void **)&(_aryLic[iLic].pcf2));

                pIClassFactory->Release();
            }

            if (hr)			
                goto Cleanup;

        }

        Assert(_aryLic[iLic].pcf2);
        Assert(_aryLic[iLic].bstrLic != NULL);
        hr = _aryLic[iLic].pcf2->CreateInstanceLic(pUnkOuter, NULL,
                                                   riid, _aryLic[iLic].bstrLic, ppvObj);
    }

    // Otherwise, use the standard COM mechanisms
    else
    {
        hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsCtx, riid, ppvObj);
    }

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetTypeLibOfClsid
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetTypeLibOfClsid(
    CLSID       clsid,
    ITypeLib ** ptlib)
{
    UNREF(clsid);
    UNREF(ptlib);
    return E_NOTIMPL;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetClassObjectOfClsid
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetClassObjectOfClsid(
    REFCLSID    rclsid,
    DWORD       dwClsCtx,
    LPVOID      lpReserved,
    REFIID      riid,
    void **     ppcClassObject)
{
    // Load the class object
    return ::CoGetClassObject(rclsid, dwClsCtx, lpReserved, riid, ppcClassObject);
}


//+----------------------------------------------------------------------------
//
//  Member:     CountRequiredClasses
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::CountRequiredClasses(
    ULONG * pcClasses)
{
    if (!pcClasses)
        return E_INVALIDARG;

    // Return the current number of classes
    *pcClasses = _aryLic.Size();

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetRequiredClasses
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetRequiredClasses(
    ULONG   iClass,
    CLSID * pclsid)
{
    if (!pclsid || iClass >= (ULONG)_aryLic.Size())
        return E_INVALIDARG;

    // Return the requested CLSID
    *pclsid = _aryLic[iClass].clsid;

    return S_OK;
}
