#if !defined(AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_)
#define AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// TextProgressCtrl.h : header file
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
// computer, causes your pet cat to fall ill, increases baldness or
// makes you car start emitting strange noises when you start it up.
//
// Expect bugs.
// 
// Please use and enjoy. Please let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into this
// file. 
#include "UIData.h"
/////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrl window

class CTextProgressCtrl : public CUIODColumnCtrl
{
	DECLARE_DYNAMIC(CTextProgressCtrl)
// Construction
public:
	CTextProgressCtrl();

// Attributes
public:

// Operations
public:
    int  SetPos(int nPos);
    int  StepIt();
    void SetRange(int nLower, int nUpper);
    void GetRange(int& nLower, int& nUpper) const;
    int  OffsetPos(int nPos);
    int  SetStep(int nStep);
// Attributes
    void SetShowText(BOOL bShow);
    COLORREF SetTextColor(COLORREF crTextClr = CLR_DEFAULT,COLORREF crSelTextClr=CLR_DEFAULT);
    COLORREF GetTextColor() const;
    COLORREF GetSelTextColor() const;
    COLORREF SetBarColor(COLORREF crBarClr = CLR_DEFAULT,COLORREF crSelBarClr = CLR_DEFAULT);
    COLORREF GetBarColor() const;
    COLORREF GetSelBarColor() const;
    COLORREF SetBgColor(COLORREF crBgClr = CLR_DEFAULT,COLORREF crSelBgClr = CLR_DEFAULT);
    COLORREF GetBgColor() const;
    COLORREF GetSelBgColor() const;
public:
// Overrides
	virtual void DoPaint(CDC *PaintDC,CRect rcClient,bool bSelected);

// Implementation
public:
	virtual ~CTextProgressCtrl();

	// Generated message map functions
protected:
    int      m_nPos, 
             m_nStepSize, 
             m_nMax, 
             m_nMin;
    CString  m_strText;
    BOOL     m_bShowText;
    int      m_nBarWidth;
private:
    COLORREF m_crBarClr,
			 m_crSelBarClr,
			 m_crTextClr,
			 m_crSelTextClr,
             m_crBgClr,
             m_crSelBgClr;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_)
