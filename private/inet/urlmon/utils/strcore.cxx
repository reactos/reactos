// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include <urlint.h>
#include <map_kv.h>
#include "coll.hxx"

/*
#include "stdafx.h"

#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW
*/

// The following macros are used on data declarations/definitions
//  (they are redefined for extension DLLs and the shared MFC DLL)
#define AFX_DATA
#define AFX_DATADEF

// used when building the "core" MFC42.DLL
#ifndef AFX_CORE_DATA
        #define AFX_CORE_DATA
        #define AFX_CORE_DATADEF
#endif

// used when building the MFC/OLE support MFCO42.DLL
#ifndef AFX_OLE_DATA
        #define AFX_OLE_DATA
        #define AFX_OLE_DATADEF
#endif

#define TRACE1(x,a)
//int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);
// conversion helpers
int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
AFX_DATADEF TCHAR afxChNil = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
static int rgInitData[] = { -1, 0, 0, 0 };
static AFX_DATADEF CStringData* afxDataNil = (CStringData*)&rgInitData;
static LPCTSTR afxPchNil = (LPCTSTR)(((BYTE*)&rgInitData)+sizeof(CStringData));
// special function to make afxEmptyString work even during initialization
const CString& AFXAPI AfxGetEmptyString()
        { return *(CString*)&afxPchNil; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CString::CString()
{
        Init();
}

CString::CString(const CString& stringSrc)
{
        ASSERT(stringSrc.GetData()->nRefs != 0);
        if (stringSrc.GetData()->nRefs >= 0)
        {
                ASSERT(stringSrc.GetData() != afxDataNil);
                m_pchData = stringSrc.m_pchData;
                InterlockedIncrement(&GetData()->nRefs);
        }
        else
        {
                Init();
                *this = stringSrc.m_pchData;
        }
}

void CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
        ASSERT(nLen >= 0);
        ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

        if (nLen == 0)
                Init();
        else
        {
                CStringData* pData =
                        (CStringData*)new BYTE[sizeof(CStringData) + (nLen+1)*sizeof(TCHAR)];
                pData->nRefs = 1;
                pData->data()[nLen] = '\0';
                pData->nDataLength = nLen;
                pData->nAllocLength = nLen;
                m_pchData = pData->data();
        }
}

void CString::Release()
{
        if (GetData() != afxDataNil)
        {
                ASSERT(GetData()->nRefs != 0);
                if (InterlockedDecrement(&GetData()->nRefs) <= 0)
                        delete[] (BYTE*)GetData();
                Init();
        }
}

void PASCAL CString::Release(CStringData* pData)
{
        if (pData != afxDataNil)
        {
                ASSERT(pData->nRefs != 0);
                if (InterlockedDecrement(&pData->nRefs) <= 0)
                        delete[] (BYTE*)pData;
        }
}

void CString::Empty()
{
        if (GetData()->nDataLength == 0)
                return;
        if (GetData()->nRefs >= 0)
                Release();
        else
                *this = &afxChNil;
        ASSERT(GetData()->nDataLength == 0);
        ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

void CString::CopyBeforeWrite()
{
        if (GetData()->nRefs > 1)
        {
                CStringData* pData = GetData();
                Release();
                AllocBuffer(pData->nDataLength);
                memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
        }
        ASSERT(GetData()->nRefs <= 1);
}

void CString::AllocBeforeWrite(int nLen)
{
        if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
        {
                Release();
                AllocBuffer(nLen);
        }
        ASSERT(GetData()->nRefs <= 1);
}

CString::~CString()
//  free any attached data
{
        if (GetData() != afxDataNil)
        {
                if (InterlockedDecrement(&GetData()->nRefs) <= 0)
                        delete[] (BYTE*)GetData();
        }
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
         int nExtraLen) const
{
        // will clone the data attached to this string
        // allocating 'nExtraLen' characters
        // Places results in uninitialized string 'dest'
        // Will copy the part or all of original data to start of new string

        int nNewLen = nCopyLen + nExtraLen;
        if (nNewLen == 0)
        {
                dest.Init();
        }
        else
        {
                dest.AllocBuffer(nNewLen);
                memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
        }
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(LPCTSTR lpsz)
{
        Init();
        if (lpsz != NULL && (DWORD_PTR)lpsz <= 0xFFFF)
        {
                UINT nID = PtrToUlong(lpsz) & 0xFFFF;
                //if (!LoadString(nID))
                //        TRACE1("Warning: implicit LoadString(%u) failed\n", nID);
        }
        else
        {
                int nLen = SafeStrlen(lpsz);
                if (nLen != 0)
                {
                        AllocBuffer(nLen);
                        memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
                }
        }
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _UNICODE
CString::CString(LPCSTR lpsz)
{
        Init();
        int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
        if (nSrcLen != 0)
        {
                AllocBuffer(nSrcLen);
                _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
                ReleaseBuffer();
        }
}
#else //_UNICODE
CString::CString(LPCWSTR lpsz)
{
        Init();
        int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
        if (nSrcLen != 0)
        {
                AllocBuffer(nSrcLen*2);
                _wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
                ReleaseBuffer();
        }
}
#endif //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// Diagnostic support

#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CString& string)
{
        dc << string.m_pchData;
        return dc;
}
#endif //_DEBUG

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
        AllocBeforeWrite(nSrcLen);
        memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
        GetData()->nDataLength = nSrcLen;
        m_pchData[nSrcLen] = '\0';
}

const CString& CString::operator=(const CString& stringSrc)
{
        if (m_pchData != stringSrc.m_pchData)
        {
                if ((GetData()->nRefs < 0 && GetData() != afxDataNil) ||
                        stringSrc.GetData()->nRefs < 0)
                {
                        // actual copy necessary since one of the strings is locked
                        AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
                }
                else
                {
                        // can just copy references around
                        Release();
                        ASSERT(stringSrc.GetData() != afxDataNil);
                        m_pchData = stringSrc.m_pchData;
                        InterlockedIncrement(&GetData()->nRefs);
                }
        }
        return *this;
}

const CString& CString::operator=(LPCTSTR lpsz)
{
        ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
        AssignCopy(SafeStrlen(lpsz), lpsz);
        return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifdef _UNICODE
const CString& CString::operator=(LPCSTR lpsz)
{
        int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
        AllocBeforeWrite(nSrcLen);
        _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
        ReleaseBuffer();
        return *this;
}
#else //!_UNICODE
const CString& CString::operator=(LPCWSTR lpsz)
{
        int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
        AllocBeforeWrite(nSrcLen*2);
        _wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
        ReleaseBuffer();
        return *this;
}
#endif  //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = TCHAR, LPCTSTR
//          CString + ?
//          ? + CString

void CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
        int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

        int nNewLen = nSrc1Len + nSrc2Len;
        if (nNewLen != 0)
        {
                AllocBuffer(nNewLen);
                memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
                memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
        }
}

CString AFXAPI operator+(const CString& string1, const CString& string2)
{
        CString s;
        s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
                string2.GetData()->nDataLength, string2.m_pchData);
        return s;
}

CString AFXAPI operator+(const CString& string, LPCTSTR lpsz)
{
        ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
        CString s;
        s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
                CString::SafeStrlen(lpsz), lpsz);
        return s;
}

CString AFXAPI operator+(LPCTSTR lpsz, const CString& string)
{
        ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
        CString s;
        s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
                string.m_pchData);
        return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
        //  -- the main routine for += operators

        // concatenating an empty string is a no-op!
        if (nSrcLen == 0)
                return;

        // if the buffer is too small, or we have a width mis-match, just
        //   allocate a new buffer (slow but sure)
        if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
        {
                // we have to grow the buffer, use the ConcatCopy routine
                CStringData* pOldData = GetData();
                ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
                ASSERT(pOldData != NULL);
                CString::Release(pOldData);
        }
        else
        {
                // fast concatenation when buffer big enough
                memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TCHAR));
                GetData()->nDataLength += nSrcLen;
                ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
                m_pchData[GetData()->nDataLength] = '\0';
        }
}

const CString& CString::operator+=(LPCTSTR lpsz)
{
        ASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
        ConcatInPlace(SafeStrlen(lpsz), lpsz);
        return *this;
}

const CString& CString::operator+=(TCHAR ch)
{
        ConcatInPlace(1, &ch);
        return *this;
}

const CString& CString::operator+=(const CString& string)
{
        ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
        return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPTSTR CString::GetBuffer(int nMinBufLength)
{
        ASSERT(nMinBufLength >= 0);

        if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
        {
                // we have to grow the buffer
                CStringData* pOldData = GetData();
                int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
                if (nMinBufLength < nOldLen)
                        nMinBufLength = nOldLen;
                AllocBuffer(nMinBufLength);
                memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
                GetData()->nDataLength = nOldLen;
                CString::Release(pOldData);
        }
        ASSERT(GetData()->nRefs <= 1);

        // return a pointer to the character storage for this string
        ASSERT(m_pchData != NULL);
        return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
        CopyBeforeWrite();  // just in case GetBuffer was not called

        if (nNewLength == -1)
                nNewLength = lstrlen(m_pchData); // zero terminated

        ASSERT(nNewLength <= GetData()->nAllocLength);
        GetData()->nDataLength = nNewLength;
        m_pchData[nNewLength] = '\0';
}

LPTSTR CString::GetBufferSetLength(int nNewLength)
{
        ASSERT(nNewLength >= 0);

        GetBuffer(nNewLength);
        GetData()->nDataLength = nNewLength;
        m_pchData[nNewLength] = '\0';
        return m_pchData;
}

void CString::FreeExtra()
{
        ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
        if (GetData()->nDataLength != GetData()->nAllocLength)
        {
                CStringData* pOldData = GetData();
                AllocBuffer(GetData()->nDataLength);
                memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(TCHAR));
                ASSERT(m_pchData[GetData()->nDataLength] == '\0');
                CString::Release(pOldData);
        }
        ASSERT(GetData() != NULL);
}

LPTSTR CString::LockBuffer()
{
        LPTSTR lpsz = GetBuffer(0);
        GetData()->nRefs = -1;
        return lpsz;
}

void CString::UnlockBuffer()
{
        ASSERT(GetData()->nRefs == -1);
        if (GetData() != afxDataNil)
                GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

#ifdef unused
int CString::Find(TCHAR ch) const
{
        // find first single character
        LPTSTR lpsz = _tcschr(m_pchData, (_TUCHAR)ch);

        // return -1 if not found and index otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneOf(LPCTSTR lpszCharSet) const
{
        ASSERT(AfxIsValidString(lpszCharSet, FALSE));
        LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}
void CString::MakeReverse()
{
        CopyBeforeWrite();
        _tcsrev(m_pchData);
}
#endif //unused

void CString::MakeUpper()
{
        CopyBeforeWrite();
        ::CharUpper(m_pchData);
}

void CString::MakeLower()
{
        CopyBeforeWrite();
        ::CharLower(m_pchData);
}


void CString::SetAt(int nIndex, TCHAR ch)
{
        ASSERT(nIndex >= 0);
        ASSERT(nIndex < GetData()->nDataLength);

        CopyBeforeWrite();
        m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
void CString::AnsiToOem()
{
        CopyBeforeWrite();
        ::AnsiToOem(m_pchData, m_pchData);
}
void CString::OemToAnsi()
{
        CopyBeforeWrite();
        ::OemToAnsi(m_pchData, m_pchData);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CString conversion helpers (these use the current system locale)

int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
        if (count == 0 && mbstr != NULL)
                return 0;

        int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1,
                mbstr, count, NULL, NULL);
        ASSERT(mbstr == NULL || result <= (int)count);
        if (result > 0)
                mbstr[result-1] = 0;
        return result;
}

int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
        if (count == 0 && wcstr != NULL)
                return 0;

        int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
                wcstr, count);
        ASSERT(wcstr == NULL || result <= (int)count);
        if (result > 0)
                wcstr[result-1] = 0;
        return result;
}

LPWSTR AFXAPI AfxA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
        if (lpa == NULL)
                return NULL;
        ASSERT(lpw != NULL);
        // verify that no illegal character present
        // since lpw was allocated based on the size of lpa
        // don't worry about the number of chars
        lpw[0] = '\0';
        VERIFY(MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars));
        return lpw;
}

LPSTR AFXAPI AfxW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
        if (lpw == NULL)
                return NULL;
        ASSERT(lpa != NULL);
        // verify that no illegal character present
        // since lpa was allocated based on the size of lpw
        // don't worry about the number of chars
        lpa[0] = '\0';
        VERIFY(WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL));
        return lpa;
}

///////////////////////////////////////////////////////////////////////////////
