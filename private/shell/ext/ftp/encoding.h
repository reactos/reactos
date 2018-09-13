/*****************************************************************************\
    FILE: encoding.h
    
    DESCRIPTION:
        Handle taking internet strings by detecting if they are UTF-8 encoded
    or DBCS and finding out what code page was used.
\*****************************************************************************/

#ifndef _STRENCODE_H
#define _STRENCODE_H

#include <mlang.h>


// Turned off until MLANG can successfully detect short strings.
// TODO: We also need to fix ftpfolder when it doesn't have a pidl
//       but still needs the site's CWireEncoding state
//#define FEATURE_CP_AUTODETECT

// FTP_FIND_DATA is different than WIN32_FIND_DATA because
// the .cFileName is in WIRECHAR instead of CHAR
#define FTP_FIND_DATA      WIN32_FIND_DATAA
#define LPFTP_FIND_DATA    LPWIN32_FIND_DATAA

// WIRESTR stands for WireBites which could be DBCS/MBCS or UTF-8
#define WIRECHAR      CHAR
#define LPCWIRESTR    LPCSTR
#define LPWIRESTR     LPSTR




BOOL SHIsUTF8Encoded(LPCWIRESTR pszIsUTF8);


/*****************************************************************************\
    CLASS: CMultiLanguageCache
    
    DESCRIPTION:
        We can't cache the IMultiLanguage2 * across threads, but we do need to
    cache it when we are in a loop because we don't want to keep calling
    CoCreateInstance.
\*****************************************************************************/
class CMultiLanguageCache
{
public:
    CMultiLanguageCache(void) {m_pml2 = NULL;};
    ~CMultiLanguageCache(void) {ATOMICRELEASE(m_pml2);};

    IMultiLanguage2 * GetIMultiLanguage2(void) {EVAL(SUCCEEDED(_Init())); return m_pml2;};

private:
    // Private member variables
    IMultiLanguage2 *       m_pml2;

    // Private member functions
    HRESULT _Init(void);
};


// dwFlags for WireBytesToUnicode() & UnicodeToWireBytes()
#define WIREENC_NONE                0x00000000  // None
#define WIREENC_USE_UTF8            0x00000001  // Prefer UTF-8 because this is a new file. For UnicodeToWireBytes() only.
#define WIREENC_IMPROVE_ACCURACY    0x00000002  // Detect the accuracy.  For WireBytesToUnicode() only.



#define DETECT_CONFIDENCE       75  // We want to be this confident.
/*****************************************************************************\
    CLASS: CWireEncoding
    
    DESCRIPTION:
    2.1.1 No Data Loss Support (UTF-8)
    Server: The server is required to support the FEAT FTP command (rfc2389 http://www.cis.ohio-state.edu/htbin/rfc/rfc2389.html) and the "utf8" feature (http://w3.hethmon.com/ftpext/drafts/draft-ietf-ftpext-intl-ftp-04.txt).   If the client sends the server the "utf8" command, the server then needs to accept  and return UTF-8 encoded filenames.  It's not known when IIS will support this but it won't be supported in the version that ships with Windows 2000.
    Network Client (wininet): Wininet needs to respect the unicode filepaths in the FtpGetFileEx() and FtpPutFileEx() APIs.  This won't be supported in IE 5.
     UI Client (msieftp): It's necessary to see if the server supports the "utf8" command via the FEAT command.  If the command is supported, it should be sent to the server and all future strings will be UTF-8 encoded.  This should be supported in IE 5 if there is enough time in the schedule.

    2.1.0 Data Loss Backward Compat (DBCS)
    MSIEFTP will only support DBCS if and only if the code page on the client matches the server's code page and all ftp directories and filenames used.  In future versions I may attempt to sniff the code page.

    IMultiLanguage2::DetectCodepage(MLDETECTCP_8BIT, 0, psz, NULL, &DetectEncodingInfo, ARRAYSIZE(DetectEncodingInfo))
    MLDETECTCP_8BIT, MLDETECTCP_DBCS, MLCONVCHARF_AUTODETECT
    DetectEncodingInfo.nCodePage (IMultiLanguage2::DetectCodepage)

    CP_1252: This is english/french/german and the most common.
    CP_JPN_SJ: Most common Japanese
    CP_CYRILLIC_AUTO = 51251L,
    CP_GREEK_AUTO   = 51253L,
    CP_ARABIC_AUTO  = 51256L,
    CP_1251         = 1251L: Lucian
\*****************************************************************************/
class CWireEncoding
{
public:
    CWireEncoding(void);
    ~CWireEncoding(void);

    HRESULT WireBytesToUnicode(CMultiLanguageCache * pmlc, LPCWIRESTR pwStr, DWORD dwFlags, LPWSTR pwzDest, DWORD cchSize);
    HRESULT UnicodeToWireBytes(CMultiLanguageCache * pmlc, LPCWSTR pwzStr, DWORD dwFlags, LPWIRESTR pwbDest, DWORD cchSize);

    HRESULT ReSetCodePages(CMultiLanguageCache * pmlc, CFtpPidlList * pFtpPidlList);
    HRESULT CreateFtpItemID(CMultiLanguageCache * pmlc, LPFTP_FIND_DATA pwfd, LPITEMIDLIST * ppidl);
    HRESULT ChangeFtpItemIDName(CMultiLanguageCache * pmlc, LPCITEMIDLIST pidlBefore, LPCWSTR pwzNewName, BOOL fUTF8, LPITEMIDLIST * ppidlAfter);
    UINT GetCodePage(void) {return m_uiCodePage;};
    INT GetConfidence(void) {return m_nConfidence;};

    BOOL IsUTF8Supported(void) {return m_fUseUTF8;};
    void SetUTF8Support(BOOL fIsUTF8Supported) {m_fUseUTF8 = fIsUTF8Supported;};

private:
    // Private member variables
    INT                     m_nConfidence;      // How accurate is our guess at m_uiCodePage.
    UINT                    m_uiCodePage;       // The code page we guess this to be.
    DWORD                   m_dwMode;           // State used by IMultiLanguage2's ::ConvertStringFromUnicode
    BOOL                    m_fUseUTF8;         // 

    // Private member functions
    void _ImproveAccuracy(CMultiLanguageCache * pmlc, LPCWIRESTR pwStr, BOOL fUpdateCP, UINT * puiCodePath);
};


#endif // _STRENCODE_H
