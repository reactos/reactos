#include "shprv.h"
#include <prsht.h>
#include <print.h>
#include <msprintx.h>
#include <wutilsp.h>

// 16 bit half of thunk to call MSPRINT.DLL.  other half is in
// shell32.dll

BOOL CALLBACK _AddPrnPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    LPPAGEARRAY lpapg=(LPPAGEARRAY)lParam;
    if (lpapg->cpages < MAX_PRN_PAGES)
    {
	lpapg->ahpage[lpapg->cpages++] = hpage;
	return TRUE;
    }
    return FALSE;
}

VOID WINAPI CallAddPropSheetPages16(LPFNADDPROPSHEETPAGES lpfn16, LPVOID hdrop, LPPAGEARRAY papg)
{
    // NOTES: This code assumes 32=>16 thunk generates 16:16 mapped pointer
    //  with offset-0.
    LPVOID lpv=(LPVOID)MAKELONG(HIWORD(hdrop), 0);
    lpfn16(lpv, _AddPrnPropSheetPage, (LPARAM)papg);
}

