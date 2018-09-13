#ifndef _INC_PGLOKPRV_H
#define _INC_PGLOKPRV_H

#include "pgbase.h"

class CLookPreviewPg : public WizardPage
{
public:
    CLookPreviewPg(LPPROPSHEETPAGE ppsp, int dwPageId, int nIdTitle, int nIdSubTitle, int nIdValueString);
    ~CLookPreviewPg(VOID);


protected: // Virtual functions
	// Override this if the values are not stored in a string table
	virtual void LoadValueArray();

	// This must be overridden so that the dialog knows what item to select as the default
	virtual int GetCurrentValue(NONCLIENTMETRICS *pncm) = 0;

	// If the dialog is using a list box, the user MUST override this function
	virtual void GetValueItemText(int nIndex, LPTSTR lpszBuffer, int nLen) {_ASSERTE(FALSE);}

	// Must be overridden to set ncm to appropriate values based on Value array index
	virtual void ModifyMyNonClientMetrics(NONCLIENTMETRICS &ncm) = 0;

protected:
	enum UITYPE {UIBUTTONS, UILISTBOX, UISLIDER};


	void UpdatePreview(int nActionCtl);

	void OnVHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
	int m_nIdValueString;
	UITYPE m_UIType;
	HWND m_hwndSlider;
	HWND m_hwndListBox;

	int m_nCountValues;
	int m_rgnValues[MAX_DISTINCT_VALUES];
	int m_nCurValueIndex;

	void ResetColors();
	void UpdateControls();
	

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);


private:

};



/////////////////////////////////////////////////////////////
// Min Legible text size TYPE 1, 2 & 3

class CLookPreviewMinTextPg : public CLookPreviewPg
{
public:
	CLookPreviewMinTextPg(LPPROPSHEETPAGE ppsp, int dwPageId)
		: CLookPreviewPg(	ppsp,
							dwPageId,
							IDS_LKPREV_MINTEXTTITLE,
							IDS_LKPREV_MINTEXTSUBTITLE,
							IDS_LKPREV_MINTEXTSIZES) {};

	virtual int GetCurrentValue(NONCLIENTMETRICS *pncm)
	{
		// Use active window caption
		return MulDiv(abs(pncm->lfCaptionFont.lfHeight), 72, g_Options.m_nLogPixelsY);
	};
	virtual void ModifyMyNonClientMetrics(NONCLIENTMETRICS &ncm)
	{
		ncm.lfCaptionFont.lfHeight = -MulDiv(m_rgnValues[m_nCurValueIndex], g_Options.m_nLogPixelsY, 72);
		ncm.lfMenuFont.lfHeight = -MulDiv(m_rgnValues[m_nCurValueIndex], g_Options.m_nLogPixelsY, 72);
		
		ncm.lfCaptionFont.lfWeight = FW_BOLD; // Caption is BOLD
		HFONT hFont = CreateFontIndirect(&ncm.lfCaptionFont);
		ncm.lfMenuFont.lfWeight = FW_NORMAL; // Still need lf for ICON
		TEXTMETRIC tm;
		HDC hdc = GetDC(m_hwnd);
		HFONT hfontOld = (HFONT)SelectObject(hdc, hFont);
		GetTextMetrics(hdc, &tm);
		if (hfontOld)
			SelectObject(hdc, hfontOld);
		ReleaseDC(m_hwnd, hdc);


		int cyBorder = GetSystemMetrics(SM_CYBORDER);
		int nSize = abs(ncm.lfCaptionFont.lfHeight) + abs(tm.tmExternalLeading) + 2 * cyBorder;
		nSize = max(nSize, GetSystemMetrics(SM_CYICON)/2 + 2 * cyBorder);

		ncm.iCaptionWidth = nSize;
		ncm.iCaptionHeight = nSize;
		ncm.iSmCaptionWidth = nSize;
		ncm.iSmCaptionHeight = nSize;
		ncm.iMenuWidth = nSize;
		ncm.iMenuHeight = nSize;
	}
};


class CLookPreviewMinText1Pg : public CLookPreviewMinTextPg
{
public:
	CLookPreviewMinText1Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewMinTextPg( ppsp, IDD_PREV_MINTEXT1) {};
};

class CLookPreviewMinText2Pg : public CLookPreviewMinTextPg
{
public:
	CLookPreviewMinText2Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewMinTextPg( ppsp, IDD_PREV_MINTEXT2) {};
};

class CLookPreviewMinText3Pg : public CLookPreviewMinTextPg
{
public:
	CLookPreviewMinText3Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewMinTextPg( ppsp, IDD_PREV_MINTEXT3) {};
protected:
	virtual void GetValueItemText(int nIndex, LPTSTR lpszBuffer, int nLen)
	{
		// Add font sizes
		TCHAR szName[100];
		wsprintf(szName, __TEXT("Font Size %i"), m_rgnValues[nIndex]);
		lstrcpyn(lpszBuffer, szName, nLen);
	}
};



/////////////////////////////////////////////////////////////
// Color page

class CLookPreviewColorPg : public CLookPreviewPg
{
public:
	CLookPreviewColorPg(LPPROPSHEETPAGE ppsp)
		: CLookPreviewPg(	ppsp,
							IDD_PREV_COLOR,
							IDS_LKPREV_COLORTITLE,
							IDS_LKPREV_COLORSUBTITLE,
							0) {};

	virtual void LoadValueArray()
	{
		// For colors, we just use 0 to GetSchemeCount()
		m_nCountValues = GetSchemeCount() + 1;
		for(int i=0;i<m_nCountValues;i++)
			m_rgnValues[i] = i;
	}
	virtual int GetCurrentValue(NONCLIENTMETRICS *pncm) {return 0;}; // Always return value of 0
	virtual void GetValueItemText(int nIndex, LPTSTR lpszBuffer, int nLen)
	{
		_ASSERTE(nIndex < GetSchemeCount() + 1);
		if(0 == nIndex)
			lstrcpyn(lpszBuffer, __TEXT("No changes"), nLen);
		else
			GetSchemeName(nIndex - 1, lpszBuffer, nLen);
	}
	virtual void ModifyMyNonClientMetrics(NONCLIENTMETRICS &ncm)
	{
		ResetColors();
	}

};

/////////////////////////////////////////////////////////////
// Scroll Bar Size TYPE 1 & 2

class CLookPreviewScrollBarPg : public CLookPreviewPg
{
public:
	CLookPreviewScrollBarPg(LPPROPSHEETPAGE ppsp, int dwPageId)
		: CLookPreviewPg(	ppsp,
							dwPageId,
							IDS_LKPREV_SCROLLBARTITLE,
							IDS_LKPREV_SCROLLBARSUBTITLE,
							IDS_LKPREV_SCROLLSIZES) {};

	virtual int GetCurrentValue(NONCLIENTMETRICS *pncm) {return max(pncm->iScrollWidth, pncm->iScrollHeight);};
	virtual void ModifyMyNonClientMetrics(NONCLIENTMETRICS &ncm)
	{
		ncm.iScrollWidth = m_rgnValues[m_nCurValueIndex];
		ncm.iScrollHeight = m_rgnValues[m_nCurValueIndex];
	}
};

class CLookPreviewScrollBar1Pg : public CLookPreviewScrollBarPg
{
public:
	CLookPreviewScrollBar1Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewScrollBarPg( ppsp, IDD_PREV_SCROLL1) {};
};

class CLookPreviewScrollBar2Pg : public CLookPreviewScrollBarPg
{
public:
	CLookPreviewScrollBar2Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewScrollBarPg( ppsp, IDD_PREV_SCROLL2) {};
};

/////////////////////////////////////////////////////////////
// Border Size TYPE 1 & 2

class CLookPreviewBorderPg : public CLookPreviewPg
{
public:
	CLookPreviewBorderPg(LPPROPSHEETPAGE ppsp, int dwPageId)
		: CLookPreviewPg(	ppsp,
							dwPageId,
							IDS_LKPREV_BORDERTITLE,
							IDS_LKPREV_BORDERSUBTITLE,
							IDS_LKPREV_BORDERSIZES) {};

	virtual int GetCurrentValue(NONCLIENTMETRICS *pncm) {return pncm->iBorderWidth;};
	virtual void ModifyMyNonClientMetrics(NONCLIENTMETRICS &ncm)
	{
		ncm.iBorderWidth = m_rgnValues[m_nCurValueIndex];
	}
};

class CLookPreviewBorder1Pg : public CLookPreviewBorderPg
{
public:
	CLookPreviewBorder1Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewBorderPg( ppsp, IDD_PREV_BORDER1) {};
};

class CLookPreviewBorder2Pg : public CLookPreviewBorderPg
{
public:
	CLookPreviewBorder2Pg(LPPROPSHEETPAGE ppsp) : CLookPreviewBorderPg( ppsp, IDD_PREV_BORDER2) {};
};

#endif // _INC_PGLOKPRV_H

