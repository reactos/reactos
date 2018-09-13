#ifndef _INC_PGGENRIC_H
#define _INC_PGGENRIC_H

#include "pgbase.h"

class CGenericWizPg : public WizardPage
{
    public:
        CGenericWizPg(LPPROPSHEETPAGE ppsp, DWORD dwPageId, int nIdTitle = IDS_GENERICPAGETITLE, int nIdSubTitle = IDS_GENERICPAGESUBTITLE);
        ~CGenericWizPg(VOID);

    private:
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);

};

#endif // _INC_PGGENRIC_H

