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

// LocaleInfo.cpp: implementation of the CLocaleInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LocaleInfo.h"
#include "TextParse.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// static
CLocaleInfo *CLocaleInfo::Instance()
{
	static CLocaleInfo data;
	return &data;
}

CLocaleInfo::CLocaleInfo()
{
	m_pszNegativeSign = NULL;
	m_pszDecimalSep = NULL;
	m_pszThousandSep = NULL;
	m_pszLongDate = NULL;
	m_pszShortDate = NULL;
	m_pszShortTime = NULL;
	m_pszLongTime = NULL;
	m_pszDateSep = NULL;
	m_pszTimeSep = NULL;
	GetAllLocaleInfo();
}

CLocaleInfo::~CLocaleInfo()
{
	delete []m_pszNegativeSign;
	delete []m_pszDecimalSep;
	delete []m_pszThousandSep;
	delete []m_pszLongDate;
	delete []m_pszShortDate;
	delete []m_pszShortTime;
	delete []m_pszLongTime;
	delete []m_pszDateSep;
	delete []m_pszTimeSep;
}

void CLocaleInfo::GetAllLocaleInfo()
{		
	m_pszShortTime = new TCHAR[MAX_PATH];
	lstrcpy(m_pszShortTime,_T("H:mm"));
	AllocLocaleInfo(LOCALE_STIMEFORMAT,&m_pszLongTime);
	AllocLocaleInfo(LOCALE_SSHORTDATE,&m_pszShortDate);
	AllocLocaleInfo(LOCALE_SLONGDATE,&m_pszLongDate);
	AllocLocaleInfo(LOCALE_SDATE,&m_pszDateSep);
	AllocLocaleInfo(LOCALE_STIME,&m_pszTimeSep);
	AllocLocaleInfo(LOCALE_SNEGATIVESIGN,&m_pszNegativeSign);
	AllocLocaleInfo(LOCALE_SDECIMAL,&m_pszDecimalSep);
	AllocLocaleInfo(LOCALE_STHOUSAND,&m_pszThousandSep);
}

void CLocaleInfo::AllocLocaleInfo(LCTYPE lctype,LPTSTR *pszInfo)
{
	int nLen = GetLocaleInfo(LOCALE_USER_DEFAULT,lctype,NULL,0);
	if (nLen)
	{
		*pszInfo = new TCHAR[nLen];
		GetLocaleInfo(LOCALE_USER_DEFAULT,lctype,*pszInfo,nLen);
	}
}

CString CLocaleInfo::FormatDateTime(const COleDateTime &oleDateTime)
{
	CString sDate, sTime;
	sDate = oleDateTime.Format(ConvertStdToWinFormat(GetShortDate()));
	sTime = oleDateTime.Format(ConvertStdToWinFormat(GetLongTime()));
	return sDate + _T(" ") + sTime;
}

CString CLocaleInfo::FormatDateTime(const FILETIME &ft)
{
	COleDateTime oleDateTime(ft);
	return FormatDateTime(oleDateTime);
}

static struct FormatCodes
{
	LPCTSTR szWinFormat;
	LPCTSTR szStdFormat;
} FC[] = { 
	{ _T("M"), _T("%#m") },
	{ _T("MM"),_T("%m") },
	{ _T("MMM"),_T("%b") },
	{ _T("MMMM"),_T("%B") },
	{ _T("d"),_T("%#d") },
	{ _T("dd"),_T("%d") },
	{ _T("dddd"),_T("%A") },
	{ _T("yy"),_T("%y") },
	{ _T("yyyy"),_T("%Y") },
	{ _T("H"),_T("%#H") },
	{ _T("HH"),_T("%H") },
	{ _T("h"),_T("%#I") },
	{ _T("hh"),_T("%I") },
	{ _T("mm"),_T("%M") },
	{ _T("ss"),_T("%S") },
	{ _T("tt"),_T("%p") },
{  NULL, NULL }
};

// Short Format
// M/d/yyyy
// M/d/yy
// MM/dd/yy
// MM/dd/yyyy
// yy/MM/dd
// yyyy-MM-dd
// dd-MMM-yy

// Long Format
// dddd, MMMM dd, yyyy
// MMMM dd, yyyy
// dddd, dd MMMM, yyyy
// dd MMMM, yyyy

// Time
// H:mm:ss
// h:mm:ss tt
// hh:mm:ss tt
// HH:mm:ss

CString CLocaleInfo::ConvertStdToWinFormat(LPCTSTR pszFormat)
{
	CString sCode;
	CString sWinFormat;
	CTextParse parse(pszFormat);
	CString sSeparators(_T(" -/,.:"));
	while (!parse.IsEnd())
	{
			if (parse.CharAtCurrent(sSeparators))
			{
				sCode = parse.CopyWhileChar(sSeparators);
			}
			else
			{
				sCode = parse.CopyUntilChar(sSeparators);
			}
			StdToWinFormatCode(sCode);
			sWinFormat += sCode;
	}
	return sWinFormat;
}

void CLocaleInfo::StdToWinFormatCode(CString &sCode)
{
	for(int i=0;FC[i].szWinFormat != NULL;i++) 
	{
		if (sCode == FC[i].szWinFormat)
		{
			sCode = FC[i].szStdFormat;
			break;
		}
	}
}
