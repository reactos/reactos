//+------------------------------------------------------------------------
//
//  File:       CopyCmd.hxx
//
//  Contents:   CCopyCommand Class.
//
//  History:    10-16-98 raminh - moved from delcmd.hxx
//
//-------------------------------------------------------------------------

#ifndef _COPYCMD_HXX_
#define _COPYCMD_HXX_ 1

//
// MtExtern's
//
class CHTMLEditor;
class CSpringLoader;
    
MtExtern(CCopyCommand)

//+---------------------------------------------------------------------------
//
//  CCopyCommand Class
//
//----------------------------------------------------------------------------

class CCopyCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCopyCommand))

    CCopyCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CCopyCommand()
    {
    }
    
protected:
    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pcmd,
                                OLECMDTEXT * pcmdtext );
};


#endif

