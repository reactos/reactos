/****************************Module*Header***********************************\
* Module Name: INPUT.H
*
* Module Descripton:
*
* Warnings:
*
* Created:
*
* Author:
\****************************************************************************/

// the string must hold, at a minimun, enough digits for a quadword binary number (ie 64)
#define MAX_STRLEN      64          // Seems to be the magic value for calc...

#define C_NUM_MAX_DIGITS    MAX_STRLEN
#define C_EXP_MAX_DIGITS    4

typedef struct
{
    BOOL    fEmpty;                 // TRUE if the number has no digits yet
    BOOL    fNeg;                   // TRUE if number is negative
    INT     cchVal;                 // number of characters in number (including dec. pnt)
    TCHAR   szVal[MAX_STRLEN+1];      //
} CALCNUMSEC, *PCALCNUMSEC;

#if C_NUM_MAX_DIGITS > MAX_STRLEN || C_EXP_MAX_DIGITS > MAX_STRLEN
#   pragma error(CALCNUMSEC.szVal is too small)
#endif

typedef struct
{
    BOOL    fExp;                   // TRUE if number has exponent
    INT     iDecPt;                 // index to decimal point of number portion.  -1 if no dec pnt
    CALCNUMSEC cnsNum;              // base number
    CALCNUMSEC cnsExp;              // exponent if it exists
} CALCINPUTOBJ, *PCALCINPUTOBJ;

#define CIO_bDecimalPt(pcio)    ((pcio)->iDecPt != -1)

void CIO_vClear(PCALCINPUTOBJ pcio);
BOOL CIO_bAddDigit(PCALCINPUTOBJ pcio, int iValue);
void CIO_vToggleSign(PCALCINPUTOBJ pcio);
BOOL CIO_bAddDecimalPt(PCALCINPUTOBJ pcio);
BOOL CIO_bExponent(PCALCINPUTOBJ pcio);
BOOL CIO_bBackspace(PCALCINPUTOBJ pcio);
void CIO_vUpdateDecimalSymbol(PCALCINPUTOBJ pcio, TCHAR chLastDP);
void CIO_vConvertToString(LPTSTR *ppszOut, PCALCINPUTOBJ pcio, int nRadix);
void CIO_vConvertToNumObj(PHNUMOBJ phnoNum, PCALCINPUTOBJ pcio);
