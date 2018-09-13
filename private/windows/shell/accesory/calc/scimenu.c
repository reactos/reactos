/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scimenu.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    MenuFunctions--handles menu options.                            ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    DisplayNum                                                      ***/
/***                                                                    ***/
/*** Last modification Thu  06-Dec-1989                                 ***/
/*** (-by- Amit Chatterjee [amitc])                                     ***/
/***                                                                    ***/
/*** Modified the 'PASTE' menu to check for unary minus, e, e+ & e-     ***/
/*** in DEC mode.                                                       ***/
/***                                                                    ***/
/*** Also modified the COPY code to not copy the last '.' in the display***/
/*** if a decimal point has not been hit.                               ***/
/***                                                                    ***/
/**************************************************************************/

#include "scicalc.h"
#include "unifunc.h"
#include "input.h"
#include <shellapi.h>
#include <ctype.h>

#define CHARSCAN    66

extern HWND        hEdit, hStatBox;
extern TCHAR       szAppName[10], szDec[5], gszSep[5], *rgpsz[CSTRINGS];
extern LPTSTR      gpszNum;
extern BOOL        bError;
extern INT         nLayout;

extern HMENU       g_hDecMenu;
extern HMENU       g_hHexMenu;

extern CALCINPUTOBJ gcio;
extern BOOL         gbRecord;
extern BOOL         gbUseSep;

/* Menu handling routine for COPY, PASTE, ABOUT, and HELP.                */
VOID NEAR PASCAL MemErrorMessage(VOID)
{
    MessageBeep(0);
    MessageBox(g_hwndDlg,rgpsz[IDS_STATMEM],NULL,MB_OK|MB_ICONHAND);
}

VOID  APIENTRY MenuFunctions(DWORD nFunc)
{
    INT              nx;
    static const int rgbMap[CHARSCAN * 2]=
    {
        TEXT('0'),IDC_0,    TEXT('1'),IDC_1,    
        TEXT('2'),IDC_2,    TEXT('3'),IDC_3,

        TEXT('4'),IDC_4,    TEXT('5'),IDC_5,
        TEXT('6'),IDC_6,    TEXT('7'),IDC_7,

        TEXT('8'),IDC_8,    TEXT('9'),IDC_9,
        TEXT('A'),IDC_A,    TEXT('B'),IDC_B,

        TEXT('C'),IDC_C,    TEXT('D'),IDC_D,
        TEXT('E'),IDC_E,    TEXT('F'),IDC_F,

        TEXT('!'),IDC_FAC,  TEXT('S'),IDC_SIN,
        TEXT('O'),IDC_COS,  TEXT('T'),IDC_TAN,

        TEXT('R'),IDC_REC,  TEXT('Y'),IDC_PWR,
        TEXT('#'),IDC_CUB,  TEXT('@'),IDC_SQR,
                        
        TEXT('M'),IDM_DEG,  TEXT('N'),IDC_LN,
        TEXT('L'),IDC_LOG,  TEXT('V'),IDC_FE,

        TEXT('X'),IDC_EXP,  TEXT('I'),IDC_INV,
        TEXT('H'),IDC_HYP,  TEXT('P'),IDC_PI,

        TEXT('/'),IDC_DIV,  TEXT('*'),IDC_MUL,
        TEXT('%'),IDC_MOD,  TEXT('-'),IDC_SUB,

        TEXT('='),IDC_EQU,  TEXT('+'),IDC_ADD,
        TEXT('&'),IDC_AND,  TEXT('|'),IDC_OR,

        TEXT('^'),IDC_XOR,  TEXT('~'),IDC_COM,
        TEXT(';'),IDC_CHOP, TEXT('<'),IDC_LSHF,


        TEXT('('),IDC_OPENP,TEXT(')'),IDC_CLOSEP,

        TEXT('\\'),    IDC_DATA,
        TEXT('Q'),     IDC_CLEAR,
        TEXT('Q')+128, IDC_CLEAR,   // ":Q"=="Q"=>CLEAR
        TEXT('S')+128, IDC_STAT,    // ":S"=>CTRL-S
        TEXT('M')+128, IDC_STORE,   // ":M"=>CTRL-M
        TEXT('P')+128, IDC_MPLUS,   // ":P"=>CTRL-P
        TEXT('C')+128, IDC_MCLEAR,  // ":C"=>CTRL-C
        TEXT('R')+128, IDC_RECALL,  // ":R"=>CTRL-R
        TEXT('A')+128, IDC_AVE,     // ":A"=>CTRL-A
        TEXT('T')+128, IDC_B_SUM,   // ":T"=>CTRL-T
        TEXT('D')+128, IDC_DEV,     // ":D"=>CTRL-D
        TEXT('2')+128, IDC_DWORD,   // ":2"=>F2     IDC_DWORD
        TEXT('3')+128, IDC_RAD,     // ":3"=>F3     IDC_WORD
        TEXT('4')+128, IDC_GRAD,    // ":4"=>F4     IDC_BYTE
        TEXT('5')+128, IDC_HEX,     // ":5"=>F5
        TEXT('6')+128, IDC_DEC,     // ":6"=>F6
        TEXT('7')+128, IDC_OCT,     // ":7"=>F7
        TEXT('8')+128, IDC_BIN,     // ":8"=>F8
        TEXT('9')+128, IDC_SIGN,    // ":9"=>F9
        TEXT('9')+3+128, IDC_QWORD  // ":9"+2=>F12 (64 bit)
   };

    switch (nFunc)
    {
        case IDM_COPY:
        {
            TCHAR  szJunk[256];

            // Copy the string into a work buffer.  It may be modified.
            if (gbRecord)
                CIO_vConvertToString(&gpszNum, &gcio, nRadix);

            lstrcpy(szJunk, gpszNum);

            // Strip a trailing decimal point if it wasn't explicitly entered.
            if (!gbRecord || !CIO_bDecimalPt(&gcio))
            {
                nx = lstrlen(szJunk);
                if (szJunk[nx - 1] == szDec[0])
                    szJunk[nx - 1] = 0;
            }

            /* Copy text to the clipboard through the hidden edit control.*/
            SetWindowText(hEdit, szJunk);
            SendMessage(hEdit, EM_SETSEL, 0, -1);   // select all text
            SendMessage(hEdit, WM_CUT, 0, 0L);
            break;
        }

        case IDM_PASTE:
        {
            HANDLE  hClipData;
            char *  lpClipData;
            char *  lpEndOfBuffer;  // used to ensure we don't GPF even if the clipboard data isn't NULL terminated
            WORD    b, bLast;
            INT     nControl;
            BOOL    bNeedIDC_SIGN = FALSE;

            /* Get a handle on the clipboard data and paste by sending the*/
            /* contents one character at a time like it was typed.        */
            if (!OpenClipboard(g_hwndDlg))
            {
                MessageBox(g_hwndDlg, rgpsz[IDS_NOPASTE], rgpsz[IDS_CALC],
                           MB_OK | MB_ICONEXCLAMATION);
                break;
            }

            hClipData=GetClipboardData(CF_TEXT);
            lpClipData=(char *)GlobalLock(hClipData);
            lpEndOfBuffer = lpClipData + GlobalSize(hClipData);
            bLast=0;

            /* Continue this as long as no error occurs.  If one      */
            /* does then it's useless to continue pasting.            */
            while (!bError && lpClipData < lpEndOfBuffer)
            {
                // we know that lpClipData points to a NULL terminated ansi 
                // string because this is the format we requested the data in.
                // As a result we call CharNextA.

                b = *lpClipData;
                lpClipData = CharNextA( lpClipData );

                /* Skip spaces and LF and CR.                             */
                if (b==32 || b==10 || b==13 || b==gszSep[0])
                    continue;

                /* We're done if we get to a NULL character */
                if ( b==0 )
                    break;

                if (b == szDec[0])
                {
                    bLast = b;
                    b = IDC_PNT;
                    goto MappingDone;
                }

/*-----------------------------------------------------------------------------;
; Now we will check for certain special cases. These are:                      ;
;                                                                              ;
;       (1) Unary Minus. If bLast is still 0 and b is '-' we will force b to   ;
;         be the code for 'SIGN'.                                              ;
;       (2) If b is 'x' we will make it the code for EXP                       ;
;       (3) if bLast is 'x' and b is '+' we will ignore b, as '+' is the dflt. ;
;       (4) if bLast is 'x' and b is '-' we will force b to be SIGN.           ;
;                                                                              ;
;  In case (3) we will go back to the top of the loop else we will jmp off     ;
;  to the sendmessage point, bypassing the table lookup.                       ;
;-----------------------------------------------------------------------------*/

                /* check for unary minuses */
                if  (!bLast && b == TEXT('-'))
                {
                    /* Doesn't work.
                    bLast = b ;
                    b = IDC_SIGN ;
                    goto MappingDone ;
                    */
                    bNeedIDC_SIGN = TRUE ;
                    continue ;
                }

                /* check for 'x' */
                if  ((b == TEXT('x') || b == TEXT('e')) && nRadix == 10)
                {
                    bLast = TEXT('x') ;
                    b = IDC_EXP ;
                    goto MappingDone ;
                }

                /* if the last character was a 'x' & this is '+' - ignore */
                if  (bLast==TEXT('x') && b ==TEXT('+') && nRadix == 10)
                    continue ;

                /* if the last character was a 'x' & this is '-' - change
                it to be the code for SIGN */
                if  (bLast==TEXT('x') && b==TEXT('-') && nRadix == 10)
                {
                    bLast = b ;
                    b = IDC_SIGN ;
                    goto MappingDone ;
                }

/* -by- AmitC   */
/*--------------------------------------------------------------------------*/


                /* Check for control character.                           */
                if (bLast==TEXT(':'))
                    nControl=128;
                else
                    nControl=0;

                bLast=b;
                if (b==TEXT(':'))
                    continue;

                b=toupper(b)+nControl;

                nx=0;
                while (b!=rgbMap[nx*2] && nx < CHARSCAN)
                    nx++;

                if (nx==CHARSCAN)
                    break;

                b=(WORD)rgbMap[(nx*2)+1];

                if (nRadix != 10)
                {
                    switch(b)
                    {
                        case IDC_DEG:
                        case IDC_RAD:
                        case IDC_GRAD:
                            b=IDC_DWORD+(b-IDC_DEG);
                        break;
                    }
                }
                        
                // REVIEW NOTE: 
                //   Conversion of IDC_MOD to IDC_PERCENT done in WM_COMMAND
                //   processing so that keyboard accelerator and paste are
                //   handled in the same place.  The old conversion was broken
                //   anyway and actually happened in

MappingDone:
                /* Send the message to the window.                        */
                SendMessage(g_hwndDlg, WM_COMMAND, GET_WM_COMMAND_MPS(b, 0, 1));
                /* Note that we may need to apply the "+/-" key (IDC_SIGN)
                   now.  (If it had been applied earlier, it would have
                   been ignored.)  Note further that it can't be applied if we
                   have seen only the "-0" of something like "-0.1". */
                if(bNeedIDC_SIGN && (IDC_0 != b))
                    {
                    SendMessage(g_hwndDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDC_SIGN, 0, 1));
                    bNeedIDC_SIGN = FALSE;
                    }
            }
            GlobalUnlock(hClipData);
            CloseClipboard();
            break;
        }

        case IDM_ABOUT:
            /* Start the About Box.                                       */
            if(ShellAbout(g_hwndDlg, rgpsz[IDS_CALC], NULL, LoadIcon(hInst, (LPTSTR)TEXT("SC"))) == -1)
                MemErrorMessage();

            break;

        case IDM_SC:
        case IDM_SSC:
        {
            INT     nTemp;
            TCHAR   szWinIni[2];

            nTemp = (INT) nFunc - IDM_SC;
            if (nCalc != nTemp)
            {
                szWinIni[0] = TEXT('0') + nTemp;
                szWinIni[1]=0;
                WriteProfileString(szAppName, TEXT("layout"), szWinIni);

                if (hStatBox && !nCalc)
                    SetStat(FALSE);

                nCalc = nTemp;
                InitSciCalc(TRUE);
            }
            break;
        }

        case IDM_USE_SEPARATOR:
        {
            gbUseSep = !gbUseSep;

            CheckMenuItem(g_hDecMenu, IDM_USE_SEPARATOR,
                          MF_BYCOMMAND|(gbUseSep ? MF_CHECKED : MF_UNCHECKED));

            if (g_hHexMenu)
            {
                CheckMenuItem(g_hHexMenu, IDM_USE_SEPARATOR,
                              MF_BYCOMMAND | \
                              (gbUseSep ? MF_CHECKED:MF_UNCHECKED));
            }

            WriteProfileString(szAppName,TEXT("UseSep"),
                               (gbUseSep ? TEXT("1") : TEXT("0")));

            break;
        }

        case IDM_HELPTOPICS:
            HtmlHelp(GetDesktopWindow(), rgpsz[IDS_CHMHELPFILE], HH_DISPLAY_TOPIC, 0L);
            break;
    }

    return;
}
