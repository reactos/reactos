//+---------------------------------------------------------------------
//
//   File:      element3.cxx
//
//  Contents:   Element class related member functions
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif


#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_TCELL_HXX_
#define X_TCELL_HXX_
#include "tcell.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_IEXTAG_HXX_
#define X_IEXTAG_HXX_
#include "iextag.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

MtDefine(CElement_pAccels, PerProcess, "CElement::_pAccels")
MtDefine(CElementGetNextSubDivision_pTabs, Locals, "CElement::GetNextSubdivision pTabs")

ExternTag(tagMsoCommandTarget);

extern DWORD GetBorderInfoHelper(CTreeNode * pNode,
                                CDocInfo * pdci,
                                CBorderInfo * pbi,
                                BOOL fAll);
extern HRESULT CreateImgDataObject(
                 CDoc            * pDoc,
                 CImgCtx         * pImgCtx,
                 CBitsCtx        * pBitsCtx,
                 CElement        * pElement,
                 CGenDataObject ** ppImgDO);

extern IActiveIMMApp * GetActiveIMM();
extern HIMC ImmGetContextDIMM(HWND hWnd);

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetBorderInfo
//
//  Synopsis:   get the elements border information.
//
//  Arguments:  pdci        - Current CDocInfo
//              pborderinfo - pointer to return the border information
//              fAll        - (FALSE by default) return all border related
//                            related information (border style's etc.),
//                            if TRUE
//
//  Returns:    0 - if no borders
//              1 - if simple border (all sides present, all the same size)
//              2 - if complex border (present, but not simple)
//
//----------------------------------------------------------------------------
DWORD
CElement::GetBorderInfo(
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fAll)
{
    return GetBorderInfoHelper(GetFirstBranch(), pdci, pborderinfo, fAll);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetFirstCp
//
//  Synopsis:   Get the first character position of this element in the text
//              flow. (relative to the Markup)
//
//  Returns:    LONG        - start character position in the flow. -1 if the
//                            element is not found in the tree
//
//----------------------------------------------------------------------------
long
CElement::GetFirstCp()
{
    CTreePos *  ptpStart;

    GetTreeExtent( &ptpStart, NULL );

    return ptpStart ? ptpStart->GetCp() + 1 : -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetLastCp
//
//  Synopsis:   Get the last character position of this element in the text
//              flow. (relative to the Markup)
//
//  Returns:    LONG        - end character position in the flow. -1 if the
//                            element is not found in the tree
//
//----------------------------------------------------------------------------
long
CElement::GetLastCp()
{
    CTreePos *  ptpEnd;

    GetTreeExtent( NULL, &ptpEnd );

    return ptpEnd ? ptpEnd->GetCp() : -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetFirstAndLastCp
//
//  Synopsis:   Get the first and last character position of this element in
//              the text flow. (relative to the Markup)
//
//  Returns:    no of characters influenced by the element, 0 if the element
//              is not found in the tree.
//
//----------------------------------------------------------------------------
LONG
CElement::GetFirstAndLastCp(long * pcpFirst, long * pcpLast)
{
    CTreePos *  ptpStart, *ptpLast;
    long        cpFirst, cpLast;

    Assert (pcpFirst || pcpLast);

    if (!pcpFirst)
        pcpFirst = &cpFirst;

    if (!pcpLast)
        pcpLast = &cpLast;

    GetTreeExtent( &ptpStart, &ptpLast );

    Assert( (ptpStart && ptpLast) || (!ptpStart && !ptpLast) );

    if( ptpStart )
    {
        *pcpFirst = ptpStart->GetCp() + 1;
        *pcpLast = ptpLast->GetCp();
    }
    else
    {
        *pcpFirst = *pcpLast = 0;
    }

    Assert( *pcpLast - *pcpFirst >= 0 );

    return *pcpLast - *pcpFirst;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::TakeCapture
//
//  Synopsis:   Set the element with the mouse capture.
//
//----------------------------------------------------------------------------

void
CElement::TakeCapture(BOOL fTake)
{
    CDoc * pDoc = Doc();

    if (fTake)
    {
        pDoc->SetMouseCapture(
                MOUSECAPTURE_METHOD(CElement, HandleCaptureMessage,
                                           handlecapturemessage),
                (void *)this,
                TRUE);
    }
    // CHROME
    // If Chrome hosted need to route the call to determine if we have capture
    // through the containter's windowless interface rather than using Win32
    // and relying on HWNDs
    else if (pDoc->GetCaptureObject() == this &&
            (!pDoc->IsChromeHosted() ? (::GetCapture() == pDoc->_pInPlace->_hwnd) : pDoc->GetCapture()))
    {
        pDoc->ClearMouseCapture((void *)this);
        if (!pDoc->_pElementOMCapture)
        {
#if DBG==1
            Assert(!TLS(fHandleCaptureChanged));
#endif DBG==1

            // CHROME
            // If Chrome hosted then release capture using the containter's windowless
            // interface rather than using Win32's ::ReleaseCapture.
            if (!pDoc->IsChromeHosted())
                ::ReleaseCapture();
            else
                pDoc->SetCapture(FALSE);
        }
    }
}

BOOL
CElement::HasCapture()
{
    CDoc * pDoc = Doc();

    // CHROME
    // If Chrome hosted use the container's windowless interface to test
    // for capture (as we are windowless) rather than using Win32's GetCapture
    // and HWNDS
    return ((pDoc->GetCaptureObject() == (void *)this) &&
        (!pDoc->IsChromeHosted() ? (::GetCapture() == pDoc->_pInPlace->_hwnd) : pDoc->GetCapture()));
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// The following seciton has the persistence support routines.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+------------------------------------------------------------------------
//
//  Member:     GetPersistID
//
//  Synopsis:   Helper function to return the ID that is used for matching up
//          persistence data to this element.  By default this is the nested ID of the
//          frame, but if no ID is specified, then we construct one using the
//          nested position of the frame in the name space of its parent.
//
//-------------------------------------------------------------------------

BSTR
CElement::GetPersistID (BSTR bstrParentName)
{
    TCHAR       ach[MAX_PERSIST_ID_LENGTH];
    LPCTSTR     pstrID;
    BOOL        fNeedToFree = FALSE;
    BSTR        bstrRealName;
    CElement  * pParent = NULL;
    CLayout   * pParentLayout = GetUpdatedParentLayout();

    // if we don't have a recursive name construction available,
    // so create the name the hard way... since we know our name
    // on our doc, we need to know our the name our doc should
    // prepend.

    if (pParentLayout && pParentLayout->Tag() == ETAG_FRAMESET)
    {
        pParent = pParentLayout->ElementOwner();
    }

    if (pParent)
    {
        fNeedToFree = TRUE;
        bstrRealName = pParent->GetPersistID(bstrParentName);  // in a framset
    }
    else
    {
        if (!bstrParentName)
        {
            fNeedToFree = TRUE;
            bstrRealName = Doc()->GetPersistID();
        }
        else
            bstrRealName = bstrParentName;
    }

    // Now get this ID and with the name base, concatenate and leave.
    pstrID = GetAAid();
    if (!pstrID)
    {
        IGNORE_HR(Format(0, ach, ARRAY_SIZE(ach), _T("<0s>#<1d>"),
                                                  bstrRealName,
                                                  GetSourceIndex()));
    }
    else
    {
        IGNORE_HR(Format(0, ach, ARRAY_SIZE(ach), _T("<0s>_<1s>"),
                                                  bstrRealName,
                                                  pstrID));
    }

    if (fNeedToFree)
        SysFreeString(bstrRealName);

    return SysAllocString(ach);
}
//+-----------------------------------------------------------------------------
//
//  Member : GetPeerPersist
//
//  Synopsis : this hepler function consolidates the test code that gets the
//      IHTmlPersistData interface from a peer, if there is one.
//      this is used in numerous ::Notify routines.
//+-----------------------------------------------------------------------------
IHTMLPersistData *
CElement::GetPeerPersist()
{
    IHTMLPersistData * pIPersist = NULL;

    if (HasPeerHolder())
    {
        IGNORE_HR(GetPeerHolder()->QueryPeerInterface(IID_IHTMLPersistData,
                                                        (void **)&pIPersist));
    }

    return pIPersist;
}
//+-----------------------------------------------------------------------------
//
//  Member : GetPersistenceCache
//
//  Synopsis : Creates and returns the XML DOC.
//      BUGBUG (carled) this needs to check the registry for a pluggable XML store
//
//------------------------------------------------------------------------------
HRESULT
CElement::GetPersistenceCache( IXMLDOMDocument **ppXMLDoc )
{
    HRESULT         hr = S_OK;
    IObjectSafety * pObjSafe = NULL;

    Assert(ppXMLDoc);
    *ppXMLDoc = NULL;

    // 3efaa428-272f-11d2-836f-0000f87a7782
    hr = THR(CoCreateInstance(CLSID_DOMDocument,
                              0,
                              CLSCTX_INPROC_SERVER,
                              IID_IXMLDOMDocument,
                              (void **)ppXMLDoc));
    if (hr)
        goto ErrorCase;

    hr = (*ppXMLDoc)->QueryInterface(IID_IObjectSafety,
                                     (void **)&pObjSafe);
    if (hr)
        goto ErrorCase;

    hr = pObjSafe->SetInterfaceSafetyOptions( IID_NULL,
                                              INTERFACE_USES_SECURITY_MANAGER,
                                              INTERFACE_USES_SECURITY_MANAGER);
    if (hr)
        goto ErrorCase;

Cleanup:
    ReleaseInterface(pObjSafe);
    RRETURN( hr );

ErrorCase:
    delete *ppXMLDoc;
    *ppXMLDoc = NULL;
    goto Cleanup;
}

//+-----------------------------------------------------------------------------
//
//  Member : TryPeerSnapshotSave
//
//  synopsis : notification helper
//
//+-----------------------------------------------------------------------------
HRESULT
CElement::TryPeerSnapshotSave (IUnknown * pDesignDoc)
{
    // check to see if we are a persistence XTag
    HRESULT hr = S_OK;
    IHTMLPersistData * pIPersist = GetPeerPersist();

    // Better be true
    Assert ( Doc()->_pPrimaryMarkup->MetaPersistEnabled((long)htmlPersistStateSnapshot) );

    if (pIPersist)
    {
        VARIANT_BOOL       fContinue;

        hr = THR(pIPersist->save(pDesignDoc,
                                 htmlPersistStateSnapshot,
                                 &fContinue));
    }

    ReleaseInterface(pIPersist);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member : TryPeerPersist
//
//  synopsis : notification helper
//
//+-----------------------------------------------------------------------------

HRESULT
CElement::TryPeerPersist(PERSIST_TYPE sn, void * pvNotify)
{
    // check to see if we are a persistence XTag
    HRESULT hr = S_OK;
    IHTMLPersistData * pIPersist = GetPeerPersist();

    if (pIPersist)
    {
        VARIANT_BOOL       fSupported = VB_FALSE;
        htmlPersistState   hps = (sn==XTAG_HISTORY_SAVE ||
                                  sn==XTAG_HISTORY_LOAD) ? htmlPersistStateHistory:
                                                          htmlPersistStateFavorite;

        // Better be true
        Assert( Doc()->_pPrimaryMarkup->MetaPersistEnabled((long)hps));

        // one last check, before going through the effort of calling:
        //  Is this particular persist Tag, appropriate for this event
        hr = THR(pIPersist->queryType((long)hps, &fSupported));
        if (!hr && fSupported == VB_TRUE)
        {
            // this can return S_FALSE to stop the propogation of
            // Notify.
            switch (sn)
            {
            case XTAG_HISTORY_SAVE:
                hr = THR(DoPersistHistorySave(pIPersist, pvNotify));
                break;

            case XTAG_HISTORY_LOAD:
                hr = THR(DoPersistHistoryLoad(pIPersist, pvNotify));
                break;

            case FAVORITES_LOAD:
            case FAVORITES_SAVE:
                hr = THR(DoPersistFavorite(pIPersist, pvNotify, sn));
                break;
            }
        }

        ReleaseInterface(pIPersist);
    }

    RRETURN1( hr, S_FALSE );
}

//+-----------------------------------------------------------------------------
//
// Member : DoPersistHistorySave
//
//  Synopsis: helper function for saving this element's/XTags's data into the
//      history stream.  Save is an odd case, since EACH element can have the
//      authordata from the Peer, as well as its own state info (e.g. checkboxs)
//      Most of our work is related to managing the two forms of information, and
//      getting it back to the correct place.
//
//------------------------------------------------------------------------------

HRESULT
CElement::DoPersistHistorySave(IHTMLPersistData *pIPersist,
                               void *            pvNotify)
{
    HRESULT              hr = S_OK;
    IXMLDOMDocument    * pXMLDoc = NULL;
    IUnknown           * pUnk       = NULL;
    VARIANT_BOOL         fContinue  = VB_TRUE;
    VARIANT_BOOL         vtbCleanThisUp;
    CDoc *               pDoc = Doc();

    if (!pIPersist)
        return E_INVALIDARG;

    // get the XML cache for the history's userdata block
    if (!pDoc->_pXMLHistoryUserData)
    {
        BSTR bstrXML;

        // we haven't needed one yet so create one...
        bstrXML = SysAllocString(_T("<ROOTSTUB />"));
        if (!bstrXML)
            return  E_OUTOFMEMORY;

        // get the xml chache and initialize it.
        hr = GetPersistenceCache(&pXMLDoc);
        if (!hr)
        {
            hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
        }
        SysFreeString(bstrXML);
        if (hr)
            goto Cleanup;

        pDoc->_pXMLHistoryUserData = pXMLDoc;
    }
    else
    {
        pXMLDoc = pDoc->_pXMLHistoryUserData;
    }

    Assert(pXMLDoc);

    hr = pXMLDoc->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;

     // call the XTAG and let it save  any userdata
     hr = pIPersist->save(pUnk, htmlPersistStateHistory, &fContinue);
     if (hr)
        goto Cleanup;

    // now in the std History stream give the element a chance
    // to do all its normal saveing. Do this by turning around
    // and giving the element the SN_SAVEHISTORY, this will catch
    // all scope elemnts too. cool.
    // although history does an outerHTML this is really necesary
    //  for object tags to get caught properly.
    if (SUCCEEDED(hr) &&
        fContinue==VB_TRUE &&
        NeedsLayout())
    {
        CNotification   nf;
        nf.SaveHistory1(this, pvNotify);

        // fire against ourself.
        Notify(&nf);

        if (nf.IsSecondChanceRequested())
        {
            nf.SaveHistory2(this, pvNotify);
            Notify(&nf);
        }
    }

    hr = (fContinue==VB_TRUE) ? hr : S_FALSE;

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN1( hr, S_FALSE );
}

//+-------------------------------------------------------------
//
// Member : DoPesistHistoryLoad
//
//  synposis: notification helper function, undoes what DoPersistHistory Save
//      started
//+-------------------------------------------------------------

HRESULT
CElement::DoPersistHistoryLoad(IHTMLPersistData *pIPersist,
                               void *            pvNotify)
{
    HRESULT             hr = E_INVALIDARG;
    IUnknown          * pUnk     = NULL;
    VARIANT_BOOL        fContinue = VB_TRUE;
    VARIANT_BOOL        vtbCleanThisUp;
    IXMLDOMDocument   * pXMLDoc  = NULL;
    CDoc *              pDoc = Doc();
    CLock               lock(this);

    if (!pIPersist)
        goto Cleanup;

    // if we are loading the the doc's XML History is NULL,
    //  then this is the first call. so create it and
    //  intialize it from the string.
    if (!pDoc->_pXMLHistoryUserData &&
         pDoc->_cstrHistoryUserData &&
         pDoc->_cstrHistoryUserData.Length())
    {
        BSTR bstrXML;

        if (FAILED(THR(GetPersistenceCache(&pXMLDoc))))
            goto Cleanup;

        hr = pDoc->_cstrHistoryUserData.AllocBSTR( &bstrXML );
        if (SUCCEEDED( hr) )
        {   
            hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
            if (SUCCEEDED(hr))
            {
                pDoc->_pXMLHistoryUserData = pXMLDoc;
                pXMLDoc->AddRef();
                hr = S_OK;
            }
            SysFreeString(bstrXML);
        }
    }
    else if (pDoc->_pXMLHistoryUserData)
    {
        pXMLDoc = pDoc->_pXMLHistoryUserData;
        pXMLDoc->AddRef();
        hr = S_OK;
    }

    if (!pXMLDoc)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // if for some reason the cache is not yet init'd. do the default
    //---------------------------------------------------------------
    if (hr)
    {
        BSTR bstrXML;

        bstrXML = SysAllocString(_T("<ROOTSTUB />"));
        if (!bstrXML)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
        SysFreeString(bstrXML);
        if (hr)
            goto Cleanup;
    }

    hr = pXMLDoc->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;

    // now that the xml object is initialized properly... call the XTag
    //  give the persist data xtag the chance to load its own information
    hr = pIPersist->load(pUnk, (long)htmlPersistStateHistory, &fContinue);

    hr = (fContinue==VB_TRUE) ? hr : S_FALSE;

Cleanup:
    ReleaseInterface(pUnk);
    ReleaseInterface(pXMLDoc);
    RRETURN1( hr, S_FALSE );
}


//+--------------------------------------------------------------------------
//
//  Member : PersistUserData
//
//  Synopsis : Handles most of the cases where persistence XTags are responsible
//      for handling favorites firing events and gathering user data associated with
//      this element.  in a nutshell :-
//          1> load/create and initialize and XML store
//          2> call the appropriate save/load method on the Peer's IHTMLPersistData
//          3> do any final processing (e.g. for shortcuts, put the xml store into
//                  the shortcut object via INamedPropertyBag)
//
//+--------------------------------------------------------------------------

HRESULT
CElement::DoPersistFavorite(IHTMLPersistData *pIPersist,
                            void *            pvNotify,
                            PERSIST_TYPE      sn)
{
    // This is called when we actually have a persistence cache and the meta tag is
    // enabling it's use
    HRESULT             hr = S_OK;
    BSTR                bstrStub  = NULL;
    VARIANT_BOOL        fContinue = VB_TRUE;
    VARIANT_BOOL        vtbCleanThisUp;
    CVariant            varValue;
    BSTR                bstrXMLText=NULL;
    IUnknown          * pUnk     = NULL;
    IXMLDOMDocument   * pXMLDoc  = NULL;
    INamedPropertyBag * pINPB    = NULL;
    IPersistStreamInit * pIPSI   = NULL;

    if (!pIPersist)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // first initialize the cache
    pINPB = ((FAVORITES_NOTIFY_INFO*)pvNotify)->pINPB;
    hr = GetPersistenceCache(&pXMLDoc);
    if (hr)
        goto Cleanup;

    // don't even initialize with this, if the permissions are wrong
    if (PersistAccessAllowed(pINPB))
    {
        PROPVARIANT  varValue;

        // shortcuts get the xml data into the INPB as the value of the property
        // "XMLUSERDATA"
        bstrStub = GetPersistID();
        if (!bstrStub)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // ask for a value of this type, if this type can't be served, then VT_ERROR
        //  comes back
        V_VT(&varValue) = VT_BLOB;
        hr = THR(pINPB->ReadPropertyNPB(bstrStub, _T("XMLUSERDATA"), &varValue));

        if (hr==S_OK && V_VT(&varValue) == VT_BLOB)
        {
            BSTR bstrXML;

            // turn the blob into a bstr
            bstrXML = SysAllocStringLen((LPOLESTR)varValue.blob.pBlobData,
                                              (varValue.blob.cbSize/sizeof(OLECHAR)));

            // now that we have the string for the xml object.
            if (bstrXML)
            {
                hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
                SysFreeString(bstrXML);
            }
            CoTaskMemFree(varValue.blob.pBlobData);
            hr = S_OK;
        }
    }
    else hr = S_FALSE;

    // if for some reason the cache is not yet init'd. do the default
    if (hr)
    {
        BSTR bstrXML;

        bstrXML = SysAllocString(_T("<ROOTSTUB />"));
        if (!bstrXML)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
        SysFreeString(bstrXML);
    }

    hr = pXMLDoc->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;

    // now that the xml object is initialized properly... call the XTag
    switch (sn)
    {
    case FAVORITES_LOAD:
        //  give the persist data xtag the chance to load its own information
        hr = pIPersist->load(pUnk, htmlPersistStateFavorite, &fContinue);
        break;

    case FAVORITES_SAVE:
        {
            PROPVARIANT varXML;

            hr = pIPersist->save(pUnk, htmlPersistStateFavorite, &fContinue);

            // now that we have fired the event and given the persist object the oppurtunity to
            // save its information, we need to actually save it.
            // First, load the variant with the XML Data

            // get the text of the XML document
            hr = THR(pXMLDoc->get_xml( &bstrXMLText));
            if (hr)
                goto Cleanup;

            // now save this in the ini file
            SysFreeString(bstrStub);
            bstrStub = GetPersistID();
            if (!bstrStub)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // set up the BLOB, don't free this
            V_VT(&varXML) = VT_BLOB;
            varXML.blob.cbSize = (1+SysStringLen(bstrXMLText))*sizeof(OLECHAR);
            varXML.blob.pBlobData = (BYTE*)bstrXMLText;

            // NOTE: security, don't allow more than 32K per object
            // otherwise, just fail silently and free thing up on the way out
            if (varXML.blob.cbSize < PERSIST_XML_DATA_SIZE_LIMIT)
            {
                // write the XML user data
                hr = THR(pINPB->WritePropertyNPB(bstrStub, _T("XMLUSERDATA"), &varXML));
                if (hr)
                    goto Cleanup;

                // now use the propvariant for the other value to write
                V_VT(&varXML) = VT_BSTR;
                if (S_OK != Doc()->_cstrUrl.AllocBSTR(&V_BSTR(&varXML)))
                    goto Cleanup;

                // and write the user data security url
                hr = THR(pINPB->WritePropertyNPB(bstrStub, _T("USERDATAURL"), &varXML));
                SysFreeString(V_BSTR(&varXML));
                if (hr)
                    goto Cleanup;
            }
        };
        break;
    }

    hr = (fContinue==VB_TRUE) ? hr : S_FALSE;


Cleanup:
    SysFreeString(bstrXMLText);
    SysFreeString(bstrStub);
    ReleaseInterface(pIPSI);
    ReleaseInterface(pUnk);
    ReleaseInterface(pXMLDoc);
    RRETURN1( hr, S_FALSE );
}


//+------------------------------------------------------------------------------
//
//  Member : AccessAllowed()
//
//  Synopsis : checks the security ID of hte document against he stored security
//      id of the XML user data section
//
//-------------------------------------------------------------------------------
BOOL
CElement::PersistAccessAllowed(INamedPropertyBag * pINPB)
{
    BOOL     fRes = FALSE;
    BSTR     bstrDomain;
    HRESULT  hr;
    PROPVARIANT varValue;
    CDoc *   pDoc = Doc();

    V_VT(&varValue) = VT_BSTR;
    V_BSTR(&varValue) = NULL;

    bstrDomain = GetPersistID();
    if (!bstrDomain)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pINPB->ReadPropertyNPB(bstrDomain,
                                     _T("USERDATAURL"),
                                     &varValue));
    SysFreeString(bstrDomain);
    if (hr)
        goto Cleanup;

    if (V_VT(&varValue) != VT_BSTR)
        goto Cleanup;

    fRes = pDoc->AccessAllowed(V_BSTR(&varValue));

Cleanup:
    if (V_BSTR(&varValue))
        SysFreeString(V_BSTR(&varValue));
    return fRes;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//          END PERSISTENCE ROUTINES
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
// Member:      HandleMnemonic
//
//  Synopsis:   This function is called because the user tried to navigate to
//              this element. There at least four ways the user can do this:
//                  1) by pressing Tab or Shift+Tab
//                  2) by pressing the access key for this element
//                  3) by clicking on a label associated with this element
//                  4) by pressing the access key for a label associated with
//                     this element
//
//              Typically, this function sets focus/currency to this element.
//              Click-able elements usually override this function to call
//              DoClick() on themselves if fTab is FALSE (i.e. navigation
//              happened due to reasons other than tabbing).
//
//----------------------------------------------------------------------------
HRESULT
CElement::HandleMnemonic(CMessage * pmsg, BOOL fDoClick, BOOL * pfYieldFailed /*=NULL*/)
{
    HRESULT     hr      = S_FALSE;
    CDoc *      pDoc    = Doc();

    Assert( IsInMarkup() );
    Assert(pmsg);

    if (IsFrameTabKey(pmsg) && Tag() != ETAG_FRAME)
        goto Cleanup;

    if ( pDoc->_pElemCurrent )
    {
        pDoc->_pElemCurrent->LostMnemonic(pmsg); // tell the element that it's losing focus due to a mnemonic
    }
    
    if (IsEditable(TRUE))
    {
        if (Tag() == ETAG_ROOT || Tag() == ETAG_BODY)
        {
            Assert( Tag() != ETAG_ROOT || this == pDoc->PrimaryRoot() );
            hr = THR(BecomeCurrentAndActive(NULL, pmsg->lSubDivision, TRUE));
        }
        else
        {
            // BUGBUG (jenlc)
            // Currently in design mode, only element with layout (site)
            // can be tabbed to.
            //

            CMarkup    * pMarkup   = GetMarkup();
            CElement *   pRootElement = pMarkup->Root();

            // BugFix 14600 / 14496 (JohnBed) 03/26/98
            // If the current element does not have layout, just fall
            // out of this so the parent element can handle the mnemonic

            CLayout * pLayout = GetUpdatedLayout();

            if (!pLayout)
                goto Cleanup;

            Assert(pRootElement);

            hr = THR(pRootElement->BecomeCurrentAndActive());

            if (hr)
                goto Cleanup;

            // Site-select the element
            // BUGBUG (MohanB) Make this into a function
            {
                CMarkupPointer      ptrStart(pDoc);
                CMarkupPointer      ptrEnd(pDoc);
                IMarkupPointer *    pIStart;
                IMarkupPointer *    pIEnd;

                hr = ptrStart.MoveAdjacentToElement(this, ELEM_ADJ_BeforeBegin);
                if (hr)
                    goto Cleanup;
                hr = ptrEnd.MoveAdjacentToElement(this, ELEM_ADJ_AfterEnd);
                if (hr)
                    goto Cleanup;

                Verify(S_OK == ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStart));
                Verify(S_OK == ptrEnd.QueryInterface(IID_IMarkupPointer, (void**)&pIEnd));
                hr = pDoc->Select(pIStart, pIEnd, SELECTION_TYPE_Control);
                if (hr)
                {
                    if (hr == E_INVALIDARG)
                    {
                        // This element was not site-selectable. Return S_FALSE, so
                        // that we can try the next element
                        hr = S_FALSE;
                    }
                }
                pIStart->Release();
                pIEnd->Release();
                if (hr)
                    goto Cleanup;
            }

            hr = THR(ScrollIntoView());
        }
    }
    else
    {
        Assert(IsFocussable(pmsg->lSubDivision));

        hr = THR(BecomeCurrentAndActive(pmsg, pmsg->lSubDivision, TRUE, pfYieldFailed));
        if(hr)
        {
            goto Cleanup;
        }

        hr = THR(ScrollIntoView());
        if (FAILED(hr))
            goto Cleanup;

        if (fDoClick 
            && (    _fActsLikeButton
                ||  Tag() == ETAG_INPUT && DYNCAST(CInput, this)->IsOptionButton()))
        {
            IGNORE_HR(DoClick(pmsg));
        }

        hr = GotMnemonic(pmsg);
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CElement::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagMsoCommandTarget, "CSite::QueryStatus"));

    Assert(IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *    pCmd = &rgCmds[0];
    HRESULT     hr = S_OK;

    Assert(!pCmd->cmdf);

    switch (IDMFromCmdID(pguidCmdGroup, pCmd->cmdID))
    {
    case IDM_DYNSRCPLAY:
    case IDM_DYNSRCSTOP:
        // The selected site wes not an image site, return disabled
       pCmd->cmdf = MSOCMDSTATE_DISABLED;
       break;

    case  IDM_SIZETOCONTROL:
    case  IDM_SIZETOCONTROLHEIGHT:
    case  IDM_SIZETOCONTROLWIDTH:
        // will be executed only if the selection is not a control range
       pCmd->cmdf = MSOCMDSTATE_DISABLED;
       break;

    case IDM_SETWALLPAPER:
        if (Doc()->_pOptionSettings->dwNoChangingWallpaper)
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;
        }
        // fall through
    case IDM_SAVEBACKGROUND:
    case IDM_COPYBACKGROUND:
    case IDM_SETDESKTOPITEM:
        pCmd->cmdf = GetNearestBgImgCtx() ? MSOCMDSTATE_UP
                                          : MSOCMDSTATE_DISABLED;
        break;

    case IDM_SELECTALL:
        if (HasFlag(TAGDESC_CONTAINER))
        {
            // Do not bubble to parent if this is a container.
            pCmd->cmdf =  DisallowSelection()
                            ? MSOCMDSTATE_DISABLED
                            : MSOCMDSTATE_UP;
        }
        break;
    }

//    if (!pCmd->cmdf )
//        hr = THR_NOTRACE(super::QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext));


    RRETURN_NOTRACE(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSite::Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------

HRESULT
CElement::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CSite::Exec"));

    Assert(IsCmdGroupSupported(pguidCmdGroup));

    UINT    idm;
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

    //
    // default processing
    //

    switch (idm = IDMFromCmdID(pguidCmdGroup, nCmdID))
    {
        case IDM_SAVEBACKGROUND:
        case IDM_SETWALLPAPER:
        case IDM_SETDESKTOPITEM:
        {
            CImgCtx *pImgCtx = GetNearestBgImgCtx();

            if (pImgCtx)
                Doc()->SaveImgCtxAs(pImgCtx, NULL, idm);

            hr = S_OK;
            break;
        }
        case IDM_COPYBACKGROUND:
        {
            CImgCtx *pImgCtx = GetNearestBgImgCtx();

            if (pImgCtx)
                CreateImgDataObject(Doc(), pImgCtx, NULL, NULL, NULL);

            hr = S_OK;
            break;
        }
    }
    if (hr != OLECMDERR_E_NOTSUPPORTED)
        goto Cleanup;

    //
    // behaviors
    //

    if (HasPeerHolder())
    {
        hr = THR_NOTRACE(GetPeerHolder()->ExecMulti(
                        pguidCmdGroup,
                        nCmdID,
                        nCmdexecopt,
                        pvarargIn,
                        pvarargOut));
        if (hr != OLECMDERR_E_NOTSUPPORTED)
            goto Cleanup;
    }

Cleanup:

    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------------------
//
//  Member: CElement::IsParent
//
//  Params: pElement: Check if pElement is the parent of this site
//
//  Descr:  Returns TRUE is pSite is a parent of this site, FALSE otherwise
//
//----------------------------------------------------------------------------
BOOL
CElement::IsParent(CElement *pElement)
{
    CTreeNode *pNodeSiteTest = GetFirstBranch();

    while (pNodeSiteTest)
    {
        if (SameScope(pNodeSiteTest, pElement))
        {
            return TRUE;
        }
        pNodeSiteTest = pNodeSiteTest->GetUpdatedParentLayoutNode();
    }
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::GetNearestBgImgCtx
//
//--------------------------------------------------------------------------
CImgCtx *
CElement::GetNearestBgImgCtx()
{
    CTreeNode * pNodeSite;
    CImgCtx * pImgCtx;

    for (pNodeSite = GetFirstBranch();
         pNodeSite;
         pNodeSite = pNodeSite->GetUpdatedParentLayoutNode())
    {
        pImgCtx = pNodeSite->Element()->GetBgImgCtx();
        if (pImgCtx && (pImgCtx->GetState() & IMGLOAD_COMPLETE))
            return pImgCtx;
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     Celement::GetParentForm()
//
//  Synopsis:   Returns the site's containing form if any.
//
//-------------------------------------------------------------------------
CFormElement *
CElement::GetParentForm()
{
    CElement * pElement = GetFirstBranch() ?
        GetFirstBranch()->SearchBranchToRootForTag(ETAG_FORM)->SafeElement() :
        NULL;

    if (pElement)
        return DYNCAST(CFormElement, pElement);
    else
        return NULL;
}


//+-------------------------------------------------------------------------
//
//  Method:   CDoc::TakeFocus
//
//  Synopsis: To have trident window take focus if it does not already have it.
//
//--------------------------------------------------------------------------
BOOL
CDoc::TakeFocus()
{
    BOOL fRet = FALSE;

    if (_pInPlace && !_pInPlace->_fDeactivating)
    {
        // CHROME
        // Can't use Win32 GetFocus() to test for focus as we have no hwnd
        // use CServer::GetFocus() instead which handles the windowless case
        if (!IsChromeHosted() ? (::GetFocus() != _pInPlace->_hwnd) : !GetFocus())
        {
            SetFocusWithoutFiringOnfocus();
            fRet = TRUE;
        }
    }

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::YieldCurrency
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pElementNew    New Element that wants currency
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CElement::YieldCurrency(CElement *pElemNew)
{
    HRESULT hr = S_OK;
    CLayout * pLayout = GetUpdatedLayout();
    CDoc* pDoc = Doc();
    if (pLayout)
    {
        CFlowLayout * pFlowLayout = pLayout->IsFlowLayout();
        if (pFlowLayout)
        {
            hr = THR(pFlowLayout->YieldCurrencyHelper(pElemNew));
            if (hr)
                goto Cleanup;
        }
    }

#ifndef NO_IME
    // Restore the IMC if we've temporarily disabled it.  See BecomeCurrent().

    if (pDoc && pDoc->_pInPlace && !pDoc->_pInPlace->_fDeactivating
        && pDoc->_pInPlace->_hwnd && pElemNew)
    {
        BOOL fIsPassword = ( pElemNew->Tag() == ETAG_INPUT 
                             && (DYNCAST(CInput, pElemNew))->GetType() == htmlInputPassword );

        // We don't know if ETAG_OBJECT is editable, so enable input
        // to be ie401 compat.

        if ((pElemNew->IsEditable() || pElemNew->Tag() == ETAG_OBJECT)
            && !fIsPassword && Doc()->_himcCache)
        {
            ImmAssociateContext(Doc()->_pInPlace->_hwnd, Doc()->_himcCache);
            Doc()->_himcCache = NULL;
        }
    }
#endif

    // Hide caret (but don't clear selection!) when focus changes
    {
        CMarkup* pMarkup = GetMarkup();
        if ( ( ( pMarkup->LoadStatus() == LOADSTATUS_DONE ) ||
               ( pMarkup->LoadStatus() == LOADSTATUS_PARSE_DONE ) ||
               ( pMarkup->LoadStatus() == LOADSTATUS_UNINITIALIZED ) )
                && ( _etag != ETAG_ROOT )) // make sure we're done parsing
        {
            if (pDoc->_pCaret)
            {
                pDoc->_pCaret->Hide();
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::YieldUI
//
//  Synopsis:   Relinquish UI, opposite of BecomeUIActive
//
//  Arguments:  pElementNew    New site that wants UI
//
//--------------------------------------------------------------------------

void
CElement::YieldUI(CElement *pElemNew)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::BecomeUIActive
//
//  Synopsis:   Force ui activity on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become ui active.
//
//--------------------------------------------------------------------------

HRESULT
CElement::BecomeUIActive()
{
    HRESULT hr = S_FALSE;
    CLayout *pLayout = GetUpdatedParentLayout();

    // This site does not care about grabbing the UI.
    // Give the parent a chance.
    if (pLayout)
    {
        hr = THR(pLayout->ElementOwner()->BecomeUIActive());
    }
    else
    {
        hr = GetMarkup()->Root()->BecomeUIActive();
    }
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::BecomeCurrent
//
//  Synopsis:   Force currency on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become current.
//
//--------------------------------------------------------------------------

HRESULT
CElement::BecomeCurrent(
    long        lSubDivision,
    BOOL *      pfYieldFailed,
    CMessage *  pmsg,
    BOOL        fTakeFocus /*=FALSE*/)
{
    HRESULT hr = S_FALSE;
    CDoc *      pDoc = Doc();
    CElement *  pElemOld = pDoc->_pElemCurrent;
    long        lSubOld = pDoc->_lSubCurrent;

    Assert(IsInMarkup());

    // Edit team's hack for TABLE && TD
    if (pDoc->_fDesignMode && (Tag() == ETAG_TABLE || Tag() == ETAG_TD))
    {
        CElement* pCurElement;

        pDoc->GetEditContext( this,
                        & pCurElement,
                        NULL,
                        NULL,
                        TRUE );
                        
        return (pCurElement->BecomeCurrent( lSubDivision, pfYieldFailed, pmsg, fTakeFocus));
    }

    // Hack for txtslave. The editing code calls BecomeCurrent() on txtslave.
    // For now, make the master current, Need to think about the nested slave tree case
    if (Tag() == ETAG_TXTSLAVE)
    {
        return MarkupMaster()->BecomeCurrent(lSubDivision, pfYieldFailed, pmsg, fTakeFocus);
    }


    Assert(pDoc->_fForceCurrentElem || pDoc->_pInPlace);
    if (!pDoc->_fForceCurrentElem && pDoc->_pInPlace->_fDeactivating)
        goto Cleanup;

    // For now, only the elements in the primary markup can become current
    Assert(GetMarkup() == pDoc->_pPrimaryMarkup);

    if (!IsFocussable(lSubDivision))
        goto Cleanup;

    hr = THR(PreBecomeCurrent(lSubDivision, pmsg));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->SetCurrentElem(this, lSubDivision, pfYieldFailed));
    if (hr)
    {
        IGNORE_HR(THR(BecomeCurrentFailed(lSubDivision, pmsg)));
        goto Cleanup;
    }

    //  The event might have killed the element

    if (! IsInMarkup() )
        goto Cleanup;

    // Do not inhibit if Currency did not change. Onfocus needs to be fired from
    // WM_SETFOCUS handler if clicking in the address bar and back to the same element
    // that previously had the focus.

    pDoc->_fInhibitFocusFiring = (this != pElemOld);

#ifndef NO_IME
    if (pDoc->_pInPlace && !pDoc->_pInPlace->_fDeactivating && pDoc->_pInPlace->_hwnd)
    {
        // If ElementOwner is not editable, disable current imm and cache the HIMC.  The 
        // HIMC is restored in YieldCurrency.
        //
        BOOL fIsPassword = ( Tag() == ETAG_INPUT 
                             && (DYNCAST(CInput, this))->GetType() == htmlInputPassword );

        // We don't know if ETAG_OBJECT is editable, so enable input
        // to be ie401 compat.

        if ((!IsEditable() && Tag() != ETAG_OBJECT)
            || fIsPassword)
        {
            HIMC himc = ImmGetContext(Doc()->_pInPlace->_hwnd);

            if (himc)
            {
                IUnknown* pUnknown = NULL;
                IGNORE_HR( this->QueryInterface(IID_IUnknown, (void**) & pUnknown) );
                IGNORE_HR( pDoc->NotifySelection(SELECT_NOTIFY_DISABLE_IME , pUnknown) );
                ReleaseInterface(pUnknown);
                
                ImmAssociateContext(Doc()->_pInPlace->_hwnd, NULL);
                Doc()->_himcCache = himc;
            }
        }
        else
        {
            if (Doc()->_himcCache)
            {
                ImmAssociateContext(Doc()->_pInPlace->_hwnd, Doc()->_himcCache);
                Doc()->_himcCache = NULL;        
            }

            if (Tag() == ETAG_INPUT || Tag() == ETAG_TEXTAREA)
                IGNORE_HR(SetImeState());

        }
    }
#endif // NO_IME

    if ( IsEditable( FALSE ) )
    {
        //
        // An editable element has just become current.
        // We tell the mshtmled.dll about this change in editing "context"
        //

        if (  _etag != ETAG_ROOT ) // make sure we're done parsing
        {
            //
            // marka - we need to put in whether to select the text or not.
            //
            hr =  THR(pDoc->SetEditContext((HasSlaveMarkupPtr()? GetSlaveMarkupPtr()->FirstElement() : this), TRUE, FALSE, TRUE ));
            if ( hr )
                goto Cleanup;

        }
    }


    //
    // Take focus only if told to do so, and the element becoming current is not 
    // the root and not an olesite.  Olesite's will do it themselves.  
    //
    
    if (fTakeFocus && 
        Tag() != ETAG_ROOT &&
        !TestClassFlag(ELEMENTDESC_OLESITE))
    {
        pDoc->TakeFocus();
    }


    pDoc->_fInhibitFocusFiring = FALSE;
    hr = S_OK;

    if (HasPeerHolder() && GetPeerHolder()->_pPeerUI)
    {
        GetPeerHolder()->_pPeerUI->OnReceiveFocus(TRUE, pDoc->_lSubCurrent);
    }

    if (pElemOld->HasPeerHolder() &&
        pElemOld->GetPeerHolder()->_pPeerUI)
    {
        pElemOld->GetPeerHolder()->_pPeerUI->OnReceiveFocus(
            FALSE,
            lSubOld);
    }

    if (Tag() != ETAG_ROOT && pDoc->_pInPlace)
    {
        CLayout * pLayout = GetUpdatedLayout();

        if (pLayout && pLayout->IsFlowLayout())
        {
            hr = THR(pLayout->IsFlowLayout()->BecomeCurrentHelper(lSubDivision, pfYieldFailed, pmsg));
            if (hr)
                goto Cleanup;
        }
    }


    hr = THR(PostBecomeCurrent(pmsg));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::BecomeCurrentAndActive
//
//  Synopsis:   Force currency and uiactivity on the site.
//
//--------------------------------------------------------------------------

HRESULT
CElement::BecomeCurrentAndActive(CMessage * pmsg, long lSubDivision, BOOL fTakeFocus /*=FALSE*/, 
                                 BOOL * pfYieldFailed /*=NULL*/)
{
    HRESULT     hr          = S_FALSE;
    CDoc *      pDoc        = Doc();
    CElement *  pElemOld    = pDoc->_pElemCurrent;
    long        lSubOld     = pDoc->_lSubCurrent;

    // Store the old current site in case the new current site cannot
    // become ui-active.  If the becomeuiactive call fails, then we
    // must reset currency to the old guy.
    //

    hr = THR(BecomeCurrent(lSubDivision, pfYieldFailed, pmsg, fTakeFocus));
    if (hr)
        goto Cleanup;

    hr = THR(BecomeUIActive());
    if (OK(hr))
    {
        hr = S_OK;
    }
    else
    {
        if (pElemOld)
        {
            // Don't take focus in this case
            Verify(!pElemOld->BecomeCurrent(lSubOld));
        }
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CElement::BubbleBecomeCurrent
//
//  Synopsis:   Bubble up BecomeCurrent requests through parent chain.
//
//----------------------------------------------------------------------------
HRESULT
CElement::BubbleBecomeCurrent(long       lSubDivision,
                              BOOL     * pfYieldFailed,
                              CMessage * pMessage,
                              BOOL       fTakeFocus)
{
    CTreeNode * pNode = NULL;
    HRESULT     hr=S_OK;

    if (Tag() == ETAG_TXTSLAVE)
    {
        return MarkupMaster()->BubbleBecomeCurrent(lSubDivision, pfYieldFailed, pMessage, fTakeFocus);
    }

    if (!GetFirstBranch())
        goto Cleanup;

    pNode = GetFirstBranch()->Parent();

    hr = THR(BecomeCurrent(lSubDivision, pfYieldFailed, pMessage, fTakeFocus));

    while (hr == S_FALSE && pNode)
    {
        hr = THR(pNode->Element()->BecomeCurrent(
                                0, pfYieldFailed, pMessage, fTakeFocus));
        if (hr == S_FALSE)
        {
            pNode = pNode->Parent();
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Method:     CElement::BubbleBecomeCurrentAndActive
//
//  Synopsis:   Bubble up BecomeCurrentAndActive requests through parent chain.
//
//----------------------------------------------------------------------------
HRESULT
CElement::BubbleBecomeCurrentAndActive(CMessage * pMessage, BOOL fTakeFocus)
{
    HRESULT     hr = S_FALSE;
    CTreeNode * pNode = GetFirstBranch();

    while (hr == S_FALSE && pNode)
    {
        hr = THR(pNode->Element()->BecomeCurrentAndActive(pMessage, 0, fTakeFocus));
        if (hr == S_FALSE)
        {
            pNode = pNode->Parent();
        }
    }
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::RequestYieldCurrency
//
//  Synopsis:   Check if OK to Relinquish currency
//
//  Arguments:  BOOl fForce -- if TRUE, force change and don't ask user about
//                             usaveable data.
//
//  Returns:    S_OK        ok to yield currency
//              S_FALSE     ok to yield currency, but user explicitly reverted
//                          the value to what the database has
//              E_*         not ok to yield currency
//
//--------------------------------------------------------------------------

HRESULT
CElement::RequestYieldCurrency(BOOL fForce)
{
    HRESULT hr = S_OK;
#ifndef NO_DATABINDING
    CElement * pElemBound;
    DBMEMBERS *pdbm;
    DBINFO dbi;

    if (!IsValid())
    {
        // BUGBUG   Need to display error message here.
        //          May not be able to handle synchronous
        //          display, so consider deferred display.
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!HasLayout())
        goto Cleanup;

    pElemBound = GetElementDataBound();
    pdbm = pElemBound->GetDBMembers();

    if (pdbm == NULL)
    {
        goto Cleanup;
    }

    // Save, with accompanying cancellable notifications
    hr = THR(pElemBound->SaveDataIfChanged(ID_DBIND_ALL, /* fLoud */ !fForce));
    if (fForce)
    {
        hr = S_OK;
    }

Cleanup:
#endif // ndef NO_DATABINDING
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member: CElement::GetInfo
//
//  Params: [gi]: The GETINFO enumeration.
//
//  Descr:  Returns the information requested in the enum
//
//----------------------------------------------------------------------------
DWORD
CElement::GetInfo(GETINFO gi)
{
    switch (gi)
    {
    case GETINFO_ISCOMPLETED:
        return TRUE;

    case GETINFO_HISTORYCODE:
        // BUGBUG (MohanB) Encode _uIndex also ? Need to reserve space
        // for GetAAtype() though, for input controls.
        return Tag();
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
// Method:      CElement::HaPercentBgImg
//
// Synopsis:    Does this element have a background image whose width or
//              height is percent based.
//
//----------------------------------------------------------------------------

BOOL
CElement::HasPercentBgImg()
{
    CImgCtx * pImgCtx = GetBgImgCtx();
    const CFancyFormat * pFF = GetFirstBranch()->GetFancyFormat();

    return pImgCtx &&
           (pFF->_cuvBgPosX.GetUnitType() == CUnitValue::UNIT_PERCENT ||
            pFF->_cuvBgPosY.GetUnitType() == CUnitValue::UNIT_PERCENT);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::IsVisible
//
//  Synopsis:   Is this layout element visible?
//
//  Parameters: fCheckParent - check the parent first
//
//----------------------------------------------------------------------------

BOOL
CElement::IsVisible (BOOL fCheckParent)
{
    BOOL        fInDesignMode   = Doc()->_fDesignMode;
    CLayout   * pLayout         = GetUpdatedNearestLayout();

    if (!pLayout)
    {
        if (Tag() == ETAG_TXTSLAVE)
        {
            CElement * pElemMaster = MarkupMaster();

            if (pElemMaster)
                return pElemMaster->IsVisible(fCheckParent);
        }
        return FALSE;
    }

    CTreeNode * pNode           = pLayout->GetFirstBranch();
    BOOL        fVisible        = !!pNode;

    if (fInDesignMode)
    {
        return TRUE;
    }

    if (pLayout->_fInvisibleAtRuntime)
        fVisible = FALSE;

    while (fVisible)
    {
        const CCharFormat * pCF;

        if (!pNode->Element()->IsInMarkup())
        {
            fVisible = FALSE;
            break;
        }

        if (!pNode->GetUpdatedLayout()->_fVisible)
        {
            fVisible = FALSE;
            break;
        }

        // BUGBUG This code was originally coded to use _this_ GetCharFormat but
        // do we want to use pNodeSite's instead? (jbeda)
        pCF = pNode->GetCharFormat();

        if (pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
        {
            fVisible = FALSE;
            break;
        }

        if (!fCheckParent)
        {
            fVisible = TRUE;
            break;
        }

        pNode = pNode->GetUpdatedParentLayoutNode();

        if (!pNode || pNode->Element()->_fEditAtBrowse)
        {
            fVisible = TRUE;
            break;
        }
    }

    return fVisible;
}


//+----------------------------------------------------------------------------
//
//  Member:     IsEditable()
//              IsEditableSlow()==IsEditable(TRUE) (but does not use cached info)
//
//  Synopsis:   returns if the site is editable (in design mode or editAtBrowse)
//  Note:
//       Check if the current element is editable
//       An element is editable has two meanings:
//        1) the element is editable in design mode, can be resized,
//            content can be edited
//        2) the content of the site is editable,
//            usually in browse mode
//
//       IsEditable(FALSE) or IsEditable() checks if the element is
//        Editable as a design mode item like the case 1)
//
//       IsEditable(TRUE) checks if the element is is NOT in case 2)
//       which means that the parent of element is in case 2)
//
//       this function has the following equivalences:
//        IsEditable(TRUE) = Doc()->_fDesignMode
//        IsEditable()     = Doc()->_fDesignMode || _fEditAtBrowse
//                         = !IsInBrowseMode() //CTxtsite method being removed
//
//       If you insist on the Document level editability, you should keep using
//        Doc()->_fDesignMode
//
//-----------------------------------------------------------------------------

BOOL CElement::IsEditableSlow()
{
    CTreeNode * pNode;
    CElement *  pElem;

    Assert( Doc() );

    pNode = GetFirstBranch();
    pElem = this;
    while (pNode)
    {
        // climb up to parent/master
        pNode = pNode->Parent();
        if (!pNode)
        {
            pElem = pElem->MarkupMaster();
            if (pElem)
            {
                pNode = pElem->GetFirstBranch();
            }
            if (!pNode)
                break;
        }

        pNode = pNode->GetUpdatedNearestLayoutNode();
        if (pNode && pNode->Element()->_fEditAtBrowse)
        {
            return TRUE;
        }
    }

    return (Doc()->_fDesignMode);
}

BOOL CElement::IsEditable(BOOL fCheckContainerOnly /*=FALSE*/)
{
    // BUGBUIG (MohanB) Since HTMLAREA is gone for now, this function
    // can be made really fast
#ifndef NEVER
    return (
                !fCheckContainerOnly && (   _fEditAtBrowse
                                         ||     Tag() == ETAG_TXTSLAVE
                                            &&  IsInMarkup()
                                            &&  GetMarkup()->IsEditable()
                                        )
            ||  Doc()->_fDesignMode
            
           );
#else
    CLayout *pLayout;

    Assert(Doc());
    // BUGBUG (MohanB) This is a hack. Need to fix this right. The
    // slave markup needs to implement IsEditable(fCheckContainerOnly).
    if (Tag() == ETAG_TXTSLAVE)
    {
        if ( MarkupMaster() )
            return MarkupMaster()->IsEditable(fCheckContainerOnly);
        else
            return FALSE; // we're not in a tree.
    }

    pLayout = GetUpdatedLayout();

    if (!pLayout)
    {
        pLayout = GetUpdatedNearestLayout();
    }
    else if (!fCheckContainerOnly && _fEditAtBrowse)
        return TRUE;

    if (!pLayout)
    {
        return (Doc()->_fDesignMode);
    }
    else if (pLayout->_fEditableDirty)
    {
        pLayout->_fIsEditable = IsEditableSlow();
        pLayout->_fEditableDirty = FALSE;
    }
    else
    {
        // we can switch to design mode without recompute format
        pLayout->_fIsEditable = pLayout->_fIsEditable || Doc()->_fDesignMode;
#if DBG == 1
        Assert(!GetFirstBranch() || !!pLayout->_fIsEditable == IsEditableSlow());
#endif
    }

    return pLayout->_fIsEditable;

#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::IsEnabled
//
//  Synopsis:   Is this element enabled? Try to get the cascaded info cached
//              in the char format. If the element is not in any markup,
//              use the 'disabled attribute directly.
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
CElement::IsEnabled()
{
    CTreeNode * pNode = GetFirstBranch();

    return !(pNode
            ? pNode->GetCharFormat()->_fDisabled
            : GetAAdisabled());
}


BOOL
CElement::IsLocked()
{
    // only absolute sites can be locked.
    if(IsAbsolute())
    {
        CVariant var;
        CStyle *pStyle;

        pStyle = GetInLineStylePtr();

        if (!pStyle || pStyle->getAttribute(L"Design_Time_Lock", 0, &var))
            return FALSE;

        return var.boolVal;
    }
    return FALSE;
}

// Accelerator Table handling
//
CElement::ACCELS::ACCELS(ACCELS * pSuper, WORD wAccels)
{
    _pSuper = pSuper;
    _wAccels = wAccels;
    _fResourcesLoaded = FALSE;
    _pAccels = NULL;
}

CElement::ACCELS::~ACCELS()
{
    delete _pAccels;
}

HRESULT
CElement::ACCELS::LoadAccelTable()
{
    HRESULT hr = S_OK;
    HACCEL  hAccel;
    int     cLoaded;

    if (!_wAccels)
        goto Cleanup;

    hAccel = LoadAccelerators(GetResourceHInst(), MAKEINTRESOURCE(_wAccels));
    if (!hAccel)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    _cAccels = CopyAcceleratorTable(hAccel, NULL, 0);
    Assert (_cAccels);

    _pAccels = new(Mt(CElement_pAccels)) ACCEL[_cAccels];
    if (!_pAccels)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    cLoaded = CopyAcceleratorTable(hAccel, _pAccels, _cAccels);
    if (cLoaded != _cAccels)
    {
        hr = E_OUTOFMEMORY;
    }

Cleanup:
    if (hr)
    {
        _cAccels = 0;
        _pAccels = NULL;
    }
    RRETURN (hr);
}


HRESULT
CElement::ACCELS::EnsureResources()
{
    if (!_fResourcesLoaded)
    {
        CGlobalLock     glock;

        if (!_fResourcesLoaded)
        {
            HRESULT hr;

            hr = THR(LoadAccelTable ());

            _fResourcesLoaded = TRUE;

            RRETURN (hr);
        }
    }

    return S_OK;
}


DWORD
CElement::ACCELS::GetCommandID(LPMSG pmsg)
{
    HRESULT     hr;
    DWORD       nCmdID = IDM_UNKNOWN;
    WORD        wVKey;
    ACCEL *     pAccel;
    int         i;
    DWORD       dwKeyState = FormsGetKeyState();

    if (WM_KEYDOWN != pmsg->message && WM_SYSKEYDOWN != pmsg->message)
        goto Cleanup;

    if (_pSuper)
    {
        nCmdID = _pSuper->GetCommandID(pmsg);
        if (IDM_UNKNOWN != nCmdID) // found id, nothing more to do
            goto Cleanup;
    }

    hr = THR(EnsureResources());
    if (hr)
        goto Cleanup;

    // loop through the table
    for (i = 0, pAccel = _pAccels; i < _cAccels; i++, pAccel++)
    {
// WINCEREVIEW - don't have VkKeyScan
#ifndef WINCE
        if (!(pAccel->fVirt & FVIRTKEY))
        {
            wVKey = LOBYTE(VkKeyScan(pAccel->key));
        }
        else
#endif // WINCE
        {
            wVKey = pAccel->key;
        }

        if (wVKey == pmsg->wParam &&
            EQUAL_BOOL(pAccel->fVirt & FCONTROL, dwKeyState & MK_CONTROL) &&
            EQUAL_BOOL(pAccel->fVirt & FSHIFT,   dwKeyState & MK_SHIFT) &&
            EQUAL_BOOL(pAccel->fVirt & FALT,     dwKeyState & MK_ALT))
        {
            nCmdID = pAccel->cmd;
            break;
        }
    }

Cleanup:
    return nCmdID;
}

CElement::ACCELS CElement::s_AccelsElementDesign =
                 CElement::ACCELS(NULL, IDR_ACCELS_SITE_DESIGN);
CElement::ACCELS CElement::s_AccelsElementRun    =
                 CElement::ACCELS(NULL, IDR_ACCELS_SITE_RUN);


//+---------------------------------------------------------------
//
//  Member:     CElement::PerformTA
//
//  Synopsis:   Forms implementation of TranslateAccelerator
//              Check against a list of accelerators for the incoming
//              message and if a match is found, fire the appropriate
//              command.  Return true if match found, false otherwise.
//
//  Input:      pMessage    Ptr to incoming message
//
//---------------------------------------------------------------
HRESULT
CElement::PerformTA(CMessage *pMessage)
{
    HRESULT     hr = S_FALSE;
    DWORD       cmdID;
    MSOCMD      msocmd;
    CDoc *      pDoc = Doc();

    cmdID = GetCommandID(pMessage);

    if (cmdID == IDM_UNKNOWN)
        goto Cleanup;

    // CONSIDER: (anandra) Think about using an Exec
    // call directly here, instead of sendmessage.
    //
    msocmd.cmdID = cmdID;
    msocmd.cmdf  = 0;

    hr = THR(pDoc->QueryStatus(
            (GUID *)&CGID_MSHTML,
            1,
            &msocmd,
            NULL));
    if (hr)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (msocmd.cmdf == 0)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (msocmd.cmdf == MSOCMDSTATE_DISABLED)
        goto Cleanup; // hr == S_OK;

    SendMessage(
            pDoc->_pInPlace->_hwnd,
            WM_COMMAND,
            GET_WM_COMMAND_MPS(cmdID, NULL, 1));
    hr = S_OK; 

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CElement::GetCommandID
//
//---------------------------------------------------------------
DWORD
CElement::GetCommandID(LPMSG lpmsg)
{
    DWORD       nCmdID;
    ACCELS  *   pAccels;

    pAccels = IsEditable() ? ElementDesc()->_pAccelsDesign
                           : ElementDesc()->_pAccelsRun;

    nCmdID = pAccels ? pAccels->GetCommandID(lpmsg) : IDM_UNKNOWN;

    // IE5 73627 -- If the SelectAll accelerator is received by an element
    // that does not allow selection (possibly because it is in a dialog),
    // treat the accelerator as unknown instead of disabled. This allows the
    // accelerator to bubble up.
    if (nCmdID == IDM_SELECTALL && DisallowSelection())
    {
        nCmdID = IDM_UNKNOWN;
    }

    return nCmdID;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::OnContextMenu
//
//  Synopsis:   Handles WM_CONTEXTMENU message.
//
//--------------------------------------------------------------------------
HRESULT
CElement::OnContextMenu(int x, int y, int id)
{
    int cx = x;
    int cy = y;
    CDoc *  pDoc = Doc();


    if (cx == -1 && cy == -1)
    {
        RECT rcWin;

        GetWindowRect(pDoc->InPlace()->_hwnd, &rcWin);
        cx = rcWin.left;
        cy = rcWin.top;
    }

    {
        EVENTPARAM  param(pDoc, TRUE);
        CTreeNode  *pNode;

        pNode = (pDoc->_pElementOMCapture && pDoc->_pNodeLastMouseOver) ?
                    pDoc->_pNodeLastMouseOver : GetFirstBranch();

        param.SetNodeAndCalcCoordinates(pNode);
        param.SetType(_T("MenuExtUnknown"));

        RRETURN1(THR(pDoc->ShowContextMenu(cx, cy, id, this)), S_FALSE);
    }
}

#ifndef NO_MENU
//+---------------------------------------------------------------
//
//  Member:     CElement::OnMenuSelect
//
//  Synopsis:   Handle WM_MENUSELECT by updating status line text.
//
//----------------------------------------------------------------
HRESULT
CElement::OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu)
{

    CDoc *pDoc = Doc();
    TCHAR achMessage[FORMS_BUFLEN + 1];

    if (hmenu == NULL && fuFlags == 0xFFFF) // menu closed
    {
        pDoc->SetStatusText(NULL, STL_ROLLSTATUS);
        return S_OK;
    }
    else if ((fuFlags & (MF_POPUP|MF_SYSMENU)) == 0 && uItem != 0)
    {
        LoadString(
                GetResourceHInst(),
                IDS_MENUHELP(uItem),
                achMessage,
                ARRAY_SIZE(achMessage));
    }

#if 0
    // what's this supposed to do????????????

    else if ((fuFlags & MF_POPUP) && (pDoc->InPlace()->_hmenuShared == hmenu))
    {
        // For top level popup menu
        //
        LoadString(
                GetResourceHInst(),
                IDS_MENUHELP(uItem),
                achMessage,
                ARRAY_SIZE(achMessage));
    }
#endif

    else
    {
        achMessage[0] = TEXT('\0');
    }
    pDoc->SetStatusText(achMessage, STL_ROLLSTATUS);

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CElement::OnInitMenuPopup
//
//  Synopsis:   Handles WM_CONTEXTMENU message.
//
//---------------------------------------------------------------
inline BOOL IsMenuItemFontOrEncoding(ULONG idm)
{
    return  ( idm >= IDM_MIMECSET__FIRST__ && idm <= IDM_MIMECSET__LAST__)
    ||  ( (idm >= IDM_BASELINEFONT1 && idm <= IDM_BASELINEFONT5)
                       || (idm == IDM_DIRLTR || idm == IDM_DIRRTL) );
}
HRESULT
CElement::OnInitMenuPopup(HMENU hmenu, int item, BOOL fSystemMenu)
{
    int             i;
    MSOCMD          msocmd;
    UINT            mf;

    for(i = 0; i < GetMenuItemCount(hmenu); i++)
    {
        msocmd.cmdID = GetMenuItemID(hmenu, i);
        if (msocmd.cmdID > 0 && !IsMenuItemFontOrEncoding(msocmd.cmdID))
        {
            Doc()->QueryStatus(
                    (GUID *) &CGID_MSHTML,
                    1,
                    &msocmd,
                    NULL);
            switch (msocmd.cmdf)
            {
            case MSOCMDSTATE_UP:
            case MSOCMDSTATE_NINCHED:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_UNCHECKED;
                break;

            case MSOCMDSTATE_DOWN:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_CHECKED;
                break;

            case MSOCMDSTATE_DISABLED:
            default:
                mf = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
                break;
            }
            CheckMenuItem(hmenu, msocmd.cmdID, mf);
            EnableMenuItem(hmenu, msocmd.cmdID, mf);
        }
    }
    return S_OK;
}
#endif // NO_MENU

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetColors
//
//  Synopsis:   Gets the color set for the Element.
//
//  Returns:    The return code is as per GetColorSet.
//              If any child site fails, that is our return code.  If any
//              child site returns S_OK, that is our return code, otherwise
//              we return S_FALSE.
//
//----------------------------------------------------------------------------
HRESULT
CElement::GetColors(CColorInfo *pCI)
{
    DWORD_PTR dw = 0;
    HRESULT   hr = S_FALSE;

    CLayout * pLayoutThis = GetUpdatedLayout();
    CLayout * pLayout;

    Assert(pLayoutThis && "CElement::GetColors() should not be called here !!!");
    if (!pLayoutThis)
        goto Error;

    for (pLayout = pLayoutThis->GetFirstLayout(&dw);
         pLayout && !pCI->IsFull() ;
         pLayout = pLayoutThis->GetNextLayout(&dw))
    {
        HRESULT hrTemp = pLayout->ElementOwner()->GetColors(pCI);
        if (FAILED(hrTemp) && hrTemp != E_NOTIMPL)
        {
            hr = hrTemp;
            goto Error;
        }
        else if (hrTemp == S_OK)
            hr = S_OK;
    }

Error:
    if (pLayoutThis)
        pLayoutThis->ClearLayoutIterator(dw, FALSE);
    RRETURN1(hr, S_FALSE);
}

#ifndef NO_DATABINDING

CElement *
CElement::GetElementDataBound()
{
    CElement * pElement = this;
    Assert(NeedsLayout() && "CElement::GetElementDataBound() should not be called !!!");
    return pElement;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::SaveDataIfChanged
//
//  Synopsis:   Determine whether or not is appropate to save the value
//              in  a control to a datasource, and do so.  Fire any appropriate
//              events.
//
//  Arguments:  id     - identifies which binding is being saved
//              fLoud  - should we put an alert in user's face on failure?
//              fForceIsCurrent - treat the element as if it were the current
//                          focus element, even if it's not
//
//  Returns:    S_OK        no work to do, or transfer successful
//              S_FALSE     user reverted value to database version
//
//--------------------------------------------------------------------------
HRESULT
CElement::SaveDataIfChanged(LONG id, BOOL fLoud, BOOL fForceIsCurrent)
{
    HRESULT     hr = S_OK;
    DBMEMBERS * pdbm;

    pdbm = GetDBMembers();
    if (!pdbm)
        goto Cleanup;

    hr = pdbm->SaveIfChanged(this, id, fLoud, fForceIsCurrent);

Cleanup:
    RRETURN1(hr, S_FALSE);
}
#endif // NO_DATABINDING

//+--------------------------------------------------------------------------------------
//  Layout related functions
//---------------------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CElement::CreateLayout
//
//  Synopsis:   Creates the layout object to be associated with the current element
//
//--------------------------------------------------------------------------
HRESULT
CElement::CreateLayout()
{
    CLayout *   pLayout=NULL;
    HRESULT     hr = S_OK;

    Assert(!HasLayoutPtr() && !_fSite);

    pLayout = new C1DLayout(this);

    _fOwnsRuns = TRUE;

    if (!pLayout)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        SetLayoutPtr(pLayout);
        pLayout->Init();
    }

    RRETURN(hr);
}


CFlowLayout *
CTreeNode::GetFlowLayout()
{
    CTreeNode   * pNode = this;
    CFlowLayout * pFL;

    while(pNode)
    {
        pFL = pNode->HasFlowLayout();
        if(pFL)
            return pFL;
        pNode = pNode->Parent();
    }

    return NULL;
}


CTreeNode *
CTreeNode::GetFlowLayoutNode()
{
    CTreeNode * pNode = this;

    while(pNode)
    {
        if(pNode->HasFlowLayout())
            return pNode;
        pNode = pNode->Parent();
    }

    return NULL;
}

CTreeNode *
CTreeNode::GetCurParentLayoutNode()
{
    CTreeNode * pNode = this;

    if (pNode)
    {
        pNode = pNode->Parent();
    }

    while(pNode)
    {
        if(pNode->HasLayout())
            return pNode;

        pNode = pNode->Parent();
    }
    return NULL;
}

CTreeNode *
CTreeNode::GetUpdatedParentLayoutNode()
{
    CTreeNode * pNode = this;

    if (pNode)
    {
        pNode = pNode->Parent();
    }

    while(pNode)
    {
        if(pNode->NeedsLayout())
            return pNode;

        pNode = pNode->Parent();
    }
    return NULL;
}

HRESULT
CElement::ScrollIntoView(SCROLLPIN spVert, SCROLLPIN spHorz, BOOL fScrollBits)
{
    HRESULT hr = S_OK;

    if(GetFirstBranch())
    {
        CLayout *   pLayout;

        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout = GetUpdatedParentLayout();

        if (pLayout)
        {
            hr = pLayout->ScrollElementIntoView(this, spVert, spHorz, fScrollBits);
        }
    }

    return hr;
}

HRESULT DetectSiteState(CLayout *pLayout)
{
    CElement * pElement = pLayout->ElementOwner();
    CDoc *     pDoc = pElement->Doc();

    if (!pElement->IsInMarkup())
        return E_UNEXPECTED;

    if (pDoc->State() < OS_INPLACE ||
        pDoc->_fHidden == TRUE ||
        !pElement->IsVisible(TRUE) ||
        !pElement->IsEnabled() ||
        !pDoc->_fEnabled)
        return CTL_E_CANTMOVEFOCUSTOCTRL;

    return S_OK;
}

STDMETHODIMP
CElement::focus()
{
    RRETURN(THR(
        (ETAG_AREA == Tag())?
                DYNCAST(CAreaElement, this)->focus()
            :   focusHelper(0)
    ));
}


HRESULT
CElement::focusHelper(long lSubDivision)
{
    HRESULT     hr          = S_OK;
    CLayout *   pLayout     = GetUpdatedLayout();
    CDoc *      pDoc        = Doc();
    BOOL        fTakeFocus;

    // BUGBUG(sramani): Hack for IE5 bug# 56032. Don't allow focus() calls on
    // hidden elements in outlook98 organize pane to return an error.

    if (!_tcsicmp(pDoc->_cstrUrl, _T("outday://")) && !IsVisible(TRUE))
        goto Cleanup;

    if (pLayout)
    {
        // bail out if the site has been detached, or the doc is not yet inplace, etc.
        hr = THR(DetectSiteState(pLayout));
        if (hr)
            goto Cleanup;
    }

    // if called on body, delegate to the window
    if (Tag() == ETAG_BODY)
    {
        pDoc->EnsureOmWindow();
        if (pDoc->_pOmWindow)
            hr = THR(pDoc->_pOmWindow->focus());

        goto Cleanup;
    }

    if (!IsFocussable(lSubDivision))
        goto Cleanup;

    // if our thread does not have any active windows, make ourselves the foreground window
    // (above all other top-level windows) only if the current foreground is a browser window
    // else make ourselves come above the topmost browser window if any, else do nothing.
    fTakeFocus = pDoc->_pInPlace && !MakeThisTopBrowserInProcess(pDoc->_pInPlace->_hwnd);

    // if we are a frame in a frameset and not current, take the focus first
    // CHROME
    // Can't use Win32 GetFocus() to test for focus as we have no hwnd
    // use CServer::GetFocus() instead which handles the windowless case
    if (fTakeFocus && pDoc->_pDocParent && pDoc->_pInPlace &&
        (!pDoc->IsChromeHosted() ? (::GetFocus() != pDoc->_pInPlace->_hwnd) : !pDoc->GetFocus()))
    {
        if (!pDoc->IsChromeHosted())
            ::SetFocus(pDoc->_pInPlace->_hwnd);
        else
            pDoc->SetFocus(TRUE);
    }

    pDoc->GetRootDoc()->_fFirstTimeTab = FALSE;
    IGNORE_HR(BecomeCurrent(lSubDivision, NULL, NULL, fTakeFocus));
    SendNotification(NTYPE_ELEMENT_ENSURERECALC);
    IGNORE_HR(ScrollIntoView(SP_MINIMAL, SP_MINIMAL));

    hr = THR(BecomeUIActive());

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


STDMETHODIMP
CElement::blur()
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    if (ETAG_AREA == Tag())
    {
        hr = THR(DYNCAST(CAreaElement, this)->blur());
        goto Cleanup;
    }

    if (HasLayout() && !IsInMarkup())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // if called on body, delegate to the window
    if (Tag() == ETAG_BODY)
    {
        pDoc->EnsureOmWindow();
        if (pDoc->_pOmWindow)
            hr = THR(pDoc->_pOmWindow->blur());

        goto Cleanup;
    }

    // Don't blur current object in focus if called on
    // another object that does not have the focus or if the
    // frame in which this object is does not currently have the focus
    if (this != pDoc->_pElemCurrent ||
        ((!pDoc->IsChromeHosted()
            ? (::GetFocus() != pDoc->_pInPlace->_hwnd)
            : !pDoc->GetFocus()) && Tag() != ETAG_SELECT))
        goto Cleanup;

    hr = THR(pDoc->GetPrimaryElementTop()->BecomeCurrent(0));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::addFilter(IUnknown *pUnk)
{
    HRESULT hr = S_OK;
    IHTMLViewFilter *pFilter = NULL;
    IHTMLViewFilter *pSource = NULL;


    if (!pUnk)
    {
        hr = E_INVALIDARG;
    }
    else if (!GetUpdatedLayout())
    {
        hr = E_FAIL;
    }
    else
    {
        // make sure this IUnknown supports IHTMLViewFilter
        hr = THR(pUnk->QueryInterface(IID_IHTMLViewFilter, (LPVOID*)&pFilter));
        if (FAILED(hr))
            goto Cleanup;

        // make sure this filter isn't already filtering something else!
        hr = THR(pFilter->GetSource(&pSource));
        if (SUCCEEDED(hr) && pSource)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (!HasFilterPtr())
        {
            CFilter *pFilter = new CFilter(this);
            if (pFilter)
            {
                SetFilterPtr(pFilter);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        hr = THR(GetFilterPtr()->AddFilter(pFilter));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ClearInterface(&pSource);
    ClearInterface(&pFilter);

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::removeFilter(IUnknown *pUnk)
{
    HRESULT hr = S_OK;
    IHTMLViewFilter *pFilter = NULL;

    if (!pUnk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if(HasLayout())
    {
        if (!HasFilterPtr())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        // make sure this IUnknown supports IHTMLViewFilter
        hr = THR(pUnk->QueryInterface(IID_IHTMLViewFilter, (LPVOID*)&pFilter));
        if (FAILED(hr))
            goto Cleanup;

        hr = GetFilterPtr()->RemoveFilter(pFilter);
    }

Cleanup:
    ClearInterface(&pFilter);
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------
//
//  member  :   get_clientWidth, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      width (not counting scrollbar, borders..)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientWidth( long * pl)
{
    HRESULT hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        CLayout *pLayout = GetUpdatedLayout();

        if(pLayout)
        {
            RECT    rect;

            // TR's are strange beasts since they have layout but
            // no display (unless they are positioned....
            if (Tag() == ETAG_TR)
            {
                CDispNode * pDispNode = pLayout->GetElementDispNode();
                if (!pDispNode)
                {
                    // we don't have a display so GetClientRect will return 0
                    // so rather than this, default to the offsetWidth.  This
                    // is the same behavior as scrollWidth
                    hr = get_offsetWidth(pl);
                    goto Cleanup;
                }
            }

            pLayout->GetClientRect(&rect, CLIENTRECT_VISIBLECONTENT);

            *pl = rect.right - rect.left;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientHeight, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      Height of the body
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientHeight( long * pl)
{
    HRESULT hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        CLayout *pLayout = GetUpdatedLayout();

        if(pLayout)
        {
            RECT    rect;

            // TR's are strange beasts since they have layout but
            // no display (unless they are positioned....
            if (Tag() == ETAG_TR)
            {
                CDispNode * pDispNode = pLayout->GetElementDispNode();
                if (!pDispNode)
                {
                    // we don't have a display so GetClientRect will return 0
                    // so rather than this, default to the offsetHeight. This
                    // is the same behavior as scrollHeight
                    hr = get_offsetHeight(pl);
                    goto Cleanup;
                }
            }

            pLayout->GetClientRect(&rect, CLIENTRECT_VISIBLECONTENT);

            *pl = rect.bottom - rect.top;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientTop, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      Top (inside borders)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientTop( long * pl)
{
    HRESULT     hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout *   pLayout;

        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout = GetUpdatedLayout();
        if (pLayout)
        {
            CDispNode * pDispNode= pLayout->GetElementDispNode();

            if (pDispNode)
            {
                CRect rcBorders;

                pDispNode->GetBorderWidths(&rcBorders);

                *pl = rcBorders.top;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------
//
//  member  :   get_clientLeft, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      Left (inside borders)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientLeft( long * pl)
{
    HRESULT     hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout *   pLayout;

        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout   = GetUpdatedLayout();

        if (pLayout)
        {
            CDispNode * pDispNode= pLayout->GetElementDispNode();

            if (pDispNode)
            {
                // border and scroll widths are dynamic. This method
                // provides a good way to get the client left amount
                // without having to have special knowledge of the
                // display tree workings. We are getting the distance
                // between the top left of the client rect and the
                // top left of the container rect.
                CRect rcClient;
                pLayout->GetClientRect(&rcClient);
                CPoint pt(rcClient.TopLeft());

                pDispNode->TransformPoint(&pt, COORDSYS_CONTENT, COORDSYS_CONTAINER);

                *pl = pt.x;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::SetDefaultElem
//
//  Synopsis:   Set the default element
//  Parameters:
//              fFindNew = TRUE: we need to find a new one
//                       = FALSE: set itself to be the default if appropriate
//
//-------------------------------------------------------------------------
void
CElement::SetDefaultElem(BOOL fFindNew /* FALSE */)
{
    CFormElement    * pForm = GetParentForm();
    CElement        **ppElem;
    CDoc            * pDoc  = Doc();

    ppElem = pForm ? &pForm->_pElemDefault : &pDoc->_pElemDefault;

    Assert(TestClassFlag(ELEMENTDESC_DEFAULT));
    if (fFindNew)
    {
        // Only find the new when the current is the default and
        // the document is not deactivating
        *ppElem = (*ppElem == this
                    && pDoc->_pInPlace
                    && !pDoc->_pInPlace->_fDeactivating) ?
                                    FindDefaultElem(TRUE, TRUE) : 0;
        Assert(*ppElem != this);
    }
    else if (!*ppElem || (*ppElem)->GetSourceIndex() > GetSourceIndex())
    {
                if (IsEnabled() && IsVisible(TRUE))
        {
            *ppElem = this;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::FindDefaultElem
//
//  Synopsis:   Find the default element
//
//-------------------------------------------------------------------------

CElement *
CElement::FindDefaultElem(BOOL fDefault,
                          BOOL fFull    /* = FALSE */)
{
    CElement        * pElem = NULL;
    CFormElement    * pForm = NULL;
    CDoc *            pDoc = Doc();

    if (!pDoc || !pDoc->_pInPlace || pDoc->_pInPlace->_fDeactivating)
        return NULL;

    if (!fFull && (fDefault ? _fDefault : _fCancel))
        return this;

    pForm = GetParentForm();

    if (pForm)
    {
        if (fFull)
        {
            pElem = pForm->FindDefaultElem(fDefault, FALSE);
            if (fDefault)
            {
                pForm->_pElemDefault = pElem;
            }
        }
        else
        {
            Assert(fDefault);
            pElem = pForm->_pElemDefault;
        }
    }
    else
    {
        if (fFull)
        {
            pElem = pDoc->FindDefaultElem(fDefault, FALSE);
            if (fDefault)
            {
                pDoc->_pElemDefault = pElem;
            }
        }
        else
        {
            Assert(fDefault);
            pElem = pDoc->_pElemDefault;
        }
    }

    return pElem;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::get_uniqueNumber
//
//  Synopsis:   Compute a unique ID of the element regardless of the id
//              or name property (these could be non-unique on our page).
//              This function guarantees a unique name which will not change
//              for the life of this document.
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_uniqueNumber(long *plUniqueNumber)
{
    HRESULT     hr = S_OK;
    CStr        cstrUniqueName;
    TCHAR      *pFoundStr;

    hr = THR(GetUniqueIdentifier(&cstrUniqueName, TRUE));
    if (hr)
        goto Cleanup;

    pFoundStr = StrStr(UNIQUE_NAME_PREFIX, cstrUniqueName);
    if (pFoundStr)
    {
        pFoundStr += lstrlenW(UNIQUE_NAME_PREFIX);

        if (ttol_with_error(pFoundStr, plUniqueNumber))
        {
            *plUniqueNumber = 0;            // Unknown number.
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::get_uniqueID
//
//  Synopsis:   Compute a unique ID of the element regardless of the id
//              or name property (these could be non-unique on our page).
//              This function guarantees a unique name which will not change
//              for the life of this document.
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_uniqueID(BSTR * pUniqueStr)
{
    HRESULT     hr = S_OK;
    CStr        cstrUniqueName;

    hr = THR(GetUniqueIdentifier(&cstrUniqueName, TRUE));
    if (hr)
        goto Cleanup;

    hr = cstrUniqueName.AllocBSTR(pUniqueStr);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::NoUIActivate
//
//  Synopsis:   Determines if this site does not allow UIActivation
//
//  Arguments:  none
//
//--------------------------------------------------------------------------
BOOL
CElement::NoUIActivate()
{
    CLayout * pLayout = GetUpdatedLayout();

    return pLayout &&
            (pLayout->_fNoUIActivate ||
                (Doc()->_fDesignMode && pLayout->_fNoUIActivateInDesign));
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::GetSubDivisionCount
//
//  Synopsis:   Get the count of subdivisions for this element
//
//-------------------------------------------------------------------------

HRESULT
CElement::GetSubDivisionCount(long *pc)
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2;
    ISubDivisionProvider *  pProvider = NULL;

    *pc = 0;
    if (HasPeerHolder())
    {
        CPeerHolder *pHolder = GetPeerHolder();

        if (pHolder->_pPeerUI)
        {
            hr2 = THR(pHolder->_pPeerUI->GetSubDivisionProvider(&pProvider));
            if (hr2)
            {
                // if this failed - e.g. peer does not implement subdivision provider, -
                // we still want to keep looking for next tab stop, so we supress the error here
                Assert (S_OK == hr);
                goto Cleanup;
            }

            hr = THR(pProvider->GetSubDivisionCount(pc));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::GetSubDivisionTabs
//
//  Synopsis:   Get the tabindices of subdivisions for this element
//
//-------------------------------------------------------------------------

HRESULT
CElement::GetSubDivisionTabs(long *pTabs, long c)
{
    HRESULT                 hr = S_OK;
    ISubDivisionProvider *  pProvider = NULL;

    if (HasPeerHolder())
    {
        CPeerHolder *pHolder = GetPeerHolder();

        if (pHolder->_pPeerUI)
        {
            hr = THR(pHolder->_pPeerUI->GetSubDivisionProvider(&pProvider));
            if (hr)
                goto Cleanup;

            hr = THR(pProvider->GetSubDivisionTabs(c, pTabs));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        Assert(c == 0);
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::SubDivisionFromPt
//
//  Synopsis:   Perform a hittest of subdivisions for this element
//
//-------------------------------------------------------------------------

HRESULT
CElement::SubDivisionFromPt(POINT pt, long *plSub)
{
    HRESULT                 hr = S_OK;
    ISubDivisionProvider *  pProvider = NULL;

    *plSub = 0;
    if (HasPeerHolder())
    {
        CPeerHolder *pHolder = GetPeerHolder();

        if (pHolder->_pPeerUI)
        {
            hr = THR_NOTRACE(pHolder->_pPeerUI->GetSubDivisionProvider(&pProvider));
            if (hr)
                goto Cleanup;

            hr = THR(pProvider->SubDivisionFromPt(pt, plSub));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetNextSubdivision
//
//  Synopsis:   Finds the next tabbable subdivision which has tabindex
//              == 0.  This is a helper meant to be used by SearchFocusTree.
//
//  Returns:    The next subdivision in plSubNext.  Set to -1, if there is no
//              such subdivision.
//
//  Notes:      lSubDivision coming in can be set to -1 to search for the
//              first possible subdivision.
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetNextSubdivision(
    FOCUS_DIRECTION dir,
    long lSubDivision,
    long *plSubNext)
{
    HRESULT hr = S_OK;
    long    c;
    long *  pTabs = NULL;
    long    i;

    hr = THR(GetSubDivisionCount(&c));
    if (hr)
        goto Cleanup;

    if (!c)
    {
        *plSubNext = (lSubDivision == -1) ? 0 : -1;
        goto Cleanup;
    }

    pTabs = new (Mt(CElementGetNextSubDivision_pTabs)) long[c];
    if (!pTabs)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(GetSubDivisionTabs(pTabs, c));
    if (hr)
        goto Cleanup;

    if (lSubDivision < 0 && dir == DIRECTION_BACKWARD)
    {
        lSubDivision = c;
    }

    //
    // Search for the next subdivision if possible to tab to it.
    //

    for (i = (DIRECTION_FORWARD == dir) ? lSubDivision + 1 :  lSubDivision - 1;
         (DIRECTION_FORWARD == dir) ? i < c : i >= 0;
         (DIRECTION_FORWARD == dir) ? i++ : i--)
    {
        if (pTabs[i] == 0 || pTabs[i] == htmlTabIndexNotSet)
        {
            //
            // Found something to tab to! Return it.  We're
            // only checking for zero here because negative
            // tab indices means cannot tab to them.  Positive
            // ones would already have been put in the focus array
            //

            *plSubNext = i;
            goto Cleanup;
        }
    }

    //
    // To reach here means that there are no further tabbable
    // subdivisions in this element.
    //

    *plSubNext = -1;

Cleanup:
    delete [] pTabs;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasCurrency
//
//  Synopsis:   Checks if this element has currency. If this element is a slave,
//              check if its master has currency.
//
//  Notes:      Can't inline, because that would need including the defn of
//              CDoc in element.hxx.
//
//----------------------------------------------------------------------------

BOOL
CElement::HasCurrency()
{
    CElement * pElemCurrent = Doc()->_pElemCurrent;

    // BUGBUG (MohanB) Need to generalize this for super-slaves
    // of arbitrary depth, by moving currency to CMarkup from CDoc
    return (    this
            &&  pElemCurrent
            &&  (pElemCurrent == this || pElemCurrent == MarkupMaster()));
}

BOOL
CElement::IsInPrimaryMarkup() const
{
    CMarkup * pMarkup = GetMarkup();

    return pMarkup ? pMarkup->IsPrimaryMarkup() : FALSE;
}

BOOL
CElement::IsInThisMarkup ( CMarkup* pMarkupIn ) const
{
    CMarkup * pMarkup = GetMarkup();

    return pMarkup == pMarkupIn;
}

CRootElement *
CElement::MarkupRoot()
{
    CMarkup * pMarkup = GetMarkup();

    return (pMarkup ? pMarkup->Root() : NULL);
}


CElement *
CElement::MarkupMaster() const
{
    CMarkup * pMarkup = GetMarkup();

    return pMarkup ? pMarkup->Master() : NULL;
}


CElement *
CElement::FireEventWith()
{
    CMarkup * pMarkup = GetMarkup();

    if (    pMarkup
        &&  pMarkup->FirstElement() == this
        &&  pMarkup->Master())
    {
        return pMarkup->Master();
    }
    return this;
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasLayoutLazy
//
//  Synopsis:   test for layoutNess requires the formats to be computed,
//              verify that formats are computed and test for layoutness.
//
//----------------------------------------------------------------------------

BOOL
CElement::HasLayoutLazy()
{
    Assert(!TestClassFlag(ELEMENTDESC_NOLAYOUT) && !_fSite);

    CTreeNode * pNode = GetFirstBranch();
    return pNode
            ? pNode->GetFancyFormat()->_fHasLayout
            : !!HasLayoutPtr();
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::Init
//
//  Synopsis:   Do any element initialization here, called after the element is
//              created from CreateElement()
//
//----------------------------------------------------------------------------

HRESULT
CElement::Init()
{
    HRESULT hr;

    hr = THR( super::Init() );

    if (hr)
        goto Cleanup;

    _fLayoutAlwaysValid = _fSite || TestClassFlag(ELEMENTDESC_NOLAYOUT);

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CElement::SetLayoutPtr
//
//  Synopsis:   Set the layout pointer
//
//----------------------------------------------------------------------------
void
CElement::SetLayoutPtr( CLayout * pLayout )
{
    Assert( ! HasLayoutPtr() );
    Assert( ! pLayout->_fHasMarkupPtr );

    pLayout->__pvChain = __pvChain;
    __pvChain = pLayout;

#if DBG == 1
    if( _fHasMarkupPtr )
    {
        pLayout->_pMarkupDbg = _pMarkupDbg;
    }
#endif
    pLayout->_fHasMarkupPtr = _fHasMarkupPtr;

    WHEN_DBG( _pLayoutDbg = pLayout );
    _fHasLayoutPtr = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DelLayoutPtr
//
//  Synopsis:   Remove the layout pointer
//
//----------------------------------------------------------------------------
CLayout *
CElement::DelLayoutPtr( )
{
    Assert( HasLayoutPtr() );

    Assert( (CLayout *) __pvChain == _pLayoutDbg );
    CLayout * pLayoutRet = (CLayout*) __pvChain;

    __pvChain = pLayoutRet->__pvChain;
    WHEN_DBG( _pLayoutDbg = NULL );
    _fHasLayoutPtr = FALSE;

    pLayoutRet->__pvChain = HasMarkupPtr() ? ( (CMarkup *)__pvChain)->Doc() : (CDoc*)__pvChain;

    WHEN_DBG( pLayoutRet->_pMarkupDbg = NULL );
    pLayoutRet->_fHasMarkupPtr = FALSE;

    return pLayoutRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetMarkupPtr
//
//  Synopsis:   Get the markup pointer if any
//
//----------------------------------------------------------------------------
CMarkup *
CElement::GetMarkupPtr() const
{
    void * pv = __pvChain;

    if (HasLayoutPtr())
        pv = ((CLayout*)pv)->__pvChain;

    if (HasMarkupPtr())
    {
        Assert( pv == _pMarkupDbg );
        return (CMarkup *)pv;
    }

    Assert( NULL == _pMarkupDbg );

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::SetMarkupPtr
//
//  Synopsis:   Set the markup pointer
//
//----------------------------------------------------------------------------
void
CElement::SetMarkupPtr( CMarkup * pMarkup )
{
    Assert( ! HasMarkupPtr() );
    Assert( pMarkup );

    if( HasLayoutPtr() )
    {
        Assert( (CLayout*)__pvChain == _pLayoutDbg );
        Assert( !_pLayoutDbg->_fHasMarkupPtr );
        Assert( pMarkup->Doc() == _pLayoutDbg->_pDocDbg );
        ( (CLayout*)__pvChain )->__pvChain = pMarkup;

        WHEN_DBG( _pLayoutDbg->_pMarkupDbg = pMarkup );
        ( (CLayout*)__pvChain )->_fHasMarkupPtr = TRUE;
    }
    else
    {
        Assert( pMarkup->Doc() == _pDocDbg );
        __pvChain = pMarkup;
    }

    WHEN_DBG( _pMarkupDbg = pMarkup );
    _fHasMarkupPtr = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DelMarkupPtr
//
//  Synopsis:   Remove the markup pointer
//
//----------------------------------------------------------------------------
void
CElement::DelMarkupPtr( )
{
    Assert( HasMarkupPtr() );

    if( HasLayoutPtr() )
    {
        Assert( (CLayout*)__pvChain == _pLayoutDbg );
        Assert( (CMarkup *)( ( (CLayout*)__pvChain )->__pvChain ) == _pMarkupDbg );
        Assert( _pLayoutDbg->_fHasMarkupPtr );
        ( (CLayout*)__pvChain )->__pvChain = ( (CMarkup *)( ( (CLayout*)__pvChain )->__pvChain ) )->Doc();
        WHEN_DBG( _pLayoutDbg->_pMarkupDbg = NULL );
        ( (CLayout*)__pvChain )->_fHasMarkupPtr = FALSE;
    }
    else
    {
        Assert( (CMarkup *)__pvChain == _pMarkupDbg );
        __pvChain = ( (CMarkup *)__pvChain )->Doc();
    }

    WHEN_DBG( _pMarkupDbg = NULL );
    _fHasMarkupPtr = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetDocPtr
//
//  Synopsis:   Get the CDoc pointer
//
//----------------------------------------------------------------------------
CDoc *
CElement::GetDocPtr() const
{
    void *  pv = __pvChain;

    if (HasLayoutPtr())
        pv = ((CLayout *)pv)->__pvChain;
    if (HasMarkupPtr())
        pv = ((CMarkup *)pv)->Doc();

    Assert( pv == _pDocDbg );

    return (CDoc*)pv;
}

#pragma optimize("", on)


//+------------------------------------------------------------------
//
//  Members: [get/put]_scroll[top/left] and get_scroll[height/width]
//
//  Synopsis : CElement members. _dp is in pixels.
//
//------------------------------------------------------------------

HRESULT
CElement::get_scrollHeight(long *plValue)
{
    HRESULT     hr = S_OK;

    if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout *   pLayout;

        // make sure that current is calced
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout = GetUpdatedLayout();

        if (pLayout)
        {
            *plValue = pLayout->GetContentHeight();
        }

        // we don't want to return a zero for the Height (only happens
        // when there is no content). so default to the offsetHeight
        if (!pLayout || *plValue==0)
        {
            // return the offsetWidth instead
            hr = THR(get_offsetHeight(plValue));
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_scrollWidth(long *plValue)
{
    HRESULT     hr = S_OK;

    if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout *   pLayout;

        // make sure that current is calced
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout = GetUpdatedLayout();

        if (pLayout)
        {
            *plValue = pLayout->GetContentWidth();
        }

        // we don't want to return a zero for teh width (only haoppens
        // when there is no content). so default to the offsetWidth
        if (!pLayout || *plValue==0)
        {
            // return the offsetWidth instead
            hr = THR(get_offsetWidth(plValue));
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_scrollTop(long *plValue)
{
    HRESULT     hr = S_OK;

     if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout *   pLayout;
        CDispNode * pDispNode;

        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        if ((pLayout = GetUpdatedLayout()) != NULL &&
            (pDispNode = pLayout->GetElementDispNode()) != NULL)
        {
            if (pDispNode->IsScroller())
            {
                CSize   sizeOffset;

                DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

                *plValue = sizeOffset.cy;
            }
            else
            {
                // if this isn't a scrolling element, then the scrollTop must be 0
                // for IE4 compatability
                *plValue = 0;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_scrollLeft(long *plValue)
{
    HRESULT     hr = S_OK;

    if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout    *pLayout;
        CDispNode  *pDispNode;

        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        if ((pLayout = GetUpdatedLayout()) != NULL &&
            (pDispNode = pLayout->GetElementDispNode()) != NULL)
        {
            if (pDispNode->IsScroller())
            {
                CSize   sizeOffset;

                DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

                *plValue = sizeOffset.cx;
            }
            else
            {
                // if this isn't a scrolling element, then the scrollTop must be 0
                // for IE$ compatability
                *plValue = 0;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::put_scrollTop(long lPixels)
{
    if(!GetFirstBranch())
    {
        RRETURN(E_FAIL);
    }
    else
    {
        CLayout *   pLayout;
        CDispNode * pDispNode;

        // make sure that the element is calc'd
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout   = GetUpdatedLayout();
        pDispNode = pLayout
                        ? pLayout->GetElementDispNode()
                        : NULL;

        if (    pDispNode
            &&  pDispNode->IsScroller())
        {

            // the display tree uses negative numbers to indicate nochange,
            // but the OM uses negative nubers to mean scrollto the top.
            pLayout->ScrollToY((lPixels <0) ? 0 : lPixels);
        }
    }

    return S_OK;
}


HRESULT
CElement::put_scrollLeft(long lPixels)
{
    if(!GetFirstBranch())
    {
        RRETURN(E_FAIL);
    }
    else
    {
        CLayout *   pLayout;
        CDispNode * pDispNode;

        // make sure that the element is calc'd
        SendNotification(NTYPE_ELEMENT_ENSURERECALC);

        pLayout   = GetUpdatedLayout();
        pDispNode = pLayout
                        ? pLayout->GetElementDispNode()
                        : NULL;

        if (    pDispNode
            &&  pDispNode->IsScroller())
        {
            pLayout->ScrollToX((lPixels<0) ? 0 : lPixels );
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------------
//
//  Member:     createControlRange
//
//  Synopsis:   Implementation of the automation interface method.
//              This creates a default structure range (CAutoTxtSiteRange) and
//              passes it back.
//
//+-------------------------------------------------------------------------------

HRESULT
CElement::createControlRange(IDispatch ** ppDisp)
{
    HRESULT             hr = E_INVALIDARG;
    CAutoTxtSiteRange * pControlRange = NULL;

    if (! ppDisp)
        goto Cleanup;

    if (! HasFlowLayout() )
    {
        hr = S_OK;
        goto Cleanup;
    }

    pControlRange = new CAutoTxtSiteRange(this);
    if (! pControlRange)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pControlRange->QueryInterface(IID_IDispatch, (void **) ppDisp) );
    pControlRange->Release();
    if (hr)
    {
        *ppDisp = NULL;
        goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


//+-------------------------------------------------------------------------------
//
//  Member:     clearAttributes
//
//+-------------------------------------------------------------------------------

HRESULT
CElement::clearAttributes()
{
    HRESULT hr = S_OK;
    CAttrArray *pAA = *(GetAttrArray());
    CMergeAttributesUndo Undo( this );

    Undo.SetClearAttr( TRUE );
    Undo.SetWasNamed( _fIsNamed );

    if (pAA)
    {
        pAA->Clear(Undo.GetAA());
        // BUGBUG (sramani) for now call onpropchange here, even though it will be duplicated
        // again if someone calls clear immediately followed by mergeAttributes (effectively
        // to do a copy). Will revisit in RTM

/*
        // BUGBUG(sramani) since ID is preserved by default, we will re-enable this in IE6
        // if\when optional param to nuke id is implemented.

        // If the element was named it's not anymore.
        if (_fIsNamed)
        {
            _fIsNamed = FALSE;
            // Inval all collections affected by a name change
                    DoElementNameChangeCollections();
        }
*/

        hr = THR(OnPropertyChange(DISPID_UNKNOWN, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS));
        if (hr)
            goto Cleanup;
    }

    IGNORE_HR(Undo.CreateAndSubmit());

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------
//
//  member : ComputeExtraFormat
//
//  Synopsis : Uses a miodified ComputeFomrats call to return requested
//              property. Only some of the properties can be returned this way.
//+----------------------------------------------------------------

HRESULT
CElement::ComputeExtraFormat(DISPID dispID,
                             BOOL fInherits,
                             CTreeNode * pTreeNode,
                             VARIANT *pVarReturn)
{
    BYTE            ab[sizeof(CFormatInfo)];
    CFormatInfo   * pInfo = (CFormatInfo *)ab;
    HRESULT         hr;

    Assert(pVarReturn);
    Assert(pTreeNode);

    // Make sure that the formats are calculated
    pTreeNode->GetCharFormatIndex();
    pTreeNode->GetFancyFormatIndex();

    VariantInit(pVarReturn);

    // Set the special mode flag so that ComputeFormats does not use
    // cached info,
    pInfo->_eExtraValues = (fInherits)
                ? ComputeFormatsType_GetInheritedValue
                : ComputeFormatsType_GetValue;

    // Save the requested property dispID
    pInfo->_dispIDExtra = dispID;
    pInfo->_pvarExtraValue = pVarReturn;

    hr = THR(ComputeFormats(pInfo, pTreeNode));
    if (hr)
        goto Cleanup;

Cleanup:
    pInfo->Cleanup();

    RRETURN(hr);
}


//+-------------------------------------------------------------------------------
//
//  Memeber:    SetImeState
//
//  Synopsis:   Check imeMode to set state of IME.
//
//+-------------------------------------------------------------------------------
HRESULT
CElement::SetImeState()
{
    HRESULT hr = S_OK;

#ifndef NO_IME
    CDoc *          pDoc = Doc();
    Assert( pDoc->_pInPlace->_hwnd );
    HIMC            himc = ImmGetContext(pDoc->_pInPlace->_hwnd);
    styleImeMode    sty;
    BOOL            fSuccess;
    VARIANT         varValue;

    if (!himc)
        goto Cleanup;

    hr = THR(ComputeExtraFormat(
        DISPID_A_IMEMODE,
        FALSE,
        GetUpdatedNearestLayoutNode(),
        &varValue));
    if(hr)
        goto Cleanup;

    sty = (((CVariant *)&varValue)->IsEmpty())
                            ? styleImeModeNotSet
                            : (styleImeMode) V_I4(&varValue);

    switch (sty)
    {
    case styleImeModeActive:
        fSuccess = ImmSetOpenStatus(himc, TRUE);
        if (!fSuccess)
            goto Cleanup;
        break;

    case styleImeModeInactive:
        fSuccess = ImmSetOpenStatus(himc, FALSE);
        if (!fSuccess)
            goto Cleanup;
        break;

    case styleImeModeDisabled:
        pDoc->_himcCache
            = ImmAssociateContext(pDoc->_pInPlace->_hwnd, NULL);

        break;

    case styleImeModeNotSet:
    case styleImeModeAuto:
    default:
        break;
    }

#endif //ndef NO_IME

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------------
//
//  Member:     mergeAttributes
//
//+-------------------------------------------------------------------------------

HRESULT
CElement::mergeAttributes(IHTMLElement *pIHTMLElementMergeThis)
{
    RRETURN(SetErrorInfo(MergeAttributesInternal(pIHTMLElementMergeThis)));
}

HRESULT
CElement::MergeAttributesInternal(IHTMLElement *pIHTMLElementMergeThis, BOOL fOMCall, BOOL fCopyID)
{
    HRESULT hr = E_INVALIDARG;
    CElement *pSrcElement;

    if (!pIHTMLElementMergeThis)
        goto Cleanup;

    hr = THR(pIHTMLElementMergeThis->QueryInterface(CLSID_CElement, (void **)&pSrcElement));
    if (hr)
        goto Cleanup;

    hr = THR(MergeAttributes(pSrcElement, fOMCall, fCopyID));
    if (hr)
        goto Cleanup;

    hr = THR(OnPropertyChange(DISPID_UNKNOWN, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetBstrFromElement
//
//  Synopsis:   A helper for data binding, fetches the text of some Element.
//
//  Arguments:  [fHTML]     - does caller want HTML or plain text?
//              [pbstr]     - where to return the BSTR holding the contents
//
//  Returns:    S_OK if successful
//
//-----------------------------------------------------------------------------

HRESULT
CElement::GetBstrFromElement ( BOOL fHTML, BSTR * pbstr )
{
    HRESULT hr;

    *pbstr = NULL;

    if (fHTML)
    {
        //
        //  Go through the HTML saver
        //
        hr = GetText(pbstr, 0);
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        //  Grab the plaintext directly from the runs
        //
        CStr cstr;

        hr = GetPlainTextInScope(&cstr);
        if (hr)
            goto Cleanup;

        hr = cstr.AllocBSTR(pbstr);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:   EnsureInMarkup()
//
//  Synopsis:   Creates a private markup for the element, if it is
//                  outside any markup.
//
//
//  Returns:    S_OK if successful
//
//-----------------------------------------------------------------------------

HRESULT
CElement::EnsureInMarkup ( )
{
    HRESULT   hr = S_OK;

    if (!IsInMarkup())
    {
        hr = THR( Doc()->CreateMarkupWithElement( NULL, this, NULL ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

CElement *CElement::GetFocusBlurFireTarget(long lSubDiv)
{
    HRESULT hr;
    CElement *pElemFireTarget = this;

    if (Tag() == ETAG_IMG)
    {
        CAreaElement *pArea = NULL;
        CImgElement *pImg = DYNCAST(CImgElement, this);

        pImg->EnsureMap();
        if (!pImg->GetMap())
            goto Cleanup;

        Assert(lSubDiv >= 0);
        hr = THR(pImg->GetMap()->GetAreaContaining(lSubDiv, &pArea));
        if (hr)
            goto Cleanup;

        Assert(pArea);
        pElemFireTarget = pArea;
    }

Cleanup:
    return pElemFireTarget;
}


BOOL
CElement::IsFocussable(long lSubDivision)
{
    // avoid visibilty and other checks for special elements
    if (Tag() == ETAG_ROOT || Tag() == ETAG_DEFAULT)
        return TRUE;

    CDoc* pDoc = Doc();
    
    FOCUSSABILITY   fcDefault   = GetDefaultFocussability(Tag());

    if ( fcDefault <= FOCUSSABILITY_MAYBE && pDoc->_fDesignMode )
    {
        if ( pDoc->IsElementUIActivatable( this ) )
            return TRUE;
    }
    
    if (    fcDefault <= FOCUSSABILITY_NEVER
        ||  !IsInMarkup()
        ||  !IsEnabled()
        ||  !IsVisible(TRUE)
        ||  NoUIActivate()
        ||  !(this == GetMarkup()->GetElementClient() || GetUpdatedParentLayoutNode())
       )
    {
        return FALSE;
    }

    // BUGBUG (MohanB) IsVisible() does not work for elements without layout.
    // Until, that is fixed, let's use a partial workaround (to do this right,
    // I probably need to walk up the tree until I hit the first layout parent)
    const CCharFormat * pCF = GetFirstBranch()->GetCharFormat();
    if (pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
        return FALSE;

    // do not  query for focussability if tabIndex is set.
    if (GetAAtabIndex() != htmlTabIndexNotSet)
        return TRUE;

    if (fcDefault < FOCUSSABILITY_FOCUSSABLE)
        return FALSE;

    // Hack for DIV and SPAN which want focus only they have a layout
    // I don't want to send them a queryfocussable because the hack is
    // more obvious here and we will try to get rid of it in IE6
    if ((Tag() == ETAG_DIV || Tag() == ETAG_SPAN) && !GetCurLayout())
        return FALSE;

    BOOL fNotify = FALSE;

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_A:
        case ETAG_IMG:
        case ETAG_SELECT:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        CQueryFocus qf;

        qf._lSubDivision    = lSubDivision;
        qf._fRetVal         = TRUE;

        SendNotification(NTYPE_ELEMENT_QUERYFOCUSSABLE, &qf);
        return qf._fRetVal;
    }
    else
    {
        return TRUE;
    }
}


BOOL
CElement::IsTabbable(long lSubDivision)
{
    FOCUSSABILITY   fcDefault   = GetDefaultFocussability(Tag());
    BOOL            fDesignMode =IsEditable(TRUE);

    if (fDesignMode)
    {
        // design-time tabbing checks for site-selectability

        // avoid visibilty and other checks for special elements
        if (Tag() == ETAG_ROOT || Tag() == ETAG_DEFAULT)
            return FALSE;

        if (!IsInMarkup() || !IsVisible(TRUE))
            return FALSE;

        // BUGBUG (MohanB) IsVisible() does not work for elements without layout.
        // Until, that is fixed, let's use a partial workaround (to do this right,
        // I probably need to walk up the tree until I hit the first layout parent)
        const CCharFormat * pCF = GetFirstBranch()->GetCharFormat();
        if (pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
            return FALSE;
    }
    else
    {
        // browse-time tabbing checks for focussability
        if (!IsFocussable(lSubDivision))
            return FALSE;
    }

    long lTabIndex = GetAAtabIndex();

    // Specifying an explicit tabIndex overrides the rest of the checks
    if (!(Tag() == ETAG_INPUT
            && DYNCAST(CInput, this)->GetType() == htmlInputRadio)
        && lTabIndex != htmlTabIndexNotSet)
    {
        return (lTabIndex >= 0);
    }

    if (    fcDefault < FOCUSSABILITY_TABBABLE
         && !(fDesignMode && GetCurLayout() && Doc()->IsElementSiteSelectable(this))
       )
    {      
        return FALSE;
    }

    BOOL fNotify = FALSE;

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_BODY:
        case ETAG_IMG:
        case ETAG_INPUT:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        CQueryFocus qf;

        qf._lSubDivision    = lSubDivision;
        qf._fRetVal         = TRUE;
        SendNotification(NTYPE_ELEMENT_QUERYTABBABLE, &qf);
        return qf._fRetVal;
    }
    else
    {
        return TRUE;
    }

}


HRESULT
CElement::PreBecomeCurrent(long lSubDivision, CMessage * pMessage)
{
    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        CSetFocus   sf;

        sf._pMessage        = pMessage;
        sf._lSubDivision    = lSubDivision;
        sf._hr              = S_OK;
        SendNotification(NTYPE_ELEMENT_SETTINGFOCUS, &sf);
        return sf._hr;
    }
    else
    {
        return S_OK;
    }
}


HRESULT
CElement::BecomeCurrentFailed(long lSubDivision, CMessage * pMessage)
{
    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        CSetFocus   sf;

        sf._pMessage        = pMessage;
        sf._lSubDivision    = lSubDivision;
        sf._hr              = S_OK;
        SendNotification(NTYPE_ELEMENT_SETFOCUSFAILED, &sf);
        return sf._hr;
    }
    else
    {
        return S_OK;
    }
}

HRESULT
CElement::PostBecomeCurrent(CMessage * pMessage)
{
    BOOL fNotify = FALSE;

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_A:
        case ETAG_BODY:
        case ETAG_BUTTON:
        case ETAG_IMG:
        case ETAG_INPUT:
        case ETAG_SELECT:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        CSetFocus   sf;

        sf._pMessage        = pMessage;
        sf._hr              = S_OK;
        SendNotification(NTYPE_ELEMENT_SETFOCUS, &sf);
        return sf._hr;
    }
    else
    {
        return S_OK;
    }
}

HRESULT
CElement::GotMnemonic(CMessage * pMessage)
{
    // Send only to the listeners
    switch (Tag())
    {
    case ETAG_FRAME:
    case ETAG_IFRAME:
    case ETAG_INPUT:
    case ETAG_TEXTAREA:
        {
            CSetFocus   sf;

            sf._pMessage    = pMessage;
            sf._hr          = S_OK;
            SendNotification(NTYPE_ELEMENT_GOTMNEMONIC, &sf);
            return sf._hr;
        }
    }
    return S_OK;
}


HRESULT
CElement::LostMnemonic(CMessage * pMessage)
{
    // Send only to the listeners
    switch (Tag())
    {
    case ETAG_INPUT:
    case ETAG_TEXTAREA:
        {
            CSetFocus   sf;

            sf._pMessage    = pMessage;
            sf._hr          = S_OK;
            SendNotification(NTYPE_ELEMENT_LOSTMNEMONIC, &sf);
            return sf._hr;
        }
    }
    return S_OK;
}

FOCUS_ITEM
CElement::GetMnemonicTarget()
{
    BOOL        fNotify = FALSE;
    FOCUS_ITEM  fi;

    fi.pElement = this;
    fi.lSubDivision = 0;
    // fi.lTabIndex is unused

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_AREA:
        case ETAG_LABEL:
        case ETAG_LEGEND:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        SendNotification(NTYPE_ELEMENT_QUERYMNEMONICTARGET, &fi);
    }
    return fi;
}

FOCUSSABILITY
GetDefaultFocussability(ELEMENT_TAG etag)
{
    switch (etag)
    {
    // FOCUSSABILITY_TABBABLE
    // These need to be in the tab order by default

    case ETAG_A:
    case ETAG_BODY:
    case ETAG_BUTTON:
    case ETAG_EMBED:
    case ETAG_FRAME:
    case ETAG_IFRAME:
    case ETAG_IMG:
    case ETAG_INPUT:
    case ETAG_ISINDEX:
    case ETAG_OBJECT:
    case ETAG_SELECT:
    case ETAG_TEXTAREA:

        return FOCUSSABILITY_TABBABLE;


    // FOCUSSABILITY_FOCUSSABLE
    // Not recommended. Better have a good reason for each tag why
    // it it is focussable but not tabbable

    //  Don't tab to applet.  This is to fix ie4 bug 41206 where the
    //  VM cannot call using IOleControlSite correctly.  If we allow
    //  tabbing into the VM, we can never ever tab out due to this.
    //  (AnandRa 8/21/97)
    case ETAG_APPLET:

    // Should be MAYBE, but for IE4 compat (IE5 #63134)
    case ETAG_CAPTION:

    // special element
    case ETAG_DEFAULT:

    // Should be MAYBE, but for IE4 compat
    case ETAG_DIV:

    // Should be MAYBE, but for IE4 compat (IE5 63626)
    case ETAG_FIELDSET:

    // special element
    case ETAG_FRAMESET:

    // Should be MAYBE, but for IE4 compat (IE5 #62701)
    case ETAG_MARQUEE:

    // special element
    case ETAG_ROOT:

    // Should be MAYBE, but for IE4 compat
    case ETAG_SPAN:

    // Should be MAYBE, but for IE4 compat
    case ETAG_TABLE:    
    case ETAG_TD:

        return FOCUSSABILITY_FOCUSSABLE;


    // Any tag that can render/have renderable content (and does not appear in the above lists)

    case ETAG_ACRONYM:
    case ETAG_ADDRESS:
    case ETAG_B:
    case ETAG_BDO:
    case ETAG_BIG:
    case ETAG_BLINK:
    case ETAG_BLOCKQUOTE:
    case ETAG_CENTER:
    case ETAG_CITE:
    case ETAG_DD:
    case ETAG_DEL:
    case ETAG_DFN:
    case ETAG_DIR:
    case ETAG_DL:
    case ETAG_DT:
    case ETAG_EM:
    case ETAG_FONT:
    case ETAG_FORM:
    case ETAG_GENERIC:
    case ETAG_GENERIC_BUILTIN:
    case ETAG_GENERIC_LITERAL:
    case ETAG_H1:
    case ETAG_H2:
    case ETAG_H3:
    case ETAG_H4:
    case ETAG_H5:
    case ETAG_H6:
    case ETAG_HR:
    case ETAG_I:
    case ETAG_INS:
    case ETAG_KBD:

    // target is always focussable, label itself is not (unless forced by tabIndex) 
    case ETAG_LABEL:
    case ETAG_LEGEND:

    case ETAG_LI:
    case ETAG_LISTING:
    case ETAG_MENU:
    case ETAG_OL:
    case ETAG_P:
    case ETAG_PLAINTEXT:
    case ETAG_PRE:
    case ETAG_Q:
    //case ETAG_RB:
    case ETAG_RT:
    case ETAG_RUBY:
    case ETAG_S:
    case ETAG_SAMP:
    case ETAG_SMALL:
    case ETAG_STRIKE:
    case ETAG_STRONG:
    case ETAG_SUB:
    case ETAG_SUP:
    case ETAG_TBODY:
    case ETAG_TC:
    case ETAG_TFOOT:
    case ETAG_TH:
    case ETAG_THEAD:
    case ETAG_TR:
    case ETAG_TT:
    case ETAG_U:
    case ETAG_UL:
    case ETAG_VAR:
    case ETAG_XMP:

        return FOCUSSABILITY_MAYBE;

    // All the others - tags that do not ever render

    // this is a subdivision, never takes focus direcxtly
    case ETAG_AREA:
    
    case ETAG_BASE:
    case ETAG_BASEFONT:
    case ETAG_BGSOUND:
    case ETAG_BR:
    case ETAG_CODE:
    case ETAG_COL:
    case ETAG_COLGROUP:
    case ETAG_COMMENT:
    case ETAG_HEAD:
    case ETAG_HTML:
    case ETAG_LINK:
    case ETAG_MAP:
    case ETAG_META:
    case ETAG_NEXTID:
    case ETAG_NOBR:
    case ETAG_NOEMBED:
    case ETAG_NOFRAMES:
    case ETAG_NOSCRIPT:

    // May change in future when we re-implement SELECT
    case ETAG_OPTION:

    case ETAG_PARAM:
    case ETAG_RAW_BEGINFRAG:
    case ETAG_RAW_BEGINSEL:
    case ETAG_RAW_CODEPAGE:
    case ETAG_RAW_COMMENT:
    case ETAG_RAW_DOCSIZE:
    case ETAG_RAW_ENDFRAG:
    case ETAG_RAW_ENDSEL:
    case ETAG_RAW_EOF:
    case ETAG_RAW_SOURCE:
    case ETAG_RAW_TEXT:
    case ETAG_RAW_TEXTFRAG:
    case ETAG_SCRIPT:
    case ETAG_STYLE:
    case ETAG_TITLE_ELEMENT:
    case ETAG_TITLE_TAG:
    case ETAG_WBR:
    case ETAG_UNKNOWN:

        return FOCUSSABILITY_NEVER;


    default:
        AssertSz(FALSE, "Focussability undefined for this tag");
        return FOCUSSABILITY_NEVER;
    }
}

HRESULT
CElement::get_tabIndex(short * puTabIndex)
{
    short tabIndex = GetAAtabIndex();

    *puTabIndex = (tabIndex == htmlTabIndexNotSet) ? 0 : tabIndex;
    return S_OK;
}

//+----------------------------------------------------------------------------
//  Member:     DestroyLayout
//
//  Synopsis:   Destroy the current layout attached to the element. This is
//              called from CFlowLayout::DoLayout to destroy the layout lazily
//              when an element loses layoutness.
//
//-----------------------------------------------------------------------------

void
CElement::DestroyLayout()
{
    CLayout  *  pLayout = GetCurLayout();

    Assert (HasLayout() &&  !NeedsLayout());
    Assert(!_fSite && !TestClassFlag(CElement::ELEMENTDESC_NOLAYOUT));

    pLayout->ElementContent()->_fOwnsRuns = FALSE;

    Verify(Doc()->OpenView());
    Verify(pLayout == DelLayoutPtr());

    pLayout->Reset(TRUE);
    pLayout->Detach();
    pLayout->Release();
}

BOOL
CElement::IsOverlapped()
{
    CTreeNode *pNode = GetFirstBranch();

    return pNode && !pNode->IsLastBranch();
}
