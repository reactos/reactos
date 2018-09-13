//+---------------------------------------------------------------------
//
//   File:      elemdb.cxx
//
//  Contents:   Element's databinding methods
//
//  Classes:    CElement
//
//------------------------------------------------------------------------


#include <headers.hxx>

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include <olesite.hxx>
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include <dmembmgr.hxx>
#endif

MtDefine(DBMEMBERS, DataBind, "DBMEMBERS")
MtDefine(DBMEMBERS_arydsb_pv, DBMEMBERS, "DBMEMBERS::_arydsb::_pv")
MtDefine(CDataBindingEvents, DataBind, "CDataBindingEvents")
MtDefine(CDataBindingEvents_aryXfer_pv, CDataBindingEvents, "CDataBindingEvents::_aryXfer::_pv")

#ifndef NO_DATABINDING
// binding method tables shared by multiple element types
const CDBindMethodsText DBindMethodsTextRichRO(DBIND_ONEWAY|DBIND_HTMLOK);
#endif

static inline LONG
StringLen(LPCTSTR str)
{
    return str ? _tcslen(str) : 0;
}


//+----------------------------------------------------------------------------
//
//  Function: (static helper)
//
//  Synopsis:   str1 and str2 are both strings of field names separated by dots.
//              Find the longest prefix of str2 that's also a suffix of str1.
//              Return a pointer to the "tail" - the part of str2 that sticks
//              out past the end of str1;  this must be non-trivial (at least
//              one field name).

static LPCTSTR
FindOverlapAndTail(LPCTSTR str1, LPCTSTR str2, TCHAR tchDelim=_T('.'))
{
    LONG lLength1 = StringLen(str1);
    LPCTSTR strTail = str2 + StringLen(str2);
    LONG lPrefixLength;

    if (strTail == NULL)
        return NULL;
    
    for (;;)
    {
        // find the next delimiter in str2 (right-to-left)
        while (--strTail >= str2)
        {
            if (*strTail == tchDelim)
                break;
        }

        if (strTail < str2)
            break;

        // compare suffix of str1 with prefix of str2
        lPrefixLength = (strTail - str2);
        if (lLength1 >= lPrefixLength &&
            0 == FormsStringNCmp(str1 + lLength1 - lPrefixLength, lPrefixLength,
                                 str2, lPrefixLength))
        {
            break;
        }
    }

    return strTail + 1;
}


//+----------------------------------------------------------------------------
//
//  Function: get_datafld
//
//  Synopsis: Object model entry point to fetch the datafld property.
//
//  Arguments:
//            [p]  - where to return BSTR containing the string
//
//  Returns:  S_OK                  - this element support such a property
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//            E_POINTER             - NULL pointer to receive BSTR
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CElement::get_dataFld(BSTR * p)
{
#ifndef NO_DATABINDING
    HRESULT hr;

    if ( p )
        *p = NULL;

    // note that set-consumers don't support datafld or dataformatas
    if (CDBindMethods::FDataFldValid(this))
    {
        hr = s_propdescCElementdataFld.b.GetStringProperty(
                p,
                this,
                (CVoid *)(void *)(GetAttrArray()) );
    }
    else
    {
        // To allow for..in to work we need to return S_OK, but an empty BSTR
        hr = SetErrorInfoPGet(S_OK, DISPID_CElement_dataFld);
    }

        RRETURN(hr);
#else
        return DISP_E_MEMBERNOTFOUND;
#endif // ndef NO_DATABINDING
}

//+----------------------------------------------------------------------------
//
//  Function: put_datafld
//
//  Synopsis: Object model entry point to put the datafld property.
//
//  Arguments:
//            [v]  - BSTR containing the new property value
//
//  Returns:  S_OK                  - successful
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//
//  Notes:    BUGBUG: to support dynamic changes to binding properties,
//            additional code should be place here or in OnPropertyChange()
//            to rebind.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CElement::put_dataFld(BSTR v)
{
#ifndef NO_DATABINDING
    HRESULT hr;

    // note that set-consumers don't support datafld or dataformatas
    if (CDBindMethods::FDataFldValid(this))
    {
        DetachDataBinding(ID_DBIND_DEFAULT);

        hr = s_propdescCElementdataFld.b.SetStringProperty(
                v,
                this,
                (CVoid *)(void *)(GetAttrArray()) );

        AttachDataBindings();   // BUGBUG: better to specify ID_DBIND_DEFAULT
        Doc()->TickleDataBinding();
    }
    else
    {
        hr = SetErrorInfoPSet(DISP_E_MEMBERNOTFOUND, DISPID_CElement_dataFld);
    }
    RRETURN(hr);
#else
        return DISP_E_MEMBERNOTFOUND;
#endif // ndef NO_DATABINDING
}

//+----------------------------------------------------------------------------
//
//  Function: get_dataformatas
//
//  Synopsis: Object model entry point to fetch the dataformatas property.
//
//  Arguments:
//            [p]  - where to return BSTR containing the string
//
//  Returns:  S_OK                  - this element support such a property
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//            E_POINTER             - NULL pointer to receive BSTR
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CElement::get_dataFormatAs(BSTR * p)
{
#ifndef NO_DATABINDING
    HRESULT hr;

    if ( p )
        *p = NULL;

    // note that set-consumers don't support datafld or dataformatas
    if (CDBindMethods::FDataFormatAsValid(this))
    {
        hr = s_propdescCElementdataFormatAs.b.GetStringProperty(
                p,
                this,
                (CVoid *)(void *)(GetAttrArray()) );
    }
    else
    {
        // To allow for..in to work we need to return S_OK, but an empty BSTR
        hr = SetErrorInfoPGet(S_OK, DISPID_CElement_dataFormatAs);
    }

    RRETURN(hr);
#else
        return DISP_E_MEMBERNOTFOUND;
#endif // ndef NO_DATABINDING
}

//+----------------------------------------------------------------------------
//
//  Function: put_dataformatas
//
//  Synopsis: Object model entry point to put the dataformatas property.
//
//  Arguments:
//            [v]  - BSTR containing the new property value
//
//  Returns:  S_OK                  - successful
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//
//  Notes:    BUGBUG: to support dynamic changes to binding properties,
//            additional code should be place here or in OnPropertyChange()
//            to rebind.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CElement::put_dataFormatAs(BSTR v)
{
#ifndef NO_DATABINDING
    HRESULT hr;

    // note that set-consumers don't support datafld or dataformatas
    if (CDBindMethods::FDataFormatAsValid(this))
    {
        DetachDataBinding(ID_DBIND_DEFAULT);

        hr = s_propdescCElementdataFormatAs.b.SetStringProperty(
                v,
                this,
                (CVoid *)(void *)(GetAttrArray()) );

        AttachDataBindings();   // BUGBUG: better to specify ID_DBIND_DEFAULT
        Doc()->TickleDataBinding();
    }
    else
    {
        hr = SetErrorInfoPSet(DISP_E_MEMBERNOTFOUND, DISPID_CElement_dataFormatAs);
    }

    RRETURN(hr);
#else
        return DISP_E_MEMBERNOTFOUND;
#endif // ndef NO_DATABINDING
}

//+----------------------------------------------------------------------------
//
//  Function: get_datasrc
//
//  Synopsis: Object model entry point to fetch the datasrc property.
//
//  Arguments:
//            [p]  - where to return BSTR containing the string
//
//  Returns:  S_OK                  - this element support such a property
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//            E_POINTER             - NULL pointer to receive BSTR
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CElement::get_dataSrc(BSTR * p)
{
#ifndef NO_DATABINDING
    HRESULT hr;

    if ( p )
        *p = NULL;

    // note that set-consumers and single-value consumers both support datasrc
    if (CDBindMethods::FDataSrcValid(this))
    {
        hr = s_propdescCElementdataSrc.b.GetStringProperty(
                p,
                this,
                (CVoid *)(void *)(GetAttrArray()) );
    }
    else
    {
        // To allow for..in to work we need to return S_OK, but an empty BSTR
        hr = SetErrorInfoPGet(S_OK, DISPID_CElement_dataSrc);
    }

    RRETURN(hr);
#else
        return DISP_E_MEMBERNOTFOUND;
#endif // ndef NO_DATABINDING
}

//+----------------------------------------------------------------------------
//
//  Function: put_datasrc
//
//  Synopsis: Object model entry point to put the datasrc property.
//
//  Arguments:
//            [v]  - BSTR containing the new property value
//
//  Returns:  S_OK                  - successful
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//
//  Notes:    BUGBUG: to support dynamic changes to binding properties,
//            additional code should be place here or in OnPropertyChange()
//            to rebind.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CElement::put_dataSrc(BSTR v)
{
#ifndef NO_DATABINDING
    HRESULT hr;

    // note that set-consumers and single-value consumers both support datasrc
    if (CDBindMethods::FDataSrcValid(this))
    {
        DetachDataBinding(ID_DBIND_DEFAULT);

        hr = s_propdescCElementdataSrc.b.SetStringProperty(
                v,
                this,
                (CVoid *)(void *)(GetAttrArray()) );

        AttachDataBindings();   // BUGBUG: better to specify ID_DBIND_DEFAULT
        Doc()->TickleDataBinding();
    }
    else
    {
        hr = SetErrorInfoPSet(DISP_E_MEMBERNOTFOUND, DISPID_CElement_dataSrc);
    }

    RRETURN(hr);
#else
        return DISP_E_MEMBERNOTFOUND;
#endif // ndef NO_DATABINDING
}

//+----------------------------------------------------------------------------
//
//  Function: FFilter
//
//  Synopsis: Determine if a given DBSPEC binding specification is consistent
//            with the rules for describing given type or types of bindings.
//
//  Arguments:
//            [dwFilter] - Some combination of at least one of
//                          DBIND_SETFILTER
//                          DBIND_CURRENTFILTER
//                          DBIND_TABLEFILTER
//            fHackyHierarchicalTableFlag - set to TRUE in calls on behalf of
//                          a TABLE.  To support hierarchy, TABLEs match
//                          dataSrc + dataFld to SETFILTER, not to CURRENTFILTER.
//
//  Returns:  TRUE - DBSPEC matches desired pattern
//            FALSE - DBSPEC doesn't match pattern
//
//-----------------------------------------------------------------------------
BOOL
DBSPEC::FFilter(DWORD dwFilter, BOOL fHackyHierarchicalTableFlag)
{
    BOOL fRet = FALSE;

    Assert(dwFilter & DBIND_ALLFILTER); // should specify at least one bit

    if (FormsIsEmptyString(_pStrDataFld))
    {
        if (!FormsIsEmptyString(_pStrDataSrc) && (dwFilter & DBIND_SETFILTER))
        {
            fRet = TRUE;
        }
    }
    else if (!FormsIsEmptyString(_pStrDataSrc))
    {
        if ((fHackyHierarchicalTableFlag && dwFilter & DBIND_SETFILTER) ||
            (!fHackyHierarchicalTableFlag && dwFilter & DBIND_CURRENTFILTER))
        {
            fRet = TRUE;
        }
    }
    else if (dwFilter & DBIND_TABLEFILTER)
    {
        fRet = TRUE;
    }

    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Function: FHTML
//
//  Synopsis: Determine if a given DBSPEC binding specifies HTML-based
//            Transfer.  Note that this method does NOT consider whether
//            or the bound element actually supports such transfer..
//
//  Returns:  TRUE - HTML binding specified
//            FALSE - HTML binding not specified
//
//-----------------------------------------------------------------------------
BOOL
DBSPEC::FHTML()
{
    return (!FormsIsEmptyString(_pStrDataFormatAs) && StrCmpIC(_pStrDataFormatAs, _T("html")) == 0);
}

//+----------------------------------------------------------------------------
//
//  Function: FLocalized
//
//  Synopsis: Determine if a given DBSPEC binding specifies localized-text
//            transfer.  Note that this method does NOT consider whether
//            or the bound element actually supports such transfer..
//
//  Returns:  TRUE - localized binding specified
//            FALSE - localized binding not specified
//
//-----------------------------------------------------------------------------
BOOL
DBSPEC::FLocalized()
{
    return (!FormsIsEmptyString(_pStrDataFormatAs) && StrCmpIC(_pStrDataFormatAs, _T("localized-text")) == 0);
}


//+----------------------------------------------------------------------------
//
//  Function: GetNextDBSpec, static
//
//  Synopsis: Enumerate next specified binding on a given element, whether or
//            not it is sensible.
//
//  Arguments:
//            [pElement] - element which is subject of this inquiry
//            [pid]      - in/out parameter; on input, contains last enumerated
//                         id, or id after which to start earch
//            [pdbs]     - output: where to put the specification
//            [dwFilter] - restrictions on search.  Some combination of at
//                         least one of
//                             DBIND_SETFILTER
//                             DBIND_CURRENTFILTER
//                             DBIND_TABLEFILTER
//                         optionally plus
//                             DBIND_ONEIDFILTER.
//                         This last flag indicates where only interested in
//                         id which is the input *pid+1.  This flag is a hack
//                         to allow trivial implementation of GetDBSpec.
//
//  Returns:  S_OK    - binding spec found matching input restrictions; *pid
//                      filled in with binding id, *pdbs filled in with binding
//                      spec.
//            S_FALSE - no matching binding spec found.  *pid undisturbed.
//                      *pdbs may have been trashed.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethods::GetNextDBSpec(CElement *pElem,
                            LONG *pid,
                            DBSPEC *pdbs,
                            DWORD dwFilter)
{
    HRESULT hr = S_FALSE;
#ifndef NO_DATABINDING
    LONG id = *pid;
    const CDBindMethods *pDBindMethods;

    Assert(dwFilter & DBIND_ALLFILTER); // at least one bit set?

    if (id == ID_DBIND_STARTLOOP)
    {
        id = ID_DBIND_DEFAULT;

        pdbs->_pStrDataFld = pElem->GetAAdataFld();
        pdbs->_pStrDataSrc = pElem->GetAAdataSrc();
        pdbs->_pStrDataFormatAs = pElem->GetAAdataFormatAs();

        if (pdbs->FFilter(dwFilter, pElem->Tag()==ETAG_TABLE))
        {
            hr = S_OK;
            goto Cleanup;
        }

        if (dwFilter & DBIND_ONEIDFILTER)
            goto Cleanup;
    }

    pDBindMethods = pElem->GetDBindMethods();

    if (pDBindMethods)
    {
        for (;;)
        {
            BOOL fFilter;

            if (pDBindMethods->GetNextDBSpecCustom(pElem, &id, pdbs))
                goto Cleanup;

            fFilter = pdbs->FFilter(dwFilter, pElem->Tag()==ETAG_TABLE);

            if (dwFilter & DBIND_ONEIDFILTER)   // only one value acceptable
            {
                if (fFilter && id == *pid + 1)
                {
                    hr = S_OK;
                }

                goto Cleanup;
            }

            if (fFilter)
            {
                hr = S_OK;
                goto Cleanup;
            }
        }
    }

Cleanup:
    if (!hr)
    {
        *pid = id;
    }
#endif // ndef NO_DATABINDING

    RRETURN1(hr, S_FALSE);
}



#ifndef NO_DATABINDING
//+-------------------------------------------------------------------------
// Member:      Get Binder (public)
//
// Returns:     my DataSourceBinder (or NULL, if none)

CDataSourceBinder *
DBMEMBERS::GetBinder(LONG id)
{
    CDataSourceBinder * pdsb = NULL;
    CDataSourceBinder ** ppdsb;
    int    i;

    Assert(id != ID_DBIND_ALL);

    for (i = _arydsb.Size(), ppdsb = _arydsb;
         i > 0;
         i--, ppdsb++)
    {
        if ((*ppdsb)->IdConsumer() == id)
        {
            pdsb = *ppdsb;
            break;
        }
    }

    return pdsb;
}


//+-------------------------------------------------------------------------
// Member:      MarkReadyToBind (public)
//
// Synopsis:    Mark all my binders "ready to bind"

void
DBMEMBERS::MarkReadyToBind()
{
    CDataSourceBinder **ppdsb;
    int i;

    for (i = _arydsb.Size(), ppdsb = _arydsb;
         i > 0;
         --i, ++ppdsb)
    {
        (*ppdsb)->SetReady(TRUE);
    }
}


//+-------------------------------------------------------------------------
// Member:      Ensure DataMemberManager (public)
//
// Synopsis:    Allocate a CDataMemberMgr, if not done already
//
// Arguments:   none
//
// Returns:     HRESULT

HRESULT
DBMEMBERS::EnsureDataMemberManager(CElement *pElement)
{
    HRESULT hr = S_OK;

    if (!_pDMembMgr)
    {
        hr = CDataMemberMgr::Create(pElement, &_pDMembMgr);

        pElement->Doc()->_fBroadcastStop = TRUE;
    }

    return(hr);
}


//+-------------------------------------------------------------------------
// Member:      Release DataMemberManager (public)
//
// Synopsis:    Deallocate the CDataMemberMgr, if not done already

void
DBMEMBERS::ReleaseDataMemberManager(CElement *pElement)
{
    if (_pDMembMgr)
    {
        _pDMembMgr->Detach();
        _pDMembMgr->Release();
        _pDMembMgr = NULL;
    }
}


//+-------------------------------------------------------------------------
// Member:      Ensure DBMembers (private)
//
// Synopsis:    Allocate a DBMEMBERS struct, if not done already
//
// Arguments:   none
//
// Returns:     HRESULT

HRESULT
CElement::EnsureDBMembers()
{
    if (HasDataBindPtr())
    {
        return(S_OK);
    }

    DBMEMBERS * pdbm = new DBMEMBERS;

    if (!pdbm)
    {
        return(E_OUTOFMEMORY);
    }

    HRESULT hr = SetDataBindPtr(pdbm);

    if (hr)
    {
        delete pdbm;
    }

    return(hr);
}


//+-------------------------------------------------------------------------
// Member:      Ensure DataMemberManager (public)
//
// Synopsis:    Allocate a CDataMemberMgr, if not done already
//
// Arguments:   none
//
// Returns:     HRESULT

HRESULT
CElement::EnsureDataMemberManager()
{
    HRESULT hr = EnsureDBMembers();

    if (!hr)
    {
        hr = GetDBMembers()->EnsureDataMemberManager(this);
    }

    return(hr);
}


//+-------------------------------------------------------------------------
// Member:      Get DataMemberManager (public)
//
// Synopsis:    Return my datamember manager
//
// Arguments:   none
//
// Returns:     HRESULT

CDataMemberMgr *
CElement::GetDataMemberManager()
{
    CDataMemberMgr *pdmm;

    if (HasDataBindPtr())
    {
        pdmm = GetDBMembers()->GetDataMemberManager();
    }
    else
    {
        pdmm = NULL;
    }
    
    return pdmm;
}


//+-------------------------------------------------------------------------
// Member:      IsDataProvider (public)
//
// Synopsis:    Decide whether I'm a data provider

BOOL
CElement::IsDataProvider()
{
    CDataMemberMgr *pdmm;
    
    EnsureDataMemberManager();
    pdmm = GetDataMemberManager();

    return pdmm && pdmm->IsDataProvider();
}


//+-------------------------------------------------------------------------
// Member:      CreateDatabindRequest (public)
//
// Synopsis:    Create a request to hookup my databinding, and register it
//              with the databinding task
//
// Arguments:   id      id of the binding
//              pdbs    DBSPEC for the binding
//
// Returns:     HRESULT

HRESULT
CElement::CreateDatabindRequest(LONG id, DBSPEC *pdbs /* NULL */)
{
    HRESULT hr = S_OK;
    DBMEMBERS *pdbm;
    DBSPEC dbs;

    // get the DBSPEC, if not already provided
    if (pdbs == NULL)
    {
        pdbs = &dbs;
        hr = CDBindMethods::GetDBSpec(this, id, pdbs);
        if (hr)
            goto Cleanup;
    }
    
    // we need DBMEMBERS allocated, so that we can hook things up.
    hr = EnsureDBMembers();
    if (hr)
        goto Cleanup;

    pdbm = GetDBMembers();

    if (!pdbm->GetBinder(id))
    {
        CDataSourceBinder *pdsbBinder;

        // get a binder object
        pdsbBinder = new CDataSourceBinder(this, id);
        if (!pdsbBinder)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // save it in my DBMEMBERS
        hr = pdbm->SetBinder(pdsbBinder);
        if (hr)
        {
            pdsbBinder->Passivate();
            goto Cleanup;
        }

        // tell binder to register itself with the databinding task
        pdsbBinder->SetReady(CDBindMethods::IsReadyToBind(this));
        hr = pdsbBinder->Register(pdbs);
        if (hr)
        {
            DetachDataBinding(id);
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     AttachDataBindings, public
//
//  Synopsis:   Make sure that all binders needed are present for the given
//              element.  (Note this doesn't actually handle state of
//              the CDataBindingEvents; other calls must be made to get
//              this set up.)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CElement::AttachDataBindings()
{
    HRESULT hrRet = S_OK;
    HRESULT hr = S_OK;
    CDoc *pDoc = Doc();
    LONG id;
    DBSPEC dbs;
    CElement *pElementOuter, *pElementRepeat;
    LPCTSTR strTail;

    // don't databind in design mode or outside the main tree
    if (pDoc==NULL || pDoc->_fDesignMode || !IsInPrimaryMarkup() ||
            !CDBindMethods::IsReadyToBind(this))
        goto Cleanup;

    // note post-operation of for-loop; hrRet is set to first error
    for (id = ID_DBIND_STARTLOOP;
         !CDBindMethods::GetNextDBSpec(this, &id, &dbs, DBIND_ALLFILTER);
         !hrRet && (hrRet = hr) != 0)
    {
        // see if I'm current-record or table-bound
        hr = FindDatabindContext(dbs._pStrDataSrc, dbs._pStrDataFld,
                                &pElementOuter, &pElementRepeat, &strTail);

        // I'm table-bound, but table isn't expanding yet (I'm in a template).
        // Ignore for now.  Wait until table expands
        if (hr == S_FALSE)
        {
            hr = S_OK;
            continue;
        }

        // something bad happened.  Punt.
        if (hr)
            continue;
        
        // table-bound - ask the table to hook me up
        if (pElementOuter)
        {
            CTable *pTable = DYNCAST(CTable, pElementOuter);
            CTableRow *pRow = DYNCAST(CTableRow, pElementRepeat);

            hr = pTable->AddDataboundElement(this, id, pRow, strTail);
        }

        // current-record-bound - ask the databind task to hook me up
        else
        {
            hr = CreateDatabindRequest(id, &dbs);
        }
    }

Cleanup:
    RRETURN(hrRet);
}

//+---------------------------------------------------------------------------
//
//  Member:     DetachDataBindings, public
//
//  Synopsis:   Let go of all of my databinding data, if any
//
//  Returns:    nothing
//
//----------------------------------------------------------------------------

void
CElement::DetachDataBindings()
{
    DBMEMBERS *pdbm = GetDataBindPtr();

    if (pdbm != NULL)
    {
        int i;

        pdbm->DetachBinding(this, ID_DBIND_ALL);

        // this loop is a little different from the standard FormsAry loop.
        //  It keeps i and ppsdb consistent in the body of the loop.
        //  It also keeps deletions at the end of the array, minimizing
        //  foolish copying, and keeping the deletions from interfering with
        //  the loop.
        for (i = pdbm->_arydsb.Size(); i-- > 0; )
        {
            if (pdbm->_arydsb[i])
            {
                (pdbm->_arydsb[i])->Passivate();
            }
            pdbm->_arydsb.Delete(i);
        }

        pdbm->ReleaseDataMemberManager(this);

        delete pdbm;

        // Can't use this call above because the detach code expects to
        // be able to call GetDataBindPtr() and find it.

        DelDataBindPtr();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     DetachDataBinding, public
//
//  Synopsis:   Let go of all databinding state, if any, for one binding id
//
//  Returns:    nothing
//
//----------------------------------------------------------------------------

void
CElement::DetachDataBinding(LONG id)
{
    DBMEMBERS *pdbm = GetDBMembers();

    Assert(id != ID_DBIND_ALL);

    if (pdbm != NULL)
    {
        CDataSourceBinder *pdsb = pdbm->GetBinder(id);

        // Call DetachBinding before pdsb->Passivate, so that the
        // element's bound value is replaced with a null-equivalent
        pdbm->DetachBinding(this, id);

        if (pdsb)
        {
            pdsb->Passivate();
            pdbm->_arydsb.DeleteByValue(pdsb);
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     FindDatabindContext, public
//
//  Synopsis:   find the enclosing repeated element with matching dataSrc
//              and dataFld attributes
//
//  Arguments:  strDataSrc  look for an element with this dataSrc attribute
//              strDataFld     ... and this dataFld attribute
//              ppElementOuter   container element (NULL, if I'm current-row bound)
//              ppElementRepeat  instance of repeating that contains me
//              pstrTail    tail of strDataFld, after stripping away prefix
//                          that matches the container [return]
//
//  Returns:    S_OK        found it, or I'm bound to current record
//              S_FALSE     found a syntactically enclosing element, but it's not
//                          repeating, *ppInstance will be NULL

HRESULT
CElement::FindDatabindContext(LPCTSTR strDataSrc, LPCTSTR strDataFld,
                            CElement **ppElementOuter, CElement **ppElementRepeat,
                            LPCTSTR *pstrTail)
{
    HRESULT hr = S_OK;
    CTreeNode *pNodeRow;
    CTreeNode *pNodeOuter = NULL;
    CTable *pTableOuter = NULL;
    DBSPEC dbsOuter;
    LPCTSTR strTail = NULL;

    *ppElementOuter = NULL;
    *ppElementRepeat = NULL;
    
    // search up the tree for a row in a table with the right spec
    for (   pNodeRow = GetFirstBranch()->Ancestor(ETAG_TR);
            pNodeRow;
            pNodeRow = pNodeOuter->Ancestor(ETAG_TR) )
    {
        pNodeOuter = pNodeRow->Ancestor(ETAG_TABLE);
        Assert(pNodeOuter && "TableRow not enclosed in Table");
        pTableOuter = DYNCAST(CTable, pNodeOuter->Element());

        // if outer table isn't bound, keep looking
        if (S_OK != CDBindMethods::GetDBSpec(pTableOuter, ID_DBIND_DEFAULT, &dbsOuter))
            continue;

        // if it's not repeating yet, I'm in a template - there's no instance
        if (!pTableOuter->IsRepeating())
        {
            *ppElementOuter = pTableOuter;
            hr = S_FALSE;
            goto Cleanup;
        }

        // if datasource attributes don't match, keep looking
        if (!FormsIsEmptyString(strDataSrc) &&
            FormsStringCmp(strDataSrc, pTableOuter->GetDataSrc()) != 0)
            continue;

        // if datafield attributes don't match, keep looking
        strTail = FindOverlapAndTail(pTableOuter->GetDataFld(), strDataFld);
        if (!pTableOuter->IsFieldKnown(strTail))
            continue;

        // I'm a table-bound element
        *ppElementOuter = pTableOuter;
        *ppElementRepeat = DYNCAST(CTableRow, pNodeRow->Element());
        goto Cleanup;
    }

    // if we fall out of the loop, I must be a current-row bound element
    strTail = strDataFld;
    
Cleanup:
    if (pstrTail)
        *pstrTail = strTail;
    RRETURN1(hr, S_FALSE);
}
#endif // ndef NO_DATABINDING


//+---------------------------------------------------------------------------
//
//  Member:     CElement::getRecordNumber
//
//  Synopsis:   returns the record number of this element if it's in a
//              databound table; otherwise returns -1
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CElement::get_recordNumber(VARIANT *retval)
{
    V_VT(retval) = VT_NULL;

    CTreeNode * pNode = GetFirstBranch();
    while (pNode)
    {
        CTreeNode * pNodeRow = NULL;

        while (!pNodeRow && pNode)
        {
            if (pNode->Tag() == ETAG_TR)
                pNodeRow = pNode;
            else
                pNode = pNode->Parent();
        }

        if (pNodeRow)
        {
            CTableLayout *pTableLayout = NULL;

            do
            {
                pNode = pNode->Parent();
                if (pNode && (pNode->Tag() == ETAG_TABLE))
                    pTableLayout = DYNCAST(CTable, pNode->Element())->Layout();
            }
            while (!pTableLayout && pNode);

            if (!pTableLayout)
                goto Cleanup;       // BUGBUG: Handle this better.  This case is valid through the DOM

            Assert(pTableLayout);

#ifndef NO_DATABINDING
            if (pTableLayout->IsRepeating())
            {
                int iRow = DYNCAST(CTableRowLayout, pNodeRow->GetCurLayout())->RowPosition();
                if (pTableLayout->IsGenerated(iRow))
                {
                    V_VT(retval) = VT_I4;
                    V_I4(retval) = pTableLayout->RowIndex2RecordNumber(iRow);
                    break;
                }
            }
#endif
        }
    }
Cleanup:
    RRETURN(SetErrorInfo(S_OK));
}

#ifndef NO_DATABINDING
//+----------------------------------------------------------------------------
//
//  Class CDBindMethods
//
//-----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
//  Function: FDataSrcValid, static
//
//  Synopsis: Indicates whether or not the object model for the given element
//            should datasrc as a valid property.
//
//-----------------------------------------------------------------------------
BOOL
CDBindMethods::FDataSrcValid(CElement *pElem)
{
    const CDBindMethods *pDBindMethods = pElem->GetDBindMethods();

    return (pDBindMethods && pDBindMethods->FDataSrcValidImpl(pElem));
}

//+----------------------------------------------------------------------------
//
//  Function: FDataSrcValidImpl, virtual (default implementation)
//
//  Synopsis: Indicates whether or not the object model for the given element
//            should datasrc as a valid property.
//
//-----------------------------------------------------------------------------
BOOL
CDBindMethods::FDataSrcValidImpl(CElement *pElem) const
{
    return (DBindKind(pElem, ID_DBIND_DEFAULT) != DBIND_NONE);
}

//+----------------------------------------------------------------------------
//
//  Function: FDataFldValid, static
//
//  Synopsis: Indicates whether or not the object model for the given element
//            should datafld as a valid property.
//
//-----------------------------------------------------------------------------
BOOL
CDBindMethods::FDataFldValid(CElement *pElem)
{
    const CDBindMethods *pDBindMethods = pElem->GetDBindMethods();

    return (pDBindMethods && pDBindMethods->FDataFldValidImpl(pElem));
}

//+----------------------------------------------------------------------------
//
//  Function: FDataFldValidImpl, virtual (default implementation)
//
//  Synopsis: Indicates whether or not the object model for the given element
//            should datasrc as a valid property.
//
//-----------------------------------------------------------------------------
BOOL
CDBindMethods::FDataFldValidImpl(CElement *pElem) const
{
    DBIND_KIND dbk = DBindKind(pElem, ID_DBIND_DEFAULT);
    return (dbk == DBIND_SINGLEVALUE) || (dbk == DBIND_TABLE);
}

//+----------------------------------------------------------------------------
//
//  Function: FDataFormatAsValid, static
//
//  Synopsis: Indicates whether or not the object model for the given element
//            should dataformatas as a valid property.
//
//-----------------------------------------------------------------------------
BOOL
CDBindMethods::FDataFormatAsValid(CElement *pElem)
{
    const CDBindMethods *pDBindMethods = pElem->GetDBindMethods();

    return (pDBindMethods && pDBindMethods->FDataFormatAsImpl());
}

//+----------------------------------------------------------------------------
//
//  Function: DBindKind, static
//
//  Synopsis: Indicate whether or not <element, id> can be databound, and
//            optionally return additional info about binding.
//
//            Wraps calls to CDBindMethods::DBindKindImpl, and handles
//            initialization of any non-NULL pdbi passed in.
//
//  Arguments:
//            [pElem] - element whose binding status/ability is being queried
//            [id]    - id of binding specification for which info is desired
//            [pdbi]  - pointer to struct to get data type and ID;
//                      may be NULL
//
//  Returns:  Binding status: one of
//              DBIND_NONE
//              DBIND_SINGLEVALUE
//              DBIND_ICURSOR
//              DBIND_IROWSET
//              DBIND_TABLE
//              DBIND_IDATASOURCE
//            For base CElement, always returns DBIND_NONE
//
//-----------------------------------------------------------------------------
DBIND_KIND
CDBindMethods::DBindKind(CElement *pElem, LONG id, DBINFO *pdbi)
{
    DBIND_KIND dbk = DBIND_NONE;
    const CDBindMethods *pDBindMethods;

    if (pdbi)
    {
        pdbi->_vt = VT_EMPTY;
        pdbi->_dwTransfer = DBIND_ONEWAY; // one-way, no HTML
    }

    pDBindMethods = pElem->GetDBindMethods();

    if (pDBindMethods)
    {
        dbk = pDBindMethods->DBindKindImpl(pElem, id, pdbi);
    }

    return dbk;
}

//+----------------------------------------------------------------------------
//
//  Function: IsReadyToBind, static
//
//  Synopsis: Indicate whether the element is ready to be bound.  Most elements
//              are, except for OBJECT/APPLET (depends on readystate) and TABLE
//              (have to see the </TABLE>, so we have all the template rows).
//
//            Wraps calls to CDBindMethods::IsReadyImpl.
//
//  Arguments:
//            [pElem] - element whose binding readiness is being queried
//-----------------------------------------------------------------------------
BOOL
CDBindMethods::IsReadyToBind(CElement *pElem)
{
    BOOL fIsReady = FALSE;
    const CDBindMethods *pDBindMethods;

    pDBindMethods = pElem->GetDBindMethods();

    if (pDBindMethods)
    {
        fIsReady = pDBindMethods->IsReadyImpl(pElem);
    }

    return fIsReady;
}

//+----------------------------------------------------------------------------
//
//  Class CDBindMethodsSimple:  support for simple controls with single binding
//
//-----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
//  Function: DBindKindImpl, CDBindMethodsSimple
//
//  Synopsis: Simple elements with only a single binding respond to DBindKind in
//              more or less the same way.  Handled here.
//  Arguments:
//            [pdbi]  - pointer to struct to get data type and ID;
//                      may be NULL.  If not NULL, has already been pre-
//                      initialized to state appropriate for DBIND_NONE.
//
//
//  Returns:  as in DBindKind
//
//-----------------------------------------------------------------------------
DBIND_KIND
CDBindMethodsSimple::DBindKindImpl(CElement *pElem,
                                   LONG id,
                                   DBINFO *pdbi) const
{
    DBIND_KIND dbk = DBIND_NONE;

    if (id == ID_DBIND_DEFAULT)
    {
        dbk = DBIND_SINGLEVALUE;

        if (pdbi)
        {
            pdbi->_vt = _vt;
            pdbi->_dwTransfer = _dwTransfer;
        }
    }

    return dbk;
}


//+----------------------------------------------------------------------------
//
//  Function: BoundValueFromElement, CDBindMethods
//
//  Synopsis: For the convenience of classes which support only 1-way bindings,
//            we provide a base implementation of BoundValueFromElement, which
//            satisfies compiler but should never be called.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsSimple::BoundValueFromElement(CElement *,
                                         LONG,
                                         BOOL,
                                         LPVOID) const
{
    Assert(FALSE);
    return E_UNEXPECTED;
}

//+----------------------------------------------------------------------------
//
//  Class CDBindMethodsText: for elements which use the text code to get and
//  set strings.
//
//-----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Default implemenation for transferring bound data to element.
//            Note that this won't be called unless some implementation of
//            BindStatus indicated a binding was present.  Assumes that we
//            are working with a textual element.
//
//  Arguments:
//            [pElem]   - destination element of transfer
//            [id]      - ID of binding point.  For the text-based
//                        must be ID_DBIND_DEFAULT.
//            [fHTML]   - whether text should be interpret as HTML
//            [pvData]  - pointer to data to transfer, in the expected data
//                        type.  For text-based transfers, must be BSTR.
//
//-----------------------------------------------------------------------------

HRESULT
CDBindMethodsText::BoundValueToElement ( CElement *pElem,
                                         LONG id,
                                         BOOL fHTML,
                                         LPVOID pvData ) const
{
    BSTR *  pBstr = (BSTR *) pvData;

    Assert( pBstr );

#if DBG==1
    {
        DBINFO dbi;

        Assert( id == ID_DBIND_DEFAULT );
        Assert( DBindKind( pElem, id, & dbi ) > DBIND_NONE );
        Assert( dbi._vt == VT_BSTR );
    }
#endif // DBG == 1

    RRETURN(
        THR(
            pElem->Inject(
                CElement::Inside, fHTML,
                *pBstr, FormsStringLen( * pBstr ) ) ) );
}



HRESULT
CDBindMethodsText::BoundValueFromElement(CElement *pElem,
                                         LONG id,
                                         BOOL fHTML,
                                         LPVOID pvData) const
{
    BSTR       *pBstr = (BSTR *) pvData;

    Assert(pBstr);
    // shouldn't be called for one-way bindings
    Assert((_dwTransfer & DBIND_ONEWAY) == 0);

#if DBG==1
    {
        DBINFO dbi;

        Assert(id == ID_DBIND_DEFAULT);
        Assert(DBindKind(pElem, id, &dbi) > DBIND_NONE);
        Assert(dbi._vt == VT_BSTR);
    }
#endif // DBG == 1

    RRETURN(pElem->GetBstrFromElement(fHTML, pBstr));
}

#endif // ndef NO_DATABINDING
