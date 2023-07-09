#ifndef _UNIXSTUFF_H_
#define _UNIXSTUFF_H_

#include "cachecpl.h"

#define FONT_UPDATE_TICK 150

UINT  RegPopulateEditText(HWND hwndCB, HKEY hkeyProtocol);
BOOL  LocalFileCheck(LPCTSTR aszFileName);
BOOL  FoundProgram(HWND hwndDlg, int nIDDlgItem);
void  FontUpdateFeedBack(int nTick, void *pvParam);
VOID  DrawXFontButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis);

BOOL  IsCacheReadOnly();

BOOL CALLBACK FontUpdDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // _UNIXSTUFF_H_
