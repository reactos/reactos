//--------------------------------------------------------------------
// Microsoft Nile to STD Mapping Layer
// (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  File:       rowset.cxx
//  Author:     Terry L. Lucas (TerryLu)
//
//  Contents:   CImpIRowset object implementation
//
//  History:
//  07/18/95    TerryLu     Creation of Nile provider implementation.
//  07/28/95    TerryLu     Removed aggregation possiblity for now (no pUnk...).
//  08/02/95    Ido         GetRowsetInfo returns appropriate flags.
//  08/07/95    TerryLu     Added IConnectionPointContainer & IConnectionPoint.
//  08/14/95    TerryLu     Removed IConnectionPointContainer and used CBase.
//

#include <dlaypch.hxx>

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include <connect.hxx>
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include "coredisp.h"
#endif

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

DeclareTag(tagNileRowsetProvider, "CImpIRowset", "STD Nile provider");
MtDefine(CImpIRowset, DataBind, "CImpIRowset")
MtDefine(CTopRowset, DataBind, "CTopRowset")
MtDefine(CChapRowset, DataBind, "CChapRowset")
MtDefine(COSPData, DataBind, "COSPData")

const CONNECTION_POINT_INFO CImpIRowset::s_acpi[] =
{
    CPI_ENTRY(IID_IRowsetNotify, DISPID_A_ROWSETNOTIFYSINK)
    CPI_ENTRY(IID_IDBAsynchNotify, DISPID_A_ROWSETASYNCHNOTIFYSINK)
    CPI_ENTRY_NULL
};


const CImpIRowset::CLASSDESC CImpIRowset::s_classdesc =
{
        NULL,                               // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        0,                                  // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
};



//+---------------------------------------------------------------------------
//  Member:     Constructor (public member)
//
//  Synopsis:   Instanciate an IRowset
//
//  Arguments:  None
//
//  Returns:    None

CImpIRowset::CImpIRowset ()
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::constructor -> %p", this ));

#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
#endif

    CBase::Init();

    _NextH2R = 1;                       // for rowmap (vestigial)
    _pUnkOuter = getpIUnknown();
}



//+---------------------------------------------------------------------------
//  Member:     Destructor (public member)
//
//  Synopsis:   Cleanup the IRowset
//
//  Arguments:  None
//
//  Returns:    None

CImpIRowset::~CImpIRowset ()
{
    TraceTag((tagNileRowsetProvider, "CImpIRowset::destructor() -> %p", this));
}



////////////////////////////////////////////////////////////////////////////////
//
//  CBase specific interfaces
//
////////////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------------------
//  Member:     PrivateQueryInterface (public member)
//
//  Synopsis:   Private QI come to here for CBase derived objects.
//
//  Arguments:  riid            IID of requested interface
//              ppv             Interface object to return
//
//  Returns:    S_OK            Interface supported
//              E_NOINTERFACE   Interface not supported.
//

STDMETHODIMP
CImpIRowset::PrivateQueryInterface (REFIID riid, LPVOID * ppv)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::QueryInterface(%p {%p, %p})", this, riid, ppv ));

    Assert(ppv);

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = getpIUnknown();
    }
#define TEST(IFace) else if (IsEqualIID(riid, IID_##IFace)) *ppv = (IFace *)this

    TEST(IRowset);
    TEST(IAccessor);
    TEST(IColumnsInfo);
    TEST(IRowsetLocate);
    TEST(IChapteredRowset);
    TEST(IRowsetChange);
    TEST(IRowsetScroll);
    TEST(IRowsetExactScroll);
    TEST(IRowsetNewRowAfter);
    TEST(IRowsetIdentity);
    TEST(IRowsetChapterMember);
    TEST(IRowsetFind);
    TEST(IRowsetInfo);
    TEST(IConvertType);
    TEST(IDBAsynchStatus);
    TEST(IRowsetFind);
    
#undef TEST

    else if (IsEqualIID(riid, IID_IConnectionPointContainer))
    {
        *((IConnectionPointContainer **)ppv) =
                                            new CConnectionPointContainer(this, NULL);
        if (!*ppv)
        {
            RRETURN(E_OUTOFMEMORY);
        }
    }
    else
    {
        *ppv = NULL;
    }

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//  Member:     Passivate (public member)
//
//  Synopsis:   Called when main CImpIRowset reference count (_ulRefs) drops
//              to zero. Do not call this method directly.  Use Release instead.
//
//  Arguments:  None
//
//  Returns:    Nothing
//

void
CImpIRowset::Passivate ()
{
    FireRowsetEvent(DBREASON_ROWSET_RELEASE, DBEVENTPHASE_DIDEVENT);

    if (_astdcolinfo)
    {
        // If there are subsidary chapters rowsets, clear them.
        // (Column 0, the bookmark column can never have one).
        for (ULONG uCol = 1; uCol <= _cCols; uCol++)
        {
            if (_astdcolinfo[uCol].pChapRowset)
                _astdcolinfo[uCol].pChapRowset->Release();

            FormsFreeString(_astdcolinfo[uCol].bstrName);
        }

        delete [] _astdcolinfo;
    }

    // release all live accessors
    while (!_dblAccessors.IsEmpty())
    {
        ULONG ulRefcount;
        HACCESSOR hAccessor = (HACCESSOR) _dblAccessors.First();
        ReleaseAccessor(hAccessor, &ulRefcount);
    }
    
    CBase::Passivate();
}

void
CTopRowset::Passivate()
{
    ClearInterface(&_pOSPData);

    super::Passivate();
}

CTopRowset::~CTopRowset()
{
    Assert(!_pOSPData && "Rowset released before Passivate!");
}


//+---------------------------------------------------------------------------
//  Member:     GetClassDesc (public member)
//
//  Synopsis:   Return the class descriptor, we only use the descriptor to
//              describe the number of connection points our container can
//              handle and the connection points.
//
//  Arguments:  None
//
//  Returns:    CLASSDESC
//

const CBase::CLASSDESC *
CImpIRowset::GetClassDesc () const
{
    return &s_classdesc;
}


////////////////////////////////////////////////////////////////////////////////
//
//  IRowset specific interfaces
//
////////////////////////////////////////////////////////////////////////////////


#if DBG == 1

//+---------------------------------------------------------------------------
//  Member:     DbgCheckOverlap (private member only in debug build)
//
//  Synopsis:   Verifies that a DBBINDING does not have any value, length or
//              status data overlap. Overlap problem will assert.
//
//  Arguments:  currBinding     Binding to validate
//
//  Returns:    None
//

void
CImpIRowset::DbgCheckOverlap (const DBBINDING &currBinding)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::DbgCheckOverlap({%p})", &currBinding ));

    DBPART    currPart = currBinding.dwPart;
    short     sCount = 0;
    ULONG     left[3], right[3];

    if (currPart & DBPART_VALUE)
    {
        left[sCount] = currBinding.obValue;
        right[sCount] = left[sCount] + currBinding.cbMaxLen - 1;
        sCount++;
    }

    if (currPart & DBPART_LENGTH)
    {
        left[sCount] = currBinding.obLength;
        right[sCount] = left[sCount] + sizeof(ULONG) - 1;
        sCount++;
    }

    if (currPart & DBPART_STATUS)
    {
        left[sCount] = currBinding.obStatus;
        right[sCount] = left[sCount] + sizeof(ULONG) - 1;
        sCount++;
    }

    // More than one comming in, check overlapping.
    if (((sCount > 1) &&
         ((left[0] <= right[1]) && (right[0] >= left[1])) ) || /*0 & 1 intersect*/
        ((sCount > 2)                                     &&
         (((left[1] <= right[2]) && (right[1] >= left[2]))  || /*1 & 2 intersect*/
          ((left[0] <= right[2]) && (right[0] >= left[2])) ))) /*0 & 2 intersect*/
    {
        Assert(!"Accessor values overlap");
    }
}
#endif  // DBG == 1



//+---------------------------------------------------------------------------
//  Member:     CreateAccessor (public member)
//
//  Synopsis:   Creates a set of bindings that can be used to send data
//              to or retrieve data from the data cache.
//
//  Arguments:  eAccFlags           How bindings are used
//              cBindings           Number of Bindings
//              aBindings           Array of DBBINDINGS
//              cbRowSize           ignored -- only used for IReadData
//              phAccessor          Accessor Handle
//              aStatus             Array of status words, one per binding
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_OUTOFMEMORY           Couldn't allocate memory
//              E_INVALIDARG            Invalid arg
//              E_UNEXPECTED            Zombie State
//              DB_E_BADACCESSORFLAGS   Bad eAccFlags
//              DB_E_BYREFACCESSORNOTSUPPORTED  Can't support PASSBYREF
//              DB_E_ERRORSOCCURRED     One or more bindings failed, see aStatus
//              DB_E_NOTREENTRANT       Can not enter function at this time.
//              DB_E_NULLACCESSORNOTSUPPORTED   No bindings.
//

STDMETHODIMP
CImpIRowset::CreateAccessor (DBACCESSORFLAGS eAccFlags,
                             DBCOUNTITEM cBindings,
                             const DBBINDING aBindings[],
                             DBLENGTH /* cbRowSize */,
                             HACCESSOR * phAccessor,
                             DBBINDSTATUS aStatus[])
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::CreateAccessor(%p {%p, %u, %p, %p, %p})",
             this,
             eAccFlags, cBindings, aBindings, phAccessor, aStatus ));

    HRESULT             hr = S_OK;
    AccessorFormat      *pAccessor;
    ULONG               ibind;
    ULONG               icol;
    ULONG               cValidBindings = 0;  // number of validated bindings

    // Check Parameters
    if (phAccessor == NULL || (cBindings>0 && aBindings==NULL))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *phAccessor = NULL;

    // Do a quick check to make sure we recognize the binding type.
    // BUGBUG: should we silently ignore DBACCESSOR_OPTIMIZE?
    if (eAccFlags != DBACCESSOR_ROWDATA)
    {
            hr = DB_E_BADBINDINFO;
            goto Cleanup;
    }

    // Sanity check on the bindings the user gave us.
    // Specifically hunt down some common client coding bugs.
    for (ibind = 0; ibind < cBindings; ibind++)
    {
        const DBBINDING &   currBinding = aBindings[ibind];
        DBPART              currPart = currBinding.dwPart;
        DBBINDSTATUS        dbsStatus = DBBINDSTATUS_OK;

        icol = currBinding.iOrdinal;
        if (icol > (_cCols + 1))        // +1 for bookmark info
        {
            Assert(!"Bad column number");
            dbsStatus = DBBINDSTATUS_BADORDINAL;
        }

        // At least one of these valid parts has to be set. In SetData I assume it is.
        if (!(currPart & (DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS)))
        {
            Assert(!"dwPart missing VALUE, LENGTH or STATUS being specified");
            dbsStatus = DBBINDSTATUS_BADBINDINFO;
        }

        // other binding problems forbidden by OLE-DB
        const DBTYPE currType = currBinding.wType;
        const DBTYPE currTypePtr = currType &(DBTYPE_BYREF | DBTYPE_ARRAY | DBTYPE_VECTOR);
        const DBTYPE currTypeBase = currType & 0xffff;
        if (
            // part other than value, length, or status
            (currPart & ~(DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS)) ||
            // type is empty or null
            (currType==DBTYPE_EMPTY || currType==DBTYPE_NULL) ||
            // byref or'd with empty, null, or reserved
            (currType&DBTYPE_BYREF && (currTypeBase==DBTYPE_EMPTY ||
                                       currTypeBase==DBTYPE_NULL || currType&DBTYPE_RESERVED)) ||
            // more than one of byref, array, and vector
            (!(currTypePtr==0 || currTypePtr==DBTYPE_BYREF ||
               currTypePtr==DBTYPE_ARRAY || currTypePtr==DBTYPE_VECTOR)) ||
            // provider owned memory for non-pointer type
            (!currTypePtr && currBinding.dwMemOwner==DBMEMOWNER_PROVIDEROWNED)
           )
        {
            dbsStatus = DBBINDSTATUS_BADBINDINFO;
        }

        // we only support client owned memory
        if (currBinding.dwMemOwner != DBMEMOWNER_CLIENTOWNED)
            dbsStatus = DBBINDSTATUS_BADBINDINFO;
        
        CSTDColumnInfo &  stdColInfo = _astdcolinfo[ColToDBColIndex(icol)];

        // Check for legal type conversion
        if (stdColInfo.dwFlags & DBCOLUMNFLAGS_ISBOOKMARK)
        {
            // For bookmarks, we support an explicit list of target types.
            if ((currBinding.wType != (DBTYPE_UI1 | DBTYPE_VECTOR)) &&
                (currBinding.wType != DBTYPE_BYTES) &&
                (currBinding.wType != DBTYPE_I4) &&
                (currBinding.wType != DBTYPE_UI4)) // sign confusion in DLcursor
            {
                dbsStatus = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
            }
            // furthermore, consumer must give us enough room for the bookmark
            if (currBinding.wType == DBTYPE_BYTES &&
                currBinding.cbMaxLen < sizeof(HROW))
            {
                dbsStatus = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
            }
        }
        else if (stdColInfo.dwFlags & DBCOLUMNFLAGS_ISCHAPTER)
        {
            if (currBinding.wType != DBTYPE_HCHAPTER)
            {
                dbsStatus = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
            }
        }
        else
        {   // for non-bookmarks, use my IConvertType::CanConvert
            if (CanConvert(stdColInfo.dwType, currBinding.wType,
                           DBCONVERTFLAGS_COLUMN) != S_OK)
            {
                dbsStatus = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
            }
        }

        // Check for correct sizing of types.
        // Sometimes macros are used which do sizeof(), but specify wrong type.
        // (i.e. bind a char[20] as a DWORD.)

// BUGBUG: Need to implement NOW ***TLL***
//        if (FAILED(CheckSizeofDBType(aBindings[ibind].dwType,
//                                     aBindings[ibind].cbMaxLen )))
//        {
//            hr = E_FAIL;
//            goto Cleanup;
//        }

        if (aStatus)
            aStatus[ibind] = dbsStatus;
        if (dbsStatus == DBBINDSTATUS_OK)
        {
            ++ cValidBindings;
        }

#if DBG == 1
        DbgCheckOverlap(currBinding);
#endif  // DBG == 1
    }

    if (cValidBindings < cBindings)
    {
        goto Cleanup;
    }
    
    // Make a copy of the client's binding array, and the type of binding.
    // We could potentially give out direct ptrs to the structure.
    // Note: Accessors with no bindings (cBindings = 0) are legal, that is the
    //       AccessorFormat for a null accessor has just the dwAccFlags and
    //       cBindings fields the aBindings[] is not allocated.

    pAccessor = new(cBindings) AccessorFormat(1, eAccFlags, cBindings, aBindings);
    if (pAccessor == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // add the accessor to the active list
    _dblAccessors.Append(pAccessor);
    
    *phAccessor = (HACCESSOR)pAccessor;

    hr = S_OK;

Cleanup:
    if (hr == S_OK)     // no global errors, check whether bindings worked
        hr = cValidBindings<cBindings   ? cValidBindings==0 ? DB_E_ERRORSOCCURRED
            // BUGBUG M9 doesn't mention DB_S_ERRORSOCCURRED, so use DB_E_ instead
                                                            : DB_E_ERRORSOCCURRED
                                        : S_OK;
    return hr;
}



//+---------------------------------------------------------------------------
//  Member:     GetBindings (public member)
//
//  Synopsis:   Returns the bindings in an accessor
//
//  Arguments:  hAccessor       Accessor Handle
//              peAccFlags      Accessor Type flag
//              pcBindings      Number of Bindings returned
//              paBindings      Bindings
//
//  Returns:    S_OK                    if everything is fine,
//              E_INVALIDARG            peAccFlags/pcBinding/paBinding was NULL
//              E_OUTOFMEMORY           Out of Memory
//              E_UNEXPECTED            Zombie State
//              DB_E_BADACCESSORHANDLE  Invalid Accessor given
//              DB_E_NOTREENTRANT       Can not enter function at this time.
//

STDMETHODIMP
CImpIRowset::GetBindings (HACCESSOR hAccessor,
                          DBACCESSORFLAGS * peAccFlags,
                          DBCOUNTITEM * pcBindings,
                          DBBINDING ** paBindings )
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::GetBindings(%p {%p, %p, %p, %p})",
             this, hAccessor, peAccFlags, pcBindings, paBindings ));

    HRESULT     hr = S_OK;
    ULONG       cBindingSize;

    Assert(hAccessor);

    // Retrieve our accessor structure from the client's hAccessor,
    // make a copy of the bindings for the user, then done.
    AccessorFormat    *pAccessor = (AccessorFormat *)hAccessor;

    // Check Other Parameters
    if (!(peAccFlags && pcBindings && paBindings))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Set values that should be returned if an error occurrs
    *peAccFlags = DBACCESSOR_INVALID;
    *pcBindings = 0;
    *paBindings = NULL;

    // Allocate and return Array of bindings
    cBindingSize = pAccessor->cBindings * sizeof(DBBINDING);
    *paBindings = (DBBINDING *)CoTaskMemAlloc(cBindingSize);
    if (*paBindings)
    {
        *peAccFlags  = pAccessor->dwAccFlags;
        *pcBindings = pAccessor->cBindings;
        memcpy(*paBindings, pAccessor->aBindings, cBindingSize);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    return hr;
}


HRESULT
CImpIRowset::EnsureReferencedRowset(HCHAPTER hChapter, ULONG uStdRow, ULONG icol,
                                    OLEDBSimpleProvider **ppOSP)
{
    HRESULT hr = S_OK;
    VARIANT varOSP;             // Variant to hold returned OSP
    OLEDBSimpleProvider *pOSP = NULL;
    OLEDBSimpleProvider *pOSPparent;
    CSTDColumnInfo& stdColInfo = _astdcolinfo[ColToDBColIndex(icol)];
    VariantInit(&varOSP);

    // if the call was made only to create the child rowset, and it already
    // exists, there's nothing to do.
    if (stdColInfo.pChapRowset != NULL && ppOSP == NULL)
        goto Cleanup;

    if (stdColInfo.dwType != DBTYPE_HCHAPTER)
    {
        hr = DB_E_NOTAREFERENCECOLUMN;
        goto Cleanup;
    }

    // get the OSP and (row,col) from which to fetch a child OSP.
    if (uStdRow > 0)
    {
        // if we have an OSP and (row,col) already, just use them.
        // This is the normal case, when GetData is trying to retrieve
        // a chapter handle from a field.
        pOSPparent = GetpOSP(hChapter);
    }
    else
    {
        // if we don't have an OSP and row, arbitrarily use the first row
        // of the metaOSP.  This is the case when a client calls
        // GetReferencedRowset on a column before actually fetching any
        // values from that column.  This strategy will fail if the metaOSP
        // doesn't have any rows, but that seems pretty unlikely.
        uStdRow = 1;
        pOSPparent = GetpMetaOSP();
    }

    // fetch the child OSP, on which we'll create the child rowset
    hr = pOSPparent->getVariant(uStdRow, icol, OSPFORMAT_RAW, &varOSP);

    if (hr || varOSP.vt != VT_UNKNOWN)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure it's an OSP
    hr = varOSP.punkVal->QueryInterface(IID_OLEDBSimpleProvider, (void **)&pOSP);
    if (hr)
        goto Cleanup;

    // return the OSP, if desired
    if (ppOSP)
    {
        pOSP->AddRef();
        *ppOSP = pOSP;
    }

    // make a child rowset, if needed
    if (stdColInfo.pChapRowset == NULL)
    {
        hr = CChapRowset::CreateRowset(pOSP,
                                       (IUnknown **)&stdColInfo.pChapRowset);
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    ClearInterface(&pOSP);
    VariantClear(&varOSP);
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     GetChapterData
//
//  Synopsis:   The Chapter returning part of GetData, pulled out for readability
//
//  Arguments:  see below
//
//  Returns:    S_OK                    if everything is fine,
//              DB_S_ERRORSOCCURRED     for compound accessor, partial success
//              DB_E_ERRORSOCCURRED     for compound accessor, all columns failed
//              E_FAIL                  Catch all (pData is NULL)
//              E_OUTOFMEMORY           Out of Memory
//              E_UNEXPECTED            Zombie State
//              DB_E_BADACCESSORHANDLE  Invalid Accessor given
//

HRESULT
CImpIRowset::GetChapterData(XferInfo &xfrData, CSTDColumnInfo &stdColInfo,
                            DBBINDING &currBinding, HCHAPTER hChapter,
                            HROW hrow, ULONG uStdRow, ULONG icol)
{
    HRESULT hr;
    OLEDBSimpleProvider *pOSP = NULL;
    HCHAPTER hChildChapter = NULL;

    *xfrData.pdwStatus = DBSTATUS_E_CANTCREATE; // assume failure
    Assert(currBinding.wType == DBTYPE_HCHAPTER);

    hr = EnsureReferencedRowset(hChapter, uStdRow, icol, &pOSP);
    if (hr)
        goto Cleanup;

    Assert(stdColInfo.pChapRowset);
    hr = stdColInfo.pChapRowset->EnsureHChapter(hrow, pOSP, &hChildChapter);
    if (hr)
        goto Cleanup;

    *(HCHAPTER *)xfrData.pData = hChildChapter;
    *xfrData.pdwStatus = DBSTATUS_S_OK;

Cleanup:

    ClearInterface(&pOSP);

    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     GetData (public member)
//
//  Synopsis:   Retrieves data from the rowset's cache
//
//  Arguments:  hRow        Row Handle
//              hAccessor   Accessor to use
//              pData       Pointer to buffer where data should go.
//
//  Returns:    S_OK                    if everything is fine,
//              DB_S_ERRORSOCCURRED     for compound accessor, partial success
//              DB_E_ERRORSOCCURRED     for compound accessor, all columns failed
//              E_FAIL                  Catch all (pData is NULL)
//              E_OUTOFMEMORY           Out of Memory
//              E_UNEXPECTED            Zombie State
//              DB_E_BADACCESSORHANDLE  Invalid Accessor given
//

STDMETHODIMP
CImpIRowset::GetData (HROW hRow, HACCESSOR hAccessor, void *pData)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::GetData(%p {%p, %p, %p})",
             this, hRow, hAccessor, pData ));

    HRESULT         hr = S_OK;
    HRESULT         hrIndex;
    HRESULT         hrLastFailure = S_OK;
    AccessorFormat  *pAccessor;
    DBBINDING       *pBinding;
    ULONG           cBindings;
    ULONG           ibind;
    ULONG           ulErrorCount;
    HCHAPTER        hChapter;
    DBCOUNTITEM     uStdRow;

    Assert(hAccessor);
    pAccessor = (AccessorFormat *)hAccessor;

    cBindings = pAccessor->cBindings;
    pBinding  = pAccessor->aBindings;

    // Ensure a place to put data, unless the accessor is the null accessor then
    // a NULL pData is okay.
    if (pData == NULL && cBindings != 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hrIndex = HROW2Index(hRow, uStdRow);
    if (hrIndex)
    {
        // Most errors mean immediate failure.
        // If row has been deleted, or is being deleted, only allow
        //  bookmark fetches.
        // Check in advance rather than in loop below, so that we don't
        //  have a problem on what success/error code to return after
        //  a DBVECTOR may have been allocated.  Avoid leaks!
        
        if (hrIndex != DB_E_DELETEDROW && hrIndex != DB_E_NOTREENTRANT)
        {
            hr = hrIndex;
            goto Cleanup;
        }
        for (ibind = 0; ibind < cBindings; ibind++)
        {
            if (pBinding[ibind].iOrdinal != 0)   // not BOOKMARK column
            {
                hr = DB_E_DELETEDROW;
                goto Cleanup;
            }
        }
    }

    hChapter = ChapterFromHRow((ChRow) hRow);

    // These restrictions should have been tested by CreateAccessor
    Assert(pAccessor->dwAccFlags == DBACCESSOR_ROWDATA);

    ulErrorCount = 0;
    for (ibind = 0; ibind < cBindings; ibind++)
    {
        XferInfo        xfrData;
        DBBINDING &     currBinding = pBinding[ibind];
        DBPART          dwPart = currBinding.dwPart;
        ULONG           icol = currBinding.iOrdinal;
        CSTDColumnInfo &stdColInfo = _astdcolinfo[ColToDBColIndex(icol)];
        ULONG           uDummyLength;
        DWORD           dwDummyStatus;

        xfrData.dwDBType = stdColInfo.dwType;
        xfrData.ulDataMaxLength = currBinding.cbMaxLen;
        xfrData.dwAccType = currBinding.wType;
        xfrData.pData = dwPart & DBPART_VALUE
                        ? ((BYTE *) pData + currBinding.obValue)
                        : NULL;
        xfrData.pulXferLength = dwPart & DBPART_LENGTH
                        ? (ULONG *)((BYTE *) pData + currBinding.obLength)
                        : &uDummyLength;
        xfrData.pdwStatus = dwPart & DBPART_STATUS
                        ? (DWORD *)((BYTE *) pData + currBinding.obStatus)
                        : &dwDummyStatus;

        // What are we retrieving a bookmark or regular column data?
        if (stdColInfo.dwFlags & DBCOLUMNFLAGS_ISBOOKMARK)
        {
            // dlcursor seems confused about UI4 vs. I4 -cfranks 11Jun96
            if (currBinding.wType == DBTYPE_I4 ||
                currBinding.wType == DBTYPE_UI4 ||
                currBinding.wType == DBTYPE_BYTES)
            {
                Assert(currBinding.wType != DBTYPE_BYTES ||
                    currBinding.cbMaxLen >= sizeof(HROW));
                *xfrData.pulXferLength = sizeof(HROW);
                if (xfrData.pData)
                {
                    *(HROW *)(xfrData.pData) = hRow;
                }
            }
            else
            {
                Assert(currBinding.wType == (DBTYPE_VECTOR|DBTYPE_UI1));
                *xfrData.pulXferLength = sizeof(DBVECTOR);
                if (xfrData.pData)
                {
                    // Get the bookmark.
                    ((DBVECTOR *)(xfrData.pData))->size = sizeof(HROW);
                    HROW *pu = (HROW *)CoTaskMemAlloc(sizeof(HROW));
                    if (!pu)
                    {
                        *xfrData.pdwStatus = DBSTATUS_E_CANTCREATE;
                        hr = E_OUTOFMEMORY;
                        goto LoopCleanup;
                    }

                    *pu = hRow;
                    ((DBVECTOR *)(xfrData.pData))->ptr = pu;
                }
            }

            *xfrData.pdwStatus = DBSTATUS_S_OK;

            hr = S_OK;
        }
        else if (stdColInfo.dwFlags & DBCOLUMNFLAGS_ISCHAPTER)
        {
            hr = GetChapterData(xfrData, stdColInfo, currBinding, hChapter,
                           hRow, uStdRow, icol);
        }
        else
        {
            // handles this case above
            Assert(!hrIndex);
            hr = DataCoerce(DataFromProvider, hChapter, uStdRow, icol, xfrData);
        }

LoopCleanup:
        if (hr)
        {
            hrLastFailure = hr;
            ulErrorCount++;
        }
    }

    // We report any lossy conversions with a special status.
    if (ulErrorCount)
    {
        Assert(hrLastFailure != S_OK);
        hr = (ulErrorCount == cBindings) ? DB_E_ERRORSOCCURRED
                                         : DB_S_ERRORSOCCURRED;
    }
    else
    {
        Assert(hr == S_OK);
    }
Cleanup:



    RRETURN4(hr, DB_E_ERRORSOCCURRED, DB_S_ERRORSOCCURRED,
                DB_E_UNSUPPORTEDCONVERSION, DB_E_BADACCESSORHANDLE);
}

//+---------------------------------------------------------------------------
//  Member:     GetNextRows (public member)
//
//  Synopsis:   Fetches rows squentially, remembering the previous position
//
//  Arguments:  hChapter            Chapter handle
//              lRowsOffset         Rows to skip before reading
//              cRows               Number of rows to fetch
//              pcRowsObtained      Number of rows obtained
//              pahRows             Array of Hrows obtained
//
//  Returns:    S_OK                    if everything is fine,
//              E_INVALIDARG            invalid argument
//              E_OUTOFMEMORY           Out of Memory
//              E_UNEXPECTED            Zombie State
//              DB_E_BADACCESSORHANDLE  Invalid Accessor given
//              DB_E_NOTREENTRANT       Can not enter function at this time.
//

STDMETHODIMP
CImpIRowset::GetNextRows (HCHAPTER hChapter,
                          DBROWOFFSET lRowsOffset,
                          DBROWCOUNT cRows,
                          DBCOUNTITEM * pcRowsObtained,
                          HROW ** pahRows)
{
    HRESULT hr;
    DBCOUNTITEM ulRow;
    BOOL fRowsAllocated = FALSE;
    COSPData * pOSPData = GetpOSPData(hChapter);

    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::GetNextRows(%p {%l, %l, %p, %p})",
             this, lRowsOffset, cRows, pcRowsObtained, pahRows ));

    *pcRowsObtained = 0;                // in case of error

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Error;
    }
    
    // Allocate memory block if we need to:
    // BUGBUG:: We're not freeing this on failure right now!!!
    if (!*pahRows)
    {
        *pahRows = (HROW *)CoTaskMemAlloc(sizeof(HROW) * cRows);
        if (!*pahRows)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
        fRowsAllocated = TRUE;
    }

    // First time?
    if (DBBMK_INITIAL==pOSPData->_iGetNextCursor)
    {
        if (cRows > 0)
        {
            pOSPData->_iGetNextCursor =
                (ULONG) (HRowFromIndex(hChapter, 1)).ToNileHRow();
        }
        else
        {
            // Negative cRow means start at end and go backwards
            pOSPData->_iGetNextCursor =
                (ULONG) (HRowFromIndex(hChapter, pOSPData->_cSTDRows)).ToNileHRow();
        }
    }

    hr = Bookmark2HRowNumber(hChapter, sizeof(pOSPData->_iGetNextCursor),
                             (BYTE *)&pOSPData->_iGetNextCursor, ulRow);
    if (hr)
    {
        if (hr != DB_S_BOOKMARKSKIPPED) goto Error;
        // Surprsingly, this call is not supposed to return DB_S_BOOKMARKSKIPPED
        hr = S_OK;
    }

    // GenerateHRowsFromHRowNumber also takes care of firing events
    hr = THR(GenerateHRowsFromHRowNumber(hChapter, ulRow, lRowsOffset, cRows,
                                         pcRowsObtained, pahRows));

    // Advance cursor
    if (!hr)                            // Don't advance on error?
    {
        ulRow += lRowsOffset +
                 (cRows > 0 ? *pcRowsObtained : -(LONG)*pcRowsObtained);
        pOSPData->_iGetNextCursor = (ULONG) (HRowFromIndex(hChapter, ulRow)).ToNileHRow();
    }

Error:
    return hr;
}



//+---------------------------------------------------------------------------
//  Member:     RestartPosition (public member)
//
//  Synopsis:   Repositions the "next fetch position" to the start of the
//              rowset/chapter.
//
//  Arguments:  hChapter            Chapter handle
//
//  Returns:    S_OK                    if everything is fine,
//              DB_S_COLUMNSCHANGED     see OLE DB docs
//              E_FAIL                  provider-specific error
//              E_UNEXPECTED            Zombie State
//              DB_E_BADCHAPTER         invalid chapter
//              DB_E_NOTREENTRANT       Can not enter function at this time.
//              DB_E_ROWSNOTRELEASED    provider insists all HROWS released
//

STDMETHODIMP
CImpIRowset::RestartPosition (HCHAPTER hChapter)
{
    HRESULT hr = S_OK;
    COSPData *pOSPData = GetpOSPData(hChapter);

    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::RestartPosition(%p {%p})",
             this, hChapter ));

    if (pOSPData)
    {
        pOSPData->_iGetNextCursor = DBBMK_INITIAL;    // reset cursor
    }
    else
    {
        hr = DB_E_BADCHAPTER;
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     GetReferencedRowset (public member)
//
//  Synopsis:   Returns an interface pointer to the rowset to which the bookmark
//              applies.
//
//  Arguments:  iOrdinal            Bookmark or Chapter Column
//              riid                ID of desired interface
//              ppReferencedRowset  where to put interface pointer 
//
//  Returns:    S_OK                        if everything is fine,
//              E_FAIL                      Provider specific Error
//              E_INVALIDARG                Invalid parameters were specified
//              E_UNEXPECTED                Zombie State
//              DB_E_NOTAREFERENCECOLUMN    iColumn contains no Bookmarks or
//                                          Chapters
//              DB_E_NOTREENTRANT           Can not enter function at this time
//

STDMETHODIMP
CImpIRowset::GetReferencedRowset (DBORDINAL iOrdinal, REFIID riid, IUnknown **ppReferencedRowset)
{
    HRESULT hr;
    CImpIRowset *pRowset;

    *ppReferencedRowset = NULL;

    hr = EnsureReferencedRowset(DB_NULL_HCHAPTER, 0, iOrdinal, NULL);
    if (hr)
        goto Cleanup;
    
    pRowset = _astdcolinfo[iOrdinal].pChapRowset;
    Assert(pRowset);
    if (pRowset)
        hr = pRowset->QueryInterface(riid, (void **)ppReferencedRowset);

Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     GetProperties (public member)
//
//  Synopsis:   Returns current settings of properites supported by
//              the rowset.
//
//  BUGBUG:     this is a temporary minimum implementation, which only handles
//              the properties that DLCURSOR cares about.
//
//  Arguments:  cPropIDSets          number of explicitly requested propID
//                                   sets; 0 means all supported by callee.
//              aPropIDSets          if cPropIDSets != 0 array of propID sets for
//                                   which properties are being requested
//              pcPropSets           where to return # of DBPROPSETs
//              paPropSets           where to return ptr to DBPROPSETs
//              
//
//  Returns:    S_OK                 The method succeeded
//              E_FAIL               Provider-specific error
//              E_INVALIDARG         Bad arguments (several possibilities)
//              E_OUTOFMEMORY        Provider can't allocate sufficient memory
//              E_UNEXPECTED         Zombie state
//              DB_S_ERRORSOCCURRED  Some but not all properties failed
//              DB_E_ERRORSOCCURRED  All properties failed
//

STDMETHODIMP
CImpIRowset::GetProperties (const ULONG cPropIDSets, const DBPROPIDSET aPropIDSets[],
                            ULONG *pcPropSets, DBPROPSET **paPropSets)
{
    TraceTag((tagNileRowsetProvider,
              "CImpIRowset::GetProperties(%p {%u, %p, %p, %p})", this,
              cPropIDSets, aPropIDSets, pcPropSets, paPropSets ));

    HRESULT hr;
    ULONG cPropSets = cPropIDSets;
    ULONG iPropSet;
    DBPROPSET* aPropSets = 0;
    BOOL bFailures=FALSE, bSuccesses=FALSE; // did none, some or all properties succeed

    if (cPropIDSets == 0)       // means "return all properties"
    {
        cPropSets = _dbpProperties.GetNPropSets();
    }
    
    // check for bad arguments
    if (pcPropSets==0 || paPropSets==0 || (cPropIDSets>0 && aPropIDSets==0))
    {
        hr = E_INVALIDARG; 
        goto Cleanup;
    }

    for (iPropSet=0; iPropSet<cPropIDSets; ++iPropSet)
    {
        if (aPropIDSets[iPropSet].cPropertyIDs > 0  &&
            aPropIDSets[iPropSet].rgPropertyIDs == 0)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // get memory for results
    aPropSets = (DBPROPSET *)
                        CoTaskMemAlloc(sizeof(DBPROPSET) * cPropSets);
    if (aPropSets == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    for (iPropSet=0; iPropSet<cPropSets; ++iPropSet)
    {
        aPropSets[iPropSet].rgProperties = 0;   // makes error cleanup simpler
    }
    
    // fill in each property set
    for (iPropSet=0; iPropSet<cPropSets; ++iPropSet)
    {
        DBPROPSET* pPropSet = & aPropSets[iPropSet];

        if (cPropIDSets == 0)               // copying all properties
        {
            hr = _dbpProperties.CopyPropertySet(iPropSet, pPropSet);
            if (hr)
                goto Error;
            bSuccesses = TRUE;
        }
        else if (aPropIDSets[iPropSet].cPropertyIDs == 0)   // all props for this GUID
        {
            hr = _dbpProperties.CopyPropertySet(aPropIDSets[iPropSet].guidPropertySet,
                                                pPropSet);
            if (hr == E_FAIL)           // no properties for this GUID
            {
                pPropSet->cProperties = 0;
                pPropSet->rgProperties = 0;
                pPropSet->guidPropertySet = aPropIDSets[iPropSet].guidPropertySet;
                hr = S_OK;
                bFailures = TRUE;
            }
            else if (hr)
                goto Error;
            bSuccesses = TRUE;
        }
        else                                // explicit list of properties
        {
            const DBPROPIDSET* pPropIDSet = & aPropIDSets[iPropSet];
            ULONG iProp;
            
            pPropSet->cProperties = pPropIDSet->cPropertyIDs;
            pPropSet->guidPropertySet = pPropIDSet->guidPropertySet;
            
            // get memory for property array
            pPropSet->rgProperties = (DBPROP *)
                            CoTaskMemAlloc(sizeof(DBPROP) * pPropIDSet->cPropertyIDs);
            if (pPropSet->rgProperties == 0)
            {
                hr = E_OUTOFMEMORY;
                pPropSet->cProperties = 0;
                goto Error;
            }

            // fill in property array
            for (iProp=0; iProp<pPropIDSet->cPropertyIDs; ++iProp)
            {
                const DBPROP* pProp = _dbpProperties.GetProperty(pPropIDSet->guidPropertySet,
                                                pPropIDSet->rgPropertyIDs[iProp]);
                if (pProp)
                {
                    pPropSet->rgProperties[iProp] = *pProp;
                    bSuccesses = TRUE;
                } else
                {
                    pPropSet->rgProperties[iProp].dwPropertyID = pPropIDSet->rgPropertyIDs[iProp];
                    pPropSet->rgProperties[iProp].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
                    bFailures = TRUE;
                }
            }
        }
    }

    *pcPropSets = cPropSets;
    *paPropSets = aPropSets;
    hr = bFailures  ? bSuccesses    ? DB_S_ERRORSOCCURRED
                                    : DB_E_ERRORSOCCURRED
                    : S_OK;

Cleanup:
    RRETURN2 (hr, DB_S_ERRORSOCCURRED, DB_E_ERRORSOCCURRED);

Error:
    // release memory
    if (aPropSets)
    {
        for (iPropSet=0; iPropSet<cPropSets; ++iPropSet)
        {
            CoTaskMemFree(aPropSets[iPropSet].rgProperties);
        }
    }
    CoTaskMemFree(aPropSets);

    // tell caller we didn't allocate memory
    *pcPropSets = 0;
    *paPropSets = 0;
    
    goto Cleanup;
}    

#if 0
// Here's the old code for the old Nile spec
    pInfo->cColumns                     = _cSTDCols + 1; // +1 for bookmark info
    pInfo->cMaxOpenRows                 = 0;
    pInfo->cMaxPendingChangeRows        = 1; // BUGBUG: Since there is no IRowsetUpdate, should this be 0?
    pInfo->cMaxOpenRowsPerChapter       = 0;
    pInfo->cMaxPendingChangesPerChapter = 1; // BUGBUG: Since there is no IRowsetUpdate, should this be 0?
    pInfo->dwFlags                      = DBROWSETFLAGS_CANFETCHBACKWARDS |
                                          DBROWSETFLAGS_LITERALBOOKMARKS  |
                                          DBROWSETFLAGS_ORDEREDBOOKMARKS  |
                                          DBROWSETFLAGS_CANHOLDROWS       |
                                          DBROWSETFLAGS_NOROWRESTRICT     |
                                          DBROWSETFLAGS_DISCONTIGUOUS     |
                                          DBROWSETFLAGS_OWNUPDATEDELETE   |
                                          DBROWSETFLAGS_OWNINSERT         |
                                          DBROWSETFLAGS_OTHERUPDATEDELETE |
                                          DBROWSETFLAGS_OTHERINSERT       |
                                          DBROWSETFLAGS_REMOVEDDELETED;

    Assert("Flags should be cleared." &&
           !(pInfo->dwFlags &             (DBROWSETFLAGS_NOCOLUMNRESTRICT |
                                           DBROWSETFLAGS_CANRELEASELOCKS  |
                                           DBROWSETFLAGS_BLOCKSOFROWS     |
                                           DBROWSETFLAGS_CHAPTERED        |
                                           DBROWSETFLAGS_MULTICHAPTERED   |
                                           DBROWSETFLAGS_TRUEIDENTITY     |
                                           DBROWSETFLAGS_REENTRANTEVENTS  |
                                           DBROWSETFLAGS_COMMITRETAINING  |
                                           DBROWSETFLAGS_ABORTRETAINING)) );
#endif

//+---------------------------------------------------------------------------
//  Member:     GetSpecification (public member)
//
//  Synopsis:   Returns the interface pointer of object that created the rowset
//
//  Arguments:  riid                IID of the interface being queried for
//              ppSpecification     Interface that instantiated this object
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_INVALIDARG            Invalid parameters were specified
//              E_NOINTERFACE           The provider did not support the riid
//              E_UNEXPECTED            Zombie State
//              DB_E_NOTREENTRANT       Can not enter function at this time
//

STDMETHODIMP
CImpIRowset::GetSpecification (REFIID riid, IUnknown **ppSpecification)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::GetSpecification(%p {%p, %p})",
             this, riid, ppSpecification ));

    HRESULT hr = E_NOINTERFACE;

    if (ppSpecification == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppSpecification = NULL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     AddRefAccessor (public member)
//
//  Synopsis:   AddRefs an Accessor
//
//  Arguments:  hAccessor   Accessor to addref
//              pcRefCount  pointer to where to return new refcount
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_UNEXPECTED            Zombie State
//              DB_E_NOTREENTRANT       Can not enter function at this time.
//

STDMETHODIMP
CImpIRowset::AddRefAccessor (HACCESSOR hAccessor, ULONG *pcRefCount)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::AddRefAccessor(%p {%p %p})", this, hAccessor, pcRefCount ));

    Assert(hAccessor);

    AccessorFormat *pAccessor = (AccessorFormat*) hAccessor;

    ++ pAccessor->cRefCount;
    if (pcRefCount)
        *pcRefCount = pAccessor->cRefCount;
        
    return S_OK;
}



//+---------------------------------------------------------------------------
//  Member:     ReleaseAccessor (public member)
//
//  Synopsis:   Releases an Accessor
//
//  Arguments:  hAccessor   Accessor to release
//              pcRefCount  pointer to where to return new refcount
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_UNEXPECTED            Zombie State
//              DB_E_NOTREENTRANT       Can not enter function at this time.
//

STDMETHODIMP
CImpIRowset::ReleaseAccessor (HACCESSOR hAccessor, ULONG *pcRefCount)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::ReleaseAccessor(%p {%p %p})", this, hAccessor, pcRefCount ));

    Assert(hAccessor);

    AccessorFormat *pAccessor = (AccessorFormat*) hAccessor;
    Assert(pAccessor->cRefCount > 0);

    -- pAccessor->cRefCount;
    if (pcRefCount)
        *pcRefCount = pAccessor->cRefCount;
        
    if (pAccessor->cRefCount == 0)
    {
        // remove the accessor from the active list
        _dblAccessors.Remove(pAccessor);
        
        // Free the actual structure.
        delete pAccessor;
    }
    return S_OK;
}



#if defined(PRODUCT_97)
// This function doesn't appear in Nile M9

//+---------------------------------------------------------------------------
//  Member:     ReleaseChapter (public member)
//
//  Synopsis:   Releases the Chapter Context
//
//  Arguments:  hChapter        Chapter handle
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_INVALIDARG            Invalid parameters were specified
//              E_UNEXPECTED            Zombie State
//              DB_E_NOTREENTRANT       Can not enter function at this time
//

STDMETHODIMP
CImpIRowset::ReleaseChapter(HCHAPTER hChapter)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::ReleaseChapter(%p {%p})",
             this, hChapter ));

// Nope, we aren't chaptered
//    FireChapterEvent(hChapter, DBREASON_CHAPTER_RELEASE);

    //Since we are not chaptered
    return E_INVALIDARG; // BUGBUG:  What does Nile really want us to return?
}
#endif


//+---------------------------------------------------------------------------
//  Member:     AddRefRows (public member)
//
//  Synopsis:   Bump ref counts on rows.
//
//  Arguments:  cRow            Number of rows to AddRef
//              ahRow           Array of handles of rows to be released
//              pcRowCounted    Count of rows actually AddRefed, may be NULL
//              aRefCount       Array of refcnts for the rows
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_INVALIDARG            Invalid parameters were specified
//              DB_S_ERRORSOCCURRED     Some but not all rows were invalid
//              DB_E_ERRORSOCCURRED     All rows were invalid
//

STDMETHODIMP
CImpIRowset::AddRefRows (DBCOUNTITEM cRows,
                         const HROW ahRow[],
                         DBREFCOUNT aRefCount[],
                         DBROWSTATUS aRowStatus[])
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::AddRefRows(%p {%u %p %p %p})",
             this, cRows, ahRow, aRefCount, aRowStatus ));
             
    HRESULT hr = S_OK;
    ULONG cErrors = 0;          // number of rows that failed
    ULONG iRow;

    if (ahRow == NULL && cRows != 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    for (iRow=0; iRow < cRows; ++iRow)
    {
        LogErrors(aRowStatus, iRow, 1, DBROWSTATUS_S_OK);
        
        if (ahRow[iRow] == NULL)
        {
            Assert(!"Null HROW to CImpIRowset::AddRefRows.");
            LogErrors(aRowStatus, iRow, 1, DBROWSTATUS_E_INVALID);
            ++ cErrors;
        }
        else
        {
            if (aRefCount)
            {
                // Since we don't actually refcount our hRow objects, we return
                // a fake refcount here.
                aRefCount[iRow] = 1;
            }
        }
    }

    hr =    cErrors == 0 ?      S_OK :
            cErrors < cRows ?   DB_S_ERRORSOCCURRED :
                                DB_E_ERRORSOCCURRED;

Cleanup:
    RRETURN2(hr, DB_S_ERRORSOCCURRED, DB_E_ERRORSOCCURRED);
}

//+---------------------------------------------------------------------------
//  Member:     ReleaseRows (public member)
//
//  Synopsis:   Releases row handles, generating appropriate notifications.
//
//  Arguments:  cRow            Number of rows to release
//              ahRow           Array of handles of rows to be released
//              aRefCount       Array of refcnts for the rows
//              aRowStatus      Array of status codes
//
//  Returns:    S_OK                    if everything is fine,
//              E_FAIL                  Provider specific Error
//              E_INVALIDARG            Invalid parameters were specified
//              E_UNEXPECTED            Zombie State
//              DB_E_NOTREENTRANT       Can not enter function at this time
//

STDMETHODIMP
CImpIRowset::ReleaseRows (DBCOUNTITEM cRows,
                          const HROW ahRow[],
                          DBROWOPTIONS aRowOptions[],
                          DBREFCOUNT aRefCount[],
                          DBROWSTATUS aRowStatus[])
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::ReleaseRows(%p {%u %p %p %p %p})",
             this, cRows, ahRow, aRowOptions, aRefCount, aRowStatus ));

    HRESULT hr = S_OK;

    if (cRows == 0)
    {
        goto Cleanup;
    }

#if defined(PRODUCT_97)
    // BUGBUG:  Should only fire event if ref-count hits 0, but
    //  should fire small number of notifications
    FireRowEvent(cRows, ahRow, DBREASON_ROW_RELEASE, DBEVENTPHASE_DIDEVENT);
#endif
    // BUGBUG: in some sense, all rows are alive, even if their ref-counts
    //  have hit 0.....
    hr = ReleaseRowsQuiet(cRows, ahRow, aRefCount, aRowStatus);


Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//  Member:     ReleaseRowsQuiet (private member)
//
//  Synopsis:   Releases row handles, without generating any notifications.
//
//  Arguments:  cRow            Number of rows to release
//              ahRow           Array of handles of rows to be released
//              aRefCounts      Where to return ref-count of each succesfully
//                              released row.  May be NULL.
//              aRowStatus      Status of each row.  May be NULL.
//
//  Returns:    S_OK                everything worked
//              DB_S_ERRORSOCCURRED some but not all rows failed
//              DB_E_ERRORSOCCURRED all rows failed
//

HRESULT
CImpIRowset::ReleaseRowsQuiet(DBCOUNTITEM cRows, const HROW ahRow[],
                                DBREFCOUNT aRefCounts[], DBROWSTATUS aRowStatus[])
{
    ULONG iRow;
    ULONG cErrors = 0;      // number of rows that caused problems
    HRESULT hr;

    // In the new world, handles don't need to be released.
    // We run this loop only to decrement the refcounts so our
    // caller believes the handles were released.   -cfranks
    for (iRow = 0; iRow<cRows; ++iRow)
    {
        LogErrors(aRowStatus, iRow, 1, DBROWSTATUS_S_OK);
        if (!ahRow[iRow])
        {
            ++ cErrors;
            LogErrors(aRowStatus, iRow, 1, DBROWSTATUS_E_INVALID);
            continue;            
        }

        if (aRefCounts)
        {
            aRefCounts[iRow] = 1;          // Fake refcount
        }
    }

    hr =    cErrors==0 ?    S_OK :
            cErrors<cRows ? DB_S_ERRORSOCCURRED :
                            DB_E_ERRORSOCCURRED;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Member:     IsSameRow (public member)
//
//  Synopsis:   compares two HROWs to see if they are logicaly the same row
//
//  Arguments:  hThisRow        First HROW to compare
//              hThatRow        Second HROW to compare
//
//  Returns:    S_OK                    HROWs are the same
//              S_FALSE                 HROWs are for different rows
//

STDMETHODIMP
CImpIRowset::IsSameRow(HROW hThisRow, HROW hThatRow)
{
    HRESULT hr = S_OK;
    if (!hThisRow || !hThatRow)
    {
        hr = DB_E_BADROWHANDLE;
        goto Cleanup;
    }
    
    if ( FhRowDeleted((ChRow)hThisRow) || FhRowDeleted((ChRow)hThatRow) )
    {
        hr = DB_E_DELETEDROW;
        goto Cleanup;
    }


    if ((IndexFromHRow((ChRow) hThisRow) != IndexFromHRow((ChRow) hThatRow)) ||
        (ChapterFromHRow((ChRow) hThisRow) != ChapterFromHRow((ChRow) hThatRow)))
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    return hr;
}


#if defined(PRODUCT_97)
// Nile M9 doesn't support chapters, so this function is useless

//+---------------------------------------------------------------------------
//  Member:     FireChapterEvent (public member)
//
//  Synopsis:   Fires the IRowsetNotify::OnChapterChange event on activation
//              or release of a chapter.
//
//  Arguments:  hChapter                chapter handle
//              eReason                 action that caused change event to fire
//
//  Returns:    S_OK                    if everything is fine
//              S_UNWANTEDEVENT         not interested in event anymore
//

HRESULT
CImpIRowset::FireChapterEvent(HCHAPTER hChapter, DBREASON eReason)
{
    IRowsetNotify *  pRSN;
    DWORD            dw    = 0;

    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::FireChapterEvent(%p {%p, %l})",
             this, hChapter, eReason ));

    HRESULT     hr = S_OK;

    if (_pRNSink)
    {
        // BUGBUG: Handle cancelling the event.
        NOTIFY_STATE ns = EnterNotify(eReason, DBEVENTPHASE_DIDEVENT);

        while ((pRSN = (IRowsetNotify*)GetNextSink(_pRNSink, &dw)) != NULL)
        {
            hr = pRSN->OnChapterChange((IRowset *) this,
                                           hChapter,
                                           eReason);
        }

        LeaveNotify(eReason, DBEVENTPHASE_DIDEVENT, ns);
    }

    return hr;
}
#endif


//+---------------------------------------------------------------------------
//  Member:     FireFieldEvent (public member)
//
//  Synopsis:   Fires the IRowsetNotify::OnFieldChange event on change to the
//              value of a field.
//
//  Arguments:  hRow                    row of field that changed
//              iColumn                 column in the row that changed
//              eReason                 action that caused change event to fire
//              ePhase                  phase of this notification
//
//  Returns:    S_OK                    if everything is fine
//              S_UNWANTEDEVENT         not interested in event anymore
//              S_UNWANTEDPHASE         not interested in this event phase
//              S_FALSE                 event/phase is vetoed
//

HRESULT
CImpIRowset::FireFieldEvent(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[],
                            DBREASON eReason, DBEVENTPHASE ePhase)
{
    AAINDEX         aaidx;
    HRESULT         hr = S_OK;
    IRowsetNotify * pRSN = NULL;
    NOTIFY_STATE    ns;
    
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::FireFieldEvent(%p {%l, %l, %p, %l, %l})",
             this, hRow, cColumns, aColumns, eReason, ePhase ));

    aaidx = AA_IDX_UNKNOWN;
    
    ns = EnterNotify(eReason, ePhase);

    for (;;)
    {
        aaidx = FindNextAAIndex(
            DISPID_A_ROWSETNOTIFYSINK, 
            CAttrValue::AA_Internal, 
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        ClearInterface(&pRSN);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pRSN)))
        {
            hr = THR(pRSN->OnFieldChange(
                    (IRowset *) this, 
                    hRow, 
                    cColumns, 
                    aColumns,
                    eReason, 
                    ePhase, 
                    TRUE));
            if (hr && (hr==DB_S_UNWANTEDPHASE || hr==DB_S_UNWANTEDREASON))
            {
                hr = S_OK;
            }
        }
    }

    LeaveNotify(eReason, ePhase, ns);

    ReleaseInterface(pRSN);
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//  Member:     FireRowEvent (public member)
//
//  Synopsis:   Fires the IRowsetNotify::OnRowChange event on first change to
//              a row or any whole-row change.
//
//  Arguments:  cRows                   count of HROWs in rghRows
//              rghRows                 array of HROWs which are changing
//              eReason                 action that caused change event to fire
//              ePhase                  phase of this notification
//
//  Returns:    S_OK                    if everything is fine
//              S_UNWANTEDEVENT         not interested in event anymore
//              S_UNWANTEDPHASE         not interested in this event phase
//              S_FALSE                 event/phase is vetoed
//

HRESULT
CImpIRowset::FireRowEvent(DBCOUNTITEM cRows, const HROW rghRows[],
                          DBREASON eReason, DBEVENTPHASE ePhase)
{
    AAINDEX         aaidx;
    HRESULT         hr = S_OK;
    IRowsetNotify * pRSN = NULL;
    NOTIFY_STATE    ns;
    
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::FireRowEvent(%p {%l, %p, %l, %l})",
             this, cRows, rghRows, eReason, ePhase ));

    aaidx = AA_IDX_UNKNOWN;
    
    ns = EnterNotify(eReason, ePhase);

    for(;;)
    {
        aaidx = FindNextAAIndex(DISPID_A_ROWSETNOTIFYSINK,
                                CAttrValue::AA_Internal, aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        ClearInterface(&pRSN);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pRSN)))
        {
            hr = THR(pRSN->OnRowChange(
                    (IRowset *) this, 
                    cRows, 
                    rghRows,
                    eReason, 
                    ePhase, 
                    FALSE));
            if (hr && (hr==DB_S_UNWANTEDPHASE || hr==DB_S_UNWANTEDREASON))
            {
                hr = S_OK;
            }
        }
    }

    LeaveNotify(eReason, ePhase, ns);
    ReleaseInterface(pRSN);
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//  Member:     FireRowsetEvent (public member)
//
//  Synopsis:   Fires the IRowsetNotify::OnRowsetChange event on any change to
//              a rowset.
//
//  Arguments:  eReason                 action that caused change event to fire
//
//  Returns:    S_OK                    if everything is fine
//              S_UNWANTEDEVENT         not interested in event anymore
//

HRESULT
CImpIRowset::FireRowsetEvent(DBREASON eReason, DBEVENTPHASE ePhase)
{
    AAINDEX         aaidx;
    HRESULT         hr = S_OK;
    IRowsetNotify * pRSN = NULL;
    NOTIFY_STATE    ns;
    
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::FireRowEvent(%p {%l, %l})",
             this, eReason, ePhase));

    aaidx = AA_IDX_UNKNOWN;
    
    ns = EnterNotify(eReason, DBEVENTPHASE_DIDEVENT);
    
    for (;;)
    {
        aaidx = FindNextAAIndex(
            DISPID_A_ROWSETNOTIFYSINK, 
            CAttrValue::AA_Internal, 
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        ClearInterface(&pRSN);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pRSN)))
        {
            hr = THR(pRSN->OnRowsetChange(
                    (IRowset *) this, 
                    eReason, 
                    ePhase, 
                    FALSE));
            if (hr && (hr==DB_S_UNWANTEDPHASE || hr==DB_S_UNWANTEDREASON))
            {
                hr = S_OK;
            }
        }
    }

    LeaveNotify(eReason, DBEVENTPHASE_DIDEVENT, ns);

    ReleaseInterface(pRSN);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     FireAsynchOnProgress
//
//  Synopsis:   Fires the IDBAsynchNotify::OnPgoress event 
//
//  Arguments:  ulProgress              the current row count
//              ulProgressMax           the estimated final row count
//              ulStatusCode            DBASYNCHPHASE_POPULATION, or
//                                      DBASYNCHPHASE_COMPLETE
//
//  Returns:    S_OK                    if everything is fine
//              S_UNWANTEDEVENT         not interested in event anymore
//              S_UNWANTEDOPERATION
//

HRESULT
CImpIRowset::FireAsynchOnProgress(DBCOUNTITEM ulProgress, DBCOUNTITEM ulProgressMax,
                                  DBASYNCHPHASE ulStatusCode)
{
    AAINDEX         aaidx;
    HRESULT         hr = S_OK;
    IDBAsynchNotify * pDBAN = NULL;
    
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::FireAsynchOnProgress(%p {%l, %l, %l})",
              this, ulProgress, ulProgressMax, ulStatusCode));

    aaidx = AA_IDX_UNKNOWN;

    for (;;)
    {
        aaidx = FindNextAAIndex(
            DISPID_A_ROWSETASYNCHNOTIFYSINK, 
            CAttrValue::AA_Internal, 
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        ClearInterface(&pDBAN);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pDBAN)))
        {
            hr = THR(pDBAN->OnProgress(
                    NULL,               // chapter
                    DBASYNCHOP_OPEN,
                    ulProgress,                 
                    ulProgressMax,
                    ulStatusCode,
                    NULL));             // status text
        }
            if (hr && (hr==DB_S_UNWANTEDPHASE || hr==DB_S_UNWANTEDOPERATION))
            {
                hr = S_OK;
            }
    }

    ReleaseInterface(pDBAN);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Member:     FireAsynchOnStop
//
//  Synopsis:   Fires the IDBAsynchNotify::OnStop event 
//
//  Arguments:  hrStatus                one of:
//                                       S_OK, DB_E_CANCELED (sp?),
//                                       E_OUTOFMEMORY, E_FAIL
//
//  Returns:    S_OK                    if everything is fine
//              S_UNWANTEDEVENT         not interested in event anymore
//              S_UNWANTEDOPERATION
//

HRESULT
CImpIRowset::FireAsynchOnStop(HRESULT hrStatus)
{
    AAINDEX         aaidx;
    HRESULT         hr = S_OK;
    IDBAsynchNotify * pDBAN = NULL;
    
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::FireAsynchOnStop(%p {%l})",
             this, hrStatus));

    aaidx = AA_IDX_UNKNOWN;

    for (;;)
    {
        aaidx = FindNextAAIndex(
            DISPID_A_ROWSETASYNCHNOTIFYSINK, 
            CAttrValue::AA_Internal, 
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        ClearInterface(&pDBAN);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pDBAN)))
        {
            hr = THR(pDBAN->OnStop(
                    NULL,               // chapter
                    DBASYNCHOP_OPEN,
                    hrStatus,
                    NULL));             // status text
            if (hr && (hr==DB_S_UNWANTEDPHASE || hr==DB_S_UNWANTEDOPERATION))
            {
                hr = S_OK;
            }
        }
    }

    ReleaseInterface(pDBAN);
    RRETURN(hr);
}

HRESULT
CImpIRowset::Init(OLEDBSimpleProvider *pSTD)
{
    return CacheMetaData();
}

//+---------------------------------------------------------------------------
//
//  Member:     Init (public member)
//
//  Synopsis:   Initializes IRowset with an ISTD
//
//  Arguments:  pSTD                Data provider
//
//  Returns:    S_OK                if everything is fine
//              E_FAIL              initialization failed
//

HRESULT
CTopRowset::Init (OLEDBSimpleProvider *pSTD)
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::Init(%p {%p})",
             this, pSTD ));

    HRESULT         hr = S_OK;

    Assert("Must have an OLEDBSimpleProvider" && pSTD);

    _pOSPData = new COSPData;
    if (!_pOSPData)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hr = _pOSPData->Init(pSTD, this);
    if (hr)
        goto Error;

    // Init our idea of the # of rows from our only OSP member
    _cRows = _pOSPData->_cSTDRows;
    TraceTag((tagOSPRowDelta, "Init. Rowset %p has %ld rows, chapter %p has %ld rows",
                this, _cRows, _pOSPData, _pOSPData->_cSTDRows));

    hr = super::Init(pSTD);

Error:
    // Any errors here fail creation of the rowset.
    // Since we may already have hooked up event sinks to our COSPData,
    // we can't just delete it here.  Even releasing it is nasty, since
    // that will happen again in Rowset Passivate.  Let Passivate clean us up.
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CreateRowset (static)
//
//  Synopsis:   Creates creates an IRowset from an OLEDBSimpleProvider returning
//              as IUnknown
//
//  Arguments:  pRowset         - empty rowset object, new'd by one of our descendent
//                                clases, but not even checked for success.
//              pSTD            - OLEDBSimpleProvider provider wrapped by Nile
//              ppUnk           - IUnknown of Nile provider (IRowset, ...) this
//                                *ppUnk is AddRef on return.
//
//  Returns:    S_OK            - Wrapping succeeded.
//
//  Mac note:   When _MACUNICODE is defined,
//              OLEDBSimpleProvider* == OLEDBSimpleProviderMac* != LPSIMPLETABULARDATA

HRESULT
CImpIRowset::CreateRowset (CImpIRowset *pRowset, LPOLEDBSimpleProvider pSTD, IUnknown **ppUnk)
{
    HRESULT             hr;

    Assert(ppUnk);
    Assert(pSTD);

    *ppUnk = NULL;

    if (!pRowset)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pRowset->Init(pSTD);
    if (!hr)
    {
        *ppUnk = pRowset->getpIUnknown();
    }
    else
    {
        pRowset->Release();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CTopRowset::CreateRowset(LPOLEDBSimpleProvider pSTD, IUnknown **ppUnk)
{
    CTopRowset      *pRowset;

    pRowset = new CTopRowset;
    return super::CreateRowset((CImpIRowset *)pRowset, pSTD, ppUnk);
}

HRESULT
CChapRowset::CreateRowset(LPOLEDBSimpleProvider pSTD, IUnknown **ppUnk)
{
    HRESULT hr;
    CChapRowset      *pRowset = NULL;

    pRowset = new CChapRowset;

    if (pRowset)
    {
        pSTD->AddRef();
        pRowset->_pMetaOSP = pSTD;

        hr = super::CreateRowset((CImpIRowset *)pRowset, pSTD, ppUnk);
        
        if (hr)
        {
            ClearInterface(&pRowset->_pMetaOSP);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//
// Add an entry to _aryOSPData.
//
HRESULT
CChapRowset::EnsureHChapter(HROW hRow,
                            OLEDBSimpleProvider *pOSP, HCHAPTER *phChildChapter)
{
    COSPData *pOSPData;
    HRESULT hr;
    DBROWCOUNT cRows;
    ULONG uRow = ChRow(hRow).DeRef();

    hr = _aryOSPData.EnsureSize(uRow+1);    // we don't use index 0
    if (hr)
        goto Error;

    // Do we already have OSP Data for this OSP?
    if (!(COSPData*)_aryOSPData[uRow])   // No.
    {
        pOSPData = new COSPData;
        if (!pOSPData)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        // Initialize the OSP data.
        // BUGBUG:: Should we do anything here to check this OSP's conformity
        // to the metaOSP's meta data?
        hr = pOSPData->Init(pOSP, this);
        if (hr)
        {
            delete pOSPData;
            goto Error;
        }

        _aryOSPData[uRow] = (PCOSPData)pOSPData;
        IGNORE_HR(pOSP->getRowCount(&cRows));
        _cRows += (ULONG)cRows;
        TraceTag((tagOSPRowDelta, "Added chapter %p with %ld rows.  Rowset %p has %ld rows",
                    pOSPData, cRows, this, _cRows));
    }

    // In case we're returning an existing OSPData, we want to double-check
    // that it's the same OSP (in debug at least).

    Assert(_aryOSPData[uRow]->_pSTD == pOSP);

    *phChildChapter = _aryOSPData[uRow];
    
    
Error:
    return hr;
}

STDMETHODIMP
CImpIRowset::AddRefChapter(HCHAPTER hChapter,
              ULONG *pcRefCount)
{
    ULONG ulRefs;

#ifdef REALLY_REFCOUNT_CHAPTERS
    if (hChapter)
    {
        // BUGBUG there's no way to verify hChapter belongs to this rowset
        COSPData *pOSPData = GetpOSPData(hChapter);
        ulRefs = pOSPData->AddRef();
    }
    else
#endif
    {
        ulRefs = 1;
    }
    
    if (pcRefCount)
        *pcRefCount = ulRefs;
    
    return S_OK;
}

STDMETHODIMP
CImpIRowset::ReleaseChapter(HCHAPTER hChapter,
              ULONG *pcRefCount)
{
    ULONG ulRefs;
    
#ifdef REALLY_REFCOUNT_CHAPTERS
    if (hChapter)
    {
        // BUGBUG there's no way to verify hChapter belongs to this rowset
        COSPData *pOSPData = GetpOSPData(hChapter);
        ulRefs = pOSPData->Release();
    }
    else
#endif
    {
        ulRefs = 1;
    }
    
    if (pcRefCount)
        *pcRefCount = ulRefs;
    
    return S_OK;
}


STDMETHODIMP
CImpIRowset::IsRowInChapter(HCHAPTER hChapter, HROW hrow)
{
    HRESULT hr;

    hr = (hChapter == ChapterFromHRow((ChRow) hrow)) ? S_OK : S_FALSE;

    return hr;
}


void
CChapRowset::Passivate()
{
    // Recursively kill any child OSPDatas we may have.
    // (This will also release the child OSPs).
    for (ULONG uRow=1; uRow < (ULONG)_aryOSPData.Count(); uRow++)
    {
        if (_aryOSPData[uRow]._pCOSPData)
        {
            _aryOSPData[uRow]->Release();
            _aryOSPData[uRow] = NULL; // off chance we're passivate twice
        }
    }
        
    ClearInterface(&_pMetaOSP);

    super::Passivate();
}

HRESULT
COSPData::Init(OLEDBSimpleProvider *pSTD, CImpIRowset * pRowset)
{
    HRESULT hr;
    int isAsync = 0;
    DBROWCOUNT cRowsTemp;
    
    Assert(pRowset && pSTD);

    _pRowset = pRowset;                 // We don't refcount our rowset.

    _pSTD = pSTD;                       // We DO refcount our OSP.
    _pSTD->AddRef();

    // Initialize rowset cache:

    hr = _pSTD->getRowCount(&cRowsTemp);
    _cSTDRows = (LONG)cRowsTemp;
    if (hr)
    {
        goto Error;
    }
    TraceTag((tagOSPRowDelta, "Init.  Chapter %p has %ld rows",
                this, _cSTDRows));

    hr = _pSTD->getColumnCount(&cRowsTemp);
    _cSTDCols = (LONG)cRowsTemp;
    if (hr)
    {
        goto Error;
    }

    // Set the PopulationComplete bit.  It's always true for synchronous OSPs.
    // For async OSPs, we may be too late for the transferComplete notification,
    // so we check if the estimated count agrees with the row count.  Some OSPs
    // will fire transferComplete when we hook up the listener, if they're already
    // complete;  this is fine.  Finally, if getEstimatedRows fails, we assume
    // the OSP is complete;  this makes things no worse than they already are.
    _fPopulationComplete = TRUE;
    IGNORE_HR(_pSTD->isAsync(&isAsync));
    if (isAsync)
    {
        DBROWCOUNT lEstimatedRows;
        if (S_OK == _pSTD->getEstimatedRows(&lEstimatedRows))
        {
            // if the OSPs estimate agrees with its row count, we're too late
            // for the transferComplete notification
            _fPopulationComplete = (_cSTDRows == (ULONG)lEstimatedRows);
        }
    }

    // Set std events:
    // NOTE: Do this before calling GetColumnInfo, since it relies on the std
    //   events to be called.
    hr = _pSTD->addOLEDBSimpleProviderListener((OLEDBSimpleProviderListener *)&_STDEvents);
    if (hr)
    {
        _pSTD->Release();
        goto Error;
    }

    _iGetNextCursor = DBBMK_INITIAL;    // special value
    _iFindRowsCursor = DBBMK_INITIAL;
Error:
    return hr;
}

STDMETHODIMP_(ULONG)
COSPData::Release()
{
    ULONG ulRefs;
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        _ulRefs = 0;
    }
    ulRefs = _ulRefs;
    SubRelease();
    return ulRefs;
}

//
// Pure IUnknown
//

STDMETHODIMP
COSPData::QueryInterface (REFIID riid, LPVOID *ppv)
{
    Assert(ppv);

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = this;
    }
    else
    {
        *ppv = NULL;
    }

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}



