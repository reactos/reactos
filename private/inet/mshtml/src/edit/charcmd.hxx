//+------------------------------------------------------------------------
//
//  File:       CharCmd.cxx
//
//  Contents:   Character Command Classes.
//
//  History:    08-28-98 ashrafm - rewrote 
//
//-------------------------------------------------------------------------

#ifndef _CHARCMD_HXX_
#define _CHARCMD_HXX_ 1

MtExtern(CBaseCharCommand)
MtExtern(CCharCommand)
MtExtern(CFontCommand)
MtExtern(CForeColorCommand)
MtExtern(CBackColorCommand)
MtExtern(CFontNameCommand)
MtExtern(CFontSizeCommand)
MtExtern(CAnchorCommand)
MtExtern(CRemoveFormatBaseCommand)
MtExtern(CRemoveFormatCommand)
MtExtern(CUnlinkCommand)

class CHTMLEditor;

//
// CBaseCharCommand contains the common phrase element command algorithm. 
//

class CBaseCharCommand : public CCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBaseCharCommand))

public:
    //
    // Exposed methods
    //
    
    HRESULT Apply( CMshtmlEd       *pCmdTarget,
                   IMarkupPointer  *pStart, 
                   IMarkupPointer  *pEnd,
                   VARIANT         *pvarargIn,
                   BOOL            fGenerateEmptyTags = FALSE);

    BOOL IsValidEditContext(IHTMLElement *pElement) {return TRUE;}
    //
    // Pure virtual methods
    //

    virtual HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOUT ) PURE;

    virtual BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB) PURE;

    virtual BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b) PURE;

    virtual BOOL IsCmdInFormatCache(IMarkupPointer *pMarkupPointer,	VARIANT *pvarargIn ) PURE;

    virtual HRESULT RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn) PURE;

    virtual HRESULT InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn) PURE;

    virtual HRESULT InsertStyleAttribute(IHTMLElement *pElement) PURE;

    virtual HRESULT RemoveStyleAttribute(IHTMLElement *pElement) PURE;

protected:    
    CBaseCharCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );

    //
    // Private Helpers
    //
    
    HRESULT PrivateApply( IMarkupPointer    *pStart, 
                          IMarkupPointer    *pEnd,
                          VARIANT           *pvarargIn,
                          BOOL              fGenerateEmptyTags);

    BOOL IsBlockOrLayout(IHTMLElement *pElement);
    
    HRESULT RemoveSimilarTags(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);

    HRESULT ExpandCommandSegment(IMarkupPointer *pStart, Direction direction, VARIANT *pvarargIn);

    HRESULT GetNormalizedTagId(IHTMLElement *pElement, ELEMENT_TAG_ID *ptagId);

    HRESULT TryTagMerge(IMarkupPointer *pCurrent);

    virtual BOOL AreAttributesEqual(IHTMLElement *pElementLeft, IHTMLElement *pElementRight)
        {return FALSE;} // false by default

    HRESULT ApplyCommandToWord(  VARIANT      * pvarargIn,
                                 VARIANT      * pvarargOut,
                                 ISegmentList * pSegmentList,
                                 BOOL           fApply = TRUE);

    HRESULT CreateAndInsert(ELEMENT_TAG_ID tagId, IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement);

    HRESULT PrivateRemove( IMarkupPointer  *pStart,
                           IMarkupPointer  *pEnd,
                           VARIANT         *pvarargIn = NULL);

    BOOL CanPushFontTagBack(IHTMLElement *pElement);

private:
    HRESULT InsertTags(IMarkupPointer *pStart, IMarkupPointer *pEnd, IMarkupPointer *pLimit, VARIANT *pvarargIn, BOOL fGenerateEmptyTags);

    HRESULT FindTagStart(IMarkupPointer *pCurrent, VARIANT *pvarargIn, IMarkupPointer *pEnd);
    
    HRESULT FindTagEnd(IMarkupPointer *pStart, IMarkupPointer *pCurrent, IMarkupPointer *pEnd);

protected:
    ELEMENT_TAG_ID _tagId;
}; 

//
// CCharCommand is used to implement B, U, I, SUP, SUB, etc..
//

class CCharCommand : public CBaseCharCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCharCommand))

public:
    HRESULT Remove( CMshtmlEd       *pCmdTarget,
                    IMarkupPointer  *pStart,
                    IMarkupPointer  *pEnd );

    HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOut );

    BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB);

    BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b);
    
    BOOL IsCmdInFormatCache(
		IMarkupPointer *	pMarkupPointer,
		VARIANT *			pvarargIn );

    BOOL AreAttributesEqual(IHTMLElement *pElementLeft, IHTMLElement *pElementRight);

protected:
    CCharCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );
    
    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pCmd,
                         OLECMDTEXT * pcmdtext );
        
    HRESULT RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn);

    HRESULT InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);

    HRESULT InsertStyleAttribute(IHTMLElement *pElement);

    HRESULT RemoveStyleAttribute(IHTMLElement *pElement);


};

//+---------------------------------------------------------------------------
//
//  CFontCommand Class
//
//----------------------------------------------------------------------------

class CFontCommand : public CBaseCharCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontCommand))

public:
    BOOL IsCmdInFormatCache(
		IMarkupPointer *	pMarkupPointer,
		VARIANT *			pvarargIn );

protected:
    CFontCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );

    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( 
    	OLECMD *            pcmd,
		OLECMDTEXT *        pcmdtext );    

    //
    // Private helpers
    //

    HRESULT GetCommandRange(VARIANTARG *pvarargOut);
        
    HRESULT GetSegmentListFontValue(
        ISegmentList    *pSegmentList,
        VARIANT         *pvar);
        
    CHAR_FORMAT_FAMILY GetCharFormatFamily();

    HRESULT RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn);

    HRESULT InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);
    
    HRESULT FindReuseableTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement);
    
    HRESULT InsertStyleAttribute(IHTMLElement *pElement)
        {return E_NOTIMPL;}

    HRESULT RemoveStyleAttribute(IHTMLElement *pElement) 
        {return E_NOTIMPL;}

    virtual HRESULT InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn) PURE;

    BOOL IsValidOnControl();
};

//+---------------------------------------------------------------------------
//
//  CForeColorCommand Class
//
//----------------------------------------------------------------------------

class CForeColorCommand : public CFontCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CForeColorCommand))

public:
    HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOut );

    BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB);

    BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b);

    HRESULT InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn);

protected:
    CForeColorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );
};

//+---------------------------------------------------------------------------
//
//  CBackColorCommand Class
//
//----------------------------------------------------------------------------
class CBackColorCommand : public CFontCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBackColorCommand))

public:
    HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOut );

    BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB);
    
    BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b);

    HRESULT InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn);

protected:
    CBackColorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );
};

//+---------------------------------------------------------------------------
//
//  CFontNameCommand Class
//
//----------------------------------------------------------------------------
class CFontNameCommand  : public CFontCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontNameCommand ))

public:
    HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOut );

    BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB);

    BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b);
    
    HRESULT InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn);

protected:
    CFontNameCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );
};

//+---------------------------------------------------------------------------
//
//  CFontSizeCommand Class
//
//----------------------------------------------------------------------------
class CFontSizeCommand  : public CFontCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontSizeCommand ))

public:
    HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOut );

    BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB);

    BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b);

    HRESULT InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn);

protected:
    CFontSizeCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );
};

//+---------------------------------------------------------------------------
//
//  CAnchorCommand Class
//
//----------------------------------------------------------------------------
class CAnchorCommand  : public CBaseCharCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAnchorCommand ))

public:
    HRESULT ConvertFormatDataToVariant(
        HTMLCharFormatData      &chFmtData,
        VARIANT                 *pvarargOut );

    BOOL IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB);

    BOOL IsFormatDataEqual(HTMLCharFormatData &a, HTMLCharFormatData &b);

    BOOL IsCmdInFormatCache(IMarkupPointer *pMarkupPointer,	VARIANT *   pvarargIn);
    
protected:
    CAnchorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd );

    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pCmd,
                         OLECMDTEXT * pcmdtext );

    HRESULT RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn);

    HRESULT InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);

    HRESULT InsertStyleAttribute(IHTMLElement *pElement) {return S_OK;}

    HRESULT RemoveStyleAttribute(IHTMLElement *pElement) {return S_OK;}

    BOOL IsCmdAbove(    IMarkupServices *pMarkupServices ,
                        IMarkupPointer* pStart,
                        IMarkupPointer* pEnd,
                        IHTMLElement**  ppFirstMatchElement,
                        elemInfluence * pInfluence ,
                        CTagBitField *  inSynonyms );

    BOOL IsValidOnControl();

    HRESULT InsertNamedAnchor(BSTR bstrName, IMarkupPointer *pStart, IMarkupPointer *pEnd);

    HRESULT UpdateAnchor(IHTMLElement *pElement, VARIANT *pvarargIn);
    
    HRESULT UpdateContainedAnchors(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn);
};

//+---------------------------------------------------------------------------
//
//  CRemoveFormatBaseCommand Class
//
//----------------------------------------------------------------------------

class CRemoveFormatBaseCommand : public CCommand
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRemoveFormatBaseCommand))

    CRemoveFormatBaseCommand (DWORD cmdId, CHTMLEditor *ped);
    virtual ~CRemoveFormatBaseCommand() {}

    HRESULT Apply(IMarkupPointer  *pStart, IMarkupPointer  *pEnd, BOOL fQueryMode = FALSE);
    
    virtual HRESULT RemoveElement(IHTMLElement *pElement, IMarkupPointer  *pStart, IMarkupPointer  *pEnd) PURE;
    
protected:
    //
    // Command target methods
    //

    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

protected:
    CTagBitField _tagsRemove; 
};

//+---------------------------------------------------------------------------
//
//  CRemoveFormat Class
//
//----------------------------------------------------------------------------

class CRemoveFormatCommand : public CRemoveFormatBaseCommand
{
    friend CHTMLEditor;
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRemoveFormatCommand))

protected:
    CRemoveFormatCommand(DWORD cmdId, CHTMLEditor *ped);

    HRESULT PrivateQueryStatus( OLECMD * pCmd, OLECMDTEXT * pcmdtext );

    HRESULT RemoveElement(IHTMLElement *pElement, IMarkupPointer  *pStart, IMarkupPointer  *pEnd);


};

//+---------------------------------------------------------------------------
//
//  CUnlinkCommand Class
//
//----------------------------------------------------------------------------

class CUnlinkCommand : public CRemoveFormatBaseCommand
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRemoveFormatCommand))

    CUnlinkCommand(DWORD cmdId, CHTMLEditor *ped);
    
    HRESULT PrivateQueryStatus( OLECMD * pCmd, OLECMDTEXT * pcmdtext );

    HRESULT RemoveElement(IHTMLElement *pElement, IMarkupPointer  *pStart, IMarkupPointer  *pEnd);

    BOOL IsValidOnControl();

};

#endif


