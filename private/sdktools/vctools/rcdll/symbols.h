/*
**  Symbol types . . .
*/
#define SYMV_SYMBOL             0x0001
#define SYMV_RAWSYMBOL          0x0002
#define SYMV_FORMAL             0x0004
#define SYMV_SEGMENT            0x0008

#define SYMV_FUNCTION           0x0010
#define SYMV_INTRINSIC          0x0020
#define SYMV_LABEL              0x0040

#define SYMV_TAG                0x0080
#define SYMV_MOE                0x0100
#define SYMV_MEMBER             0x0200
#define SYMV_FIELD              0x0400
#define SYMV_OPC                0x0800

#define SYMV_ABSTRSYM           0x1000

#define SYM_ISSYMBOL(p)         (SYM_ANYVTAG(p) & SYMV_SYMBOL)
#define SYM_ISFUNCTION(p)       (SYM_ANYVTAG(p) & SYMV_FUNCTION)
#define SYM_ISFORMAL(p)         (SYM_ANYVTAG(p) & SYMV_FORMAL)
#define SYM_ISABSTRSYM(p)       (SYM_ANYVTAG(p) & SYMV_ABSTRSYM)
#define SYM_ISLABEL(p)          (SYM_ANYVTAG(p) & SYMV_LABEL)
#define SYM_ISTAG(p)            (SYM_ANYVTAG(p) & SYMV_TAG)
#define SYM_ISMOE(p)            (SYM_ANYVTAG(p) & SYMV_MOE)
#define SYM_ISMEMBER(p)         (SYM_ANYVTAG(p) & SYMV_MEMBER)
#define SYM_ISFIELD(p)          (SYM_ANYVTAG(p) & SYMV_FIELD)
#define SYM_ISINTRINSIC(p)      (SYM_ANYVTAG(p) & SYMV_INTRINSIC)
#define SYM_ISRAWSYMBOL(p)      (SYM_ANYVTAG(p) & SYMV_RAWSYMBOL)
#define SYM_ISSEGMENT(p)        (SYM_ANYVTAG(p) & SYMV_SEGMENT)
/*
**  parameter list types
*/
#define NO_LIST         0
#define EMPTY_LIST      1
#define NAME_LIST       2
#define PASCAL_LIST     3
#define TYPE_LIST       4
/*
**  symbol storage classes
**  symbol adjectives, these have nothing to do with types.
*/
#define SCundef         0
#define SCauto          0x01
#define SCextern        0x02
#define SCregister      0x04
#define SCstatic        0x08
#define SCtypedef       0x10
#define SCglobal        0x20
#define SCabsolute      0x40
#define SCreally        0x80    /* used w/ SCregister by p2 if it allocs a reg */

#define SCp2stuff       SCreally

/*
**  symbol table sizes
*/
#define LEVEL_0         0xff
#define LEVEL_N         0x0f

struct  s_adj   {
        uchar_t bit_0:1;
        uchar_t bit_1:1;
        uchar_t bit_2:1;
        uchar_t bit_3:1;

        uchar_t bit_4:1;
        uchar_t bit_5:1;
        uchar_t bit_6:1;
        uchar_t bit_7:1;

        uchar_t bit_8:1;
        uchar_t bit_9:1;
        uchar_t bit_10:1;
        uchar_t bit_11:1;

        uchar_t bit_12:1;
        uchar_t bit_13:1;
        uchar_t bit_14:1;
        uchar_t bit_15:1;
        };

#if VERS_DEBUG
/*
 * d=DEFINED o=OUTPUT S=INASEGMENT n=NEAR
 * v=VISIBLE p=PASCAL i=INIT s=STRING
 * N=HASANAMELIST E=DEFNBEFOREUSE C=CODESEGNAME D=DATASEGNAME
 * B=ISBACKREF F=FORWARDREF
 */
#define SYM_ADJFMT                      "??FBDCENsipvnSod"      /* prword() fmt */
#endif

#define IS_INVISIBLE                    0
#define IS_VISIBLE                      1

#define SYM_ISDEFINED(s)                ((SYM_ANYADJ(s)).bit_0) /* all */
#define SYM_ISOUTPUT(s)                 ((SYM_ANYADJ(s)).bit_1) /* all */
#define SYM_ISINASEGMENT(s)             ((SYM_ANYADJ(s)).bit_2) /* all */
#define SYM_ISNEAR(s)                   ((SYM_ANYADJ(s)).bit_3) /* all */
#define SYM_ISVISIBLE(s)                ((SYM_ANYADJ(s)).bit_4) /* all */
#define SYM_ISPASCAL(s)                 ((SYM_ANYADJ(s)).bit_5) /* all */

#define SYM_ISINITIALIZED(s)            ((SYM_ANYADJ(s)).bit_6) /* symbol */
#define SYM_ISSTRING(s)                 ((SYM_ANYADJ(s)).bit_7) /* symbol */

#define SYM_HASANAMELIST(s)             ((SYM_ANYADJ(s)).bit_8) /* funcs */
#define SYM_DEFNBEFOREUSE(s)            ((SYM_ANYADJ(s)).bit_9) /* overload for QC */

#define SYM_ISCODESEGNAME(s)            ((SYM_ANYADJ(s)).bit_10)/* segment */
#define SYM_ISDATASEGNAME(s)            ((SYM_ANYADJ(s)).bit_11)/* segment */

#define SYM_ISBACKREF(s)                ((SYM_ANYADJ(s)).bit_12)/* label */
#define SYM_ISFORWARDREF(s)             ((SYM_ANYADJ(s)).bit_13)/* label */
#define SYM_ISMASM(s)                   ((SYM_ANYADJ(s)).bit_14)/* label */
#define SYM_TOLEVEL0(s)                 ((SYM_ANYADJ(s)).bit_15)/* funcs moved to 0 */

typedef struct  sym_200 {                       /* for fields */
        uchar_t         field_bitstart;         /* in which bit does the field start */
        uchar_t         field_nbits;            /* number of bits in this field */
        } sym_200_t;

typedef struct  sym_190 {                       /* struct/union members/fields */
        fpsym_t         member_prev;            /* previous member */
        ushort_t        member_offset;          /* offset of this member in the struct */
        sym_200_t       m200;
        } sym_190_t;

#define SOB_sym_190     (sizeof(sym_190_t) - sizeof(sym_200_t))

typedef struct  sym_180 {                       /* struct/union/enum tags */
        len_t           tag_size;                       /* sizeof the struct/union */
        ushort_t        tag_align;                      /* alignment of this struct */
        } sym_180_t;

typedef struct  sym_170 {                       /* intrinsics */
        ushort_t        intrin_ino;                     /* intrinsic number */
        ushort_t        intrin_nparms;          /* number of actuals it takes */
        } sym_170_t;

typedef struct  sym_160 {                       /* labels */
        NA_TYPE         label_template;         /* label template */
        } sym_160_t;

typedef struct  sym_150 {                       /* formals */
        fpsym_t         formal_prev;            /* ptr to previous formal */
        } sym_150_t;

typedef struct  sym_140 {                       /* raw symbols */
        hash_t          raw_hash;                       /* the hash of this symbol */
        } sym_140_t;

typedef union   sym_135 {
        sym_140_t       m140;                           /* raw symbols */
        sym_150_t       m150;                           /* formals */
        sym_160_t       m160;                           /* labels */
        } sym_135_t;

typedef struct  sym_130 {
/*
        SYMV_SYMBOL
        SYMV_RAWSYMBOL
        SYMV_FORMAL
        SYMV_SEGMENT
        SYMV_FUNCTION
        SYMV_LABEL
*/
        fpsym_t         sym_anysegment;         /* ptr to segment this is alloc'd in */
        sym_135_t       m135;
        } sym_130_t;

#define SOB_sym_130     (sizeof(sym_130_t) - sizeof(sym_135_t))

typedef union   sym_125 {
        sym_130_t       m130;
        sym_170_t       m170;
        } sym_125_t;

typedef struct  sym_120 {
/*
        case SYMV_SYMBOL:
        case SYMV_RAWSYMBOL:
        case SYMV_FORMAL:
        case SYMV_SEGMENT:
        case SYMV_FUNCTION:
        case SYMV_INTRINSIC:
        case SYMV_LABEL:
*/
        refcnt_t        sym_anyrefcnt;  /* reference count  */
        class_t         sym_anyclass;   /* symbol's class */
        symadj_t        sym_anyadj;             /* adjectives  */
        sym_125_t       m125;
        } sym_120_t;

#define SOB_sym_120     (sizeof(sym_120_t) - sizeof(sym_125_t))

typedef union   sym_115 {
        sym_120_t       m120;
        sym_180_t       m180;
        } sym_115_t;

typedef struct  sym_110 {
/*
        case SYMV_SYMBOL:
        case SYMV_RAWSYMBOL:
        case SYMV_FORMAL:
        case SYMV_SEGMENT:
        case SYMV_FUNCTION:
        case SYMV_INTRINSIC:
        case SYMV_LABEL:
        case SYMV_TAG:
*/
        p1key_t         sym_anykey;
        sym_115_t       m115;
        } sym_110_t;

#define SOB_sym_110     (sizeof(sym_110_t) - sizeof(sym_115_t))

typedef union   sym_105 {
        sym_110_t       m110;
        sym_190_t       m190;
        } sym_105_t;

typedef struct  sym_100 {
        ptype_t         sym_anytype;
        sym_105_t       m105;
        } sym_100_t;

#define SOB_sym_100     (sizeof(sym_100_t) - sizeof(sym_105_t))

typedef union   sym_95  {
        long            moe_value;
        sym_100_t       m100;
        } sym_95_t;

struct  s_sym   {
        fpsym_t         sym_anynext;            /*  pointer to next ident  */
        fpuchar_t       sym_anyname;            /*  pointer to name */
        ushort_t        sym_anyvtag;            /*  which variant do we have? */
        sym_95_t                m95;
        };

#define M95(p)  ((p)->m95)
#define M100(p) ((p)->m95.m100)
#define M105(p) ((p)->m95.m100.m105)
#define M110(p) ((p)->m95.m100.m105.m110)
#define M115(p) ((p)->m95.m100.m105.m110.m115)
#define M120(p) ((p)->m95.m100.m105.m110.m115.m120)
#define M125(p) ((p)->m95.m100.m105.m110.m115.m120.m125)
#define M130(p) ((p)->m95.m100.m105.m110.m115.m120.m125.m130)
#define M135(p) (M130(p).m135)
#define M140(p) (M135(p).m140)
#define M150(p) (M135(p).m150)
#define M160(p) (M135(p).m160)
#define M170(p) ((p)->m95.m100.m105.m110.m115.m120.m125.m170)
#define M180(p) ((p)->m95.m100.m105.m110.m115.m180)
#define M190(p) ((p)->m95.m100.m105.m190)
#define M200(p) (M190(p).m200)

#define SO_BASICSYM     (sizeof(sym_t) - sizeof(sym_95_t))

#define SO_SYMBOL               (SO_BASICSYM \
                                             + SOB_sym_100\
                                             + SOB_sym_110\
                                             + SOB_sym_120\
                                             + SOB_sym_130\
                                             )
#define SO_SEGMENT              (SO_SYMBOL)
#define SO_FUNCTION             (SO_SYMBOL)
#define SO_RAWSYMBOL            (SO_SYMBOL \
                                           + sizeof(sym_140_t)\
                                           )
#define SO_FORMAL               (SO_SYMBOL \
                                           + sizeof(sym_150_t)\
                                           )
#define SO_LABEL                (SO_SYMBOL \
                                           + sizeof(sym_160_t)\
                                           )
#define SO_INTRINSIC            (SO_BASICSYM \
                                             + SOB_sym_100\
                                             + SOB_sym_110\
                                             + SOB_sym_120\
                                             + sizeof(sym_170_t)\
                                             )
#define SO_TAG                  (SO_BASICSYM \
                                             + SOB_sym_100\
                                             + SOB_sym_110\
                                             + sizeof(sym_180_t)\
                                             )

#define SO_MEMBER               (SO_BASICSYM \
                                             + SOB_sym_100\
                                             + SOB_sym_190\
                                             )
#define SO_FIELD                (SO_MEMBER \
                                           + sizeof(sym_200_t)\
                                           )
#define SO_MOE                  (SO_BASICSYM + sizeof(long))    /* sizeof(moe_value) */

#if 0   /* this is just a big comment */

*all* use s_sym and the following parts.

SYMV_SYMBOL(and SYMV_SEGMENT, SYMV_FUNCTION)
                                uses : sym_100, sym_110, sym_120, sym_130

SYMV_RAWSYMBOL  uses : SYMV_SYMBOL *and* sym_140
SYMV_FORMAL             uses : SYMV_SYMBOL *and* sym_150
SYMV_LABEL              uses : SYMV_SYMBOL *and* sym_160

SYMV_INTRINSIC  uses : sym_100, sym_110, sym_120, sym_170

SYMV_TAG                uses : sym_100, sym_110, sym_180

SYMV_MEMBER             uses : sym_100, sym_190

SYMV_FIELD              uses : SYMV_MEMBER *and* sym_200

SYMV_MOE                uses : moe_value

SYMV_OPC

#endif

#define SYM_ANYNEXT(p)          ((p)->sym_anynext)
#define SYM_ANYNAME(p)          ((p)->sym_anyname)
#define SYM_ANYVTAG(p)          ((p)->sym_anyvtag)

#define SYM_ANYTYPE(p)          ((p)->m95.m100.sym_anytype)
#define SYM_ANYKEY(p)           ((p)->m95.m100.m105.m110.sym_anykey)
#define SYM_ANYREFCNT(p)        ((p)->m95.m100.m105.m110.m115.m120.sym_anyrefcnt)
#define SYM_ANYCLASS(p)         ((p)->m95.m100.m105.m110.m115.m120.sym_anyclass)
#define SYM_ANYADJ(p)           ((p)->m95.m100.m105.m110.m115.m120.sym_anyadj)
#define SYM_ANYSEGMENT(p)       (M130(p).sym_anysegment)
#define SYM_ANYAOFF(p)          (M130(p).sym_anyaoff)   /* P-2 allocation offset */
#define SYM_ANYAREGS(p)         (M130(p).sym_anyaregs)  /* P-2 allocation regs */
#define SYM_ANYASEG(p)          (M130(p).sym_anyaseg)   /* P-2 allocation segment */

#define SYM_SYNEXT(p)           (SYM_ANYNEXT(p))
#define SYM_SYNAME(p)           (SYM_ANYNAME(p))
#define SYM_SYTYPE(p)           (SYM_ANYTYPE(p))
#define SYM_SYKEY(p)            (SYM_ANYKEY(p))
#define SYM_SYREFCNT(p)         (SYM_ANYREFCNT(p))
#define SYM_SYCLASS(p)          (SYM_ANYCLASS(p))
#define SYM_SYADJ(p)            (SYM_ANYADJ(p))
#define SYM_SYSEGMENT(p)        (SYM_ANYSEGMENT(p))
#define SYM_SYAOFF(p)           (SYM_ANYAOFF(p))
#define SYM_SYASEG(p)           (SYM_ANYASEG(p))

#define SYM_SENEXT(p)           (SYM_ANYNEXT(p))
#define SYM_SENAME(p)           (SYM_ANYNAME(p))
#define SYM_SETYPE(p)           (SYM_ANYTYPE(p))
#define SYM_SEKEY(p)            (SYM_ANYKEY(p))
#define SYM_SEREFCNT(p)         (SYM_ANYREFCNT(p))
#define SYM_SECLASS(p)          (SYM_ANYCLASS(p))
#define SYM_SEADJ(p)            (SYM_ANYADJ(p))
#define SYM_SEAOFF(p)           (SYM_ANYAOFF(p))

#define SYM_RANAME(p)           (SYM_ANYNAME(p))
#define SYM_RATYPE(p)           (SYM_ANYTYPE(p))
#define SYM_RASEGMENT(p)        (SYM_ANYSEGMENT(p))
#define SYM_RAHASH(p)           (M140(p).raw_hash)

#define SYM_FUNEXT(p)           (SYM_ANYNEXT(p))
#define SYM_FUNAME(p)           (SYM_ANYNAME(p))
#define SYM_FUTYPE(p)           (SYM_ANYTYPE(p))
#define SYM_FUKEY(p)            (SYM_ANYKEY(p))
#define SYM_FUREFCNT(p)         (SYM_ANYREFCNT(p))
#define SYM_FUCLASS(p)          (SYM_ANYCLASS(p))
#define SYM_FUADJ(p)            (SYM_ANYADJ(p))
#define SYM_FUSEGMENT(p)        (SYM_ANYSEGMENT(p))
#define SYM_FUAOFF(p)           (SYM_ANYAOFF(p))
#define SYM_FUASEG(p)           (SYM_ANYASEG(p))

#define SYM_FONEXT(p)           (SYM_ANYNEXT(p))
#define SYM_FONAME(p)           (SYM_ANYNAME(p))
#define SYM_FOTYPE(p)           (SYM_ANYTYPE(p))
#define SYM_FOKEY(p)            (SYM_ANYKEY(p))
#define SYM_FOREFCNT(p)         (SYM_ANYREFCNT(p))
#define SYM_FOCLASS(p)          (SYM_ANYCLASS(p))
#define SYM_FOADJ(p)            (SYM_ANYADJ(p))
#define SYM_FOAOFF(p)           (SYM_ANYAOFF(p))
#define SYM_FOPREV(p)           (M150(p).formal_prev)

#define SYM_LANEXT(p)           (SYM_ANYNEXT(p))
#define SYM_LANAME(p)           (SYM_ANYNAME(p))
#define SYM_LATYPE(p)           (SYM_ANYTYPE(p))
#define SYM_LAKEY(p)            (SYM_ANYKEY(p))
#define SYM_LAREFCNT(p)         (SYM_ANYREFCNT(p))
#define SYM_LACLASS(p)          (SYM_ANYCLASS(p))
#define SYM_LAADJ(p)            (SYM_ANYADJ(p))
#define SYM_LAAOFF(p)           (SYM_ANYAOFF(p))
#define SYM_LAASEG(p)           (SYM_ANYASEG(p))
#define SYM_LATEMPLATE(p)       (M160(p).label_template)

#define SYM_INNEXT(p)           (SYM_ANYNEXT(p))
#define SYM_INNAME(p)           (SYM_ANYNAME(p))
#define SYM_INTYPE(p)           (SYM_ANYTYPE(p))
#define SYM_INKEY(p)            (SYM_ANYKEY(p))
#define SYM_INREFCNT(p)         (SYM_ANYREFCNT(p))
#define SYM_INCLASS(p)          (SYM_ANYCLASS(p))
#define SYM_INADJ(p)            (SYM_ANYADJ(p))
#define SYM_INNUMBER(p)         (M170(p).intrin_ino)
#define SYM_INNPARMS(p)         (M170(p).intrin_nparms)

#define SYM_TANEXT(p)           (SYM_ANYNEXT(p))
#define SYM_TANAME(p)           (SYM_ANYNAME(p))
#define SYM_TATYPE(p)           (SYM_ANYTYPE(p))
#define SYM_TAKEY(p)            (SYM_ANYKEY(p))
#define SYM_TASIZE(p)           (M180(p).tag_size)
#define SYM_TAALIGN(p)          (M180(p).tag_align)

#define SYM_MENEXT(p)           (SYM_ANYNEXT(p))
#define SYM_MENAME(p)           (SYM_ANYNAME(p))
#define SYM_METYPE(p)           (SYM_ANYTYPE(p))
#define SYM_MEPREV(p)           (M190(p).member_prev)
#define SYM_MEOFFSET(p)         (M190(p).member_offset)
#define SYM_MEVACCESS(p)        (M190(p).member_vaccess)

#define SYM_FINEXT(p)           (SYM_ANYNEXT(p))
#define SYM_FINAME(p)           (SYM_ANYNAME(p))
#define SYM_FITYPE(p)           (SYM_ANYTYPE(p))
#define SYM_FIPREV(p)           (M190(p).member_prev)
#define SYM_FIOFFSET(p)         (M190(p).member_offset)
#define SYM_FIVACCESS(p)        (M190(p).member_vaccess)
#define SYM_FIBITSTART(p)       (M200(p).field_bitstart)
#define SYM_FINBITS(p)          (M200(p).field_nbits)

#define SYM_MONEXT(p)           (SYM_ANYNEXT(p))
#define SYM_MONAME(p)           (SYM_ANYNAME(p))
#define SYM_MOVALUE(p)          ((p)->m95.moe_value)

/*
**      macros for acessing informmation on symbols type
*/
#define SYM_ANYBTYPE(P)         (TY_BTYPE(SYM_ANYTYPE(P)))
#define SYM_ANYESU(P)           (TY_ESU(SYM_ANYTYPE(P)))
#define SYM_ANYTINDEX(P)        (TY_TINDEX(SYM_ANYTYPE(P)))
#define SYM_ANYINDIR(P)         (TY_INDIR(SYM_SYTYPE(P)))

#define SYM_ANYISUB(P)          (INDIR_ISUB(SYM_ANYINDIR(P)))
#define SYM_ANYINEXT(P)         (INDIR_INEXT(SYM_ANYINDIR(P)))
#define SYM_ANYITYPE(P)         (INDIR_ITYPE(SYM_ANYINDIR(P)))
#define SYM_ANYIFORMALS(P)      (INDIR_IFORMALS(SYM_ANYINDIR(P)))

#define SYM_NEAR(P)             (IS_NEAR(SYM_ANYBTYPE(P)))
#define SYM_FAR(P)              (IS_FAR(SYM_ANYBTYPE(P)))
#define SYM_HUGE(P)             (IS_HUGE(SYM_ANYBTYPE(P)))
#define SYM_CONST(P)            (IS_CONST(SYM_ANYBTYPE(P)))
/*
**  the symbol table
*/
struct  s_table {
        table_t         *st_next;       /*  link to next  */
        table_t         *st_incl;       /*  block is included in block pointer  */
        fpsym_t         *st_table;      /*  ptr to hash table  */
        blknum_t        st_level;       /*  block level  */
        uchar_t         st_size;        /*  number of entries in hash table  */
        };
/*
**  macros for accessing the symbol tables.
**      `level' is the level of interest.
**      `ptab' is a ptr to a symbol table
*/
#define ST_NEXT(ptab)                   ((ptab)->st_next)
#define ST_INCL(ptab)                   ((ptab)->st_incl)
#define ST_SYM(ptab)                    ((ptab)->st_table)
#define ST_LEVEL(ptab)                  ((ptab)->st_level)
#define ST_MOD(ptab)                    ((ptab)->st_size)

#define ST_TABLE(level)                 ((level) ? Table_n : Table_0)
#define ST_BUCKET(ptab,hash)            (((ptab)->st_table[hash & ST_MOD(ptab)]))

/*
**      A Hash/Length/Name string is one where the first character is the hash
**      of the name.  The second character is the length of the identifier
**      including the hash and length characters.  The name begins at the third
**      character.
*/
#define HLN_IDENT_HASH(P)               (HLN_HASH(*(P)))
#define HLN_IDENT_LENGTH(P)             (HLN_LENGTH(*(P)))
#define HLN_IDENTP_NAME(P)              (HLN_NAME(*(P)))
/*
**  delcaration specifiers, used by to hold both the class and the type.
*/
struct  s_declspec      {
        class_t ds_class;
        ptype_t ds_type;
        };

#define DSP_CLASS(p)            ((p)->ds_class)
#define DSP_TYPE(p)             ((p)->ds_type)
#define DS_CLASS(p)             ((p).ds_class)
#define DS_TYPE(p)              ((p).ds_type)
