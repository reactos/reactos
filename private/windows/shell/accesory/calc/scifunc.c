/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scifunc.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    SciCalcFunctions--do sin, cos, tan, com, log, ln, rec, fac, etc.***/
/***    DisplayError--Error display driver.                             ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    SciCalcFunctions call DisplayError.                             ***/
/***                                                                    ***/
/*** Last modification. Fri  05-Jan-1990.                               ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee. [amitc]  05-Jan-1990.                                                      ***/
/*** Calc did not have a floating point exception signal handler. This  ***/
/*** would cause CALC to be forced to exit on a FP exception as that's  ***/
/*** the default.                                                                                                                                                  ***/
/*** The signal handler is defined in here, in SCIMAIN.C we hook the    ***/
/*** the signal.                                                                                                                                    ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee. [amitc] 14-Dec-1989                                                   ***/
/*** The REC function will not depend on the bInv flag. It used to ret  ***/
/*** a random number when the bInv flag was set.                                                 ***/
/***                                                                    ***/
/*** -by- Amit Chatterjee.      [amitc] 08-Dec-1989                                                   ***/
/*** Did a minor bug fix. The EnableToggles routine now sets the focus  ***/
/*** back to the main window before disabling HEX,DEC etc.. Without this***/
/*** the window with the focus would get disable and cause MOVE to not  ***/
/*** work right.                                                                                                                ***/
/***                                                                    ***/
/**************************************************************************/

#include "scicalc.h"
//#include "float.h"

extern HNUMOBJ     ghnoLastNum;
extern BOOL        bError;
extern TCHAR       *rgpsz[CSTRINGS];
INT                gnPendingError ;

/* Routines for more complex mathematical functions/error checking.       */

VOID  APIENTRY SciCalcFunctions (PHNUMOBJ phnoNum, DWORD wOp)
{
    try
    {
        switch (wOp)
        {
            case IDC_CHOP:
                if (bInv)
                {
                    // fractional portion
                    fracrat( phnoNum );
                }
                else
                {
                    // integer portion
                    intrat( phnoNum );
                }
                return;

            /* Return complement.                                             */
            case IDC_COM:
                NumObjNot( phnoNum );
                return;


            case IDC_PERCENT:
                {
                    DECLARE_HNUMOBJ( hno );
                    DECLARE_HNUMOBJ( hno100 );

                    try
                    {
                        NumObjAssign( &hno, ghnoLastNum );
                        NumObjSetIntValue( &hno100, 100 );

                        divrat( &hno, hno100 );

                        NumObjDestroy( &hno100 );

                        mulrat( phnoNum, hno );

                        NumObjDestroy( &hno );
                    }
                    catch ( DWORD nErrCode )
                    {
                        if ( hno != NULL )
                            NumObjDestroy( &hno );
                        if ( hno100 != NULL ) 
                            NumObjDestroy( &hno100 );
                        throw nErrCode;
                    }
                    return;
                }

            case IDC_SIN: /* Sine; normal, hyperbolic, arc, and archyperbolic     */
                if (F_INTMATH())
                {
                    MessageBeep(0);
                    return;
                }

                if(bInv)
                {
                    if (bHyp)
                    {
                        asinhrat( phnoNum );
                    }
                    else
                    {
                        asinanglerat( phnoNum, nDecMode );
                    }
                }
                else
                {
                    if (bHyp)
                    {
                        // hyperbolic sine
                        sinhrat( phnoNum );
                    }
                    else
                    {
                        NumObjSin( phnoNum );
                    }
                }
                return;

            case IDC_COS: /* Cosine, follows convention of sine function.         */
                if (F_INTMATH())
                {
                    MessageBeep(0);
                    return;
                }

                if(bInv)
                {
                    if (bHyp)
                    {
                        acoshrat( phnoNum );
                    }
                    else
                    {
                        acosanglerat( phnoNum, nDecMode );
                    }
                }
                else
                {
                    if (bHyp)
                        coshrat( phnoNum );
                    else
                    {
                        // cos()
                        NumObjCos( phnoNum );
                    }
                }
                return;

            case IDC_TAN: /* Same as sine and cosine.                             */
                if (F_INTMATH())
                {
                    MessageBeep(0);
                    return;
                }

                if(bInv)
                {
                    if (bHyp)
                    {
                        atanhrat( phnoNum );
                    }
                    else
                    {
                        atananglerat( phnoNum, nDecMode );
                    }
                }
                else
                {
                    if (bHyp)
                        tanhrat( phnoNum );
                    else
                    {
                        // Get the answer
                        NumObjTan( phnoNum );
                    }
                }
                return;

            case IDC_REC: /* Reciprocal.                                          */
                NumObjInvert( phnoNum );
                return;

            case IDC_SQR: /* Square and square root.                              */
            case IDC_SQRT:
                if(bInv || nCalc)
                {
                    rootrat( phnoNum, HNO_TWO );
                }
                else
                {
                    ratpowlong( phnoNum, 2 );
                }
                return;

            case IDC_CUB: /* Cubing and cube root functions.                      */
                if(bInv) {
                    DECLARE_HNUMOBJ( hno );

                    // REVIEW: if constants like 3 are going to be used repeatedly, it will be
                    // much quicker to define them once and then keep around the definition.
                    try
                    {
                        NumObjAssign( &hno, HNO_ONE );
                        addrat( &hno, HNO_TWO );

                        rootrat( phnoNum, hno );

                        NumObjDestroy( &hno );
                    }
                    catch ( DWORD nErrCode )
                    {
                        if ( hno != NULL )
                            NumObjDestroy( &hno );

                        throw nErrCode;
                    }
                }
                else {
                    /* Cube it, you dig?       */
                    ratpowlong( phnoNum, 3 );
                }
                return;

            case IDC_LOG: /* Functions for common and natural log.                */
            case IDC_LN:
                if(bInv)
                {
                    /* Check maximum for exponentiation for 10ü and eü.       */
                    if (wOp==IDC_LOG) /* Do exponentiation.                       */
                        NumObjAntiLog10( phnoNum ); // 10ü.
                    else
                        exprat( phnoNum );  // eü.
                }
                else
                {
                    // ratpak checks for valid range and throws error code if needed
                    if (wOp==IDC_LOG)
                        log10rat( phnoNum );
                    else
                        lograt( phnoNum );

                    // REVIEW: Is conversion of epsilon still needed?
                    NumObjCvtEpsilonToZero( phnoNum );
                }
                return;

            case IDC_FAC: /* Calculate factorial.  Inverse is ineffective.        */
                factrat( phnoNum );
                return;

            case IDC_DMS:
                {
                    if (F_INTMATH())
                    {
                        MessageBeep(0);
                    } 
                    else 
                    {
                        DECLARE_HNUMOBJ(hnoMin);
                        DECLARE_HNUMOBJ(hnoSec);
                        DECLARE_HNUMOBJ(hnoShft);

                        try
                        {
                            NumObjSetIntValue( &hnoShft, bInv ? 100 : 60 );

                            NumObjAssign( &hnoMin, *phnoNum );
                            intrat( phnoNum );

                            subrat( &hnoMin, *phnoNum );
                            mulrat( &hnoMin, hnoShft );
                            NumObjAssign( &hnoSec, hnoMin );
                            intrat( &hnoMin );

                            subrat( &hnoSec, hnoMin );
                            mulrat( &hnoSec, hnoShft );

                            //
                            // *phnoNum == degrees, hnoMin == minutes, hnoSec == seconds
                            //

                            NumObjSetIntValue( &hnoShft, bInv ? 60 : 100 );
                            divrat( &hnoSec, hnoShft );
                            addrat( &hnoMin, hnoSec );

                            divrat( &hnoMin, hnoShft );
                            addrat( phnoNum, hnoMin );

                            NumObjDestroy( &hnoShft );
                            NumObjDestroy( &hnoMin );
                            NumObjDestroy( &hnoSec );
                        }
                        catch ( DWORD nErrCode )
                        {
                            if ( hnoShft != NULL )
                                NumObjDestroy( &hnoShft );
                            if ( hnoMin != NULL )
                                NumObjDestroy( &hnoMin );
                            if ( hnoSec != NULL )
                                NumObjDestroy( &hnoSec );
                            throw nErrCode;
                        }
                    }
                    return;
                }
        }   // end switch( nOp )
    }
    catch( DWORD nErrCode )
    {
        DisplayError( nErrCode );
    }

    return;
}



/* Routine to display error messages and set bError flag.  Errors are     */
/* called with DisplayError (n), where n is a INT   between 0 and 5.      */

VOID  APIENTRY DisplayError (INT   nError)
{
    SetDlgItemText(g_hwndDlg, IDC_DISPLAY, rgpsz[IDS_ERRORS+nError]);
    bError=TRUE; /* Set error flag.  Only cleared with CLEAR or CENTR.    */

     /* save the pending error */
     gnPendingError = nError ;

    return;
}

