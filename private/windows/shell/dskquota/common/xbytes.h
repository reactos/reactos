#ifndef _INC_DSKQUOTA_XBYTES_H
#define _INC_DSKQUOTA_XBYTES_H
///////////////////////////////////////////////////////////////////////////////
/*  File: xbytes.h

    Description: This module implements a class that coordinates the operation
        between the edit control and combo box used for entering byte values.
        The name "XBytes" is used because the control can represent
        KBytes, MBytes, GBytes etc.

        The cooperation between edit control and combo control is required
        so that the user can enter a byte value in the edit control then
        indicate it's order (KB, MB, GB...) using a selection from the combo box.

        A simple external interface is provided to initially set the
        object's byte value then retrieve the byte value when needed.  The
        object's client is also required to call two member functions when
        the parent dialog receives an EN_UPDATE notification and a CBN_SELCHANGE
        message.  The XBytes object handles all of the value scaling
        internally.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
    10/15/96    Added m_MaxBytes member.                             BrianAu
    10/22/96    Added ValueInRange() member.                         BrianAu
    05/29/98    Removed ValueInRange() and m_MaxBytes members.       BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

const INT MAX_DECIMAL_SEP = 10;
const INT MAX_NOLIMIT_LEN = 80; // This should be plenty for localization.

#define MAX_ORDER         e_Exa
#define VALID_ORDER(ord)  ((ord) >= e_Byte && (ord) <= MAX_ORDER)


class XBytes
{
    private:
        INT64 m_ValueBytes;     // Byte value.
        HWND  m_hDlg;           // Parent dlg.
        DWORD m_idCtlEdit;      // Edit control.
        DWORD m_idCtlCombo;     // Combo control.
        static TCHAR m_szNoLimit[MAX_NOLIMIT_LEN];

        VOID CommonInit(VOID);

        INLINE BOOL IsCharNumeric(TCHAR ch)
            { return IsCharAlphaNumeric(ch) && !IsCharAlpha(ch); }
        BOOL StrToInt(LPCTSTR pszValue, INT64 *pIntValue);

        INLINE INT_PTR SendToEditCtl(UINT message, WPARAM wParam, LPARAM lParam)
            { return SendMessage(GetDlgItem(m_hDlg, m_idCtlEdit), message, wParam, lParam); }
        INLINE INT_PTR SendToCombo(UINT message, WPARAM wParam, LPARAM lParam)
            { return SendMessage(GetDlgItem(m_hDlg, m_idCtlCombo), message, wParam, lParam); }

        INLINE INT_PTR GetOrderFromCombo(VOID)
            { return SendToCombo(CB_GETCURSEL, 0, 0) + 1; }
        INLINE INT_PTR SetOrderInCombo(INT iOrder)
            { return SendToCombo(CB_SETCURSEL, iOrder-1, 0); }

        VOID LoadComboItems(INT64 MaxBytes);
        VOID SetBestDisplay(VOID);

        BOOL Store(INT64 Value, INT xbOrder);
        BOOL Store(LPCTSTR pszValue, INT xbOrder);

        INT64 Fetch(INT64 *pDecimal, INT xbOrder);    // Fetch in requested order.
        DWORD Fetch(DWORD *pDecimal, INT *pxbOrder);  // Fetch in "best" order.

        bool UndoLastEdit(void);

        static VOID LoadStaticStrings(void);
        static VOID FormatForDisplay(LPTSTR pszDest,
                                     UINT cchDest,
                                     DWORD dwWholePart,
                                     DWORD dwFracPart);
        VOID Enable(BOOL bEnable);

    public:

        //
        // With the exception of e_Byte, these must match the order
        // of the IDS_ORDERKB, IDS_ORDERMB... string resource IDs.
        // There is no IDS_ORDERBYTE string resource.
        //
        enum {e_Byte, e_Kilo, e_Mega, e_Giga, e_Tera, e_Peta, e_Exa};

        XBytes(VOID);

        XBytes(HWND hDlg, DWORD idCtlEdit, DWORD idCtlCombo, INT64 CurrentBytes);

        static double ConvertFromBytes(INT64 ValueBytes, INT xbOrder);
        static INT64 BytesToParts(INT64 ValueBytes, INT64 *pDecimal, INT xbOrder);
        static DWORD BytesToParts(INT64 ValueBytes, LPDWORD pDecimal, INT *pxbOrder);
        static VOID FormatByteCountForDisplay(INT64 Bytes, LPTSTR pszDest, UINT cchDest);
        static VOID FormatByteCountForDisplay(INT64 Bytes, LPTSTR pszDest, UINT cchDest, INT *pOrder);
        static VOID FormatByteCountForDisplay(INT64 Bytes, LPTSTR pszDest, UINT cchDest, INT Order);

        INT64 GetBytes(VOID)
            { return Fetch(NULL, e_Byte); }

        VOID SetBytes(INT64 Value);

        //
        // EN_xxxx handlers.  Client must call this on EN_UPDATE.
        //
        BOOL OnEditNotifyUpdate(LPARAM lParam);
        BOOL OnEditKillFocus(LPARAM lParam);

        //
        // CBN_xxxx handlers. Client must call this on CBN_SELCHANGE.
        //
        BOOL OnComboNotifySelChange(LPARAM lParam);

        BOOL IsEnabled(VOID);
};


#endif // _INC_DSKQUOTA_XBYTES_H
