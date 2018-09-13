/*-----------------------------------------------------------------------------

  debug.cpp

	This file implements the debuggin features

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved

	Authors:
		ChrisK	Chris Kauffman

	Histroy:
		7/22/96	ChrisK	Cleaned and formatted
		7/31/96 ValdonB Changes for Win16
	
-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>





BOOL fInAssert=FALSE;

// ############################################################################
// DebugSz
//
// This function outputs debug string
// 
//  Created 1/28/96,		Chris Kauffman
// ############################################################################
void DebugSz(LPCSTR psz)
{
#if defined(DEBUG)
	OutputDebugString(psz);
#endif	
} // DebugSz

// ############################################################################
// Debug Printf to debug output screen
void Dprintf(LPCSTR pcsz, ...)
{
#ifdef DEBUG
	va_list	argp;
	char	szBuf[1024];
	
	va_start(argp, pcsz);

	wvsprintf(szBuf, pcsz, argp);

	DebugSz(szBuf);
	va_end(argp);
#endif
} // Dprintf()

// ############################################################################
// Handle asserts
BOOL FAssertProc(LPCSTR szFile,  DWORD dwLine, LPCSTR szMsg, DWORD dwFlags)
{

	BOOL fAssertIntoDebugger = FALSE;

	char szMsgEx[1024], szTitle[255], szFileName[MAX_PATH];
	int id;
	UINT fuStyle;
	//BYTE	szTime[80];
#if !defined(WIN16)
	CHAR	szTime[80];
	HANDLE	hAssertTxt;
	SYSTEMTIME st;
	DWORD	cbWritten;
#endif
	
	// no recursive asserts
	if (fInAssert)
		{
		DebugSz("***Recursive Assert***\r\n");
		return(FALSE);
		}

	fInAssert = TRUE;
	
#if defined(WIN16)
	GetModuleFileName(g_hInstDll, szFileName, MAX_PATH);
	wsprintf(szMsgEx,"%s:#%ld\r\n%s,\r\n%s", szFile, dwLine, szFileName, szMsg);
#else
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	wsprintf(szMsgEx,"%s:#%d\r\nProcess ID: %d %s, Thread ID: %d\r\n%s",
		szFile,dwLine,GetCurrentProcessId(),szFileName,GetCurrentThreadId(),szMsg);
#endif
	wsprintf(szTitle,"Assertion Failed");

	fuStyle = MB_APPLMODAL | MB_ABORTRETRYIGNORE;
	fuStyle |= MB_ICONSTOP;

	DebugSz(szTitle);		
	DebugSz(szMsgEx);		

	// dump the assert into ASSERT.TXT
#if !defined(WIN16)
	hAssertTxt = CreateFile("assert.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (INVALID_HANDLE_VALUE != hAssertTxt) 
		{
		SetFilePointer(hAssertTxt, 0, NULL, FILE_END);
		GetLocalTime(&st);   
		wsprintf(szTime, "\r\n\r\n%02d/%02d/%02d %d:%02d:%02d\r\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
		WriteFile(hAssertTxt, szTime, lstrlen(szTime), &cbWritten, NULL);
		WriteFile(hAssertTxt, szMsgEx, lstrlen(szMsgEx), &cbWritten, NULL);
		CloseHandle(hAssertTxt);
		}
#endif

    id = MessageBox(NULL, szMsgEx, szTitle, fuStyle);
    switch (id)
    	{
    	case IDABORT:
#if defined(WIN16)
			abort();
#else
    		ExitProcess(0);
#endif
    		break;
    	case IDCANCEL:
    	case IDIGNORE:
    		break;
    	case IDRETRY:
    		fAssertIntoDebugger = TRUE;
    		break;
    	}
				
	fInAssert = FALSE;  
	
	return(fAssertIntoDebugger);
} // AssertProc()
