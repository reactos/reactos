
typedef HRESULT (STDAPICALLTYPE *PFNREADPROP)(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf);
    
typedef struct {
    const FMTID *pfmtid;    // FMTID_ for property set
    UINT idProp;            // property ID
    PFNREADPROP pfnRead;    // function to fetch the string representation
    UINT idFmtString;       // format string to use, should be "%1String %2"
} ITEM_PROP;

// standard PFNREADPROP callback types
STDAPI GetStringProp(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf);
STDAPI GetFileTimeProp(IPropertyStorage *ppropstg, PROPID propid, LPTSTR pszBuf, DWORD cchBuf);
STDAPI GetInfoTipFromStorage(IPropertySetStorage *ppropsetstg, const ITEM_PROP *pip, WCHAR **ppszTip);

