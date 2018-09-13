#ifndef __PPAGES_H__
#define __PPAGES_H__

enum SCH_TYPES
{
    HOME_CURRENT_HOME,
    HOME_CURRENT_PAGE,
    HOME_DEFAULT,
    SCH_CURRENT_SEARCH,
    SCH_CURRENT_PAGE,
    SCH_DEFAULT
};


struct CHomePropSheetPage
{
public:
    PROPSHEETPAGE 	page;
    TCHAR	_szHome[MAX_URL_STRING];
    TCHAR	_szSearch[MAX_URL_STRING];
    static BOOL CALLBACK s_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    CHomePropSheetPage() {
	memset(this, 0x00, SIZEOF(CHomePropSheetPage));
    }

protected:
    VOID 	_SetCurrentHomeSearchTo( HWND hDlg, enum SCH_TYPES set_to);
    void 	_OnButtonClick(HWND hDlg, WPARAM wParam, LPARAM lParam, enum SCH_TYPES uHome1, enum SCH_TYPES uHome2);
    VOID 	_SetHomeSearchLText(HWND hDlg, BOOL fHome);
};

#endif // __PPAGES_H__  
