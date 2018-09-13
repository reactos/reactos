//+------------------------------------------------------------------------
//
//  File:       DlgCmd.hxx
//
//  Contents:   CDialogCommand Class.
//
//  History:    08-17-98 
//
//-------------------------------------------------------------------------

#ifndef _DLGCMD_HXX_
#define _DLGCMD_HXX_ 1

class CHTMLEditor;

//
// MtExtern's
//
    
MtExtern(CDialogCommand)


//+---------------------------------------------------------------------------
//
//  CDialogCommand Class
//
//----------------------------------------------------------------------------

class CDialogCommand : public CCommand
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDialogCommand));

    CDialogCommand(DWORD cmdId, CHTMLEditor * ped );

    ~CDialogCommand();
    
protected:

    HRESULT PrivateExec( DWORD nCmdexecopt,
                  VARIANTARG * pvarargIn,
                  VARIANTARG * pvarargOut );

    HRESULT PrivateQueryStatus(OLECMD * pCmd,
                            OLECMDTEXT * pcmdtext );

private:

    HRESULT CreateResourceMoniker(HINSTANCE hInst, TCHAR *pchRID, IMoniker **ppmk);

    HRESULT ShowEditDialog(UINT idm, VARIANT * pvarExecArgIn, HWND hwndParent, 
                           VARIANT * pvarArgReturn, IMarkupServices * pMarkupServices);
};


#endif

