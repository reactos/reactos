// ShellExtensions.cpp : implementation file
//

#include "stdafx.h"
#include "Ext.h"
#include "Extaw.h"
#include "shextdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ShellExtensions dialog


ShellExtensions::ShellExtensions()
	: CAppWizStepDlg(ShellExtensions::IDD)
{
	//{{AFX_DATA_INIT(ShellExtensions)
	m_bContextMenu = FALSE;
	m_bContextMenu2 = FALSE;
	m_bContextMenu3 = FALSE;
	m_bCopyHook = FALSE;
	m_bDataObject = FALSE;
	m_bDragAndDrop = FALSE;
	m_bDropTarget = FALSE;
	m_bIcon = FALSE;
	m_bInfoTip = FALSE;
	m_bPropertySheet = FALSE;
	//}}AFX_DATA_INIT
}


void ShellExtensions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ShellExtensions)
	DDX_Control(pDX, IDC_DND, m_btnDragAndDrop);
	DDX_Control(pDX, IDC_CONTEXTMENU3, m_btnContextMenu3);
//	DDX_Control(pDX, IDC_CONTEXTMENU2, m_btnContextMenu2);
	DDX_Check(pDX, IDC_CONTEXTMENU, m_bContextMenu);
//	DDX_Check(pDX, IDC_CONTEXTMENU2, m_bContextMenu2);
	DDX_Check(pDX, IDC_CONTEXTMENU3, m_bContextMenu3);
	DDX_Check(pDX, IDC_COPYHOOK, m_bCopyHook);
	DDX_Check(pDX, IDC_DATAOBJECT, m_bDataObject);
	DDX_Check(pDX, IDC_DND, m_bDragAndDrop);
	DDX_Check(pDX, IDC_DROPTARGET, m_bDropTarget);
	DDX_Check(pDX, IDC_ICONHANDLER, m_bIcon);
	DDX_Check(pDX, IDC_INFOTIP, m_bInfoTip);
	DDX_Check(pDX, IDC_PROPERTYSHEET, m_bPropertySheet);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ShellExtensions, CAppWizStepDlg)
	//{{AFX_MSG_MAP(ShellExtensions)
	ON_BN_CLICKED(IDC_CONTEXTMENU, OnContextmenu)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ShellExtensions message handlers

BOOL ShellExtensions::OnDismiss()
{
    UpdateData(TRUE);
    GUID guidTemp;
    WCHAR wszGUID[50];
    BOOL bSomethingSelected = FALSE;

    if (m_bContextMenu)
    {
        Extensionsaw.m_Dictionary[TEXT("IContextMenu")] = TEXT("1");

        if (m_bContextMenu3)
            Extensionsaw.m_Dictionary[TEXT("IContextMenu3")] = TEXT("1");
        else
            Extensionsaw.m_Dictionary.RemoveKey(TEXT("IContextMenu3"));


        if (m_bDragAndDrop)
            Extensionsaw.m_Dictionary[TEXT("DragAndDrop")] = TEXT("1");
        else
            Extensionsaw.m_Dictionary.RemoveKey(TEXT("DragAndDrop"));

        if (SUCCEEDED(CoCreateGuid(&guidTemp)))
        {
            StringFromGUID2(guidTemp, wszGUID, ARRAYSIZE(wszGUID));
            Extensionsaw.m_Dictionary[TEXT("ContextMenuGUID")] = StripCurly(wszGUID);
        }

        bSomethingSelected = TRUE;
    }
    else
    {
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("IContextMenu"));
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("IContextMenu3"));
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("DragAndDrop"));
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("ContextMenuGUID"));
    }

    
    if (m_bIcon)
    {
        Extensionsaw.m_Dictionary[TEXT("Icon")] = TEXT("1");
        if (SUCCEEDED(CoCreateGuid(&guidTemp)))
        {
            StringFromGUID2(guidTemp, wszGUID, ARRAYSIZE(wszGUID));
            Extensionsaw.m_Dictionary[TEXT("IconGUID")] = StripCurly(wszGUID);
        }
        bSomethingSelected = TRUE;
    }
    else
    {
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("Icon"));
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("IconGUID"));
    }


    if (m_bPropertySheet)
    {
        Extensionsaw.m_Dictionary[TEXT("PropertySheet")] = TEXT("1");
        if (SUCCEEDED(CoCreateGuid(&guidTemp)))
        {
            StringFromGUID2(guidTemp, wszGUID, ARRAYSIZE(wszGUID));
            Extensionsaw.m_Dictionary[TEXT("PropertySheetGUID")] = StripCurly(wszGUID);
        }
        bSomethingSelected = TRUE;
    }
    else
    {
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("PropertySheet"));
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("PropertySheetGUID"));
    }

    if (m_bInfoTip)
    {
        Extensionsaw.m_Dictionary[TEXT("InfoTip")] = TEXT("1");
        if (SUCCEEDED(CoCreateGuid(&guidTemp)))
        {
            StringFromGUID2(guidTemp, wszGUID, ARRAYSIZE(wszGUID));
            Extensionsaw.m_Dictionary[TEXT("InfoTipGUID")] = StripCurly(wszGUID);
        }
        bSomethingSelected = TRUE;
    }
    else
    {
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("InfoTip"));
        Extensionsaw.m_Dictionary.RemoveKey(TEXT("InfoTipGUID"));
    }
#if 0
    Extensionsaw.m_Dictionary[TEXT("CopyHook")] = m_bCopyHook? TEXT("TRUE") : TEXT("FALSE");
    Extensionsaw.m_Dictionary[TEXT("DataObject")] = m_bDataObject? TEXT("TRUE") : TEXT("FALSE");
    Extensionsaw.m_Dictionary[TEXT("DropTarget")] = m_bDropTarget? TEXT("TRUE") : TEXT("FALSE");
    Extensionsaw.m_Dictionary[TEXT("InfoTip")] = m_bInfoTip? TEXT("TRUE") : TEXT("FALSE");
#endif

    return bSomethingSelected;
;
}

void ShellExtensions::OnContextmenu() 
{
    UpdateData(TRUE);
    m_btnContextMenu2.EnableWindow(m_bContextMenu);
    m_btnContextMenu3.EnableWindow(m_bContextMenu);
    m_btnDragAndDrop.EnableWindow(m_bContextMenu);
}
