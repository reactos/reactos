///////////////////////////////////////////////////////////////////////////////
/*  File: xbytes.cpp

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

        NOTE: I experimented with adding a spin control to the edit control.
            I found that without some fancy intervention, the spin control
            didn't support fractional values (i.e. 2.5MB).  Decided to keep
            fractional values and ditch the spinner.  I think fractional
            values will be more useful to disk admins.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
    07/23/97    Added default ctor and CommonInit() function.        BrianAu
                Also added g_ForLoadingStaticStrings instance.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"  // PCH
#pragma hdrstop

#include "resource.h"
#include "xbytes.h"

const TCHAR CH_NUL      = TEXT('\0');
const TCHAR CH_ZERO     = TEXT('0');
const INT MAX_EDIT_TEXT = 16;                     // Max chars in edit text.
const INT MAX_CMB_TEXT  = 10;                     // For "KB", "MB", "GB" etc.
const INT64 MAX_VALUE   = ((1i64 << 60) * 6i64);  // Max is 6EB.
const INT64 MIN_VALUE   = 1024i64;                // Min value is 1KB.

TCHAR XBytes::m_szNoLimit[];            // "No Limit" edit control text.

///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::XBytes

    Description: Constructor

    Arguments:
        hDlg - Handle to parent dialog.

        idCtlEdit - Control ID for edit control.

        idCtlCombo - Control ID for combo box control.

        CurrentBytes - Initial byte value.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
    10/15/96    Added m_MaxBytes member.                             BrianAu
    05/29/98    Removed m_MaxBytes member. Don't want to limit       BrianAu
                user's ability to enter a value larger than
                max disk space.
*/
///////////////////////////////////////////////////////////////////////////////
XBytes::XBytes(
    HWND hDlg,
    DWORD idCtlEdit,
    DWORD idCtlCombo,
    INT64 CurrentBytes
    ) : m_hDlg(hDlg),
        m_idCtlEdit(idCtlEdit),
        m_idCtlCombo(idCtlCombo),
        m_ValueBytes(0)
{
    CommonInit();

    LoadComboItems(MAXLONGLONG);        // Load options into combo.
    CurrentBytes = MIN(CurrentBytes, MAX_VALUE);
    if (NOLIMIT != CurrentBytes)
        CurrentBytes = MAX(CurrentBytes, MIN_VALUE);

    SetBytes(CurrentBytes);             // Set "current bytes".
    //
    // Note: SetBytes() calls SetBestDisplay().
    //
}

//
// This constructor is sort of a hack.  Since the m_szNoLimit string
// is static, and since it is initialized in
// the constructor, at least one instance of XBytes must be created.
// There are cases where the static function FormatByteCountForDisplay
// may be useful when there is no need for an XBytes object.  The
// DiskQuota watchdog is just such an example.  If an XBytes object
// is not created, the two strings are not created and the function
// doesn't work correctly.  To fix this, I've defined a single global
// XBytes object constructed using this default constructor.  It's sole
// purpose is to load these static strings. [7/23/97 - brianau]
//
XBytes::XBytes(
    VOID
    ) : m_hDlg(NULL),
        m_idCtlEdit((DWORD)-1),
        m_idCtlCombo((DWORD)-1),
        m_ValueBytes(0)
{
    CommonInit();
}

//
// Initialization common to both constructors.
//
VOID
XBytes::CommonInit(
    VOID
    )
{
    if (NULL != m_hDlg)
        SendMessage(m_hDlg, m_idCtlEdit, EM_LIMITTEXT, MAX_EDIT_TEXT);

    LoadStaticStrings();
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::SetBytes

    Description: Stores a new byte value and updates the display to the
        proper units (order).

    Arguments:
        ValueBytes - Value in bytes.
            If the value is NOLIMIT, the controls are disabled.
            Otherwise the controls are enabled.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::SetBytes(INT64 ValueBytes)
{
    if (NOLIMIT != ValueBytes)
        ValueBytes = MAX(MIN_VALUE, ValueBytes);

    ValueBytes = MIN(MAX_VALUE, ValueBytes);
    Store(ValueBytes, e_Byte);
    SetBestDisplay();
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::Enable

    Description: Enables/Disables the edit and combo controls.

    Arguments:
        bEnable - TRUE = Enable, FALSE = Disable.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/28/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::Enable(
    BOOL bEnable
    )
{
    EnableWindow(GetDlgItem(m_hDlg, m_idCtlCombo), bEnable);
    EnableWindow(GetDlgItem(m_hDlg, m_idCtlEdit), bEnable);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::IsEnabled

    Description: Returns the "enabled" state of the edit control.  As long
        as the client doesn't enable/disable the edit/combo controls
        individually, this represents the state of the control pair.
        By using only the SetBytes() method to control enabling/disabling,
        this is ensured.

    Arguments:
        bEnable - TRUE = Enable, FALSE = Disable.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/28/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
XBytes::IsEnabled(
    VOID
    )
{
    return IsWindowEnabled(GetDlgItem(m_hDlg, m_idCtlEdit));
}


bool
XBytes::UndoLastEdit(
    void
    )
{
    if (SendToEditCtl(EM_CANUNDO, 0, 0))
    {
        SendToEditCtl(EM_UNDO, 0, 0);
        SendToEditCtl(EM_EMPTYUNDOBUFFER, 0, 0);
        SendToEditCtl(EM_SETSEL, SendToEditCtl(EM_LINELENGTH, 0, 0), -1);
        return true;
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::OnEditNotifyUpdate

    Description: Must be called whenever the parent window receives a
        EN_UPDATE notification for the edit control.  The function
        reads the current string in the edit control and tries to store it
        as a byte value.  If the store operation fails, the number is invalid
        and an alarm is sounded.

    Arguments:
        lParam - lParam argument to EN_UPDATE notification.  It is unused.

    Returns:
        Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
    10/15/96    Added check for too-large input.                     BrianAu
    10/22/96    Re-organized and added ValueInRange() function.      BrianAu
                This was to support value check/adjustment when
                user changes the combo-box setting (bug).
    02/26/97    Added EM_CANUNDO and EM_EMPTYUNDOBUFFER.             BrianAu
    05/29/98    Removed ValueInRange() function and replaced with    BrianAu
                check for negative number.
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
XBytes::OnEditNotifyUpdate(
    LPARAM lParam
    )
{
    TCHAR szEditText[MAX_PATH];
    bool bBeep = false;

    DBGASSERT((MAX_EDIT_TEXT < MAX_PATH));

    GetDlgItemText(m_hDlg, m_idCtlEdit, szEditText, ARRAYSIZE(szEditText));
    if (lstrlen(szEditText) > MAX_EDIT_TEXT)
    {
        szEditText[MAX_EDIT_TEXT] = TEXT('\0');
        SetDlgItemText(m_hDlg, m_idCtlEdit, szEditText);
    }

    if (0 != lstrcmpi(XBytes::m_szNoLimit, szEditText))
    {
        //
        // If text in edit control is not "No Limit", convert the text to
        // a number, verify that it is in range and store it.
        //
        if (Store(szEditText, (INT)GetOrderFromCombo()))
        {
            //
            // If number is negative, force it to the minimum.
            //
            if (0 > Fetch(NULL, e_Byte))
            {
                SetBytes(MIN_VALUE);
                bBeep = true;
            }

            SendToEditCtl(EM_EMPTYUNDOBUFFER, 0, 0);
        }
        else
        {
            bBeep = true;
            if (!UndoLastEdit())
            {
                //
                // Number must be too large for the selected order.
                // Found that this can happen when first opening the disk quota UI
                // after someone's set the value out of the range acceptable by
                // the UI.  Remember, because we allow decimal values in the UI,
                // the UI cannot accept values quite as large as the dskquota APIs.
                // Beep the user and force the value to the largest acceptable
                // value.
                //
                SetBytes(MAX_VALUE);
            }
        }
        if (bBeep)
        {
            //
            // Sound beep for either an invalid value or an out-of-range value.
            //
            MessageBeep(MB_OK);
        }
    }

    return FALSE;
}


BOOL
XBytes::OnEditKillFocus(
    LPARAM lParam
    )
{
    TCHAR szEditText[MAX_EDIT_TEXT];
    bool bBeep = false;

    GetDlgItemText(m_hDlg, m_idCtlEdit, szEditText, ARRAYSIZE(szEditText));

    if (0 != lstrcmpi(XBytes::m_szNoLimit, szEditText))
    {
        INT64 value = Fetch(NULL, e_Byte);
        if (MIN_VALUE > value)
        {
            SetBytes(MIN_VALUE);
            bBeep = true;
        }
        else if (MAX_VALUE < value)
        {
            SetBytes(MAX_VALUE);
            bBeep = true;
        }
        if (bBeep)
        {
            MessageBeep(MB_OK);
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::OnComboNotifySelChange

    Description: Must be called whenever the parent window receives a
        CBM_SELCHANGE message for the combo box control.  The function
        scales the stored byte value to the new units.

    Arguments:
        lParam - lParam argument to CBM_SELCHANGE message.  It is unused.

    Returns:
        Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
    10/22/96    Modified to just call OnEditNotifyUpdate().          BrianAu
                Combo-box selection should have same value
                check/adjust behavior as edit control changes.
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
XBytes::OnComboNotifySelChange(
    LPARAM lParam
    )
{
    TCHAR szEditText[MAX_EDIT_TEXT];
    bool bBeep = false;

    GetDlgItemText(m_hDlg, m_idCtlEdit, szEditText, ARRAYSIZE(szEditText));

    if (0 != lstrcmpi(XBytes::m_szNoLimit, szEditText))
    {
        //
        // If text in edit control is not "No Limit", convert the text to
        // a number, verify that it is in range and store it.
        //
        if (Store(szEditText, (INT)GetOrderFromCombo()))
        {
            //
            // If number is less than the minimum, force it to the minimum.
            //
            if (MIN_VALUE > Fetch(NULL, e_Byte))
            {
                SetBytes(MIN_VALUE);
                bBeep = true;
            }
        }
        else
        {
            //
            // Number must be too large for the selected order.
            // Beep the user and force the value to the largest acceptable
            // value.
            //
            SetBytes(MAX_VALUE);
            bBeep = true;
        }
        if (bBeep)
        {
            MessageBeep(MB_OK);
        }
    }
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::LoadComboItems

    Description: Initializes the combo box with its selections.
        [ "KB", "MB", "GB"... "PB" ].  The function only adds options that are
        reasonable for the size of the drive.  For example, if the drive
        is less than 1 GB in size, only KB and MB are displayed.

    Arguments:
        MaxBytes - Maximum bytes available on the drive (drive size).

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::LoadComboItems(
    INT64 MaxBytes
    )
{
    TCHAR szText[MAX_CMB_TEXT];
    INT idMsg = 0;
    INT order = e_Kilo;

    //
    // Find the string resource ID for the largest units possible.
    //
    // WARNING: This code assumes that the resource IDs for
    //          IDS_ORDERKB through IDS_ORDEREB are consecutive
    //          increasing integers.  Hence the following assertions.
    //
    DBGASSERT((IDS_ORDERMB == IDS_ORDERKB + 1));
    DBGASSERT((IDS_ORDERGB == IDS_ORDERKB + 2));
    DBGASSERT((IDS_ORDERTB == IDS_ORDERKB + 3));
    DBGASSERT((IDS_ORDERPB == IDS_ORDERKB + 4));
    DBGASSERT((IDS_ORDEREB == IDS_ORDERKB + 5));

    for (idMsg = IDS_ORDERKB; idMsg < IDS_ORDEREB; idMsg++)
    {
        if ((INT64)(1i64 << (10 * order++)) > MaxBytes)
        {
            idMsg--;
            break;
        }
    }

    //
    // idMsg is at largest units string we'll use.
    // Add strings to combo box.
    //
    while(idMsg >= IDS_ORDERKB)
    {
        if (LoadString(g_hInstDll, idMsg, szText, ARRAYSIZE(szText)))
            SendToCombo(CB_INSERTSTRING, 0, (LPARAM)szText);
        idMsg--;
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::SetBestDisplay

    Description: Displays the byte value in the highest order that will
        produce a whole part of 3 digits or less.  That way you see
        "25.5" MB instead of "25500 KB".

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::SetBestDisplay(
    VOID
    )
{
    INT iOrder = e_Byte;
    TCHAR szValue[MAX_EDIT_TEXT];

    //
    // Display NOLIMIT as 0.  Edit and combo controls will be disabled
    // by property page code.  NOLIMIT is (-1).  Defined by NTFS.
    //
    if (NOLIMIT != m_ValueBytes)
    {
        //
        // Format the byte count for display.  Leave off the KB, MB... extension.
        // That part will be displayed in the combo box.
        //
        FormatByteCountForDisplay(m_ValueBytes, szValue, ARRAYSIZE(szValue), &iOrder);

        //
        // If value is 0, display MB units.  That's our default.
        //
        if (0 == m_ValueBytes)
            iOrder = e_Mega;

        //
        // Set the value string in the edit control and the order in the combo box.
        //
        SetOrderInCombo(iOrder);
        SetDlgItemText(m_hDlg,
                       m_idCtlEdit,
                       szValue);

        Enable(TRUE);
    }
    else
    {
        //
        // Set edit control to display "No Limit".
        //
        SetOrderInCombo(0);  // This will cause the combo to display nothing.
        SetDlgItemText(m_hDlg,
                       m_idCtlEdit,
                       m_szNoLimit);

        Enable(FALSE);
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::Store

    Description: Store a value in a given order as a byte count.

    Arguments:
        Value - Byte value in order xbOrder.

        xbOrder - Order of number in Value.
            One of set { e_Byte, e_Kilo, e_Mega ... e_Exa }

    Returns:
        TRUE    - Success.  Always returns TRUE.
                  Event though we're not returning anything useful, I want
                  the return type for both Store() methods to be the same.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
XBytes::Store(
    INT64 Value,
    INT xbOrder
    )
{
    DBGASSERT((VALID_ORDER(xbOrder)));

    m_ValueBytes = INT64(Value) << (10 * (xbOrder - e_Byte));
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::Store

    Description: Store a numeric string in a given order as a byte count.

    Arguments:
        pszSource - Numeric string.

        xbOrder - Order of number in pszSource.
            One of set { e_Byte, e_Kilo, e_Mega ... e_Exa }

    Returns:
        TRUE    - Success.
        FALSE   - Invalid number in string.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
XBytes::Store(
    LPCTSTR pszSource,
    INT xbOrder
    )
{
    TCHAR szValue[MAX_EDIT_TEXT];    // Temp buffer.
    TCHAR szDecimalSep[MAX_DECIMAL_SEP];
    LPTSTR pszValue = szValue;       // Pointer into temp buffer.
    LPTSTR pszDec   = szValue;       // Pointer to decimal part of temp buffer.
    BOOL bResult    = FALSE;
    UINT uMult      = 1;             // Digit multiplier.
    INT64 WholePart = 0;
    INT64 FracPart  = 0;
    DWORD xbOrderX10 = xbOrder * 10; // Saves multiple computations.

    DBGASSERT((NULL != pszSource));
    DBGASSERT((VALID_ORDER(xbOrder)));

    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SDECIMAL,
                  szDecimalSep,
                  ARRAYSIZE(szDecimalSep));

    //
    // Local copy to party on.
    //
    lstrcpyn(szValue, pszSource, ARRAYSIZE(szValue));

    //
    // Find the start of the decimal separator.
    //
    while(NULL != *pszDec && szDecimalSep[0] != *pszDec)
        pszDec++;

    if (CH_NUL != *pszDec)
    {
        *pszDec = CH_NUL;      // Terminate the whole part.

        //
        // Skip over the decimal separator character(s).
        // Remember, separator is localized.
        //
        LPTSTR pszDecimalSep = &szDecimalSep[1];
        pszDec++;
        while(*pszDecimalSep && *pszDec && *pszDec == *pszDecimalSep)
        {
            pszDecimalSep++;
            pszDec++;
        }
    }
    else
        pszDec = NULL;          // No decimal pt found.

    //
    // Convert whole part to an integer.
    //
    if (!StrToInt(pszValue, &WholePart))
        goto not_a_number;

    //
    // Check to make sure the number entered will fit into a 64-bit int when
    // scaled up.
    // With the text entry field and order combo, users can specify numbers
    // that will overflow an __int64.  Can't let this happen.  Treat overflows
    // as invalid entry.  The (-1) accounts for the largest fractional part
    // that the user could enter.
    //
    if (WholePart > ((MAXLONGLONG >> xbOrderX10) - 1))
        goto not_a_number;

    //
    // Scale whole part according to order.
    //
    WholePart *= (1i64 << xbOrderX10);

    //
    // Convert fractional part to an integer.
    //
    if (NULL != pszDec)
    {
        //
        // Trim any trailing zero's first.
        //
        LPTSTR pszZero = pszDec + lstrlen(pszDec) - 1;
        while(pszZero >= pszDec && CH_ZERO == *pszZero)
            *pszZero-- = CH_NUL;

        //
        // Convert decimal portion of string to an integer.
        //
        if (!StrToInt(pszDec, &FracPart))
            goto not_a_number;

        //
        // Scale fractional part according to order.
        //
        FracPart *= (1i64 << xbOrderX10);

        DWORD dwDivisor = 1;
        while(pszZero-- >= pszDec)
            dwDivisor *= 10;

        //
        // Round up to the nearest muliple of the divisor to prevent
        // undesireable truncation during integer division we do below.
        //
        DWORD dwRemainder = (DWORD)(FracPart % dwDivisor);
        if (0 != dwRemainder)
            FracPart += dwDivisor - dwRemainder;

        FracPart /= dwDivisor;
    }

    m_ValueBytes = WholePart + FracPart;
    bResult = TRUE;

not_a_number:

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::Fetch

    Description: Retrieve the byte count from the object using a specified
        order (magnitude).  i.e. For 60.5 MB, the order is e_Mega, the decimal
        part is 5 and the returned value is 60.

    Arguments:
        pDecimal [optional] - Address of DWORD to receive the fractional part
            of the byte count.  May be NULL.

        xbOrder - Order desired for the returned value.  Must be from the
            enumeration set { e_Byte, e_Kilo, e_Mega ... e_Exa }

    Returns:
        Returns the whole part of the byte count.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT64
XBytes::Fetch(
    INT64 *pDecimal,
    INT xbOrder
    )
{
    return BytesToParts(m_ValueBytes, pDecimal, xbOrder);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::Fetch

    Description: Retrieve the byte count from the object and tell the caller
        what the best order is for display.  The logic used for "best order"
        is to use the first order that results in a 3-digit number.

    Arguments:
        pDecimal - Address of DWORD to receive the fractional part of the
            byte count.

        pxbOrder - Address of integer to receive the order of the number
            being returned.  The returned order is in the enumeration
            { e_Byte, e_Kilo, e_Mega ... e_Exa }

    Returns:
        Returns the whole part of the byte count.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
XBytes::Fetch(
    LPDWORD pDecimal,
    INT *pxbOrder
    )
{
    return BytesToParts(m_ValueBytes, pDecimal, pxbOrder);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::StrToInt

    Description: Converts a string to an integer.

    Arguments:
        pszValue - Address of string to convert.

        pIntValue - Address of INT64 variable to receive resulting number.

    Returns:
        TRUE    - Successful conversion.
        FALSE   - String was not a valid integer.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
XBytes::StrToInt(
    LPCTSTR pszValue,
    INT64 *pIntValue
    )
{
    INT iMult   = 1;
    INT64 Value = 0;
    LPCTSTR pszDigit = pszValue + lstrlen(pszValue) - 1; // Start at right-most

    DBGASSERT((NULL != pszValue));
    DBGASSERT((NULL != pIntValue));

    *pIntValue = 0;
    while(pszDigit >= pszValue)
    {
        //
        // Moving left... check each digit.
        //
        if (IsCharNumeric(*pszDigit))
        {
            //
            // Valid digit.  Add it's value to sum.
            //
            Value += iMult * (*pszDigit - CH_ZERO);
            pszDigit--;
            iMult *= 10;
        }
        else
            return FALSE; // Invalid character.
    }
    *pIntValue = Value;
    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::BytesToParts

    Description: Converts a byte value to it's whole and fractional parts
        for a given magnitude (order).  This is a static member function
        that can be used outside of the context of an XBytes object.

    Arguments:
        ValueBytes - Value to convert expressed in bytes.

        pDecimal [optional] - Address of variable to receive the fractional
            part.  May be NULL.

        xbOrder - Order that the parts are to represent.
            { e_Byte, e_Kilo, e_Mega ... e_Exa }

    Returns: Returns the whole part of the value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT64
XBytes::BytesToParts(
    INT64 ValueBytes,
    INT64 *pDecimal,
    INT xbOrder
    )
{
    INT64 Value = ValueBytes;
    UINT64 DecMask = 0;             // Start with a blank mask.
    DWORD dwOrderDeltaX10 = 10 * (xbOrder - e_Byte);

    DBGASSERT((VALID_ORDER(xbOrder)));

    //
    // Convert the value from order e_Byte to the order requested.
    // Also build a mask that can extract the decimal portion
    // from the original byte value.   The following 2 statements implement
    // this logic.
    //
    // for (INT i = e_Byte; i < xbOrder; i++)
    // {
    //     ValueBytes >>= 10;  // Divide byte value by 1024.
    //     DecMask <<= 10;     // Shift current mask bits 10 left.
    //     DecMask |= 0x3FF;   // OR in another 10 bits.
    // }
    //
    Value >>= dwOrderDeltaX10;
    DecMask = (1i64 << dwOrderDeltaX10) - 1;

    if (NULL != pDecimal)
    {
        //
        // Caller wants fractional part.
        // Extract fractional part from byte value and scale it to the
        // specified order.
        // Pseudocode:
        //      x   = value & mask
        //      pct = x / (2**order)    // ** = "raise to the power of".
        //      dec = 100 * pct
        //
        *pDecimal = (INT64)(100 * (ValueBytes & DecMask)) >> (10 * xbOrder);
    }

    return Value;
}


double
XBytes::ConvertFromBytes(
    INT64 ValueBytes,
    INT xbOrder
    )
{
    return (double)ValueBytes / (double)(10 * xbOrder);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::BytesToParts

    Description: Converts a byte value to it's whole and fractional parts.
        Determines the maximum magnitude (order) that will display the
        whole part in 3 digits or less.
        This is a static member function that can be used outside of the
        context of an XBytes object.

    Arguments:
        ValueBytes - Value to convert expressed in bytes.

        pDecimal [optional] - Address of variable to receive the fractional
            part.  May be NULL.

        pxbOrder - Address of variable to receive the determined order.
            { e_Byte, e_Kilo, e_Mega ... e_Exa }

    Returns: Returns the whole part of the value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
DWORD
XBytes::BytesToParts(
    INT64 ValueBytes,
    LPDWORD pDecimal,
    INT *pxbOrder
    )
{
    INT64 Value   = 0;
    INT64 Decimal = 0;
    INT xbOrder   = e_Byte;

    DBGASSERT((NULL != pDecimal));
    DBGASSERT((NULL != pxbOrder));

    //
    // Determine the best order for display.
    //
    while(xbOrder <= MAX_ORDER)
    {
        Value = BytesToParts(ValueBytes, &Decimal, xbOrder);
        if (Value < (INT64)1000)
            break;
        xbOrder++;
    }

    //
    // Return the results.
    //
    *pxbOrder = xbOrder;
    *pDecimal = (DWORD)Decimal;  // Fetch() guarantees this cast is OK.

    return (DWORD)Value;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::FormatByteCountForDisplay

    Description: Given a byte count, this static method formats a character
        string with the 999.99XB number where "XB" is the maximum units
        that can display the whole part in 3 digits or less.

    Arguments:
        Bytes - Number of bytes to format.

        pszDest - Address of destination character buffer.

        cchDest - Size of destination buffer in characters.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::FormatByteCountForDisplay(
    INT64 Bytes,
    LPTSTR pszDest,
    UINT cchDest
    )
{
    DWORD dwWholePart = 0;
    DWORD dwFracPart  = 0;
    INT Order         = XBytes::e_Byte;

    //
    // To avoid using a local temp buffer, the caller's buffer must be
    // large enough for final string.  "999.99 MB" plus NUL and some pad to
    // allow for possible multi-char decimal separators (localized).
    //
    DBGASSERT((NULL != pszDest));

    FormatByteCountForDisplay(Bytes, pszDest, cchDest, &Order);

    DWORD dwLen = lstrlen(pszDest);
    //
    // Insert a space between the number and the suffix (i.e. "99 MB").
    // dwLen is incremented to allow for the added space.
    //
    *(pszDest + dwLen++) = TEXT(' ');
    //
    // Append the suffix.
    //
    LoadString(g_hInstDll, IDS_ORDERKB + Order - 1, pszDest + dwLen, cchDest - dwLen);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::FormatByteCountForDisplay

    Description: Given a byte count, this static method formats a character
        string with the 999.99 number and returns the enumerted value
        representing the order in *pOrder.  This function complements
        the one above for those callers not needing the "KB", "MB"...
        suffix.  In particular, our combo box.

    Arguments:
        Bytes - Number of bytes to format.

        pszDest - Address of destination character buffer.

        cchDest - Size of destination buffer in characters.

        pOrder - Address of variable to receive the enumerated order value.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::FormatByteCountForDisplay(
    INT64 Bytes,
    LPTSTR pszDest,
    UINT cchDest,
    INT *pOrder
    )
{
    DBGASSERT((NULL != pszDest));
    DBGASSERT((NULL != pOrder));

    DWORD dwWholePart = 0;
    DWORD dwFracPart  = 0;

    dwWholePart = BytesToParts(Bytes, &dwFracPart, pOrder);

    FormatForDisplay(pszDest, cchDest, dwWholePart, dwFracPart);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::FormatByteCountForDisplay

    Description: Given a byte count, and a specified order, this static method
        formats a character string with the 999.99 number in the specified
        order.

    Arguments:
        Bytes - Number of bytes to format.

        pszDest - Address of destination character buffer.

        cchDest - Size of destination buffer in characters.

        Order - Order of the value in the resultant string.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::FormatByteCountForDisplay(
    INT64 Bytes,
    LPTSTR pszDest,
    UINT cchDest,
    INT Order
    )
{
    LONGLONG llWholePart;
    LONGLONG llFracPart;

    DBGASSERT((NULL != pszDest));

    //
    // WARNING: This code assumes that the whole and fractional parts will
    //          each be less than 2^32.  I think a valid assumption for scaled
    //          quota information.
    //
    llWholePart = BytesToParts(Bytes, &llFracPart, Order);
    FormatForDisplay(pszDest, cchDest, (DWORD)llWholePart, (DWORD)llFracPart);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: XBytes::FormatForDisplay

    Description: Given a whole part and a fractional part, format a decimal
        number suitable for display in 999.99 format.  If the fractional
        part is 0, no decimal part is included.

    Arguments:
        pszDest - Address of destination character buffer.

        cchDest - Size of destination buffer in characters.

        dwWholePart - Whole part of the number.

        dwFracPart - Fractional part of the number.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/30/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
XBytes::FormatForDisplay(
    LPTSTR pszDest,
    UINT cchDest,
    DWORD dwWholePart,
    DWORD dwFracPart
    )
{
    DBGASSERT((NULL != pszDest));

    TCHAR szTemp[80];

    if (0 != dwFracPart)
    {
        TCHAR szFmt[] = TEXT("%d%s%02d");
        TCHAR szDecimalSep[MAX_DECIMAL_SEP];

        if ((dwFracPart >= 10) && (0 == (dwFracPart % 10)))
        {
            //
            // Whack off the trailing zero for display.
            //
            dwFracPart /= 10;
            szFmt[6] = TEXT('1');
        }

        GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_SDECIMAL,
                      szDecimalSep,
                      ARRAYSIZE(szDecimalSep));

        wsprintf(szTemp, szFmt, dwWholePart, szDecimalSep, dwFracPart);
    }
    else
        wsprintf(szTemp, TEXT("%d"), dwWholePart);

    lstrcpyn(pszDest, szTemp, cchDest);
}

//
// Load the static strings if they haven't been loaded.
//
VOID
XBytes::LoadStaticStrings(
    void
    )
{
    //
    // Initialize the "No Limit" text string for display in the
    // edit control.  This is the same string used in the details list
    // view columns.
    //
    if (TEXT('\0') == m_szNoLimit[0])
    {
        INT cchLoaded = LoadString(g_hInstDll,
                                   IDS_NO_LIMIT,
                                   m_szNoLimit,
                                   ARRAYSIZE(m_szNoLimit));

        DBGASSERT((0 < cchLoaded));
    }
}
