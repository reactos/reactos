/****************************Module*Header***********************************\
* Module Name: SCICOMM.C
*
* Module Descripton:
*
* Warnings:
*
* Created:
*
* Author:
\****************************************************************************/

#include "scicalc.h"
#include "calchelp.h"
#include "unifunc.h"
#include "input.h"

extern HWND        hStatBox;
extern HNUMOBJ     ghnoNum, ghnoLastNum, ghnoMem;
extern HNUMOBJ     ghnoParNum[25], ghnoPrecNum[25];

extern eNUMOBJ_FMT nFE;
extern INT         nTempCom, nParNum, nPrecNum, gcIntDigits,
                   nOpCode, nOp[25], nPrecOp[25];
extern BOOL        bError;
extern TCHAR       szBlank[6];
extern TCHAR      *rgpsz[CSTRINGS];


int             nLastCom;   // Last command entered.
CALCINPUTOBJ    gcio;       // Global calc input object for decimal strings
BOOL            gbRecord;   // Global mode: recording or displaying


/* Puts up the wait cursor if the calc will take a long time */
HCURSOR ghcurOld = NULL;

BOOL SetWaitCursor( BOOL fOn ) {
    if (fOn && ghcurOld == NULL) {
        ghcurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );
    } else if (!fOn && ghcurOld != NULL) {
        SetCursor( ghcurOld );
        ghcurOld = NULL;
    }

    return (fOn && ghcurOld != NULL);
}

/* Process all keyclicks whether by mouse or accelerator.                 */
VOID NEAR RealProcessCommands(WPARAM wParam);

VOID NEAR ProcessCommands(WPARAM wParam)
{
    if (wParam != IDM_ABOUT)
    {
        TimeCalc(TRUE);
    }

    try
    {
        RealProcessCommands( wParam );
    }
    catch( ... )
    {
        // note:  it should be impossible for a throw to reach this level, this is put here as an
        // emergency backup only.  Throws are normally caught at the boundry between calc and ratpak.
        ASSERT( 0 );
        MessageBox( g_hwndDlg, TEXT("An unknown error has occured."), TEXT("Error"), MB_OK );
    }

    if (wParam != IDM_ABOUT)
    {
        TimeCalc(FALSE);
    }
}

VOID NEAR RealProcessCommands(WPARAM wParam)
{
    static BOOL    bNoPrevEqu=TRUE, /* Flag for previous equals.          */
                   bChangeOp=FALSE; /* Flag for changing operation.       */
    INT            nx, ni;
    TCHAR          szJunk[50], szTemp[50];
    static BYTE    rgbPrec[24]={      0,0,  IDC_OR,0, IDC_XOR,0,  IDC_AND,1, 
                                IDC_ADD,2, IDC_SUB,2,    RSHF,3, IDC_LSHF,3,
                                IDC_MOD,3, IDC_DIV,3, IDC_MUL,3,  IDC_PWR,4};

    // Make sure we're only getting commands we understand.

    ASSERT( xwParam(IDC_FIRSTCONTROL, IDC_LASTCONTROL) || // Is it a button?
            xwParam(IDM_FIRSTMENU,    IDM_LASTMENU) );    // or a menu command?

    // Save the last command.  Some commands are not saved in this manor, these
    // commands are:
    // Inv, Hyp, Deg, Rad, Grad, Stat, FE, MClear, Back, and Exp.  The excluded
    // commands are not
    // really mathematical operations, rather they are GUI mode settings.

    if ( !xwParam(IDC_INV, IDC_HYP)    && !xwParam(IDM_HEX, IDM_BIN)  &&
         !xwParam(IDM_QWORD, IDM_BYTE) && !xwParam(IDM_DEG, IDM_GRAD) &&
         wParam!=IDC_STAT && wParam!=IDC_FE &&
         wParam!=IDC_MCLEAR && wParam!=IDC_BACK && wParam!=IDC_EXP)
    {
        nLastCom=nTempCom;
        nTempCom=(INT)wParam;
    }

    // If error and not a clear key or help key, BEEP.

    if (bError && (wParam !=IDC_CLEAR) && (wParam !=IDC_CENTR) &&
        (wParam != IDM_HELPTOPICS))
    {
        MessageBeep(0);
        return;
    }

    // Toggle Record/Display mode if appropriate.

    if (gbRecord)
    {
        if (xwParam(IDC_AND, IDC_MPLUS)        ||
            xwParam(IDC_AVE, IDC_CLOSEP)       ||
            xwParam(IDC_INV, IDC_HYP)          ||
            xwParam(IDM_HEX, IDM_BIN)          ||
            xwParam(IDM_QWORD, IDM_BYTE)       ||
            xwParam(IDM_DEG, IDM_GRAD)         ||
            wParam == IDM_PASTE)
        {
            gbRecord = FALSE;
            SetWaitCursor(TRUE);
            CIO_vConvertToNumObj(&ghnoNum, &gcio);
            DisplayNum();   // Causes 3.000 to shrink to 3. on first op.
            SetWaitCursor(FALSE);
        }
    }
    else
    {
        if ( xwParam(IDC_0, IDC_F) || wParam == IDC_PNT)
        {
            gbRecord = TRUE;
            CIO_vClear(&gcio);
        }
    }

    // Interpret digit keys.

    if (xwParam(IDC_0, IDC_F))
    {
        int iValue = (int)(wParam-IDC_0);

        // this is redundant, illegal keys are disabled
        if (iValue >= nRadix)
        {
            //ASSERT( 0 );
            MessageBeep(0);
            return;
        }


        if (!CIO_bAddDigit(&gcio, iValue))
        {
            MessageBeep(0);
            return;
        }

        DisplayNum();
        return;
    }


    // STATISTICAL FUNCTIONS:
    if (xwParam(IDC_AVE,IDC_DATA))
    {
        /* Do statistics functions on data in fpStatNum array.        */
        if (hStatBox)
        {
            DisplayNum();       // Make sure gpszNum has the correct string
            try
            {
                StatFunctions (wParam);
            }
            catch ( ... )
            {
                ASSERT( 0 );    // the only thing stat box should be able to throw is out of memory
                        // which in previous versions of calc caused a program crash
            }
            if (!bError)
                DisplayNum ();
        }
        else
            /* Beep if the stat box is not active.                    */
            MessageBeep(0);

        /* Reset the inverse flag since some functions use it.        */
        SetBox (IDC_INV, bInv=FALSE);
        return;
    }


    // BINARY OPERATORS:
    if (xwParam(IDC_AND,IDC_PWR))
    {
        if (bInv && wParam==IDC_LSHF)
        {
            SetBox (IDC_INV, bInv=FALSE);
            wParam=RSHF;
        }

        /* Change the operation if last input was operation.          */
        if (nLastCom >=IDC_AND && nLastCom <=IDC_PWR)
        {
            nOpCode=(INT)wParam;
            return;
        }

        /* bChangeOp is true if there was an operation done and the   */
        /* current ghnoNum is the result of that operation.  This is so */
        /* entering 3+4+5= gives 7 after the first + and 12 after the */
        /* the =.  The rest of this stuff attempts to do precedence in*/
        /* Scientific mode.                                           */
        if (bChangeOp)
        {
        DoPrecedenceCheckAgain:

            nx=0;
            while (wParam!=rgbPrec[nx*2] && nx <12)
                nx++;

            ni=0;
            while (nOpCode!=rgbPrec[ni*2] && ni <12)
                ni++;

            if (nx==12) nx=0;
            if (ni==12) ni=0;

            if (rgbPrec[nx*2+1] > rgbPrec[ni*2+1] && nCalc==0)
            {
                if (nPrecNum <25)
                {
                    NumObjAssign( &ghnoPrecNum[nPrecNum], ghnoLastNum );
                    nPrecOp[nPrecNum]=nOpCode;
                }
                else
                {
                    nPrecNum=24;
                    MessageBeep(0);
                }
                nPrecNum++;
            }
            else
            {
                /* do the last operation and then if the precedence array is not
                 * empty or the top is not the '(' demarcator then pop the top
                 * of the array and recheck precedence against the new operator
                 */

                SetWaitCursor(TRUE);

                DoOperation(nOpCode, &ghnoNum, ghnoLastNum);

                SetWaitCursor(FALSE);

                if ((nPrecNum !=0) && (nPrecOp[nPrecNum-1]))
                {
                    nPrecNum--;
                    nOpCode=nPrecOp[nPrecNum] ;
                    if (NumObjOK( ghnoPrecNum[nPrecNum] ))
                        NumObjAssign(&ghnoLastNum , ghnoPrecNum[nPrecNum]);
                    else
                        NumObjAssign(&ghnoLastNum, HNO_ZERO);

                    goto DoPrecedenceCheckAgain ;
                }

                if (!bError)
                    DisplayNum ();
            }
        }

        NumObjAssign(&ghnoLastNum, ghnoNum);
        NumObjAssign(&ghnoNum, HNO_ZERO);
        nOpCode=(INT)wParam;
        bNoPrevEqu=bChangeOp=TRUE;
        return;
    }

    // UNARY OPERATORS:
    if (xwParam(IDC_CHOP,IDC_PERCENT))
    {
        /* Functions are unary operations.                            */

        /* If the last thing done was an operator, ghnoNum was cleared. */
        /* In that case we better use the number before the operator  */
        /* was entered, otherwise, things like 5+ 1/x give Divide By  */
        /* zero.  This way 5+=gives 10 like most calculators do.      */
        if (nLastCom >= IDC_AND && nLastCom <= IDC_PWR)
            NumObjAssign( &ghnoNum, ghnoLastNum );

        SetWaitCursor(TRUE);
        SciCalcFunctions ( &ghnoNum, (DWORD)wParam);
        SetWaitCursor(FALSE);

        if (bError)
            return;

        /* Display the result, reset flags, and reset indicators.     */
        DisplayNum ();

        /* reset the bInv and bHyp flags and indicators if they are set
            and have been used */

        if (bInv &&
            (wParam == IDC_CHOP || wParam == IDC_SIN || wParam == IDC_COS ||
             wParam == IDC_TAN  || wParam == IDC_SQR || wParam == IDC_CUB ||
             wParam == IDC_LOG  || wParam == IDC_LN  || wParam == IDC_DMS))
        {
            bInv=FALSE;
            SetBox (IDC_INV, FALSE);
        }

        if (bHyp &&
            (wParam == IDC_SIN || wParam == IDC_COS || wParam == IDC_TAN))
        {
            bHyp = FALSE;
            SetBox (IDC_HYP, FALSE);
        }
        bNoPrevEqu=TRUE;
        return;
    }

    // BASE CHANGES:
    if (xwParam(IDM_HEX, IDM_BIN))
    {
        // Change radix and update display.
        if (nCalc==1)
        {
            wParam=IDM_DEC;
        }

        SetRadix((DWORD)wParam);
        return;
    }

    SetWaitCursor(TRUE);

    /* Now branch off to do other commands and functions.                 */
    switch(wParam)
    {
        case IDM_COPY:
        case IDM_PASTE:
        case IDM_ABOUT:
        case IDM_SC:
        case IDM_SSC:
        case IDM_USE_SEPARATOR:
        case IDM_HELPTOPICS:
            // Jump to menu command handler in scimenu.c.
            MenuFunctions((DWORD)wParam);
            DisplayNum();
            break;

        case IDC_CLEAR: /* Total clear.                                       */
            NumObjAssign( &ghnoLastNum, HNO_ZERO );
            nPrecNum=nTempCom=nLastCom=nOpCode=nParNum=bChangeOp=FALSE;
            nFE = FMT_FLOAT;    // back to the default number format
            bNoPrevEqu=TRUE;

            /* clear the paranthesis status box indicator, this will not be
                cleared for CENTR */

            SetDlgItemText(g_hwndDlg, IDC_PARTEXT, szBlank);

            /* fall through */

        case IDC_CENTR: /* Clear only temporary values.                       */
            NumObjAssign( &ghnoNum, HNO_ZERO );

            if (!nCalc)
            {
                // Clear the INV, HYP indicators & leave (=xx indicator active

                SetBox (IDC_INV, bInv=FALSE);
                SetBox (IDC_HYP, bHyp=FALSE);
            }

            bError=FALSE;
            CIO_vClear(&gcio);
            gbRecord = TRUE;
            DisplayNum ();
            break;

        case IDC_STAT: /* Shift focus to Statistix Box if it's active.       */
            if (hStatBox)
                SetFocus(hStatBox);
            else
                SetStat (TRUE);
            break;

        case IDC_BACK:
            // Divide number by the current radix and truncate.
            // Only allow backspace if we're recording.
            if (gbRecord)
            {
                if (!CIO_bBackspace(&gcio))
                    MessageBeep(0);

                DisplayNum();
            }
            else
                MessageBeep(0);
            break;

        /* EQU enables the user to press it multiple times after and      */
        /* operation to enable repeats of the last operation.  I don't    */
        /* know if I can explain what the hell I did here...              */
        case IDC_EQU:
            do {
                // BUGBUG: a static HNUMOBJ will cause a memory leak as it won't get freed
                static HNUMOBJ  hnoHold;

                /* Last thing keyed in was an operator.  Lets do the op on*/
                /* a duplicate of the last entry.                         */
                if ((nLastCom >= IDC_AND) && (nLastCom <= IDC_PWR))
                    NumObjAssign( &ghnoNum, ghnoLastNum );

                if (nOpCode) /* Is there a valid operation around?        */
                {
                    /* If this is the first EQU in a string, set hnoHold=ghnoNum */
                    /* Otherwise let ghnoNum=hnoTemp.  This keeps ghnoNum constant */
                    /* through all EQUs in a row.                         */
                    if (bNoPrevEqu)
                        NumObjAssign(&hnoHold, ghnoNum);
                    else
                        NumObjAssign(&ghnoNum, hnoHold);

                    /* Do the current or last operation.                  */
                    DoOperation (nOpCode, &ghnoNum, ghnoLastNum);
                    NumObjAssign(&ghnoLastNum, ghnoNum );

                    /* Check for errors.  If this wasn't done, DisplayNum */
                    /* would immediately overwrite any error message.     */
                    if (!bError)
                        DisplayNum ();

                    /* No longer the first EQU.                           */
                    bNoPrevEqu=FALSE;
                }
                else if (!bError)
                    DisplayNum();

                if (nPrecNum==0 || nCalc==1)
                    break;

                nOpCode=nPrecOp[--nPrecNum];
                if (NumObjOK( ghnoPrecNum[nPrecNum] ))
                    NumObjAssign(&ghnoLastNum , ghnoPrecNum[nPrecNum]);
                else
                    NumObjAssign(&ghnoLastNum, HNO_ZERO);
                bNoPrevEqu=TRUE;
            } while (nPrecNum >= 0);

            bChangeOp=FALSE;
            break;


        case IDC_OPENP:
        case IDC_CLOSEP:
            nx=0;
            if (wParam==IDC_OPENP)
                nx=1;

            // -IF- the Paren holding array is full and we try to add a paren
            // -OR- the paren holding array is empty and we try to remove a
            //      paren
            // -OR- the the precidence holding array is full
            if ((nParNum >= 25 && nx) || (!nParNum && !nx)
                || ( (nPrecNum >= 25 && nPrecOp[nPrecNum-1]!=0) ) )
            {
                MessageBeep(0);
                break;
            }

            if (nx)
            {
                /* Open level of parentheses, save number and operation.   */
                NumObjAssign( &ghnoParNum[nParNum], ghnoLastNum);
                nOp[nParNum++]=nOpCode;

                /* save a special marker on the precedence array */
                nPrecOp[nPrecNum++]=0 ;

                NumObjAssign( &ghnoLastNum, HNO_ZERO );
                nTempCom=0;
                nOpCode=IDC_ADD;
            }
            else
            {
                /* Get the operation and number and return result.         */
                DoOperation (nOpCode, &ghnoNum, ghnoLastNum);

                /* now process the precedence stack till we get to an
                    opcode which is zero. */

                while (nOpCode = nPrecOp[--nPrecNum])
                {
                    if (NumObjOK( ghnoPrecNum[nPrecNum] ))
                        NumObjAssign(&ghnoLastNum , ghnoPrecNum[nPrecNum]);
                    else
                        NumObjAssign(&ghnoLastNum, HNO_ZERO);

                    DoOperation (nOpCode, &ghnoNum, ghnoLastNum);
                }

                /* now get back the operation and opcode at the begining
                    of this paranthesis pair */

                nParNum -= 1;
                NumObjAssign( &ghnoLastNum, ghnoParNum[nParNum] );
                nOpCode=nOp[nParNum];

                /* if nOpCode is a valid operator then set bChangeOp to
                    be true else set it false */

                if  (nOpCode)
                    bChangeOp=TRUE;
                else
                    bChangeOp=FALSE ;
            }

            /* Set the "(=xx" indicator.                     */
            lstrcpy(szJunk, TEXT("(="));
            lstrcat(szJunk, UToDecT(nParNum, szTemp));
            SetDlgItemText(g_hwndDlg, IDC_PARTEXT,
                           (nParNum) ? (szJunk) : (szBlank));

            if (bError)
                break;

            if (nx)
            {
                /* Build a display string of nParNum "("'s.  */
                for (nx=0; nx < nParNum; nx++)
                    szJunk[nx]=TEXT('(');

                szJunk[nx]=0; /* Null-terminate.  */
                SetDlgItemText(g_hwndDlg, IDC_DISPLAY, szJunk);
                bChangeOp=FALSE;
            }
            else
                DisplayNum ();
            break;

        case IDM_QWORD:
        case IDM_DWORD:
        case IDM_WORD:
        case IDM_BYTE:
        case IDM_DEG:
        case IDM_RAD:
        case IDM_GRAD:

            if (!F_INTMATH())
            {
                // in decimal mode, these buttons simply set a flag which is
                // passed to the ratpak to handle angle conversions

                if (xwParam(IDM_DEG, IDM_GRAD))
                {
                    nDecMode = (ANGLE_TYPE)(wParam - IDM_DEG);

                    CheckMenuRadioItem(GetSubMenu(GetMenu(g_hwndDlg), 1),
                                       IDM_DEG, IDM_GRAD, IDM_DEG+nDecMode,
                                       MF_BYCOMMAND);
                
                    CheckRadioButton(g_hwndDlg, IDC_DEG, IDC_GRAD, 
                                     IDC_DEG+nDecMode);
                }
            }
            else
            {
                if (xwParam(IDM_DEG, IDM_GRAD))
                {
                    // if in hex mode, but we got a decimal key press this
                    // likely is the accelorator.  map this to the correct key

                    wParam=IDM_DWORD+(wParam-IDM_DEG);
                }
                
                if ( gbRecord )
                {
                    CIO_vConvertToNumObj(&ghnoNum, &gcio);
                    gbRecord = FALSE;
                }

                // Compat. mode BaseX: Qword, Dword, Word, Byte
                nHexMode = (int)(wParam - IDM_QWORD);
                switch (nHexMode)
                {
                    case 0: dwWordBitWidth = 64; break;
                    case 1: dwWordBitWidth = 32; break;
                    case 2: dwWordBitWidth = 16; break;
                    case 3: dwWordBitWidth =  8; break;
                    default:
                        ASSERT( 0 );    // Invalid Word Size
                        break;
                }

                // different wordsize means the new wordsize determines
                // the precision

                BaseOrPrecisionChanged();

                CheckMenuRadioItem(GetSubMenu(GetMenu(g_hwndDlg), 1),
                                   IDM_QWORD, IDM_BYTE, IDM_QWORD+nHexMode,
                                   MF_BYCOMMAND);

                CheckRadioButton(g_hwndDlg, IDC_QWORD, IDC_BYTE, 
                                 IDC_QWORD+nHexMode);
               
            }


            // BUGBUG: the call to display number is what actually does the
            // chop. it would make more sense to do the chop here when the
            // wordsize changes. the chop must be done when a different
            // wordsize is selected AND when the base is changed to non-decimal
            DisplayNum();
            break;

        case IDC_SIGN:
            // Change the sign.
            if (gbRecord)
                CIO_vToggleSign(&gcio);
            else {
                NumObjNegate( &ghnoNum );
            }

            DisplayNum();
            break;

        case IDC_RECALL:
            /* Recall immediate memory value.                             */
            NumObjAssign( &ghnoNum, ghnoMem );

            DisplayNum ();
            break;

        case IDC_MPLUS:
            /* MPLUS adds ghnoNum to immediate memory and kills the "mem"   */
            /* indicator if the result is zero.                           */
            addrat( &ghnoMem, ghnoNum);
            SetDlgItemText(g_hwndDlg,IDC_MEMTEXT,
                           !NumObjIsZero(ghnoMem) ? (TEXT(" M")):(szBlank));
            break;

        case IDC_STORE:
        case IDC_MCLEAR:
            if (wParam==IDC_STORE)
            {
                NumObjAssign( &ghnoMem, ghnoNum );
            }
            else
            {
                NumObjAssign( &ghnoMem, HNO_ZERO );
            }
            SetDlgItemText(g_hwndDlg,IDC_MEMTEXT,
                           !NumObjIsZero(ghnoMem) ? (TEXT(" M")):(szBlank));
            break;

        case IDC_PI:
            if (!F_INTMATH())
            {
                /* Return PI if bInv==FALSE, or 2PI if bInv==TRUE.          */
                if (bInv)
                    NumObjAssign( &ghnoNum, HNO_2PI );
                else
                    NumObjAssign( &ghnoNum, HNO_PI );

                DisplayNum();
                SetBox(IDC_INV, bInv=FALSE);
            }
            else
                MessageBeep(0);
            break;

        case IDC_FE:
            // Toggle exponential notation display.
            nFE = NUMOBJ_FMT(!(int)nFE);
            DisplayNum();
            break;

        case IDC_EXP:
            if (gbRecord && !F_INTMATH())
                if (CIO_bExponent(&gcio))
                {
                    DisplayNum();
                    break;
                }
            MessageBeep(0);
            break;

        case IDC_PNT:
            if (gbRecord && !F_INTMATH()) {
                if (CIO_bAddDecimalPt(&gcio)) {

                    DisplayNum();
                    break;
                }
            }
            MessageBeep(0);
            break;

        case IDC_INV:
            SetBox((int)wParam, bInv=!bInv);
            break;

        case IDC_HYP:
            SetBox((int)wParam, bHyp=!bHyp);
            break;
    }

    SetWaitCursor(FALSE);
}
