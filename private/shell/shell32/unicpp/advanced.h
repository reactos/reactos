#ifndef _ADVANCED_H_
#define _ADVANCED_H_

BOOL_PTR CALLBACK AdvancedOptionsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);

// BUGBUG - raymondc - temp hack - stolen from shdocvw
// Prototype flags
#define PF_USERMENUS        0x00000001      // Use traditional USER menu bar
#define PF_NEWFAVMENU       0x00000002      // New favorites menu
#define PF_NOBROWSEUI       0x00001000          // don't use browseui

#endif
