
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

/**********************************************************/
/* implementation of class CUIListCtrlData                  */
/*                                                        */
/**********************************************************/

#include "stdafx.h"
#include "UIData.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIconObj

IMPLEMENT_DYNAMIC(CUIODColumnCtrl, CObject)
IMPLEMENT_DYNAMIC(CUIListCtrlData, CObject)
IMPLEMENT_DYNAMIC(CUIDBListCtrlData, CUIListCtrlData)
IMPLEMENT_DYNAMIC(CUIStrListCtrlData, CUIListCtrlData)
IMPLEMENT_DYNAMIC(CUIComboBoxData, CUIListCtrlData)

/////////////////////////////////////////////////////////////////////////////
// CUIListCtrlData construction/destruction
CUIListCtrlData::CUIListCtrlData(int nCols)
{
	Init(nCols);
}

void CUIListCtrlData::Init(int nCols)
{
	const int cMaxCols=50;
	m_arCtrl.SetSize(nCols < cMaxCols ? cMaxCols : nCols);
	m_arFonts.SetSize(nCols < cMaxCols ? cMaxCols : nCols);
	m_arTextColors.SetSize(nCols < cMaxCols ? cMaxCols : nCols);
	m_arBkColors.SetSize(nCols);
	for(int i=0;i < nCols;i++)
	{
		SetDefaultTextColor(i);
		SetDefaultBkColor(i);
	}
	m_dwExtData = 0;
	m_bAutoDelete = false;
	m_bDeleted = false;
}

CUIListCtrlData::~CUIListCtrlData()
{
	DestroyCtrls();
	DestroyFonts();
}

void CUIListCtrlData::DestroyFonts()
{
	CFont *pFont=NULL;
	for(int i=0;i < m_arFonts.GetSize();i++)
	{
		pFont = (CFont*)m_arFonts[i];
		delete pFont;
	}
}

void CUIListCtrlData::DestroyCtrls()
{
	CUIODColumnCtrl *pCtrl=NULL;
	for(int i=0;i < m_arCtrl.GetSize();i++)
	{
		pCtrl = (CUIODColumnCtrl*)m_arCtrl[i];
		delete pCtrl;
	}
}

BOOL CUIListCtrlData::IsFontSet(int nCol) const
{
	if (nCol == -1)
	{
		BOOL bRet=TRUE;
		for(int i=0;i < m_arFonts.GetSize();i++)
		{
			if (m_arFonts[i] == NULL)
			{
				bRet = FALSE;
				break;
			}
		}
		return bRet;
	}
	return(m_arFonts[nCol] && ((CFont*)m_arFonts[nCol])->GetSafeHandle() != NULL);
}

void CUIListCtrlData::SetFont(CFont *pFont,int nCol) 
{ 
	LOGFONT lf;
	pFont->GetLogFont(&lf);
	if (nCol == -1)
	{
		for(int i=0;i < m_arFonts.GetSize();i++)
		{
			CreateNewFont(i,lf);
		}
	}
	else
	{
		CreateNewFont(nCol,lf);
	}
}

void CUIListCtrlData::CreateNewFont(int nCol,LOGFONT &lf)
{
	CFont *pNewFont = NULL;
	CFont *pOldFont = (CFont*)m_arFonts[nCol];
	if (pOldFont)
	{
		if (pOldFont->GetSafeHandle())
			pOldFont->DeleteObject();
		pNewFont = pOldFont;
	}
	else
		pNewFont = new CFont;
	pNewFont->CreateFontIndirect(&lf);
	m_arFonts[nCol] = pNewFont;	
}
/////////////////////////////////////////////////////////////////////////////
//
// CUIComboBoxData 
// 
/////////////////////////////////////////////////////////////////////////////
// columns in a combo box
CString CUIComboBoxData::GetText(int nCol) const
{ 
	TCHAR szDest[256];
	if (nCol == -1 || m_strText.IsEmpty())
		return m_strText; 
	LPCTSTR p = m_strText; 
	LPTSTR pDest;
	nCol++;
	while (nCol > 0) 
	{
		pDest = szDest;
		while (*p && *p != _T('\t')) 
		{
			  *pDest = *p;
			  pDest = _tcsinc(pDest);
			  p = _tcsinc(p);
		}
	   nCol--;
	   if (*p == _T('\t'))		
		   p = _tcsinc(p);
	   else
		   break;

	} 
	*pDest = '\0';
	CString str;
	if (nCol)
		str.Empty();
	else
		str = szDest;
	return str;
}

/////////////////////////////////////////////////////////////////////////////
// CUIStrListCtrlData 
/////////////////////////////////////////////////////////////////////////////
CUIStrListCtrlData::CUIStrListCtrlData(int nColumns) : CUIListCtrlData(nColumns)
{
	m_StringArray.SetSize(nColumns);
}

LPTSTR CUIStrListCtrlData::GetString(int nCol)
{ 
	return (LPTSTR)(LPCTSTR)m_StringArray[nCol];
}

// return TRUE if needs updating
BOOL CUIStrListCtrlData::AddString(int nCol,LPCTSTR szStr)
{ 
	if (nCol < 0)
		return FALSE;
	BOOL ret = !m_StringArray[nCol].IsEmpty();
	m_StringArray[nCol] = szStr;
	return ret;
}
/////////////////////////////////////////////////////////////////////////////
