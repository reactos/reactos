// colorlis.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// CColorMenu window

class CColorMenu : public CMenu
{
    // The following structure is used for accessibility.  Accessibility tools
    // use it to get a descriptive string out of an owner-draw menu.  This
    // stuff will probably be put in a system header someday.

#define MSAA_MENU_SIG 0xAA0DF00DL

    // Menu's dwItemData should point to one of these structs:
    // (or can point to an app-defined struct containing this as the first 
    // member)
    typedef struct tagMSAAMENUINFO {
        DWORD   dwMSAASignature; // Must be MSAA_MENU_SIG
        DWORD   cchWText;        // Length of text in chars
        LPWSTR  pszWText;        // NUL-terminated text, in Unicode
    } MSAAMENUINFO, *LPMSAAMENUINFO;

    // Private struct to add the color index in

    struct MenuInfo
    {
        MSAAMENUINFO    msaa;
        int             index;
    };

// Construction
public:
	CColorMenu();

// Attributes
public:
    static MenuInfo m_menuInfo[17];
 
	static COLORREF GetColor(UINT id);

// Operations
public:

// Implementation
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);

};

/////////////////////////////////////////////////////////////////////////////
