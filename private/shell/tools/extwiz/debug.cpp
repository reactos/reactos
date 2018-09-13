#include "stdafx.h"

#ifdef _PSEUDO_DEBUG   // entire file

#ifdef _PSEUDO_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LONG AssertBusy = -1;
LONG AssertReallyBusy = -1;

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine)
{
	TCHAR szMessage[_MAX_PATH*2];

	InterlockedDecrement(&AssertReallyBusy);

	// format message into buffer
	wsprintf(szMessage, _T("File %hs, Line %d"),
		lpszFileName, nLine);

	TCHAR szT[_MAX_PATH*2 + 20];
	wsprintf(szT, _T("Assertion Failed: %s\n"), szMessage);
	OutputDebugString(szT);

	if (InterlockedIncrement(&AssertBusy) > 0)
	{
		InterlockedDecrement(&AssertBusy);

		// assert within assert (examine call stack to determine first one)
		DebugBreak();
		return FALSE;
	}

	// active popup window for the current thread
	HWND hWndParent = GetActiveWindow();
	if (hWndParent != NULL)
		hWndParent = GetLastActivePopup(hWndParent);

	// display the assert
	int nCode = ::MessageBox(hWndParent, szMessage, _T("Assertion Failed!"),
		MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

	// cleanup
	InterlockedDecrement(&AssertBusy);

	if (nCode == IDIGNORE)
		return FALSE;   // ignore

	if (nCode == IDRETRY)
		return TRUE;    // will cause DebugBreak

	AfxAbort();     // should not return (but otherwise DebugBreak)
	return TRUE;
}

void Trace(LPCTSTR lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	TCHAR szBuffer[512];

	nBuf = _vstprintf(szBuffer, lpszFormat, args);
	ASSERT(nBuf < (sizeof(szBuffer)/sizeof(szBuffer[0])));

	CString strMessage;

	if (AfxGetApp() != NULL)
		strMessage = ((CString) (AfxGetApp()->m_pszExeName)) + _T(": ");
	strMessage += szBuffer;
	OutputDebugString(strMessage);

	va_end(args);
}


#endif // _PSEUDO_DEBUG
