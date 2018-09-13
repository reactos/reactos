/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* P0PREPRO.C - Main Preprocessor                                       */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"
#include <ddeml.h>

/************************************************************************/
/* Internal constants                                                   */
/************************************************************************/
#define GOT_IF                  1       /* last nesting command was an if.. */
#define GOT_ELIF                2       /* last nesting command was an if.. */
#define GOT_ELSE                3       /* last nesting command was an else */
#define GOT_ENDIF               4       /* found endif */
#define ELSE_OR_ENDIF           5       /* skip to either #else or #endif */
#define ENDIF_ONLY              6       /* skip to #endif -- #else is an error*/

int     ifstack[IFSTACK_SIZE];


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
void chk_newline(PWCHAR);
void in_standard(void);
int incr_ifstack(void);
token_t next_control(void);
unsigned long int pragma(void);
int skipto(int);
void skip_quoted(int);
PWCHAR sysinclude(void);


/************************************************************************/
/* incr_ifstack - Increment the IF nesting stack                        */
/************************************************************************/

int
incr_ifstack(
    void
    )
{
    if(++Prep_ifstack >= IFSTACK_SIZE) {
        strcpy (Msg_Text, GET_MSG (1052));
        fatal (1052);
    }
    return(Prep_ifstack);
}


/************************************************************************
 * SYSINCLUDE - process a system include : #include <foo>
 *
 * ARGUMENTS - none
 *
 * RETURNS - none
 *
 * SIDE EFFECTS - none
 *
 * DESCRIPTION
 *      Get the system include file name.  Since the name is not a "string",
 *      the name must be built much the same as the -E option rebuilds the text
 *      by using the Tokstring expansion for tokens with no expansion already
 *
 *  NOTE : IS THIS ANSI? note we're just reading chars, and not expanding
 * any macros. NO, it's not. it must expand the macros.
 * TODO : have it call yylex() unless and until it finds a '>' or a newline.
 * (probably have to set On_pound_line to have yylex return the newline.)
 *
 * AUTHOR
 *                      Ralph Ryan      Sep. 1982
 *
 * MODIFICATIONS - none
 *
 *
 ************************************************************************/
PWCHAR
sysinclude(
    void
    )
{
    REG int     c;
    REG WCHAR  *p_fname;

    p_fname = Reuse_W;
    c = skip_cwhite();
    if( c == L'\n' ) {
        UNGETCH();
        strcpy (Msg_Text, GET_MSG (2012));
        error(2012);    /* missing name after '<' */
        return(NULL);
    }
    while( c != L'>' && c != L'\n' ) {
        *p_fname++ = (WCHAR)c;          /* check for buffer overflow ??? */
        c = get_non_eof();
    }
    if( c == L'\n' ) {
        UNGETCH();
        strcpy (Msg_Text, GET_MSG (2013));
        error(2013);    /* missing '>' */
        return(NULL);
    }
    if(p_fname != Reuse_W) {
        p_fname--;
        while((p_fname >= Reuse_W) && iswspace(*p_fname)) {
            p_fname--;
        }
        p_fname++;
    }
    *p_fname = L'\0';
    return(Reuse_W);
}


/************************************************************************
**  preprocess : the scanner found a # which was the first non-white char
**  on a line.
************************************************************************/
void
preprocess(
    void
    )
{
    REG WCHAR   c;
    long        eval;
    int         condition;
    token_t     deftok;
    hln_t       identifier;
    unsigned long int   cp;

    if(Macro_depth != 0) {      /* # only when not in a macro */
        return;
    }
    switch(CHARMAP(c = skip_cwhite())) {
        case LX_ID:
            getid(c);
            HLN_NAME(identifier) = Reuse_W;
            HLN_LENGTH(identifier) = (UINT)Reuse_W_length;
            HLN_HASH(identifier) = Reuse_W_hash;
            break;
        case LX_NL:
            UNGETCH();
            return;
        default:
            Msg_Temp = GET_MSG (2019);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
            error(2019);    /* unknown preprocessor command */
            skip_cnew();    /* finds a newline */
            return;
    }

    On_pound_line = TRUE;
start:
    switch(deftok = is_pkeyword(HLN_IDENTP_NAME(&identifier))) {
        int                     old_prep;

        case P0_DEFINE :
            define();
            break;
        case P0_LINE :
            old_prep = Prep;
            Prep = FALSE;
            yylex();
            if(Basic_token != L_CINTEGER) {         /* #line needs line number */
                Msg_Temp = GET_MSG (2005);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, TS_STR(Basic_token));
                error(2005);        /* unknown preprocessor command */
                Prep = old_prep;
                skip_cnew();
                On_pound_line = FALSE;
                return;
            }
            /*
            **  -1 because there's a newline at the end of this line
            **  which will be counted later when we find it.
            **  the #line says the next line is the number we've given
            */
            Linenumber = TR_LVALUE(yylval.yy_tree) - 1;
            yylex();
            Prep = old_prep;
            switch(Basic_token) {
                case L_STRING:
                    if( wcscmp(Filename, yylval.yy_string.str_ptr) != 0) {
                        wcsncpy(Filename,
                            yylval.yy_string.str_ptr,
                            sizeof(Filebuff) / sizeof(WCHAR)
                            );
                    }
                case L_NOTOKEN:
                    break;
                default:
                    Msg_Temp = GET_MSG (2130);
                    SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, TS_STR(Basic_token));
                    error(2130);         /* #line needs a string */
                    skip_cnew();
                    On_pound_line = FALSE;
                    return;
                    break;
            }
            emit_line();
            chk_newline(L"#line");
            break;
        case P0_INCLUDE :
            old_prep = Prep;
            Prep = FALSE;
            InInclude = TRUE;
            yylex();
            InInclude = FALSE;
            Prep = old_prep;
            switch(Basic_token) {
                case L_LT:
                    if((sysinclude()) == NULL) {
                        skip_cnew();
                        On_pound_line = FALSE;
                        return;
                        break;          /* error already emitted */
                    }
                    yylval.yy_string.str_ptr = Reuse_W;
                    break;
                case L_STRING:
                    break;
                default:
                    Msg_Temp = GET_MSG (2006);
                    SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, TS_STR(Basic_token));
                    error(2006);        /* needs file name */
                    skip_cnew();
                    On_pound_line = FALSE;
                    return;
                    break;
            }
            wcscpy(Reuse_Include, yylval.yy_string.str_ptr);
            chk_newline(L"#include");
            if( wcschr(Path_chars, *yylval.yy_string.str_ptr) ||
                (wcschr(Path_chars, L':') && (yylval.yy_string.str_ptr[1] == L':'))) {
                /*
                **  we have a string which either has a 1st char which is a path
                **  delimiter or, if ':' is a path delimiter (DOS), which has
                **  "<drive letter>:" as the first two characters.  Such names
                **  specify a fully qualified pathnames. Do not append the search
                **  list, just look it up.
                */
                if( ! newinput(yylval.yy_string.str_ptr, MAY_OPEN)) {
                    Msg_Temp = GET_MSG (1015);
                    SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
                    fatal (1015); /* can't find include file */
                }
            }
            else if( (Basic_token != L_STRING) || (! nested_include())) {
                in_standard();
            }
            break;
        case P0_IFDEF :
        case P0_IFNDEF :
            if(CHARMAP(c = skip_cwhite()) != LX_ID) {
                strcpy (Msg_Text, GET_MSG (1016));
                fatal (1016);
            }
            getid(c);
            eval = (get_defined()) ? TRUE : FALSE;
            chk_newline((deftok == P0_IFDEF) ? L"#ifdef" : L"#ifndef");
            if(deftok == P0_IFNDEF) {
                eval = ( ! eval );
            }
            if( eval || ((condition = skipto(ELSE_OR_ENDIF)) == GOT_ELSE) ) {
                /*
                **  expression is TRUE or when we skipped the false part
                **  we found a #else that will be expanded.
                */
                ifstack[incr_ifstack()] = GOT_IF;
            } else if(condition == GOT_ELIF) {
                /* hash is wrong, but it won't be used */
                HLN_NAME(identifier) = L"if";                /* sleazy HACK */
                goto start;
            }
            break;
        case P0_IF :
            old_prep = Prep;
            Prep = FALSE;
            InIf = TRUE;
            eval = do_constexpr();
            InIf = FALSE;
            Prep = old_prep;
            chk_newline(PPifel_str /* "#if/#elif" */);
            if((eval) || ((condition = skipto(ELSE_OR_ENDIF)) == GOT_ELSE) ) {
                /*
                **  expression is TRUE or when we skipped the false part
                **  we found a #else that will be expanded.
                */
                ifstack[incr_ifstack()] = GOT_IF;
                if(Eflag && !eval)
                    emit_line();
            } else {
                /*
                **  here the #if's expression was false, so we skipped until we found
                **  an #elif. we'll restart and fake that we're processing a #if
                */
                if(Eflag)
                    emit_line();
                if(condition == GOT_ELIF) {
                    /* hash is wrong, but it won't be needed */
                    HLN_NAME(identifier) = L"if";            /* sleazy HACK */
                    goto start;
                }
            }
            break;
        case P0_ELIF :
            /*
            **  here, we have found a #elif. first check to make sure that
            **  this is not an occurrance of a #elif with no preceding #if.
            **  (if Prep_ifstack < 0) then there is no preceding #if.
            */
            if(Prep_ifstack-- < 0) {
                strcpy (Msg_Text, GET_MSG (1018));
                fatal (1018);
            }
            /*
            **  now, the preceding #if/#elif was true, and we've
            **  just found the next #elif. we want to skip all #else's
            **  and #elif's from here until we find the enclosing #endif
            */
            while(skipto(ELSE_OR_ENDIF) != GOT_ENDIF) {
                ;
            }
            if(Eflag)
                emit_line();
            break;
        case P0_ELSE :      /*  the preceding #if/#elif was true  */
            if((Prep_ifstack < 0) || (ifstack[Prep_ifstack--] != GOT_IF)) {
                strcpy (Msg_Text, GET_MSG (1019));
                fatal (1019); /*  make sure there was one  */
            }
            chk_newline(PPelse_str /* "#else" */);
            skipto(ENDIF_ONLY);
            if(Eflag)
                emit_line();
            break;
        case P0_ENDIF :     /*  only way here is a lonely #endif  */
            if(Prep_ifstack-- < 0) {
                strcpy (Msg_Text, GET_MSG (1020));
                fatal (1020);
            }
            if(Eflag)
                emit_line();
            chk_newline(PPendif_str /* "#endif" */);
            break;
        case P0_PRAGMA :
            cp = pragma();
            if (cp != 0) {
                if (cp == CP_WINUNICODE) {
                    strcpy (Msg_Text, GET_MSG (4213));
                    if (fWarnInvalidCodePage) {
                        warning(4213);
                    } else {
                        fatal(4213);
                    }
                    break;
                }
                if (!IsValidCodePage(cp)) {
                    strcpy (Msg_Text, GET_MSG (4214));
                    if (fWarnInvalidCodePage) {
                        warning(4214);
                    } else {
                        fatal(4214);
                    }
                    break;
                }
                if (cp != uiCodePage) {
                    if (!io_restart(cp)) {
                        strcpy (Msg_Text, GET_MSG (1121));
                        fatal(1121);
                    }
                    uiCodePage = cp;    // can't be set until now!
                }
            }
            break;
        case P0_UNDEF :
            if(CHARMAP(c = skip_cwhite()) != LX_ID) {
                strcpy (Msg_Text, GET_MSG (4006));
                warning(4006);      /* missing identifier on #undef */
            } else {
                getid(c);
                undefine();
            }
            chk_newline(L"#undef");
            break;
        case P0_ERROR:
            {
                PWCHAR      p;

                p = Reuse_W;
                while((c = get_non_eof()) != LX_EOS) {
                    if(c == L'\n') {
                        UNGETCH();
                        break;
                    }
                    *p++ = c;
                }
                *p = L'\0';
            }
            Msg_Temp = GET_MSG (2189);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
            error(2188);
            chk_newline(L"#error");
            break;
        case P0_IDENT:
            old_prep = Prep ;
            Prep = FALSE;
            yylex();
            Prep = old_prep;
            if(Basic_token != L_STRING) {
                Msg_Temp = GET_MSG (4079);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, TS_STR(Basic_token));
                warning(4079);
            }
            chk_newline(L"#error");
            break;
        case P0_NOTOKEN:
            Msg_Temp = GET_MSG (1021);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, HLN_IDENTP_NAME(&identifier));
            fatal (1021);
            break;
    }
    On_pound_line = FALSE;
}


/************************************************************************
 * SKIPTO - skip code until the end of an undefined block is reached.
 *
 * ARGUMENTS
 *      short key - skip to an ELSE or ENDIF or just an ENDIF
 *
 * RETURNS  - none
 *
 * SIDE EFFECTS
 *      - throws away input
 *
 * DESCRIPTION
 *      The preprocessor is skipping code between failed ifdef, etc. and
 *      the corresponding ELSE or ENDIF (when key == ELSE_OR_ENDIF).
 *      Or it is skipping code between a failed ELSE and the ENDIF (when
 *      key == ENDIF_ONLY).
 *
 * AUTHOR - Ralph Ryan, Sept. 16, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int
skipto(
    int key
    )
{
    REG int             level;
    REG token_t tok;

    level = 0;
    tok = P0_NOTOKEN;
    for(;;) {
        /* make sure that IF [ELSE] ENDIF s are balanced */
        switch(next_control()) {
            case P0_IFDEF:
            case P0_IFNDEF:
            case P0_IF:
                level++;
                break;
            case P0_ELSE:
                tok = P0_ELSE;
                /*
                            **  FALLTHROUGH
                            */
            case P0_ELIF:
                /*
                **  we found a #else or a #elif. these have their only chance
                **  at being valid if they're at level 0.
                **  if we're at any other level,
                **  then this else/elif belongs to some other #if and we skip them.
                **  if we were looking for an endif, the we have an error.
                */
                if(level != 0) {
                    tok = P0_NOTOKEN;
                    break;
                }
                if(key == ENDIF_ONLY) {
                    strcpy (Msg_Text, GET_MSG (1022));
                    fatal (1022);   /* expected #endif */
                } else if(tok == P0_ELSE) {
                    chk_newline(PPelse_str /* "#else" */);
                    return(GOT_ELSE);
                } else {
                    return(GOT_ELIF);
                }
                break;
            case P0_ENDIF:
                if(level == 0) {
                    chk_newline(PPendif_str /* "#endif" */);
                    return(GOT_ENDIF);
                } else {
                    level--;
                }
                break;
        }
    }
}


/*************************************************************************
**  in_standard : search for the given file name in the directory list.
**              Input : ptr to include file name.
**              Output : fatal error if not found.
*************************************************************************/
void
in_standard(
    void
    )
{
    int     i;
    int     stop;
    WCHAR   *p_dir;
    WCHAR   *p_file;
    WCHAR   *p_tmp;

    stop = Includes.li_top;

    for(i = MAXLIST-1; i >= stop; i--) {
        p_file = yylval.yy_string.str_ptr;
        if( ((p_dir = Includes.li_defns[i])!=0) &&(wcscmp(p_dir, L"./") != 0) ) {
            /*
            **  there is a directory to prepend and it's not './'
            */
            p_tmp = Exp_ptr;
            while((*p_tmp++ = *p_dir++) != 0)
                ;
            /*
            **  above loop increments p_tmp past null.
            **  this replaces that null with a '/' if needed.  Not needed if the
            **  last character of the directory spec is a path delimiter.
            **  we then point to the char after the '/'
            */
            if(wcschr(Path_chars, p_dir[-2]) == 0) {
                p_tmp[-1] = L'/';
            } else {
                --p_tmp;
            }
            while((*p_tmp++ = *p_file++) != 0)
                ;
            p_file = Exp_ptr;
        }
        if(newinput(p_file,MAY_OPEN)) { /* this is the non-error way out */
            return;
        }
    }
    Msg_Temp = GET_MSG (1015);
    SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, yylval.yy_string.str_ptr);
    fatal (1015);       /* can't find include file */
}


/*************************************************************************
**  chk_newline : check for whitespace only before a newline.
**  eat the newline.
*************************************************************************/
void
chk_newline(
    PWCHAR cmd
    )
{
    if(skip_cwhite() != L'\n') {
        Msg_Temp = GET_MSG (4067);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, cmd);
        warning(4067);          /* cmd expected newline */
        skip_cnew();
    } else {
        UNGETCH();
    }
}

/*************************************************************************
**  skip_quoted : skips chars until it finds a char which matches its arg.
*************************************************************************/
void
skip_quoted(
    int sc
    )
{
    REG WCHAR   c;

    for(;;) {
        switch(CHARMAP(c = GETCH())) {
            case LX_NL:
                strcpy (Msg_Text, GET_MSG (4093));
                warning(4093);
                UNGETCH();
                return;
                break;
            case LX_DQUOTE:
            case LX_SQUOTE:
                if(c == (WCHAR)sc)
                    return;
                break;
            case LX_EOS:
                if(handle_eos() == BACKSLASH_EOS) {
                    SKIPCH();       /* might be /" !! */
                }
                break;
            case LX_LEADBYTE:
                get_non_eof();
                break;
        }
    }
}


/*************************************************************************
**  next_control : find a newline. find a pound sign as the first non-white.
**  find an id start char, build an id look it up and return the token.
**  this knows about strings/char const and such.
*************************************************************************/
token_t
next_control(
    void
    )
{
    REG WCHAR   c;

    for(;;) {
        c = skip_cwhite();
first_switch:
        switch(CHARMAP(c)) {
            case LX_NL:
                Linenumber++;
                // must manually write '\r' with '\n' when writing 16-bit strings
                if(Prep) {
                    myfwrite(L"\r\n", 2 * sizeof(WCHAR), 1, OUTPUTFILE);
                }
                if((c = skip_cwhite()) == L'#') {
                    if(LX_IS_IDENT(c = skip_cwhite())) {
                        /*
                        **  this is the only way to return to the caller.
                        */
                        getid(c);
                        return(is_pkeyword(Reuse_W));       /* if its predefined  */
                    }
                }
                goto first_switch;
                break;
            case LX_DQUOTE:
            case LX_SQUOTE:
                skip_quoted(c);
                break;
            case LX_EOS:
                if(handle_eos() == BACKSLASH_EOS) {
                    SKIPCH();       /* might be \" !! */
                }
                break;
        }
    }
}


/*************************************************************************
**  do_defined : does the work for the defined(id)
**              should parens be counted, or just be used as delimiters (ie the
**              first open paren matches the first close paren)? If this is ever
**              an issue, it really means that there is not a legal identifier
**              between the parens, causing an error anyway, but consider:
**              #if (defined(2*(x-1))) || 1
**              #endif
**              It is friendlier to allow compilation to continue
*************************************************************************/
int
do_defined(
    PWCHAR p_tmp
    )
{
    REG UINT    c;
    REG int     value=0;
    int         paren_level = 0;

    /*
    ** we want to allow:
    **      #define FOO             defined
    **      #define BAR(a,b)        a FOO | b
    **      #define SNAFOO          0
    **      #if FOO BAR
    **      print("BAR is defined");
    **      #endif
    **      #if BAR(defined, SNAFOO)
    **      print("FOO is defined");
    **      #endif
    */
    if(wcscmp(p_tmp,L"defined") != 0) {
        return(0);
    }
    if((!can_get_non_white()) && (Tiny_lexer_nesting == 0)) {
        /* NL encountered */
        return(value);
    }
    if((c = CHECKCH())== L'(') { /* assumes no other CHARMAP form of OPAREN */
        *Exp_ptr++ = (WCHAR)c;
        SKIPCH();
        paren_level++;
        if((!can_get_non_white()) && (Tiny_lexer_nesting == 0)) {
            /* NL encountered */
            return(value);
        }
    }
    if(Tiny_lexer_nesting>0) {
        if((CHARMAP((WCHAR)(c=CHECKCH()))==LX_MACFORMAL) || (CHARMAP((WCHAR)c)==LX_ID)) {
            SKIPCH();
            tl_getid((UCHAR)c);
        }
    } else {
        if(LX_IS_IDENT(((WCHAR)(c = CHECKCH())))) {
            SKIPCH();
            if(Macro_depth >0) {
                lex_getid((WCHAR)c);
            } else {
                getid((WCHAR)c);
            }
            value = (get_defined()) ? TRUE : FALSE;
        } else {
            if(paren_level==0) {
                strcpy (Msg_Text, GET_MSG (2003));
                error(2003);
            } else {
                strcpy (Msg_Text, GET_MSG (2004));
                error(2004);
            }
        }
    }
    if((CHARMAP(((WCHAR)(c = CHECKCH()))) == LX_WHITE) || (CHARMAP((WCHAR)c) == LX_EOS)) {
        if( ! can_get_non_white()) {
            return(value);
        }
    }
    if(paren_level) {
        if((CHARMAP(((WCHAR)(c = CHECKCH()))) == LX_CPAREN)) {
            SKIPCH();
            paren_level--;
            *Exp_ptr++ = (WCHAR)c;
        }
    }
    if((paren_level > 0) && (Tiny_lexer_nesting == 0)) {
        strcpy (Msg_Text, GET_MSG (4004));
        warning(4004);
    }
    return(value);
}


/*************************************************************************
 * NEXTIS - The lexical interface for #if expression parsing.
 * If the next token does not match what is wanted, return FALSE.
 * otherwise Set Currtok to L_NOTOKEN to force scanning on the next call.
 * Return TRUE.
 * will leave a newline as next char if it finds one.
 *************************************************************************/
int
nextis(
    register token_t tok
    )
{
    if(Currtok != L_NOTOKEN) {
        if(tok == Currtok) {
            Currtok = L_NOTOKEN;                        /*  use up the token  */
            return(TRUE);
        } else {
            return(FALSE);
        }
    }
    switch(yylex()) {                           /*  acquire a new token  */
        case 0:
            break;
        case L_CONSTANT:
            if( ! IS_INTEGRAL(TR_BTYPE(yylval.yy_tree))) {
                    strcpy (Msg_Text, GET_MSG (1017));
                    fatal (1017);
            } else {
                Currval = TR_LVALUE(yylval.yy_tree);
            }
            if(tok == L_CINTEGER) {
                return(TRUE);
            }
            Currtok = L_CINTEGER;
            break;
        case L_IDENT:
            Currval = do_defined(HLN_IDENTP_NAME(&yylval.yy_ident));
            if(tok == L_CINTEGER) {
                return(TRUE);
            }
            Currtok = L_CINTEGER;
            break;
        default:
            if(tok == Basic_token) {
                return(TRUE);
            }
            Currtok = Basic_token;
            break;
    }
    return(FALSE);
}


/************************************************************************
**  skip_cnew : reads up to and including the next newline.
************************************************************************/
void
skip_cnew(
    void
    )
{
    for(;;) {
        switch(CHARMAP(GETCH())) {
            case LX_NL:
                UNGETCH();
                return;
            case LX_SLASH:
                skip_comment();
                break;
            case LX_EOS:
                handle_eos();
                break;
        }
    }
}


/************************************************************************
**  skip_NLonly : reads up to the next newline, disallowing comments
************************************************************************/
void
skip_NLonly(
    void
    )
{
    for(;;) {
        switch(CHARMAP(GETCH())) {
            case LX_NL:
                UNGETCH();
                return;
            case LX_EOS:
                handle_eos();
                break;
        }
    }
}


/************************************************************************
**  pragma : handle processing the pragma directive
**  called by preprocess() after we have seen the #pragma
**  and are ready to handle the keyword which follows.
************************************************************************/
unsigned long
pragma(
    void
    )
{
    WCHAR   c;
    unsigned long int cp=0;

    c = skip_cwhite();
    if (c != L'\n') {
        getid(c);
        _wcsupr(Reuse_W);
        if (wcscmp(L"CODE_PAGE", Reuse_W) == 0) {
            if ((c = skip_cwhite()) == L'(') {
                c = skip_cwhite();  // peek token
                if (iswdigit(c)) {
                    token_t tok;
                    int old_prep = Prep;

                    Prep = FALSE;
                    tok = getnum(c);
                    Prep = old_prep;

                    switch(tok) {
                        default:
                        case L_CFLOAT:
                        case L_CDOUBLE:
                        case L_CLDOUBLE:
                        case L_FLOAT:
                        case L_DOUBLE:
                            break;
                        case L_CINTEGER:
                        case L_LONGINT:
                        case L_CUNSIGNED:
                        case L_LONGUNSIGNED:
                        case L_SHORT:
                        case L_LONG:
                        case L_SIGNED:
                        case L_UNSIGNED:
                            cp = TR_LVALUE(yylval.yy_tree);
                            break;
                    }
                }
                if (cp == 0) {
                    getid(c);
                    _wcsupr(Reuse_W);
                    if (wcscmp(L"DEFAULT", Reuse_W) == 0) {
                        cp = uiDefaultCodePage;
                    } else {
                        wsprintfA(Msg_Text, "%s%ws", GET_MSG(4212), Reuse_W);
                        error(4212);
                    }
                }
                if ((c = skip_cwhite()) != L')') {
                    UNGETCH();
                    strcpy (Msg_Text, GET_MSG (4211));
                    error(4211);
                }
            } else {
                UNGETCH();
                strcpy (Msg_Text, GET_MSG (4210));
                error(4210);
            }

            swprintf(Reuse_W, L"#pragma code_page %d\r\n", cp);
            myfwrite(Reuse_W, wcslen(Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);
        }
    }
    // Skip #pragma statements
    while((c = get_non_eof()) != L'\n');
    UNGETCH();
    return cp;
}
