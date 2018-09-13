/*
**      Tree types
*/
#define TTfree                  0x0
#define TTconstant              0x01
#define TTsymbol                0x02
#define TTunary                 0x04
#define TTleaf                  0x08
#define TTbinary                0x10
#define TThybrid                0x20
#define TTBasicShape    (TTfree|TTconstant|TTsymbol|TTunary|TTbinary|TThybrid)
#define TTzero                  0x40

typedef USHORT          p2type_t;
typedef p2type_t        *pp2type_t;

typedef struct  tree_200        {
        ptree_t         tr_left;        /*  left child  */
        ptree_t         tr_right;       /*  right child  */
        } tree_200_st;

typedef struct  tree_190        {
        ptree_t         tr_uchild;      /*  unary child  */
        } tree_190_st;

typedef struct  tree_180        {
        psym_t          tr_symbol;      /*  symbol  */
        } tree_180_st;

typedef struct  tree_170        {
        value_t         tr_value;       /*  value of the tree  */
        } tree_170_st;

typedef union   tree_100        {
        tree_200_st     t200;
        tree_190_st     t190;
        tree_180_st     t180;
        tree_170_st     t170;
        } tree_100_st;

struct  s_tree  {
        token_t         tr_token;       /*  tree's token  */
        shape_t         tr_shape;       /*  tree shape  */
        ptype_t         tr_p1type;      /*  p1's view of the type  */
        p2type_t        tr_p2type;      /*  p1's view of the type p2 should have */
        tree_100_st     t100;
        };

#define TR_SHAPE(P)     ((P)->tr_shape)
#define BASIC_SHAPE(S)  ((S) & TTBasicShape)
#define TR_TOKEN(P)     ((P)->tr_token)
#define TR_P1TYPE(P)    ((P)->tr_p1type)                /*  resultant type  */
#define TR_P2TYPE(P)    ((P)->tr_p2type)                /*  resultant type  */
#define TR_ISZERO(P)    (TR_SHAPE(P) & TTzero)

#define TR_LEFT(P)      ((P)->t100.t200.tr_left)        /*  left child  */
#define TR_RIGHT(P)     ((P)->t100.t200.tr_right)       /*  right child  */
#define TR_UCHILD(P)    ((P)->t100.t190.tr_uchild)      /*  unary's child */
#define TR_SVALUE(P)    ((P)->t100.t180.tr_symbol)      /*  ptr to the symbol */
#define TR_VALUE(P)     ((P)->t100.t170.tr_value)       /*  value of tree  */

#define TR_RCON(P)      (TR_VALUE(P).v_rcon)            /*  real constant  */
#define TR_DVALUE(P)    (TR_RCON(P)->rcon_real) /*  double value  */
#define TR_LVALUE(P)    (TR_VALUE(P).v_long)            /*  long value  */
#define TR_STRING(P)    (TR_VALUE(P).v_string)  /*  string value  */

#define TR_CVALUE(P)    (TR_STRING(P).str_ptr)  /*  ptr to string  */
#define TR_CLEN(P)      (TR_STRING(P).str_len)  /*  length of string  */

#define TR_BTYPE(P)     (TY_BTYPE(TR_P1TYPE(P)))/*  base type  */
#define TR_ESU(P)       (TY_ESU(TR_P1TYPE(P)))  /*  parent enum/struct/union  */
#define TR_INDIR(P)     (TY_INDIR(TR_P1TYPE(P)))

#define TR_INEXT(P)     (INDIR_INEXT(TR_INDIR(P)))
#define TR_ITYPE(P)     (INDIR_ITYPE(TR_INDIR(P)))
#define TR_ISUB(P)      (INDIR_ISUB(TR_INDIR(P)))
#define TR_IFORMALS(P)  (INDIR_IFORMALS(TR_INDIR(P)))
/*
**  for cases
*/
struct  s_case  {
        case_t  *c_next;        /*  next in list  */
        long    c_expr;         /*  value of constant expression  */
        p1key_t c_label;        /*  label to which to jump if expr  */
        };

#define NEXT_CASE(p)    ((p)->c_next)
#define CASE_EXPR(p)    ((p)->c_expr)
#define CASE_LABEL(p)   ((p)->c_label)

/*
**  loop inversion structs
**  for( init; test; incr ) { ... }
**  we handle : sym | const relop sym | const; sym op sym | const
*/
typedef struct  s_loopia         loopia_t, *loopiap_t;
typedef struct  s_liarray        liarray_t, *liarrayp_t;

struct  s_loopia        {
        token_t         lia_token;
        union   {
                psym_t          lia_sym;
                long            lia_value;
                liarrayp_t      lia_array;
                } lia_union;
        };

#define LIA_TOKEN(p)    ((p)->lia_token)
#define LIA_SYM(p)      ((p)->lia_union.lia_sym)
#define LIA_VALUE(p)    ((p)->lia_union.lia_value)
#define LIA_ARRAY(p)    ((p)->lia_union.lia_array)

typedef struct  s_liarray       {
        loopia_t        liar_left;
        loopia_t        liar_right;
        } liarray;

#define LIAR_LEFT(p)    (&((p)->liar_left))
#define LIAR_RIGHT(p)   (&((p)->liar_right))

typedef struct  s_loopi {
        int             li_relop;
        int             li_incop;
        loopia_t        li_w;
        loopia_t        li_x;
        loopia_t        li_y;
        loopia_t        li_z;
        } loopi_t, *loopip_t;

#define LOOP_RELOP(p)   ((p)->li_relop)
#define LOOP_INCOP(p)   ((p)->li_incop)

#define LOOP_W(p)       (&((p)->li_w))
#define LOOP_X(p)       (&((p)->li_x))
#define LOOP_Y(p)       (&((p)->li_y))
#define LOOP_Z(p)       (&((p)->li_z))

#define LOOP_W_TOKEN(p) LIA_TOKEN(LOOP_W(p))
#define LOOP_X_TOKEN(p) LIA_TOKEN(LOOP_X(p))
#define LOOP_Y_TOKEN(p) LIA_TOKEN(LOOP_Y(p))
#define LOOP_Z_TOKEN(p) LIA_TOKEN(LOOP_Z(p))

#define LOOP_W_SYM(p)   LIA_SYM(LOOP_W(p))
#define LOOP_X_SYM(p)   LIA_SYM(LOOP_X(p))
#define LOOP_Y_SYM(p)   LIA_SYM(LOOP_Y(p))
#define LOOP_Z_SYM(p)   LIA_SYM(LOOP_Z(p))

#define LOOP_W_VALUE(p) LIA_VALUE(LOOP_W(p))
#define LOOP_X_VALUE(p) LIA_VALUE(LOOP_X(p))
#define LOOP_Y_VALUE(p) LIA_VALUE(LOOP_Y(p))
#define LOOP_Z_VALUE(p) LIA_VALUE(LOOP_Z(p))
/*
**      stack structure for saving items which must be stacked at various places
*/
struct  s_stack {
        stack_t *stk_next;
        union   {
                ptree_t         sv_tree;
                psym_t          sv_sym;
                int             sv_int;
                loopip_t        sv_loopi;
                } stk_value;
        };

#define TEST_LAB                (Test->stk_value.sv_tree)
#define START_LAB               (Start->stk_value.sv_tree)
#define CONTINUE_LAB            (Continue->stk_value.sv_tree)
#define BREAK_LAB               (Break->stk_value.sv_tree)
#define CA_LAB                  (Case->stk_value.sv_tree)
#define DEFAULT_LAB             (Default->stk_value.sv_tree)

#define LOOPI(p)                ((p)->stk_value.sv_loopi)
