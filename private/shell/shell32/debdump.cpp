#include "shellprv.h"
#pragma  hdrstop

#include "sfviewp.h"
#include "docfindx.h"


#define DM_FIXME    0       // trace/break when hit unknown guy

struct DBClassInfo {
    int     cbSize;
    TCHAR * pszName;
};

#define TABENT(c)   { SIZEOF(c), TEXT(#c) }
static const struct DBClassInfo DBClassInfoTab[] =
{
    // BUGBUG tons of table entries missing
    // maybe drive off same file as debug extensions dumpers?
    TABENT(CDefView),
    TABENT(CDFFolder),
    TABENT(CDocFindSFVCB),
    { 0 },
};
#undef  TABENT

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
    const  struct DBClassInfo *p;

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
