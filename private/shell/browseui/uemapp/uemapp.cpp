//***   uemapp.cpp -- application side of event monitor
// DESCRIPTION
//  event generators, actions, helpers, etc.

#include "priv.h"
#include "stream.h"
#include "uemapp.h"
#include "uemdb.h"

#define DM_UEMTRACE DM_TRACE

// glue for standalone test {
//#define XXX_TEST
//#define XXX_DEBUG

#ifdef XXX_TEST // {
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define lstrlen(s1)         strlen(s1)
#define lstrcmp(s1, s2)     strcmp(s1, s2)
#define lstrncmp(s1, s2, n) strncmp(s1, s2, n)

#define TRUE    1
#define FALSE   0
#endif // }
// }

// {
//***   event firers

STDAPI_(BOOL) SHGetUEMLogger(CEMDBLog **p);
CEMDBLog *g_uempDb;

//***   UEMRegister -- init/term various event monitor things 
//
void UEMRegister(BOOL fRegister)
{
    BOOL f;

    if (fRegister) {
        f = SHGetUEMLogger(&g_uempDb);
        ASSERT(g_uempDb);
    }
    else {
        ASSERT(0);              // never called?
        SAFERELEASE(g_uempDb);
    }
    return;
}

#define TABDAT(uemc)    uemc,
int UEMCValTab[] = {
    #include "uemcdat.h"
};
#undef  TABDAT

#define TABDAT(uemc)    TEXT(# uemc),
TCHAR *UEMCStrTab[] = {
    #include "uemcdat.h"
};
#undef  TABDAT

//***
// NOTES
//  BUGBUG sundown: int/ptr casts (i/UEMCStrTab not kosher...)
TCHAR *UEMCToStr(int uemc)
{
    int i;

    i = SHSearchMapInt(UEMCValTab, (int *)UEMCStrTab, ARRAYSIZE(UEMCValTab), uemc);
    return i == -1 ? TEXT("UEMC_?") : (TCHAR *)i;
}

//***   UEMTrace -- fire event
// NOTES
//  eventually this will fire events (and func name will change).  for now it
// just TraceMsg's them.
//  BUGBUG todo:
//  - pri=1 gotta filter events for privacy issues (esp. Ger).  not sure if
//  we should add a param saying 'usage' of event or just infer it from the
//  event.
//  - pri=? gotta encrypt the data we log
//
STDAPI_(void) UEMTrace(int eCmd, LPARAM lParam)
{
    static BOOL fVirg = TRUE;
    char szBuf[MAX_URL_STRING];
    TCHAR *psz;

    // for now nobody gets it (DEBUG or RETAIL) unless they ask for it
    if (!(g_dwPrototype & 0x00000800))
        return;

    if (fVirg) {
        fVirg = FALSE;
        UEMRegister(TRUE);
    }

    _try {
        switch (eCmd) {
        // UEME_UI
        case UEME_UIMENU:
            TraceMsg(DM_UEMTRACE, "uemt: e=uimenu idm=0x%x(%d)", (int)lParam, (int)lParam);
            psz = TEXT("UEME_UIMENU");
            goto Lcount;
            //break;
        case UEME_UISCUT:
            TraceMsg(DM_UEMTRACE, "uemt: e=uiscut lParam=0x%x(%d)", lParam, lParam);
            psz = TEXT("UEME_UISCUT");
            goto Lcount;
            //break;
        case UEME_UIQCUT:
            TraceMsg(DM_UEMTRACE, "uemt: e=uiqcut lParam=0x%x(%d)", lParam, lParam);
            psz = TEXT("UEME_UIQCUT");
            goto Lcount;
            //break;
        case UEME_UIHOTKEY:
            TraceMsg(DM_UEMTRACE, "uemt: e=uihotkey ghid=0x%x(%d)", (int)lParam, (int)lParam);
            psz = TEXT("UEME_UIHOTKEY");
            goto Lcount;
            //break;

        // UEME_RUN*
        case UEME_RUNWMCMD:
            psz = UEMCToStr((int)lParam);
            TraceMsg(DM_UEMTRACE, "uemt: e=runwmcmd id=%s(%d)", psz, (int)lParam);
Lcount:
            if (g_uempDb)
                g_uempDb->CountIncr(psz);
            break;
        case UEME_RUNPIDL:
            {
            szBuf[0] = 0;
            EVAL(ILGetDisplayName((LPCITEMIDLIST)lParam, szBuf));
            TraceMsg(DM_UEMTRACE, "uemt: e=runpidl pidl=%s(0x%x)", szBuf, (int)lParam);
            }
            break;
        case UEME_DBTRACEA:
            TraceMsg(DM_UEMTRACE, "uemt: e=runtrace s=%hs(0x%x)", (int)lParam, (int)lParam);
            break;
        case UEME_DBTRACEW:
            TraceMsg(DM_UEMTRACE, "uemt: e=runtrace s=%ls(0x%x)", (int)lParam, (int)lParam);
            break;

        // UEME_DONE*
        case UEME_DONECANCEL:
            TraceMsg(DM_UEMTRACE, "uemt: e=donecancel lP=%x", (int)lParam);
            break;

        // UEME_ERROR*
        case UEME_ERRORA:
            TraceMsg(DM_UEMTRACE, "uemt: e=errora id=%hs(0x%x)", (LPSTR)lParam, (int)lParam);
            break;
        case UEME_ERRORW:
            TraceMsg(DM_UEMTRACE, "uemt: e=errorw id=%ls(0x%x)", (LPWSTR)lParam, (int)lParam);
            break;

        default:
            TraceMsg(DM_UEMTRACE, "uemt: e=0x%x(%d) lP=0x%x(%d)", eCmd, eCmd, (int)lParam, (int)lParam);
            break;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // since we're called from the DebMemLeak, we're only *guessing*
        // that we have a vptr etc., so we might fault.
        TraceMsg(TF_ALWAYS, "uemt: GPF");
    }

    return;
}

// }

// {
#if 0 // currently unused
//***   rule: smart rename
// DESCRIPTION
//  we recognize renames that remove prefixes (e.g. "Shortcut to ..."),
//  and apply automatically to future renames.  adding back the prefix
//  resets to the original state.
// NOTES
//  BUGBUG assumes only care about prefix, which is bogus for non-US

struct renpre {
    char    *pszPrefix;     // prefix we care about
    int     fChange;        // recent change: 0=nil +1=add -1=del
};

struct renpre SmartPrefixTab[] = {
    { "Shortcut to ", 0 },
    0
};

//***   IsSuffix -- return index of (possible) suffix in string
// ENTRY/EXIT
//  i   (return) index of suffix in string, o.w. -1
int IsSuffix(char *pszStr, char *pszSuf)
{
    int cchStr, cchSuf;
    int i;

    cchStr = lstrlen(pszStr);
    cchSuf = lstrlen(pszSuf);
    if (cchSuf > cchStr)
        return -1;
    i = cchStr - cchSuf;
    if (lstrcmp(pszStr + i, pszSuf) == 0)
        return i;
    return -1;
}

int UpdateSmartPrefix(char *pszOld, char *pszNew, int fChange);

//***   UEMOnRename -- track renames for interesting patterns
// ENTRY/EXIT
//  (SE)    updates smart prefix table if rename effects it
void UEMOnRename(char *pszOld, char *pszNew)
{
    if (UpdateSmartPrefix(pszOld, pszNew, -1)) {
#ifdef XXX_DEBUG
        //  "Shortcut to foo" to "foo"
        printf("or: -1\n");
#endif
    }
    else if (UpdateSmartPrefix(pszNew, pszOld, +1)) {
#ifdef XXX_DEBUG
        //  "foo" to "Shortcut to foo"
        printf("or: +1\n");
#endif
    }
    return;
}

//***   UpdateSmartPrefix -- if op effects prefix, mark change
//
int UpdateSmartPrefix(char *pszOld, char *pszNew, int fChange)
{
    int i;
    struct renpre *prp;

    // (note that if we rename to the same thing, i==0, so we don't
    // do anything.  this is as it should be).

    if ((i = IsSuffix(pszOld, pszNew)) > 0) {
        prp = &SmartPrefixTab[0];   // TODO: for each ...

        // iSuf==cchPre, so pszOld[0..iSuf-1] is prefix
        if (i == lstrlen(prp->pszPrefix) && lstrncmp(pszOld, prp->pszPrefix, i) == 0) {
#ifdef XXX_DEBUG
            printf("usp: o=%s n=%s p=%s f=%d\n", pszOld, pszNew, SmartPrefixTab[0].pszPrefix, fChange);
#endif
            prp->fChange = fChange;
            return 1;
        }
    }

    return 0;
}

//***   GetSmartRename --
// ENTRY/EXIT
//  pszDef  proposed default name (in 'original' form, e.g. 'Shortcut to')
//  i       (ret) index of 'smart' default name
int GetSmartRename(char *pszDef)
{
    char *pszPre;
    int cchPre;

    // for each prefix in smartPrefixList ...
    pszPre = SmartPrefixTab[0].pszPrefix;

    cchPre = lstrlen(pszPre);
    if (strncmp(pszDef, pszPre, cchPre) == 0) {
        if (SmartPrefixTab[0].fChange == -1)
            return cchPre;
    }
    return 0;
}
#endif
// }

#ifdef XXX_TEST // {
char c_szScToFoo[] = "Shortcut to foo";
char c_szScToFred[] = "Shortcut to fred";

char *TestTab[] = {
    c_szScToFoo,
    c_szScToFred,
    "bar",
    0
};

pr(char *p)
{
    int i;

    i = GetSmartRename(p);
    if (i >= 0)
        printf("\t<%s>", p + i);
    else
        printf("\t<%s>", p);
    return;
}

prtab()
{
    pr("foo");
    pr(c_szScToFoo);
    pr(c_szScToFred);
    printf("\n");
}

TstRename()
{
    int i;

    // original
    prtab();

    // delete
    printf("del\n");
    UEMOnRename(c_szScToFoo, "foo");
    prtab();

    // ... again
    printf("del\n");
    UEMOnRename(c_szScToFoo, "foo");
    prtab();

    // add (restore)
    printf("add\n");
    UEMOnRename("foo", c_szScToFoo);
    prtab();

    // ... again
    printf("add\n");
    UEMOnRename("foo", c_szScToFoo);
    prtab();

    // delete partial (shouldn't look like prefix)
    printf("del partial\n");
    UEMOnRename(c_szScToFoo, "to foo");
    prtab();

    // rename to same (shouldn't look like prefix)
    printf("ren same\n");
    UEMOnRename(c_szScToFoo, "c_szScToFoo");
    prtab();
}

main()
{
    TstRename();
}

#endif // }
