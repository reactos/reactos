//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#include "stdafx.h"
#include "TextParse.h"

CTextParse::CTextParse()
{
	m_szBuffer[0] = '\0';
	m_pStartLine = m_szBuffer;
	m_pLine = m_szBuffer;
	m_pSavePos = m_szBuffer;
}

CTextParse::CTextParse(LPCTSTR pszLine)
{
	m_szBuffer[0] = '\0';
	Set(pszLine);
}

CTextParse::~CTextParse()
{
}

CTextParse::CTextParse(const CTextParse &tp)
{
	m_pLine = tp.m_pLine;
	m_pStartLine = tp.m_pStartLine;
	m_pSavePos = tp.m_pSavePos;
	m_szCopyBuf[0] = '\0';
	m_szBuffer[0] = '\0';
	_tcscpy(m_szCopyBuf,tp.m_szCopyBuf);
	_tcscpy(m_szBuffer,tp.m_szBuffer);
}

const CTextParse& CTextParse::operator=(LPCTSTR lpsz)
{
	Set(lpsz);
	m_szCopyBuf[0] = '\0';
	m_szBuffer[0] = '\0';
	return *this;
}

const CTextParse& CTextParse::operator=(const CTextParse &tp)
{
	if (*this == tp)
		return *this;
	m_pLine = tp.m_pLine;
	m_pStartLine = tp.m_pStartLine;
	m_pSavePos = tp.m_pSavePos;
	_tcscpy(m_szCopyBuf,tp.m_szCopyBuf);
	_tcscpy(m_szBuffer,tp.m_szBuffer);
	return *this;
}

BOOL CTextParse::CharExistFromCurPos(int c,BOOL bForward)
{
	LPCTSTR p = m_pLine;
	if (bForward) 
	{
		while (*p && *p != c)
			   p = _tcsinc(p);
	}
	else
	{
		while (p > m_pStartLine && *p != c)
			   p = _tcsdec(m_pLine,p);
	}
	return *p == c;
}

BOOL CTextParse::ValidCppCharExist(int c,BOOL bForward)
{
	LPCTSTR p = m_pLine;
	while (*p && *p != c && *p != '/')
		   p = _tcsinc(p);
	return *p == c;
}

BOOL CTextParse::CharExist(LPCTSTR str)
{
	LPCTSTR p = m_pStartLine;
	while (*p && !IsToken(str,p))
		   p = _tcsinc(p);
	return *p != '\0';
}

BOOL CTextParse::FindString(LPCTSTR str)
{
	LPCTSTR p = _tcsstr(m_pLine,str);
	if (p)
		m_pLine = p;
	return p ? TRUE : FALSE;
}

BOOL CTextParse::FindChar(int c)
{
	LPCTSTR p = _tcschr(m_pLine,c);
	if (p)
		m_pLine = p;
	return p ? TRUE : FALSE;
}

BOOL CTextParse::MoveUntilChar(int c,BOOL bForward)
{
	if (bForward) 
	{
		while(*m_pLine && *m_pLine != c)
			   m_pLine = _tcsinc(m_pLine);
		return *m_pLine != '\0';
	}
	else 
	{
		while(m_pLine > m_pStartLine && *m_pLine != c)
			   m_pLine = _tcsdec(m_pStartLine,m_pLine);
		return *m_pLine == c;
	}
}

BOOL CTextParse::MoveUntilChar(LPCTSTR strTok,BOOL bForward)
{
	if (bForward) 
	{
		while(*m_pLine && !IsToken(strTok,m_pLine))
			   m_pLine = _tcsinc(m_pLine);
		return *m_pLine != '\0';
	}
	else 
	{
		while(m_pLine > m_pStartLine && !IsToken(strTok,m_pLine))
			   m_pLine = _tcsdec(m_pStartLine,m_pLine);
		return IsToken(strTok,m_pLine);
	}
}

BOOL CTextParse::MoveUntilString(LPCTSTR str,BOOL bForward)
{
	if (bForward) 
	{
		while(*m_pLine && !IsString(str))
			   m_pLine = _tcsinc(m_pLine);
		return *m_pLine != '\0';
	}
	else
	{
		while(m_pLine > m_pStartLine && !IsString(str))
			   m_pLine = _tcsdec(m_pStartLine,m_pLine);
		return IsString(str);
	}
}

BOOL CTextParse::MoveWhileChar(int c,BOOL bForward)
{
	if (bForward) 
	{
		while(*m_pLine && *m_pLine == c)
			   m_pLine = _tcsinc(m_pLine);
		return *m_pLine != '\0';
	}
	else
	{
		while(m_pLine > m_pStartLine && *m_pLine == c)
			   m_pLine = _tcsdec(m_pStartLine,m_pLine);
		return *m_pLine == c;
	}
}

BOOL CTextParse::MoveWhileChar(LPCTSTR strTok,BOOL bForward)
{
	if (bForward) 
	{
		while(*m_pLine && IsToken(strTok,m_pLine))
			   m_pLine = _tcsinc(m_pLine);
		return *m_pLine != '\0';
	}
	else
	{
		while(m_pLine > m_pStartLine && IsToken(strTok,m_pLine))
			   m_pLine = _tcsdec(m_pStartLine,m_pLine);
		return IsToken(strTok,m_pLine);
	}
}

LPCTSTR CTextParse::CopyWhileChar(int c)
{
	for(int i=0;i < MAX_BUF && *m_pLine != '\0' && *m_pLine == c;i++)
	{
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::CopyUntilString(LPCTSTR pszText)
{
	int nLen = _tcslen(pszText);
	for(int i=0;i < MAX_BUF && *m_pLine != '\0' && _tcsncmp(m_pLine,pszText,nLen) != 0;i++)
	{
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::CopyUntilChar(int c)
{
	for(int i=0;i < MAX_BUF && *m_pLine != '\0' && *m_pLine != c;i++)
	{
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::CopyWhileChar(LPCTSTR strTok)
{
	for(int i=0;i < MAX_BUF && *m_pLine != '\0' && IsToken(strTok,m_pLine);i++)
	{
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::CopyUntilChar(LPCTSTR strTok)
{
	for(int i=0;i < MAX_BUF && *m_pLine != '\0' && !IsToken(strTok,m_pLine);i++)
	{
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::CopyFuncUntilChar(LPCTSTR strTok)
{
	for(int i=0;i < MAX_BUF && !IsEnd() && !IsToken(strTok,m_pLine);i++) 
	{
/*		if (*m_pLine == '=') {
			MoveUntilChar(",)");
			if (m_szCopyBuf[i-1] == ' ') {
				i--;
			}
		}*/
		if (CharAtCurrent('/')) 
		{
			MoveForward();
			MoveUntilChar(_T("/"));
			if (m_szCopyBuf[i-1] == ' ') 
			{
				i--;
			    MoveForward();
			}
		}
		m_szCopyBuf[i] = *m_pLine;
		MoveForward();
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

BOOL CTextParse::IsToken(LPCTSTR strTok,LPCTSTR p)
{
	while(*strTok) 
	{
		  if (*p == *strTok)
			  break;
		   strTok = _tcsinc(strTok);
	}
	return *strTok != '\0';
}

LPCTSTR CTextParse::ExtractConstructor()
{
	MoveUntilChar('(');
	CopyUntilChar(')');
	_tcscat(m_szCopyBuf,_T(")"));
	return m_szCopyBuf;
}

LPCTSTR CTextParse::ExtractDeclareMacro()
{
	return CopyUntilChar('(');
}

LPCTSTR CTextParse::ExtractFuncName()
{
	Reset();
	// skip keywords
	if (_tcsncmp(m_pLine,_T("const"),5) == 0 || _tcsncmp(m_pLine,_T("afx_msg"),7) == 0 || _tcsncmp(m_pLine,_T("virtual"),7) == 0) 
	{
		CopyUntilWhiteSpace();
		MoveWhileWhiteSpace();
	}
	MoveUntilChar(_T("("));
	MoveUntilWhiteSpace(FALSE);
	if (*m_pLine == CPP_SPACE) // regular function
		MoveWhileWhiteSpace();
	else
	{ // else constructor or destructor
		// allow for virtual destructors
		if (_tcsncmp(m_pLine,_T("virtual"),7) == 0)
		{
			CopyUntilWhiteSpace();
			MoveWhileWhiteSpace();
		}
		Reset();
	}
	// returning pointer or reference
	if (*m_pLine == '*' || *m_pLine == '&')
	   m_pLine = _tcsinc(m_pLine);
	return CopyUntilChar('(');	
}

LPCTSTR CTextParse::ExtractClassName()
{
	MoveWhileWhiteSpace();
	if (_tcsncmp(m_pLine,_T("class"),5) != 0)
	{
		m_szCopyBuf[0] = 0;
		return m_szCopyBuf;
	}
	Reset();
	// check if derived class
	if (!CharExistFromCurPos(':'))
	{
		if (!CharExistFromCurPos('{'))
		{
			MoveToLastChar();
			MoveWhileWhiteSpace(FALSE);
			MoveUntilWhiteSpace(FALSE);
			MoveWhileWhiteSpace();
		}
		else
		{
			MoveUntilChar('{');
			MoveUntilWhiteSpace(FALSE);
			MoveWhileWhiteSpace(FALSE);			
			MoveUntilWhiteSpace(FALSE);
			MoveWhileWhiteSpace();			
		}
	}
	else
	{
		MoveUntilChar(':');
		// go back and skip colon
		MoveUntilWhiteSpace(FALSE);
		MoveWhileWhiteSpace(FALSE);
		if (CharAtCurrent('>'))
		{
			MoveUntilChar('<',FALSE);
			MoveUntilWhiteSpace(FALSE);
			MoveWhileWhiteSpace(FALSE);
		}
		MoveUntilWhiteSpace(FALSE);
		MoveWhileWhiteSpace();
	}
	return CopyUntilChar(_T(" \t<"));	
}

LPCTSTR CTextParse::ExtractBaseClassName()
{
	Reset();
	MoveWhileWhiteSpace();
	if (_tcsncmp(m_pLine,_T("class"),5) != 0)
	{
		m_szCopyBuf[0] = 0;
		return m_szCopyBuf;
	}
	Reset();
	// check if derived class
	if (CharExistFromCurPos(':'))
	{
		MoveUntilChar(':');
		// go back
		MoveWhileChar(':');		
		MoveWhileWhiteSpace();
		MoveUntilWhiteSpace();
		MoveWhileWhiteSpace();
	}
	else
	{
		m_szCopyBuf[0] = 0;
		return m_szCopyBuf;
	}
	return CopyUntilChar(_T(" \t<"));	
}

BOOL CTextParse::IsCommentBlock(LPCTSTR strStart,LPCTSTR strEnd)
{
	LPCTSTR p = _tcsstr(m_pLine,strStart);
	if (p)
	{
		if (p == m_pStartLine && *(p+sizeof(TCHAR)*2) == '+')
			return FALSE;
		if (p == m_pStartLine || (p != m_pStartLine && *(p-sizeof(TCHAR)) != '/') ) 
		{
			if (_tcsstr(m_pLine,strEnd) == NULL)
				return TRUE;
		}
	}
	return FALSE;
}

LPCTSTR CTextParse::CopyWholeWord()
{
	bool bQuote=false;
	for(int i=0;i < MAX_BUF && *m_pLine != '\0';i++) 
	{
		if (*m_pLine == '"') 
		{
			bQuote = !bQuote;
		}
		else if (*m_pLine == ' ' && bQuote == false)
		{
			break;
		}
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	SkipWhiteSpace();
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::CopyUntilEnd()
{
	for(int i=0;i < MAX_BUF && *m_pLine != '\0';i++) 
	{
		m_szCopyBuf[i] = *m_pLine;
		m_pLine = _tcsinc(m_pLine);
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

bool CTextParse::FindWholeWord(LPCTSTR pszText)
{
	bool bQuote = _tcsstr(pszText,_T("\"")) != NULL;
	if (*pszText == '+' || *pszText == '-')
		pszText = _tcsinc(pszText);
	int nLen = _tcslen(pszText);
	int nRet=-1;
	for(int i=0;i < MAX_BUF && *m_pLine != '\0';i++) 
	{
		SkipWhiteSpace();
		if (*m_pLine == '"')
		{
			if (bQuote == true) 
			{
				nRet = _tcsncmp(m_pLine,pszText,nLen);
				if (nRet == 0)
					break;
			}
			MoveWhileChar('"');
			MoveUntilChar('"');
			MoveWhileChar('"');
		}
		else
		{
			nRet = _tcsncmp(m_pLine,pszText,GetWordLen());
			if (nRet == 0)
				break;
			SkipWord();
		}
	}
	return nRet == 0;
}

int CTextParse::GetWordLen()
{
	LPCTSTR p = m_pLine;
	for (int i=0;*p && !IsToken(CPP_WHITE_SPACE,p);i++)
	{
		p++;
	}
	return i;
}

bool CTextParse::SkipHTMLCommand(bool bSkipCRLF)
{
	if (CharAtCurrent('&'))
	{
		MoveUntilChar(';');
		if (CharAtCurrent(';'))
			MoveForward();
		return true;
	}
	LPCTSTR p = m_pLine;
	if (bSkipCRLF)
	{
		while (*p == '\r' || *p == '\n')
			p = _tcsinc(p);
	}
	if (*p != '<')
		return false;
	while (*p != '>' && *p != '\0')
			p = _tcsinc(p);
	if (*p == '>')
		p = _tcsinc(p);
	m_pLine = p;
	return true;
}

void CTextParse::SkipHTMLCommands(bool bSkipCRLF)
{
	while (SkipHTMLCommand(bSkipCRLF))
		;
}

BOOL CTextParse::IsValidCPP(LPCTSTR pszText)
{
	BOOL bRet=FALSE;
	if (FindString(pszText))
	{
		// check if a comment
		SaveCurPos();
		if (!MoveUntilString(_T("//"),FALSE))
		{
			RestorePos();
			if (!MoveUntilString(_T("/*"),FALSE))
				bRet = TRUE;
		}
	}
	Reset();
	return bRet;
}

BOOL CTextParse::IsPrivate()
{
	return IsValidCPP(_T("private"));
}

BOOL CTextParse::IsPublic()
{
	return IsValidCPP(_T("public"));
}

BOOL CTextParse::IsProtected()
{
	return IsValidCPP(_T("protected"));
}

BOOL CTextParse::IsVirtualFunc()
{
	return IsValidCPP(_T("virtual"));
}

BOOL CTextParse::IsStartBrace()
{
	return IsValidCPP(_T("{"));
}

BOOL CTextParse::IsEndBrace()
{
	return IsValidCPP(_T("}"));
}

BOOL CTextParse::IsAccessSpecifier()
{
	return IsValidCPP(_T(":"));
}

BOOL CTextParse::IsConstructor(LPCTSTR pszClassName)
{
	CString sTest(pszClassName);
	sTest += _T("(");
	if (FindChar('~'))
	{
		Reset();
		return FALSE;
	}
	return IsValidCPP(sTest);
}

BOOL CTextParse::IsMsgMap()
{
	return FindString(_T("{{AFX_MSG"));
}

BOOL CTextParse::IsDeclareMacro()
{
	BOOL bRet = FALSE;
	if (IsValidCPP(_T("DECLARE_")))
	{
		if (!FindString(_T("()")))
			bRet = TRUE;
		Reset();
	}
	return bRet;
}

BOOL CTextParse::IsStartCommentBlock()
{
	if (!FindString(_T("//")))
		return FindString(_T("/*"));
	return FALSE;
}

BOOL CTextParse::IsEndCommentBlock()
{
	return FindString(_T("*/"));
}

BOOL CTextParse::IsClass()
{
	BOOL bRet=TRUE;
	if (FindString(_T("template")))
	{
		bRet = FALSE;
	}
	if (!StringAtCurrent(_T("class ")))
	{
		bRet = FALSE;
	}
	Reset();
	// any line comments before 'class '
	MoveUntilString(_T("class "));
	if (MoveUntilString(_T("//"),FALSE))
	{
		bRet = FALSE;		
	}
	Reset();
	if (FindChar(_T(';')))
	{
		bRet = FALSE;
	}
	if (FindChar(_T('#')))
	{
		bRet = FALSE;
	}
	Reset();
	return bRet;
}

BOOL CTextParse::ExtractArgs(CString &sRet,CStringArray &asArgs)
{
	asArgs.RemoveAll();
	SaveCurPos();
	if (!CharAtCurrent('(') && FindChar('('))
	{
		RestorePos();
		SkipWhiteSpace();
		if (StringAtCurrent(_T("const")))
		{
			MoveUntilWhiteSpace();			
			SkipWhiteSpace();
		}
		sRet = CopyUntilWhiteSpace();
		if (MoveUntilChar('('))
		{
			MoveForward();
		}
		else
		{
			RestorePos();
		}
	}
	while (!CharAtCurrent(')') && !IsEnd())
	{
		SkipWhiteSpace();
		if (StringAtCurrent(_T("const")))
		{
			MoveUntilWhiteSpace();			
			SkipWhiteSpace();
		}
		MoveUntilWhiteSpace();
		SkipWhiteSpace();
		if (StringAtCurrent(_T("void")))
			break;
		if (CharAtCurrent('*') || CharAtCurrent('&'))
			MoveForward();
		asArgs.Add(CopyUntilChar(_T(",)\t ")));
		if (CharAtCurrent(','))
			MoveForward();
		else
			SkipWhiteSpace();
	}
	return TRUE;
}

LPCTSTR CTextParse::ExtractHTMLText(bool bRemoveCRLF)
{
	return ExtractHTMLText(_T("<"),bRemoveCRLF);
}

LPCTSTR CTextParse::ExtractHTMLText(LPCTSTR pszUntil,bool bRemoveCRLF)
{
	int nLen=_tcslen(pszUntil);
	int i=0;
	bool bEnd = false;
	while(i < MAX_BUF && *m_pLine != '\0' && _tcsncmp(m_pLine,pszUntil,nLen) != 0)
	{
		if (*m_pLine == '<' || *m_pLine == '&')
		{
			while (SkipHTMLCommand(false))
			{
				if (_tcsncmp(m_pLine,pszUntil,nLen) == 0)
				{
					bEnd = true;
					break;
				}
			}
		}
		if (bEnd)
			break;
		if (bRemoveCRLF && (*m_pLine == '\n' || *m_pLine == '\r'))
		{
			m_pLine = _tcsinc(m_pLine);
		}
		else
		{
			m_szCopyBuf[i++] = *m_pLine;
			m_pLine = _tcsinc(m_pLine);
		}
	}
	m_szCopyBuf[i] = '\0';
	return m_szCopyBuf;
}

LPCTSTR CTextParse::ExtractHTMLLink()
{
	m_szCopyBuf[0] = '\0';
	if (!FindString(_T("href=")))
		return m_szCopyBuf;
	MoveUntilChar('"');
	LPCTSTR p = m_pLine;
	if (*p == '"')
	{
		p = _tcsinc(p);
		m_pLine = p;
		LPCTSTR pRet = CopyUntilChar('"');
		MoveUntilChar('>');
		MoveWhileChar('>');
		return pRet;
	}
	return m_szCopyBuf;
}

