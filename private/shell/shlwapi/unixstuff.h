#ifdef UNICODE
#define WNetGetResourceInformation WNetGetResourceInformationW 
#else
#define WNetGetResourceInformation WNetGetResourceInformationA
#endif

#ifdef __cplusplus
PRIVATE HRESULT UnixWininetCopyUrlForParse(PSHSTRW pstrDst, LPCWSTR pszSrc);
#endif

