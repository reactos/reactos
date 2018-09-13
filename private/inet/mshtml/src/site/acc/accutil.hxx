//Prototypes for helper functions
BOOL    IsSupportedElement(CElement *pElem );
HRESULT ScrollIn_Focus_Click( CElement* pElem );
HRESULT ScrollIn_Focus( CElement* pElem );
BOOL    IsFocusable( CElement* pElem );
HRESULT GetHitChildID( CElement* pElem, CMessage* pMsg, VARIANT * pvarChild );
HRESULT AccStepRight( CMarkupPointer* pMarkup, long * pCounter );
HRESULT GetMarkupLimits( CElement* pElem, CMarkupPointer* pBegin, CMarkupPointer* pEnd );
HRESULT GetChildCount( CMarkupPointer* pStart, CMarkupPointer* pEnd, long* pChildCnt);
HRESULT GetTextFromMarkup( CMarkupPointer *pMarkup, BSTR * pbstrText );
CAccBase * GetAccObjOfElement( CElement* pElem );
HRESULT SelectText( CElement * pElem, IMarkupPointer * pBegin );
