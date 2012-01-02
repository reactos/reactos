/*
 * Declarations for OLEDLG
 *
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_OLEDLG_H
#define __WINE_OLEDLG_H

#ifndef __WINESRC__
# include <windows.h>
#endif
#include <shellapi.h>
#include <commdlg.h>
#include <ole2.h>
#ifndef __WINESRC__
# include <string.h>
# include <tchar.h>
#endif
#include <dlgs.h>
#include <prsht.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct IOleUILinkContainerA *POLEUILINKCONTAINERA, *LPOLEUILINKCONTAINERA;
typedef struct IOleUILinkContainerW *POLEUILINKCONTAINERW, *LPOLEUILINKCONTAINERW;

typedef struct IOleUILinkInfoA *POLEUILINKINFOA, *LPOLEUILINKINFOA;
typedef struct IOleUILinkInfoW *POLEUILINKINFOW, *LPOLEUILINKINFOW;

typedef struct IOleUIObjInfoA *POLEUIOBJINFOA, *LPOLEUIOBJINFOA;
typedef struct IOleUIObjInfoW *POLEUIOBJINFOW, *LPOLEUIOBJINFOW;

#define IDC_OLEUIHELP                   99


#define OLEUI_ERR_STANDARDMAX           116

#define OLEUI_BZERR_HTASKINVALID        (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_BZ_SWITCHTOSELECTED       (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_BZ_RETRYSELECTED          (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_BZ_CALLUNBLOCKED          (OLEUI_ERR_STANDARDMAX+3)

#define OLEUI_FALSE   0
#define OLEUI_SUCCESS 1 /* Same as OLEUI_OK */
#define OLEUI_OK      1 /* OK button pressed */
#define OLEUI_CANCEL  2 /* Cancel button pressed */

#define OLEUI_ERR_STANDARDMIN           100
#define OLEUI_ERR_OLEMEMALLOC           100
#define OLEUI_ERR_STRUCTURENULL         101
#define OLEUI_ERR_STRUCTUREINVALID      102
#define OLEUI_ERR_CBSTRUCTINCORRECT     103
#define OLEUI_ERR_HWNDOWNERINVALID      104
#define OLEUI_ERR_LPSZCAPTIONINVALID    105
#define OLEUI_ERR_LPFNHOOKINVALID       106
#define OLEUI_ERR_HINSTANCEINVALID      107
#define OLEUI_ERR_LPSZTEMPLATEINVALID   108
#define OLEUI_ERR_HRESOURCEINVALID      109
#define OLEUI_ERR_FINDTEMPLATEFAILURE   110
#define OLEUI_ERR_LOADTEMPLATEFAILURE   111
#define OLEUI_ERR_DIALOGFAILURE         112
#define OLEUI_ERR_LOCALMEMALLOC         113
#define OLEUI_ERR_GLOBALMEMALLOC        114
#define OLEUI_ERR_LOADSTRING            115
#define OLEUI_ERR_STANDARDMAX           116

typedef UINT (CALLBACK *LPFNOLEUIHOOK)(HWND, UINT, WPARAM, LPARAM);

/*****************************************************************************
 * Registered Message Names
 */
#define SZOLEUI_MSG_HELPA              "OLEUI_MSG_HELP"
#define SZOLEUI_MSG_ENDDIALOGA         "OLEUI_MSG_ENDDIALOG"
#define SZOLEUI_MSG_BROWSEA            "OLEUI_MSG_BROWSE"
#define SZOLEUI_MSG_CHANGEICONA        "OLEUI_MSG_CHANGEICON"
#define SZOLEUI_MSG_CLOSEBUSYDIALOGA   "OLEUI_MSG_CLOSEBUSYDIALOG"
#define SZOLEUI_MSG_CONVERTA           "OLEUI_MSG_CONVERT"
#define SZOLEUI_MSG_CHANGESOURCEA      "OLEUI_MSG_CHANGESOURCE"
#define SZOLEUI_MSG_ADDCONTROLA        "OLEUI_MSG_ADDCONTROL"
#define SZOLEUI_MSG_BROWSE_OFNA        "OLEUI_MSG_BROWSE_OFN"
#if defined(__GNUC__)
# define SZOLEUI_MSG_HELPW             (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','H','E','L','P',0 }
# define SZOLEUI_MSG_ENDDIALOGW        (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','E','N','D','D','I','A','L','O','G',0 }
# define SZOLEUI_MSG_BROWSEW           (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','B','R','O','W','S','E',0 }
# define SZOLEUI_MSG_CHANGEICONW       (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','C','H','A','N','G','E','I','C','O','N',0 }
# define SZOLEUI_MSG_CLOSEBUSYDIALOGW  (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','C','L','O','S','E','B','U','S','Y','D','I','A','L','O','G',0 }
# define SZOLEUI_MSG_CONVERTW          (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','C','O','N','V','E','R','T',0 }
# define SZOLEUI_MSG_CHANGESOURCEW     (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','C','H','A','N','G','E','S','O','U','R','C','E',0 }
# define SZOLEUI_MSG_ADDCONTROLW       (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','A','D','D','C','O','N','T','R','O','L',0 }
# define SZOLEUI_MSG_BROWSE_OFNW       (const WCHAR []){ 'O','L','E','U','I','_','M','S','G','_','B','R','O','W','S','E','_','O','F','N',0 }
#elif defined(_MSC_VER)
# define SZOLEUI_MSG_HELPW              L"OLEUI_MSG_HELP"
# define SZOLEUI_MSG_ENDDIALOGW         L"OLEUI_MSG_ENDDIALOG"
# define SZOLEUI_MSG_BROWSEW            L"OLEUI_MSG_BROWSE"
# define SZOLEUI_MSG_CHANGEICONW        L"OLEUI_MSG_CHANGEICON"
# define SZOLEUI_MSG_CLOSEBUSYDIALOGW   L"OLEUI_MSG_CLOSEBUSYDIALOG"
# define SZOLEUI_MSG_CONVERTW           L"OLEUI_MSG_CONVERT"
# define SZOLEUI_MSG_CHANGESOURCEW      L"OLEUI_MSG_CHANGESOURCE"
# define SZOLEUI_MSG_ADDCONTROLW        L"OLEUI_MSG_ADDCONTROL"
# define SZOLEUI_MSG_BROWSE_OFNW        L"OLEUI_MSG_BROWSE_OFN"
#else
static const WCHAR SZOLEUI_MSG_HELPW[]             = { 'O','L','E','U','I','_','M','S','G','_','H','E','L','P',0 };
static const WCHAR SZOLEUI_MSG_ENDDIALOGW[]        = { 'O','L','E','U','I','_','M','S','G','_','E','N','D','D','I','A','L','O','G',0 };
static const WCHAR SZOLEUI_MSG_BROWSEW[]           = { 'O','L','E','U','I','_','M','S','G','_','B','R','O','W','S','E',0 };
static const WCHAR SZOLEUI_MSG_CHANGEICONW[]       = { 'O','L','E','U','I','_','M','S','G','_','C','H','A','N','G','E','I','C','O','N',0 };
static const WCHAR SZOLEUI_MSG_CLOSEBUSYDIALOGW[]  = { 'O','L','E','U','I','_','M','S','G','_','C','L','O','S','E','B','U','S','Y','D','I','A','L','O','G',0 };
static const WCHAR SZOLEUI_MSG_CONVERTW[]          = { 'O','L','E','U','I','_','M','S','G','_','C','O','N','V','E','R','T',0 };
static const WCHAR SZOLEUI_MSG_CHANGESOURCEW[]     = { 'O','L','E','U','I','_','M','S','G','_','C','H','A','N','G','E','S','O','U','R','C','E',0 };
static const WCHAR SZOLEUI_MSG_ADDCONTROLW[]       = { 'O','L','E','U','I','_','M','S','G','_','A','D','D','C','O','N','T','R','O','L',0 };
static const WCHAR SZOLEUI_MSG_BROWSE_OFNW[]       = { 'O','L','E','U','I','_','M','S','G','_','B','R','O','W','S','E','_','O','F','N',0 };
#endif
#define SZOLEUI_MSG_HELP             WINELIB_NAME_AW(SZOLEUI_MSG_HELP)
#define SZOLEUI_MSG_ENDDIALOG        WINELIB_NAME_AW(SZOLEUI_MSG_ENDDIALOG)
#define SZOLEUI_MSG_BROWSE           WINELIB_NAME_AW(SZOLEUI_MSG_BROWSE)
#define SZOLEUI_MSG_CHANGEICON       WINELIB_NAME_AW(SZOLEUI_MSG_CHANGEICON)
#define SZOLEUI_MSG_CLOSEBUSYDIALOG  WINELIB_NAME_AW(SZOLEUI_MSG_CLOSEBUSYDIALOG)
#define SZOLEUI_MSG_CONVERT          WINELIB_NAME_AW(SZOLEUI_MSG_CONVERT)
#define SZOLEUI_MSG_CHANGESOURCE     WINELIB_NAME_AW(SZOLEUI_MSG_CHANGESOURCE)
#define SZOLEUI_MSG_ADDCONTROL       WINELIB_NAME_AW(SZOLEUI_MSG_ADDCONTROL)
#define SZOLEUI_MSG_BROWSE_OFN       WINELIB_NAME_AW(SZOLEUI_MSG_BROWSE_OFN)


/*****************************************************************************
 * INSERT OBJECT DIALOG
 */
typedef struct tagOLEUIINSERTOBJECTA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    CLSID           clsid;
    LPSTR           lpszFile;
    UINT            cchFile;
    UINT            cClsidExclude;
    LPCLSID         lpClsidExclude;
    IID             iid;
    DWORD           oleRender;
    LPFORMATETC     lpFormatEtc;
    LPOLECLIENTSITE lpIOleClientSite;
    LPSTORAGE       lpIStorage;
    LPVOID          *ppvObj;
    SCODE           sc;
    HGLOBAL         hMetaPict;
} OLEUIINSERTOBJECTA, *POLEUIINSERTOBJECTA, *LPOLEUIINSERTOBJECTA;

typedef struct tagOLEUIINSERTOBJECTW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    CLSID           clsid;
    LPWSTR          lpszFile;
    UINT            cchFile;
    UINT            cClsidExclude;
    LPCLSID         lpClsidExclude;
    IID             iid;
    DWORD           oleRender;
    LPFORMATETC     lpFormatEtc;
    LPOLECLIENTSITE lpIOleClientSite;
    LPSTORAGE       lpIStorage;
    LPVOID          *ppvObj;
    SCODE           sc;
    HGLOBAL         hMetaPict;
} OLEUIINSERTOBJECTW, *POLEUIINSERTOBJECTW, *LPOLEUIINSERTOBJECTW;

DECL_WINELIB_TYPE_AW(OLEUIINSERTOBJECT)
DECL_WINELIB_TYPE_AW(POLEUIINSERTOBJECT)
DECL_WINELIB_TYPE_AW(LPOLEUIINSERTOBJECT)

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

/*****************************************************************************
 * CONVERT DIALOG
 */
typedef struct tagOLEUICONVERTA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    CLSID           clsid;
    CLSID           clsidConvertDefault;
    CLSID           clsidActivateDefault;
    CLSID           clsidNew;
    DWORD           dvAspect;
    WORD            wFormat;
    BOOL            fIsLinkedObject;
    HGLOBAL         hMetaPict;
    LPSTR           lpszUserType;
    BOOL            fObjectsIconChanged;
    LPSTR           lpszDefLabel;
    UINT            cClsidExclude;
    LPCLSID         lpClsidExclude;
} OLEUICONVERTA, *POLEUICONVERTA, *LPOLEUICONVERTA;

typedef struct tagOLEUICONVERTW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    CLSID           clsid;
    CLSID           clsidConvertDefault;
    CLSID           clsidActivateDefault;
    CLSID           clsidNew;
    DWORD           dvAspect;
    WORD            wFormat;
    BOOL            fIsLinkedObject;
    HGLOBAL         hMetaPict;
    LPWSTR          lpszUserType;
    BOOL            fObjectsIconChanged;
    LPWSTR          lpszDefLabel;
    UINT            cClsidExclude;
    LPCLSID         lpClsidExclude;
} OLEUICONVERTW, *POLEUICONVERTW, *LPOLEUICONVERTW;

DECL_WINELIB_TYPE_AW(OLEUICONVERT)
DECL_WINELIB_TYPE_AW(POLEUICONVERT)
DECL_WINELIB_TYPE_AW(LPOLEUICONVERT)

#define CF_SHOWHELPBUTTON               0x00000001L
#define CF_SETCONVERTDEFAULT            0x00000002L
#define CF_SETACTIVATEDEFAULT           0x00000004L
#define CF_SELECTCONVERTTO              0x00000008L
#define CF_SELECTACTIVATEAS             0x00000010L
#define CF_DISABLEDISPLAYASICON         0x00000020L
#define CF_DISABLEACTIVATEAS            0x00000040L
#define CF_HIDECHANGEICON               0x00000080L
#define CF_CONVERTONLY                  0x00000100L

/*****************************************************************************
 * CHANGE ICON DIALOG
 */
typedef struct tagOLEUICHANGEICONA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    HGLOBAL         hMetaPict;
    CLSID           clsid;
    CHAR            szIconExe[MAX_PATH];
    INT             cchIconExe;
} OLEUICHANGEICONA, *POLEUICHANGEICONA, *LPOLEUICHANGEICONA;

typedef struct tagOLEUICHANGEICONW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    HGLOBAL         hMetaPict;
    CLSID           clsid;
    WCHAR           szIconExe[MAX_PATH];
    INT             cchIconExe;
} OLEUICHANGEICONW, *POLEUICHANGEICONW, *LPOLEUICHANGEICONW;

DECL_WINELIB_TYPE_AW(OLEUICHANGEICON)
DECL_WINELIB_TYPE_AW(POLEUICHANGEICON)
DECL_WINELIB_TYPE_AW(LPOLEUICHANGEICON)


#define CIF_SHOWHELP                    0x00000001L
#define CIF_SELECTCURRENT               0x00000002L
#define CIF_SELECTDEFAULT               0x00000004L
#define CIF_SELECTFROMFILE              0x00000008L
#define CIF_USEICONEXE                  0x00000010L

/*****************************************************************************
 * PASTE SPECIAL DIALOG
 */

typedef enum tagOLEUIPASTEFLAG
{
   OLEUIPASTE_ENABLEICON    = 2048,
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

typedef struct tagOLEUIPASTEENTRYA
{
   FORMATETC        fmtetc;
   LPCSTR           lpstrFormatName;
   LPCSTR           lpstrResultText;
   DWORD            dwFlags;
   DWORD            dwScratchSpace;
} OLEUIPASTEENTRYA, *POLEUIPASTEENTRYA, *LPOLEUIPASTEENTRYA;

typedef struct tagOLEUIPASTEENTRYW
{
   FORMATETC        fmtetc;
   LPCWSTR          lpstrFormatName;
   LPCWSTR          lpstrResultText;
   DWORD            dwFlags;
   DWORD            dwScratchSpace;
} OLEUIPASTEENTRYW, *POLEUIPASTEENTRYW, *LPOLEUIPASTEENTRYW;

DECL_WINELIB_TYPE_AW(OLEUIPASTEENTRY)
DECL_WINELIB_TYPE_AW(POLEUIPASTEENTRY)
DECL_WINELIB_TYPE_AW(LPOLEUIPASTEENTRY)

typedef struct tagOLEUIPASTESPECIALA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    LPDATAOBJECT    lpSrcDataObj;
    LPOLEUIPASTEENTRYA arrPasteEntries;
    INT             cPasteEntries;
    UINT*           arrLinkTypes;
    INT             cLinkTypes;
    UINT            cClsidExclude;
    LPCLSID         lpClsidExclude;
    INT             nSelectedIndex;
    BOOL            fLink;
    HGLOBAL         hMetaPict;
    SIZEL           sizel;
} OLEUIPASTESPECIALA, *POLEUIPASTESPECIALA, *LPOLEUIPASTESPECIALA;

typedef struct tagOLEUIPASTESPECIALW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    LPDATAOBJECT    lpSrcDataObj;
    LPOLEUIPASTEENTRYW arrPasteEntries;
    INT             cPasteEntries;
    UINT*           arrLinkTypes;
    INT             cLinkTypes;
    UINT            cClsidExclude;
    LPCLSID         lpClsidExclude;
    INT             nSelectedIndex;
    BOOL            fLink;
    HGLOBAL         hMetaPict;
    SIZEL           sizel;
} OLEUIPASTESPECIALW, *POLEUIPASTESPECIALW, *LPOLEUIPASTESPECIALW;

DECL_WINELIB_TYPE_AW(OLEUIPASTESPECIAL)
DECL_WINELIB_TYPE_AW(POLEUIPASTESPECIAL)
DECL_WINELIB_TYPE_AW(LPOLEUIPASTESPECIAL)

#define PS_MAXLINKTYPES                 8

#define PSF_SHOWHELP                    0x00000001L
#define PSF_SELECTPASTE                 0x00000002L
#define PSF_SELECTPASTELINK             0x00000004L

#define PSF_CHECKDISPLAYASICON          0x00000008L
#define PSF_DISABLEDISPLAYASICON        0x00000010L
#define PSF_HIDECHANGEICON              0x00000020L
#define PSF_STAYONCLIPBOARDCHANGE       0x00000040L
#define PSF_NOREFRESHDATAOBJECT         0x00000080L

#define OLEUI_IOERR_SRCDATAOBJECTINVALID    (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_IOERR_ARRPASTEENTRIESINVALID  (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_IOERR_ARRLINKTYPESINVALID     (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_PSERR_CLIPBOARDCHANGED        (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_PSERR_GETCLIPBOARDFAILED      (OLEUI_ERR_STANDARDMAX+4)

/* Paste Special dialog resource ids */
#define IDD_PASTESPECIAL       1003
#define IDD_PASTESPECIAL4      1108

/* Paste Special dialog control ids */
#define IDC_PS_PASTE           500
#define IDC_PS_PASTELINK       501
#define IDC_PS_SOURCETEXT      502
#define IDC_PS_PASTELIST       503
#define IDC_PS_PASTELINKLIST   504
#define IDC_PS_DISPLAYLIST     505
#define IDC_PS_DISPLAYASICON   506
#define IDC_PS_ICONDISPLAY     507
#define IDC_PS_CHANGEICON      508
#define IDC_PS_RESULTIMAGE     509
#define IDC_PS_RESULTTEXT      510

/*****************************************************************************
 * EDIT LINKS DIALOG
 */

#define ELF_SHOWHELP               0x00000001L
#define ELF_DISABLEUPDATENOW       0x00000002L
#define ELF_DISABLEOPENSOURCE      0x00000004L
#define ELF_DISABLECHANGESOURCE    0x00000008L
#define ELF_DISABLECANCELLINK      0x00000010L

typedef struct tagOLEUIEDITLINKSW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    LPOLEUILINKCONTAINERW lpOleUILinkContainer;
} OLEUIEDITLINKSW, *POLEUIEDITLINKSW, *LPOLEUIEDITLINKSW;

typedef struct tagOLEUIEDITLINKSA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    LPOLEUILINKCONTAINERA lpOleUILinkContainer;
} OLEUIEDITLINKSA, *POLEUIEDITLINKSA, *LPOLEUIEDITLINKSA;

DECL_WINELIB_TYPE_AW(OLEUIEDITLINKS)
DECL_WINELIB_TYPE_AW(POLEUIEDITLINKS)
DECL_WINELIB_TYPE_AW(LPOLEUIEDITLINKS)


/***********************************************************************************
 * BUSY DIALOG
 */
typedef struct tagOLEUIBUSYA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    HTASK           hTask;
    HWND*           lphWndDialog;
} OLEUIBUSYA, *POLEUIBUSYA, *LPOLEUIBUSYA;

typedef struct tagOLEUIBUSYW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    HTASK           hTask;
    HWND*           lphWndDialog;
} OLEUIBUSYW, *POLEUIBUSYW, *LPOLEUIBUSYW;

DECL_WINELIB_TYPE_AW(OLEUIBUSY)
DECL_WINELIB_TYPE_AW(POLEUIBUSY)
DECL_WINELIB_TYPE_AW(LPOLEUIBUSY)


#define BZ_DISABLECANCELBUTTON          0x00000001L
#define BZ_DISABLESWITCHTOBUTTON        0x00000002L
#define BZ_DISABLERETRYBUTTON           0x00000004L
#define BZ_NOTRESPONDINGDIALOG          0x00000008L

/***********************************************************************************
 * OBJECT PROPERTIES DIALOG
 */

struct tagOLEUIOBJECTPROPSW;
struct tagOLEUIOBJECTPROPSA;

typedef struct tagOLEUIGNRLPROPSA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSA* lpOP;

} OLEUIGNRLPROPSA, *POLEUIGNRLPROPSA, *LPOLEUIGNRLPROPSA;

typedef struct tagOLEUIGNRLPROPSW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSW* lpOP;
} OLEUIGNRLPROPSW, *POLEUIGNRLPROPSW, *LPOLEUIGNRLPROPSW;

DECL_WINELIB_TYPE_AW(OLEUIGNRLPROPS)
DECL_WINELIB_TYPE_AW(POLEUIGNRLPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUIGNRLPROPS)

typedef struct tagOLEUIVIEWPROPSA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSA* lpOP;
    INT             nScaleMin;
    INT             nScaleMax;
} OLEUIVIEWPROPSA, *POLEUIVIEWPROPSA, *LPOLEUIVIEWPROPSA;

typedef struct tagOLEUIVIEWPROPSW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSW* lpOP;
    INT             nScaleMin;
    INT             nScaleMax;
} OLEUIVIEWPROPSW, *POLEUIVIEWPROPSW, *LPOLEUIVIEWPROPSW;

DECL_WINELIB_TYPE_AW(OLEUIVIEWPROPS)
DECL_WINELIB_TYPE_AW(POLEUIVIEWPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUIVIEWPROPS)


#define VPF_SELECTRELATIVE          0x00000001L
#define VPF_DISABLERELATIVE         0x00000002L
#define VPF_DISABLESCALE            0x00000004L

typedef struct tagOLEUILINKPROPSA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSA* lpOP;
} OLEUILINKPROPSA, *POLEUILINKPROPSA, *LPOLEUILINKPROPSA;

typedef struct tagOLEUILINKPROPSW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSW* lpOP;
} OLEUILINKPROPSW, *POLEUILINKPROPSW, *LPOLEUILINKPROPSW;

DECL_WINELIB_TYPE_AW(OLEUILINKPROPS)
DECL_WINELIB_TYPE_AW(POLEUILINKPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUILINKPROPS)

typedef struct tagOLEUIOBJECTPROPSA
{
    DWORD                cbStruct;
    DWORD                dwFlags;
    LPPROPSHEETHEADERA   lpPS;
    DWORD                dwObject;
    LPOLEUIOBJINFOA      lpObjInfo;
    DWORD                dwLink;
    LPOLEUILINKINFOA     lpLinkInfo;
    LPOLEUIGNRLPROPSA    lpGP;
    LPOLEUIVIEWPROPSA    lpVP;
    LPOLEUILINKPROPSA    lpLP;
} OLEUIOBJECTPROPSA, *POLEUIOBJECTPROPSA, *LPOLEUIOBJECTPROPSA;

typedef struct tagOLEUIOBJECTPROPSW
{
    DWORD                cbStruct;
    DWORD                dwFlags;
    LPPROPSHEETHEADERW   lpPS;
    DWORD                dwObject;
    LPOLEUIOBJINFOW      lpObjInfo;
    DWORD                dwLink;
    LPOLEUILINKINFOW     lpLinkInfo;
    LPOLEUIGNRLPROPSW    lpGP;
    LPOLEUIVIEWPROPSW    lpVP;
    LPOLEUILINKPROPSW    lpLP;
} OLEUIOBJECTPROPSW, *POLEUIOBJECTPROPSW, *LPOLEUIOBJECTPROPSW;

DECL_WINELIB_TYPE_AW(OLEUIOBJECTPROPS)
DECL_WINELIB_TYPE_AW(POLEUIOBJECTPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUIOBJECTPROPS)

#define OPF_OBJECTISLINK                0x00000001L
#define OPF_NOFILLDEFAULT               0x00000002L
#define OPF_SHOWHELP                    0x00000004L
#define OPF_DISABLECONVERT              0x00000008L


/************************************************************************************
 * CHANGE SOURCE DIALOG
 */


typedef struct tagOLEUICHANGESOURCEW
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCWSTR         lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCWSTR         lpszTemplate;
    HRSRC           hResource;
    OPENFILENAMEW*lpOFN;
    DWORD           dwReserved1[4];
    LPOLEUILINKCONTAINERW lpOleUILinkContainer;
    DWORD           dwLink;
    LPWSTR          lpszDisplayName;
    ULONG           nFileLength;
    LPWSTR          lpszFrom;
    LPWSTR          lpszTo;
} OLEUICHANGESOURCEW, *POLEUICHANGESOURCEW, *LPOLEUICHANGESOURCEW;


typedef struct tagOLEUICHANGESOURCEA
{
    DWORD           cbStruct;
    DWORD           dwFlags;
    HWND            hWndOwner;
    LPCSTR          lpszCaption;
    LPFNOLEUIHOOK   lpfnHook;
    LPARAM          lCustData;
    HINSTANCE       hInstance;
    LPCSTR          lpszTemplate;
    HRSRC           hResource;
    OPENFILENAMEA*  lpOFN;
    DWORD           dwReserved1[4];
    LPOLEUILINKCONTAINERA lpOleUILinkContainer;
    DWORD           dwLink;
    LPSTR           lpszDisplayName;
    ULONG           nFileLength;
    LPSTR           lpszFrom;
    LPSTR           lpszTo;
} OLEUICHANGESOURCEA, *POLEUICHANGESOURCEA, *LPOLEUICHANGESOURCEA;


DECL_WINELIB_TYPE_AW(OLEUICHANGESOURCE)
DECL_WINELIB_TYPE_AW(POLEUICHANGESOURCE)
DECL_WINELIB_TYPE_AW(LPOLEUICHANGESOURCE)

/* Change Source Dialog flags */
#define CSF_SHOWHELP                    0x00000001L
#define CSF_VALIDSOURCE                 0x00000002L
#define CSF_ONLYGETSOURCE               0x00000004L
#define CSF_EXPLORER                    0x00000008L


/*****************************************************************************
 * IOleUILinkContainer interface
 */
#define INTERFACE   IOleUILinkContainerA
DECLARE_INTERFACE_(IOleUILinkContainerA,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleUILinkContainerA methods ***/
    STDMETHOD_(DWORD,GetNextLink)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(SetLinkUpdateOptions)(THIS_ DWORD dwLink, DWORD dwUpdateOpt) PURE;
    STDMETHOD(GetLinkUpdateOptions)(THIS_ DWORD dwLink, DWORD *lpdwUpdateOpt) PURE;
    STDMETHOD(SetLinkSource)(THIS_ DWORD dwLink, LPSTR lpszDisplayName,
                ULONG lenFileName, ULONG *pchEaten, BOOL fValidateSource) PURE;
    STDMETHOD(GetLinkSource)(THIS_ DWORD dwLink, LPSTR *lplpszDisplayName, ULONG *lplenFileName,
                LPSTR *lplpszFullLinkType,  LPSTR *lplpszShortLinkType,
                BOOL *lpfSourceAvailable,  BOOL *lpfIsSelected) PURE;
    STDMETHOD(OpenLinkSource)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(UpdateLink)(THIS_ DWORD dwLink, BOOL fErrorMessage, BOOL fReserved) PURE;
    STDMETHOD(CancelLink)(THIS_ DWORD dwLink) PURE;
};
#undef INTERFACE

#define INTERFACE   IOleUILinkContainerW
DECLARE_INTERFACE_(IOleUILinkContainerW,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleUILinkContainerW methods ***/
    STDMETHOD_(DWORD,GetNextLink)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(SetLinkUpdateOptions)(THIS_ DWORD dwLink,  DWORD dwUpdateOpt) PURE;
    STDMETHOD(GetLinkUpdateOptions)(THIS_ DWORD dwLink,  DWORD *lpdwUpdateOpt) PURE;
    STDMETHOD(SetLinkSource)(THIS_ DWORD dwLink,  LPWSTR lpszDisplayName,
                ULONG lenFileName,  ULONG *pchEaten,  BOOL fValidateSource) PURE;
    STDMETHOD(GetLinkSource)(THIS_ DWORD dwLink, LPWSTR *lplpszDisplayName, ULONG *lplenFileName,
                LPWSTR *lplpszFullLinkType,  LPWSTR *lplpszShortLinkType,
                BOOL *lpfSourceAvailable,  BOOL *lpfIsSelected) PURE;
    STDMETHOD(OpenLinkSource)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(UpdateLink)(THIS_ DWORD dwLink, BOOL fErrorMessage, BOOL fReserved) PURE;
    STDMETHOD(CancelLink)(THIS_ DWORD dwLink) PURE;
};
#undef INTERFACE

DECL_WINELIB_TYPE_AW(IOleUILinkContainer)
DECL_WINELIB_TYPE_AW(POLEUILINKCONTAINER)
DECL_WINELIB_TYPE_AW(LPOLEUILINKCONTAINER)

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IOleUILinkContainer_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleUILinkContainer_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IOleUILinkContainer_Release(p)                     (p)->lpVtbl->Release(p)
/*** IOleUILinkContainer methods ***/
#define IOleUILinkContainer_GetNextLink(p,a)               (p)->lpVtbl->GetNextLink(p,a)
#define IOleUILinkContainer_SetLinkUpdateOptions(p,a,b)    (p)->lpVtbl->SetLinkUpdateOptions(p,a,b)
#define IOleUILinkContainer_GetLinkUpdateOptions(p,a,b)    (p)->lpVtbl->GetLinkUpdateOptions(p,a,b)
#define IOleUILinkContainer_SetLinkSource(p,a,b,c,d,e)     (p)->lpVtbl->SetLinkSource(p,a,b,c,d,e)
#define IOleUILinkContainer_GetLinkSource(p,a,b,c,d,e,f,g) (p)->lpVtbl->GetLinkSource(p,a,b,c,d,e,f,g)
#define IOleUILinkContainer_OpenLinkSource(p,a)            (p)->lpVtbl->OpenLinkSource(p,a)
#define IOleUILinkContainer_UpdateLink(p,a,b,c)            (p)->lpVtbl->UpdateLink(p,a,b,c)
#define IOleUILinkContainer_CancelLink(p,a)                (p)->lpVtbl->CancelLink(p,a)
#endif


/*****************************************************************************
 * IOleUILinkInfo interface
 */
#define INTERFACE   IOleUILinkInfoA
DECLARE_INTERFACE_(IOleUILinkInfoA,IOleUILinkContainerA)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleUILinkContainerA methods ***/
    STDMETHOD_(DWORD,GetNextLink)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(SetLinkUpdateOptions)(THIS_ DWORD dwLink, DWORD dwUpdateOpt) PURE;
    STDMETHOD(GetLinkUpdateOptions)(THIS_ DWORD dwLink, DWORD *lpdwUpdateOpt) PURE;
    STDMETHOD(SetLinkSource)(THIS_ DWORD dwLink, LPSTR lpszDisplayName,
                ULONG lenFileName, ULONG *pchEaten, BOOL fValidateSource) PURE;
    STDMETHOD(GetLinkSource)(THIS_ DWORD dwLink, LPSTR *lplpszDisplayName, ULONG *lplenFileName,
                LPSTR *lplpszFullLinkType,  LPSTR *lplpszShortLinkType,
                BOOL *lpfSourceAvailable,  BOOL *lpfIsSelected) PURE;
    STDMETHOD(OpenLinkSource)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(UpdateLink)(THIS_ DWORD dwLink, BOOL fErrorMessage, BOOL fReserved) PURE;
    STDMETHOD(CancelLink)(THIS_ DWORD dwLink) PURE;
    /*** IOleUILinkInfoA methods ***/
    STDMETHOD(GetLastUpdate)(THIS_ DWORD dwLink,  FILETIME *lpLastUpdate) PURE;
};
#undef INTERFACE

#define INTERFACE   IOleUILinkInfoW
DECLARE_INTERFACE_(IOleUILinkInfoW,IOleUILinkContainerW)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleUILinkContainerW methods ***/
    STDMETHOD_(DWORD,GetNextLink)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(SetLinkUpdateOptions)(THIS_ DWORD dwLink,  DWORD dwUpdateOpt) PURE;
    STDMETHOD(GetLinkUpdateOptions)(THIS_ DWORD dwLink,  DWORD *lpdwUpdateOpt) PURE;
    STDMETHOD(SetLinkSource)(THIS_ DWORD dwLink,  LPWSTR lpszDisplayName,
                ULONG lenFileName,  ULONG *pchEaten,  BOOL fValidateSource) PURE;
    STDMETHOD(GetLinkSource)(THIS_ DWORD dwLink, LPWSTR *lplpszDisplayName, ULONG *lplenFileName,
                LPWSTR *lplpszFullLinkType,  LPWSTR *lplpszShortLinkType,
                BOOL *lpfSourceAvailable,  BOOL *lpfIsSelected) PURE;
    STDMETHOD(OpenLinkSource)(THIS_ DWORD dwLink) PURE;
    STDMETHOD(UpdateLink)(THIS_ DWORD dwLink, BOOL fErrorMessage, BOOL fReserved) PURE;
    STDMETHOD(CancelLink)(THIS_ DWORD dwLink) PURE;
    /*** IOleUILinkInfoW methods ***/
    STDMETHOD(GetLastUpdate)(THIS_ DWORD dwLink,  FILETIME *lpLastUpdate) PURE;
};
#undef  INTERFACE

DECL_WINELIB_TYPE_AW(IOleUILinkInfo)
DECL_WINELIB_TYPE_AW(POLEUILINKINFO)
DECL_WINELIB_TYPE_AW(LPOLEUILINKINFO)

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IOleUILinkInfo_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleUILinkInfo_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IOleUILinkInfo_Release(p)                     (p)->lpVtbl->Release(p)
/*** IOleUILinkContainer methods ***/
#define IOleUILinkInfo_GetNextLink(p,a)               (p)->lpVtbl->GetNextLink(p,a)
#define IOleUILinkInfo_SetLinkUpdateOptions(p,a,b)    (p)->lpVtbl->SetLinkUpdateOptions(p,a,b)
#define IOleUILinkInfo_GetLinkUpdateOptions(p,a,b)    (p)->lpVtbl->GetLinkUpdateOptions(p,a,b)
#define IOleUILinkInfo_SetLinkSource(p,a,b,c,d,e)     (p)->lpVtbl->SetLinkSource(p,a,b,c,d,e)
#define IOleUILinkInfo_GetLinkSource(p,a,b,c,d,e,f,g) (p)->lpVtbl->GetLinkSource(p,a,b,c,d,e,f,g)
#define IOleUILinkInfo_OpenLinkSource(p,a)            (p)->lpVtbl->OpenLinkSource(p,a)
#define IOleUILinkInfo_UpdateLink(p,a,b,c)            (p)->lpVtbl->UpdateLink(p,a,b,c)
#define IOleUILinkInfo_CancelLink(p,a)                (p)->lpVtbl->CancelLink(p,a)
/*** IOleUILinkInfo methods ***/
#define IOleUILinkInfo_GetLastUpdate(p,a,b)           (p)->lpVtbl->GetLastUpdate(p,a,b)
#endif


/*****************************************************************************
 * IOleUIObjInfo interface
 */
#define INTERFACE   IOleUIObjInfoA
DECLARE_INTERFACE_(IOleUIObjInfoA,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleUIObjInfoA methods ***/
    STDMETHOD(GetObjectInfo)(THIS_ DWORD dwObject, DWORD *lpdwObjSize, LPSTR *lplpszLabel,
                LPSTR *lplpszType, LPSTR *lplpszShortType, LPSTR *lplpszLocation) PURE;
    STDMETHOD(GetConvertInfo)(THIS_ DWORD dwObject, CLSID *lpClassID, WORD *lpwFormat,
                CLSID *lpConvertDefaultClassID, LPCLSID *lplpClsidExclude, UINT *lpcClsidExclude) PURE;
    STDMETHOD(ConvertObject)(THIS_ DWORD dwObject,  REFCLSID clsidNew) PURE;
    STDMETHOD(GetViewInfo)(THIS_ DWORD dwObject, HGLOBAL *phMetaPict, DWORD *pdvAspect, INT *pnCurrentScale) PURE;
    STDMETHOD(SetViewInfo)(THIS_ DWORD dwObject, HGLOBAL hMetaPict, DWORD dvAspect,
                INT nCurrentScale, BOOL bRelativeToOrig) PURE;
};
#undef INTERFACE

#define INTERFACE   IOleUIObjInfoW
DECLARE_INTERFACE_(IOleUIObjInfoW,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IOleUIObjInfoW methods ***/
    STDMETHOD(GetObjectInfo)(THIS_ DWORD dwObject, DWORD *lpdwObjSize, LPWSTR *lplpszLabel,
                LPWSTR *lplpszType,  LPWSTR *lplpszShortType,  LPWSTR *lplpszLocation) PURE;
    STDMETHOD(GetConvertInfo)(THIS_ DWORD dwObject, CLSID *lpClassID, WORD *lpwFormat,
                CLSID *lpConvertDefaultClassID, LPCLSID *lplpClsidExclude, UINT *lpcClsidExclude) PURE;
    STDMETHOD(ConvertObject)(THIS_ DWORD dwObject,  REFCLSID clsidNew) PURE;
    STDMETHOD(GetViewInfo)(THIS_ DWORD dwObject, HGLOBAL *phMetaPict, DWORD *pdvAspect, INT *pnCurrentScale) PURE;
    STDMETHOD(SetViewInfo)(THIS_ DWORD dwObject, HGLOBAL hMetaPict, DWORD dvAspect,
                INT nCurrentScale, BOOL bRelativeToOrig) PURE;
};
#undef INTERFACE

DECL_WINELIB_TYPE_AW(IOleUIObjInfo)
DECL_WINELIB_TYPE_AW(POLEUIOBJINFO)
DECL_WINELIB_TYPE_AW(LPOLEUIOBJINFO)

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IOleUIObjInfo_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IOleUIObjInfo_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IOleUIObjInfo_Release(p)                       (p)->lpVtbl->Release(p)
/*** IOleUIObjInfo methods ***/
#define IOleUIObjInfo_GetObjectInfo(p,a,b,c,d,e,f)     (p)->lpVtbl->GetObjectInfo(p,a,b,c,d,e,f)
#define IOleUIObjInfo_GetConvertInfo(p,a,b,c,d,e,f)    (p)->lpVtbl->GetConvertInfo(p,a,b,c,d,e,f)
#define IOleUIObjInfo_ConvertObject(p,a,b)             (p)->lpVtbl->ConvertObject(p,a,b)
#define IOleUIObjInfo_GetViewInfo(p,a,b,c,d)           (p)->lpVtbl->GetViewInfo(p,a,b,c,d)
#define IOleUIObjInfo_SetViewInfo(p,a,b,c,d,e)         (p)->lpVtbl->SetViewInfo(p,a,b,c,d,e)
#endif

UINT WINAPI  OleUIInsertObjectW(LPOLEUIINSERTOBJECTW);
UINT WINAPI  OleUIInsertObjectA(LPOLEUIINSERTOBJECTA);
#define      OleUIInsertObject WINELIB_NAME_AW(OleUIInsertObject)
UINT WINAPI  OleUIConvertA(LPOLEUICONVERTA);
UINT WINAPI  OleUIConvertW(LPOLEUICONVERTW);
#define      OleUIConvert WINELIB_NAME_AW(OleUIConvert)
UINT WINAPI  OleUIChangeIconA(LPOLEUICHANGEICONA);
UINT WINAPI  OleUIChangeIconW(LPOLEUICHANGEICONW);
#define      OleUIChangeIcon WINELIB_NAME_AW(OleUIChangeIcon)
UINT WINAPI  OleUIBusyA(LPOLEUIBUSYA);
UINT WINAPI  OleUIBusyW(LPOLEUIBUSYW);
#define      OleUIBusy WINELIB_NAME_AW(OleUIBusy)
UINT WINAPI  OleUIObjectPropertiesA(LPOLEUIOBJECTPROPSA);
UINT WINAPI  OleUIObjectPropertiesW(LPOLEUIOBJECTPROPSW);
#define      OleUIObjectProperties WINELIB_NAME_AW(OleUIObjectProperties)
UINT WINAPI  OleUIChangeSourceW(LPOLEUICHANGESOURCEW);
UINT WINAPI  OleUIChangeSourceA(LPOLEUICHANGESOURCEA);
#define      OleUIChangeSource WINELIB_NAME_AW(OleUIChangeSource)
UINT WINAPI  OleUIEditLinksA(LPOLEUIEDITLINKSA lpOleUIEditLinks);
UINT WINAPI  OleUIEditLinksW(LPOLEUIEDITLINKSW lpOleUIEditLinks);
#define      OleUIEditLinks WINELIB_NAME_AW(OleUIEditLinks)
BOOL WINAPI  OleUIUpdateLinksA(LPOLEUILINKCONTAINERA lpOleUILinkCntr, HWND hwndParent, LPSTR lpszTitle, INT cLinks);
BOOL WINAPI  OleUIUpdateLinksW(LPOLEUILINKCONTAINERW lpOleUILinkCntr, HWND hwndParent, LPWSTR lpszTitle, INT cLinks);
#define      OleUIUpdateLinks WINELIB_NAME_AW(OleUIUpdateLinks)
BOOL WINAPI  OleUIAddVerbMenuA(LPOLEOBJECT lpOleObj, LPCSTR lpszShortType, HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
  BOOL bAddConvert, UINT idConvert, HMENU *lphMenu);
BOOL WINAPI  OleUIAddVerbMenuW(LPOLEOBJECT lpOleObj, LPCWSTR lpszShortType, HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
  BOOL bAddConvert, UINT idConvert, HMENU *lphMenu);
#define      OleUIAddVerbMenu WINELIB_NAME_AW(OleUIAddVerbMenu)
UINT WINAPI  OleUIPasteSpecialA(LPOLEUIPASTESPECIALA lpOleUIPasteSpecial);
UINT WINAPI  OleUIPasteSpecialW(LPOLEUIPASTESPECIALW lpOleUIPasteSpecial);
#define      OleUIPasteSpecial WINELIB_NAME_AW(OleUIPasteSpecial)
INT  __cdecl OleUIPromptUserA(INT,HWND, ...);
INT  __cdecl OleUIPromptUserW(INT,HWND, ...);
#define      OleUIPromptUser WINELIB_NAME_AW(OleUIPromptUser)

#ifdef __cplusplus
} /* Extern "C" */
#endif


#endif  /* __WINE_OLEDLG_H */
