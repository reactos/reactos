#define RCON_REAL(p)    ((p)->rcon_real)

/*
**  given a VALUE ptr, return the various fields.
*/
#define PV_RCON(P)      ((P)->v_rcon)
#define PV_DOUBLE(P)    (PV_RCON(P)->rcon_real)
#define PV_LONG(P)      ((P)->v_long)
#define PV_STRPTR(P)    ((P)->v_string.str_ptr)
#define PV_STRLEN(P)    ((P)->v_string.str_len)
#define PV_SYM(P)       ((P)->v_symbol)
/*
**  given a reference to a VALUE, return the given field
*/
#define V_RCON(V)       ((V).v_rcon)
#define V_DOUBLE(V)     (V_RCON(V)->rcon_real)
#define V_LONG(V)       ((V).v_long)
#define V_STRPTR(V)     ((V).v_string.str_ptr)
#define V_STRLEN(V)     ((V).v_string.str_len)
#define V_SYM(V)        ((V).v_symbol)
