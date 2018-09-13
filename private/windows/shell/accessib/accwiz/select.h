
#ifndef _INC_SELECT_H
#define _INC_SELECT_H


class WizardPage;
void DrawHilight(HWND hWnd, LPDRAWITEMSTRUCT ldi);

// CIconSizePg
class CIconSizePg : public WizardPage
{
public:
    CIconSizePg(LPPROPSHEETPAGE ppsp);

protected:
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam );
	LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);

private:
	void Draw(LPDRAWITEMSTRUCT ldi, int i);
	inline void SetFocussedItem(int m_nCurrentHilight);
	inline void InvalidateRects(int PrevHilight);
	LRESULT SelectionChanged(int nNewSelection);
	UINT GetCtrlID(int);

	int m_nCountValues;
	int m_rgnValues[MAX_DISTINCT_VALUES];
	int m_nCurValueIndex;
	int m_nCurrentHilight;
	BOOL syncInit;
	UINT uIDEvent;
};


// CScrollBarPg
class CScrollBarPg : public WizardPage
{
public:
    CScrollBarPg(LPPROPSHEETPAGE ppsp);

protected:
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	
	LRESULT OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam );
	LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);

private:
	void Draw(LPDRAWITEMSTRUCT ldi, int i);
	inline void SetFocussedItem(int m_nCurrentHilight);
	inline void InvalidateRects(int PrevHilight);
	LRESULT SettingChanged(int sel);
	UINT GetCtrlID(int);

	int m_nCountValues;
	int m_rgnValues[MAX_DISTINCT_VALUES];
	int m_nCurValueIndex;
	int m_nCurrentHilight;
	BOOL syncInit;
	UINT uIDEvent;
};


#endif // _INC_SELECT_H
