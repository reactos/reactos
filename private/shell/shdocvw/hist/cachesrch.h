/**********************************************************************
  Cache Search Stuff (simple, fast strstr)

  Marc Miller (t-marcmi) - 1998
 **********************************************************************/

#include "priv.h"

#ifndef __HISTORY_CACHE_SEARCH__
#define __HISTORY_CACHE_SEARCH__

#define UNICODE_SIGNATURE           0xFFFE
#define UNICODE_SIGNATURE_BACKWARDS 0xFEFF

namespace CacheSearchEngine {

    class IWideSequentialReadStream {
    public:
        virtual BOOL GetNextChar(WCHAR &wc) = 0;
    };
    
    class StreamSearcher {
    protected:
        LPWSTR _pwszPreparedSearchTarget;
        static inline    BOOL s_IsWhiteSpace(const WCHAR &wc) {
            return ((wc == L' ') || (wc == L'\t') || (wc == L'\n') || (wc == L'\r'));
        }
        static inline LPCWSTR s_SkipWhiteSpace(LPCWSTR pwszStr) {
            LPCWSTR pwszTemp = pwszStr;
            while(s_IsWhiteSpace(*pwszTemp))
                ++pwszTemp;
            return pwszTemp;
        }
        void _PrepareSearchTarget(LPCWSTR pwszSearchTarget);
    public:
        StreamSearcher(LPCWSTR pwszSearchTarget) { _PrepareSearchTarget(pwszSearchTarget); }
        ~StreamSearcher() { LocalFree(_pwszPreparedSearchTarget); }
        BOOL SearchCharStream(IWideSequentialReadStream &wsrs, BOOL fIsHTML = FALSE);
    };
    
    class StringStream : public IWideSequentialReadStream {
        BOOL    fCleanup; // the string we hold needs to be deallocated by us
        LPCWSTR pwszStr;
        UINT    uCurrentPos;
    public:
        StringStream(LPCWSTR pwszStr, BOOL fDuplicate = FALSE) : uCurrentPos(0), fCleanup(fDuplicate) {
            if (fDuplicate)
                SHStrDupW(pwszStr, const_cast<LPWSTR *>(&(this->pwszStr)));
            else
                this->pwszStr = pwszStr;
        }
        StringStream(LPCSTR  pszStr, BOOL fDuplicate = FALSE)  : uCurrentPos(0), fCleanup(TRUE) {
            SHStrDupA(pszStr, const_cast<LPWSTR *>(&(pwszStr)));
        }
        ~StringStream() {
            if (fCleanup)
                CoTaskMemFree(const_cast<LPWSTR>(pwszStr));
        }
        BOOL GetNextChar(WCHAR &wc) {
            wc = pwszStr[uCurrentPos];
            if (wc)
                ++uCurrentPos;
            return wc;
        }
    };
    
    class CacheStreamWrapper : public IWideSequentialReadStream {
    protected:
        HANDLE  _hCacheStream;
        DWORD   _dwCacheStreamLoc;  // our offset into the actual cache file
        BOOL    _fEndOfFile;
        
        // I can never remember which one is little endian and which is big endian
        enum DATATYPEENUM { UNICODE_DATA = 0, UNICODE_BACKWARDS_DATA, ASCII_DATA } _dataType;
        static inline BOOL s_IsUnicode(DATATYPEENUM dte) { return dte < ASCII_DATA; }
        static inline UINT s_Charsize (DATATYPEENUM dte) { return s_IsUnicode(dte) ? sizeof(USHORT) : sizeof(CHAR); }
        
        static DWORD s_dwPageSize;
        
        LPBYTE _pbBuff;      /* buffer of bytes which are type-neutral
                                _pbBuff is allocated with VirtualAlloc */
        LPBYTE _pbBuffPos;   // current position in buffer
        LPBYTE _pbBuffLast;  // last byte in buffer
        DWORD  _dwBuffSize;  // current valid buffer size (not allocated size)
        
        BOOL   _ReadNextBlock();
        BOOL   _GetNextByte(BYTE &b);
        
    public:
        CacheStreamWrapper(HANDLE hCacheStream);
        ~CacheStreamWrapper();
        BOOL GetNextChar(WCHAR &wc);
    };

    
    BOOL SearchCacheStream(StreamSearcher &cse, HANDLE hCacheStream, BOOL fIsHTML = FALSE);
}

#endif
