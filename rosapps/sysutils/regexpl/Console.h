// Console.h: interface for the CConsole class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(CONSOLE_H__FEF419EC_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
#define CONSOLE_H__FEF419EC_6EB6_11D3_907D_204C4F4F5020__INCLUDED_

#include "TextHistory.h"

typedef const TCHAR * (*ReplaceCompletionCallback)(unsigned __int64& rnIndex, const BOOL *pblnForward,
												   const TCHAR *pchContext, const TCHAR *pchBegin);

class CConsole  
{
public:
	void EnableWrite();
	void DisableWrite();
	void SetReplaceCompletionCallback(ReplaceCompletionCallback pfCallback);
	BOOL SetInsertMode(BOOL blnInsetMode);
	void BeginScrollingOperation();
	TCHAR * Init(DWORD dwBufferSize, DWORD dwMaxHistoryLines = 0);
	BOOL ReadLine();
	BOOL FlushInputBuffer();
//	BOOL SetOutputMode(DWORD dwMode);
//	BOOL SetInputMode(DWORD dwMode);
	BOOL SetTextAttribute(WORD wAttributes);
	BOOL GetTextAttribute(WORD& rwAttributes);
	BOOL SetTitle(TCHAR *p);
	BOOL Write(const TCHAR *p, DWORD dwChars = 0);
	CConsole();
	virtual ~CConsole();
private:
	HANDLE m_hStdOut;
	HANDLE m_hStdIn;
	HANDLE m_hStdError;
	COORD m_CursorPosition;
	COORD m_BufferSize;
	WORD m_wAttributes;
	SHORT m_Lines;
	BOOL WriteString(TCHAR *pchString, COORD Position);
	BOOL WriteChar(TCHAR ch);
	BOOL m_blnInsetMode;	// TRUE - insert, FALSE - overwrite
	DWORD m_dwInsertModeCursorHeight;
	DWORD m_dwOverwriteModeCursorHeight;
	TCHAR *m_pchBuffer;
	TCHAR *m_pchBuffer1;
	TCHAR *m_pchBuffer2;
	DWORD m_dwBufferSize;
	ReplaceCompletionCallback m_pfReplaceCompletionCallback;
	SHORT m_LinesScrolled;
	BOOL m_blnMoreMode;
	CTextHistory m_History;
	BOOL m_blnDisableWrite;
	DWORD m_dwOldOutputMode;
	DWORD m_dwOldInputMode;
	BOOL m_blnOldInputModeSaved;
	BOOL m_blnOldOutputModeSaved;
};

#endif // !defined(CONSOLE_H__FEF419EC_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
