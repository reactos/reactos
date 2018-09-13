#include "priv.h"

#ifdef DEBUG

#define DM_FIXME    0       // trace/break when hit unknown guy

struct DBClassInfo {
    int     cbSize;
    TCHAR * pszName;
};

//
// EXTERNALOBJECTS is a macro which simply expands to X(C,0) X(D,1)
// X(E, 2)...
// where C, D, E, ... are classes whose sizes are defined externally.
//

#define EXTERNALOBJECTS 
//    X(CSDWindows, 0)  \
//    X(CDesktopBrowser, 1)  \

#define TABENT(c)   { SIZEOF(c), TEXT(#c) }
#define X(c, n)  { 0, TEXT(#c) },
struct DBClassInfo DBClassInfoTab[] =
{

    #define NUM_INTERNAL_OBJECTS 11

    EXTERNALOBJECTS // 3...
    { 0 },
};
#undef  TABENT
#undef  X

#define X(c, n) extern "C" extern const int SIZEOF_##c;
EXTERNALOBJECTS
#undef X

//***   DBGetClassSymbolic -- map size to class name (guess)
// NOTES
//  we just take the 1st hit, so if there are multiple classes w/ the
//  same size you get the wrong answer.  if that turns out to be a pblm
//  we can add special-case heuristics for the relevant classes.
//
//  BUGBUG TODO: should use a generic DWORD value/data pair lookup
//  helper func.
//

TCHAR *DBGetClassSymbolic(int cbSize)
{
    struct DBClassInfo *p;

#define X(c, n) \
    DBClassInfoTab[NUM_INTERNAL_OBJECTS+n].cbSize = SIZEOF_##c;
    EXTERNALOBJECTS
#undef X

    for (p = DBClassInfoTab; p->cbSize != 0; p++) {
        if (p->cbSize == cbSize)
            return p->pszName;
    }
    if (DM_FIXME) {
        TraceMsg(DM_FIXME, "DBgcs: cbSize=%d  no entry", cbSize);
        ASSERT(0);
    }
    return NULL;
}

#endif // #ifdef DEBUG

