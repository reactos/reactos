/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scioper.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    DoOperation--Does common operations.                            ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    DisplayError                                                    ***/
/***                                                                    ***/
/*** Last modification Thu  31-Aug-1989                                 ***/
/**************************************************************************/

#include "scicalc.h"

extern BOOL        bInv;
extern LONG        nPrecision;


/****************************************************************************\
* HNUMOBJ NEAR DoOperation (short nOperation, HNUMOBJ fpx)
*
* Routines to perform standard operations &|^~<<>>+-/*% and pwr.
*
\****************************************************************************/

void DoOperation (INT nOperation, HNUMOBJ *phnoNum, HNUMOBJ hnoX)
{
    // this really just sets a pointer to NULL, no allocation
    // DECLARE_HNUMOBJ( hno );
    //  BUGBUG volatile is used here because of a compiler bug! vc 5 AND 6...
    volatile PRAT hno = NULL;

    try
    {
        switch (nOperation)
        {
        /* Buncha ops.  Hope *this* doesn't confuse anyone <smirk>.       */
        case IDC_AND:
            andrat( phnoNum, hnoX );
            return;

        case IDC_OR:
            orrat( phnoNum, hnoX );
            return;

        case IDC_XOR:
            xorrat( phnoNum, hnoX );
            return;

        case RSHF:
            NumObjAssign( &hno, *phnoNum );
            NumObjAssign( phnoNum, hnoX );

            rshrat( phnoNum, hno );
            break;

        case IDC_LSHF:
            NumObjAssign( &hno, *phnoNum );
            NumObjAssign( phnoNum, hnoX );

            lshrat( phnoNum, hno );
            break;

        case IDC_ADD:
            addrat( phnoNum, hnoX );
            return;

        case IDC_SUB:
            // in order to do ( hnoX - phnoNum ) we actually do -(phnoNum - hnoX ) cause it's quicker
            subrat( phnoNum, hnoX );
            NumObjNegate( phnoNum );
            return;

        case IDC_MUL:
            mulrat( phnoNum, hnoX );
            return;

        case IDC_DIV:
        case IDC_MOD:
            {
                // REVIEW:  These lengthly number assignments can be replaced with some quick pointer swaps.
                // the swaps cannot change the value of hnoX unless we also modify the code that calls
                // the DoOperation function.
                NumObjAssign( &hno, *phnoNum );
                NumObjAssign( phnoNum, hnoX );

                if (nOperation==IDC_DIV) {
                    divrat(phnoNum, hno );   /* Do division.                       */
                } else {
                    modrat( phnoNum, hno );
                }

                break;
            }

        case IDC_PWR:       /* Calculates hnoX to the hnoNum(th) power or root.   */
            {
                NumObjAssign( &hno, *phnoNum );
                NumObjAssign( phnoNum, hnoX );

                if (bInv)   /* Switch for hnoNum(th) root. Null root illegal.    */
                {
                    SetBox (IDC_INV, bInv=FALSE);
                    rootrat( phnoNum, hno);        /* Root.                           */
                }
                else 
                {
                    powrat( phnoNum, hno );    /* Power.                          */
                }

                break;
            }
        }

        if ( hno != NULL )
            NumObjDestroy( &hno );
    }
    catch ( DWORD dwErrCode )
    {
        // if ratpak throws an error, we may need to free the memory used by hno
        if ( hno != NULL )
            NumObjDestroy( &hno );

        DisplayError( dwErrCode );
    }

    return;
}
