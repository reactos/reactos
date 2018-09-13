/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* P1SUP.C - First pass C stuff which probably is not used              */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"

/* trees */
#define LEFT                    1
#define RIGHT                   2

#define MORE_CHECKING   2

int TypeCount;
int TreeCount;

type_entry_t    *Type_table[TYPE_TABLE_SIZE];


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
ptype_t  hash_type(ptype_t);
int     types_equal(ptype_t, ptype_t);


/************************************************************************
**  hash_type : returns a pointer to an already built type, if it
**  exists, or builds one.
************************************************************************/
ptype_t
hash_type(
    type_t *p_type
    )
{
    REG type_entry_t    *p_tmp;
    type_entry_t        **p_start;

    /* Try to get a unique hash value for every type...keep
         * type_equal in mind if changing this
         */
    p_start = &Type_table[(TY_BTYPE(p_type) + TY_DTYPE(p_type) + (INT_PTR) TY_INDIR(p_type)) & (TYPE_TABLE_SIZE - 1)];

    for(p_tmp = *p_start; p_tmp; p_tmp = p_tmp->te_next ) {
        if(types_equal(p_type,&(p_tmp->te_type))) {
            return(&(p_tmp->te_type));
        }
    }
    p_tmp = (type_entry_t *) MyAlloc(sizeof(type_entry_t));
    if (p_tmp == NULL) {
        strcpy (Msg_Text, GET_MSG (1002));
        error(1002);
        return NULL;
    }
    p_tmp->te_next = *p_start;
    *p_start = p_tmp;
    p_tmp->te_type = *p_type;
    TY_TINDEX(&(p_tmp->te_type)) = 0;
    return(&(p_tmp->te_type));
}


/************************************************************************
**  types_equal : are two types equal?
************************************************************************/
int
types_equal(
    REG ptype_t p1,
    REG ptype_t p2
    )
{
    return((TY_BTYPE(p1) == TY_BTYPE(p2))
        &&
        (TY_DTYPE(p1) == TY_DTYPE(p2))
        &&
        TY_INDIR(p1) == TY_INDIR(p2)
        );
}

/************************************************************************
**      build_const - builds and returns a pointer to a constant tree.
**              Input   : constant type.
**                      : ptr to a union containing the value of the constant
**              Output  : Pointer to constant tree.
************************************************************************/
ptree_t
build_const(
    REG token_t type,
    value_t *value
    )
{
    REG ptree_t res;
    ptype_t     p_type;
    btype_t     btype;

    res = (ptree_t) MyAlloc(sizeof(tree_t));
    if (res == NULL) {
        strcpy (Msg_Text, GET_MSG (1002));
        error(1002);
        return NULL;
    }
    TR_SHAPE(res) = TTconstant;
    TR_TOKEN(res) = type;
    switch( type ) {
        case L_CINTEGER:
        case L_LONGINT:
        case L_CUNSIGNED:
        case L_LONGUNSIGNED:
            if( type == L_CUNSIGNED || type == L_LONGUNSIGNED ) {
                btype = (btype_t)(BT_UNSIGNED |
                     (btype_t)((type == L_CUNSIGNED) ? BTint : BTlong));
            } else {
                btype = (btype_t)((type == L_CINTEGER) ? BTint : BTlong);
            }

            if((TR_LVALUE(res) = PV_LONG(value)) == 0) {
                TR_SHAPE(res) |= TTzero;
            }
            break;
        case L_CFLOAT:
            btype = BTfloat;
            TR_RCON(res) = PV_RCON(value);
            break;
        case L_CDOUBLE:
            btype = BTdouble;
            TR_RCON(res) = PV_RCON(value);
            break;
        case L_CLDOUBLE:
            btype = BTldouble;
            TR_RCON(res) = PV_RCON(value);
            break;
        default:
            break;
    }
    p_type = (ptype_t) MyAlloc(sizeof(type_t));
    if (p_type == NULL) {
        strcpy (Msg_Text, GET_MSG (1002));
        error(1002);
        return NULL;
    }
    TY_BTYPE(p_type) = (btype_t)(btype | BT_CONST);
    TR_P1TYPE(res) = hash_type(p_type);
    return(res);
}
