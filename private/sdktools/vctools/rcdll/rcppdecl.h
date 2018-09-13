/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* RCPPDECL.H - RCPP function prototype declarations                    */
/*                                                                      */
/* 04-Dec-90 w-BrianM  Created                                          */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/* ERROR.C                                                              */
/************************************************************************/
void error      (int);
void fatal      (int);
void warning    (int);

/************************************************************************/
/* GETMSG.C                                                             */
/************************************************************************/
PCHAR GET_MSG  (int);
void  SET_MSG  (PCHAR, UINT, PCHAR, ...);

/************************************************************************/
/* GETFLAGS.C                                                           */
/************************************************************************/
#if i386 == 1
int crack_cmd   (struct cmdtab *, WCHAR *, WCHAR *(*)(void), int);
#else /* MIPS */
struct cmdtab;
int crack_cmd   (struct cmdtab *, WCHAR *, WCHAR *(*)(void), int);
#endif /* i386 */

/************************************************************************/
/* LTOA.C                                                               */
/************************************************************************/
int zltoa       (long, WCHAR *, int);

/************************************************************************/
/* P0EXPR.C                                                             */
/************************************************************************/
long do_constexpr       (void);

/************************************************************************/
/* P0GETTOK.C                                                           */
/************************************************************************/
token_t         yylex(void);
int             lex_getid (WCHAR);

/************************************************************************/
/* P0IO.C                                                               */
/************************************************************************/
void            emit_line (void);
WCHAR           fpop (void);
int             io_eob (void);
int             io_restart (unsigned long int);
int             newinput (WCHAR *, int);
int             nested_include (void);
void            p0_init (PWCHAR, PWCHAR, LIST *, LIST *);
void            p0_terminate (void);

/************************************************************************/
/* P0KEYS.C                                                             */
/************************************************************************/
token_t         is_pkeyword (WCHAR *);

/************************************************************************/
/* P0MACROS.C                                                           */
/************************************************************************/
int             can_get_non_white (void);
int             can_expand (pdefn_t);
void            define (void);
void            definstall (WCHAR *, int, int);
pdefn_t         get_defined (void);
int             handle_eos (void);
int             tl_getid (WCHAR);
void            undefine (void);

/************************************************************************/
/* P0PREPRO.C                                                           */
/************************************************************************/
int             do_defined (PWCHAR);
int             nextis (token_t);
void            preprocess (void);
void            skip_cnew (void);
void            skip_NLonly (void);

/************************************************************************/
/* P1SUP.C                                                              */
/************************************************************************/
ptree_t         build_const (token_t, value_t *);

/************************************************************************/
/* RCPPUTIL.C                                                           */
/************************************************************************/
WCHAR *         pstrdup (WCHAR *);
WCHAR *         pstrndup (WCHAR *, int);
WCHAR *         strappend (WCHAR *, WCHAR *);

/************************************************************************/
/* SCANNER.C                                                            */
/************************************************************************/
token_t         char_const (void);
int             checknl (void);
int             checkop (int);
void            do_newline (void);
void            dump_comment (void);
void            DumpSlashComment (void);
void            getid (UINT);
WCHAR           get_non_eof (void);
token_t         getnum (WCHAR);
token_t         get_real (PWCHAR);
hash_t          local_c_hash (WCHAR *);
void            prep_string (WCHAR);
WCHAR           skip_cwhite (void);
int             skip_comment (void);
void            str_const (void);

/************************************************************************/
/* P0 I/O MACROS                                                        */
/************************************************************************/

//
// These macros could be a problem when working with non-spacing marks.
//
#define GETCH()         (*Current_char++)
#define CHECKCH()       (*Current_char)
#define UNGETCH()       (Current_char--)
#define PREVCH()        (*(Current_char - 1))
#define SKIPCH()        (Current_char++)


/************************************************************************/
/* RCPPX extensions needed for symbols                                  */
/************************************************************************/
void AfxOutputMacroDefn(pdefn_t p);
void AfxOutputMacroUse(pdefn_t p);
void move_to_exp(ptext_t);

/************************************************************************/
/* RCFUTIL utility routine						*/
/************************************************************************/
void myfwrite(const void *pv, size_t s, size_t n, FILE *fp);
