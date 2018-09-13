// eventrap.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "eventrap.h"
#include "trapdlg.h"
#include "globals.h"
#include "utils.h"
#include "trapreg.h"
#include "busy.h"
#include "dlgsavep.h"

/////////////////////////////////////////////////////////////////////////////
// CEventrapApp

BEGIN_MESSAGE_MAP(CEventrapApp, CWinApp)
        //{{AFX_MSG_MAP(CEventrapApp)
                // NOTE - the ClassWizard will add and remove mapping macros here.
                //    DO NOT EDIT what you see in these blocks of generated code!
        //}}AFX_MSG
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEventrapApp construction

CEventrapApp::CEventrapApp()
{
        // TODO: add construction code here,
        // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CEventrapApp object

CEventrapApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CEventrapApp initialization


LPCTSTR GetNextParam(LPTSTR pszDst, LPCTSTR pszSrc, LONG nchDst)
{
        // Skip any leading white space
        while((*pszSrc==' ') || (*pszSrc=='\t')) {
                ++pszSrc;
        }

        // Reserve a byte for the null terminator
        ASSERT(nchDst >= 1);
        --nchDst;

        // Copy the next parameter to the destination buffer.
        while (nchDst > 0) {
                INT iCh = *pszSrc;
                if ((iCh == 0) || (iCh==' ') || (iCh=='\t')) {
                        break;
                }
                ++pszSrc;
                *pszDst++ = (TCHAR)iCh;
                --nchDst;
        }
        *pszDst = 0;

        return pszSrc;
}

void ParseParams(CStringArray& asParams, LPCTSTR pszParams)
{
        TCHAR szParam[MAX_STRING];
        while(pszParams != NULL) {
                pszParams = GetNextParam(szParam, pszParams, MAX_STRING);
                if (szParam[0] == 0) {
                        break;
                }
                asParams.Add(szParam);
        }
}


BOOL CEventrapApp::InitInstance()
{

    GetThousandSeparator(&g_chThousandSep);

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    CStringArray asParams;
    ParseParams(asParams, m_lpCmdLine);


    SCODE sc;
    LPCTSTR pszComputerName = NULL;

    switch(asParams.GetSize()) {
    case 0:
        break;
    case 1:
        if (!asParams[0].IsEmpty()) {
            pszComputerName = asParams[0];
        }
        break;
    default:
        AfxMessageBox(IDS_ERR_INVALID_ARGUMENT);
        return FALSE;
        break;
    }


    CBusy busy;

    g_reg.m_pdlgLoadProgress = new CDlgSaveProgress;
    g_reg.m_pdlgLoadProgress->Create(IDD_LOAD_PROGRESS, NULL);
    g_reg.m_pdlgLoadProgress->BringWindowToTop();

	//if the local machine is the same as
	//the machine name passed as an argument
	//don't use a machine name.
	if (NULL != pszComputerName)
	{
		TCHAR t_buff[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;

		if (GetComputerName(t_buff, &dwLen))
		{
			if (_tcsicmp(t_buff, pszComputerName) == 0)
			{
				pszComputerName = NULL;
			}
		}
	}

    sc = g_reg.Connect(pszComputerName);
    if ((sc==S_LOAD_CANCELED) || FAILED(sc)) {
        delete g_reg.m_pdlgLoadProgress;
        g_reg.m_pdlgLoadProgress = NULL;
        return FALSE;
    }


    // Read the current event to trap configuration from the registry
    sc = g_reg.Deserialize();
    if ((sc==S_LOAD_CANCELED) || FAILED(sc)) {
        delete g_reg.m_pdlgLoadProgress;
        g_reg.m_pdlgLoadProgress = NULL;
        return FALSE;
    }

    CEventTrapDlg* pdlg = new CEventTrapDlg;
    m_pMainWnd = pdlg;
    pdlg->Create(IDD_EVNTTRAPDLG, NULL);
    pdlg->BringWindowToTop();

    // Since we are running a modeless dialog, return TRUE so that the message
    // pump is run.

	return TRUE;
}

int CEventrapApp::ExitInstance()
{
    return CWinApp::ExitInstance();
}

BOOL CEventrapApp::ProcessMessageFilter(int code, LPMSG lpMsg)
{
    // TODO: Add your specialized code here and/or call the base class

    return CWinApp::ProcessMessageFilter(code, lpMsg);
}

