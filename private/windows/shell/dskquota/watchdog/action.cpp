///////////////////////////////////////////////////////////////////////////////
/*  File: action.cpp

    Description: Implements classes to handle actions associated
        with user notifications (email, popup dialog etc).
        
            CAction
            CActionEmail
            CActionPopup

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include <precomp.hxx>
#pragma hdrstop

#include <commctrl.h>
#include "action.h"
#include "history.h"
#include "stats.h"
#include "resource.h"
#include "mapisend.h"

//-----------------------------------------------------------------------------
// CActionEmail
//-----------------------------------------------------------------------------

#ifdef UNICODE
#   define EMAIL_UNICODE TRUE
#else
#   define EMAIL_UNICODE FALSE
#endif

CActionEmail::CActionEmail(
    CMapiSession& MapiSession,  // For sending message.
    LPMAPIFOLDER pMapiFolder,   // For initializing message object.
    LPTSTR pszRecipientsTo,     // Other recips on "To:" line.
    LPTSTR pszRecipientsCc,     // Other recips on "Cc:" line.
    LPTSTR pszRecipientsBcc,    // Other recips on "Bcc:" line.
    LPCTSTR pszSubject,         // Message subject line.
    CMapiMessageBody& MsgBody   // Message body text.
    ) : m_MapiSession(MapiSession),
        m_MapiRecipients(EMAIL_UNICODE),
        m_MapiMsg(pMapiFolder, MsgBody, pszSubject)
{
    HRESULT hr;

    m_Mapi.Load();

    //
    // NOTE:  We hold a reference to a CMapiSession object.
    //        The CMapiSession object doen't employ any reference
    //        counting of it's own.  This code assumes that the 
    //        lifetime of the referenced section object exceeds the
    //        lifetime of the action object.  

    LPSPropValue pProps = NULL;
    ULONG cbProps = 0;

    //
    // Get the address properties for the MAPI session user.
    //
    hr = m_MapiSession.GetSessionUser(&pProps, &cbProps);
    if (SUCCEEDED(hr))
    {
        if (5 == cbProps)
        {
            SPropValue rgProp[5];

            //
            // Get the string resource containing "NT Disk Quota Administrator".
            // It's a resource for localization.
            //
            // BUGBUG: This currently doesn't work although the Exchange guys
            //         tell me it should.  Currently, the mail message always
            //         arrives with the local user's name on the "From:" line.
            //         It should read "NT Disk Quota Administrator".
            //         Needs work. [brianau - 07/10/97]
            //
            CString strEmailFromName(g_hInstDll, IDS_EMAIL_FROM_NAME);

            //
            // Set the "PR_SENT_REPRESENTING_XXXX" props to the same
            // values as the "PR_SENDER_XXXX" props.
            //
            rgProp[0].ulPropTag      = PR_SENT_REPRESENTING_ADDRTYPE;
            rgProp[0].Value.LPSZ     = pProps[0].Value.LPSZ;

            rgProp[1].ulPropTag      = PR_SENT_REPRESENTING_NAME;
            rgProp[1].Value.LPSZ     = (LPTSTR)strEmailFromName;

            rgProp[2].ulPropTag      = PR_SENT_REPRESENTING_EMAIL_ADDRESS;
            rgProp[2].Value.LPSZ     = pProps[2].Value.LPSZ;

            rgProp[3].ulPropTag      = PR_SENT_REPRESENTING_ENTRYID;
            rgProp[3].Value.bin.cb   = pProps[3].Value.bin.cb;
            rgProp[3].Value.bin.lpb  = pProps[3].Value.bin.lpb;

            rgProp[4].ulPropTag      = PR_SENT_REPRESENTING_SEARCH_KEY;
            rgProp[4].Value.bin.cb   = pProps[4].Value.bin.cb;
            rgProp[4].Value.bin.lpb  = pProps[4].Value.bin.lpb;

            LPSPropProblemArray pProblems = NULL;

            //
            // Set the new properties.
            //
            hr = m_MapiMsg.SetProps(ARRAYSIZE(rgProp), rgProp, &pProblems);        
            hr = m_MapiMsg.SaveChanges(KEEP_OPEN_READWRITE);

            //
            // Add the recipient to the list of recipients.
            //
            hr = m_MapiRecipients.AddRecipient(pProps[2].Value.LPSZ, MAPI_TO);
        }
        m_Mapi.FreeBuffer(pProps);
    }

    //
    // Each element of this array contains a pointer to a list of 
    // recipient names (semicolon-delmited) and a recipient type
    // code.  This allows us to process all of the recipients
    // in a single loop.
    //
    struct recip {
        LPTSTR pszName;
        DWORD dwType;
    } rgRecips[] = {
                       { pszRecipientsTo,  MAPI_TO  },
                       { pszRecipientsCc,  MAPI_CC  },
                       { pszRecipientsBcc, MAPI_BCC },
                   };

    for (INT i = 0; i < ARRAYSIZE(rgRecips); i++)
    {
        LPTSTR pszNext  = rgRecips[i].pszName;
        LPCTSTR pszPrev = pszNext;
        //
        // Process the current list of recipient names until we reach
        // the terminating nul character.
        //
        while(TEXT('\0') != *pszPrev)
        {
            while((TEXT('\0') != *pszNext) && (TEXT(';') != *pszNext))
            {
                //
                // Find the next semicolon or the terminating nul.
                //
                pszNext++;
            }
            if (TEXT('\0') != *pszNext)
            {
                //
                // Found a semicolon.  Replace it with a nul and
                // skip ahead to the start of the next name.
                //
                *pszNext++ = TEXT('\0');
            }
            //
            // Add the name of the recipient pointed to by pszPrev
            // using the type code associated with this list of recipients.
            //
            m_MapiRecipients.AddRecipient(pszPrev, rgRecips[i].dwType);
            pszPrev = pszNext;
        }
    }
}

CActionEmail::~CActionEmail(
    VOID
    )
{
    m_Mapi.Unload();
}


//
// Send the email and record the send operation in our history record.
//
HRESULT
CActionEmail::DoAction(
    CHistory& history
    )
{
    HRESULT hr;

    //
    // Try sending the mail using the current ANSI/Unicode contents.
    //
    hr = m_MapiSession.Send(m_MapiRecipients, m_MapiMsg);
    if (MAPI_E_BAD_CHARWIDTH == hr)
    {
        //
        // Failed because the provider can't handle the character width.
        // Convert the address list to the opposite character width.
        //
        // BUGBUG:  Currently, we just convert the address list.  We 
        //          should probably do the same thing with the message body
        //          and subject line.
        //
        CMapiRecipients recipTemp(!m_MapiRecipients.IsUnicode());
        recipTemp = m_MapiRecipients;
        //
        // Try to send again.
        //
        hr = m_MapiSession.Send(recipTemp, m_MapiMsg);
    }
    if (SUCCEEDED(hr))
    {
        //
        // Record in the history log that we've sent email.
        //
        history.RecordEmailSent();
    }
    return hr;
}

//-----------------------------------------------------------------------------
// CActionPopup
//-----------------------------------------------------------------------------

UINT CActionPopup::m_idAutoCloseTimer    = 1;
UINT CActionPopup::m_uAutoCloseTimeout   = 300000;  // Timeout in 5 minutes.

CActionPopup::CActionPopup(
    CStatisticsList& stats
    ) : m_stats(stats),
        m_hiconDialog(NULL),
        m_hwnd(NULL)
{
    m_hiconDialog = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_QUOTA));
}

CActionPopup::~CActionPopup(
    VOID
    )
{


}

typedef BOOL (WINAPI *LPFNINITCOMMONCONTROLSEX)(LPINITCOMMONCONTROLSEX);

HRESULT
CActionPopup::CreateAndRunPopup(
    HINSTANCE hInst,
    LPCTSTR pszDlgTemplate,
    HWND hwndParent
    )
{
    INT iResult = 1;

    //
    // Load and initialize comctl32.dll.
    // We need it for the listview control in the dialog.
    //
    m_hmodCOMCTL32 = ::LoadLibrary(TEXT("comctl32.dll"));
    if (NULL != m_hmodCOMCTL32)
    {
        LPFNINITCOMMONCONTROLSEX pfnInitCommonControlsEx = NULL;

        pfnInitCommonControlsEx = (LPFNINITCOMMONCONTROLSEX)::GetProcAddress(m_hmodCOMCTL32, "InitCommonControlsEx");
        if (NULL != pfnInitCommonControlsEx)
        {
            INITCOMMONCONTROLSEX iccex;

            iccex.dwSize = sizeof(iccex);
            iccex.dwICC  = ICC_LISTVIEW_CLASSES;

            if ((*pfnInitCommonControlsEx)(&iccex))
            {
                iResult = DialogBoxParam(hInst,
                                         pszDlgTemplate,
                                         hwndParent,
                                         (DLGPROC)DlgProc,
                                         (LPARAM)this);
            }
        }
    }

    return (0 == iResult) ? NOERROR : E_FAIL;
}


HRESULT 
CActionPopup::DoAction(
    CHistory& history
    )
{
    HRESULT hr = E_FAIL;
    if (0 == CreateAndRunPopup(g_hInstDll,
                               MAKEINTRESOURCE(IDD_QUOTA_POPUP),
                               GetDesktopWindow()))
    {
        //
        // Record in the history log that we've popped up a dialog.
        //
        history.RecordDialogPoppedUp();
        hr = NOERROR;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CActionPopup::DlgProc [static]

    Description: Message procedure for the dialog.

    Arguments: Standard Win32 message proc arguments.

    Returns: Standard Win32 message proc return values.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK 
CActionPopup::DlgProc(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Retrieve the dialog object's ptr from the window's userdata.
    // Place there in response to WM_INITDIALOG.
    //
    CActionPopup *pThis = (CActionPopup *)GetWindowLong(hwnd, GWL_USERDATA);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            //
            // Store "this" ptr in window's userdata.
            //
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)lParam);
            pThis = (CActionPopup *)lParam;
            //
            // Save the HWND in our object.  We'll need it later.
            //
            pThis->m_hwnd = hwnd;

            return pThis->OnInitDialog(hwnd);

        case WM_DESTROY:
            return pThis->OnDestroy(hwnd);

        case WM_NCDESTROY:
            return pThis->OnNcDestroy(hwnd);

        case WM_TIMER:
            if (m_idAutoCloseTimer != wParam)
                break;
            //
            // Fall through to EndDialog...
            //
            DebugMsg(DM_ERROR, TEXT("CActionPopup::DlgProc - Dialog closed automatically."));

        case WM_COMMAND:
            EndDialog(hwnd, 0);
            break;

    }
    return FALSE;
}


BOOL
CActionPopup::OnInitDialog(
    HWND hwnd
    )
{
    BOOL bResult = TRUE;

    //
    // Set the quota icon.
    //
    SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)m_hiconDialog);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hiconDialog);

    //
    // Populate the listview with notification records.
    //
    InitializeList(GetDlgItem(hwnd, IDC_LIST_POPUP));

    //
    // Set the timer that will automatically close the dialog after 2 minutes.
    //
    SetTimer(hwnd, m_idAutoCloseTimer, m_uAutoCloseTimeout, NULL);

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CActionPopup::OnDestroy

    Description: 

    Arguments: 
        hwnd - Dialog window handle.
        
    Returns: Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL 
CActionPopup::OnDestroy(
    HWND hwnd
    )
{
    KillTimer(hwnd, m_idAutoCloseTimer);
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: CActionPopup::OnNcDestroy

    Description: 

    Arguments: 
        hwnd - Dialog window handle.
        
    Returns: Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL 
CActionPopup::OnNcDestroy(
    HWND hwnd
    )
{
    //
    // We no longer need comctl32.
    // Unload it.
    //
    if (NULL != m_hmodCOMCTL32)
    {
        FreeLibrary(m_hmodCOMCTL32);
        m_hmodCOMCTL32 = NULL;
    }
    return FALSE;
}


//
// Creates the listview columns and populates the listview from
// the statistics list object.
//
VOID
CActionPopup::InitializeList(
    HWND hwndList
    )
{
    //
    // We want to base pixel units off of dialog units.
    //
    INT DialogBaseUnitsX = LOWORD(GetDialogBaseUnits());

#define PIXELSX(du)  ((INT)((DialogBaseUnitsX * du) / 4))

    //
    // Create the header titles.
    //
    CString strVolume(g_hInstDll,  IDS_LVHDR_VOLUME);
    CString strUsed(g_hInstDll,    IDS_LVHDR_USED);
    CString strWarning(g_hInstDll, IDS_LVHDR_WARNING);
    CString strLimit(g_hInstDll,   IDS_LVHDR_LIMIT);

#define LVCOLMASK (LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM)

    LV_COLUMN rgCols[] = { 
                         { LVCOLMASK, LVCFMT_LEFT, PIXELSX(70), strVolume,  0, 0 },
                         { LVCOLMASK, LVCFMT_LEFT, PIXELSX(60), strUsed,    0, 1 },
                         { LVCOLMASK, LVCFMT_LEFT, PIXELSX(50), strWarning, 0, 2 },
                         { LVCOLMASK, LVCFMT_LEFT, PIXELSX(50), strLimit,   0, 3 }
                         };

    //
    // Add the columns to the listview.
    //
    for (INT i = 0; i < ARRAYSIZE(rgCols); i++)
    {
        if (-1 == ListView_InsertColumn(hwndList, i, &rgCols[i]))
        {
            DebugMsg(DM_ERROR, TEXT("CActionPopup::InitializeList failed to add column %d"), i);
        }
    }

    //
    // How many statistics objects are there in the stats list?
    //
    INT cEntries = m_stats.Count();
    //
    // This prevents the listview from having to extend itself each time we
    // add an item.
    //
    ListView_SetItemCount(hwndList, cEntries);

    //
    // Item struct for adding listview items and setting item text.
    //
    LV_ITEM item;
    item.mask = LVIF_TEXT;

    //
    // Scratch string for storing formatted column text.
    //
    CString str;

    //
    // For each row...
    //
    INT iRow = 0;
    for (INT iEntry = 0; iEntry < cEntries; iEntry++)
    {
        item.iItem = iRow;
        //
        // Retrieve the statistics object for this row.
        //
        const CStatistics *pStats = m_stats.GetEntry(iEntry);
        Assert(NULL != pStats);

        if (0 == iEntry)
        {
            //
            // First row.  Get the user's display name and
            // format/set the header message.
            //
            str.Format(g_hInstDll, IDS_POPUP_HEADER);
            SetWindowText(GetDlgItem(m_hwnd, IDC_TXT_POPUP_HEADER), str);
        }
                          
        if (pStats->IncludeInReport())
        {
            //
            // For each column...
            //
            for (INT iCol = 0; iCol < ARRAYSIZE(rgCols); iCol++)
            {
                item.iSubItem = iCol;
                switch(iCol)
                {
                    case 0:
                        //
                        // Location (volume display name)
                        //
                        item.pszText = pStats->GetVolumeDisplayName() ?
                                       (LPTSTR)((LPCTSTR)pStats->GetVolumeDisplayName()) :
                                       TEXT("");
                        break;                    

                    case 1:
                    {
                        TCHAR szBytes[40];
                        TCHAR szBytesOver[40];
                        //
                        // Quota Used
                        //
                        XBytes::FormatByteCountForDisplay(pStats->GetUserQuotaUsed().QuadPart,
                                                          szBytes, ARRAYSIZE(szBytes));

                        __int64 diff = pStats->GetUserQuotaUsed().QuadPart - pStats->GetUserQuotaThreshold().QuadPart;
                        if (0 > diff)
                        {
                            diff = 0;
                        }

                        XBytes::FormatByteCountForDisplay(diff, szBytesOver, ARRAYSIZE(szBytesOver));
                        str.Format(g_hInstDll, IDS_LVFMT_USED, szBytes, szBytesOver);

                        item.pszText = (LPTSTR)str;
                        break;
                    }

                    case 2:
                        //
                        // Warning Level
                        //
                        XBytes::FormatByteCountForDisplay(pStats->GetUserQuotaThreshold().QuadPart,
                                                          str.GetBuffer(40), 40);
                        item.pszText = (LPTSTR)str;
                        break;

                    case 3:
                        //
                        // Quota Limit.
                        //
                        XBytes::FormatByteCountForDisplay(pStats->GetUserQuotaLimit().QuadPart,
                                                          str.GetBuffer(40), 40);
                        item.pszText = (LPTSTR)str;
                        break;

                    default:
                        break;
                }
                if (0 == iCol)
                {
                    //
                    // Add the item to the listview.
                    //
                    if (-1 == ListView_InsertItem(hwndList, &item))
                    {
                        DebugMsg(DM_ERROR, TEXT("CActionPopup::InitializeList failed to add entry %d,%d"), iRow, iCol);
                    }
                }
                else
                {
                    //
                    // Set the text for a listview column entry.
                    // Note: There's no return value to check.
                    //
                    ListView_SetItemText(hwndList, iRow, iCol, (item.pszText));
                }
            }
            iRow++;
        }
    } 
}

