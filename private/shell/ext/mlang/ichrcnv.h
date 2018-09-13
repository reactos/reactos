
#ifndef ICHRCNV_H_
#define ICHRCNV_H_

#define MAXOVERFLOWCHARS 16


class CICharConverter
{
private:

    DWORD _dwWinCodePage;
    DWORD _dwInternetEncoding;
    DWORD _dwUTFEncoding;
    DWORD _dwUnicodeEncoding;
    DWORD _dwFlag;  
    WCHAR * _lpFallBack;

    BOOL  _bConvertDirt;

    LPSTR _lpUnicodeStr;
    LPSTR _lpInterm1Str;
    LPSTR _lpInterm2Str;

    HCINS _hcins;
    int   _hcins_dst;
    int   _cvt_count;


public:
	DWORD _dwConvertType;
	LPSTR _lpDstStr;
	LPSTR _lpSrcStr;
	int _nSrcSize;

	CICharConverter();
   	CICharConverter(DWORD dwFlag, WCHAR *lpFallBack);
	CICharConverter(DWORD dwSrcEncoding, DWORD dwDstEncoding);
	~CICharConverter();
	HRESULT ConvertSetup(DWORD dwSrcEncoding, DWORD dwDstEncoding);
	HRESULT DoCodeConvert(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
		 LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
	BOOL ConvertCleanUp();

private:
    HRESULT ConvertUUWI(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT ConvertIWUU(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT UnicodeToMultiByteEncoding(DWORD dwDstEncoding, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT UTF78ToUnicode(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize);
    HRESULT UnicodeToUTF78(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize);
    HRESULT UnicodeToWindowsCodePage(LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT UnicodeToInternetEncoding(LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT WindowsCodePageToUnicode(LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize);
    HRESULT InternetEncodingToUnicode(LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize);
    HRESULT WindowsCodePageToInternetEncoding(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT InternetEncodingToWindowsCodePage(LPDWORD lpdwMode, LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT WindowsCodePageToInternetEncodingWrap(LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);
    HRESULT InternetEncodingToWindowsCodePageWrap(LPCSTR lpSrcStr, LPINT lpnSrcSize,
        LPSTR lpDstStr, LPINT lpnDstSize, DWORD dwFlag, WCHAR *lpFallBack);

    HRESULT CreateINetString(BOOL fInbound, UINT uCodePage, int nCodeSet);
    HRESULT DoConvertINetString(LPDWORD lpdwMode, BOOL fInbound, UINT uCodePage, int nCodeSet,
      LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDestStr, int cchDest, LPINT lpnSize);

    HRESULT KSC5601ToEUCKR(LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDestStr, int cchDest, LPINT lpnSize);
protected:

};

HRESULT WINAPI _IStreamConvertINetString(CICharConverter * INetConvert, LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut, DWORD *pdwProperty);

#endif /* ICHRCNV_H_ */


