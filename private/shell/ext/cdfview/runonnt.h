#define ILGetDisplayName        _AorW_ILGetDisplayName
#define PathCleanupSpec         _AorW_PathCleanupSpec

// The following functions were originally only TCHAR versions
// in Win95, but now have A/W versions.  Since we still need to
// run on Win95, we need to treat them as TCHAR versions and
// undo the A/W #define

// Define the prototypes for each of these forwarders...

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

extern BOOL _AorW_ILGetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName);
extern int _AorW_PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec);

//
//  This is the "RunOn95" section, which thunks UNICODE functions
//  back down to ANSI so we can run on Win95 in browser-only mode.
//

STDAPI Priv_SHDefExtractIcon(LPCTSTR pszIconFile, int iIndex, UINT uFlags,
                          HICON *phiconLarge, HICON *phiconSmall,
                          UINT nIconSize);

#ifdef __cplusplus
}

#endif  /* __cplusplus */
