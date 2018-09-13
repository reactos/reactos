#ifndef I_MARKUPCTX_HXX_
#define I_MARKUPCTX_HXX_
#pragma INCMSG("--- Beg 'markupctx.hxx'")

MtExtern(CMarkupContext)

class CHtmlComponent;

//+------------------------------------------------------------------------
//
//  Class:  CMarkupContext
//
//-------------------------------------------------------------------------

class CMarkupContext : public CVoid
{
public: 
    DECLARE_CLASS_TYPES(CMarkupContext, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMarkupContext))

    //
    // methods
    //

    CMarkupContext();
    ~CMarkupContext();

    inline CHtmlComponent * HtmlComponent() { return _pHtmlComponent; }

    //
    // data
    //

    CMarkup *            _pMarkup;

    //
    // HTC support (html components)
    //

    CHtmlComponent *    _pHtmlComponent;
};

#pragma INCMSG("--- End 'markupctx.hxx'")
#else
#pragma INCMSG("*** Dup 'markupctx.hxx'")
#endif
 