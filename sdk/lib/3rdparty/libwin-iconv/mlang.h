HRESULT WINAPI ConvertINetString(
    LPDWORD lpdwMode,
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding,
    LPCSTR lpSrcStr,
    LPINT lpnSrcSize,
    LPBYTE lpDstStr,
    LPINT lpnDstSize
);

HRESULT WINAPI ConvertINetMultiByteToUnicode(
    LPDWORD lpdwMode,
    DWORD dwSrcEncoding,
    LPCSTR lpSrcStr,
    LPINT lpnMultiCharCount,
    LPWSTR lpDstStr,
    LPINT lpnWideCharCount
);

HRESULT WINAPI ConvertINetUnicodeToMultiByte(
    LPDWORD lpdwMode,
    DWORD dwEncoding,
    LPCWSTR lpSrcStr,
    LPINT lpnWideCharCount,
    LPSTR lpDstStr,
    LPINT lpnMultiCharCount
);

HRESULT WINAPI IsConvertINetStringAvailable(
    DWORD dwSrcEncoding,
    DWORD dwDstEncoding
);

HRESULT WINAPI LcidToRfc1766A(
    LCID Locale,
    LPSTR pszRfc1766,
    int nChar
);

HRESULT WINAPI LcidToRfc1766W(
    LCID Locale,
    LPWSTR pszRfc1766,
    int nChar
);

HRESULT WINAPI Rfc1766ToLcidA(
    LCID *pLocale,
    LPSTR pszRfc1766
);

HRESULT WINAPI Rfc1766ToLcidW(
    LCID *pLocale,
    LPWSTR pszRfc1766
);
