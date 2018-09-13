/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* MAIN.C - Main Program                                                */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"


/************************************************************************/
/* Global Varialbes                                                     */
/************************************************************************/
WCHAR   *Unknown = NULL;                /* holder for bad flags */
int     Argc;
WCHAR   **Argv;

/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
WCHAR   *nextword(void);
void    to_human(void);


struct  subtab  Ztab[] = {
    L'a',        UNFLAG, &Extension,
    L'e',        FLAG,   &Extension,
    L'E',        FLAG,   &Ehxtension,
    L'i',        FLAG,   &Symbolic_debug,
    L'g',        FLAG,   &Out_funcdef,
    L'p',        FLAG,   &Cmd_pack_size,
    L'I',        FLAG,   &Inteltypes,
    L'c',        FLAG,   &ZcFlag,
    0,           0,              0,
};

struct cmdtab cmdtab[] = {
    L"-pc#",             (char *)&Path_chars,            1,      STRING,
    L"-pf",              (char *)&NoPasFor,              1,      FLAG,
    L"-C",               (char *)&Cflag,                 1,      FLAG,
    L"-CP#",             (char *)&uiCodePage,            1,      NUMBER,
    L"-D#",              (char *)&Defs,                  1,      PSHSTR,
    L"-U#",              (char *)&UnDefs,                1,      PSHSTR,
    L"-E",               (char *)&Eflag,                 1,      FLAG,
    L"-I#",              (char *)&Includes,              1,      PSHSTR,
    L"-P",               (char *)&Pflag,                 1,      FLAG,
    L"-f",               (char *)&Input_file,            1,      STRING,
    L"-g",               (char *)&Output_file,           1,      STRING,
    L"-J",               (char *)&Jflag,                 1,      FLAG,
    L"-Zp",              (char *)&Cmd_pack_size,         1,      FLAG,
    L"-Zp#",             (char *)&Cmd_pack_size,         1,      NUMBER,
    L"-Z*",              (char *)Ztab,                   1,      SUBSTR,
    L"-Oi",              (char *)&Cmd_intrinsic,         1,      FLAG,
    L"-Ol",              (char *)&Cmd_loop_opt,          1,      FLAG,
    L"-db#",             (char *)&Debug,                 1,      STRING,
    L"-il#",             (char *)&Basename,              1,      STRING,
    L"-xc",              (char *)&Cross_compile,         1,      FLAG,
    L"-H",               (char *)&HugeModel,             1,      FLAG,
    L"-V#",              (char *)&Version,               1,      STRING,
    L"-Gs",              (char *)&Cmd_stack_check,       1,      UNFLAG,
    L"-Gc",              (char *)&Plm,                   1,      FLAG,
    L"-char#",           (char *)&Char_align,            1,      NUMBER,
    L"-A#",              (char *)&A_string,              1,      STRING,
    L"-Q#",              (char *)&Q_string,              1,      STRING,
    L"-Fs",              (char *)&Srclist,               1,      FLAG,
    L"-R",               (char *)&Rflag,                 1,      FLAG,
    L"*",                (char *)&Unknown,               0,      STRING,
    0,                   0,                              0,      0,
};

/************************************************************************/
/* nextword -                                                           */
/************************************************************************/
WCHAR   *nextword(void)
{
    return((--Argc > 0) ? (*++Argv) : 0);
}

/************************************************************************/
/* main -                                                               */
/************************************************************************/
int __cdecl
rcpp_main(
    int argc,
    PWCHAR*argv
    )
{
    Argc = argc;
    Argv = argv;

    if(Argv == NULL) {
        strcpy (Msg_Text, GET_MSG (1002));
        fatal(1007);    /* no memory */
    }

    while(crack_cmd(cmdtab, nextword(), nextword, 0)) ;

    if(Unknown) {
        Msg_Temp = GET_MSG (1007);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, Unknown, "c1");
        fatal(1007);    /* unknown flag */
    }

    if( ! Input_file) {
        strcpy (Msg_Text, GET_MSG (1008));
        fatal(1008);            /* no input file specified */
    }

    if( ! Output_file) {
        strcpy (Msg_Text, GET_MSG (1010));
        fatal(1010);            /* no output file specified */
    }

    Prep = TRUE;
    if( !Eflag && !Pflag ) {
        Eflag = TRUE;
    }

    wcsncpy(Filename,Input_file,128);

    p0_init(Input_file, Output_file, &Defs, &UnDefs);
    to_human();

    if( Prep_ifstack >= 0 ) {
        strcpy (Msg_Text, GET_MSG (1022));
        fatal(1022);            /* expected #endif */
    }

    p0_terminate();
    return Nerrors;
}


/************************************************************************/
/* to_human : outputs preprocessed text in human readable form.         */
/************************************************************************/
void
to_human(
    void
    )
{
    PWCHAR value;

    for(;;) {
        switch(yylex()) {
            case 0:
                return;
            case L_NOTOKEN:
                break;
            default:
                if (Basic_token == 0) {
                    strcpy (Msg_Text, GET_MSG (1011));
                    fatal(1011);
                }
                value = Tokstrings[Basic_token - L_NOTOKEN].k_text;
                myfwrite(value, wcslen(value) * sizeof(WCHAR), 1, OUTPUTFILE);
                break;
        }
    }
}
