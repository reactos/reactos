#pragma once

#ifndef _CHSTRING_H
#define _CHSTRING_H

#include <windows.h>
#include <provexce.h>

struct CHStringData
{
    long nRefs;
    int nDataLength;
    int nAllocLength;

    WCHAR* data()
    {
        return (WCHAR*)(this+1);
    }
};

class CHString
{
public:
    CHString();
    CHString(WCHAR ch, int nRepeat = 1) throw (CHeap_Exception);
    CHString(LPCWSTR lpsz) throw (CHeap_Exception);
    CHString(LPCWSTR lpch, int nLength) throw (CHeap_Exception);
    CHString(LPCSTR lpsz) throw (CHeap_Exception);
    CHString(const CHString& stringSrc);
    CHString(const unsigned char* lpsz);
    ~CHString();

    BSTR AllocSysString() const throw (CHeap_Exception);
    int Collate(LPCWSTR lpsz) const;
    int Compare(LPCWSTR lpsz) const;
    int CompareNoCase(LPCWSTR lpsz) const;
    void Empty();
    int Find(WCHAR ch) const;
    int Find(LPCWSTR lpszSub) const;
    int FindOneOf(LPCWSTR lpszCharSet) const;
    void Format(UINT nFormatID, ...) throw (CHeap_Exception);
    void Format(LPCWSTR lpszFormat, ...) throw (CHeap_Exception);
    void FormatMessageW(UINT nFormatID, ...) throw (CHeap_Exception);
    void FormatMessageW(LPCWSTR lpszFormat, ...) throw (CHeap_Exception);
    void FormatV(LPCWSTR lpszFormat, va_list argList);
    void FreeExtra() throw (CHeap_Exception);
    int GetAllocLength() const;
    WCHAR GetAt(int nIndex) const;
    LPWSTR GetBuffer(int nMinBufLength) throw (CHeap_Exception);
    LPWSTR GetBufferSetLength(int nNewLength) throw (CHeap_Exception);
    int GetLength() const;
    BOOL IsEmpty() const;
    CHString Left(int nCount) const throw (CHeap_Exception);
    int LoadStringW(UINT nID) throw (CHeap_Exception);
    LPWSTR LockBuffer();
    void MakeLower() throw (CHeap_Exception);
    void MakeReverse() throw (CHeap_Exception);
    void MakeUpper() throw (CHeap_Exception);
    CHString Mid(int nFirst) const throw (CHeap_Exception);
    CHString Mid(int nFirst, int nCount) const throw (CHeap_Exception);
    void ReleaseBuffer(int nNewLength = -1) throw (CHeap_Exception);
    int ReverseFind(WCHAR ch) const;
    CHString Right(int nCount) const throw (CHeap_Exception);
    void SetAt(int nIndex, WCHAR ch) throw (CHeap_Exception);
    CHString SpanExcluding(LPCWSTR lpszCharSet) const throw (CHeap_Exception);
    CHString SpanIncluding(LPCWSTR lpszCharSet) const throw (CHeap_Exception);
    void TrimLeft() throw (CHeap_Exception);
    void TrimRight() throw (CHeap_Exception);
    void UnlockBuffer();

    const CHString& operator=(char ch) throw (CHeap_Exception);
    const CHString& operator=(WCHAR ch) throw (CHeap_Exception);
    const CHString& operator=(CHString *p) throw (CHeap_Exception);
    const CHString& operator=(LPCSTR lpsz) throw (CHeap_Exception);
    const CHString& operator=(LPCWSTR lpsz) throw (CHeap_Exception);
    const CHString& operator=(const CHString& stringSrc) throw (CHeap_Exception);
    const CHString& operator=(const unsigned char* lpsz) throw (CHeap_Exception);

    const CHString& operator+=(char ch) throw (CHeap_Exception);
    const CHString& operator+=(WCHAR ch) throw (CHeap_Exception);
    const CHString& operator+=(LPCWSTR lpsz) throw (CHeap_Exception);
    const CHString& operator+=(const CHString& string) throw (CHeap_Exception);

    WCHAR operator[](int nIndex) const;

    operator LPCWSTR() const;

    friend CHString WINAPI operator+(WCHAR ch, const CHString& string) throw (CHeap_Exception);
    friend CHString WINAPI operator+(const CHString& string, WCHAR ch) throw (CHeap_Exception);
    friend CHString WINAPI operator+(const CHString& string, LPCWSTR lpsz) throw (CHeap_Exception);
    friend CHString WINAPI operator+(LPCWSTR lpsz, const CHString& string) throw (CHeap_Exception);
    friend CHString WINAPI operator+(const CHString& string1, const CHString& string2) throw (CHeap_Exception);

protected:
    LPWSTR m_pchData;

    void AllocBeforeWrite(int nLen) throw (CHeap_Exception);
    void AllocBuffer(int nLen) throw (CHeap_Exception);
    void AllocCopy(CHString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const throw (CHeap_Exception);
    void AssignCopy(int nSrcLen, LPCWSTR lpszSrcData) throw (CHeap_Exception);
    void ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data, int nSrc2Len, LPCWSTR lpszSrc2Data) throw (CHeap_Exception);
    void ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData);
    void CopyBeforeWrite() throw (CHeap_Exception);
    CHStringData* GetData() const;
    void Init();
    int LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf) throw (CHeap_Exception);
    void Release();
    static void WINAPI Release(CHStringData* pData);
    static int WINAPI SafeStrlen(LPCWSTR lpsz);
};

inline BOOL operator==(const CHString& s1, LPCWSTR s2) { return s1.Compare(s2) == 0; }
inline BOOL operator==(const CHString& s1, const CHString& s2) { return s1.Compare(s2) == 0; }

inline BOOL operator!=(const CHString& s1, LPCWSTR s2) { return s1.Compare(s2) != 0; }
inline BOOL operator!=(const CHString& s1, const CHString& s2) { return s1.Compare(s2) != 0; }

inline BOOL operator<(const CHString& s1, LPCWSTR s2) { return s1.Compare(s2) < 0; }
inline BOOL operator<(const CHString& s1, const CHString& s2) { return s1.Compare(s2) < 0; }

inline BOOL operator>(const CHString& s1, LPCWSTR s2) { return s1.Compare(s2) > 0; }
inline BOOL operator>(const CHString& s1, const CHString& s2) { return s1.Compare(s2) > 0; }

inline BOOL operator<=(const CHString& s1, LPCWSTR s2) { return s1.Compare(s2) <= 0; }
inline BOOL operator<=(const CHString& s1, const CHString& s2) { return s1.Compare(s2) <= 0; }

inline BOOL operator>=(const CHString& s1, LPCWSTR s2) { return s1.Compare(s2) >= 0; }
inline BOOL operator>=(const CHString& s1, const CHString& s2) { return s1.Compare(s2) >= 0; }

#endif
