#ifndef FTEDIT_H
#define FTEDIT_H

#include "ftdlg.h"

typedef struct tagFTEDITPARAM
{
    TCHAR   szExt[MAX_EXT];
    DWORD   dwExt;
    TCHAR   szProgIDDescr[MAX_PROGIDDESCR];
    DWORD   dwProgIDDescr;
    TCHAR   szProgID[MAX_PROGID];
    DWORD   dwProgID;
} FTEDITPARAM;

class CFTEditDlg : public CFTDlg
{
public:
    CFTEditDlg(FTEDITPARAM* pftEditParam, BOOL fAutoDelete = FALSE);
    ~CFTEditDlg();

///////////////////////////////////////////////////////////////////////////////
//  Implementation
private:
// Message handlers
    //Dialog messages
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
    LRESULT OnOK(WORD wNotif);
    LRESULT OnCancel(WORD wNotif);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

    //Control specific
    LRESULT OnAdvancedButton(WORD wNotif);
    LRESULT OnEdit(WORD wNotif);

    LRESULT OnTimer(UINT nTimer);

// Misc
    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

// Member variables
private:
    int             _iLVSel;

    BOOL            _fAdvanced;

    UINT_PTR        _nTimer;

    HANDLE          _hHeapProgID;
    FTEDITPARAM*    _pftEditParam;
///////////////////////////////////////////////////////////////////////////////
//  Helpers
private:
    HRESULT _GetProgIDDescrFromExt(LPTSTR pszExt, LPTSTR pszProgIDDescr,
        DWORD* pcchProgIDDescr);
    HRESULT _GetProgIDInfo(IAssocInfo* pAI, LPTSTR pszProgID, DWORD* pcchProgID,
        BOOL* pfNewProgID, BOOL* pfExplicitNew);
    HRESULT _HandleProgIDAssoc(IAssocInfo* pAI, LPTSTR pszExt, BOOL fExtExist);
    HRESULT _ProgIDComboHelper();
    void _ResizeDlg();
    HRESULT _FillProgIDDescrCombo();
    BOOL _SelectProgIDDescr(LPTSTR pszProgIDDescr);
    void _ConfigureDlg();
    LPTSTR _AddProgID(LPTSTR pszProgID);
    void _CleanupProgIDs();
};

#endif //FTEDIT_H