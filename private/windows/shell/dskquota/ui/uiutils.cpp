#include "pch.h"
#pragma hdrstop

#include "uiutils.h"

void
CAutoWaitCursor::Reset(
    void
    )
{
    ShowCursor(FALSE);
    if (NULL != m_hCursor)
        SetCursor(m_hCursor);
    m_hCursor = NULL;
}


bool UseWindowsHelp(int idCtl)
{
    bool bUseWindowsHelp = false;
    switch(idCtl)
    {
        case IDOK:
        case IDCANCEL:
        case IDC_STATIC:
            bUseWindowsHelp = true;
            break;

        default:
            break;
    }
    return bUseWindowsHelp;
}