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

// LocaleInfo.h: interface for the CLocaleInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOCALEINFO_H__53D6820E_C28F_4DB9_AB7D_87B9E8EAE233__INCLUDED_)
#define AFX_LOCALEINFO_H__53D6820E_C28F_4DB9_AB7D_87B9E8EAE233__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTRL_EXT_CLASS CLocaleInfo  
{
public:
	CLocaleInfo();
	virtual ~CLocaleInfo();
public:
// Properties
	LPCTSTR GetNegativeSign() const;
	LPCTSTR GetDecimalSep() const;
	LPCTSTR GetThousandSep() const;
	LPCTSTR GetLongDate() const;
	LPCTSTR GetShortDate() const;
	LPCTSTR GetShortTime() const;
	LPCTSTR GetLongTime() const;
	LPCTSTR GetDateSep() const;
	LPCTSTR GetTimeSep() const;
	CString ConvertStdToWinFormat(LPCTSTR pszFormat);
	CString FormatDateTime(const COleDateTime &oleDateTime);
	CString FormatDateTime(const FILETIME &ft);
protected:
	void StdToWinFormatCode(CString &sCode);
	void GetAllLocaleInfo();
	void AllocLocaleInfo(LCTYPE lctype,LPTSTR *pszInfo);
private:
    LPTSTR m_pszNegativeSign;
    LPTSTR m_pszDecimalSep;
    LPTSTR m_pszThousandSep;
    LPTSTR m_pszLongDate;
    LPTSTR m_pszShortDate;
    LPTSTR m_pszLongTime;
    LPTSTR m_pszShortTime;
    LPTSTR m_pszDateSep;
    LPTSTR m_pszTimeSep;
public:
	static CLocaleInfo *CLocaleInfo::Instance();
};

inline LPCTSTR CLocaleInfo::GetNegativeSign() const
{
	return m_pszNegativeSign;
}

inline LPCTSTR CLocaleInfo::GetDecimalSep() const
{
	return m_pszDecimalSep;
}

inline LPCTSTR CLocaleInfo::GetThousandSep() const
{
	return m_pszThousandSep;
}

inline LPCTSTR CLocaleInfo::GetLongDate() const
{
	return m_pszLongDate;
}

inline LPCTSTR CLocaleInfo::GetShortTime() const
{
	return m_pszShortTime;
}

inline LPCTSTR CLocaleInfo::GetLongTime() const
{
	return m_pszLongTime;
}

inline LPCTSTR CLocaleInfo::GetShortDate() const
{
	return m_pszShortDate;
}

inline LPCTSTR CLocaleInfo::GetDateSep() const
{
	return m_pszDateSep;
}

inline LPCTSTR CLocaleInfo::GetTimeSep() const
{
	return m_pszTimeSep;
}

#endif // !defined(AFX_LOCALEINFO_H__53D6820E_C28F_4DB9_AB7D_87B9E8EAE233__INCLUDED_)
