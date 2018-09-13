#ifndef _INC_PGWELCOM_H
#define _INC_PGWELCOM_H

#include "pgbase.h"

class CWelcomePg : public WizardPage
{
public:
    CWelcomePg(LPPROPSHEETPAGE ppsp);
    ~CWelcomePg(VOID);

protected:
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	inline void InvalidateRects(int PrevHilight);
	LRESULT OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam );
	LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	inline void SetFocussedItem(int m_nCurrentHilight);

	TCHAR m_szWelcomeText[4][85];
private:

	void Draw(LPDRAWITEMSTRUCT ldi, int i);
	int m_nCountValues;
	int m_rgnValues[MAX_DISTINCT_VALUES];
	int m_nCurValueIndex;
	int m_nCurrentHilight;
	BOOL syncInit;
	UINT uIDEvent;
};

#endif // _INC_PGWELCOM_H

