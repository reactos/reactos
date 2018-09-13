//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "smtidy.h"
#include "killactv.h"
#include "util.h"
#include "resource.h"

//----------------------------------------------------------------------------
void KillActiveLostTargets(PSMTIDYINFO psmti, HWND hDlg)
{
    // Reset flags.
    psmti->dwFlags &= ~SMTIF_FIX_BROKEN_SHORTCUTS;
    
    if (IsDlgButtonChecked(hDlg, IDC_REMOVE_BROKEN_SHORTCUTS))
        psmti->dwFlags |= SMTIF_FIX_BROKEN_SHORTCUTS;
}

//----------------------------------------------------------------------------
void KillActiveUnusedShortcuts(PSMTIDYINFO psmti, HWND hDlg)
{
    psmti->dwFlags &= ~(SMTIF_GROUP_UNUSED_SHORTCUTS|SMTIF_DELETE_UNUSED_SHORTCUTS);
    
    if (IsDlgButtonChecked(hDlg, IDC_GROUP_UNUSED_SHORTCUTS))
        psmti->dwFlags |= SMTIF_GROUP_UNUSED_SHORTCUTS;
    else if (IsDlgButtonChecked(hDlg, IDC_REMOVE_UNUSED_SHORTCUTS))
        psmti->dwFlags |= SMTIF_DELETE_UNUSED_SHORTCUTS;
}

//----------------------------------------------------------------------------
void KillActiveGroupReadMes(PSMTIDYINFO psmti, HWND hDlg)
{
    psmti->dwFlags &= ~(SMTIF_GROUP_READMES|SMTIF_DELETE_READMES);
    
    if (IsDlgButtonChecked(hDlg, IDC_GROUP_READMES))
        psmti->dwFlags |= SMTIF_GROUP_READMES;
    else if (IsDlgButtonChecked(hDlg, IDC_REMOVE_READMES))
        psmti->dwFlags |= SMTIF_DELETE_READMES;
}

//----------------------------------------------------------------------------
void KillActiveSingleItemFolder(PSMTIDYINFO psmti, HWND hDlg)
{
    psmti->dwFlags &= ~SMTIF_FIX_SINGLE_ITEM_FOLDERS;
    
    if (IsDlgButtonChecked(hDlg, IDC_MOVE_SINGLE_ITEMS))
        psmti->dwFlags |= SMTIF_FIX_SINGLE_ITEM_FOLDERS;

}

//----------------------------------------------------------------------------
void KillActiveEmptyFolders(PSMTIDYINFO psmti, HWND hDlg)
{
    psmti->dwFlags &= ~SMTIF_REMOVE_EMPTY_FOLDERS;
    
    if (IsDlgButtonChecked(hDlg, IDC_REMOVE_EMPTY_FOLDERS))
        psmti->dwFlags |= SMTIF_REMOVE_EMPTY_FOLDERS;

}

