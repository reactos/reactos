/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** sciset.c                                                           ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    SetRadix--Changes the number base and the radiobuttons.         ***/
/***    SetBox--Handles the checkboxes for inv/hyp.                     ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    none                                                            ***/
/***                                                                    ***/
/*** History:
 ***    12-Dec-1996 JonPa   -   Added SetMaxIntDigits
 ***    Whenever-97 ToddB   -   Removed SetMaxIntDigits
 ***/
/**************************************************************************/

#include "scicalc.h"
#include "unifunc.h"

extern TCHAR    szBlank[6];
extern INT      gcIntDigits;
extern TCHAR    *rgpsz[CSTRINGS];
extern TCHAR    szDec[];
extern RECT     rcDeg[6];
extern HMENU    g_hDecMenu;
extern HMENU    g_hHexMenu;

long oldRadix = (unsigned)-1;

void ActivateButtons()
{
    static int  aDecOnlyKeys[] = { IDC_FE, IDC_DMS, IDC_SIN, IDC_COS, IDC_TAN, IDC_EXP, IDC_PI };   // controls used only in Decimal mode

    if (oldRadix != nRadix)
    {
        int i;
        BOOL bDecMode = (nRadix == 10);
        
        // Only send messages to the the "Decimal Only keys" if this change in
        // base effects those keys

        if ((oldRadix == 10) || bDecMode)
        {
            // we are changing to or from decimal mode
            for ( i = 0; i <= ARRAYSIZE(aDecOnlyKeys) ; i++ )
            {
                EnableWindow( GetDlgItem(g_hwndDlg, aDecOnlyKeys[i]), 
                              bDecMode );
            }
        }

        // insure that nRadix is within the allowed range
        ASSERT( (nRadix >= 2) && (nRadix <= 16) );
        
        // turn on digit keys less than nRadix and turn off digit keys >= nRadix
        for (i=2; i<nRadix; i++)
            EnableWindow( GetDlgItem(g_hwndDlg, IDC_0+i), TRUE );

        for ( ; i<16; i++ )
            EnableWindow( GetDlgItem(g_hwndDlg, IDC_0+i), FALSE );
    }
    oldRadix = nRadix;
}

// SetRadix sets the display mode according to the selected button.
// ToddB:  As a hack to allow setting other bases, wRadix can be one of
//         the base buttons OR it can be the desired nRadix.

// MAXIUM: for Dec the precision is limited to the nPrecision, 
//  otherwise it is limited to the word size.

VOID NEAR SetRadix(DWORD wRadix)
{
    static INT  nRadish[4]={2,8,10,16}; /* Number bases.               */

    int   id=IDM_DEC;

    // convert special bases into symbolic values
    switch ( wRadix )
    {
    case 2:
        id=IDM_BIN;
        break;

    case 8:
        id=IDM_OCT;
        break;

    case 10:
        id=IDM_DEC;
        break;

    case 16:
        id=IDM_HEX;
        break;

    case IDM_HEX:
    case IDM_DEC:
    case IDM_OCT:
    case IDM_BIN:
        id=wRadix;
        wRadix = nRadish[IDM_BIN - wRadix];
        break;
    }

    // we select which group of toggles we are setting, decimal mode gets the
    // angular notation buttons (deg, rad, grad) otherwise we get the word size 
    // buttons (dword, word, byte)

    SwitchModes(wRadix, nDecMode, nHexMode);

    CheckMenuRadioItem(GetSubMenu(GetMenu(g_hwndDlg),1),IDM_HEX,IDM_BIN,id,
                       MF_BYCOMMAND);

    CheckRadioButton(g_hwndDlg,IDM_HEX, IDM_BIN, id);

    nRadix = wRadix;

    // inform ratpak that a change in base or precision has occured
    BaseOrPrecisionChanged();
    
    // update the UI elements to the correct state
    ActivateButtons();

    // display the correct number for the new state (ie convert displayed 
    //  number to correct base)
    DisplayNum();
}


// Check/uncheck the visible inverse/hyperbolic

VOID NEAR SetBox (int id, BOOL bOnOff)
{
    CheckDlgButton(g_hwndDlg, id, (WORD) bOnOff);
    return;
}

//
// Description:
//   This will switch the displayed/enabled mode buttons.  This also updates
//   The switches the menu under view and sets the correct state.
//
void
SwitchModes(DWORD wRadix, int nDecMode, int nHexMode)
{
    int iID, id;

    if (10 == wRadix)
    {
        id=IDM_DEG+nDecMode;

        if (NULL != g_hDecMenu)
            SetMenu(g_hwndDlg, g_hDecMenu);

        CheckMenuRadioItem(g_hDecMenu, IDM_DEG, IDM_GRAD, id, MF_BYCOMMAND);
        CheckRadioButton(g_hwndDlg,IDC_DEG, IDC_GRAD, id);
    }
    else
    {
        id=IDM_QWORD+nHexMode;

        if (NULL != g_hHexMenu)
            SetMenu(g_hwndDlg, g_hHexMenu);

        CheckMenuRadioItem(g_hHexMenu, IDM_QWORD, IDM_BYTE, id, MF_BYCOMMAND);
        CheckRadioButton(g_hwndDlg,IDC_QWORD, IDC_BYTE, id);
    }

    for (iID = IDC_QWORD; iID <= IDC_BYTE; iID++)
    {
        EnableWindow( GetDlgItem( g_hwndDlg, iID ), (wRadix != 10) );
        ShowWindow( GetDlgItem( g_hwndDlg, iID ),
                    (wRadix == 10) ? SW_HIDE : SW_SHOW );
    }

    for (iID = IDC_DEG; iID <= IDC_GRAD; iID++)
    {
        EnableWindow( GetDlgItem( g_hwndDlg, iID ), (wRadix == 10) );
        ShowWindow( GetDlgItem( g_hwndDlg, iID ), 
                    (wRadix != 10) ? SW_HIDE : SW_SHOW );
    }
}


