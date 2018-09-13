/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* SCANNER.C - Routines for token scanning                              */
/*                                                                      */
/* 29-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"


#define ABS(x) ((x > 0) ? x : -1 * x)


#define ALERT_CHAR      L'\007'          /* ANSI alert character is ASCII BEL */

ULONG lCPPTotalLinenumber = 0;

extern int vfCurrFileType;      //- Added for 16-bit file support.


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
token_t c_size(long);
int     ctoi(int);
int     escape(int);
token_t get_real(PWCHAR);
token_t l_size(long);
long    matol(PWCHAR, int);
token_t uc_size(long);
token_t ul_size(long);
void    skip_1comment(void);



/************************************************************************/
/* local_c_hash                                                         */
/************************************************************************/
hash_t
local_c_hash(
    REG WCHAR *name
    )
{
    REG hash_t  i;

    i = 0;
    while(*name) {
        i += (*name & HASH_MASK);
        name++;
    }
    return(i);
}


/************************************************************************
 * GETID - Get an identifier or keyword.
 * (we know that we're given at least 1 id char)
 * in addition, we'll hash the value using 'c'.
 ************************************************************************/
void
getid(
    REG  UINT c
    )
{
    REG WCHAR   *p;

    p = Reuse_W;
    *p++ = (WCHAR)c;
    c &= HASH_MASK;

repeat:
    while(LXC_IS_IDENT(*p = GETCH())) {    /* while it's an id char . . . */
        c += (*p & HASH_MASK);                  /* hash it */
        p++;
    }
    if(*p != EOS_CHAR) {
        if((*p == L'\\') && (checknl())) {
            goto repeat;
        }
        UNGETCH();
        if(p >= LIMIT(Reuse_W)) {
            strcpy (Msg_Text, GET_MSG (1067));
            fatal(1067);
        }
        if(     ((p - Reuse_W) > LIMIT_ID_LENGTH) && ( ! Prep )) {
            p = Reuse_W + LIMIT_ID_LENGTH;
            *p++ = L'\0';
            c = local_c_hash(Reuse_W);
            Msg_Temp = GET_MSG (4011);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
            warning(4011);      /* id truncated */
        } else {
            *p++ = L'\0';
        }
        Reuse_W_hash = (hash_t)c;
        Reuse_W_length = (UINT)(p - Reuse_W);
        return;
    }
    if(io_eob()) {                      /* end of file in middle of id */
        strcpy (Msg_Text, GET_MSG (1004));
        fatal(1004);
    }
    goto repeat;
}


/************************************************************************
**      prep_string : outputs char/string constants when preprocessing only
************************************************************************/
void
prep_string(
    REG WCHAR c
    )
{
    REG WCHAR *p_buf;
    int term_char;

    p_buf = Reuse_W;

    term_char = c;

    *p_buf++ = c;               /*  save the open quote  */

    for(;;) {
        switch(CHARMAP(c = GETCH())) {
            case LX_DQUOTE:
            case LX_SQUOTE:
                if(c == (WCHAR)term_char) {
                    *p_buf++ = (WCHAR)term_char;/* save the terminating quote */
                    goto out_of_loop;
                }
                break;
            case LX_BACKSLASH:
                *p_buf++ = c;
                break;
            case LX_CR:
                continue;
            case LX_NL:
                UNGETCH();
                goto out_of_loop;
            case LX_EOS:
                if(c == L'\\') {
                    *p_buf++ = c;
                    c = get_non_eof();
                    break;
                }
                handle_eos();
                continue;
        }
        *p_buf++ = c;
        if(p_buf >= &Reuse_W[MED_BUFFER - 1]) {
            *p_buf = L'\0';
            myfwrite(Reuse_W, (size_t)(p_buf - Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);
            p_buf = Reuse_W;
        }
    }

out_of_loop:
    *p_buf = L'\0';
    myfwrite(Reuse_W, (size_t)(p_buf - Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);
}


/************************************************************************
**      char_const : gather up a character constant
**  we're called after finding the openning single quote.
************************************************************************/
token_t
char_const(
    void
    )
{
    REG WCHAR c;
    value_t value;
    token_t tok;

    tok = (token_t)(Jflag ? L_CUNSIGNED : L_CINTEGER);

first_switch:

    switch(CHARMAP(c = GETCH())) {
        case LX_BACKSLASH:
            break;
        case LX_SQUOTE:
            strcpy (Msg_Text, GET_MSG (2137));  //"empty character constant"
            error(2137);
            value.v_long = 0;
            UNGETCH();
            break;
        case LX_EOS:                /* ??? assumes i/o buffering > 1 char */
            if(handle_eos() != BACKSLASH_EOS) {
                goto first_switch;
            }
            value.v_long = escape(get_non_eof());
            if( tok == L_CUNSIGNED ) {              /* don't sign extend */
                value.v_long &= 0xff;
            }
            break;
        case LX_NL:
            /* newline in character constant */
            strcpy (Msg_Text, GET_MSG (2001));
            error (2001);
            UNGETCH();
            /*
                    **  FALLTHROUGH
                    */
        default:
            value.v_long = c;
            break;
    }

    if((c = get_non_eof()) != L'\'') {
        strcpy (Msg_Text, GET_MSG (2015));
        error (2015);           /* too many chars in constant */
        do {
            if(c == L'\n') {
                strcpy (Msg_Text, GET_MSG (2016));
                error(2016);            /* missing closing ' */
                    break;
            }
        } while((c = get_non_eof()) != L'\'');
    }
    yylval.yy_tree = build_const(tok, &value);
    return(tok);
}


/************************************************************************
**      str_const : gather up a string constant
************************************************************************/
void
str_const(
    VOID
    )
{
    REG WCHAR c;
    REG PWCHAR  p_buf;
    int not_warned_yet = TRUE;

    p_buf = yylval.yy_string.str_ptr = Macro_buffer;
    /*
        **      Is it possible that reading this string during a rescan will
        **      overwrite the expansion being rescanned?  No, because a macro
        **      expansion is limited to the top half of Macro_buffer.
        **      For Macro_depth > 0, this is like copying the string from
        **      somewhere in the top half of Macro_buffer to the bottom half
        **      of Macro_buffer.
        **      Note that the restriction on the size of an expanded macro is
        **      stricter than the limit on an L_STRING length.  An expanded
        **      macro is limited to around 1019 bytes, but an L_STRING is
        **      limited to 2043 bytes.
        */
    for(;;) {
        switch(CHARMAP(c = GETCH())) {
            case LX_NL:
                UNGETCH();
                strcpy (Msg_Text, GET_MSG (2001));
                error(2001);
                /*
                **  FALLTHROUGH
                */
            case LX_DQUOTE:
                *p_buf++ = L'\0';
                yylval.yy_string.str_len = (USHORT)(p_buf-yylval.yy_string.str_ptr);
                return;
                break;
            case LX_EOS:
                if(handle_eos() != BACKSLASH_EOS) {
                    continue;
                }
                if(InInclude) {
                    break;
                }
                else {
                    c = (WCHAR)escape(get_non_eof());  /* process escaped char */
                }
                break;
        }
        if(p_buf - Macro_buffer > LIMIT_STRING_LENGTH) {
            if( not_warned_yet ) {
                strcpy (Msg_Text, GET_MSG (4009));
                warning(4009);          /* string too big, truncating */
                not_warned_yet = FALSE;
            }
        } else {
            *p_buf++ = c;
        }
    }
}


/************************************************************************
**  do_newline : does work after a newline has been found.
************************************************************************/

void
do_newline(
    void
    )
{
    ++Linenumber;
    for(;;) {
        switch(CHARMAP(GETCH())) {
            case LX_BOM:  // ignore Byte Order Mark
                break;
            case LX_CR:
                break;
            case LX_POUND:
                preprocess();
                break;
            case LX_SLASH:
                if( ! skip_comment()) {
                    goto leave_do_newline;
                }
                break;
            case LX_NL:
                if ((lCPPTotalLinenumber++ & RC_PREPROCESS_UPDATE) == 0)
                    UpdateStatus(1, lCPPTotalLinenumber);
                Linenumber++;
                // must manually write '\r' with '\n' when writing 16-bit strings
                if( Prep ) {                /*  preprocessing only */
                    myfwrite(L"\r", sizeof(WCHAR), 1, OUTPUTFILE);
                }
                /*
                **  FALLTHROUGH
                */
            case LX_WHITE:
                if( Prep ) {                /*  preprocessing only, output whitespace  */
                    myfwrite(&(PREVCH()), sizeof(WCHAR), 1, OUTPUTFILE);
                } else {
                    do {
                        ;
                    } while(LXC_IS_WHITE(GETCH()));
                    UNGETCH();
                }
                break;
            case LX_EOS:
                if(PREVCH() == EOS_CHAR || PREVCH() == CONTROL_Z) {
                    if(io_eob()) {          /* leaves us pointing at a valid char */
                        return;
                    }
                    break;
                }
                if(checknl()) {
                    continue;
                }
                /* it's a backslash */
                /*
                **      FALLTHROUGH
                */
            default:                        /* first non-white is not a '#', leave */

leave_do_newline:

                UNGETCH();
                return;
        }
    }
}


/************************************************************************
 * GETNUM - Get a number from the input stream.
 *
 * ARGUMENTS
 *      radix - the radix of the number to be accumulated.  Can only be 8, 10,
 *                      or 16
 *      pval - a pointer to a VALUE union to be filled in with the value
 *
 * RETURNS - type of the token (L_CINTEGER or L_CFLOAT)
 *
 * SIDE EFFECTS -
 *      does push back on the input stream.
 *      writes into pval by reference
 *  uses buffer Reuse_W
 *
 * DESCRIPTION -
 *      Accumulate the number according to the rules for each radix.
 *      Set up the format string according to the radix (or distinguish
 *      integer from float if radix is 10) and convert to binary.
 *
 * AUTHOR - Ralph Ryan, Sept. 8, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
token_t
getnum(
    REG WCHAR c
    )
{
    REG WCHAR   *p;
    WCHAR       *start;
    int         radix;
    token_t     tok;
    value_t     value;

    tok = L_CINTEGER;
    start = (Tiny_lexer_nesting ? Exp_ptr : Reuse_W);
    p = start;
    if( c == L'0' ) {
        c = get_non_eof();
        if( IS_X(c) ) {
            radix = 16;
            if( Prep ) {
                *p++ = L'0';
                *p++ = L'x';
            }
            for(c = get_non_eof(); LXC_IS_XDIGIT(c); c = get_non_eof()) {
                /* no check for overflow? */
                *p++ = c;
            }
            if((p == Reuse_W) && (Tiny_lexer_nesting == 0)) {
                strcpy (Msg_Text, GET_MSG (2153));
                error(2153);
            }
            goto check_suffix;
        } else {
            radix = 8;
            *p++ = L'0'; /* for preprocessing or 0.xxx case */
        }
    } else {
        radix = 10;
    }

    while( LXC_IS_DIGIT((WCHAR)c) ) {
        *p++ = c;
        c = get_non_eof();
    }

    if( IS_DOT(c) || IS_E(c) ) {
        UNGETCH();
        return(get_real(p));
    }

check_suffix:
    if( IS_EL(c) ) {
        if( Prep ) {
            *p++ = c;
        }
        c = get_non_eof();
        if( IS_U(c) ) {
            if(Prep) {
                *p++ = c;
            }
            tok = L_LONGUNSIGNED;
        } else {
            tok = L_LONGINT;
            UNGETCH();
        }
    } else if( IS_U(c) ) {
        if( Prep ) {
            *p++ = c;
        }
        c = get_non_eof();
        if( IS_EL(c) ) {
            if( Prep ) {
                *p++ = c;
            }
            tok = L_LONGUNSIGNED;
        } else {
            tok = L_CUNSIGNED;
            UNGETCH();
        }
    } else {
        UNGETCH();
    }
    *p = L'\0';
    if( start == Exp_ptr ) {
        Exp_ptr = p;
        return(L_NOTOKEN);
    } else if( Prep ) {
        myfwrite( Reuse_W, (size_t)(p - Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);
        return(L_NOTOKEN);
    }
    value.v_long = matol(Reuse_W,radix);
    switch(tok) {
        case L_CINTEGER:
            tok = (radix == 10)
                ? c_size(value.v_long)
                : uc_size(value.v_long)
                ;
            break;
        case L_LONGINT:
            tok = l_size(value.v_long);
            break;
        case L_CUNSIGNED:
            tok = ul_size(value.v_long);
            break;
    }
    yylval.yy_tree = build_const(tok, &value);
    return(tok);
}


/************************************************************************
**  get_real : gathers the real part/exponent of a real number.
**              Input  : ptr to the null terminator of the whole part
**                               pointer to receive value.
**              Output : L_CFLOAT
**
**  ASSUMES whole part is either at Exp_ptr or Reuse_W.
************************************************************************/
token_t
get_real(
    REG PWCHAR p
    )
{
    REG int c;
    token_t tok;

    c = get_non_eof();
    if(Cross_compile && (Tiny_lexer_nesting == 0)) {
        strcpy (Msg_Text, GET_MSG (4012));
        warning(4012);  /* float constant in cross compilation */
        Cross_compile = FALSE;  /*  only one msg per file */
    }
    /*
    **  if the next char is a digit, then we've been called after
    **  finding a '.'. if this is true, then
    **  we want to find the fractional part of the number.
    **  if it's a '.', then we've been called after finding
    **  a whole part, and we want the fraction.
    */
    if( LXC_IS_DIGIT((WCHAR)c) || IS_DOT(c) ) {
        do {
            *p++ = (WCHAR)c;
            c = (int)get_non_eof();
        } while( LXC_IS_DIGIT((WCHAR)c) );
    }
    if( IS_E((WCHAR)c) ) {              /*  now have found the exponent  */
        *p++ = (WCHAR)c;                /*  save the 'e'  */
        c = (WCHAR)get_non_eof();       /*  skip it  */
        if( IS_SIGN(c) ) {              /*  optional sign  */
            *p++ = (WCHAR)c;            /*  save the sign  */
            c = (int)get_non_eof();
        }
        if( ! LXC_IS_DIGIT((WCHAR)c)) {
            if( ! Rflag ) {
                if(Tiny_lexer_nesting == 0) {
                    Msg_Temp = GET_MSG (2021);
                    SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
                    error(2021); /* missing or malformed exponent */
                }
                *p++ = L'0';
            }
        } else {
            do {                        /* gather the exponent */
                *p++ = (WCHAR)c;
                c = (int)get_non_eof();
            } while( LXC_IS_DIGIT((WCHAR)c) );
        }
    }
    if( IS_F((WCHAR)c) ) {
        tok = L_CFLOAT;
        if( Prep ) {
            *p++ = (WCHAR)c;
        }
    } else if( IS_EL((WCHAR)c) ) {
        tok = L_CLDOUBLE;
        if( Prep ) {
            *p++ = (WCHAR)c;
        }
    } else {
        UNGETCH();
        tok = L_CDOUBLE;
    }
    *p = L'\0';
    if( Tiny_lexer_nesting > 0 ) {
        Exp_ptr = p;
        return(L_NOTOKEN);
    }
    else if( Prep ) {
        myfwrite( Reuse_W, (size_t)(p - Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);
        return(L_NOTOKEN);
    }
    /*
        ** reals aren't used during preprocessing
        */
    return(tok);
}


/************************************************************************
**  matol : ascii to long, given a radix.
************************************************************************/
long
matol(
    REG PWCHAR p_start,
    REG int radix
    )
{
    long        result, old_result;
    unsigned    int     i;

    old_result = result = 0;
    while(*p_start) {
        result *= radix;
        i = ctoi(*p_start);
        if( ((int)i >= radix) && (! Prep) ) {
            Msg_Temp = GET_MSG (2020);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, *p_start, radix);
            error(2020); /* illegal digit % for base % */
        }
        result += i;
        p_start++;
        if(radix == 10) {
            if(result < old_result) {
                p_start--;   /*  fix the string ptr since we have overflowed  */
                break;
            }
        } else if(*p_start) {
            /*
                **  the loop is not finished.
                **  we will multiply by the radix again
                **  check the upper bits. if they're on, then
                **  that mult will overflow the value
                */
            if(radix == 8) {
                if(result & 0xe0000000) {
                    break;
                }
            } else if(result & 0xf0000000) {
                break;
            }
        }
        old_result = result;
    }
    if(*p_start) {
        strcpy (Msg_Text, GET_MSG (2177));
        error(2177);            /* constant too big */
        result = 0;
    }
    return(result);
}


/************************************************************************
**  uc_size : returns 'int' or 'long' (virtual unsigned).
**  if their are no bits in the upper part of the value,
**  then it's an int. otherwise, it's a long.
**  this is valid too if target sizeof(int) != sizeof(long).
**  then L_CINTEGER and L_LONGINT are synonymous.
************************************************************************/
token_t
uc_size(
    long value
    )
{
    return((token_t)((value > INT_MAX) ? L_CUNSIGNED : L_CINTEGER));
}


/************************************************************************
**  c_size : returns 'int' or 'long' for signed numbers.
**  if the sign bit of the lower word is on or any bits
**  in the upper word are on, then we must use 'long'.
************************************************************************/
token_t
c_size(
    long value
    )
{
    return((token_t)((ABS(value) > INT_MAX) ? L_LONGINT : L_CINTEGER));
}


/************************************************************************
**  l_size : returns 'longint' or 'longunsigned' for long numbers.
**  if the sign bit of the high word is on this is 'longunsigned';
************************************************************************/
token_t
l_size(
    long value
    )
{
    return((token_t)((value > LONG_MAX) ? L_LONGUNSIGNED : L_LONGINT));
}


/************************************************************************
**      ul_size : returns 'unsigned' or 'longunsigned' for unsigned numbers.
**      if the number can't be represented as unsigned, it is promoted to
**      unsignedlong.
************************************************************************/
token_t
ul_size(
    long value
    )
{
    return((token_t)((ABS(value) > UINT_MAX-1) ? L_LONGUNSIGNED : L_CUNSIGNED));
}


/************************************************************************
**  ctoi : character to int.
************************************************************************/
int
ctoi(
    int c
    )
{
    if(LXC_IS_DIGIT((WCHAR)c)) {
        return(c - L'0');
    } else {
        return(towupper((WCHAR)c) - towupper(L'A') + 10);
    }
}


/************************************************************************
 * ESCAPE - get an escaped character
 *
 * ARGUMENTS - none
 *
 * RETURNS - value of escaped character
 *
 * SIDE EFFECTS - may push back input
 *
 * DESCRIPTION - An escape ( '\' ) was discovered in the input.  Translate
 *       the next symbol or symbols into an escape sequence.
 *
 * AUTHOR - Ralph Ryan, Sept. 7, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int
escape(
    REG int c
    )
{
    REG int value;
    int cnt;

escape_again:

    if( LXC_IS_ODIGIT((WCHAR)c) ) {/* \ooo is an octal number, must fit into a byte */
        cnt = 1;
        for(value = ctoi(c), c = get_non_eof();
            (cnt < 3) && LXC_IS_ODIGIT((WCHAR)c);
            cnt++, c = get_non_eof()
            ) {
            value *= 8;
            value += ctoi(c);
        }
        if( ! Prep ) {
            if(value > 255) {
                Msg_Temp = GET_MSG (2022);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, value);
                error (2022);
            }
        }
        UNGETCH();
        return((char)value);
    }
    switch( c ) {
        case L'a':
            return(ALERT_CHAR);
            break;
        case L'b':
            return(L'\b');
            break;
        case L'f':
            return(L'\f');
            break;
        case L'n':
            return fMacRsrcs ? (L'\r') : (L'\n');
            break;
        case L'r':
            return fMacRsrcs ? (L'\n') : (L'\r');
            break;
        case L't':
            return(L'\t');
            break;
        case L'v':
            return(L'\v');
            break;
        case L'x':
            cnt = 0;
            value = 0;
            c = get_non_eof();
            while((cnt < 3) && LXC_IS_XDIGIT((WCHAR)c)) {
                value *= 16;
                value += ctoi(c);
                c = get_non_eof();
                cnt++;
            }
            if(cnt == 0) {
                strcpy (Msg_Text, GET_MSG (2153));
                error (2153);
            }
            UNGETCH();
            return((char)value);    /* cast to get sign extend */
        default:
            if(c != L'\\') {
                return(c);
            } else {
                if(checknl()) {
                    c = get_non_eof();
                    goto escape_again;
                } else {
                    return(c);
                }
            }
    }
}


/************************************************************************
 * CHECKOP - Check whether the next input character matches the argument.
 *
 * ARGUMENTS
 *      short op - the character to be checked against
 *
 * RETURNS
 *      TRUE or FALSE
 *
 * SIDE EFFECTS
 *      Will push character back onto the input if there is no match.
 *
 * DESCRIPTION
 *      If the next input character matches op, return TRUE.  Otherwise
 *      push it back onto the input.
 *
 * AUTHOR - Ralph Ryan, Sept. 9, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int
checkop(
    int op
    )
{
    if(op == (int)get_non_eof()) {
        return(TRUE);
    }
    UNGETCH();
    return(FALSE);
}


/************************************************************************
**  DumpSlashComment : while skipping a comment, output it.
************************************************************************/
void
DumpSlashComment(
    VOID
    )
{
    if( ! Cflag ) {
        skip_NLonly();
        return;
    }
    myfwrite(L"//", 2 * sizeof(WCHAR), 1, OUTPUTFILE);
    for(;;) {
        WCHAR c;

        switch(CHARMAP(c = GETCH())) {
            // must manually write '\r' with '\n' when writing 16-bit strings
            //case LX_CR:
            //    continue;
            case LX_EOS:
                handle_eos();
                continue;
            case LX_NL:
                UNGETCH();
                return;
        }
        myfwrite(&c, sizeof(WCHAR), 1, OUTPUTFILE);
    }
}


/************************************************************************
**  dump_comment : while skipping a comment, output it.
************************************************************************/
void
dump_comment(
    void
    )
{
    if( ! Cflag ) {
        skip_1comment();
        return;
    }
    myfwrite(L"/*", 2 * sizeof(WCHAR), 1, OUTPUTFILE);
    for(;;) {
        WCHAR c;

        switch(CHARMAP(c = GETCH())) {
            case LX_STAR:
                if(checkop(L'/')) {
                    myfwrite(L"*/", 2 * sizeof(WCHAR), 1, OUTPUTFILE);
                    return;
                }
                break;
            case LX_EOS:
                handle_eos();
                continue;
            case LX_NL:
                Linenumber++;
                break;      /* output below */
            // must manually write '\r' with '\n' when writing 16-bit strings
            //case LX_CR:
            //    continue;
        }
        myfwrite(&c, sizeof(WCHAR), 1, OUTPUTFILE);
    }
}

/************************************************************************/
/* skip_comment()                                                       */
/************************************************************************/
int
skip_comment(
    void
    )
{
    if(checkop(L'*')) {
        skip_1comment();
        return(TRUE);
    } else if(checkop(L'/')) {
        skip_NLonly();
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/************************************************************************
**  skip_1comment : we're called when we're already in a comment.
**  we're looking for the comment close. we also count newlines
**  and output them if we're preprocessing.
************************************************************************/
void
skip_1comment(
    void
    )
{
    UINT c;

    for(;;) {
        c = GETCH();
        if(c == L'*') {
recheck:
            c = GETCH();
            if(c == L'/') {      /* end of comment */
                return;
            } else if(c == L'*') {
                /*
                **  if we get another '*' go back and check for a slash
                */
                goto recheck;
            } else if(c == EOS_CHAR) {
                handle_eos();
                goto recheck;
            }
        }
        /*
        **  note we fall through here. we know this baby is not a '*'
        **  we used to unget the char and continue. since we check for
        **  another '*' inside the above test, we can fall through here
        **  without ungetting/getting and checking again.
        */
        if(c <= L'\n') {
            /*
            **  hopefully, the above test is less expensive than doing two tests
            */
            if(c == L'\n') {
                Linenumber++;
                if(Prep) {
                    myfwrite(L"\r\n", 2 * sizeof(WCHAR), 1, OUTPUTFILE);
                }
            } else if(c == EOS_CHAR) {
                handle_eos();
            }
        }
    }
}


/************************************************************************
**  skip_cwhite : while the current character is whitespace or a comment.
**  a newline is NOT whitespace.
************************************************************************/
WCHAR
skip_cwhite(
    void
    )
{
    REG WCHAR           c;

skip_cwhite_again:
    while((c = GETCH()) <= L'/') {      /* many chars are above this */
        if(c == L'/') {
            if( ! skip_comment()) {
                return(L'/');
            }
        } else if(c > L' ') {           /* char is between '!' and '.' */
            return(c);
        } else {
            switch(CHARMAP(c)) {
                case LX_EOS:
                    handle_eos();
                    break;
                case LX_WHITE:
                    continue;
                    break;
                case LX_CR:
                    continue;
                    break;
                default:
                    return(c);
                    break;
            }
        }
    }
    if((c == L'\\') && (checknl())) {
        goto skip_cwhite_again;
    }
    return(c);
}


/************************************************************************
**  checknl : check for newline, skipping carriage return if there is one.
**  also increments Linenumber, so this should be used by routines which
**  will not push the newline back in such a way that rawtok() will be invoked,
**  find the newline and do another increment.
************************************************************************/
int
checknl(
    void
    )
{
    REG WCHAR  c;

    for(;;) {
        c = GETCH();
        if(c > L'\r') {
            UNGETCH();
            return(FALSE);
        }
        switch(c) {
            case L'\n':
                Linenumber++;
                // must manually write '\r' with '\n' when writing 16-bit strings
                if( Prep ) {
                    myfwrite(L"\r\n", 2 * sizeof(WCHAR), 1, OUTPUTFILE);
                }
                return(TRUE);
                break;
            case L'\r':
                continue;
                break;
            case EOS_CHAR:
                handle_eos();
                PREVCH() = L'\\';    /* M00HACK - needs pushback */
                continue;
                break;
            default:
                UNGETCH();
                return(FALSE);
                break;
        }
    }
}


/************************************************************************
**  get_non_eof : get a real char.
************************************************************************/
WCHAR
get_non_eof(
    void
    )
{
    WCHAR    c;

get_non_eof_again:
    while((c = GETCH()) <= L'\r') {
        if(c == L'\r') {
            continue;
        } else if(c != EOS_CHAR) {
            break;
        }
        if(Tiny_lexer_nesting > 0) {
            break;
        }
        handle_eos();
    }
    if((c == L'\\') && (checknl())) {
        goto get_non_eof_again;
    }
    return(c);
}
