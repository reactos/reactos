//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1995 - 1996 Microsoft Corporation. All Rights Reserved.
//
//  File: oleacc.h
//
//--------------------------------------------------------------------------
#ifndef _OLEACC_H_
#define _OLEACC_H_


// PROPERTIES:  Hierarchical
#define DISPID_ACC_PARENT                   (-5000)
#define DISPID_ACC_CHILDCOUNT               (-5001)
#define DISPID_ACC_CHILD                    (-5002)
                                             
// PROPERTIES:  Descriptional
#define DISPID_ACC_NAME                     (-5003)
#define DISPID_ACC_VALUE                    (-5004)
#define DISPID_ACC_DESCRIPTION              (-5005)
#define DISPID_ACC_ROLE                     (-5006)
#define DISPID_ACC_STATE                    (-5007)
#define DISPID_ACC_HELP                     (-5008)
#define DISPID_ACC_HELPTOPIC                (-5009)
#define DISPID_ACC_KEYBOARDSHORTCUT         (-5010)
#define DISPID_ACC_FOCUS                    (-5011)
#define DISPID_ACC_SELECTION                (-5012)
#define DISPID_ACC_DEFAULTACTION            (-5013)

// METHODS
#define DISPID_ACC_SELECT                   (-5014)
#define DISPID_ACC_LOCATION                 (-5015)
#define DISPID_ACC_NAVIGATE                 (-5016)
#define DISPID_ACC_HITTEST                  (-5017)
#define DISPID_ACC_DODEFAULTACTION          (-5018)



#ifndef __MKTYPLIB__

//  CONSTANTS

//
// Input to DISPID_ACC_NAVIGATE
//
#define NAVDIR_MIN                      0x00000000
#define NAVDIR_UP                       0x00000001
#define NAVDIR_DOWN                     0x00000002
#define NAVDIR_LEFT                     0x00000003   
#define NAVDIR_RIGHT                    0x00000004
#define NAVDIR_NEXT                     0x00000005
#define NAVDIR_PREVIOUS                 0x00000006
#define NAVDIR_FIRSTCHILD               0x00000007
#define NAVDIR_LASTCHILD                0x00000008
#define NAVDIR_MAX                      0x00000009

// Input to DISPID_ACC_SELECT
#define SELFLAG_NONE                    0x00000000
#define SELFLAG_TAKEFOCUS               0x00000001
#define SELFLAG_TAKESELECTION           0x00000002
#define SELFLAG_EXTENDSELECTION         0x00000004
#define SELFLAG_ADDSELECTION            0x00000008
#define SELFLAG_REMOVESELECTION         0x00000010
#define SELFLAG_VALID                   0x0000001F

// Output from DISPID_ACC_STATE
#define STATE_SYSTEM_UNAVAILABLE        0x00000001  // Disabled
#define STATE_SYSTEM_SELECTED           0x00000002
#define STATE_SYSTEM_FOCUSED            0x00000004
#define STATE_SYSTEM_PRESSED            0x00000008
#define STATE_SYSTEM_CHECKED            0x00000010
#define STATE_SYSTEM_MIXED              0x00000020  // 3-state checkbox or toolbar button
#define STATE_SYSTEM_READONLY           0x00000040
#define STATE_SYSTEM_HOTTRACKED         0x00000080
#define STATE_SYSTEM_DEFAULT            0x00000100
#define STATE_SYSTEM_EXPANDED           0x00000200
#define STATE_SYSTEM_COLLAPSED          0x00000400
#define STATE_SYSTEM_BUSY               0x00000800
#define STATE_SYSTEM_FLOATING           0x00001000  // Children "owned" not "contained" by parent
#define STATE_SYSTEM_MARQUEED           0x00002000
#define STATE_SYSTEM_ANIMATED           0x00004000
#define STATE_SYSTEM_INVISIBLE          0x00008000
#define STATE_SYSTEM_OFFSCREEN          0x00010000
#define STATE_SYSTEM_SIZEABLE           0x00020000
#define STATE_SYSTEM_MOVEABLE           0x00040000
#define STATE_SYSTEM_SELFVOICING        0x00080000
#define STATE_SYSTEM_FOCUSABLE          0x00100000
#define STATE_SYSTEM_SELECTABLE         0x00200000
#define STATE_SYSTEM_LINKED             0x00400000
#define STATE_SYSTEM_TRAVERSED          0x00800000
#define STATE_SYSTEM_VALID              0x00FFFFFF

// Output from DISPID_ACC_ROLE
#define ROLE_SYSTEM_TITLEBAR            0x00000001
#define ROLE_SYSTEM_MENUBAR             0x00000002
#define ROLE_SYSTEM_SCROLLBAR           0x00000003
#define ROLE_SYSTEM_GRIP                0x00000004
#define ROLE_SYSTEM_SOUND               0x00000005
#define ROLE_SYSTEM_CURSOR              0x00000006
#define ROLE_SYSTEM_CARET               0x00000007
#define ROLE_SYSTEM_ALERT               0x00000008
#define ROLE_SYSTEM_WINDOW              0x00000009
#define ROLE_SYSTEM_CLIENT              0x0000000A
#define ROLE_SYSTEM_MENUPOPUP           0x0000000B
#define ROLE_SYSTEM_MENUITEM            0x0000000C
#define ROLE_SYSTEM_TOOLTIP             0x0000000D
#define ROLE_SYSTEM_APPLICATION         0x0000000E
#define ROLE_SYSTEM_DOCUMENT            0x0000000F
#define ROLE_SYSTEM_PANE                0x00000010
#define ROLE_SYSTEM_CHART               0x00000011
#define ROLE_SYSTEM_DIALOG              0x00000012
#define ROLE_SYSTEM_BORDER              0x00000013
#define ROLE_SYSTEM_GROUPING            0x00000014
#define ROLE_SYSTEM_SEPARATOR           0x00000015
#define ROLE_SYSTEM_TOOLBAR             0x00000016
#define ROLE_SYSTEM_STATUSBAR           0x00000017
#define ROLE_SYSTEM_TABLE               0x00000018
#define ROLE_SYSTEM_COLUMNHEADER        0x00000019
#define ROLE_SYSTEM_ROWHEADER           0x0000001A
#define ROLE_SYSTEM_COLUMN              0x0000001B
#define ROLE_SYSTEM_ROW                 0x0000001C
#define ROLE_SYSTEM_CELL                0x0000001D
#define ROLE_SYSTEM_LINK                0x0000001E
#define ROLE_SYSTEM_HELPBALLOON         0x0000001F
#define ROLE_SYSTEM_CHARACTER           0x00000020
#define ROLE_SYSTEM_LIST                0x00000021
#define ROLE_SYSTEM_LISTITEM            0x00000022
#define ROLE_SYSTEM_OUTLINE             0x00000023
#define ROLE_SYSTEM_OUTLINEITEM         0x00000024
#define ROLE_SYSTEM_PAGETAB             0x00000025
#define ROLE_SYSTEM_PROPERTYPAGE        0x00000026
#define ROLE_SYSTEM_INDICATOR           0x00000027
#define ROLE_SYSTEM_GRAPHIC             0x00000028
#define ROLE_SYSTEM_STATICTEXT          0x00000029
#define ROLE_SYSTEM_TEXT                0x0000002A  // Editable, selectable, etc.
#define ROLE_SYSTEM_PUSHBUTTON          0x0000002B
#define ROLE_SYSTEM_CHECKBUTTON         0x0000002C
#define ROLE_SYSTEM_RADIOBUTTON         0x0000002D
#define ROLE_SYSTEM_COMBOBOX            0x0000002E
#define ROLE_SYSTEM_DROPLIST            0x0000002F
#define ROLE_SYSTEM_PROGRESSBAR         0x00000030
#define ROLE_SYSTEM_DIAL                0x00000031
#define ROLE_SYSTEM_HOTKEYFIELD         0x00000032
#define ROLE_SYSTEM_SLIDER              0x00000033
#define ROLE_SYSTEM_SPINBUTTON          0x00000034
#define ROLE_SYSTEM_DIAGRAM             0x00000035
#define ROLE_SYSTEM_ANIMATION           0x00000036
#define ROLE_SYSTEM_EQUATION            0x00000037
#define ROLE_SYSTEM_BUTTONDROPDOWN      0x00000038
#define ROLE_SYSTEM_BUTTONMENU          0x00000039
#define ROLE_SYSTEM_BUTTONDROPDOWNGRID  0x0000003A
#define ROLE_SYSTEM_WHITESPACE          0x0000003B
#define ROLE_SYSTEM_PAGETABLIST         0x0000003C
#define ROLE_SYSTEM_CLOCK               0x0000003D


////////////////////////////////////////////////////////////////////////////
//  IAccessible definition
//
//  #define NO_ACCESSIBLE_INTERFACE if you need to include multiple header
//  files that define the IAccessible interface.      

#ifdef NO_ACCESSIBLE_INTERFACE

interface IAccessible;

#else

#undef INTERFACE
#define INTERFACE   IAccessible

DECLARE_INTERFACE_(IAccessible, IDispatch)
{
#ifndef NO_BASEINTERFACE_FUNCS
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;
    STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo) PURE;
    STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames,
        LCID lcid, DISPID FAR* rgdispid) PURE;
    STDMETHOD(Invoke)(THIS_ DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,
        UINT FAR* puArgErr) PURE;
#endif

    STDMETHOD(get_accParent)(THIS_ IDispatch * FAR* ppdispParent) PURE;
    STDMETHOD(get_accChildCount)(THIS_ long FAR* pChildCount) PURE;
    STDMETHOD(get_accChild)(THIS_ VARIANT varChildIndex, IDispatch * FAR* ppdispChild) PURE;

    STDMETHOD(get_accName)(THIS_ VARIANT varChild, BSTR* pszName) PURE;
    STDMETHOD(get_accValue)(THIS_ VARIANT varChild, BSTR* pszValue) PURE;
    STDMETHOD(get_accDescription)(THIS_ VARIANT varChild, BSTR FAR* pszDescription) PURE;
    STDMETHOD(get_accRole)(THIS_ VARIANT varChild, VARIANT *pvarRole) PURE;
    STDMETHOD(get_accState)(THIS_ VARIANT varChild, VARIANT *pvarState) PURE;
    STDMETHOD(get_accHelp)(THIS_ VARIANT varChild, BSTR* pszHelp) PURE;
    STDMETHOD(get_accHelpTopic)(THIS_ BSTR* pszHelpFile, VARIANT varChild, long* pidTopic) PURE;
    STDMETHOD(get_accKeyboardShortcut)(THIS_ VARIANT varChild, BSTR* pszKeyboardShortcut) PURE;
    STDMETHOD(get_accFocus)(THIS_ VARIANT FAR * pvarFocusChild) PURE;
    STDMETHOD(get_accSelection)(THIS_ VARIANT FAR * pvarSelectedChildren) PURE;
    STDMETHOD(get_accDefaultAction)(THIS_ VARIANT varChild, BSTR* pszDefaultAction) PURE;

    STDMETHOD(accSelect)(THIS_ long flagsSelect, VARIANT varChild) PURE;
    STDMETHOD(accLocation)(THIS_ long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) PURE;
    STDMETHOD(accNavigate)(THIS_ long navDir, VARIANT varStart, VARIANT * pvarEndUpAt) PURE;
    STDMETHOD(accHitTest)(THIS_ long xLeft, long yTop, VARIANT * pvarChildAtPoint) PURE;
    STDMETHOD(accDoDefaultAction)(THIS_ VARIANT varChild) PURE;

    STDMETHOD(put_accName)(THIS_ VARIANT varChild, BSTR szName) PURE;
    STDMETHOD(put_accValue)(THIS_ VARIANT varChild, BSTR pszValue) PURE;
};
#endif // NO_ACCESSIBLE_INTERFACE

typedef IAccessible* LPACCESSIBLE;



////////////////////////////////////////////////////////////////////////////
//  GUIDs (these GUIDs can be linked to from OLEACC.LIB)
EXTERN_C const GUID     LIBID_Accessibility;
EXTERN_C const IID      IID_IAccessible;


////////////////////////////////////////////////////////////////////////////
//  Types to help dynamic binding to OLEACC.DLL
typedef LRESULT (STDAPICALLTYPE *LPFNLRESULTFROMOBJECT)(REFIID riid, WPARAM wParam, LPUNKNOWN punk);
typedef HRESULT (STDAPICALLTYPE *LPFNOBJECTFROMLRESULT)(LRESULT lResult, REFIID riid, WPARAM wParam, void** ppvObject);
typedef HRESULT (STDAPICALLTYPE *LPFNACCESSIBLEOBJECTFROMWINDOW)(HWND hwnd, DWORD dwId, REFIID riid, void** ppvObject);
typedef HRESULT (STDAPICALLTYPE *LPFNACCESSIBLEOBJECTFROMPOINT)(POINT ptScreen, IAccessible** ppacc, VARIANT* pvarChild);
typedef HRESULT (STDAPICALLTYPE *LPFNCREATESTDACCESSIBLEOBJECT)(HWND hwnd, LONG idObject, REFIID riid, void** ppvObject);


////////////////////////////////////////////////////////////////////////////
//  Prototypes
STDAPI_(LRESULT) LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN punk);
STDAPI          ObjectFromLresult(LRESULT lResult, REFIID riid, WPARAM wParam, void** ppvObject);

STDAPI          AccessibleObjectFromWindow(HWND hwnd, DWORD dwId, REFIID riid, void **ppvObject);
STDAPI          AccessibleObjectFromPoint(POINT ptScreen, IAccessible ** ppoleAcc,
                    VARIANT * pvarElement);

STDAPI          CreateStdAccessibleObject(HWND hwnd, LONG idObject, REFIID riid, void** ppvObject);


STDAPI_(UINT)   GetRoleTextA(DWORD lRole, LPSTR lpszRole, UINT cchRoleMax);
STDAPI_(UINT)   GetRoleTextW(DWORD lRole, LPWSTR lpszRole, UINT cchRoleMax);
#ifdef UNICODE
#define GetRoleText     GetRoleTextW
#else
#define GetRoleText     GetRoleTextA
#endif // UNICODE

STDAPI_(UINT)   GetStateTextA(DWORD lStateBit, LPSTR lpszState, UINT cchState);
STDAPI_(UINT)   GetStateTextW(DWORD lStateBit, LPWSTR lpszState, UINT cchState);
#ifdef UNICODE
#define GetStateText    GetStateTextW
#else
#define GetStateText    GetStateTextA
#endif // UNICODE

#endif // !__MKTYPLIB__

#endif // _OLEACC_H_
