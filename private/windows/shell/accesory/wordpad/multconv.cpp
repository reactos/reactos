// convert.cpp : implementation file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "wordpad.h"
#include "multconv.h"
#include "mswd6_32.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#ifdef CONVERTERS
CConverter* CConverter::m_pThis = NULL;
#endif

#define BUFFSIZE 4096

CTrackFile::CTrackFile(CFrameWnd* pWnd) : CFile()
{
    m_nLastPercent = -1;
    m_dwLength = 0;
    m_pFrameWnd = pWnd;
    VERIFY(m_strComplete.LoadString(IDS_COMPLETE));
    VERIFY(m_strWait.LoadString(IDS_PLEASE_WAIT));
    VERIFY(m_strSaving.LoadString(IDS_SAVING));
//  OutputPercent(0);
}

CTrackFile::~CTrackFile()
{
    OutputPercent(100);
    if (m_pFrameWnd != NULL)
        m_pFrameWnd->SetMessageText(AFX_IDS_IDLEMESSAGE);
}

UINT CTrackFile::Read(void FAR* lpBuf, UINT nCount)
{
    UINT n = CFile::Read(lpBuf, nCount);
    if (m_dwLength != 0)
        OutputPercent((int)((GetPosition()*100)/m_dwLength));
    return n;
}

void CTrackFile::Write(const void FAR* lpBuf, UINT nCount)
{
    CFile::Write(lpBuf, nCount);
    OutputString(m_strSaving);
//  if (m_dwLength != 0)
//      OutputPercent((int)((GetPosition()*100)/m_dwLength));
}

void CTrackFile::OutputString(LPCTSTR lpsz)
{
    if (m_pFrameWnd != NULL)
    {
        m_pFrameWnd->SetMessageText(lpsz);
        CWnd* pBarWnd = m_pFrameWnd->GetMessageBar();
        if (pBarWnd != NULL)
            pBarWnd->UpdateWindow();
    }
}

void CTrackFile::OutputPercent(int nPercentComplete)
{
    if (m_pFrameWnd != NULL && m_nLastPercent != nPercentComplete)
    {
        m_nLastPercent = nPercentComplete;
        TCHAR buf[64];
        int n = nPercentComplete;
        wsprintf(buf, (n==100) ? m_strWait : m_strComplete, n);
        OutputString(buf);
    }
}

COEMFile::COEMFile(CFrameWnd* pWnd) : CTrackFile(pWnd)
{
}

UINT COEMFile::Read(void FAR* lpBuf, UINT nCount)
{
    UINT n = CTrackFile::Read(lpBuf, nCount);
    OemToCharBuffA((const char*)lpBuf, (char*)lpBuf, n);
    return n;
}

void COEMFile::Write(const void FAR* lpBuf, UINT nCount)
{
    CharToOemBuffA((const char*)lpBuf, (char*)lpBuf, nCount);
    CTrackFile::Write(lpBuf, nCount);
}

#ifdef CONVERTERS

HGLOBAL CConverter::StringToHGLOBAL(LPCSTR pstr)
{
    HGLOBAL hMem = NULL;
    if (pstr != NULL)
    {
        hMem = GlobalAlloc(GHND, (lstrlenA(pstr)*2)+1);
        if (NULL == hMem)
            AfxThrowMemoryException();
        char* p = (char*) GlobalLock(hMem);
        if (p != NULL)
            lstrcpyA(p, pstr);
        GlobalUnlock(hMem);
    }
    return hMem;
}

CConverter::CConverter(LPCTSTR pszLibName, CFrameWnd* pWnd) : CTrackFile(pWnd)
{
    USES_CONVERSION;
    m_hBuff = NULL;
    m_pBuf = NULL;
    m_nBytesAvail = 0;
    m_nBytesWritten = 0;
    m_nPercent = 0;
    m_hEventFile = NULL;
    m_hEventConv = NULL;
    m_bDone = TRUE;
    m_bConvErr = FALSE;
    m_hFileName = NULL;
    m_bUseOEM = TRUE;

    #ifndef _X86_

    //Prevent known alignment exception problems in write converter
    //from crashing the app on some RISC machines

    m_uPrevErrMode = SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);

    #endif

    m_hLibCnv = LoadLibrary(pszLibName);

    if (NULL != m_hLibCnv)
    {
        LoadFunctions();
        ASSERT(m_pInitConverter != NULL);
        if (m_pInitConverter != NULL)
        {
         //
         // For the current converters, you have to pass a *static*
         // string to InitConverter32
         //

            VERIFY(m_pInitConverter(AfxGetMainWnd()->GetSafeHwnd(), "WORDPAD"));
        }

        if (m_pRegisterApp != NULL)
        {
            NegotiateForNonOEM();
        }
    }
}

CConverter::~CConverter()
{
    if (!m_bDone) // converter thread hasn't exited
    {
        m_bDone = TRUE;

        if (!m_bForeignToRtf)
            WaitForConverter();

        m_nBytesAvail = 0;
        VERIFY(ResetEvent(m_hEventFile));
        m_nBytesAvail = 0;
        SetEvent(m_hEventConv);
        WaitForConverter();// wait for DoConversion exit
        VERIFY(ResetEvent(m_hEventFile));
    }

    if (m_hEventFile != NULL)
        VERIFY(CloseHandle(m_hEventFile));
    if (m_hEventConv != NULL)
        VERIFY(CloseHandle(m_hEventConv));
    if (m_hLibCnv != NULL)
        FreeLibrary(m_hLibCnv);
    if (m_hFileName != NULL)
        GlobalFree(m_hFileName);

    #ifndef _X86_

    //Reset error mode to what it was before we changed it in
    //the constructor

    SetErrorMode(m_uPrevErrMode);

    #endif
}

void CConverter::WaitForConverter()
{
    // while event not signalled -- process messages
    while (MsgWaitForMultipleObjects(1, &m_hEventFile, FALSE, INFINITE,
        QS_SENDMESSAGE) != WAIT_OBJECT_0)
    {
        MSG msg;
        PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE);
    }
}

void CConverter::WaitForBuffer()
{
    // while event not signalled -- process messages
    while (MsgWaitForMultipleObjects(1, &m_hEventConv, FALSE, INFINITE,
        QS_SENDMESSAGE) != WAIT_OBJECT_0)
    {
        MSG msg;
        PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE);
    }
}

UINT AFX_CDECL CConverter::ConverterThread(LPVOID)  // AFX_CDECL added by t-stefb
{
    ASSERT(m_pThis != NULL);

#if defined(_DEBUG)
    HRESULT hRes = OleInitialize(NULL);
    ASSERT(hRes == S_OK || hRes == S_FALSE);
#else
    OleInitialize(NULL);
#endif

    m_pThis->DoConversion();
    OleUninitialize();

    return 0;
}

BOOL CConverter::IsFormatCorrect(LPCTSTR pszFileName)
{
    USES_CONVERSION;
    int nRet;
    if (m_hLibCnv == NULL || m_pIsFormatCorrect == NULL)
        return FALSE;

    char buf[_MAX_PATH];
    strcpy(buf, T2CA(pszFileName));

    if (m_bUseOEM)
        CharToOemA(buf, buf);

    HGLOBAL hFileName = StringToHGLOBAL(buf);
    HGLOBAL hDesc = GlobalAlloc(GHND, 256);
    if (NULL == hDesc)
        AfxThrowMemoryException();
    nRet = m_pIsFormatCorrect(hFileName, hDesc);
    GlobalFree(hDesc);
    GlobalFree(hFileName);
    return (nRet == 1) ? TRUE : FALSE;
}

// static callback function
int CALLBACK CConverter::WriteOutStatic(int cch, int nPercentComplete)
{
    ASSERT(m_pThis != NULL);
    return m_pThis->WriteOut(cch, nPercentComplete);
}

int CALLBACK CConverter::WriteOut(int cch, int nPercentComplete)
{
    ASSERT(m_hBuff != NULL);
    m_nPercent = nPercentComplete;
    if (m_hBuff == NULL)
        return -9;

    //
    // If m_bDone is TRUE that means the richedit control has stopped
    // streaming in text and is trying to destroy the CConverter object but
    // the converter still has more data to give
    //

    if (m_bDone)
    {
        ASSERT(!"Richedit control stopped streaming prematurely");
        AfxMessageBox(IDS_CONVERTER_ABORTED);
        return -9;
    }

    if (cch != 0)
    {
        WaitForBuffer();
        VERIFY(ResetEvent(m_hEventConv));
        m_nBytesAvail = cch;
        SetEvent(m_hEventFile);
        WaitForBuffer();
    }
    return 0; //everything OK
}

int CALLBACK CConverter::ReadInStatic(int /*flags*/, int nPercentComplete)
{
    ASSERT(m_pThis != NULL);
    return m_pThis->ReadIn(nPercentComplete);
}

int CALLBACK CConverter::ReadIn(int /*nPercentComplete*/)
{
    ASSERT(m_hBuff != NULL);
    if (m_hBuff == NULL)
        return -8;

    SetEvent(m_hEventFile);
    WaitForBuffer();
    VERIFY(ResetEvent(m_hEventConv));

    return m_nBytesAvail;
}

BOOL CConverter::DoConversion()
{
    USES_CONVERSION;
    m_nLastPercent = -1;
//  m_dwLength = 0; // prevent Read/Write from displaying
    m_nPercent = 0;

    ASSERT(m_hBuff != NULL);
    ASSERT(m_pThis != NULL);
    HGLOBAL hDesc = StringToHGLOBAL("");
    HGLOBAL hSubset = StringToHGLOBAL("");

    int nRet = -1;
    if (m_bForeignToRtf && NULL != m_pForeignToRtf)
    {
        ASSERT(m_pForeignToRtf != NULL);
        ASSERT(m_hFileName != NULL);
        nRet = m_pForeignToRtf(m_hFileName, NULL, m_hBuff, hDesc, hSubset,
            (LPFNOUT)WriteOutStatic);
        // wait for next CConverter::Read to come through
        WaitForBuffer();
        VERIFY(ResetEvent(m_hEventConv));
    }
    else if (!m_bForeignToRtf && NULL != m_pRtfToForeign)
    {
        ASSERT(m_pRtfToForeign != NULL);
        ASSERT(m_hFileName != NULL);
        nRet = m_pRtfToForeign(m_hFileName, NULL, m_hBuff, hDesc,
            (LPFNIN)ReadInStatic);
        // don't need to wait for m_hEventConv
    }

    GlobalFree(hDesc);
    GlobalFree(hSubset);
    if (m_pBuf != NULL)
        GlobalUnlock(m_hBuff);
    GlobalFree(m_hBuff);

    if (nRet != 0)
        m_bConvErr = TRUE;

    m_bDone = TRUE;
    m_nPercent = 100;
    m_nLastPercent = -1;

    SetEvent(m_hEventFile);

    return (nRet == 0);
}

void CConverter::LoadFunctions()
{
    m_pInitConverter = (PINITCONVERTER)GetProcAddress(m_hLibCnv, "InitConverter32");
    m_pIsFormatCorrect = (PISFORMATCORRECT)GetProcAddress(m_hLibCnv, "IsFormatCorrect32");
    m_pForeignToRtf = (PFOREIGNTORTF)GetProcAddress(m_hLibCnv, "ForeignToRtf32");
    m_pRtfToForeign = (PRTFTOFOREIGN)GetProcAddress(m_hLibCnv, "RtfToForeign32");
    m_pRegisterApp = (PREGISTERAPP) GetProcAddress(m_hLibCnv, "RegisterApp");
}
#endif

///////////////////////////////////////////////////////////////////////////////

BOOL CConverter::Open(LPCTSTR pszFileName, UINT nOpenFlags,
    CFileException* pException)
{
    USES_CONVERSION;

    // The converters only speak ansi

    char buf[_MAX_PATH];
    buf[_MAX_PATH - 1] = '\0';
    strncpy(buf, T2CA(pszFileName), _MAX_PATH);

    // Make sure we don't overflow the buffer on DBCS strings
    if ('\0' != buf[_MAX_PATH - 1])
        return FALSE;

    if (m_bUseOEM)
        CharToOemA(buf, buf);

    // let's make sure we could do what is wanted directly even though we aren't
    m_bCloseOnDelete = FALSE;
    m_hFile = (UINT)hFileNull;

    BOOL bOpen = CFile::Open(A2CT(buf), nOpenFlags, pException);
    CFile::Close();
    if (!bOpen)
        return FALSE;

    m_bForeignToRtf = !(nOpenFlags & (CFile::modeReadWrite | CFile::modeWrite));

    // check for reading empty file
    if (m_bForeignToRtf)
    {
        CFileStatus _stat;
        if (CFile::GetStatus(A2CT(buf), _stat) && _stat.m_size == 0)
            return TRUE;
    }

    //set security attributes to inherit handle
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    //create the events
    m_hEventFile = CreateEvent(&sa, TRUE, FALSE, NULL);
    m_hEventConv = CreateEvent(&sa, TRUE, FALSE, NULL);
    //create the converter thread and create the events

    ASSERT(m_hFileName == NULL);
    m_hFileName = StringToHGLOBAL(buf);

    m_pThis = this;
    m_bDone = FALSE;
    m_hBuff = GlobalAlloc(GHND, BUFFSIZE);
    ASSERT(m_hBuff != NULL);

    AfxBeginThread(ConverterThread, this, THREAD_PRIORITY_NORMAL, 0, 0, &sa);

    return TRUE;
}

// m_hEventConv -- the main thread signals this event when ready for more data
// m_hEventFile -- the converter signals this event when data is ready

UINT CConverter::Read(void FAR* lpBuf, UINT nCount)
{
    ASSERT(m_bForeignToRtf);
    if (m_bDone)
        return 0;
    // if converter is done
    int cch = nCount;
    BYTE* pBuf = (BYTE*)lpBuf;
    while (cch != 0)
    {
        if (m_nBytesAvail == 0)
        {
            if (m_pBuf != NULL)
                GlobalUnlock(m_hBuff);
            m_pBuf = NULL;
            SetEvent(m_hEventConv);
            WaitForConverter();
            VERIFY(ResetEvent(m_hEventFile));
            if (m_bConvErr)
                AfxThrowFileException(CFileException::generic);
            if (m_bDone)
                return nCount - cch;
            m_pBuf = (BYTE*)GlobalLock(m_hBuff);
            ASSERT(m_pBuf != NULL);
        }
        int nBytes = min(cch, m_nBytesAvail);
        memcpy(pBuf, m_pBuf, nBytes);
        pBuf += nBytes;
        m_pBuf += nBytes;
        m_nBytesAvail -= nBytes;
        cch -= nBytes;
        OutputPercent(m_nPercent);
    }
    return nCount - cch;
}

void CConverter::Write(const void FAR* lpBuf, UINT nCount)
{
    ASSERT(!m_bForeignToRtf);

    m_nBytesWritten += nCount;
    while (nCount != 0)
    {
        WaitForConverter();
        VERIFY(ResetEvent(m_hEventFile));
        if (m_bConvErr)
            AfxThrowFileException(CFileException::generic);
        m_nBytesAvail = min(nCount, BUFFSIZE);
        nCount -= m_nBytesAvail;
        BYTE* pBuf = (BYTE*)GlobalLock(m_hBuff);
        ASSERT(pBuf != NULL);
        memcpy(pBuf, lpBuf, m_nBytesAvail);
        GlobalUnlock(m_hBuff);
        SetEvent(m_hEventConv);
    }
    OutputString(m_strSaving);
}

LONG CConverter::Seek(LONG lOff, UINT nFrom)
{
    if (lOff != 0 && nFrom != current)
        AfxThrowNotSupportedException();
    return 0;
}

DWORD CConverter::GetPosition() const
{
    return 0;
}

void CConverter::Flush()
{
}

void CConverter::Close()
{
    if (!m_bDone) // converter thread hasn't exited
    {
        m_bDone = TRUE;

        if (!m_bForeignToRtf)
            WaitForConverter();

        m_nBytesAvail = 0;
        VERIFY(ResetEvent(m_hEventFile));
        m_nBytesAvail = 0;
        SetEvent(m_hEventConv);
        WaitForConverter();// wait for DoConversion exit
        VERIFY(ResetEvent(m_hEventFile));
    }

    if (m_bConvErr)
        AfxThrowFileException(CFileException::generic);
}

void CConverter::Abort()
{
}

DWORD CConverter::GetLength() const
{
    ASSERT_VALID(this);
    return 1;
}

CFile* CConverter::Duplicate() const
{
    AfxThrowNotSupportedException();
    return NULL;
}

void CConverter::LockRange(DWORD, DWORD)
{
    AfxThrowNotSupportedException();
}

void CConverter::UnlockRange(DWORD, DWORD)
{
    AfxThrowNotSupportedException();
}

void CConverter::SetLength(DWORD)
{
    AfxThrowNotSupportedException();
}



//+--------------------------------------------------------------------------
//
//  Method:     CConverter::NegotiateForNonOEM
//
//  Synopsis:   Try to tell the converter not to expect OEM filenames
//
//  Parameters: None
//
//  Returns:    void
//
//  Notes:      The converter's RegisterApp function will return a handle
//              containing it's preferences (what it supports).  The
//              data structure is a 16-bit size and then a sequence of
//              records.  For each record the first byte is the size, the
//              second is the "opcode", and then some variable-length opcode
//              specific data.  All sizes are inclusive.
//
//---------------------------------------------------------------------------

void CConverter::NegotiateForNonOEM()
{
    ASSERT(NULL != m_pRegisterApp);

    HGLOBAL     hPrefs;
    BYTE       *pPrefs;
    __int16     cbPrefs;

    //
    // Tell the converter we don't want to use OEM
    //

    hPrefs = (*m_pRegisterApp)(fRegAppSupportNonOem, NULL);

    if (NULL == hPrefs)
        return;

    pPrefs = (BYTE *) GlobalLock(hPrefs);

    if (NULL == pPrefs)
    {
        ASSERT(!"GlobalLock failed");
        GlobalFree(hPrefs);
        return;
    }

    //
    // Parse the returned structure looking for a RegAppOpcodeCharset opcode.
    // The argument for this opcode should be either ANSI_CHARSET or
    // OEM_CHARSET.  If its ANSI_CHARSET then we can talk Ansi otherwise were
    // stuck with OEM.
    //

    cbPrefs = (__int16) ((* (__int16 *) pPrefs) - sizeof(cbPrefs));
    pPrefs += sizeof(cbPrefs);

    while (cbPrefs > 0)
    {
        if (RegAppOpcodeCharset == pPrefs[1])
        {
            ASSERT(ANSI_CHARSET == pPrefs[2] || OEM_CHARSET == pPrefs[2]);

            m_bUseOEM = (OEM_CHARSET == pPrefs[2]);
            break;
        }
        else
        {
            if (pPrefs[0] <= 0)
            {
                ASSERT(!"RegisterApp is returning bogus data");
                break;
            }

            cbPrefs = (__int16) (cbPrefs - pPrefs[0]);
            pPrefs += pPrefs[0];
        }
    }

    GlobalUnlock(pPrefs);
    GlobalFree(hPrefs);
}
