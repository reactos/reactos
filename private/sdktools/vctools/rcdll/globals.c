/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* GLOBALS.C - Global variable definitions                              */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"

/* shared strings */
WCHAR   Union_str[] = L"union";
WCHAR   Struct_str[] = L"struct";
WCHAR   Cdecl_str[] = L"cdecl";
WCHAR   Cdecl1_str[] = L"cdecl L";
WCHAR   Fortran_str[] = L"fortran";
WCHAR   Fortran1_str[] = L"fortran L";
WCHAR   Pascal_str[] = L"pascal";
WCHAR   Pascal1_str[] = L"pascal L";
WCHAR   PPelse_str[] = L"#else";
WCHAR   PPendif_str[] = L"#endif";
WCHAR   PPifel_str[] = L"#if/#elif";
WCHAR   Syntax_str[] = L"syntax error";


PFILE   OUTPUTFILE;                     /* File for output of program */

WCHAR   *A_string;                      /* model encoding */
WCHAR   *Debug;                         /* debugging switches */
WCHAR   *Input_file;                    /* the input .rc file */
WCHAR   *Output_file;                   /* the output .res file */
WCHAR   *Q_string;                      /* hardware characteristics */
WCHAR   *Version;
UINT    uiDefaultCodePage;
UINT    uiCodePage;

int     In_alloc_text;
int     Bad_pragma;
int     Cross_compile;                  /* is this a cross compile ? */
int     Ehxtension;                     /* near/far keywords, but no huge */
int     HugeModel;                      /* Huge Model program ?? */
int     Inteltypes;                     /* using strict Intel types or not */
int     Nerrors;
int     NoPasFor;                       /* no fortran/pascal keywords ? */
int     Out_funcdef;                    /* output function definitions */
int     Plm;                            /* non-C calling sequence */
int     Prep;                           /* preprocess */
int     Srclist;                        /* put msgs to il file if source listing */

int     Cmd_intrinsic;                  /* implicit intrinsics */
int     Cmd_loop_opt;
int     Cmd_pointer_check;

int     Symbolic_debug;                 /* Whether to put out dbil info or not */
int     Cflag;                          /* leave in comments */
int     Eflag;                          /* insert #line */
int     Jflag;                          /* no Kanji */
int     Pflag;                          /* no #line */
int     Rflag;                          /* mkhives - no exponent missing error */
int     ZcFlag;                         /* case insensitive compare */
int     In_define;
int     InInclude;
int     InIf;
int     Macro_depth;
int     Linenumber;

CHAR    chBuf[MED_BUFFER+1];
WCHAR   Reuse_W[BIG_BUFFER];
WCHAR   Filebuff[MED_BUFFER+1];
WCHAR   Macro_buffer[BIG_BUFFER * 4];

WCHAR   Reuse_Include[MED_BUFFER+1];

token_t Basic_token = L_NOTOKEN;
LIST    Defs = {MAXLIST};               /* -D list */
LIST    UnDefs = {MAXLIST};             /* -U list */
LIST    Includes = {MAXLIST, {0}};      /* for include file names */
WCHAR   *Path_chars = L"/";             /* path delimiter chars */
WCHAR   *Basename = L"";                /* base IL file name */
WCHAR   *Filename = Filebuff;

int     Char_align = 1;                 /* alignment of chars in structs */
int     Cmd_stack_check = TRUE;
int     Stack_check = TRUE;
int     Prep_ifstack = -1;
int     Switch_check = TRUE;
int     Load_ds_with;
int     Plmn;
int     Plmf;
int     On_pound_line;
int     Listing_value;
hash_t  Reuse_W_hash;
UINT    Reuse_W_length;
token_t Currtok = L_NOTOKEN;

int     Extension = TRUE;               /* near/far keywords? */
int     Cmd_pack_size = 2;
int     Pack_size = 2;

lextype_t yylval;

/*** I/O Variable for PreProcessor ***/
ptext_t Current_char;

/*** w-BrianM - Re-write of fatal(), error() ***/
CHAR     Msg_Text[MSG_BUFF_SIZE];
PCHAR    Msg_Temp;
