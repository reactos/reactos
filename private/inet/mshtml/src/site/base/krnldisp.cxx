//+---------------------------------------------------------------------
//
//  File:       krnldisp.cxx
//
//  Contents:   dispatch implemtation for form object
//
//  Classes:    CDoc (partial)
//
//  History:
//              5-22-95     kfl     converted WCHAR to TCHAR
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

MtDefine(CDocBuildObjectTypeInfo_aryEntry_pv, Locals, "CDoc::BuildObjectTypeInfo aryEntry::_pv")

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
CDoc::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    return GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid);
}


HRESULT
CDoc::Invoke(DISPID          dispid,
             REFIID,
             LCID            lcid,
             WORD            wFlags,
             DISPPARAMS *    pdispparams,
             VARIANT *       pvarResult,
             EXCEPINFO *     pexcepinfo,
             UINT *)
{
    return InvokeEx(dispid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::InvokeEx
//
//  Synopsis:   Typical automation invoke.
//
//---------------------------------------------------------------

HRESULT
CDoc::InvokeEx(DISPID dispid,
               LCID lcid,
               WORD wFlags,
               DISPPARAMS *pdispparams,
               VARIANT *pvarResult,
               EXCEPINFO *pexcepinfo,
               IServiceProvider *pSrvProvider)
{
    return _pPrimaryMarkup ? _pPrimaryMarkup->InvokeEx(dispid,
                                     lcid,
                                     wFlags,
                                     pdispparams,
                                     pvarResult,
                                     pexcepinfo,
                                     pSrvProvider) : DISP_E_MEMBERNOTFOUND;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetDispID (IDispatchEx)
//
//  Synopsis:   Similar to GetIDsOfNames with the exception of supporting
//              expando and creation of a property based on grfdex.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetDispID(BSTR bstrName,
            DWORD grfdex,
            DISPID *pid)
{
    return _pPrimaryMarkup ? _pPrimaryMarkup->GetDispID(bstrName, grfdex, pid) : DISP_E_MEMBERNOTFOUND;
}





//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
CDoc::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetNextDispID (IDispatchEx)
//
//  Synopsis:   Enumerates through all properties and html attributes.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetNextDispID(DWORD grfdex,
                    DISPID id,
                    DISPID *prgid)
{
    return _pPrimaryMarkup->GetNextDispID(grfdex, id, prgid);
}

HRESULT
CDoc::GetMemberName(DISPID id,
                    BSTR *pbstrName)
{
    return _pPrimaryMarkup->GetMemberName(id, pbstrName);
}

HRESULT CDoc::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT CDoc::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

HRESULT
CDoc::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    Assert (_pOmWindow);

    hr = THR(_pOmWindow->QueryInterface(IID_IDispatchEx, (void **) ppunk));

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureObjectTypeInfo
//
//
//---------------------------------------------------------------------------

HRESULT
CDoc::EnsureObjectTypeInfo()
{
    HRESULT     hr = S_OK;
    CCollectionCache *pCollectionCache;

    if (!_pTypInfo || !_pTypInfoCoClass)
    {
        hr = THR(PrimaryMarkup()->InitCollections());
        if (hr)
            goto Cleanup;

        pCollectionCache = PrimaryMarkup()->CollectionCache();

        hr = THR(BuildObjectTypeInfo(
            pCollectionCache,
            CMarkup::ELEMENT_COLLECTION,                                     // index of array in coll cache
            DISPID_COLLECTION_MIN,                                  // Min DISPID
            pCollectionCache->GetMinDISPID(CMarkup::WINDOW_COLLECTION) - 1, // Max DISPID
            &_pTypInfo,
            &_pTypInfoCoClass,
            TRUE));
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CDoc::BuildObjectTypeInfo
//
//  Synopsis:   Build a typeinfo from the objects that are
//              to be added to some namespace.
//
//  Arguments:  guidTLib        ???
//              guidTInfo       ???
//              pCollCache      The collection cache to use
//              lIndex          Index in the collection cache to use
//              dispidMin       Minimum dispid to put in the typeinfo
//              ppTI            Outgoing generated typeinfo
//              ppTICoClass     Outgoing generated coclass
//              pvObject        Object implementing validate function.
//              pfnValidate     Function to call for validation of element
//
//---------------------------------------------------------------------------

HRESULT
CDoc::BuildObjectTypeInfo(
    CCollectionCache *  pCollCache,
    long                lIndex,
    DISPID              dispidMin,
    DISPID              dispidMax,
    ITypeInfo **        ppTI,
    ITypeInfo **        ppTICoClass,
    BOOL                fDocument/* = FALSE*/)
{
    HRESULT             hr          = S_OK;
    FUNCDESC            funcdesc    = {0};
    TYPEDESC            tdescUser   = {0};
    long                index       = 0;
    long                i;
    BOOL                fAtomAssigned;
    CElement *          pElement;
    TCHAR *             pchName;
    CStr                cstrUniqueName;
    ITypeInfo *         pTypInfoElement = NULL;
    CDataAry<DISPID>    aryEntry(Mt(CDocBuildObjectTypeInfo_aryEntry_pv));
    BOOL                fDidCreate;

    CCreateTypeInfoHelper Helper;

    Assert ( dispidMin < dispidMax );

    //
    // start creating the typeinfo
    //

    hr = THR(Helper.Start(g_Zero.guid));
    if (hr)
        goto Cleanup;

    //
    // Set up the function descriptor we'll be using.
    //

    funcdesc.funckind = FUNC_DISPATCH;
    funcdesc.invkind = INVOKE_PROPERTYGET;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.cScodes = -1;
    funcdesc.elemdescFunc.tdesc.vt = VT_PTR;
    funcdesc.elemdescFunc.tdesc.lptdesc = &tdescUser;
    tdescUser.vt = VT_USERDEFINED;
    funcdesc.wFuncFlags = FUNCFLAG_FSOURCE;

    //
    // ensure the array
    //

    if (!pCollCache)
    {
        hr = THR(PrimaryMarkup()->EnsureCollectionCache(lIndex));
        if (hr)
            goto Cleanup;
        pCollCache = PrimaryMarkup()->CollectionCache();
    }

    hr = THR(pCollCache->EnsureAry(lIndex));
    if (hr)
        goto Cleanup;

    //
    // Finally loop through all elements in specified collection,
    // creating typeinfos for those elements which pass the
    // validation criteria defined in the validate function.
    //

    // BUGBUG: (anandra) This will forestall addition of controls whose ID
    // changes after this occurs.

    for (i = 0; i < pCollCache->SizeAry(lIndex); i++)
    {

        hr = THR(pCollCache->GetIntoAry(lIndex, i, &pElement));
        if (hr)
            goto Cleanup;
        Assert(pElement);

        // if we are being called on the DOCUMENT's collectionCachethen
        // we are using the ALL collecion and not the window_collection.
        //are we w/n the scope of a form just continue the loop
        if (fDocument &&  
            PrimaryMarkup()->InFormCollection(pElement->GetFirstBranch()))
        {
            continue;
        }

        //
        // Validate this element: calculate name for type info
        //

        if (ETAG_INPUT == pElement->Tag()
            && DYNCAST(CInput, pElement)->GetType() == htmlInputRadio)
        {
            // radio buttons case - get id (can not be be accessed by name) 
            pchName = (TCHAR *) pElement->GetAAid();
        }
        else
        {
            // The forms have been moved to the window collection as a result
            // we don't want the form names to be sinked up twice once for the
            // AddNamedItem done in Init2 of CFormElement and then again when
            // drilling through the dynamic typelib.  If not form for window
            // then the main case -- get id or name.
            pchName = (fDocument && lIndex == CMarkup::ELEMENT_COLLECTION && pElement->Tag() == ETAG_FORM) ?
                        NULL : (TCHAR *) pElement->GetIdentifier();
        }

        fAtomAssigned = FALSE;


        if (pchName)
        {
            // if name is already present in atom table, this call will return index
            // of the name in table
            hr = THR_NOTRACE(_AtomTable.AddNameToAtomTable(pchName, &funcdesc.memid));
            if (hr)
                goto Cleanup;

            if (-1 == aryEntry.FindIndirect(&funcdesc.memid))
            {
                //
                // the id/name does not conflict with any other name
                //
                fAtomAssigned = TRUE;
            }
            else
            {
                //
                // name conflict; have to use unique name
                //
                hr = THR(pElement->GetUniqueIdentifier(&cstrUniqueName, TRUE, &fDidCreate ));
                if (hr)
                    goto Cleanup;

                pchName = (TCHAR*) cstrUniqueName;

                // Need to rebuild the WINDOW_COLLECTION, when added a unique name.
                // Do not use the passed in Collection Cache, it may not be the right one
                if ( fDidCreate )
                {
                    Assert(PrimaryMarkup());
                    Assert(PrimaryMarkup()->CollectionCache());
                    PrimaryMarkup()->CollectionCache()->InvalidateItem (CMarkup::WINDOW_COLLECTION );
                }
            }
        }

        // add VBScript scriptlets, if there are any.
        // if (NULL == pchName && (there is a scriptlet to add)) this call will
        // create a unique identifier
        hr = THR(pElement->AddAllScriptlets (pchName));
        if (hr)
            goto Cleanup;

        if (!pchName)
        {
            pchName = (TCHAR*) pElement->GetAAuniqueName();
        }

        if (!pchName || !*pchName)
            continue;

        // Add this element to the atom table of the doc

        if (!fAtomAssigned) 
        {
            hr = THR(_AtomTable.AddNameToAtomTable(pchName, &funcdesc.memid));
            if (hr)
                goto Cleanup;
        }

        // at this point it should not be possible to have id conflicts
        Assert (-1 == aryEntry.FindIndirect(&funcdesc.memid));

        hr = THR(aryEntry.AppendIndirect(&funcdesc.memid));
        if (hr)
            goto Cleanup;

        // The ID'd items here are resolved alongside the regular window collection items
        // The difference here is that the collection needs to return just the first
        // item whose name matches.
        funcdesc.memid += dispidMin;


        // Detect too many items - this is a seriously unlikely event !!
        if ( dispidMax <= funcdesc.memid)
            break;

        //
        // If we're here then it's ok for this element to create an
        // entry in the typelibrary for this element.
        //

        hr = THR(pElement->GetClassInfo(&pTypInfoElement));
        if (hr)
            goto Cleanup;

        hr = THR(Helper.pTypInfoCreate->AddRefTypeInfo(
            pTypInfoElement,
            &tdescUser.hreftype));
        if (hr)
            goto Cleanup;

        hr = THR(Helper.pTypInfoCreate->AddFuncDesc(index, &funcdesc));
        if (hr)
            goto Cleanup;

        LPTSTR ncpch = const_cast <LPTSTR> (pchName);
        hr = THR(Helper.pTypInfoCreate->SetFuncAndParamNames(
            index,
            &ncpch,
            1));
        if (hr)
            goto Cleanup;

        ClearInterface(&pTypInfoElement);
        index++;
    } // eo for (i = 0; i < pCollCache->SizeAry(lIndex); i++)

    //
    // finalize creating the typeinfo
    //

    hr = THR(Helper.Finalize(IMPLTYPEFLAG_FDEFAULT));
    if (hr)
        goto Cleanup;

    ReplaceInterface (ppTI, Helper.pTIOut);
    ReplaceInterface (ppTICoClass, Helper.pTICoClassOut);

Cleanup:
    aryEntry.DeleteAll();
    ReleaseInterface(pTypInfoElement);

    RRETURN(hr);
}
