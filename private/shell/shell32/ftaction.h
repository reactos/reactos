#ifndef FTACTION_H
#define FTACTION_H

#include "ftdlg.h"

class CFTActionDlg : public CFTDlg
{
public:
    CFTActionDlg(PROGIDACTION* pProgIDAction, LPTSTR pszProgIDDescr, BOOL fEdit, 
        BOOL fAutoDelete = FALSE);
    ~CFTActionDlg();

public:
    void SetShowAgain();

///////////////////////////////////////////////////////////////////////////////
//  Implementation
private:
// Message handlers
    //Dialog messages
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);

    LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

    //Control specific
    LRESULT OnOK(WORD wNotif);
    LRESULT OnCancel(WORD wNotif);
    LRESULT OnUseDDE(WORD wNotif);
    LRESULT OnBrowse(WORD wNotif);

private:
// Member variables
    PROGIDACTION* _pProgIDAction;
    LPTSTR _pszProgIDDescr;

    BOOL _fEdit;
    // used when need to reshow dlg because user entered bad data
    BOOL _fShowAgain;
///////////////////////////////////////////////////////////////////////////////
//  Helpers
private:
    // AssocStore
    BOOL _Validate();
    void _ResizeDlgForDDE(BOOL fShow);
};

#endif //FTACTION_H