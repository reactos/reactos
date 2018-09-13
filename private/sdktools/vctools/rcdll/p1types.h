/*
**  symbols basic types
**  if the values are changed check : init'd arrays in globals.c
*/
#define BTundef         0
#define BTchar          1
#define BTshort         2
#define BTint           3
#define BTlong          4
#define BTenumuse       5
#define BTfloat         6
#define BTdouble        7
#define BTldouble       8
#define BTseg           9
#define BTBASIC         9   /* used elsewhere to indicate the last basic type */
#define BTvoid          10
#define BTenum          11
#define BTstruct        12
#define BTunion         13
#define BTstuse         14
#define BTunuse         15

#define BT_MASK         0x0f    /* basic type mask */

/*
**  the following are also used in indirection strings as modifiers
**  to the basic indirections.
**  NOTE THIS DOESN'T really work for C600, but for just one case.
**  if a typedef has 'near' on it, 'consolidate_types' will consider
**  it to be a 'signed' bit, and remove it, thus, near never gets
**  added to whatever the typedef is used on.
*/
#define BT_UNSIGNED             0x0010  /* unsigned keyword used */
#define BT_SIGNED               0x0020  /* signed keyword used */
#define SU_MASK                 0x0030  /* signed/unsigned mask */

#define BT_NEAR                 0x0040  /* near keyword used */
#define BT_FAR                  0x0080  /* far keyword used */
#define BT_HUGE                 0x00c0  /* huge keyword used */
#define NFH_MASK                0x00c0  /* near/far/huge mask */

#define BT_INTERRUPT            0x0100  /* interrupt seen */
#define BT_SAVEREGS             0x0200  /* dynalink seen */
#define BT_EXPORT               0x0400  /* export seen */
#define BT_LOADDS               0x0800  /* loadds seen */
#define CODEMOD_MASK            0x0f00  /* code modifiers */

#define BT_CONST                0x1000  /* constant keyword used */
#define BT_VOLATILE             0x2000  /* volatile keyword used */
#define CV_MASK                 0x3000  /* const/volatile mask */

#define BT_CDECL                0x4000  /* cdecl keyword used */
#define BT_FORTRAN              0x8000  /* fortran keyword used */
#define BT_PASCAL               0xc000  /* pascal keyword used */
#define LANGUAGE_MASK           0xc000  /* cdecl/fortran/pascal mask */

#define MODIFIER_MASK   (NFH_MASK | LANGUAGE_MASK | CODEMOD_MASK | CV_MASK)
#define ALL_MODIFIERS   (MODIFIER_MASK | SU_MASK)
/*
**      macros for getting/setting basic type information
**  Q_* to query the flag.
**      S_* to set the flag.
**  the Q_near/far/huge things are defined later, and are called IS_*.
*/
#define IS_BTBASIC(P)           ((P) <= BTBASIC)
#define IS_BTINTEGRAL(P)        ((P) <= BTenumuse)
#define IS_BTFLOAT(P)           ((BTfloat <= (P)) && ((P) <= BTldouble))
#define IS_BTVOID(P)            ((P) == BTvoid)

#define IS_BASIC(P)             (IS_BTBASIC(Q_BTYPE(P)))
#define IS_INTEGRAL(P)          (IS_BTINTEGRAL(Q_BTYPE(P)))
#define IS_FLOAT(P)             (IS_BTFLOAT(Q_BTYPE(P)))
#define IS_VOID(P)              (IS_BTVOID(Q_BTYPE(P)))

#define IS_MULTIBYTE(P) ((BTstruct <= (P)) && ((P) <= BTunuse))
#define IS_UNSIGNED(P)  ((P) & BT_UNSIGNED)
#define IS_SIGNED(P)    ((P) & BT_SIGNED)
#define CLR_SIGNED(P)   ((P) &= ~BT_SIGNED)

#define S_UNSIGNED(P)   ((P) |= BT_UNSIGNED)
#define S_SIGNED(P)     ((P) |= BT_SIGNED)
#define S_CONST(P)      ((P) |= BT_CONST)
#define S_VOLATILE(P)   ((P) |= BT_VOLATILE)
#define S_NEAR(P)       ((P) |= BT_NEAR)
#define S_FAR(P)        ((P) |= BT_FAR)
#define S_HUGE(P)       ((P) |= BT_HUGE)
#define S_CDECL(P)      ((P) |= BT_CDECL)
#define S_FORTRAN(P)    ((P) |= BT_FORTRAN)
#define S_PASCAL(P)     ((P) |= BT_PASCAL)
#define S_INTERRUPT(P)  ((P) |= BT_INTERRUPT)
#define S_SAVEREGS(P)   ((P) |= BT_SAVEREGS)

#define Q_BTYPE(P)      ((P) & ( BT_MASK ))
#define S_BTYPE(P,V)    ((P) = (((P) & ( ~ BT_MASK )) | V))

struct  s_flist         {                       /* formal parameter list of types */
        ptype_t         fl_type;                /* type of formal */
        pflist_t        fl_next;                /* next one */
        };

#define FL_NEXT(P)              ((P)->fl_next)
#define FL_TYPE(P)              ((P)->fl_type)

union   u_ivalue        {
        abnd_t          ind_subscr;             /*  array subscript size  */
        psym_t          ind_formals;    /*  formal symbol list  */
        pflist_t        ind_flist;              /*  formal type list  */
        psym_t          ind_basesym;    /*  segment we're based on  */
        ptype_t         ind_basetype;   /*  type we're based on  */
        phln_t          ind_baseid;             /*  id we're based on  */
        };

#define PIVALUE_ISUB(P)                 ((P)->ind_subscr)
#define PIVALUE_IFORMALS(P)             ((P)->ind_formals)
#define PIVALUE_IFLIST(P)               ((P)->ind_flist)
#define PIVALUE_BASEDSYM(P)             ((P)->ind_basesym)
#define PIVALUE_BASEDTYPE(P)            ((P)->ind_basetype)
#define PIVALUE_BASEDID(P)              ((P)->ind_baseid)

#define IVALUE_ISUB(P)                  (PIVALUE_ISUB(&(P)))
#define IVALUE_IFORMALS(P)              (PIVALUE_IFORMALS(&(P)))
#define IVALUE_IFLIST(P)                (PIVALUE_IFLIST(&(P)))
#define IVALUE_BASEDSYM(P)              (PIVALUE_BASEDSYM(&(P)))
#define IVALUE_BASEDTYPE(P)             (PIVALUE_BASEDTYPE(&(P)))
#define IVALUE_BASEDID(P)               (PIVALUE_BASEDID(&(P)))

struct  s_indir {
        btype_t         ind_type;               /*  what kind ?  */
        pindir_t        ind_next;               /*  next one  */
        ivalue_t        ind_info;               /*  subscript/function's params  */
        };

#define INDIR_INEXT(P)          ((P)->ind_next)
#define INDIR_ITYPE(P)          ((P)->ind_type)
#define INDIR_INFO(P)           ((P)->ind_info)
#define INDIR_ISUB(P)           (IVALUE_ISUB(INDIR_INFO(P)))
#define INDIR_IFORMALS(P)       (IVALUE_IFORMALS(INDIR_INFO(P)))
#define INDIR_IFLIST(P)         (IVALUE_IFLIST(INDIR_INFO(P)))
#define INDIR_BASEDSYM(P)       (IVALUE_BASEDSYM(INDIR_INFO(P)))
#define INDIR_BASEDTYPE(P)      (IVALUE_BASEDTYPE(INDIR_INFO(P)))
#define INDIR_BASEDID(P)        (IVALUE_BASEDID(INDIR_INFO(P)))
/*
**  optimal choices for these things.
**  however, everyone uses macros to test them, so if i'm wrong,
**  it should be easy to change the values, but think well !!!
*/
#define IN_FUNC                         0x00
#define IN_PFUNC                        0x01
#define IN_ARRAY                        0x02
#define IN_PDATA                        0x03
#define IN_VOIDLIST                     0x04
#define IN_VARARGS                      0x08
#define IN_MASK                         (IN_ARRAY | IN_PDATA | IN_PFUNC | IN_FUNC)
#define IN_ADDRESS                      (IN_ARRAY | IN_PDATA | IN_PFUNC)
#define IN_DATA_ADDRESS                 (IN_ARRAY & IN_PDATA)   /* yes, i meant '&' */
#define IN_POINTER                      (IN_PFUNC & IN_PDATA)   /* yes, i meant '&' */
#if IN_DATA_ADDRESS == 0
#error IN_DATA_ADDRESS is ZERO
#endif
#if IN_POINTER == 0
#error IN_POINTER is ZERO
#endif
#define IS_ARRAY(I)                     (((I) & IN_MASK) == IN_ARRAY)
#define IS_PDATA(I)                     (((I) & IN_MASK) == IN_PDATA)
#define IS_PFUNC(I)                     (((I) & IN_MASK) == IN_PFUNC)
#define IS_FUNC(I)                      (((I) & IN_MASK) == IN_FUNC)
#define IS_EXTRACT(I)                   ((I) & IN_POINTER)
#define IS_DATA_ADDRESS(I)              ((I) & IN_DATA_ADDRESS)
#define IS_ADDRESS(I)                   ((I) & IN_ADDRESS)
#define IS_INDIR(I)                     ((I) & IN_MASK)
#define MASK_INDIR(I)                   ((I) & IN_MASK)
#define IS_VOIDLIST(I)                  ((I) & IN_VOIDLIST)
#define IS_VARARGS(I)                   ((I) & IN_VARARGS)

#define IS_NFH(I)                       ((I) & NFH_MASK)
#define IS_NEARNFH(I)                   ((I) == BT_NEAR)
#define IS_FARNFH(I)                    ((I) == BT_FAR)
#define IS_HUGENFH(I)                   ((I) == BT_HUGE)
#define IS_BASEDNFH(I)                  ((I) >= BT_BASED)
#define IS_BASEDSELFNFH(I)              ((I) == BT_BASEDSELF)
#define IS_BASEDIDNFH(I)                ((I) == BT_BASEDID)
#define IS_BASEDSYMNFH(I)               ((I) == BT_BASEDSYM)
#define IS_BASEDTYPENFH(I)              ((I) == BT_BASEDTYPE)

#define IS_NEAR(I)                      (IS_NEARNFH(IS_NFH(I)))
#define IS_FAR(I)                       (IS_FARNFH(IS_NFH(I)))
#define IS_HUGE(I)                      (IS_HUGENFH(IS_NFH(I)))
#define IS_BASED(I)                     (IS_BASEDNFH(IS_NFH(I)))
#define IS_BASEDSELF(I)                 (IS_BASEDSELFNFH(IS_NFH(I)))
#define IS_BASEDID(I)                   (IS_BASEDIDNFH(IS_NFH(I)))
#define IS_BASEDSYM(I)                  (IS_BASEDSYMNFH(IS_NFH(I)))
#define IS_BASEDTYPE(I)                 (IS_BASEDTYPENFH(IS_NFH(I)))

#define IS_INTERRUPT(I)         ((I) & BT_INTERRUPT)
#define IS_SAVEREGS(I)          ((I) & BT_SAVEREGS)
#define IS_EXPORT(I)            ((I) & BT_EXPORT)
#define IS_LOADDS(I)            ((I) & BT_LOADDS)
#define IS_CODEMOD(I)           ((I) & CODEMOD_MASK)

#define IS_CONST(I)             ((I) & BT_CONST)
#define IS_VOLATILE(I)          ((I) & BT_VOLATILE)

#define IS_MODIFIED(I)          ((I) & (MODIFIER_MASK))
#define ANY_MODIFIER(I)         ((I) & (ALL_MODIFIERS))

#define INTERF(I)               (MASK_INDIR(I) + (((I) & NFH_MASK) > 4))

#define S_ITYPE(I,V)            ((I) = ((I) & ( ~ IN_MASK )) | (V))
#define S_INFH(I,V)             ((I) = ((I) & ( ~ NFH_MASK )) | (V))
/*
**  type info for symbols
*/
struct  s_type  {
        btype_t         ty_bt;          /*  base type specifiers  */
        pindir_t        ty_indir;       /*  indirection string  */
        p1key_t         ty_dtype;       /*  derived type */
        psym_t          ty_esu;         /*  enum/structure/union/static defining type  */
        USHORT          ty_index;       /*      unique index of type for debugger */
        };
/*
**  help getting type info. P is pointer to TYPE (struct s_type).
**      TYPE contains the basic type, adjectives and an optional pointer
**      to a symbol which is an enumeration, structure, union which is the type
**      of this TYPE.
*/
#define TY_BTYPE(P)             ((P)->ty_bt)    /*  basic type  */
#define TY_DTYPE(P)             ((P)->ty_dtype) /*  derived type  */
#define TY_ESU(P)               ((P)->ty_esu)   /*  ptr to parent enum/struct/union  */
#define TY_INDIR(P)             ((P)->ty_indir) /*  indirection string  */
#define TY_TINDEX(P)            ((P)->ty_index) /*  type index */
#define TY_INEXT(P)             (INDIR_INEXT(TY_INDIR(P)))
#define TY_ITYPE(P)             (INDIR_ITYPE(TY_INDIR(P)))
#define TY_ISUB(P)              (INDIR_ISUB(TY_INDIR(P)))
#define TY_IFORMALS(P)          (INDIR_IFORMALS(TY_INDIR(P)))
#define TY_IFLIST(P)            (INDIR_IFLIST(TY_INDIR(P)))

typedef struct  s_indir_entry   indir_entry_t;
typedef struct  s_type_entry    type_entry_t;

struct  s_indir_entry   {
        indir_entry_t   *ind_next;
        indir_t          ind_type;
        };

struct  s_type_entry    {
        type_entry_t    *te_next;
        type_t           te_type;
        };

#define TYPE_TABLE_SIZE         0x100
#define INDIR_TABLE_SIZE        0x040
/*
**  HASH_MASK : is a value which consists of the bits in common
**  between upper and lower case. we mask each char we read with this
**  to sum them for a hash value. we do this so that all names consisting
**  of the same chars (case insensitive), will hash to the same location.
*/
#define HASH_MASK                       0x5f

#define DATASEGMENT                     0
#define TEXTSEGMENT                     1
