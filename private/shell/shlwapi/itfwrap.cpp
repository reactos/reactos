//
// Wrapper functions for shell interfaces
//
//  Many ISVs screw up various IShellFolder methods, so we centralize the
//  workarounds so everybody wins.
//
//  Someday, IExtractIcon and IShellLink wrappers may also be added, should
//  the need arise.
//

#include "priv.h"
#include <shlobj.h>

//----------------------------------------------------------------------------
//
//  IShellFolder::GetDisplayNameOf was not very well documented.  Lots of
//  people don't realize that the SHGDN values are flags, so they use
//  equality tests instead of bit tests.  So whenever we add a new flag,
//  these people say "Huh?  I don't understand."  So we have to keep
//  retrying with fewer and fewer flags until finally they get something
//  they like.  SHGDN_FORPARSING has the opposite problem:  Some people
//  demand that the flag be set.

//
//  This array lists the things we try to do to get the uFlags into a state
//  that the app will eventually like.
//
//  We walk through the list and do this:
//
//      uFlags = (uFlags & AND) | OR
//
//  Most of the time, the entry will turn off a bit in the uFlags, but
//  SHGDN_FORPARSING is weird and it's a flag you actually want to turn on
//  instead of off.
//

typedef struct GDNCOMPAT {
    DWORD   dwAnd;
    DWORD   dwOr;
    DWORD   dwAllow;                    // flag to allow this rule to fire
} GDNCOMPAT;

#define GDNADDFLAG(f)   ~0, f           // Add a flag to uFlags
#define GDNDELFLAG(f)   ~f, 0           // Remove a flag from uFlags

#define ISHGDN2_CANREMOVEOTHERFLAGS 0x80000000

GDNCOMPAT c_gdnc[] = {
  { GDNDELFLAG(SHGDN_FOREDITING),       ISHGDN2_CANREMOVEOTHERFLAGS },  // Some apps don't like this flag
  { GDNDELFLAG(SHGDN_FORADDRESSBAR),    ISHGDN2_CANREMOVEOTHERFLAGS },  // Some apps don't like this flag
  { GDNADDFLAG(SHGDN_FORPARSING),       ISHGDN2_CANREMOVEOTHERFLAGS },  // Some apps require this flag
  { GDNDELFLAG(SHGDN_FORPARSING),       ISHGDN2_CANREMOVEFORPARSING },  // And others don't like it
  { GDNDELFLAG(SHGDN_INFOLDER),         ISHGDN2_CANREMOVEOTHERFLAGS },  // Desperation - remove this flag too
};

//
//  These are the return values we tend to get back when people see
//  flags they don't like.
//
BOOL __inline IsBogusHRESULT(HRESULT hres)
{
    return  hres == E_FAIL ||
            hres == E_INVALIDARG ||
            hres == E_NOTIMPL;
}

//
//  dwFlags2 controls how aggressively we try to find a working display name.
//
//  ISHGDN2_CANREMOVEFORPARSING
//      Normally, we do not turn off the SHGDN_FORPARSING flag because
//      if a caller asks for the parse name, it probably really wants the
//      parse name.  This flag indicates that we are allowed to turn off
//      SHGDN_FORPARSING if we think it'll help.
//

STDAPI IShellFolder_GetDisplayNameOf(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    DWORD uFlags,
    LPSTRRET lpName,
    DWORD dwFlags2)
{
    HRESULT hres;

    hres = psf->GetDisplayNameOf(pidl, uFlags, lpName);
    if (!IsBogusHRESULT(hres))
        return hres;

    int i;
    DWORD uFlagsOrig = uFlags;

    //
    //  If the caller didn't pass SHGDN_FORPARSING, then clearly it's
    //  safe to remove it.
    //
    if (!(uFlags & SHGDN_FORPARSING)) {
        dwFlags2 |= ISHGDN2_CANREMOVEFORPARSING;
    }

    // We can always remove other flags.
    dwFlags2 |= ISHGDN2_CANREMOVEOTHERFLAGS;

    for (i = 0; i < ARRAYSIZE(c_gdnc); i++)
    {
        if (c_gdnc[i].dwAllow & dwFlags2)
        {
            DWORD uFlagsNew = (uFlags & c_gdnc[i].dwAnd) | c_gdnc[i].dwOr;
            if (uFlagsNew != uFlags)
            {
                uFlags = uFlagsNew;
                hres = psf->GetDisplayNameOf(pidl, uFlags, lpName);
                if (!IsBogusHRESULT(hres))
                    return hres;
            }
        }
    }

    // By now, we should've removed all the flags, except perhaps for
    // SHGDN_FORPARSING.
    if (dwFlags2 & ISHGDN2_CANREMOVEFORPARSING) {
        ASSERT(uFlags == SHGDN_NORMAL);
    } else {
        ASSERT(uFlags == SHGDN_NORMAL || uFlags == SHGDN_FORPARSING);
    }

    return hres;
}

//----------------------------------------------------------------------------
//
//  The documentation on IShellFolder::ParseDisplayName wasn't clear that
//  pchEaten and pdwAttributes can be NULL, and some people dereference
//  them unconditionally.  So make sure it's safe to dereference them.
//
//  It is also popular to forget to set *ppidl=NULL on failure, so we null
//  it out here.
//
//  We request no attributes, so people who aren't buggy won't go out of
//  their way trying to retrieve expensive attributes.
//

STDAPI IShellFolder_ParseDisplayName(
    IShellFolder *psf,
    HWND hwnd,
    LPBC pbc,
    LPOLESTR pszDisplayName,
    ULONG *pchEaten,
    LPITEMIDLIST *ppidl,
    ULONG *pdwAttributes)
{
    ULONG cchEaten;
    ULONG dwAttributes = 0;

    if (pchEaten == NULL)
        pchEaten = &cchEaten;
    if (pdwAttributes == NULL)
        pdwAttributes = &dwAttributes;

    if (ppidl)
        *ppidl = NULL;

    return psf->ParseDisplayName(hwnd, pbc, pszDisplayName, pchEaten, ppidl, pdwAttributes);
}

//----------------------------------------------------------------------------
//
//  IShellFolder::EnumObjects
//
CLSID CLSID_ZipFolder =
{ 0xe88dcce0, 0xb7b3, 0x11d1, { 0xa9, 0xf0, 0x00, 0xaa, 0x00, 0x60, 0xfa, 0x31 } };

STDAPI IShellFolder_EnumObjects(
    IShellFolder *psf,
    HWND hwnd,
    DWORD grfFlags,
    IEnumIDList **ppenumIDList)
{
    if (hwnd == NULL || hwnd == GetDesktopWindow())
    {
        //  The first parameter to EnumObjects is supposed to be the window
        //  on which to parent UI, or NULL for no UI, or GetDesktopWindow()
        //  for "parentless UI".
        //
        //  Win98 Plus! Zip Folders takes the hwnd and uses it as the basis
        //  for a search for a rebar window, since they (for some bizarre
        //  reason) want to hide the address bar when an enumeration starts.
        //
        //  We used to pass NULL or GetDesktopWindow(), but this caused zip
        //  folders to start searching from the desktop, which means that
        //  it eventually finds the taskbar and tries to send it
        //  inter-process rebar messages, which causes the shell to fault.
        //
        //  When we discover we are about to pass NULL to Zip Folders,
        //  we change it to HWND_BOTTOM.  This is not a valid window handle,
        //  which causes Zip Folders' search to bail out quickly and it ends
        //  up not killing anyone.
        //

        CLSID clsid;
        if (SUCCEEDED(IUnknown_GetClassID(psf, &clsid)) &&
            IsEqualCLSID(clsid, CLSID_ZipFolder))
            hwnd = HWND_BOTTOM;
    }

    return psf->EnumObjects(hwnd, grfFlags, ppenumIDList);
}
