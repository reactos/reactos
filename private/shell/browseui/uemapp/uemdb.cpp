#include "priv.h"
#include "stream.h"
#include "uemdb.h"

char c_szCount[] = TEXT("Count");
#if 0
char c_szDotDot[] = TEXT("..");     // RegStrFS does *not* support
#endif

// {
//***   CEMDBLog --

CEMDBLog *g_uempDbSvr;

//***   SHGetUEMLogger -- get the (shared) instance of our logger object
// NOTES
//  BUGBUG we leak g_uempDbSvr.  this is by design.
STDAPI_(BOOL) SHGetUEMLogger(CEMDBLog **p)
{
    HRESULT hr;

    if (g_uempDbSvr == 0) {     // BUGBUG race!
        g_uempDbSvr = CEMDBLog_Create(SHGetExplorerHkey(), STGM_WRITE);

        if (g_uempDbSvr) {
            hr = g_uempDbSvr->ChDir(TEXT("UIProf"));
            if (SUCCEEDED(hr)) {
                hr = g_uempDbSvr->ChDir(c_szCount);
            }
            if (FAILED(hr)) {
                ASSERT(0);
                SAFERELEASE(g_uempDbSvr);
            }
        }
    }

    if (g_uempDbSvr)
        g_uempDbSvr->AddRef();

    *p = g_uempDbSvr;
    return g_uempDbSvr != 0;
}

CEMDBLog *CEMDBLog_Create(HKEY hk, DWORD grfMode)
{
    CEMDBLog *p = new CEMDBLog;

    if (p && FAILED(p->Initialize(hk, grfMode))) {
        p->Release();
        p = NULL;
    }

    return p;
}

CEMDBLog::CEMDBLog()
{
    return;
}

CEMDBLog::~CEMDBLog()
{
    return;
}

//***   THIS::Count -- increment profile count for command
// NOTES
//  BUGBUG NYI: *must* be made thread-safe
HRESULT CEMDBLog::CountIncr(char *cmd)
{
    HRESULT hr;
    DWORD cnt, cb;

#if 0 // ChDir is currently done at create time 
    hr = _prsfs->ChDir(c_szCount);
#endif
    hr = S_OK;
    if (SUCCEEDED(hr)) {
        cnt = 0;
        cb = SIZEOF(cnt);
        hr = /*_prsfs->*/QueryValue(cmd, (BYTE *)&cnt, &cb);
        ASSERT(SUCCEEDED(hr) || cnt == 0);

        ++cnt;

        ASSERT(cb == SIZEOF(cnt));
        hr = /*_prsfs->*/SetValue(cmd, REG_DWORD, (BYTE *)&cnt, cb);
        ASSERT(SUCCEEDED(hr));

#if 0
        ASSERT(0);      // BUGBUG: ".." NYI!!!
        hr = /*_prsfs->*/ChDir(c_szDotDot);
#endif
    }

    return hr;
}

#if 0
#ifdef DEBUG
void emdbtst()
{
    HRESULT hr;
    CEMDBLog *pdb = new CEMDBLog;

    hr = pdb->Initialize(HKEY_CURRENT_USER, TEXT("UIProf"));
    ASSERT(SUCCEEDED(hr));

    pdb->CountIncr("foo");
    pdb->CountIncr("bar");
    pdb->CountIncr("foo");

    delete pdb;

    return;
}
#endif
#endif

// }
