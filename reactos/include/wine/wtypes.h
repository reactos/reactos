#include_next <wtypes.h>

#ifndef __WINE_WTYPES_H
#define __WINE_WTYPES_H

typedef CHAR OLECHAR16;

typedef LPSTR LPOLESTR16;

typedef LPCSTR LPCOLESTR16;

/* Should be in MSHCTX enum but is not in w32api */
#define MSHCTX_CROSSCTX 4

#endif /* __WINE_WTYPES_H */
