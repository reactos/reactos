/**************************************************/
/*					                              */
/*					                              */
/*	EudcEditor Utillity funcs	                  */
/*					                              */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include	"stdafx.h"
#include	"eudcedit.h"
#include	"util.h"

/****************************************/
/*					*/
/*	Output Message function		*/
/*					*/
/****************************************/
int 
OutputMessageBox(
HWND	hWnd,
UINT 	TitleID,
UINT	MessgID,
BOOL	OkFlag)
{
	CString	TitleStr, MessgStr;
	int	mResult;

	TitleStr.LoadString( TitleID);
	MessgStr.LoadString( MessgID);
	if( OkFlag){
		mResult = ::MessageBox( hWnd, MessgStr, TitleStr,
			MB_OK | MB_ICONEXCLAMATION);
	}else{
		mResult = ::MessageBox( hWnd, MessgStr, TitleStr,
			MB_YESNOCANCEL | MB_ICONQUESTION);
	}
	return mResult;
}

#ifdef BUILD_ON_WINNT
int 
OutputMessageBoxEx(
HWND	hWnd,
UINT 	TitleID,
UINT	MessgID,
BOOL	OkFlag,
        ...)
{
	CString	TitleStr, MessgStr;
	int	mResult;
    va_list argList;
    LPTSTR  MessageBody;

    va_start(argList, OkFlag);
	TitleStr.LoadString( TitleID);
	MessgStr.LoadString( MessgID);

    ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                     MessgStr,0,0,(LPTSTR)&MessageBody,0,&argList);

	if( OkFlag){
		mResult = ::MessageBox( hWnd, MessageBody, TitleStr,
			MB_OK | MB_ICONEXCLAMATION);
	}else{
		mResult = ::MessageBox( hWnd, MessageBody, TitleStr,
			MB_YESNOCANCEL | MB_ICONQUESTION);
	}
    ::LocalFree(MessageBody);
	return mResult;
}
#endif // BUILD_ON_WINNT

/****************************************/
/*					*/
/*   	Get String from resource	*/
/*					*/
/****************************************/
void 
GetStringRes( 
LPTSTR 	lpStr, 
UINT 	sID)
{
	CString	cStr;
	int	StrLength;
	TCHAR 	*Swap;

	cStr.LoadString( sID);
	StrLength = cStr.GetLength();
	Swap = cStr.GetBuffer(StrLength + 1);
	lstrcpy( lpStr, Swap);
	cStr.ReleaseBuffer();

	return;
}

/****************************************/
/*					*/
/*   	Convert String from resource	*/
/*					*/
/****************************************/
void 
ConvStringRes( 
LPTSTR 	lpStr, 
CString	String)
{
	TCHAR 	*Swap;

	int StrLength = String.GetLength();
	Swap = String.GetBuffer(StrLength + 1);
	lstrcpy( lpStr, Swap);
	String.ReleaseBuffer();

	return;
}

#ifndef UNICODE
char * Mystrrchr(char *pszString, char ch)
{
	CHAR *p1, *p2;
	p1 = NULL;
	for (p2 = pszString; *p2; p2=CharNext(p2))
	{
		if (*p2 == ch)
		{
			p1 = p2;
		}
	}
	return (p1);
}

char * Mystrchr(char *pszString, char ch)
{
	CHAR *p;
	
	for (p = pszString; *p; p=CharNext(p))
	{
		if (*p == ch)
		{
			return (p);
		}
	}
	return (NULL);
}
#endif
