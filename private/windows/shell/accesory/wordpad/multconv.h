// convert.h : header file
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

#ifdef CONVERTERS

/////////////////////////////////////////////////////////////////////////////
// CConverter

typedef int (CALLBACK *LPFNOUT)(int cch, int nPercentComplete);
typedef int (CALLBACK *LPFNIN)(int flags, int nPercentComplete);
typedef BOOL (FAR PASCAL *PINITCONVERTER)(HWND hWnd, LPCSTR lpszModuleName);
typedef BOOL (FAR PASCAL *PISFORMATCORRECT)(HANDLE ghszFile, HANDLE ghszClass);
typedef int (FAR PASCAL *PFOREIGNTORTF)(HANDLE ghszFile, LPVOID lpv, HANDLE ghBuff, 
	HANDLE ghszClass, HANDLE ghszSubset, LPFNOUT lpfnOut);
typedef int (FAR PASCAL *PRTFTOFOREIGN)(HANDLE ghszFile, LPVOID lpv, HANDLE ghBuff, 
	HANDLE ghszClass, LPFNIN lpfnIn);
typedef HGLOBAL (FAR PASCAL *PREGISTERAPP)(long lFlags, void *lpRegApp);


//
// Some defines taken from the converter group's convapi.h
//

#define fRegAppSupportNonOem    0x00000008  // supports non-Oem filenames
#define RegAppOpcodeCharset             0x03    // for REGAPPRET


#endif

/////////////////////////////////////////////////////////////////////////////
// CTrackFile
class CTrackFile : public CFile
{ 
public:
//Construction
	CTrackFile(CFrameWnd* pWnd);
	~CTrackFile();
	
//Attributes
	int m_nLastPercent;
	DWORD m_dwLength;
	CFrameWnd* m_pFrameWnd;
	CString m_strComplete;
	CString m_strWait;
	CString m_strSaving;
//Operations
	void OutputPercent(int nPercentComplete = 0);
	void OutputString(LPCTSTR lpsz);
	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);
};

class COEMFile : public CTrackFile
{
public:
	COEMFile(CFrameWnd* pWnd);
	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);
};

#ifdef CONVERTERS

class CConverter : public CTrackFile
{
public:
	CConverter(LPCTSTR pszLibName, CFrameWnd* pWnd = NULL);

public:
//Attributes
	int m_nPercent;
	BOOL m_bDone;
	BOOL m_bConvErr;
	virtual DWORD GetPosition() const;

// Operations
	BOOL IsFormatCorrect(LPCTSTR pszFileName);
	BOOL DoConversion();
	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags,
		CFileException* pError = NULL);
	void WaitForConverter();
	void WaitForBuffer();

// Overridables
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual DWORD GetLength() const;

	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);

	virtual void Abort();
	virtual void Flush();
	virtual void Close();

// Unsupported
	virtual CFile* Duplicate() const;
	virtual void LockRange(DWORD dwPos, DWORD dwCount);
	virtual void UnlockRange(DWORD dwPos, DWORD dwCount);
	virtual void SetLength(DWORD dwNewLen);

//Implementation
public:
	~CConverter();

protected:
	int         m_nBytesAvail;
	int         m_nBytesWritten;
	HANDLE      m_hEventFile;
	HANDLE      m_hEventConv;
	BOOL        m_bForeignToRtf;        // True to convert to RTF, else from
	HGLOBAL     m_hBuff;                // Buffer for converter data
	BYTE*       m_pBuf;                 // Pointer to m_hBuff data
	HGLOBAL     m_hFileName;            // File to convert
	HINSTANCE   m_hLibCnv;              // The converter dll
    BOOL        m_bUseOEM;              // TRUE to use OEM filenames

    // Entry points into the converter dll

	PINITCONVERTER      m_pInitConverter;
	PISFORMATCORRECT    m_pIsFormatCorrect;
	PFOREIGNTORTF       m_pForeignToRtf;
	PRTFTOFOREIGN       m_pRtfToForeign;
    PREGISTERAPP        m_pRegisterApp;

	int CALLBACK WriteOut(int cch, int nPercentComplete);
	int CALLBACK ReadIn(int nPercentComplete);
	static HGLOBAL StringToHGLOBAL(LPCSTR pstr);
	static int CALLBACK WriteOutStatic(int cch, int nPercentComplete);
	static int CALLBACK ReadInStatic(int flags, int nPercentComplete);
	static UINT AFX_CDECL ConverterThread(LPVOID pParam);
	static CConverter *m_pThis;

	void LoadFunctions();
    void NegotiateForNonOEM();

    #ifndef _X86_

    //We need to change the error mode when using the write converter
    //to fix some alignment problems caused by the write converter.  These
    //problems do not affect x86 platforms.

    UINT m_uPrevErrMode ;

    #endif
};

#endif

/////////////////////////////////////////////////////////////////////////////
