// TextProgressCtrl.cpp : implementation file
//
// Written by Chris Maunder (cmaunder@mail.com)
// Copyright 1998.
//
// TextProgressCtrl is a drop-in replacement for the standard 
// CProgressCtrl that displays text in a progress control.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed by any means PROVIDING it is not sold for
// profit without the authors written consent, and providing that this
// notice and the authors name is included. If the source code in 
// this file is used in any commercial application then an email to
// the me would be nice.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to your
// computer or anything else vaguely within it's vicinity.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 
// Modified by Philip Oldaker 2000

#include "stdafx.h"
#include "TextProgressCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CTextProgressCtrl,CUIODColumnCtrl)
/////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrl

CTextProgressCtrl::CTextProgressCtrl()
{
    m_nPos      = 0;
    m_nStepSize = 1;
    m_nMax      = 100;
    m_nMin      = 0;
    m_bShowText = TRUE;
    m_crBarClr		= CLR_DEFAULT;
    m_crBgClr		= CLR_DEFAULT;
	m_crTextClr		= CLR_DEFAULT;
    m_crSelBarClr   = CLR_DEFAULT;
    m_crSelBgClr    = CLR_DEFAULT;
	m_crSelTextClr  = CLR_DEFAULT;
}

CTextProgressCtrl::~CTextProgressCtrl()
{
}

void CTextProgressCtrl::DoPaint(CDC *PaintDC,CRect rcClient,bool bSelected) 
{
    if (m_nMin >= m_nMax) 
        return;

	bool bInvert=bSelected;
    COLORREF crBarColour = (m_crBarClr == CLR_DEFAULT)? RGB(10,20,200) : m_crBarClr;
    COLORREF crBgColour = (m_crBgClr == CLR_DEFAULT)? ::GetSysColor(COLOR_MENU) : m_crBgClr;
	COLORREF crTextColor = (m_crTextClr == CLR_DEFAULT)? ::GetSysColor(COLOR_BTNTEXT)  : m_crTextClr;
	if (bSelected)
	{
		if (m_crSelBarClr != CLR_DEFAULT)
		{
			crBarColour = m_crSelBarClr;
			bInvert = false;
		}
		if (m_crSelBgClr != CLR_DEFAULT)
		{
			crBgColour = m_crSelBgClr;
			bInvert = false;
		}
		if (m_crSelTextClr != CLR_DEFAULT)
		{
			crTextColor = m_crSelTextClr;
			bInvert = false;
		}
	}
    double Fraction = (double)(m_nPos - m_nMin) / ((double)(m_nMax - m_nMin));

	CDC &dc = *PaintDC;
	dc.Rectangle(rcClient.left+1,rcClient.top+1,rcClient.right-1,rcClient.bottom-1);
	rcClient.DeflateRect(2,2);
    CRect LeftRect(rcClient);
	CRect RightRect(rcClient);

    LeftRect.right = LeftRect.left + (int)((LeftRect.right - LeftRect.left)*Fraction);
    RightRect.left = LeftRect.right;
    dc.FillSolidRect(LeftRect, crBarColour);
    dc.FillSolidRect(RightRect, crBgColour);
    // Draw Text if not vertical
    if (m_bShowText)
    {
        CString str;
        if (m_strText.GetLength())
            str = m_strText;
        else
            str.Format(_T("%d%%"), (int)(Fraction*100.0));
		CSize sz = dc.GetTextExtent(str);
		int nTextPos = 0;
		if (rcClient.Width() > sz.cx)
			nTextPos = (rcClient.Width() - sz.cx) / 2;
		if (!bSelected && LeftRect.Width() >= nTextPos) 
		{
			dc.SetTextColor(RGB(255,255,255));
		}
		else
			dc.SetTextColor(crTextColor);
        dc.SetBkMode(TRANSPARENT);
        DWORD dwTextStyle = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
        dc.DrawText(str, rcClient, dwTextStyle);
    }
	if (bInvert)
	{
		dc.InvertRect(rcClient);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrl operations

void CTextProgressCtrl::SetShowText(BOOL bShow)
{ 
    m_bShowText = bShow;
}

void CTextProgressCtrl::SetRange(int nLower, int nUpper)
{
    m_nMax = nUpper;
    m_nMin = nLower;
}

void CTextProgressCtrl::GetRange(int& nLower, int& nUpper) const
{
    nUpper = m_nMax;
    nLower = m_nMin;
}

int CTextProgressCtrl::SetPos(int nPos) 
{    
    int nOldPos = m_nPos;
    m_nPos = nPos;
    return nOldPos;
}

int CTextProgressCtrl::StepIt() 
{    
   return SetPos(m_nPos + m_nStepSize);
}

int CTextProgressCtrl::OffsetPos(int nPos)
{
    return SetPos(m_nPos + nPos);
}

int CTextProgressCtrl::SetStep(int nStep)
{
    int nOldStep = m_nStepSize;
    m_nStepSize = nStep;
    return nOldStep;
}

COLORREF CTextProgressCtrl::SetBarColor(COLORREF crBarClr /*= CLR_DEFAULT*/,COLORREF crSelBarClr /*= CLR_DEFAULT*/)
{
    COLORREF crOldBarClr = m_crBarClr;
    m_crBarClr = crBarClr;
    m_crSelBarClr = crSelBarClr;
    return crOldBarClr;
}

COLORREF CTextProgressCtrl::GetBarColor() const
{ 
    return m_crBarClr;
}

COLORREF CTextProgressCtrl::GetSelBarColor() const
{ 
    return m_crSelBarClr;
}

COLORREF CTextProgressCtrl::SetBgColor(COLORREF crBgClr /*= CLR_DEFAULT*/,COLORREF crSelBgClr /*= CLR_DEFAULT*/)
{
    COLORREF crOldBgClr = m_crBgClr;
    m_crBgClr = crBgClr;
	m_crSelBgClr = crSelBgClr;
    return crOldBgClr;
}

COLORREF CTextProgressCtrl::GetBgColor() const
{ 
    return m_crBgClr;
}


COLORREF CTextProgressCtrl::GetSelBgColor() const
{ 
    return m_crSelBgClr;
}

COLORREF CTextProgressCtrl::SetTextColor(COLORREF crTextClr /*= CLR_DEFAULT*/,COLORREF crSelTextClr /*= CLR_DEFAULT*/)
{
    COLORREF crOldTextClr = m_crTextClr;
    m_crTextClr = crTextClr;
	m_crSelTextClr = crSelTextClr;
    return crOldTextClr;
}

COLORREF CTextProgressCtrl::GetTextColor() const
{ 
    return m_crTextClr;
}

COLORREF CTextProgressCtrl::GetSelTextColor() const
{ 
    return m_crSelTextClr;
}
