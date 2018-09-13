#ifndef _INC_PGMINTXT_H
#define _INC_PGMINTXT_H

#include "pgbase.h"

class CMinTextPg : public WizardPage
{
public:
    CMinTextPg(LPPROPSHEETPAGE ppsp);
    ~CMinTextPg(VOID);

protected:
	LOGFONT m_lfFont;
	HFONT m_hFont;
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);


private:

};

#endif // _INC_PGMINTXT_H

