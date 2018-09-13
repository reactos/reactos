#ifndef _PRSHTCPP_INC
#define _PRSHTCPP_INC

#include "propsht.h"

// attrib treewalkcb
STDAPI_(BOOL) ApplyRecursiveFolderAttribs(LPCTSTR pszDir, FILEPROPSHEETPAGE* pfpsp);

// progress dlg
STDAPI_(BOOL) CreateAttributeProgressDlg(FILEPROPSHEETPAGE* pfpsp);
STDAPI_(BOOL) DestroyAttributeProgressDlg(FILEPROPSHEETPAGE* pfpsp);
STDAPI SetProgressDlgPath(FILEPROPSHEETPAGE* pfpsp, LPCTSTR pszPath, BOOL fCompactPath);
STDAPI UpdateProgressBar(FILEPROPSHEETPAGE* pfpsp);
STDAPI_(BOOL) HasUserCanceledAttributeProgressDlg(FILEPROPSHEETPAGE* pfpsp);
STDAPI_(HWND) GetProgressWindow(FILEPROPSHEETPAGE* pfpsp);

// assoc store stuff
STDAPI UpdateOpensWithInfo(FILEPROPSHEETPAGE* pfpsp);
STDAPI CleanupOpensWithInfo(FILEPROPSHEETPAGE* pfpsp);

#endif // _PRSHTCPP_INC