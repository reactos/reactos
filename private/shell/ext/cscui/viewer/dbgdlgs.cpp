//////////////////////////////////////////////////////////////////////////////
/*  File: dbgdlgs.cpp

    Description: 

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    12/09/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "dbgdlgs.h"
#include <resource.h>
#include "viewerp.h"
#include "uncpath.h"
#include "cscutils.h"
#include "msgbox.h"

ShareDbgDialog::ShareDbgDialog(
    LPCTSTR pszShare,
    const CnxNameCache& cnc
    ) : m_hInstance(NULL),
        m_strShare(pszShare),
        m_CnxNameCache(cnc)
{

}


void
ShareDbgDialog::Run(
    HINSTANCE hInstance,
    HWND hwndParent
    )
{
    m_hInstance = hInstance;
    int iResult = DialogBoxParam(m_hInstance,
              MAKEINTRESOURCE(IDD_CACHEVIEW_SHAREDBG),
              hwndParent,
              DlgProc,
              reinterpret_cast<LPARAM>(this));
    if (-1 == iResult)
    {
        CscWin32Message(hwndParent, GetLastError(), CSCUI::SEV_ERROR);
    }
}



BOOL CALLBACK
ShareDbgDialog::DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    ShareDbgDialog *pThis = reinterpret_cast<ShareDbgDialog *>(GetWindowLongPtr(hwnd, DWLP_USER));
    switch(message)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<ShareDbgDialog *>(lParam);
            SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)pThis);
            DBGASSERT((NULL != pThis));
            pThis->OnInitDialog(hwnd);
            return TRUE;

        case WM_ENDSESSION:
            EndDialog(hwnd, 0);
            return true;

        case WM_COMMAND:    
            DBGASSERT((NULL != pThis));
            if (BN_CLICKED == HIWORD(wParam))
            {
                switch(LOWORD(wParam))
                {
                    case IDCANCEL:
                    case IDOK:
                        EndDialog(hwnd, 0);
                        return TRUE;

                    case IDC_SHRDBG_BTN_REFRESH:
                        pThis->Refresh(hwnd);
                        return TRUE;

                    default:
                        break;
                }
            }
            break;
    }
    return FALSE;
}



void
ShareDbgDialog::OnInitDialog(
    HWND hwnd
    )
{
    CString strText;
    TCHAR szDrive[] = TEXT("?");
    UNCPath unc(m_strShare);

    szDrive[0] = m_CnxNameCache.GetShareDriveLetter(m_strShare);
    if (TEXT('\0') != szDrive[0])
    {
        //
        // Drive character provided.
        // Format as "ntspecs (E:) on 'worf'"
        //
        strText.Format(m_hInstance, 
                       IDS_FMT_SHARENAME_LONG,
                       (LPCTSTR)unc.m_strShare,
                       (LPCTSTR)unc.m_strServer,
                       szDrive);
    }
    else
    {
        //
        // Drive character not provided.
        // Format as "ntspecs on 'worf'"
        //
        strText.Format(m_hInstance,
                       IDS_FMT_SHARENAME_LONG_NODRIVE,
                       (LPCTSTR)unc.m_strShare,
                       (LPCTSTR)unc.m_strServer);
    }
    SetWindowText(GetDlgItem(hwnd, IDC_SHRDBG_TXT_SHARENAME), (LPCTSTR)strText);

    Refresh(hwnd);
}


void
ShareDbgDialog::Refresh(
    HWND hwnd
    )
{
    CscShareInformation si;
    TCHAR szNumber[40];

    CscGetShareInformation(m_strShare, &si);

    //
    // PBMF means "pointer to boolean member function"
    //
    typedef bool (CscShareInformation::*PBMF)(void) const;

    static const struct
    {
        int idCtl;
        PBMF pfn;

    } rgStatus[] = {{ IDC_SHRDBG_TXT_MODOFFLINE,      si.ModifiedOffline },
                    { IDC_SHRDBG_TXT_CONNECTED,       si.Connected       },
                    { IDC_SHRDBG_TXT_FILESOPEN,       si.FilesOpen       },
                    { IDC_SHRDBG_TXT_FINDSINPROG,     si.FindsInProgress },
                    { IDC_SHRDBG_TXT_DISCONNECTEDOP,  si.DisconnectedOp  },
                    { IDC_SHRDBG_TXT_MERGING,         si.Merging         }
                   };

    for (int i = 0; i < ARRAYSIZE(rgStatus); i++)
    {
        PBMF pfn = rgStatus[i].pfn;

        wsprintf(szNumber, TEXT("%d"), (si.*pfn)());
        SetWindowText(GetDlgItem(hwnd, rgStatus[i].idCtl), szNumber);
    }

    SendMessage(GetDlgItem(hwnd, IDC_SHRDBG_ICON), 
                STM_SETICON,
                (WPARAM)LoadIcon(m_hInstance, MAKEINTRESOURCE(si.Connected() ? IDI_SHARE : IDI_SHARE_NOCNX)),
                0);
}




FileDbgDialog::FileDbgDialog(
    LPCTSTR pszFile,
    bool bIsDirectory
    ) : m_hInstance(NULL),
        m_strFile(pszFile),
        m_bIsDirectory(bIsDirectory)
{

}


void
FileDbgDialog::Run(
    HINSTANCE hInstance,
    HWND hwndParent
    )
{
    m_hInstance = hInstance;
    int iResult = DialogBoxParam(m_hInstance,
              MAKEINTRESOURCE(IDD_CACHEVIEW_FILEDBG),
              hwndParent,
              DlgProc,
              reinterpret_cast<LPARAM>(this));
    if (-1 == iResult)
    {
        CscWin32Message(hwndParent, GetLastError(), CSCUI::SEV_ERROR);
    }
}



BOOL CALLBACK
FileDbgDialog::DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    FileDbgDialog *pThis = reinterpret_cast<FileDbgDialog *>(GetWindowLongPtr(hwnd, DWLP_USER));
    switch(message)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<FileDbgDialog *>(lParam);
            SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)pThis);
            DBGASSERT((NULL != pThis));
            pThis->OnInitDialog(hwnd);
            return TRUE;

        case WM_ENDSESSION:
            EndDialog(hwnd, 0);
            return true;

        case WM_COMMAND:    
            DBGASSERT((NULL != pThis));
            if (BN_CLICKED == HIWORD(wParam))
            {
                switch(LOWORD(wParam))
                {
                    case IDCANCEL:
                    case IDOK:
                        EndDialog(hwnd, 0);
                        return TRUE;

                    case IDC_FILEDBG_BTN_REFRESH:
                        pThis->Refresh(hwnd);
                        return TRUE;

                    default:
                        break;
                }
                
            }
            break;
    }
    return FALSE;
}



void
FileDbgDialog::OnInitDialog(
    HWND hwnd
    )
{
    SetWindowText(GetDlgItem(hwnd, IDC_FILEDBG_TXT_FILENAME), (LPCTSTR)m_strFile);
    Refresh(hwnd);
}


void
FileDbgDialog::Refresh(
    HWND hwnd
    )
{
    CscFileInformation fi;
    TCHAR szNumber[40];

    CscGetFileInformation(m_strFile, &fi);

    //
    // PBMF means "pointer to boolean member function"
    //
    typedef bool (CscFileInformation::*PBMF)(void) const;

    static const struct
    {
        int idCtl;
        PBMF pfn;

    } rgStatus[] = {{ IDC_FILEDBG_TXT_DATAMOD,           fi.DataLocallyModified   },
                    { IDC_FILEDBG_TXT_ATTRIBMOD,         fi.AttribLocallyModified },
                    { IDC_FILEDBG_TXT_TIMEMOD,           fi.TimeLocallyModified   },
                    { IDC_FILEDBG_TXT_CREATED,           fi.LocallyCreated        },
                    { IDC_FILEDBG_TXT_DELETED,           fi.LocallyDeleted        },
                    { IDC_FILEDBG_TXT_STALE,             fi.Stale                 },
                    { IDC_FILEDBG_TXT_SPARSE,            fi.Sparse                },
                    { IDC_FILEDBG_TXT_ORPHAN,            fi.Orphan                },
                    { IDC_FILEDBG_TXT_SUSPECT,           fi.Suspect               },
                    { IDC_FILEDBG_TXT_PINUSER,           fi.HintPinUser           },
                    { IDC_FILEDBG_TXT_PININHERITUSER,    fi.HintPinInheritUser    },
                    { IDC_FILEDBG_TXT_PININHERITSYSTEM,  fi.HintPinInheritSystem  },
                    { IDC_FILEDBG_TXT_CONSERVEBANDWIDTH, fi.HintConserveBandwidth },
                   };

    for (int i = 0; i < ARRAYSIZE(rgStatus); i++)
    {
        PBMF pfn = rgStatus[i].pfn;

        wsprintf(szNumber, TEXT("%d"), (fi.*pfn)());
        SetWindowText(GetDlgItem(hwnd, rgStatus[i].idCtl), szNumber);
    }

    wsprintf(szNumber, TEXT("%d"), fi.PinCount());
    SetWindowText(GetDlgItem(hwnd, IDC_FILEDBG_TXT_PINCOUNT), szNumber);

    SendMessage(GetDlgItem(hwnd, IDC_FILEDBG_ICON), 
                STM_SETICON,
                (WPARAM)LoadIcon(m_hInstance, MAKEINTRESOURCE(m_bIsDirectory ? IDI_FOLDER : IDI_DOCUMENT)),
                0);
}

