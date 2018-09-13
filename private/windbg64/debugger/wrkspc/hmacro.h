#undef STD_VAR_WRKSPC
#define STD_VAR_WRKSPC(type, varname, value)            type varname

#undef VAR_WRKSPC
#define VAR_WRKSPC(type, regwrapper, varname, value)    type varname

#undef BIN_VAR_WRKSPC
#define BIN_VAR_WRKSPC(type, regwrapper, varname)       type varname

#undef CONT_WRKSPC
#define CONT_WRKSPC(type, varname)                      type varname

// Dynamic container
#undef D_CONT_WRKSPC
#define D_CONT_WRKSPC(type, varname)                    type varname

// Mirrored container
#undef M_CONT_WRKSPC
#define M_CONT_WRKSPC(type, varname)                    type varname

// Mirrored & Dynamic container
#undef D_M_CONT_WRKSPC
#define D_M_CONT_WRKSPC(type, varname)                  type varname

