#ifndef _INC_PGICONSZ_H
#define _INC_PGICONSZ_H

#include "pgbase.h"

class CIconSizePg : public WizardPage
{
public:
    CIconSizePg(LPPROPSHEETPAGE ppsp);
    ~CIconSizePg(VOID);

protected:
	HIMAGELIST m_himageNormalSmall;
	HIMAGELIST m_himageNormalLarge;
	HIMAGELIST m_himageLargeSmall;
	HIMAGELIST m_himageLargeLarge;
	HIMAGELIST m_himageExLargeSmall;
	HIMAGELIST m_himageExLargeLarge;

	HWND m_hwndList;

	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGICONSZ_H

