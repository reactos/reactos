#ifndef __INC_DSKQUOTA_UIUTILS_H
#define __INC_DSKQUOTA_UIUTILS_H

//
// Simple class for automating the display and resetting of a wait cursor.
//
class CAutoWaitCursor
{
    public:
        CAutoWaitCursor(void)
            : m_hCursor(SetCursor(LoadCursor(NULL, IDC_WAIT)))
            { ShowCursor(TRUE); }

        ~CAutoWaitCursor(void)
            { Reset(); }

        void Reset(void);

    private:
        HCURSOR m_hCursor;
};

bool UseWindowsHelp(int idCtl);

#endif //__INC_DSKQUOTA_UIUTILS_H
