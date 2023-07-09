#undef STD_VAR_WRKSPC
#define STD_VAR_WRKSPC(type, varname, value)                        \
varname = value

#undef VAR_WRKSPC
#define VAR_WRKSPC(type, regwrapper, varname, value)                \
{                                                                   \
    varname = value;                                                \
    regwrapper * p = new regwrapper(#varname, &varname);            \
    Assert(p);                                                      \
    m_listItems.InsertTail(p);                                      \
}

#undef BIN_VAR_WRKSPC
#define BIN_VAR_WRKSPC(type, regwrapper, varname)                   \
{                                                                   \
    ZeroMemory(&varname, sizeof(varname) );                         \
    regwrapper * p = new regwrapper(#varname, &varname);            \
    Assert(p);                                                      \
    m_listItems.InsertTail(p);                                      \
}

#undef CONT_WRKSPC
#define CONT_WRKSPC(type, varname)          \
varname.Init(this, #varname, FALSE, FALSE)

// Dynamic container
#undef D_CONT_WRKSPC
#define D_CONT_WRKSPC(type, varname)        \
varname.Init(this, #varname, FALSE, TRUE)


// Mirrored container
#undef M_CONT_WRKSPC
#define M_CONT_WRKSPC(type, varname)        \
varname.Init(this, #varname, TRUE, FALSE)


// Mirrored & Dynamic container
#undef D_M_CONT_WRKSPC
#define D_M_CONT_WRKSPC(type, varname)      \
varname.Init(this, #varname, TRUE, TRUE)
