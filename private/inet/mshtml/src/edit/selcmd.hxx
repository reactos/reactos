//+------------------------------------------------------------------------
//
//  File:       SelCmd.hxx
//
//  Contents:   Selection commands.
//
//  History:    09-01-98 ashrafm - created
//
//-------------------------------------------------------------------------

#ifndef _SELCMD_HXX_ 
#define _SELCMD_HXX_ 1

//
// MtExtern's
//

MtExtern(CSelectAllCommand)
MtExtern(CClearSelectionCommand)


//+---------------------------------------------------------------------------
//
//  CSelectAllCommand Class
//
//----------------------------------------------------------------------------

class CSelectAllCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CSelectAllCommand))

    CSelectAllCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CSelectAllCommand()
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

//+---------------------------------------------------------------------------
//
//  CClearSelectionCommand Class
//
//----------------------------------------------------------------------------

class CClearSelectionCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CClearSelectionCommand))

    CClearSelectionCommand(DWORD cmdId, CHTMLEditor * pEd )
    :  CCommand( cmdId, pEd )
    {
    }

    virtual ~CClearSelectionCommand()
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

