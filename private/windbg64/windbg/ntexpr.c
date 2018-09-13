/*** ntexpr.c - expression evaluator for NT debugger
*
*   Copyright <C> 1990, Microsoft Corporation
*
*   Purpose:
*       With the current command line at *pchCommand, parse
*       and evaluate the next expression.
*
*   Revision History:
*
*   [-]  18-Apr-1990 Richk      Created - split from ntcmd.c.
*
*************************************************************************/


#include "precomp.h"
#pragma hdrstop

#include "ntexpr.h"

#define INVALID_CLASS ((ULONG)-1)

ULONG   PeekToken(PLONG64);
ULONG   GetTokenSym(PLONG64);
ULONG   NextToken(PLONG64);
void    AcceptToken(void);
UCHAR   PeekChar(void);

ULONG64 GetCommonExpression(VOID);
void    GetLowerString(PUCHAR, ULONG);
LONG64  GetExpr(void);
LONG64  GetLRterm(void);
LONG64  GetLterm(void);
LONG64  GetAterm(void);
LONG64  GetMterm(void);
LONG64  GetTerm(void);
USHORT  addrExpression;
NTSDADDR    tempAddr;


static PUCHAR pchCommand;
static ULONG   savedClass;
static LONG64  savedValue;
static UCHAR   *savedpchCmd;

static ULONG64 EXPRLastExpression = 0;
static int     BaseDefault = 16;
static MPT     MptProcessorType;

static struct Res {
    UCHAR    chRes[3];
    ULONG    classRes;
    ULONG    valueRes;
} Reserved[] = {
    { 'o', 'r', '\0', LOGOP_CLASS, LOGOP_OR  },
    { 'b', 'y', '\0', UNOP_CLASS,  UNOP_BY   },
    { 'w', 'o', '\0', UNOP_CLASS,  UNOP_WO   },
    { 'd', 'w', '\0', UNOP_CLASS,  UNOP_DW   },
    { 'h', 'i', '\0', UNOP_CLASS,  UNOP_HI   },
    { 'm', 'o', 'd',  MULOP_CLASS, MULOP_MOD },
    { 'x', 'o', 'r',  LOGOP_CLASS, LOGOP_XOR },
    { 'a', 'n', 'd',  LOGOP_CLASS, LOGOP_AND },
    { 'p', 'o', 'i',  UNOP_CLASS,  UNOP_POI  },
    { 'n', 'o', 't',  UNOP_CLASS,  UNOP_NOT  },
    { 'l', 'o', 'w',  UNOP_CLASS,  UNOP_LOW  },
#if 0
    { 'e', 'a', 'x',  REG_CLASS,   REGEAX   },
    { 'e', 'b', 'x',  REG_CLASS,   REGEBX   },
    { 'e', 'c', 'x',  REG_CLASS,   REGECX   },
    { 'e', 'd', 'x',  REG_CLASS,   REGEDX   },
    { 'e', 'b', 'p',  REG_CLASS,   REGEBP   },
    { 'e', 's', 'p',  REG_CLASS,   REGESP   },
    { 'e', 'i', 'p',  REG_CLASS,   REGEIP   },
    { 'e', 's', 'i',  REG_CLASS,   REGESI   },
    { 'e', 'd', 'i',  REG_CLASS,   REGEDI   },
    { 'e', 'f', 'l',  REG_CLASS,   REGEFL   }
#endif
};

#define RESERVESIZE (sizeof(Reserved) / sizeof(struct Res))

static char * SegRegs[] = {
    "cs", "ds", "es", "fs", "gs", "ss"
};
#define SEGREGSIZE (sizeof(SegRegs)/sizeof(char *))





#ifdef KERNEL
extern USHORT DefaultProcessor;
#endif

void
error(
    ULONG_PTR errorval
    )
{
    RaiseException(NTEXPR_EXCEPTION, 0, 1, &errorval);
}

ULONG
GetExprFilter(
    LPEXCEPTION_POINTERS ep,
    PULONG64 status
    )
{
    if (ep->ExceptionRecord->ExceptionCode != NTEXPR_EXCEPTION) {
        return EXCEPTION_CONTINUE_SEARCH;
    } else {
        *status = ep->ExceptionRecord->ExceptionInformation[0];
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

ULONG
GetExpression (
    PUCHAR szExpr,
    PULONG64 pResult,
    int radix,
    MPT mptProcessorType,
    PUCHAR *lpNext
    )
/*++

Routine Description:

    read and evaluate the next possible expression from szExpr
    and return its value.  The expression is parsed and evaluated
    using a recursive descent method.

Arguments:

    szExpr - Supplies the expression string

    pResult - Returns the value of the expression

    radix - Supplies the default numeric radix

    mptProcessorType - Supplies the type of CPU for register recognition


Return Value:

    returns 0 if successful.  Non-zero values are error codes.

--*/
{
    ULONG64    value;
    ULONG64    result;

    _try {
        if (szExpr) {
            pchCommand = szExpr;
        }
        BaseDefault = radix;
        MptProcessorType = mptProcessorType;

        value = GetCommonExpression();

        *lpNext = pchCommand;

        EXPRLastExpression = value;
        *pResult = value;
        result = 0;
    } _except (GetExprFilter(GetExceptionInformation(), &result)) {
        ;
    }

    return (ULONG)result;
}

/*** GetAddrExpression - read and evaluate address expression
*
*   Purpose:
*       Used to get an address expression.
*
*   Returns:
*       Pointer to address packet
*
*   Exceptions:
*       error exit: SYNTAX - empty expression or premature end-of-line
*
*
*************************************************************************/
ULONG
GetAddrExpression (
    PUCHAR szExpr,
    PNTSDADDR Address,
    int radix,
    MPT mptProcessorType,
    ULONG defaultSeg,
    PUCHAR * lpNext
    )
{
    ULONG64 value;
    ULONG status;

    NotFlat(*Address);

    //  Do a normal GetExpression call

    status = GetExpression(szExpr, &value, radix, mptProcessorType, lpNext);
    if (status) {
        return status;
    }
    *Address = tempAddr;

    //  If it wasn't an explicit address expression
    //  force it to be an address

    if (!(addrExpression & ~INSTR_POINTER)) {

        if ( !Is64PtrSE(value) ) {
            value = SE32To64( value );
        }

        Off(*Address) = value;

#if 0
        if (mptProcessorType == mptix86) {
            addrExpression = Address->type =
                     fVm86 ? ADDR_V86 : (f16pm ? ADDR_16 : ADDR_FLAT);
            if ((addrExpression != ADDR_FLAT) && defaultSeg != -1L) {
                Address->seg = (USHORT)GetRegFlagValue(defaultSeg);
            }
        } else {
            addrExpression = Address->type = ADDR_FLAT;
        }
#else
        addrExpression = Address->type = ADDR_FLAT;
#endif
        ComputeFlatAddress(Address, NULL);

    } else if fnotFlat(*Address) {

        //  This case (i.e., addrExpression && !flat) results from
        //  an override (i.e., %,,&, or #) being used but no segment
        //  being specified to force a flat address computation.

        Type(*Address)= addrExpression;
        Off(*Address) = value;
        if (mptProcessorType == mptix86) {
            if (defaultSeg == -1L) {
                Address->seg = 0;
            } else {

                //  test flag for IP or EIP as register argument
                //      if so, use CS as default register

                if (fInstrPtr(*Address)) {
                    defaultSeg = GetRegString( (PUCHAR) "cs", NULL);
                }
                Address->seg = (USHORT)GetRegFlagValue(defaultSeg);
            }
        }
        ComputeFlatAddress(Address, NULL);
    }

    return 0;
}

/*** GetCommonExpression - read and evaluate expression
*
*   Purpose:
*       From the current command line position at pchCommand,
*       read and evaluate the next possible expression.
*       return either its value if a memory expression, or
*       its symbol file and line number structures if a source
*       line expression.
*
*   Input:
*       pchCommand - command line position
*
*   Outputs:
*       *ppSymfile - nonNULL for symbol file structure
*                    NULL for returning memory offset
*       *ppLineno - lineno structure
*       fSourceDefault - treat '.' as line number if TRUE
*                        else as program counter value in GetExpr.
*
*   Returns:
*       if (*ppSymfile) - line number (USHORT)
*       if (!*ppSymfile) - ULONG value of expression.
*   Exceptions:
*       error exit: SYNTAX - empty expression or premature end-of-line
*
*   Notes:
*       parsing for source line expressions is done here. the syntax is:
*
*               [<module>! [<file>[.<expr>]]]
*
*       if no <module> and/or <file> is/are specified, the current
*           program counter is used to determine the defaults.
*       if '.' is used, <expr> evaluates to a line number, else
*           a memory address is calculated.  NO WHITE SPACE BETWEEN
*           '.' AND <expr>.
*
*************************************************************************/

ULONG64
GetCommonExpression(
    VOID
    )
{
    PUCHAR       pchCommandSaved;
    UCHAR        chModule[40];
    UCHAR        chFilename[40];
    UCHAR        ch;
    ULONG64      value;
    PUCHAR       pchFilename;

    savedClass = INVALID_CLASS;
    pchCommandSaved = pchCommand;

    if (PeekChar() == '!') {
        pchCommand++;
    }

    GetLowerString(chModule, sizeof(chModule) );
    ch = PeekChar();

    //  if '!' is next, chModule has <module>, chFilename has <file>
    //      else copy chFilename to chModule and set chModule to NULL

    if (ch == '!') {
        pchCommand++;
        GetLowerString(chFilename, sizeof(chFilename));
        ch = PeekChar();
    } else {
        strcpy( (PSTR) chFilename, (PSTR) chModule);
        chModule[0] = '\0';
    }

    pchCommand = pchCommandSaved;

    ch = PeekChar();
    switch(ch) {
        case '&':
            pchCommand++;
            addrExpression = ADDR_V86;
            break;
        case '#':
            pchCommand++;
            addrExpression = ADDR_16;
            break;
        case '%':
            pchCommand++;
            addrExpression = ADDR_FLAT;
            break;
        default:
            addrExpression = FALSE;
            break;
        }
    ch = PeekChar();
    value = (ULONG64)GetExpr();

    return value;
}

void
GetLowerString (
    PUCHAR pchBuffer,
    ULONG cbBuffer
    )
{
    UCHAR   ch;

    ch = PeekChar();
    ch = (UCHAR)tolower(ch);
    while ( (ch == '_' 
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')
        || (ch >= '0' && ch <= '9')
        ) && --cbBuffer) {

        
        *pchBuffer++ = ch;
        ch = *++pchCommand;
    }
    *pchBuffer = '\0';
}

/*** GetExpr - Get expression
*
*   Purpose:
*       Parse logical-terms separated by logical operators into
*       expression value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<logic-op> <lterm>]*
*       <logic-op> = AND (&), OR (|), XOR (^)
*
*************************************************************************/

LONG64
GetExpr (
    void
    )
{
    LONG64  value1;
    LONG64  value2;
    ULONG   opclass;
    LONG64  opvalue;

    value1 = GetLRterm();
    while ((opclass = PeekToken(&opvalue)) == LOGOP_CLASS) {
        AcceptToken();
        value2 = GetLRterm();
        switch (opvalue) {
            case LOGOP_AND:
                value1 &= value2;
                break;
            case LOGOP_OR:
                value1 |= value2;
                break;
            case LOGOP_XOR:
                value1 ^= value2;
                break;
            default:
                error(ERR_SYNTAX);
        }
    }
    return value1;
}

/*** GetLRterm - get logical relational term
*
*   Purpose:
*       Parse logical-terms separated by logical relational
*       operators into the expression value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<rel-logic-op> <lterm>]*
*       <logic-op> = '==' or '=', '!=', '>', '<'
*
*************************************************************************/

LONG64
GetLRterm (
    )
{
    LONG64  value1;
    LONG64  value2;
    ULONG   opclass;
    LONG64  opvalue;

//dprintf("LONG GetLRterm ()\n");
    value1 = GetLterm();
    while ((opclass = PeekToken(&opvalue)) == LRELOP_CLASS) {
        AcceptToken();
        value2 = GetLterm();
        switch (opvalue) {
            case LRELOP_EQ:
                value1 = (value1 == value2);
                break;
            case LRELOP_NE:
                value1 = (value1 != value2);
                break;
            case LRELOP_LT:
                value1 = (value1 < value2);
                break;
            case LRELOP_GT:
                value1 = (value1 > value2);
                break;
            default:
                error(ERR_SYNTAX);
            }
        }
    return value1;
}

/*** GetLterm - get logical term
*
*   Purpose:
*       Parse additive-terms separated by additive operators into
*       logical term value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of sum.
*
*   Exceptions:
*       error exit: SYNTAX - bad logical term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <lterm> = <aterm> [<add-op> <aterm>]*
*       <add-op> = +, -
*
*************************************************************************/

LONG64
GetLterm (
    void
    )
{
    LONG64  value1 = GetAterm();
    LONG64  value2;
    ULONG   opclass;
    LONG64  opvalue;
    BOOL    faddr = (BOOL   ) addrExpression;

//dprintf("LONG GetLterm ()\n");
    while ((opclass = PeekToken(&opvalue)) == ADDOP_CLASS) {
        AcceptToken();
        value2 = GetAterm();
        if (!faddr && addrExpression) {
            LONG64 tmp = value1;
            value1 = value2;
            value2 = tmp;
        }
        if ( (MptProcessorType != mptix86 && addrExpression) ||
             (MptProcessorType == mptix86 && (addrExpression & ~INSTR_POINTER)) ) {

            switch (opvalue) {
                case ADDOP_PLUS:
                    AddrAdd(&tempAddr,value2);
                    value1 += value2;
                    break;
                case ADDOP_MINUS:
                    AddrSub(&tempAddr,value2);
                    value1 -= value2;
                    break;
                default:
                    error(ERR_SYNTAX);
            }
        }
        else {
            switch (opvalue) {
                case ADDOP_PLUS:
                    value1 += value2;
                    break;
                case ADDOP_MINUS:
                    value1 -= value2;
                    break;
                default:
                    error(ERR_SYNTAX);
            }
        }
    }
    return value1;
}

/*** GetAterm - get additive term
*
*   Purpose:
*       Parse multiplicative-terms separated by multipicative operators
*       into additive term value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of product.
*
*   Exceptions:
*       error exit: SYNTAX - bad additive term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <aterm> = <mterm> [<mult-op> <mterm>]*
*       <mult-op> = *, /, MOD (%)
*
*************************************************************************/

LONG64
GetAterm (
    void
    )

{
    LONG64  value1;
    LONG64  value2;
    ULONG   opclass;
    LONG64  opvalue;

//dprintf("LONG GetAterm ()\n");
    value1 = GetMterm();
    while ((opclass = PeekToken(&opvalue)) == MULOP_CLASS) {
        AcceptToken();
        value2 = GetAterm();
        switch (opvalue) {
            case MULOP_MULT:
                value1 *= value2;
                break;
            case MULOP_DIVIDE:
                value1 /= value2;
                break;
            case MULOP_MOD:
                value1 %= value2;
                break;
            case MULOP_SEG:{
                PXDESCRIPTOR_TABLE_ENTRY pdesc=NULL;
                XDESCRIPTOR_TABLE_ENTRY desc;

                if (addrExpression) {
                   Type(tempAddr) = addrExpression;
                } else {
                   // We don't know what kind of address this is
                   // Let's try to figure it out.
                    if (MptProcessorType != mptix86) {
                        addrExpression = Type(tempAddr) = ADDR_FLAT;
#if 0
                    } else if (fVm86) {
                        addrExpression = Type(tempAddr) = ADDR_V86;
#endif
                    } else {
                        desc.Selector = (DWORD)value1;
                        if (!LookupSelector(LptdCur, &desc)) {
                            error(ERR_BADSEG);
                        }
                        addrExpression = Type(tempAddr) =
                              (UCHAR)desc.Descriptor.HighWord.
                                     Bits.Default_Big? ADDR_1632:ADDR_16;
                        pdesc = &desc;
                    }
                }

                tempAddr.seg  = (USHORT)value1;
                tempAddr.off  = value2;
                ComputeFlatAddress(&tempAddr, pdesc);
                value1 = value2;
                }
                break;


                //
                // 64-bit address specifier.
                //
                // Treat identically to 32-bit address.
                //

            case MULOP_64:
                addrExpression = ADDR_FLAT;
                tempAddr.type = ADDR_FLAT;
                tempAddr.seg  = 0;

                tempAddr.off  = value2;
                ComputeFlatAddress(&tempAddr, NULL);
                value1 = value2;
                break;

            default:
                error(ERR_SYNTAX);
            }
        }

    return value1;
}

/*** GetMterm - get multiplicative term
*
*   Purpose:
*       Parse basic-terms optionally prefaced by one or more
*       unary operators into a multiplicative term.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of multiplicative term.
*
*   Exceptions:
*       error exit: SYNTAX - bad multiplicative term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <mterm> = [<unary-op>] <term> | <unary-op> <mterm>
*       <unary-op> = <add-op>, ~ (NOT), BY, WO, DW, HI, LOW
*
*************************************************************************/

LONG64
GetMterm (
    void
    )
{
    LONG64  value;
    USHORT  wvalue;
    UCHAR   bvalue;
    ULONG   opclass;
    LONG64  opvalue;

//dprintf("LONG GetMterm ()\n");
    if ((opclass = PeekToken(&opvalue)) != UNOP_CLASS &&
                                opclass != ADDOP_CLASS) {

        value = GetTerm();

    } else {

        AcceptToken();
        value = GetMterm();
        switch (opvalue) {
            case UNOP_NOT:
                value = !value;
                break;
            case UNOP_BY:
            case UNOP_WO:
            case UNOP_DW:
            case UNOP_POI:

                NotFlat(tempAddr);

                Off(tempAddr) = value;
                Type(tempAddr) = ADDR_FLAT;

                ComputeFlatAddress(&tempAddr, NULL);

                switch (opvalue) {
                    case UNOP_BY:
                        if (!GetMemByte(&tempAddr, &bvalue)) {
                            error(ERR_MEMORY);
                        }
                        value = (LONG)bvalue;
                        break;
                    case UNOP_WO:
                        if (!GetMemWord(&tempAddr, &wvalue)) {
                            error(ERR_MEMORY);
                        }
                        value = (LONG)wvalue;
                        break;
                    case UNOP_DW:
                        if (!GetMemDword(&tempAddr, (PULONG)&value)) {
                            error(ERR_MEMORY);
                        }
                        break;
                    case UNOP_POI:
                        //
                        // There should be some special processing for
                        // 16:16 or 16:32 addresses (i.e. take the DWORD)
                        // and make it back into a value with a possible
                        // segment, but I've left this for others who might
                        // know more of what they want.
                        //
                        if (!GetMemDword(&tempAddr, (PULONG)&value)) {
                            error(ERR_MEMORY);
                        }
                        break;
                }
                break;

            case UNOP_LOW:
                value &= 0xffff;
                break;
            case UNOP_HI:
                value = ((ULONG)value >> 16);
                break;
            case ADDOP_PLUS:
                break;
            case ADDOP_MINUS:
                value = -value;
                break;
            default:
                error(ERR_SYNTAX);
        }
    }
    return value;
}

/*** GetTerm - get basic term
*
*   Purpose:
*       Parse numeric, variable, or register name into a basic
*       term value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of basic term.
*
*   Exceptions:
*       error exit: SYNTAX - empty basic term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <term> = ( <expr> ) | <register-value> | <number> | <variable>
*       <register-value> = @<register-name>
*
*************************************************************************/

LONG64
GetTerm (
    void
    )
{
    LONG64  value = 0;
    ULONG   opclass;
    LONG64  opvalue;

//dprintf("LONG GetTerm ()\n");
    opclass = GetTokenSym(&opvalue);
    if (opclass == LPAREN_CLASS) {
        value = GetExpr();
        if (GetTokenSym(&opvalue) != RPAREN_CLASS) {
            error(ERR_SYNTAX);
        }
    }
    else if (opclass == REG_CLASS) {
        value = GetRegFlagValue((ULONG)opvalue);
    }
    else if (opclass == REG_PC_CLASS) {
        addrExpression |= INSTR_POINTER;
        value = GetRegFlagValue((ULONG)opvalue);
    }
    else if (opclass == NUMBER_CLASS || opclass == SYMBOL_CLASS) {
        value = opvalue;
    }
    else {
        error(ERR_SYNTAX);
    }

    return value;
}

/*** GetRange - parse address range specification
*
*   Purpose:
*       With the current command line position, parse an
*       address range specification.  Forms accepted are:
*       <start-addr>            - starting address with default length
*       <start-addr> <end-addr> - inclusive address range
*       <start-addr> l<count>   - starting address with item count
*
*   Input:
*       pchCommand - present command line location
*       size - nonzero - (for data) size in bytes of items to list
*                        specification will be "length" type with
*                        *fLength forced to TRUE.
*              zero - (for instructions) specification either "length"
*                     or "range" type, no size assumption made.
*
*   Output:
*       *addr - starting address of range
*       *value - if *fLength = TRUE, count of items (forced if size != 0)
*                              FALSE, ending address of range
*       (*addr and *value unchanged if no second argument in command)
*       lpbSecondParamIsALength - Indicates whether second address is of the form <L count>.
*
*   Returns:
*       A value of TRUE is returned if no length is specified, or a length
*       or an ending address is specified and size is not zero. Otherwise,
*       a value of FALSE is returned.
*
*   Exceptions:
*       error exit:
*               SYNTAX - expression error
*               BADRANGE - if ending address before starting address
*
*************************************************************************/

BOOL
GetRange (
    PUCHAR szExpr,
    PNTSDADDR addr,
    PULONG64 value,
    ULONG size,
    BOOL *fLength,
    int radix,
    MPT mptProcessorType,
    ULONG defaultSeg,
    PUCHAR * lpNext,
    LPBOOL  lpbSecondParamIsALength
    )
/*++

Routine Description:

    With the current command line position, parse an
    address range specification.  Forms accepted are:
    <start-addr>            - starting address with default length
    <start-addr> <end-addr> - inclusive address range
    <start-addr> l<count>   - starting address with item count

Arguments:

    szExpr - Supplies the command string

    addr - Returns the starting address

    value -

    size -

    fLength -

    radix -

    mptProcessorType -

    defaultSeg -

    lpNext - Returns the location of the first character not consumed from
                the input string.

    lpbSecondParamIsALength - (out) Optional, can be null. If not null, will return TRUE or FALSE.
                  TRUE indicates that the second address is of the form <L count>.
                  FALSE indicates that the second address is of the form <address>.
                  If an error occurs, this value is undefined.

Return Value:

    0 for success, ntsd error status otherwise.

--*/
{

    UCHAR ch;
    PUCHAR psz;
    static NTSDADDR EndRange;
    BOOL    fL = FALSE;
    BOOL    fSpace = FALSE;
    ULONG   status;

    pchCommand = szExpr;

    if (mptProcessorType != mptix86) {
        defaultSeg = 0;
    }

    PeekChar();          //  skip leading whitespace first

    //  Pre-parse the line, look for a " L"

    for (psz = pchCommand; *psz; psz++) {
        if ((*psz == 'L' || *psz == 'l') && fSpace) {
            fL = TRUE;
            *psz = '\0';
            break;
        }

        fSpace = (*psz == ' ');
    }

    *fLength = TRUE;
    if ((ch = PeekChar()) != '\0' && ch != ';') {
        status = GetAddrExpression(NULL, addr, radix, mptProcessorType, defaultSeg, lpNext);
        if (status) {
            return status;
        }

        if (((ch = PeekChar()) != '\0' && ch != ';') || fL) {
            if (!fL) {
                // An address was specified
                if (lpbSecondParamIsALength) {
                    *lpbSecondParamIsALength = FALSE;
                }

                status = GetAddrExpression(NULL,
                                           &EndRange,
                                           radix,
                                           mptProcessorType,
                                           defaultSeg,
                                           lpNext
                                           );
                if (status) {
                    return status;
                }
                if (AddrGt(*addr, EndRange)) {
                    return ERR_BADRANGE;
                }

                if (size) {
                    *value = AddrDiff(EndRange, *addr) / size + 1;
                } else {
                    *value = (ULONG_PTR)&EndRange;
                    *fLength = FALSE;
                }

            } else {
                // A length was specified.
                if (lpbSecondParamIsALength) {
                    *lpbSecondParamIsALength = TRUE;
                }

                pchCommand = psz + 1;
                status = GetExpression(NULL, value, radix, mptProcessorType, lpNext);
                *psz = 'l';
                if (status) {
                    return status;
                }
            }
        }
    }

    return 0;
}

/*** PeekChar - peek the next non-white-space character
*
*   Purpose:
*       Return the next non-white-space character and update
*       pchCommand to point to it.
*
*   Input:
*       pchCommand - present command line position.
*
*   Returns:
*       next non-white-space character
*
*************************************************************************/

UCHAR
PeekChar (
    void
    )
{
    UCHAR    ch;

//dprintf("UCHAR PeekChar (void)\n");
    do {
        ch = *pchCommand++;
    } while (ch == ' ' || ch == '\t');
    pchCommand--;
    return ch;
}

/*** PeekToken - peek the next command line token
*
*   Purpose:
*       Return the next command line token, but do not advance
*       the pchCommand pointer.
*
*   Input:
*       pchCommand - present command line position.
*
*   Output:
*       *pvalue - optional value of token
*   Returns:
*       class of token
*
*   Notes:
*       savedClass, savedValue, and savedpchCmd saves the token getting
*       state for future peeks.  To get the next token, a GetToken or
*       AcceptToken call must first be made.
*
*************************************************************************/

ULONG
PeekToken (
    PLONG64 pvalue
    )
{
    UCHAR    *pchTemp;

//dprintf("ULONG PeekToken (PLONG pvalue)\n");
    //  Get next class and value, but do not
    //  move pchCommand, but save it in savedpchCmd.
    //  Do not report any error condition.

    if (savedClass == INVALID_CLASS) {
        pchTemp = pchCommand;
        savedClass = NextToken(&savedValue);
        savedpchCmd = pchCommand;
        pchCommand = pchTemp;
    }
    *pvalue = savedValue;
    return savedClass;
}

/*** AcceptToken - accept any peeked token
*
*   Purpose:
*       To reset the PeekToken saved variables so the next PeekToken
*       will get the next token in the command line.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void
AcceptToken (
    void
    )
{
//dprintf("void AcceptToken (void)\n");
    savedClass = INVALID_CLASS;
    pchCommand = savedpchCmd;
}

/*** GetToken - peek and accept the next token
*
*   Purpose:
*       Combines the functionality of PeekToken and AcceptToken
*       to return the class and optional value of the next token
*       as well as updating the command pointer pchCommand.
*
*   Input:
*       pchCommand - present command string pointer
*
*   Output:
*       *pvalue - pointer to the token value optionally set.
*   Returns:
*       class of the token read.
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
GetTokenSym (
    PLONG64 pvalue
    )
{
    ULONG   opclass;

//dprintf("ULONG GetTokenSym (PLONG pvalue)\n");
    if (savedClass != INVALID_CLASS) {
        opclass = savedClass;
        savedClass = INVALID_CLASS;
        *pvalue = savedValue;
        pchCommand = savedpchCmd;
    }
    else {
        opclass = NextToken(pvalue);
    }

    if (opclass == ERROR_CLASS) {
        error((ULONG_PTR)*pvalue);
    }

    return opclass;
}

/*** NextToken - process the next token
*
*   Purpose:
*       Parse the next token from the present command string.
*       After skipping any leading white space, first check for
*       any single character tokens or register variables.  If
*       no match, then parse for a number or variable.  If a
*       possible variable, check the reserved word list for operators.
*
*   Input:
*       pchCommand - pointer to present command string
*
*   Output:
*       *pvalue - optional value of token returned
*       pchCommand - updated to point past processed token
*   Returns:
*       class of token returned
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
NextToken (
    PLONG64 pvalue
    )
{
    ULONG               base;
    UCHAR               chSymbol[SYMBOLSIZE];
    UCHAR               chSymbolString[SYMBOLSIZE];
    UCHAR               chPreSym[9] = {0};
    ULONG               cbSymbol = 0;
    BOOL                fNumber = TRUE;
    BOOL                fSymbol = TRUE;
    BOOL                fForceReg = FALSE;
    BOOL                fForceSym = FALSE;
    ULONG               errNumber = 0;
    UCHAR               ch;
    UCHAR               chlow;
    UCHAR               chtemp;
    UCHAR               limit1 = '9';
    UCHAR               limit2 = '9';
    BOOL                fDigit = FALSE;
    ULONG64             value = 0;
    ULONG64             tmpvalue;
    ULONG               index;
    PUCHAR              pchCmdSave;
    BOOL                UseDeferred;
    RD                  rd;
    ADDR                addr;


    base = BaseDefault;

    //  skip leading white space.

    do {
        ch = *pchCommand++;
    } while (ch == ' ' || ch == '\t');

    chlow = (UCHAR)tolower(ch);

    //  test for special character operators and register variable

    switch (chlow) {
        case '\0':
        case ';':
            pchCommand--;
            return EOL_CLASS;
        case '+':
            *pvalue = ADDOP_PLUS;
            return ADDOP_CLASS;
        case '-':
            *pvalue = ADDOP_MINUS;
            return ADDOP_CLASS;
        case '*':
            *pvalue = MULOP_MULT;
            return MULOP_CLASS;
        case '/':
            *pvalue = MULOP_DIVIDE;
            return MULOP_CLASS;
        case '%':
            *pvalue = MULOP_MOD;
            return MULOP_CLASS;
        case '&':
            *pvalue = LOGOP_AND;
            return LOGOP_CLASS;
        case '|':
            *pvalue = LOGOP_OR;
            return LOGOP_CLASS;
        case '^':
            *pvalue = LOGOP_XOR;
            return LOGOP_CLASS;
        case '=':
            if (*pchCommand == '=') {
                pchCommand++;
            }
            *pvalue = LRELOP_EQ;
            return LRELOP_CLASS;
        case '>':
            *pvalue = LRELOP_GT;
            return LRELOP_CLASS;
        case '<':
            *pvalue = LRELOP_LT;
            return LRELOP_CLASS;
        case '!':
            if (*pchCommand != '=') {
                break;
            }
            pchCommand++;
            *pvalue = LRELOP_NE;
            return LRELOP_CLASS;
        case '~':
            *pvalue = UNOP_NOT;
            return UNOP_CLASS;
        case '(':
            return LPAREN_CLASS;
        case ')':
            return RPAREN_CLASS;
        case '[':
            return LBRACK_CLASS;
        case ']':
            return RBRACK_CLASS;
        case '.':
#ifdef _PPC_
            // Modified to test for .# to all .symbol (added if{}break; IBMCDB
            if (*(pchCommand + 1) >= '0' && *(pchCommand + 1) <= '9') {
#endif
               GetRegPCValue(&tempAddr);
               *pvalue = Flat(tempAddr);
               addrExpression =  Type(tempAddr);
               return NUMBER_CLASS;
#ifdef _PPC_
            } else {
               fForceSym = TRUE;
               fForceReg = FALSE;
               fNumber = FALSE;
               break;
            }
#endif
        case ':':
            *pvalue = MULOP_SEG;
            return MULOP_CLASS;

        case '`':
            *pvalue = MULOP_64;
            return MULOP_CLASS;

        }

    //  special prefixes - '@' for register - '!' for symbol

    if (chlow == '@' || chlow == '!' ||
       (chlow == '.' && *(pchCommand+1) == '.'))
    {               // CDB
        fForceReg = (BOOL   )(chlow == '@');
        fForceSym = (BOOL   )!fForceReg;
        fNumber = FALSE;
        ch = *pchCommand++;
        chlow = (UCHAR)tolower(ch);
    }

    //  if string is followed by '!', but not '!=',
    //      then it is a module name and treat as text

    pchCmdSave = pchCommand;

    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') ||
           (chlow == '_') || (chlow == '$') || (chlow == '.'))
    {
        chlow = (UCHAR)tolower(*pchCommand); pchCommand++;
    }

    //  treat as symbol if a nonnull string is followed by '!',
    //      but not '!='

    if (chlow == '!' && *pchCommand != '=' && pchCmdSave != pchCommand) {
        fNumber = FALSE;
    }

    pchCommand = pchCmdSave;
    chlow = (UCHAR)tolower(ch);       //  ch was NOT modified


    if (fNumber) {
        if (chlow == '\'') {
            *pvalue = 0;
            while (TRUE) {
                ch = *pchCommand++;
                if (!ch) {
                    *pvalue = ERR_SYNTAX;
                    return ERROR_CLASS;
                }
                if (ch == '\'') {
                    if (*pchCommand != '\'') {
                        break;
                    }
                    ch = *pchCommand++;
                }
                else
                if (ch == '\\') {
                    ch = *pchCommand++;
                }
                *pvalue = (*pvalue << 8) | ch;
            }

            return NUMBER_CLASS;
        }

        //  if first character is a decimal digit, it cannot
        //  be a symbol.  leading '0' implies octal, except
        //  a leading '0x' implies hexadecimal.

        if (chlow >= '0' && chlow <= '9') {
            if (fForceReg) {
                *pvalue = ERR_SYNTAX;
                return ERROR_CLASS;
            }
            fSymbol = FALSE;
            if (chlow == '0') {
                ch = *pchCommand++;
                chlow = (UCHAR)tolower(ch);
                if (chlow == 'x') {
                    base = 16;
                    ch = *pchCommand++;
                    chlow = (UCHAR)tolower(ch);
                    fDigit = TRUE;
                }
                else if (chlow == 'n' || chlow == 't') {
                    base = 10;
                    ch = *pchCommand++;
                    chlow = (UCHAR)tolower(ch);
                }
                else {
                    base = 8;
                    fDigit = TRUE;
                }
            }
        }

        //  a number can start with a letter only if base is
        //  hexadecimal and it is a hexadecimal digit 'a'-'f'.

        else if ((chlow < 'a' || chlow > 'f') || base != 16) {
            fNumber = FALSE;
        }

        //  set limit characters for the appropriate base.

        if (base == 8) {
            limit1 = '7';
        }
        if (base == 16) {
            limit2 = 'f';
        }
    }

    //  perform processing while character is a letter,
    //  digit, underscore, or dollar-sign.

    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') ||
           (chlow == '_') || (chlow == '$') || (chlow == '.')) {
        //  if possible number, test if within proper range,
        //  and if so, accumulate sum.

        if (fNumber) {
            if ((chlow >= '0' && chlow <= limit1) ||
                    (chlow >= 'a' && chlow <= limit2)) {
                fDigit = TRUE;
                tmpvalue = value * base;
                if (tmpvalue < value) {
                    errNumber = ERR_OVERFLOW;
                }
                chtemp = (UCHAR)(chlow - '0');
                if (chtemp > 9) {
                    chtemp -= 'a' - '0' - 10;
                }
                value = tmpvalue + (ULONG)chtemp;
                if (value < tmpvalue) {
                    errNumber = ERR_OVERFLOW;
                }
            }
            else {
                fNumber = FALSE;
                errNumber = ERR_SYNTAX;
            }
        }
        if (fSymbol) {
            if (cbSymbol < 9) {
                chPreSym[cbSymbol] = chlow;
            }
            if (cbSymbol < SYMBOLSIZE - 1) {
                chSymbol[cbSymbol++] = ch;
            }
        }
        ch = *pchCommand++;
        chlow = (UCHAR)tolower(ch);
    }

    //  back up pointer to first character after token.

    pchCommand--;

    if (cbSymbol < 9) {
        chPreSym[cbSymbol] = '\0';
    }

    if (MptProcessorType == mptix86) {
        //
        //  catch segment overrides here
        //

        if (!fForceReg && ch == ':') {
            for (index = 0; index < SEGREGSIZE; index++) {
                if (!strncmp( (PSTR) chPreSym, SegRegs[index], 2)) {
                    fForceReg = TRUE;
                    fSymbol = FALSE;
                    break;
                }
            }
        }
    }

    //  if fForceReg, check for register name and return
    //      success or failure

    if (fForceReg) {
        if ((*pvalue = GetRegString(chPreSym, &rd)) != -1) {
            return (rd.rt & rtPC)? REG_PC_CLASS : REG_CLASS;
        } else {
            *pvalue = ERR_BADREG;
            return ERROR_CLASS;
        }
    }

    //  test if number

    if (fNumber && !errNumber && fDigit) {
        *pvalue = value;
        return NUMBER_CLASS;
    }

    //  next test for reserved word and symbol string

    if (fSymbol && !fForceReg) {

        //  check lowercase string in chPreSym for text operator
        //  or register name.
        //  otherwise, return symbol value from name in chSymbol.

        if (!fForceSym && (cbSymbol == 2 || cbSymbol == 3)) {
            for (index = 0; index < RESERVESIZE; index++) {
                if (!strncmp( (PSTR) chPreSym, (PSTR) Reserved[index].chRes, 3)) {
                    *pvalue = Reserved[index].valueRes;
                    return Reserved[index].classRes;
                }
            }
        }

        //  start processing string as symbol

        chSymbol[cbSymbol] = '\0';

        //  test if symbol is a module name (followed by '!')
        //  if so, get next token and treat as symbol

        if (PeekChar() == '!') {
            // chSymbolString holds the name of the symbol to be searched.
            // chSymbol holds the symbol image file name.

            pchCommand++;
            ch = PeekChar();
            pchCommand++;
            cbSymbol = 0;
            while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                   (ch >= '0' && ch <= '9') || (ch == '_') || (ch == '$') ||
                   (ch == '.'))
            {                               // IBMCDB
                chSymbolString[cbSymbol++] = ch;
                ch = *pchCommand++;
            }
            chSymbolString[cbSymbol] = '\0';
            pchCommand--;

            if (cbSymbol == 0) {
                *pvalue = ERR_SYNTAX;
                return( ERROR_CLASS );
            }

            strcat( (PSTR) chSymbol, "!" );
            strcat( (PSTR) chSymbol, (PSTR) chSymbolString );

            if (GetOffsetFromSym( (PSTR) chSymbol, &addr )) {
                *pvalue = GetAddrOff(addr);
                Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
                Flat(tempAddr) = Off(tempAddr)  = *pvalue;
                addrExpression  = Type(tempAddr);
                return SYMBOL_CLASS;
            }

        } else {

            if (cbSymbol == 0) {
                *pvalue = ERR_SYNTAX;
                return( ERROR_CLASS );
            }

            if (GetOffsetFromSym( (PSTR) chSymbol, &addr )) {
                *pvalue = GetAddrOff(addr);
                Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
                Flat(tempAddr) = Off(tempAddr)  = *pvalue;
                addrExpression  = Type(tempAddr);
                return SYMBOL_CLASS;
            }

            //
            // Quick test for register names too
            //
            if (!fForceSym && (*pvalue = GetRegString(chPreSym, &rd)) != -1) {
                return (rd.rt & rtPC)? REG_PC_CLASS : REG_CLASS;
            }
        }

        //
        //  symbol is undefined.
        //  if a possible hex number, do not set the error type
        //
        if (!fNumber) {
            errNumber = ERR_VARDEF;
        }
    }


    //
    //  last chance, undefined symbol and illegal number,
    //      so test for register, will handle old format
    //
    if (!fForceSym && (*pvalue = GetRegString(chPreSym, &rd)) != -1) {
        return (rd.rt & rtPC)? REG_PC_CLASS : REG_CLASS;
    }

    //
    //  no success, so set error message and return
    //
    *pvalue = (ULONG)errNumber;
    return ERROR_CLASS;
}

