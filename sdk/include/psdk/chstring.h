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
    CHString(WCHAR ch, int nRepeat = 1);
    CHString(LPCWSTR lpsz);
    CHString(LPCWSTR lpch, int nLength);
    CHString(LPCSTR lpsz);
    CHString(const CHString& stringSrc);
    CHString(const unsigned char* lpsz);
    ~CHString();

    BSTR AllocSysString() const;
    int Collate(LPCWSTR lpsz) const;
    int Compare(LPCWSTR lpsz) const;
    int CompareNoCase(LPCWSTR lpsz) const;
    void Empty();
    int Find(WCHAR ch) const;
    int Find(LPCWSTR lpszSub) const;
    int FindOneOf(LPCWSTR lpszCharSet) const;
    void Format(UINT nFormatID, ...);
    void Format(LPCWSTR lpszFormat, ...);
    void FormatMessageW(UINT nFormatID, ...);
    void FormatMessageW(LPCWSTR lpszFormat, ...);
    void FormatV(LPCWSTR lpszFormat, va_list argList);
    void FreeExtra();
    int GetAllocLength() const;
    WCHAR GetAt(int nIndex) const;
    LPWSTR GetBuffer(int nMinBufLength);
    LPWSTR GetBufferSetLength(int nNewLength);
    int GetLength() const;
    BOOL IsEmpty() const;
    CHString Left(int nCount) const;
    int LoadStringW(UINT nID);
    LPWSTR LockBuffer();
    void MakeLower();
    void MakeReverse();
    void MakeUpper();
    CHString Mid(int nFirst) const;
    CHString Mid(int nFirst, int nCount) const;
    void ReleaseBuffer(int nNewLength = -1);
    int ReverseFind(WCHAR ch) const;
    CHString Right(int nCount) const;
    void SetAt(int nIndex, WCHAR ch);
    CHString SpanExcluding(LPCWSTR lpszCharSet) const;
    CHString SpanIncluding(LPCWSTR lpszCharSet) const;
    void TrimLeft();
    void TrimRight();
    void UnlockBuffer();

    const CHString& operator=(char ch);
    const CHString& operator=(WCHAR ch);
    const CHString& operator=(CHString *p);
    const CHString& operator=(LPCSTR lpsz);
    const CHString& operator=(LPCWSTR lpsz);
    const CHString& operator=(const CHString& stringSrc);
    const CHString& operator=(const unsigned char* lpsz);

    const CHString& operator+=(char ch);
    const CHString& operator+=(WCHAR ch);
    const CHString& operator+=(LPCWSTR lpsz);
    const CHString& operator+=(const CHString& string);

    WCHAR operator[](int nIndex) const;

    operator LPCWSTR() const;

    friend CHString WINAPI operator+(WCHAR ch, const CHString& string);
    friend CHString WINAPI operator+(const CHString& string, WCHAR ch);
    friend CHString WINAPI operator+(const CHString& string, LPCWSTR lpsz);
    friend CHString WINAPI operator+(LPCWSTR lpsz, const CHString& string);
    friend CHString WINAPI operator+(const CHString& string1, const CHString& string2);

protected:
    LPWSTR m_pchData;

    void AllocBeforeWrite(int nLen);
    void AllocBuffer(int nLen);
    void AllocCopy(CHString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AssignCopy(int nSrcLen, LPCWSTR lpszSrcData);
    void ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data, int nSrc2Len, LPCWSTR lpszSrc2Data);
    void ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData);
    void CopyBeforeWrite();
    CHStringData* GetData() const;
    void Init();
    int LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf);
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
