// ExtensionChoice.cpp : implementation file
//

#include "stdafx.h"
#include "Ext.h"
#include "Extaw.h"
#include "Extdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ExtensionChoice dialog


ExtensionChoice::ExtensionChoice()
	: CAppWizStepDlg(ExtensionChoice::IDD)
{
	//{{AFX_DATA_INIT(ExtensionChoice)
	m_strClassDescription = _T("");
	m_strClassType = _T("");
	m_strFileExt = _T("");
	//}}AFX_DATA_INIT
}


void ExtensionChoice::DoDataExchange(CDataExchange* pDX)
{
	CAppWizStepDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ExtensionChoice)
	DDX_Control(pDX, IDC_EXT_EDIT, m_edtExt);
	DDX_Text(pDX, IDC_CLASSDESC_EDIT, m_strClassDescription);
	DDX_Text(pDX, IDC_CLASSTYPE_EDIT, m_strClassType);
	DDX_Text(pDX, IDC_EXT_EDIT, m_strFileExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ExtensionChoice, CAppWizStepDlg)
	//{{AFX_MSG_MAP(ExtensionChoice)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ExtensionChoice message handlers

CString StripSpaces(CString& str)
{
    CString str2;
    for(int i = 0; i < str.GetLength(); i++)
    {
        if (str[i] != TEXT(' '))
            str2 += str[i];
    }
    return str2;
}


BOOL ExtensionChoice::OnDismiss()
{
    BOOL bRet = TRUE;
    UpdateData(TRUE);
    CString strWarn;
    if (m_strFileExt.Left(1) == TEXT("."))
    {
        strWarn.LoadString(IDS_FILEWARN);

        bRet = FALSE;
    }

    if (bRet)
    {
        if (!m_strFileExt.IsEmpty() && !m_strClassType.IsEmpty())
        {
            Extensionsaw.m_Dictionary[TEXT("Extension")] = m_strFileExt;
//            Extensionsaw.m_Dictionary[TEXT("Class Type")] = m_strClassType;
            Extensionsaw.m_Dictionary[TEXT("ClassType")] = m_strClassType;
            Extensionsaw.m_Dictionary[TEXT("ClassDescription")] = m_strClassDescription;
        }
        else
        {
            strWarn.LoadString(IDS_BLANKWARN);
            bRet = FALSE;
        }

    }

    if (!bRet)
    {
        CString strProgram((LPCTSTR)IDS_PROGRAM);
        MessageBox(strWarn, strProgram, MB_OK);
        m_edtExt.SetFocus();
    }


    return bRet;

}
