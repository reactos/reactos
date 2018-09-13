
#pragma pack(1)
typedef struct _IDNETRESOURCE   // idn
{
    WORD    cb;
    BYTE    bFlags;         // Display type in low nibble
    BYTE    uType;
    BYTE    uUsage;         // Usage in low nibble, More Flags in high nibble
    CHAR    szNetResName[1];
    // char szProvider[*] - If NET_HASPROVIDER bit is set
    // char szComment[*]  - If NET_HASCOMMENT bit is set.
    // WCHAR szNetResNameWide[*] - If NET_UNICODE bit it set.
    // WCHAR szProviderWide[*]   - If NET_UNICODE and NET_HASPROVIDER
    // WCHAR szCommentWide[*]    - If NET_UNICODE and NET_HASCOMMENT
} IDNETRESOURCE, *LPIDNETRESOURCE;
typedef const IDNETRESOURCE *LPCIDNETRESOURCE;
#pragma pack()

//===========================================================================
// CNetwork: Some private macro - but probably needed in .cpp file.
//===========================================================================
#define NET_DISPLAYNAMEOFFSET           ((UINT)((LPIDNETRESOURCE)0)->szNetResName)
#define NET_GetFlags(pidnRel)           ((pidnRel)->bFlags)
#define NET_GetDisplayType(pidnRel)     ((pidnRel)->bFlags & 0x0f)
#define NET_GetType(pidnRel)            ((pidnRel)->uType)
#define NET_GetUsage(pidnRel)           ((pidnRel)->uUsage & 0x0f)
#define NET_IsReg(pidnRel)              ((pidnRel)->bFlags == SHID_NET_REGITEM)
#define NET_IsJunction(pidnRel)         ((pidnRel)->bFlags & SHID_JUNCTION)
#define NET_IsRootReg(pidnRel)          ((pidnRel)->bFlags == SHID_NET_ROOTREGITEM)

// Define some Flags that are on high nibble of uUsage byte
#define NET_HASPROVIDER                 0x80    // Has own copy of provider
#define NET_HASCOMMENT                  0x40    // Has comment field in pidl
#define NET_REMOTEFLD                   0x20    // Is remote folder
#define NET_UNICODE                     0x10    // Has unicode names
#define NET_FHasComment(pidnRel)        ((pidnRel)->uUsage & NET_HASCOMMENT)
#define NET_FHasProvider(pidnRel)       ((pidnRel)->uUsage & NET_HASPROVIDER)
#define NET_IsRemoteFld(pidnRel)        ((pidnRel)->uUsage & NET_REMOTEFLD)
#define NET_IsUnicode(pidnRel)          ((pidnRel)->uUsage & NET_UNICODE)

STDMETHODIMP CNetwork_EnumSearches(IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum);


STDAPI_(IShellFolderViewCB*) Net_CreateSFVCB(IShellFolder* psf, UINT uDisplayType, LPCITEMIDLIST pidlMonitor, LONG lEvents);
STDAPI CNetwork_DFMCallBackBG(LPSHELLFOLDER psf, HWND hwndOwner,
                IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

STDAPI_(BOOL) NET_GetProviderKeyName(IShellFolder *psf, LPTSTR pszName, UINT uNameLen);
STDAPI_(BOOL) NET_IsRemoteRegItem(LPCITEMIDLIST pidl, REFCLSID rclsid, LPITEMIDLIST* ppidlRemainder);

// These are exported form netviewx.c they are wrappers around the same WNet
// APIs, but play with the parameters to make it easier to call.  They accept
// full paths rather than just drive letters.

STDAPI_(DWORD) SHWNetDisconnectDialog1 (LPDISCDLGSTRUCT lpConnDlgStruct);
STDAPI_(DWORD) SHWNetGetConnection (LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength);


