// lexer-specific defines and types

// reserved word and special token ID's
typedef enum {
    RW_NOTFOUND = 0,
// special character RWs
    RW_EOF,
    RW_LBRACKET,
    RW_RBRACKET,
    RW_LCURLY,
    RW_RCURLY,
    RW_LPAREN,
    RW_RPAREN,
    RW_SEMI,
    RW_COLON,
    RW_PERIOD,
    RW_COMMA,
    RW_ASSIGN,
    RW_POINTER,
    RW_HYPHEN,

// keyword RWs
    RW_LIBRARY,
    RW_TYPEDEF,
    RW_ENUM,
    RW_STRUCT,
    RW_MODULE,
    RW_INTERFACE,
    RW_DISPINTERFACE,
    RW_COCLASS,
    RW_PROPERTIES,
    RW_METHODS,
    RW_IMPORTLIB,
    RW_PASCAL,
    RW_CDECL,
    RW_STDCALL,
    RW_UNSIGNED,
    RW_UNION,
    RW_EXTERN,
    RW_FAR,
    RW_SAFEARRAY,
    RW_CONST,
#if FV_PROPSET
    RW_PROPERTY_SET,
#endif //FV_PROPSET

// literals
    LIT_STRING,
    LIT_NUMBER,
    LIT_UUID,
    LIT_ID,

// operators
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EXP,
    OP_LOG_AND,
    OP_LOG_OR,
    OP_LOG_NOT,
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_NOT,
    OP_LSHIFT,
    OP_RSHIFT,
    OP_EQ,
    OP_LE,
    OP_LT,
    OP_GE,
    OP_GT,

    OP_MIN = OP_ADD,
    OP_MAX = OP_GT,

// attribute reserved words
    ATTR_UUID,
    ATTR_VERSION,
    ATTR_DLLNAME,
    ATTR_ENTRY,
    ATTR_ID,
    ATTR_HELPSTRING,
    ATTR_HELPCONTEXT,
    ATTR_HELPFILE,
    ATTR_LCID,
    ATTR_PROPGET,
    ATTR_PROPPUT,
    ATTR_PROPPUTREF,
    ATTR_OPTIONAL,
    ATTR_IN,
    ATTR_OUT,
    ATTR_STRING,
    ATTR_VARARG,
    ATTR_APPOBJECT,
    ATTR_RESTRICTED,
    ATTR_PUBLIC,
    ATTR_READONLY,
    ATTR_ODL,
    ATTR_DEFAULT,
    ATTR_SOURCE,
    ATTR_BINDABLE,
    ATTR_REQUESTEDIT,
    ATTR_DISPLAYBIND,
    ATTR_DEFAULTBIND,
    ATTR_LICENSED,
    ATTR_PREDECLID,
    ATTR_HIDDEN,
    ATTR_RETVAL,
    ATTR_CONTROL,
    ATTR_DUAL,
    ATTR_NONEXTENSIBLE,
    ATTR_OLEAUTOMATION,
} TOKID;

#define	fACCEPT_ATTR		0x0001
#define	fACCEPT_UUID		0x0002
#define	fACCEPT_NUMBER		0x0004
#define	fACCEPT_EOF		0x0008
#define	fACCEPT_OPERATOR	0x0010
#define	fACCEPT_STRING		0x0020


typedef	struct {
    TOKID	id;
    union {
       LPSTR	lpsz;
       GUID FAR * lpUuid;
       DWORD	number;
    };
    WORD cbsz;	// only if string
} TOKEN;
