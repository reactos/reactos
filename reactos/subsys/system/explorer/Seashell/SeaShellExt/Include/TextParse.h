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

#ifndef __LINEPARSE_H__
#define __LINEPARSE_H__

#define CPP_SPACE ' '
#define CPP_TAB '\t'
#define CPP_NEWLINE '\n'
#define CPP_CRLF _T("\r\n")
#define CPP_WHITE_SPACE _T(" \t")

class CTRL_EXT_CLASS CTextParse 
{
	enum { MAX_BUF = 4096 };
public:
	CTextParse();
	CTextParse(const CTextParse &tp);
	CTextParse(LPCTSTR pszLine);
	~CTextParse();
public:
	operator LPCTSTR() const;
	operator LPTSTR();
	const CTextParse& operator=(LPCTSTR lpsz);
	const CTextParse& operator=(const CTextParse &tp);
	LPCTSTR operator++(int);
	LPTSTR &operator++();
	LPCTSTR operator--(int);
	LPCTSTR &operator--();
	int GetMax();
	void Set(LPCTSTR p);
	void Reset();
	void SaveCurPos();
	void RestorePos();
	void SetAtCurrent(int c);
	void MoveForward();
	void MoveBack();
	void MoveForward(int nCount);
	void MoveBack(int nCount);
	BOOL IsVirtualFunc();
	BOOL IsPrivate();
	BOOL IsPublic();
	BOOL IsProtected();
	BOOL IsEnd();
	BOOL IsClass();
	BOOL IsStartBrace();
	BOOL IsEndBrace();
	BOOL IsAccessSpecifier();
	BOOL IsMsgMap();
	BOOL IsDeclareMacro();
	BOOL IsStartCommentBlock();
	BOOL IsEndCommentBlock();
	BOOL IsConstructor(LPCTSTR pszClassName);
	BOOL IsValidCPP(LPCTSTR pszText);
	BOOL CharAtStart(int c);
	BOOL CharAtStart(LPCTSTR strTok);
	BOOL CharAtCurrent(int c);
	BOOL CharAtCurrent(LPCTSTR strTok);
	BOOL StringAtStart(LPCTSTR str);
	BOOL StringAtCurrent(LPCTSTR str);
	BOOL CharExist(int c,BOOL bForward = TRUE);
	BOOL StringExist(LPCTSTR str);
	BOOL StringExistInString(LPCTSTR str);
	BOOL SkipWord(BOOL bForward = TRUE);
	BOOL SkipWhiteSpace(BOOL bForward = TRUE);

	BOOL CharExistFromCurPos(int c,BOOL bForward = TRUE);
	BOOL ValidCppCharExist(int c,BOOL bForward = TRUE);
	BOOL CharExist(LPCTSTR str);
	BOOL FindString(LPCTSTR str);
	BOOL FindChar(int c);
	BOOL MoveWhileWhiteSpace(BOOL bForward = TRUE);
	BOOL MoveUntilWhiteSpace(BOOL bForward = TRUE);
	BOOL MoveUntilChar(int c,BOOL bForward = TRUE);
	BOOL MoveUntilChar(LPCTSTR strTok,BOOL bForward = TRUE);
	BOOL MoveUntilString(LPCTSTR str,BOOL bForward = TRUE);
	BOOL MoveWhileChar(int c,BOOL bForward = TRUE);
	BOOL MoveWhileChar(LPCTSTR strTok,BOOL bForward = TRUE);
	void MoveToLastChar();
	LPCTSTR CopyUntilWhiteSpace();
	LPCTSTR CopyUntilChar(int c);
	LPCTSTR CopyUntilString(LPCTSTR pszText);
	LPCTSTR CopyUntilChar(LPCTSTR strTok);
	LPCTSTR CopyWhileChar(int c);
	LPCTSTR CopyWhileChar(LPCTSTR strTok);
	LPCTSTR CopyFuncUntilChar(LPCTSTR strTok);
	LPCTSTR CopyUntilEnd();
	LPCTSTR CopyWhileWhiteSpace();
	BOOL IsCommentBlock(LPCTSTR strStart,LPCTSTR strEnd);
	BOOL ExtractArgs(CString &sRet,CStringArray &asArgs);
	LPCTSTR ExtractDeclareMacro();
	LPCTSTR ExtractConstructor();
	LPCTSTR ExtractFuncName();
	LPCTSTR ExtractClassName();
	LPCTSTR ExtractBaseClassName();
	LPCTSTR ExtractHTMLText(bool bRemoveCRLF=false);
	LPCTSTR ExtractHTMLText(LPCTSTR pszUntil,bool bRemoveCRLF=false);
	LPCTSTR ExtractHTMLLink();
	LPCTSTR ExtractDefaultArgs();
	LPCTSTR CopyWholeWord();
	bool FindWholeWord(LPCTSTR pszText);
	int GetWordLen();
	int GetCurrentChar();
	bool SkipHTMLCommand(bool bSkipCRLF=true);
	void SkipHTMLCommands(bool bSkipCRLF=true);
protected:
	BOOL IsToken(LPCTSTR strTok,LPCTSTR p);
	BOOL IsString(LPCTSTR str);
private:
	LPCTSTR m_pLine;
	LPCTSTR m_pStartLine;
	LPCTSTR m_pSavePos;
	TCHAR m_szCopyBuf[MAX_BUF+1];
	TCHAR m_szBuffer[MAX_BUF+1];
};

inline void CTextParse::MoveForward()
{
	m_pLine = _tcsinc(m_pLine);
}

inline void CTextParse::MoveBack()
{
	m_pLine = _tcsdec(m_pStartLine,m_pLine);
}

inline void CTextParse::MoveForward(int nCount)
{
	m_pLine = _tcsninc(m_pLine,nCount);
}

inline void CTextParse::MoveBack(int nCount)
{
	int i=nCount;
	LPCTSTR p = m_pLine;
	while (p > m_pStartLine && i > 0)
	{
		   p = _tcsdec(m_pLine,p);
		   i--;
	}
	m_pLine = p;
}

inline CTextParse::operator LPCTSTR() const
{
	return m_pStartLine;
}

inline CTextParse::operator LPTSTR() 
{
	Reset();
	return m_szBuffer;
}

// prefix
inline LPCTSTR CTextParse::operator++(int)
{
	MoveForward();
	return (LPCTSTR&)*m_pLine;
}

// postfix
inline LPTSTR &CTextParse::operator++()
{
	LPCTSTR p = m_pLine;
	MoveForward();
	return (LPTSTR&)*p;
}

inline LPCTSTR CTextParse::operator--(int)
{
	MoveBack();
	return m_pLine;
}

inline LPCTSTR &CTextParse::operator--()
{
	LPCTSTR p = m_pLine;
	MoveBack();
	return (LPCTSTR&)*p;
}

inline BOOL CTextParse::IsEnd()
{
	return *m_pLine == '\0';
}

inline int CTextParse::GetMax()
{
	return MAX_BUF;
}

inline int CTextParse::GetCurrentChar()
{
	return *m_pLine;
}

inline void CTextParse::SetAtCurrent(int c)
{
	int i = m_pLine-m_szBuffer;
	m_szBuffer[i] = c;
}

inline void CTextParse::Set(LPCTSTR p)
{
	m_pStartLine = p;
	m_pLine = p;
	m_pSavePos = p;
}

inline void CTextParse::Reset()
{
	m_pLine = m_pStartLine;
}

inline void CTextParse::SaveCurPos()
{
	m_pSavePos = m_pLine;
}

inline void CTextParse::RestorePos()
{
	m_pLine = m_pSavePos;
}

inline BOOL CTextParse::CharAtStart(int c)
{
	return *m_pStartLine == c;
}

inline BOOL CTextParse::CharAtStart(LPCTSTR strTok)
{
	return IsToken(strTok,m_pStartLine);
}

inline BOOL CTextParse::CharAtCurrent(int c)
{
	return *m_pLine == c;
}

inline BOOL CTextParse::CharAtCurrent(LPCTSTR strTok)
{
	return IsToken(strTok,m_pLine);
}

inline BOOL CTextParse::StringAtStart(LPCTSTR str)
{
	return(_tcsncmp(m_pStartLine,str,_tcslen(str)) == 0);
}

inline BOOL CTextParse::StringAtCurrent(LPCTSTR str)
{
	return(_tcsncmp(m_pLine,str,_tcslen(str)) == 0);
}

inline BOOL CTextParse::CharExist(int c,BOOL bForward)
{
	return _tcschr(m_pStartLine,c) != NULL;
}

inline BOOL CTextParse::StringExist(LPCTSTR str)
{
	return _tcsstr(m_pLine,str) != NULL;
}

inline BOOL CTextParse::StringExistInString(LPCTSTR str)
{
	return _tcsstr(m_pStartLine,str) != NULL;
}

inline BOOL CTextParse::SkipWord(BOOL bForward)
{
	return MoveUntilChar(CPP_WHITE_SPACE,bForward);
}

inline BOOL CTextParse::SkipWhiteSpace(BOOL bForward)
{
	return MoveWhileChar(CPP_WHITE_SPACE,bForward);
}

inline BOOL CTextParse::IsString(LPCTSTR str)
{
	return(_tcsncmp(m_pLine,str,_tcslen(str)) == 0);
}

inline LPCTSTR CTextParse::CopyUntilWhiteSpace()
{
	return CopyUntilChar(CPP_WHITE_SPACE);
}

inline LPCTSTR CTextParse::CopyWhileWhiteSpace()
{
	return CopyWhileChar(CPP_WHITE_SPACE);
}

inline BOOL CTextParse::MoveWhileWhiteSpace(BOOL bForward)
{
	return MoveWhileChar(CPP_WHITE_SPACE,bForward);
}

inline BOOL CTextParse::MoveUntilWhiteSpace(BOOL bForward)
{
	return MoveUntilChar(CPP_WHITE_SPACE,bForward);
}

// move to last char
inline void CTextParse::MoveToLastChar()
{
	LPCTSTR p = m_pLine;
	while (*p != '\0')
		   p = _tcsinc(p);
	if (p != m_pLine)
		p = _tcsdec(m_pStartLine,p);
	m_pLine = p;
}

#endif
