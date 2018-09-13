//+------------------------------------------------------------------------
//
//  File:       PasteCmd.hxx
//
//  Contents:   CPasteCommand Class.
//
//  History:    10-16-98 raminh - moved from delcmd.hxx
//
//-------------------------------------------------------------------------

#ifndef _PASTECMD_HXX_
#define _PASTECMD_HXX_ 1

//
// MtExtern's
//
class CHTMLEditor;
class CSpringLoader;
    
MtExtern(CPasteCommand)


//+---------------------------------------------------------------------------
//
//  CPasteCommand Class
//
//----------------------------------------------------------------------------

class CPasteCommand : public CDeleteCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPasteCommand))

    CPasteCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CDeleteCommand( cmdId, pEd )
    {
    }

    virtual ~CPasteCommand()
    {
    }

    HRESULT Paste( IMarkupPointer* pStart, IMarkupPointer* pEnd, CSpringLoader * psl, BSTR bstrText = NULL );   

    HRESULT PasteFromClipboard( IMarkupPointer* pStart, IMarkupPointer* pEnd, IDataObject * pDO, CSpringLoader * psl );

    HRESULT InsertText( OLECHAR * pchText, long cch, IMarkupPointer * pPointerTarget, CSpringLoader * psl );
    
protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd,
                                OLECMDTEXT * pcmdtext );

private:

    HRESULT IsPastePossible ( IMarkupPointer * pStart,
                              IMarkupPointer * pEnd,
                              BOOL * pfResult );

};

#endif

