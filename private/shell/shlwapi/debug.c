#include "priv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "shlwapi"
#define SZ_MODULE           "SHLWAPI"
#define DECLARE_DEBUG
#include <debug.h>


#ifdef DEBUG

/*
 * macro for simplifying result to string translation, assumes result string
 * pointer pcsz
 */

#define STRING_CASE(val)               case val: pcsz = TEXT(#val); break


#if 0
//
//  Debug value-to-string mapping functions
//


/*----------------------------------------------------------
Purpose: Return the string form of the clipboard format.

Returns: pointer to a static string
Cond:    --
*/
LPCTSTR 
Dbg_GetCFName(
    UINT ucf)
{
    LPCTSTR pcsz;
    static TCHAR s_szCFName[MAX_PATH];

    switch (ucf)
    {
        STRING_CASE(CF_TEXT);
        STRING_CASE(CF_BITMAP);
        STRING_CASE(CF_METAFILEPICT);
        STRING_CASE(CF_SYLK);
        STRING_CASE(CF_DIF);
        STRING_CASE(CF_TIFF);
        STRING_CASE(CF_OEMTEXT);
        STRING_CASE(CF_DIB);
        STRING_CASE(CF_PALETTE);
        STRING_CASE(CF_PENDATA);
        STRING_CASE(CF_RIFF);
        STRING_CASE(CF_WAVE);
        STRING_CASE(CF_UNICODETEXT);
        STRING_CASE(CF_ENHMETAFILE);
        STRING_CASE(CF_HDROP);
        STRING_CASE(CF_LOCALE);
        STRING_CASE(CF_MAX);
        STRING_CASE(CF_OWNERDISPLAY);
        STRING_CASE(CF_DSPTEXT);
        STRING_CASE(CF_DSPBITMAP);
        STRING_CASE(CF_DSPMETAFILEPICT);
        STRING_CASE(CF_DSPENHMETAFILE);

    default:
        if (! GetClipboardFormatName(ucf, s_szCFName, SIZECHARS(s_szCFName)))
            lstrcpy(s_szCFName, TEXT("UNKNOWN CLIPBOARD FORMAT"));
        pcsz = s_szCFName;
        break;
    }

    ASSERT(pcsz);

    return(pcsz);
}


LPCTSTR 
Dbg_GetHRESULTName(
    HRESULT hr)
{
    LPCTSTR pcsz;
    static TCHAR s_rgchHRESULT[] = TEXT("0x12345678");

    switch (hr)
        {
        STRING_CASE(S_OK);
        STRING_CASE(S_FALSE);

        STRING_CASE(DRAGDROP_S_CANCEL);
        STRING_CASE(DRAGDROP_S_DROP);
        STRING_CASE(DRAGDROP_S_USEDEFAULTCURSORS);

        STRING_CASE(E_UNEXPECTED);
        STRING_CASE(E_NOTIMPL);
        STRING_CASE(E_OUTOFMEMORY);
        STRING_CASE(E_INVALIDARG);
        STRING_CASE(E_NOINTERFACE);
        STRING_CASE(E_POINTER);
        STRING_CASE(E_HANDLE);
        STRING_CASE(E_ABORT);
        STRING_CASE(E_FAIL);
        STRING_CASE(E_ACCESSDENIED);

        STRING_CASE(CLASS_E_NOAGGREGATION);

        STRING_CASE(CO_E_NOTINITIALIZED);
        STRING_CASE(CO_E_ALREADYINITIALIZED);
        STRING_CASE(CO_E_INIT_ONLY_SINGLE_THREADED);

        STRING_CASE(DV_E_DVASPECT);
        STRING_CASE(DV_E_LINDEX);
        STRING_CASE(DV_E_TYMED);
        STRING_CASE(DV_E_FORMATETC);

#ifdef __INTSHCUT_H__

        STRING_CASE(E_FLAGS);

        STRING_CASE(URL_E_INVALID_SYNTAX);
        STRING_CASE(URL_E_UNREGISTERED_PROTOCOL);

        STRING_CASE(IS_E_EXEC_FAILED);

        STRING_CASE(E_FILE_NOT_FOUND);
        STRING_CASE(E_PATH_NOT_FOUND);

#endif

    default:
        wsprintf(s_rgchHRESULT, TEXT("%#lx"), hr);
        pcsz = s_rgchHRESULT;
        break;
        }

    ASSERT(IS_VALID_STRING_PTR(pcsz, -1));

    return(pcsz);
}


#define STRING_RIID(val)               { &val, TEXT(#val) }

struct 
    {
    REFIID riid;
    LPCTSTR psz;
    } const c_mpriid[] = 
        {
        STRING_RIID(IID_IUnknown),
        STRING_RIID(IID_IEnumUnknown),
        STRING_RIID(IID_IShellBrowser),
        STRING_RIID(IID_IShellView),
        STRING_RIID(IID_IContextMenu),
        STRING_RIID(IID_IShellFolder),
        STRING_RIID(IID_IShellExtInit),
        STRING_RIID(IID_IShellPropSheetExt),
        STRING_RIID(IID_IPersistFolder),
        STRING_RIID(IID_IExtractIcon),
        STRING_RIID(IID_IShellLink),
        STRING_RIID(IID_IDataObject),
        STRING_RIID(IID_IContextMenu2),
        STRING_RIID(IID_INewShortcutHook),
        STRING_RIID(IID_IPersist),
        STRING_RIID(IID_IPersistStream),
        STRING_RIID(IID_IUniformResourceLocator),
        };


LPCTSTR
Dbg_GetREFIIDName(
    REFIID riid)
{
    int i;

    for (i = 0; i < ARRAYSIZE(c_mpriid); i++)
        {
        if (IsEqualIID(riid, c_mpriid[i].riid))
            return c_mpriid[i].psz;
        }

    return TEXT("Unknown REFIID");
}

#endif

#endif // DEBUG
