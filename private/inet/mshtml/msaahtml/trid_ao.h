//================================================================================
//      File:   TRID_AO.H
//      Date:   5/21/97
//      Desc:   contains definition of CTridentAO class.  CTridentAO is 
//              the base class for all accessible Trident objects. Also
//              contains definition for CTridentAO member objects 
//              CImplIAccessible and CImplIOleWindow, which implement 
//              the IAccessible and IOleWindow OLE interfaces.
//================================================================================

#ifndef __TRID_AO__
#define __TRID_AO__


//================================================================================
// includes
//================================================================================

#include <list>
#include "accelem.h"
#include "accobj.h"
#include "cache.h"

//================================================================================
// defines
//================================================================================

//--------------------------------------------------
// these #defines are used to determine which part
// of the bounding rect is visible.  The caller 
// uses these values to parse the returned DWORD 
// in the CTridentAO::getVisibleCorner() method.
//--------------------------------------------------

#define POINT_XMASK     0x00000FFF
#define POINT_XLEFT     0x00000001
#define POINT_XRIGHT    0x00000010
#define POINT_XMID      0x00000100

#define POINT_YMASK     0x00FFF000
#define POINT_YTOP      0x00001000
#define POINT_YBOTTOM   0x00010000
#define POINT_YMID      0x00100000

//================================================================================
// class forwards
//================================================================================

class CImplIAccessible;
class CImplIOleWindow;
class CEnumVariant;
class CDocumentAO;
class CAnchorAO;
class CAOMMgr;

//================================================================================
// CTridentAO class definition.
//================================================================================

class CTridentAO : public CAccObject
{

public:

    //------------------------------------------------
    // IUnknown
    //------------------------------------------------

    virtual STDMETHODIMP        QueryInterface(REFIID riid, void** ppv);

    //--------------------------------------------------
    // [10/7/97 Arunj] removed PURE specification 
    // unsupported tags are created as CTridentAO objects.
    //--------------------------------------------------

    //--------------------------------------------------
    // Internal IAccessible support 
    //--------------------------------------------------

    virtual HRESULT GetAccFocus(IUnknown **ppIUnknown);
    virtual HRESULT GetAccSelection(IUnknown **ppIUnknown);
    virtual HRESULT AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown);
    virtual HRESULT AccHitTest(long xLeft, long yTop,IUnknown **ppIUnknown);
    virtual HRESULT GetAccParent(IDispatch ** ppdispParent);
    virtual HRESULT GetAccChildCount(long* pChildCount);
    virtual HRESULT GetAccChild(long lChild, IDispatch ** ppdispChild);
    virtual HRESULT GetAccName(long lChild, BSTR * pbstrName);
    virtual HRESULT GetAccValue(long lChild, BSTR * pbstrValue);
    virtual HRESULT GetAccDescription(long lChild, BSTR * pbstrDescription);
    virtual HRESULT GetAccState(long lChild, long *plState);
    virtual HRESULT GetAccHelp(long lChild, BSTR * pbstrHelp);
    virtual HRESULT GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic);
    virtual HRESULT GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut);
    virtual HRESULT GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);
    virtual HRESULT AccSelect(long flagsSel, long lChild);
    virtual HRESULT AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);
    virtual HRESULT AccDoDefaultAction(long lChild);
    virtual HRESULT SetAccName(long lChild, BSTR bstrName);
    virtual HRESULT SetAccValue(long lChild, BSTR bstrValue);

    //--------------------------------------------------
    // Helper methods for interacting w/TEOs
    //--------------------------------------------------

    HRESULT ScrollIntoView(void);

    //--------------------------------------------------
    // Accessor methods
    //--------------------------------------------------

    long    GetTOMAllIndex() { return m_nTOMIndex; }
    virtual CDocumentAO * GetDocumentAO(void) { return(m_pDocAO); }
    HRESULT GetFocusedTOMElementIndex(UINT *puTOMIndex);
    virtual HRESULT GetAOMMgr(CAOMMgr ** ppAOMMgr);

    //--------------------------------------------------
    // access method to the TEO's IUnknown pointer
    //--------------------------------------------------

    LPUNKNOWN GetTEOIUnknown() { return m_pTOMObjIUnk; }

    //--------------------------------------------------
    // access method to the TEO's IHTMLElement pointer
    //--------------------------------------------------

    virtual IHTMLElement* GetTEOIHTMLElement() { return m_pIHTMLElement; }

    //--------------------------------------------------
    // allow parent resetting.
    //--------------------------------------------------

    void SetParent(CTridentAO *pNewParentAO) { m_pParent = pNewParentAO; }
    CTridentAO * GetParent() { return m_pParent; }

    //--------------------------------------------------
    // list/child access methods
    //--------------------------------------------------

    virtual HRESULT GetChildList(std::list<CAccElement *> **ppChildList);
    virtual HRESULT AddChild(CAccElement * pChild);

    /*
    //--------------------------------------------------
    // remove method for now until after 1.0  This 
    // method isnt necessary for a bug fix, but
    // should be re-instated once v2.0 work begins.
    //--------------------------------------------------

    virtual HRESULT RemoveChild(CAccElement * pChild);
    */

    virtual HRESULT GetAccElemFromChildID(  LONG lChild,CAccElement ** ppAEChild,
                                            std::list<CAccElement *>::iterator *ppitPos = NULL,BOOL 
                                            bFullTreeSearch = FALSE );
    virtual HRESULT GetAccElemFromTomIndex(LONG lIndex,CAccElement ** ppAccEl);
    HRESULT IsPointInTextChildren(  long xLeft,
                                    long yTop,
                                    IHTMLTxtRange * pDocTxtRange,
                                    CAccElement ** ppAccEl);
    BOOL HasChildren(void)
    {
        if ( m_AEList.size() )
            return(TRUE);
        else
            return(FALSE);
    }

    //--------------------------------------------------
    // detach related methods
    //--------------------------------------------------

    BOOL    IsDetached () { return (m_bDetached); }
    HRESULT DetachChildren ();
    void    Detach ();
    BOOL    IsZombified () { return m_bZombified; }
    virtual void    Zombify();
    virtual BOOL    DoBlockForDetach ();

    //--------------------------------------------------
    // derived classes that cache Trident iface pointers
    // need to override this method and call down to
    // the base class's method
    //--------------------------------------------------

    virtual void    ReleaseTridentInterfaces ();

    //--------------------------------------------------
    // standard object methods
    //--------------------------------------------------

    CTridentAO( CTridentAO * pAOParent,
                CDocumentAO * pDocAO,
                UINT nTOMIndex,
                UINT nChildID,
                HWND hWnd,
                BOOL bUnSupportedTag = FALSE);

    virtual ~CTridentAO();
    virtual HRESULT Init(IUnknown* pTOMObjIUnk);

    //--------------------------------------------------
    // AOMMgr will set and get resolved state when
    // building the tree underneath this item.
    //--------------------------------------------------

    void SetResolvedState(BOOL bResolvedState)
    {
        m_bResolvedState = bResolvedState;
    }

    BOOL GetResolvedState(void)
    {
        return(m_bResolvedState);
    }

    //--------------------------------------------------
    // In certain scenarios, objects need to look up the
    // tree to see if they are ancestored by an anchor.
    //--------------------------------------------------

    HRESULT IsAncestorAnchor( CAnchorAO** ppAO );

protected:

    //--------------------------------------------------
    // WARNING: If a derived class does not call
    //          CTridentAO::Init(), it must call
    //          createInterfaceImplementors() to
    //          create the CImplIAccessible and
    //          CImplIOleWindow members.
    //          (CTridentAO::Init() calls this method
    //          internally.)
    //--------------------------------------------------

    HRESULT createInterfaceImplementors( LPUNKNOWN pIUnk = NULL );

    //--------------------------------------------------
    // methods
    //--------------------------------------------------

    virtual HRESULT ensureResolvedTree(void);
    HRESULT virtual freeChildren(void);
    void CTridentAO::freeDataMembers(void); 
    HRESULT createEnumerator( CEnumVariant** ppcenum, std::list<CAccElement *> *pAccElemList = NULL );
    HRESULT getSelectedChildren( IUnknown** ppIUnknown );
    HRESULT getTitleFromIHTMLElement(BSTR *pbstrTitle);
    HRESULT adjustOffsetToRootParent(long * pxLeft,long * pyTop);
    HRESULT adjustOffsetForClientArea(long *pxLeft,long *pyTop);
    HRESULT click(void);
    virtual HRESULT getVisibleCorner(POINT * pPt,DWORD * pdwCorner);
    virtual HRESULT resolveNameAndDescription( void );
    virtual HRESULT getDescriptionString( BSTR* pbstrDescStr );

    //------------------------------------------------
    // member variables
    //------------------------------------------------

    CTridentAO     *m_pParent;          // pointer to parent object.
    UINT            m_nTOMIndex;        // all collection index
    IUnknown       *m_pTOMObjIUnk;      // pointer to Trident Object base interface.
    BOOL            m_bOffScreen;       // offscreen flag for element : used
                                        // to determine state.
    BOOL            m_bResolvedState;   // flag is set when children of 
                                        // AO have been fully resolved.
    BOOL            m_bDetached;        // flag indicating this AO was ref'd by a client 
                                        // but removed from the tree due to trident 
                                        // window-unload
    BOOL            m_bZombified;       // flag indicating this AO has released all its
                                        // trident iface ptrs due to trident window-unload
    CDocumentAO     *m_pDocAO;          // pointer to document object.
    BOOL            m_bNameAndDescriptionResolved;
    CImplIAccessible *m_pImplIAccessible;// embedded OLE interface implementation
    CImplIOleWindow  *m_pImplIOleWindow; //  classes.
    
    //------------------------------------------------
    // Cached members for data that is static for the
    // lifetime of the AE/AO. Initialize these in
    // derived classes.
    //------------------------------------------------

    CCache          m_cache;
    IHTMLElement    *m_pIHTMLElement;
    IHTMLStyle      *m_pIHTMLStyle;
    BSTR            m_bstrStyle;
    BSTR            m_bstrDisplay;

    BSTR            m_bstrName;
    BSTR            m_bstrValue;
    BSTR            m_bstrDescription;
    BSTR            m_bstrDefaultAction;
    BSTR            m_bstrKbdShortcut;
    std::list<CAccElement *>    m_AEList;

#ifdef _DEBUG
    TCHAR           m_szAOMName[256];   // meaningful object name for debug
#endif
};

#endif  // __TRID_AO__