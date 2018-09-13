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
		{
			LoadString(g_hInstDll, IDS_SCHEME_CURRENTCOLORSCHEME, lpszBuffer, nLen);
		}
		else
			GetSchemeName(nIndex - 1, lpszBuffer, nLen);
	}
	virtual void ModifyMyNonClientMetrics(NONCLIENTMETRICS &ncm)
	{
		ResetColors();
	}

};


#endif // _INC_PGLOKPRV_H

