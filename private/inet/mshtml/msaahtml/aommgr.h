//=======================================================================
//      File:   AOMMGR.H
//      Date:   6-2-97
//      Desc:   Contains the definition of CAOMMgr class.  
//=======================================================================

#ifndef __AOMMGR__
#define __AOMMGR__


//================================================================================
//  Includes
//================================================================================

#ifdef __AOMMGR_ENABLETEXT

#include <deque>
#include <stack>

#endif


    //--------------------------------------------------
    // Include the definition of INOOBJECTFOCUSED.
    //--------------------------------------------------

#include "focusdef.h"


//================================================================================
//  Class forwards
//================================================================================

class CWindowAO;
class CTOMMapMgr;
class CTextAE;


//================================================================================
// #defines, enums, ect
//================================================================================

#define IPOINT_UNINITIALIZED    -1
#define INOCURRENTMAPINDEX      -1

    //--------------------------------------------------
    // _BUILDSTATE_ used to determine what state
    // the build manager is in.  It can only be in
    // one of the three states.
    //--------------------------------------------------

enum _BUILDSTATE_
{
    IHITTESTBUILDING,
    IFULLCHILDBUILDING,
    IFOCUSEDOBJECTBUILDING
};


//=======================================================================
//  CAOMMgr class definition
//=======================================================================

class CAOMMgr
{
public:

    //------------------------------------------------
    //  Constructor & destructor methods
    //------------------------------------------------

    CAOMMgr(CWindowAO * pRootWindowAO);
    ~CAOMMgr();


    //--------------------------------------------------
    // startup/reset/cleanup methods.
    //--------------------------------------------------

    HRESULT Init(IHTMLDocument2 * pIHTMLDocument2);
    
    void ResetState() 
    {
        m_PtHitTest.x   = IPOINT_UNINITIALIZED;
        m_PtHitTest.y   = IPOINT_UNINITIALIZED;
        m_lFocusID      = INOOBJECTFOCUS;
        m_iBuildState   = IFULLCHILDBUILDING;

        unsetHitList( &m_PtHitList );
    }

    virtual void    ReleaseTridentInterfaces ();

    //------------------------------------------------
    // AOM Tree creation methods :
    //
    // GetAccessibleObject() builds the tree directionally,
    // based on an input point.
    //------------------------------------------------
    
    HRESULT GetAccessibleObjectFromPt(  /* in */    CTridentAO * pAOToResolve, 
                                        /* in */    POINT * pPtHitTest, 
                                        /* out */   CAccElement ** ppRetAccEl);

    //--------------------------------------------------
    // GetAccessibleObjectFromID builds the tree directionally
    // based on input ID.
    //--------------------------------------------------

    HRESULT GetAccessibleObjectFromID(  /* in */    CTridentAO *pStartAO,
                                        /* in */    long lObjectID, 
                                        /* out */   CAccElement **ppAE); 

    //--------------------------------------------------
    // GetFocusedObjectFromID builds the tree directionally
    // based on input ID.
    //--------------------------------------------------

    HRESULT GetFocusedObjectFromID( /* in */    CTridentAO* pStartAO,
                                    /* in */    long lFocusedObjectTOMID, 
                                    /* out */   CAccElement** ppAE );

    //--------------------------------------------------
    // ResolveChildren() builds the full child list
    // of an input CTridentAO object, making sure that
    // all children of the object are supported in the 
    // AOM.
    //--------------------------------------------------

    HRESULT ResolveChildren(        /* in */    CTridentAO * pAOToResolve );

    

    //--------------------------------------------------
    // GetAOMID() returns the current AOMID for 
    // creation of objects external to the AOMMGR.
    //--------------------------------------------------

    long GetAOMID(void) { return(m_nCurrentAOMItemID); }

    //--------------------------------------------------
    // IncrementAOMID() increments the AOMID by 1 
    // externally (for when objects are being created 
    // external to the AOMMgr)
    //--------------------------------------------------

    void IncrementAOMID(void) { m_nCurrentAOMItemID++; }

protected:

    //------------------------------------------------
    //  m_nCurrentAOMItemID holds the value to be
    //  assigned as the unique ID of the next AOM
    //  item (a CTridentAO or CTridentAE) created.
    //------------------------------------------------

    long            m_nCurrentAOMItemID;
    CWindowAO  *    m_pRootWindowAO;

    long            m_iBuildState;
    POINT           m_PtHitTest;
    long            m_lFocusID;

    long            m_lCurrentMapIndex;

    VARIANT*        m_pVars;
    long            m_lCurSizeVariantArray;

    CTOMMapMgr*     m_pTOMMapMgr;

    IHTMLDocument2          *m_pIHTMLDocument2;
    IHTMLElementCollection  *m_pIHTMLAllCollection;

    std::list<IHTMLElement *> m_PtHitList;
    std::list<IHTMLElement *>::iterator m_itCurPtHitListPos;


#ifdef __AOMMGR_ENABLETEXT

    IHTMLTxtRange * m_pIHTMLTxtRangeParent;
    IHTMLTxtRange * m_pIHTMLTxtRangeChild;
    IHTMLTxtRange * m_pIHTMLTxtRangeTemp;

    BSTR            m_bstrStartToStart;
    BSTR            m_bstrStartToEnd;
    BSTR            m_bstrEndToStart;
    BSTR            m_bstrEndToEnd;

#endif //__AOMMGR_ENABLETEXT

    //------------------------------------------------
    //  Protected AOM Tree creation methods
    //------------------------------------------------

    HRESULT setFocusHitList(    /* in */    long lObjectID, 
                                /* out */   std::list<IHTMLElement *> *pList );

    HRESULT setAreaFocusedHitList(  /* in */    IHTMLElement* pIHTMLElemArea,
                                    /* out */   std::list<IHTMLElement *> *pList );

    HRESULT setHitList( /* in */    POINT pt, 
                        /* out */   std::list<IHTMLElement *> *pList);

    HRESULT setHitList( /* in */    long lObjectID, 
                        /* out */   std::list<IHTMLElement *> *pList);

    HRESULT setHitList( /* in */    IHTMLElement * pIHTMLElement,
                        /* out */   std::list<IHTMLElement *> *pList);

    HRESULT unsetHitList(/* in */   std::list<IHTMLElement *> *pList);

    HRESULT getNormalizedParent(    /* in */    IHTMLElement * pIHTMLElement, 
                                    /* out  */  IHTMLElement ** ppIHTMLElementParent);

    HRESULT normalizeIHTMLElement(  /* in-out */ IHTMLElement** ppIHTMLElement );

    HRESULT isHitObject(    /* in */ CTridentAO * pAOParent,
                            /* in */ CAccElement * pAccEl);

    HRESULT isChildMarked(  /* in */        std::list<CAccElement *>::iterator itCurPos,
                            /* in */        std::list<IHTMLElement *> * pHitList,
                            /* in-out */    std::list<IHTMLElement *>::iterator * pCurHitListPos);


    HRESULT isElementMarked(    /* in */        IHTMLElement * pIHTMLElement,
                                /* in */        std::list<IHTMLElement *> * pHitList,
                                /* in-out */    std::list<IHTMLElement *>::iterator *pitCurHitElementPos,
                                /* out  */      BOOL * pIsElementHit = NULL);
    
    HRESULT getAOMChildren( /* in */        CTridentAO*     pAOParent);
    
    HRESULT getHitAOMChild( /* in */        CTridentAO*     pAOParent,
                            /* in */        std::list<IHTMLElement *> *pHitList,
                            /* in-out */    std::list<IHTMLElement *>::iterator * pitCurHitListPos,
                            /* out */       std::list<CAccElement *>::iterator * pitCurPos,
                            /* out */       CAccElement ** ppRetAccEl);
    
    HRESULT collapse(   /* in */    CTridentAO *pParentAO,
                        /* in */    CTridentAO *pAOToCollapse,
                        /* out */   std::list<CAccElement *>::iterator *pitNewChildPos); 

    HRESULT enforceAOMTreeRules(    /* in */    CTridentAO * pAOParent,
                                    /* in */    CAccElement * pAccElement,
                                    /* out */   CAccElement **ppAccElement);

    HRESULT fullyResolveChildren(   /* in */ CTridentAO * pParentAO);

    HRESULT getIHTMLElement(    /* in */    CAccElement * pAccEl,
                                /* out */   IHTMLElement ** ppIHTMLElement);

    HRESULT compareIHTMLElements(   /* in */    CTridentAO* pAO,
                                    /* in */    IHTMLElement* pIHTMLElement );

    HRESULT compareIHTMLElements(   /* in */    CAccElement* pAccElem,
                                    /* in */    IHTMLElement* pIHTMLElement );

    HRESULT compareIHTMLElements(   /* in */    IHTMLElement* pIHTMLElem1,
                                    /* in */    IHTMLElement* pIHTMLElem2 );

    HRESULT createAOMItem( /* in */  IHTMLElement*          pIHTMLElement,
                           /* in */  CDocumentAO*           pDocAO,
                           /* in */  CTridentAO*            pAOParent,
                           /* in */  ULONG                  lAOMItemType,
                           /* in */  HWND                   hWnd,
                           /* out */ CAccElement**          ppAccElem );

    
    HRESULT setTEOType(     /* in */  IHTMLElement*         pIHTMLElement,
                            /* out */ ULONG*                plAOMItemType );

    HRESULT doesTOMAnchorHaveHREF( /* in */  IHTMLElement*  pIHTMLElement,
                                   /* out */ ULONG*         plAOMItemType );

    HRESULT isTOMInputTypeSupported( /* in */  IHTMLElement*    pIHTMLElement,
                                     /* out */ ULONG*           plAOMItemType );

    HRESULT getImageAtPoint(    /* in */    IHTMLElement *pIHTMLElement,
                                /* in */    CDocumentAO * pDocAO,
                                /* in */    POINT ptTest,
                                /* out */   CAccElement **ppAccImageEl);

    HRESULT processTOMImage(    /* in */    IHTMLElement*   pTEOImage,
                                /* in */    CDocumentAO*    pDocAO, 
                                /* in */    CTridentAO*     pAOImage,
                                /* in */    HWND            hWnd );

    HRESULT createAOMMap(       /* in */    IHTMLElement*       pIHTMLElement,
                                /* in */    IHTMLMapElement*    pIHTMLMapElement,
                                /* in */    CDocumentAO*        pDocAO,
                                /* in */    CTridentAO*         pAOParent,
                                /* in */    HWND                hWnd );

    HRESULT createAOMMap(       /* in */    IHTMLElement*       pIHTMLElement,
                                /* in */    CDocumentAO*        pDocAO,
                                /* in */    CTridentAO*         pAOParent,
                                /* in */    HWND                hWnd );

    HRESULT createAOMMap(       /* in */    IHTMLMapElement*    pIHTMLMapElement,
                                /* in */    CDocumentAO*        pDocAO,
                                /* in */    CTridentAO*         pAOParent,
                                /* in */    HWND                hWnd );

    BOOL ptHasChanged(POINT pt);

    HRESULT reallocateVariantArray( /* in */ long lNewSize );


#ifdef __AOMMGR_ENABLETEXT

    BOOL canObjectContainText(CTridentAO *pAO);

    HRESULT initializeTextRangeBSTRs( void );

    HRESULT getDocumentTxtRanges(   /* out */ IHTMLTxtRange** ppIHTMLTxtRangeParent,
                                    /* out */ IHTMLTxtRange** ppIHTMLTxtRangeChild,
                                    /* out */ IHTMLTxtRange** ppIHTMLTxtRangeTemp );

    HRESULT getText(    /* in */    CTridentAO*     pAOParent,
                        /* in */    IHTMLTxtRange*  pIHTMLTxtRangeParent,
                        /* in */    IHTMLTxtRange*  pIHTMLTxtRangeChild,
                        /* in */    IHTMLTxtRange*  pIHTMLTxtRangeTemp );

    HRESULT setTextRangesEqual( /* in */ IHTMLTxtRange* pIHTMLTxtRangeDst,
                                /* in */ IHTMLTxtRange* pIHTMLTxtRangeSrc );

    HRESULT updateParentTextRangeEndPoint(  /* in */  IHTMLTxtRange*    pIHTMLTxtRangeParent,
                                            /* in */  IHTMLTxtRange*    pIHTMLTxtRangeChild,
                                            /* in */  IHTMLTxtRange*    pIHTMLTxtRangeTemp,
                                            /* in */  IHTMLElement*     pIHTMLElementChild );

    HRESULT getRightMostTextRange(  /* in */  IHTMLElement*     pIHTMLElement,
                                    /* out */ IHTMLTxtRange*    pIHTMLTxtRangeRightMost,
                                    /* out */ IHTMLElement**    ppIHTMLElementRightMost );

    HRESULT getLastAllItem( /* in */  IHTMLElement*     pIHTMLElement,
                            /* out */ IHTMLElement**    ppIHTMLElementLastInAll );

    HRESULT splitTextAE(    /* in */    CTridentAO*     pAOParent,
                            /* in */    CTextAE*        pTextAE,
                            /* in */    IHTMLTxtRange*  pIHTMLTxtRangeParent,
                            /* in */    IHTMLTxtRange*  pIHTMLTxtRangeTemp,
                            /* in */    std::list<CAccElement *>::iterator  itInsertAfterPos );

    HRESULT compareForValidText(    /* in */    CTridentAO * pAOParent,
                                    /* in */    IHTMLTxtRange* pIHTMLTxtRangeFirst,
                                    /* in */    IHTMLTxtRange* pIHTMLTxtRangeSecond,
                                    /* in */    IHTMLTxtRange* pIHTMLTxtRangeTemp,
                                    /* in-out */std::list<CAccElement *>::iterator itCurPos);

    HRESULT createTextAE(   /* in */ IHTMLDocument2*    pIHTMLDocument2,
                            /* in */ CDocumentAO*       pDocAO,
                            /* in */ CTridentAO*        pAOParent,
                            /* in */ std::list<CAccElement *>::iterator itInsertBeforePos,
                            /* in */ HWND               hWnd,
                            /* in */ IHTMLTxtRange*     pIHTMLTxtRange );

    HRESULT isTextRangeTextOnlyWhiteSpace(  /* in */        IHTMLTxtRange*  pIHTMLTxtRange,
                                            /* out */   BOOL*           pbOnlyWhiteSpace );

    HRESULT ignoreCurrentTextRange( /* in */     IHTMLTxtRange* pIHTMLTxtRangeCur,
                                    /* in-out */ IHTMLTxtRange* pIHTMLTxtRangePrev,
                                    /* in-out */ IHTMLTxtRange* pIHTMLTxtRangeMain );

#endif  //__AOMMGR_ENABLETEXT
};

#endif  //__AOMMGR__
