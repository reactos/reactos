//+------------------------------------------------------------------------
//
//  File:       DelCmd.hxx
//
//  Contents:   CDeleteCommand Class.
//
//  History:    07-14-98 ashrafm - moved from edcom.hxx
//
//-------------------------------------------------------------------------

#ifndef _CUTCMD_HXX_
#define _CUTCMD_HXX_ 1

//
// MtExtern's
//
class CHTMLEditor;
class CSpringLoader;
    
MtExtern(CCutCommand)

//+---------------------------------------------------------------------------
//
//  CCutCommand Class
//
//----------------------------------------------------------------------------

class CCutCommand : public CDeleteCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCutCommand))

    CCutCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CDeleteCommand( cmdId, pEd )
    {
    }

    virtual ~CCutCommand()
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

