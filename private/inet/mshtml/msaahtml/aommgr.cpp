//=======================================================================
//      File:   AOMMGR.CPP
//      Date:   6-2-97
//      Desc:   Contains the implementation of the CAOMMgr class.  
// 
//      Notes:  The AOM Manager bridges the gap between the Trident
//              Object Model and the Accessible Object Model, creating
//              the latter based on the former.  This involves
//              dealing with Trident Element Objects on one hand and
//              Accessible Objects and Elements on the other, which can
//              get rather confusing.  So, the source code written in
//              this file will attempt to abide by the following variable
//              naming conventions:
//
//              "TEO" -> refers to a "Trident Element Object"
//              "TOM" -> refers to the "Trident Object Model"
//              pIHTMLxxx -> pointer to the IHTMLxxx TOM interface
//
//              "AOM" -> refers to the "Accessible Object Model"
//              "AO"  -> refers to an "Accessible Object"
//              "AE"  -> refers to an "Accessible Element"
//              "AccElem" -> refers to a CAccElement (abstract class)
//              "AccObj" -> refers to a CAccObject (abstract class)
//
//
//              Examples:
//
//              setTEOType() -> method assigns an appropriate flag
//                              for the current TEO.  Sets type of
//                              TEO to one AOM supports, or unsupported.
//                              ** NOTE ** this method formerly called
//                              isTEOSupported(), and was changed to
//                              accomodate lazy build algorithm requirements.
//
//              pTOMDoc -> pointer to the IHTMLDocument2 iface
//              pIHTMLDocument2 -> pointer to the IHTMLDocument2 iface
//
//              getAOMChildren() -> method that obtains the Accessible
//                                  Object Model children of the current
//                                  AccElem-derived object
//              pAccElem -> pointer to a CAccElement-derived object
//              pAOParent -> pointer to a CTridentAO being used as
//                           the current AOM sub-tree parent
//
//=======================================================================


//=======================================================================
//  Include Files
//=======================================================================

#include "stdafx.h"
#include "accelem.h"
#include "trid_ae.h"
#include "accobj.h"
#include "trid_ao.h"
#include "aommgr.h"
#include "document.h"
#include "button.h"
#include "checkbox.h"
#include "radiobtn.h"
#include "editfld.h"
#include "area.h"
#include "image.h"
#include "map.h"
#include "anchor.h"
#include "plugin.h"
#include "table.h"
#include "tablcell.h"
#include "div.h"
#include "marquee.h"
#include "select.h"
#include "window.h"
#include "document.h"
#include "prxymgr.h"
#include "imgbtn.h"

#ifdef __AOMMGR_ENABLETEXT
#include <tchar.h>
#include "text.h"
#endif

//=======================================================================
//  Defines
//=======================================================================

//------------------------------------------------
//  Supported HTML tag names.
//
//  TODO: Move these definitions into a string
//  resource table.
//------------------------------------------------

#define TEO_TAGNAME_ADDRESS     L"ADDRESS"
#define TEO_TAGNAME_ANCHOR      L"A"
#define TEO_TAGNAME_APPLET      L"APPLET"
#define TEO_TAGNAME_AREA        L"AREA"
#define TEO_TAGNAME_BGSOUND     L"BGSOUND"
#define TEO_TAGNAME_BLOCKQUOTE  L"BLOCKQUOTE"
#define TEO_TAGNAME_BODY        L"BODY"
#define TEO_TAGNAME_BR          L"BR"
#define TEO_TAGNAME_BUTTON      L"BUTTON"
#define TEO_TAGNAME_CAPTION     L"CAPTION"
#define TEO_TAGNAME_CENTER      L"CENTER"
#define TEO_TAGNAME_COL         L"COL"
#define TEO_TAGNAME_DD          L"DD"
#define TEO_TAGNAME_DFN         L"DFN"
#define TEO_TAGNAME_DIR         L"DIR"
#define TEO_TAGNAME_DIV         L"DIV"
#define TEO_TAGNAME_DL          L"DL"
#define TEO_TAGNAME_DT          L"DT"
#define TEO_TAGNAME_EMBED       L"EMBED"
#define TEO_TAGNAME_FONT        L"FONT"
#define TEO_TAGNAME_FORM        L"FORM"
#define TEO_TAGNAME_FRAME       L"FRAME"
#define TEO_TAGNAME_H1          L"H1"
#define TEO_TAGNAME_H2          L"H2"
#define TEO_TAGNAME_H3          L"H3"
#define TEO_TAGNAME_H4          L"H4"
#define TEO_TAGNAME_H5          L"H5"
#define TEO_TAGNAME_H6          L"H6"
#define TEO_TAGNAME_HR          L"HR"
#define TEO_TAGNAME_HTML        L"HTML"
#define TEO_TAGNAME_IFRAME      L"IFRAME"
#define TEO_TAGNAME_IMG         L"IMG"
#define TEO_TAGNAME_INPUT       L"INPUT"
#define TEO_TAGNAME_LI          L"LI"
#define TEO_TAGNAME_LISTING     L"LISTING"
#define TEO_TAGNAME_MAP         L"MAP"
#define TEO_TAGNAME_MARQUEE     L"MARQUEE"
#define TEO_TAGNAME_MENU        L"MENU"
#define TEO_TAGNAME_OBJECT      L"OBJECT"
#define TEO_TAGNAME_OL          L"OL"
#define TEO_TAGNAME_P           L"P"
#define TEO_TAGNAME_PLAINTEXT   L"PLAINTEXT"
#define TEO_TAGNAME_PRE         L"PRE"
#define TEO_TAGNAME_SCRIPT      L"SCRIPT"
#define TEO_TAGNAME_SELECT      L"SELECT"
#define TEO_TAGNAME_STYLE       L"STYLE"
#define TEO_TAGNAME_TABLE       L"TABLE"
#define TEO_TAGNAME_TD          L"TD"
#define TEO_TAGNAME_TEXTAREA    L"TEXTAREA"
#define TEO_TAGNAME_TH          L"TH"
#define TEO_TAGNAME_UL          L"UL"
#define TEO_TAGNAME_XMP         L"XMP"



//------------------------------------------------
//  Various TYPEs of INPUT controls.
//
//  TODO: Move these definitions into a string
//  resource table.
//------------------------------------------------

#define TEO_INPUTTYPE_BUTTON    L"BUTTON"
#define TEO_INPUTTYPE_CHECKBOX  L"CHECKBOX"
#define TEO_INPUTTYPE_IMAGE     L"IMAGE"
#define TEO_INPUTTYPE_HIDDEN    L"HIDDEN"
#define TEO_INPUTTYPE_PASSWORD  L"PASSWORD"
#define TEO_INPUTTYPE_RADIO     L"RADIO"
#define TEO_INPUTTYPE_RESET     L"RESET"
#define TEO_INPUTTYPE_SUBMIT    L"SUBMIT"
#define TEO_INPUTTYPE_TEXT      L"TEXT"
#define TEO_INPUTTYPE_TEXTAREA  L"TEXTAREA"


//------------------------------------------------
//  If an IMG is to be used as a client-side
//  image map, the first character of its USEMAP
//  property string must be a pound sign ('#').
//
//  TODO: Move this definition into a string
//  resource table.
//------------------------------------------------

#define TEO_IMG_USEMAPFIRSTCHAR         ((TCHAR)'#')


//--------------------------------------------------
//  Default size of the VARIANT array.  This array
//  is initially allocated in CAOMMgr::Init().
//--------------------------------------------------

#define AOMMGR_DEFAULTVARIANTARRAYSIZE  64


//================================================================================
// externs
//================================================================================

extern CProxyManager    *g_pProxyMgr; 



//=======================================================================
//  CTOMMapMgr class declaration
//=======================================================================

//--------------------------------------------------
//  Private class to manage the TOM document's
//  MAP collection.  Used exclusively by
//  processTOMImage().
//--------------------------------------------------

class CTOMMapMgr
{
public:

    CTOMMapMgr( /* in */ IHTMLElementCollection* pIHTMLElementCollectionAll );
    ~CTOMMapMgr();

    HRESULT MatchMapName( /* in */ LPWSTR lpWStrMapName );
    IHTMLMapElement* GetMatchingIHTMLMapElement( void );

    // helper function
    void Zombify();

protected:

    struct _TOMMapInfo
    {
        BSTR                m_bstrMapName;
        IHTMLMapElement*    m_pIHTMLMapElement;
    };

    HRESULT getTOMMaps( void );

    IHTMLElementCollection*     m_pIHTMLElementCollectionAll;
    struct _TOMMapInfo*         m_pMapInfo;
    long                        m_nCnt;
    int                         m_nIdxMatchingMap;
};




//=======================================================================
//  CTOMMapMgr class : public methods
//=======================================================================

//-----------------------------------------------------------------------
//  CTOMMapMgr::CTOMMapMgr()
//
//  DESCRIPTION:
//   
//  CTOMMapMgr constructor.  Simply initializes data members.
//
//  PARAMETERS:
//
//  pIHTMLElementCollectionAll      An IHTMLElementCollection pointer
//                                  to the TOM document's all collection.
//
//  RETURNS:
//
//  None.
//
//-----------------------------------------------------------------------

CTOMMapMgr::CTOMMapMgr( IHTMLElementCollection* pIHTMLElementCollectionAll )
{
    assert( pIHTMLElementCollectionAll );

    m_pIHTMLElementCollectionAll    = pIHTMLElementCollectionAll;
    m_pIHTMLElementCollectionAll->AddRef();
    m_pMapInfo                      = NULL;
    m_nCnt                          = 0;
    m_nIdxMatchingMap               = -1;
}


//-----------------------------------------------------------------------
//  CTOMMapMgr::~CTOMMapMgr()
//
//  DESCRIPTION:
//   
//  CTOMMapMgr destructor.  Free any allocated data.
//
//  PARAMETERS:
//
//  None.
//
//  RETURNS:
//
//  None.
//-----------------------------------------------------------------------

CTOMMapMgr::~CTOMMapMgr( void )
{
    Zombify();
}

void
CTOMMapMgr::Zombify()
{
    if ( m_pMapInfo )
    {
        for ( int i = 0; i < m_nCnt; i++ )
        {
            if ( m_pMapInfo[i].m_bstrMapName )
                SysFreeString( m_pMapInfo[i].m_bstrMapName );

            if ( m_pMapInfo[i].m_pIHTMLMapElement )
                m_pMapInfo[i].m_pIHTMLMapElement->Release();
        }

        delete [] m_pMapInfo;
        m_pMapInfo= NULL;
    }

    if (m_pIHTMLElementCollectionAll)
    {
        m_pIHTMLElementCollectionAll->Release();
        m_pIHTMLElementCollectionAll = NULL;
    }

}


//-----------------------------------------------------------------------
//  CTOMMapMgr::GetMatchingIHTMLMapElement()
//
//  DESCRIPTION:
//   
//  Returns the IHTMLMapElement pointer of the "matched" map.
//  A map is marked as matched through the MatchMapName() method.
//
//  PARAMETERS:
//
//  None.
//
//  RETURNS:
//
//  IHTMLMapElement*.
//-----------------------------------------------------------------------

inline IHTMLMapElement* CTOMMapMgr::GetMatchingIHTMLMapElement( void )
{
    if ( m_nIdxMatchingMap == -1 )
        return NULL;

    assert( m_nIdxMatchingMap < m_nCnt  );

    return m_pMapInfo[m_nIdxMatchingMap].m_pIHTMLMapElement;
}


//-----------------------------------------------------------------------
//  CTOMMapMgr::MatchMapName()
//
//  DESCRIPTION:
//   
//  Determines which MAP in the document.all.tags("MAP") collection
//  has the same NAME attribute as the input string.
//
//  PARAMETERS:
//
//  lpWStrMapName                   Wide character string containing the
//                                  name of the desired map.
//
//  RETURNS:
//
//  HRESULT                         S_OK on success,
//                                  S_FAIL if no COM failures but no
//                                      matching map, or
//                                  standard COM error code.
//-----------------------------------------------------------------------

HRESULT CTOMMapMgr::MatchMapName( /* in */  LPWSTR lpWStrMapName )
{
    HRESULT     hr;


    assert( lpWStrMapName );


    m_nIdxMatchingMap = -1;


    if ( !m_nCnt )
        if ( hr = getTOMMaps() )
            return hr;


    hr = S_FALSE;

    for ( int i = 0; i < m_nCnt; i++ )
    {
        if ( !m_pMapInfo[i].m_bstrMapName )
        {
            if ( hr = m_pMapInfo[i].m_pIHTMLMapElement->get_name( &m_pMapInfo[i].m_bstrMapName ) )
                return hr;
        }

        if ( !_wcsicmp( m_pMapInfo[i].m_bstrMapName, lpWStrMapName ) )
        {
            m_nIdxMatchingMap = i;
            hr = S_OK;
            break;
        }
    }

    return hr;
}


//=======================================================================
//  CTOMMapMgr class : protected methods
//=======================================================================

//-----------------------------------------------------------------------
//  CTOMMapMgr::getTOMMaps()
//
//  DESCRIPTION:
//   
//  Uses the member all collection pointer to build an array of
//  IHTMLMapElement pointer, each item of which point to one of the
//  TOM document's MAP objects.  This array effectively caches the
//  MAP object pointers.
//
//  PARAMETERS:
//
//  None.
//
//  RETURNS:
//
//  HRESULT         S_OK on success or standard COM error code.
//-----------------------------------------------------------------------

HRESULT CTOMMapMgr::getTOMMaps( void )
{
    HRESULT         hr = S_OK;
    int             i = 0;
    VARIANT         varMap;
    ULONG           ulNumFetched = 0;

    IDispatch*                  pIDispatch = NULL;
    IHTMLElementCollection*     pMapCollection = NULL;
    IUnknown*                   pIUnk = NULL;
    IEnumVARIANT*               pIEnumVARIANT = NULL;
    VARIANT*                    pVars = NULL;

    assert( m_pIHTMLElementCollectionAll );

    varMap.vt = VT_BSTR;
    varMap.bstrVal = SysAllocString( L"MAP" );

    hr = m_pIHTMLElementCollectionAll->tags( varMap, &pIDispatch );

    SysFreeString( varMap.bstrVal );

    if ( hr != S_OK )
        goto CleanUpAndReturn;
    assert( pIDispatch );


    if ( hr = pIDispatch->QueryInterface( IID_IHTMLElementCollection, (void**) &pMapCollection ) )
        goto CleanUpAndReturn;
    assert( pMapCollection );


    if ( hr = pMapCollection->get_length( &m_nCnt ) )
        goto CleanUpAndReturn;


    if ( hr = pMapCollection->get__newEnum( &pIUnk ) )
        goto CleanUpAndReturn;
    assert( pIUnk );


    if ( hr = pIUnk->QueryInterface( IID_IEnumVARIANT, (void**) &pIEnumVARIANT ) )
        goto CleanUpAndReturn;
    assert( pIEnumVARIANT );


    if ( !( pVars = new VARIANT[ m_nCnt ] ) )
    {
        hr = E_OUTOFMEMORY;
        goto CleanUpAndReturn;
    }


    if ( hr = pIEnumVARIANT->Next( m_nCnt, pVars, &ulNumFetched ) )
    {
        //--------------------------------------------------
        //  If Next() returns S_FALSE, it means that we
        //  weren't given the complete set of MAPs.
        //--------------------------------------------------

        if ( hr == S_FALSE )
        {
            hr = E_UNEXPECTED;
            m_nCnt = ulNumFetched;
        }
        goto CleanUpAndReturn;
    }


    if ( !( m_pMapInfo = new _TOMMapInfo[ m_nCnt ] ) )
    {
        hr = E_OUTOFMEMORY;
        goto CleanUpAndReturn;
    }

    ZeroMemory( m_pMapInfo, sizeof( _TOMMapInfo ) * m_nCnt );


    for ( i = 0; i < m_nCnt; i++ )
    {
        if ( hr = pVars[i].pdispVal->QueryInterface( IID_IHTMLMapElement, (void**) &m_pMapInfo[i].m_pIHTMLMapElement ) )
            goto CleanUpAndReturn;

        assert( m_pMapInfo[i].m_pIHTMLMapElement );
    }


CleanUpAndReturn:

    if ( hr != S_OK )
    {
        if ( m_pMapInfo )
        {
            for ( int j = 0; j < i; j++ )
                if ( m_pMapInfo[j].m_pIHTMLMapElement )
                    m_pMapInfo[j].m_pIHTMLMapElement->Release();

            delete [] m_pMapInfo;
            m_pMapInfo = NULL;
        }

        m_nCnt = 0;
    }

    if ( pVars )
    {
        for ( i = 0; i < m_nCnt; i++ )
            if ( pVars[i].pdispVal )
                pVars[i].pdispVal->Release();
        delete [] pVars;
    }

    if ( pIEnumVARIANT )
        pIEnumVARIANT->Release();

    if ( pIUnk )
        pIUnk->Release();

    if ( pMapCollection )
        pMapCollection->Release();

    if ( pIDispatch )
        pIDispatch->Release();


    return hr;
}

//=======================================================================
//  CAOMMgr class : public methods
//=======================================================================

//-----------------------------------------------------------------------
//  CAOMMgr::CAOMMgr()
//
//  DESCRIPTION:
//  constructor
//
//  PARAMETERS:
//  pRootAO - the root of this tree.
//
//  RETURNS:
//  none.
//
// ----------------------------------------------------------------------

CAOMMgr::CAOMMgr(CWindowAO * pRootWindowAO)
{
    assert( pRootWindowAO );

    m_pRootWindowAO = pRootWindowAO;

    //--------------------------------------------------
    // The 0 ID is reserved for top level windows.  Since
    // CWindowAO's create their AOMMgr's, the AOMMgr
    // starts incrementing the IDs from 1.
    //--------------------------------------------------

    m_nCurrentAOMItemID     = 1;
    
    m_iBuildState           = IFULLCHILDBUILDING;
    m_PtHitTest.x           = IPOINT_UNINITIALIZED;
    m_PtHitTest.y           = IPOINT_UNINITIALIZED;
    m_lFocusID              = INOOBJECTFOCUS;
    
    m_itCurPtHitListPos     = NULL;

    m_pVars                 = NULL;
    m_lCurSizeVariantArray  = 0;

    m_pTOMMapMgr            = NULL;

    //--------------------------------------------------
    // these cached pointers will be set in the 
    // Init() method.
    //--------------------------------------------------

    m_pIHTMLDocument2       = NULL;
    m_pIHTMLAllCollection   = NULL;

#ifdef __AOMMGR_ENABLETEXT

    m_pIHTMLTxtRangeParent  = NULL;
    m_pIHTMLTxtRangeChild   = NULL;
    m_pIHTMLTxtRangeTemp    = NULL;

    m_bstrStartToStart      = NULL;
    m_bstrStartToEnd        = NULL;
    m_bstrEndToStart        = NULL;
    m_bstrEndToEnd          = NULL;

#endif //__AOMMGR_ENABLETEXT

    
    //--------------------------------------------------
    // initialize the current map index : used in 
    // building image maps.
    //--------------------------------------------------

    m_lCurrentMapIndex = INOCURRENTMAPINDEX;
}



//-----------------------------------------------------------------------
//  CAOMMgr::~CAOMMgr()
//
//  DESCRIPTION:
//
//  destructor. The one thing done here is that the hit path
//  is cleared (because memory is allocated to build it)
//
//  PARAMETERS:
//
//  none.
//
//  RETURNS:
//
//  S_OK | standard COM error code.
// ----------------------------------------------------------------------

CAOMMgr::~CAOMMgr()
{
    if ( m_pTOMMapMgr )
    {
        delete m_pTOMMapMgr;
        m_pTOMMapMgr = NULL;
    }

    if ( m_pVars )
    {
        delete [] m_pVars;
        m_pVars = NULL;
    }
    
    //--------------------------------------------------
    // release cached pointers.
    //--------------------------------------------------

    ReleaseTridentInterfaces();


#ifdef __AOMMGR_ENABLETEXT
    
    if ( m_bstrStartToStart )
    {
        SysFreeString( m_bstrStartToStart );
        m_bstrStartToStart = NULL;
    }

    if ( m_bstrStartToEnd )
    {
        SysFreeString( m_bstrStartToEnd );
        m_bstrStartToEnd = NULL;
    }

    if ( m_bstrEndToStart )
    {
        SysFreeString( m_bstrEndToStart );
        m_bstrEndToStart = NULL;
    }

    if ( m_bstrEndToEnd )
    {
        SysFreeString( m_bstrEndToEnd );
        m_bstrEndToEnd = NULL;
    }

#endif //__AOMMGR_ENABLETEXT

}


//-----------------------------------------------------------------------
//  CAOMMgr::ReleaseTridentInterfaces()
//
//  DESCRIPTION: Calls release on all CAOMMgr-specific cached Trident
//               object interface pointers.
//
//  PARAMETERS: None
//      
//  RETURNS: None
//
// ----------------------------------------------------------------------

void 
CAOMMgr::ReleaseTridentInterfaces ()
{
    unsetHitList( &m_PtHitList );

    if ( m_pIHTMLAllCollection )
    {
        m_pIHTMLAllCollection->Release();
        m_pIHTMLAllCollection = NULL;
    }

    if ( m_pIHTMLDocument2 )
    {
        m_pIHTMLDocument2->Release();
        m_pIHTMLDocument2 = NULL;
    }

    if (m_pTOMMapMgr)
        m_pTOMMapMgr->Zombify();

#ifdef __AOMMGR_ENABLETEXT

    if ( m_pIHTMLTxtRangeParent )
    {
        m_pIHTMLTxtRangeParent->Release();
        m_pIHTMLTxtRangeParent = NULL;
    }

    if ( m_pIHTMLTxtRangeChild )
    {
        m_pIHTMLTxtRangeChild->Release();
        m_pIHTMLTxtRangeChild = NULL;
    }
    
    if ( m_pIHTMLTxtRangeTemp )
    {
        m_pIHTMLTxtRangeTemp->Release();
        m_pIHTMLTxtRangeTemp = NULL;
    }

#endif //__AOMMGR_ENABLETEXT
}


//-----------------------------------------------------------------------
//  CAOMMgr::Init()
//
//  DESCRIPTION:
//
//  caches several pointers that are frequently used - 
//  (1) ptr to IHTMLDocument2
//  (2) ptr to ALL collection.
//  (3) ptrs to text ranges used to find and create text objects.
//
//  NOTE: these pointers must be released in destructor.
// 
//  PARAMETERS:
//
//  pIHTMLDocument2     pointer to document pointer : lock this down,.
//
//  RETURNS:
//
//  S_OK | standard COM error.
// ----------------------------------------------------------------------

HRESULT 
CAOMMgr::Init(IHTMLDocument2 * pIHTMLDocument2)
{
    HRESULT hr = E_FAIL;
    
    //--------------------------------------------------
    // validate input
    //--------------------------------------------------

    if(!pIHTMLDocument2)
        return(E_INVALIDARG);

    assert(pIHTMLDocument2);

    m_pIHTMLDocument2 = pIHTMLDocument2;

    //--------------------------------------------------
    // lock this pointer down to insure that it is valid
    // for the lifetime of CAOMMgr
    //--------------------------------------------------

    m_pIHTMLDocument2->AddRef();

    //--------------------------------------------------
    // pointer to the all collection is used when walking
    // up parent chain, in order to get pointers to 
    // valid elements.  See CAOMMgr::getNormalizedParent()
    //--------------------------------------------------  

    if(hr = m_pIHTMLDocument2->get_all( &m_pIHTMLAllCollection ))
        return(hr);
    
    if(!m_pIHTMLAllCollection)
        return(E_NOINTERFACE);


    if ( hr = reallocateVariantArray( AOMMGR_DEFAULTVARIANTARRAYSIZE ) )
        return hr;


#ifdef __AOMMGR_ENABLETEXT

    //--------------------------------------------------
    // create and cache the three text ranges that 
    // will be used to find and create Text AOs.
    //--------------------------------------------------

    if(hr = getDocumentTxtRanges(&m_pIHTMLTxtRangeParent,&m_pIHTMLTxtRangeChild,&m_pIHTMLTxtRangeTemp) )
    {

        //--------------------------------------------------
        // S_FALSE means that this window contains a 
        // frameset, and therefore has no body, and 
        // no text.  Text ranges were not created 
        // because they will never be needed.
        //
        //--------------------------------------------------
        
        if(hr != S_FALSE)
            return(hr);
    }

    if ( hr = initializeTextRangeBSTRs() )
        return hr;

#endif

    return(S_OK);

}

//-----------------------------------------------------------------------
//  CAOMMgr::GetAccessibleObjectFromPt()
//
//  DESCRIPTION:
//  This method builds the tree down to the next object that encapsulates
//  the input point. If none of the object's children contain the point, or
//  if the object doesn't have any children, the returned object will match
//  the input object.
//
//  PARAMETERS:
//
//  pAOToResolve        node to build under
//  pPtHitTest          point to build towards
//  ppRetAccEl          pointer to hit object to return to caller.          
//
//  RETURNS:
//
//  HRESULT S_OK | E_FAIL 
// ----------------------------------------------------------------------

HRESULT 
CAOMMgr::GetAccessibleObjectFromPt( /* in */    CTridentAO * pAOToResolve, 
                                    /* in */    POINT * pPtHitTest, 
                                    /* out */   CAccElement ** ppRetAccEl)
{
    HRESULT hr;

    assert(pAOToResolve);
    assert(pPtHitTest);
    assert(ppRetAccEl);


    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    if( !(pAOToResolve) || !(pPtHitTest) || !(ppRetAccEl) )
        return(E_INVALIDARG);

    assert(m_pRootWindowAO);

    //--------------------------------------------------
    // Evaluate the current build state prior to this
    // resolution, and determine (1) if it needs to
    // change and (2) what needs to be done if it is 
    // changing.
    //--------------------------------------------------

    switch(m_iBuildState)
    {
    case IHITTESTBUILDING:

        //--------------------------------------------------
        // if the current point doesn't equal the input
        // point, that means that AccHitTest() has changed.
        // We need to regenerate a path for the new point.
        //--------------------------------------------------

        if(ptHasChanged(*pPtHitTest))
        {
            if(hr = setHitList(*pPtHitTest,&m_PtHitList))
            {
                return(hr);
            }

            //--------------------------------------------------
            // set up m_itCurPtHitListPos as pointer to first element
            // in chain.
            //--------------------------------------------------

            m_itCurPtHitListPos = m_PtHitList.begin();

            //--------------------------------------------------
            // save point for later evaluation.
            // Instead of making a cross process call to 
            // check the hit element, this is a cheaper method
            // of comparison.
            //--------------------------------------------------
            
            m_PtHitTest.x = pPtHitTest->x;
            m_PtHitTest.y = pPtHitTest->y;

        }
        else
        {

            //--------------------------------------------------
            // check to see if this is the hit element : if it 
            // is, there is no need to build children
            //--------------------------------------------------

            CComPtr<IHTMLElement> pIHTMLElement;

            if(hr = getIHTMLElement(pAOToResolve,&pIHTMLElement))
            {
                return(hr);
            }

            assert(pIHTMLElement);

            
            BOOL bHit = FALSE;


            //--------------------------------------------------
            // TODO: evaluate whether this should be moved to
            // debug only. Right now, with text enabled,
            // it doesnt save us any time.. 
            //--------------------------------------------------

            //--------------------------------------------------
            // reset current hit pointer to top of list in 
            // order to make sure that this element resides 
            // somewhere on the list.  the current hit pointer
            // will be reset to this element if this element
            // resides on the list.
            //--------------------------------------------------

            m_itCurPtHitListPos = m_PtHitList.begin();

            if(hr = isElementMarked(pIHTMLElement,&m_PtHitList,&m_itCurPtHitListPos,&bHit))
            {

                //--------------------------------------------------
                // if the element is not in the hit list, and we are 
                // drilling down,there is a logic error.  Usually, 
                // the logic error indicates that we still think we 
                // are hit testing on a point even after that point 
                // has been found. The build state has not been reset, 
                // and the new element is not being found in the old 
                // hit list.
                //
                // reset the hit list upon finding the point, and 
                // this assertion will go away.  This assertion 
                // can manifest itself in basic MSAA functionality
                // failing (during release builds).
                //--------------------------------------------------

                assert(!hr);
                return(hr);
                
            }
            else
            {    
                if(bHit)
                {
                    //--------------------------------------------------
                    // if the element is NOT a parent of the hit 
                    // element, it MUST be the element.  return here
                    // (and save some time)
                    //--------------------------------------------------

                    *ppRetAccEl = pAOToResolve;
                    
#ifdef __AOMMGR_ENABLETEXT

                    
                    //--------------------------------------------------
                    // if this object corresponds to the FRAMESET tag,
                    // it wont have a body element on it. This is fine:
                    // just means that the object can have no text.
                    //--------------------------------------------------

                    CComPtr<IHTMLElement> pIHTMLElement;

                    if(hr = getIHTMLElement(pAOToResolve,&pIHTMLElement))
                    {
                        return(hr);
                    }


                    CComQIPtr<IHTMLFrameSetElement,&IID_IHTMLFrameSetElement> pIHTMLFrameSetElement(pIHTMLElement);

                    
                    if(pIHTMLFrameSetElement)
                    {
                        //--------------------------------------------------
                        // no more procesing to be done on this frameset.
                        //--------------------------------------------------

                        return(S_OK);
                    }

#else

                    return(S_OK);

#endif  // __AOMMGR_ENABLETEXT

                }

            }
        }

        break;
    
    case IFULLCHILDBUILDING:
    case IFOCUSEDOBJECTBUILDING:
    
        //--------------------------------------------------
        //  a passed in point means we switch build state to
        //  HITTESTBUILDING.  
        //  (1) create marked path based on input point
        //  (2) switch state
        //--------------------------------------------------
        
        if(hr = setHitList(*pPtHitTest,&m_PtHitList))
        {
            return(hr);
        }

        //--------------------------------------------------
        // set up m_itCurPtHitListPos as pointer to first element
        // in chain.
        //--------------------------------------------------

        m_itCurPtHitListPos = m_PtHitList.begin();

        //--------------------------------------------------
        // save the point for later evaluation.
        // Instead of making a cross process call to 
        // check the hit element, this is a cheaper method
        // of comparison.
        //--------------------------------------------------

        m_PtHitTest.x = pPtHitTest->x;
        m_PtHitTest.y = pPtHitTest->y;

        m_lFocusID = INOOBJECTFOCUS;

        m_iBuildState = IHITTESTBUILDING;

        break;

    default:
        
        return(E_FAIL);
    }

    //--------------------------------------------------
    // now the build state has been correctly set.  If
    // the current object doesn't have a child list,
    // then build the tree.
    //--------------------------------------------------


    if(!(pAOToResolve->HasChildren() ))
    {

        //--------------------------------------------------
        //  if there are no children, get an initial list.
        //  **NOTE** this list will be the unparsed list
        //  of all trident children, both supported and 
        //  unsupported.  
        //--------------------------------------------------
        
        if(hr = getAOMChildren(pAOToResolve))
        {

            //--------------------------------------------------
            // no standard children were built : continue
            // looking for text children.
            //--------------------------------------------------

            if(hr != S_FALSE)
            {
                return(hr);
            }
        }

    }

    //--------------------------------------------------
    // Now the tree has been built : 
    // find out which AOM object encapsulates the point.
    // **NOTE** this may involve more building.
    //--------------------------------------------------

    if(hr = getHitAOMChild(pAOToResolve,&m_PtHitList,&m_itCurPtHitListPos,NULL,ppRetAccEl))
    {

        //--------------------------------------------------
        // S_FALSE means that the containing object encapsulates 
        // the point, but none of its standard children do.
        // at this point, we need to see if the containing 
        // object contains any text children, and if any of
        // those text children contain the point.
        //--------------------------------------------------

        if(hr == S_FALSE)
        {
            *ppRetAccEl = (CAccElement *)pAOToResolve;
            
#ifdef __AOMMGR_ENABLETEXT

            //--------------------------------------------------
            // we need to fully resolve the child list
            // to get all text.
            //--------------------------------------------------

            if(hr = fullyResolveChildren(pAOToResolve))
            {
                return(hr);
            }

            
            //--------------------------------------------------
            // if the passed in point resides in any of the 
            // text children, return that text child.
            //--------------------------------------------------

            if(hr = pAOToResolve->IsPointInTextChildren(m_PtHitTest.x,
                                                        m_PtHitTest.y,
                                                        m_pIHTMLTxtRangeTemp,
                                                        ppRetAccEl))
            {
                if(hr == S_FALSE)
                {
                    *ppRetAccEl = pAOToResolve;
                }
                else
                {
                    return(hr);
                }
            }

            assert(*ppRetAccEl);

#endif // _AOMMGR_ENABLETEXT

        }
        else
        {
            *ppRetAccEl = NULL;
            return(hr);

        }
    }

    //--------------------------------------------------
    // should always have something here :
    // at this point, we need to apply our tree rules
    // to the object -- i.e. move the object to the 
    // correct position in the tree if it is in an
    // invalid position in the tree.
    //--------------------------------------------------    
    
    CAccElement *pNewAccEl = NULL;

    if(hr = enforceAOMTreeRules(pAOToResolve,*ppRetAccEl,&pNewAccEl))
    {
        return(hr);
    }
    
    //--------------------------------------------------
    // if for some reason the hit object has been
    // duped later on down the tree, get the 
    // new hit object.
    //--------------------------------------------------

    if(pNewAccEl)
    {
        if(*ppRetAccEl != pNewAccEl)
        {
            *ppRetAccEl = pNewAccEl;
        }
    }

    //--------------------------------------------------
    // now we need
    // to find out if the object we are returning
    // is the last hit element.  
    //
    // if hit element test passes, 
    // reset the point value of the hit test so 
    // that concurrent hit tests don't use the same 
    // hit list. The list itself will be reset in
    // the next SetHitList().
    //--------------------------------------------------


    if(hr = isHitObject(pAOToResolve,*ppRetAccEl))
    {
        if(hr != S_FALSE)
            return(hr);
    }
    else
    {
        m_PtHitTest.x = IPOINT_UNINITIALIZED;
        m_PtHitTest.y = IPOINT_UNINITIALIZED;
    }


    assert(*ppRetAccEl);

    return(S_OK);
    
}


//-----------------------------------------------------------------------
//  CAOMMgr::GetAccessibleObjectFromID()
//
//  DESCRIPTION:
//
//  builds tree/navigates tree down to obj whose ID matches 
//  the requested ID. If no such ID, then we return S_FALSE.
//
//  PARAMETERS:
//
//  lObjectID       ID of Trident element to build tree down to.
//  ppRetAccEl      pointer to return Accessible Object in.
//
//  RETURNS:
//
//  S_OK if good, S_FALSE if no element, else standard COM error.
//
//  NOTE: 
//  right now this method is only valid if the CTridentAO that is passed 
//  in is the CWindowAO ROOT of the tree.  This seems to be the only place
//  that we would want to start building the tree from in response to an
//  ID request.  If this is not the case, more validation needs to be done
//  when setting up the hit list in order to make sure that the passed in 
//  CTridentAO is actually on the hit list. This sort of validation is done
//  in GetAccessibleObjectFromPt().
// ----------------------------------------------------------------------

HRESULT CAOMMgr::GetAccessibleObjectFromID( /* in */    CTridentAO *pStartAO,   
                                            /* in */    long lObjectID, 
                                            /* out */   CAccElement **ppAE)
{
    HRESULT hr = E_FAIL;
    int iPreviousBuildState = 0;

    std::list<IHTMLElement *>::iterator itCurhitListPos;
    std::list<IHTMLElement *>::iterator itEndhitListPos;
    CTridentAO*     pCurrentAO;
    CAccElement*    pHitAE;
    CAccElement*    pNewAccEl;

#ifdef _DEBUG
    std::list<IHTMLElement *>::iterator itOldhitListPos;
    int iSize;
    IHTMLElement*   pIHTMLElement;
#endif

    //--------------------------------------------------
    // validate parmeters: 
    // no non zero objects (should have been assigned
    // to window already)
    //
    // return pointer must exist.
    //--------------------------------------------------

    if(!lObjectID)
        return S_FALSE;

    if(!ppAE)
        return E_INVALIDARG;

    if (pStartAO->IsDetached())
        return E_FAIL;


    //--------------------------------------------------
    // set the internal build state to IHITTESTBUILDING
    // no matter what : but reset it to the previous
    // state upon leaving the method.
    //--------------------------------------------------

    iPreviousBuildState = m_iBuildState;

    m_iBuildState = IHITTESTBUILDING;
    

    //--------------------------------------------------
    // set the hit list (ID to build to and its 
    // parent chain) so that tree can build in 
    // the correct direction.
    //--------------------------------------------------

    std::list<IHTMLElement *> hitList;

    if(hr = setHitList(lObjectID, &hitList))
    {
        m_iBuildState = iPreviousBuildState;
        goto CleanupAndReturn;
    }

    
    itCurhitListPos = hitList.begin();
    itEndhitListPos = hitList.end();
    itEndhitListPos--;

    
#ifdef _DEBUG

    itOldhitListPos = itCurhitListPos;

    iSize = hitList.size();

    assert(iSize);

#endif

    pCurrentAO = pStartAO;

    //--------------------------------------------------
    // start at the earliest grandparent of the list
    // and walk down parent by parent, to the hit item.
    // build the tree along the way.
    //--------------------------------------------------

    while(itCurhitListPos != itEndhitListPos)
    {
        
#ifdef _DEBUG
        
        itOldhitListPos = itCurhitListPos;
#endif
        //--------------------------------------------------
        // build children if none exist
        //--------------------------------------------------

        if(!pCurrentAO->HasChildren())
        {

            if(hr = getAOMChildren(pCurrentAO))
            {
                m_iBuildState = iPreviousBuildState;
                goto CleanupAndReturn;
            }

        }
    
        //--------------------------------------------------
        // get the next hit child 
        //--------------------------------------------------

        if(hr = getHitAOMChild(pCurrentAO,&hitList,&itCurhitListPos,NULL,&pHitAE))
        {
            m_iBuildState = iPreviousBuildState;
            goto CleanupAndReturn;
        }

        //--------------------------------------------------
        // should always have something here :
        // at this point, we need to apply our tree rules
        // to the object -- i.e. move the object to the 
        // correct position in the tree if it is in an
        // invalid position in the tree.
        //--------------------------------------------------    
        
        pNewAccEl = NULL;

        if(hr = enforceAOMTreeRules(pCurrentAO,pHitAE,&pNewAccEl))
        {
            m_iBuildState = iPreviousBuildState;
            goto CleanupAndReturn;
        }
        
        //--------------------------------------------------
        // if for some reason the hit object has been
        // duped later on down the tree, get the 
        // new hit object.
        //--------------------------------------------------

        if(pNewAccEl)
        {
            if(pHitAE != pNewAccEl)
            {
                pHitAE = pNewAccEl;
            }
        }

        //--------------------------------------------------
        // assign the current parent ptr to the hit child
        // to drill down one more level. The hit list
        // pos should have been incremented in getHitAOMChild()
        //--------------------------------------------------

#ifdef _DEBUG
        
        assert(itOldhitListPos != itCurhitListPos);

#endif
        pCurrentAO = (CTridentAO *)pHitAE;
    }


    //--------------------------------------------------
    // this should be the hit element
    //--------------------------------------------------

#ifdef _DEBUG

    //--------------------------------------------------
    // confirm that this is the hit element, and that 
    // we have gone all the way down the hit list.
    //--------------------------------------------------

    assert(pHitAE);

    pIHTMLElement = NULL;
    assert( (hr = getIHTMLElement(pHitAE,&pIHTMLElement)) == S_OK );
    assert( compareIHTMLElements( *itCurhitListPos, pIHTMLElement ) == S_OK );

    if ( pIHTMLElement )
        pIHTMLElement->Release();

#endif
    
    *ppAE = pHitAE; 

    //--------------------------------------------------
    // reset state, return current HRESULT.
    //--------------------------------------------------

    m_iBuildState = iPreviousBuildState;


CleanupAndReturn:
    unsetHitList( &hitList );

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::GetFocusedObjectFromID()
//
//  DESCRIPTION:
//
//  This method builds the tree down the focus chain, one level at a
//  time.  The object returned will be a child of pStartAO.  This object
//  will either have the focus or will be an ancestor of the actual
//  focused object.
//
//  PARAMETERS:
//
//  pStartAO            pointer to the AO to check for focus
//  lFocusedObjectTOMID TOM ID of the TEO that has the focus
//  ppAE                pointer to the pointer of the focused object
//
//  RETURNS:
//
//  S_OK if pStartAO is on the focus hit list and if it or one of its
//      descendants has the focus.
//  S_FALSE if pStartAO is not on the focus hit list.
//  Standard COM error codes on error.
//
// ----------------------------------------------------------------------

HRESULT CAOMMgr::GetFocusedObjectFromID(    /* in */    CTridentAO* pStartAO,
                                            /* in */    long lFocusedObjectTOMID,
                                            /* out */   CAccElement** ppAE )
{
    HRESULT hr = E_FAIL;
    BOOL    bFound = FALSE;
    BOOL    bSupported = FALSE;


    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    assert( pStartAO );
    assert( ppAE );

    if( !(pStartAO) || !(ppAE) )
        return E_INVALIDARG;


    //--------------------------------------------------
    // initialize the outputs.
    //--------------------------------------------------

    *ppAE = NULL;


    //--------------------------------------------------
    //  Check to see if the input AO is the focused
    //  object.  If it is, we're done.
    //--------------------------------------------------

    if ( pStartAO->GetTOMAllIndex() == lFocusedObjectTOMID )
    {
        *ppAE = (CAccElement*) pStartAO;
        return S_OK;
    }


    //--------------------------------------------------
    // Evaluate the current build state prior to this
    // resolution, and determine (1) if it needs to
    // change and (2) what needs to be done if it is 
    // changing.
    //--------------------------------------------------

    switch ( m_iBuildState )
    {
    case IHITTESTBUILDING:
    case IFULLCHILDBUILDING:
    
        m_PtHitTest.x   = IPOINT_UNINITIALIZED;
        m_PtHitTest.y   = IPOINT_UNINITIALIZED;
        m_lFocusID      = INOOBJECTFOCUS;
        m_iBuildState   = IFOCUSEDOBJECTBUILDING;

    case IFOCUSEDOBJECTBUILDING:

        //--------------------------------------------------
        //  If the current focus TOM ID doesn't equal the
        //  input ID, we need to generate a new focus
        //  hit list for the new ID.
        //--------------------------------------------------

        if ( m_lFocusID != lFocusedObjectTOMID )
        {
            if ( hr = setFocusHitList( lFocusedObjectTOMID, &m_PtHitList ) )
            {
                return hr;
            }

            //--------------------------------------------------
            // save input ID for later evaluation.
            //--------------------------------------------------
            
            m_lFocusID = lFocusedObjectTOMID;
        }

        break;
    
    default:
        
        return(E_FAIL);
    }


    //--------------------------------------------------
    //  Set (or reset) m_itCurPtHitListPos to point to
    //  the first item in the focus hit list.
    //--------------------------------------------------

    m_itCurPtHitListPos = m_PtHitList.begin();


    //--------------------------------------------------
    //  Determine if pStartAO is in the focus hit list.
    //  If not, return S_FALSE as neither pStartAO nor
    //  any of its children will have the focus.
    //--------------------------------------------------

    bFound = FALSE;
    while ( m_itCurPtHitListPos != m_PtHitList.end() && !bFound )
    {
        hr = compareIHTMLElements( pStartAO, *m_itCurPtHitListPos );

        if ( FAILED( hr ) )
            return hr;
        else if ( hr == S_OK )
            bFound = TRUE;

        m_itCurPtHitListPos++;
    }


    if ( !bFound )
        return S_FALSE;


    //--------------------------------------------------
    //  If we are at the end of the focus hit list,
    //  something is wrong--at least a logic error.
    //  For bFound to be TRUE and for us to be at the
    //  end of the list, that would mean that pStartAO
    //  has the focus, but we checked for that case
    //  prior to starting the loop!
    //--------------------------------------------------

    assert( m_itCurPtHitListPos != m_PtHitList.end() );

    if ( m_itCurPtHitListPos == m_PtHitList.end() )
        return E_FAIL;


    //--------------------------------------------------
    //  Ensure that the next TEO in the focus hit list
    //  is supported.
    //--------------------------------------------------

    bSupported = FALSE;
    while ( m_itCurPtHitListPos != m_PtHitList.end() && !bSupported )
    {
        ULONG   ulAOMItemType;

        if ( hr = setTEOType( (*m_itCurPtHitListPos), &ulAOMItemType ) )
            return hr;

        if ( ulAOMItemType == AOMITEM_NOTSUPPORTED )
            m_itCurPtHitListPos++;
        else
            bSupported = TRUE;
    }


    //--------------------------------------------------
    //  Again, if we are at the end of the focus hit
    //  list, something is wrong--at least a logic
    //  error--because we support all TEOs that can
    //  have the focus!
    //
    //  BUGBUG!  POTENTIAL IE5 CHANGE!
    //  Trident for IE5 may add more focusable element
    //  objects.  If this happens, we need to either
    //  create AOM items for those TEOs or change this
    //  algorithm.
    //--------------------------------------------------

    assert( m_itCurPtHitListPos != m_PtHitList.end() );

    if ( m_itCurPtHitListPos == m_PtHitList.end() )
        return E_FAIL;


    //--------------------------------------------------
    //  If the current object doesn't have a child list,
    //  then build it.
    //--------------------------------------------------

    if ( !(pStartAO->HasChildren()) )
    {
        //--------------------------------------------------
        //  If there are no children, get an initial list.
        //  **NOTE** This list will be the unparsed list
        //  of all trident children, both supported and 
        //  unsupported.  A fully resolved child list 
        //  can only contain supported children.
        //--------------------------------------------------
        
        if ( hr = getAOMChildren( pStartAO ) )
        {
            //--------------------------------------------------
            //  If getAOMChildren() returns S_FALSE, there were 
            //  no standard children to build.  However, there
            //  may still be text children, so continue on
            //  to fullyResolveChildren().
            //--------------------------------------------------

            if ( hr != S_FALSE )
                return hr;
        }
    }

    //--------------------------------------------------
    //  The child list has been built so resolve it.
    //--------------------------------------------------

    if ( hr = fullyResolveChildren( pStartAO ) )
        return hr;


    //--------------------------------------------------
    //  Search pStartAO's child list for the child that
    //  is also in the focus hit list.  We should find
    //  a match as we know pStartAO is on the list.
    //--------------------------------------------------

    std::list<CAccElement *>* pChildList;

    if ( hr = pStartAO->GetChildList( &pChildList ) )
        return hr;


    std::list<CAccElement *>::iterator itCurChildListPos = pChildList->begin();

    bFound = FALSE;
    while ( itCurChildListPos != pChildList->end() && !bFound )
    {
        hr = compareIHTMLElements( *itCurChildListPos, *m_itCurPtHitListPos );

        if ( FAILED( hr ) )
            return hr;
        else if ( hr == S_OK )
            bFound = TRUE;

        if ( !bFound )
            itCurChildListPos++;
    }

    if ( !bFound )
        return E_FAIL;


    assert( itCurChildListPos != pChildList->end() );


    //--------------------------------------------------
    //  We have found the child of pStartAO that is
    //  on the focus hit list.
    //--------------------------------------------------

    *ppAE = *itCurChildListPos;


    return S_OK;
}


//-----------------------------------------------------------------------
//  CAOMMgr::ResolveChildren()
//
//  DESCRIPTION:
//  builds the entire child list of the input object, fully resolving 
//  the child list by discarding unsupported objects.
//
//  PARAMETERS:
//
//  pAOToResolve    pointer to object to resolve the child list of.
//
//  RETURNS:
//
//  S_OK | standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::ResolveChildren(CTridentAO * pAOToResolve)
{
    HRESULT hr;

    assert(pAOToResolve);
    
    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    if( !(pAOToResolve) )
        return(E_FAIL);

    assert(m_pRootWindowAO);

    
    //--------------------------------------------------
    // reset build state.
    //--------------------------------------------------

    if ( m_iBuildState != IFULLCHILDBUILDING )
        ResetState();


    //--------------------------------------------------
    // now the build state has been correctly set.  If
    // the current object doesn't have a child list,
    // then build the tree.
    //--------------------------------------------------


    if(!(pAOToResolve->HasChildren() ))
    {

        //--------------------------------------------------
        //  if there are no children, get an initial list.
        //  **NOTE** this list will be the unparsed list
        //  of all trident children, both supported and 
        //  unsupported.  A fully resolved child list 
        //  can only contain supported children.
        //--------------------------------------------------
        
        if(hr = getAOMChildren(pAOToResolve))
        {

            //--------------------------------------------------
            // an S_FALSE return code means that there were 
            // no standard children to build. However,
            // there may still be text, so continue on
            // to fullyResolveChildren().
            //--------------------------------------------------

            if(hr != S_FALSE)
            {
                return(hr);
            }
            
        }

    }

    //--------------------------------------------------
    // now that a child list has been built, resolve the
    // children.
    //--------------------------------------------------

    if(hr = fullyResolveChildren(pAOToResolve))
        return(hr);

    return(S_OK);
}


//=======================================================================
//  CAOMMgr class : protected methods
//=======================================================================

//-----------------------------------------------------------------------
//  CAOMMgr::setFocusHitList()
//
//  DESCRIPTION:
//
//  generates a focus hit list from an input ID
//
//  PARAMETERS:
//
//  lObjectID   - ID value.
//  pList       - pointer to list to fill out.
//
//  RETURNS:
//
//  S_OK | standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::setFocusHitList(   /* in */    long lObjectID, 
                                    /* out */   std::list<IHTMLElement *> *pList )
{
    HRESULT hr = E_FAIL;

    CComPtr<IHTMLElement> pIHTMLElement;


    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( lObjectID );
    assert( pList );


    //--------------------------------------------------
    // validate dependencies
    //--------------------------------------------------

    assert( m_pIHTMLDocument2 );


    //--------------------------------------------------
    // get the IHTMLElement that corresponds to the 
    // focused element.
    //--------------------------------------------------

    VARIANT varName;
    VARIANT varIndex;

    VariantInit(&varName);

    VariantInit(&varIndex);
    varIndex.vt = VT_I4;
    varIndex.lVal = lObjectID;

    CComPtr<IDispatch> pIDispatch;

    if ( hr = m_pIHTMLAllCollection->item( varIndex, varName, &pIDispatch ) )
        return hr;
    assert( pIDispatch );
    if ( !pIDispatch )
        return E_NOINTERFACE;

    if ( hr = pIDispatch->QueryInterface( IID_IHTMLElement, (void**)&pIHTMLElement ) )
        return hr;
    assert( pIHTMLElement );
    if ( !pIHTMLElement )
        return E_NOINTERFACE;

    //--------------------------------------------------
    //  Account for an AREA having the focus.
    //
    //  BUGBUG!  IE4 HACK!
    //
    //  There are a couple of known Trident problems
    //  with MAPs and AREAs that will affect how the
    //  focus hit list is to be built when an AREA has
    //  the focus.  (First, like anchors, AREAs don't
    //  show up in document.activeElement.  When an
    //  AREA has the focus, document.activeElement is
    //  actually IMG whose image map is the MAP parent
    //  of the AREA.  Second, a MAP's parent is the
    //  Trident under which it is defined, even if the
    //  MAP is associated with more than one IMG on
    //  the page.)  We need to ensure that our focus
    //  hit list has the correct format:
    //
    //  Window, document, ..., IMG, MAP, AREA
    //
    //  So, if an AREA has the focus, we modify the
    //  focus hit list such that we can easily build
    //  the AOM tree down to the focused AREA.
    //
    //  NOTE: Trident fires FOCUS WinEvents for AREAs.
    //  If we think that an AREA has the focus at this
    //  point, it's because we've received a FOCUS
    //  notification telling us as much.
    //--------------------------------------------------

    CComQIPtr<IHTMLAreaElement,&IID_IHTMLAreaElement> pIHTMLAreaElem( pIHTMLElement );

    if ( pIHTMLAreaElem )
    {
        //--------------------------------------------------
        //  Focused element is an AREA.
        //--------------------------------------------------

        return setAreaFocusedHitList( pIHTMLElement, pList );
    }
    else
    {
        //--------------------------------------------------
        //  Focused element is not an AREA, so build a
        //  standard (focus) hit list down to the focused
        //  element.
        //--------------------------------------------------

        return setHitList( pIHTMLElement, pList );
    }
}


//-----------------------------------------------------------------------
//  CAOMMgr::setAreaFocusedHitList()
//
//  DESCRIPTION:
//
//  Builds a focus hit list when an <AREA> has the focus.  The list from
//  <IMG> up to the <HTML> is built first, then the <MAP> and the <AREA>
//  are added on.
//
//  Reuses setHitList(IHTMLElement*,std::list<IHTMLElement*>*) to build
//  the first part of the list.
//
//
//  PARAMETERS:
//
//  pIHTMLElemArea  IHTMLElement* to the AREA TEO to start hit list from.
//
//  pList           pointer to list to fill out.
//
//
//  RETURNS:
//
//  S_OK | E_NOINTERFACE | other standard COM error
// ----------------------------------------------------------------------

HRESULT CAOMMgr::setAreaFocusedHitList( /* in */    IHTMLElement * pIHTMLElemArea,
                                        /* out */   std::list<IHTMLElement *> *pList )
{
    HRESULT                 hr = E_FAIL;
    CComPtr<IHTMLElement>   pIHTMLElemImg;


    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pIHTMLElemArea );
    assert( pList );


    //--------------------------------------------------
    // validate dependencies
    //--------------------------------------------------

    assert( m_pIHTMLDocument2 );


    //--------------------------------------------------
    //  Get the document's active element.
    //  This should be an IMG.
    //--------------------------------------------------

    if ( hr = m_pIHTMLDocument2->get_activeElement( &pIHTMLElemImg ) )
        return hr;
    if ( !pIHTMLElemImg )
        return E_NOINTERFACE;

    //--------------------------------------------------
    //  Build the focus hit list down to the IMG.
    //--------------------------------------------------

    if ( hr = setHitList( pIHTMLElemImg, pList ) )
        return hr;


    //--------------------------------------------------
    //  Add the AREA's parent, a MAP, to the list.
    //--------------------------------------------------

        //--------------------------------------------------
        //  pIHTMLElemMap is a standard iface ptr, not a
        //  smart COM ptr, because the iface it points to
        //  will be inserted into the focus hit list and
        //  released when unsetHitList() is called.
        //--------------------------------------------------

    IHTMLElement*   pIHTMLElemMap = NULL;

    if ( hr = pIHTMLElemArea->get_parentElement( &pIHTMLElemMap ) )
        return hr;

    assert( pIHTMLElemMap );
    if ( !pIHTMLElemMap )
        return E_NOINTERFACE;

    pList->push_back( pIHTMLElemMap );


    //--------------------------------------------------
    //  Now, finally, insert our focused AREA at the
    //  end of the list.
    //--------------------------------------------------

    pIHTMLElemArea->AddRef();
    pList->push_back( pIHTMLElemArea );


    return S_OK;
}


//-----------------------------------------------------------------------
//  CAOMMgr::setHitList()
//
//  DESCRIPTION:
//  
//  generates a hit list from an input point.
//
//  PARAMETERS:
//
//  pt      - point value.
//  pList   - pointer to list to fill out.
//  
//  RETURNS:
//  
//  S_OK or standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::setHitList(    /* in */    POINT pt, 
                                /* out */   std::list<IHTMLElement *> *pList)
{
    HRESULT hr;


    assert( m_pIHTMLDocument2 );

    assert( pList );


    IHTMLElement*   pIHTMLElement = NULL;

    //--------------------------------------------------
    // get the element under the point.
    //--------------------------------------------------

    hr = m_pIHTMLDocument2->elementFromPoint(pt.x,
                                             pt.y,
                                             &pIHTMLElement);

    if ( hr == S_OK )
    {
        if ( pIHTMLElement )
        {
            hr = normalizeIHTMLElement( &pIHTMLElement );

            if ( hr == S_OK )
            {
                //--------------------------------------------------
                //  Build the hit list to that element.
                //--------------------------------------------------

                hr = setHitList( pIHTMLElement, pList );
            }

            pIHTMLElement->Release();
        }
        else
            hr = S_FALSE;
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::setHitList()
//
//  DESCRIPTION:
//
//  generates a hit list from an input ID
//
//  PARAMETERS:
//
//  lObjectID   - ID value.
//  pList       - pointer to list to fill out.
//
//  RETURNS:
//
//  S_OK | standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::setHitList(    /* in */    long lObjectID, 
                                /* out */   std::list<IHTMLElement *> *pList)
{
    HRESULT hr = E_FAIL;

    CComPtr<IHTMLElement> pIHTMLElement;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------
    
    assert(lObjectID);
    assert(pList);


    //--------------------------------------------------
    // get the IHTMLElement that corresponds to the 
    // hit element.
    //--------------------------------------------------

    assert(m_pIHTMLAllCollection);

    VARIANT varName;

    VariantInit(&varName);

    VARIANT varIndex;
                                                
    VariantInit(&varIndex);

    varIndex.vt = VT_I4;

    varIndex.lVal = lObjectID;

    CComPtr<IDispatch> pIDispatch;

    if(hr = m_pIHTMLAllCollection->item(varIndex,varName,&pIDispatch) )
        return(hr);

    assert( pIDispatch );
    if ( !pIDispatch )
        return E_NOINTERFACE;

    if(hr = pIDispatch->QueryInterface(IID_IHTMLElement,(void **)&pIHTMLElement) )
        return(hr);

    //--------------------------------------------------
    // build the hit list to that element.
    //--------------------------------------------------

    return(setHitList(pIHTMLElement,pList));
    
}

//-----------------------------------------------------------------------
//  CAOMMgr::setHitList()
//
//  DESCRIPTION:
//  builds a 'hit list', which consists of the input IHTMLElement and 
//  its parent chain, all the way up to the <HTML> tag.
//  
//  **NOTE** 
//  This method is relatively expensive because of the possibility of 
//  overlapping tags, which can create a Trident tree where the structure
//  of the tree when navigating up from the hit element is different than
//  the structure of the tree when navigating down from the top of the 
//  tree.  In order to always make sure that what we see coming up matches
//  what we see coming down, we need to make sure that each parent element
//  in the retrieved chain is a true element, and not a context proxy generated
//  by Trident to compensate for overlapping HTML tags.  The way we do this
//  is to get the parent element from the ALL collection, using the index
//  that we get from the proxy.  This is expensive because it requires 
//  a lot of different interfaces to do the job, but for now thats OK
//  because this method is called only once per overall hit test, and 
//  hit paths are usually under 7 elements long, because the TOM hierarchy 
//  is relatively shallow. 
//
//
//  PARAMETERS:
//  
//  
//  pIHTMLElement   pointer to Trident element interface to start hit list from.
//
//  pList           pointer to list to fill out.
//
//  RETURNS:
//
//  S_OK | E_FAIL | S_FALSE if point not on document.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::setHitList(    /* in */    IHTMLElement * pIHTMLElement,
                                /* out */   std::list<IHTMLElement *> *pList)
{
    HRESULT hr = E_FAIL;

    IHTMLElement * pIHTMLElementParent  = NULL;

    //--------------------------------------------------
    // check input parameters
    //--------------------------------------------------

    assert(pIHTMLElement);
    assert(pList);

    if ( !pIHTMLElement )
        return E_NOINTERFACE;


    //--------------------------------------------------
    // ensure that the root window is available
    //--------------------------------------------------

    assert(m_pRootWindowAO);


    //--------------------------------------------------
    // clean up hit list before resetting it.
    //--------------------------------------------------

    unsetHitList(pList);


    //--------------------------------------------------
    // store element and all of its parents up to the 
    // root <HTML> element.
    //--------------------------------------------------

    //--------------------------------------------------
    // keep this pointer around for the lifetime of 
    // the list.
    //--------------------------------------------------

    pIHTMLElement->AddRef();
    pList->push_front(pIHTMLElement);
    
    while(pIHTMLElement)
    {
    
        if(hr = getNormalizedParent(pIHTMLElement,&pIHTMLElementParent))
        {   

            //--------------------------------------------------
            // S_FALSE means that there was no parent, so 
            // hit path marking is over.
            //--------------------------------------------------

            if(hr == S_FALSE)
                break;
            else
                return(hr);
        }

        
        //--------------------------------------------------
        // **NOTE** the element returned from getNormalizedParent()
        // already has been addref()'d.
        //--------------------------------------------------

        pList->push_front(pIHTMLElementParent);

        //--------------------------------------------------
        // move up the chain
        //--------------------------------------------------

        pIHTMLElement = pIHTMLElementParent;
    }

    
    return(S_OK);
}

//-----------------------------------------------------------------------
//  CAOMMgr::getNormalizedParent()
//
//  DESCRIPTION:
//  returns a parent element that is 'normalized' -- the returned interface
//  points to the scoping parent of the input element, not the context
//  correct parent.  This is important in instances where the tree
//  has to look the same from the bottom up as it does from the top 
//  down, as in CAOMMgr::setHitList().
//
//  PARAMETERS:
//
//  IHTMLElement *pIHTMLElement     pointer to the IHTMLElement interface
//                                  to get the parent of.
//
//  IHTMLElement **ppIHTMLElementParent     pointer to the returned 
//                                          parent interface.
//  
//  RETURNS:
//
//  S_OK, else S_FALSE if no parent, else standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::getNormalizedParent(   /* in */    IHTMLElement * pIHTMLElement, 
                                        /* out  */  IHTMLElement ** ppIHTMLElementParent)
{
    HRESULT hr;


    assert( pIHTMLElement );
    assert( ppIHTMLElementParent );


    *ppIHTMLElementParent = NULL;


    if ( hr = pIHTMLElement->get_parentElement( ppIHTMLElementParent ) )
        return hr;

    //--------------------------------------------------
    // if there is no parent, notify user.
    //--------------------------------------------------

    if ( !*ppIHTMLElementParent )
        return S_FALSE;


    return normalizeIHTMLElement( ppIHTMLElementParent );
}


//-----------------------------------------------------------------------
//  CAOMMgr::normalizeIHTMLElement()
//
//  DESCRIPTION:
//
//  This method "normalizes" the input element, which is to ensure that
//  the interface points to the actual element in the all collection,
//  not to a proxy node.
//
//  This is necessary in instances where the tree has to look the same
//  from the bottom up as it does from the top  down, as in
//  CAOMMgr::setHitList().
//
//  PARAMETERS:
//
//  ppIHTMLElement          pointer to the IHTMLElement interface
//                          to normalize
//
//  RETURNS:
//
//  S_OK, else else standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::normalizeIHTMLElement( /* in-out */ IHTMLElement** ppIHTMLElement )
{
    HRESULT hr = E_FAIL;


    assert( m_pIHTMLAllCollection );

    assert( ppIHTMLElement );
    assert( *ppIHTMLElement );


    //--------------------------------------------------
    // (1) get source index of item to find item 
    // in all collection.
    //--------------------------------------------------

    long lSourceIndex;

    if ( hr = (*ppIHTMLElement)->get_sourceIndex( &lSourceIndex ) )
        return hr;


    //--------------------------------------------------
    // (2) get the true IHTMLElement interface of 
    // the item by getting it from the ALL collection 
    // by its source index.
    //--------------------------------------------------

    VARIANT varName;
    VariantInit(&varName);

    VARIANT varIndex;
    VariantInit(&varIndex);
    varIndex.vt = VT_I4;
    varIndex.lVal = lSourceIndex;

    CComPtr<IDispatch> pIDispatch;

    if ( hr = m_pIHTMLAllCollection->item( varIndex, varName, &pIDispatch ) )
        return hr;

    assert( pIDispatch );


#ifdef _DEBUG

    //--------------------------------------------------
    //  Get the tag name of the current element.
    //--------------------------------------------------

    BSTR    bstrOldTagName = NULL;

    assert( (*ppIHTMLElement)->get_tagName( &bstrOldTagName ) == S_OK );
    
#endif  


    //--------------------------------------------------
    //  Release the current element.
    //--------------------------------------------------

    (*ppIHTMLElement)->Release();
    *ppIHTMLElement = NULL;


    //--------------------------------------------------
    //  Get the normalized element.
    //--------------------------------------------------

    hr = pIDispatch->QueryInterface( IID_IHTMLElement, (void**) ppIHTMLElement );


#ifdef _DEBUG

    if ( hr == S_OK )
    {
        assert( *ppIHTMLElement );

        //--------------------------------------------------
        //  Compare the tag name of the normalized version
        //  of the element with that of the original.
        //--------------------------------------------------

        BSTR    bstrNewTagName = NULL;

        assert( (*ppIHTMLElement)->get_tagName( &bstrNewTagName ) == S_OK );

        assert( !_wcsicmp( bstrNewTagName, bstrOldTagName ) );

        SysFreeString( bstrNewTagName );
    }

    SysFreeString( bstrOldTagName );

#endif  

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::unsetHitList() 
//
//  DESCRIPTION:
//  unmarks hit path set in earlier call to hit path.
//
//  BUGBUG: right now, the method of marking used is only good for standard
//  apps like inspect that hit test in one thread.  If a multithreaded app 
//  that hit tests using multiple threads comes along, we need to guarantee
//  uniqueness of each thread. 
//
//  PARAMETERS:
//
//  pList   pointer to list to unset.
//
//  RETURNS:
//
//  S_OK | E_FAIL | S_FALSE if point not on document.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::unsetHitList(std::list<IHTMLElement *> *pList)
{
    HRESULT hr = E_FAIL;

    assert(pList);
    
    //--------------------------------------------------
    // delete pointers to current path.
    //--------------------------------------------------
    
    int iSize = 0;

    if(iSize = pList->size())
    {
        std::list<IHTMLElement *>::iterator itcurpos = pList->begin();

        //--------------------------------------------------
        // walk entire list and release the contents of
        // the list.
        //--------------------------------------------------

        for(int i = 0; i < iSize; i++)
        {
            ((IHTMLElement *)*itcurpos)->Release();
            itcurpos++;
        }
        
        //--------------------------------------------------
        // erase list nodes.
        //--------------------------------------------------

        std::list<IHTMLElement *>::iterator startpos = pList->begin();
        std::list<IHTMLElement *>::iterator endpos   = pList->end();

        pList->erase(startpos,endpos);
    }

    return(S_OK);

}


//-----------------------------------------------------------------------
//  CAOMMgr::isHitObject()
//
//  DESCRIPTION:
//  checks to see if the returned object is the hit one (the last element
//  in the hit list)
//
//  PARAMETERS:
//
//  pAccEl      pointer to element to evaluate.
//
//  RETURNS:
//
//  S_OK if this is the one, S_FALSE if not, else std COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::isHitObject(   /* in */ CTridentAO * pAOParent,
                                /* in */ CAccElement * pHitAccEl)
{
    HRESULT hr = E_FAIL;

    assert(pAOParent);
    assert(pHitAccEl);

    switch(pHitAccEl->GetAOMType())
    {

    //--------------------------------------------------
    // text is an accessible element : it has no children.
    // therefore it is guaranteed to be the lowest level
    // object. Search is complete.
    //--------------------------------------------------

    case AOMITEM_TEXT:

    //--------------------------------------------------
    // If we hit a frame element, we actually create
    // a window element.  a frame element is the lowest
    // level element in the hit list, because it has its 
    // own AOMMgr and its own hit list. We need to
    // clean up the current hit list before entering 
    // the frame (window).
    //--------------------------------------------------

    case AOMITEM_FRAME:

    //--------------------------------------------------
    // select lists proxy a separate HWND : if the hit 
    // item is a select list, we need to clean up the 
    // hit list because drilling will continue in the
    // proxy of the select list.
    //--------------------------------------------------

    case AOMITEM_SELECTLIST:


            return(S_OK);
    default:

        //--------------------------------------------------
        // the hit element is == the input element. Search
        // is complete.
        //--------------------------------------------------

        if(pAOParent == pHitAccEl)
        {
            return(S_OK);
        }

    }
    return(S_FALSE);
}

//-----------------------------------------------------------------------
//  CAOMMgr::isChildMarked()
//
//  DESCRIPTION:
//
//  checks input element to see if it has been marked with the 
//  specific attribute type flag.
//
//  PARAMETERS:
//
//  pIHTMLElement   pointer to IHTMLElement to evaluate.
//
//
//  RETURNS:
//
//  S_OK if hit | S_FALSE  if not hit, else COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::isChildMarked( /* in */        std::list<CAccElement *>::iterator itCurPos,
                                /* in */        std::list<IHTMLElement *> * pHitList,
                                /* in-out */    std::list<IHTMLElement *>::iterator * pitCurHitListPos)

{
    HRESULT hr = E_FAIL;

    
    CComPtr<IHTMLElement> pIHTMLElement;

    
    assert(*itCurPos);
    assert(pHitList);
    assert(pitCurHitListPos);

    //--------------------------------------------------
    // get IHTMLElement pointer from AE/AO, to
    // input into isElementMarked()
    //--------------------------------------------------

    if(hr = getIHTMLElement((*itCurPos),&pIHTMLElement))
    {
        return(hr);
    }

    //--------------------------------------------------
    // should always have something at this point.
    //--------------------------------------------------

    assert(pIHTMLElement);

    return(isElementMarked(pIHTMLElement,pHitList,pitCurHitListPos));
}

//-----------------------------------------------------------------------
//  CAOMMgr::isElementMarked()
//
//  DESCRIPTION:
//
//  checks input element to see if it is in the hit list, 
//  return TRUE if flag is passed in and element is the 
//  actual hit element.
//
//  PARAMETERS:
//
//  pIHTMLElement   pointer to IHTMLElement to evaluate.
//  iAttributeType  type of attribute to look for.
//  pbIsElementHit  BOOL flagging if element is the hit element.
//
//  RETURNS:
//
//  S_OK if hit | S_FALSE  if not hit, else COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::isElementMarked(/* in */   IHTMLElement * pIHTMLElement,
                                 /* in */   std::list<IHTMLElement *> * pHitList,
                                 /* out */  std::list<IHTMLElement *>::iterator * pitCurHitListPos,
                                 /* out */  BOOL * pbIsElementHit)
{
    HRESULT hr = E_FAIL;


    //--------------------------------------------------
    // always need a pIHTMLElement, and the 
    // current position pointer.
    // pbIsElementHit is optional.
    //--------------------------------------------------

    assert( pIHTMLElement );
    assert( pHitList );
    assert( pitCurHitListPos );

    //--------------------------------------------------
    // initialize passed in BOOL flag to FALSE for
    // initial test.
    //--------------------------------------------------

    if(pbIsElementHit)
    {
        *pbIsElementHit = FALSE;
    }
    
    //--------------------------------------------------
    // is the element in the current hit list.
    //--------------------------------------------------
    
    std::list<IHTMLElement *>::iterator hitpos = *pitCurHitListPos;     


    std::list<IHTMLElement *>::iterator endpos = pHitList->end();


    //--------------------------------------------------
    // if the input element is at the end of the list,
    // return now without further processing.
    //--------------------------------------------------

    if(*pitCurHitListPos == endpos)
    {
        return(S_FALSE);
    }


    BOOL bFoundInList = FALSE;

    while(hitpos != endpos)
    {
        hr = compareIHTMLElements( pIHTMLElement, *hitpos );

        if ( FAILED( hr ) )
            return hr;
        else if ( hr == S_OK )
        {
            bFoundInList = TRUE;
            *pitCurHitListPos = hitpos;
            break;
        }

        hitpos++;
    }

    //--------------------------------------------------
    // if this element isnt hit, return and notify
    // caller.
    //--------------------------------------------------

    if(!bFoundInList)
        return(S_FALSE);

    //--------------------------------------------------
    // if this element is the last one in the list,
    // it is the hit element.
    //--------------------------------------------------

    if(hitpos == --endpos)
    {
        //--------------------------------------------------
        // if the BOOL was passed in, then set it.
        //--------------------------------------------------

        if(pbIsElementHit)
        {
            *pbIsElementHit = TRUE;
        }
    }

    //--------------------------------------------------
    // whether the element is hit or is in the parent
    // chain of the hit element, return and notify 
    // caller.
    //--------------------------------------------------

    return(S_OK);
    
}


//-----------------------------------------------------------------------
//  CAOMMgr::compareIHTMLElements()
//
//  DESCRIPTION:
//
//  Compares the TEO associated with an AO to the IHTMLElement* input
//  parameter.
//
//  PARAMETERS:
//
//  pAO             pointer to the CTridentAO whose associated
//                      IHTMLElement we wish to compare
//  pIHTMLElement   pointer to the IHTMLElement to compare
//
//  RETURNS:
//
//  S_OK if the objects are the same
//  S_FALSE if the objects are not the same or if we are unable to
//      obtain an IHTMLElement* for the CTridentAO.
//  COM error otherwise.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::compareIHTMLElements(  /* in */    CTridentAO* pAO,
                                        /* in */    IHTMLElement* pIHTMLElement )
{
    HRESULT hr = E_FAIL;
    CComPtr<IHTMLElement> pIHTMLElem;

    if ( hr = getIHTMLElement( pAO, &pIHTMLElem ) )
        return hr;

    if ( !pIHTMLElem )
        return S_FALSE;

    return compareIHTMLElements( pIHTMLElem, pIHTMLElement );
}


//-----------------------------------------------------------------------
//  CAOMMgr::compareIHTMLElements()
//
//  DESCRIPTION:
//
//  Compares the TEO associated with a CAccElem* to the IHTMLElement*
//  input parameter.
//
//  PARAMETERS:
//
//  pAccElem        CAccElem pointer to a CTrident[AO|AE] whose associated
//                      IHTMLElement we wish to compare
//  pIHTMLElement   pointer to the IHTMLElement to compare
//
//  RETURNS:
//
//  S_OK if the objects are the same
//  S_FALSE if the objects are not the same or if we are unable to
//      obtain an IHTMLElement* for the CAccElem.
//  COM error otherwise.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::compareIHTMLElements(  /* in */    CAccElement* pAccElem,
                                        /* in */    IHTMLElement* pIHTMLElement )
{
    if ( IsAOMTypeAE( pAccElem ) )
    {
        IHTMLElement*   pIHTMLElem = ((CTridentAE*)pAccElem)->GetTEOIHTMLElement();

        if ( !pIHTMLElem )
            return S_FALSE;

        return compareIHTMLElements( pIHTMLElem, pIHTMLElement );
    }
    else
    {
        return compareIHTMLElements( (CTridentAO*)pAccElem, pIHTMLElement );
    }
}


//-----------------------------------------------------------------------
//  CAOMMgr::compareIHTMLElements()
//
//  DESCRIPTION:
//
//  Compares two IHTMLElement pointers using the IObjectIdentity iface.
//
//  An IHTMLElement* can either point to a real element or an element
//  proxy.  Elements and proxies are two distintly different objects,
//  with different IUnknown*s, that appear as one seamless object
//  through the Trident Object Model.  But COM only allows comparing
//  IUnknown*s for object equality!  So, the IHTMLElement interface
//  supports the IObjectIdentity interface for object equality
//  comparison.
//
//  PARAMETERS:
//
//  pIHTMLElem1     pointer to the first IHTMLElement to compare
//  pIHTMLElem2     pointer to the second IHTMLElement to compare
//
//  RETURNS:
//
//  S_OK | standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::compareIHTMLElements(  /* in */    IHTMLElement* pIHTMLElem1,
                                        /* in */    IHTMLElement* pIHTMLElem2 )
{
    CComQIPtr<IObjectIdentity,&IID_IObjectIdentity> pIObjectIdentity( pIHTMLElem1 );

    assert( pIObjectIdentity );
    if ( !pIObjectIdentity )
        return E_NOINTERFACE;

    return pIObjectIdentity->IsEqualObject( pIHTMLElem2 );
}


//-----------------------------------------------------------------------
//  CAOMMgr::getIHTMLElement()
//
//  DESCRIPTION:
//
//  this method gets the IHTMLElement pointer from the AO/AE.
//
//  PARAMETERS:
//
//  pAccEl          pointer to the AO/AE to get IHTMLElement pointer from.
//  ppIHTMLElement  pointer to place to store IHTMLElement.
//
//  RETURNS:
//
//  S_OK | standard COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::getIHTMLElement( /* in */ CAccElement* pAccEl, /* out */ IHTMLElement** ppIHTMLElement )
{
    HRESULT         hr;
    IHTMLElement*   pIHTMLElement;


    //--------------------------------------------------
    //  Verify the parameters.
    //--------------------------------------------------

    assert(pAccEl);
    assert(ppIHTMLElement);


    //--------------------------------------------------
    //  Initialize the output parameter.
    //--------------------------------------------------

    *ppIHTMLElement = NULL;


    //--------------------------------------------------
    //  Get the IHTMLElement* of the Trident element
    //  object associated with the AO or AE.
    //--------------------------------------------------

    if ( IsAOMTypeAE(pAccEl) )
        pIHTMLElement = ((CTridentAE*) pAccEl)->GetTEOIHTMLElement();
    else
        pIHTMLElement = ((CTridentAO*) pAccEl)->GetTEOIHTMLElement();

    if ( !pIHTMLElement )
    {
        //--------------------------------------------------
        //  The only AO or AEs that don't have an associated
        //  TEO are CTextAEs.
        //--------------------------------------------------

        assert( pAccEl->GetAOMType() == AOMITEM_TEXT );

        hr = E_NOINTERFACE;
    }
    else
    {
        *ppIHTMLElement = pIHTMLElement;
        (*ppIHTMLElement)->AddRef();

        hr = S_OK;
    }


    return hr;
}



//-----------------------------------------------------------------------
//  CAOMMgr::getAOMChildren()
//
//  DESCRIPTION:
//
//  gets all elements (supported and unsupported) of the passed in parent
//  and adds them to the parent's child list.
//
//  PARAMETERS:
//
//      pAOParent               accessible object parent pointer to
//                              the CTrident[AO|AE]-derived objects
//                              that may be created
//
//
//  RETURNS:
//
//      HRESULT             S_OK if successful build, 
//                          S_FALSE if no children to build, 
//                          or a COM error code.
//
//  NOTES:
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::getAOMChildren( /* in */ CTridentAO* pAOParent )
{
    HRESULT                     hr = E_FAIL;
    CAccElement*                pAccElem = NULL;
    ULONG                       ulAOMItemType = 0;
    ULONG                       ulNumFetched = 0;
    int                         i = 0;
    long                        lTOMChildCollectionLen = 0;
    HWND                        hWnd = NULL;

    IHTMLElement*               pIHTMLElemParent = NULL;
    IDispatch*                  pIDispatch = NULL;
    IHTMLElementCollection*     pIHTMLChildCollection = NULL;
    IUnknown*                   pIUnk = NULL;
    IEnumVARIANT*               pIEnumVARIANT = NULL;



    assert( pAOParent );
    assert( m_pRootWindowAO );

    //--------------------------------------------------
    // root window's child should already be built
    // in its initialization.
    //--------------------------------------------------

    assert( m_pRootWindowAO != pAOParent );


    //--------------------------------------------------
    //  This should never be called twice as will result
    //  in the creation of redundant objects.
    //  If child list is already built, bail now.
    //--------------------------------------------------

    if ( pAOParent->HasChildren() )
        return S_OK;


    //--------------------------------------------------
    //  Get the IHTMLElement pointer of the TEO
    //  corresponding to this AO.  This pointer will be
    //  used to get the collection of the TOM children.
    //--------------------------------------------------

    if ( hr = getIHTMLElement( pAOParent, &pIHTMLElemParent ) )
        return hr;
        
    assert( pIHTMLElemParent );

    //--------------------------------------------------
    //  Get the children collection of the TEO parent.
    //--------------------------------------------------

    if ( hr = pIHTMLElemParent->get_children( &pIDispatch ) )
        goto CleanUpAndReturn;

    assert( pIDispatch );


    if ( hr = pIDispatch->QueryInterface( IID_IHTMLElementCollection, (void**) &pIHTMLChildCollection ) )
        goto CleanUpAndReturn;

    assert( pIHTMLChildCollection );


    hr = pIHTMLChildCollection->get_length( &lTOMChildCollectionLen );

    if ( hr != S_OK )
        goto CleanUpAndReturn;


    //--------------------------------------------------
    //  Break out NOW if no children to build.
    //  S_FALSE indicates no children to the caller.
    //--------------------------------------------------

    if ( lTOMChildCollectionLen <= 0 )
    {
        hr = S_FALSE;
        goto CleanUpAndReturn;
    }

    
    //--------------------------------------------------
    //  Instead of walking the children collection
    //  which requires going back to the Trident OM for
    //  each item, use a VARIANT array which contains
    //  an IDispatch* for each item.
    //--------------------------------------------------

    if ( hr = pIHTMLChildCollection->get__newEnum( &pIUnk ) )
        goto CleanUpAndReturn;

    assert( pIUnk );


    if ( hr = pIUnk->QueryInterface( IID_IEnumVARIANT, (void**) &pIEnumVARIANT ) )
        goto CleanUpAndReturn;
                                    
    assert( pIEnumVARIANT );


    if ( hr = reallocateVariantArray( lTOMChildCollectionLen ) )
        goto CleanUpAndReturn;


    if ( hr = pIEnumVARIANT->Next( lTOMChildCollectionLen, m_pVars, &ulNumFetched ) )
    {
        //--------------------------------------------------
        //  If Next() returns S_FALSE, it means that we
        //  weren't given the complete set of children.
        //--------------------------------------------------

        if ( hr == S_FALSE )
        {
            hr = E_UNEXPECTED;
            for ( i = 0; i < ulNumFetched; i++ )
                m_pVars[i].pdispVal->Release();
        }
        goto CleanUpAndReturn;
    }


    //--------------------------------------------------
    //  Get proxied window's HWND.  This is used when
    //  constructing/initializing new AO items.
    //--------------------------------------------------

    hWnd = m_pRootWindowAO->GetWindowHandle();


    //--------------------------------------------------
    //  For each child in array, process and create
    //  either an AOM item or a generic AO to be
    //  collapsed later.
    //--------------------------------------------------

    for ( i = 0; i < lTOMChildCollectionLen; i++ )
    {                                                
        IHTMLElement*   pIHTMLElemChild = NULL;

        if ( (hr = m_pVars[i].pdispVal->QueryInterface( IID_IHTMLElement, (void**) &pIHTMLElemChild )) == S_OK )
        {
            assert( pIHTMLElemChild );

            //--------------------------------------------------
            //  Determine the type of AO to be created.
            //--------------------------------------------------

            hr = setTEOType( pIHTMLElemChild, &ulAOMItemType );

            if ( hr == S_OK )
            {
                //--------------------------------------------------
                // create an item that is either supported or not :
                // resolution will happen later. Insertion happens
                // in this tree.
                //--------------------------------------------------

                hr = createAOMItem( pIHTMLElemChild, pAOParent->GetDocumentAO(), pAOParent, ulAOMItemType, hWnd, &pAccElem );
            }

            pIHTMLElemChild->Release();
        }

        m_pVars[i].pdispVal->Release();

        if ( hr )
            break;
    }

    //--------------------------------------------------
    //  If some of the IDispatch*s in the VARIANT array
    //  still need to be released, do it now.
    //--------------------------------------------------

    if ( i < lTOMChildCollectionLen )
    {
        for ( i++ ; i < lTOMChildCollectionLen; i++ )
            m_pVars[i].pdispVal->Release();
    }


CleanUpAndReturn:

    if ( pIEnumVARIANT )
        pIEnumVARIANT->Release();

    if ( pIUnk )
        pIUnk->Release();

    if ( pIHTMLChildCollection )
        pIHTMLChildCollection->Release();

    if ( pIDispatch )
        pIDispatch->Release();

    if ( pIHTMLElemParent )
        pIHTMLElemParent->Release();


    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::getHitAOMChild()
//
//  DESCRIPTION:
//
//  drills down into tree, building path until the hit object
//  is a supported object.
//
//  PARAMETERS:
//
//  pAOParent       parent to get hit child of.
//  pitStartPos     position in the parent list to start search from.
//  ppRetAccEl      returned obj that encapsulates hit point.
//
//  RETURNS:
//
//  S_OK or standard COM error code.
// ----------------------------------------------------------------------
 

HRESULT CAOMMgr::getHitAOMChild( /* in */       CTridentAO*     pAOParent,
                                 /* in */       std::list<IHTMLElement *> * pHitList,
                                 /* in-out */   std::list<IHTMLElement *>::iterator * pitCurHitListPos, 
                                 /* in-out */   std::list<CAccElement *>::iterator * pitStartPos, 
                                 /* out */      CAccElement ** ppRetAccEl)
{
    HRESULT hr = E_FAIL;

    BOOL bIsTheHitElement   = FALSE;
    BOOL bAccessibleObject  = FALSE;

    assert(pAOParent);
    assert(ppRetAccEl);
    assert(pHitList);
    assert(pitCurHitListPos);

    *ppRetAccEl = NULL;

    //--------------------------------------------------
    // set up start point of search (this could
    // be in the middle of the list, if this method
    // is being called after a collapse.
    //--------------------------------------------------

    std::list<CAccElement *> * pAEList;
    
    if(hr = pAOParent->GetChildList(&pAEList))
        return(hr);

    //--------------------------------------------------
    // if no children in list, return S_FALSE to indicate
    // no hit child object.
    //--------------------------------------------------

    if(!pAEList->size())
        return(S_FALSE);

    std::list<CAccElement *>::iterator itCurAETreePos;
    std::list<CAccElement *>::iterator itEndAETreePos;

    if(!(pitStartPos))
    {
        itCurAETreePos = pAEList->begin();
    }
    else
    {
        itCurAETreePos = *pitStartPos;
    }

    itEndAETreePos = pAEList->end();

    //--------------------------------------------------
    // walk child list, looking for hit tags.
    //--------------------------------------------------

    while(itCurAETreePos != itEndAETreePos)
    {
        //--------------------------------------------------
        // see if the element is marked.
        //--------------------------------------------------

        if(hr = isChildMarked(itCurAETreePos,pHitList,pitCurHitListPos) )
        {

            //--------------------------------------------------
            // S_FALSE means that this wasn't marked, or
            // this was a TextAE. This is a
            // valid return condition, so increment iterator 
            // and continue.
            //
            //
            // [v-arunj 10/27/97] now that textAE children
            // can exist at this point, we need to trap 
            // E_NOINTERFACE and treat it as a valid error code 
            // also (no text child is going to be in the hit path, 
            // and all text children will fail the marking test
            // algorithm with an E_NOINTERFACE error.
            //--------------------------------------------------

            if((hr != S_FALSE) && (hr != E_NOINTERFACE) )
            {
                return(hr);
            }
        }
        else
        {

            //--------------------------------------------------
            // break out of the loop if this is the marked 
            // element (there can be only one marked element
            // at each level of the tree)
            //--------------------------------------------------

            break;
        }

        //--------------------------------------------------
        // if not found yet, move down the list.
        //--------------------------------------------------

        itCurAETreePos++;
    }

    //--------------------------------------------------
    // If no element was marked, get out.
    //--------------------------------------------------
    
    if( !(*itCurAETreePos) || (itCurAETreePos == itEndAETreePos) )
    {
        return(S_FALSE);
    }


    //--------------------------------------------------
    // pAccEl is the marked element, and is a valid
    // element.
    //--------------------------------------------------

    CAccElement * pAccEl = (CAccElement *)*itCurAETreePos;

    //--------------------------------------------------
    // if pAccEl is an unsupported object, collapse it
    // **NOTE** this will recurse if the next hit item
    // underneath this one is also unsupported.
    //--------------------------------------------------

    while( !(pAccEl->IsSupported()) )
    {
        //--------------------------------------------------
        // cast the object pointer to an AO pointer
        // (because that is all that it can be)
        //--------------------------------------------------

        CTridentAO * pAO  = NULL;
    
        pAO = (CTridentAO *)pAccEl;

        //--------------------------------------------------
        // if the hit tag is unsupported, create its children, 
        // drill down again.
        //--------------------------------------------------

        if(!pAO->HasChildren())
        {
            if(hr = getAOMChildren(pAO)) 
            {
                //--------------------------------------------------
                // S_FALSE means there were no children, so
                // just continue on : this object will be collapsed
                // out of the tree.
                //--------------------------------------------------

                if(hr != S_FALSE)
                    return(hr);
            }
        }


#ifdef __AOMMGR_ENABLETEXT

        //--------------------------------------------------
        // [v-arunj 10/27/97] get the text children
        // when collapsing.  This gets the text children
        // in the context of their unsupported parents, which
        // guarantees contextual correctness.
        //
        // **NOTE** this also incurs a performance penalty..
        //--------------------------------------------------

        if(hr = getText (   pAO,
                            m_pIHTMLTxtRangeParent,
                            m_pIHTMLTxtRangeChild,
                            m_pIHTMLTxtRangeTemp))
        {
            if(hr != S_FALSE)
                return(hr);
        }

#endif // __AOMMGR_ENABLETEXT

        //--------------------------------------------------
        // collapse the unsupported object : insert its
        // children into the parent tree.
        //--------------------------------------------------

        if(hr = collapse(pAOParent,pAO,&itCurAETreePos) )
        {
            assert(SUCCEEDED(hr));
            return(hr);
        }

        //--------------------------------------------------
        // continue search for hit object, starting with
        // first collapsed child (which has been 
        // inserted into the parent tree at itNewChildPos
        //--------------------------------------------------

        if(hr = getHitAOMChild(pAOParent,
                               pHitList,
                               pitCurHitListPos,
                               &itCurAETreePos,
                               &pAccEl) )
            return(hr);
    }
    
    //--------------------------------------------------
    // reset pointer so that if this method was called
    // recursively, the returned pointer will point to 
    // the correct child.
    //--------------------------------------------------

    if(pitStartPos)
    {
        *pitStartPos = itCurAETreePos;
    }
    
    //--------------------------------------------------
    // at this point, the ppRetAccEl should be a supported
    // tag. 
    // BUGBUG (carled), we are not addref'ing the return value. I 
    // think we should. This will require downstream changes.
    //--------------------------------------------------

    *ppRetAccEl = pAccEl;
    
    assert(*ppRetAccEl);

    return(S_OK);   
}

//-----------------------------------------------------------------------
//  CAOMMgr::collapse()
//
//  DESCRIPTION:
//  collapses an unsupported tag : removes it from the child list of
//  its parent tag, and replaces it with its children.
//
//  PARAMETERS:
//
//  pParentAO       parent of the item to collapse : the collapsed items 
//                  child list will be inserted into the parent's child list.
//  
//  pAOToCollapse   item to collapse.
//  
//  pitNewChildPos  this parameter will have the position of the 
//                  pAOToCollapse in the parent list, and will return the 
//                  position of the first child of the collapsed item after 
//                  its children have been inserted into the parent list.
//
//  RETURNS:
//
//  S_OK if successful collapse.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::collapse(  /* in */    CTridentAO *pParentAO,
                            /* in */    CTridentAO *pAOToCollapse,
                            /*out */    std::list<CAccElement *>::iterator *pitNewChildPos)
{
    HRESULT hr = E_FAIL;

    std::list<CAccElement *> * pParentList;
    std::list<CAccElement *> * pAOToCollapseList;


    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pParentAO );
    assert( pAOToCollapse );
    assert( *pitNewChildPos != NULL );


    
#ifdef _DEBUG

    //--------------------------------------------------
    // get tag names.
    //--------------------------------------------------

    BSTR    bstrTag;

    CComPtr<IHTMLElement> pParentIHTMLElement;

    if(hr = getIHTMLElement(pParentAO,&pParentIHTMLElement))
        return hr;

    if (hr = pParentIHTMLElement->get_tagName( &bstrTag ))
        return hr;

    SysFreeString( bstrTag );

    CComPtr<IHTMLElement> pChildIHTMLElement;

    if(hr = getIHTMLElement(pAOToCollapse,&pChildIHTMLElement))
        return hr;

    if (hr = pChildIHTMLElement->get_tagName( &bstrTag ))
        return hr;

    SysFreeString( bstrTag );

#endif


    //--------------------------------------------------
    // get list pointers to manipulate parent and child
    // lists.
    //--------------------------------------------------

    if(hr = pParentAO->GetChildList(&pParentList))
    {
        return(hr);
    }

    assert( pParentList );

#ifdef _DEBUG

    //--------------------------------------------------
    // get the size of the parent before inserts 
    // for later checking.
    //--------------------------------------------------

    long lSizeParentBeforeInsert = 0;

    lSizeParentBeforeInsert = pParentList->size();

#endif

    //--------------------------------------------------
    // if the AO to collapse has children, 
    //  (1) insert them into its parent's child list, 
    //  (2) remove those elements from its list, 
    //
    // whether it has children or not, 
    // 
    //  (3) remove the AO from its parent list.
    //  (4) delete the AO
    //--------------------------------------------------


    if(hr = pAOToCollapse->GetChildList(&pAOToCollapseList))
    {
        return(hr);
    }

    assert( pAOToCollapseList );

    long lSizeCollapseChild = 0;

    lSizeCollapseChild = pAOToCollapseList->size();

    //--------------------------------------------------
    // save pointer to original position so that
    // we can remove the unsupported  object from
    // the list after inserting the list.
    //--------------------------------------------------

    std::list<CAccElement *>::iterator itParentInsertPos = *pitNewChildPos;

    //--------------------------------------------------
    // save iterator to first element in list of 
    // collapsing element : this will be the position
    // that will be returned to the calling method 
    // for further evaluation.
    //--------------------------------------------------

    *pitNewChildPos = pAOToCollapseList->begin();


    if(!lSizeCollapseChild)
    {
        //--------------------------------------------------
        // if no elements in child list, set returned
        // pointer to the next element in parent list.
        // then decrement pointer to return it to original
        // position.
        //
        // NOTE : because we are using std list iterators,
        // the only way to get the next value in the list
        // is to actually increment the iterator.  That is 
        // why we are incrementing and then decrementing 
        // the iterator.  Aah, the joy of overloaded operators !!
        //--------------------------------------------------

        *pitNewChildPos = ++itParentInsertPos;

        itParentInsertPos--;


    }
    else
    {
        //--------------------------------------------------
        // reset the parent pointers of all elements in the
        // list that is about to be collapsed to point to 
        // the AO whose child list they are being inserted
        // into.
        //--------------------------------------------------

        std::list<CAccElement *>::iterator itCurPos = *pitNewChildPos;
        std::list<CAccElement *>::iterator itEndPos = pAOToCollapseList->end();

#ifdef _DEBUG

        //--------------------------------------------------
        // check this BOOL to make sure that the loop is 
        // entered : it MUST be entered if there are 
        // children.
        //--------------------------------------------------

        BOOL bLoopEntered = FALSE;

#endif
        
        
        while(itCurPos != itEndPos)
        {
#ifdef _DEBUG
            
            bLoopEntered = TRUE;

#endif

            if ( IsAOMTypeAE( *itCurPos ) )
                ((CTridentAE *)*itCurPos)->SetParent(pParentAO);
            else
                ((CTridentAO *)*itCurPos)->SetParent(pParentAO);

            itCurPos++;
        }

        //--------------------------------------------------
        // this should NEVER fail, because loop should 
        // ALWAYS be entered. If loop is not entered, 
        // parent pointers are invalid, and will point
        // to freed up memory.
        //--------------------------------------------------

        assert(bLoopEntered);

        //--------------------------------------------------
        // insert the contents of the child list into the 
        // parent list and remove them from the child list.
        //--------------------------------------------------

        pParentList->splice(itParentInsertPos,*pAOToCollapseList);

    }
    
#ifdef _DEBUG

    //--------------------------------------------------
    // debug check : get the size of the list before
    // the erase() call. make sure that it == size of
    // old parent list + size of old child list
    //--------------------------------------------------

    long lSizeAfterSplice   = 0;

    lSizeAfterSplice = pParentList->size();

    assert(lSizeAfterSplice == lSizeParentBeforeInsert + lSizeCollapseChild);

#endif


    //--------------------------------------------------
    // remove the unsupported obj from the parent list
    // and delete it.
    //--------------------------------------------------

    std::list<CAccElement *>::const_iterator itObjToRemove      = itParentInsertPos;
    
    (*itObjToRemove)->Release();  // this should delete it

    pParentList->erase(itObjToRemove);


#ifdef  _DEBUG


    //--------------------------------------------------
    // debug stuff is only good if a new list was
    // inserted.
    //--------------------------------------------------

    if(lSizeCollapseChild)
    {
        //--------------------------------------------------
        // debug check : get the size of the list after 
        // the debug call, and make sure that the list
        // was decremented by 1
        //--------------------------------------------------

        long lSizeAfterErase    = 0;

        lSizeAfterErase = pParentList->size();

        assert(lSizeAfterSplice == lSizeAfterErase + 1);


        //--------------------------------------------------
        // debug check : is pointer to first child element
        // of collapsed item still valid ? (it should be)
        //--------------------------------------------------

        IUnknown * pIUnknown = (IUnknown *)**pitNewChildPos;

        IUnknown * pTestIUnknown = NULL;

        if ( IsAOMTypeAE( **pitNewChildPos ) )
            pTestIUnknown = ((CTridentAE *)**pitNewChildPos)->GetTEOIUnknown();
        else
            pTestIUnknown = ((CTridentAO *)**pitNewChildPos)->GetTEOIUnknown();

        assert(pTestIUnknown);
    }

#endif

    return(S_OK);
}

//-----------------------------------------------------------------------
//  CAOMMgr::enforceAOMTreeRules()
//
//  DESCRIPTION:
//  based on accelement input type, apply AOM tree rules.  This may
//  mean repositioning the element in the tree, or removing it, ect.
//
//  PARAMETERS:
//  
//  pAOParent           parent of element to apply rules to.
//  pAccElement         Child to apply rules to.
//  ppNewAccElement     new element : check this value for
//                      
//  RETURNS:
//
//  S_OK else std COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::enforceAOMTreeRules(   /* in */    CTridentAO * pAOParent,
                                        /* in */    CAccElement * pAccElement,
                                        /* out */   CAccElement ** ppNewAccElement)
{

    HRESULT hr = E_FAIL;
    long lMapIndex = 0;
    long lImageIndex = 0;
    CImageAO * pImageAO = NULL;

    CComPtr<IHTMLElement> pIHTMLElementMap;


    assert(pAOParent);
    assert(pAccElement);
    assert(ppNewAccElement);

    *ppNewAccElement = NULL;


    switch(pAccElement->GetAOMType())
    {
    case AOMITEM_MAP:

        lMapIndex = ((CTridentAO *)pAccElement)->GetTOMAllIndex();

        
        if(lMapIndex == m_lCurrentMapIndex)
        {
            //--------------------------------------------------
            // if the image that we are searching for is 
            // embedded in the map that we are trying to 
            // associate the image with, then dont do anything
            // (otherwise we are looking at an infinite loop).
            //--------------------------------------------------

            return(S_OK);
        }
        else
        {

            //--------------------------------------------------
            // otherwise, set the current Map index and continue.
            //--------------------------------------------------

            m_lCurrentMapIndex = lMapIndex;
        }


        //--------------------------------------------------
        // build the image that contains the point.
        //--------------------------------------------------

        if(hr = getIHTMLElement(pAccElement,&pIHTMLElementMap))
            return(hr);
        
        

        if(hr = getImageAtPoint(pIHTMLElementMap,pAOParent->GetDocumentAO(),m_PtHitTest,(CAccElement **)&pImageAO))
        {
            //--------------------------------------------------
            // no images associated with this map
            //--------------------------------------------------

            if(hr == S_FALSE)
            {
                break;
            }

            return(hr);
        }

        assert(pImageAO);
            
        //--------------------------------------------------
        // if the image found at the point doesn't have children,
        // build the map and area children.
        //--------------------------------------------------

        if(!pImageAO->HasChildren())
        {
            if(hr = createAOMMap(   pIHTMLElementMap, 
                                    pAOParent->GetDocumentAO(), 
                                    pImageAO, 
                                    pAOParent->GetWindowHandle()  ))
                return(hr);
        }

        //--------------------------------------------------
        // if the map element doesnt exist, fail here
        //--------------------------------------------------

        *ppNewAccElement = (CAccElement *)pImageAO->GetMap();

        if(!*ppNewAccElement)
        {
            return(E_FAIL);
        }


        //--------------------------------------------------
        // unset the current map index.
        //--------------------------------------------------

        m_lCurrentMapIndex = INOCURRENTMAPINDEX;

        break;
    }

    return(S_OK);
}


//-----------------------------------------------------------------------
//  CAOMMgr::fullyResolveChildren() 
//
//  DESCRIPTION:
//
//  drill down on all unresolved objects in child list.
//
//  PARAMETERS:
//  
//  pParentAO       owner of list to drill on.
//
//  RETURNS:
//
//  S_OK if good drill,else error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::fullyResolveChildren( /* in */ CTridentAO * pParentAO)
{
    HRESULT hr = E_FAIL;

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pParentAO );


    //--------------------------------------------------
    // if this item has already been resolved, 
    // we don't need to do it again.
    //--------------------------------------------------

    if(pParentAO->GetResolvedState())
    {
        return(S_OK);
    }


    //--------------------------------------------------
    // get the parent's child list
    //--------------------------------------------------
    
    std::list<CAccElement *> *pParentList;
    
    if(hr = pParentAO->GetChildList(&pParentList))
    {
        return(hr);
    }

    //--------------------------------------------------
    // collapse if there are children to collapse.
    //--------------------------------------------------

    if(pParentList->size())
    {

        //--------------------------------------------------
        // walk the child list, collapsing all unsupported
        // objects until all members of the list are 
        // supported objects.
        //--------------------------------------------------

        std::list<CAccElement *>::iterator itCurPos = pParentList->begin();
        std::list<CAccElement *>::iterator itEndPos = pParentList->end();

        BOOL bSupported = FALSE;

        while(itCurPos != itEndPos)
        {
            if ( ((CAccElement*) *itCurPos)->GetAOMType() == AOMITEM_NOTSUPPORTED )
            {
                CTridentAO * pAO = (CTridentAO *)*itCurPos;

                //--------------------------------------------------
                // generate the child list of the unsupported tag
                // if it doesnt have children.
                //--------------------------------------------------

                if(!pAO->HasChildren())
                {
                    if(hr = getAOMChildren(pAO))
                    {
                        //--------------------------------------------------
                        // if there are no children (S_FALSE), then continue, 
                        // otherwise return error to caller.            
                        //--------------------------------------------------

                        if(hr != S_FALSE)
                            return(hr);
                    }
                }

                //--------------------------------------------------
                // [v-arunj 10/27/97] get the text children
                // when collapsing.  This gets the text children
                // in the context of their unsupported parents, which
                // guarantees contextual correctness.
                //
                // **NOTE** this also incurs a performance penalty..
                //--------------------------------------------------
#ifdef __AOMMGR_ENABLETEXT
        
                if(hr = getText (   pAO,
                                    m_pIHTMLTxtRangeParent,
                                    m_pIHTMLTxtRangeChild,
                                    m_pIHTMLTxtRangeTemp))
                {
                    if(hr != S_FALSE)
                        return(hr);
                }

#endif // __AOMMGR_ENABLETEXT

                //--------------------------------------------------
                // collapse the unsupported object
                //--------------------------------------------------

                if ( hr = collapse( pParentAO, pAO, &itCurPos ) )
                    return(hr);

                //--------------------------------------------------
                // return to the top of the loop : itCurPos will
                // already have been incremented to the start
                // of the collapsed item's child list that has
                // been inserted into pParentAO's child list.
                //--------------------------------------------------

            }
            else
                //--------------------------------------------------
                // if an item is supported, continue past it to the
                // next item.
                //--------------------------------------------------

                itCurPos++;
        }
    }

#ifdef __AOMMGR_ENABLETEXT

    //--------------------------------------------------
    // find and create text objects (if any) of the 
    // current parent AO.
    //--------------------------------------------------

    if(hr = getText(    pParentAO,
                        m_pIHTMLTxtRangeParent,
                        m_pIHTMLTxtRangeChild,
                        m_pIHTMLTxtRangeTemp) )
    {
        if(hr != S_FALSE)
            return(hr);
    }

#endif  // __AOMMGR_ENABLETEXT

    //--------------------------------------------------
    // the parent is now fully resolved, and no
    // more building needs to be done.
    //--------------------------------------------------

    pParentAO->SetResolvedState(TRUE);

    return(S_OK);
}

//-----------------------------------------------------------------------
//  CAOMMgr::createAOMItem()
//
//  DESCRIPTION:
//
//      Creates a CTrident[AO|AE]-derived object based on the value
//      of lAOMItemType.
//
//  PARAMETERS:
//
//      pIHTMLElement       pointer to the IHTMLElement, the TEO
//
//      pDocAO              pointer to owner document/frame object
//                          (AOs need this information at creation time)
//
//      pAOParent           pointer to the CTridentAO-derived object
//                          that is the AOM parent of the AOM item to
//                          be created
//
//      lAOMItemType        pointer to a ULONG value that indicates which
//                          CTrident[AO|AE]-derived object should be
//                          created (this value is obtained in the method
//                          setTEOType)
//
//      hWnd                handle to the window for which MSAA events
//                          will be fired
//
//      ppAccElem           pointer to a CAccElement pointer that will
//                          point to newly created CTrident[AO|AE]-
//                          derived object
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      It is assumed that the IHTMLElement* pointer (parameter 1)
//      will be valid for the duration of this method and that it
//      is not this method's responsibility to release the interface.
//
//  TODO:
//
//      Investigate a cleaner (code-wise) solution for creating creating
//      the various CTrident[AO|AE]-derived objects.  Currently, there
//      is a lot of code duplication due to the following:
//
//          *   CTridentAO and CTridentAE only share CAccElement as a
//              common base class
//
//          *   the Init() methods of CTrident[AO|AE]-derived classes
//              may be overridden
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::createAOMItem( /* in */  IHTMLElement* pIHTMLElement,
                                /* in */  CDocumentAO*  pDocAO,
                                /* in */  CTridentAO*   pAOParent,
                                /* in */  ULONG         lAOMItemType,
                                /* in */  HWND          hWnd,
                                /* out */ CAccElement** ppAccElem )
{
    HRESULT hr = S_OK;

                
    //--------------------------------------------------
    //  Validate the parameters.
    //--------------------------------------------------
    
    if ( !pIHTMLElement || !pAOParent || !ppAccElem )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  Initialize the out parameters to default values.
    //--------------------------------------------------

    *ppAccElem = NULL;


    //--------------------------------------------------
    //  Obtain the TEO's sourceIndex.
    //--------------------------------------------------

    long    lIndex;

    hr = pIHTMLElement->get_sourceIndex( &lIndex );

    if ( hr != S_OK )
        return hr;


    //------------------------------------------------
    //  Create an IUnknown smart COM pointer from
    //  the IHTMLElement pointer to initialize the
    //  CTrident[AO|AE]-derived object.
    //------------------------------------------------

    CComPtr<IUnknown> pIUnknown(pIHTMLElement);

    if ( !pIUnknown )
        return E_NOINTERFACE;


    //--------------------------------------------------
    //  Create the appropriate CTridentAO-derived or
    //  CTridentAE-derived object based on the value
    //  of lAOMItemType.
    //--------------------------------------------------

    switch ( lAOMItemType & AOMITEM_ITEM_MASK )
    {
        //--------------------------------------------------
        //  Create a CTableAO.
        //--------------------------------------------------

        case AOMITEM_TABLE:
            *ppAccElem = (CAccElement*) new CTableAO( pAOParent, pDocAO,lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CTableAO*) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        //  Create a CTableCellAO.
        //--------------------------------------------------

        case AOMITEM_TABLECELL:
            *ppAccElem = (CAccElement*) new CTableCellAO( pAOParent, pDocAO,lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CTableCellAO*) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        // Create a CDivAO
        //--------------------------------------------------

        case AOMITEM_DIV:
            *ppAccElem = (CAccElement*) new CDivAO( pAOParent, pDocAO,lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CDivAO*) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        //  Create a CButtonAO.
        //--------------------------------------------------

        case AOMITEM_BUTTON:
            *ppAccElem = (CAccElement*) new CButtonAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
            {
                hr = ((CButtonAO*) *ppAccElem)->Init( pIUnknown );
            }
            break;


        //--------------------------------------------------
        //  Create a CImageButtonAO.
        //--------------------------------------------------

        case AOMITEM_IMAGEBUTTON:
            *ppAccElem = (CAccElement*) new CImageButtonAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
            {
                hr = ((CImageButtonAO*) *ppAccElem)->Init( pIUnknown );
            }
            break;


        //--------------------------------------------------
        // create a CCheckboxAO
        //--------------------------------------------------

        case AOMITEM_CHECKBOX:
            *ppAccElem = (CAccElement*) new CCheckboxAO(pAOParent,pDocAO,lIndex,m_nCurrentAOMItemID, hWnd );

            if( !*ppAccElem)
                return E_OUTOFMEMORY;
            else
            {
                hr = ((CCheckboxAO *) *ppAccElem)->Init(pIUnknown);
            }
            break;
            
        //--------------------------------------------------
        // create a CRadioButtonAO
        //--------------------------------------------------

        case AOMITEM_RADIOBUTTON:
            *ppAccElem = (CAccElement*) new CRadioButtonAO(pAOParent,pDocAO,lIndex,m_nCurrentAOMItemID, hWnd );

            if( !*ppAccElem)
                return E_OUTOFMEMORY;
            else
            {
                hr = ((CRadioButtonAO *) *ppAccElem)->Init(pIUnknown);
            }
            break;

        //--------------------------------------------------
        // create a CEditFieldAO
        //--------------------------------------------------
            
        case  AOMITEM_EDITFIELD:
            *ppAccElem = (CAccElement*) new CEditFieldAO(pAOParent,pDocAO,lIndex,m_nCurrentAOMItemID, hWnd );

            if( !*ppAccElem)
                return E_OUTOFMEMORY;
            else
            {
                hr = ((CEditFieldAO *) *ppAccElem)->Init(pIUnknown);
            }
            break;
            
        //--------------------------------------------------
        //  Create a CAnchorAO.
        //--------------------------------------------------

        case AOMITEM_ANCHOR:
            *ppAccElem = (CAccElement*) new CAnchorAO( pAOParent, pDocAO,lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CAnchorAO *) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        //  Create a CImageAO.
        //--------------------------------------------------

        case AOMITEM_IMAGE:
            *ppAccElem = (CAccElement*) new CImageAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
            {
                hr = ((CImageAO *) *ppAccElem)->Init( pIUnknown );

                if(hr == S_OK)
                {

                    //--------------------------------------------------
                    // find associated image map (if any)
                    //--------------------------------------------------

                    hr = processTOMImage( pIHTMLElement, pDocAO,(CTridentAO*) *ppAccElem, hWnd );
                }
            
            }

            break;
/*
        
        //--------------------------------------------------
        // [v-arunj 10/31/97] I am removing sound support
        // because it has been punted for MSAAHTML v1.0
        //--------------------------------------------------

        //--------------------------------------------------
        //  Create a CSoundAE.
        //--------------------------------------------------

        case AOMITEM_SOUND:
            *ppAccElem = (CAccElement *) new CSoundAE( pAOParent, lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CSoundAE *) *ppAccElem)->Init( pIUnknown );
            break;
*/
        //--------------------------------------------------
        //  Create a CMapAO.
        //--------------------------------------------------

        case AOMITEM_MAP:


            //--------------------------------------------------
            // TODO : build the tree down to the found image, then
            // set the parent to the image, and create MAP AO.
            //--------------------------------------------------

            *ppAccElem = (CAccElement*) new CMapAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CMapAO *) *ppAccElem)->Init( pIUnknown );
            break;


        //--------------------------------------------------
        //  Create a CAreaAE.
        //--------------------------------------------------

        case AOMITEM_AREA:
            *ppAccElem = (CAccElement*) new CAreaAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );

            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CAreaAO *) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        // Create a CPluginAO
        //--------------------------------------------------

        case AOMITEM_PLUGIN:
            *ppAccElem = (CAccElement*) new CPluginAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );
            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CPluginAO *) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        // create a Marquee AO
        //--------------------------------------------------

        case AOMITEM_MARQUEE:
            *ppAccElem = (CAccElement*) new CMarqueeAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );
            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CMarqueeAO *) *ppAccElem)->Init( pIUnknown );
            break;

        //--------------------------------------------------
        // create a Select AO
        //--------------------------------------------------

        case AOMITEM_SELECTLIST:
            *ppAccElem = (CAccElement*) new CSelectAO( pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd );
            if ( !*ppAccElem )
                return E_OUTOFMEMORY;
            else
                hr = ((CSelectAO *) *ppAccElem)->Init( pIUnknown );
            break;

        case AOMITEM_FRAME:
            
            //--------------------------------------------------
            // create the new tree : the new tree proxies a
            // new window handle which is retrieved from
            // the location of the passed in IHTMLElement and 
            // the parent window handle. 
            //--------------------------------------------------

            if(hr = g_pProxyMgr->CreateAOMWindow(NULL,pAOParent,pIHTMLElement,lIndex,m_nCurrentAOMItemID,(CWindowAO **)ppAccElem))
                return(hr);


            //--------------------------------------------------
            // we should have a pointer to the new CWindowAO 
            // object now.
            //--------------------------------------------------

            assert( *ppAccElem );
            
            break;
             

        case AOMITEM_NOTSUPPORTED:

            if(!( *ppAccElem = new CTridentAO(pAOParent, pDocAO, lIndex, m_nCurrentAOMItemID, hWnd,TRUE) ))
                return(E_OUTOFMEMORY);
            else
                hr = ((CTridentAO *)*ppAccElem)->Init(pIHTMLElement);
    }


    //--------------------------------------------------
    //  If the CTrident[AO|AE]-derived object was
    //  constructed but not initialized properly,
    //  clean up the mess.
    //--------------------------------------------------

    if ( hr != S_OK && *ppAccElem )
    {
        (*ppAccElem)->Detach();
        *ppAccElem = NULL;
    }



    if ( hr == S_OK )
    {
        
        //------------------------------------------------
        //  Increment the current AOM item unique
        //  ID because we successfully created and 
        //  initialized a valid AOM item.
        //
        // BUGBUG: HOW DOES THIS AFFECT FRAMES ? we may
        // need to account for frame ID
        //------------------------------------------------

        m_nCurrentAOMItemID++;
        
        //------------------------------------------------
        //  Add the newly created AOM item to its parent's
        //  child list.  This will link the AOM item and
        //  its parent in a tree-like fashion.
        //------------------------------------------------

        pAOParent->AddChild( *ppAccElem );
    }

    return hr;
}

//-----------------------------------------------------------------------
//  CAOMMgr::setTEOType()
//
//  DESCRIPTION:
//
//      Determines if the TEOpointed to by pIHTMLElement is one
//      that we support (provide accessibility for).  If it is,
//      the corresponding AOM item will be created.
//
//  PARAMETERS:
//
//      pIHTMLElement       pointer to the current IHTMLElement
//
//      plAOMItemType       pointer to the ULONG that will hold the
//                          numeric value that indicates which AOM
//                          item (which CTrident[AO|AE]-derived object)
//                          should be created for the current TEO
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      It is assumed that the IHTMLElement* pointer (parameter 1)
//      will be valid for the duration of this method and that it
//      is not this method's responsibility to release the interface.
//
//      TODO:   Provide comprehensive functionality.  Method currently
//              only supports a few of the TEO tag names.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::setTEOType(    /* in */  IHTMLElement* pIHTMLElement,
                                /* out */ ULONG* plAOMItemType )
{
    HRESULT hr = S_OK;

                
    //--------------------------------------------------
    //  Validate the parameters.
    //--------------------------------------------------

    if ( !pIHTMLElement || !plAOMItemType )
        return E_INVALIDARG;

    //--------------------------------------------------
    //  Initialize the out parameter to a default value.
    //--------------------------------------------------

    *plAOMItemType = AOMITEM_NOTSUPPORTED;

    //--------------------------------------------------
    //  Get the tag name of the IHTMLElement.  This will
    //  distinguish the type of Trident Element Object.
    //--------------------------------------------------

    BSTR bstrTag;
    hr = pIHTMLElement->get_tagName( &bstrTag );

    if ( hr != S_OK )
        return hr;

    //--------------------------------------------------
    //  Compare the tag name with the supported set
    //  of tag names.
    //
    //  TODO: Optimize the comparison routine.
    //  Investigate an atom table or a hashing routine.
    //--------------------------------------------------
    
    //--------------------------------------------------
    //  Is it a BUTTON tag?
    //--------------------------------------------------


    if ( !_wcsicmp( bstrTag, TEO_TAGNAME_BUTTON ) )
        *plAOMItemType = AOMITEM_BUTTON | AOMITEM_BUTTONBUTTON;

    //--------------------------------------------------
    //  Is it a FRAME tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_FRAME ) )
        *plAOMItemType = AOMITEM_FRAME;

    //--------------------------------------------------
    //  Is it an IFRAME tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_IFRAME ) )
        *plAOMItemType = AOMITEM_FRAME;

    //--------------------------------------------------
    //  Is it a TABLE tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_TABLE) )
        *plAOMItemType = AOMITEM_TABLE | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it a TD tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_TD ) )
        *plAOMItemType = AOMITEM_TABLECELL | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it a TH tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_TH ) )
        *plAOMItemType = AOMITEM_TABLECELL | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it a DIV tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_DIV ) )
        *plAOMItemType = AOMITEM_DIV | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it an ANCHOR tag?
    //--------------------------------------------------
                 
    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_ANCHOR ) )
    {
        hr = doesTOMAnchorHaveHREF( pIHTMLElement, plAOMItemType );

        if ( hr != S_OK )
            *plAOMItemType = AOMITEM_NOTSUPPORTED;
    }

    //--------------------------------------------------
    //  Is it an IMG tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_IMG ) )
        *plAOMItemType = AOMITEM_IMAGE | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it a MAP tag ? : ONLY ID Map tags IF WE
    //  ARE HITTEST or FOCUSED OBJECT BUILDING :
    //  they will be resolved during full child building
    //  when their associated image is built.
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_MAP ) )
    {
        if ( m_iBuildState == IHITTESTBUILDING ||
             m_iBuildState == IFOCUSEDOBJECTBUILDING )
        {
            *plAOMItemType = AOMITEM_MAP | AOMITEM_MAYHAVECHILDREN;
        }
    }

    //--------------------------------------------------
    //  Is it an AREA tag ? ONLY ID Area tags IF WE
    //  ARE HITTEST or FOCUSED OBJECT BUILDING :
    //  they will be resolved during full child building
    //  when their associated image is built.
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_AREA ) )
    {
        if ( m_iBuildState == IHITTESTBUILDING ||
             m_iBuildState == IFOCUSEDOBJECTBUILDING )
        {
            *plAOMItemType = AOMITEM_AREA;
        }
    }

    //--------------------------------------------------
    //  Is it an INPUT tag?  If so, is its TYPE one that
    //  we support?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_INPUT ) )
    {
        hr = isTOMInputTypeSupported( pIHTMLElement, plAOMItemType );

        if ( hr != S_OK )
            *plAOMItemType = AOMITEM_NOTSUPPORTED;
    }     

    //--------------------------------------------------
    // is it a TEXTAREA tag ?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_TEXTAREA ) )
        *plAOMItemType = AOMITEM_EDITFIELD | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it an APPLET tag ?
    //--------------------------------------------------
        
    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_APPLET ) )
        *plAOMItemType = AOMITEM_PLUGIN;

    //--------------------------------------------------
    //  Is it an EMBED tag ?
    //--------------------------------------------------
        
    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_EMBED ) )
        *plAOMItemType = AOMITEM_PLUGIN;

    //--------------------------------------------------
    //  Is it an OBJECT tag ?
    //--------------------------------------------------
        
    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_OBJECT ) )
        *plAOMItemType = AOMITEM_PLUGIN;

    //--------------------------------------------------
    //  Is it a BGSOUND tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_BGSOUND ) )
        *plAOMItemType = AOMITEM_SOUND;

    //--------------------------------------------------
    // Is it a MARQUEE tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_MARQUEE ) )
        *plAOMItemType = AOMITEM_MARQUEE | AOMITEM_MAYHAVECHILDREN;

    //--------------------------------------------------
    //  Is it a SELECT tag?
    //--------------------------------------------------

    else if ( !_wcsicmp( bstrTag, TEO_TAGNAME_SELECT ) )
        *plAOMItemType = AOMITEM_SELECTLIST;

    //--------------------------------------------------
    // if it hasnt been hit by this point, its
    // unsupported.  Thats OK, we'll create a an 
    // object for it and collapse that obj. as needed.
    //--------------------------------------------------

    SysFreeString(bstrTag);

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::doesTOMAnchorHaveHREF()
//
//  DESCRIPTION:
//            
//      Determines if the Trident Anchor has an HREF attribute.
//      If so, then the anchor is at least a "jump to" style anchor
//      and it is a Trident Element Object that we support (provide
//      accessibility for).
//
//  PARAMETERS:
//
//      pIHTMLElement       pointer to the ANCHOR IHTMLElement
//
//      plAOMItemType       pointer to the ULONG that will hold the
//                          numeric value that indicates which AOM
//                          item (which CTrident[AO|AE]-derived object)
//                          should be created, if any, for the current
//                          TYPE of INPUT Trident Element Object
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      It is assumed that the IHTMLElement* pointer (parameter 1)
//      will be valid for the duration of this method and that it
//      is not this method's responsibility to release the interface.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::doesTOMAnchorHaveHREF( /* in */  IHTMLElement* pIHTMLElement,
                                        /* out */ ULONG* plAOMItemType )
{
    HRESULT hr = S_OK;

                
    //--------------------------------------------------
    //  Validate the parameters.
    //--------------------------------------------------

    if ( !pIHTMLElement || !plAOMItemType )
        return E_INVALIDARG;


    //--------------------------------------------------
    //  Initialize the out parameter to a default value.
    //--------------------------------------------------

    *plAOMItemType = AOMITEM_NOTSUPPORTED;


    //--------------------------------------------------
    //  Get the HTML tag of the IHTMLElement.  This will
    //  distinguish the type of Trident Element Object.
    //--------------------------------------------------

    CComQIPtr<IHTMLAnchorElement,&IID_IHTMLAnchorElement> pIHTMLAnchorElement(pIHTMLElement);

    if ( !pIHTMLAnchorElement )
        return E_NOINTERFACE;


    //--------------------------------------------------
    //  Get the HREF property of the IHTMLAnchorElement.
    //--------------------------------------------------

    BSTR bstr;
    hr = pIHTMLAnchorElement->get_href( &bstr );

    if ( hr != S_OK )
        return hr;


    //--------------------------------------------------
    //  If the length of its HREF attribute string is
    //  non-zero, the anchor is a "jump to" type of
    //  anchor.
    //--------------------------------------------------

    if ( SysStringLen( bstr ) > 0 )
        *plAOMItemType = AOMITEM_ANCHOR | AOMITEM_MAYHAVECHILDREN;

    SysFreeString(bstr);

    return hr;
}




//-----------------------------------------------------------------------
//  CAOMMgr::isTOMInputTypeSupported()
//
//  DESCRIPTION:
//
//      Determines the setting of the current INPUT TEO's TYPE attribute
//      and decides which type of AOM item should be created for it.
//
//  PARAMETERS:
//
//      pIHTMLElement       pointer to the INPUT IHTMLElement
//
//      plAOMItemType       pointer to the ULONG that will hold the
//                          numeric value that indicates which AOM
//                          item (which CTrident[AO|AE]-derived object)
//                          should be created, if any, for the current
//                          TYPE of INPUT Trident Element Object
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      It is assumed that the IHTMLElement* pointer (parameter 1)
//      will be valid for the duration of this method and that it
//      is not this method's responsibility to release the interface.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::isTOMInputTypeSupported( /* in */  IHTMLElement* pIHTMLElement,
                                          /* out */ ULONG* plAOMItemType )
{
    HRESULT hr = S_OK;
    BSTR    bstrType;
                
    //--------------------------------------------------
    //  Validate the parameters.
    //--------------------------------------------------

    if ( !pIHTMLElement || !plAOMItemType )
        return E_INVALIDARG;

    //--------------------------------------------------
    //  Initialize the out parameter to a default value.
    //--------------------------------------------------

    *plAOMItemType = AOMITEM_NOTSUPPORTED;

    //--------------------------------------------------
    // QI to see if it is a button
    //--------------------------------------------------

    CComQIPtr<IHTMLInputButtonElement,&IID_IHTMLInputButtonElement> pIHTMLButtonEl(pIHTMLElement);

    if(pIHTMLButtonEl)
    {
        *plAOMItemType = AOMITEM_BUTTON | AOMITEM_INPUTBUTTON;
        return(S_OK);
    }

    //--------------------------------------------------
    //  QI to see if it is a checkbox/radio button
    //--------------------------------------------------
    
    CComQIPtr<IHTMLOptionButtonElement,&IID_IHTMLOptionButtonElement> pIHTMLOptionButtonEl(pIHTMLElement);

    if(pIHTMLOptionButtonEl)
    {
        //--------------------------------------------------
        // checkbox and radiobutton differ by type.
        //--------------------------------------------------

        hr = pIHTMLOptionButtonEl->get_type(&bstrType);

        if(hr != S_OK)
            return(hr);

        if( !_wcsicmp( bstrType, TEO_INPUTTYPE_CHECKBOX) )
        {
            *plAOMItemType = AOMITEM_CHECKBOX;
            return(S_OK);
        }
        else if( !_wcsicmp( bstrType, TEO_INPUTTYPE_RADIO) )
        {
            *plAOMItemType = AOMITEM_RADIOBUTTON;
            return(S_OK);
        }
        else
        {
            //--------------------------------------------------
            // this should never happen : input type should be
            // one of the two types above.
            //--------------------------------------------------
            assert(0);

            SysFreeString( bstrType );
            return(E_FAIL);
        }

        SysFreeString( bstrType );
    }

    //--------------------------------------------------
    // QI to see if it is an editfield
    //--------------------------------------------------
    
    CComQIPtr<IHTMLInputTextElement,&IID_IHTMLInputTextElement> pIHTMLInputTextEl(pIHTMLElement);

    if(pIHTMLInputTextEl)
    {
        //--------------------------------------------------
        //  Weed out any INPUT TYPE=HIDDEN controls.
        //--------------------------------------------------
    
        hr = pIHTMLInputTextEl->get_type( &bstrType );

        if ( hr == S_OK )
        {
            if ( !_wcsicmp( bstrType, TEO_INPUTTYPE_HIDDEN ) )
                *plAOMItemType = AOMITEM_NOTSUPPORTED;
            else
                *plAOMItemType = AOMITEM_EDITFIELD;
        }

        SysFreeString( bstrType );
    }

    //--------------------------------------------------
    // QI to see if it is an image button
    //--------------------------------------------------

    CComQIPtr<IHTMLInputImage,&IID_IHTMLInputImage> pIHTMLInputImage(pIHTMLElement);

    if ( pIHTMLInputImage )
    {
        *plAOMItemType = AOMITEM_IMAGEBUTTON;
        return(S_OK);
    }

    //--------------------------------------------------
    // TODO : add support for other input types as 
    // needed.
    //--------------------------------------------------
    
    return hr;
}

//-----------------------------------------------------------------------
//  CAOMMgr::getImageAtPoint()  
//
//  DESCRIPTION:
//  gets the associated image of the input map element.
//
//  PARAMETERS:
//
//  pIHTMLElement   IHTMLElement interface of map object.
//  pDocAO          pointer to document object.
//  ptTest          point to test for image.
//  ppAccImageEl    pointer to return image in.
//
//  RETURNS:
//
//  S_OK if good return, S_FALSE if no image found, else std. COM error.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::getImageAtPoint(   /* in */    IHTMLElement *pIHTMLElement,
                            /* in */    CDocumentAO * pDocAO,
                            /* in */    POINT ptTest,
                            /* out */   CAccElement **ppAccImageEl)
{
    CComPtr<IHTMLElementCollection> pIHTMLElementCollection;
    HRESULT         hr;
    VARIANT         varIndex;
    VARIANT         var2;
    RECT            rc;
    BSTR            bstrMapName         = NULL;
    BSTR            bstrUseMap          = NULL;
    CImageAO        *pTestImage         = NULL;
    CImageAO        *pFirstImage        = NULL;
    long            lImageCollectionLen = 0;
    long            lFoundImageIndex    = 0;
    long            xLeft               = 0;
    long            yTop                = 0;
    long            cxWidth             = 0;
    long            cyHeight            = 0;
    long            i; 


    VariantInit(&varIndex);
    VariantInit(&var2);

    //--------------------------------------------------
    // validate inputs
    //--------------------------------------------------

    assert( pIHTMLElement );
    assert( pDocAO );
    assert( ppAccImageEl );

    BOOL bFoundImage = FALSE;

    ClientToScreen(pDocAO->GetWindowHandle(),&ptTest);

    //--------------------------------------------------
    // get the name of the input map element to compare
    // image usemap properties against
    //--------------------------------------------------

    CComQIPtr<IHTMLMapElement,&IID_IHTMLMapElement> pIHTMLMapElement(pIHTMLElement);

    if ( !pIHTMLMapElement )
    {
        hr =  E_NOINTERFACE;
        goto CleanupAndReturn;
    }

    if(hr = pIHTMLMapElement->get_name( &bstrMapName ))
        goto CleanupAndReturn;

    //--------------------------------------------------
    // get the images collection
    //--------------------------------------------------

    assert(m_pIHTMLDocument2);

    hr = m_pIHTMLDocument2->get_images( &pIHTMLElementCollection );

    if (( hr != S_OK ) || !pIHTMLElementCollection )
        goto CleanupAndReturn;

    hr = pIHTMLElementCollection->get_length( &lImageCollectionLen );

    if ( hr != S_OK )
        goto CleanupAndReturn;

    //--------------------------------------------------
    //  Walk the Images collection and look for an image
    // that uses the passed in map.
    //--------------------------------------------------

    for ( i = 0; i < lImageCollectionLen; i++ )
    {
        //--------------------------------------------------
        //  Get the current item, an IDispatch, in the
        //  Image collection.
        //--------------------------------------------------

        varIndex.vt   = VT_UINT;
        varIndex.lVal = i;
        VariantInit( &var2 );

        CComPtr<IDispatch> pIDisp;

        hr = pIHTMLElementCollection->item( varIndex, var2, &pIDisp );

        if ( hr != S_OK )
            continue;

        CComQIPtr<IHTMLImgElement,&IID_IHTMLImgElement> pIHTMLImgElement(pIDisp);

        if ( !pIHTMLImgElement )
        {
            hr = E_NOINTERFACE;
            continue;
        }

        //--------------------------------------------------
        //  Get the USEMAP property of the IHTMLImgElement.
        //--------------------------------------------------

        hr = pIHTMLImgElement->get_useMap( &bstrUseMap );

        if ( hr != S_OK )
            goto CleanupAndReturn;

        if ( SysStringLen( bstrUseMap ) > 0 )
        {
            //--------------------------------------------------
            //  ASSUMPTION: All client-side image map name
            //  values will begin with a pound sign (#).
            //  (This tidbit was inferred from the TOM docs.)
            //--------------------------------------------------

            if ( bstrUseMap[0] != L'#' )
                goto CleanupAndReturn;
                    
            //--------------------------------------------------
            // Is this a the named MAP we are looking for?
            //  If so, does it encompass the input point ?
            //--------------------------------------------------

            if ( !_wcsicmp( bstrMapName, &bstrUseMap[1] ) )
            {
                //--------------------------------------------------
                // get the source index of the Image.
                //--------------------------------------------------

                CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(pIHTMLImgElement);

                if(hr = pIHTMLElement->get_sourceIndex(&lFoundImageIndex))
                    goto CleanupAndReturn;

                //--------------------------------------------------
                // build the tree down to the image.
                //--------------------------------------------------

                if(hr = GetAccessibleObjectFromID(  pDocAO,
                                                    lFoundImageIndex,
                                                    (CAccElement **)&pTestImage))
                {
                    goto CleanupAndReturn;
                }

                //--------------------------------------------------
                // HACKHACK :
                // If a map doesn't fall in the bounds
                // of any of its associated images (via point value)
                // associate it with the first image to get a valid
                // object back from this call.  
                //
                // this scenario happens in events handling, where
                // there is no point to validate the map.  
                // this hack handles 99% cases, where a map is only
                // associated w/one image.
                //
                // save the first image here to handle the 
                // 'not found in any images' case. This has to be 
                // done since maps cannot be constrained to just 
                // one image, and don't reside under images in 
                // the Trident Object Model.
                //--------------------------------------------------
    
                if(!pFirstImage)
                    pFirstImage = pTestImage;

                //--------------------------------------------------
                // get the screen location of the image.
                //--------------------------------------------------

                if(hr = pTestImage->AccLocation(&xLeft,&yTop,&cxWidth,&cyHeight,0))
                    goto CleanupAndReturn;

                rc.left     = xLeft;
                rc.top      = yTop;
                rc.right    = xLeft + cxWidth;
                rc.bottom   = yTop + cyHeight;

                if(PtInRect(&rc,ptTest))
                {
                    bFoundImage = TRUE;
                    break;
                }
            }
        }
    }

    if(bFoundImage)
    {
        *ppAccImageEl = pTestImage;
        hr = S_OK;
        goto CleanupAndReturn;
    }
    else
    {
        //--------------------------------------------------
        // no image was found that matches the 
        // point value : default to the first image if
        // there is one. See HACKHACK notes above for
        // full explanation of why we have to do this.
        //--------------------------------------------------

        if(pFirstImage)
        {
            *ppAccImageEl = pFirstImage;
            hr =  S_OK;
            goto CleanupAndReturn;
        }
        
        hr = S_FALSE;
    }

CleanupAndReturn:

    SysFreeString( bstrMapName );
    SysFreeString( bstrUseMap );

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::processTOMImage()
//
//  DESCRIPTION:
//
//      Determines if the current image is actually a client-side image
//      map.  If so, it walks the All collection in search of the
//      MAP TEO associated with it.
//
//  PARAMETERS:
//
//      pTEOImage           pointer to the image IHTMLElement
//
//      pAOImage            pointer to the AOM CImageAO that will
//                          be the parent of the CMapAO, if one
//                          needs to be created
//
//      hWnd                handle to the window
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      A TOM image is a client-side image map if its USEMAP
//      property is non-empty.  The value of this property will be
//      the name of the underlying area map.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::processTOMImage( /* in */  IHTMLElement* pTEOImage,
                                     /* in */  CDocumentAO * pDocAO,
                                     /* in */  CTridentAO* pAOImage,
                                     /* in */  HWND hWnd )
{
    HRESULT         hr = E_FAIL;
    int             i = 0;
    BSTR            bstrUseMap = NULL;
    BOOL            bMapFound = FALSE;


    assert( m_pIHTMLAllCollection );


    //--------------------------------------------------
    //  Validate the parameters.
    //--------------------------------------------------

    assert( pTEOImage );
    assert( pDocAO );
    assert( pAOImage );


    //--------------------------------------------------
    //  For this IMG TEO, get the its USEMAP property
    //  to determine if it is associated with a client-
    //  side image map.
    //--------------------------------------------------

    assert( ((CImageAO*) pAOImage)->GetIHTMLImgElement() );

    hr = ((CImageAO*) pAOImage)->GetIHTMLImgElement()->get_useMap( &bstrUseMap );

    if ( hr != S_OK )
        return hr;


    //--------------------------------------------------
    //  If the length of its USEMAP property string is
    //  non-zero, the IMG TEO has an image map
    //  associated with it.
    //--------------------------------------------------

    if ( SysStringLen( bstrUseMap ) )
    {
        if ( !m_pTOMMapMgr )
            if ( !( m_pTOMMapMgr = new CTOMMapMgr( m_pIHTMLAllCollection ) ) )
                return E_OUTOFMEMORY;

        //--------------------------------------------------
        //  The USEMAP property string has a leading "#"
        //  as defined by HTML, so strip out that character
        //  when trying to match the USEMAP with a MAP NAME.
        //--------------------------------------------------

        hr = m_pTOMMapMgr->MatchMapName( &bstrUseMap[1] );

        if ( hr == S_OK )
        {
            //--------------------------------------------------
            //  Increment the current AOM item ID so the CMapAO
            //  isn't created with the image's AOM item ID.
            //--------------------------------------------------

            m_nCurrentAOMItemID++;

            //--------------------------------------------------
            //  Create the CMapAO and its CAreaAO children.
            //--------------------------------------------------

            hr = createAOMMap( m_pTOMMapMgr->GetMatchingIHTMLMapElement(), pDocAO, pAOImage, hWnd );
        }

        //--------------------------------------------------
        //  CTOMMapMgr::MatchMapName() may return S_FALSE
        //  if no match is made and yet no other failures
        //  occurred.  In this case, simply return S_OK.
        //--------------------------------------------------

        else if ( hr == S_FALSE )
            hr = S_OK;
    }

    SysFreeString( bstrUseMap );


    return hr;
}




//-----------------------------------------------------------------------
//  CAOMMgr::createAOMMap()
//
//  DESCRIPTION:
//
//      Creates the AOM representation of the TOM image map.
//
//  PARAMETERS:
//
//      pIHTMLElement       pointer to the MAP's IHTMLElement
//
//      pIHTMLMapElement    pointer to the MAP's IHTMLMapElement
//
//      pAOParent           pointer to the AOM CImageAO that will
//                          be the parent of the CMapAO that is to
//                          be created
//
//      hWnd                handle to the window that the created
//                          AOM Map and Areas will fire events for
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      It is assumed that the IHTMLElement* pointer (parameter 1)
//      will be valid for the duration of this method and that it
//      is not this method's responsibility to release the interface.
//
//      This method creates the AOM Image Map by first creating 
//      the Map AO for the Map TEO using createAOMItem() and then
//      traversing the Map TEO's Areas collection to create an Area AE
//      for each Area TEO in the collection.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::createAOMMap(  /* in */    IHTMLElement* pIHTMLElement,
                                /* in */    IHTMLMapElement* pIHTMLMapElement,
                                /* in */    CDocumentAO* pDocAO,
                                /* in */    CTridentAO* pAOParent,
                                /* in */    HWND hWnd )
{
    HRESULT hr = S_OK;


    //--------------------------------------------------
    //  Create the map AO.
    //--------------------------------------------------

    CAccElement*    pAccElemMap = NULL;

    hr = createAOMItem( pIHTMLElement,
                        pDocAO,
                        pAOParent,
                        AOMITEM_MAP | AOMITEM_MAYHAVECHILDREN,
                        hWnd,
                        &pAccElemMap );

    if ( hr != S_OK )
        return hr;


    //--------------------------------------------------
    // set the image to point at this map.
    //--------------------------------------------------
    
    if(pAOParent->GetAOMType() == AOMITEM_IMAGE)
    {
        ((CImageAO *)pAOParent)->SetMap((CMapAO *)pAccElemMap);
    }

    //--------------------------------------------------
    //  Get the Areas collection of the MAP TEO.
    //--------------------------------------------------

    CComPtr<IHTMLAreasCollection> pIHTMLAreasCollection;

    hr = pIHTMLMapElement->get_areas( &pIHTMLAreasCollection );

    if ( hr != S_OK )
        return hr;

    assert( pIHTMLAreasCollection );


    //--------------------------------------------------
    //  Determine the length of the Areas collection.
    //--------------------------------------------------

    long    nTOMAreasCollectionLen;

    hr = pIHTMLAreasCollection->get_length( &nTOMAreasCollectionLen );

    if ( hr != S_OK )
        return hr;


    //--------------------------------------------------
    //  Walk the Areas collection, creating an Area
    //  AOM item for TOM Area in the collection.
    //--------------------------------------------------

    for ( long i = 0; i < nTOMAreasCollectionLen; i++ )
    {
        //--------------------------------------------------
        //  Get the current Area IHTMLElement.
        //--------------------------------------------------

        VARIANT varIndex;
        varIndex.vt = VT_UINT;
        varIndex.lVal = i;

        VARIANT var2;
        VariantInit( &var2 );


        CComPtr<IDispatch> pIDispatch;

        hr = pIHTMLAreasCollection->item( varIndex, var2, &pIDispatch );

        if ( hr != S_OK )
            break;

        
        CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(pIDispatch);

        if ( !pIHTMLElement )
        {
            hr = E_NOINTERFACE;
            break;
        }


        //--------------------------------------------------
        //  Create the Area AE.
        //--------------------------------------------------

        CAccElement*    pAccElemArea = NULL;

        hr = createAOMItem( pIHTMLElement,
                            pDocAO,
                            (CTridentAO*) pAccElemMap,
                            AOMITEM_AREA,
                            hWnd,
                            &pAccElemArea );

        if ( hr != S_OK )
            break;
                        
    }


    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::createAOMMap()
//
//  DESCRIPTION:
//
//      Overloaded version of createAOMMap().  This one QIs the
//      input IHTMLElement* for a IHTMLMapElement and then calls
//      the full createAOMMap().
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::createAOMMap(  /* in */    IHTMLElement* pIHTMLElement,
                                /* in */    CDocumentAO* pDocAO,
                                /* in */    CTridentAO* pAOParent,
                                /* in */    HWND hWnd )
{
    assert( pIHTMLElement );
    assert( pDocAO );
    assert( pAOParent );


    CComQIPtr<IHTMLMapElement,&IID_IHTMLMapElement> pIHTMLMapElement( pIHTMLElement );

    assert( pIHTMLMapElement );
    if ( !pIHTMLMapElement )
        return E_NOINTERFACE;

    return createAOMMap( pIHTMLElement, pIHTMLMapElement, pDocAO, pAOParent, hWnd );
}


//-----------------------------------------------------------------------
//  CAOMMgr::createAOMMap()
//
//  DESCRIPTION:
//
//      Overloaded version of createAOMMap().  This one QIs the
//      input IHTMLMapElement* for a IHTMLElement and then calls
//      the full createAOMMap().
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::createAOMMap(  /* in */    IHTMLMapElement* pIHTMLMapElement,
                                /* in */    CDocumentAO* pDocAO,
                                /* in */    CTridentAO* pAOParent,
                                /* in */    HWND hWnd )
{
    assert( pIHTMLMapElement );
    assert( pDocAO );
    assert( pAOParent );


    CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement( pIHTMLMapElement );

    assert( pIHTMLElement );
    if ( !pIHTMLElement )
        return E_NOINTERFACE;

    return createAOMMap( pIHTMLElement, pIHTMLMapElement, pDocAO, pAOParent, hWnd );
}



//-----------------------------------------------------------------------
//  BOOL ptHasChanged()
//
//  DESCRIPTION:
//
//  Determines whether or not the new hit test point is enough
//  different from the cached hit test point to warrant rebuilding
//  the hit list.
//
//  PARAMETERS:
//
//  pt  to evaluate against m_PtHitTest
//
//  RETURNS:
//
//  TRUE if point is equal to m_PtHitTest, FALSE otherwise.
//
//  NOTE:
//
//  This method is inline as it currently performs a trival point-to-
//  point comparison.
// ----------------------------------------------------------------------

inline BOOL CAOMMgr::ptHasChanged(POINT pt)
{
    if ( ( m_PtHitTest.x == pt.x ) && ( m_PtHitTest.y == pt.y ) )
        return FALSE;

    return TRUE;
}



//-----------------------------------------------------------------------
//  CAOMMgr::reallocateVariantArray()
//
//  DESCRIPTION:
//
//  Reallocates the VARIANT array if the desired size is greater than
//  the current size.
//
//  PARAMETERS:
//
//  long        Desired size of VARIANT array
//
//  RETURNS:
//
//  HRESULT     S_OK if the new size is less than or equal to current
//                  size, or if allocation of VARIANT array succeeds.
//              E_OUTOFMEMORY if allocation of array failed.
//
//  NOTES:
//
//  This method assumes that the array's user has properly cleaned up
//  after themselves.  When the VARIANT array is deleted, VariantClear()
//  is NOT called for each item.
//-----------------------------------------------------------------------

HRESULT CAOMMgr::reallocateVariantArray( /* in */ long lNewSize )
{
    if ( lNewSize <= m_lCurSizeVariantArray )
        return S_OK;


    if ( m_pVars )
    {
        delete [] m_pVars;
        m_pVars = NULL;
    }


    m_pVars = new VARIANT[lNewSize];


    if ( !m_pVars  )
    {
        m_lCurSizeVariantArray = 0;
        return E_OUTOFMEMORY;
    }
    else
    {
        m_lCurSizeVariantArray = lNewSize;
        return S_OK;
    }
}




//=======================================================================
//  CAOMMgr class : protected methods
//
//  The following CAOMMgr public methods are used when text processing
//  is *enabled* (when the preprocessor flag __AOMMGR_ENABLETEXT is
//  defined).  
//=======================================================================

#ifdef __AOMMGR_ENABLETEXT


//-----------------------------------------------------------------------
//  CAOMMgr::getDocumentTxtRanges()
//
//  DESCRIPTION:
//
//      Initializes the three Trident text ranges used for processing
//      text on an HTML document.
//
//  PARAMETERS:
//
//
//      pIHTMLTxtRangeParent    pointer to the text range of the parentAO
//
//      pIHTMLTxtRangeChild     pointer to the text range of the current child (cached
//                              for optimization. 
//
//      pIHTMLTxtRangeTemp      pointer to a temp text range, also
//                              cached for optimization.
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::getDocumentTxtRanges( /* out */ IHTMLTxtRange** ppIHTMLTxtRangeParent,
                                       /* out */ IHTMLTxtRange** ppIHTMLTxtRangeChild,
                                       /* out */ IHTMLTxtRange** ppIHTMLTxtRangeTemp )
{
    HRESULT hr;


    //--------------------------------------------------
    //  Initialize the out parameters.
    //--------------------------------------------------

    *ppIHTMLTxtRangeParent = NULL;
    *ppIHTMLTxtRangeChild = NULL;
    *ppIHTMLTxtRangeTemp  = NULL;


    assert(m_pIHTMLDocument2);

    
    CComPtr<IHTMLElement> pIHTMLElement;

    if(hr = m_pIHTMLDocument2->get_body(&pIHTMLElement))
    {
        return(hr);
    }

    if ( !pIHTMLElement )
        return E_NOINTERFACE;

    CComQIPtr<IHTMLBodyElement,&IID_IHTMLBodyElement> pIHTMLBodyElement(pIHTMLElement);


    //--------------------------------------------------
    // if there is no body element, that means that
    // the body is actually a FRAMESET. Return here,
    // notifying the users that the text ranges have
    // not been created.
    //--------------------------------------------------


    if(!pIHTMLBodyElement)
        return(S_FALSE);

    //--------------------------------------------------
    //  Create a text range off the BODY.
    //--------------------------------------------------

    hr = pIHTMLBodyElement->createTextRange( ppIHTMLTxtRangeParent );

    if ( hr != S_OK )
        return hr;

    if ( !*ppIHTMLTxtRangeParent )
        return E_NOINTERFACE;


    //--------------------------------------------------
    //  Create an empty text range with its start and
    //  end at the start of the BODY's text range.
    //--------------------------------------------------

    hr = pIHTMLBodyElement->createTextRange( ppIHTMLTxtRangeChild );

    if ( hr != S_OK )
        return hr;

    if ( !*ppIHTMLTxtRangeChild )
        return E_NOINTERFACE;


    //--------------------------------------------------
    //  Create another empty text range.
    //--------------------------------------------------

    hr = pIHTMLBodyElement->createTextRange( ppIHTMLTxtRangeTemp );

    if ( hr != S_OK )
        return hr;

    if ( !*ppIHTMLTxtRangeTemp )
        return E_NOINTERFACE;

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::initializeTextRangeBSTRs()
//
//  DESCRIPTION:
//
//  Initializes the cached BSTR members used when manipulating
//  Trident text ranges.
//
//  PARAMETERS:
//
//      None
//
//  RETURNS:
//
//      HRESULT     S_OK if all BSTRs allocated successfully,
//                  E_OUTOFMEMORY if any BSTR cannot be allocated.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::initializeTextRangeBSTRs( void )
{
    HRESULT     hr = E_OUTOFMEMORY;

    assert( m_bstrStartToStart == NULL );
    assert( m_bstrStartToEnd == NULL );
    assert( m_bstrEndToStart == NULL );
    assert( m_bstrEndToEnd == NULL );

    if ( (m_bstrStartToStart = SysAllocString( L"STARTTOSTART" )) )
        if ( (m_bstrStartToEnd = SysAllocString( L"STARTTOEND" )) )
            if ( (m_bstrEndToStart = SysAllocString( L"ENDTOSTART" )) )
                if ( (m_bstrEndToEnd = SysAllocString( L"ENDTOEND" )) )
                    hr = S_OK;

    if ( hr != S_OK )
    {
        assert( hr == S_OK );

        if ( m_bstrEndToEnd )
        {
            SysFreeString( m_bstrEndToEnd );
            m_bstrEndToEnd = NULL;
        }

        if ( m_bstrEndToStart )
        {
            SysFreeString( m_bstrEndToStart );
            m_bstrEndToStart = NULL;
        }

        if ( m_bstrStartToEnd )
        {
            SysFreeString( m_bstrStartToEnd );
            m_bstrStartToEnd = NULL;
        }

        if ( m_bstrStartToStart )
        {
            SysFreeString( m_bstrStartToStart );
            m_bstrStartToStart = NULL;
        }
    }

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::getText()
//
//  DESCRIPTION:
//
//      This method uses the character position based functionality of
//      text ranges to create text objects for text that exists within the 
//      scope of the passed in AO object's HTML Tag.  The text that is
//      created is text that is NOT in the children of the passed in object.
//
//
//  PARAMETERS:
//
//      pAOParent               accessible object parent pointer to
//                              the CTrident[AO|AE]-derived objects
//                              that may be created
//      
//      pIHTMLTxtRangeParent    pointer to the text range of the parentAO
//
//      pIHTMLTxtRangeChild     pointer to the text range of the current child (cached
//                              for optimization. 
//
//      pIHTMLTxtRangeTemp      pointer to a temp text range, also
//                              cached for optimization.
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code, or S_FALSE if
//                          there are no text ranges on the body (no text to search).
//
//  TODO:
//
//  The text ranges passed into this method should be considered "empty
//  slates" upon which this method writes to complete its tasks.
//  The ranges these objects scope are setup/determined wholly within
//  the scope of this method, the caller need not worry about that.
//  So, since the ranges passed into this method are really just member
//  data of the CAOMMgr, they should be removed from the parameter list.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::getText    ( /* in */      CTridentAO*     pAOParent,
                              /* in */      IHTMLTxtRange*  pIHTMLTxtRangeParent,
                              /* in */      IHTMLTxtRange*  pIHTMLTxtRangeChild,
                              /* in */      IHTMLTxtRange*  pIHTMLTxtRangeTemp )
{
    HRESULT         hr;


    assert( pAOParent );


    //--------------------------------------------------
    // if one of the text ranges is NULL, then don't
    // search for text.
    //
    // this happens if we are trying to get the text
    // range on a Frameset.
    //--------------------------------------------------

    if( !pIHTMLTxtRangeParent || !pIHTMLTxtRangeChild || !pIHTMLTxtRangeTemp )
        return S_FALSE;
    

    //--------------------------------------------------
    // if the parent cannot contain text, return S_OK
    //--------------------------------------------------

    if ( !canObjectContainText( pAOParent ) )
        return S_OK;


    //--------------------------------------------------
    //  Adjust pIHTMLTxtRangeParent to point to the
    //  start of text range for pAOParent.
    //--------------------------------------------------

    CComPtr<IHTMLElement> pIHTMLElementParent;

    if ( hr = getIHTMLElement( pAOParent, &pIHTMLElementParent ) )
        return hr;


#ifdef _DEBUG

    BSTR bstrParentTag = NULL;
    assert( pIHTMLElementParent->get_tagName(&bstrParentTag) == S_OK );
    SysFreeString( bstrParentTag );

#endif


    if ( hr = pIHTMLTxtRangeParent->moveToElementText( pIHTMLElementParent ) )
        return hr;


    //--------------------------------------------------
    //  The temporary text range is used to scope the
    //  difference between the parent's range and a
    //  child's range and to create a CTextAE for that
    //  difference.  This range is used primarily in
    //  compareForValidtext(), but it must be
    //  initialized here to ensure it's initial scope
    //  is appropriate for this parent.
    //--------------------------------------------------

    if ( hr = pIHTMLTxtRangeTemp->moveToElementText( pIHTMLElementParent ) )
        return hr;


#ifdef _DEBUG

    BSTR bstrParent = NULL;
    assert( pIHTMLTxtRangeParent->get_text(&bstrParent) == S_OK );
    SysFreeString( bstrParent );

#endif


    //--------------------------------------------------
    // get the child list of the parent to iterate 
    // through.
    //--------------------------------------------------

    std::list<CAccElement *>            *pAEList;
    std::list<CAccElement *>::iterator  itCurPos;
    long                                lSize;
    
    if ( hr = pAOParent->GetChildList( &pAEList ) )
        return hr;

    lSize = pAEList->size();
    itCurPos = pAEList->begin();

    BOOL    bDoneWithParent = FALSE;

    for ( int i = 0; i < lSize; i++ )
    {
        CComPtr<IHTMLElement>   pIHTMLElementChild;
        BOOL                    bCurrentChildIsTextAE = FALSE;

        //--------------------------------------------------
        // get the text range of the current child
        //--------------------------------------------------

        if ( ((CAccElement*)*itCurPos)->GetAOMType() == AOMITEM_TEXT )
        {
            if ( hr = setTextRangesEqual( pIHTMLTxtRangeChild, ((CTextAE*)*itCurPos)->GetTextRangePtr() ) )
                return hr;

            bCurrentChildIsTextAE = TRUE;
        }
        else
        {
            if ( hr = getIHTMLElement( *itCurPos, &pIHTMLElementChild ) )
            {
                assert( hr == S_OK );
                return hr;
            }

#ifdef _DEBUG

            BSTR bstrChildTag = NULL;
            assert( pIHTMLElementChild->get_tagName(&bstrChildTag) == S_OK );
            SysFreeString( bstrChildTag );

#endif

            if ( hr = pIHTMLTxtRangeChild->moveToElementText( pIHTMLElementChild ) )
                return hr;
        }


        //--------------------------------------------------
        //  Determine if the start of the parent range is
        //  less than the start of the current child's
        //  range.  If so, a text object might need to be
        //  created for that text.
        //--------------------------------------------------

        if ( hr = compareForValidText( pAOParent, pIHTMLTxtRangeParent, pIHTMLTxtRangeChild, pIHTMLTxtRangeTemp, itCurPos ) )
            return hr;


        //--------------------------------------------------
        //  The parent range now needs to be updated
        //  appropriately such that its start is moved
        //  beyond the end of the range for the current
        //  child, because the range scoped by the child
        //  will be processed in another getText() pass.
        //--------------------------------------------------

        long    lCompare;

        if ( hr = pIHTMLTxtRangeParent->compareEndPoints( m_bstrEndToEnd, pIHTMLTxtRangeChild, &lCompare ) )
            return hr;


        if ( lCompare == 1 )
        {
            //--------------------------------------------------
            //  The end point of the parent's range is greater
            //  than that of the child.  If the current child
            //  is a text AE, simply move the head of the parent
            //  range to the end of the child range.
            //  Otherwise, the head of the parent range must be
            //  moved to the furthest of either the end of the
            //  child range or the end of the range of the
            //  child's descendant that overlaps it the most.
            //--------------------------------------------------

            if ( bCurrentChildIsTextAE )
            {
                if ( hr = pIHTMLTxtRangeParent->setEndPoint( m_bstrStartToEnd, pIHTMLTxtRangeChild ) )
                    return hr;
            }
            else
            {
                if ( hr = updateParentTextRangeEndPoint( pIHTMLTxtRangeParent, pIHTMLTxtRangeChild, pIHTMLTxtRangeTemp, pIHTMLElementChild ) )
                {
                    if ( hr == S_FALSE )
                    {
                        hr = S_OK;
                        bDoneWithParent = TRUE;
                        break;
                    }
                    else
                        return hr;
                }
            }
        }
        else
        {
            //--------------------------------------------------
            //  The end point of the child's range is greater
            //  than or equal to that of the parent, so stop
            //  getting text for the parent.
            //--------------------------------------------------

            bDoneWithParent = TRUE;


            if ( lCompare == -1 && bCurrentChildIsTextAE )
            {
                //--------------------------------------------------
                //  The end point of the child's range is greater
                //  than that of the parent AND the current child
                //  is a text AE, so break up the child into two
                //  text objects: one fully scoped by the parent
                //  and one what remains.
                //--------------------------------------------------

                hr = splitTextAE( pAOParent, (CTextAE*)*itCurPos, pIHTMLTxtRangeParent, pIHTMLTxtRangeTemp, itCurPos );
            }

            break;
        }


        itCurPos++;
    }


    if ( !bDoneWithParent )
    {
        //--------------------------------------------------
        //  After looping through all the children, there
        //  may be text at the end of the parent's range not
        //  scoped by any child.  If so, a text object needs
        //  to be created for this text.
        //
        //  Collapse the child range to the end of the
        //  parent's range and then look for any text.
        //--------------------------------------------------

        if ( !lSize )
        {
            if ( hr = pIHTMLTxtRangeChild->moveToElementText( pIHTMLElementParent ) )
                return hr;
        }
        else
        {
            if ( hr = pIHTMLTxtRangeChild->setEndPoint( m_bstrEndToEnd, pIHTMLTxtRangeParent ) )
                return hr;
        }

        if ( hr = pIHTMLTxtRangeChild->collapse( 0 ) )
            return hr;

        hr = compareForValidText( pAOParent, pIHTMLTxtRangeParent, pIHTMLTxtRangeChild, pIHTMLTxtRangeTemp, itCurPos );
    }


    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::setTextRangesEqual()
//
//  DESCRIPTION:
//
//  Moves both end points of the first text range to those of the
//  second text range.
//  This method wraps two calls to IHTMLTxtRange::setEndPoint().
//
//  PARAMETERS:
//
//      pIHTMLTxtRangeDst
//      pIHTMLTxtRangeSrc
//
//  RETURNS:
//
//      S_OK if the text range's end points are successfully moved;
//      E_OUTOFMEMORY or other standard COM error otherwise.
//
// ----------------------------------------------------------------------

HRESULT CAOMMgr::setTextRangesEqual(    /* in */ IHTMLTxtRange* pIHTMLTxtRangeDst,
                                        /* in */ IHTMLTxtRange* pIHTMLTxtRangeSrc )
{
    HRESULT     hr;
    long        lCompare;
    BSTR*       pbstrFirstMove;
    BSTR*       pbstrSecondMove;


    assert( pIHTMLTxtRangeSrc );
    assert( pIHTMLTxtRangeDst );


    //--------------------------------------------------
    //  We need to prevent the destination range from
    //  being updated incorrectly (i.e., moving its
    //  start/end past its current end/start such that
    //  the end points get swapped unexpectedly from
    //  our point of view).
    //  
    //  If the destination range's start is greater
    //  than or equal to the source range's end, move
    //  start-to-start and then end-to-end; otherwise,
    //  move end-to-end and then start-to-start.
    //--------------------------------------------------

    if ( hr = pIHTMLTxtRangeDst->compareEndPoints( m_bstrStartToEnd, pIHTMLTxtRangeSrc, &lCompare ) )
        return hr;


    if ( lCompare > -1 )
    {
        pbstrFirstMove = &m_bstrStartToStart;
        pbstrSecondMove = &m_bstrEndToEnd;
    }
    else
    {
        pbstrFirstMove = &m_bstrEndToEnd;
        pbstrSecondMove = &m_bstrStartToStart;
    }


    hr = pIHTMLTxtRangeDst->setEndPoint( *pbstrFirstMove, pIHTMLTxtRangeSrc );

    if ( hr == S_OK )
        hr = pIHTMLTxtRangeDst->setEndPoint( *pbstrSecondMove, pIHTMLTxtRangeSrc );


    return hr;
}



//-----------------------------------------------------------------------
//  CAOMMgr::updateParentTextRangeEndPoint()
//
//  DESCRIPTION:
//
//  Moves the start point of the parent text range to the greater of
//  either the end point of the child's text range or the end point of
//  the text range of the furthest overlapping element on the child's
//  branch.
//
//  This is and CAOMMgr::getRightMostTextRange() are the key methods
//  to detecting and handling OVERLAPPED HTML!
//
//  PARAMETERS:
//
//      pIHTMLElement
//      pIHTMLTxtRange
//
//  RETURNS:
//
//      S_OK if the parent text range was successfully updated;
//      S_FALSE if the parent is overlapped by the last item in the
//          child's all collection;
//      standard COM error otherwise.
//
//  NOTES:
//
//  Upon entry to this method, we know that the end of the parent
//  text range is greater than the end of the child text range.
//
//  TODO:
//
//  The text ranges passed into this method should be considered "empty
//  slates" upon which this method writes to complete its tasks.
//  The ranges these objects scope are setup/determined wholly within
//  the scope of this method, the caller need not worry about that.
//  So, since the ranges passed into this method are really just member
//  data of the CAOMMgr, they should be removed from the parameter list.
//
// ----------------------------------------------------------------------

HRESULT CAOMMgr::updateParentTextRangeEndPoint( /* in */  IHTMLTxtRange* pIHTMLTxtRangeParent,
                                                /* in */  IHTMLTxtRange* pIHTMLTxtRangeChild,
                                                /* in */  IHTMLTxtRange* pIHTMLTxtRangeTemp,
                                                /* in */  IHTMLElement* pIHTMLElementChild )
{
    HRESULT         hr;
    long            lCompare;
    IHTMLElement*   pIHTMLElement1 = NULL;
    IHTMLElement*   pIHTMLElement2 = NULL;
    IHTMLTxtRange*  pIHTMLTxtRangeOverlap = NULL;


    assert( pIHTMLTxtRangeParent );
    assert( pIHTMLTxtRangeChild );
    assert( pIHTMLTxtRangeTemp );
    assert( pIHTMLElementChild );


    //--------------------------------------------------
    //  Get the descendant (and its text range) of the
    //  child whose the text range end point is
    //  rightmost.
    //--------------------------------------------------

    if ( hr = getRightMostTextRange( pIHTMLElementChild, pIHTMLTxtRangeTemp, &pIHTMLElement1 ) )
    {
        if ( hr != S_FALSE )
            goto CleanUpAndReturn;
        else
        {
            //--------------------------------------------------
            //  The child has no descendants, so move the start
            //  of the parent text range to the end of the child
            //  text range.
            //--------------------------------------------------

            hr = pIHTMLTxtRangeParent->setEndPoint( m_bstrStartToEnd, pIHTMLTxtRangeChild );
            goto CleanUpAndReturn;
        }
    }


    //--------------------------------------------------
    //  Compare the end point of the child text range
    //  to the end point of the rightmost text range.
    //--------------------------------------------------

    if ( hr = pIHTMLTxtRangeChild->compareEndPoints( m_bstrEndToEnd, pIHTMLTxtRangeTemp, &lCompare ) )
        goto CleanUpAndReturn;

    if ( lCompare > -1 )
    {
        //--------------------------------------------------
        //  The end point of the child text range is
        //  greater than or equal to the end point of the
        //  rightmost text range.  The child is NOT
        //  overlapped by any of the items in its branch,
        //  so move the head of the parent's text range to
        //  the end of the child.
        //--------------------------------------------------

        hr = pIHTMLTxtRangeParent->setEndPoint( m_bstrStartToEnd, pIHTMLTxtRangeChild );
    }
    else
    {
        //--------------------------------------------------
        //  The child is overlapped by at least one of the
        //  the elements in its branch, so drill down
        //  through the all collections of all overlapped
        //  elements on this branch to find the element
        //  (and its range) that is rightmost.
        //--------------------------------------------------

        do
        {
            if ( pIHTMLTxtRangeOverlap )
            {
                if ( hr = setTextRangesEqual( pIHTMLTxtRangeTemp, pIHTMLTxtRangeOverlap ) )
                    goto CleanUpAndReturn;
            }
            else
            {
                if ( hr = pIHTMLTxtRangeTemp->duplicate( &pIHTMLTxtRangeOverlap ) )
                    goto CleanUpAndReturn;
            }

            if ( hr = getRightMostTextRange( pIHTMLElement1, pIHTMLTxtRangeOverlap, &pIHTMLElement2 ) )
            {
                if ( hr == S_FALSE )
                    break;
                else
                    goto CleanUpAndReturn;
            }

            if ( hr = pIHTMLTxtRangeTemp->compareEndPoints( m_bstrEndToEnd, pIHTMLTxtRangeOverlap, &lCompare ) )
                goto CleanUpAndReturn;

            pIHTMLElement1->Release();
            pIHTMLElement1 = pIHTMLElement2;
            pIHTMLElement2 = NULL;
        }
        while ( lCompare < 0 );


        //--------------------------------------------------
        //  Compare the end point of the parent text range
        //  to the end point of the rightmost text range on
        //  this branch.
        //--------------------------------------------------

        if ( hr = pIHTMLTxtRangeParent->compareEndPoints( m_bstrEndToEnd, pIHTMLTxtRangeTemp, &lCompare ) )
            goto CleanUpAndReturn;

        if ( lCompare > 0 )
        {
            //--------------------------------------------------
            //  The end point of the parent text range is
            //  greater than the end point of the rightmost
            //  text range on this branch.
            //  Move the head of the parent's text range to the
            //  end of the rightmost text range.
            //--------------------------------------------------

            if ( hr = pIHTMLTxtRangeParent->setEndPoint( m_bstrStartToEnd, pIHTMLTxtRangeTemp ) )
                return hr;
        }
        else
        {
            //--------------------------------------------------
            //  The end point of the parent text range is
            //  less than or equal to the end point of the
            //  rightmost text range on this branch.
            //  We need to stop getting text for the parent
            //  to prevent creating redundant text objects.
            //--------------------------------------------------

            hr = S_FALSE;
        }
    }


CleanUpAndReturn:
    if ( pIHTMLTxtRangeOverlap )
        pIHTMLTxtRangeOverlap->Release();

    if ( pIHTMLElement2 )
        pIHTMLElement2->Release();

    if ( pIHTMLElement1 )
        pIHTMLElement1->Release();


    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::getRightMostTextRange()
//
//  DESCRIPTION:
//
//  Obtains the descendant element of the input element, and the
//  descendant's text range, whose text range has the rightmost end
//  point.  (For "rightmost", think of text ranges as being linear
//  extending left to right, the way we read.)  If the input element
//  is overlapped, this range will be the one that overlaps it the most.
//
//  This is and CAOMMgr::updateParentTextRangeEndPoint() are the key
//  methods to detecting and handling OVERLAPPED HTML!
//
//  PARAMETERS:
//
//      pIHTMLElement
//      pIHTMLTxtRangeRightMost
//      ppIHTMLElementRightMost
//
//  RETURNS:
//
//      S_OK if the text range was successfully obtained;
//      S_FALSE if the IHTMLElement has no children;
//      standard COM error otherwise.
//
//  NOTES:
//
//  To find the element with the rightmost text range end point,
//  get the last element of the input element's all collection.
//  Walk up the parent chain of this last element comparing text
//  ranges.  Keep the element with the range whose end point is
//  greatest.  Stop the parent chain walk when the input element
//  is encountered.
//
// ----------------------------------------------------------------------

HRESULT CAOMMgr::getRightMostTextRange( /* in */  IHTMLElement* pIHTMLElement,
                                        /* out */ IHTMLTxtRange* pIHTMLTxtRangeRightMost,
                                        /* out */ IHTMLElement** ppIHTMLElementRightMost )
{
    HRESULT         hr;
    IHTMLElement*   pIHTMLElementTemp = NULL;
    IHTMLElement*   pIHTMLElementParent = NULL;
    IHTMLTxtRange*  pIHTMLTxtRangeTemp = NULL;

#ifdef _DEBUG
    BSTR    bstrParentTag = NULL;
    BSTR    bstrElementTag = NULL;
#endif


    assert( pIHTMLElement );
    assert( pIHTMLTxtRangeRightMost );
    assert( ppIHTMLElementRightMost );
    assert( !*ppIHTMLElementRightMost );


    //--------------------------------------------------
    //  Get the last element object in the input
    //  element's all collection. 
    //--------------------------------------------------

    if ( hr = getLastAllItem( pIHTMLElement, ppIHTMLElementRightMost ) )
    {
        //--------------------------------------------------
        //  If hr equals S_FALSE, the element has no
        //  descendants; otherwise, hr equals a standard
        //  COM error.
        //--------------------------------------------------

        return hr;
    }
    assert( *ppIHTMLElementRightMost );


    //--------------------------------------------------
    //  Do the parent chain walk and text range
    //  comparisons.
    //--------------------------------------------------

    if ( hr = (*ppIHTMLElementRightMost)->get_parentElement( &pIHTMLElementParent ) )
        goto CleanUpAndReturn;


    if ( hr = pIHTMLTxtRangeRightMost->moveToElementText( *ppIHTMLElementRightMost ) )
    {
        // E_INVALIDARG is returned if for some reason trident could not set the range.
        //   the most likely cause for this is that the element was inside a button.
        //   e.g.  <BUTTON><U>C</U>lose</BUTTON>
        if (hr == E_INVALIDARG)
            hr = S_FALSE;

        goto CleanUpAndReturn;
    }


#ifdef _DEBUG
    assert( pIHTMLElementParent->get_tagName( &bstrParentTag ) == S_OK );
    assert( pIHTMLElement->get_tagName( &bstrElementTag ) == S_OK );
#endif


    if ( compareIHTMLElements( pIHTMLElementParent, pIHTMLElement ) )
    {
        if ( hr = pIHTMLTxtRangeRightMost->duplicate( &pIHTMLTxtRangeTemp ) )
            goto CleanUpAndReturn;

        do
        {
            long    lCompare;

            pIHTMLElementTemp = pIHTMLElementParent;
            pIHTMLElementParent = NULL;

            if ( hr = pIHTMLTxtRangeTemp->moveToElementText( pIHTMLElementTemp ) )
                goto CleanUpAndReturn;

            if ( hr = pIHTMLTxtRangeRightMost->compareEndPoints( m_bstrEndToEnd, pIHTMLTxtRangeTemp, &lCompare ) )
                goto CleanUpAndReturn;

            if ( lCompare == -1 )
            {
                if ( hr = setTextRangesEqual( pIHTMLTxtRangeRightMost, pIHTMLTxtRangeTemp ) )
                    goto CleanUpAndReturn;

                (*ppIHTMLElementRightMost)->Release();
                *ppIHTMLElementRightMost = pIHTMLElementTemp;
                (*ppIHTMLElementRightMost)->AddRef();
            }

            if ( hr = pIHTMLElementTemp->get_parentElement( &pIHTMLElementParent ) )
                goto CleanUpAndReturn;

            pIHTMLElementTemp->Release();
            pIHTMLElementTemp = NULL;

#ifdef _DEBUG
            if ( bstrParentTag )
                SysFreeString( bstrParentTag );

            if ( bstrElementTag )
                SysFreeString( bstrElementTag );

            bstrParentTag = NULL;
            bstrElementTag = NULL;

            assert( pIHTMLElementParent->get_tagName( &bstrParentTag ) == S_OK );
            assert( pIHTMLElement->get_tagName( &bstrElementTag ) == S_OK );
#endif
        }
        while ( compareIHTMLElements( pIHTMLElementParent, pIHTMLElement ) );
    }


CleanUpAndReturn:
    if ( hr )
    {
        (*ppIHTMLElementRightMost)->Release();
        *ppIHTMLElementRightMost = NULL;
    }

    if ( pIHTMLTxtRangeTemp )
        pIHTMLTxtRangeTemp->Release();

    if ( pIHTMLElementParent )
        pIHTMLElementParent->Release();

    if ( pIHTMLElementTemp )
        pIHTMLElementTemp->Release();


#ifdef _DEBUG
    if ( bstrParentTag )
        SysFreeString( bstrParentTag );

    if ( bstrElementTag )
        SysFreeString( bstrElementTag );
#endif

    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::getLastAllItem()
//
//  DESCRIPTION:
//
//  Obtains the last element in the input IHTMLElement's all collection.
//
//  PARAMETERS:
//
//      pIHTMLElement
//      ppIHTMLElementLastInAll
//
//  RETURNS:
//
//      S_OK if the text range was successfully obtained;
//      S_FALSE if the IHTMLElement has no children;
//      standard COM error otherwise.
// ----------------------------------------------------------------------

HRESULT CAOMMgr::getLastAllItem(    /* in */  IHTMLElement* pIHTMLElement,
                                    /* out */ IHTMLElement** ppIHTMLElementLastInAll )
{
    HRESULT                     hr;
    long                        lAllCollLen;
    ULONG                       ulFetched;
    IDispatch*                  pIDispatch = NULL;
    IHTMLElementCollection*     pIHTMLElementCollection = NULL;
    IUnknown*                   pIUnknown = NULL;
    IEnumVARIANT*               pIEnumVARIANT = NULL;


    assert( pIHTMLElement );
    assert( ppIHTMLElementLastInAll );
    assert( !*ppIHTMLElementLastInAll );


#ifdef _DEBUG
    BSTR    bstrTag = NULL;
    assert( pIHTMLElement->get_tagName( &bstrTag ) == S_OK );
    SysFreeString( bstrTag );
#endif


    //--------------------------------------------------
    // we want to treat the button as an atomic unit
    // (even if the html would suggest that the button
    // should have children) because we'll fail when
    // trying to resolve any text ranges of elements
    // scoped by the button due to a Trident bug
    //--------------------------------------------------

    CComQIPtr<IHTMLButtonElement,&IID_IHTMLButtonElement> pIHTMLButtonElement(pIHTMLElement);

    if ( pIHTMLButtonElement )
    {
        hr = S_FALSE;
        goto CleanUpAndReturn;
    }


    //--------------------------------------------------
    // get the input element's all collection
    //--------------------------------------------------

    if ( hr = pIHTMLElement->get_all( &pIDispatch ) )
    {
        if ( hr != E_INVALIDARG )
            goto CleanUpAndReturn;

        //--------------------------------------------------
        //  The get_all() call failed with E_INVALIDARG,
        //  but the argument is correct for the
        //  IHTMLElement::get_all() contract: the address
        //  of an IDispatch pointer! Something is amiss,
        //  and we are probably dealing with a proxy object
        //  rather than a element object.  Normalize the
        //  object and try get_all() again.
        //
        //  TODO:
        //
        //  Follow up with the Trident team to determine
        //  just what is happening.  Is this a Trident bug?
        //  If not, why does this only seem to happen with
        //  the FORM tags under IE4.01?
        //--------------------------------------------------

        CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElementNormal( pIHTMLElement );
        if ( !pIHTMLElementNormal )
        {
            hr = E_NOINTERFACE;
            goto CleanUpAndReturn;
        }

        if ( hr = normalizeIHTMLElement( &(pIHTMLElementNormal.p) ) )
            goto CleanUpAndReturn;

        if ( hr = pIHTMLElementNormal->get_all( &pIDispatch ) )
            goto CleanUpAndReturn;
    }
    assert( pIDispatch );

    if ( hr = pIDispatch->QueryInterface( IID_IHTMLElementCollection, (void**) &pIHTMLElementCollection ) )
        goto CleanUpAndReturn;
    assert( pIHTMLElementCollection );

    if ( hr = pIHTMLElementCollection->get_length( &lAllCollLen ) )
        goto CleanUpAndReturn;

    if ( lAllCollLen <= 0 )
    {
        hr = S_FALSE;
        goto CleanUpAndReturn;
    }

    if ( hr = pIHTMLElementCollection->get__newEnum( &pIUnknown ) )
        goto CleanUpAndReturn;
    assert( pIUnknown );

    if ( hr = pIUnknown->QueryInterface( IID_IEnumVARIANT, (void**) &pIEnumVARIANT ) )
        goto CleanUpAndReturn;
    assert( pIEnumVARIANT );

    if ( lAllCollLen > 1 )
        if ( hr = pIEnumVARIANT->Skip( lAllCollLen - 1 ) )
            goto CleanUpAndReturn;

    if ( hr = pIEnumVARIANT->Next( 1, m_pVars, &ulFetched ) )
        goto CleanUpAndReturn;
    assert( m_pVars[0].pdispVal );
    assert( ulFetched == 1 );

    hr = m_pVars[0].pdispVal->QueryInterface( IID_IHTMLElement, (void**) ppIHTMLElementLastInAll );

    m_pVars[0].pdispVal->Release();

#ifdef _DEBUG
    if ( hr == S_OK )
    {
        assert( *ppIHTMLElementLastInAll );

        BSTR    bstrTagName = NULL;
        assert( (*ppIHTMLElementLastInAll)->get_tagName( &bstrTagName ) == S_OK );
        SysFreeString( bstrTagName );
    }
#endif


CleanUpAndReturn:
    if ( pIEnumVARIANT )
        pIEnumVARIANT->Release();

    if ( pIUnknown )
        pIUnknown->Release();

    if ( pIHTMLElementCollection )
        pIHTMLElementCollection->Release();

    if ( pIDispatch )
        pIDispatch->Release();


    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::splitTextAE()
//
//  DESCRIPTION:
//
//  Breaks a single CTextAE into two.  The text range of the first
//  (original) text AE is fully scoped by the text range of the AE's
//  parent.  The text range of the second (new) text AE is the part
//  of the original range not scoped by the original AE's parent.
//
//  PARAMETERS:
//
//      pAOParent,
//      pTextAE,
//      pIHTMLTxtRangeParent,
//      pIHTMLTxtRangeTemp,
//      itInsertAfterPos
//
//  RETURNS:
//
//      S_OK if the CTextAE is successfully divided;
//      standard COM error otherwise.
//
// ----------------------------------------------------------------------

HRESULT CAOMMgr::splitTextAE(   /* in */ CTridentAO* pAOParent,
                                /* in */ CTextAE* pTextAE,
                                /* in */ IHTMLTxtRange* pIHTMLTxtRangeParent,
                                /* in */ IHTMLTxtRange* pIHTMLTxtRangeTemp,
                                /* in */ std::list<CAccElement *>::iterator itInsertAfterPos )
{
    HRESULT     hr;


    assert( pAOParent );
    assert( pTextAE );
    assert( pIHTMLTxtRangeParent );
    assert( pIHTMLTxtRangeTemp );


    //--------------------------------------------------
    //  Adjust pIHTMLTxtRangeTemp so it points to the
    //  to the text of the text AE's text range not scoped
    //  by pIHTMLTxtRangeParent.
    //--------------------------------------------------

    if ( hr = pIHTMLTxtRangeTemp->setEndPoint( m_bstrEndToEnd, pTextAE->GetTextRangePtr() ) )
        return hr;

    if ( hr = pIHTMLTxtRangeTemp->setEndPoint( m_bstrStartToEnd, pIHTMLTxtRangeParent ) )
        return hr;


#ifdef _DEBUG
    //--------------------------------------------------
    //  Ensure that the temp text range is contained
    //  by the text AE's text range but not by the
    //  parent's text range.
    //--------------------------------------------------

    VARIANT_BOOL    b;

    assert( pIHTMLTxtRangeParent->inRange( pIHTMLTxtRangeTemp, &b ) == S_OK );
    assert( b == 0 );

    assert( (pTextAE->GetTextRangePtr())->inRange( pIHTMLTxtRangeTemp, &b ) == S_OK );
    assert( b != 0 );
#endif


    //--------------------------------------------------
    //  Adjust the text range of pTextAE to point only
    //  to text scoped by the parent text range.
    //--------------------------------------------------

    if ( hr = (pTextAE->GetTextRangePtr())->setEndPoint( m_bstrEndToEnd, pIHTMLTxtRangeParent ) )
        return hr;


#ifdef _DEBUG
    //--------------------------------------------------
    //  Ensure that the text AE's text range is fully
    //  contained by the parent's text range.
    //--------------------------------------------------

    assert( pIHTMLTxtRangeParent->inRange( pTextAE->GetTextRangePtr(), &b ) == S_OK );
    assert( b != 0 );
#endif


    //--------------------------------------------------
    //  Create a CTextAE for the text range scoped by
    //  pIHTMLTxtRange.
    //--------------------------------------------------

    itInsertAfterPos++;

    hr = createTextAE(  m_pIHTMLDocument2,
                        m_pRootWindowAO->GetDocumentAO(),
                        pAOParent,
                        itInsertAfterPos,
                        pAOParent->GetWindowHandle(),
                        pIHTMLTxtRangeTemp );


    return hr;
}


//-----------------------------------------------------------------------
//  CAOMMgr::canObjectContainText()
//
//  DESCRIPTION:
//  evaluates whether the inputAO can or cannot contain text. Right now
//  only MAPs can't contain text
//
//  PARAMETERS:
//
//  pAO - AO to evalulate.
//
//  RETURNS:
//
//
// ----------------------------------------------------------------------

BOOL CAOMMgr::canObjectContainText(CTridentAO *pAO)
{

    assert(pAO);

    switch(pAO->GetAOMType())
    {
        case AOMITEM_MAP:
        case AOMITEM_AREA:  
        case AOMITEM_BUTTON:    
        case AOMITEM_CHECKBOX:
        case AOMITEM_EDITFIELD:
        case AOMITEM_RADIOBUTTON:
        case AOMITEM_IMAGE:
        case AOMITEM_WINDOW:
        case AOMITEM_FRAME:
            return(FALSE);
        default:
            return(TRUE);
    }
}

//-----------------------------------------------------------------------
//  CAOMMgr::compareForValidText()
//
//  DESCRIPTION:
//  takes two previously set text ranges, and compares them. The text
//  ranges are presumed to be in order: the start of the first one
//  is presumed to be in front of the start of the second one.
//  
//  Then finds any valid text between the start of the first text range 
//  and the start of the second text range, and creates CTextAE's for 
//  the valid text that it finds.
//
//  PARAMETERS:
//
//  pAOParent               pointer to the CTridentAO that contains the 
//                          text that is being evaluated for valid text
//                          objects.    
//  pIHTMLTxtRangeFirst     pointer to the first text range : its position
//                          is 'first' because it is placed before the 
//                          other text range parameter.
//
//  pIHTMLTxtRangeSecond    pointer to the second text range: its position
//                          is second because it is placed after the other
//                          text range parameter.
//
//  pIHTMLTxtRangeTemp      pointer to a temp range that will be used to
//                          create any text.
//
//  NOTE:
//  
//   TODO : determine whether we need to enforce the assumption about 
//   the text ranges, because even if they do not match the assumption,
//   the method still has valid logic.
//                      
//  RETURNS:
//
//  S_OK or standard COM error code.
// ----------------------------------------------------------------------


HRESULT CAOMMgr::compareForValidText(   /* in */    CTridentAO * pAOParent,
                                        /* in */    IHTMLTxtRange* pIHTMLTxtRangeFirst,
                                        /* in */    IHTMLTxtRange* pIHTMLTxtRangeSecond,
                                        /* in */    IHTMLTxtRange* pIHTMLTxtRangeTemp,
                                        /* in-out */std::list<CAccElement *>::iterator itCurPos)
{
    HRESULT hr = E_FAIL;
    BSTR bstrTmpText;
    long lCompare = 0;


    //--------------------------------------------------
    // validate inputs.
    //--------------------------------------------------

    assert(pAOParent);
    assert(pIHTMLTxtRangeFirst);
    assert(pIHTMLTxtRangeSecond);
    assert(pIHTMLTxtRangeTemp);
        
    
#ifdef _DEBUG

    if ( hr = pIHTMLTxtRangeFirst->get_text( &bstrTmpText )) 
        return(hr);

    SysFreeString(bstrTmpText);

    if ( hr = pIHTMLTxtRangeSecond->get_text( &bstrTmpText )) 
        return(hr);

    SysFreeString(bstrTmpText);

#endif

    if ( hr = pIHTMLTxtRangeFirst->compareEndPoints( m_bstrStartToStart, pIHTMLTxtRangeSecond, &lCompare ) )
        return hr;
    
    //--------------------------------------------------
    // if there is any text between the current position
    // of the parent text range and the start of the 
    // child text range, that is possible text.
    //--------------------------------------------------

    if ( lCompare == -1 )
    {

#ifdef _DEBUG

        if ( hr = pIHTMLTxtRangeTemp->get_text( &bstrTmpText )) 
            return(hr);

        SysFreeString(bstrTmpText);

#endif

        //--------------------------------------------------
        // Adjust the tail of the temp text range to point
        // to the head of the second text range.
        //--------------------------------------------------

        if (hr = pIHTMLTxtRangeTemp->setEndPoint( m_bstrEndToStart, pIHTMLTxtRangeSecond ))
            return hr;

#ifdef _DEBUG

        if ( hr = pIHTMLTxtRangeTemp->get_text( &bstrTmpText )) 
            return(hr);

        SysFreeString(bstrTmpText);

#endif
        
        //--------------------------------------------------
        // Adjust the head of the temp text range to point
        // to the head of the first text range.
        //--------------------------------------------------

        if (hr = pIHTMLTxtRangeTemp->setEndPoint( m_bstrStartToStart, pIHTMLTxtRangeFirst ))
            return hr;

#ifdef _DEBUG

        if ( hr = pIHTMLTxtRangeTemp->get_text( &bstrTmpText )) 
            return(hr);

        SysFreeString(bstrTmpText);

#endif

        //--------------------------------------------------
        // Create a text AE for the temp text range if
        // its text property has a non-zero length.
        //
        // NOTE: If we fail to get the text property of
        // the temp text range, just update the parent
        // text range and loop again.
        //--------------------------------------------------

        if ( (hr = pIHTMLTxtRangeTemp->get_text( &bstrTmpText )) == S_OK )
        {
            long lTmpTextStrLen = SysStringLen( bstrTmpText );

            SysFreeString( bstrTmpText );

            if ( lTmpTextStrLen )
            {
                if (hr = createTextAE( m_pIHTMLDocument2, m_pRootWindowAO->GetDocumentAO(), pAOParent, itCurPos, pAOParent->GetWindowHandle(), pIHTMLTxtRangeTemp ))
                    return hr;
            }
        }
    }

    return(S_OK);
}

//-----------------------------------------------------------------------
//  CAOMMgr::ignoreCurrentTextRange()
//
//  DESCRIPTION:
//  TODO : determine if this method is still needed.
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//
// ----------------------------------------------------------------------


HRESULT CAOMMgr::ignoreCurrentTextRange( /* in */     IHTMLTxtRange* pIHTMLTxtRangeCur,
                                         /* in-out */ IHTMLTxtRange* pIHTMLTxtRangePrev,
                                         /* in-out */ IHTMLTxtRange* pIHTMLTxtRangeMain )
{
    HRESULT hr;

    hr = pIHTMLTxtRangeMain->setEndPoint( m_bstrStartToEnd, pIHTMLTxtRangeCur );

    if ( hr != S_OK )
        return hr;

    hr = pIHTMLTxtRangePrev->setEndPoint( m_bstrEndToEnd, pIHTMLTxtRangeCur );

    if ( hr != S_OK )
        return hr;

    hr = pIHTMLTxtRangePrev->collapse( 0 );

    return hr;
}

//-----------------------------------------------------------------------
//  CAOMMgr::createTextAE()
//
//  DESCRIPTION:
//
//      Creates and initializes a CTextAE, and inserts it in 
//      the position specified by the caller.
//
//  PARAMETERS:
//
//      pIHTMLDocument2     pointer to the IHTMLDocument2 interface
//                          that represents the TOM Document object
//
//      pDocAO              pointer to owner document/frame object
//                          (AOs need this information at creation time)
//
//      pAOParent           pointer to the CTridentAO-derived object
//                          that is the AOM parent of the AOM item to
//                          be created
//
//      itInsertBeforePos   iterator to insert text before (if text
//                          is created).
//
//      hWnd                handle to the window for which MSAA events
//                          will be fired
//
//      pIHTMLTxtRange      pointer to the text range to be associated
//                          with the new CTextAE
//
//  RETURNS:
//
//      HRESULT             S_OK or a COM error code.
//
//  NOTES:
//
//      The caller of this method is responsible for ensuring that the
//      passed in iterator points to the correct insertion position
//      in the list. If the iterator points to the end of the list,
//      the text item will be inserted accordingly.
//
//-----------------------------------------------------------------------

HRESULT CAOMMgr::createTextAE( /* in */ IHTMLDocument2*     pIHTMLDocument2,
                               /* in */ CDocumentAO*        pDocAO,
                               /* in */ CTridentAO*         pAOParent,
                               /* in */ std::list<CAccElement *>::iterator itInsertBeforePos,
                               /* in */ HWND                hWnd,
                               /* in */ IHTMLTxtRange*      pIHTMLTxtRange )
{
    HRESULT hr = S_OK;

                
    //--------------------------------------------------
    //  Validate the parameters.
    // TODO : remove arg checking, replace w/assertions.
    //--------------------------------------------------

    if ( !pDocAO || !pAOParent || !pIHTMLTxtRange )
        return E_INVALIDARG;

    
    //--------------------------------------------------
    //  Don't create a text AE for a text range that
    //  only contains white space.
    //--------------------------------------------------

    BOOL    bOnlyWhiteSpace = FALSE;

    hr = isTextRangeTextOnlyWhiteSpace( pIHTMLTxtRange, &bOnlyWhiteSpace );

    // BUGBUG: if whitespace only, should we return an error? Otherwise the 
    //  caller assumes a text object has been created. The potential caveat
    //  is that we must ensure a whitespace object is truly whitespace.  
    //  Otherwise, unsupported objects might actually be created when nothing should
    //  be created.  For example, DBCS strings under non US Win 95 platforms
    //  might not pass the conventional white space test, and cause
    //  unsupported tags to be created.

    if ( bOnlyWhiteSpace  ||  hr != S_OK )
        return hr;


    //------------------------------------------------
    //  Create an IUnknown smart COM pointer from
    //  the IHTMLDocument2 pointer to initialize the
    //  CTridentAO-derived CTextAE object.
    //------------------------------------------------

    CComPtr<IUnknown> pIUnkTOMDoc(pIHTMLDocument2);

    if ( !pIUnkTOMDoc )
        return E_NOINTERFACE;


    //------------------------------------------------
    //  Create an IUnknown smart COM pointer from
    //  the IHTMLTxtRange pointer to initialize the
    //  CTridentAO-derived CTextAE object.
    //------------------------------------------------

    CComPtr<IUnknown> pIUnkTxtRng(pIHTMLTxtRange);

    if ( !pIUnkTxtRng )
        return E_NOINTERFACE;


    //--------------------------------------------------
    //  Create the CTextAE.
    //--------------------------------------------------

    CTextAE* pTextAE = new CTextAE( pAOParent, m_nCurrentAOMItemID, hWnd );

    if ( !pTextAE )
        return E_OUTOFMEMORY;
    else
        hr = pTextAE->Init( pIUnkTxtRng, pIUnkTOMDoc );


    //--------------------------------------------------
    //  If the CTextAE object was constructed but not
    //  initialized properly, clean up the mess.
    //--------------------------------------------------

    if ( hr != S_OK && pTextAE )
        pTextAE->Release();


    if ( hr == S_OK )
    {
        //------------------------------------------------
        //  Increment the current AOM item unique
        //  ID because we successfully created and 
        //  initialized a new AOM item.
        //------------------------------------------------

        m_nCurrentAOMItemID++;

        //------------------------------------------------
        //  insert the new text object in context correct
        //  order (see note above)
        //------------------------------------------------

        std::list<CAccElement *> * pAEList;

        if(hr = pAOParent->GetChildList(&pAEList))
            return(hr);


        if(itInsertBeforePos == pAEList->end())
        {
            pAEList->push_back(pTextAE);
        }
        else
        {
            pAEList->insert(itInsertBeforePos,pTextAE);
        }
    }

    return hr;
}




//-----------------------------------------------------------------------
//  CAOMMgr::isTextRangeTextOnlyWhiteSpace()
//
//  DESCRIPTION:
//   
//  returns TRUE if this text range is blank.
//
//  PARAMETERS:
//
//
//  RETURNS:
//
//
// ----------------------------------------------------------------------

HRESULT CAOMMgr::isTextRangeTextOnlyWhiteSpace( /* in */    IHTMLTxtRange*  pIHTMLTxtRange,
                                                /* out */   BOOL*           pbOnlyWhiteSpace )
{
    HRESULT hr;
    BSTR    bstrText;

    //--------------------------------------------------
    //  Validate the parameters.
    //--------------------------------------------------

    if ( !pIHTMLTxtRange || !pbOnlyWhiteSpace )
        return E_INVALIDARG;

    *pbOnlyWhiteSpace = TRUE;

    //--------------------------------------------------
    //  Get the text of the text range.
    //--------------------------------------------------

    hr = pIHTMLTxtRange->get_text( &bstrText );

    if ( hr != S_OK )
        return hr;

    if ( !bstrText )
        return E_FAIL;

    int nLen = SysStringLen( bstrText );

    for ( int i = 0; i < nLen; i++ )
    {
        // UNDONE: the punct and alphanum routines don't appear to work reliably 
        //  on Win 98/J for double byte chars.  Perhaps check for control
        //  characters too, or set the proper locale first.

//      if ( iswpunct( bstrText[i] ) || iswalnum( bstrText[i] ))
        if ( !iswspace( bstrText[i] ) )
        {
            *pbOnlyWhiteSpace = FALSE;
            break;
        }
    }

    return S_OK;
}

#endif  //__AOMMGR_ENABLETEXT

//  end of AOMMGR.CPP
