//+------------------------------------------------------------------------
//
//  File:       MiscCmd.hxx
//
//  Contents:   Miscellaneous Edit Command Classes.
//
//  History:    08-05-98 OliverSe - created for IDM_COMPOSESETTINGS
//
//-------------------------------------------------------------------------

#ifndef _MISCCMD_HXX_
#define _MISCCMD_HXX_ 1

struct COMPOSE_SETTINGS;

MtExtern(CComposeSettingsCommand)
MtExtern(COverwriteCommand)
MtExtern(CAutoDetectCommand)
MtExtern(CMovePointerToSelectionCommand)
MtExtern(CLocalizeEditorCommand)

//+------------------------------------------------------------------------
//  Class: CComposeSettingsCommand
//-------------------------------------------------------------------------

class CComposeSettingsCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CComposeSettingsCommand))
    
    CComposeSettingsCommand(DWORD cmdId, CHTMLEditor * pEd) : CCommand(cmdId, pEd) {}

    static void ExtractLastComposeSettings(CHTMLEditor * pEditor, BOOL fEditorHasComposeSettings);
    static void Init();
    static void Deinit();

protected:

    HRESULT PrivateExec(DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut);
    HRESULT PrivateQueryStatus(OLECMD * pCmd, OLECMDTEXT * pcmdtext);

    HRESULT QueryComposeSettings(VARIANTARG * pvarargIn, VARIANTARG * pvarargOut);

    static HRESULT ParseComposeSettings(BSTR bstrComposeSettings, struct COMPOSE_SETTINGS * pComposeSettings);
    static void    SetDefaultComposeSettings(struct COMPOSE_SETTINGS * pComposeSettings);

    // BUGBUG: This is a hack for Outlook 98 to keep signatures from losing their compose settings as the
    // editor is destroyed and recreated.  40810
    static CRITICAL_SECTION s_csLastComposeSettings;
    static BSTR             s_bstrLastComposeSettings;
}; 


//+------------------------------------------------------------------------
//  Class: COverwriteCommand
//-------------------------------------------------------------------------

class COverwriteCommand: public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(COverwriteCommand))
    
    COverwriteCommand( DWORD cmdId, CHTMLEditor * pEd ) : CCommand(cmdId, pEd) {}

protected:

    HRESULT PrivateExec( DWORD nCmdexecopt,
                         VARIANTARG * pvarargIn,
                         VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pCmd,
                                OLECMDTEXT * pcmdtext );
}; 

//+------------------------------------------------------------------------
//  Class: CAutoDetectCommand
//-------------------------------------------------------------------------

class CAutoDetectCommand: public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAutoDetectCommand))
    
    CAutoDetectCommand( DWORD cmdId, CHTMLEditor * pEd ) : CCommand(cmdId, pEd) {}

protected:

    HRESULT PrivateExec( DWORD nCmdexecopt,
                         VARIANTARG * pvarargIn,
                         VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( OLECMD * pCmd,
                                OLECMDTEXT * pcmdtext );
}; 

//+---------------------------------------------------------------------------
//
//  CLocalizeEditorCommand Class
//
//----------------------------------------------------------------------------

class CLocalizeEditorCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLocalizeEditorCommand))

    CLocalizeEditorCommand(DWORD cmdId, CHTMLEditor * pEd)
    : CCommand(cmdId, pEd) 
    {
    }

    virtual ~CLocalizeEditorCommand()
    {
    }       

protected:    
    HRESULT PrivateExec( 
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus( 
        OLECMD * pcmds,
        OLECMDTEXT * pcmdtext );
};


#endif // ndef _MISCCMD_HXX_
