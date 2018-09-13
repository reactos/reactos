/***    deblex.c - lexer module for expression evaluator
 *
 *       Lexer routines for expression evaluator.
 */


#include "debexpr.h"
#include <limits.h>
#include "ldouble.h"
#include "locale.h"

ulong  ParseOp (uchar *, token_t *);
ulong  CanonOp (uchar *, ptoken_t);
ulong  ParseIntConst (uchar *, ptoken_t, uint, UQUAD *);
ulong  ParseFloatConst (uchar *, ptoken_t);
ulong  ParseIdent (uchar *, ptoken_t, bool_t);
ulong  ParseChar (uchar *, ptoken_t);
ulong  ParseString (uchar *, ptoken_t);
ulong  FakeIdent (uchar *pb, ptoken_t pTok);
bool_t FInRadix (char, uint);

struct Op {
    char    str[5];
} SEGBASED (_segname("_CODE")) OpStr[] = {
    {"\x003""->*"},
    {"\x003"">>="},
    {"\x003""<<="},
    {"\x002""+="},
    {"\x002""-="},
    {"\x002""*="},
    {"\x002""/="},
    {"\x002""%="},
    {"\x002""^="},
    {"\x002""&="},
    {"\x002""|="},
    {"\x002""<<"},
    {"\x002"">>"},
    {"\x002""=="},
    {"\x002""!="},
    {"\x002""<="},
    {"\x002"">="},
    {"\x002""&&"},
    {"\x002""||"},
    {"\x002""++"},
    {"\x002""--"},
    {"\x002""->"},
    {"\x001""+"},
    {"\x001""-"},
    {"\x001""*"},
    {"\x001""/"},
    {"\x001""%"},
    {"\x001""^"},
    {"\x001""&"},
    {"\x001""|"},
    {"\x001""~"},
    {"\x001""!"},
    {"\x001""="},
    {"\x001""<"},
    {"\x001"">"},
    {"\x001"","},
};

#define OPCNT  (sizeof (OpStr)/sizeof (struct Op))




/***    GetDBToken - Fetch next token from expression string
 *
 *      status = GetDBToken (pbExpr, ptoken, radix, oper)
 *
 *      Entry   pbExpr = far pointer to expression string
 *              ptoken = pointer to token return location
 *              radix = default radix for numeric conversion
 *              oper = previous operator
 *
 *      Exit    *ptoken = token as lexed from input string.  If an
 *              error occurred, the token will be of type OP_badtok.
 *              If the token is a constant, its value will be determined and
 *              placed in the token's 'val' field, and its type
 *              (e.g., T_USHORT) will be placed in the token's 'typ' field.
 *              If the previous operator is ., ->, ::, then ~ as the next
 *              character is taken to be part of an identifier
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered in parse
 *
 *      Calls the appropriate routine to lex the input string.  Handles:
 *
 *      foo             Identifiers                 OP_ident
 *      +, -, etc.      Operators                   OP_...
 *      345             Decimal constants           OP_const
 *      0123            Octal constants             OP_const
 *      0xABCD          Hexadecimal constants       OP_const
 *      'a', '\n'       Character constants         OP_const
 *      "foo"           String constants            OP_const
 *      L"foo"          Wide string constants       OP_const
 *      3.45            Floating point constants    OP_const
 *      HSYM_MARKER     handle to symbol follows    OP_ident
 *
 *      The handle to symbol is a hack to make sure that an expression
 *      can be generated from and locked to the handle to symbol that is
 *      passed to EEGetTMFromHSYM by the kernel
 */


ulong  GetDBToken (uchar *pbExpr, ptoken_t pTok, uint radix, op_t oper)
{
    uchar       c;
    uchar               *pbSave = pbExpr;
    ulong       error;
        uint            tokLen;

    memset (pTok, 0, sizeof (token_t));
    pTok->opTok = OP_badtok;
    pTok->pbTok = (char *)pbExpr;
    c = *pbExpr;
    if (c == '~') {
        switch (oper) {
            case OP_dot:
            case OP_pointsto:
            case OP_uscope:
            case OP_bscope:
            case OP_pmember:
            case OP_dotmember:
                error = ParseIdent (pbExpr, pTok, TRUE);
                                tokLen = (uint) (pTok->pbEnd - pTok->pbTok);
                                if ( tokLen > 255 ) //
                                {
                                        error = ERR_SYNTAX;
                                }
                                else
                                {
                                        pTok->cbTok = (uchar)tokLen;
                                }
                return (error);
        }
    }

    if (_istdigit (c)) {
        error = ParseConst (pbExpr, pTok, radix);
    }
    else if ((c == '\'') || ((c == 'L') && (pbExpr[1] == '\''))) {
        error = ParseChar (pbExpr, pTok);
    }
    else if (((c == 'L') && (pbExpr[1] == '"')) || (c == '"')) {
        error = ParseString (pbExpr, pTok);
    }
    else if ((_istcsymf(c)) || (c == '?') || (c == '$') || (c == '@')) {
        error = ParseIdent (pbExpr, pTok, FALSE);
    }
    else if (c == '.') {

        c =  *(pbExpr+1);

        if ( (c == 0) || (c == '+') || (c=='-') || (c==')')) {
            error = ParseIdent (pbExpr, pTok, FALSE);
        } else if (_istdigit(c) ) {
            error = ParseConst (pbExpr, pTok, radix);
        } else {
            error = ParseOp (pbExpr, pTok);
        }
    }
    else if (c == HSYM_MARKER) {
        error = FakeIdent (pbExpr, pTok);
    }
    else {
        error = ParseOp (pbExpr, pTok);
    }

        tokLen = (uint) (pTok->pbEnd - pTok->pbTok);
        if ( tokLen > 255 ) //
        {
                error = ERR_SYNTAX;
        }
        else
        {
                pTok->cbTok = (uchar)tokLen;
        }

    // note that caller must compute index of token

    return (error);
}




/**     ParseConst - Parse an integer or floating point constant string
 *
 *      error = ParseConst (pb, pTok, radix);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *              radix = default radix for numeric conversion
 *
 *      Exit    *pTok initialized for constant
 *              pTok->pbEnd = end of token
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 */


ulong  ParseConst (uchar *pb, ptoken_t pTok, uint radixin)
{
    uint        radix = radixin;
    bool_t      fUSuffix = FALSE;
    bool_t      fLSuffix = FALSE;
    bool_t      fFitsInt = FALSE;
    bool_t      fFitsUint = FALSE;
    bool_t      fFitsLong = FALSE;
    bool_t      fFitsULong = FALSE;
    bool_t      fFitsQuad = FALSE;
    UQUAD       value;
    CV_typ_t    typ;
    ulong       error;

    // check beginning of constant for radix specifiers

    if ((*pb == '0') && (*(pb + 1) != '.')) {
        pb++;
        if (_totupper (*pb) == 'X') {
            // Hex constant 0x.......
            radix = 16;
            ++pb;
        }
        else if (_totupper (*pb) == 'N') {
            // Decimal constant 0n.......
            radix = 10;
            ++pb;
        }
        else {
            // Octal constant 0........
            radix = 8;
            --pb;
        }
    }

    //
    // handle 0.xxx first:
    //

    if (*pb == '.') {

        return (ParseFloatConst (pb, pTok));


        //
        // Handle asm constants [0-9]xxxh
        //
    } else if (FInRadix(*pb, 10) &&
               (error = ParseIntConst (pb, pTok, 16, &value)) == ERR_NONE &&
               *pTok->pbEnd == 'h') {
        //
        // it worked: keep it and skip over the U and L check.
        //
        pb = pTok->pbEnd + 1;
        goto SkipUL;

    } else if (!FInRadix (*pb, radix)) {

        return (ERR_SYNTAX);

    } else if ((error = ParseIntConst (pb, pTok, radix, &value)) != ERR_NONE) {

        // error parsing as integer constant
        return (error);

    } else if ((*pTok->pbEnd == '.') || (_totupper (*pTok->pbEnd) == 'E')) {

        // Back up and reparse string as floating point
        return (ParseFloatConst (pb, pTok));

    }

    // Check for the 'u' and 'l' modifiers.

    pb = (uchar *) pTok->pbEnd;
    if (_totupper(*pb) == 'U') {
        ++pb;
        fUSuffix = TRUE;
        if (_totupper(*pb) == 'L') {
            ++pb;
            fLSuffix = TRUE;
        }
    }
    else if (_totupper(*pb) == 'L') {
        ++pb;
        fLSuffix = TRUE;
        if (_totupper(*pb) == 'U') {
            ++pb;
            fUSuffix = TRUE;
        }
    }

SkipUL:

    // ANSI spec, section 3.1.3.2:
    //
    // The type of an integer constant is the first of the corresponding
    // list in which its value can be represented.  Unsuffixed decimal:
    // int, long int, unsigned long int; unsuffixed octal or hexadecimal:
    // int, unsigned int, long int, unsigned long int; suffixed by the
    // letter u or U: unsigned int, unsigned long int; suffixed by the
    // letter l or L: long int, unsigned long int; suffixed by both the
    // letters u or U and l or L: unsigned long int.

    if (value < 0x8000L) {
        fFitsInt = TRUE;
    }
    if (value < 0x10000L) {
        fFitsUint = TRUE;
    }
    if (value < 0x80000000L) {
        fFitsLong = TRUE;
    }
    if (value < (UQUAD) 0x100000000) {
        fFitsULong = TRUE;
    }
    if (value < (UQUAD) 0x800000000) {
        fFitsQuad = TRUE;
    }

    if (fFitsULong) {
        typ = T_ULONG;

        if ((fUSuffix) && (fLSuffix)) {
            ;
        }
        else if (pExState->state.f32bit) {
            if (fUSuffix) {
                typ = T_UINT4;
            }
            else if (fLSuffix) {
                if (fFitsLong) {
                    typ = T_LONG;
                }
            }
            else {
                if (fFitsLong) {
                    typ = T_INT4;
                }
                else if (radix != 10) {
                    typ = T_UINT4;
                }
            }
        }
        else if (fUSuffix) {
            if (fFitsUint) {
                typ = T_UINT2;
            }
        }
        else if (fLSuffix) {
            if (fFitsLong) {
                typ = T_LONG;
            }
        }
        else {
            if (fFitsInt) {
                typ = T_INT2;
            }
            else if ((fFitsUint) && (radix != 10)) {
                typ = T_UINT2;
            }
            else if (fFitsLong) {
                typ = T_LONG;
            }
        }
    }
    else {
        // we have a 64 bit integral value. This can only
        // be T_QUAD or T_UQUAD, since "really 64bit int types"
        // i.e., T_INT8, T_UINT8, etc. are not supported by
        // the compiler (ints are 32 bits long in the 32 bit
        // world, and 16 bits long in the 16 bit world).
        typ = T_UQUAD;
        if (fFitsQuad && !fUSuffix) {
            typ = T_QUAD;
        }
    }

    pTok->typ = typ;
    pTok->opTok = OP_const;
    pTok->pbEnd = (char *) pb;
    VAL_UQUAD (pTok) = value;
    return (ERR_NONE);
}




/***    ParseIntConst - Parse an integer constant
 *
 *      error = ParseIntConst (pb, pTok, radix, pval)
 *
 *      Entry   pb = pointer to pointer to input string
 *              pTok = pointer to token return
 *              radix = radix (8, 10, 16)
 *              pval = pointer to ulong for value of constant
 *
 *      Exit    pTok updated to reflect token
 *
 *      Returns ERR_NONE if the input string was successfully parsed as an integer
 *              constant with the given radix
 *              ERR_... if error.
 */


ulong
ParseIntConst (
    uchar *pb,
    ptoken_t pTok,
    uint radix,
    UQUAD *pval
    )
{
    uchar       c;
    uint        nextDigit ;
    UQUAD       maxvalue =  _UI64_MAX / radix;

    *pval = 0L;

    for (;;) {
        c = *pb;

        if (((radix > 10) && !_istxdigit (c)) ||
          ((radix <= 10) && !_istdigit (c))) {
            // Must have reached the end
            break;
        }
        if (*pval > maxvalue) {
            // number will overflow
            return (ERR_CONSTANT);
        }

        if (!FInRadix(c, radix)) {
            return (ERR_SYNTAX);
        }

        *pval *= radix;

        if (_istdigit (c = *pb)) {
            nextDigit = (c - '0');
        }
        else {
            nextDigit = (_totupper(c) - 'A' + 10);
        }

        if (( _UI64_MAX - *pval ) < nextDigit ) {
            // [cuda:3093 9th Apr 93 sanjays] - additional overflow chk reqd.
            // number will overflow.
            return (ERR_CONSTANT);
        }
        else {
            *pval += nextDigit ;
        }
        pb = (uchar *) _tcsinc((_TXCHAR *)pb);
    }
    pTok->pbEnd = (char *) pb;
    return (ERR_NONE);
}




/**     ParseFloatConst - Parse a floating-point constant
 *
 *      fSuccess = ParseFloatConst (pb, pTok);
 *
 *      Entry   pb = pointer to input string
 *              pTok = pointer to parse token structure
 *
 *      Exit    pTok updated to reflect floating point number if one
 *              is found.
 *
 *      Returns ERR_NONE if no error encountered
 *              ERR_... error
 */


ulong
ParseFloatConst (
    uchar *pb,
    ptoken_t pTok
    )
{
    char       *pEnd;
    char        chSave;
    CV_typ_t    typ;
    int         cbbuf;

    // check for a single '.' - strtold returns 0 in such a case

    if (((*pb == '.') && (!_istdigit (*(pb + 1)))) ||
        ((cbbuf = _tcslen ((TCHAR *)pb)) >= 100))  {
        return (ERR_SYNTAX);
    }

    // Call strtold () to figure out the value.  This will also
    // return a pointer to the first character which is not
    // part of the value -- this allows us to check for an
    // 'f' or 'l' suffix character:
    //
    // ANSI, Section 3.1.3.1:
    //
    // "An unsuffixed floating constant has type double.
    // If suffixed by the letter f or F, it has type float.
    // If suffixed by the letter l or L, it has type long double."

        // always parse numbers in English (especially periods)
        setlocale( LC_NUMERIC, "C" );

#if defined(WIN32) && !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_IA64_) 
    VAL_LDOUBLE(pTok) = LdFromSz (pb, &pEnd);
#else
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_IA64_) 
    VAL_DOUBLE(pTok) = _tcstod(pb, &pEnd);
#else
    VAL_LDOUBLE(pTok) = _strtold (pb, &pEnd);
#endif
#endif

        setlocale( LC_NUMERIC, "" );                            // revert back to user locale

    if (pEnd == pb) {
        // Call to strtold () failed
        return (ERR_SYNTAX);
    }

    // Save and check char

    pb += pEnd - pb;
    chSave = *pb;
    if (_totupper (chSave) == 'F') {
        pb++;
        typ = T_REAL32;
#if defined(_MIPS_) || defined(_ALPHA_)
        VAL_FLOAT(pTok) = (float) VAL_DOUBLE(pTok);
#else
        VAL_FLOAT(pTok) = FloatFromFloat10 ( VAL_LDOUBLE(pTok) );
#endif
    }
    else if (_totupper(chSave) == 'L') {
        pb++;
#if defined(_MIPS_) || defined(_ALPHA_)
        typ = T_REAL64;
#else
        typ = T_REAL80;
#endif
    }
    else {
        typ = T_REAL64;
#if !defined(_MIPS_) && !defined(_ALPHA_)
        VAL_DOUBLE(pTok) = DoubleFromFloat10 ( VAL_LDOUBLE(pTok) );
#endif
    }
    pTok->opTok = OP_const;
    pTok->typ = typ;
    pTok->pbEnd = (char *) pb;
    return (ERR_NONE);
}




/***    FakeIdent - Fake an identifier from handle to symbol
 *
 *      error = FakeIdent (pb, pTok);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *
 *      Exit    *pTok initialized for identifier fro handle to symbol
 *              pTok->pbEnd = end of token (first non-identifier character)
 *
 *      Returns ERR_NONE
 *
 */


ulong  FakeIdent (uchar *pb, ptoken_t pTok)
{
    pTok->opTok = OP_hsym;
    pTok->pbEnd = (char *) (pb + sizeof (char) + HSYM_CODE_LEN);
    return (ERR_NONE);
}





/***    ParseIdent - Parse an identifier
 *
 *      error = ParseIdent (pb, pTok, fTilde);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *              fTilde = TRUE if ~ acceptable as first character
 *
 *      Exit    *pTok initialized for identifier
 *              pTok->pbEnd = end of token (first non-identifier character)
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 *       Also handles the 'sizeof', 'by', 'wo' and 'dw' operators, since these
 *       look like identifiers.
 *
 */


ulong  ParseIdent (uchar *pb, ptoken_t pTok, bool_t fTilde)
{
    int         len;

    if ( *pb == '.' ) {
        ++pb;
        pTok->opTok = OP_ident;
        pTok->pbEnd = (char*)pb;
    } else if ((_istcsymf(*pb)) || (*pb == '?') || (*pb == '$') ||
               (*pb == '@') || ((*pb == '~') && (fTilde == TRUE))) {
        ++pb;
        while ((_istcsym(*pb)) || (*pb == '?') || (*pb == '$') ||
          (*pb == '@')) {
            ++pb;
        }
        pTok->opTok = OP_ident;
        pTok->pbEnd = (char *) pb;
    }

    // Check for the 'operator', 'sizeof', 'by', 'wo' and 'dw' operators.

    if ((len = (int) (pTok->pbEnd - pTok->pbTok)) == 6) {
        if (_tcsncmp (pTok->pbTok, "sizeof", 6) == 0) {
            pTok->opTok = OP_sizeof;
        }
    }
    else if (len == 8) {
        if (_tcsncmp (pTok->pbTok, "operator", 8) == 0) {
            // allow for operator op
            return (CanonOp (pb, pTok));
        }
    }
    else if (len == 2) {
        // Could be 'by', 'wo' or 'dw'...
        if (_tcsnicmp (pTok->pbTok, "BY", 2) == 0) {
            pTok->opTok = OP_by;
        }
        else if (_tcsnicmp (pTok->pbTok, "WO", 2) == 0) {
            pTok->opTok = OP_wo;
        }
        else if (_tcsnicmp (pTok->pbTok, "DW", 2) == 0) {
            pTok->opTok = OP_dw;
        }
    }
    // check for special token denoting return value of the
    // current function
    else if (len == 12) {
        if (_tcsnicmp (pTok->pbTok, "$ReturnValue", 12) == 0) {
            pTok->opTok = OP_retval;
        }
    }
    return (ERR_NONE);
}




/**     CanonOp - canonicalize operator string
 *
 *      error = CanonOp (pb, pTok)
 *
 *      Entry   pb = pointer to first character after "operator"
 *
 *      Exit    string rewritten to ripple excess white space to the right
 *              pTok updated to reflect total function name
 *              pb points to '(' of function call
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error
 */


ulong  CanonOp (uchar *pb, ptoken_t pTok)
{
    uchar *pOp = pb;
    uchar *pTemp;
    int         i;

    while (_istspace (*pb)) {
        pb++;
    }
    if (*pb == 0) {
        return (ERR_SYNTAX);
    }
    if (_istalpha (*pb)) {
        // process new, delete
        // process
        //   {[const &| volatile] id [const &| volatile]}
        //     [{\*[const &| volatile]}*[{\&[const &| volatile]}]]
        //
        //  Note that the current code only processes a single id
        //  new (), delete () and type () will pass.  All others will
        //  cause a syntax error later.

        pTemp = pb;
        while (_istalpha (*pTemp)) {
            // skip to end of alpha string
            pTemp++;
        }
        *pOp++ = ' ';
        memmove (pOp, pb, (size_t) (pTemp - pb));
        pOp += pTemp - pb;
        pb = pTemp;
    }
    else if (*pb == '(') {
        // process "(    )"
        pb++;
        while (*pb++ != ')') {
            if (!_istspace (*pb)) {
                return (ERR_SYNTAX);
            }
        }
        *pOp++ = '(';
        *pOp++ = ')';
    }
    else if (*pb == '[') {
        // process "[    ]"
        pb++;
        while (*pb++ != ']') {
            if (!_istspace (*pb)) {
                return (ERR_SYNTAX);
            }
        }
        *pOp++ = '[';
        *pOp++ = ']';
    }
    else {
        // process operator strings
        for ( i = 0; i < OPCNT; i++) {
            if (_tcsncmp (OpStr[i].str + 1, (_TXCHAR *) pb, OpStr[i].str[0]) == 0) {
                break;
            }
        }
        if (i == OPCNT) {
            return (ERR_SYNTAX);
        }
        memmove (pOp, OpStr[i].str + 1, OpStr[i].str[0]);
        pOp += OpStr[i].str[0];
        pb += OpStr[i].str[0];
    }

    // blank out moved characters

    pTok->pbEnd = (char *) pOp;
    while (pOp < pb) {
        *pOp++ = ' ';
    }

    // skip to the next token and check to make sure it is a (
    // the zero check is to allow "bp operator +"

    while (_istspace (*pb)) {
        pb++;
    }
    if ((*pb == '(') || (*pb == 0)) {
        return (ERR_NONE);
    }
    else {
        return (ERR_SYNTAX);
    }
}


/**    GetEscapedChar - Parse an escaped character
 *
 *      error = GetEscapedChar (ppb, pVal);
 *
 *      Entry   ppb = far pointer to far pointer to string. ppb points to
 *              character after the \
 *
 *      Exit    ppb updated to end of escaped character constant
 *              *pVal = value of escaped character constant
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 */



ulong
GetEscapedChar (
    char * *ppb,
    ulong  *pcbChar,
    ushort *pVal
    )
{
    char    c;
    uint    nval = 0;
    char   *pbT = *ppb;

    c = (char)(**(TCHAR **)ppb);
    *ppb = _tcsinc (*ppb);
    *pcbChar = (ulong) (*ppb - pbT);

    switch (c) {
        case 'n':
                        if (TargetMachine == mptmppc)
                                *pVal = '\r';
                        else
                                *pVal = '\n';
            break;

        case 't':
            *pVal = '\t';
            break;

        case 'b':
            *pVal = '\b';
            break;

        case 'r':
                        if (TargetMachine == mptmppc)
                                *pVal = '\n';
                        else
                                *pVal = '\r';
            break;

        case 'f':
            *pVal = '\f';
            break;

        case 'v':
            *pVal = '\v';
            break;

        case 'a':
            *pVal = '\a';
            break;

        case 'x':
            c = (char)(**(TCHAR **)ppb);

            if (!FInRadix (c, 16)) {
                return (ERR_SYNTAX);
            }
            for (;;) {
                c = (char)(**(TCHAR **)ppb);
                if (!FInRadix (c, 16)) {
                    break;
                }
                nval *= 16;
                if (_istdigit ((_TUCHAR)c)) {
                    nval += c - '0';
                }
                else {
                    nval += _totupper((_TUCHAR)c) - 'A' + 10;
                }
                if (nval > 255) {
                    return (ERR_CONSTANT);
                }
                *ppb = _tcsinc (*ppb);
            }
            *pVal = (uchar)nval;
            break;

        default:
            if (FInRadix (c, 8)) {
                // Octal character constant
                nval = (c - '0');
                for (;;) {
                    c = (char)(**(TCHAR **)ppb);
                    if (!_istdigit ((_TUCHAR)c)) {
                        break;
                    }
                    if (!FInRadix (c, 8)) {
                        return (ERR_SYNTAX);
                    }
                    nval = nval * 8 + (c - '0');
                    if (nval > 255) {
                        return (ERR_CONSTANT);
                    }
                    *ppb = _tcsinc (*ppb);
                }
                *pVal = (uchar)nval;
            }
            else {
                *pVal = c;
            }
            break;
    }
    return (ERR_NONE);
}




/**    ParseChar - Parse an character constant
 *
 *      error = ParseChar (pb, pTok);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *
 *      Exit    *pTok initialized for character constant
 *              pTok->pbEnd = end of token
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 */



ulong
ParseChar (
    uchar *pb,
    ptoken_t pTok
    )
{
    unsigned int    value = 0;
    ulong           cbTotal = 0;
    ulong           retval;

    if (*(TCHAR *)pb == _T('L') ) {
        // skip initial L if L'
        pb = (uchar *) _tcsinc ((_TXCHAR *) pb);
    }

    DASSERT( *(TCHAR *)pb == _T('\'') );
    pb = (uchar *) _tcsinc ((_TXCHAR *)pb);

    if ((*(TCHAR *)pb == _T('\'') ) || (*(TCHAR *)pb == 0)) {
        return (ERR_SYNTAX);
    }
    while ((*(TCHAR *)pb != _T('\'') ) && (*(TCHAR *)pb != 0)) {
        uint    c = 0;
        uchar * pbT;
        ulong   cbChar;

        pbT = pb;
        pb = (uchar *) _tcsinc ((_TXCHAR *)pb);
        cbChar = (ulong) (pb - pbT);

        switch ( cbChar ) {
            case 1:
                c = *(uchar *)pbT;
                break;
            case 2:
                c = *(ushort *)pbT;
                break;
            default:
                pTok->opTok = OP_badtok;
                return ( ERR_CONSTANT );
        }

        if ( c == _T('\\') ) {
            ushort escVal;

            // Escaped character constant
            if ((retval = GetEscapedChar ((_TXCHAR **)&pb, &cbChar, &escVal)) != ERR_NONE) {
                return (retval);
            }
            c = (TCHAR)escVal;
        }

        value = (value << (cbChar * 8)) | (ushort)c;
        cbTotal += cbChar;
    }

    if ( *(TCHAR *)pb != _T('\'') ) {
        return (ERR_MISSINGSQ);
    }
    pb = (uchar *) _tcsinc ((_TXCHAR *) pb);
    pTok->opTok = OP_const;

    switch ( cbTotal ) {
        case 1:
            VAL_CHAR(pTok) = (char) value;
            pTok->typ = T_RCHAR;
            break;
        case 2:
            VAL_SHORT(pTok) = (short)value;
            pTok->typ = T_SHORT;
            break;
        case 3:
        case 4:
            VAL_LONG(pTok) = (long)value;
            pTok->typ = T_LONG;
            break;
        default:
            pTok->opTok = OP_badtok;
            return ( ERR_CONSTANT );
    }

    pTok->pbEnd = (char *) pb;
    return (ERR_NONE);
}




/**    ParseString - Parse a string constant "..." or L"..."
 *
 *      error = ParseString (pb, pTok, fWide);
 *
 *      Entry   pb = far pointer to string
 *              pTok = pointer to return token
 *
 *      Exit    *pTok initialized for string constant
 *              pTok->pbEnd = end of token
 *
 *      Returns ERR_NONE if no error
 *              ERR_... if error encountered
 *
 *      Note    The string pointer will point to the initial " or L"
 *              and the byte count will include beginning " or L" and the
 *              ending ".  The evaluator will have to adjust for the extra
 *              characters and store the proper data.
 */


ulong  ParseString (uchar *pb, ptoken_t pTok)
{
    if (*pb =='L') {
        // skip initial L if L"
        pb++;
    }

    // skip initial "
    DASSERT (*pb == '"');
    pb++;

    // search for ending double quote

    while ((*pb != 0) && (*pb != '"')) {
        if (*pb == '\\' && *(pb + 1) == '"') {
            pb++;
        }
        pb = (uchar *) _tcsinc((_TXCHAR *) pb);
    }
    if (!*pb) {
        // reached end of string
        return (ERR_MISSINGDQ);
    }
    pTok->opTok = OP_const;
    if (pExState->state.f32bit) {
        pTok->typ = T_32PRCHAR;
    }
    else {
        pTok->typ = T_PRCHAR;
    }
    pTok->pbEnd = (char *) (pb + 1);
    return (ERR_NONE);
}




/***    FInRadix - Is character appropriate for radix?
 *
 *      fOK = FInRadix (ch, radix)
 *
 *      Entry   ch = character to check
 *              radix = 8, 10, 16
 *
 *      Exit    none
 *
 *      Returns TRUE if character is in radix
 *              FALSE if not.
 *
 */


bool_t FInRadix (char ch, uint radix)
{
    switch (radix) {
        case 8:
            if (ch >= '8' || ch < '0') {
                return (FALSE);
            }
            // Fall through

        case 10:
            return (_istdigit((_TUCHAR)ch));

        case 16:
            return (_istxdigit((_TUCHAR)ch));

        default:
            DASSERT (FALSE);
            return (FALSE);
    }
}

