/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* P0DEFS.H - Defintions for PreProcessor parsing code                  */
/*                                                                      */
/* 06-Dec-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

struct  s_defn  {
    pdefn_t     defn_next;              /*  pointer to next ident  */
    PWCHAR      defn_ident;             /*  pointer to name */
    PWCHAR      defn_text;              /*  definition substitution string  */
    char        defn_nformals;          /*number of formal arguments - can be <0*/
    char        defn_expanding;         /* are we already expanding this one? */
};

#define DEFN_IDENT(P)           ((P)->defn_ident)
#define DEFN_NAME(P)            (DEFN_IDENT(P))
#define DEFN_NEXT(P)            ((P)->defn_next)
#define DEFN_TEXT(P)            ((P)->defn_text)
#define DEFN_NFORMALS(P)        ((P)->defn_nformals)
#define DEFN_EXPANDING(P)       ((P)->defn_expanding)

#define FILE_EOS                0x01L
#define ACTUAL_EOS              0x02L
#define DEFINITION_EOS          0x04L
#define RESCAN_EOS              0x08L
#define BACKSLASH_EOS           0x10L
#define ANY_EOS                 ( FILE_EOS | ACTUAL_EOS | DEFINITION_EOS \
                                                                         | RESCAN_EOS | BACKSLASH_EOS )

/*
**      arbitrarily chosen characters that get special treatment when found
**      after EOS in handle_eos()
*/
#define EOS_ACTUAL              L'A'
#define EOS_DEFINITION          L'D'
#define EOS_RESCAN              L'R'
#define EOS_PAD                 L'P'

#define FROM_COMMAND            -2
#define PRE_DEFINED(P)          (DEFN_NFORMALS(P) < FROM_COMMAND)

typedef struct s_expstr {
    ptext_t     exp_string;     /* ptr to next character in stream aft macro */
    WCHAR       *exp_actuals;   /* ptr to start of actuals linked list */
    ptext_t     exp_text;       /* ptr to expanded text for this macro */
    pdefn_t     exp_macro;      /* ptr to macro defn */
    UCHAR       exp_nactuals;   /* number of actuals */
    UCHAR       exp_nactsexpanded;/* number of expanded actuals for handle_eos*/
} expansion_t;

/*
**      note that CURRENT_STRING usually points into an area in the macro
**      expansion buffer, but the first item used (Macro_depth equals 1) points
**      to text read from a file.  In some versions, the heap is reshuffled
**      and this pointer must be updated for the first item.
*/
#define CURRENT_STRING          Macro_expansion[Macro_depth].exp_string
#define CURRENT_ACTUALS         Macro_expansion[Macro_depth].exp_actuals
#define CURRENT_TEXT            Macro_expansion[Macro_depth].exp_text
#define CURRENT_MACRO           Macro_expansion[Macro_depth].exp_macro
#define CURRENT_NACTUALS        Macro_expansion[Macro_depth].exp_nactuals
#define CURRENT_NACTSEXPANDED Macro_expansion[Macro_depth].exp_nactsexpanded

/*
**      finds address after last element in an array. Used to check for
**      buffer overflows.
*/
#define LIMIT(a)        &(a)[sizeof(a) / sizeof(*a)]

#define IS_CHAR(c,uc)   (towupper(c) == (uc))
#define IS_B(c)         IS_CHAR(c, L'B')
#define IS_D(c)         IS_CHAR(c, L'D')
#define IS_E(c)         IS_CHAR(c, L'E')
#define IS_F(c)         IS_CHAR(c, L'F')
#define IS_H(c)         IS_CHAR(c, L'H')
#define IS_EL(c)        IS_CHAR(c, L'L')
#define IS_O(c)         IS_CHAR(c, L'O')
#define IS_Q(c)         IS_CHAR(c, L'Q')
#define IS_U(c)         IS_CHAR(c, L'U')
#define IS_X(c)         IS_CHAR(c, L'X')
#define IS_DOT(c)       (c == L'.')
#define IS_SIGN(c)      ((c == L'+') || (c ==L'-'))

#define P0_IF           0
#define P0_ELIF         1
#define P0_ELSE         2
#define P0_ENDIF        3
#define P0_IFDEF        4
#define P0_IFNDEF       5
#define P0_DEFINE       6
#define P0_INCLUDE      7
#define P0_PRAGMA       8
#define P0_UNDEF        9
#define P0_LINE         10
#define P0_NOTOKEN      11
#define P0_ERROR        12
#define P0_IDENT        13


#define HLN_NAME(s)     ((s).hln_name)
#define HLN_HASH(s)     ((s).hln_hash)
#define HLN_LENGTH(s)   ((s).hln_length)
#define HLN_IDENT_HASH(p)       (HLN_HASH(*(p)))
#define HLN_IDENT_LENGTH(p)     (HLN_LENGTH(*(p)))
#define HLN_IDENTP_NAME(p)      (HLN_NAME(*(p)))
#define HLN_TO_NAME(S)          ((PWCHAR)pstrndup(HLN_IDENTP_NAME(S),HLN_IDENT_LENGTH(S)))

#define HASH_MASK               0x5f

#define LIMIT_ID_LENGTH         31
#define LIMIT_NESTED_INCLUDES   1024
#define LIMIT_MACRO_DEPTH       64
#define LIMIT_STRING_LENGTH     2043
#define LEVEL_0                 0xffL

#define MUST_OPEN       1
#define MAY_OPEN        0

/*** The following are defined to use on the Token Table ***/

#define TS_STR(idx)     (Tokstrings[idx-L_NOTOKEN].k_text)
#define TS_VALUE(idx)   (Tokstrings[idx-L_NOTOKEN].k_token)
