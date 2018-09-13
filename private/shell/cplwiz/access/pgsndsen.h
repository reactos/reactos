#ifndef _INC_PGSNDSEN_H
#define _INC_PGSNDSEN_H

#include "pgbase.h"

class CSoundSentryShowSoundsPg : public WizardPage
{
public:
    CSoundSentryShowSoundsPg(LPPROPSHEETPAGE ppsp);
    ~CSoundSentryShowSoundsPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);


private:

};

#endif // _INC_PGSNDSEN_H

