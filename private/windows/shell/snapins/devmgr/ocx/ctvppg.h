//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctvppg.h
//
//--------------------------------------------------------------------------

// CTVPpg.h : Declaration of the CTVPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CTVPropPage : See CTVPpg.cpp.cpp for implementation.

class CTVPropPage : public COlePropertyPage
{
    DECLARE_DYNCREATE(CTVPropPage)
    DECLARE_OLECREATE_EX(CTVPropPage)

// Constructor
public:
    CTVPropPage();

// Dialog Data
    //{{AFX_DATA(CTVPropPage)
    enum { IDD = IDD_PROPPAGE_TV };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
    //{{AFX_MSG(CTVPropPage)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
