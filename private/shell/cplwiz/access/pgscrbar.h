#ifndef _INC_PGSCRBAR_H
#define _INC_PGSCRBAR_H

#include "pgbase.h"

class CScrollBarPg : public WizardPage
{
public:
    CScrollBarPg(LPPROPSHEETPAGE ppsp);
    ~CScrollBarPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	int m_nCountValues;
	int m_rgnValues[MAX_DISTINCT_VALUES];
	int m_nCurValueIndex;
	RECT m_rgrcScrollBars[MAX_DISTINCT_VALUES];

private:

};

#endif // _INC_PGSCRBAR_H

