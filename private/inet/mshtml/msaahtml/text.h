//================================================================================
//      File:   TEXT.H
//      Date:   7/28/97
//      Desc:   Contains definition of CTextAE class.  CTextAE implements 
//              the accessible proxy for the Trident Table object.
//
//      Author: Jay Clark
//================================================================================

#ifndef __TEXTAE__
#define __TEXTAE__

//================================================================================
// Includes
//================================================================================

#include "trid_ae.h"

//================================================================================
// Class forwards
//================================================================================

class CDocumentAO;

//================================================================================
// CTextAE class definition.
//================================================================================

class CTextAE : public CTridentAE
{
public:

    //------------------------------------------------
    // IUnknown methods
    //------------------------------------------------

    virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);

    
    //--------------------------------------------------
    // Internal IAccessible methods
    //
    // TODO: Add/Remove internal methods here
    //--------------------------------------------------

    virtual HRESULT GetAccName(long lChild, BSTR * pbstrName);

    virtual HRESULT GetAccState(long lChild, long *plState);

    virtual HRESULT AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild);
    
    virtual HRESULT AccDoDefaultAction(long lChild);

    virtual HRESULT GetAccDefaultAction(long lChild, BSTR * pbstrDefAction);

    virtual HRESULT AccSelect(long flagsSel, long lChild);

    virtual HRESULT GetAccValue(long lChild, BSTR * pbstrValue);

    virtual HRESULT GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut);

    //--------------------------------------------------
    // Constructors/Destructors
    //--------------------------------------------------

    CTextAE( CTridentAO* pAOParent, UINT nChildID, HWND hWnd );
    ~CTextAE();


    //--------------------------------------------------
    // Standard class methods
    //--------------------------------------------------

    virtual HRESULT Init( IUnknown* pTxtRngObjIUnk, IUnknown* pTOMDocIUnk );
    virtual void ReleaseTridentInterfaces ();

    HRESULT ContainsPoint(long xLeft,long yTop, IHTMLTxtRange * pIHTMLTxtRange);

    //--------------------------------------------------
    // access method to the text range pointer
    //--------------------------------------------------

    IHTMLTxtRange* GetTextRangePtr() { return m_pIHTMLTxtRange; }

    //--------------------------------------------------
    // access method to the TEO's IHTMLElement pointer
    //
    // The CTextAE is associated with a Trident text
    // range not an element object, so return NULL.
    //--------------------------------------------------

    virtual IHTMLElement* GetTEOIHTMLElement( void ) { return NULL; }


protected:
    
    //------------------------------------------------
    // methods
    //------------------------------------------------
    
    HRESULT isInClientWindow(long xLeft,long yTop, long cxWidth, long cyHeight);

    HRESULT getBoundingRect(RECT * pRect,BOOL bOrigin = FALSE);

    HRESULT containsTextRange(IHTMLTxtRange *pDocTxtRange);

    //------------------------------------------------
    // member variables
    //------------------------------------------------

    IHTMLDocument2*     m_pIHTMLDocument2;
    IHTMLTxtRange*      m_pIHTMLTxtRange;

};

#endif  // __TEXTAO__