//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctvppg.cpp
//
//--------------------------------------------------------------------------

// CTVPpg.cpp : Implementation of the CTVPropPage property page class.

#include "stdafx.h"
#include "ctv.h"
#include "CTVPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CTVPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CTVPropPage, COlePropertyPage)
    //{{AFX_MSG_MAP(CTVPropPage)
    // NOTE - ClassWizard will add and remove message map entries
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CTVPropPage, "CTREEVIEW.CTreeViewPropPage.1",
    0xcd6c7869, 0x5864, 0x11d0, 0xab, 0xf0, 0, 0x20, 0xaf, 0x6b, 0xb, 0x7a)


/////////////////////////////////////////////////////////////////////////////
// CTVPropPage::CTVPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CTVPropPage

BOOL CTVPropPage::CTVPropPageFactory::UpdateRegistry(BOOL bRegister)
{
    if (bRegister)
        return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
            m_clsid, IDS_TV_PPG);
    else
        return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CTVPropPage::CTVPropPage - Constructor

CTVPropPage::CTVPropPage() :
    COlePropertyPage(IDD, IDS_TV_PPG_CAPTION)
{
    //{{AFX_DATA_INIT(CTVPropPage)
    // NOTE: ClassWizard will add member initialization here
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CTVPropPage::DoDataExchange - Moves data between page and properties

void CTVPropPage::DoDataExchange(CDataExchange* pDX)
{
    //{{AFX_DATA_MAP(CTVPropPage)
    // NOTE: ClassWizard will add DDP, DDX, and DDV calls here
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA_MAP
    DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CTVPropPage message handlers
