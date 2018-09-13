/*++ BUILD Version: 0002    Increment this if a change has global effects

Copyright (c) 1993-1996, Microsoft Corporation

Module Name:

        oledlg.h

Abstract:

        Include file for the OLE common dialogs.
        The following dialog implementations are provided:
                - Insert Object Dialog
                - Convert Object Dialog
                - Paste Special Dialog
                - Change Icon Dialog
                - Edit Links Dialog
                - Update Links Dialog
                - Change Source Dialog
                - Busy Dialog
                - User Error Message Dialog
                - Object Properties Dialog

--*/

#ifndef _OLEDLG_H_
#define _OLEDLG_H_

#ifndef RC_INVOKED

#ifndef __cplusplus
#define NONAMELESSUNION     // use strict ANSI standard (for DVOBJ.H)
#endif

// syncronize UNICODE options
#if defined(_UNICODE) && !defined(UNICODE)
        #define UNICODE
#endif
#if defined(UNICODE) && !defined(_UNICODE)
        #define _UNICODE
#endif

#ifndef _WINDOWS_
#include <windows.h>
#endif
#ifndef _INC_SHELLAPI
#include <shellapi.h>
#endif
#ifndef _INC_COMMDLG
#include <commdlg.h>
#endif
#ifndef _OLE2_H_
#include <ole2.h>
#endif
#include <string.h>
#include <tchar.h>

#endif // RC_INVOKED

#include <dlgs.h>           // common dialog IDs

// Help Button Identifier
#define IDC_OLEUIHELP                   99

// Insert Object Dialog identifiers
#define IDC_IO_CREATENEW                2100
#define IDC_IO_CREATEFROMFILE           2101
#define IDC_IO_LINKFILE                 2102
#define IDC_IO_OBJECTTYPELIST           2103
#define IDC_IO_DISPLAYASICON            2104
#define IDC_IO_CHANGEICON               2105
#define IDC_IO_FILE                     2106
#define IDC_IO_FILEDISPLAY              2107
#define IDC_IO_RESULTIMAGE              2108
#define IDC_IO_RESULTTEXT               2109
#define IDC_IO_ICONDISPLAY              2110
#define IDC_IO_OBJECTTYPETEXT           2111    //{{NOHELP}}
#define IDC_IO_FILETEXT                 2112    //{{NOHELP}}
#define IDC_IO_FILETYPE                 2113
#define IDC_IO_INSERTCONTROL            2114
#define IDC_IO_ADDCONTROL               2115
#define IDC_IO_CONTROLTYPELIST          2116

// Paste Special Dialog identifiers
#define IDC_PS_PASTE                    500
#define IDC_PS_PASTELINK                501
#define IDC_PS_SOURCETEXT               502
#define IDC_PS_PASTELIST                503     //{{NOHELP}}
#define IDC_PS_PASTELINKLIST            504     //{{NOHELP}}
#define IDC_PS_DISPLAYLIST              505
#define IDC_PS_DISPLAYASICON            506
#define IDC_PS_ICONDISPLAY              507
#define IDC_PS_CHANGEICON               508
#define IDC_PS_RESULTIMAGE              509
#define IDC_PS_RESULTTEXT               510

// Change Icon Dialog identifiers
#define IDC_CI_GROUP                    120     //{{NOHELP}}
#define IDC_CI_CURRENT                  121
#define IDC_CI_CURRENTICON              122
#define IDC_CI_DEFAULT                  123
#define IDC_CI_DEFAULTICON              124
#define IDC_CI_FROMFILE                 125
#define IDC_CI_FROMFILEEDIT             126
#define IDC_CI_ICONLIST                 127
#define IDC_CI_LABEL                    128     //{{NOHELP}
#define IDC_CI_LABELEDIT                129
#define IDC_CI_BROWSE                   130
#define IDC_CI_ICONDISPLAY              131

// Convert Dialog identifiers
#define IDC_CV_OBJECTTYPE               150
#define IDC_CV_DISPLAYASICON            152
#define IDC_CV_CHANGEICON               153
#define IDC_CV_ACTIVATELIST             154
#define IDC_CV_CONVERTTO                155
#define IDC_CV_ACTIVATEAS               156
#define IDC_CV_RESULTTEXT               157
#define IDC_CV_CONVERTLIST              158
#define IDC_CV_ICONDISPLAY              165

// Edit Links Dialog identifiers
#define IDC_EL_CHANGESOURCE             201
#define IDC_EL_AUTOMATIC                202
#define IDC_EL_CANCELLINK               209
#define IDC_EL_UPDATENOW                210
#define IDC_EL_OPENSOURCE               211
#define IDC_EL_MANUAL                   212
#define IDC_EL_LINKSOURCE               216
#define IDC_EL_LINKTYPE                 217
#define IDC_EL_LINKSLISTBOX             206
#define IDC_EL_COL1                     220     //{{NOHELP}}
#define IDC_EL_COL2                     221     //{{NOHELP}}
#define IDC_EL_COL3                     222     //{{NOHELP}}

// Busy dialog identifiers
#define IDC_BZ_RETRY                    600
#define IDC_BZ_ICON                     601
#define IDC_BZ_MESSAGE1                 602     //{{NOHELP}}
#define IDC_BZ_SWITCHTO                 604

// Update Links dialog identifiers
#define IDC_UL_METER                    1029    //{{NOHELP}}
#define IDC_UL_STOP                     1030    //{{NOHELP}}
#define IDC_UL_PERCENT                  1031    //{{NOHELP}}
#define IDC_UL_PROGRESS                 1032    //{{NOHELP}}

// User Prompt dialog identifiers
#define IDC_PU_LINKS                    900     //{{NOHELP}}
#define IDC_PU_TEXT                     901     //{{NOHELP}}
#define IDC_PU_CONVERT                  902     //{{NOHELP}}
#define IDC_PU_ICON                     908     //{{NOHELP}}

// General Properties identifiers
#define IDC_GP_OBJECTNAME               1009
#define IDC_GP_OBJECTTYPE               1010
#define IDC_GP_OBJECTSIZE               1011
#define IDC_GP_CONVERT                  1013
#define IDC_GP_OBJECTICON               1014    //{{NOHELP}}
#define IDC_GP_OBJECTLOCATION           1022

// View Properties identifiers
#define IDC_VP_PERCENT                  1000
#define IDC_VP_CHANGEICON               1001
#define IDC_VP_EDITABLE                 1002
#define IDC_VP_ASICON                   1003
#define IDC_VP_RELATIVE                 1005
#define IDC_VP_SPIN                     1006
#define IDC_VP_SCALETXT                 1034
#define IDC_VP_ICONDISPLAY              1021
#define IDC_VP_RESULTIMAGE              1033

// Link Properties identifiers
#define IDC_LP_OPENSOURCE               1006
#define IDC_LP_UPDATENOW                1007
#define IDC_LP_BREAKLINK                1008
#define IDC_LP_LINKSOURCE               1012
#define IDC_LP_CHANGESOURCE             1015
#define IDC_LP_AUTOMATIC                1016
#define IDC_LP_MANUAL                   1017
#define IDC_LP_DATE                     1018
#define IDC_LP_TIME                     1019

// Dialog Identifiers as passed in Help messages to identify the source.
#define IDD_INSERTOBJECT                1000
#define IDD_CHANGEICON                  1001
#define IDD_CONVERT                     1002
#define IDD_PASTESPECIAL                1003
#define IDD_EDITLINKS                   1004
#define IDD_BUSY                        1006
#define IDD_UPDATELINKS                 1007
#define IDD_CHANGESOURCE                1009
#define IDD_INSERTFILEBROWSE            1010
#define IDD_CHANGEICONBROWSE            1011
#define IDD_CONVERTONLY                 1012
#define IDD_CHANGESOURCE4               1013
#define IDD_GNRLPROPS                   1100
#define IDD_VIEWPROPS                   1101
#define IDD_LINKPROPS                   1102
#define IDD_CONVERT4                    1103
#define IDD_CONVERTONLY4                1104
#define IDD_EDITLINKS4                  1105
#define IDD_GNRLPROPS4                  1106
#define IDD_LINKPROPS4                  1107
#define IDD_PASTESPECIAL4               1108

// The following Dialogs are message dialogs used by OleUIPromptUser API
#define IDD_CANNOTUPDATELINK            1008
#define IDD_LINKSOURCEUNAVAILABLE       1020
#define IDD_SERVERNOTFOUND              1023
#define IDD_OUTOFMEMORY                 1024
#define IDD_SERVERNOTREGW               1021
#define IDD_LINKTYPECHANGEDW            1022
#define IDD_SERVERNOTREGA               1025
#define IDD_LINKTYPECHANGEDA            1026
#ifdef UNICODE
#define IDD_SERVERNOTREG                IDD_SERVERNOTREGW
#define IDD_LINKTYPECHANGED             IDD_LINKTYPECHANGEDW
#else
#define IDD_SERVERNOTREG                IDD_SERVERNOTREGA
#define IDD_LINKTYPECHANGED             IDD_LINKTYPECHANGEDA
#endif

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 8)

// Delimeter used to separate ItemMoniker pieces of a composite moniker
#ifdef _MAC
        #define OLESTDDELIM ":"
#else
        #define OLESTDDELIM TEXT("\\")
#endif

// Hook type used in all structures.
typedef UINT (CALLBACK *LPFNOLEUIHOOK)(HWND, UINT, WPARAM, LPARAM);

// Strings for registered messages
#define SZOLEUI_MSG_HELP                TEXT("OLEUI_MSG_HELP")
#define SZOLEUI_MSG_ENDDIALOG           TEXT("OLEUI_MSG_ENDDIALOG")
#define SZOLEUI_MSG_BROWSE              TEXT("OLEUI_MSG_BROWSE")
#define SZOLEUI_MSG_CHANGEICON          TEXT("OLEUI_MSG_CHANGEICON")
#define SZOLEUI_MSG_CLOSEBUSYDIALOG     TEXT("OLEUI_MSG_CLOSEBUSYDIALOG")
#define SZOLEUI_MSG_CONVERT             TEXT("OLEUI_MSG_CONVERT")
#define SZOLEUI_MSG_CHANGESOURCE        TEXT("OLEUI_MSG_CHANGESOURCE")
#define SZOLEUI_MSG_ADDCONTROL          TEXT("OLEUI_MSG_ADDCONTROL")
#define SZOLEUI_MSG_BROWSE_OFN          TEXT("OLEUI_MSG_BROWSE_OFN")

// Identifiers for SZOLEUI_MSG_BROWSE_OFN (in wParam)
#define ID_BROWSE_CHANGEICON            1
#define ID_BROWSE_INSERTFILE            2
#define ID_BROWSE_ADDCONTROL            3
#define ID_BROWSE_CHANGESOURCE          4

// Standard success/error definitions
#define OLEUI_FALSE                     0
#define OLEUI_SUCCESS                   1     // No error, same as OLEUI_OK
#define OLEUI_OK                        1     // OK button pressed
#define OLEUI_CANCEL                    2     // Cancel button pressed

#define OLEUI_ERR_STANDARDMIN           100
#define OLEUI_ERR_OLEMEMALLOC           100
#define OLEUI_ERR_STRUCTURENULL         101   // Standard field validation
#define OLEUI_ERR_STRUCTUREINVALID      102
#define OLEUI_ERR_CBSTRUCTINCORRECT     103
#define OLEUI_ERR_HWNDOWNERINVALID      104
#define OLEUI_ERR_LPSZCAPTIONINVALID    105
#define OLEUI_ERR_LPFNHOOKINVALID       106
#define OLEUI_ERR_HINSTANCEINVALID      107
#define OLEUI_ERR_LPSZTEMPLATEINVALID   108
#define OLEUI_ERR_HRESOURCEINVALID      109

#define OLEUI_ERR_FINDTEMPLATEFAILURE   110   // Initialization errors
#define OLEUI_ERR_LOADTEMPLATEFAILURE   111
#define OLEUI_ERR_DIALOGFAILURE         112
#define OLEUI_ERR_LOCALMEMALLOC         113
#define OLEUI_ERR_GLOBALMEMALLOC        114
#define OLEUI_ERR_LOADSTRING            115

#define OLEUI_ERR_STANDARDMAX           116  // Start here for specific errors.

// Miscellaneous utility functions.
STDAPI_(BOOL) OleUIAddVerbMenuW(LPOLEOBJECT lpOleObj, LPCWSTR lpszShortType,
        HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
        BOOL bAddConvert, UINT idConvert, HMENU *lphMenu);
STDAPI_(BOOL) OleUIAddVerbMenuA(LPOLEOBJECT lpOleObj, LPCSTR lpszShortType,
        HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
        BOOL bAddConvert, UINT idConvert, HMENU *lphMenu);
#ifdef UNICODE
#define OleUIAddVerbMenu OleUIAddVerbMenuW
#else
#define OleUIAddVerbMenu OleUIAddVerbMenuA
#endif

/////////////////////////////////////////////////////////////////////////////
// INSERT OBJECT DIALOG

typedef struct tagOLEUIINSERTOBJECTW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIINSERTOBJECT.
        CLSID           clsid;          // OUT: Return space for class ID
        LPWSTR          lpszFile;       // IN-OUT: Filename for inserts or links
        UINT            cchFile;        // IN: Size of lpszFile buffer: MAX_PATH
        UINT            cClsidExclude;  // IN: CLSIDs in lpClsidExclude
        LPCLSID         lpClsidExclude; // IN: List of CLSIDs to exclude from listing.

        // Specific to create objects if flags say so
        IID             iid;            // IN: Requested interface on creation.
        DWORD           oleRender;      // IN: Rendering option
        LPFORMATETC     lpFormatEtc;    // IN: Desired format
        LPOLECLIENTSITE lpIOleClientSite;   // IN: Site to be use for the object.
        LPSTORAGE       lpIStorage;     // IN: Storage used for the object
        LPVOID          *ppvObj;        // OUT: Where the object is returned.
        SCODE           sc;             // OUT: Result of creation calls.
        HGLOBAL         hMetaPict;      // OUT: metafile aspect (METAFILEPICT)

} OLEUIINSERTOBJECTW, *POLEUIINSERTOBJECTW, *LPOLEUIINSERTOBJECTW;
typedef struct tagOLEUIINSERTOBJECTA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIINSERTOBJECT.
        CLSID           clsid;          // OUT: Return space for class ID
        LPSTR           lpszFile;       // IN-OUT: Filename for inserts or links
        UINT            cchFile;        // IN: Size of lpszFile buffer: MAX_PATH
        UINT            cClsidExclude;  // IN: CLSIDs in lpClsidExclude
        LPCLSID         lpClsidExclude; // IN: List of CLSIDs to exclude from listing.

        // Specific to create objects if flags say so
        IID             iid;            // IN: Requested interface on creation.
        DWORD           oleRender;      // IN: Rendering option
        LPFORMATETC     lpFormatEtc;    // IN: Desired format
        LPOLECLIENTSITE lpIOleClientSite;   // IN: Site to be use for the object.
        LPSTORAGE       lpIStorage;     // IN: Storage used for the object
        LPVOID          *ppvObj;        // OUT: Where the object is returned.
        SCODE           sc;             // OUT: Result of creation calls.
        HGLOBAL         hMetaPict;      // OUT: metafile aspect (METAFILEPICT)

} OLEUIINSERTOBJECTA, *POLEUIINSERTOBJECTA, *LPOLEUIINSERTOBJECTA;

STDAPI_(UINT) OleUIInsertObjectW(LPOLEUIINSERTOBJECTW);
STDAPI_(UINT) OleUIInsertObjectA(LPOLEUIINSERTOBJECTA);

#ifdef UNICODE
#define tagOLEUIINSERTOBJECT tagOLEUIINSERTOBJECTW
#define OLEUIINSERTOBJECT OLEUIINSERTOBJECTW
#define POLEUIINSERTOBJECT POLEUIINSERTOBJECTW
#define LPOLEUIINSERTOBJECT LPOLEUIINSERTOBJECTW
#define OleUIInsertObject OleUIInsertObjectW
#else
#define tagOLEUIINSERTOBJECT tagOLEUIINSERTOBJECTA
#define OLEUIINSERTOBJECT OLEUIINSERTOBJECTA
#define POLEUIINSERTOBJECT POLEUIINSERTOBJECTA
#define LPOLEUIINSERTOBJECT LPOLEUIINSERTOBJECTA
#define OleUIInsertObject OleUIInsertObjectA
#endif

// Insert Object flags
#define IOF_SHOWHELP                    0x00000001L
#define IOF_SELECTCREATENEW             0x00000002L
#define IOF_SELECTCREATEFROMFILE        0x00000004L
#define IOF_CHECKLINK                   0x00000008L
#define IOF_CHECKDISPLAYASICON          0x00000010L
#define IOF_CREATENEWOBJECT             0x00000020L
#define IOF_CREATEFILEOBJECT            0x00000040L
#define IOF_CREATELINKOBJECT            0x00000080L
#define IOF_DISABLELINK                 0x00000100L
#define IOF_VERIFYSERVERSEXIST          0x00000200L
#define IOF_DISABLEDISPLAYASICON        0x00000400L
#define IOF_HIDECHANGEICON              0x00000800L
#define IOF_SHOWINSERTCONTROL           0x00001000L
#define IOF_SELECTCREATECONTROL         0x00002000L

// Insert Object specific error codes
#define OLEUI_IOERR_LPSZFILEINVALID         (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_IOERR_LPSZLABELINVALID        (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_IOERR_HICONINVALID            (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_IOERR_LPFORMATETCINVALID      (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_IOERR_PPVOBJINVALID           (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_IOERR_LPIOLECLIENTSITEINVALID (OLEUI_ERR_STANDARDMAX+5)
#define OLEUI_IOERR_LPISTORAGEINVALID       (OLEUI_ERR_STANDARDMAX+6)
#define OLEUI_IOERR_SCODEHASERROR           (OLEUI_ERR_STANDARDMAX+7)
#define OLEUI_IOERR_LPCLSIDEXCLUDEINVALID   (OLEUI_ERR_STANDARDMAX+8)
#define OLEUI_IOERR_CCHFILEINVALID          (OLEUI_ERR_STANDARDMAX+9)

/////////////////////////////////////////////////////////////////////////////
// PASTE SPECIAL DIALOG

// The OLEUIPASTEFLAG enumeration is used by the OLEUIPASTEENTRY structure.
//
// OLEUIPASTE_ENABLEICON: If the container does not specify this flag for
//      the entry in the OLEUIPASTEENTRY array passed as input to
//      OleUIPasteSpecial, the DisplayAsIcon button will be unchecked and
//      disabled when the the user selects the format that corresponds to
//      the entry.
//
// OLEUIPASTE_PASTEONLY: Indicates that the entry in the OLEUIPASTEENTRY
//      array is valid for pasting only.
//
// OLEUIPASTE_PASTE: Indicates that the entry in the OLEUIPASTEENTRY array
//      is valid for pasting. It may also be valid for linking if any of
//      the following linking flags are specified.
//
// If the entry in the OLEUIPASTEENTRY array is valid for linking, the
// following flags indicate which link types are acceptable by OR'ing
// together the appropriate OLEUIPASTE_LINKTYPE<#> values.
//
// These values correspond as follows to the array of link types passed to
// OleUIPasteSpecial:
//
//   OLEUIPASTE_LINKTYPE1 = arrLinkTypes[0]
//   OLEUIPASTE_LINKTYPE2 = arrLinkTypes[1]
//   OLEUIPASTE_LINKTYPE3 = arrLinkTypes[2]
//   OLEUIPASTE_LINKTYPE4 = arrLinkTypes[3]
//   OLEUIPASTE_LINKTYPE5 = arrLinkTypes[4]
//   OLEUIPASTE_LINKTYPE6 = arrLinkTypes[5]
//   OLEUIPASTE_LINKTYPE7 = arrLinkTypes[6]
//   OLEUIPASTE_LINKTYPE8 = arrLinkTypes[7]
//
// where,
//   UINT arrLinkTypes[8] is an array of registered clipboard formats for
//   linking. A maximium of 8 link types are allowed.

typedef enum tagOLEUIPASTEFLAG
{
   OLEUIPASTE_ENABLEICON    = 2048,     // enable display as icon
   OLEUIPASTE_PASTEONLY     = 0,
   OLEUIPASTE_PASTE         = 512,
   OLEUIPASTE_LINKANYTYPE   = 1024,
   OLEUIPASTE_LINKTYPE1     = 1,
   OLEUIPASTE_LINKTYPE2     = 2,
   OLEUIPASTE_LINKTYPE3     = 4,
   OLEUIPASTE_LINKTYPE4     = 8,
   OLEUIPASTE_LINKTYPE5     = 16,
   OLEUIPASTE_LINKTYPE6     = 32,
   OLEUIPASTE_LINKTYPE7     = 64,
   OLEUIPASTE_LINKTYPE8     = 128
} OLEUIPASTEFLAG;

// OLEUIPASTEENTRY structure
//
// An array of OLEUIPASTEENTRY entries is specified for the PasteSpecial
// dialog box. Each entry includes a FORMATETC which specifies the
// formats that are acceptable, a string that is to represent the format
// in the  dialog's list box, a string to customize the result text of the
// dialog and a set of flags from the OLEUIPASTEFLAG enumeration.  The
// flags indicate if the entry is valid for pasting only, linking only or
// both pasting and linking.

typedef struct tagOLEUIPASTEENTRYW
{
   FORMATETC        fmtetc;         // Format that is acceptable.
   LPCWSTR          lpstrFormatName;// String that represents the format
                                                                        // to the user. %s is replaced by the
                                                                        // full user type name of the object.
   LPCWSTR          lpstrResultText;// String to customize the result text
                                                                        // of the dialog when the user
                                                                        // selects the format correspoding to
                                                                        // this entry. Any %s in this string
                                                                        // is replaced by the the application
                                                                        // name or FullUserTypeName of the
                                                                        // object on the clipboard.
   DWORD            dwFlags;        // Values from OLEUIPASTEFLAG enum
   DWORD            dwScratchSpace; // Scratch space used internally.

} OLEUIPASTEENTRYW, *POLEUIPASTEENTRYW, *LPOLEUIPASTEENTRYW;
typedef struct tagOLEUIPASTEENTRYA
{
   FORMATETC        fmtetc;         // Format that is acceptable.
   LPCSTR           lpstrFormatName;// String that represents the format
                                                                        // to the user. %s is replaced by the
                                                                        // full user type name of the object.
   LPCSTR           lpstrResultText;// String to customize the result text
                                                                        // of the dialog when the user
                                                                        // selects the format correspoding to
                                                                        // this entry. Any %s in this string
                                                                        // is replaced by the the application
                                                                        // name or FullUserTypeName of the
                                                                        // object on the clipboard.
   DWORD            dwFlags;        // Values from OLEUIPASTEFLAG enum
   DWORD            dwScratchSpace; // Scratch space used internally.

} OLEUIPASTEENTRYA, *POLEUIPASTEENTRYA, *LPOLEUIPASTEENTRYA;
#ifdef UNICODE
#define tagOLEUIPASTEENTRY tagOLEUIPASTEENTRYW
#define OLEUIPASTEENTRY OLEUIPASTEENTRYW
#define POLEUIPASTEENTRY POLEUIPASTEENTRYW
#define LPOLEUIPASTEENTRY LPOLEUIPASTEENTRYW
#else
#define tagOLEUIPASTEENTRY tagOLEUIPASTEENTRYA
#define OLEUIPASTEENTRY OLEUIPASTEENTRYA
#define POLEUIPASTEENTRY POLEUIPASTEENTRYA
#define LPOLEUIPASTEENTRY LPOLEUIPASTEENTRYA
#endif

// Maximum number of link types
#define PS_MAXLINKTYPES  8

typedef struct tagOLEUIPASTESPECIALW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIPASTESPECIAL.
        LPDATAOBJECT    lpSrcDataObj;   // IN-OUT: Source IDataObject* on the clipboard
            // If lpSrcDataObj is NULL when OleUIPasteSpecial is called, then
            // OleUIPasteSpecial will attempt to retrieve a pointer to an
            // IDataObject from the clipboard.  If OleUIPasteSpecial succeeds
            // then it is the caller's responsibility to free the IDataObject
            // returned in lpSrcDataObj.
        LPOLEUIPASTEENTRYW arrPasteEntries;// IN: Array of acceptable formats
        int             cPasteEntries;  // IN: No. of OLEUIPASTEENTRY array entries
        UINT FAR*       arrLinkTypes;   // IN: List of acceptable link types
        int             cLinkTypes;     // IN: Number of link types
        UINT            cClsidExclude;  // IN: Number of CLSIDs in lpClsidExclude
        LPCLSID         lpClsidExclude; // IN: List of CLSIDs to exclude from list.
        int             nSelectedIndex; // OUT: Index that the user selected
        BOOL            fLink;          // OUT: Indicates if Paste or PasteLink
        HGLOBAL         hMetaPict;      // OUT: Handle to Metafile containing icon
        SIZEL           sizel;          // OUT: size of object/link in its source
                                                                        //  may be 0,0 if different display
                                                                        //  aspect is chosen.

} OLEUIPASTESPECIALW, *POLEUIPASTESPECIALW, *LPOLEUIPASTESPECIALW;
typedef struct tagOLEUIPASTESPECIALA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIPASTESPECIAL.
        LPDATAOBJECT    lpSrcDataObj;   // IN-OUT: Source IDataObject* on the clipboard
            // If lpSrcDataObj is NULL when OleUIPasteSpecial is called, then
            // OleUIPasteSpecial will attempt to retrieve a pointer to an
            // IDataObject from the clipboard.  If OleUIPasteSpecial succeeds
            // then it is the caller's responsibility to free the IDataObject
            // returned in lpSrcDataObj.
        LPOLEUIPASTEENTRYA arrPasteEntries;// IN: Array of acceptable formats
        int             cPasteEntries;  // IN: No. of OLEUIPASTEENTRY array entries
        UINT FAR*       arrLinkTypes;   // IN: List of acceptable link types
        int             cLinkTypes;     // IN: Number of link types
        UINT            cClsidExclude;  // IN: Number of CLSIDs in lpClsidExclude
        LPCLSID         lpClsidExclude; // IN: List of CLSIDs to exclude from list.
        int             nSelectedIndex; // OUT: Index that the user selected
        BOOL            fLink;          // OUT: Indicates if Paste or PasteLink
        HGLOBAL         hMetaPict;      // OUT: Handle to Metafile containing icon
        SIZEL           sizel;          // OUT: size of object/link in its source
                                                                        //  may be 0,0 if different display
                                                                        //  aspect is chosen.

} OLEUIPASTESPECIALA, *POLEUIPASTESPECIALA, *LPOLEUIPASTESPECIALA;
#ifdef UNICODE

#define tagOLEUIPASTESPECIAL tagOLEUIPASTESPECIALW
#define OLEUIPASTESPECIAL OLEUIPASTESPECIALW
#define POLEUIPASTESPECIAL POLEUIPASTESPECIALW
#define LPOLEUIPASTESPECIAL LPOLEUIPASTESPECIALW
#else
#define tagOLEUIPASTESPECIAL tagOLEUIPASTESPECIALA
#define OLEUIPASTESPECIAL OLEUIPASTESPECIALA
#define POLEUIPASTESPECIAL POLEUIPASTESPECIALA
#define LPOLEUIPASTESPECIAL LPOLEUIPASTESPECIALA
#endif

STDAPI_(UINT) OleUIPasteSpecialW(LPOLEUIPASTESPECIALW);
STDAPI_(UINT) OleUIPasteSpecialA(LPOLEUIPASTESPECIALA);
#ifdef UNICODE
#define OleUIPasteSpecial OleUIPasteSpecialW
#else
#define OleUIPasteSpecial OleUIPasteSpecialA
#endif

// Paste Special specific flags
#define PSF_SHOWHELP                    0x00000001L
#define PSF_SELECTPASTE                 0x00000002L
#define PSF_SELECTPASTELINK             0x00000004L
// NOTE: PSF_CHECKDISPLAYASICON is strictly an output flag.
//       It is ignored if set when calling OleUIPasteSpecial.
#define PSF_CHECKDISPLAYASICON          0x00000008L
#define PSF_DISABLEDISPLAYASICON        0x00000010L
#define PSF_HIDECHANGEICON              0x00000020L
#define PSF_STAYONCLIPBOARDCHANGE       0x00000040L
#define PSF_NOREFRESHDATAOBJECT         0x00000080L

// Paste Special specific error codes
#define OLEUI_IOERR_SRCDATAOBJECTINVALID    (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_IOERR_ARRPASTEENTRIESINVALID  (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_IOERR_ARRLINKTYPESINVALID     (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_PSERR_CLIPBOARDCHANGED        (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_PSERR_GETCLIPBOARDFAILED      (OLEUI_ERR_STANDARDMAX+4)

/////////////////////////////////////////////////////////////////////////////
// EDIT LINKS DIALOG

// IOleUILinkContainer interface
//
//    This interface must be implemented by container applications that
//    want to use the EditLinks dialog. the EditLinks dialog calls back
//    to the container app to perform the OLE functions to manipulate
//    the links within the container.

#undef  INTERFACE
#define INTERFACE   IOleUILinkContainerW

DECLARE_INTERFACE_(IOleUILinkContainerW, IUnknown)
{
        // *** IUnknown methods *** //
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS) PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;

        // *** IOleUILinkContainer *** //
        STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD dwUpdateOpt) PURE;
        STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD FAR* lpdwUpdateOpt) PURE;
        STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPWSTR lpszDisplayName,
                ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource) PURE;
        STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
                LPWSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
                LPWSTR FAR* lplpszFullLinkType, LPWSTR FAR* lplpszShortLinkType,
                BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected) PURE;
        STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
                BOOL fErrorMessage, BOOL fReserved) PURE;
        STDMETHOD(CancelLink) (THIS_ DWORD dwLink) PURE;
};

typedef IOleUILinkContainerW FAR* LPOLEUILINKCONTAINERW;

#undef  INTERFACE
#define INTERFACE   IOleUILinkContainerA

DECLARE_INTERFACE_(IOleUILinkContainerA, IUnknown)
{
        // *** IUnknown methods *** //
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS) PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;

        // *** IOleUILinkContainer *** //
        STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD dwUpdateOpt) PURE;
        STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD FAR* lpdwUpdateOpt) PURE;
        STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPSTR lpszDisplayName,
                ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource) PURE;
        STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
                LPSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
                LPSTR FAR* lplpszFullLinkType, LPSTR FAR* lplpszShortLinkType,
                BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected) PURE;
        STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
                BOOL fErrorMessage, BOOL fReserved) PURE;
        STDMETHOD(CancelLink) (THIS_ DWORD dwLink) PURE;
};

typedef IOleUILinkContainerA FAR* LPOLEUILINKCONTAINERA;

#ifdef UNICODE
#define IOleUILinkContainer IOleUILinkContainerW
#define IOleUILinkContainerVtbl IOleUILinkContainerWVtbl
#define LPOLEUILINKCONTAINER LPOLEUILINKCONTAINERW
#else
#define IOleUILinkContainer IOleUILinkContainerA
#define IOleUILinkContainerVtbl IOleUILinkContainerAVtbl
#define LPOLEUILINKCONTAINER LPOLEUILINKCONTAINERA
#endif

typedef struct tagOLEUIEDITLINKSW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: Flags
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIEDITLINKS.
        LPOLEUILINKCONTAINERW lpOleUILinkContainer;  // IN: Interface to manipulate
                                                                                                // links in the container

} OLEUIEDITLINKSW, *POLEUIEDITLINKSW, *LPOLEUIEDITLINKSW;

typedef struct tagOLEUIEDITLINKSA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: Flags
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIEDITLINKS.
        LPOLEUILINKCONTAINERA lpOleUILinkContainer;  // IN: Interface to manipulate
                                                                                                // links in the container

} OLEUIEDITLINKSA, *POLEUIEDITLINKSA, *LPOLEUIEDITLINKSA;

#ifdef UNICODE
#define tagOLEUIEDITLINKS tagOLEUIEDITLINKSW
#define OLEUIEDITLINKS OLEUIEDITLINKSW
#define POLEUIEDITLINKS POLEUIEDITLINKSW
#define LPOLEUIEDITLINKS LPOLEUIEDITLINKSW
#else
#define tagOLEUIEDITLINKS tagOLEUIEDITLINKSA
#define OLEUIEDITLINKS OLEUIEDITLINKSA
#define POLEUIEDITLINKS POLEUIEDITLINKSA
#define LPOLEUIEDITLINKS LPOLEUIEDITLINKSA
#endif

#define OLEUI_ELERR_LINKCNTRNULL        (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_ELERR_LINKCNTRINVALID     (OLEUI_ERR_STANDARDMAX+1)

STDAPI_(UINT) OleUIEditLinksW(LPOLEUIEDITLINKSW);
STDAPI_(UINT) OleUIEditLinksA(LPOLEUIEDITLINKSA);

#ifdef UNICODE
#define OleUIEditLinks OleUIEditLinksW
#else
#define OleUIEditLinks OleUIEditLinksA
#endif

// Edit Links flags
#define ELF_SHOWHELP                    0x00000001L
#define ELF_DISABLEUPDATENOW            0x00000002L
#define ELF_DISABLEOPENSOURCE           0x00000004L
#define ELF_DISABLECHANGESOURCE         0x00000008L
#define ELF_DISABLECANCELLINK           0x00000010L

/////////////////////////////////////////////////////////////////////////////
// CHANGE ICON DIALOG

typedef struct tagOLEUICHANGEICONW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUICHANGEICON.
        HGLOBAL         hMetaPict;      // IN-OUT: Current and final image.
                                                                        //  Source of the icon is embedded in
                                                                        //  the metafile itself.
        CLSID           clsid;          // IN: class used to get Default icon
        WCHAR           szIconExe[MAX_PATH];    // IN: explicit icon source path
        int             cchIconExe;     // IN: number of characters in szIconExe

} OLEUICHANGEICONW, *POLEUICHANGEICONW, *LPOLEUICHANGEICONW;

typedef struct tagOLEUICHANGEICONA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUICHANGEICON.
        HGLOBAL         hMetaPict;      // IN-OUT: Current and final image.
                                                                        //  Source of the icon is embedded in
                                                                        //  the metafile itself.
        CLSID           clsid;          // IN: class used to get Default icon
        CHAR            szIconExe[MAX_PATH];    // IN: explicit icon source path
        int             cchIconExe;     // IN: number of characters in szIconExe

} OLEUICHANGEICONA, *POLEUICHANGEICONA, *LPOLEUICHANGEICONA;

STDAPI_(UINT) OleUIChangeIconW(LPOLEUICHANGEICONW);
STDAPI_(UINT) OleUIChangeIconA(LPOLEUICHANGEICONA);

#ifdef UNICODE
#define tagOLEUICHANGEICON tagOLEUICHANGEICONW
#define OLEUICHANGEICON OLEUICHANGEICONW
#define POLEUICHANGEICON POLEUICHANGEICONW
#define LPOLEUICHANGEICON LPOLEUICHANGEICONW
#define OleUIChangeIcon OleUIChangeIconW
#else
#define tagOLEUICHANGEICON tagOLEUICHANGEICONA
#define OLEUICHANGEICON OLEUICHANGEICONA
#define POLEUICHANGEICON POLEUICHANGEICONA
#define LPOLEUICHANGEICON LPOLEUICHANGEICONA
#define OleUIChangeIcon OleUIChangeIconA
#endif

// Change Icon flags
#define CIF_SHOWHELP                    0x00000001L
#define CIF_SELECTCURRENT               0x00000002L
#define CIF_SELECTDEFAULT               0x00000004L
#define CIF_SELECTFROMFILE              0x00000008L
#define CIF_USEICONEXE                  0x00000010L

// Change Icon specific error codes
#define OLEUI_CIERR_MUSTHAVECLSID           (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_CIERR_MUSTHAVECURRENTMETAFILE (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_CIERR_SZICONEXEINVALID        (OLEUI_ERR_STANDARDMAX+2)

// Property used by ChangeIcon dialog to give its parent window access to
// its hDlg. The PasteSpecial dialog may need to force the ChgIcon dialog
// down if the clipboard contents change underneath it. if so it will send
// a IDCANCEL command to the ChangeIcon dialog.
#define PROP_HWND_CHGICONDLG    TEXT("HWND_CIDLG")

/////////////////////////////////////////////////////////////////////////////
// CONVERT DIALOG

typedef struct tagOLEUICONVERTW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUICONVERT.
        CLSID           clsid;          // IN: Class ID sent in to dialog: IN only
        CLSID           clsidConvertDefault;    // IN: use as convert default: IN only
        CLSID           clsidActivateDefault;   // IN: use as activate default: IN only

        CLSID           clsidNew;       // OUT: Selected Class ID
        DWORD           dvAspect;       // IN-OUT: either DVASPECT_CONTENT or
                                                                        //  DVASPECT_ICON
        WORD            wFormat;        // IN" Original data format
        BOOL            fIsLinkedObject;// IN: true if object is linked
        HGLOBAL         hMetaPict;      // IN-OUT: metafile icon image
        LPWSTR          lpszUserType;   // IN-OUT: user type name of original class.
                                                                        //      We'll do lookup if NULL.
                                                                        //      This gets freed on exit.
        BOOL            fObjectsIconChanged; // OUT: TRUE == ChangeIcon was called
        LPWSTR          lpszDefLabel;   //IN-OUT: default label to use for icon.
                                                                        //  if NULL, the short user type name
                                                                        //  will be used. if the object is a
                                                                        //  link, the caller should pass the
                                                                        //  DisplayName of the link source
                                                                        //  This gets freed on exit.

        UINT            cClsidExclude;  //IN: No. of CLSIDs in lpClsidExclude
        LPCLSID         lpClsidExclude; //IN: List of CLSIDs to exclude from list

} OLEUICONVERTW, *POLEUICONVERTW, *LPOLEUICONVERTW;

typedef struct tagOLEUICONVERTA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUICONVERT.
        CLSID           clsid;          // IN: Class ID sent in to dialog: IN only
        CLSID           clsidConvertDefault;    // IN: use as convert default: IN only
        CLSID           clsidActivateDefault;   // IN: use as activate default: IN only

        CLSID           clsidNew;       // OUT: Selected Class ID
        DWORD           dvAspect;       // IN-OUT: either DVASPECT_CONTENT or
                                                                        //  DVASPECT_ICON
        WORD            wFormat;        // IN" Original data format
        BOOL            fIsLinkedObject;// IN: true if object is linked
        HGLOBAL         hMetaPict;      // IN-OUT: metafile icon image
        LPSTR           lpszUserType;   // IN-OUT: user type name of original class.
                                                                        //      We'll do lookup if NULL.
                                                                        //      This gets freed on exit.
        BOOL            fObjectsIconChanged; // OUT: TRUE == ChangeIcon was called
        LPSTR           lpszDefLabel;   //IN-OUT: default label to use for icon.
                                                                        //  if NULL, the short user type name
                                                                        //  will be used. if the object is a
                                                                        //  link, the caller should pass the
                                                                        //  DisplayName of the link source
                                                                        //  This gets freed on exit.

        UINT            cClsidExclude;  //IN: No. of CLSIDs in lpClsidExclude
        LPCLSID         lpClsidExclude; //IN: List of CLSIDs to exclude from list

} OLEUICONVERTA, *POLEUICONVERTA, *LPOLEUICONVERTA;

STDAPI_(UINT) OleUIConvertW(LPOLEUICONVERTW);
STDAPI_(UINT) OleUIConvertA(LPOLEUICONVERTA);

#ifdef UNICODE
#define tagOLEUICONVERT tagOLEUICONVERTW
#define OLEUICONVERT OLEUICONVERTW
#define POLEUICONVERT POLEUICONVERTW
#define LPOLEUICONVERT LPOLEUICONVERTW
#define OleUIConvert OleUIConvertW
#else
#define tagOLEUICONVERT tagOLEUICONVERTA
#define OLEUICONVERT OLEUICONVERTA
#define POLEUICONVERT POLEUICONVERTA
#define LPOLEUICONVERT LPOLEUICONVERTA
#define OleUIConvert OleUIConvertA
#endif

// Determine if there is at least one class that can Convert or ActivateAs
// the given clsid.
STDAPI_(BOOL) OleUICanConvertOrActivateAs(
        REFCLSID rClsid, BOOL fIsLinkedObject, WORD wFormat);

// Convert Dialog flags
#define CF_SHOWHELPBUTTON               0x00000001L
#define CF_SETCONVERTDEFAULT            0x00000002L
#define CF_SETACTIVATEDEFAULT           0x00000004L
#define CF_SELECTCONVERTTO              0x00000008L
#define CF_SELECTACTIVATEAS             0x00000010L
#define CF_DISABLEDISPLAYASICON         0x00000020L
#define CF_DISABLEACTIVATEAS            0x00000040L
#define CF_HIDECHANGEICON               0x00000080L
#define CF_CONVERTONLY                  0x00000100L

// Convert specific error codes
#define OLEUI_CTERR_CLASSIDINVALID      (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_CTERR_DVASPECTINVALID     (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_CTERR_CBFORMATINVALID     (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_CTERR_HMETAPICTINVALID    (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_CTERR_STRINGINVALID       (OLEUI_ERR_STANDARDMAX+5)

/////////////////////////////////////////////////////////////////////////////
// BUSY DIALOG

typedef struct tagOLEUIBUSYW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: see below
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIBUSY.
        HTASK           hTask;          // IN: HTask which is blocking
        HWND *          lphWndDialog;   // OUT: Dialog's HWND is placed here

} OLEUIBUSYW, *POLEUIBUSYW, *LPOLEUIBUSYW;

typedef struct tagOLEUIBUSYA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: see below
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // Specifics for OLEUIBUSY.
        HTASK           hTask;          // IN: HTask which is blocking
        HWND *          lphWndDialog;   // OUT: Dialog's HWND is placed here

} OLEUIBUSYA, *POLEUIBUSYA, *LPOLEUIBUSYA;

STDAPI_(UINT) OleUIBusyW(LPOLEUIBUSYW);
STDAPI_(UINT) OleUIBusyA(LPOLEUIBUSYA);

#ifdef UNICODE
#define tagOLEUIBUSY tagOLEUIBUSYW
#define OLEUIBUSY OLEUIBUSYW
#define POLEUIBUSY POLEUIBUSYW
#define LPOLEUIBUSY LPOLEUIBUSYW
#define OleUIBusy OleUIBusyW
#else
#define tagOLEUIBUSY tagOLEUIBUSYA
#define OLEUIBUSY OLEUIBUSYA
#define POLEUIBUSY POLEUIBUSYA
#define LPOLEUIBUSY LPOLEUIBUSYA
#define OleUIBusy OleUIBusyA
#endif

// Flags for the Busy dialog
#define BZ_DISABLECANCELBUTTON          0x00000001L
#define BZ_DISABLESWITCHTOBUTTON        0x00000002L
#define BZ_DISABLERETRYBUTTON           0x00000004L

#define BZ_NOTRESPONDINGDIALOG          0x00000008L

// Busy specific error/return codes
#define OLEUI_BZERR_HTASKINVALID     (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_BZ_SWITCHTOSELECTED    (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_BZ_RETRYSELECTED       (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_BZ_CALLUNBLOCKED       (OLEUI_ERR_STANDARDMAX+3)

/////////////////////////////////////////////////////////////////////////////
// CHANGE SOURCE DIALOG

// Data to and from the ChangeSource dialog hook
typedef struct tagOLEUICHANGESOURCEW
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCWSTR         lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCWSTR         lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // INTERNAL ONLY: do not modify these members
        OPENFILENAMEW*  lpOFN;          // pointer OPENFILENAME struct
        DWORD           dwReserved1[4]; // (reserved for future use)

        // Specifics for OLEUICHANGESOURCE.
        LPOLEUILINKCONTAINERW lpOleUILinkContainer;  // IN: used to validate link sources
        DWORD           dwLink;         // IN: magic# for lpOleUILinkContainer
        LPWSTR          lpszDisplayName;// IN-OUT: complete source display name
        ULONG           nFileLength;    // IN-OUT: file moniker part of lpszDisplayName
        LPWSTR          lpszFrom;       // OUT: prefix of source changed from
        LPWSTR          lpszTo;         // OUT: prefix of source changed to

} OLEUICHANGESOURCEW, *POLEUICHANGESOURCEW, *LPOLEUICHANGESOURCEW;

typedef struct tagOLEUICHANGESOURCEA
{
        // These IN fields are standard across all OLEUI dialog functions.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT:  Flags
        HWND            hWndOwner;      // Owning window
        LPCSTR          lpszCaption;    // Dialog caption bar contents
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        HINSTANCE       hInstance;      // Instance for customized template name
        LPCSTR          lpszTemplate;   // Customized template name
        HRSRC           hResource;      // Customized template handle

        // INTERNAL ONLY: do not modify these members
        OPENFILENAMEA*  lpOFN;          // pointer OPENFILENAME struct
        DWORD           dwReserved1[4]; // (reserved for future use)

        // Specifics for OLEUICHANGESOURCE.
        LPOLEUILINKCONTAINERA lpOleUILinkContainer;  // IN: used to validate link sources
        DWORD           dwLink;         // IN: magic# for lpOleUILinkContainer
        LPSTR           lpszDisplayName;// IN-OUT: complete source display name
        ULONG           nFileLength;    // IN-OUT: file moniker part of lpszDisplayName
        LPSTR           lpszFrom;       // OUT: prefix of source changed from
        LPSTR           lpszTo;         // OUT: prefix of source changed to

} OLEUICHANGESOURCEA, *POLEUICHANGESOURCEA, *LPOLEUICHANGESOURCEA;

STDAPI_(UINT) OleUIChangeSourceW(LPOLEUICHANGESOURCEW);
STDAPI_(UINT) OleUIChangeSourceA(LPOLEUICHANGESOURCEA);

#ifdef UNICODE
#define tagOLEUICHANGESOURCE tagOLEUICHANGESOURCEW
#define OLEUICHANGESOURCE OLEUICHANGESOURCEW
#define POLEUICHANGESOURCE POLEUICHANGESOURCEW
#define LPOLEUICHANGESOURCE LPOLEUICHANGESOURCEW
#define OleUIChangeSource OleUIChangeSourceW
#else
#define tagOLEUICHANGESOURCE tagOLEUICHANGESOURCEA
#define OLEUICHANGESOURCE OLEUICHANGESOURCEA
#define POLEUICHANGESOURCE POLEUICHANGESOURCEA
#define LPOLEUICHANGESOURCE LPOLEUICHANGESOURCEA
#define OleUIChangeSource OleUIChangeSourceA
#endif

// Change Source Dialog flags
#define CSF_SHOWHELP                    0x00000001L // IN: enable/show help button
#define CSF_VALIDSOURCE                 0x00000002L // OUT: link was validated
#define CSF_ONLYGETSOURCE               0x00000004L // IN: disables validation of source
#define CSF_EXPLORER                    0x00000008L // IN: use new OFN_EXPLORER custom template behavior

// Change Source Dialog errors
#define OLEUI_CSERR_LINKCNTRNULL        (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_CSERR_LINKCNTRINVALID     (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_CSERR_FROMNOTNULL         (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_CSERR_TONOTNULL           (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_CSERR_SOURCENULL          (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_CSERR_SOURCEINVALID       (OLEUI_ERR_STANDARDMAX+5)
#define OLEUI_CSERR_SOURCEPARSERROR     (OLEUI_ERR_STANDARDMAX+6)
#define OLEUI_CSERR_SOURCEPARSEERROR    (OLEUI_ERR_STANDARDMAX+6)

/////////////////////////////////////////////////////////////////////////////
// OBJECT PROPERTIES DIALOG

#undef  INTERFACE
#define INTERFACE   IOleUIObjInfoW

DECLARE_INTERFACE_(IOleUIObjInfoW, IUnknown)
{
        // *** IUnknown methods *** //
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS) PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;

        // *** extra for General Properties *** //
        STDMETHOD(GetObjectInfo) (THIS_ DWORD dwObject,
                DWORD FAR* lpdwObjSize, LPWSTR FAR* lplpszLabel,
                LPWSTR FAR* lplpszType, LPWSTR FAR* lplpszShortType,
                LPWSTR FAR* lplpszLocation) PURE;
        STDMETHOD(GetConvertInfo) (THIS_ DWORD dwObject,
                CLSID FAR* lpClassID, WORD FAR* lpwFormat,
                CLSID FAR* lpConvertDefaultClassID,
                LPCLSID FAR* lplpClsidExclude, UINT FAR* lpcClsidExclude) PURE;
        STDMETHOD(ConvertObject) (THIS_ DWORD dwObject, REFCLSID clsidNew) PURE;

        // *** extra for View Properties *** //
        STDMETHOD(GetViewInfo) (THIS_ DWORD dwObject,
                HGLOBAL FAR* phMetaPict, DWORD* pdvAspect, int* pnCurrentScale) PURE;
        STDMETHOD(SetViewInfo) (THIS_ DWORD dwObject,
                HGLOBAL hMetaPict, DWORD dvAspect,
                int nCurrentScale, BOOL bRelativeToOrig) PURE;
};

typedef IOleUIObjInfoW FAR* LPOLEUIOBJINFOW;

#undef  INTERFACE
#define INTERFACE   IOleUIObjInfoA

DECLARE_INTERFACE_(IOleUIObjInfoA, IUnknown)
{
        // *** IUnknown methods *** //
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS) PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;

        // *** extra for General Properties *** //
        STDMETHOD(GetObjectInfo) (THIS_ DWORD dwObject,
                DWORD FAR* lpdwObjSize, LPSTR FAR* lplpszLabel,
                LPSTR FAR* lplpszType, LPSTR FAR* lplpszShortType,
                LPSTR FAR* lplpszLocation) PURE;
        STDMETHOD(GetConvertInfo) (THIS_ DWORD dwObject,
                CLSID FAR* lpClassID, WORD FAR* lpwFormat,
                CLSID FAR* lpConvertDefaultClassID,
                LPCLSID FAR* lplpClsidExclude, UINT FAR* lpcClsidExclude) PURE;
        STDMETHOD(ConvertObject) (THIS_ DWORD dwObject, REFCLSID clsidNew) PURE;

        // *** extra for View Properties *** //
        STDMETHOD(GetViewInfo) (THIS_ DWORD dwObject,
                HGLOBAL FAR* phMetaPict, DWORD* pdvAspect, int* pnCurrentScale) PURE;
        STDMETHOD(SetViewInfo) (THIS_ DWORD dwObject,
                HGLOBAL hMetaPict, DWORD dvAspect,
                int nCurrentScale, BOOL bRelativeToOrig) PURE;
};

typedef IOleUIObjInfoA FAR* LPOLEUIOBJINFOA;

#ifdef UNICODE
#define IOleUIObjInfo IOleUIObjInfoW
#define IOleUIObjInfoVtbl IOleUIObjInfoWVtbl
#define LPOLEUIOBJINFO LPOLEUIOBJINFOW
#else
#define IOleUIObjInfo IOleUIObjInfoA
#define IOleUIObjInfoVtbl IOleUIObjInfoAVtbl
#define LPOLEUIOBJINFO LPOLEUIOBJINFOA
#endif

#undef  INTERFACE
#define INTERFACE   IOleUILinkInfoW

DECLARE_INTERFACE_(IOleUILinkInfoW, IOleUILinkContainerW)
{
        // *** IUnknown methods *** //
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS) PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;

        // *** IOleUILinkContainer *** //
        STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD dwUpdateOpt) PURE;
        STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD FAR* lpdwUpdateOpt) PURE;
        STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPWSTR lpszDisplayName,
                ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource) PURE;
        STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
                LPWSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
                LPWSTR FAR* lplpszFullLinkType, LPWSTR FAR* lplpszShortLinkType,
                BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected) PURE;
        STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
                BOOL fErrorMessage, BOOL fReserved) PURE;
        STDMETHOD(CancelLink) (THIS_ DWORD dwLink) PURE;

        // *** extra for Link Properties *** //
        STDMETHOD(GetLastUpdate) (THIS_ DWORD dwLink,
                FILETIME FAR* lpLastUpdate) PURE;
};

typedef IOleUILinkInfoW FAR* LPOLEUILINKINFOW;

#undef  INTERFACE
#define INTERFACE   IOleUILinkInfoA

DECLARE_INTERFACE_(IOleUILinkInfoA, IOleUILinkContainerA)
{
        // *** IUnknown methods *** //
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
        STDMETHOD_(ULONG,AddRef) (THIS) PURE;
        STDMETHOD_(ULONG,Release) (THIS) PURE;

        // *** IOleUILinkContainer *** //
        STDMETHOD_(DWORD,GetNextLink) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(SetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD dwUpdateOpt) PURE;
        STDMETHOD(GetLinkUpdateOptions) (THIS_ DWORD dwLink,
                DWORD FAR* lpdwUpdateOpt) PURE;
        STDMETHOD(SetLinkSource) (THIS_ DWORD dwLink, LPSTR lpszDisplayName,
                ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource) PURE;
        STDMETHOD(GetLinkSource) (THIS_ DWORD dwLink,
                LPSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
                LPSTR FAR* lplpszFullLinkType, LPSTR FAR* lplpszShortLinkType,
                BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected) PURE;
        STDMETHOD(OpenLinkSource) (THIS_ DWORD dwLink) PURE;
        STDMETHOD(UpdateLink) (THIS_ DWORD dwLink,
                BOOL fErrorMessage, BOOL fReserved) PURE;
        STDMETHOD(CancelLink) (THIS_ DWORD dwLink) PURE;

        // *** extra for Link Properties *** //
        STDMETHOD(GetLastUpdate) (THIS_ DWORD dwLink,
                FILETIME FAR* lpLastUpdate) PURE;
};

typedef IOleUILinkInfoA FAR* LPOLEUILINKINFOA;

#ifdef UNICODE
#define IOleUILinkInfo IOleUILinkInfoW
#define IOleUILinkInfoVtbl IOleUILinkInfoWVtbl
#define LPOLEUILINKINFO LPOLEUILINKINFOW
#else
#define IOleUILinkInfo IOleUILinkInfoA
#define IOleUILinkInfoVtbl IOleUILinkInfoAVtbl
#define LPOLEUILINKINFO LPOLEUILINKINFOA
#endif

struct tagOLEUIOBJECTPROPSW;
struct tagOLEUIOBJECTPROPSA;

typedef struct tagOLEUIGNRLPROPSW
{
        // These IN fields are standard across all OLEUI property pages.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: flags specific to general page
        DWORD           dwReserved1[2];
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        DWORD           dwReserved2[3];

        struct tagOLEUIOBJECTPROPSW* lpOP;   // (used internally)

} OLEUIGNRLPROPSW, *POLEUIGNRLPROPSW, FAR* LPOLEUIGNRLPROPSW;

typedef struct tagOLEUIGNRLPROPSA
{
        // These IN fields are standard across all OLEUI property pages.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: flags specific to general page
        DWORD           dwReserved1[2];
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback
        LPARAM          lCustData;      // Custom data to pass to hook
        DWORD           dwReserved2[3];

        struct tagOLEUIOBJECTPROPSA* lpOP;   // (used internally)

} OLEUIGNRLPROPSA, *POLEUIGNRLPROPSA, FAR* LPOLEUIGNRLPROPSA;

#ifdef UNICODE
#define tagOLEUIGNRLPROPS tagOLEUIGNRLPROPSW
#define OLEUIGNRLPROPS OLEUIGNRLPROPSW
#define POLEUIGNRLPROPS POLEUIGNRLPROPSW
#define LPOLEUIGNRLPROPS LPOLEUIGNRLPROPSW
#else
#define tagOLEUIGNRLPROPS tagOLEUIGNRLPROPSA
#define OLEUIGNRLPROPS OLEUIGNRLPROPSA
#define POLEUIGNRLPROPS POLEUIGNRLPROPSA
#define LPOLEUIGNRLPROPS LPOLEUIGNRLPROPSA
#endif

typedef struct tagOLEUIVIEWPROPSW
{
        // These IN fields are standard across all OLEUI property pages.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: flags specific to view page
        DWORD           dwReserved1[2];
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback (not used in this dialog)
        LPARAM          lCustData;      // Custom data to pass to hook
        DWORD           dwReserved2[3];

        struct tagOLEUIOBJECTPROPSW* lpOP; // (used internally)

        int             nScaleMin;      // scale range
        int             nScaleMax;

} OLEUIVIEWPROPSW, *POLEUIVIEWPROPSW, FAR* LPOLEUIVIEWPROPSW;

typedef struct tagOLEUIVIEWPROPSA
{
        // These IN fields are standard across all OLEUI property pages.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: flags specific to view page
        DWORD           dwReserved1[2];
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback (not used in this dialog)
        LPARAM          lCustData;      // Custom data to pass to hook
        DWORD           dwReserved2[3];

        struct tagOLEUIOBJECTPROPSA* lpOP; // (used internally)

        int             nScaleMin;      // scale range
        int             nScaleMax;

} OLEUIVIEWPROPSA, *POLEUIVIEWPROPSA, FAR* LPOLEUIVIEWPROPSA;

#ifdef UNICODE
#define tagOLEUIVIEWPROPS tagOLEUIVIEWPROPSW
#define OLEUIVIEWPROPS OLEUIVIEWPROPSW
#define POLEUIVIEWPROPS POLEUIVIEWPROPSW
#define LPOLEUIVIEWPROPS LPOLEUIVIEWPROPSW
#else
#define tagOLEUIVIEWPROPS tagOLEUIVIEWPROPSA
#define OLEUIVIEWPROPS OLEUIVIEWPROPSA
#define POLEUIVIEWPROPS POLEUIVIEWPROPSA
#define LPOLEUIVIEWPROPS LPOLEUIVIEWPROPSA
#endif

// Flags for OLEUIVIEWPROPS
#define VPF_SELECTRELATIVE          0x00000001L // IN: relative to orig
#define VPF_DISABLERELATIVE         0x00000002L // IN: disable relative to orig
#define VPF_DISABLESCALE            0x00000004L // IN: disable scale option

typedef struct tagOLEUILINKPROPSW
{
        // These IN fields are standard across all OLEUI property pages.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: flags specific to links page
        DWORD           dwReserved1[2];
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback (not used in this dialog)
        LPARAM          lCustData;      // Custom data to pass to hook
        DWORD           dwReserved2[3];

        struct tagOLEUIOBJECTPROPSW* lpOP; // (used internally)

} OLEUILINKPROPSW, *POLEUILINKPROPSW, FAR* LPOLEUILINKPROPSW;

typedef struct tagOLEUILINKPROPSA
{
        // These IN fields are standard across all OLEUI property pages.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: flags specific to links page
        DWORD           dwReserved1[2];
        LPFNOLEUIHOOK   lpfnHook;       // Hook callback (not used in this dialog)
        LPARAM          lCustData;      // Custom data to pass to hook
        DWORD           dwReserved2[3];

        struct tagOLEUIOBJECTPROPSA* lpOP; // (used internally)

} OLEUILINKPROPSA, *POLEUILINKPROPSA, FAR* LPOLEUILINKPROPSA;

#ifdef UNICODE
#define tagOLEUILINKPROPS tagOLEUILINKPROPSW
#define OLEUILINKPROPS OLEUILINKPROPSW
#define POLEUILINKPROPS POLEUILINKPROPSW
#define LPOLEUILINKPROPS LPOLEUILINKPROPSW
#else
#define tagOLEUILINKPROPS tagOLEUILINKPROPSA
#define OLEUILINKPROPS OLEUILINKPROPSA
#define POLEUILINKPROPS POLEUILINKPROPSA
#define LPOLEUILINKPROPS LPOLEUILINKPROPSA
#endif

#if (WINVER >= 0x400)
// Under Windows 95 prsht.h is NOT a part of the normal Windows
// environment, so we explicitly include it here to be safe.
#include <prsht.h>

#ifndef PSM_SETFINISHTEXTA
// We are building under Windows 95.
//
// Under Windows 95 there are no wide-character definitions
// for the property sheet code.
//
// Since the UNICODE version of our API is not implemented on Windows 95,
// this only creates a semantic problem.  The entry points will still
// look the same and the code will still work the same if we define
// LPPROPSHEETHEADERW to be the narrow version of the structure.

typedef struct _PROPSHEETHEADER FAR* LPPROPSHEETHEADERW;
typedef struct _PROPSHEETHEADER FAR* LPPROPSHEETHEADERA;

#else
// We are building under Windows NT.

// Go ahead and define LPPROPSHEETHEADERW as it should be defined!

typedef struct _PROPSHEETHEADERW FAR* LPPROPSHEETHEADERW;
typedef struct _PROPSHEETHEADERA FAR* LPPROPSHEETHEADERA;

#ifdef UNICODE
#define LPPROPSHEETHEADER LPPROPSHEETHEADERW
#else
#define LPPROPSHEETHEADER LPPROPSHEETHEADERA
#endif

#endif // PSM_SETFINISHTEXTA

#else // WINVER

// If WINVER < 0x400, then PROPSHEETHEADER stuff isn't defined.
// The user won't be able to use the prop-sheet code, so we just define the
// necessary structures to be void pointers to enable to header file to
// at least compile correctly.

typedef void FAR* LPPROPSHEETHEADERW;
typedef void FAR* LPPROPSHEETHEADERA;

#ifdef UNICODE
#define LPPROPSHEETHEADER LPPROPSHEETHEADERW
#else
#define LPPROPSHEETHEADER LPPROPSHEETHEADERA
#endif

#endif // WINVER

typedef struct tagOLEUIOBJECTPROPSW
{
        // These IN fields are standard across all OLEUI property sheets.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: global flags for the sheet

        // Standard PROPSHEETHEADER used for extensibility
        LPPROPSHEETHEADERW   lpPS;         // IN: property sheet header

        // Data which allows manipulation of the object
        DWORD           dwObject;       // IN: identifier for the object
        LPOLEUIOBJINFOW lpObjInfo;      // IN: interface to manipulate object

        // Data which allows manipulation of the link
        DWORD           dwLink;         // IN: identifier for the link
        LPOLEUILINKINFOW lpLinkInfo;     // IN: interface to manipulate link

        // Data specfic to each page
        LPOLEUIGNRLPROPSW lpGP;          // IN: general page
        LPOLEUIVIEWPROPSW lpVP;          // IN: view page
        LPOLEUILINKPROPSW lpLP;          // IN: link page

} OLEUIOBJECTPROPSW, *POLEUIOBJECTPROPSW, FAR* LPOLEUIOBJECTPROPSW;

typedef struct tagOLEUIOBJECTPROPSA
{
        // These IN fields are standard across all OLEUI property sheets.
        DWORD           cbStruct;       // Structure Size
        DWORD           dwFlags;        // IN-OUT: global flags for the sheet

        // Standard PROPSHEETHEADER used for extensibility
        LPPROPSHEETHEADERA  lpPS;         // IN: property sheet header

        // Data which allows manipulation of the object
        DWORD           dwObject;       // IN: identifier for the object
        LPOLEUIOBJINFOA lpObjInfo;      // IN: interface to manipulate object

        // Data which allows manipulation of the link
        DWORD           dwLink;         // IN: identifier for the link
        LPOLEUILINKINFOA lpLinkInfo;     // IN: interface to manipulate link

        // Data specfic to each page
        LPOLEUIGNRLPROPSA lpGP;          // IN: general page
        LPOLEUIVIEWPROPSA lpVP;          // IN: view page
        LPOLEUILINKPROPSA lpLP;          // IN: link page

} OLEUIOBJECTPROPSA, *POLEUIOBJECTPROPSA, FAR* LPOLEUIOBJECTPROPSA;

STDAPI_(UINT) OleUIObjectPropertiesW(LPOLEUIOBJECTPROPSW);
STDAPI_(UINT) OleUIObjectPropertiesA(LPOLEUIOBJECTPROPSA);

#ifdef UNICODE
#define tagOLEUIOBJECTPROPS tagOLEUIOBJECTPROPSW
#define OLEUIOBJECTPROPS OLEUIOBJECTPROPSW
#define POLEUIOBJECTPROPS POLEUIOBJECTPROPSW
#define LPOLEUIOBJECTPROPS LPOLEUIOBJECTPROPSW
#define OleUIObjectProperties OleUIObjectPropertiesW
#else
#define tagOLEUIOBJECTPROPS tagOLEUIOBJECTPROPSA
#define OLEUIOBJECTPROPS OLEUIOBJECTPROPSA
#define POLEUIOBJECTPROPS POLEUIOBJECTPROPSA
#define LPOLEUIOBJECTPROPS LPOLEUIOBJECTPROPSA
#define OleUIObjectProperties OleUIObjectPropertiesA
#endif

// Flags for OLEUIOBJECTPROPS
#define OPF_OBJECTISLINK                0x00000001L
#define OPF_NOFILLDEFAULT               0x00000002L
#define OPF_SHOWHELP                    0x00000004L
#define OPF_DISABLECONVERT              0x00000008L

// Errors for OleUIObjectProperties
#define OLEUI_OPERR_SUBPROPNULL         (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_OPERR_SUBPROPINVALID      (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_OPERR_PROPSHEETNULL       (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_OPERR_PROPSHEETINVALID    (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_OPERR_SUPPROP             (OLEUI_ERR_STANDARDMAX+4)
#define OLEUI_OPERR_PROPSINVALID        (OLEUI_ERR_STANDARDMAX+5)
#define OLEUI_OPERR_PAGESINCORRECT      (OLEUI_ERR_STANDARDMAX+6)
#define OLEUI_OPERR_INVALIDPAGES        (OLEUI_ERR_STANDARDMAX+7)
#define OLEUI_OPERR_NOTSUPPORTED        (OLEUI_ERR_STANDARDMAX+8)
#define OLEUI_OPERR_DLGPROCNOTNULL      (OLEUI_ERR_STANDARDMAX+9)
#define OLEUI_OPERR_LPARAMNOTZERO       (OLEUI_ERR_STANDARDMAX+10)

#define OLEUI_GPERR_STRINGINVALID       (OLEUI_ERR_STANDARDMAX+11)
#define OLEUI_GPERR_CLASSIDINVALID      (OLEUI_ERR_STANDARDMAX+12)
#define OLEUI_GPERR_LPCLSIDEXCLUDEINVALID   (OLEUI_ERR_STANDARDMAX+13)
#define OLEUI_GPERR_CBFORMATINVALID     (OLEUI_ERR_STANDARDMAX+14)
#define OLEUI_VPERR_METAPICTINVALID     (OLEUI_ERR_STANDARDMAX+15)
#define OLEUI_VPERR_DVASPECTINVALID     (OLEUI_ERR_STANDARDMAX+16)
#define OLEUI_LPERR_LINKCNTRNULL        (OLEUI_ERR_STANDARDMAX+17)
#define OLEUI_LPERR_LINKCNTRINVALID     (OLEUI_ERR_STANDARDMAX+18)

#define OLEUI_OPERR_PROPERTYSHEET       (OLEUI_ERR_STANDARDMAX+19)
#define OLEUI_OPERR_OBJINFOINVALID      (OLEUI_ERR_STANDARDMAX+20)
#define OLEUI_OPERR_LINKINFOINVALID     (OLEUI_ERR_STANDARDMAX+21)

// wParam used by PSM_QUERYSIBLINGS
#define OLEUI_QUERY_GETCLASSID          0xFF00  // override class id for icon
#define OLEUI_QUERY_LINKBROKEN          0xFF01  // after link broken

/////////////////////////////////////////////////////////////////////////////
// PROMPT USER DIALOGS

int CDECL OleUIPromptUserW(int nTemplate, HWND hwndParent, ...);
int CDECL OleUIPromptUserA(int nTemplate, HWND hwndParent, ...);

#ifdef UNICODE
#define OleUIPromptUser OleUIPromptUserW
#else
#define OleUIPromptUser OleUIPromptUserA
#endif

STDAPI_(BOOL) OleUIUpdateLinksW(LPOLEUILINKCONTAINERW lpOleUILinkCntr,
        HWND hwndParent, LPWSTR lpszTitle, int cLinks);
STDAPI_(BOOL) OleUIUpdateLinksA(LPOLEUILINKCONTAINERA lpOleUILinkCntr,
        HWND hwndParent, LPSTR lpszTitle, int cLinks);

#ifdef UNICODE
#define OleUIUpdateLinks OleUIUpdateLinksW
#else
#define OleUIUpdateLinks OleUIUpdateLinksA
#endif

/////////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // RC_INVOKED

#endif  //_OLEDLG_H_

/////////////////////////////////////////////////////////////////////////////
