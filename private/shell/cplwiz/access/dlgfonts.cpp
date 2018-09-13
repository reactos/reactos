#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "DlgFonts.h"


HFONT BigBoldFont = NULL;
HFONT BoldFont = NULL;


// Helper function
void SetControlFont(HFONT hFont, HWND hwnd, int nId)
{
	if(!hFont)
		return;
	HWND hwndControl = GetDlgItem(hwnd, nId);
	if(!hwndControl)
		return;
	SetWindowFont(hwndControl, hFont, TRUE);
}


void SetupFonts(HWND hwnd)
{
	// Only execute this code once
	static BOOL bOneTime = TRUE;
	if(bOneTime)
		bOneTime = FALSE;
	else
		return;

	// Create the fonts we need based on the dialog font
	NONCLIENTMETRICS ncm;
	memset(&ncm, 0, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont = ncm.lfMessageFont;
	LOGFONT BoldLogFont = ncm.lfMessageFont;

	// Create Big Bold Font and Bold Font
    BigBoldLogFont.lfWeight = FW_BOLD;
	BoldLogFont.lfWeight = FW_BOLD;

    TCHAR FontSizeString[24];
    int FontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if(!LoadString(g_hInstDll,IDS_LARGEFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE)) {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Serif"));
    }

    if(LoadString(g_hInstDll,IDS_LARGEFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) {
#ifdef UNICODE
        FontSize = wcstoul(FontSizeString,NULL,10);
#else
		FontSize = strtoul(FontSizeString,NULL, 10);
#endif
    } else {
        FontSize = 18;
    }

	HDC hdc;
    if(hdc = GetDC(hwnd)) {

        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        BigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		BoldFont = CreateFontIndirect(&BoldLogFont);

        ReleaseDC(hwnd,hdc);
    }

}

void DialogFonts_InitWizardPage(
	IN HWND hwndWizardPage
	)
{
	SetupFonts(hwndWizardPage);
	SetControlFont(BigBoldFont, hwndWizardPage, IDC_BIGBOLDTITLE);
	SetControlFont(BoldFont, hwndWizardPage, IDC_BOLDTITLE);
}

