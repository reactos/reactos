/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* P0MACROS.C - Preprocessor Macros definitions                         */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"


/************************************************************************
**
**      WARNING:        gather_chars() depends ELIMIT being the boundary of
**              Macro_buffer.
************************************************************************/
#define ACT_BUFFER              &Macro_buffer[0]
#define EXP_BUFFER              &Macro_buffer[BIG_BUFFER * 2]
#define EXP_PAD                 5
#define ALIMIT                  &Macro_buffer[BIG_BUFFER * 2]
#define ELIMIT                  (&Macro_buffer[BIG_BUFFER * 4] - EXP_PAD)


/************************************************************************
**  actual argument lists are length preceeded strings which are copied
**  into ACT_BUFFER. the first argument is pt'd to by exp_actuals in the
**  expansion_t construct. the next actual is obtained by adding the length
**  of the current actual to the start of the current actual.
************************************************************************/
#define ACTUAL_SIZE(P)  (*(short *)(P))
#define ACTUAL_TEXT(P)  ((ptext_t)(((char *)(P)) + sizeof(short)))
#define ACTUAL_NEXT(P)  ((ptext_t)(((char *)(P)) + ACTUAL_SIZE(P)))


expansion_t     Macro_expansion[LIMIT_MACRO_DEPTH];

ptext_t P_defn_start;
int             N_formals;
pdefn_t Defn_level_0[LEVEL_0 + 1];


/************************************************************************
**      These are needed by p0scanner (Exp_ptr,Tiny_lexer_nesting)
************************************************************************/
ptext_t Exp_ptr = EXP_BUFFER;           /* ptr to free exp space */
int             Tiny_lexer_nesting;     /* stay in tiny lexer or back to main */

static  ptext_t Act_ptr = ACT_BUFFER;           /* ptr to free actuals space */
static  ptext_t Save_Exp_ptr = EXP_BUFFER;      /* for buffering unbal parens */

static  ptext_t P_actuals;              /* actuals for this (level) macro */
static  int             N_actuals;      /* number of actuals in invocation */
static  int             Macro_line;     /*  where we started the macro  */


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
void    chkbuf(ptext_t);
ptext_t do_strformal(void);
ptext_t do_macformal(int *);
void    expand_actual(UCHAR);
void    expand_definition(void);
void    expand_macro(void);
void    fatal_in_macro(int);
ptext_t gather_chars(ptext_t, WCHAR);
void    get_actuals(pdefn_t, int);
int     get_definition(void);
void    get_formals(void);
int     is_macro_arg(ptext_t);
void    move_to_actual(ptext_t, ptext_t);
void    move_to_exp(ptext_t);
void    move_to_exp_esc(int, ptext_t);
int     post_paste(void);
void    push_macro(pdefn_t);
int     redefn (ptext_t, ptext_t, int);
int     rescan_expansion(void);


/************************************************************************
** UNDEFINE - remove a symbol from the symbol table
**      No noise is made if the programmer attempts to undefine a predefined
**              macro, but it is not done.
************************************************************************/
void
undefine(
    void
    )
{
    pdefn_t     pdef;
    pdefn_t     prev;

    prev = NULL;
    pdef = Defn_level_0[Reuse_W_hash & LEVEL_0];
    while(pdef) {
        if(memcmp (Reuse_W, DEFN_IDENT(pdef), Reuse_W_length * sizeof(WCHAR)) == 0) {
            if(PRE_DEFINED(pdef)) {
                Msg_Temp = GET_MSG (4117);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W, "#undef");
                warning(4117);
                break;
            }
            if(prev == NULL)    /*  match at head of list  */
                Defn_level_0[Reuse_W_hash & LEVEL_0] = DEFN_NEXT(pdef);
            else
                DEFN_NEXT(prev) = DEFN_NEXT(pdef);

            if (wcscmp(DEFN_NAME(pdef), afxSzHiddenSymbols) == 0)
                afxHiddenSymbols = FALSE;
            if (wcscmp(DEFN_NAME(pdef), afxSzReadOnlySymbols) == 0)
                afxReadOnlySymbols = FALSE;

            break;
        }
        prev = pdef;
        pdef = DEFN_NEXT(pdef);
    }
}


/************************************************************************
**  BEGIN DEFINE A MACRO {
************************************************************************/
void
define(
    void
    )
{
    WCHAR       c;

    if (! (LX_IS_IDENT(c = skip_cwhite())) ) {
        strcpy (Msg_Text, GET_MSG (2007));
        error (2007); /* #define syntax */
        skip_cnew();
        return;
    }
    getid(c);
    N_formals = 0;
    P_defn_start = Macro_buffer;
/*
**  the next character must be white space or an open paren
*/
first_switch:
    switch(CHARMAP(c = GETCH())) {
        case LX_OPAREN:                 /*  we have formal parameters  */
            get_formals();              /*  changes N_formals and fills Macro_buffer */
            if(N_formals == 0) {        /*  empty formal list  */
                /*
                **  we must special case this since the expand() reads in the
                **  actual arguments iff there are formal parameters. thus if we
                **      #define foo()   bar()
                **              . . .
                **              foo()
                **  will expand as
                **              bar()()
                **  we put the right paren in to fool the expander into looking
                **  for actuals.
                */
                N_formals = -1;
            }
            break;
        case LX_WHITE:
            break;
        case LX_CR:
            goto first_switch;
        case LX_SLASH:
            if( ! skip_comment()) {
                Msg_Temp = GET_MSG (2008);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, L'/');
                error (2008);
            }
            break;
        case LX_NL:                 /* no definition */
            UNGETCH();
            definstall((ptext_t)0, 0, 0);
            return;
            break;
        case LX_EOS:
            if(handle_eos() != BACKSLASH_EOS) {
                goto first_switch;
            }
            /* got BACKSLASH_EOS */
            /*
            **  FALLTHROUGH
            */
        default:
            Msg_Temp = GET_MSG (2008);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
            error (2008); /* unexpected character in macro definition */
    }
    definstall(P_defn_start, get_definition(), N_formals);
}


/************************************************************************
**  get_formals : collect comma separated idents until the first closing paren
**  (the openning paren has already been read)
**  since we can't be in a macro when we're asked for this, we can be assured
**  that we can use a single buffer to collect all the formal names.
************************************************************************/
void
get_formals(
    void
    )
{
    WCHAR       c;
    ptext_t     p_stop;
    ptext_t     p_id;

    p_id = p_stop = ACTUAL_TEXT(P_defn_start);
    for(;;) {
        switch(CHARMAP(c = skip_cwhite())) {
            case LX_ID:
                if( p_id != p_stop ) {
                    Msg_Temp = GET_MSG (2010);
                    SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
                    error (2010);
                }
                *p_stop++ = c;
                for(;;) {
                    while(LXC_IS_IDENT(c = GETCH())) {      /* while an id char */
                        *p_stop++ = c;                      /* collect it */
                    } if(c == EOS_CHAR) {
                            /*
                            **  found end of buffer marker, make sure it is,
                            **  then handle it.
                            */
                        if(io_eob()) {      /* end of buffer in here is bad */
                            strcpy (Msg_Text, GET_MSG (1004));
                            fatal (1004);
                        }
                        continue;
                    }
                    if((c == L'\\') && (checknl())) {
                        continue;
                    }
                    UNGETCH();
                    break;
                }
                *p_stop++ = L'\0';
                break;
            case LX_COMMA:
            case LX_CPAREN:
                if( p_stop > p_id ) {
                    /* make sure an identifier was read */
                    if((p_stop - p_id) >= TINY_BUFFER) {
                        p_id[TINY_BUFFER - 1] = L'\0';
                        strcpy (Msg_Text, GET_MSG (4111));
                        warning(4011);              /* id truncated */
                        p_stop = p_id + TINY_BUFFER;
                    }
                    if(is_macro_arg(p_id) >= 1) {
                        Msg_Temp = GET_MSG (2009);
                        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, p_id);
                        error(2009);                /* reuse of formal */
                    } else {
                        ACTUAL_SIZE(P_defn_start) = (short)(p_stop - P_defn_start) * sizeof(WCHAR);
                        P_defn_start = p_stop;
                        N_formals++;
                    }
                } else {
                    if( (CHARMAP(c) == LX_COMMA) || (N_formals > 0) ) {
                        Msg_Temp = GET_MSG (2010);
                        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
                        error(2010);
                    }
                }
                if( CHARMAP(c) == LX_CPAREN ) {
                    return;
                }
                p_id = p_stop = ACTUAL_TEXT(P_defn_start);
                break;
            default:
                Msg_Temp = GET_MSG (2010);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
                error(2010); /*  unexpected char in formal list */
                break;
        }
    }
}


/************************************************************************
** definstall - Install a new definition. id is in Reuse_W.
**      p_text : ptr to the definition
**      n : number of bytes in the definition (may contain embedded nulls)
**      number : number of formals
************************************************************************/
void
definstall(
    WCHAR * p_text,
    int n,
    int number
    )
{
    pdefn_t     p;

    if(n == 0) {
        p_text = NULL;
    }
    if( wcscmp (Reuse_W, L"defined") == 0) {
        Msg_Temp = GET_MSG (4117);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W, "#define");
        warning(4117);/* name reserved */
        return;
    }
    if((p = get_defined()) != 0) {
        if(PRE_DEFINED(p)) {
            Msg_Temp = GET_MSG (4117);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W, "#define");
            warning(4117);/* name reserved */
            return;
        } else {
            if(redefn(p_text, DEFN_TEXT(p), n)) {
                Msg_Temp = GET_MSG (4005);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
                warning(4005);/* redefinition */
            } else {
                return;
            }
        }
    } else {
        hln_t   ident;

        HLN_NAME(ident) = Reuse_W;
        HLN_HASH(ident) = Reuse_W_hash;
        HLN_LENGTH(ident) = (UINT)Reuse_W_length;
        p = (pdefn_t) MyAlloc(sizeof(defn_t));
        if (p == NULL) {
            strcpy (Msg_Text, GET_MSG (1002));
            error(1002);
            return;
        }
        DEFN_IDENT(p) = HLN_TO_NAME(&ident);
        DEFN_NEXT(p) = Defn_level_0[Reuse_W_hash & LEVEL_0];
        DEFN_TEXT(p) = (WCHAR*)NULL;
        DEFN_EXPANDING(p) = 0;
        Defn_level_0[Reuse_W_hash & LEVEL_0] = p;
    }
    if(n != 0) {
        DEFN_TEXT(p) = pstrndup(p_text, n);
        if(number == FROM_COMMAND) {    /* special case from cmd line */
            *(DEFN_TEXT(p) + n - 1) = EOS_DEFINITION;   /* for handle_eos */
        }
    }
    DEFN_NFORMALS(p) = (char)((number != FROM_COMMAND) ? number : 0);

    if (fAFXSymbols && !PRE_DEFINED(p) && DEFN_NFORMALS(p) == 0)
        AfxOutputMacroDefn(p);
}


/************************************************************************
**  get_defined : is the given id in the macro symbol table?
**  return a ptr to it if so, NULL if not.
************************************************************************/
pdefn_t
get_defined(
    void
    )
{
    pdefn_t     pdef;

    for( pdef = Defn_level_0[Reuse_W_hash & LEVEL_0]; pdef;
        pdef = DEFN_NEXT(pdef))         {
        if(memcmp (Reuse_W, DEFN_IDENT(pdef), Reuse_W_length * sizeof(WCHAR)) == 0) {
            return(pdef);
        }
    }
    return(NULL);
}


/************************************************************************
**  redefn : are the two definitions the same?
************************************************************************/
int
redefn(
    REG  PWCHAR p_new,
    PWCHAR p_old,
    int n
    )
{
    if(p_old && p_new) {
        if(wcsncmp(p_new, p_old, n) == 0) {     /* strings are exact */
            return(FALSE);
        }
        return(TRUE);
    }
    return((p_old != NULL) || (p_new != NULL));
}


/************************************************************************
**  get_definition : accumulate the macro definition, stops when it finds
**  a newline (it uses it). returns a ptr to the end of the string it builds.
**  builds the string in Macro_buffer. (given the start in P_defn_start)
************************************************************************/
int
get_definition(
    void
    )
{
    REG ptext_t p;
    WCHAR       c;
    int         stringize = FALSE;
    int         charize = FALSE;

    p = P_defn_start;
    c = skip_cwhite();
    for(;;) {
        chkbuf(p);
        switch(CHARMAP(c)) {
            case LX_EOS:
                if(handle_eos() == BACKSLASH_EOS) {
                    /* got backslash EOS */
                    /* \<anything else> goes out as is.  The <anything else>
                     * character must be emitted now, so that
                     *               #define FOO(name)       \name
                     *               . . .
                     *               FOO(bar)
                     *
                     * does NOT see occurence of name in the definition as an
                     * occurence of the formal param and emit \bar when it is
                     * expanded later,but if the definition is \nname it will
                     * find name as a formal paramater and emit \nbar
                     */
                    *p++ = c;       /* put in backslash, break'll add new char */
                    c = get_non_eof();
                } else {
                    c = GETCH();
                    continue;
                }
                break;
            case LX_NL:             /*  only way out  */
                UNGETCH();
                if(p == P_defn_start) {
                    return(0);
                }
                chkbuf(p);
                *p++ = EOS_CHAR;
                *p++ = EOS_DEFINITION;      /* tells handle_eos defn finished */
                return((int)(p - P_defn_start));/* p's last incr counts the 0*/
                break;
            case LX_DQUOTE:
            case LX_SQUOTE:
                p = gather_chars(p, c);
                c = GETCH();
                continue;
                break;
            case LX_POUND:
split_op:
                switch(CHARMAP(GETCH())) {
                    case LX_POUND:
                        /*
                        **  handle ## processing. cant be the first or the last.
                        */
                        if(p == P_defn_start) {
                            strcpy (Msg_Text, GET_MSG (2160));
                            error(2160);        /* ## not allowed as first entry */
                            continue;
                        }
                        if(*(p - 1) == L' ') {   /* hose the last blank */
                            p--;
                        }
                        if(CHARMAP(c = skip_cwhite()) == LX_NL) {
                            UNGETCH();
                            strcpy (Msg_Text, GET_MSG(2161));
                            error(2161);
                            continue;
                        }
                        /* this case does *not* fall through to LX_ID */
                        continue;
                        break;
                    case LX_EACH:
                        charize = TRUE;
                        break;
                    case LX_EOS:
                        if( handle_eos() != BACKSLASH_EOS ) {
                            goto split_op;
                        }
                        /*
                        **      FALLTHROUGH
                        */
                    default:
                        UNGETCH();
                        stringize = TRUE;
                        break;
                }
                if(CHARMAP(c = skip_cwhite()) != LX_ID) {
                    strcpy (Msg_Text, GET_MSG(2162));
                    error(2162);    /* must have id following */
                    continue;
                }
                /*
                **  FALLTHROUGH
                */
            case LX_ID:
                {
                    /* we have the start of an identifier - check it to see if
                     * its an occurence of a formal parameter name.
                     * we gather the id ourselves (instead of getid()) since this
                     * wil save us from having to copy it to our string if it's
                     * not a formal parameter.
                     */
                    int                     n;
                    ptext_t p_macformal;

                    p_macformal = p;
                    do {
                        chkbuf(p);
                        *p++ = c;
get_more_id:
                        c = GETCH();
                    } while(LXC_IS_IDENT(c));
                    if(CHARMAP(c) == LX_EOS) {
                        if(handle_eos() != BACKSLASH_EOS) {
                            goto get_more_id;
                        }
                    }
                    *p = L'\0'; /* term. string, but do not advance ptr */
                    if((n = is_macro_arg(p_macformal)) >= 1) {
                        /*
                        **  this is an occurance of formal 'n', replace the id with
                        **  the special MAC character.
                        */
                        p = p_macformal;
                        if(stringize) {
                            *p++ = LX_FORMALSTR;
                        } else {
                            if(charize) {
                                *p++ = LX_FORMALCHAR;
                            } else {
                                *p++ = LX_FORMALMARK;
                            }
                        }
                        *p++ = (WCHAR) n;
                    } else if(charize || stringize) {
                        strcpy (Msg_Text, GET_MSG(2162));
                        error(2162);
                    }
                    stringize = FALSE;
                    charize = FALSE;
                    continue;       /* we broke out of the loop with a new char */
                }
            case LX_SLASH:
                if( ! skip_comment() ) {    /* really is a slash */
                    break;
                }
                /*
                **  FALLTHROUGH
                */
            case LX_CR:
            case LX_WHITE:
                /*
                **  this is white space, all contiguous whitespace is transformed
                **  to 1 blank. (hence the skip_cwhite() and the continue).
                */
                if(CHARMAP(c = skip_cwhite()) != LX_NL) {
                    *p++ = L' ';
                }
                continue;                           /* restart loop */
            case LX_ILL:
                Msg_Temp = GET_MSG (2018);
                SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, c);
                error(2018);
                c = GETCH();
                continue;
        }
        *p++ = c;
        c = GETCH();
    }
}


/************************************************************************/
/* is_macro_arg ()                                                      */
/************************************************************************/
int
is_macro_arg(
    ptext_t name
    )
{
    REG int     i;
    REG ptext_t p;

    p = Macro_buffer;
    for(i = 1; i <= N_formals; i++) {
        if( wcscmp(name, ACTUAL_TEXT(p)) == 0) {
            return(i);
        }
        p = ACTUAL_NEXT(p);
    }
    return(-1);
}



/************************************************************************/
/* chkbuf ()                                                            */
/************************************************************************/
void
chkbuf(
    ptext_t p
    )
{
    if( p >= ELIMIT ) {
        Msg_Temp = GET_MSG (1011);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
        fatal (1011);
    }
}


/************************************************************************
**  gather_chars : collect chars until a matching one is found.
**  skip backslashed chars. moves the chars into the buffer,
**  returns a ptr past the last char copied.
************************************************************************/
ptext_t
gather_chars(
    REG ptext_t p,
    WCHAR match_c
    )
{
    WCHAR       c;

    *p++ = match_c;
    for(;;) {
        if(p > ELIMIT) {
            return(ELIMIT);
        }
        switch(CHARMAP(c = GETCH())) {
            case LX_NL:
                strcpy (Msg_Text, GET_MSG(2001));
                error(2001);
                UNGETCH();
                c = match_c;
                /*
                **  FALLTHROUGH
                */
            case LX_DQUOTE:
            case LX_SQUOTE:
                if(c == match_c) {
                    *p++ = c;
                    return(p);              /* only way out */
                }
                break;
            case LX_EOS:
                if(handle_eos() != BACKSLASH_EOS) {
                    continue;
                } else {
                    /* got backslash */
                    *p++ = L'\\';
                    c = get_non_eof();
                    if((c == '\\') && (checknl())) {
                        continue;
                    }
                }
                break;
        }
        *p++ = c;
    }
}
/************************************************************************
**  END DEFINING MACROS }
************************************************************************/

/************************************************************************
**  BEGIN EXPANDING MACROS {
************************************************************************/
/************************************************************************
**      can_expand:             tries to expand the macro passed to it - returns
**              true if it succeeded in expanding it.  It will only return FALSE
**              if a macro name was found, a paren was expected, and a paren was
**              not the next non white character.
************************************************************************/
int
can_expand(
    pdefn_t pdef
    )
{
    WCHAR   c;
    int     n_formals;
    int     return_value = FALSE;

    Tiny_lexer_nesting = 0;
    Save_Exp_ptr = Exp_ptr;             /* not necessarily EXP_BUFFER */
    Macro_line = Linenumber;
expand_name:

    P_actuals = Act_ptr;
    N_actuals = 0;

    n_formals = DEFN_NFORMALS(pdef);
    if( PRE_DEFINED(pdef) ) {
        push_macro(pdef);
        DEFN_EXPANDING(CURRENT_MACRO)++;
        if(rescan_expansion()) {
            return(TRUE);                       /* could expand macro */
        }
    }
    else if( n_formals == 0 ) {
        return_value = TRUE;
        if(DEFN_TEXT(pdef)) {
            push_macro(pdef);
            expand_definition();
        } else {
            /*
            **      Macro expands to nothing (no definition).  Since it
            **      didn't have any actuals, Act_ptr is already correct.
            **      Exp_ptr must be changed however to delete the
            **      identifier from the expanded text.
            */
            Exp_ptr = Save_Exp_ptr;
        }
    } else {
        if( n_formals == -1 ) {
            n_formals = 0;
        }
name_comment_paren:
        if( can_get_non_white()) {
            if(CHARMAP(CHECKCH()) == LX_SLASH) {
                SKIPCH();
                if(skip_comment()) {
                    goto name_comment_paren;
                } else {
                    UNGETCH();
                }
            }
            if(CHARMAP(CHECKCH())==LX_OPAREN) {
                SKIPCH();
                return_value = TRUE;
                get_actuals(pdef, n_formals);
            } else {
                /*
                **      #define xx(a) a
                **  xx bar();
                **  don't lose white space between "xx" and "bar"
                */
                ptext_t p = Exp_ptr;

                push_macro(pdef);
                DEFN_EXPANDING(CURRENT_MACRO)++;
                Exp_ptr = p;
                if( rescan_expansion() ) {
                    return(FALSE);
                }
            }
        } else {
        }
    }
    /*
    **      makes sure a macro is being worked on. At this point, there will
    **      be a macro to expand, unless the macro expand_the_named_macro was
    **      passed had no definition text.  If it had no defintion text,
    **      Tiny_lexer_nesting was not incremented.
    */
    while(Tiny_lexer_nesting != 0) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
        switch(CHARMAP(c = GETCH())) {
            case LX_ID:
            case LX_MACFORMAL:
                Save_Exp_ptr = Exp_ptr;
                if(tl_getid(c) && ((pdef = get_defined())!= 0)) {
                    if(DEFN_EXPANDING(pdef)) {
                        /*
                        **      the macro is already being expanded, so just
                        **      write the do not expand marker and the
                        **      identifier to the expand area.  The do not
                        **      expand marker is necessary so this macro
                        **      doesn't get expanded on the rescan
                        */
                        int         len = Reuse_W_length - 1;

                        *Exp_ptr++ = LX_NOEXPANDMARK;
                        *Exp_ptr++ = ((WCHAR)len);
                    } else {
                        /*
                        ** a legal identifier was read, it is defined, and
                        ** it is not currently being expanded.  This means
                        ** there is reason to believe it can be expanded.
                        */
                        goto expand_name;
                    }
                }
                if(InIf &&(memcmp(Reuse_W, L"defined", 8 * sizeof(WCHAR)) ==0)) {
                    do_defined(Reuse_W);
                }
                continue;
                break;
            case LX_NUMBER:
                /* getnum with Prep on to keep leading 0x on number */
                {
                    int     Save_prep = Prep;
                    Prep = TRUE;
                    getnum(c);
                    Prep = Save_prep;
                }
                continue;
                break;
            case LX_DOT:
                *Exp_ptr++ = L'.';
dot_switch:
                switch(CHARMAP(c = GETCH())) {
                    case LX_EOS:
                        if(handle_eos() != BACKSLASH_EOS) {
                            if(Tiny_lexer_nesting > 0) {
                                goto dot_switch;
                            }
                            continue;
                        }
                        break;
                    case LX_DOT:
                        *Exp_ptr++ = L'.';
                        if( ! checkop(L'.')) {
                            break;      /* error will be caught on rescan */
                        }
                        *Exp_ptr++ = L'.';
                        continue;
                        break;
                    case LX_NUMBER:
                        *Exp_ptr++ = c;
                        get_real(Exp_ptr);
                        continue;
                }
                UNGETCH();
                continue;
            case LX_CHARFORMAL:
                move_to_exp_esc(L'\'', do_strformal());
                continue;
                break;
            case LX_STRFORMAL:
                move_to_exp_esc(L'"', do_strformal());
                continue;
                break;
            case LX_DQUOTE:
            case LX_SQUOTE:
                /*
                **  gather_chars is called even though the error reported
                **  on overflow may need to be changed.
                */
                Exp_ptr = gather_chars(Exp_ptr, c);
                continue;
                break;
            case LX_WHITE:
                while(LXC_IS_WHITE(GETCH())) {
                    ;
                }
                UNGETCH();
                c = L' ';
                break;
            case LX_EOS:
                if(handle_eos() == BACKSLASH_EOS) {
                    *Exp_ptr++ = c;
                    c = GETCH();
                    break;
                }
                continue;
                break;
        }
        *Exp_ptr++ = c;
    }
    return(return_value);
}


/************************************************************************
**  get_actuals :  Paren must already be found.  If all the actuals can
**              be read, the macro is pushed and expansion begins. Otherwise,
**              this function is quickly exited and lets the tiny lexer take
**              care of rescanning.
************************************************************************/
void
get_actuals(
    pdefn_t pdef,
    int n_formals
    )
{
    /*
    **  The only concern with this is that a rescan could finish while
    **  this is trying to collect actuals.  When a rescan finishes, it
    **  may reset Act_ptr and Exp_ptr.  Unless these are saved before the
    **  end of rescan is handled, the part of the actual collected so far
    **  would be lost.
    */
    REG ptext_t start;
    WCHAR       c;
    ptext_t     actuals_start;
    int         paste;
    int         level;

    *Exp_ptr++ = PREVCH();                      /* must be oparen */
    level = 0;
    actuals_start = Act_ptr;

    while( level >= 0) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
more_white:
        if( ! can_get_non_white()) {
            return;
        }
        if(CHARMAP(CHECKCH()) == LX_SLASH) {
            SKIPCH();
            if(skip_comment()) {
                goto more_white;
            } else {
                start = Exp_ptr;
                *Exp_ptr++ = L'/';
            }
        } else {
            start = Exp_ptr;
        }
        paste = FALSE;

        for(;;) {
            switch(CHARMAP(c = GETCH())) {
                case LX_CPAREN:
                    if(--level < 0) {
                        goto leave_loop;
                    }
                    break;
                case LX_COMMA:
                    /*
                    **      if the comma is not at level == 0, it is part of
                    **      a parenthesized list and not a delimiter
                    */
                    if(level == 0) {
                        goto leave_loop;
                    }
                    break;
                case LX_SLASH:
                    if( ! skip_comment()) {
                        break;
                    }
                    if(*(Exp_ptr - 1) == L' ') {
                        continue;
                    }
                    c = L' ';
                    break;
                case LX_CR:
                case LX_NL:
                case LX_WHITE:
                    UNGETCH();              /* This char is valid white space */
                    if( ! can_get_non_white()) {
                        return;
                    }
                    continue;
                    break;
                case LX_OPAREN:
                    ++level;
                    break;
                case LX_DQUOTE:
                case LX_SQUOTE:
                    Exp_ptr = gather_chars(Exp_ptr, c);
                    continue;
                    break;
                case LX_ID:
                    *Exp_ptr++ = c;
                    while(LXC_IS_IDENT(c = GETCH())) {
                        if(Exp_ptr >= ELIMIT) {
                            fatal_in_macro(10056);
                        }
                        *Exp_ptr++ = c;
                    }
                    if(CHARMAP(c) != LX_MACFORMAL) {
                        UNGETCH();
                        continue;
                    }
                    paste = TRUE;
                    /*
                    **      FALLTHROUGH
                    */
                case LX_MACFORMAL:
                    move_to_exp(do_macformal(&paste));
                    continue;
                    break;
                case LX_STRFORMAL:
                    move_to_exp_esc(L'"', do_strformal());
                    continue;
                    break;
                case LX_CHARFORMAL:
                    move_to_exp_esc(L'\'', do_strformal());
                    continue;
                    break;
                case LX_EOS:
                    /*
                    **      Will saving this pointers create dead space in the
                    **      buffers?  Yes, but only temporarily.
                    **
                    **      handle_eos() may reset Act_ptr and Exp_ptr to the
                    **      beginning of the buffers if a rescan is finishing
                    **      and Macro_depth is going to be 0.  ANSI allows
                    **      actuals to start within a macro defintion and be
                    **      completed (further actuals and closing paren) later
                    **      in the text.
                    **
                    **      These buffer pointers will eventually be reset to
                    **      the beginnings of their respective buffers when the
                    **      macro for the actuals being collected right now
                    **      finish rescan
                    **
                    **      This is special handling for folks who use
                    **      unbalanced parens in macro definitions
                    */
                    {
                        ptext_t     Exp_save;
                        ptext_t     Act_save;
                        int eos_res;

                        Exp_save = Exp_ptr;
                        Act_save = Act_ptr;
                        if((eos_res = handle_eos()) & (ACTUAL_EOS | RESCAN_EOS)) {
                            return;
                        }
                        Act_ptr = Act_save;
                        Exp_ptr = Exp_save;
                        if(eos_res == BACKSLASH_EOS) {      /* ??? DFP QUESTION  */
                            *Exp_ptr++ = c;         /*  save the \  */
                            c = get_non_eof();      /*  get char following \  */
                            break;
                        }
                    }
                    continue;
                    break;
            }
            *Exp_ptr++ = c;
        }
leave_loop:
        /*
                **      if the last character was whitespace, hose it
                */
        if(CHARMAP(*(Exp_ptr - 1)) == LX_WHITE) {
            Exp_ptr--;
        }
        /*
        **      if Exp_ptr <= start, foo() was read, don't incr N_actuals
        */
        if(Exp_ptr > start) {
            N_actuals++;
            move_to_actual(start, Exp_ptr);
        }
        *Exp_ptr++ = c;
    }

    P_actuals = actuals_start;
    if(n_formals < N_actuals) {
        Msg_Temp = GET_MSG (4002);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
        warning(4002);
    }
    else if(n_formals > N_actuals) {
        Msg_Temp = GET_MSG (4003);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
        warning(4003);
    }

    if(DEFN_TEXT(pdef)) {
        push_macro(pdef);
        expand_macro();
    } else {
        /*
        **      the macro expands to nothing (no definition)
        **      This essentially means delete the macro and its actuals
        **      from the expanded text
        */
        Act_ptr = P_actuals;    /* reset pointer to get rid of actuals */
        Exp_ptr = Save_Exp_ptr; /* delete macro & actuals from exp text */
    }
}

/************************************************************************
**      rescan_expansion:       pops a level off of tiny lexer.  If this is the
**              original macro called, the rescan is set up, otherwise the MACRO
**              (not only the tiny lexer level) is popped.
************************************************************************/
int
rescan_expansion(
    void
    )
{
    if(--Tiny_lexer_nesting == 0) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
        if (fAFXSymbols && !InIf && (DEFN_NFORMALS(CURRENT_MACRO)==0))
            AfxOutputMacroUse(CURRENT_MACRO);

        *Exp_ptr++ = EOS_CHAR;
        *Exp_ptr++ = EOS_RESCAN;
        Current_char = CURRENT_TEXT;
        return(TRUE);                   /* rescan the expanded text */
    } else {
        /* reset Current_char, pop the macro */

        Current_char = CURRENT_STRING;
        Act_ptr = CURRENT_ACTUALS;      /* don't need its actuals */
        DEFN_EXPANDING(CURRENT_MACRO)--;
        --Macro_depth;
        return(FALSE);                  /* do not rescan expanded text */
    }
}


/************************************************************************
** move_to_actual:      moves the string located between start and finish
**              inclusive to the current location in ACT_BUFFER as a new actual.
************************************************************************/
void
move_to_actual(
    ptext_t start,
    ptext_t finish
    )
{
    REG ptext_t p;
    REG int     len;

    len = (int)(finish - start);
    if(Act_ptr + len >= ALIMIT - 2) {
        fatal_in_macro(10056);
    }
    wcsncpy(ACTUAL_TEXT(Act_ptr), start, len);
    p = ACTUAL_TEXT(Act_ptr);
    p += len;
    if ((((ULONG_PTR)p) & 1) == 0) {
        *p++ = EOS_CHAR;
        *p++ = EOS_ACTUAL;
    } else {
        *p++ = EOS_CHAR;
        *p++ = EOS_PAD;
        *p++ = EOS_ACTUAL;
    }
    ACTUAL_SIZE(Act_ptr) = (short)(p - Act_ptr) * sizeof(WCHAR);
    Act_ptr = p;
}


/************************************************************************
** move_to_exp_esc:     moves zero terminated string starting at source to
**      the current position in EXP_BUFFER, with quotes placed around the
**      string and interior backslashes and dquotes are escaped with a
**      backslash.  The terminating null should not be copied.  The null
**      does not come from the property of a string, but rather is the
**      marker used to indicate there is no more actual.
************************************************************************/
void
move_to_exp_esc(
    int quote_char,
    REG ptext_t source
    )
{
    int     mapped_c;
    int     mapped_quote;
    int     in_quoted = FALSE;

    if( ! source ) {
        return;
    }

    *Exp_ptr++ = (WCHAR)quote_char;
    for(;;) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
        switch(mapped_c = CHARMAP(*source)) {
            case LX_EOS:
                if(*source == EOS_CHAR) {
                    goto leave_move_stringize;
                }
                /* got BACKSLASH */
                /* but it can't be backslash-newline combination because
                                    ** we are reprocessing text already read in
                                    */
                if(in_quoted) {
                    *Exp_ptr++ = L'\\';
                }
                break;

            case LX_DQUOTE:
                if(CHARMAP((WCHAR)quote_char) == LX_DQUOTE) {
                    *Exp_ptr++ = L'\\';
                }
                /*
                **      FALLTHROUGH
                */
            case LX_SQUOTE:
                if(CHARMAP((WCHAR)quote_char) == LX_SQUOTE) {
                    break;
                }
                if(in_quoted ) {
                    if(mapped_c == mapped_quote) {
                        in_quoted = FALSE;
                    }
                } else {
                    in_quoted = TRUE;
                    mapped_quote = mapped_c;
                }
                break;
        }
        *Exp_ptr++ = *source++;
    }

leave_move_stringize:
    *Exp_ptr++ = (WCHAR)quote_char;
}


/************************************************************************
**      move_to_exp:    moves zero terminated string starting at source to
**              the current position in EXP_BUFFER.  The terminating null should
**              not be copied.
************************************************************************/
void
move_to_exp(
    REG ptext_t source
    )
{
    if( ! source ) {
        return;
    }

    while( *source ) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
        *Exp_ptr++ = *source++;
    }
}


/************************************************************************
** push_macro:                  pushes macro information onto the macro stack.
**      Information such as the current location in the Exp and Act buffers
**      will be used by whatever macros this one may call.
************************************************************************/
void
push_macro(
    pdefn_t pdef
    )
{
    /*
    **      note that increment leaves element 0 of the macro stack unused.
    **      this element can be reserved for links to dynamically allocated
    **      macro expansion stacks, if they become desirable
    */
    if(++Macro_depth >= LIMIT_MACRO_DEPTH) {
        Msg_Temp = GET_MSG (1009);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Reuse_W);
        fatal (1009);
    }
    Tiny_lexer_nesting++;
    CURRENT_MACRO = pdef;
    CURRENT_ACTUALS = P_actuals;
    CURRENT_NACTUALS = (UCHAR)N_actuals;
    CURRENT_NACTSEXPANDED = 0;
    CURRENT_STRING = Current_char;
    CURRENT_TEXT = Exp_ptr = Save_Exp_ptr;
}


/************************************************************************
**expand_definition:            sets the input stream to start reading from
**              the macro definition.  Also marks the macro as in the process of
**              expanding so if it eventually invokes itself, it will not expand
**              the new occurence.
************************************************************************/
void
expand_definition(
    void
    )
{
    Current_char = DEFN_TEXT(CURRENT_MACRO);
    DEFN_EXPANDING(CURRENT_MACRO)++;
}


/************************************************************************
**expand_actual:        sets the input stream to start reading from
**              the actual specified in actual.
************************************************************************/
void
expand_actual(
    UCHAR actual
    )
{
    ptext_t     p;
    p = CURRENT_ACTUALS;
    while(--actual) {
        p = ACTUAL_NEXT(p);
    }
    Current_char = ACTUAL_TEXT(p);
}

/************************************************************************
**      expand_macro:           if there are still actuals for this macro to be
**              expanded, the next one is set up, otherwise this sets up to
**              expand the macro definition
************************************************************************/
void
expand_macro(
    void
    )
{
    if(CURRENT_NACTUALS > CURRENT_NACTSEXPANDED) {
        expand_actual(++CURRENT_NACTSEXPANDED);
    } else {
        expand_definition();
    }
}


/************************************************************************
**post_paste:           looks ahead one character to find out if a paste has
**      been requested immediately after this identifier.  If the next
**      character can continue an identifier, or is the macformal marker,
**      a paste should be done.  This is called after a macformal is found
**      to find out if the expanded or unexpanded actual should be used.
************************************************************************/
int
post_paste(
    void
    )
{
    WCHAR       c;

    if((CHARMAP(c = GETCH()) == LX_MACFORMAL) || (LXC_IS_IDENT(c))) {
        UNGETCH();
        return(TRUE);
    }
    UNGETCH();
    return(FALSE);
}

/************************************************************************
**do_macformal:         This function is called after a macformal marker is
**      found.  It reads the next character to find out which macformal is
**      wanted.  Then it checks to see if a paste is wanted, to find out
**      if the expanded or unexpanded actual should be used.  The return
**      value is a pointer to the text of the actual wanted, or NULL if the
**      actual asked for was not provided.
************************************************************************/
ptext_t
do_macformal(
    int *pre_paste
    )
{
    WCHAR       n;
    ptext_t     p;
    int temp_paste;

    p = CURRENT_ACTUALS;
    n = GETCH();
    if(n > CURRENT_NACTUALS) {
        return(NULL);           /* already output warning */
    }
    temp_paste = post_paste();
    if(( ! (*pre_paste)) && ( ! temp_paste) ) {
        /*
        **      if the programmer provided x actuals, actuals x+1 to 2x are
        **      those actuals expanded
        */
        n += CURRENT_NACTUALS;
    }
    *pre_paste = temp_paste;
    if (n != 0)
        while(--n) {
            p = ACTUAL_NEXT(p);
        }

    return(ACTUAL_TEXT(p));
}


/************************************************************************
**tl_getid:             This function reads an identifier for the tiny lexer
**      into EXP_BUFFER.  if macformal is found, the text of that actual
**      (expanded or not) is appended to the identifier.  It is possible
**      that this text will contain characters that are not legal
**      identifiers so return value is whether checking to see if the
**      "identifier" is defined is worth the bother.
************************************************************************/
int
tl_getid(
    WCHAR c
    )
{
    WCHAR  *p;
    int     paste;
    int     legal_identifier;
    int     length = 0;

    p = Exp_ptr;
    paste = FALSE;
    legal_identifier = TRUE;

do_handle_macformal:
    if(CHARMAP(c) == LX_MACFORMAL) {
        ptext_t p_buf;

        if((p_buf = do_macformal(&paste)) != 0) {
            while( *p_buf ) {
                if( ! LXC_IS_IDENT(*p_buf)) {
                    legal_identifier = FALSE;
                }
                if(Exp_ptr >= ELIMIT) {
                    fatal_in_macro(10056);
                }
                *Exp_ptr++ = *p_buf++;
            }
        }
    } else {
        *Exp_ptr++ = c;
    }

do_handle_eos:
    while(LXC_IS_IDENT(c = GETCH())) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
        *Exp_ptr++ = c;
    }

    if(CHARMAP(c) == LX_NOEXPAND) {
        length = (int)GETCH();                  /* just skip length */
        goto do_handle_eos;
    }

    if(CHARMAP(c) == LX_MACFORMAL) {
        paste = TRUE;
        goto do_handle_macformal;
    }

    UNGETCH();
    if(legal_identifier && (length == (Exp_ptr - p))) {
        legal_identifier = FALSE;
    }

    if(legal_identifier) {
        if(((Exp_ptr - p) > LIMIT_ID_LENGTH) && ( ! Prep)) {
            Exp_ptr = &p[LIMIT_ID_LENGTH];
            *Exp_ptr = L'\0';    /* terminates identifier for warning */
            Msg_Temp = GET_MSG (4011);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, p);
            warning(4011);              /* id truncated */
        } else {
            *Exp_ptr = L'\0';    /* terminates identifier for expandable check */
        }
        /*
        **      Whether or not we are doing Prep output, we still have to make
        **      sure the identifier will fit in Reuse_W
        */
        if((Exp_ptr - p) > (sizeof(Reuse_W) / sizeof(WCHAR))) {
            Exp_ptr = &p[LIMIT_ID_LENGTH];
            *Exp_ptr = L'\0';
            Msg_Temp = GET_MSG (4011);
            SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, p);
            warning(4011);
        }
        /*
        **      copy into Reuse_W for warnings about mismatched number of
        **      formals/actuals, and in case it's not expandable
        */
        memcpy(Reuse_W, p, (int)((Exp_ptr - p) + 1) * sizeof(WCHAR));
        Reuse_W_hash = local_c_hash(Reuse_W);
        /*
        **      the characters from Exp_ptr to p inclusive do not include the
        **      the hash character, the length character, and the terminating
        **      null.
        */
        Reuse_W_length = (UINT)((Exp_ptr - p) + 1);
    }
    return(legal_identifier);
}


/************************************************************************
**  do_strformal:   returns pointer to the actual requested without
**          checking for paste (a legal token is not possible, so if a paste
**          is being done on a strformal, the behavior is undefined
************************************************************************/
ptext_t
do_strformal(
    void
    )
{
    WCHAR   n;
    ptext_t p;

    /* use unexpanded actual */
    p = CURRENT_ACTUALS;
    n = GETCH();
    if(n > CURRENT_NACTUALS) {
        return(NULL);           /* already output warning */
    }
    if (n != 0)
        while(--n) {
            p = ACTUAL_NEXT(p);
        }
    return(ACTUAL_TEXT(p));
}


/************************************************************************
**  can_get_non_white:      tries to get the next non white character
**          using P1 rules for white space (NL included).  If the end of
**          an actual, or a rescan is found, this returns FALSE, so control
**          can drop into one of the lexers.
************************************************************************/
int
can_get_non_white(
    void
    )
{
    int return_value = FALSE;
    int white_found = FALSE;

    for(;;) {
        switch(CHARMAP(GETCH())) {
            case LX_NL:
                if(On_pound_line) {
                    UNGETCH();
                    goto leave_cgnw;
                }
                Linenumber++;
                /*
                **      FALLTHROUGH
                */
            case LX_WHITE:
            case LX_CR:
                white_found = TRUE;
                break;
            case LX_EOS:
                {
                    int     eos_res;
                    if((eos_res = handle_eos()) & (ACTUAL_EOS | RESCAN_EOS)) {
                        goto leave_cgnw;
                    }
                    if(eos_res != BACKSLASH_EOS) {
                        break;
                    }
                }
                /*
                **      FALLTHROUGH
                */
            default:
                UNGETCH();
                return_value = TRUE;
                goto leave_cgnw;
                break;
        }
    }
leave_cgnw:
    if(white_found) {
        if(Exp_ptr >= ELIMIT) {
            fatal_in_macro(10056);
        }
        if(*(Exp_ptr - 1) != L' ') {
            *Exp_ptr++ = L' ';
        }
    }
    return(return_value);               /* could you get next non white? */
}


/************************************************************************/
/* fatal_in_macro ()                                                    */
/************************************************************************/
void
fatal_in_macro(
    int e
    )
{
    Linenumber = Macro_line;
    strcpy (Msg_Text, GET_MSG(e));
    fatal (e);
}


/************************************************************************
**  handle_eos : handle the end of a string.
************************************************************************/
int
handle_eos(
    void
    )
{
    if(PREVCH() == L'\\') {
        if(checknl()) {
            return(FILE_EOS);
        } else {
            return(BACKSLASH_EOS);
        }
    }
    if(Macro_depth == 0) {      /* found end of file buffer or backslash */
        if(io_eob()) {          /* end of buffer in here is bad */
            strcpy (Msg_Text, GET_MSG(1004));
            fatal (1004);
        }
        return(FILE_EOS);
    }

again:
    switch(GETCH()) {
        case EOS_PAD:
            goto again;
        case EOS_ACTUAL:
            /*
            ** Just finished expanding actual.  Check to see if there are
            ** any more actuals to be expanded.  If there are, set up to
            ** expand them and return.  Otherwise, set up to expand defn
            */

            /* move expanded text of this actual to act_buffer */
            move_to_actual(CURRENT_TEXT, Exp_ptr);

            /* reset Exp_ptr for more expansions at this macro depth */
            Exp_ptr = CURRENT_TEXT;

            /* expand next actual if there, otherwise expand definition */
            expand_macro();

            return(ACTUAL_EOS);
            break;
        case EOS_DEFINITION:
            if(rescan_expansion()) {
                return(RESCAN_EOS);
            } else {
                return(DEFINITION_EOS);
            }
            break;
        case EOS_RESCAN:
            /*
            ** Reset Current_char, Exp_ptr and Act_ptr, pop the macro
            */

            /*      get input from the previous stream */
            Current_char = CURRENT_STRING;

            /* mark this macro as not expanding */
            DEFN_EXPANDING(CURRENT_MACRO)--;


            /*
            **      if looking for the actuals of a macro, these pointers
            **      should really not be reset, however, it is cleaner to
            **      save them before calling handle_eos, and restore them
            **      upon returning, than check a static variable here.
            */
            if(Macro_depth == 1) {
                Act_ptr = ACT_BUFFER;
                Exp_ptr = EXP_BUFFER;
            }
            --Macro_depth;
            return(DEFINITION_EOS);
            break;
            /* the following conditional compile is so brackets match */

        default:
            return(FILE_EOS);
    }
}
/************************************************************************
**      END EXPANDING MACRO }
************************************************************************/
