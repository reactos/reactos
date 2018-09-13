/****************************Module*Header***********************************\
* Module Name: INPUT.C
*
* Module Descripton: Decimal floating point input
*
* Warnings:
*
* Created:
*
* Author:
\****************************************************************************/

#include "scicalc.h"
#include "unifunc.h"
#include "input.h"

extern BOOL     gbRecord;
extern TCHAR    szDec[5];       // The decimal point we use
extern INT gcIntDigits;

TCHAR const szZeroInit[] = TEXT("0");

#define CH_BASE_10_EXP          TEXT('e')
#define CH_BASE_X_EXP           TEXT('^')

/****************************************************************************/
void CIO_vClearNSec( PCALCNUMSEC pcns ) {
    pcns->fEmpty = TRUE;
    pcns->fNeg = FALSE;
    lstrcpy( pcns->szVal, szZeroInit );
    pcns->cchVal = lstrlen(pcns->szVal);
}

void CIO_vClear(PCALCINPUTOBJ pcio)
{
    CIO_vClearNSec( &(pcio->cnsNum) );
    CIO_vClearNSec( &(pcio->cnsExp) );
    pcio->fExp = FALSE;
    pcio->iDecPt = -1;
}

/****************************************************************************/

void CIO_vConvertToNumObj(PHNUMOBJ phnoNum, PCALCINPUTOBJ pcio)
{
    HNUMOBJ hnoValue;
    LPTSTR pszExp = NULL;

    // ZTerm the strings
    pcio->cnsNum.szVal[pcio->cnsNum.cchVal] = TEXT('\0');

    if (pcio->fExp ) {
        pszExp = pcio->cnsExp.szVal;
        pszExp[pcio->cnsExp.cchVal] = TEXT('\0');
    }

    hnoValue = NumObjMakeNumber( pcio->cnsNum.fNeg, pcio->cnsNum.szVal,  pcio->cnsExp.fNeg, pszExp );
    NumObjAssign( phnoNum, hnoValue );

    return;
}

/****************************************************************************/

void CIO_vConvertToString(LPTSTR *ppszOut, PCALCINPUTOBJ pcio, int nRadix)
{
    //In theory both the base and exponent could be C_NUM_MAX_DIGITS long.
    TCHAR szTemp[C_NUM_MAX_DIGITS*2+4];
    LPTSTR psz;
    int i;

    // ZTerm the strings
    pcio->cnsNum.szVal[pcio->cnsNum.cchVal] = TEXT('\0');

    if ( pcio->fExp )
        pcio->cnsExp.szVal[pcio->cnsExp.cchVal] = TEXT('\0');

    i = 0;
    if (pcio->cnsNum.fNeg)
        szTemp[i++] = TEXT('-');

    lstrcpy( &szTemp[i], pcio->cnsNum.szVal );
    i += pcio->cnsNum.cchVal;

    // Add a '.' if it is not already there
    if (pcio->iDecPt == -1 )
        szTemp[i++] = szDec[0];

    if (pcio->fExp) {
        szTemp[i++] = nRadix == 10 ? CH_BASE_10_EXP : CH_BASE_X_EXP;

        if (pcio->cnsExp.fNeg)
            szTemp[i++] = TEXT('-');
        else
            szTemp[i++] = TEXT('+');

        lstrcpy( &szTemp[i], pcio->cnsExp.szVal );
        i += pcio->cnsExp.cchVal;
    }

    psz = (LPTSTR)NumObjAllocMem( (lstrlen( szTemp )+1) * sizeof(TCHAR) );
    if (psz) {
        if (*ppszOut != NULL) {
            NumObjFreeMem( *ppszOut );
        }
        *ppszOut = psz;
    }

    // Don't show '.' if in int math
    if (F_INTMATH() && szTemp[i-1] == szDec[0])
        i--;

    szTemp[i] = TEXT('\0');

    lstrcpy( *ppszOut, szTemp );

    return;
}

/****************************************************************************/

BOOL CIO_bAddDigit(PCALCINPUTOBJ pcio, int iValue)
{
    PCALCNUMSEC pcns;
    TCHAR chDigit;
    int cchMaxDigits;

    // convert from an integer into a character
    chDigit = (iValue < 10)?(TEXT('0')+iValue):(TEXT('A')+iValue-10);

    if (pcio->fExp)
    {
        pcns = &(pcio->cnsExp);
        cchMaxDigits = C_EXP_MAX_DIGITS;
    }
    else
    {
        pcns = &(pcio->cnsNum);
        ASSERT( gcIntDigits <= C_NUM_MAX_DIGITS );
        cchMaxDigits = gcIntDigits;
    }

    // Ignore leading zeros
    if ( pcns->fEmpty && (iValue == 0) )
    {
        return TRUE;
    }

    if ( pcns->cchVal < cchMaxDigits )
    {
        if (pcns->fEmpty)
        {
            pcns->cchVal = 0;   // Clobber the default zero
            pcns->fEmpty = FALSE;
        }

        pcns->szVal[pcns->cchVal++] = chDigit;
        return TRUE;
    }

    // if we are in base 8 entering a mantica and we're on the last digit then
    // there are special cases where we can actually add one more digit.
    if ( nRadix == 8 && pcns->cchVal == cchMaxDigits && !pcio->fExp )
    {
        BOOL bAllowExtraDigit = FALSE;

        switch ( dwWordBitWidth % 3 )
        {
            case 1:
                // in 16bit word size, if the first digit is a 1 we can enter 6 digits
                if ( pcns->szVal[0] == TEXT('1') )
                    bAllowExtraDigit = TRUE;
                break;

            case 2:
                // in 8 or 32bit word size we get an extra digit if the first digit is 3 or less
                if ( pcns->szVal[0] <= TEXT('3') )
                    bAllowExtraDigit = TRUE;
                break;
        }

        if ( bAllowExtraDigit )
        {
            pcns->szVal[pcns->cchVal++] = chDigit;
            return TRUE;
        }
    }

    return FALSE;
}

/****************************************************************************/

void CIO_vToggleSign(PCALCINPUTOBJ pcio)
{

    // Zero is always positive
    if (pcio->cnsNum.fEmpty)
    {
        pcio->cnsNum.fNeg = FALSE;
        pcio->cnsExp.fNeg = FALSE;
    }
    else if (pcio->fExp)
    {
        pcio->cnsExp.fNeg = !pcio->cnsExp.fNeg;
    }
    else
    {
        pcio->cnsNum.fNeg = !pcio->cnsNum.fNeg;
    }
}

/****************************************************************************/

BOOL CIO_bAddDecimalPt(PCALCINPUTOBJ pcio)
{
    ASSERT(gbRecord == TRUE);

    if (pcio->iDecPt != -1)                      // Already have a decimal pt
        return FALSE;

    if (pcio->fExp)                             // Entering exponent
        return FALSE;

    pcio->cnsNum.fEmpty = FALSE;                // Zeros become significant

    pcio->iDecPt = pcio->cnsNum.cchVal++;
    pcio->cnsNum.szVal[pcio->iDecPt] = szDec[0];

    return TRUE;
}

/****************************************************************************/

BOOL CIO_bExponent(PCALCINPUTOBJ pcio)
{
    ASSERT(gbRecord == TRUE);

    // For compatability, add a trailing dec pnt to base num if it doesn't have one
    CIO_bAddDecimalPt( pcio );

    if (pcio->fExp)                             // Already entering exponent
        return FALSE;

    pcio->fExp = TRUE;                          // Entering exponent

    return TRUE;
}

/****************************************************************************/

BOOL CIO_bBackspace(PCALCINPUTOBJ pcio)
{
    ASSERT(gbRecord == TRUE);

    if (pcio->fExp)
    {
        if ( !(pcio->cnsExp.fEmpty) )
        {
            pcio->cnsExp.cchVal--;

            if (pcio->cnsExp.cchVal == 0)
            {
                CIO_vClearNSec( &(pcio->cnsExp) );
            }
        }
        else
        {
            pcio->fExp = FALSE;
        }
    }
    else
    {
        if ( !(pcio->cnsNum.fEmpty) )
        {
            pcio->cnsNum.cchVal--;
        }

        if ( pcio->cnsNum.cchVal <= pcio->iDecPt )
            //Backed up over decimal point
            pcio->iDecPt = -1;

        if ((pcio->cnsNum.cchVal == 0) || ((pcio->cnsNum.cchVal == 1) && (pcio->cnsNum.szVal[0] == TEXT('0'))))
            CIO_vClearNSec( &(pcio->cnsNum) );
    }

    return TRUE;
}

/****************************************************************************/

void CIO_vUpdateDecimalSymbol(PCALCINPUTOBJ pcio, TCHAR chLastDP)
{
    int iDP;

    ASSERT(pcio);

    iDP = pcio->iDecPt;                            // Find the DP index

    if (iDP == -1)
        return;

    ASSERT(pcio->cnsNum.szVal[iDP] == chLastDP);

    pcio->cnsNum.szVal[iDP] = szDec[0];                   // Change to new decimal pt
}

/****************************************************************************/
