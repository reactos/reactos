#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "DlgFonts.h"
#include "accwiz.h" // for g_Options


HFONT BigBoldFont = NULL;
HFONT BoldFont = NULL;
HFONT BigFont = NULL;


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
	LOGFONT BigLogFont = ncm.lfMessageFont;

	// Create Big Bold Font and Bold Font
    BigBoldLogFont.lfWeight = FW_BOLD;
	BoldLogFont.lfWeight = FW_BOLD;
    BigLogFont.lfWeight = FW_NORMAL;

    TCHAR FontSizeString[24];
    int FontSizeBigBold;
    int FontSizeBold;
    int FontSizeBig;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
	BigBoldLogFont.lfCharSet = g_Options.m_lfCharSet;
	BoldLogFont.lfCharSet = g_Options.m_lfCharSet;
	BigLogFont.lfCharSet = g_Options.m_lfCharSet;

    if(!LoadString(g_hInstDll,IDS_BIGBOLDFONTNAME,BigBoldLogFont.lfFaceName,LF_FACESIZE)) {
        lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Serif"));
    }

    if(!LoadString(g_hInstDll,IDS_BOLDFONTNAME,BoldLogFont.lfFaceName,LF_FACESIZE)) {
        lstrcpy(BoldLogFont.lfFaceName,TEXT("MS Serif"));
    }

    if(!LoadString(g_hInstDll,IDS_BIGFONTNAME,BigLogFont.lfFaceName,LF_FACESIZE)) {
        lstrcpy(BigLogFont.lfFaceName,TEXT("MS Serif"));
    }

    if(LoadString(g_hInstDll,IDS_BIGBOLDFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) {
		FontSizeBigBold = _tcstoul(FontSizeString,NULL,10);
    } else {
        FontSizeBigBold = 16;
    }

    if(LoadString(g_hInstDll,IDS_BOLDFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) {
        FontSizeBold = _tcstoul(FontSizeString,NULL,10);
    } else {
        FontSizeBold = 8;
    }

    if(LoadString(g_hInstDll,IDS_BIGFONTSIZE,FontSizeString,sizeof(FontSizeString)/sizeof(TCHAR))) {
        FontSizeBig = _tcstoul(FontSizeString,NULL,10);
    } else {
        FontSizeBig = 16;
    }

	HDC hdc;
    if(hdc = GetDC(hwnd)) {

        BigBoldLogFont.lfHeight = 0 - (int)((float)GetDeviceCaps(hdc,LOGPIXELSY) * (float)FontSizeBigBold / (float)72 + (float).5);
        BoldLogFont.lfHeight = 0 - (int)((float)GetDeviceCaps(hdc,LOGPIXELSY) * (float)FontSizeBold / (float)72 + (float).5);
        BigLogFont.lfHeight = 0 - (int)((float)GetDeviceCaps(hdc,LOGPIXELSY) * (float)FontSizeBig / (float)72 + (float).5);

        BigBoldFont = CreateFontIndirect(&BigBoldLogFont);
		BoldFont = CreateFontIndirect(&BoldLogFont);
        BigFont = CreateFontIndirect(&BigLogFont);

        ReleaseDC(hwnd,hdc);
    }

}

void DialogFonts_InitWizardPage(
	IN HWND hwndWizardPage
	)
{
	SetupFonts(hwndWizardPage);

	// If we are going to change the fonts of all wizard pages,
	// we can't allow the user to go back and change the size
	// they picked.  This is because this function is only called
	// once for each page.
/*
	if(-1 != g_Options.m_nMinimalFontSize)
	{
*/
	HWND hwndChild = GetTopWindow(hwndWizardPage);
	do
	{
		int nId = GetDlgCtrlID(hwndChild);
		switch(nId)
		{
		case IDC_BOLDTITLE:
			SetControlFont(BoldFont, hwndWizardPage, IDC_BOLDTITLE);
			break;
		case IDC_BIGBOLDTITLE:
			SetControlFont(BigBoldFont, hwndWizardPage, IDC_BIGBOLDTITLE);
			break;
		case IDC_BIGTITLE:
			SetControlFont(BigFont, hwndWizardPage, IDC_BIGTITLE);
			break;
#if 0 // This used to be for the icon size page
		case IDC_STATICNORMAL:
			SetWindowFont(hwndChild, g_Options.GetClosestMSSansSerif(8), TRUE);
			break;
		case IDC_STATICLARGE:
			SetWindowFont(hwndChild, g_Options.GetClosestMSSansSerif(12), TRUE);
			break;
		case IDC_STATICEXTRALARGE:
			SetWindowFont(hwndChild, g_Options.GetClosestMSSansSerif(18), TRUE);
			break;
#endif
		default:
#if 0 // We decided that we weren't going to resize the fonts in the dialog
			// DON'T go above 12 points for the dialog fonts
			SetWindowFont(hwndChild, g_Options.GetClosestMSSansSerif(min(12, g_Options.m_nMinimalFontSize)), TRUE);
#endif
			break;
		}
	}
	while(hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT));
}

