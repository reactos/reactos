// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  HTML.H
//
//  Knows how to wrap the HTML control to expose graphics, tables, and links
//
// --------------------------------------------------------------------------


//
// DUPLICATED IN THE INET\OHARE PROJECT
//

// FOR ACCESSIBILITY
typedef struct tagAXAINFO
{
    DWORD   cbSize;

    LONG    lFlags;
    BOOL    fAnimated:1;
    
    UINT    eleType;
    HWND    hwndEle;

    RECT    rcItem;
    LPSTR   lpNameText;
    UINT    cchNameTextMax;
    LPSTR   lpValueText;
    UINT    cchValueTextMax;
} AXAINFO, * LPAXAINFO;


// eleTypes
#define ELE_NOT			    0
#define ELE_TEXT		    1
#define ELE_IMAGE		    2
#define ELE_VERTICALTAB	    3
#define ELE_HR			    4
#define ELE_NEWLINE         5
#define ELE_BEGINLIST	    6
#define ELE_ENDLIST		    7
#define ELE_LISTITEM	    8
#define ELE_EDIT		    9
#define ELE_PASSWORD	    10
#define ELE_CHECKBOX	    11
#define ELE_RADIO		    12
#define ELE_SUBMIT		    13
#define ELE_RESET		    14
#define ELE_COMBO		    15
#define ELE_LIST		    16
#define ELE_TEXTAREA	    17
#define ELE_INDENT		    18
#define ELE_OUTDENT		    19
#define ELE_BEGINFORM	    20
#define ELE_ENDFORM		    21
#define ELE_MULTILIST	    22
#define ELE_HIDDEN		    23
#define ELE_TAB			    24
#define ELE_OPENLISTITEM	25
#define ELE_CLOSELISTITEM	26
#define ELE_FORMIMAGE		27
#ifdef UNIX
#define ELE_BULLET			28
#endif
#define ELE_BEGINBLOCKQUOTE	28
#define ELE_ENDBLOCKQUOTE	29
#define ELE_FETCH			30
#define ELE_MARQUEE			31
#define ELE_BGSOUND			32
#define ELE_FRAME			33	// enclosing frame used for tables and table cells
#define ELE_OBJECT			34
#define ELE_SUBSCRIPT		35
#define ELE_SUPSCRIPT		36
#define ELE_ALIAS			37
#define ELE_EMBED			38
#define ELE_PARAM			39
#define	ELE_EVENT			40
#define ELE_FRAMESET		41
#define ELE_BUTTON          42
#define ELE_STYLESHEET		43	// Link to a stylesheet - needed for downloading
#define ELE_ENDDOC		    -1		/* Flags end-of-document */


// lFlags
#define ELEFLAG_VISITED             0x00000001
#define ELEFLAG_ANCHOR              0x00000002
#define ELEFLAG_NAME                0x00000004
#define ELEFLAG_IMAGEMAP            0x00000008
#define ELEFLAG_USEMAP              0x00000010
#define ELEFLAG_CENTER              0x00000020
#define ELEFLAG_HR_NOSHADE          0x00000040 

#define ELEFLAG_HR_PERCENT          0x00000080
#define ELEFLAG_PERCENT_WIDTH       0x00000080

#define ELEFLAG_NOBREAK             0x00000100
#define ELEFLAG_BACKGROUND_IMAGE    0x00000200
#define ELEFLAG_WBR                 0x00000400
#define ELEFLAG_MARQUEE_PERCENT     0x00000800
#define ELEFLAG_PERCENT_HEIGHT      0x00001000 
#define ELEFLAG_HIDDEN              0x00002000
#define ELEFLAG_CELLTEXT            0x00004000
#define ELEFLAG_ANCHORNOCACHE       0x00008000
#define ELEFLAG_FULL_PANE_VRML_FIRST_LOAD 0x00010000,
#define ELEFLAG_LEFT                0x00020000
#define ELEFLAG_RIGHT               0x00040000


//
// Because map items don't live in the element stream, we need to do something
// special for IDs.  So the LOW 24 bits is the element index, the HIGH 8
// bits is the map area index.
//
#define MAKE_AXA_INDEX(nElement, nArea)\
    (((nElement) & 0x00FFFFFF) | (((nArea) & 0x000000FF) << 24))

#define GET_AXA_ELEINDEX(lIndex)\
    ((lIndex) & 0x00FFFFFF)

#define GET_AXA_MAPINDEX(lIndex)\
    ((((lIndex) & 0xFF000000) >> 24) & 0x000000FF)


// Gets back the number of items on the page
#define WM_HTML_GETAXACHILDCOUNT        (WM_USER+344)   // wParam=item index+1, lParam=NOTUSED, RetVal=# of items

// Gets back information about a particular link
#define WM_HTML_GETAXAINFO         (WM_USER+345)   // wParam=item index+1, lParam=LPAXAINFO, RetVal=TRUE if success

// Gets back the currently focused item (index+1) if there is one
#define WM_HTML_GETAXAFOCUS             (WM_USER+346)   // wParam=NOTUSED, lParam=NOTUSED, RetVal=index+1 of item w/ focus

// Gets back the hittest item (index+1) if there is one
#define WM_HTML_GETAXAHITTEST           (WM_USER+347)   // wParam=NOTUSED, lParam=PtClient, RetVal=index+1

// Gets back the next or previous item after index+1
#define WM_HTML_GETAXANEXTITEM          (WM_USER+348)   // wParam=item index+1, lParam=BOOL fForward

// Do focus/default action for item (index+1)
#define WM_HTML_DOAXAACTION             (WM_USER+349)   // wParam=item index+1, lParam=BOOL fFocus

class CHtml32 : public CClient
{
    public:
        // IAccessible
        STDMETHODIMP        get_accChildCount(long*);
        STDMETHODIMP        get_accChild(VARIANT, IDispatch**);

        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP        get_accState(VARIANT, VARIANT*);

        STDMETHODIMP        get_accFocus(VARIANT*);
        STDMETHODIMP        get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP        accSelect(long, VARIANT);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);
        STDMETHODIMP        accDoDefaultAction(VARIANT);

        // IEnumVARIANT
        STDMETHODIMP        Next(ULONG, VARIANT*, ULONG*);
        STDMETHODIMP        Skip(ULONG);
        STDMETHODIMP        Reset(void);

        BOOL    ValidateChild(VARIANT*);
        void    SetupChildren(void);

        BOOL    GetAxaInfo(long, LPAXAINFO);
        BOOL    IsWhiteSpace(UINT);

        CHtml32(HWND, long);
};



class CImageMap : public CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP        get_accParent(IDispatch** ppdisp);

        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accValue(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP        get_accState(VARIANT, VARIANT*);

        STDMETHODIMP        get_accFocus(VARIANT*);
        STDMETHODIMP        get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP        accSelect(long, VARIANT);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);
        STDMETHODIMP        accDoDefaultAction(VARIANT);

        // IEnumVARIANT
        STDMETHODIMP        Clone(IEnumVARIANT** ppenum);

        CImageMap(HWND, CHtml32*, int, long);
        ~CImageMap();

    protected:
        CHtml32* m_phtmlParent;
        int      m_ielMap;
};



HRESULT CreateImageMap(HWND hwnd, CHtml32* phtmlParent, int iElMap, long iChildCur,
    REFIID riid, void** ppvImage);


