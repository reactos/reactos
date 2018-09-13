/*** cp.c -- Command Parsing Subsystem API


Copyright <C> 1990, Microsoft Corporation

Purpose:


*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <mathsup.h>
#include <rtlproto.h>

#include <ntsdsup.h>
#include <ntsdtok.h>

#define MAXNESTING      (50)

static char rgchOpenQuote[]  = {'\"', '\'', '(', '{', '['};
static char rgchCloseQuote[] = {'\"', '\'', ')', '}', ']'};
#define MAXQUOTE        (sizeof(rgchOpenQuote) / sizeof(rgchOpenQuote[0]))

static char rgchDelim[] = { ' ', '\t', ',' };
#define MAXDELIM        (sizeof(rgchDelim) / sizeof(rgchDelim[0]))

extern  LPSHF   Lpshf;


CPSTATUS
CPStatus(
    EESTATUS ee
    )
{
    switch(ee) {
        case EENOERROR:         return CPNOERROR;
        case EENOMEMORY:        return CPNOMEMORY;
        case EEGENERAL:         return CPGENERAL;
        case EEBADADDR:         return CPBADADDR;
        case EECATASTROPHIC:    return CPCATASTROPHIC;
        default:                return CPGENERAL;
    }
}

CPSTATUS
CPStatusNtsd(
    ULONG Status
    )
{
    switch(Status) {

        case 0:                 return CPNOERROR;

        case ERR_OVERFLOW:
        case ERR_SYNTAX:
        case ERR_BADRANGE:
        case ERR_VARDEF:
        case ERR_EXTRACHARS:
        case ERR_LISTSIZE:
        case ERR_STRINGSIZE:
        case ERR_MEMORY:
        case ERR_BADREG:
        case ERR_BADOPCODE:
        case ERR_SUFFIX:
        case ERR_OPERAND:
        case ERR_ALIGNMENT:
        case ERR_PREFIX:
        case ERR_DISPLACEMENT:
        case ERR_BPLISTFULL:
        case ERR_BPDUPLICATE:
        case ERR_BADTHREAD:
        case ERR_DIVIDE:
        case ERR_TOOFEW:
        case ERR_TOOMANY:
        case ERR_SIZE:
        case ERR_BADSEG:
        case ERR_RELOC:
        case ERR_BADPROCESS:
        case ERR_AMBIGUOUS:
        case ERR_FILEREAD:
        case ERR_LINENUMBER:
        case ERR_BADSEL:
        case ERR_SYMTOOSMALL:
        case ERR_BPIONOTSUP:

                                return CPGENERAL;
        default:                return CPGENERAL;
    }
}


char *
CPSzToken (
    LPSTR *lplpSrc,
    LPSTR buf OPTIONAL
    )

/*++

Routine Description:

    Parse a token from a string.

    The scan will be terminated by a delimiter character or the end of the
    input string.  Delimiter characters will be ignored when in a quoted
    segment.  Quotes will always be preserved.

Arguments:

    lplpSrc - Supplies and returns a pointer to a pointer to the source string

    buf - Optionally supplies a buffer to receive the scanned token.

Return Value:

    The beginning of the token, or NULL if an error occurs.

    lplpSrc will point to the last character not consumed in the string.
    This is not necessarily the same as the beginning of the next token.

    If an error occurs, lplpSrc will point to the place where the scanner
    stopped.

    If a buffer is supplied, the token will be copied to the supplied buffer,
    and the address of the buffer will be returned.  If not, the delimiter
    which terminated the scan will be replaced by a NUL character, and a pointer
    to the beginning of the token in the source string will be returned.

--*/
{
    char *lpSrc;
    char *lpToken;
    int CharType;
    int QSP;
    int QuoteStack[MAXNESTING];
    int QuoteIndex;

    enum {
        Consume,
        Error,
        Done
    } State;

    lpSrc = *lplpSrc;

    if (!lpSrc) {
        return NULL;
    }

    //
    // Skip whitespace
    //

    lpToken =
    lpSrc = CPSkipWhitespace(lpSrc);

    //
    // Scan until a delimiter is found in unquoted space.
    //

    State = Consume;
    QSP = 0;

    while (*lpSrc && State == Consume) {

        CharType = CPQueryChar(lpSrc, "");

        switch (CharType) {

            default:
                // consume it.
                break;

            case CPISDELIM:
                // if stack is empty, finish
                // otherwise, just consume it.
                if (QSP == 0) {
                    State = Done;
                }
                break;

            case CPISOPENANDCLOSEQUOTE:
                //
                // got a ' or a "
                // If it matches the last quote, pop it.
                // If it doesn't match, push it.
                //
                QuoteIndex = CPQueryQuoteIndex( lpSrc );
                if (QSP > 0 && QuoteStack[QSP-1] == QuoteIndex) {
                    --QSP;
                } else {
                    QuoteStack[QSP++] = QuoteIndex;
                }
                break;

            case CPISOPENQUOTE:
                //
                // Got a new open quote.  Always valid.
                //
                QuoteIndex = CPQueryQuoteIndex( lpSrc );
                QuoteStack[QSP++] = QuoteIndex;
                break;

            case CPISCLOSEQUOTE:
                //
                // Got a close quote.  This must match the
                // current open quote.
                //
                if (QSP < 1 || *lpSrc != rgchCloseQuote[QuoteStack[QSP-1]]) {
                    State = Error;
                } else {
                    --QSP;
                }
                break;
        }

        if (State == Consume) {
            ++lpSrc;
        }
    }

    if (QSP || State == Error) {
        //
        // Leave the string pointing to the error
        //
        *lplpSrc = lpSrc;
        return NULL;
    }


    //
    // if lpSrc is sitting on a delimiter, discard it.
    //

    *lplpSrc = CPAdvance(lpSrc, "");

    if (buf) {
        char *p = buf;
        while (lpToken < lpSrc) {
            *p++ = *lpToken++;
        }
        *p = 0;

        lpToken = buf;

    } else {
        if (*lpSrc) {
            *lpSrc = 0;
        }
    }

    return lpToken;

}




int
CPCopyString(
    LPSTR * lplps,
    LPSTR lpT,
    char  chEscape,
    BOOL  fQuote
    )
/*++

Routine Description:

    Scan and copy an optionally quoted C-style string.  If the first character is
    a quote, a matching quote will terminate the string, otherwise the scanning will
    stop at the first whitespace encountered.  The target string will be null
    terminated if any characters are copied.

Arguments:

    lplps    - Supplies a pointer to a pointer to the source string

    lpt      - Supplies a pointer to the target string

    chEscape - Supplies the escape character (typically '\\')

    fQuote   - Supplies a flag indicating whether the first character is a quote

Return Value:

    The number of characters copied into lpt[].  If an error occurs, -1 is returned.

--*/
{
    LPSTR lps = *lplps;
    LPSTR lpt = lpT;
    int   i;
    int   n;
    int   err = 0;
    char  cQuote = '\0';

    if (fQuote) {
        if (*lps) {
            cQuote = *lps++;
        }
    }

    while (!err) {

        if (*lps == 0)
        {
            if (fQuote) {
                err = 1;
            } else {
                *lpt = '\0';
            }
            break;
        }
        else if (fQuote && *lps == cQuote)
        {
            *lpt = '\0';
            // eat the quote
            lps++;
            break;
        }
        else if (!fQuote &&  (!*lps || *lps == ' ' || *lps == '\t' || *lps == '\r' || *lps == '\n'))
        {
            *lpt = '\0';
            break;
        }

        else if (*lps != chEscape)
        {
            *lpt++ = *lps++;
        }
        else
        {
            switch (*++lps) {
              case 0:
                err = 1;
                --lps;
                break;

              default:     // any char - usually escape or quote
                *lpt++ = *lps;
                break;

              case 'b':    // backspace
                *lpt++ = '\b';
                break;

              case 'f':    // formfeed
                *lpt++ = '\f';
                break;

              case 'n':    // newline
                *lpt++ = '\n';
                break;

              case 'r':    // return
                *lpt++ = '\r';
                break;

              case 's':    // space
                *lpt++ = ' ';
                break;

              case 't':    // tab
                *lpt++ = '\t';
                break;

              case '0':    // octal escape
                for (n = 0, i = 0; i < 3; i++) {
                    ++lps;
                    if (*lps < '0' || *lps > '7') {
                        --lps;
                        break;
                    }
                    n = (n<<3) + *lps - '0';
                }
                *lpt++ = (UCHAR)(n & 0xff);
                break;
            }
            lps++;    // skip char from switch
        }

    }  // while

    if (err) {
        return -1;
    } else {
        *lplps = lps;
        return (int) (lpt - lpT);
    }
}


int
CPCopyToken(
    LPSTR * lplps,
    LPSTR lpt
    )
/*++

Routine Description:

    Copy a whitespace delimited token into lpt[].  Lpt[] is not modified
    if there is no token to copy.  If a token is copied, it is null terminated.

Arguments:

    lplps  - Supplies a pointer to a pointer to a string of characters

    lpt    - Supplies a pointer to the string to receive the token

Return Value:

    The number of characters copied into lpt[].  If return value is 0,
    lpt[] is unmodified.  The pointer pointed to by lplps is modified to
    point to the character where scanning stopped; either the whitespace
    after the token or the null at the end of the string.

--*/
{
    LPSTR lps = *lplps;
    int cc = 0;
    // DON'T modify lpt[] if there is no token!!
    while (*lps && (*lps == ' ' || *lps == '\t' || *lps == '\r' || *lps == '\n')) {
        lps++;
    }
    while (*lps && *lps != ' ' && *lps != '\t' && *lps != '\r' && *lps != '\n') {
        *lpt++ = *lps++;
        *lpt = 0;
        cc++;
    }
    *lplps = lps;    // points to separator or 0
    return cc;
}


/*** CPQueryChar - Check the delimiter and Quote table for given char

Purpose: Given a character return whether or not it is a character in our
     delimiter table or our Quoting table

Input:   szSrc   - command line entered by user

Output:

    Returns:

Exceptions:

Notes:


*************************************************************************/
int
CPQueryChar (
    char * szSrc,
    char * szUserDelim
    )
{

    int i, nUserDelim;

    Assert( szSrc != NULL );
    for ( i = 0; i < MAXQUOTE; i++ ) {
        if (*szSrc == rgchOpenQuote[i] && *szSrc == rgchCloseQuote[i] ) {
            return CPISOPENANDCLOSEQUOTE;
        }
        else if (*szSrc == rgchOpenQuote[i] ) {
            return CPISOPENQUOTE;
        }
        else if ( *szSrc == rgchCloseQuote[i] ) {
            return CPISCLOSEQUOTE;
        }
    }

    for ( i = 0; i < MAXDELIM; i++ ) {
        if ( *szSrc == rgchDelim[i] ) {
            return CPISDELIM;
        }
    }

    nUserDelim = strlen( szUserDelim );
    for ( i = 0; i < nUserDelim; i++ ) {
        if ( *szSrc == szUserDelim[i] ) {
            return CPISDELIM;
        }
    }
    return CPNOERROR;
}

/*** CPQueryQuoteIndex - Given a Character return the index

Purpose: Given a character we must be able to get the index in the quote table
         for the character

Input:   szSrc   - command line entered by user

Output:

    Returns:

Exceptions:

Notes:


*************************************************************************/
int
CPQueryQuoteIndex(
    char * szSrc
    )
{

    int i;

    Assert( szSrc != NULL );
    for ( i = 0; i < MAXQUOTE; i++ ) {
        if (*szSrc == rgchOpenQuote[i] ) {
            return i;
        }
    }
    return -1;
}


LPSTR
CPAdvance (
    char * szSrc,
    char * szUserDelim
    )

/*++

Routine Description:

    Consume all whitespace and one or zero non-white delimiter characters,
    until end of string or a non-delimiter character.

Arguments:

    szSrc - Supplies the string to parse

    szUserDelim - Supplies an optional string of delimiter characters

Return Value:

    A pointer to the next token.

--*/
{
    while (*szSrc == ' ' || *szSrc == '\t') {
        szSrc++;
    }
    if ( CPQueryChar ( szSrc, szUserDelim ) == CPISDELIM ) {
        szSrc++;
    }
    while (*szSrc == ' ' || *szSrc == '\t') {
        szSrc++;
    }
    return szSrc;
}


CPSTATUS
CPGetCastNbr(
    char *  szExpr,
    USHORT  type,
    int     radix,
    int     fCase,
    PCXF    pCxf,
    char *  pValue,
    char *  szErrMsg,
    BOOL    fSpecial
    )
/*++

Routine Description:

    Evaluate an expression and convert the result to a particular type.

Arguments:

    szExpr  - Supplies the expression to evaluate

    type - Supplies a type index

    radix - Supplies the default number base for parser

    fCase - Supplies TRUE to cause case-sensitive symbol recognition

    pCxf - Supplies a context and frame for expression evaluation

    pValue - Returns the result

    szErrMsg - Returns an error string upon failure

    fSpecial - Supplies TRUE if the NTSD expression evaluator is to be used.
            The NTSD EE only supports ULONG type.

Return Value:

    CPSTATUS value indicating success or reason for failure

--*/
{
    HTM     hTM = (HTM)NULL;
    HTI     hTI = (HTI)NULL;
    PTI     pTI = NULL;
    ULONG64 vResult = 0;
    RTMI    RIT;
    EESTATUS Err;
    DWORD    strIndex;
    UNALIGNED PULONG_PTR pUValue = (PULONG_PTR)pValue; 

    if (fSpecial) {
        ULONG Status;
        PUCHAR lpNext;

        Status = GetExpression ( (PUCHAR) szExpr,
                                &vResult,
                                radix,
                                LppdCur ? LppdCur->mptProcessorType : -1,
                                &lpNext
                                );
        *pUValue = (ULONG_PTR)vResult;
        return CPStatusNtsd(Status);
    }

    // initialize some stuff
    Err = EENOERROR;
    if (szErrMsg) {
        *szErrMsg = '\0';
    }

    memset( &RIT, 0, sizeof(RTMI) );
    RIT.fValue = TRUE;
    RIT.Type   = type;
    RIT.fSzBytes = TRUE;

    // parse the expression

    Err = EEParse(szExpr, radix, fCase, &hTM, &strIndex);
    if(!Err) Err = EEBindTM(&hTM, SHpCXTFrompCXF(pCxf), TRUE, FALSE);
    if(!Err) Err = EEvaluateTM(&hTM, SHhFrameFrompCXF(pCxf), EEHORIZONTAL);
    if(!Err) Err = EEInfoFromTM(&hTM, &RIT, &hTI);


    if (!Err) {
        // lock down the TI
        if( !hTI  ||  !(pTI = (PTI) MMLpvLockMb (hTI)) ) {

            Err = NOROOM;

        } else {

            // now see if we have the value
            if( pTI->fResponse.fValue  &&  pTI->fResponse.Type == RIT.Type ) {
                _fmemcpy (pValue, (char *) pTI->Value, (short) pTI->cbValue);
            } else {
                Err = BADTYPECAST;
            }
            MMbUnlockMb(hTI);
        }

        // get the error
        if( szErrMsg ) {
            CVMessage(ERRORMSG, Err, MSGSTRING, szErrMsg);
        }

        // get the error

    } else {

        if ( szErrMsg ) {
            CVExprErr(Err, MSGSTRING, &hTM, szErrMsg);
        } else {
            CVExprErr ( Err, MSGGERRSTR, &hTM, NULL );
            Err = GEXPRERR;
        }

    }
    // free any handles
    if(hTM) {
        EEFreeTM(&hTM);
    }

    if( hTI ) {
        EEFreeTI(&hTI);
    }

    // return the error code

    return(CPStatus(Err));
}



/*** CPGetNbr
*
* Purpose: To convert and expression into a number
*
* Input:
*   szExpr  - The expression to evaluate
*
* Output:
*   pErr    - The Expression Evaluators error msg nbr.
*
*  Returns The numeric value of the expression. Or zero. If the result
*      is zero, check the Err value to determine if an error occured.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
long
CPGetNbr(
    char *  szExpr,
    int     radix,
    int     fCase,
    PCXF    pCxf,
    char *  szErrMsg,
    int  *  pErr,
    BOOL    fSpecial
    )
{
    long        Value;

    *pErr = CPGetCastNbr(szExpr,
                         0x22,
                         radix,
                         fCase,
                         pCxf,
                         (char *)&Value,
                         szErrMsg,
                         fSpecial
                         );

    return (*pErr == CPNOERROR) ? Value : 0;
}


CPSTATUS
CPGetFPNbr(
    LPSTR   lpExpr,
    int     cBits,
    int     nRadix,
    int     fCase,
    PCXF    pCxf,
    LPSTR   lpBuf,
    LPSTR   lpErr
    )
/*++

Routine Description:

    Get a floating point number.  This is a front end for CPGetCastNbr
    which maps a bitcount into an OMF type.

Arguments:

    lpExpr   - Supplies expr to evaluate
    cBits    - Supplies size in bits of result type
    nRadix   - Supplies default radix for integer exprs
    fCase    - Supplies case sensitivity flag
    pCxf     - Supplies context/frame for EE
    lpBuf    - Return result in buffer this points to
    lpErr    - Return error string from EE

Return Value:

    CPSTATUS code

--*/
{
    USHORT     omftype;

    switch (cBits) {
      case 32:
        omftype = T_REAL32;
        break;

      case 64:
        omftype = T_REAL64;
        break;

      case 80:
        omftype = T_REAL80;
        break;

      default:
        return CPCATASTROPHIC;
    }

    return CPGetCastNbr(lpExpr,
                        omftype,
                        nRadix,
                        fCase,
                        pCxf,
                        lpBuf,
                        lpErr,
                        FALSE
                        );
}


/***    CPGetInt
**
*/

long
CPGetInt(
    char * szExpr,
    int  * pErr,
    int  * cLength
    )
{
    long    lVal = 0;
    int     cb = 0;

    /*
    **  Clear out the error field first
    */

    *pErr = FALSE;

    /*
    **  First check for a null string.  Return 0 and an error
    */

    if (*szExpr == 0) {
        *pErr = TRUE;
        return 0;
    }

    /*
    **  Check that first character is numeric
    */

    if ((*szExpr < '0') || ('9' < *szExpr)) {
        *pErr = TRUE;
        return 0;
    }

    /*
    **
    */

    while (('0' <= *szExpr) && (*szExpr <= '9')) {
        lVal = lVal*10 + *szExpr - '0';
        szExpr++;
        cb += 1;
    }

    /*
    **
    */

    *cLength = cb;
    return lVal;
}                   /* CPGetInt() */

CPSTATUS
CPGetAddress(
    LPCSTR      lpExprOrig,
    LPINT       lpcch,
    PADDR       lpAddr,
    EERADIX     radix,
    PCXF        pcxf,
    BOOL        fCase,
    BOOL        fSpecial
    )
/*++

Routine Description:

    This routine will attempt to take the first whitespace delimited
    portion of the expression string and convert it into an address.
    If the routine is not successful then it will give a reason error
    code.


Arguments:

    lpExpr  - string which has the address in it

    lpcch   - pointer to return location for count of characters used

    lpAddr  - pointer to address structure to return value in

    radix   - radix to use for evaluation

    pcxf    - pointer to cxf structure

    fCase   - TRUE if case sensitive parse

    fSpecial - Use NTSD expression eval

Return Value:


--*/
{
    HTM         hTm = (HTM) NULL;
    HTI         hTi = (HTI) NULL;
    PTI         pTi = NULL;
    RTMI        ri;
    EESTATUS    eeErr = EENOERROR;
    DWORD       strIndex;
    int         cch = 0;
    LPSTR       lpExpr;
    LPSTR       TokenBuffer;
    LPSTR       p;
    HEXE        hexe;
    CHAR        szFullContext[512];
    LPSTR       ExeName;
    SHE         She;
    BOOL        fUse;
    BOOL        fLoad;
    CHAR        fname[MAX_PATH];
    SHE         she;
    LPSTR       lpNext;


    if (fSpecial) {
        NTSDADDR NtsdAddress;
        ULONG Status;

        Status = GetAddrExpression( (PUCHAR) lpExprOrig,
                                   &NtsdAddress,
                                   radix,
                                   LppdCur ? LppdCur->mptProcessorType : -1,
                                   (ULONG)-1,
                                   (PUCHAR*)&lpNext
                                   );
        if (Status == 0) {
            NtsdAddrToAddr(&NtsdAddress, lpAddr);
            *lpcch = (int) (lpNext - lpExprOrig);
        }
        return CPStatusNtsd(Status);
    }

    /*
    **  Setup Initialization
    */

    memset( &ri, 0, sizeof(RTMI) );
    memset( lpAddr, 0, sizeof(*lpAddr));
    ri.fAddr = TRUE;

    /*
    **  Skip over leading white space and then find the next white space
    */

    TokenBuffer = (LPSTR)malloc(strlen(lpExprOrig) +1);

    lpNext = (LPSTR)lpExprOrig;

    lpExpr = CPSzToken(&lpNext, TokenBuffer);

    if (!lpExpr || !*lpExpr) {

        eeErr = EEGENERAL;

    } else {

        //
        // get the length of the expr
        //

        cch = (int) (lpNext - lpExprOrig);

        //
        // check for a context override
        //
        p = (PSTR) strchr( (PUCHAR) lpExpr, '}' );
        if (!p) {
            p = (PSTR) strchr( (PUCHAR) lpExpr, '!' );
        }

        if (!p) {

            //
            //  Parse the expression
            //
            eeErr = EEParse(lpExpr, radix, fCase, &hTm, &strIndex);
            if (eeErr == EENOERROR) {
                eeErr = EEBindTM(&hTm,
                                 SHpCXTFrompCXF(pcxf),
                                 TRUE,
                                 FALSE
                                );
            }
            if (eeErr == EENOERROR) {
                eeErr = EEvaluateTM(&hTm, SHhFrameFrompCXF(pcxf), EEHORIZONTAL);
            }
            if (eeErr == EENOERROR) {
                eeErr = EEInfoFromTM(&hTm, &ri, &hTi);
            }

        } else {

            //
            // first try the context passed in
            //
            eeErr = EEParse(lpExpr, radix, fCase, &hTm, &strIndex);
            if (eeErr == EENOERROR) {
                eeErr = EEBindTM(&hTm,
                                 SHpCXTFrompCXF(pcxf),
                                 TRUE,
                                 FALSE
                                );
            }
            if (eeErr == EENOERROR) {
                eeErr = EEvaluateTM(&hTm,
                                    SHhFrameFrompCXF(pcxf),
                                    EEHORIZONTAL
                                   );
            }
            if (eeErr == EENOERROR) {
                eeErr = EEInfoFromTM(&hTm, &ri, &hTi);
            }

            if (eeErr != EENOERROR) {
                //
                // search all contexts looking for the expression
                //
                fLoad = FALSE;
search_again:
                hexe = (HEXE) NULL;
                while ((( hexe = SHGetNextExe( hexe ) ) != 0) ) {

                    //
                    //  We try a module only if its symbols are loaded. If the
                    //  symbols are defered and the caller wants to load them,
                    //  we load them.
                    //
                    fUse = FALSE;
                    if (SHSymbolsLoaded(hexe, &she)) {
                        fUse = TRUE;
                    }
                    else if (she == sheDeferSyms) {
                        if ( fLoad ) {
                            SHWantSymbols( hexe );
                            if (SHSymbolsLoaded(hexe, NULL)) {
                                fUse = TRUE;
                            }
                        }
                    }

                    if ( fUse ) {

                        //
                        // format a fully qualified symbol name
                        //
                        ExeName =  SHGetExeName( hexe );
                        if (ExeName) {
                            _splitpath( ExeName, NULL, NULL, fname, NULL );
                            sprintf( szFullContext, "%s!%s", fname, p+1 );
                        } else {
                            strcpy( szFullContext, lpExpr );
                        }

                        //
                        // try to parse and bind the expression
                        //
                        eeErr = EEParse(szFullContext,
                                        radix,
                                        fCase,
                                        &hTm,
                                        &strIndex
                                        );
                        if (eeErr == EENOERROR) {
                            eeErr = EEBindTM(&hTm,
                                             SHpCXTFrompCXF(pcxf),
                                             TRUE,
                                             FALSE
                                            );
                        }
                        if (eeErr == EENOERROR) {
                            eeErr = EEvaluateTM(&hTm,
                                                SHhFrameFrompCXF(pcxf),
                                                EEHORIZONTAL
                                                );
                        }
                        if (eeErr == EENOERROR) {
                            eeErr = EEInfoFromTM(&hTm, &ri, &hTi);
                        }
                        if (eeErr == EENOERROR) {
                             break;
                        }
                        if (hTm) {
                             EEFreeTM( &hTm );
                             hTm = NULL;
                        }
                        if (hTi) {
                             EEFreeTI( &hTi );
                             hTi = NULL;
                        }
                    }
                }

                if ((eeErr != EENOERROR) && (!fLoad)) {
                    fLoad = TRUE;
                    goto search_again;
                }
            }
        }


        //
        //  Extract the desired information
        //

        if (eeErr == EENOERROR) {
            if (!hTi || !(pTi = (PTI) MMLpvLockMb( hTi ))) {
                eeErr = EEGENERAL;
            } else {

                *lpcch = cch;

                if (pTi->fResponse.fAddr) {
                    *lpAddr = pTi->AI;
                } else
                if (pTi->fResponse.fValue && pTi->fResponse.fSzBytes &&
                  pTi->cbValue >= sizeof(WORD)) {

                    switch( pTi->cbValue ) {
                      case sizeof(WORD):
                        SetAddrOff( lpAddr, *((WORD *) pTi->Value));
                        break;

                      case sizeof(DWORD):
                        SE_SetAddrOff( lpAddr, *((DWORD *) pTi->Value));
                        break;

                      case sizeof(DWORDLONG):
                        SetAddrOff( lpAddr, *((DWORDLONG *) pTi->Value));
                        break;
                    }

                    // set the segment

                    if ((pTi->SegType & EEDATA) == EEDATA) {
                        ADDR addrData = {0};

                        OSDGetAddr( LppdCur->hpid, LptdCur->htid, adrData, &addrData );
                        SetAddrSeg( lpAddr, (SEGMENT) GetAddrSeg( addrData ));
                        ADDR_IS_FLAT(*lpAddr) = ADDR_IS_FLAT(addrData);
                        SYUnFixupAddr ( lpAddr );
                    } else if ((pTi->SegType & EECODE) == EECODE) {
                        ADDR addrPC = {0};

                        OSDGetAddr( LppdCur->hpid, LptdCur->htid, adrPC, &addrPC);
                        SetAddrSeg( lpAddr, (SEGMENT) GetAddrSeg( addrPC ));
                        ADDR_IS_FLAT(*lpAddr) = ADDR_IS_FLAT(addrPC);
                        SYUnFixupAddr( lpAddr );
                    } else {
                        ADDR addrData = {0};

                        OSDGetAddr( LppdCur->hpid, LptdCur->htid, adrData, &addrData );
                        SetAddrSeg( lpAddr, (SEGMENT) GetAddrSeg( addrData ));
                        ADDR_IS_FLAT(*lpAddr) = ADDR_IS_FLAT(addrData);
                        SYUnFixupAddr ( lpAddr );
                    }
                }
                MMbUnlockMb( hTi );
            }
        }
    }
    /*
    **  Free up any handles
    */

    if (hTm) {
        EEFreeTM( &hTm );
    }
    if (hTi) {
        EEFreeTI( &hTi );
    }

    if (TokenBuffer) {
        free(TokenBuffer);
    }

    return CPStatus(eeErr);
}                   /* CPGetAddress() */


CPSTATUS
CPGetRange(
    char     * lpszExpr,
    int      * lpcch,
    ADDR     * lpAddr1,
    ADDR     * lpAddr2,
    PULONG     lpItems,
    EERADIX    radix,
    int        cbDefault,
    int        cbSize,
    CXF      * pcxf,
    BOOL       fCase,
    BOOL       fSpecial,
    LPBOOL     lpbSecondParamIsALength
    )

/*++

Routine Description:

    Decode a range expression of the form:
    addr1 [ l count | addr2 ]

    return it as two addresses.

Arguments:

    lpszExpr    - Supplies pointer to argument string

    lpcch       - Returns count of characters used

    lpAddr1     - Returns start address

    lpAddr2     - Returns end address.

    lpItems     - Returns item count, if possible.

    radix       - Supplies default radix for expression parser

    cbDefault   - Supplies default item count value

    cbSize      - Supplies size in bytes of data item

    pcxf        - Supplies pointer to context/frame info

    fCase       - Supplies case sensitivity flag for parser

    fSpecial    - Use NTSD expr eval


    lpbSecondParamIsALength - (out) Optional, can be null. If not null, will return TRUE or FALSE.
                  TRUE indicates that the second address is of the form <L length>.
                  FALSE indicates that the second address is of the form <address>.
                  If an error occurs, this value is undefined.

Return Value:

    0 For success, or error code (from parser?)
    Note: this will succeed if addr2 < addr1 or addr1.seg != adr2.seg;
    caller must decide whether range makes sense.

--*/
{
    LPSTR   lpsz;
    LPSTR   lpsz1;
    LPSTR   lpsz0;
    ADDR    addr1, addr2;
    LONG64  ll;
    int     err;
    int     cch;
    LONG64  count;
    char    ch;
    BOOL    fError = FALSE;

    if (fSpecial) {
        ULONG Status;
        PUCHAR lpNext;
        ULONG64 value;
        NTSDADDR NtsdAddr;
        BOOL fLength;

        value = cbDefault;

        Status = GetRange( (PUCHAR) lpszExpr,
                          &NtsdAddr,
                          &value,
                          cbSize,
                          &fLength,
                          radix,
                          LppdCur ? LppdCur->mptProcessorType : -1,
                          (ULONG)-1,
                          &lpNext,
                          lpbSecondParamIsALength
                          );

        *lpcch = (int) ((PSTR) lpNext - lpszExpr);

        // if Status == 0, it succeeded, otherwise it is
        // an error status.
        if (Status == 0) {

            NtsdAddrToAddr(&NtsdAddr, lpAddr1);

            if (fLength) {
                *lpItems = (ULONG)value;
                *lpAddr2 = *lpAddr1;
                GetAddrOff(*lpAddr2) += cbSize * value;
            } else {
                *lpItems = 0;
                NtsdAddrToAddr((PNTSDADDR)value, lpAddr2);
            }
        }
        SYFixupAddr ( lpAddr1 );
        SYFixupAddr ( lpAddr2 );
        return CPStatusNtsd(Status);
    }

    lpsz0 = lpsz = _strdup(lpszExpr);

    // first arg must be an address:
    if ((err = CPGetAddress(lpsz, &cch, &addr1, radix, pcxf, fCase, fSpecial)) != EENOERROR) {
        fError = TRUE;
        goto done;
    }

    // Then, see how much s/b added to get the end.

    lpsz = CPSkipWhitespace(lpsz + cch);

    count = 0;

    if ((ch = *lpsz) == '\0') {

        // no second part - fill in default
        addr2 = addr1;
        ll = cbDefault * cbSize;
        count = cbDefault;

    } else if (!strchr( (PUCHAR) "iIlL", ch) || *(lpsz = CPSkipWhitespace(lpsz+1)) == '\0') {

        // End address specified (not L or L is last token)
        // must be an addr

        if (lpbSecondParamIsALength) {
            *lpbSecondParamIsALength = FALSE;
        }

        if ((err = CPGetAddress(lpsz, &cch, &addr2, radix, pcxf, fCase, fSpecial)) != EENOERROR) {
            fError = TRUE;
            goto done;
        }

        lpsz += cch;

        ll = 0;

    } else {

        // Length specified

        if (lpbSecondParamIsALength) {
            *lpbSecondParamIsALength = TRUE;
        }

        if (!(lpsz1 = CPSzToken(&lpsz, NULL))) {
            fError = TRUE;
            goto done;
        }

        count = CPGetNbr(lpsz1, radix, fCase, pcxf, NULL, &err, fSpecial);
        if (count == 0 && err != EENOERROR) {
            fError = TRUE;
            goto done;
        }

        addr2 = addr1;

        ll = count * cbSize;
    }

    // Fixup each address to the final values.

    SYFixupAddr ( &addr1 );
    SYFixupAddr ( &addr2 );
    GetAddrOff(addr2) += ll;

    if (!count && cbSize) {
        count = (GetAddrOff(addr2) - GetAddrOff(addr1)) / cbSize + 1;
        GetAddrOff(addr2) = GetAddrOff(addr1) + cbSize * count;
    }

    // Sign extend
    GetAddrOff(addr2) = SEPtrTo64( GetAddrOff(addr1) );

    *lpcch = (int) (lpsz - lpsz0);
    *lpAddr1 = addr1;
    *lpAddr2 = addr2;
    Assert(count < MAX_ULONG);
    *lpItems = (ULONG) count;

done:
    free(lpsz0);

    if (fError) {
        return CPGENERAL;
    } else {
        return CPNOERROR;
    }

}   /* CPGetRange */



/***    CPSkipWhitespace
**
**  Synopsis:
**  lpsz = CPSkipWhitespace(lpszIn)
**
**  Entry:
**  lpszIn  - string to skip white space on
**
**  Returns:
**  Pointer to first non-white space character in string
**
**  Description:
**  This function will skip over any leading white space in a string
**  and return a pointer to the first non-whitespace character
**
*/

char *
CPSkipWhitespace(
    PCSTR lpszIn
    )
{
    Assert(lpszIn);

    while (*lpszIn == ' ' || *lpszIn == '\t') {
        lpszIn++;
    }
    return (PSTR) lpszIn;
}                   /* CPSkipWhiteSpace() */


/***    CPRemoveTrailingWhitespace
**
**  Synopsis:
**  CPRemoveTrailingWhitespace(pszStart)
**
**  Entry:
**  pszStart  - string to remove trailing white space from
**
**  Returns:
**  nothing
**
**  Description:
**  This function will remove any tariling white space in a string
**  by setting the null terminator past the 
**
*/

void
CPRemoveTrailingWhitespace(
    PSTR pszStart
    )
{
    Assert(pszStart);
    PSTR pszEnd = pszStart + _tcslen(pszStart); 

    pszEnd--;
    while ( !*pszEnd && pszEnd >= pszStart ) {
        pszEnd--;

    }
}                   /* CPSkipWhiteSpace() */




struct format {
    DWORD  cBits;
    FMTTYPE fmtType;
    DWORD  radix;
    DWORD  fTwoFields;
    DWORD  cchMax;
    LPSTR lpszDescription;
} RgFormats[] = {
    {8,  fmtAscii,  0, FALSE,  1,  "ASCII"},
    {8,  fmtInt,   16, TRUE,   2,  "Byte"},
    {16, fmtInt,   10, FALSE,  6,  "Short"},
    {16, fmtUInt,  16, FALSE,  4,  "Short Hex"},
    {16, fmtUInt,  10, FALSE,  5,  "Short Unsigned"},
    {32, fmtInt,   10, FALSE,  11, "Long"},
    {32, fmtUInt,  16, FALSE,  8,  "Long Hex"},
    {32, fmtUInt,  10, FALSE,  10, "Long Unsigned"},
    {64, fmtInt,   10, FALSE,  21, "Quad"},
    {64, fmtUInt,  16, FALSE,  16, "Quad Hex"},
    {64, fmtUInt,  10, FALSE,  20, "Quad Unsigned"},
    {32, fmtFloat, 10, FALSE,  14, "Real (32-bit)"},
    {64, fmtFloat, 10, FALSE,  23, "Real (64-bit)"},
    {80, fmtFloat, 10, FALSE,  25, "Real (10-byte)"}
//    {128,fmtFloat, 10, FALSE,  42, "Real (16-byte)"}
};

//
// range[i] is smallest value larger than that
// expressible in 'i' bits.
//

ULONG range[] = {
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,

 };



LPSTR
LargeIntegerFormat(
    LARGE_INTEGER li,
    DWORD                  radix,
    BOOL                  signe,
    char *                buf,
    unsigned long         max
    )

/*++

Routine Description:

    LargeIntegerFormat.

    This routine formats a large integer, signed or unsigned,
    in radix 8, 10 or 16, into a string.
    This works on any machine with a LARGE_INTEGER type;  it
    does not require LARGE_INTEGER support from sprintf.

Arguments:

    li     - the large integer value to be formatted

    radix  - the radix to use in formatting (8, 10, 16)

    signed - whether the li is signed

    buf    - where to store the result

    max    - the buffer size.

Returns:

    pointer into buf where string begins

--*/

{

    LARGE_INTEGER radixli, remain;
    int digit;
    BOOL    needsign = FALSE;


    //
    // make sure the radix is ok, and put in LARGE_INTEGER
    //

    Assert (radix == 8 || radix == 10 || radix == 16 );

    radixli.LowPart  = radix;
    radixli.HighPart = 0;

    remain.LowPart = remain.HighPart = 0;

    //
    // null-terminate the string
    //

    max--;
    buf[max] = '\0';
    digit = 1;

    //
    // If we are to do a signed operation, and the value is negative
    // operate on its inverse, and prepend a '-' sign when complete.
    //

    if (signe && li.QuadPart < 0) {
        li.QuadPart = li.QuadPart * -1;
        needsign = TRUE;
    }

    if (li.HighPart) {
        sprintf(buf, "-%x%08x", li.HighPart, li.LowPart);
    } else {
        sprintf(buf, "-%x", li.LowPart);
    }

    if (needsign) {
        return buf;
    } else {
        return buf+1;    // skip minus skip if not needed
    }

#if 0
    //
    // Starting with LSD, pull the digits out
    // and put them in the string at the right end.
    //
    do {
        remain.QuadPart = li.QuadPart - (li.QuadPart / radixli.QuadPart);
        li.QuadPart = li.QuadPart / radixli.QuadPart;

        //
        // If remainder is > 9, then radix was 16, and
        // we need to print A-E, else print 0-9.
        //

        if (remain.LowPart > 9) {
            buf[max - digit++] = (char)('A' + remain.LowPart - 10);
        } else {
            buf[max - digit++] = (char)('0' + remain.LowPart);
        }

    } while ( li.LowPart || li.HighPart );

    if (needsign) {
        buf[max-digit++] = '-';
    }

    return(&buf[max-digit+1]);
#endif
}


CPSTATUS
CPFormatMemory(
    LPCH    lpchTarget,
    DWORD    cchTarget,
    LPBYTE  lpbSource,
    DWORD    cBits,
    FMTTYPE fmtType,
    DWORD    radix
    )

/*++

Routine Description:

    CPFormatMemory.

    formats a value by template

Arguments:

    lpchTarget - Destination buffer.

    cchTarget - Size of destination buffer.

    lpbSource - Data to be formatted.

    cBits - Number of bits in the data.

    fmtType - Determines how the data will be treated?? UINT, float, real, ...

    radix - Radix to use when formatting.

Return Value:

    None.

--*/
{
    LONG64      l;
    long        cb;
    ULONG64     ul = 0;
    char        rgch[512] = {0};
    
    
    Assert (radix == 8 || radix == 10 || radix == 16 ||
            (fmtType & fmtBasis) == fmtAscii ||
            (fmtType & fmtBasis) == fmtUnicode);
    Assert (cBits != 0);
    Assert (cchTarget <= sizeof(rgch));
    
    switch (fmtType & fmtBasis) {
    //
    //  Format from memory bytes into an integer format number
    //
    case fmtInt:
        
        if (radix == 10) {
            
            switch( (cBits + 7)/8 ) {
            case 1:
                l = *(signed char *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    sprintf(rgch, "%0*I64d", cchTarget-1, l);
                } else {
                    sprintf(rgch, "% I64d", l);
                }
                break;
                
            case 2:
                l = *(short *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    sprintf(rgch, "%0*I64d", cchTarget-1, l);
                } else {
                    sprintf(rgch, "% I64d", l);
                }
                break;
                
            case 4:
                l = *(long *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    sprintf(rgch, "%0*I64d", cchTarget-1, l);
                } else {
                    sprintf(rgch, "% I64d", l);
                }
                break;
                
            case 8:
                l = *(LONG64 *)lpbSource;
                if (fmtType & fmtZeroPad) {
                    sprintf(rgch, "%0*I64d", cchTarget-1, l);
                } else {
                    sprintf(rgch, "% I64d", l);
                }
                break;

            default:
                return CPBADFORMAT;
            }
            
            
            if (strlen(rgch) >= cchTarget) {
                return CPOVERRUN;
            }
            
            strcpy(lpchTarget, rgch);
            
            break;
        }
        //
        // then we should handle this as UInt
        //
        
    case fmtUInt:
        
        cb = (cBits + 7)/8;
        switch( cb ) {
        case 1:
            ul = *(BYTE *) lpbSource;
            break;
            
        case 2:
            ul = *(USHORT *) lpbSource;
            break;
            
        case 4:
            ul = *(ULONG *) lpbSource;
            break;
            
//
// MBH - bugbug - CENTAUR bug;
// putting contents of instead of address of structure
// for return value in a0.
//

        case 8:
            ul = *(ULONG64 *) lpbSource;
            break;
            
            
        default:
            if (radix != 16 || (fmtType & fmtZeroPad) == 0) {
                return CPBADFORMAT;
            }
        }
        
        if (fmtType & fmtZeroPad) {
            switch (radix) {
            case 8:
                sprintf(rgch, "%0*.*I64o", cchTarget-1, cchTarget-1, ul);
                break;
            case 10:
                sprintf(rgch, "%0*.*I64u", cchTarget-1, cchTarget-1, ul);
                break;
            case 16:
                if (cb <= 8) {
                    sprintf(rgch, "%0*.*I64x", cchTarget-1, cchTarget-1, ul);
                } else {
                    // handle any size:
                    // NOTENOTE a-kentf this is dependent on byte order
                    for (l = 0; l < cb; l++) {
                        sprintf(rgch+l+l, "%02.2x", lpbSource[cb - l - 1]);
                    }
                    //sprintf(rgch, "%0*.*x", cchTarget-1, cchTarget-1, ul);
                }
                break;
            }
        } else {
            switch (radix) {
            case 8:
                sprintf(rgch, "%I64o", ul);
                break;
            case 10:
                sprintf(rgch, "%I64u", ul);
                break;
            case 16:
                sprintf(rgch, "%I64x", ul);
                break;
            }
        }
            
        
        if (strlen(rgch) >= cchTarget) {
            return CPOVERRUN;
        }
        
        strcpy(lpchTarget, rgch);
        
        break;
        
        
        case fmtAscii:
            if ( cBits != 8 ) {
                return CPBADFORMAT;
            }
            lpchTarget[0] = *(BYTE *) lpbSource;
            if ((lpchTarget[0] < ' ') || (lpchTarget[0] > 0x7e)) {
                lpchTarget[0] = '.';
            }
            lpchTarget[1] = 0;
            return CPNOERROR;
            
        case fmtUnicode:
            if (cBits != 16) {
                return CPBADFORMAT;
            }
            Assert((DWORD)MB_CUR_MAX <= cchTarget);
            if ((wctomb(lpchTarget, *(LPWCH)lpbSource) == -1) ||
                (lpchTarget[0] < ' ') ||
                (lpchTarget[0] > 0x7e)) {
                lpchTarget[0] = '.';
            }
            lpchTarget[1] = 0;
            return CPNOERROR;
            
        case fmtFloat:
            switch ( cBits ) {
            case 4*8:
                sprintf(rgch, "% 12.6e",*((float *) lpbSource));
                break;
                
            case 8*8:
                //            sprintf(rgch, "% 17.11le", *((double *) lpbSource));
                sprintf(rgch, "% 21.14le", *((double *) lpbSource));
                break;
                
            case 10*8:
                if (_uldtoa((_ULDOUBLE *)lpbSource, 25, rgch) == NULL) {
                    return CPBADFORMAT;
                }
                break;
                
            case 16*8:
                // v-vadimp this is an IA64 float - may have to rethink the format here
                // what we are getting here is really FLOAT128
                if (_uldtoa((_ULDOUBLE *)(lpbSource), 30, rgch) == NULL) {
                    return CPBADFORMAT;
                }
                break;
                
            default:
                return CPBADFORMAT;
                
            }
            
            if (strlen(rgch) >= cchTarget) {
                return CPOVERRUN;
            }
            
            strncpy(lpchTarget, rgch, cchTarget-1);
            lpchTarget[cchTarget-1] = 0;
            return CPNOERROR;
            
            case fmtAddress:
                return CPBADFORMAT;
                
            case fmtZeroPad:
                return CPBADFORMAT;
                
            case fmtBit:
                { 
                    WORD i,j,shift=0; //shift will allow for a blank after each 8 bits
                    for (i=0;i<cBits/8;i++)  {
                        for(j=0;j<8;j++) {
                            if((lpbSource[i]>>j) & 0x1) {
                                rgch[i*8+j+shift]='1';
                            } else {
                                rgch[i*8+j+shift]='0';
                            }
                        }
                        rgch[(i+1)*8+shift]=' ';
                        shift++;
                    }
                    rgch[cBits+shift-1]='\0';
                    strcpy(lpchTarget,rgch);
                }
                return CPNOERROR;
                
    }

    return CPNOERROR;
}                   /* CPFormatMemory() */


CPSTATUS
CPUnformatMemory(
                LPBYTE   lpbTarget,
                LPSTR    lpszSource,
                DWORD    cBits,
                FMTTYPE  fmtType,
                DWORD    nradix
                )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    ULARGE_INTEGER largeint;
    ULONG l;
    char *ptr = lpszSource;

    switch ( fmtType & fmtBasis ) {
    case fmtBit:
    {
        ULONGLONG ul = 0;
        ptr += strlen(lpszSource);
        if (ptr != lpszSource) while (--ptr != lpszSource) {
            switch (*ptr) {
                case '1':
                    ul += 1;
                case '0':
                    if (ptr!=lpszSource) ul <<= 1;
                case ' ':
                    break;
                default:
                    Assert(!"Bad binary digit");
                    return CPBADFORMAT;
            }
        }
        *(ULONGLONG*)lpbTarget = ul;
        return CPNOERROR;
    }

    case fmtInt:
    case fmtUInt:

        Assert (nradix == 8 || nradix == 10 || nradix == 16);

        l = 0;

        ptr = CPSkipWhitespace(ptr);

        if ((fmtType & fmtOverRide) == 0) {
            if (*ptr == '0') {
                // force radix - is it hex or octal?
                ++ptr;
                if (*ptr == 'x' || *ptr == 'X') {
                    ++ptr;
                    nradix = 16;
                } else if ((*ptr == 'o') || (*ptr == 'O')) {
                    nradix = 8;
                }
            }
        }

        errno = 0;
        largeint = (strtouli(ptr, &ptr, (int)nradix));
        l = largeint.LowPart;

        if ((l == 0 || l == ULONG_MAX) && errno == ERANGE) {
            return CPBADFORMAT;
        }

        if (cBits < 32 &&  l > range[cBits-1] ) {
            return CPBADFORMAT;
        }

        if (*CPSkipWhitespace(ptr)) {
            return CPBADFORMAT;
        }

        switch ( (cBits + 7)/8 ) {
        case 1:
            *(BYTE *) lpbTarget = (BYTE) l;
            break;

        case 2:
            *(USHORT *) lpbTarget = (USHORT) l;
            break;

        case 4:
            *(ULONG *) lpbTarget = l;
            break;

        case 8:
            *(PULARGE_INTEGER)lpbTarget = largeint;
            break;


        default:
            return CPBADFORMAT;
        }
        break;

    case fmtFloat:
        // radix is ALWAYS 10 in the world of floats
        switch ( (cBits + 7)/8 ) {

        case 4:
            if (sscanf(lpszSource, "%f", (float *)lpbTarget) != 1) {
                return CPBADFORMAT;
            }
            break;

        case 8:
            if (sscanf(lpszSource, "%lf", (double *)lpbTarget) != 1) {
                return CPBADFORMAT;
            }
            break;

        case 10:
            // i = sscanf(lpszSource, "%Lf", (long double *)lpbTarget);
            _atoldbl( (_ULDOUBLE *)lpbTarget, lpszSource );
            break;

        case 16:  //v-vadimp this is an IA64 float (FLOAT128)
            _atoldbl( (_ULDOUBLE *)lpbTarget, lpszSource );
            break;

        default:
            return CPBADFORMAT;
        }

        break;

    case fmtAscii:
    case fmtUnicode:
    case fmtAddress:
        // these aren't handled here.
        return CPBADFORMAT;
    }
    return CPNOERROR;
}                   /* CPUnformatMemory() */



CPSTATUS
CPUnformatAddr(
    LPADDR lpaddr,
    char * lpsz,
    PBOOL pbSegAlwaysZero
    )

/*++

Routine Description:

    This routine takes an address string and converts it into an
    ADDR packet.  The assumption is that the address is in one of the
    following formats:

    XXXX:XXXX                           16:16 address ( 9)
    0xXXXX:0xXXXX                       16:16 address (13)
    XXXX:XXXXXXXX                       16:32 address (13)
    0xXXXX:0xXXXXXXXX                   16:32 address (17)
    0xXXXXXXXX                           0:32 address (10)
    XXXXXXXX                             0:32 address ( 8)
    0xXXXXXXXXXXXXXXXX                   0:64 address (18)
    XXXXXXXXXXXXXXXX                     0:64 address (16)

Arguments:

    lpaddr - Supplies the address packet to be filled in

    lpsz   - Supplies the string to be converted into an addr packet.

    pbSegAlwaysZero - May be NULL.

             This is dependent on the format of the
             string passed into the function. With some formats, the segment
             is always zero, with others, the segment may or may not be zero.

             TRUE - Indicates that the segment of the address will always be
                    0 regardless of the value passed in.
             FALSE - The segment may or may not be zero.

Return Value:

    CPNOERROR - no errors
    CPGENERAL - unable to do unformatting

--*/
{
    int         i;
    SEGMENT     seg = 0;
    OFFSET      off = 0;
    BOOL        fReal = FALSE;

    if (pbSegAlwaysZero) {
        *pbSegAlwaysZero = TRUE;
    }

    memset(lpaddr, 0, sizeof(*lpaddr));

    if (lpsz == NULL) {
        return CPGENERAL;
    }

    switch( strlen(lpsz) ) {
    case 9:
        for (i=0; i<9; i++) {
            switch( i ) {
            case 0:
            case 1:
            case 2:
            case 3:
                if (pbSegAlwaysZero) {
                    *pbSegAlwaysZero = FALSE;
                }

                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    seg = seg * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    seg = seg * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    seg = seg * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return CPGENERAL;
                }
                break;

            case 4:
                if (lpsz[i] == '#') {
                    ADDR_IS_REAL(*lpaddr) = TRUE;
                } else if (lpsz[i] != ':') {
                    return CPGENERAL;
                }
                break;

            case 5:
            case 6:
            case 7:
            case 8:
                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    off = off * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    off = off * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    off = off * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return CPGENERAL;
                }
                break;
            }
        }

        SetAddrSeg(lpaddr, seg);
        SE_SetAddrOff(lpaddr, off);
        break;


    case 13:
        if (lpsz[1] == 'x') {
            for (i=0; i<13; i++) {
                switch( i ) {
                case 0:
                case 7:
                    if (lpsz[i] != '0') {
                        return CPGENERAL;
                    }
                    break;

                case 1:
                case 8:
                    if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                        return CPGENERAL;
                    }
                    break;

                case 2:
                case 3:
                case 4:
                case 5:
                    if (pbSegAlwaysZero) {
                        *pbSegAlwaysZero = FALSE;
                    }

                    if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                        seg = seg * 16 + lpsz[i] - '0';
                    } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                        seg = seg * 16 + lpsz[i] - 'a' + 10;
                    } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                        seg = seg * 16 + lpsz[i] - 'A' + 10;
                    } else {
                        return CPGENERAL;
                    }
                    break;

                case 6:
                    if (lpsz[i] == '#') {
                        ADDR_IS_REAL(*lpaddr) = TRUE;
                    } else if (lpsz[i] != ':') {
                        return CPGENERAL;
                    }
                    break;

                case 9:
                case 10:
                case 11:
                case 12:
                    if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                        off = off * 16 + lpsz[i] - '0';
                    } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                        off = off * 16 + lpsz[i] - 'a' + 10;
                    } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                        off = off * 16 + lpsz[i] - 'A' + 10;
                    } else {
                        return CPGENERAL;
                    }
                    break;
                }
            }

            SetAddrSeg(lpaddr, seg);
            SE_SetAddrOff(lpaddr, off);
        } else {
            for (i=0; i<13; i++) {
                switch( i ) {
                case 0:
                case 1:
                case 2:
                case 3:
                    if (pbSegAlwaysZero) {
                        *pbSegAlwaysZero = FALSE;
                    }

                    if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                        seg = seg * 16 + lpsz[i] - '0';
                    } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                        seg = seg * 16 + lpsz[i] - 'a' + 10;
                    } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                        seg = seg * 16 + lpsz[i] - 'A' + 10;
                    } else {
                        return CPGENERAL;
                    }
                    break;

                case 4:
                    if (lpsz[i] == '#') {
                        fReal = TRUE;
                    } else if (lpsz[i] != ':') {
                        return CPGENERAL;
                    }
                    break;

                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                    if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                        off = off * 16 + lpsz[i] - '0';
                    } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                        off = off * 16 + lpsz[i] - 'a' + 10;
                    } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                        off = off * 16 + lpsz[i] - 'A' + 10;
                    } else {
                        return CPGENERAL;
                    }
                    break;
                }
            }

            SetAddrSeg(lpaddr, seg);
            SE_SetAddrOff(lpaddr, off);
            ADDR_IS_OFF32(*lpaddr) = TRUE;
            ADDR_IS_FLAT(*lpaddr) = FALSE;
        }
        break;



    case 17:
        for (i=0; i<17; i++) {
            switch( i ) {
            case 0:
            case 7:
                if (lpsz[i] != '0') {
                    return CPGENERAL;

                }
                break;

            case 1:
            case 8:
                if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                    return CPGENERAL;
                }
                break;

            case 2:
            case 3:
            case 4:
            case 5:
                if (pbSegAlwaysZero) {
                    *pbSegAlwaysZero = FALSE;
                }

                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    seg = seg * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    seg = seg * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    seg = seg * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return CPGENERAL;
                }
                break;

            case 6:
                if (lpsz[i] != ':') {
                    return CPGENERAL;
                }
                break;

            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    off = off * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    off = off * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    off = off * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return CPGENERAL;
                }
                break;
            }
        }

        SetAddrSeg(lpaddr, seg);
        SE_SetAddrOff(lpaddr, off);
        ADDR_IS_OFF32(*lpaddr) = TRUE;
        ADDR_IS_FLAT(*lpaddr) = TRUE;
        break;


    case 10:
        for (i=0; i<10; i++) {
            switch( i ) {
            case 0:
                if (lpsz[i] != '0') {
                    return CPGENERAL;

                }
                break;

            case 1:
                if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                    return CPGENERAL;
                }
                break;

            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    off = off * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    off = off * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    off = off * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return CPGENERAL;
                }
                break;
            }
        }

        SetAddrSeg(lpaddr, seg);
        SE_SetAddrOff(lpaddr, SE32To64(off) );
        ADDR_IS_OFF32(*lpaddr) = TRUE;
        ADDR_IS_FLAT(*lpaddr) = TRUE;
        break;


    case 8:
        for (i=0; i<8; i++) {
            if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                off = off * 16 + lpsz[i] - '0';
            } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                off = off * 16 + lpsz[i] - 'a' + 10;
            } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                off = off * 16 + lpsz[i] - 'A' + 10;
            } else {
                return CPGENERAL;
            }
        }

        SetAddrSeg(lpaddr, seg);
        SE_SetAddrOff(lpaddr, SE32To64(off) );
        ADDR_IS_OFF32(*lpaddr) = TRUE;
        ADDR_IS_FLAT(*lpaddr) = TRUE;
        break;


    case 18:
        for (i=0; i<18; i++) {
            if (i == 0) {
                if (lpsz[i] != '0') {
                    return CPGENERAL;
                }
            } else if (i==1) {
                if ((lpsz[i] != 'x') && (lpsz[i] != 'X')) {
                    return CPGENERAL;
                }
            } else {
                if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                    off = off * 16 + lpsz[i] - '0';
                } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                    off = off * 16 + lpsz[i] - 'a' + 10;
                } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                    off = off * 16 + lpsz[i] - 'A' + 10;
                } else {
                    return CPGENERAL;
                }
            }
        }

        SetAddrSeg(lpaddr, seg);
        SE_SetAddrOff(lpaddr, off);
        ADDR_IS_OFF32(*lpaddr) = TRUE; //v-vadimp ????? may be FALSE(i.e. 64-bit), same below
        ADDR_IS_FLAT(*lpaddr) = TRUE;
        break;


    case 16:
        for (i=0; i<16; i++) {
            if (('0' <= lpsz[i]) && (lpsz[i] <= '9')) {
                off = off * 16 + lpsz[i] - '0';
            } else if (('a' <= lpsz[i]) && lpsz[i] <= 'f') {
                off = off * 16 + lpsz[i] - 'a' + 10;
            } else if (('A' <= lpsz[i]) && lpsz[i] <= 'F') {
                off = off * 16 + lpsz[i] - 'A' + 10;
            } else {
                return CPGENERAL;
            }
        }

        SetAddrSeg(lpaddr, seg);
        SE_SetAddrOff(lpaddr, off);
        ADDR_IS_OFF32(*lpaddr) = TRUE;
        ADDR_IS_FLAT(*lpaddr) = TRUE;
        break;


    default:
        return CPGENERAL;
    }

    return CPNOERROR;
}
                               /* EEUnFormatAddr() */



CPSTATUS
CPFormatEnumerate(
    DWORD      iFmt,
    LPDWORD    lpcBits,
    FMTTYPE *  lpFmtType,
    PEERADIX   lpRadix,
    LPDWORD    lpFTwoFields,
    LPDWORD    lpcchMax,
    LPSTR *    lplpszDesc
)
{
    if (iFmt >= sizeof(RgFormats)/sizeof(struct format)) {
        return CPGENERAL;
    }

    *lpcBits = RgFormats[iFmt].cBits;
    *lpFmtType = RgFormats[iFmt].fmtType;
    *lpRadix = RgFormats[iFmt].radix;
    *lpFTwoFields = RgFormats[iFmt].fTwoFields;
    *lpcchMax = RgFormats[iFmt].cchMax;
    if (lplpszDesc != NULL) {
        *lplpszDesc = &RgFormats[iFmt].lpszDescription[0];
    }

    return CPNOERROR;
}                   /* CPFormatEnumerate() */
