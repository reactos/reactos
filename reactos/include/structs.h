/*
   Structures.h

   Declarations for all the Windows32 API Structures

   Copyright (C) 1996 Free Software Foundation, Inc.

   Author:  Scott Christley <scottc@net-community.com>
   Date: 1996

   This file is part of the Windows32 API Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   If you are interested in a warranty or support for this source code,
   contact Scott Christley <scottc@net-community.com> for more information.

   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef _GNU_H_WINDOWS32_STRUCTURES
#define _GNU_H_WINDOWS32_STRUCTURES

#include <base.h>
#include <ntos/security.h>
#include <ntos/time.h>
#include <ntdll/rtl.h>
#include <ntos/console.h>
#include <ntos/keyboard.h>
#include <ntos/heap.h>
#include <ntos/mm.h>
#include <ntos/file.h>
#include <ntos/ps.h>
#include <ntos/disk.h>
#include <ntos/gditypes.h>

/* NOTE - _DISABLE_TIDENTS exists to keep ReactOS's source from
   accidentally utilitizing ANSI/UNICODE-generic structs, defines
   or functions. */
#ifndef _DISABLE_TIDENTS
#  ifdef UNICODE
#    define typedef_tident(ident) typedef ident##W ident;
#  else
#    define typedef_tident(ident) typedef ident##A ident;
#  endif
#else
#  define typedef_tident(ident)
#endif

typedef struct _VALENT_A {
   LPSTR ve_valuename;
   DWORD ve_valuelen;
   DWORD ve_valueptr;
   DWORD ve_type;
} VALENTA, *PVALENTA;

typedef struct _VALENT_W {
   LPWSTR ve_valuename;
   DWORD  ve_valuelen;
   DWORD  ve_valueptr;
   DWORD  ve_type;
} VALENTW, *PVALENTW;

typedef_tident(VALENT)
typedef_tident(PVALENT)

#ifndef WIN32_LEAN_AND_MEAN

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _ABC {
  int     abcA;
  UINT    abcB;
  int     abcC;
} ABC, *LPABC;

typedef struct _ABCFLOAT {
  FLOAT   abcfA;
  FLOAT   abcfB;
  FLOAT   abcfC;
} ABCFLOAT, *LPABCFLOAT;

typedef struct tagACCEL {
  BYTE   fVirt;
  WORD   key;
  WORD   cmd;
} ACCEL, *LPACCEL;

typedef struct tagACCESSTIMEOUT {
  UINT  cbSize;
  DWORD dwFlags;
  DWORD iTimeOutMSec;
} ACCESSTIMEOUT;

typedef struct _ACTION_HEADER {
  ULONG   transport_id;
  USHORT  action_code;
  USHORT  reserved;
} ACTION_HEADER;

typedef struct _ADAPTER_STATUS {
  UCHAR   adapter_address[6];
  UCHAR   rev_major;
  UCHAR   reserved0;
  UCHAR   adapter_type;
  UCHAR   rev_minor;
  WORD    duration;
  WORD    frmr_recv;
  WORD    frmr_xmit;
  WORD    iframe_recv_err;
  WORD    xmit_aborts;
  DWORD   xmit_success;
  DWORD   recv_success;
  WORD    iframe_xmit_err;
  WORD    recv_buff_unavail;
  WORD    t1_timeouts;
  WORD    ti_timeouts;
  DWORD   reserved1;
  WORD    free_ncbs;
  WORD    max_cfg_ncbs;
  WORD    max_ncbs;
  WORD    xmit_buf_unavail;
  WORD    max_dgram_size;
  WORD    pending_sess;
  WORD    max_cfg_sess;
  WORD    max_sess;
  WORD    max_sess_pkt_size;
  WORD    name_count;
} ADAPTER_STATUS;

typedef struct _ADDJOB_INFO_1A {
  LPSTR   Path;
  DWORD   JobId;
} ADDJOB_INFO_1A;

typedef struct _ADDJOB_INFO_1W {
  LPWSTR  Path;
  DWORD   JobId;
} ADDJOB_INFO_1W;

typedef_tident(ADDJOB_INFO_1)

typedef struct tagANIMATIONINFO {
  UINT cbSize;
  int  iMinAnimate;
} ANIMATIONINFO, *LPANIMATIONINFO;

typedef struct _RECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *LPRECT, *LPCRECT, *PRECT,
RECTL, *LPRECTL, *LPCRECTL, *PRECTL;

typedef struct _PATRECT {
	RECT r;
	HBRUSH hBrush;
} PATRECT, * PPATRECT;

#if 0
typedef struct _RECTL {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECTL, *LPRECTL, *LPCRECTL, *PRECTL;
#endif

typedef struct _AppBarData {
  DWORD  cbSize;
  HWND   hWnd;
  UINT   uCallbackMessage;
  UINT   uEdge;
  RECT   rc;
  LPARAM lParam;
} APPBARDATA, *PAPPBARDATA;

typedef struct tagBITMAP
{
  LONG        bmType;
  LONG        bmWidth;
  LONG        bmHeight;
  LONG        bmWidthBytes;
  WORD        bmPlanes;
  WORD        bmBitsPixel;
  LPVOID      bmBits;
} BITMAP, *PBITMAP,   *NPBITMAP,   *LPBITMAP;

typedef struct tagBITMAPCOREHEADER {
  DWORD   bcSize;
  WORD    bcWidth;
  WORD    bcHeight;
  WORD    bcPlanes;
  WORD    bcBitCount;
} BITMAPCOREHEADER;

typedef struct tagRGBTRIPLE {
  BYTE rgbtBlue;
  BYTE rgbtGreen;
  BYTE rgbtRed;
} RGBTRIPLE, *PRGBTRIPLE;

typedef struct _BITMAPCOREINFO {
  BITMAPCOREHEADER  bmciHeader;
  RGBTRIPLE         bmciColors[1];
} BITMAPCOREINFO, *PBITMAPCOREINFO, *LPBITMAPCOREINFO;

#include <pshpack1.h>

typedef struct tagBITMAPFILEHEADER {
  WORD    bfType;
  DWORD   bfSize;
  WORD    bfReserved1;
  WORD    bfReserved2;
  DWORD   bfOffBits;
} BITMAPFILEHEADER;

#include <poppack.h>

typedef struct tagBITMAPINFOHEADER {
  DWORD  biSize;
  LONG   biWidth;
  LONG   biHeight;
  WORD   biPlanes;
  WORD   biBitCount;
  DWORD  biCompression;
  DWORD  biSizeImage;
  LONG   biXPelsPerMeter;
  LONG   biYPelsPerMeter;
  DWORD  biClrUsed;
  DWORD  biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  BYTE    rgbBlue;
  BYTE    rgbGreen;
  BYTE    rgbRed;
  BYTE    rgbReserved;
} RGBQUAD, *PRGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFO {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;

typedef long FXPT2DOT30,  * LPFXPT2DOT30;

typedef struct tagCIEXYZ
{
  FXPT2DOT30 ciexyzX;
  FXPT2DOT30 ciexyzY;
  FXPT2DOT30 ciexyzZ;
} CIEXYZ;
typedef CIEXYZ  * LPCIEXYZ;

typedef struct tagCIEXYZTRIPLE
{
  CIEXYZ  ciexyzRed;
  CIEXYZ  ciexyzGreen;
  CIEXYZ  ciexyzBlue;
} CIEXYZTRIPLE;
typedef CIEXYZTRIPLE  * LPCIEXYZTRIPLE;

typedef struct {
  DWORD        bV4Size;
  LONG         bV4Width;
  LONG         bV4Height;
  WORD         bV4Planes;
  WORD         bV4BitCount;
  DWORD        bV4V4Compression;
  DWORD        bV4SizeImage;
  LONG         bV4XPelsPerMeter;
  LONG         bV4YPelsPerMeter;
  DWORD        bV4ClrUsed;
  DWORD        bV4ClrImportant;
  DWORD        bV4RedMask;
  DWORD        bV4GreenMask;
  DWORD        bV4BlueMask;
  DWORD        bV4AlphaMask;
  DWORD        bV4CSType;
  CIEXYZTRIPLE bV4Endpoints;
  DWORD        bV4GammaRed;
  DWORD        bV4GammaGreen;
  DWORD        bV4GammaBlue;
} BITMAPV4HEADER,   *LPBITMAPV4HEADER, *PBITMAPV4HEADER;

#ifndef __BLOB_T_DEFINED
#define __BLOB_T_DEFINED
typedef struct _BLOB {
  ULONG   cbSize;
  BYTE    *pBlobData;
} BLOB, *PBLOB,*LPBLOB;
#endif

typedef struct _SHITEMID {
  USHORT cb;
  BYTE   abID[1];
} SHITEMID, * LPSHITEMID;
typedef const SHITEMID  * LPCSHITEMID;

typedef struct _ITEMIDLIST {
  SHITEMID mkid;
} ITEMIDLIST, * LPITEMIDLIST;
typedef const ITEMIDLIST * LPCITEMIDLIST;

typedef struct _browseinfo {
  HWND hwndOwner;
  LPCITEMIDLIST pidlRoot;
  LPSTR pszDisplayName;
  LPCSTR lpszTitle;
  UINT ulFlags;
  BFFCALLBACK lpfn;
  LPARAM lParam;
  int iImage;
} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;

typedef struct _BY_HANDLE_FILE_INFORMATION {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    dwVolumeSerialNumber;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    nNumberOfLinks;
  DWORD    nFileIndexHigh;
  DWORD    nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

typedef struct _FIXED {
  WORD  fract;
  short value;
} FIXED;

typedef struct tagPOINT {
  LONG x;
  LONG y;
} POINT, *LPPOINT, *PPOINT;

typedef struct tagPOINTFX {
  FIXED x;
  FIXED y;
} POINTFX;

typedef struct _POINTL {
  LONG x;
  LONG y;
} POINTL, *PPOINTL;

typedef struct tagPOINTS {
  SHORT x;
  SHORT y;
} POINTS;

typedef struct {
	ULONG State[4];
	ULONG Unknown[2];
} MD4_CONTEXT, *PMD4_CONTEXT;

typedef struct {
	ULONG Unknown[2];
	ULONG State[4];
} MD5_CONTEXT, *PMD5_CONTEXT;

typedef struct {
	ULONG Unknown1[6];
	ULONG State[5];
	ULONG Unknown2[2];
} SHA_CONTEXT, *PSHA_CONTEXT;

typedef struct _tagCANDIDATEFORM {
  DWORD  dwIndex;
  DWORD  dwStyle;
  POINT  ptCurrentPos;
  RECT   rcArea;
} CANDIDATEFORM, *LPCANDIDATEFORM;

typedef struct _tagCANDIDATELIST {
  DWORD  dwSize;
  DWORD  dwStyle;
  DWORD  dwCount;
  DWORD  dwSelection;
  DWORD  dwPageStart;
  DWORD  dwPageSize;
  DWORD  dwOffset[1];
} CANDIDATELIST, *LPCANDIDATELIST;

typedef struct tagCREATESTRUCTA {
  LPVOID    lpCreateParams;
  HINSTANCE hInstance;
  HMENU     hMenu;
  HWND      hwndParent;
  int       cy;
  int       cx;
  int       y;
  int       x;
  LONG      style;
  LPCSTR   lpszName;
  LPCSTR   lpszClass;
  DWORD     dwExStyle;
} CREATESTRUCTA, *LPCREATESTRUCTA;

typedef struct tagCREATESTRUCTW {
  LPVOID    lpCreateParams;
  HINSTANCE hInstance;
  HMENU     hMenu;
  HWND      hwndParent;
  int       cy;
  int       cx;
  int       y;
  int       x;
  LONG      style;
  LPCWSTR   lpszName;
  LPCWSTR   lpszClass;
  DWORD     dwExStyle;
} CREATESTRUCTW, *LPCREATESTRUCTW;

typedef_tident(CREATESTRUCT)
typedef_tident(LPCREATESTRUCT)

typedef struct tagHW_PROFILE_INFOA {
    DWORD  dwDockInfo;
    CHAR   szHwProfileGuid[HW_PROFILE_GUIDLEN];
    CHAR   szHwProfileName[MAX_PROFILE_LEN];
} HW_PROFILE_INFOA, *LPHW_PROFILE_INFOA;
typedef struct tagHW_PROFILE_INFOW {
    DWORD  dwDockInfo;
    WCHAR  szHwProfileGuid[HW_PROFILE_GUIDLEN];
    WCHAR  szHwProfileName[MAX_PROFILE_LEN];
} HW_PROFILE_INFOW, *LPHW_PROFILE_INFOW;

typedef_tident(HW_PROFILE_INFO)
typedef_tident(LPHW_PROFILE_INFO)
typedef struct _tagDATETIME {
    WORD    year;
    WORD    month;
    WORD    day;
    WORD    hour;
    WORD    min;
    WORD    sec;
} DATETIME;

typedef struct _tagIMEPROA {
    HWND        hWnd;
    DATETIME    InstDate;
    UINT        wVersion;
    BYTE        szDescription[50];
    BYTE        szName[80];
    BYTE        szOptions[30];
} IMEPROA,*PIMEPROA,*NPIMEPROA,FAR *LPIMEPROA;
typedef struct _tagIMEPROW {
    HWND        hWnd;
    DATETIME    InstDate;
    UINT        wVersion;
    WCHAR       szDescription[50];
    WCHAR       szName[80];
    WCHAR       szOptions[30];
} IMEPROW,*PIMEPROW,*NPIMEPROW,FAR *LPIMEPROW;
typedef_tident(IMEPRO)
typedef_tident(NPIMEPRO)
typedef_tident(LPIMEPRO)
typedef_tident(PIMEPRO)

typedef struct _cpinfoexA {
    UINT    MaxCharSize;
    BYTE    DefaultChar[MAX_DEFAULTCHAR];
    BYTE    LeadByte[MAX_LEADBYTES];
    WCHAR   UnicodeDefaultChar;
    UINT    CodePage;
    CHAR    CodePageName[MAX_PATH];
} CPINFOEXA, *LPCPINFOEXA;
typedef struct _cpinfoexW {
    UINT    MaxCharSize;
    BYTE    DefaultChar[MAX_DEFAULTCHAR];
    BYTE    LeadByte[MAX_LEADBYTES];
    WCHAR   UnicodeDefaultChar;
    UINT    CodePage;
    WCHAR   CodePageName[MAX_PATH];
} CPINFOEXW, *LPCPINFOEXW;

typedef_tident(CPINFOEX)
typedef_tident(LPCPINFOEX)

typedef struct tagCBT_CREATEWNDA {
  LPCREATESTRUCTA lpcs;
  HWND            hwndInsertAfter;
} CBT_CREATEWNDA;

typedef struct tagCBT_CREATEWNDW {
  LPCREATESTRUCTW lpcs;
  HWND            hwndInsertAfter;
} CBT_CREATEWNDW;

typedef_tident(CBT_CREATEWND)

typedef struct tagCBTACTIVATESTRUCT {
  WINBOOL fMouse;
  HWND hWndActive;
} CBTACTIVATESTRUCT;

typedef struct _CHAR_INFO {
  union {
    WCHAR UnicodeChar;
    CHAR AsciiChar;
  } Char;
  WORD Attributes;
} CHAR_INFO, *PCHAR_INFO;

typedef struct _charformatA {
  UINT     cbSize;
  DWORD    dwMask;
  DWORD    dwEffects;
  LONG     yHeight;
  LONG     yOffset;
  COLORREF crTextColor;
  BYTE     bCharSet;
  BYTE     bPitchAndFamily;
  CHAR     szFaceName[LF_FACESIZE];
} CHARFORMATA;

typedef struct _charformatW {
  UINT     cbSize;
  DWORD    dwMask;
  DWORD    dwEffects;
  LONG     yHeight;
  LONG     yOffset;
  COLORREF crTextColor;
  BYTE     bCharSet;
  BYTE     bPitchAndFamily;
  WCHAR    szFaceName[LF_FACESIZE];
} CHARFORMATW;

typedef_tident(CHARFORMAT)

typedef struct _charrange {
  LONG cpMin;
  LONG cpMax;
} CHARRANGE;

typedef struct tagCHARSET {
  DWORD aflBlock[3];
  DWORD flLang;
} CHARSET;

typedef struct tagFONTSIGNATURE {
  DWORD  fsUsb[4];
  DWORD  fsCsb[2];
} FONTSIGNATURE, *LPFONTSIGNATURE;

typedef struct {
  UINT ciCharset;
  UINT ciACP;
  FONTSIGNATURE fs;
} CHARSETINFO, *LPCHARSETINFO;

typedef struct tagWCRANGE
{
    WCHAR  wcLow;
    USHORT cGlyphs;
} WCRANGE, *PWCRANGE,FAR *LPWCRANGE;

typedef struct tagGLYPHSET
{
    DWORD    cbThis;
    DWORD    flAccel;
    DWORD    cGlyphsSupported;
    DWORD    cRanges;
    WCRANGE  ranges[1];
} GLYPHSET, *PGLYPHSET, FAR *LPGLYPHSET;

typedef struct {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HWND         hInstance;
  COLORREF     rgbResult;
  COLORREF*    lpCustColors;
  DWORD        Flags;
  LPARAM       lCustData;
  LPCCHOOKPROC lpfnHook;
  LPCSTR       lpTemplateName;
} CHOOSECOLORA, *LPCHOOSECOLORA;

typedef struct {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HWND         hInstance;
  COLORREF     rgbResult;
  COLORREF*    lpCustColors;
  DWORD        Flags;
  LPARAM       lCustData;
  LPCCHOOKPROC lpfnHook;
  LPCWSTR      lpTemplateName;
} CHOOSECOLORW, *LPCHOOSECOLORW;

typedef_tident(CHOOSECOLOR)
typedef_tident(LPCHOOSECOLOR)

typedef struct _OBJECT_TYPE_LIST {
    WORD   Level;
    WORD   Sbz;
    GUID *ObjectType;
} OBJECT_TYPE_LIST, *POBJECT_TYPE_LIST;

typedef struct tagLOGFONTA {
  LONG lfHeight;
  LONG lfWidth;
  LONG lfEscapement;
  LONG lfOrientation;
  LONG lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, *LPLOGFONTA, *PLOGFONTA;

typedef struct tagLOGFONTW {
  LONG lfHeight;
  LONG lfWidth;
  LONG lfEscapement;
  LONG lfOrientation;
  LONG lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  WCHAR lfFaceName[LF_FACESIZE];
} LOGFONTW, *LPLOGFONTW, *PLOGFONTW;

typedef_tident(LOGFONT)
typedef_tident(LPLOGFONT)
typedef_tident(PLOGFONT)

typedef struct tagRAWINPUTHEADER {
    DWORD dwType;
    DWORD dwSize;
    HANDLE hDevice;
    WPARAM wParam;
} RAWINPUTHEADER, *PRAWINPUTHEADER, *LPRAWINPUTHEADER;

typedef struct tagRAWINPUTDEVICELIST {
    HANDLE hDevice;
    DWORD dwType;
} RAWINPUTDEVICELIST, *PRAWINPUTDEVICELIST;

typedef struct tagRAWINPUTDEVICE {
    USHORT usUsagePage;
    USHORT usUsage;
    DWORD dwFlags;
    HWND hwndTarget;
} RAWINPUTDEVICE, *PRAWINPUTDEVICE, *LPRAWINPUTDEVICE;

typedef CONST RAWINPUTDEVICE* PCRAWINPUTDEVICE;

typedef struct tagRAWMOUSE {
    USHORT usFlags;
    union {
        ULONG ulButtons;
        struct  {
            USHORT  usButtonFlags;
            USHORT  usButtonData;
        };
    };
    ULONG ulRawButtons;
    LONG lLastX;
    LONG lLastY;
    ULONG ulExtraInformation;
} RAWMOUSE, *PRAWMOUSE, *LPRAWMOUSE;

typedef struct tagRAWKEYBOARD {
    USHORT MakeCode;
    USHORT Flags;
    USHORT Reserved;
    USHORT VKey;
    UINT   Message;
    ULONG ExtraInformation;
} RAWKEYBOARD, *PRAWKEYBOARD, *LPRAWKEYBOARD;

typedef struct tagRAWHID {
    DWORD dwSizeHid;
    DWORD dwCount;
    BYTE bRawData[1];
} RAWHID, *PRAWHID, *LPRAWHID;

typedef struct tagRAWINPUT {
    RAWINPUTHEADER header;
    union {
        RAWMOUSE    mouse;
        RAWKEYBOARD keyboard;
        RAWHID      hid;
    } data;
} RAWINPUT, *PRAWINPUT, *LPRAWINPUT;

typedef struct tagCHOOSEFONTA {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HDC          hDC;
  LPLOGFONTA   lpLogFont;
  INT          iPointSize;
  DWORD        Flags;
  DWORD        rgbColors;
  LPARAM       lCustData;
  LPCFHOOKPROC lpfnHook;
  LPCSTR       lpTemplateName;
  HINSTANCE    hInstance;
  LPSTR        lpszStyle;
  WORD         nFontType;
  WORD         ___MISSING_ALIGNMENT__;
  INT          nSizeMin;
  INT          nSizeMax;
} CHOOSEFONTA, *LPCHOOSEFONTA;

typedef struct tagCHOOSEFONTW {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HDC          hDC;
  LPLOGFONTW   lpLogFont;
  INT          iPointSize;
  DWORD        Flags;
  DWORD        rgbColors;
  LPARAM       lCustData;
  LPCFHOOKPROC lpfnHook;
  LPCWSTR      lpTemplateName;
  HINSTANCE    hInstance;
  LPWSTR        lpszStyle;
  WORD         nFontType;
  WORD         ___MISSING_ALIGNMENT__;
  INT          nSizeMin;
  INT          nSizeMax;
} CHOOSEFONTW, *LPCHOOSEFONTW;

typedef_tident(CHOOSEFONT)
typedef_tident(LPCHOOSEFONT)

typedef struct _IDA {
  UINT cidl;
  UINT aoffset[1];
} CIDA, * LPIDA;

typedef struct tagCLIENTCREATESTRUCT {
  HANDLE hWindowMenu;
  UINT   idFirstChild;
} CLIENTCREATESTRUCT;

typedef CLIENTCREATESTRUCT *LPCLIENTCREATESTRUCT;

typedef struct _CMInvokeCommandInfo {
  DWORD cbSize;
  DWORD fMask;
  HWND hwnd;
  LPCSTR lpVerb;
  LPCSTR lpParameters;
  LPCSTR lpDirectory;
  int nShow;
  DWORD dwHotKey;
  HANDLE hIcon;
} CMINVOKECOMMANDINFO, *LPCMINVOKECOMMANDINFO;

typedef struct  tagCOLORADJUSTMENT {
  WORD  caSize;
  WORD  caFlags;
  WORD  caIlluminantIndex;
  WORD  caRedGamma;
  WORD  caGreenGamma;
  WORD  caBlueGamma;
  WORD  caReferenceBlack;
  WORD  caReferenceWhite;
  SHORT caContrast;
  SHORT caBrightness;
  SHORT caColorfulness;
  SHORT caRedGreenTint;
} COLORADJUSTMENT, *LPCOLORADJUSTMENT;

typedef struct _COLORMAP {
  COLORREF from;
  COLORREF to;
} COLORMAP,  * LPCOLORMAP;

typedef struct _DCB {
  DWORD DCBlength;
  DWORD BaudRate;
  DWORD fBinary: 1;
  DWORD fParity: 1;
  DWORD fOutxCtsFlow:1;
  DWORD fOutxDsrFlow:1;
  DWORD fDtrControl:2;
  DWORD fDsrSensitivity:1;
  DWORD fTXContinueOnXoff:1;
  DWORD fOutX: 1;
  DWORD fInX: 1;
  DWORD fErrorChar: 1;
  DWORD fNull: 1;
  DWORD fRtsControl:2;
  DWORD fAbortOnError:1;
  DWORD fDummy2:17;
  WORD wReserved;
  WORD XonLim;
  WORD XoffLim;
  BYTE ByteSize;
  BYTE Parity;
  BYTE StopBits;
  char XonChar;
  char XoffChar;
  char ErrorChar;
  char EofChar;
  char EvtChar;
  WORD wReserved1;
} DCB, *LPDCB;

typedef struct _COMM_CONFIG {
  DWORD dwSize;
  WORD  wVersion;
  WORD  wReserved;
  DCB   dcb;
  DWORD dwProviderSubType;
  DWORD dwProviderOffset;
  DWORD dwProviderSize;
  WCHAR wcProviderData[1];
} COMMCONFIG, *LPCOMMCONFIG;

typedef struct _COMMPROP {
  WORD  wPacketLength;
  WORD  wPacketVersion;
  DWORD dwServiceMask;
  DWORD dwReserved1;
  DWORD dwMaxTxQueue;
  DWORD dwMaxRxQueue;
  DWORD dwMaxBaud;
  DWORD dwProvSubType;
  DWORD dwProvCapabilities;
  DWORD dwSettableParams;
  DWORD dwSettableBaud;
  WORD  wSettableData;
  WORD  wSettableStopParity;
  DWORD dwCurrentTxQueue;
  DWORD dwCurrentRxQueue;
  DWORD dwProvSpec1;
  DWORD dwProvSpec2;
  WCHAR wcProvChar[1];
} COMMPROP, *LPCOMMPROP;

typedef struct _COMMTIMEOUTS {
  DWORD ReadIntervalTimeout;
  DWORD ReadTotalTimeoutMultiplier;
  DWORD ReadTotalTimeoutConstant;
  DWORD WriteTotalTimeoutMultiplier;
  DWORD WriteTotalTimeoutConstant;
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;

typedef struct tagCOMPAREITEMSTRUCT {
	UINT	CtlType;
	UINT	CtlID;
	HWND	hwndItem;
	UINT	itemID1;
	DWORD	itemData1;
	UINT	itemID2;
	DWORD	itemData2;
	DWORD	dwLocaleId;
} COMPAREITEMSTRUCT,*LPCOMPAREITEMSTRUCT;


typedef struct {
  COLORREF crText;
  COLORREF crBackground;
  DWORD dwEffects;
} COMPCOLOR;

typedef struct _tagCOMPOSITIONFORM {
  DWORD  dwStyle;
  POINT  ptCurrentPos;
  RECT   rcArea;
} COMPOSITIONFORM, *LPCOMPOSITIONFORM;

typedef struct _COMSTAT {
  DWORD fCtsHold : 1;
  DWORD fDsrHold : 1;
  DWORD fRlsdHold : 1;
  DWORD fXoffHold : 1;
  DWORD fXoffSent : 1;
  DWORD fEof : 1;
  DWORD fTxim : 1;
  DWORD fReserved : 25;
  DWORD cbInQue;
  DWORD cbOutQue;
} COMSTAT, *LPCOMSTAT;

typedef struct tagCONVCONTEXT {
  UINT  cb;
  UINT  wFlags;
  UINT  wCountryID;
  int   iCodePage;
  DWORD dwLangID;
  DWORD dwSecurity;
  SECURITY_QUALITY_OF_SERVICE qos;
} CONVCONTEXT;

typedef CONVCONTEXT *PCONVCONTEXT;

typedef struct tagCONVINFO {
  DWORD       cb;
  DWORD       hUser;
  HCONV       hConvPartner;
  HSZ         hszSvcPartner;
  HSZ         hszServiceReq;
  HSZ         hszTopic;
  HSZ         hszItem;
  UINT        wFmt;
  UINT        wType;
  UINT        wStatus;
  UINT        wConvst;
  UINT        wLastError;
  HCONVLIST   hConvList;
  CONVCONTEXT ConvCtxt;
  HWND        hwnd;
  HWND        hwndPartner;
} CONVINFO, *PCONVINFO;

typedef struct tagCOPYDATASTRUCT {
  DWORD dwData;
  DWORD cbData;
  PVOID lpData;
} COPYDATASTRUCT;

typedef struct tagACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA {
    PVOID lpInformation;
    PVOID lpSectionBase;
    ULONG ulSectionLength;
    PVOID lpSectionGlobalDataBase;
    ULONG ulSectionGlobalDataLength;
} ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, *PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA;
typedef const ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA *PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA;

typedef struct tagACTCTX_SECTION_KEYED_DATA {
    ULONG cbSize;
    ULONG ulDataFormatVersion;
    PVOID lpData;
    ULONG ulLength;
    PVOID lpSectionGlobalData;
    ULONG ulSectionGlobalDataLength;
    PVOID lpSectionBase;
    ULONG ulSectionTotalLength;
    HANDLE hActCtx;
    ULONG ulAssemblyRosterIndex;
// 2600 stops here
    ULONG ulFlags;
    ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA AssemblyMetadata;
} ACTCTX_SECTION_KEYED_DATA, *PACTCTX_SECTION_KEYED_DATA;
typedef const ACTCTX_SECTION_KEYED_DATA * PCACTCTX_SECTION_KEYED_DATA;

typedef struct _cpinfo {
  UINT MaxCharSize;
  BYTE DefaultChar[MAX_DEFAULTCHAR];
  BYTE LeadByte[MAX_LEADBYTES];
} CPINFO, *LPCPINFO;

typedef struct tagCPLINFO {
  int  idIcon;
  int  idName;
  int  idInfo;
  LONG lData;
} CPLINFO;

typedef struct _CREATE_PROCESS_DEBUG_INFO {
  HANDLE hFile;
  HANDLE hProcess;
  HANDLE hThread;
  LPVOID lpBaseOfImage;
  DWORD dwDebugInfoFileOffset;
  DWORD nDebugInfoSize;
  LPVOID lpThreadLocalBase;
  LPTHREAD_START_ROUTINE lpStartAddress;
  LPVOID lpImageName;
  WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO;

typedef struct _CREATE_THREAD_DEBUG_INFO {
  HANDLE hThread;
  LPVOID lpThreadLocalBase;
  LPTHREAD_START_ROUTINE lpStartAddress;
} CREATE_THREAD_DEBUG_INFO;

/*
 TODO: sockets
typedef struct _SOCKET_ADDRESS {
  LPSOCKADDR lpSockaddr ;
  INT iSockaddrLength ;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;
*/

/*
typedef struct _CSADDR_INFO {
  SOCKET_ADDRESS  LocalAddr;
  SOCKET_ADDRESS  RemoteAddr;
  INT             iSocketType;
  INT             iProtocol;
} CSADDR_INFO;
*/

typedef struct {
    UINT cbSize;
    HDESK hdesk;
    HWND hwnd;
    LUID luid;
} BSMINFO, *PBSMINFO;

typedef struct _currencyfmtA {
  UINT      NumDigits;
  UINT      LeadingZero;
  UINT      Grouping;
  LPSTR     lpDecimalSep;
  LPSTR     lpThousandSep;
  UINT      NegativeOrder;
  UINT      PositiveOrder;
  LPSTR     lpCurrencySymbol;
} CURRENCYFMTA;

typedef struct _currencyfmtW {
  UINT      NumDigits;
  UINT      LeadingZero;
  UINT      Grouping;
  LPWSTR    lpDecimalSep;
  LPWSTR    lpThousandSep;
  UINT      NegativeOrder;
  UINT      PositiveOrder;
  LPWSTR    lpCurrencySymbol;
} CURRENCYFMTW;

typedef_tident(CURRENCYFMT)

typedef struct _SERVICE_DESCRIPTIONA {
    LPSTR       lpDescription;
} SERVICE_DESCRIPTIONA, *LPSERVICE_DESCRIPTIONA;
typedef struct _SERVICE_DESCRIPTIONW {
    LPWSTR      lpDescription;
} SERVICE_DESCRIPTIONW, *LPSERVICE_DESCRIPTIONW;
typedef_tident(SERVICE_DESCRIPTION)
typedef_tident(LPSERVICE_DESCRIPTION)

typedef enum _SC_ACTION_TYPE {
    SC_ACTION_NONE = 0,
    SC_ACTION_RESTART = 1,
    SC_ACTION_REBOOT = 2,
    SC_ACTION_RUN_COMMAND = 3
} SC_ACTION_TYPE;

typedef struct _SC_ACTION {
    SC_ACTION_TYPE  Type;
    DWORD           Delay;
} SC_ACTION, *LPSC_ACTION;

typedef struct _SERVICE_FAILURE_ACTIONSA {
    DWORD       dwResetPeriod;
    LPSTR       lpRebootMsg;
    LPSTR       lpCommand;
    DWORD       cActions;
    SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONSA, *LPSERVICE_FAILURE_ACTIONSA;

typedef struct _SERVICE_FAILURE_ACTIONSW {
    DWORD       dwResetPeriod;
    LPWSTR      lpRebootMsg;
    LPWSTR      lpCommand;
    DWORD       cActions;
    SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONSW, *LPSERVICE_FAILURE_ACTIONSW;

typedef_tident(SERVICE_FAILURE_ACTIONS)
typedef_tident(LPSERVICE_FAILURE_ACTIONS)

typedef struct tagACTCTXA {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCSTR      lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCSTR      lpAssemblyDirectory;
    LPCSTR      lpResourceName;
    LPCSTR      lpApplicationName;
    HMODULE     hModule;
} ACTCTXA, *PACTCTXA;

typedef struct tagACTCTXW {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCWSTR     lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCWSTR     lpAssemblyDirectory;
    LPCWSTR     lpResourceName;
    LPCWSTR     lpApplicationName;
    HMODULE     hModule;
} ACTCTXW, *PACTCTXW;

typedef struct _JOB_SET_ARRAY {
    HANDLE JobHandle;
    DWORD MemberLevel;
    DWORD Flags;
} JOB_SET_ARRAY, *PJOB_SET_ARRAY;

typedef struct _MEMORYSTATUSEX {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullTotalPageFile;
    DWORDLONG ullAvailPageFile;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

typedef const ACTCTXA *PCACTCTXA;
typedef const ACTCTXW *PCACTCTXW;
typedef_tident(ACTCTX)
typedef_tident(PACTCTX)

typedef struct _TRIVERTEX
{
    LONG    x;
    LONG    y;
    COLOR16 Red;
    COLOR16 Green;
    COLOR16 Blue;
    COLOR16 Alpha;
}TRIVERTEX,*PTRIVERTEX,*LPTRIVERTEX;

typedef struct tagCURSORSHAPE {
  int     xHotSpot;
  int     yHotSpot;
  int     cx;
  int     cy;
  int     cbWidth;
  BYTE    Planes;
  BYTE    BitsPixel;
} CURSORSHAPE,   *LPCURSORSHAPE;

typedef struct tagCWPRETSTRUCT {
  LRESULT lResult;
  LPARAM  lParam;
  WPARAM  wParam;
  DWORD   message;
  HWND    hwnd;
} CWPRETSTRUCT;

typedef struct tagCWPSTRUCT {
  LPARAM  lParam;
  WPARAM  wParam;
  UINT    message;
  HWND    hwnd;
} CWPSTRUCT;

typedef struct _DATATYPES_INFO_1A {
  LPSTR  pName;
} DATATYPES_INFO_1A;

typedef struct _DATATYPES_INFO_1W {
  LPWSTR pName;
} DATATYPES_INFO_1W;

typedef_tident(DATATYPES_INFO_1)

typedef struct {
  unsigned short bAppReturnCode:8,
    reserved:6,
    fBusy:1,
    fAck:1;
} DDEACK;

typedef struct {
  unsigned short reserved:14,
    fDeferUpd:1,
    fAckReq:1;
  short cfFormat;
} DDEADVISE;

typedef struct {
  unsigned short unused:12,
    fResponse:1,
    fRelease:1,
    reserved:1,
    fAckReq:1;
  short cfFormat;
  BYTE  Value[1];
} DDEDATA;

typedef struct {
  unsigned short unused:13,
    fRelease:1,
    fDeferUpd:1,
    fAckReq:1;
  short cfFormat;
} DDELN;

typedef struct tagDDEML_MSG_HOOK_DATA {
  UINT  uiLo;
  UINT  uiHi;
  DWORD cbData;
  DWORD Data[8];
} DDEML_MSG_HOOK_DATA;

typedef struct {
  unsigned short unused:13,
    fRelease:1,
    fReserved:2;
  short cfFormat;
  BYTE  Value[1];
} DDEPOKE;

typedef struct {
  unsigned short unused:12,
    fAck:1,
    fRelease:1,
    fReserved:1,
    fAckReq:1;
  short cfFormat;
  BYTE rgb[1];
} DDEUP;

typedef struct _EXCEPTION_DEBUG_INFO {
  EXCEPTION_RECORD ExceptionRecord;
  DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO;

typedef struct _EXIT_PROCESS_DEBUG_INFO {
  DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO;

typedef struct _EXIT_THREAD_DEBUG_INFO {
  DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO;

typedef struct _LOAD_DLL_DEBUG_INFO {
  HANDLE hFile;
  LPVOID lpBaseOfDll;
  DWORD  dwDebugInfoFileOffset;
  DWORD  nDebugInfoSize;
  LPVOID lpImageName;
  WORD fUnicode;
} LOAD_DLL_DEBUG_INFO;

typedef struct _UNLOAD_DLL_DEBUG_INFO {
  LPVOID lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO;

typedef struct _OUTPUT_DEBUG_STRING_INFO {
  LPSTR lpDebugStringData;
  WORD  fUnicode;
  WORD  nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO;

typedef struct _RIP_INFO {
  DWORD  dwError;
  DWORD  dwType;
} RIP_INFO;

typedef struct _DEBUG_EVENT {
  DWORD dwDebugEventCode;
  DWORD dwProcessId;
  DWORD dwThreadId;
  union {
    EXCEPTION_DEBUG_INFO Exception;
    CREATE_THREAD_DEBUG_INFO CreateThread;
    CREATE_PROCESS_DEBUG_INFO CreateProcessInfo;
    EXIT_THREAD_DEBUG_INFO ExitThread;
    EXIT_PROCESS_DEBUG_INFO ExitProcess;
    LOAD_DLL_DEBUG_INFO LoadDll;
    UNLOAD_DLL_DEBUG_INFO UnloadDll;
    OUTPUT_DEBUG_STRING_INFO DebugString;
    RIP_INFO RipInfo;
  } u;
} DEBUG_EVENT, *LPDEBUG_EVENT;

typedef struct tagDEBUGHOOKINFO {
  DWORD  idThread;
  DWORD  idThreadInstaller;
  LPARAM lParam;
  WPARAM wParam;
  int    code;
} DEBUGHOOKINFO;

typedef struct tagDELETEITEMSTRUCT {
  UINT CtlType;
  UINT CtlID;
  UINT itemID;
  HWND hwndItem;
  UINT itemData;
} DELETEITEMSTRUCT;

typedef struct _DEV_BROADCAST_HDR {
  ULONG dbch_size;
  ULONG dbch_devicetype;
  ULONG dbch_reserved;
} DEV_BROADCAST_HDR;
typedef DEV_BROADCAST_HDR *PDEV_BROADCAST_HDR;

typedef struct _DEV_BROADCAST_OEM {
  ULONG dbco_size;
  ULONG dbco_devicetype;
  ULONG dbco_reserved;
  ULONG dbco_identifier;
  ULONG dbco_suppfunc;
} DEV_BROADCAST_OEM;
typedef DEV_BROADCAST_OEM *PDEV_BROADCAST_OEM;

typedef struct _DEV_BROADCAST_PORT {
  ULONG dbcp_size;
  ULONG dbcp_devicetype;
  ULONG dbcp_reserved;
  char dbcp_name[1];
} DEV_BROADCAST_PORT;
typedef DEV_BROADCAST_PORT *PDEV_BROADCAST_PORT;

struct _DEV_BROADCAST_USERDEFINED {
  struct _DEV_BROADCAST_HDR dbud_dbh;
  char  dbud_szName[1];
  BYTE  dbud_rgbUserDefined[1];
};

typedef struct _DEV_BROADCAST_VOLUME {
  ULONG dbcv_size;
  ULONG dbcv_devicetype;
  ULONG dbcv_reserved;
  ULONG dbcv_unitmask;
  USHORT dbcv_flags;
} DEV_BROADCAST_VOLUME;
typedef DEV_BROADCAST_VOLUME *PDEV_BROADCAST_VOLUME;

typedef struct tagDEVNAMES {
  WORD wDriverOffset;
  WORD wDeviceOffset;
  WORD wOutputOffset;
  WORD wDefault;
} DEVNAMES, *LPDEVNAMES;

typedef struct tagDIBSECTION {
  BITMAP              dsBm;
  BITMAPINFOHEADER    dsBmih;
  DWORD               dsBitfields[3];
  HANDLE              dshSection;
  DWORD               dsOffset;
} DIBSECTION;

typedef struct _DISK_PERFORMANCE {
  LARGE_INTEGER BytesRead;
  LARGE_INTEGER BytesWritten;
  LARGE_INTEGER ReadTime;
  LARGE_INTEGER WriteTime;
  DWORD ReadCount;
  DWORD WriteCount;
  DWORD QueueDepth;
} DISK_PERFORMANCE ;


#include <pshpack1.h>

typedef struct {
  DWORD style;
  DWORD dwExtendedStyle;
  short x;
  short y;
  short cx;
  short cy;
  WORD  id;
} DLGITEMTEMPLATE;

typedef DLGITEMTEMPLATE *LPDLGITEMTEMPLATE;
typedef DLGITEMTEMPLATE *PDLGITEMTEMPLATE;

typedef struct {
  DWORD style;
  DWORD dwExtendedStyle;
  WORD  cdit;
  short x;
  short y;
  short cx;
  short cy;
} DLGTEMPLATE;

#include <poppack.h>


typedef DLGTEMPLATE *LPDLGTEMPLATE;
typedef const DLGTEMPLATE *LPCDLGTEMPLATE;

typedef struct _DOC_INFO_1A {
  LPSTR  pDocName;
  LPSTR  pOutputFile;
  LPSTR  pDatatype;
} DOC_INFO_1A;

typedef struct _DOC_INFO_1W {
  LPWSTR pDocName;
  LPWSTR pOutputFile;
  LPWSTR pDatatype;
} DOC_INFO_1W;

typedef_tident(DOC_INFO_1)

typedef struct _DOC_INFO_2A {
  LPSTR  pDocName;
  LPSTR  pOutputFile;
  LPSTR  pDatatype;
  DWORD  dwMode;
  DWORD  JobId;
} DOC_INFO_2A;

typedef struct _DOC_INFO_2W {
  LPWSTR pDocName;
  LPWSTR pOutputFile;
  LPWSTR pDatatype;
  DWORD  dwMode;
  DWORD  JobId;
} DOC_INFO_2W;

typedef_tident(DOC_INFO_2)

typedef struct {
  int     cbSize;
  LPCSTR  lpszDocName;
  LPCSTR  lpszOutput;
  LPCSTR  lpszDatatype;
  DWORD   fwType;
} DOCINFOA, *PDOCINFOA;

typedef struct {
  int     cbSize;
  LPCWSTR lpszDocName;
  LPCWSTR lpszOutput;
  LPCWSTR lpszDatatype;
  DWORD   fwType;
} DOCINFOW, *PDOCINFOW;

typedef_tident(DOCINFO)
typedef_tident(PDOCINFO)

typedef struct {
  UINT uNotification;
  HWND hWnd;
  POINT ptCursor;
} DRAGLISTINFO, *LPDRAGLISTINFO;

typedef struct tagDRAWITEMSTRUCT {
  UINT  CtlType;
  UINT  CtlID;
  UINT  itemID;
  UINT  itemAction;
  UINT  itemState;
  HWND  hwndItem;
  HDC   hDC;
  RECT  rcItem;
  DWORD itemData;
} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT, *PDRAWITEMSTRUCT;

typedef struct {
  UINT cbSize;
  int  iTabLength;
  int  iLeftMargin;
  int  iRightMargin;
  UINT uiLengthDrawn;
} DRAWTEXTPARAMS, *LPDRAWTEXTPARAMS;



typedef struct _EXTTEXTMETRIC
    {
    short   emSize;
    short   emPointSize;
    short   emOrientation;
    short   emMasterHeight;
    short   emMinScale;
    short   emMaxScale;
    short   emMasterUnits;
    short   emCapHeight;
    short   emXHeight;
    short   emLowerCaseAscent;
    short   emLowerCaseDescent;
    short   emSlant;
    short   emSuperScript;
    short   emSubScript;
    short   emSuperScriptSize;
    short   emSubScriptSize;
    short   emUnderlineOffset;
    short   emUnderlineWidth;
    short   emDoubleUpperUnderlineOffset;
    short   emDoubleLowerUnderlineOffset;
    short   emDoubleUpperUnderlineWidth;
    short   emDoubleLowerUnderlineWidth;
    short   emStrikeOutOffset;
    short   emStrikeOutWidth;
    WORD    emKernPairs;
    WORD    emKernTracks;
} EXTTEXTMETRIC, *PEXTTEXTMETRIC;

typedef struct _DRIVER_INFO_1A {
  LPSTR  pName;
} DRIVER_INFO_1A;

typedef struct _DRIVER_INFO_1W {
  LPWSTR pName;
} DRIVER_INFO_1W;

typedef_tident(DRIVER_INFO_1)

typedef struct _DRIVER_INFO_2A {
  DWORD  cVersion;
  LPSTR  pName;
  LPSTR  pEnvironment;
  LPSTR  pDriverPath;
  LPSTR  pDataFile;
  LPSTR  pConfigFile;
} DRIVER_INFO_2A;

typedef struct _DRIVER_INFO_2W {
  DWORD  cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
} DRIVER_INFO_2W;

typedef_tident(DRIVER_INFO_2)

typedef struct _DRIVER_INFO_3A {
  DWORD  cVersion;
  LPSTR  pName;
  LPSTR  pEnvironment;
  LPSTR  pDriverPath;
  LPSTR  pDataFile;
  LPSTR  pConfigFile;
  LPSTR  pHelpFile;
  LPSTR  pDependentFiles;
  LPSTR  pMonitorName;
  LPSTR  pDefaultDataType;
} DRIVER_INFO_3A;

typedef struct _DRIVER_INFO_3W {
  DWORD  cVersion;
  LPWSTR pName;
  LPWSTR pEnvironment;
  LPWSTR pDriverPath;
  LPWSTR pDataFile;
  LPWSTR pConfigFile;
  LPWSTR pHelpFile;
  LPWSTR pDependentFiles;
  LPWSTR pMonitorName;
  LPWSTR pDefaultDataType;
} DRIVER_INFO_3W;

typedef_tident(DRIVER_INFO_3)

typedef struct _editstream {
  DWORD dwCookie;
  DWORD dwError;
  EDITSTREAMCALLBACK pfnCallback;
} EDITSTREAM;

typedef struct tagEMR
{
  DWORD iType;
  DWORD nSize;
} EMR, *PEMR;

typedef struct tagEMRANGLEARC
{
  EMR     emr;
  POINTL  ptlCenter;
  DWORD   nRadius;
  FLOAT   eStartAngle;
  FLOAT   eSweepAngle;
} EMRANGLEARC, *PEMRANGLEARC;

typedef struct tagEMRARC
{
  EMR    emr;
  RECTL  rclBox;
  POINTL ptlStart;
  POINTL ptlEnd;
} EMRARC,   *PEMRARC,
    EMRARCTO, *PEMRARCTO,
    EMRCHORD, *PEMRCHORD,
    EMRPIE,   *PEMRPIE;

typedef struct  _XFORM
{
  FLOAT   eM11;
  FLOAT   eM12;
  FLOAT   eM21;
  FLOAT   eM22;
  FLOAT   eDx;
  FLOAT   eDy;
} XFORM, *PXFORM, *LPXFORM;

typedef struct tagEMRBITBLT
{
  EMR      emr;
  RECTL    rclBounds;
  LONG     xDest;
  LONG     yDest;
  LONG     cxDest;
  LONG     cyDest;
  DWORD    dwRop;
  LONG     xSrc;
  LONG     ySrc;
  XFORM    xformSrc;
  COLORREF crBkColorSrc;
  DWORD    iUsageSrc;
  DWORD    offBmiSrc;
  DWORD    offBitsSrc;
  DWORD    cbBitsSrc;
} EMRBITBLT, *PEMRBITBLT;

typedef struct tagLOGBRUSH {
  UINT     lbStyle;
  COLORREF lbColor;
  LONG     lbHatch;
} LOGBRUSH, *PLOGBRUSH;

typedef struct tagEMRCREATEBRUSHINDIRECT
{
  EMR      emr;
  DWORD    ihBrush;
  LOGBRUSH lb;
} EMRCREATEBRUSHINDIRECT, *PEMRCREATEBRUSHINDIRECT;

typedef LONG LCSCSTYPE;
typedef LONG LCSGAMUTMATCH;

typedef struct tagLOGCOLORSPACEA {
  DWORD         lcsSignature;
  DWORD         lcsVersion;
  DWORD         lcsSize;

  LCSCSTYPE     lcsCSType;
  LCSGAMUTMATCH lcsIntent;
  CIEXYZTRIPLE  lcsEndpoints;
  DWORD         lcsGammaRed;
  DWORD         lcsGammaGreen;
  DWORD         lcsGammaBlue;
  CHAR          lcsFilename[MAX_PATH];
} LOGCOLORSPACEA, *LPLOGCOLORSPACEA;

typedef struct tagLOGCOLORSPACEW {
  DWORD         lcsSignature;
  DWORD         lcsVersion;
  DWORD         lcsSize;

  LCSCSTYPE     lcsCSType;
  LCSGAMUTMATCH lcsIntent;
  CIEXYZTRIPLE  lcsEndpoints;
  DWORD         lcsGammaRed;
  DWORD         lcsGammaGreen;
  DWORD         lcsGammaBlue;
  WCHAR         lcsFilename[MAX_PATH];
} LOGCOLORSPACEW, *LPLOGCOLORSPACEW;

typedef_tident(LOGCOLORSPACE)

typedef struct tagEMRCREATECOLORSPACEA
{
  EMR            emr;
  DWORD          ihCS;
  LOGCOLORSPACEA lcs;
} EMRCREATECOLORSPACEA, *PEMRCREATECOLORSPACEA;

typedef struct tagEMRCREATECOLORSPACEW
{
  EMR            emr;
  DWORD          ihCS;
  LOGCOLORSPACEW lcs;
} EMRCREATECOLORSPACEW, *PEMRCREATECOLORSPACEW;

typedef_tident(EMRCREATECOLORSPACE)
typedef_tident(PEMRCREATECOLORSPACE)

typedef struct tagEMRCREATEDIBPATTERNBRUSHPT
{
  EMR   emr;
  DWORD ihBrush;
  DWORD iUsage;
  DWORD offBmi;
  DWORD cbBmi;
  DWORD offBits;
  DWORD cbBits;
} EMRCREATEDIBPATTERNBRUSHPT,
    PEMRCREATEDIBPATTERNBRUSHPT;

typedef struct tagEMRCREATEMONOBRUSH
{
  EMR   emr;
  DWORD ihBrush;
  DWORD iUsage;
  DWORD offBmi;
  DWORD cbBmi;
  DWORD offBits;
  DWORD cbBits;
} EMRCREATEMONOBRUSH, *PEMRCREATEMONOBRUSH;

typedef struct tagPALETTEENTRY {
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY, *PPALETTEENTRY;

typedef struct tagLOGPALETTE {
  WORD         palVersion;
  WORD         palNumEntries;
  PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE, *PLOGPALETTE;

typedef struct tagEMRCREATEPALETTE
{
  EMR        emr;
  DWORD      ihPal;
  LOGPALETTE lgpl;
} EMRCREATEPALETTE, *PEMRCREATEPALETTE;

typedef struct tagLOGPEN {
  UINT     lopnStyle;
  POINT    lopnWidth;
  COLORREF lopnColor;
} LOGPEN, *PLOGPEN;

typedef struct tagEMRCREATEPEN
{
  EMR    emr;
  DWORD  ihPen;
  LOGPEN lopn;
} EMRCREATEPEN, *PEMRCREATEPEN;

typedef struct tagEMRELLIPSE
{
  EMR   emr;
  RECTL rclBox;
} EMRELLIPSE,  *PEMRELLIPSE,
    EMRRECTANGLE, *PEMRRECTANGLE;

typedef struct tagEMREOF
{
  EMR     emr;
  DWORD   nPalEntries;
  DWORD   offPalEntries;
  DWORD   nSizeLast;
} EMREOF, *PEMREOF;

typedef struct tagEMREXCLUDECLIPRECT
{
  EMR   emr;
  RECTL rclClip;
} EMREXCLUDECLIPRECT,   *PEMREXCLUDECLIPRECT,
    EMRINTERSECTCLIPRECT, *PEMRINTERSECTCLIPRECT;

typedef struct tagPANOSE {
  BYTE bFamilyType;
  BYTE bSerifStyle;
  BYTE bWeight;
  BYTE bProportion;
  BYTE bContrast;
  BYTE bStrokeVariation;
  BYTE bArmStyle;
  BYTE bLetterform;
  BYTE bMidline;
  BYTE bXHeight;
} PANOSE;

typedef struct tagEXTLOGFONTA {
    LOGFONTA elfLogFont;
    CHAR     elfFullName[LF_FULLFACESIZE];
    CHAR     elfStyle[LF_FACESIZE];
    DWORD    elfVersion;
    DWORD    elfStyleSize;
    DWORD    elfMatch;
    DWORD    elfReserved;
    BYTE     elfVendorId[ELF_VENDOR_SIZE];
    DWORD    elfCulture;
    PANOSE   elfPanose;
} EXTLOGFONTA, *LPEXTLOGFONTA;

typedef struct tagEXTLOGFONTW {
    LOGFONTW elfLogFont;
    WCHAR    elfFullName[LF_FULLFACESIZE];
    WCHAR    elfStyle[LF_FACESIZE];
    DWORD    elfVersion;
    DWORD    elfStyleSize;
    DWORD    elfMatch;
    DWORD    elfReserved;
    BYTE     elfVendorId[ELF_VENDOR_SIZE];
    DWORD    elfCulture;
    PANOSE   elfPanose;
} EXTLOGFONTW, *LPEXTLOGFONTW;

typedef_tident(EXTLOGFONT)
typedef_tident(LPEXTLOGFONT)

typedef struct tagEMREXTCREATEFONTINDIRECTW
{
  EMR         emr;
  DWORD       ihFont;
  EXTLOGFONTW elfw;
} EMREXTCREATEFONTINDIRECTW,
    PEMREXTCREATEFONTINDIRECTW;

typedef struct tagEXTLOGPEN {
  UINT     elpPenStyle;
  UINT     elpWidth;
  UINT     elpBrushStyle;
  COLORREF elpColor;
  LONG     elpHatch;
  DWORD    elpNumEntries;
  DWORD    elpStyleEntry[1];
} EXTLOGPEN;

typedef struct tagEMREXTCREATEPEN
{
  EMR       emr;
  DWORD     ihPen;
  DWORD     offBmi;
  DWORD     cbBmi;
  DWORD     offBits;
  DWORD     cbBits;
  EXTLOGPEN elp;
} EMREXTCREATEPEN, *PEMREXTCREATEPEN;

typedef struct tagEMREXTFLOODFILL
{
  EMR     emr;
  POINTL  ptlStart;
  COLORREF crColor;
  DWORD   iMode;
} EMREXTFLOODFILL, *PEMREXTFLOODFILL;

typedef struct tagEMREXTSELECTCLIPRGN
{
  EMR   emr;
  DWORD cbRgnData;
  DWORD iMode;
  BYTE  RgnData[1];
} EMREXTSELECTCLIPRGN, *PEMREXTSELECTCLIPRGN;

typedef struct tagEMRTEXT
{
  POINTL ptlReference;
  DWORD  nChars;
  DWORD  offString;
  DWORD  fOptions;
  RECTL  rcl;
  DWORD  offDx;
} EMRTEXT, *PEMRTEXT;

typedef struct tagEMREXTTEXTOUTA
{
  EMR     emr;
  RECTL   rclBounds;
  DWORD   iGraphicsMode;
  FLOAT   exScale;
  FLOAT   eyScale;
  EMRTEXT emrtext;
} EMREXTTEXTOUTA, *PEMREXTTEXTOUTA,
    EMREXTTEXTOUTW, *PEMREXTTEXTOUTW;

typedef struct tagEMRFILLPATH
{
  EMR   emr;
  RECTL rclBounds;
} EMRFILLPATH,          *PEMRFILLPATH,
    EMRSTROKEANDFILLPATH, *PEMRSTROKEANDFILLPATH,
    EMRSTROKEPATH,        *PEMRSTROKEPATH;

typedef struct tagEMRFILLRGN
{
  EMR   emr;
  RECTL rclBounds;
  DWORD cbRgnData;
  DWORD ihBrush;
  BYTE  RgnData[1];
} EMRFILLRGN, *PEMRFILLRGN;

typedef struct tagEMRFORMAT {
  DWORD   dSignature;
  DWORD   nVersion;
  DWORD   cbData;
  DWORD   offData;
} EMRFORMAT;

typedef struct tagSIZE {
  LONG cx;
  LONG cy;
} SIZE, *PSIZE, *LPSIZE, SIZEL, *PSIZEL, *LPSIZEL;

typedef struct tagEMRFRAMERGN
{
  EMR   emr;
  RECTL rclBounds;
  DWORD cbRgnData;
  DWORD ihBrush;
  SIZEL szlStroke;
  BYTE  RgnData[1];
} EMRFRAMERGN, *PEMRFRAMERGN;

typedef struct tagEMRGDICOMMENT
{
  EMR   emr;
  DWORD cbData;
  BYTE  Data[1];
} EMRGDICOMMENT, *PEMRGDICOMMENT;

typedef struct tagEMRINVERTRGN
{
  EMR   emr;
  RECTL rclBounds;
  DWORD cbRgnData;
  BYTE  RgnData[1];
} EMRINVERTRGN, *PEMRINVERTRGN,
    EMRPAINTRGN,  *PEMRPAINTRGN;

typedef struct tagEMRLINETO
{
  EMR    emr;
  POINTL ptl;
} EMRLINETO,   *PEMRLINETO,
    EMRMOVETOEX, *PEMRMOVETOEX;

typedef struct tagEMRMASKBLT
{
  EMR     emr;
  RECTL   rclBounds;
  LONG    xDest;
  LONG    yDest;
  LONG    cxDest;
  LONG    cyDest;
  DWORD   dwRop;
  LONG    xSrc;
  LONG    ySrc;
  XFORM   xformSrc;
  COLORREF crBkColorSrc;
  DWORD   iUsageSrc;
  DWORD   offBmiSrc;
  DWORD   cbBmiSrc;
  DWORD   offBitsSrc;
  DWORD   cbBitsSrc;
  LONG    xMask;
  LONG    yMask;
  DWORD   iUsageMask;
  DWORD   offBmiMask;
  DWORD   cbBmiMask;
  DWORD   offBitsMask;
  DWORD   cbBitsMask;
} EMRMASKBLT, *PEMRMASKBLT;

typedef struct tagEMRMODIFYWORLDTRANSFORM
{
  EMR   emr;
  XFORM xform;
  DWORD iMode;
} EMRMODIFYWORLDTRANSFORM,
    PEMRMODIFYWORLDTRANSFORM;

typedef struct tagEMROFFSETCLIPRGN
{
  EMR    emr;
  POINTL ptlOffset;
} EMROFFSETCLIPRGN, *PEMROFFSETCLIPRGN;

typedef struct tagEMRPLGBLT
{
  EMR      emr;
  RECTL    rclBounds;
  POINTL   aptlDest[3];
  LONG    xSrc;
  LONG    ySrc;
  LONG     cxSrc;
  LONG     cySrc;
  XFORM   xformSrc;
  COLORREF crBkColorSrc;
  DWORD    iUsageSrc;
  DWORD    offBmiSrc;
  DWORD   cbBmiSrc;
  DWORD   offBitsSrc;
  DWORD   cbBitsSrc;
  LONG    xMask;
  LONG    yMask;
  DWORD   iUsageMask;
  DWORD   offBmiMask;
  DWORD   cbBmiMask;
  DWORD   offBitsMask;
  DWORD   cbBitsMask;
} EMRPLGBLT, *PEMRPLGBLT;

typedef struct tagEMRPOLYDRAW
{
  EMR    emr;
  RECTL  rclBounds;
  DWORD  cptl;
  POINTL aptl[1];
  BYTE   abTypes[1];
} EMRPOLYDRAW, *PEMRPOLYDRAW;

typedef struct tagEMRPOLYDRAW16
{
  EMR    emr;
  RECTL  rclBounds;
  DWORD  cpts;
  POINTS apts[1];
  BYTE   abTypes[1];
} EMRPOLYDRAW16, *PEMRPOLYDRAW16;

typedef struct tagEMRPOLYLINE
{
  EMR    emr;
  RECTL  rclBounds;
  DWORD  cptl;
  POINTL aptl[1];
} EMRPOLYLINE,     *PEMRPOLYLINE,
    EMRPOLYBEZIER,   *PEMRPOLYBEZIER,
    EMRPOLYGON,      *PEMRPOLYGON,
    EMRPOLYBEZIERTO, *PEMRPOLYBEZIERTO,
    EMRPOLYLINETO,   *PEMRPOLYLINETO;

typedef struct tagEMRPOLYLINE16
{
  EMR    emr;
  RECTL  rclBounds;
  DWORD  cpts;
  POINTL apts[1];
} EMRPOLYLINE16,     *PEMRPOLYLINE16,
    EMRPOLYBEZIER16,   *PEMRPOLYBEZIER16,
    EMRPOLYGON16,      *PEMRPOLYGON16,
    EMRPOLYBEZIERTO16, *PEMRPOLYBEZIERTO16,
    EMRPOLYLINETO16,   *PEMRPOLYLINETO16;

typedef struct tagEMRPOLYPOLYLINE
{
  EMR     emr;
  RECTL   rclBounds;
  DWORD   nPolys;
  DWORD   cptl;
  DWORD   aPolyCounts[1];
  POINTL  aptl[1];
} EMRPOLYPOLYLINE, *PEMRPOLYPOLYLINE,
    EMRPOLYPOLYGON,  *PEMRPOLYPOLYGON;

typedef struct tagEMRPOLYPOLYLINE16
{
  EMR     emr;
  RECTL   rclBounds;
  DWORD   nPolys;
  DWORD   cpts;
  DWORD   aPolyCounts[1];
  POINTS  apts[1];
} EMRPOLYPOLYLINE16, *PEMRPOLYPOLYLINE16,
    EMRPOLYPOLYGON16,  *PEMRPOLYPOLYGON16;

typedef struct tagEMRPOLYTEXTOUTA
{
  EMR     emr;
  RECTL   rclBounds;
  DWORD   iGraphicsMode;
  FLOAT   exScale;
  FLOAT   eyScale;
  LONG    cStrings;
  EMRTEXT aemrtext[1];
} EMRPOLYTEXTOUTA, *PEMRPOLYTEXTOUTA,
    EMRPOLYTEXTOUTW, *PEMRPOLYTEXTOUTW;

typedef struct tagEMRRESIZEPALETTE
{
  EMR   emr;
  DWORD ihPal;
  DWORD cEntries;
} EMRRESIZEPALETTE, *PEMRRESIZEPALETTE;

typedef struct tagEMRRESTOREDC
{
  EMR  emr;
  LONG iRelative;
} EMRRESTOREDC, *PEMRRESTOREDC;

typedef struct tagEMRROUNDRECT
{
  EMR   emr;
  RECTL rclBox;
  SIZEL szlCorner;
} EMRROUNDRECT, *PEMRROUNDRECT;

typedef struct tagEMRSCALEVIEWPORTEXTEX
{
  EMR  emr;
  LONG xNum;
  LONG xDenom;
  LONG yNum;
  LONG yDenom;
} EMRSCALEVIEWPORTEXTEX, *PEMRSCALEVIEWPORTEXTEX,
    EMRSCALEWINDOWEXTEX,   *PEMRSCALEWINDOWEXTEX;

typedef struct tagEMRSELECTCOLORSPACE
{
  EMR     emr;
  DWORD   ihCS;
} EMRSELECTCOLORSPACE, *PEMRSELECTCOLORSPACE,
    EMRDELETECOLORSPACE, *PEMRDELETECOLORSPACE;
typedef struct tagEMRSELECTOBJECT
{
  EMR   emr;
  DWORD ihObject;
} EMRSELECTOBJECT, *PEMRSELECTOBJECT,
    EMRDELETEOBJECT, *PEMRDELETEOBJECT;

typedef struct tagEMRSELECTPALETTE
{
  EMR   emr;
  DWORD ihPal;
} EMRSELECTPALETTE, *PEMRSELECTPALETTE;

typedef struct tagEMRSETARCDIRECTION
{
  EMR   emr;
  DWORD iArcDirection;
} EMRSETARCDIRECTION, *PEMRSETARCDIRECTION;

typedef struct tagEMRSETTEXTCOLOR
{
  EMR      emr;
  COLORREF crColor;
} EMRSETBKCOLOR,   *PEMRSETBKCOLOR,
    EMRSETTEXTCOLOR, *PEMRSETTEXTCOLOR;

typedef struct tagEMRSETCOLORADJUSTMENT
{
  EMR  emr;
  COLORADJUSTMENT ColorAdjustment;
} EMRSETCOLORADJUSTMENT, *PEMRSETCOLORADJUSTMENT;

typedef struct tagEMRSETDIBITSTODEVICE
{
  EMR   emr;
  RECTL rclBounds;
  LONG  xDest;
  LONG  yDest;
  LONG  xSrc;
  LONG  ySrc;
  LONG  cxSrc;
  LONG  cySrc;
  DWORD offBmiSrc;
  DWORD cbBmiSrc;
  DWORD offBitsSrc;
  DWORD cbBitsSrc;
  DWORD iUsageSrc;
  DWORD iStartScan;
  DWORD cScans;
} EMRSETDIBITSTODEVICE, *PEMRSETDIBITSTODEVICE;

typedef struct tagEMRSETMAPPERFLAGS
{
  EMR   emr;
  DWORD dwFlags;
} EMRSETMAPPERFLAGS, *PEMRSETMAPPERFLAGS;

typedef struct tagEMRSETMITERLIMIT
{
  EMR   emr;
  FLOAT eMiterLimit;
} EMRSETMITERLIMIT, *PEMRSETMITERLIMIT;

typedef struct tagEMRSETPALETTEENTRIES
{
  EMR          emr;
  DWORD        ihPal;
  DWORD        iStart;
  DWORD        cEntries;
  PALETTEENTRY aPalEntries[1];
} EMRSETPALETTEENTRIES, *PEMRSETPALETTEENTRIES;

typedef struct tagEMRSETPIXELV
{
  EMR     emr;
  POINTL  ptlPixel;
  COLORREF crColor;
} EMRSETPIXELV, *PEMRSETPIXELV;

typedef struct tagEMRSETVIEWPORTEXTEX
{
  EMR   emr;
  SIZEL szlExtent;
} EMRSETVIEWPORTEXTEX, *PEMRSETVIEWPORTEXTEX,
    EMRSETWINDOWEXTEX,   *PEMRSETWINDOWEXTEX;

typedef struct tagEMRSETVIEWPORTORGEX
{
  EMR    emr;
  POINTL ptlOrigin;
} EMRSETVIEWPORTORGEX, *PEMRSETVIEWPORTORGEX,
    EMRSETWINDOWORGEX,   *PEMRSETWINDOWORGEX,
    EMRSETBRUSHORGEX,    *PEMRSETBRUSHORGEX;

typedef struct tagEMRSETWORLDTRANSFORM
{
  EMR   emr;
  XFORM xform;
} EMRSETWORLDTRANSFORM, *PEMRSETWORLDTRANSFORM;

typedef struct tagEMRSTRETCHBLT
{
  EMR      emr;
  RECTL    rclBounds;
  LONG     xDest;
  LONG     yDest;
  LONG     cxDest;
  LONG     cyDest;
  DWORD    dwRop;
  LONG     xSrc;
  LONG     ySrc;
  XFORM    xformSrc;
  COLORREF crBkColorSrc;
  DWORD    iUsageSrc;
  DWORD    offBmiSrc;
  DWORD    cbBmiSrc;
  DWORD    offBitsSrc;
  DWORD    cbBitsSrc;
  LONG     cxSrc;
  LONG     cySrc;
} EMRSTRETCHBLT, *PEMRSTRETCHBLT;

typedef struct tagEMRSTRETCHDIBITS
{
  EMR   emr;
  RECTL rclBounds;
  LONG  xDest;
  LONG  yDest;
  LONG  xSrc;
  LONG  ySrc;
  LONG  cxSrc;
  LONG  cySrc;
  DWORD offBmiSrc;
  DWORD cbBmiSrc;
  DWORD offBitsSrc;
  DWORD cbBitsSrc;
  DWORD iUsageSrc;
  DWORD dwRop;
  LONG  cxDest;
  LONG  cyDest;
} EMRSTRETCHDIBITS, *PEMRSTRETCHDIBITS;

typedef struct tagABORTPATH
{
  EMR emr;
} EMRABORTPATH,      *PEMRABORTPATH,
    EMRBEGINPATH,      *PEMRBEGINPATH,
    EMRENDPATH,        *PEMRENDPATH,
    EMRCLOSEFIGURE,    *PEMRCLOSEFIGURE,
    EMRFLATTENPATH,    *PEMRFLATTENPATH,
    EMRWIDENPATH,      *PEMRWIDENPATH,
    EMRSETMETARGN,     *PEMRSETMETARGN,
    EMRSAVEDC,         *PEMRSAVEDC,
    EMRREALIZEPALETTE, *PEMRREALIZEPALETTE;

typedef struct tagEMRSELECTCLIPPATH
{
  EMR   emr;
  DWORD iMode;
} EMRSELECTCLIPPATH,    *PEMRSELECTCLIPPATH,
    EMRSETBKMODE,         *PEMRSETBKMODE,
    EMRSETMAPMODE,        *PEMRSETMAPMODE,
    EMRSETPOLYFILLMODE,   *PEMRSETPOLYFILLMODE,
    EMRSETROP2,           *PEMRSETROP2,
    EMRSETSTRETCHBLTMODE, *PEMRSETSTRETCHBLTMODE,
    EMRSETTEXTALIGN,      *PEMRSETTEXTALIGN,
    EMRENABLEICM,       *PEMRENABLEICM;

typedef struct tagNMHDR {
  HWND hwndFrom;
  UINT idFrom;
  UINT code;
} NMHDR;

typedef struct _encorrecttext {
  NMHDR nmhdr;
  CHARRANGE chrg;
  WORD seltyp;
} ENCORRECTTEXT;

typedef struct _endropfiles {
  NMHDR nmhdr;
  HANDLE hDrop;
  LONG cp;
  WINBOOL fProtected;
} ENDROPFILES;

typedef struct {
  NMHDR nmhdr;
  LONG cObjectCount;
  LONG cch;
} ENSAVECLIPBOARD;

typedef struct {
  NMHDR nmhdr;
  LONG iob;
  LONG lOper;
  HRESULT hr;
} ENOLEOPFAILED;

typedef struct tagENHMETAHEADER {
  DWORD iType;
  DWORD nSize;
  RECTL rclBounds;
  RECTL rclFrame;
  DWORD dSignature;
  DWORD nVersion;
  DWORD nBytes;
  DWORD nRecords;
  WORD  nHandles;
  WORD  sReserved;
  DWORD nDescription;
  DWORD offDescription;
  DWORD nPalEntries;
  SIZEL szlDevice;
  SIZEL szlMillimeters;
  DWORD cbPixelFormat;
  DWORD offPixelFormat;
  DWORD bOpenGL;
  SIZEL szlMicrometers;
} ENHMETAHEADER, *LPENHMETAHEADER;

typedef struct tagENHMETARECORD {
  DWORD iType;
  DWORD nSize;
  DWORD dParm[1];
} ENHMETARECORD, *PENHMETARECORD, *LPENHMETARECORD;

typedef struct _enprotected {
  NMHDR nmhdr;
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;
  CHARRANGE chrg;
} ENPROTECTED;

typedef struct _SERVICE_STATUS {
  DWORD dwServiceType;
  DWORD dwCurrentState;
  DWORD dwControlsAccepted;
  DWORD dwWin32ExitCode;
  DWORD dwServiceSpecificExitCode;
  DWORD dwCheckPoint;
  DWORD dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

typedef struct _SERVICE_STATUS_PROCESS {
    DWORD   dwServiceType;
    DWORD   dwCurrentState;
    DWORD   dwControlsAccepted;
    DWORD   dwWin32ExitCode;
    DWORD   dwServiceSpecificExitCode;
    DWORD   dwCheckPoint;
    DWORD   dwWaitHint;
    DWORD   dwProcessId;
    DWORD   dwServiceFlags;
} SERVICE_STATUS_PROCESS, *LPSERVICE_STATUS_PROCESS;

typedef struct _ENUM_SERVICE_STATUSA {
  LPSTR lpServiceName;
  LPSTR lpDisplayName;
  SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSA, *LPENUM_SERVICE_STATUSA;

typedef struct _ENUM_SERVICE_STATUSW {
  LPWSTR lpServiceName;
  LPWSTR lpDisplayName;
  SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSW, *LPENUM_SERVICE_STATUSW;

typedef_tident(ENUM_SERVICE_STATUS)
typedef_tident(LPENUM_SERVICE_STATUS)

typedef struct _ENUM_SERVICE_STATUS_PROCESSA {
    LPSTR                     lpServiceName;
    LPSTR                     lpDisplayName;
    SERVICE_STATUS_PROCESS    ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESSA, *LPENUM_SERVICE_STATUS_PROCESSA;

typedef struct _ENUM_SERVICE_STATUS_PROCESSW {
    LPWSTR                    lpServiceName;
    LPWSTR                    lpDisplayName;
    SERVICE_STATUS_PROCESS    ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESSW, *LPENUM_SERVICE_STATUS_PROCESSW;

typedef_tident(ENUM_SERVICE_STATUS_PROCESS)
typedef_tident(LPENUM_SERVICE_STATUS_PROCESS)

typedef struct tagENUMLOGFONTA {
  LOGFONTA elfLogFont;
  CHAR     elfFullName[LF_FULLFACESIZE];
  CHAR     elfStyle[LF_FACESIZE];
} ENUMLOGFONTA, *LPENUMLOGFONTA;

typedef struct tagENUMLOGFONTW {
  LOGFONTW elfLogFont;
  WCHAR    elfFullName[LF_FULLFACESIZE];
  WCHAR    elfStyle[LF_FACESIZE];
} ENUMLOGFONTW, *LPENUMLOGFONTW;

typedef_tident(ENUMLOGFONT)
typedef_tident(LPENUMLOGFONT)

typedef struct tagENUMLOGFONTEXA {
  LOGFONTA elfLogFont;
  CHAR     elfFullName[LF_FULLFACESIZE];
  CHAR     elfStyle[LF_FACESIZE];
  CHAR     elfScript[LF_FACESIZE];
} ENUMLOGFONTEXA, *LPENUMLOGFONTEXA;

typedef struct tagENUMLOGFONTEXW {
  LOGFONTW elfLogFont;
  WCHAR    elfFullName[LF_FULLFACESIZE];
  WCHAR    elfStyle[LF_FACESIZE];
  WCHAR    elfScript[LF_FACESIZE];
} ENUMLOGFONTEXW, *LPENUMLOGFONTEXW;

typedef_tident(ENUMLOGFONTEX)
typedef_tident(LPENUMLOGFONTEX)

typedef struct tagDESIGNVECTOR
{
    DWORD  dvReserved;
    DWORD  dvNumAxes;
    LONG   dvValues[MM_MAX_NUMAXES];
} DESIGNVECTOR, *PDESIGNVECTOR, FAR *LPDESIGNVECTOR;

typedef struct tagENUMLOGFONTEXDVA
{
    ENUMLOGFONTEXA elfEnumLogfontEx;
    DESIGNVECTOR   elfDesignVector;
} ENUMLOGFONTEXDVA, *PENUMLOGFONTEXDVA, FAR *LPENUMLOGFONTEXDVA;
typedef struct tagENUMLOGFONTEXDVW
{
    ENUMLOGFONTEXW elfEnumLogfontEx;
    DESIGNVECTOR   elfDesignVector;
} ENUMLOGFONTEXDVW, *PENUMLOGFONTEXDVW, FAR *LPENUMLOGFONTEXDVW;

typedef_tident(ENUMLOGFONTEXDV)
typedef_tident(PENUMLOGFONTEXDV)
typedef_tident(LPENUMLOGFONTEXDV)

typedef struct _EVENTLOGRECORD {
  DWORD  Length;
  DWORD  Reserved;
  DWORD  RecordNumber;
  DWORD  TimeGenerated;
  DWORD  TimeWritten;
  DWORD  EventID;
  WORD   EventType;
  WORD   NumStrings;
  WORD   EventCategory;
  WORD   ReservedFlags;
  DWORD  ClosingRecordNumber;
  DWORD  StringOffset;
  DWORD  UserSidLength;
  DWORD  UserSidOffset;
  DWORD  DataLength;
  DWORD  DataOffset;

/*
  Then follow:

  TCHAR SourceName[]
  TCHAR Computername[]
  SID   UserSid
  TCHAR Strings[]
  BYTE  Data[]
  CHAR  Pad[]
  DWORD Length;
*/

} EVENTLOGRECORD;

typedef struct tagEVENTMSG {
  UINT  message;
  UINT  paramL;
  UINT  paramH;
  DWORD time;
  HWND  hwnd;
} EVENTMSG;

typedef struct _EXT_BUTTON {
  WORD idCommand;
  WORD idsHelp;
  WORD fsStyle;
} EXT_BUTTON, *LPEXT_BUTTON;

typedef struct tagFILTERKEYS {
  UINT  cbSize;
  DWORD dwFlags;
  DWORD iWaitMSec;
  DWORD iDelayMSec;
  DWORD iRepeatMSec;
  DWORD iBounceMSec;
} FILTERKEYS;

typedef struct _FIND_NAME_BUFFER {
  UCHAR length;
  UCHAR access_control;
  UCHAR frame_control;
  UCHAR destination_addr[6];
  UCHAR source_addr[6];
  UCHAR routing_info[18];
} FIND_NAME_BUFFER;

typedef struct _FIND_NAME_HEADER {
  WORD  node_count;
  UCHAR reserved;
  UCHAR unique_group;
} FIND_NAME_HEADER;

typedef
enum _FINDEX_INFO_LEVELS
{
	FindExInfoStandard,
	FindExInfoMaxInfoLevel
} FINDEX_INFO_LEVELS;

typedef
enum _FINDEX_SEARCH_OPS
{
	FindExSearchNameMatch,
	FindExSearchLimitToDirectories,
	FindExSearchLimitToDevices,
	FindExSearchMaxSearchOp

} FINDEX_SEARCH_OPS;

#define FIND_FIRST_EX_CASE_SENSITIVE   0x00000001

typedef struct {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HINSTANCE    hInstance;
  DWORD        Flags;
  LPSTR        lpstrFindWhat;
  LPSTR        lpstrReplaceWith;
  WORD         wFindWhatLen;
  WORD         wReplaceWithLen;
  LPARAM       lCustData;
  LPFRHOOKPROC lpfnHook;
  LPCSTR       lpTemplateName;
} FINDREPLACEA, *LPFINDREPLACEA;

typedef struct {
  DWORD        lStructSize;
  HWND         hwndOwner;
  HINSTANCE    hInstance;
  DWORD        Flags;
  LPWSTR       lpstrFindWhat;
  LPWSTR       lpstrReplaceWith;
  WORD         wFindWhatLen;
  WORD         wReplaceWithLen;
  LPARAM       lCustData;
  LPFRHOOKPROC lpfnHook;
  LPCWSTR      lpTemplateName;
} FINDREPLACEW, *LPFINDREPLACEW;

typedef_tident(FINDREPLACE)
typedef_tident(LPFINDREPLACE)

typedef struct _findtext {
  CHARRANGE chrg;
  LPSTR lpstrText;
} FINDTEXT;

typedef struct _findtextex {
  CHARRANGE chrg;
  LPSTR lpstrText;
  CHARRANGE chrgText;
} FINDTEXTEX;

typedef struct _FMS_GETDRIVEINFOA {
  DWORD dwTotalSpace;
  DWORD dwFreeSpace;
  CHAR   szPath[260];
  CHAR   szVolume[14];
  CHAR   szShare[128];
} FMS_GETDRIVEINFOA;

typedef struct _FMS_GETDRIVEINFOW {
  DWORD dwTotalSpace;
  DWORD dwFreeSpace;
  WCHAR  szPath[260];
  WCHAR  szVolume[14];
  WCHAR  szShare[128];
} FMS_GETDRIVEINFOW;

typedef_tident(FMS_GETDRIVEINFO)

typedef struct _FMS_GETFILESELA {
  FILETIME ftTime;
  DWORD    dwSize;
  BYTE     bAttr;
  CHAR     szName[260];
} FMS_GETFILESELA;

typedef struct _FMS_GETFILESELW {
  FILETIME ftTime;
  DWORD    dwSize;
  BYTE     bAttr;
  WCHAR     szName[260];
} FMS_GETFILESELW;

typedef_tident(FMS_GETFILESEL)

typedef struct _FMS_LOADA {
  DWORD dwSize;
  CHAR  szMenuName[MENU_TEXT_LEN];
  HMENU hMenu;
  UINT  wMenuDelta;
} FMS_LOADA;

typedef struct _FMS_LOADW {
  DWORD dwSize;
  WCHAR  szMenuName[MENU_TEXT_LEN];
  HMENU hMenu;
  UINT  wMenuDelta;
} FMS_LOADW;

typedef_tident(FMS_LOAD)

typedef struct _FMS_TOOLBARLOAD {
  DWORD        dwSize;
  LPEXT_BUTTON lpButtons;
  WORD         cButtons;
  WORD         cBitmaps;
  WORD         idBitmap;
  HBITMAP      hBitmap;
} FMS_TOOLBARLOAD;


typedef struct _FORM_INFO_1A {
  DWORD  Flags;
  LPSTR  pName;
  SIZEL  Size;
  RECTL  ImageableArea;
} FORM_INFO_1A;

typedef struct _FORM_INFO_1W {
  DWORD  Flags;
  LPWSTR pName;
  SIZEL  Size;
  RECTL  ImageableArea;
} FORM_INFO_1W;

typedef_tident(FORM_INFO_1)

typedef struct _FORMAT_PARAMETERS {
  MEDIA_TYPE MediaType;
  DWORD StartCylinderNumber;
  DWORD EndCylinderNumber;
  DWORD StartHeadNumber;
  DWORD EndHeadNumber;
} FORMAT_PARAMETERS ;

typedef struct _formatrange {
  HDC hdc;
  HDC hdcTarget;
  RECT rc;
  RECT rcPage;
  CHARRANGE chrg;
} FORMATRANGE;

typedef struct tagGCP_RESULTSA {
  DWORD  lStructSize;
  LPSTR  lpOutString;
  UINT  *lpOrder;
  INT  *lpDx;
  INT  *lpCaretPos;
  LPSTR lpClass;
  UINT  *lpGlyphs;
  UINT  nGlyphs;
  UINT  nMaxFit;
} GCP_RESULTSA, *LPGCP_RESULTSA;

typedef struct tagGCP_RESULTSW {
  DWORD  lStructSize;
  LPWSTR  lpOutString;
  UINT  *lpOrder;
  INT  *lpDx;
  INT  *lpCaretPos;
  LPWSTR lpClass;
  UINT  *lpGlyphs;
  UINT  nGlyphs;
  UINT  nMaxFit;
} GCP_RESULTSW, *LPGCP_RESULTSW;

typedef_tident(GCP_RESULTS)


typedef struct _GLYPHMETRICS {
  UINT  gmBlackBoxX;
  UINT  gmBlackBoxY;
  POINT gmptGlyphOrigin;
  short gmCellIncX;
  short gmCellIncY;
} GLYPHMETRICS, *LPGLYPHMETRICS;

typedef struct tagHANDLETABLE {
  HGDIOBJ objectHandle[1];
} HANDLETABLE, *LPHANDLETABLE;

typedef struct _HD_HITTESTINFO {
  POINT pt;
  UINT flags;
  int iItem;
} HD_HITTESTINFO;

typedef struct _HD_ITEMA {
  UINT    mask;
  int     cxy;
  LPSTR   pszText;
  HBITMAP hbm;
  int     cchTextMax;
  int     fmt;
  LPARAM  lParam;
} HD_ITEMA;

typedef struct _HD_ITEMW {
  UINT    mask;
  int     cxy;
  LPWSTR  pszText;
  HBITMAP hbm;
  int     cchTextMax;
  int     fmt;
  LPARAM  lParam;
} HD_ITEMW;

typedef_tident(HD_ITEM)

typedef struct _WINDOWPOS {
  HWND hwnd;
  HWND hwndInsertAfter;
  int  x;
  int  y;
  int  cx;
  int  cy;
  UINT flags;
} WINDOWPOS, *PWINDOWPOS, *LPWINDOWPOS;

typedef struct _HD_LAYOUT {
  RECT  * prc;
  WINDOWPOS  * pwpos;
} HD_LAYOUT;

typedef struct _HD_NOTIFYA {
  NMHDR     hdr;
  int       iItem;
  int       iButton;
  HD_ITEMA *pitem;
} HD_NOTIFYA;

typedef struct _HD_NOTIFYW {
  NMHDR     hdr;
  int       iItem;
  int       iButton;
  HD_ITEMW *pitem;
} HD_NOTIFYW;

typedef_tident(HD_NOTIFY)

typedef  struct  tagHELPINFO {
  UINT   cbSize;
  int    iContextType;
  int    iCtrlId;
  HANDLE hItemHandle;
  DWORD  dwContextId;
  POINT  MousePos;
} HELPINFO,   *LPHELPINFO;

typedef struct tagMULTIKEYHELPA {
    DWORD   mkSize;
    CHAR    mkKeyList;
    CHAR    szKeyphrase[1];
} MULTIKEYHELPA, *PMULTIKEYHELPA, *LPMULTIKEYHELPA;

typedef struct tagMULTIKEYHELPW {
    DWORD   mkSize;
    WCHAR   mkKeyList;
    WCHAR   szKeyphrase[1];
} MULTIKEYHELPW, *PMULTIKEYHELPW, *LPMULTIKEYHELPW;

typedef_tident(MULTIKEYHELP)
typedef_tident(PMULTIKEYHELP)
typedef_tident(LPMULTIKEYHELP)

typedef struct {
	int wStructSize;
	int x;
	int y;
	int dx;
	int dy;
	int wMax;
	CHAR rgchMember[2];
} HELPWININFOA, *PHELPWININFOA, *LPHELPWININFOA;

typedef struct {
	int wStructSize;
	int x;
	int y;
	int dx;
	int dy;
	int wMax;
	WCHAR rgchMember[2];
} HELPWININFOW, *PHELPWININFOW, *LPHELPWININFOW;

typedef_tident(HELPWININFO)
typedef_tident(PHELPWININFO)
typedef_tident(LPHELPWININFO)

typedef struct tagHIGHCONTRASTA {
  UINT cbSize;
  DWORD dwFlags;
  LPSTR lpszDefaultScheme;
} HIGHCONTRASTA, *LPHIGHCONTRASTA;

typedef struct tagHIGHCONTRASTW {
  UINT cbSize;
  DWORD dwFlags;
  LPWSTR lpszDefaultScheme;
} HIGHCONTRASTW, *LPHIGHCONTRASTW;

typedef_tident(HIGHCONTRAST)
typedef_tident(LPHIGHCONTRAST)

typedef struct tagHSZPAIR {
  HSZ hszSvc;
  HSZ hszTopic;
} HSZPAIR;

typedef struct _ICONINFO {
  WINBOOL    fIcon;
  DWORD   xHotspot;
  DWORD   yHotspot;
  HBITMAP hbmMask;
  HBITMAP hbmColor;
} ICONINFO, *PICONINFO;

typedef struct tagICONMETRICSA {
  UINT     cbSize;
  int      iHorzSpacing;
  int      iVertSpacing;
  int      iTitleWrap;
  LOGFONTA lfFont;
} ICONMETRICSA, *LPICONMETRICSA;

typedef struct tagICONMETRICSW {
  UINT     cbSize;
  int      iHorzSpacing;
  int      iVertSpacing;
  int      iTitleWrap;
  LOGFONTW lfFont;
} ICONMETRICSW, *LPICONMETRICSW;

typedef_tident(ICONMETRICS)
typedef_tident(LPICONMETRICS)

typedef struct _IMAGEINFO {
  HBITMAP hbmImage;
  HBITMAP hbmMask;
  int     Unused1;
  int     Unused2;
  RECT    rcImage;
} IMAGEINFO;

typedef struct _JOB_INFO_1A {
  DWORD  JobId;
  LPSTR  pPrinterName;
  LPSTR  pMachineName;
  LPSTR  pUserName;
  LPSTR  pDocument;
  LPSTR  pDatatype;
  LPSTR  pStatus;
  DWORD  Status;
  DWORD  Priority;
  DWORD  Position;
  DWORD  TotalPages;
  DWORD  PagesPrinted;
  SYSTEMTIME Submitted;
} JOB_INFO_1A;

typedef struct _JOB_INFO_1W {
  DWORD  JobId;
  LPWSTR pPrinterName;
  LPWSTR pMachineName;
  LPWSTR pUserName;
  LPWSTR pDocument;
  LPWSTR pDatatype;
  LPWSTR pStatus;
  DWORD  Status;
  DWORD  Priority;
  DWORD  Position;
  DWORD  TotalPages;
  DWORD  PagesPrinted;
  SYSTEMTIME Submitted;
} JOB_INFO_1W;

typedef_tident(JOB_INFO_1)

#if 0
typedef struct _JOB_INFO_2A {
  DWORD      JobId;
  LPSTR      pPrinterName;
  LPSTR      pMachineName;
  LPSTR      pUserName;
  LPSTR      pDocument;
  LPSTR      pNotifyName;
  LPSTR      pDatatype;
  LPSTR      pPrintProcessor;
  LPSTR      pParameters;
  LPSTR      pDriverName;
  LPDEVMODE  pDevMode;
  LPSTR      pStatus;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD      Status;
  DWORD      Priority;
  DWORD      Position;
  DWORD      StartTime;
  DWORD      UntilTime;
  DWORD      TotalPages;
  DWORD      Size;
  SYSTEMTIME Submitted;
  DWORD      Time;
  DWORD      PagesPrinted ;
} JOB_INFO_2A;

typedef struct _JOB_INFO_2W {
  DWORD      JobId;
  LPWSTR     pPrinterName;
  LPWSTR     pMachineName;
  LPWSTR     pUserName;
  LPWSTR     pDocument;
  LPWSTR     pNotifyName;
  LPWSTR     pDatatype;
  LPWSTR     pPrintProcessor;
  LPWSTR     pParameters;
  LPWSTR     pDriverName;
  LPDEVMODE  pDevMode;
  LPWSTR     pStatus;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD      Status;
  DWORD      Priority;
  DWORD      Position;
  DWORD      StartTime;
  DWORD      UntilTime;
  DWORD      TotalPages;
  DWORD      Size;
  SYSTEMTIME Submitted;
  DWORD      Time;
  DWORD      PagesPrinted ;
} JOB_INFO_2W;

typedef_tident(JOB_INFO_2)
#endif/*0*/

typedef struct tagKERNINGPAIR {
  WORD wFirst;
  WORD wSecond;
  int  iKernAmount;
} KERNINGPAIR, *LPKERNINGPAIR;

typedef struct _LANA_ENUM {
  UCHAR length;
  UCHAR lana[MAX_LANA];
} LANA_ENUM;


typedef struct tagLOADPARMS32 {
  LPSTR lpEnvAddress;
  LPSTR lpCmdLine;
  LPSTR lpCmdShow;
  DWORD dwReserved;
} LOADPARMS32;


typedef struct tagLOCALESIGNATURE {
  DWORD  lsUsb[4];
  DWORD  lsCsbDefault[2];
  DWORD  lsCsbSupported[2];
} LOCALESIGNATURE;

   #if 0
typedef struct _LOCALGROUP_MEMBERS_INFO_0 {
  PSID  lgrmi0_sid;
} LOCALGROUP_MEMBERS_INFO_0;
 #endif

typedef struct _LOCALGROUP_MEMBERS_INFO_3 {
  LPWSTR  lgrmi3_domainandname;
} LOCALGROUP_MEMBERS_INFO_3;

typedef long FXPT16DOT16,  * LPFXPT16DOT16;



typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;

typedef struct _LV_COLUMNA {
  UINT mask;
  int fmt;
  int cx;
  LPSTR pszText;
  int cchTextMax;
  int iSubItem;
} LV_COLUMNA;

typedef struct _LV_COLUMNW {
  UINT mask;
  int fmt;
  int cx;
  LPWSTR pszText;
  int cchTextMax;
  int iSubItem;
} LV_COLUMNW;

typedef_tident(LV_COLUMN)

typedef struct _LV_ITEMA {
  UINT   mask;
  int    iItem;
  int    iSubItem;
  UINT   state;
  UINT   stateMask;
  LPSTR  pszText;
  int    cchTextMax;
  int    iImage;
  LPARAM lParam;
} LV_ITEMA;

typedef struct _LV_ITEMW {
  UINT   mask;
  int    iItem;
  int    iSubItem;
  UINT   state;
  UINT   stateMask;
  LPWSTR  pszText;
  int    cchTextMax;
  int    iImage;
  LPARAM lParam;
} LV_ITEMW;

typedef_tident(LV_ITEM)

typedef struct tagLV_DISPINFOA {
  NMHDR    hdr;
  LV_ITEMA item;
} LV_DISPINFOA;

typedef struct tagLV_DISPINFOW {
  NMHDR    hdr;
  LV_ITEMW item;
} LV_DISPINFOW;

typedef_tident(LV_DISPINFO)

typedef struct _LV_FINDINFOA {
  UINT flags;
  LPCSTR psz;
  LPARAM lParam;
  POINT pt;
  UINT vkDirection;
} LV_FINDINFOA;

typedef struct _LV_FINDINFOW {
  UINT flags;
  LPCWSTR psz;
  LPARAM lParam;
  POINT pt;
  UINT vkDirection;
} LV_FINDINFOW;

typedef_tident(LV_FINDINFO)

typedef struct _LV_HITTESTINFO {
  POINT pt;
  UINT flags;
  int iItem;
} LV_HITTESTINFO;

typedef struct tagLV_KEYDOWN {
  NMHDR hdr;
  WORD wVKey;
  UINT flags;
} LV_KEYDOWN;

typedef struct _MAT2 {
  FIXED eM11;
  FIXED eM12;
  FIXED eM21;
  FIXED eM22;
} MAT2, *LPMAT2;

typedef struct tagMDICREATESTRUCTA {
  LPCSTR  szClass;
  LPCSTR  szTitle;
  HANDLE  hOwner;
  int     x;
  int     y;
  int     cx;
  int     cy;
  DWORD   style;
  LPARAM  lParam;
} MDICREATESTRUCTA, *LPMDICREATESTRUCTA;

typedef struct tagMDICREATESTRUCTW {
  LPCWSTR szClass;
  LPCWSTR szTitle;
  HANDLE  hOwner;
  int     x;
  int     y;
  int     cx;
  int     cy;
  DWORD   style;
  LPARAM  lParam;
} MDICREATESTRUCTW, *LPMDICREATESTRUCTW;

typedef_tident(MDICREATESTRUCT)
typedef_tident(LPMDICREATESTRUCT)

typedef struct tagMDINEXTMENU {
	HMENU hmenuIn;
	HMENU hmenuNext;
	HWND hwndNext;
} MDINEXTMENU,*PMDINEXTMENU,*LPMDINEXTMENU;

typedef struct tagMEASUREITEMSTRUCT {
  UINT  CtlType;
  UINT  CtlID;
  UINT  itemID;
  UINT  itemWidth;
  UINT  itemHeight;
  DWORD itemData;
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;

typedef struct _MEMORYSTATUS {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORD dwTotalPhys;
  DWORD dwAvailPhys;
  DWORD dwTotalPageFile;
  DWORD dwAvailPageFile;
  DWORD dwTotalVirtual;
  DWORD dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef struct {
  WORD  wVersion;
  WORD  wOffset;
  DWORD dwHelpId;
} MENUEX_TEMPLATE_HEADER;

typedef struct {
  DWORD  dwType;
  DWORD  dwState;
  UINT   uId;
  BYTE   bResInfo;
  WCHAR  szText[1];
  DWORD dwHelpId;
} MENUEX_TEMPLATE_ITEM;


typedef struct tagMENUITEMINFOA {
  UINT    cbSize;
  UINT    fMask;
  UINT    fType;
  UINT    fState;
  UINT    wID;
  HMENU   hSubMenu;
  HBITMAP hbmpChecked;
  HBITMAP hbmpUnchecked;
  DWORD   dwItemData;
  LPSTR   dwTypeData;
  UINT    cch;
  HBITMAP  hbmpItem;
} MENUITEMINFOA, *LPMENUITEMINFOA;
typedef CONST MENUITEMINFOA* LPCMENUITEMINFOA;

typedef struct tagMENUITEMINFOW {
  UINT    cbSize;
  UINT    fMask;
  UINT    fType;
  UINT    fState;
  UINT    wID;
  HMENU   hSubMenu;
  HBITMAP hbmpChecked;
  HBITMAP hbmpUnchecked;
  DWORD   dwItemData;
  LPWSTR  dwTypeData;
  UINT    cch;
  HBITMAP  hbmpItem;
} MENUITEMINFOW, *LPMENUITEMINFOW;
typedef CONST MENUITEMINFOW* LPCMENUITEMINFOW;

typedef_tident(MENUITEMINFO)
typedef_tident(LPMENUITEMINFO)
typedef_tident(LPCMENUITEMINFO)

typedef struct {
  WORD mtOption;
  WORD mtID;
  WCHAR mtString[1];
} MENUITEMTEMPLATE;

typedef struct {
  WORD versionNumber;
  WORD offset;
} MENUITEMTEMPLATEHEADER;
typedef VOID MENUTEMPLATE, *LPMENUTEMPLATE;

typedef struct tagMETAFILEPICT {
  LONG      mm;
  LONG      xExt;
  LONG      yExt;
  HMETAFILE hMF;
} METAFILEPICT, *PMETAFILEPICT, *LPMETAFILEPICT;


#include <pshpack1.h>

typedef struct tagMETAHEADER {
  WORD  mtType;
  WORD  mtHeaderSize;
  WORD  mtVersion;
  DWORD mtSize;
  WORD  mtNoObjects;
  DWORD mtMaxRecord;
  WORD  mtNoParameters;
} METAHEADER;

#include <poppack.h>


typedef struct tagMETARECORD {
  DWORD rdSize;
  WORD  rdFunction;
  WORD  rdParm[1];
} METARECORD, *LPMETARECORD;

typedef struct tagMINIMIZEDMETRICS {
  UINT    cbSize;
  int     iWidth;
  int     iHorzGap;
  int     iVertGap;
  int     iArrange;
}   MINIMIZEDMETRICS,   *LPMINIMIZEDMETRICS;

typedef struct tagMINMAXINFO {
  POINT ptReserved;
  POINT ptMaxSize;
  POINT ptMaxPosition;
  POINT ptMinTrackSize;
  POINT ptMaxTrackSize;
} MINMAXINFO;

typedef struct modemdevcaps_tag {
  DWORD dwActualSize;
  DWORD dwRequiredSize;
  DWORD dwDevSpecificOffset;
  DWORD dwDevSpecificSize;

  DWORD dwModemProviderVersion;
  DWORD dwModemManufacturerOffset;
  DWORD dwModemManufacturerSize;
  DWORD dwModemModelOffset;
  DWORD dwModemModelSize;
  DWORD dwModemVersionOffset;
  DWORD dwModemVersionSize;

  DWORD dwDialOptions;
  DWORD dwCallSetupFailTimer;
  DWORD dwInactivityTimeout;
  DWORD dwSpeakerVolume;
  DWORD dwSpeakerMode;
  DWORD dwModemOptions;
  DWORD dwMaxDTERate;
  DWORD dwMaxDCERate;

  BYTE abVariablePortion [1];
} MODEMDEVCAPS, *PMODEMDEVCAPS, *LPMODEMDEVCAPS;

typedef struct modemsettings_tag {
  DWORD dwActualSize;
  DWORD dwRequiredSize;
  DWORD dwDevSpecificOffset;
  DWORD dwDevSpecificSize;

  DWORD dwCallSetupFailTimer;
  DWORD dwInactivityTimeout;
  DWORD dwSpeakerVolume;
  DWORD dwSpeakerMode;
  DWORD dwPreferredModemOptions;

  DWORD dwNegotiatedModemOptions;
  DWORD dwNegotiatedDCERate;

  BYTE  abVariablePortion[1];
} MODEMSETTINGS, *PMODEMSETTINGS, *LPMODEMSETTINGS;

typedef struct tagMONCBSTRUCT {
  UINT   cb;
  DWORD  dwTime;
  HANDLE hTask;
  DWORD  dwRet;
  UINT   wType;
  UINT   wFmt;
  HCONV  hConv;
  HSZ    hsz1;
  HSZ    hsz2;
  HDDEDATA hData;
  DWORD    dwData1;
  DWORD    dwData2;
  CONVCONTEXT cc;
  DWORD  cbData;
  DWORD  Data[8];
} MONCBSTRUCT;

typedef struct tagMONCONVSTRUCT {
  UINT   cb;
  WINBOOL   fConnect;
  DWORD  dwTime;
  HANDLE hTask;
  HSZ    hszSvc;
  HSZ    hszTopic;
  HCONV  hConvClient;
  HCONV  hConvServer;
} MONCONVSTRUCT;

typedef struct tagMONERRSTRUCT {
  UINT   cb;
  UINT   wLastError;
  DWORD  dwTime;
  HANDLE hTask;
} MONERRSTRUCT;

typedef struct tagMONHSZSTRUCTA {
  UINT   cb;
  WINBOOL   fsAction;
  DWORD  dwTime;
  HSZ    hsz;
  HANDLE hTask;
  CHAR   str[1];
} MONHSZSTRUCTA;

typedef struct tagMONHSZSTRUCTW {
  UINT   cb;
  WINBOOL   fsAction;
  DWORD  dwTime;
  HSZ    hsz;
  HANDLE hTask;
  WCHAR   str[1];
} MONHSZSTRUCTW;

typedef_tident(MONHSZSTRUCT)

typedef struct _MONITOR_INFO_1A {
  LPSTR  pName;
} MONITOR_INFO_1A;

typedef struct _MONITOR_INFO_1W {
  LPWSTR pName;
} MONITOR_INFO_1W;

typedef_tident(MONITOR_INFO_1)

typedef struct _MONITOR_INFO_2A {
  LPSTR  pName;
  LPSTR  pEnvironment ;
  LPSTR  pDLLName ;
} MONITOR_INFO_2A;

typedef struct _MONITOR_INFO_2W {
  LPWSTR pName;
  LPWSTR pEnvironment ;
  LPWSTR pDLLName ;
} MONITOR_INFO_2W;

typedef_tident(MONITOR_INFO_2)

typedef struct tagMONLINKSTRUCT {
  UINT   cb;
  DWORD  dwTime;
  HANDLE hTask;
  WINBOOL   fEstablished;
  WINBOOL   fNoData;
  HSZ    hszSvc;
  HSZ    hszTopic;
  HSZ    hszItem;
  UINT   wFmt;
  WINBOOL   fServer;
  HCONV  hConvServer;
  HCONV  hConvClient;
} MONLINKSTRUCT;

typedef struct tagMONMSGSTRUCT {
  UINT   cb;
  HWND   hwndTo;
  DWORD  dwTime;
  HANDLE hTask;
  UINT   wMsg;
  WPARAM wParam;
  LPARAM lParam;
  DDEML_MSG_HOOK_DATA dmhd;
} MONMSGSTRUCT;

typedef struct tagMOUSEHOOKSTRUCT {
  POINT pt;
  HWND  hwnd;
  UINT  wHitTestCode;
  DWORD dwExtraInfo;
} MOUSEHOOKSTRUCT, *PMOUSEHOOKSTRUCT, *LPMOUSEHOOKSTRUCT;

typedef struct _MOUSEKEYS {
  DWORD cbSize;
  DWORD dwFlags;
  DWORD iMaxSpeed;
  DWORD iTimeToMaxSpeed;
  DWORD iCtrlSpeed;
  DWORD dwReserved1;
  DWORD dwReserved2;
} MOUSEKEYS;

typedef struct tagMSG {
  HWND   hwnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
  POINT  pt;
} MSG, *LPMSG;

typedef void (CALLBACK *MSGBOXCALLBACK) (LPHELPINFO lpHelpInfo);

typedef struct {
  UINT      cbSize;
  HWND      hwndOwner;
  HINSTANCE hInstance;
  LPCSTR    lpszText;
  LPCSTR    lpszCaption;
  DWORD     dwStyle;
  LPCSTR    lpszIcon;
  DWORD     dwContextHelpId;
  MSGBOXCALLBACK lpfnMsgBoxCallback;
  DWORD     dwLanguageId;
} MSGBOXPARAMS, *PMSGBOXPARAMS,   *LPMSGBOXPARAMS;

typedef struct _msgfilter {
  NMHDR nmhdr;
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;
} MSGFILTER;

typedef struct _NAME_BUFFER {
  UCHAR name[NCBNAMSZ];
  UCHAR name_num;
  UCHAR name_flags;
} NAME_BUFFER;

typedef struct _NCB {
  UCHAR  ncb_command;
  UCHAR  ncb_retcode;
  UCHAR  ncb_lsn;
  UCHAR  ncb_num;
  PUCHAR ncb_buffer;
  WORD   ncb_length;
  UCHAR  ncb_callname[NCBNAMSZ];
  UCHAR  ncb_name[NCBNAMSZ];
  UCHAR  ncb_rto;
  UCHAR  ncb_sto;
  void (*ncb_post) (struct _NCB *);
  UCHAR  ncb_lana_num;
  UCHAR  ncb_cmd_cplt;
  UCHAR  ncb_reserve[10];
  HANDLE ncb_event;
} NCB;

typedef struct _NCCALCSIZE_PARAMS {
  RECT        rgrc[3];
  PWINDOWPOS  lppos;
} NCCALCSIZE_PARAMS;

typedef struct _NDDESHAREINFOA {
  LONG   lRevision;
  LPSTR  lpszShareName;
  LONG   lShareType;
  LPSTR  lpszAppTopicList;
  LONG   fSharedFlag;
  LONG   fService;
  LONG   fStartAppFlag;
  LONG   nCmdShow;
  LONG   qModifyId[2];
  LONG   cNumItems;
  LPSTR  lpszItemList;
}NDDESHAREINFOA;

typedef struct _NDDESHAREINFOW {
  LONG   lRevision;
  LPWSTR lpszShareName;
  LONG   lShareType;
  LPWSTR lpszAppTopicList;
  LONG   fSharedFlag;
  LONG   fService;
  LONG   fStartAppFlag;
  LONG   nCmdShow;
  LONG   qModifyId[2];
  LONG   cNumItems;
  LPWSTR lpszItemList;
}NDDESHAREINFOW;

typedef_tident(NDDESHAREINFO)

typedef struct _NETRESOURCEA {
  DWORD  dwScope;
  DWORD  dwType;
  DWORD  dwDisplayType;
  DWORD  dwUsage;
  LPSTR  lpLocalName;
  LPSTR  lpRemoteName;
  LPSTR  lpComment;
  LPSTR  lpProvider;
} NETRESOURCEA, *LPNETRESOURCEA;

typedef struct _NETRESOURCEW {
  DWORD  dwScope;
  DWORD  dwType;
  DWORD  dwDisplayType;
  DWORD  dwUsage;
  LPWSTR lpLocalName;
  LPWSTR lpRemoteName;
  LPWSTR lpComment;
  LPWSTR lpProvider;
} NETRESOURCEW, *LPNETRESOURCEW;

typedef_tident(NETRESOURCE)
typedef_tident(LPNETRESOURCE)

typedef struct tagNEWCPLINFOA {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwHelpContext;
  LONG  lData;
  HICON hIcon;
  CHAR  szName[32];
  CHAR  szInfo[64];
  CHAR  szHelpFile[128];
} NEWCPLINFOA;

typedef struct tagNEWCPLINFOW {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwHelpContext;
  LONG  lData;
  HICON hIcon;
  WCHAR  szName[32];
  WCHAR  szInfo[64];
  WCHAR  szHelpFile[128];
} NEWCPLINFOW;

typedef_tident(NEWCPLINFO)

typedef struct tagNEWTEXTMETRICA {
  LONG   tmHeight;
  LONG   tmAscent;
  LONG   tmDescent;
  LONG   tmInternalLeading;
  LONG   tmExternalLeading;
  LONG   tmAveCharWidth;
  LONG   tmMaxCharWidth;
  LONG   tmWeight;
  LONG   tmOverhang;
  LONG   tmDigitizedAspectX;
  LONG   tmDigitizedAspectY;
  CHAR   tmFirstChar;
  CHAR   tmLastChar;
  CHAR   tmDefaultChar;
  CHAR   tmBreakChar;
  BYTE   tmItalic;
  BYTE   tmUnderlined;
  BYTE   tmStruckOut;
  BYTE   tmPitchAndFamily;
  BYTE   tmCharSet;
  DWORD  ntmFlags;
  UINT   ntmSizeEM;
  UINT   ntmCellHeight;
  UINT   ntmAvgWidth;
} NEWTEXTMETRICA;

typedef struct tagNEWTEXTMETRICW {
  LONG   tmHeight;
  LONG   tmAscent;
  LONG   tmDescent;
  LONG   tmInternalLeading;
  LONG   tmExternalLeading;
  LONG   tmAveCharWidth;
  LONG   tmMaxCharWidth;
  LONG   tmWeight;
  LONG   tmOverhang;
  LONG   tmDigitizedAspectX;
  LONG   tmDigitizedAspectY;
  WCHAR  tmFirstChar;
  WCHAR  tmLastChar;
  WCHAR  tmDefaultChar;
  WCHAR  tmBreakChar;
  BYTE   tmItalic;
  BYTE   tmUnderlined;
  BYTE   tmStruckOut;
  BYTE   tmPitchAndFamily;
  BYTE   tmCharSet;
  DWORD  ntmFlags;
  UINT   ntmSizeEM;
  UINT   ntmCellHeight;
  UINT   ntmAvgWidth;
} NEWTEXTMETRICW;

typedef_tident(NEWTEXTMETRIC)

typedef struct tagNEWTEXTMETRICEXA {
  NEWTEXTMETRICA ntmTm;
  FONTSIGNATURE  ntmFontSig;
} NEWTEXTMETRICEXA;

typedef struct tagNEWTEXTMETRICEXW {
  NEWTEXTMETRICW ntmTm;
  FONTSIGNATURE  ntmFontSig;
} NEWTEXTMETRICEXW;

typedef_tident(NEWTEXTMETRICEX)

typedef struct tagNM_LISTVIEW {
  NMHDR hdr;
  int   iItem;
  int   iSubItem;
  UINT  uNewState;
  UINT  uOldState;
  UINT  uChanged;
  POINT ptAction;
  LPARAM lParam;
} NM_LISTVIEW;

typedef struct _TREEITEM *HTREEITEM;

typedef struct _TV_ITEMA {
  UINT       mask;
  HTREEITEM  hItem;
  UINT       state;
  UINT       stateMask;
  LPSTR      pszText;
  int        cchTextMax;
  int        iImage;
  int        iSelectedImage;
  int        cChildren;
  LPARAM     lParam;
} TV_ITEMA,   *LPTV_ITEMA;

typedef struct _TV_ITEMW {
  UINT       mask;
  HTREEITEM  hItem;
  UINT       state;
  UINT       stateMask;
  LPWSTR     pszText;
  int        cchTextMax;
  int        iImage;
  int        iSelectedImage;
  int        cChildren;
  LPARAM     lParam;
} TV_ITEMW,   *LPTV_ITEMW;

typedef_tident(TV_ITEM)
typedef_tident(LPTV_ITEM)

typedef struct _NM_TREEVIEWA {
  NMHDR    hdr;
  UINT     action;
  TV_ITEMA itemOld;
  TV_ITEMA itemNew;
  POINT    ptDrag;
} NM_TREEVIEWA, *LPNM_TREEVIEWA;

typedef struct _NM_TREEVIEWW {
  NMHDR    hdr;
  UINT     action;
  TV_ITEMW itemOld;
  TV_ITEMW itemNew;
  POINT    ptDrag;
} NM_TREEVIEWW, *LPNM_TREEVIEWW;

typedef_tident(NM_TREEVIEW)
typedef_tident(LPNM_TREEVIEW)


typedef struct _NM_UPDOWN {
  NMHDR    hdr;
  int     iPos;
  int  iDelta;
} NM_UPDOWNW;

typedef struct tagNONCLIENTMETRICSA {
  UINT     cbSize;
  int      iBorderWidth;
  int      iScrollWidth;
  int      iScrollHeight;
  int      iCaptionWidth;
  int      iCaptionHeight;
  LOGFONTA lfCaptionFont;
  int      iSmCaptionWidth;
  int      iSmCaptionHeight;
  LOGFONTA lfSmCaptionFont;
  int      iMenuWidth;
  int      iMenuHeight;
  LOGFONTA lfMenuFont;
  LOGFONTA lfStatusFont;
  LOGFONTA lfMessageFont;
} NONCLIENTMETRICSA, *LPNONCLIENTMETRICSA;

typedef struct tagNONCLIENTMETRICSW {
  UINT     cbSize;
  int      iBorderWidth;
  int      iScrollWidth;
  int      iScrollHeight;
  int      iCaptionWidth;
  int      iCaptionHeight;
  LOGFONTW lfCaptionFont;
  int      iSmCaptionWidth;
  int      iSmCaptionHeight;
  LOGFONTW lfSmCaptionFont;
  int      iMenuWidth;
  int      iMenuHeight;
  LOGFONTW lfMenuFont;
  LOGFONTW lfStatusFont;
  LOGFONTW lfMessageFont;
} NONCLIENTMETRICSW, *LPNONCLIENTMETRICSW;

typedef_tident(NONCLIENTMETRICS)
typedef_tident(LPNONCLIENTMETRICS)

#include <serviceinfo.h>

#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short  Data2;
    unsigned short  Data3;
    unsigned char Data4[8];
} GUID, *LPGUID;
typedef GUID CLSID, *LPCLSID;

#endif/*GUID_DEFINED*/

typedef_tident(SERVICE_INFO);
typedef_tident(LPSERVICE_INFO);

typedef struct _NS_SERVICE_INFOA {
  DWORD         dwNameSpace;
  SERVICE_INFOA ServiceInfo;
} NS_SERVICE_INFOA;

typedef struct _NS_SERVICE_INFOW {
  DWORD         dwNameSpace;
  SERVICE_INFOW ServiceInfo;
} NS_SERVICE_INFOW;

typedef_tident(NS_SERVICE_INFO);

typedef struct _numberfmtA {
  UINT      NumDigits;
  UINT      LeadingZero;
  UINT      Grouping;
  LPSTR     lpDecimalSep;
  LPSTR     lpThousandSep;
  UINT      NegativeOrder;
} NUMBERFMTA;

typedef struct _numberfmtW {
  UINT      NumDigits;
  UINT      LeadingZero;
  UINT      Grouping;
  LPWSTR    lpDecimalSep;
  LPWSTR    lpThousandSep;
  UINT      NegativeOrder;
} NUMBERFMTW;

typedef_tident(NUMBERFMT)

typedef struct _OFSTRUCT {
  BYTE cBytes;
  BYTE fFixedDisk;
  WORD nErrCode;
  WORD Reserved1;
  WORD Reserved2;
  CHAR szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT;

typedef struct tagOFNA {
  DWORD         lStructSize;
  HWND          hwndOwner;
  HINSTANCE     hInstance;
  LPCSTR        lpstrFilter;
  LPSTR         lpstrCustomFilter;
  DWORD         nMaxCustFilter;
  DWORD         nFilterIndex;
  LPSTR         lpstrFile;
  DWORD         nMaxFile;
  LPSTR         lpstrFileTitle;
  DWORD         nMaxFileTitle;
  LPCSTR        lpstrInitialDir;
  LPCSTR        lpstrTitle;
  DWORD         Flags;
  WORD          nFileOffset;
  WORD          nFileExtension;
  LPCSTR        lpstrDefExt;
  DWORD         lCustData;
  LPOFNHOOKPROC lpfnHook;
  LPCSTR        lpTemplateName;
} OPENFILENAMEA, *LPOPENFILENAMEA;

typedef struct tagOFNW {
  DWORD         lStructSize;
  HWND          hwndOwner;
  HINSTANCE     hInstance;
  LPCWSTR       lpstrFilter;
  LPWSTR        lpstrCustomFilter;
  DWORD         nMaxCustFilter;
  DWORD         nFilterIndex;
  LPWSTR        lpstrFile;
  DWORD         nMaxFile;
  LPWSTR        lpstrFileTitle;
  DWORD         nMaxFileTitle;
  LPCWSTR       lpstrInitialDir;
  LPCWSTR       lpstrTitle;
  DWORD         Flags;
  WORD          nFileOffset;
  WORD          nFileExtension;
  LPCWSTR       lpstrDefExt;
  DWORD         lCustData;
  LPOFNHOOKPROC lpfnHook;
  LPCWSTR       lpTemplateName;
} OPENFILENAMEW, *LPOPENFILENAMEW;

typedef_tident(OPENFILENAME)
typedef_tident(LPOPENFILENAME)

typedef struct _OFNOTIFYA {
  NMHDR           hdr;
  LPOPENFILENAMEA lpOFN;
  LPSTR           pszFile;
} OFNOTIFYA, *LPOFNOTIFYA;

typedef struct _OFNOTIFYW {
  NMHDR           hdr;
  LPOPENFILENAMEW lpOFN;
  LPWSTR          pszFile;
} OFNOTIFYW, *LPOFNOTIFYW;

typedef_tident(OFNOTIFY)
typedef_tident(LPOFNOTIFY)

typedef struct _OSVERSIONINFOA {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR szCSDVersion[ 128 ];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;

typedef struct _OSVERSIONINFOW {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  WCHAR szCSDVersion[ 128 ];
} OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW, RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

typedef_tident(OSVERSIONINFO)

typedef struct _OSVERSIONINFOEXA
#if defined(__cplusplus)
: public OSVERSIONINFOA
{
#elif 0
{
 OSVERSIONINFOA;
#else
{
 DWORD dwOSVersionInfoSize;
 DWORD dwMajorVersion;
 DWORD dwMinorVersion;
 DWORD dwBuildNumber;
 DWORD dwPlatformId;
 CHAR szCSDVersion[ 128 ];
#endif
 WORD wServicePackMajor;
 WORD wServicePackMinor;
 WORD wSuiteMask;
 BYTE wProductType;
 BYTE wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;

typedef struct _OSVERSIONINFOEXW
#if defined(__cplusplus)
: public OSVERSIONINFOW
{
#elif 0
{
 OSVERSIONINFOW;
#else
{
 DWORD dwOSVersionInfoSize;
 DWORD dwMajorVersion;
 DWORD dwMinorVersion;
 DWORD dwBuildNumber;
 DWORD dwPlatformId;
 WCHAR szCSDVersion[ 128 ];
#endif
 WORD wServicePackMajor;
 WORD wServicePackMinor;
 WORD wSuiteMask;
 BYTE wProductType;
 BYTE wReserved;
} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW, RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

typedef_tident(OSVERSIONINFOEX)

typedef struct tagTEXTMETRICA {
  LONG tmHeight;
  LONG tmAscent;
  LONG tmDescent;
  LONG tmInternalLeading;
  LONG tmExternalLeading;
  LONG tmAveCharWidth;
  LONG tmMaxCharWidth;
  LONG tmWeight;
  LONG tmOverhang;
  LONG tmDigitizedAspectX;
  LONG tmDigitizedAspectY;
  CHAR tmFirstChar;
  CHAR tmLastChar;
  CHAR tmDefaultChar;
  CHAR tmBreakChar;
  BYTE tmItalic;
  BYTE tmUnderlined;
  BYTE tmStruckOut;
  BYTE tmPitchAndFamily;
  BYTE tmCharSet;
} TEXTMETRICA, *LPTEXTMETRICA;

typedef struct tagTEXTMETRICW {
  LONG tmHeight;
  LONG tmAscent;
  LONG tmDescent;
  LONG tmInternalLeading;
  LONG tmExternalLeading;
  LONG tmAveCharWidth;
  LONG tmMaxCharWidth;
  LONG tmWeight;
  LONG tmOverhang;
  LONG tmDigitizedAspectX;
  LONG tmDigitizedAspectY;
  WCHAR tmFirstChar;
  WCHAR tmLastChar;
  WCHAR tmDefaultChar;
  WCHAR tmBreakChar;
  BYTE tmItalic;
  BYTE tmUnderlined;
  BYTE tmStruckOut;
  BYTE tmPitchAndFamily;
  BYTE tmCharSet;
} TEXTMETRICW, *LPTEXTMETRICW;

typedef_tident(TEXTMETRIC)
typedef_tident(LPTEXTMETRIC)

typedef struct _OUTLINETEXTMETRICA {
  UINT   otmSize;
  TEXTMETRICA otmTextMetrics;
  BYTE   otmFiller;
  PANOSE otmPanoseNumber;
  UINT   otmfsSelection;
  UINT   otmfsType;
  int    otmsCharSlopeRise;
  int    otmsCharSlopeRun;
  int    otmItalicAngle;
  UINT   otmEMSquare;
  int    otmAscent;
  int    otmDescent;
  UINT   otmLineGap;
  UINT   otmsCapEmHeight;
  UINT   otmsXHeight;
  RECT   otmrcFontBox;
  int    otmMacAscent;
  int    otmMacDescent;
  UINT   otmMacLineGap;
  UINT   otmusMinimumPPEM;
  POINT  otmptSubscriptSize;
  POINT  otmptSubscriptOffset;
  POINT  otmptSuperscriptSize;
  POINT  otmptSuperscriptOffset;
  UINT   otmsStrikeoutSize;
  int    otmsStrikeoutPosition;
  int    otmsUnderscoreSize;
  int    otmsUnderscorePosition;
  PSTR   otmpFamilyName;
  PSTR   otmpFaceName;
  PSTR   otmpStyleName;
  PSTR   otmpFullName;
} OUTLINETEXTMETRICA, *LPOUTLINETEXTMETRICA;

typedef struct _OUTLINETEXTMETRICW {
  UINT   otmSize;
  TEXTMETRICW otmTextMetrics;
  BYTE   otmFiller;
  PANOSE otmPanoseNumber;
  UINT   otmfsSelection;
  UINT   otmfsType;
  int    otmsCharSlopeRise;
  int    otmsCharSlopeRun;
  int    otmItalicAngle;
  UINT   otmEMSquare;
  int    otmAscent;
  int    otmDescent;
  UINT   otmLineGap;
  UINT   otmsCapEmHeight;
  UINT   otmsXHeight;
  RECT   otmrcFontBox;
  int    otmMacAscent;
  int    otmMacDescent;
  UINT   otmMacLineGap;
  UINT   otmusMinimumPPEM;
  POINT  otmptSubscriptSize;
  POINT  otmptSubscriptOffset;
  POINT  otmptSuperscriptSize;
  POINT  otmptSuperscriptOffset;
  UINT   otmsStrikeoutSize;
  int    otmsStrikeoutPosition;
  int    otmsUnderscoreSize;
  int    otmsUnderscorePosition;
  PSTR   otmpFamilyName;
  PSTR   otmpFaceName;
  PSTR   otmpStyleName;
  PSTR   otmpFullName;
} OUTLINETEXTMETRICW, *LPOUTLINETEXTMETRICW;

typedef_tident(OUTLINETEXTMETRIC)
typedef_tident(LPOUTLINETEXTMETRIC)

typedef struct _OVERLAPPED {
  DWORD  Internal;
  DWORD  InternalHigh;
  DWORD  Offset;
  DWORD  OffsetHigh;
  HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct tagPSDA {
    DWORD           lStructSize;
    HWND            hwndOwner;
    HGLOBAL         hDevMode;
    HGLOBAL         hDevNames;
    DWORD           Flags;
    POINT           ptPaperSize;
    RECT            rtMinMargin;
    RECT            rtMargin;
    HINSTANCE       hInstance;
    LPARAM          lCustData;
    LPPAGESETUPHOOK lpfnPageSetupHook;
    LPPAGEPAINTHOOK lpfnPagePaintHook;
    LPCSTR          lpPageSetupTemplateName;
    HGLOBAL         hPageSetupTemplate;
} PAGESETUPDLGA, *LPPAGESETUPDLGA;

typedef struct tagPSDW {
    DWORD           lStructSize;
    HWND            hwndOwner;
    HGLOBAL         hDevMode;
    HGLOBAL         hDevNames;
    DWORD           Flags;
    POINT           ptPaperSize;
    RECT            rtMinMargin;
    RECT            rtMargin;
    HINSTANCE       hInstance;
    LPARAM          lCustData;
    LPPAGESETUPHOOK lpfnPageSetupHook;
    LPPAGEPAINTHOOK lpfnPagePaintHook;
    LPCWSTR         lpPageSetupTemplateName;
    HGLOBAL         hPageSetupTemplate;
} PAGESETUPDLGW, *LPPAGESETUPDLGW;

typedef_tident(PAGESETUPDLG)
typedef_tident(LPPAGESETUPDLG)

typedef struct tagPAINTSTRUCT {
  HDC  hdc;
  WINBOOL fErase;
  RECT rcPaint;
  WINBOOL fRestore;
  WINBOOL fIncUpdate;
  BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct _paraformat {
  UINT cbSize;
  DWORD dwMask;
  WORD  wNumbering;
  WORD  wReserved;
  LONG  dxStartIndent;
  LONG  dxRightIndent;
  LONG  dxOffset;
  WORD  wAlignment;
  SHORT cTabCount;
  LONG  rgxTabs[MAX_TAB_STOPS];
} PARAFORMAT;

typedef struct _PERF_COUNTER_BLOCK {
  DWORD ByteLength;
} PERF_COUNTER_BLOCK;

typedef struct _PERF_COUNTER_DEFINITION {
  DWORD  ByteLength;
  DWORD  CounterNameTitleIndex;
  LPWSTR CounterNameTitle;
  DWORD  CounterHelpTitleIndex;
  LPWSTR CounterHelpTitle;
  DWORD  DefaultScale;
  DWORD  DetailLevel;
  DWORD  CounterType;
  DWORD  CounterSize;
  DWORD  CounterOffset;
} PERF_COUNTER_DEFINITION;

typedef struct _PERF_DATA_BLOCK {
  WCHAR         Signature[4];
  DWORD         LittleEndian;
  DWORD         Version;
  DWORD         Revision;
  DWORD         TotalByteLength;
  DWORD         HeaderLength;
  DWORD         NumObjectTypes;
  DWORD         DefaultObject;
  SYSTEMTIME    SystemTime;
  LARGE_INTEGER PerfTime;
  LARGE_INTEGER PerfFreq;
  LARGE_INTEGER PerfTime100nSec;
  DWORD         SystemNameLength;
  DWORD         SystemNameOffset;
} PERF_DATA_BLOCK;

typedef struct _PERF_INSTANCE_DEFINITION {
  DWORD ByteLength;
  DWORD ParentObjectTitleIndex;
  DWORD ParentObjectInstance;
  DWORD UniqueID;
  DWORD NameOffset;
  DWORD NameLength;
} PERF_INSTANCE_DEFINITION;

typedef struct _PERF_OBJECT_TYPE {
  DWORD  TotalByteLength;
  DWORD  DefinitionLength;
  DWORD  HeaderLength;
  DWORD  ObjectNameTitleIndex;
  LPWSTR ObjectNameTitle;
  DWORD  ObjectHelpTitleIndex;
  LPWSTR ObjectHelpTitle;
  DWORD  DetailLevel;
  DWORD  NumCounters;
  DWORD  DefaultCounter;
  DWORD  NumInstances;
  DWORD  CodePage;
  LARGE_INTEGER PerfTime;
  LARGE_INTEGER PerfFreq;
} PERF_OBJECT_TYPE;

typedef struct _POLYTEXTA {
  int     x;
  int     y;
  UINT    n;
  LPCSTR  lpstr;
  UINT    uiFlags;
  RECT    rcl;
  int     *pdx;
} POLYTEXTA, *LPPOLYTEXTA;

typedef struct _POLYTEXTW {
  int     x;
  int     y;
  UINT    n;
  LPCWSTR lpstr;
  UINT    uiFlags;
  RECT    rcl;
  int     *pdx;
} POLYTEXTW, *LPPOLYTEXTW;

typedef_tident(POLYTEXT)
typedef_tident(LPPOLYTEXT)

typedef struct _PORT_INFO_1A {
  LPSTR  pName;
} PORT_INFO_1A;

typedef struct _PORT_INFO_1W {
  LPWSTR pName;
} PORT_INFO_1W;

typedef_tident(PORT_INFO_1)

typedef struct _PORT_INFO_2A {
  LPSTR  pPortName;
  LPSTR  pMonitorName;
  LPSTR  pDescription;
  DWORD  fPortType;
  DWORD  Reserved;
} PORT_INFO_2A;

typedef struct _PORT_INFO_2W {
  LPWSTR pPortName;
  LPWSTR pMonitorName;
  LPWSTR pDescription;
  DWORD  fPortType;
  DWORD  Reserved;
} PORT_INFO_2W;

typedef_tident(PORT_INFO_2)

typedef struct _PREVENT_MEDIA_REMOVAL {
  BOOLEAN PreventMediaRemoval;
} PREVENT_MEDIA_REMOVAL ;


#include <pshpack1.h>

typedef struct tagPDA {
  DWORD     lStructSize;
  HWND      hwndOwner;
  HANDLE    hDevMode;
  HANDLE    hDevNames;
  HDC       hDC;
  DWORD     Flags;
  WORD      nFromPage;
  WORD      nToPage;
  WORD      nMinPage;
  WORD      nMaxPage;
  WORD      nCopies;
  HINSTANCE hInstance;
  DWORD     lCustData;
  LPPRINTHOOKPROC lpfnPrintHook;
  LPSETUPHOOKPROC lpfnSetupHook;
  LPCSTR    lpPrintTemplateName;
  LPCSTR    lpSetupTemplateName;
  HANDLE    hPrintTemplate;
  HANDLE    hSetupTemplate;
} PRINTDLGA, *LPPRINTDLGA;

typedef struct tagPDW {
  DWORD     lStructSize;
  HWND      hwndOwner;
  HANDLE    hDevMode;
  HANDLE    hDevNames;
  HDC       hDC;
  DWORD     Flags;
  WORD      nFromPage;
  WORD      nToPage;
  WORD      nMinPage;
  WORD      nMaxPage;
  WORD      nCopies;
  HINSTANCE hInstance;
  DWORD     lCustData;
  LPPRINTHOOKPROC lpfnPrintHook;
  LPSETUPHOOKPROC lpfnSetupHook;
  LPCWSTR   lpPrintTemplateName;
  LPCWSTR   lpSetupTemplateName;
  HANDLE    hPrintTemplate;
  HANDLE    hSetupTemplate;
} PRINTDLGW, *LPPRINTDLGW;

#include <poppack.h>


typedef_tident(PRINTDLG)
typedef_tident(LPPRINTDLG)

typedef struct _PRINTER_DEFAULTSA
{
  LPSTR       pDatatype;
  LPDEVMODEA  pDevMode;
  ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSA, *PPRINTER_DEFAULTSA, *LPPRINTER_DEFAULTSA;

typedef struct _PRINTER_DEFAULTSW
{
  LPWSTR      pDatatype;
  LPDEVMODEW  pDevMode;
  ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSW, *PPRINTER_DEFAULTSW, *LPPRINTER_DEFAULTSW;

typedef_tident(PRINTER_DEFAULTS)
typedef_tident(PPRINTER_DEFAULTS)
typedef_tident(LPPRINTER_DEFAULTS)

typedef struct _PRINTER_INFO_1A {
  DWORD  Flags;
  LPSTR  pDescription;
  LPSTR  pName;
  LPSTR  pComment;
} PRINTER_INFO_1A, *PPRINTER_INFO_1A, *LPPRINTER_INFO_1A;

typedef struct _PRINTER_INFO_1W {
  DWORD  Flags;
  LPWSTR pDescription;
  LPWSTR pName;
  LPWSTR pComment;
} PRINTER_INFO_1W, *PPRINTER_INFO_1W, *LPPRINTER_INFO_1W;

typedef_tident(PRINTER_INFO_1)
typedef_tident(PPRINTER_INFO_1)
typedef_tident(LPPRINTER_INFO_1)

#if 0
typedef struct _PRINTER_INFO_2A {
  LPSTR      pServerName;
  LPSTR      pPrinterName;
  LPSTR      pShareName;
  LPSTR      pPortName;
  LPSTR      pDriverName;
  LPSTR      pComment;
  LPSTR      pLocation;
  LPDEVMODEA pDevMode;
  LPSTR      pSepFile;
  LPSTR      pPrintProcessor;
  LPSTR      pDatatype;
  LPSTR      pParameters;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD      Attributes;
  DWORD      Priority;
  DWORD      DefaultPriority;
  DWORD      StartTime;
  DWORD      UntilTime;
  DWORD      Status;
  DWORD      cJobs;
  DWORD      AveragePPM;
} PRINTER_INFO_2A;

typedef struct _PRINTER_INFO_2W {
  LPWSTR     pServerName;
  LPWSTR     pPrinterName;
  LPWSTR     pShareName;
  LPWSTR     pPortName;
  LPWSTR     pDriverName;
  LPWSTR     pComment;
  LPWSTR     pLocation;
  LPDEVMODEW pDevMode;
  LPWSTR     pSepFile;
  LPWSTR     pPrintProcessor;
  LPWSTR     pDatatype;
  LPWSTR     pParameters;
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
  DWORD      Attributes;
  DWORD      Priority;
  DWORD      DefaultPriority;
  DWORD      StartTime;
  DWORD      UntilTime;
  DWORD      Status;
  DWORD      cJobs;
  DWORD      AveragePPM;
} PRINTER_INFO_2W;

typedef_tident(PRINTER_INFO_2)

typedef struct _PRINTER_INFO_3 {
  PSECURITY_DESCRIPTOR pSecurityDescriptor;
} PRINTER_INFO_3;
#endif

typedef struct _PRINTER_INFO_4A {
  LPSTR  pPrinterName;
  LPSTR  pServerName;
  DWORD  Attributes;
} PRINTER_INFO_4A;

typedef struct _PRINTER_INFO_4W {
  LPWSTR pPrinterName;
  LPWSTR pServerName;
  DWORD  Attributes;
} PRINTER_INFO_4W;

typedef_tident(PRINTER_INFO_4)

typedef struct _PRINTER_INFO_5A {
  LPSTR     pPrinterName;
  LPSTR     pPortName;
  DWORD     Attributes;
  DWORD     DeviceNotSelectedTimeout;
  DWORD     TransmissionRetryTimeout;
} PRINTER_INFO_5A;

typedef struct _PRINTER_INFO_5W {
  LPWSTR    pPrinterName;
  LPWSTR    pPortName;
  DWORD     Attributes;
  DWORD     DeviceNotSelectedTimeout;
  DWORD     TransmissionRetryTimeout;
} PRINTER_INFO_5W;

typedef_tident(PRINTER_INFO_5)

typedef struct _PRINTER_NOTIFY_INFO_DATA {
  WORD   Type;
  WORD   Field;
  DWORD  Reserved;
  DWORD  Id;
  union {
    DWORD  adwData[2];
    struct {
      DWORD  cbBuf;
      LPVOID pBuf;
    } Data;
  } NotifyData;
} PRINTER_NOTIFY_INFO_DATA;

typedef struct _PRINTER_NOTIFY_INFO {
  DWORD  Version;
  DWORD  Flags;
  DWORD  Count;
  PRINTER_NOTIFY_INFO_DATA  aData[1];
} PRINTER_NOTIFY_INFO;

typedef struct _PRINTER_NOTIFY_OPTIONS_TYPE {
  WORD   Type;
  WORD   Reserved0;
  DWORD  Reserved1;
  DWORD  Reserved2;
  DWORD  Count;
  PWORD  pFields;
} PRINTER_NOTIFY_OPTIONS_TYPE, *PPRINTER_NOTIFY_OPTIONS_TYPE;

typedef struct _PRINTER_NOTIFY_OPTIONS {
  DWORD  Version;
  DWORD  Flags;
  DWORD  Count;
  PPRINTER_NOTIFY_OPTIONS_TYPE  pTypes;
} PRINTER_NOTIFY_OPTIONS;

typedef struct _PRINTPROCESSOR_INFO_1A {
  LPSTR  pName;
} PRINTPROCESSOR_INFO_1A;

typedef struct _PRINTPROCESSOR_INFO_1W {
  LPWSTR pName;
} PRINTPROCESSOR_INFO_1W;

typedef_tident(PRINTPROCESSOR_INFO_1)

typedef struct _PROCESS_HEAP_ENTRY {
  PVOID lpData;
  DWORD cbData;
  BYTE cbOverhead;
  BYTE iRegionIndex;
  WORD wFlags;
  DWORD dwCommittedSize;
  DWORD dwUnCommittedSize;
  LPVOID lpFirstBlock;
  LPVOID lpLastBlock;
  HANDLE hMem;
} PROCESS_HEAPENTRY, *LPPROCESS_HEAP_ENTRY;

typedef struct _PROCESS_INFORMATION {
  HANDLE hProcess;
  HANDLE hThread;
  DWORD dwProcessId;
  DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef UINT (CALLBACK *LPFNPSPCALLBACK) (HWND, UINT, LPVOID);

typedef struct _PROPSHEETPAGEA {
  DWORD     dwSize;
  DWORD     dwFlags;
  HINSTANCE hInstance;
  union {
    LPCSTR         pszTemplate;
    LPCDLGTEMPLATE pResource;
  } u1;
  union {
    HICON   hIcon;
    LPCSTR  pszIcon;
  } u2;
  LPCSTR  pszTitle;
  DLGPROC pfnDlgProc;
  LPARAM  lParam;
  LPFNPSPCALLBACK pfnCallback;
  UINT   * pcRefParent;
} PROPSHEETPAGEA, *LPPROPSHEETPAGEA;
typedef const PROPSHEETPAGEA* LPCPROPSHEETPAGEA;

typedef struct _PROPSHEETPAGEW {
  DWORD     dwSize;
  DWORD     dwFlags;
  HINSTANCE hInstance;
  union {
    LPCWSTR        pszTemplate;
    LPCDLGTEMPLATE pResource;
  } u1;
  union {
    HICON   hIcon;
    LPCWSTR pszIcon;
  } u2;
  LPCWSTR pszTitle;
  DLGPROC pfnDlgProc;
  LPARAM  lParam;
  LPFNPSPCALLBACK pfnCallback;
  UINT   * pcRefParent;
} PROPSHEETPAGEW, *LPPROPSHEETPAGEW;
typedef const PROPSHEETPAGEW* LPCPROPSHEETPAGEW;

typedef_tident(PROPSHEETPAGE)
typedef_tident(LPPROPSHEETPAGE)
typedef_tident(LPCPROPSHEETPAGE)

typedef struct _PSP *HPROPSHEETPAGE;

typedef struct _PROPSHEETHEADERA {
  DWORD      dwSize;
  DWORD      dwFlags;
  HWND       hwndParent;
  HINSTANCE  hInstance;
  union {
    HICON   hIcon;
    LPCSTR  pszIcon;
  } u1;
  LPCSTR     pszCaption;
  UINT       nPages;
  union {
    UINT    nStartPage;
    LPCSTR  pStartPage;
  } u2;
  union {
    LPCPROPSHEETPAGEA  ppsp;
    HPROPSHEETPAGE    *phpage;
  } u3;
  PFNPROPSHEETCALLBACK pfnCallback;
} PROPSHEETHEADERA, *LPPROPSHEETHEADERA;
typedef const PROPSHEETHEADERA *LPCPROPSHEETHEADERA;

typedef struct _PROPSHEETHEADERW {
  DWORD      dwSize;
  DWORD      dwFlags;
  HWND       hwndParent;
  HINSTANCE  hInstance;
  union {
    HICON   hIcon;
    LPCWSTR pszIcon;
  } u1;
  LPCWSTR    pszCaption;
  UINT       nPages;
  union {
    UINT    nStartPage;
    LPCWSTR pStartPage;
  } u2;
  union {
    LPCPROPSHEETPAGEW  ppsp;
    HPROPSHEETPAGE    *phpage;
  } u3;
  PFNPROPSHEETCALLBACK pfnCallback;
} PROPSHEETHEADERW, *LPPROPSHEETHEADERW;
typedef const PROPSHEETHEADERW *LPCPROPSHEETHEADERW;

typedef_tident(PROPSHEETHEADER)
typedef_tident(LPPROPSHEETHEADER)
typedef_tident(LPCPROPSHEETHEADER)

/* PropertySheet callbacks */
typedef WINBOOL (CALLBACK *LPFNADDPROPSHEETPAGE) (HPROPSHEETPAGE, LPARAM);
typedef WINBOOL (CALLBACK *LPFNADDPROPSHEETPAGES) (LPVOID,
						   LPFNADDPROPSHEETPAGE,
						   LPARAM);

typedef  struct _PROTOCOL_INFOA {
  DWORD  dwServiceFlags;
  INT  iAddressFamily;
  INT  iMaxSockAddr;
  INT  iMinSockAddr;
  INT  iSocketType;
  INT  iProtocol;
  DWORD  dwMessageSize;
  LPSTR  lpProtocol;
} PROTOCOL_INFOA;

typedef  struct _PROTOCOL_INFOW {
  DWORD  dwServiceFlags;
  INT  iAddressFamily;
  INT  iMaxSockAddr;
  INT  iMinSockAddr;
  INT  iSocketType;
  INT  iProtocol;
  DWORD  dwMessageSize;
  LPWSTR  lpProtocol;
} PROTOCOL_INFOW;

typedef_tident(PROTOCOL_INFO)

typedef struct _PROVIDOR_INFO_1A {
  LPSTR  pName;
  LPSTR  pEnvironment ;
  LPSTR  pDLLName ;
} PROVIDOR_INFO_1A;

typedef struct _PROVIDOR_INFO_1W {
  LPWSTR pName;
  LPWSTR pEnvironment ;
  LPWSTR pDLLName ;
} PROVIDOR_INFO_1W;

typedef_tident(PROVIDOR_INFO_1)

typedef struct _PSHNOTIFY {
  NMHDR hdr;
  LPARAM lParam;
} PSHNOTIFY,   *LPPSHNOTIFY;

typedef struct _punctuationA {
  UINT   iSize;
  LPSTR  szPunctuation;
} PUNCTUATIONA;

typedef struct _punctuationW {
  UINT   iSize;
  LPWSTR szPunctuation;
} PUNCTUATIONW;

typedef_tident(PUNCTUATION)

typedef struct _QUERY_SERVICE_CONFIGA {
  DWORD dwServiceType;
  DWORD dwStartType;
  DWORD dwErrorControl;
  LPSTR lpBinaryPathName;
  LPSTR lpLoadOrderGroup;
  DWORD dwTagId;
  LPSTR lpDependencies;
  LPSTR lpServiceStartName;
  LPSTR lpDisplayName;
} QUERY_SERVICE_CONFIGA, *LPQUERY_SERVICE_CONFIGA;

typedef struct _QUERY_SERVICE_CONFIGW {
  DWORD dwServiceType;
  DWORD dwStartType;
  DWORD dwErrorControl;
  LPWSTR lpBinaryPathName;
  LPWSTR lpLoadOrderGroup;
  DWORD dwTagId;
  LPWSTR lpDependencies;
  LPWSTR lpServiceStartName;
  LPWSTR lpDisplayName;
} QUERY_SERVICE_CONFIGW, *LPQUERY_SERVICE_CONFIGW;

typedef_tident(QUERY_SERVICE_CONFIG)
typedef_tident(LPQUERY_SERVICE_CONFIG)

typedef struct _QUERY_SERVICE_LOCK_STATUSA {
  DWORD fIsLocked;
  LPSTR lpLockOwner;
  DWORD dwLockDuration;
} QUERY_SERVICE_LOCK_STATUSA, *LPQUERY_SERVICE_LOCK_STATUSA;

typedef struct _QUERY_SERVICE_LOCK_STATUSW {
  DWORD fIsLocked;
  LPWSTR lpLockOwner;
  DWORD dwLockDuration;
} QUERY_SERVICE_LOCK_STATUSW, *LPQUERY_SERVICE_LOCK_STATUSW;

typedef_tident(QUERY_SERVICE_LOCK_STATUS)
typedef_tident(LPQUERY_SERVICE_LOCK_STATUS)

typedef  struct  _RASAMBA {
  DWORD    dwSize;
  DWORD    dwError;
  CHAR     szNetBiosError[ NETBIOS_NAME_LEN + 1 ];
  BYTE     bLana;
} RASAMBA;

typedef  struct  _RASAMBW {
  DWORD    dwSize;
  DWORD    dwError;
  WCHAR    szNetBiosError[ NETBIOS_NAME_LEN + 1 ];
  BYTE     bLana;
} RASAMBW;

typedef_tident(RASAMB)

typedef struct _RASCONNA {
  DWORD     dwSize;
  HRASCONN  hrasconn;
  CHAR      szEntryName[RAS_MaxEntryName + 1];

  /* WINVER >= 0x400 */
  CHAR      szDeviceType[ RAS_MaxDeviceType + 1 ];
  CHAR      szDeviceName[ RAS_MaxDeviceName + 1 ];

  /* WINVER >= 0x401 */
  CHAR      szPhonebook[ MAX_PATH ];
  DWORD     dwSubEntry;

  /* WINVER >= 0x500 */
  GUID      guidEntry;

  /* WINVER >= 0x501 */
  DWORD     dwSessionId;
  DWORD     dwFlags;
  LUID      luid;
} RASCONNA ;

typedef struct _RASCONNW {
  DWORD     dwSize;
  HRASCONN  hrasconn;
  WCHAR     szEntryName[RAS_MaxEntryName + 1];

  /* WINVER >= 0x400 */
  WCHAR     szDeviceType[ RAS_MaxDeviceType + 1 ];
  WCHAR     szDeviceName[ RAS_MaxDeviceName + 1 ];

  /* WINVER >= 0x401 */
  WCHAR     szPhonebook[ MAX_PATH ];
  DWORD     dwSubEntry;

  /* WINVER >= 0x500 */
  GUID      guidEntry;

  /* WINVER >= 0x501 */
  DWORD     dwSessionId;
  DWORD     dwFlags;
  LUID      luid;
} RASCONNW ;

typedef_tident(RASCONN)

typedef struct _RASCONNSTATUSA {
  DWORD         dwSize;
  RASCONNSTATE  rasconnstate;
  DWORD         dwError;
  CHAR          szDeviceType[RAS_MaxDeviceType + 1];
  CHAR          szDeviceName[RAS_MaxDeviceName + 1];
} RASCONNSTATUSA;

typedef struct _RASCONNSTATUSW {
  DWORD         dwSize;
  RASCONNSTATE  rasconnstate;
  DWORD         dwError;
  WCHAR         szDeviceType[RAS_MaxDeviceType + 1];
  WCHAR         szDeviceName[RAS_MaxDeviceName + 1];
} RASCONNSTATUSW;

typedef_tident(RASCONNSTATUS)

typedef  struct  _RASDIALEXTENSIONS {
  DWORD    dwSize;
  DWORD    dwfOptions;
  HWND     hwndParent;
  DWORD    reserved;
} RASDIALEXTENSIONS;

typedef struct _RASDIALPARAMSA {
  DWORD  dwSize;
  CHAR   szEntryName[RAS_MaxEntryName + 1];
  CHAR   szPhoneNumber[RAS_MaxPhoneNumber + 1];
  CHAR   szCallbackNumber[RAS_MaxCallbackNumber + 1];
  CHAR   szUserName[UNLEN + 1];
  CHAR   szPassword[PWLEN + 1];
  CHAR   szDomain[DNLEN + 1] ;
} RASDIALPARAMSA;

typedef struct _RASDIALPARAMSW {
  DWORD  dwSize;
  WCHAR  szEntryName[RAS_MaxEntryName + 1];
  WCHAR  szPhoneNumber[RAS_MaxPhoneNumber + 1];
  WCHAR  szCallbackNumber[RAS_MaxCallbackNumber + 1];
  WCHAR  szUserName[UNLEN + 1];
  WCHAR  szPassword[PWLEN + 1];
  WCHAR  szDomain[DNLEN + 1] ;
} RASDIALPARAMSW;

typedef_tident(RASDIALPARAMS)

typedef struct _RASENTRYNAMEA {
  DWORD  dwSize;
  CHAR   szEntryName[RAS_MaxEntryName + 1];
}RASENTRYNAMEA;

typedef struct _RASENTRYNAMEW {
  DWORD  dwSize;
  WCHAR  szEntryName[RAS_MaxEntryName + 1];
}RASENTRYNAMEW;

typedef_tident(RASENTRYNAME)

typedef  struct  _RASPPPIPA {
  DWORD    dwSize;
  DWORD    dwError;
  CHAR     szIpAddress[ RAS_MaxIpAddress + 1 ];
} RASPPPIPA;

typedef  struct  _RASPPPIPW {
  DWORD    dwSize;
  DWORD    dwError;
  WCHAR    szIpAddress[ RAS_MaxIpAddress + 1 ];
} RASPPPIPW;

typedef_tident(RASPPPIP)

typedef  struct  _RASPPPIPXA {
  DWORD    dwSize;
  DWORD    dwError;
  CHAR     szIpxAddress[ RAS_MaxIpxAddress + 1 ];
} RASPPPIPXA;

typedef  struct  _RASPPPIPXW {
  DWORD    dwSize;
  DWORD    dwError;
  WCHAR    szIpxAddress[ RAS_MaxIpxAddress + 1 ];
} RASPPPIPXW;

typedef_tident(RASPPPIPX)

typedef  struct  _RASPPPNBFA {
  DWORD    dwSize;
  DWORD    dwError;
  DWORD    dwNetBiosError;
  CHAR     szNetBiosError[ NETBIOS_NAME_LEN + 1 ];
  CHAR     szWorkstationName[ NETBIOS_NAME_LEN + 1 ];
  BYTE     bLana;
} RASPPPNBFA;

typedef  struct  _RASPPPNBFW {
  DWORD    dwSize;
  DWORD    dwError;
  DWORD    dwNetBiosError;
  WCHAR    szNetBiosError[ NETBIOS_NAME_LEN + 1 ];
  WCHAR    szWorkstationName[ NETBIOS_NAME_LEN + 1 ];
  BYTE     bLana;
} RASPPPNBFW;

typedef_tident(RASPPPNBF)

typedef struct _RASTERIZER_STATUS {
  short nSize;
  short wFlags;
  short nLanguageID;
} RASTERIZER_STATUS, *LPRASTERIZER_STATUS;

typedef struct _REASSIGN_BLOCKS {
  WORD   Reserved;
  WORD   Count;
  DWORD BlockNumber[1];
} REASSIGN_BLOCKS ;

typedef struct _REMOTE_NAME_INFOA {
  LPSTR   lpUniversalName;
  LPSTR   lpConnectionName;
  LPSTR   lpRemainingPath;
} REMOTE_NAME_INFOA;

typedef struct _REMOTE_NAME_INFOW {
  LPWSTR  lpUniversalName;
  LPWSTR  lpConnectionName;
  LPWSTR  lpRemainingPath;
} REMOTE_NAME_INFOW;

typedef_tident(REMOTE_NAME_INFO)

/*
 TODO: OLE
typedef struct _reobject {
  DWORD  cbStruct;
  LONG   cp;
  CLSID  clsid;
  LPOLEOBJECT      poleobj;
  LPSTORAGE        pstg;
  LPOLECLIENTSITE  polesite;
  SIZEL  sizel;
  DWORD  dvaspect;
  DWORD  dwFlags;
  DWORD  dwUser;
} REOBJECT;
*/

typedef struct _repastespecial {
  DWORD  dwAspect;
  DWORD  dwParam;
} REPASTESPECIAL;

typedef struct _reqresize {
  NMHDR nmhdr;
  RECT rc;
} REQRESIZE;

typedef struct _RGNDATAHEADER {
  DWORD dwSize;
  DWORD iType;
  DWORD nCount;
  DWORD nRgnSize;
  RECT  rcBound;
} RGNDATAHEADER;

typedef struct _RGNDATA {
  RGNDATAHEADER rdh;
  char          Buffer[1];
} RGNDATA, *PRGNDATA, *LPRGNDATA;

typedef struct tagSCROLLINFO {
  UINT cbSize;
  UINT fMask;
  int  nMin;
  int  nMax;
  UINT nPage;
  int  nPos;
  int  nTrackPos;
}   SCROLLINFO, *LPSCROLLINFO;
typedef SCROLLINFO const *LPCSCROLLINFO;


typedef struct _selchange {
  NMHDR nmhdr;
  CHARRANGE chrg;
  WORD seltyp;
} SELCHANGE;

typedef struct tagSERIALKEYS {
  DWORD cbSize;
  DWORD dwFlags;
  LPSTR lpszActivePort;
  LPSTR lpszPort;
  DWORD iBaudRate;
  DWORD iPortState;
} SERIALKEYS,  * LPSERIALKEYS;

typedef struct _SERVICE_TABLE_ENTRYA {
  LPSTR lpServiceName;
  LPSERVICE_MAIN_FUNCTIONA lpServiceProc;
} SERVICE_TABLE_ENTRYA, *LPSERVICE_TABLE_ENTRYA;

typedef struct _SERVICE_TABLE_ENTRYW {
  LPWSTR lpServiceName;
  LPSERVICE_MAIN_FUNCTIONW lpServiceProc;
} SERVICE_TABLE_ENTRYW, *LPSERVICE_TABLE_ENTRYW;

typedef_tident(SERVICE_TABLE_ENTRY)
typedef_tident(LPSERVICE_TABLE_ENTRY)

typedef struct _SERVICE_TYPE_VALUE_ABSA {
  DWORD   dwNameSpace;
  DWORD   dwValueType;
  DWORD   dwValueSize;
  LPSTR   lpValueName;
  PVOID   lpValue;
} SERVICE_TYPE_VALUE_ABSA;

typedef struct _SERVICE_TYPE_VALUE_ABSW {
  DWORD   dwNameSpace;
  DWORD   dwValueType;
  DWORD   dwValueSize;
  LPWSTR  lpValueName;
  PVOID   lpValue;
} SERVICE_TYPE_VALUE_ABSW;

typedef_tident(SERVICE_TYPE_VALUE_ABS)

typedef struct _SERVICE_TYPE_INFO_ABSA {
  LPSTR                   lpTypeName;
  DWORD                   dwValueCount;
  SERVICE_TYPE_VALUE_ABSA Values[1];
} SERVICE_TYPE_INFO_ABSA;

typedef struct _SERVICE_TYPE_INFO_ABSW {
  LPWSTR                  lpTypeName;
  DWORD                   dwValueCount;
  SERVICE_TYPE_VALUE_ABSW Values[1];
} SERVICE_TYPE_INFO_ABSW;

typedef_tident(SERVICE_TYPE_INFO_ABS)

typedef struct _SESSION_BUFFER {
  UCHAR lsn;
  UCHAR state;
  UCHAR local_name[NCBNAMSZ];
  UCHAR remote_name[NCBNAMSZ];
  UCHAR rcvs_outstanding;
  UCHAR sends_outstanding;
} SESSION_BUFFER;

typedef struct _SESSION_HEADER {
  UCHAR sess_name;
  UCHAR num_sess;
  UCHAR rcv_dg_outstanding;
  UCHAR rcv_any_outstanding;
} SESSION_HEADER;

typedef enum tagSHCONTF { 
  SHCONTF_FOLDERS = 32,         
  SHCONTF_NONFOLDERS = 64,      
  SHCONTF_INCLUDEHIDDEN = 128,  
  SHCONTF_INIT_ON_FIRST_NEXT = 256,
  SHCONTF_NETPRINTERSRCH = 512,
  SHCONTF_SHAREABLE = 1024,
  SHCONTF_STORAGE = 2048
} SHCONTF; 
 
typedef struct _SHFILEINFO {
  HICON hIcon;
  int   iIcon;
  DWORD dwAttributes;
  char  szDisplayName[MAX_PATH];
  char  szTypeName[80];
} SHFILEINFO; 

typedef WORD FILEOP_FLAGS;

typedef struct _SHFILEOPSTRUCTA {
  HWND         hwnd;
  UINT         wFunc;
  LPCSTR       pFrom;
  LPCSTR       pTo;
  FILEOP_FLAGS fFlags;
  WINBOOL      fAnyOperationsAborted;
  LPVOID       hNameMappings;
  LPCSTR       lpszProgressTitle;
} SHFILEOPSTRUCTA, *LPSHFILEOPSTRUCTA;

typedef struct _SHFILEOPSTRUCTW {
  HWND         hwnd;
  UINT         wFunc;
  LPCWSTR      pFrom;
  LPCWSTR      pTo;
  FILEOP_FLAGS fFlags;
  WINBOOL      fAnyOperationsAborted;
  LPVOID       hNameMappings;
  LPCWSTR      lpszProgressTitle;
} SHFILEOPSTRUCTW, *LPSHFILEOPSTRUCTW;

typedef_tident(SHFILEOPSTRUCT)
typedef_tident(LPSHFILEOPSTRUCT)

typedef enum tagSHGDN {
  SHGDN_NORMAL = 0,
  SHGDN_INFOLDER = 1,
  SHGDN_FORPARSING = 0x8000,
} SHGNO; 

typedef struct _SHNAMEMAPPINGA {
  LPSTR  pszOldPath;
  LPSTR  pszNewPath;
  int    cchOldPath;
  int    cchNewPath;
} SHNAMEMAPPINGA, *LPSHNAMEMAPPINGA;

typedef struct _SHNAMEMAPPINGW {
  LPWSTR pszOldPath;
  LPWSTR pszNewPath;
  int    cchOldPath;
  int    cchNewPath;
} SHNAMEMAPPINGW, *LPSHNAMEMAPPINGW;

typedef_tident(SHNAMEMAPPING)
typedef_tident(LPSHNAMEMAPPING)

typedef struct tagSOUNDSENTRYA {
  UINT   cbSize;
  DWORD  dwFlags; 
  DWORD  iFSTextEffect; 
  DWORD  iFSTextEffectMSec; 
  DWORD  iFSTextEffectColorBits; 
  DWORD  iFSGrafEffect; 
  DWORD  iFSGrafEffectMSec; 
  DWORD  iFSGrafEffectColor; 
  DWORD  iWindowsEffect; 
  DWORD  iWindowsEffectMSec; 
  LPSTR  lpszWindowsEffectDLL; 
  DWORD  iWindowsEffectOrdinal; 
} SOUNDSENTRYA, *LPSOUNDSENTRYA; 

typedef struct tagSOUNDSENTRYW {
  UINT   cbSize;
  DWORD  dwFlags; 
  DWORD  iFSTextEffect; 
  DWORD  iFSTextEffectMSec; 
  DWORD  iFSTextEffectColorBits; 
  DWORD  iFSGrafEffect; 
  DWORD  iFSGrafEffectMSec; 
  DWORD  iFSGrafEffectColor; 
  DWORD  iWindowsEffect; 
  DWORD  iWindowsEffectMSec; 
  LPWSTR lpszWindowsEffectDLL; 
  DWORD  iWindowsEffectOrdinal; 
} SOUNDSENTRYW, *LPSOUNDSENTRYW; 

typedef_tident(SOUNDSENTRY)
typedef_tident(LPSOUNDSENTRY)

typedef struct _STARTUPINFOA {
  DWORD   cb;
  LPSTR   lpReserved;
  LPSTR   lpDesktop;
  LPSTR   lpTitle;
  DWORD   dwX;
  DWORD   dwY;
  DWORD   dwXSize;
  DWORD   dwYSize;
  DWORD   dwXCountChars;
  DWORD   dwYCountChars;
  DWORD   dwFillAttribute;
  DWORD   dwFlags;
  WORD    wShowWindow;
  WORD    cbReserved2;
  LPBYTE  lpReserved2;
  HANDLE  hStdInput;
  HANDLE  hStdOutput;
  HANDLE  hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _STARTUPINFOW {
  DWORD   cb;
  LPWSTR  lpReserved;
  LPWSTR  lpDesktop;
  LPWSTR  lpTitle;
  DWORD   dwX;
  DWORD   dwY;
  DWORD   dwXSize;
  DWORD   dwYSize;
  DWORD   dwXCountChars;
  DWORD   dwYCountChars;
  DWORD   dwFillAttribute;
  DWORD   dwFlags;
  WORD    wShowWindow;
  WORD    cbReserved2;
  LPBYTE  lpReserved2;
  HANDLE  hStdInput;
  HANDLE  hStdOutput;
  HANDLE  hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

typedef_tident(STARTUPINFO)
typedef_tident(LPSTARTUPINFO)

typedef struct tagSTICKYKEYS {
  DWORD cbSize;
  DWORD dwFlags;
} STICKYKEYS, *LPSTICKYKEYS;

typedef struct _STRRET {
  UINT uType;
  union
    {
      LPWSTR pOleStr;
      UINT   uOffset;
      char   cStr[MAX_PATH];
    } DUMMYUNIONNAME;
} STRRET, *LPSTRRET;

typedef struct _tagSTYLEBUF {
  DWORD  dwStyle;
  CHAR  szDescription[32];
} STYLEBUF, *LPSTYLEBUF;

typedef struct tagSTYLESTRUCT {
  DWORD styleOld;
  DWORD styleNew;
} STYLESTRUCT, * LPSTYLESTRUCT;

typedef struct _ACCESS_ALLOWED_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD SidStart;
} ACCESS_ALLOWED_ACE;

typedef ACCESS_ALLOWED_ACE *PACCESS_ALLOWED_ACE;

typedef struct _ACCESS_DENIED_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD SidStart;
} ACCESS_DENIED_ACE;
typedef ACCESS_DENIED_ACE *PACCESS_DENIED_ACE;

typedef struct _SYSTEM_AUDIT_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD SidStart;
} SYSTEM_AUDIT_ACE;
typedef SYSTEM_AUDIT_ACE *PSYSTEM_AUDIT_ACE;

typedef struct _SYSTEM_ALARM_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD SidStart;
} SYSTEM_ALARM_ACE;
typedef SYSTEM_ALARM_ACE *PSYSTEM_ALARM_ACE;

typedef struct _ACCESS_ALLOWED_OBJECT_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD Flags;
 GUID ObjectType;
 GUID InheritedObjectType;
 DWORD SidStart;
} ACCESS_ALLOWED_OBJECT_ACE, *PACCESS_ALLOWED_OBJECT_ACE;

typedef struct _ACCESS_DENIED_OBJECT_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD Flags;
 GUID ObjectType;
 GUID InheritedObjectType;
 DWORD SidStart;
} ACCESS_DENIED_OBJECT_ACE, *PACCESS_DENIED_OBJECT_ACE;

typedef struct _SYSTEM_AUDIT_OBJECT_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD Flags;
 GUID ObjectType;
 GUID InheritedObjectType;
 DWORD SidStart;
} SYSTEM_AUDIT_OBJECT_ACE, *PSYSTEM_AUDIT_OBJECT_ACE;

typedef struct _SYSTEM_ALARM_OBJECT_ACE {
 ACE_HEADER Header;
 ACCESS_MASK Mask;
 DWORD Flags;
 GUID ObjectType;
 GUID InheritedObjectType;
 DWORD SidStart;
} SYSTEM_ALARM_OBJECT_ACE, *PSYSTEM_ALARM_OBJECT_ACE;

typedef struct _SYSTEM_INFO
{
  union
    {
      DWORD dwOemId;
      struct
        {
          WORD wProcessorArchitecture;
          WORD wReserved;
        }
      s;
    }
  u;
  DWORD  dwPageSize;
  LPVOID lpMinimumApplicationAddress;
  LPVOID lpMaximumApplicationAddress;
  DWORD  dwActiveProcessorMask;
  DWORD  dwNumberOfProcessors;
  DWORD  dwProcessorType;
  DWORD  dwAllocationGranularity;
  WORD  wProcessorLevel;
  WORD  wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

typedef struct _SYSTEM_POWER_STATUS {
  BYTE ACLineStatus;
  BYTE  BatteryFlag;
  BYTE  BatteryLifePercent;
  BYTE  Reserved1;
  DWORD  BatteryLifeTime;
  DWORD  BatteryFullLifeTime;
} SYSTEM_POWER_STATUS;
typedef SYSTEM_POWER_STATUS *LPSYSTEM_POWER_STATUS;

typedef struct _TAPE_CREATE_PARTITION {
  ULONG Method;
  ULONG Count;
  ULONG Size;
} TAPE_CREATE_PARTITION, *PTAPE_CREATE_PARTITION;

typedef struct _TAPE_ERASE {
  ULONG Type;
  BOOLEAN Immediate;
} TAPE_ERASE, *PTAPE_ERASE;

typedef struct _TAPE_GET_DRIVE_PARAMETERS {
  BOOLEAN ECC;
  BOOLEAN Compression;
  BOOLEAN DataPadding;
  BOOLEAN ReportSetmarks;
  ULONG DefaultBlockSize;
  ULONG MaximumBlockSize;
  ULONG MinimumBlockSize;
  ULONG MaximumPartitionCount;
  ULONG FeaturesLow;
  ULONG FeaturesHigh;
  ULONG EOTWarningZoneSize;
} TAPE_GET_DRIVE_PARAMETERS, *PTAPE_GET_DRIVE_PARAMETERS;

typedef struct _TAPE_GET_MEDIA_PARAMETERS {
  LARGE_INTEGER Capacity;
  LARGE_INTEGER Remaining;
  ULONG BlockSize;
  ULONG PartitionCount;
  BOOLEAN WriteProtected;
} TAPE_GET_MEDIA_PARAMETERS, *PTAPE_GET_MEDIA_PARAMETERS;

typedef struct _TAPE_GET_POSITION {
  ULONG Type;
  ULONG Partition;
  LARGE_INTEGER Offset;
} TAPE_GET_POSITION, *PTAPE_GET_POSITION;

typedef struct _TAPE_PREPARE {
  ULONG Operation;
  BOOLEAN Immediate;
} TAPE_PREPARE, *PTAPE_PREPARE;

typedef struct _TAPE_SET_DRIVE_PARAMETERS {
  BOOLEAN ECC;
  BOOLEAN Compression;
  BOOLEAN DataPadding;
  BOOLEAN ReportSetmarks;
  ULONG EOTWarningZoneSize;
} TAPE_SET_DRIVE_PARAMETERS, *PTAPE_SET_DRIVE_PARAMETERS;

typedef struct _TAPE_SET_MEDIA_PARAMETERS {
  ULONG BlockSize;
} TAPE_SET_MEDIA_PARAMETERS, *PTAPE_SET_MEDIA_PARAMETERS;

typedef struct _TAPE_SET_POSITION {
  ULONG Method;
  ULONG Partition;
  LARGE_INTEGER Offset;
  BOOLEAN Immediate;
} TAPE_SET_POSITION, *PTAPE_SET_POSITION;

typedef struct _TAPE_WRITE_MARKS {
  ULONG Type;
  ULONG Count;
  BOOLEAN Immediate;
} TAPE_WRITE_MARKS, *PTAPE_WRITE_MARKS;

typedef struct {
  HINSTANCE hInst;
  UINT nID;
} TBADDBITMAP, *LPTBADDBITMAP;

typedef struct _TBBUTTON {
  int iBitmap;
  int idCommand;
  BYTE fsState;
  BYTE fsStyle;
  DWORD dwData;
  int iString;
} TBBUTTON,  * PTBBUTTON,  * LPTBBUTTON;
typedef const TBBUTTON  * LPCTBBUTTON;

typedef struct {
  NMHDR    hdr;
  int      iItem;
  TBBUTTON tbButton;
  int      cchText;
  LPSTR    pszText;
} TBNOTIFYA, *LPTBNOTIFYA;

typedef struct {
  NMHDR    hdr;
  int      iItem;
  TBBUTTON tbButton;
  int      cchText;
  LPWSTR   pszText;
} TBNOTIFYW, *LPTBNOTIFYW;

typedef_tident(TBNOTIFY)
typedef_tident(LPTBNOTIFY)

typedef struct {
  HKEY    hkr;
  LPCSTR  pszSubKey;
  LPCSTR  pszValueName;
} TBSAVEPARAMSA;

typedef struct {
  HKEY    hkr;
  LPCWSTR pszSubKey;
  LPCWSTR pszValueName;
} TBSAVEPARAMSW;

typedef_tident(TBSAVEPARAMS)

typedef struct _TC_HITTESTINFO {
  POINT pt;
  UINT  flags;
} TC_HITTESTINFO;

typedef struct _TC_ITEMA {
  UINT   mask;
  UINT   lpReserved1;
  UINT   lpReserved2;
  LPSTR  pszText;
  int    cchTextMax;
  int    iImage;
  LPARAM lParam;
} TC_ITEMA;

typedef struct _TC_ITEMW {
  UINT   mask;
  UINT   lpReserved1;
  UINT   lpReserved2;
  LPWSTR pszText;
  int    cchTextMax;
  int    iImage;
  LPARAM lParam;
} TC_ITEMW;

typedef_tident(TC_ITEM)

typedef struct _TC_ITEMHEADERA {
  UINT   mask;
  UINT   lpReserved1;
  UINT   lpReserved2;
  LPSTR  pszText;
  int    cchTextMax;
  int    iImage;
} TC_ITEMHEADERA;

typedef struct _TC_ITEMHEADERW {
  UINT   mask;
  UINT   lpReserved1;
  UINT   lpReserved2;
  LPWSTR pszText;
  int    cchTextMax;
  int    iImage;
} TC_ITEMHEADERW;

typedef_tident(TC_ITEMHEADER)

typedef struct _TC_KEYDOWN {
  NMHDR hdr;
  WORD wVKey;
  UINT flags;
} TC_KEYDOWN;

typedef struct _textrangeA {
  CHARRANGE chrg;
  LPSTR     lpstrText;
} TEXTRANGEA;

typedef struct _textrangeW {
  CHARRANGE chrg;
  LPWSTR    lpstrText;
} TEXTRANGEW;

typedef_tident(TEXTRANGE)

typedef struct tagTOGGLEKEYS {
  DWORD cbSize;
  DWORD dwFlags;
} TOGGLEKEYS;

typedef struct {
  UINT      cbSize;
  UINT      uFlags;
  HWND      hwnd;
  UINT      uId;
  RECT      rect;
  HINSTANCE hinst;
  LPSTR     lpszText;
} TOOLINFOA, *PTOOLINFOA, *LPTOOLINFOA;

typedef struct {
  UINT      cbSize;
  UINT      uFlags;
  HWND      hwnd;
  UINT      uId;
  RECT      rect;
  HINSTANCE hinst;
  LPWSTR    lpszText;
} TOOLINFOW, *PTOOLINFOW, *LPTOOLINFOW;

typedef_tident(TOOLINFO)
typedef_tident(PTOOLINFO)
typedef_tident(LPTOOLINFO)

typedef struct {
  NMHDR     hdr;
  LPSTR     lpszText;
  CHAR      szText[80];
  HINSTANCE hinst;
  UINT      uFlags;
} TOOLTIPTEXTA, *LPTOOLTIPTEXTA;

typedef struct {
  NMHDR     hdr;
  LPWSTR    lpszText;
  WCHAR     szText[80];
  HINSTANCE hinst;
  UINT      uFlags;
} TOOLTIPTEXTW, *LPTOOLTIPTEXTW;

typedef_tident(TOOLTIPTEXT)
typedef_tident(LPTOOLTIPTEXT)

typedef struct tagTPMPARAMS {
  UINT cbSize;
  RECT rcExclude;
} TPMPARAMS,   *LPTPMPARAMS;

#if 0 /* RobD - typedef removed due to conflict with mingw headers */
typedef struct _TRANSMIT_FILE_BUFFERS {
  PVOID Head;
  DWORD HeadLength;
  PVOID Tail;
  DWORD TailLength;
} TRANSMIT_FILE_BUFFERS;
#endif

typedef struct _TT_HITTESTINFOA {
  HWND      hwnd;
  POINT     pt;
  TOOLINFOA ti;
} TTHITTESTINFOA, *LPHITTESTINFOA;

typedef struct _TT_HITTESTINFOW {
  HWND      hwnd;
  POINT     pt;
  TOOLINFOW ti;
} TTHITTESTINFOW, *LPHITTESTINFOW;

typedef_tident(TTHITTESTINFO)
typedef_tident(LPHITTESTINFO)

typedef struct tagTTPOLYCURVE {
  WORD    wType;
  WORD    cpfx;
  POINTFX apfx[1];
} TTPOLYCURVE,  * LPTTPOLYCURVE;

typedef struct _TTPOLYGONHEADER {
  DWORD   cb;
  DWORD   dwType;
  POINTFX pfxStart;
} TTPOLYGONHEADER, *PTTPOLYGONHEADER, *LPTTPOLYGONHEADER;

typedef struct _TV_DISPINFOA {
  NMHDR    hdr;
  TV_ITEMA item;
} TV_DISPINFOA;

typedef struct _TV_DISPINFOW {
  NMHDR    hdr;
  TV_ITEMW item;
} TV_DISPINFOW;

typedef_tident(TV_DISPINFO)

typedef struct _TVHITTESTINFO {
  POINT     pt;
  UINT      flags;
  HTREEITEM hItem;
} TV_HITTESTINFO, *LPTV_HITTESTINFO;

typedef struct _TV_INSERTSTRUCTA {
  HTREEITEM hParent;
  HTREEITEM hInsertAfter;
  TV_ITEMA  item;
} TV_INSERTSTRUCTA, *LPTV_INSERTSTRUCTA;

typedef struct _TV_INSERTSTRUCTW {
  HTREEITEM hParent;
  HTREEITEM hInsertAfter;
  TV_ITEMW  item;
} TV_INSERTSTRUCTW, *LPTV_INSERTSTRUCTW;

typedef_tident(TV_INSERTSTRUCT)
typedef_tident(LPTV_INSERTSTRUCT)

typedef struct _TV_KEYDOWN {
  NMHDR hdr;
  WORD  wVKey;
  UINT  flags;
} TV_KEYDOWN;

typedef struct _TV_SORTCB {
  HTREEITEM    hParent;
  PFNTVCOMPARE lpfnCompare;
  LPARAM       lParam;
} TV_SORTCB,   *LPTV_SORTCB;

typedef struct {
  UINT nSec;
  UINT nInc;
} UDACCEL;

typedef struct _UNIVERSAL_NAME_INFOA {
  LPSTR   lpUniversalName;
} UNIVERSAL_NAME_INFOA;

typedef struct _UNIVERSAL_NAME_INFOW {
  LPWSTR  lpUniversalName;
} UNIVERSAL_NAME_INFOW;

typedef_tident(UNIVERSAL_NAME_INFO)

typedef struct tagUSEROBJECTFLAGS {
  WINBOOL fInherit;
  WINBOOL fReserved;
  DWORD dwFlags;
} USEROBJECTFLAGS;

typedef struct _VERIFY_INFORMATION {
  LARGE_INTEGER  StartingOffset;
  DWORD  Length;
} VERIFY_INFORMATION ;

typedef struct _VS_FIXEDFILEINFO {
  DWORD dwSignature;
  DWORD dwStrucVersion;
  DWORD dwFileVersionMS;
  DWORD dwFileVersionLS;
  DWORD dwProductVersionMS;
  DWORD dwProductVersionLS;
  DWORD dwFileFlagsMask;
  DWORD dwFileFlags;
  DWORD dwFileOS;
  DWORD dwFileType;
  DWORD dwFileSubtype;
  DWORD dwFileDateMS;
  DWORD dwFileDateLS;
} VS_FIXEDFILEINFO;

typedef struct _WIN32_FIND_DATAA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  CHAR     cFileName[ MAX_PATH ];
  CHAR     cAlternateFileName[ 14 ];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA, *PWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW {
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  WCHAR    cFileName[ MAX_PATH ];
  WCHAR    cAlternateFileName[ 14 ];
} WIN32_FIND_DATAW, *LPWIN32_FIND_DATAW, *PWIN32_FIND_DATAW;

typedef_tident(WIN32_FIND_DATA)
typedef_tident(PWIN32_FIND_DATA)
typedef_tident(LPWIN32_FIND_DATA)

typedef struct _WIN32_STREAM_ID {
  DWORD dwStreamId;
  DWORD dwStreamAttributes;
  LARGE_INTEGER Size;
  DWORD dwStreamNameSize;
  WCHAR *cStreamName;
} WIN32_STREAM_ID;

typedef struct _WINDOWPLACEMENT {
  UINT  length;
  UINT  flags;
  UINT  showCmd;
  POINT ptMinPosition;
  POINT ptMaxPosition;
  RECT  rcNormalPosition;
} WINDOWPLACEMENT;

typedef struct _WNDCLASSA {
  UINT    style;
  WNDPROC lpfnWndProc;
  int     cbClsExtra;
  int     cbWndExtra;
  HANDLE  hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  LPCSTR  lpszMenuName;
  LPCSTR  lpszClassName;
} WNDCLASSA, *LPWNDCLASSA;

typedef struct _WNDCLASSW {
  UINT    style;
  WNDPROC lpfnWndProc;
  int     cbClsExtra;
  int     cbWndExtra;
  HANDLE  hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  LPCWSTR lpszMenuName;
  LPCWSTR lpszClassName;
} WNDCLASSW, *LPWNDCLASSW;

typedef_tident(WNDCLASS)

typedef struct _WNDCLASSEXA {
  UINT    cbSize;
  UINT    style;
  WNDPROC lpfnWndProc;
  int     cbClsExtra;
  int     cbWndExtra;
  HANDLE  hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  LPCSTR  lpszMenuName;
  LPCSTR  lpszClassName;
  HICON   hIconSm;
} WNDCLASSEXA, *LPWNDCLASSEXA;

typedef struct _WNDCLASSEXW {
  UINT    cbSize;
  UINT    style;
  WNDPROC lpfnWndProc;
  int     cbClsExtra;
  int     cbWndExtra;
  HANDLE  hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  LPCWSTR lpszMenuName;
  LPCWSTR lpszClassName;
  HICON   hIconSm;
} WNDCLASSEXW, *LPWNDCLASSEXW;

typedef_tident(WNDCLASSEX)

typedef struct _CONNECTDLGSTRUCTA {
  DWORD          cbStructure;
  HWND           hwndOwner;
  LPNETRESOURCEA lpConnRes;
  DWORD          dwFlags;
  DWORD          dwDevNum;
} CONNECTDLGSTRUCTA, *LPCONNECTDLGSTRUCTA;

typedef struct _CONNECTDLGSTRUCTW {
  DWORD          cbStructure;
  HWND           hwndOwner;
  LPNETRESOURCEW lpConnRes;
  DWORD          dwFlags;
  DWORD          dwDevNum;
} CONNECTDLGSTRUCTW, *LPCONNECTDLGSTRUCTW;

typedef_tident(CONNECTDLGSTRUCT)
typedef_tident(LPCONNECTDLGSTRUCT)

typedef struct _DISCDLGSTRUCTA {
  DWORD           cbStructure;
  HWND            hwndOwner;
  LPSTR           lpLocalName;
  LPSTR           lpRemoteName;
  DWORD           dwFlags;
} DISCDLGSTRUCTA, *LPDISCDLGSTRUCTA;

typedef struct _DISCDLGSTRUCTW {
  DWORD           cbStructure;
  HWND            hwndOwner;
  LPWSTR          lpLocalName;
  LPWSTR          lpRemoteName;
  DWORD           dwFlags;
} DISCDLGSTRUCTW, *LPDISCDLGSTRUCTW;

typedef_tident(DISCDLGSTRUCT)
typedef_tident(LPDISCDLGSTRUCT)

typedef struct _NETINFOSTRUCT{
    DWORD cbStructure;
    DWORD dwProviderVersion;
    DWORD dwStatus;
    DWORD dwCharacteristics;
    DWORD dwHandle;
    WORD  wNetType;
    DWORD dwPrinters;
    DWORD dwDrives;
} NETINFOSTRUCT, *LPNETINFOSTRUCT;

typedef struct _NETCONNECTINFOSTRUCT{
  DWORD cbStructure;
  DWORD dwFlags;
  DWORD dwSpeed;
  DWORD dwDelay;
  DWORD dwOptDataSize;
} NETCONNECTINFOSTRUCT, *LPNETCONNECTINFOSTRUCT;

typedef int (CALLBACK *ENUMMETAFILEPROC) (HDC, HANDLETABLE,
					  METARECORD, int, LPARAM);
typedef int (CALLBACK *ENHMETAFILEPROC) (HDC, HANDLETABLE,
					 ENHMETARECORD, int, LPARAM);

typedef int (CALLBACK *ENUMFONTSPROCA) (LPLOGFONTA, LPTEXTMETRICA, DWORD, LPARAM);
typedef int (CALLBACK *ENUMFONTSPROCW) (LPLOGFONTW, LPTEXTMETRICW, DWORD, LPARAM);
typedef_tident(ENUMFONTSPROC)
typedef int (CALLBACK *FONTENUMPROCA) (ENUMLOGFONTA *, NEWTEXTMETRICA *,
				       int, LPARAM);
typedef int (CALLBACK *FONTENUMPROCW) (ENUMLOGFONTW *, NEWTEXTMETRICW *,
				       int, LPARAM);
typedef_tident(FONTENUMPROC)
typedef int (CALLBACK *FONTENUMEXPROCA) (ENUMLOGFONTEXA *, NEWTEXTMETRICEXA *,
				         int, LPARAM);
typedef int (CALLBACK *FONTENUMEXPROCW) (ENUMLOGFONTEXW *, NEWTEXTMETRICEXW *,
				         int, LPARAM);
typedef_tident(FONTENUMEXPROC)

typedef VOID (CALLBACK *LPOVERLAPPED_COMPLETION_ROUTINE) (DWORD, DWORD,
							  LPOVERLAPPED);

/*
  Structures for the extensions to OpenGL
  */
typedef struct _POINTFLOAT
{
  FLOAT   x;
  FLOAT   y;
} POINTFLOAT, *PPOINTFLOAT;

typedef struct _GLYPHMETRICSFLOAT
{
  FLOAT       gmfBlackBoxX;
  FLOAT       gmfBlackBoxY;
  POINTFLOAT  gmfptGlyphOrigin;
  FLOAT       gmfCellIncX;
  FLOAT       gmfCellIncY;
} GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, *LPGLYPHMETRICSFLOAT;

typedef struct tagLAYERPLANEDESCRIPTOR
{
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerPlane;
  BYTE  bReserved;
  COLORREF crTransparent;
} LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, *LPLAYERPLANEDESCRIPTOR;

typedef struct tagPIXELFORMATDESCRIPTOR
{
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerType;
  BYTE  bReserved;
  DWORD dwLayerMask;
  DWORD dwVisibleMask;
  DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

typedef struct
{
  LPWSTR    usri2_name;
  LPWSTR    usri2_password;
  DWORD     usri2_password_age;
  DWORD     usri2_priv;
  LPWSTR    usri2_home_dir;
  LPWSTR    usri2_comment;
  DWORD     usri2_flags;
  LPWSTR    usri2_script_path;
  DWORD     usri2_auth_flags;
  LPWSTR    usri2_full_name;
  LPWSTR    usri2_usr_comment;
  LPWSTR    usri2_parms;
  LPWSTR    usri2_workstations;
  DWORD     usri2_last_logon;
  DWORD     usri2_last_logoff;
  DWORD     usri2_acct_expires;
  DWORD     usri2_max_storage;
  DWORD     usri2_units_per_week;
  PBYTE     usri2_logon_hours;
  DWORD     usri2_bad_pw_count;
  DWORD     usri2_num_logons;
  LPWSTR    usri2_logon_server;
  DWORD     usri2_country_code;
  DWORD     usri2_code_page;
} USER_INFO_2, *PUSER_INFO_2, *LPUSER_INFO_2;

typedef struct
{
  LPWSTR    usri0_name;
} USER_INFO_0, *PUSER_INFO_0, *LPUSER_INFO_0;

typedef struct
{
  LPWSTR    usri3_name;
  LPWSTR    usri3_password;
  DWORD     usri3_password_age;
  DWORD     usri3_priv;
  LPWSTR    usri3_home_dir;
  LPWSTR    usri3_comment;
  DWORD     usri3_flags;
  LPWSTR    usri3_script_path;
  DWORD     usri3_auth_flags;
  LPWSTR    usri3_full_name;
  LPWSTR    usri3_usr_comment;
  LPWSTR    usri3_parms;
  LPWSTR    usri3_workstations;
  DWORD     usri3_last_logon;
  DWORD     usri3_last_logoff;
  DWORD     usri3_acct_expires;
  DWORD     usri3_max_storage;
  DWORD     usri3_units_per_week;
  PBYTE     usri3_logon_hours;
  DWORD     usri3_bad_pw_count;
  DWORD     usri3_num_logons;
  LPWSTR    usri3_logon_server;
  DWORD     usri3_country_code;
  DWORD     usri3_code_page;
  DWORD     usri3_user_id;
  DWORD     usri3_primary_group_id;
  LPWSTR    usri3_profile;
  LPWSTR    usri3_home_dir_drive;
  DWORD     usri3_password_expired;
} USER_INFO_3, *PUSER_INFO_3, *LPUSER_INFO_3;

typedef struct
{
  LPWSTR   grpi2_name;
  LPWSTR   grpi2_comment;
  DWORD    grpi2_group_id;
  DWORD    grpi2_attributes;
} GROUP_INFO_2, *PGROUP_INFO_2;

typedef struct
{
  LPWSTR   lgrpi0_name;
} LOCALGROUP_INFO_0, *PLOCALGROUP_INFO_0, *LPLOCALGROUP_INFO_0;

/* PE executable header.  */
/* Windows.h now includes pe.h to avoid conflicts! */

typedef struct _DISPLAY_DEVICE {
  DWORD cb;
  WCHAR DeviceName[32];
  WCHAR DeviceString[128];
  DWORD StateFlags;
  WCHAR DeviceID[128];
  WCHAR DeviceKey[128];
} DISPLAY_DEVICE, *PDISPLAY_DEVICE;

typedef HANDLE HMONITOR;
typedef HANDLE HDEVNOTIFY;

typedef BOOL (CALLBACK *MonitorEnumProc)(
  HMONITOR hMonitor,
  HDC hdcMonitor,
  LPRECT lprcMonitor,
  LPARAM dwData);

typedef MonitorEnumProc MONITORENUMPROC;

typedef struct {
  UINT  cbSize;
  HWND  hwnd;
  DWORD dwFlags;
  UINT  uCount;
  DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;

typedef struct tagALTTABINFO {
  DWORD cbSize;
  int   cItems;
  int   cColumns;
  int   cRows;
  int   iColFocus;
  int   iRowFocus;
  int   cxItem;
  int   cyItem;
  POINT ptStart;
} ALTTABINFO, *PALTTABINFO, *LPALTTABINFO;

typedef struct tagCOMBOBOXINFO {
  DWORD cbSize;
  RECT  rcItem;
  RECT  rcButton;
  DWORD stateButton;
  HWND  hwndCombo;
  HWND  hwndItem;
  HWND  hwndList;
} COMBOBOXINFO, *PCOMBOBOXINFO, *LPCOMBOBOXINFO;

typedef struct tagCURSORINFO {
  DWORD   cbSize;
  DWORD   flags;
  HCURSOR hCursor;
  POINT   ptScreenPos;
} CURSORINFO, *PCURSORINFO, *LPCURSORINFO;

typedef struct tagGUITHREADINFO {
  DWORD   cbSize;
  DWORD   flags;
  HWND    hwndActive;
  HWND    hwndFocus;
  HWND    hwndCapture;
  HWND    hwndMenuOwner;
  HWND    hwndMoveSize;
  HWND    hwndCaret;
  RECT    rcCaret;
} GUITHREADINFO, *PGUITHREADINFO, *LPGUITHREADINFO;

typedef struct tagLASTINPUTINFO {
  UINT cbSize;
  DWORD dwTime;
} LASTINPUTINFO, *PLASTINPUTINFO;

typedef struct tagMENUBARINFO {
  DWORD cbSize;
  RECT  rcBar;
  HMENU hMenu;
  HWND  hwndMenu;
  BOOL  fBarFocused:1;
  BOOL  fFocused:1;
} MENUBARINFO, *PMENUBARINFO;

typedef struct tagMENUINFO {
  DWORD   cbSize;
  DWORD   fMask;
  DWORD   dwStyle;
  UINT    cyMax;
  HBRUSH  hbrBack;
  DWORD   dwContextHelpID;
  ULONG_PTR  dwMenuData;
} MENUINFO, FAR *LPMENUINFO;
typedef MENUINFO CONST FAR *LPCMENUINFO;

typedef struct tagMONITORINFO {
  DWORD cbSize;
  RECT rcMonitor;
  RECT rcWork;
  DWORD dwFlags;
} MONITORINFO, *LPMONITORINFO;

typedef struct tagMOUSEMOVEPOINT {
  int       x;
  int       y;
  DWORD     time;
  ULONG_PTR dwExtraInfo;
} MOUSEMOVEPOINT, *PMOUSEMOVEPOINT, *LPMOUSEMOVEPOINT;

#define CCHILDREN_SCROLLBAR 5

typedef struct tagSCROLLBARINFO {
  DWORD cbSize;
  RECT  rcScrollBar;
  int   dxyLineButton;
  int   xyThumbTop;
  int   xyThumbBottom;
  int   reserved;
  DWORD rgstate[CCHILDREN_SCROLLBAR+1];
} SCROLLBARINFO, *PSCROLLBARINFO, *LPSCROLLBARINFO;

#define CCHILDREN_TITLEBAR 5

typedef struct tagTITLEBARINFO {
  DWORD cbSize;
  RECT  rcTitleBar;
  DWORD rgstate[CCHILDREN_TITLEBAR+1];
} TITLEBARINFO, *PTITLEBARINFO, *LPTITLEBARINFO;

typedef struct tagWINDOWINFO {
  DWORD   cbSize;
  RECT    rcWindow;
  RECT    rcClient;
  DWORD   dwStyle;
  DWORD   dwExStyle;
  DWORD   dwWindowStatus;
  UINT    cxWindowBorders;
  UINT    cyWindowBorders;
  ATOM    atomWindowType;
  WORD    wCreatorVersion;
} WINDOWINFO, *PWINDOWINFO, *LPWINDOWINFO;

typedef struct tagMOUSEINPUT {
  LONG    dx;
  LONG    dy;
  DWORD   mouseData;
  DWORD   dwFlags;
  DWORD   time;
  ULONG_PTR   dwExtraInfo;
} MOUSEINPUT, *PMOUSEINPUT;

typedef struct tagKEYBDINPUT {
  WORD      wVk;
  WORD      wScan;
  DWORD     dwFlags;
  DWORD     time;
  ULONG_PTR dwExtraInfo;
} KEYBDINPUT, *PKEYBDINPUT;

typedef struct tagHARDWAREINPUT {
  DWORD   uMsg;
  WORD    wParamL;
  WORD    wParamH;
} HARDWAREINPUT, *PHARDWAREINPUT;

typedef struct tagINPUT {
  DWORD type;
  union
  {
    MOUSEINPUT mi;
    KEYBDINPUT ki;
    HARDWAREINPUT hi;
  } u;
} INPUT, *PINPUT, FAR* LPINPUT;

typedef struct tagTRACKMOUSEEVENT {
  DWORD cbSize;
  DWORD dwFlags;
  HWND  hwndTrack;
  DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

typedef IMAGE_THUNK_DATA *          PImgThunkData;
typedef const IMAGE_THUNK_DATA *    PCImgThunkData;

typedef struct ImgDelayDescr {
    DWORD           grAttrs;
    LPCSTR          szName;
    HMODULE *       phmod;
    PImgThunkData   pIAT;
    PCImgThunkData  pINT;
    PCImgThunkData  pBoundIAT;
    PCImgThunkData  pUnloadIAT;
    DWORD           dwTimeStamp;
    } ImgDelayDescr, * PImgDelayDescr;

typedef const ImgDelayDescr *   PCImgDelayDescr;

typedef struct DelayLoadProc {
    BOOL                fImportByName;
    union {
        LPCSTR          szProcName;
        DWORD           dwOrdinal;
        };
    } DelayLoadProc;

typedef struct DelayLoadInfo {
    DWORD               cb;
    PCImgDelayDescr     pidd;
    FARPROC *           ppfn;
    LPCSTR              szDll;
    DelayLoadProc       dlp;
    HMODULE             hmodCur;
    FARPROC             pfnCur;
    DWORD               dwLastError;
    } DelayLoadInfo, * PDelayLoadInfo;

typedef struct _RTL_HEAP_TAG_INFO {
	ULONG AllocCount;
	ULONG FreeCount;
	ULONG MemoryUsed;
} RTL_HEAP_TAG_INFO, *LPRTL_HEAP_TAG_INFO, *PRTL_HEAP_TAG_INFO;

typedef struct _PORT_MESSAGE {
	USHORT DataSize;
	USHORT MessageSize;
	USHORT MessageType;
	USHORT VirtualRangesOffset;
	CLIENT_ID ClientId;
	ULONG MessageId;
	ULONG SectionSize;
//	UCHAR Data [];
} PORT_MESSAGE,*PPORT_MESSAGE;

typedef struct _PORT_SECTION_WRITE {
	ULONG Length;
	HANDLE SectionHandle;
	ULONG SectionOffset;
	ULONG ViewSize;
	PVOID ViewBase;
	PVOID TargetViewBase;
} PORT_SECTION_WRITE,*PPORT_SECTION_WRITE;

typedef struct _PORT_SECTION_READ {
	ULONG Length;
	ULONG ViewSize;
	ULONG ViewBase;
} PORT_SECTION_READ,*PPORT_SECTION_READ;

typedef struct _FILE_USER_QUOTA_INFORMATION {
	ULONG NextEntryOffset;
	ULONG SidLength;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER QuotaUsed;
	LARGE_INTEGER QuotaThreshold;
	LARGE_INTEGER QuotaLimit;
	SID Sid [1 ];
} FILE_USER_QUOTA_INFORMATION,*PFILE_USER_QUOTA_INFORMATION;

typedef struct _FILE_QUOTA_LIST_INFORMATION {
	ULONG NextEntryOffset;
	ULONG SidLength;
	SID Sid [1 ];
} FILE_QUOTA_LIST_INFORMATION,*PFILE_QUOTA_LIST_INFORMATION;

typedef struct _BLENDFUNCTION {
  BYTE     BlendOp;
  BYTE     BlendFlags;
  BYTE     SourceConstantAlpha;
  BYTE     AlphaFormat;
}BLENDFUNCTION, *PBLENDFUNCTION, *LPBLENDFUNCTION;

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard
} GET_FILEEX_INFO_LEVELS;

typedef struct _WIN32_FILE_ATTRIBUTES_DATA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

typedef struct _GRADIENT_TRIANGLE {
  ULONG Vertex1;
  ULONG Vertex2;
  ULONG Vertex3;
} GRADIENT_TRIANGLE, *PGRADIENT_TRIANGLE, *LPGRADIENT_TRIANGLE;

typedef struct _GRADIENT_RECT {
  ULONG UpperLeft;
  ULONG LowerRight;
} GRADIENT_RECT, *PGRADIENT_RECT, *LPGRADIENT_RECT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WIN32_LEAN_AND_MEAN */

#endif /* _GNU_H_WINDOWS32_STRUCTURES */
