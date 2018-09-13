/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* RCPPTYPE.H - Type definitions for RCPP                               */
/*                                                                      */
/* 04-Dec-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/* Define types for greater visibility and easier portability           */
/************************************************************************/

#ifndef _WINDOWS_
typedef char            CHAR;
typedef unsigned char   BYTE;
typedef CHAR *          PCHAR;
typedef unsigned short  WCHAR;
typedef unsigned char   UCHAR;
typedef UCHAR *         PUCHAR;
typedef WCHAR *         PWCHAR;

typedef short           SHORT;
typedef SHORT *         PSHORT;
typedef unsigned short  USHORT;
typedef USHORT *        PUSHORT;

typedef int             INT;
typedef INT             BOOL;
typedef INT *           PINT;
typedef unsigned int    UINT;
typedef UINT *          PUINT;

typedef long            LONG;
typedef LONG *          PLONG;
typedef unsigned long   ULONG;
typedef ULONG *         PULONG;

typedef void            VOID;
typedef VOID *          PVOID;
#endif

typedef double          DOUBLE;
typedef DOUBLE *        PDOUBLE;


/************************************************************************/
/*                                                                      */
/* Define internal types                                                */
/*                                                                      */
/************************************************************************/

#define TRUE    1
#define FALSE   0

#define EXTERN  extern
#define REG     register
#define STATIC  static


#define BIG_BUFFER      512
#define MED_BUFFER      256
#define SMALL_BUFFER    128
#define TINY_BUFFER     32
#define MSG_BUFF_SIZE   2048
#define IFSTACK_SIZE    TINY_BUFFER


/*
**      some commonly used typdefs for scalar items
*/
typedef UINT    p1key_t;
typedef UCHAR   hash_t;
typedef UCHAR   token_t;
typedef UCHAR   shape_t;

typedef UCHAR   blknum_t;       /*  lexical level  */
typedef UCHAR   class_t;

typedef USHORT  btype_t;        /*  basic type specifier  */
typedef USHORT  refcnt_t;       /*  symbol's reference count  */
typedef USHORT  hey_t;          /*  unique keys  */
typedef USHORT  offset_t;       /*  members offset within a struct  */

typedef ULONG   abnd_t;         /*  array bound type  */
typedef ULONG   len_t;          /*  number of bytes/bits of member/field  */

typedef struct  s_adj           symadj_t;
typedef struct  s_defn          defn_t;
typedef struct  s_flist         flist_t;
typedef struct  s_indir         indir_t;
typedef struct  s_stack         stack_t;
typedef struct  s_sym           sym_t;
typedef struct  s_table         table_t;
typedef struct  s_toklist       toklist_t;
typedef struct  s_tree          tree_t;
typedef struct  s_type          type_t;
typedef struct  s_case          case_t;

typedef union   u_ivalue        ivalue_t;

/*
**      abstract char pointer types
*/
typedef PWCHAR          ptext_t;        /* wherever input text comes from */

/*
**      other abstract pointer types
*/
typedef type_t *        ptype_t;        /* ptr to types */
typedef indir_t *       pindir_t;       /* ptr to indirections */
typedef flist_t *       pflist_t;       /* ptr to formal list type */
typedef sym_t *         psym_t;         /* symbol ptrs */
typedef defn_t *        pdefn_t;        /* #define names */

typedef tree_t *        ptree_t;


typedef struct s_realt {
    LONG        S_sizet;
    DOUBLE      S_realt;
} Srealt_t;


/* declspec type */
struct s_declspec {
    class_t ds_calss;
    ptype_t ds_type;
};
typedef struct s_declspec       declspec_t;
typedef declspec_t *            pdeclspec_t;


/* string type */
struct s_string {
    WCHAR *     str_ptr;
    USHORT      str_len;
};
typedef struct s_string         string_t;
typedef string_t *              pstring_t;


/* rcon type */
struct rcon {
    Srealt_t    rcon_real;
};
typedef struct rcon             rcon_t;
typedef struct rcon *           prcon_t;


/* hln type */
struct s_hln {
    PWCHAR hln_name;
    UCHAR hln_hash;
    UCHAR hln_length;
};
typedef struct  s_hln           hln_t;
typedef hln_t *                 phln_t;


/*
**      union used to return values from the lexer
*/
typedef union   s_lextype       {
        btype_t         yy_btype;
        PWCHAR          yy_cstr;
        int             yy_int;
        int             yy_class;
        long            yy_long;
        hln_t           yy_ident;
        declspec_t      yy_declspec;
        string_t        yy_string;
        psym_t          yy_symbol;
        token_t         yy_token;
        ptree_t         yy_tree;
        ptype_t         yy_type;
        } lextype_t;

/* value_t definition */
union u_value {
    prcon_t     v_rcon;
    long        v_long;
    string_t    v_string;
    psym_t      v_symbol;
};
typedef union   u_value         value_t;

/* keytab_t definition */
typedef struct {
    WCHAR *     k_text;
    UCHAR       k_token;
} keytab_t;


/************************************************************************/
/* LIST definition for \D values                                        */
/************************************************************************/
#define MAXLIST 100

typedef struct LIST {
        INT      li_top;
        WCHAR *  li_defns[MAXLIST];
} LIST;

#define UNREFERENCED_PARAMETER(x) (x)
