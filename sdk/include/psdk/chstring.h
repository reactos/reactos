#pragma once

#ifndef _CHSTRING_H
#define _CHSTRING_H

#include <windows.h>
#include <provexce.h>

/*
 * Framedyn dates from ancient times when wchar_t was a typedef to unsigned short.
 * In order be able to use newer compilers, we define inline overloaded wrappers.
 */
using CHSTRING_WCHAR=unsigned short ;
using CHSTRING_LPCWSTR=const CHSTRING_WCHAR*;
using CHSTRING_LPWSTR=CHSTRING_WCHAR*;
static_assert(sizeof(CHSTRING_WCHAR) == sizeof(wchar_t), "CHSTRING_WCHAR must be of same size than wchar_t");


struct CHStringData
{
    long nRefs;
    int nDataLength;
    int nAllocLength;

    CHSTRING_WCHAR* data()
    {
        return (CHSTRING_WCHAR*)(this+1);
    }
};

class CHString
{
public:
    CHString();
    CHString(CHSTRING_WCHAR ch, int nRepeat = 1);
    CHString(CHSTRING_LPCWSTR lpsz);
    CHString(CHSTRING_LPCWSTR lpch, int nLength);
    CHString(LPCSTR lpsz);
    CHString(const CHString& stringSrc);
    CHString(const unsigned char* lpsz);
    ~CHString();

    CHSTRING_LPWSTR AllocSysString() const;
    int Collate(CHSTRING_LPCWSTR lpsz) const;
    int Collate(const wchar_t* lpsz) const
    {
        return Collate(reinterpret_cast<CHSTRING_LPCWSTR>(lpsz));
    }
    int Compare(CHSTRING_LPCWSTR lpsz) const;
    int Compare(const wchar_t* lpsz) const
    {
        return Compare(reinterpret_cast<CHSTRING_LPCWSTR>(lpsz));
    }
    int CompareNoCase(CHSTRING_LPCWSTR lpsz) const;
    int CompareNoCase(const wchar_t* lpsz) const
    {
        return CompareNoCase(reinterpret_cast<CHSTRING_LPCWSTR>(lpsz));
    }
    void Empty();
    int Find(CHSTRING_WCHAR ch) const;
    int Find(CHSTRING_LPCWSTR lpszSub) const;
    int Find(const wchar_t*  lpszSub) const
    {
        return FindOneOf(reinterpret_cast<CHSTRING_LPCWSTR>(lpszSub));
    }
    int FindOneOf(CHSTRING_LPCWSTR lpszCharSet) const;
    int FindOneOf(const wchar_t*  lpszCharSet) const
    {
        return FindOneOf(reinterpret_cast<CHSTRING_LPCWSTR>(lpszCharSet));
    }
    void Format(UINT nFormatID, ...);
    void Format(CHSTRING_LPCWSTR lpszFormat, ...);
    template <typename ...Params>
    void Format(const wchar_t* lpszFormat, Params&&... params)
    {
        Format(reinterpret_cast<CHSTRING_LPCWSTR>(lpszFormat), params...);
    }
    void FormatMessageW(UINT nFormatID, ...);
    void FormatMessageW(CHSTRING_LPCWSTR lpszFormat, ...);
    template <typename ...Params>
    void FormatMessageW(const wchar_t* lpszFormat, Params&&... params)
    {
        FormatMessageW(reinterpret_cast<CHSTRING_LPCWSTR>(lpszFormat), params...);
    }
    void FormatV(CHSTRING_LPCWSTR lpszFormat, va_list argList);
    void FormatV(const wchar_t* lpszFormat, va_list argList)
    {
        return FormatV(reinterpret_cast<CHSTRING_LPCWSTR>(lpszFormat), argList);
    }
    void FreeExtra();
    int GetAllocLength() const;
    CHSTRING_WCHAR GetAt(int nIndex) const;
    CHSTRING_LPWSTR GetBuffer(int nMinBufLength);
    CHSTRING_LPWSTR GetBufferSetLength(int nNewLength);
    int GetLength() const;
    BOOL IsEmpty() const;
    CHString Left(int nCount) const;
    int LoadStringW(UINT nID);
    CHSTRING_LPWSTR LockBuffer();
    void MakeLower();
    void MakeReverse();
    void MakeUpper();
    CHString Mid(int nFirst) const;
    CHString Mid(int nFirst, int nCount) const;
    void ReleaseBuffer(int nNewLength = -1);
    int ReverseFind(CHSTRING_WCHAR ch) const;
    CHString Right(int nCount) const;
    void SetAt(int nIndex, CHSTRING_WCHAR ch);
    CHString SpanExcluding(CHSTRING_LPCWSTR lpszCharSet) const;
    CHString SpanExcluding(const wchar_t* lpszCharSet) const
    {
        return SpanExcluding(reinterpret_cast<CHSTRING_LPCWSTR>(lpszCharSet));
    }
    CHString SpanIncluding(CHSTRING_LPCWSTR lpszCharSet) const;
    CHString SpanIncluding(const wchar_t* lpszCharSet) const
    {
        return SpanIncluding(reinterpret_cast<CHSTRING_LPCWSTR>(lpszCharSet));
    }
    void TrimLeft();
    void TrimRight();
    void UnlockBuffer();

    const CHString& operator=(char ch);
    const CHString& operator=(CHSTRING_WCHAR ch);
    const CHString& operator=(CHString *p);
    const CHString& operator=(LPCSTR lpsz);
    const CHString& operator=(CHSTRING_LPCWSTR lpsz);
    const CHString& operator=(const CHString& stringSrc);
    const CHString& operator=(const unsigned char* lpsz);

    const CHString& operator+=(char ch);
    const CHString& operator+=(CHSTRING_WCHAR ch);
    const CHString& operator+=(CHSTRING_LPCWSTR lpsz);
    const CHString& operator+=(const CHString& string);

    CHSTRING_WCHAR operator[](int nIndex) const;

    operator CHSTRING_LPCWSTR() const;

    friend CHString WINAPI operator+(CHSTRING_WCHAR ch, const CHString& string);
    friend CHString WINAPI operator+(const CHString& string, CHSTRING_WCHAR ch);
    friend CHString WINAPI operator+(const CHString& string, CHSTRING_LPCWSTR lpsz);
    friend CHString WINAPI operator+(CHSTRING_LPCWSTR lpsz, const CHString& string);
    friend CHString WINAPI operator+(const CHString& string1, const CHString& string2);

protected:
    CHSTRING_LPWSTR m_pchData;

    void AllocBeforeWrite(int nLen);
    void AllocBuffer(int nLen);
    void AllocCopy(CHString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AssignCopy(int nSrcLen, CHSTRING_LPCWSTR lpszSrcData);
    void AssignCopy(int nSrcLen, const wchar_t* lpszSrcData)
    {
        AssignCopy(nSrcLen, reinterpret_cast<CHSTRING_LPCWSTR>(lpszSrcData));
    }
    void ConcatCopy(int nSrc1Len, CHSTRING_LPCWSTR lpszSrc1Data, int nSrc2Len, CHSTRING_LPCWSTR lpszSrc2Data);
    void ConcatCopy(int nSrc1Len, const wchar_t* lpszSrc1Data, int nSrc2Len, const wchar_t* lpszSrc2Data)
    {
        ConcatCopy(nSrc1Len, reinterpret_cast<CHSTRING_LPCWSTR>(lpszSrc1Data), nSrc2Len, reinterpret_cast<CHSTRING_LPCWSTR>(lpszSrc2Data));
    }
    void ConcatInPlace(int nSrcLen, CHSTRING_LPCWSTR lpszSrcData);
    void ConcatInPlace(int nSrcLen, const wchar_t* lpszSrcData)
    {
        ConcatInPlace(nSrcLen, reinterpret_cast<CHSTRING_LPCWSTR>(lpszSrcData));
    }
    void CopyBeforeWrite();
    CHStringData* GetData() const;
    void Init();
    int LoadStringW(UINT nID, CHSTRING_LPWSTR lpszBuf, UINT nMaxBuf);
    void Release();
    static void WINAPI Release(CHStringData* pData);
    static int WINAPI SafeStrlen(CHSTRING_LPCWSTR lpsz);
    static int WINAPI SafeStrlen(const wchar_t* lpsz)
    {
        return SafeStrlen(reinterpret_cast<CHSTRING_LPCWSTR>(lpsz));
    }
};

inline BOOL operator==(const CHString& s1, CHSTRING_LPCWSTR s2) { return s1.Compare(s2) == 0; }
inline BOOL operator==(const CHString& s1, const CHString& s2) { return s1.Compare(s2) == 0; }

inline BOOL operator!=(const CHString& s1, CHSTRING_LPCWSTR s2) { return s1.Compare(s2) != 0; }
inline BOOL operator!=(const CHString& s1, const CHString& s2) { return s1.Compare(s2) != 0; }

inline BOOL operator<(const CHString& s1, CHSTRING_LPCWSTR s2) { return s1.Compare(s2) < 0; }
inline BOOL operator<(const CHString& s1, const CHString& s2) { return s1.Compare(s2) < 0; }

inline BOOL operator>(const CHString& s1, CHSTRING_LPCWSTR s2) { return s1.Compare(s2) > 0; }
inline BOOL operator>(const CHString& s1, const CHString& s2) { return s1.Compare(s2) > 0; }

inline BOOL operator<=(const CHString& s1, CHSTRING_LPCWSTR s2) { return s1.Compare(s2) <= 0; }
inline BOOL operator<=(const CHString& s1, const CHString& s2) { return s1.Compare(s2) <= 0; }

inline BOOL operator>=(const CHString& s1, CHSTRING_LPCWSTR s2) { return s1.Compare(s2) >= 0; }
inline BOOL operator>=(const CHString& s1, const CHString& s2) { return s1.Compare(s2) >= 0; }

/* Have GCC link to the symbols exported by framedyn.dll */
#ifdef __GNUC__

#define DEFINE_FRAMEDYN_ALIAS(alias, orig) __asm__(".set " #alias ", \"" #orig "\"");

#ifdef _M_IX86
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString16AllocBeforeWriteEi, ?AllocBeforeWrite@CHString@@IAEXH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString11AllocBufferEi, ?AllocBuffer@CHString@@IAEXH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString9AllocCopyERS_iii, ?AllocCopy@CHString@@IBEXAAV1@HHH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString14AllocSysStringEv, ?AllocSysString@CHString@@QBEPAGXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString10AssignCopyEiPKt, ?AssignCopy@CHString@@IAEXHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1ERKS_, ??0CHString@@QAE@ABV0@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1EPKc, ??0CHString@@QAE@PBD@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1EPKh, ??0CHString@@QAE@PBE@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1EPKt, ??0CHString@@QAE@PBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1EPKti, ??0CHString@@QAE@PBGH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1Eti, ??0CHString@@QAE@GH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC1Ev, ??0CHString@@QAE@XZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringC2Ev, ??0CHString@@QAE@XZ) // CHString::CHString(void)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString7CollateEPKt, ?Collate@CHString@@QBEHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString7CompareEPKt, ?Compare@CHString@@QBEHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString13CompareNoCaseEPKt, ?CompareNoCase@CHString@@QBEHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString10ConcatCopyEiPKtiS1_, ?ConcatCopy@CHString@@IAEXHPBGH0@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString13ConcatInPlaceEiPKt, ?ConcatInPlace@CHString@@IAEXHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString15CopyBeforeWriteEv, ?CopyBeforeWrite@CHString@@IAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString5EmptyEv, ?Empty@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString4FindEPKt, ?Find@CHString@@QBEHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString4FindEt, ?Find@CHString@@QBEHG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString9FindOneOfEPKt, ?FindOneOf@CHString@@QBEHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString6FormatEjz, ?Format@CHString@@QAAXIZZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString6FormatEPKtz, ?Format@CHString@@QAAXPBGZZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString14FormatMessageWEjz, ?FormatMessageW@CHString@@QAAXIZZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString14FormatMessageWEPKtz, ?FormatMessageW@CHString@@QAAXPBGZZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString7FormatVEPKtPc, ?FormatV@CHString@@QAEXPBGPAD@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString9FreeExtraEv, ?FreeExtra@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString14GetAllocLengthEv, ?GetAllocLength@CHString@@QBEHXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString5GetAtEi, ?GetAt@CHString@@QBEGH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString9GetBufferEi, ?GetBuffer@CHString@@QAEPAGH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString18GetBufferSetLengthEi, ?GetBufferSetLength@CHString@@QAEPAGH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString7GetDataEv, ?GetData@CHString@@IBEPAUCHStringData@@XZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString9GetLengthEv, ?GetLength@CHString@@QBEHXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString4InitEv, ?Init@CHString@@IAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString7IsEmptyEv, ?IsEmpty@CHString@@QBEHXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString4LeftEi, ?Left@CHString@@QBE?AV1@H@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString11LoadStringWEj, ?LoadStringW@CHString@@QAEHI@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString11LoadStringWEjPtj, ?LoadStringW@CHString@@IAEHIPAGI@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString10LockBufferEv, ?LockBuffer@CHString@@QAEPAGXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString9MakeLowerEv, ?MakeLower@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString11MakeReverseEv, ?MakeReverse@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString9MakeUpperEv, ?MakeUpper@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString3MidEi, ?Mid@CHString@@QBE?AV1@H@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString3MidEii, ?Mid@CHString@@QBE?AV1@HH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString7ReleaseEP12CHStringData@4, ?Release@CHString@@KGXPAUCHStringData@@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString7ReleaseEv, ?Release@CHString@@IAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString13ReleaseBufferEi, ?ReleaseBuffer@CHString@@QAEXH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString11ReverseFindEt, ?ReverseFind@CHString@@QBEHG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString5RightEi, ?Right@CHString@@QBE?AV1@H@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString10SafeStrlenEPKt@4, ?SafeStrlen@CHString@@KGHPBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString5SetAtEit, ?SetAt@CHString@@QAEXHG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString13SpanExcludingEPKt, ?SpanExcluding@CHString@@QBE?AV1@PBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHString13SpanIncludingEPKt, ?SpanIncluding@CHString@@QBE?AV1@PBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString8TrimLeftEv, ?TrimLeft@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString9TrimRightEv, ?TrimRight@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHString12UnlockBufferEv, ?UnlockBuffer@CHString@@QAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHStringcvPKwEv, ??BCHString@@QBEPBGXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringpLERKS_, ??YCHString@@QAEABV0@ABV0@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringpLEc, ??YCHString@@QAEABV0@D@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringpLEPKw, ??YCHString@@QAEABV0@PBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringpLEw, ??YCHString@@QAEABV0@G@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSEPS_, ??4CHString@@QAEABV0@PAV0@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSERKS_, ??4CHString@@QAEABV0@ABV0@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSEPKc, ??4CHString@@QAEABV0@PBD@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSEc, ??4CHString@@QAEABV0@D@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSEPKh, ??4CHString@@QAEABV0@PBE@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSEPKw, ??4CHString@@QAEABV0@PBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringaSEw, ??4CHString@@QAEABV0@G@Z)
DEFINE_FRAMEDYN_ALIAS(__ZNK8CHStringixEi, ??ACHString@@QBEGH@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringD1Ev, ??1CHString@@QAE@XZ) // CHString::~CHString() complete object destructor
DEFINE_FRAMEDYN_ALIAS(__ZN8CHStringD2Ev, ??1CHString@@QAE@XZ) // CHString::~CHString() base object destructor
DEFINE_FRAMEDYN_ALIAS(__ZplwRK8CHString, ??H@YG?AVCHString@@GABV0@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZplRK8CHStringw, ??H@YG?AVCHString@@ABV0@G@Z)
DEFINE_FRAMEDYN_ALIAS(__ZplRK8CHStringPKw, ??H@YG?AVCHString@@ABV0@PBG@Z)
DEFINE_FRAMEDYN_ALIAS(__ZplPKwRK8CHString, ??H@YG?AVCHString@@PBGABV0@@Z)
DEFINE_FRAMEDYN_ALIAS(__ZplRK8CHStringS1_, ??H@YG?AVCHString@@ABV0@0@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8Provider5FlushEv, ?Flush@Provider@@MAEXXZ)
DEFINE_FRAMEDYN_ALIAS(__ZN8Provider21ValidateDeletionFlagsEl, ?ValidateDeletionFlags@Provider@@MAEJJ@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8Provider19ValidateMethodFlagsEl, ?ValidateMethodFlags@Provider@@MAEJJ@Z)
DEFINE_FRAMEDYN_ALIAS(__ZN8Provider18ValidateQueryFlagsEl, ?ValidateQueryFlags@Provider@@MAEJJ@Z)
#elif defined(_M_AMD64)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString16AllocBeforeWriteEi, ?AllocBeforeWrite@CHString@@IEAAXH@Z) // protected: void __thiscall CHString::AllocBeforeWrite(int)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString11AllocBufferEi, ?AllocBuffer@CHString@@IEAAXH@Z) // protected: void __thiscall CHString::AllocBuffer(int)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString9AllocCopyERS_iii, ?AllocCopy@CHString@@IEBAXAEAV1@HHH@Z) // protected: void __thiscall CHString::AllocCopy(class CHString &,int,int,int)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString14AllocSysStringEv, ?AllocSysString@CHString@@QEBAPEAGXZ)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString10AssignCopyEiPKt, ?AssignCopy@CHString@@IEAAXHPEBG@Z)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1ERKS_, ??0CHString@@QEAA@AEBV0@@Z) // CHString::CHString(CHString const&)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1EPKc, ??0CHString@@QEAA@PEBD@Z) // CHString::CHString(char const*)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1EPKh, ??0CHString@@QEAA@PEBE@Z) // CHString::CHString(unsigned char const*)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1EPKt, ??0CHString@@QEAA@PEBG@Z) // CHString::CHString(unsigned short const*)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1EPKti, ??0CHString@@QEAA@PEBGH@Z) // CHString::CHString(unsigned short const*, int)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1Eti, ??0CHString@@QEAA@GH@Z) // CHString::CHString(unsigned short, int)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC1Ev, ??0CHString@@QEAA@XZ) // public: __thiscall CHString::CHString(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringC2Ev, ??0CHString@@QEAA@XZ) // public: __thiscall CHString::CHString(void)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString7CollateEPKt, ?Collate@CHString@@QEBAHPEBG@Z) // public: int __thiscall CHString::Collate(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString7CompareEPKt, ?Compare@CHString@@QEBAHPEBG@Z) // public: int __thiscall CHString::Compare(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString13CompareNoCaseEPKt, ?CompareNoCase@CHString@@QEBAHPEBG@Z) // public: int __thiscall CHString::CompareNoCase(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString10ConcatCopyEiPKtiS1_, ?ConcatCopy@CHString@@IEAAXHPEBGH0@Z) // protected: void __thiscall CHString::ConcatCopy(int,unsigned short const *,int,unsigned short const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString13ConcatInPlaceEiPKt, ?ConcatInPlace@CHString@@IEAAXHPEBG@Z) // protected: void __thiscall CHString::ConcatInPlace(int,unsigned short const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString15CopyBeforeWriteEv, ?CopyBeforeWrite@CHString@@IEAAXXZ) // protected: void __thiscall CHString::CopyBeforeWrite(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString5EmptyEv, ?Empty@CHString@@QEAAXXZ) // public: void __thiscall CHString::Empty(void)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString4FindEPKt, ?Find@CHString@@QEBAHPEBG@Z) // public: int __thiscall CHString::Find(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString4FindEt, ?Find@CHString@@QEBAHG@Z) // public: int __thiscall CHString::Find(unsigned short)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString9FindOneOfEPKt, ?FindOneOf@CHString@@QEBAHPEBG@Z) // public: int __thiscall CHString::FindOneOf(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString6FormatEjz, ?Format@CHString@@QEAAXIZZ) // public: void __cdecl CHString::Format(unsigned int,...)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString6FormatEPKtz, ?Format@CHString@@QEAAXPEBGZZ) // public: void __cdecl CHString::Format(unsigned short const *,...)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString14FormatMessageWEjz, ?FormatMessageW@CHString@@QEAAXIZZ) // public: void __cdecl CHString::FormatMessageW(unsigned int,...)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString14FormatMessageWEPKtz, ?FormatMessageW@CHString@@QEAAXPEBGZZ) // public: void __cdecl CHString::FormatMessageW(unsigned short const *,...)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString7FormatVEPKtPc, ?FormatV@CHString@@QEAAXPEBGPEAD@Z) // public: void __thiscall CHString::FormatV(unsigned short const *,char *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString9FreeExtraEv, ?FreeExtra@CHString@@QEAAXXZ) // public: void __thiscall CHString::FreeExtra(void)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString14GetAllocLengthEv, ?GetAllocLength@CHString@@QEBAHXZ) // public: int __thiscall CHString::GetAllocLength(void)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString5GetAtEi, ?GetAt@CHString@@QEBAGH@Z) // public: unsigned short __thiscall CHString::GetAt(int)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString9GetBufferEi, ?GetBuffer@CHString@@QEAAPEAGH@Z) // public: unsigned short * __thiscall CHString::GetBuffer(int)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString18GetBufferSetLengthEi, ?GetBufferSetLength@CHString@@QEAAPEAGH@Z) // public: unsigned short * __thiscall CHString::GetBufferSetLength(int)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString7GetDataEv, ?GetData@CHString@@IEBAPEAUCHStringData@@XZ) // protected: struct CHStringData * __thiscall CHString::GetData(void)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString9GetLengthEv, ?GetLength@CHString@@QEBAHXZ) // public: int __thiscall CHString::GetLength(void)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString4InitEv, ?Init@CHString@@IEAAXXZ) // protected: void __thiscall CHString::Init(void)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString7IsEmptyEv, ?IsEmpty@CHString@@QEBAHXZ) // public: int __thiscall CHString::IsEmpty(void)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString4LeftEi, ?Left@CHString@@QEBA?AV1@H@Z) // public: class CHString __thiscall CHString::Left(int)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString11LoadStringWEj, ?LoadStringW@CHString@@QEAAHI@Z) // public: int __thiscall CHString::LoadStringW(unsigned int)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString11LoadStringWEjPtj, ?LoadStringW@CHString@@IEAAHIPEAGI@Z) // protected: int __thiscall CHString::LoadStringW(unsigned int,unsigned short *,unsigned int)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString10LockBufferEv, ?LockBuffer@CHString@@QEAAPEAGXZ) // public: unsigned short * __thiscall CHString::LockBuffer(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString9MakeLowerEv, ?MakeLower@CHString@@QEAAXXZ) // public: void __thiscall CHString::MakeLower(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString11MakeReverseEv, ?MakeReverse@CHString@@QEAAXXZ) // public: void __thiscall CHString::MakeReverse(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString9MakeUpperEv, ?MakeUpper@CHString@@QEAAXXZ) // public: void __thiscall CHString::MakeUpper(void)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString3MidEi, ?Mid@CHString@@QEBA?AV1@H@Z) // public: class CHString __thiscall CHString::Mid(int)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString3MidEii, ?Mid@CHString@@QEBA?AV1@HH@Z) // public: class CHString __thiscall CHString::Mid(int,int)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString7ReleaseEP12CHStringData, ?Release@CHString@@KAXPEAUCHStringData@@@Z) // protected: static void __stdcall CHString::Release(struct CHStringData *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString7ReleaseEv, ?Release@CHString@@IEAAXXZ) // protected: void __thiscall CHString::Release(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString13ReleaseBufferEi, ?ReleaseBuffer@CHString@@QEAAXH@Z) // public: void __thiscall CHString::ReleaseBuffer(int)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString11ReverseFindEt, ?ReverseFind@CHString@@QEBAHG@Z) // public: int __thiscall CHString::ReverseFind(unsigned short)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString5RightEi, ?Right@CHString@@QEBA?AV1@H@Z) // public: class CHString __thiscall CHString::Right(int)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString10SafeStrlenEPKt, ?SafeStrlen@CHString@@KAHPEBG@Z) // protected: static int__stdcall CHString::SafeStrlen(unsigned short const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString5SetAtEit, ?SetAt@CHString@@QEAAXHG@Z) // public: void __thiscall CHString::SetAt(int,unsigned short)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString13SpanExcludingEPKt, ?SpanExcluding@CHString@@QEBA?AV1@PEBG@Z) // public: class CHString __thiscall CHString::SpanExcluding(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHString13SpanIncludingEPKt, ?SpanIncluding@CHString@@QEBA?AV1@PEBG@Z) // public: class CHString __thiscall CHString::SpanIncluding(unsigned short const *)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString8TrimLeftEv, ?TrimLeft@CHString@@QEAAXXZ) // public: void __thiscall CHString::TrimLeft(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString9TrimRightEv, ?TrimRight@CHString@@QEAAXXZ) // public: void __thiscall CHString::TrimRight(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHString12UnlockBufferEv, ?UnlockBuffer@CHString@@QEAAXXZ) // public: void __thiscall CHString::UnlockBuffer(void)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHStringcvPKtEv, ??BCHString@@QEBAPEBGXZ) // public: __thiscall CHString::operator unsigned short const *(void)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringpLERKS_, ??YCHString@@QEAAAEBV0@AEBV0@@Z) // public: class CHString const & __thiscall CHString::operator+=(class CHString const &)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringpLEc, ??YCHString@@QEAAAEBV0@D@Z) // public: class CHString const & __thiscall CHString::operator+=(char)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringpLEPKt, ??YCHString@@QEAAAEBV0@PEBG@Z) // public: class CHString const & __thiscall CHString::operator+=(unsigned short const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringpLEt, ??YCHString@@QEAAAEBV0@G@Z) // public: class CHString const & __thiscall CHString::operator+=(unsigned short)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSEPS_, ??4CHString@@QEAAAEBV0@PEAV0@@Z) // public: class CHString const & __thiscall CHString::operator=(class CHString *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSERKS_, ??4CHString@@QEAAAEBV0@AEBV0@@Z) // public: class CHString const & __thiscall CHString::operator=(class CHString const &)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSEPKc, ??4CHString@@QEAAAEBV0@PEBD@Z) // public: class CHString const & __thiscall CHString::operator=(char const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSEc, ??4CHString@@QEAAAEBV0@D@Z) // public: class CHString const & __thiscall CHString::operator=(char)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSEPKh, ??4CHString@@QEAAAEBV0@PEBE@Z) // public: class CHString const & __thiscall CHString::operator=(unsigned char const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSEPKt, ??4CHString@@QEAAAEBV0@PEBG@Z) // public: class CHString const & __thiscall CHString::operator=(unsigned short const *)
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringaSEt, ??4CHString@@QEAAAEBV0@G@Z) // public: class CHString const & __thiscall CHString::operator=(unsigned short)
DEFINE_FRAMEDYN_ALIAS(_ZNK8CHStringixEi, ??ACHString@@QEBAGH@Z) // public: unsigned short __thiscall CHString::operator[](int)const
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringD1Ev, ??1CHString@@QEAA@XZ) // public: __thiscall CHString::~CHString(void), complete object destructor
DEFINE_FRAMEDYN_ALIAS(_ZN8CHStringD2Ev, ??1CHString@@QEAA@XZ) // public: __thiscall CHString::~CHString(void), base object destructor
DEFINE_FRAMEDYN_ALIAS(_ZpltRK8CHString, ??H@YA?AVCHString@@GAEBV0@@Z) // class CHString __stdcall operator+(unsigned short,class CHString const &)
DEFINE_FRAMEDYN_ALIAS(_ZplRK8CHStringt, ??H@YA?AVCHString@@AEBV0@G@Z) // class CHString __stdcall operator+(class CHString const &,unsigned short)
DEFINE_FRAMEDYN_ALIAS(_ZplRK8CHStringPKt, ??H@YA?AVCHString@@AEBV0@PEBG@Z) // class CHString __stdcall operator+(class CHString const &,unsigned short const *)
DEFINE_FRAMEDYN_ALIAS(_ZplPKtRK8CHString, ??H@YA?AVCHString@@PEBGAEBV0@@Z) // class CHString __stdcall operator+(unsigned short const *,class CHString const &)
DEFINE_FRAMEDYN_ALIAS(_ZplRK8CHStringS1_, ??H@YA?AVCHString@@AEBV0@0@Z) // class CHString __stdcall operator+(class CHString const &,class CHString const &)
DEFINE_FRAMEDYN_ALIAS(_ZN8Provider5FlushEv, ?Flush@Provider@@MEAAXXZ) // protected: virtual void __thiscall Provider::Flush(void)
DEFINE_FRAMEDYN_ALIAS(_ZN8Provider21ValidateDeletionFlagsEl, ?ValidateDeletionFlags@Provider@@MEAAJJ@Z) // protected: virtual long __thiscall Provider::ValidateDeletionFlags(long)
DEFINE_FRAMEDYN_ALIAS(_ZN8Provider19ValidateMethodFlagsEl, ?ValidateMethodFlags@Provider@@MEAAJJ@Z) // protected: virtual long __thiscall Provider::ValidateMethodFlags(long)
DEFINE_FRAMEDYN_ALIAS(_ZN8Provider18ValidateQueryFlagsEl, ?ValidateQueryFlags@Provider@@MEAAJJ@Z) // protected: virtual long __thiscall Provider::ValidateQueryFlags(long)
#else
#error Unsupported arch
#endif

#undef DEFINE_FRAMEDYN_ALIAS

#endif // __GNUC__

#endif
