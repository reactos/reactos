// NumEdit.cpp : implementation file
//

#include "stdafx.h"
#include "magnify.h"
#include "NumEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNumberEdit

CNumberEdit::CNumberEdit()
{
}

CNumberEdit::~CNumberEdit()
{
}


BEGIN_MESSAGE_MAP(CNumberEdit, CEdit)
	//{{AFX_MSG_MAP(CNumberEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumberEdit message handlers

void CNumberEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if(nChar >= '1' && nChar <= '9')	
	{
		TCHAR sz[2];
		sz[0] = (TCHAR)nChar;
		sz[1] = 0;
		SetWindowText(sz);
	}
//	CEdit::OnChar(nChar, nRepCnt, nFlags);
}
