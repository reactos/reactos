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

#ifdef COMPILER_LARGE_INTEGERS

typedef long long int LARGE_INTEGER, *PLARGE_INTEGER;
typedef unsigned long long int ULARGE_INTEGER, *PULARGE_INTEGER;

#else

typedef struct _LARGE_INTEGER { 
  DWORD LowPart; 
  LONG  HighPart; 
} LARGE_INTEGER, *PLARGE_INTEGER; 

typedef struct _ULARGE_INTEGER { 
  DWORD LowPart; 
  DWORD HighPart; 
} ULARGE_INTEGER, *PULARGE_INTEGER; 

#endif

typedef struct _LIST_ENTRY { 
  struct _LIST_ENTRY *Flink; 
  struct _LIST_ENTRY *Blink; 
} LIST_ENTRY, *PLIST_ENTRY; 

typedef DWORD ACCESS_MASK; 


typedef struct _SID_IDENTIFIER_AUTHORITY { 
  BYTE Value[6]; 
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY, 
    *LPSID_IDENTIFIER_AUTHORITY; 


typedef struct _ACL {
  BYTE AclRevision; 
  BYTE Sbz1; 
  WORD AclSize; 
  WORD AceCount; 
  WORD Sbz2; 
} ACL, *PACL; 


typedef struct _SID {
   BYTE  Revision;
   BYTE  SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
   DWORD SubAuthority[ANYSIZE_ARRAY];
} SID, *PSID;

typedef WORD SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

typedef struct _SECURITY_DESCRIPTOR {
  BYTE  Revision;
  BYTE  Sbz1;
  SECURITY_DESCRIPTOR_CONTROL Control;
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;
} SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR;

typedef struct _SECURITY_QUALITY_OF_SERVICE { 
  DWORD Length; 
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel; 
  /* SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode; */
  WINBOOL ContextTrackingMode; 
  BOOLEAN EffectiveOnly; 
} SECURITY_QUALITY_OF_SERVICE; 

typedef LARGE_INTEGER LUID, *PLUID;

typedef struct _TOKEN_SOURCE {
  CHAR SourceName[8]; 
  LUID SourceIdentifier; 
} TOKEN_SOURCE; 

typedef struct _PARTITION_INFORMATION { 
  BYTE PartitionType; 
  BOOLEAN BootIndicator; 
  BOOLEAN RecognizedPartition; 
  BOOLEAN RewritePartition; 
  LARGE_INTEGER StartingOffset; 
  LARGE_INTEGER PartitionLength; 
  LARGE_INTEGER HiddenSectors; 
} PARTITION_INFORMATION ; 

typedef struct _SINGLE_LIST_ENTRY { 
  struct _SINGLE_LIST_ENTRY *Next; 
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY; 

 
typedef struct _CRITICAL_SECTION_DEBUG {
    WORD   Type;
    WORD   CreatorBackTraceIndex;
    struct _CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Depth;
    PVOID OwnerBackTrace[ 5 ];
} CRITICAL_SECTION_DEBUG, *PCRITICAL_SECTION_DEBUG;

typedef struct _CRITICAL_SECTION {
    PCRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD Reserved;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _GENERIC_MAPPING { 
  ACCESS_MASK GenericRead; 
  ACCESS_MASK GenericWrite; 
  ACCESS_MASK GenericExecute; 
  ACCESS_MASK GenericAll; 
} GENERIC_MAPPING, *PGENERIC_MAPPING; 

typedef struct _LUID_AND_ATTRIBUTES { 
  LUID  Luid; 
  DWORD Attributes; 
} LUID_AND_ATTRIBUTES; 

typedef struct _PRIVILEGE_SET { 
  DWORD PrivilegeCount; 
  DWORD Control; 
  LUID_AND_ATTRIBUTES Privilege[ANYSIZE_ARRAY]; 
} PRIVILEGE_SET, *PPRIVILEGE_SET, *LPPRIVILEGE_SET; 

#ifdef __i386__

typedef struct _FLOATING_SAVE_AREA {
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    BYTE    RegisterArea[80];
    DWORD   Cr0NpxState;
} FLOATING_SAVE_AREA;

typedef struct _CONTEXT {
    DWORD ContextFlags;

    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    FLOATING_SAVE_AREA FloatSave;

    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs; 
    DWORD   EFlags;
    DWORD   Esp;
    DWORD   SegSs;
} CONTEXT, *PCONTEXT, *LPCONTEXT;

#else /* __ppc__ */

typedef struct
  {
    /* Floating point registers returned when CONTEXT_FLOATING_POINT is set */
    double Fpr0;
    double Fpr1;
    double Fpr2;
    double Fpr3;
    double Fpr4;
    double Fpr5;
    double Fpr6;
    double Fpr7;
    double Fpr8;
    double Fpr9;
    double Fpr10;
    double Fpr11;
    double Fpr12;
    double Fpr13;
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    double Fpscr;

    /* Integer registers returned when CONTEXT_INTEGER is set.  */
    DWORD Gpr0;
    DWORD Gpr1;
    DWORD Gpr2;
    DWORD Gpr3;
    DWORD Gpr4;
    DWORD Gpr5;
    DWORD Gpr6;
    DWORD Gpr7;
    DWORD Gpr8;
    DWORD Gpr9;
    DWORD Gpr10;
    DWORD Gpr11;
    DWORD Gpr12;
    DWORD Gpr13;
    DWORD Gpr14;
    DWORD Gpr15;
    DWORD Gpr16;
    DWORD Gpr17;
    DWORD Gpr18;
    DWORD Gpr19;
    DWORD Gpr20;
    DWORD Gpr21;
    DWORD Gpr22;
    DWORD Gpr23;
    DWORD Gpr24;
    DWORD Gpr25;
    DWORD Gpr26;
    DWORD Gpr27;
    DWORD Gpr28;
    DWORD Gpr29;
    DWORD Gpr30;
    DWORD Gpr31;

    DWORD Cr;			/* Condition register */
    DWORD Xer;			/* Fixed point exception register */

    /* The following are set when CONTEXT_CONTROL is set.  */
    DWORD Msr;			/* Machine status register */
    DWORD Iar;			/* Instruction address register */
    DWORD Lr;			/* Link register */
    DWORD Ctr;			/* Control register */

    /* Control which context values are returned */
    DWORD ContextFlags;
    DWORD Fill[3];

    /* Registers returned if CONTEXT_DEBUG_REGISTERS is set.  */
    DWORD Dr0;                          /* Breakpoint Register 1 */
    DWORD Dr1;                          /* Breakpoint Register 2 */
    DWORD Dr2;                          /* Breakpoint Register 3 */
    DWORD Dr3;                          /* Breakpoint Register 4 */
    DWORD Dr4;                          /* Breakpoint Register 5 */
    DWORD Dr5;                          /* Breakpoint Register 6 */
    DWORD Dr6;                          /* Debug Status Register */
    DWORD Dr7;                          /* Debug Control Register */
} CONTEXT, *PCONTEXT, *LPCONTEXT;
#endif


typedef struct value_ent {
    LPTSTR   ve_valuename;
    DWORD ve_valuelen;
    DWORD ve_valueptr;
    DWORD ve_type;
} VALENT, *PVALENT;


typedef struct _EXCEPTION_RECORD { 
  DWORD ExceptionCode; 
  DWORD ExceptionFlags; 
  struct _EXCEPTION_RECORD *ExceptionRecord; 
  PVOID ExceptionAddress; 
  DWORD NumberParameters; 
  DWORD ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS]; 
} EXCEPTION_RECORD, *PEXCEPTION_RECORD, *LPEXCEPTION_RECORD; 

typedef struct _DRIVE_LAYOUT_INFORMATION { 
  DWORD  PartitionCount; 
  DWORD  Signature; 
  PARTITION_INFORMATION  PartitionEntry[1]; 
} DRIVE_LAYOUT_INFORMATION; 

typedef struct _KEY_EVENT_RECORD { 
  WINBOOL bKeyDown;             
  WORD wRepeatCount;         
  WORD wVirtualKeyCode;      
  WORD wVirtualScanCode; 

  char AsciiChar;
  char pad;
#if 0
  union { 
    WCHAR UnicodeChar; 
    CHAR  AsciiChar; 
  } uChar;  
#endif
  DWORD dwControlKeyState;   
} KEY_EVENT_RECORD PACKED;

typedef struct _COORD {
  SHORT X;    
  SHORT Y;    
} COORD; 

typedef struct _MOUSE_EVENT_RECORD { 
  COORD dwMousePosition; 
  DWORD dwButtonState; 
  DWORD dwControlKeyState; 
  DWORD dwEventFlags; 
} MOUSE_EVENT_RECORD; 

typedef struct _DISK_GEOMETRY { 
  LARGE_INTEGER  Cylinders; 
  MEDIA_TYPE  MediaType; 
  DWORD  TracksPerCylinder; 
  DWORD  SectorsPerTrack; 
  DWORD  BytesPerSector; 
} DISK_GEOMETRY ; 


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

typedef struct _ACE_HEADER {
  BYTE AceType; 
  BYTE AceFlags; 
  WORD AceSize; 
} ACE_HEADER; 

typedef ACCESS_MASK REGSAM;

typedef struct _ACCESS_ALLOWED_ACE {
  ACE_HEADER Header; 
  ACCESS_MASK Mask; 
  DWORD SidStart; 
} ACCESS_ALLOWED_ACE; 

typedef struct _ACCESS_DENIED_ACE {
  ACE_HEADER  Header; 
  ACCESS_MASK Mask; 
  DWORD       SidStart; 
} ACCESS_DENIED_ACE; 

typedef struct tagACCESSTIMEOUT {
  UINT  cbSize; 
  DWORD dwFlags; 
  DWORD iTimeOutMSec; 
} ACCESSTIMEOUT; 

typedef struct _ACL_REVISION_INFORMATION {    
  DWORD   AclRevision; 
} ACL_REVISION_INFORMATION; 

typedef struct _ACL_SIZE_INFORMATION {
  DWORD   AceCount; 
  DWORD   AclBytesInUse; 
  DWORD   AclBytesFree; 
} ACL_SIZE_INFORMATION; 

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

typedef struct _ADDJOB_INFO_1 {
  LPTSTR  Path; 
  DWORD   JobId; 
} ADDJOB_INFO_1; 

typedef struct tagANIMATIONINFO { 
  UINT cbSize; 
  int  iMinAnimate; 
} ANIMATIONINFO, *LPANIMATIONINFO; 

typedef struct _RECT {  
  LONG left;       
  LONG top;        
  LONG right;      
  LONG bottom;     
} RECT, *LPRECT, *PRECT; 
 
typedef struct _RECTL { 
  LONG left;     
  LONG top;      
  LONG right;    
  LONG bottom;   
} RECTL; 
 
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
} RGBTRIPLE; 
 
typedef struct _BITMAPCOREINFO {
  BITMAPCOREHEADER  bmciHeader; 
  RGBTRIPLE         bmciColors[1]; 
} BITMAPCOREINFO, *PBITMAPCOREINFO, *LPBITMAPCOREINFO; 

typedef struct tagBITMAPFILEHEADER {
  WORD    bfType; 
  DWORD   bfSize; 
  WORD    bfReserved1; 
  WORD    bfReserved2; 
  DWORD   bfOffBits; 
} BITMAPFILEHEADER __attribute__((packed));

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
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFO; 

typedef struct tagRGBQUAD { 
  BYTE    rgbBlue; 
  BYTE    rgbGreen; 
  BYTE    rgbRed; 
  BYTE    rgbReserved; 
} RGBQUAD; 
 
typedef struct tagBITMAPINFO {
  BITMAPINFOHEADER bmiHeader; 
  RGBQUAD          bmiColors[1]; 
} BITMAPINFO, *LPBITMAPINFO; 

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

typedef struct _BLOB { 
  ULONG   cbSize; 
  BYTE    *pBlobData; 
} BLOB; 

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

typedef struct _FILETIME { 
  DWORD dwLowDateTime; 
  DWORD dwHighDateTime; 
} FILETIME, *LPFILETIME, *PFILETIME; 
 
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
} POINTL; 
 
typedef struct tagPOINTS { 
  SHORT x;    
  SHORT y;    
} POINTS; 
 
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

typedef struct tagCREATESTRUCT { 
  LPVOID    lpCreateParams;  
  HINSTANCE hInstance;       
  HMENU     hMenu;           
  HWND      hwndParent;      
  int       cy;              
  int       cx;              
  int       y;               
  int       x;               
  LONG      style;           
  LPCTSTR   lpszName;        
  LPCTSTR   lpszClass;       
  DWORD     dwExStyle;       
} CREATESTRUCT, *LPCREATESTRUCT; 
 
typedef struct tagCBT_CREATEWND {
  LPCREATESTRUCT lpcs; 
  HWND           hwndInsertAfter; 
} CBT_CREATEWND; 

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

typedef struct _charformat { 
  UINT     cbSize; 
  DWORD    dwMask; 
  DWORD    dwEffects; 
  LONG     yHeight; 
  LONG     yOffset; 
  COLORREF crTextColor; 
  BYTE     bCharSet; 
  BYTE     bPitchAndFamily; 
  TCHAR    szFaceName[LF_FACESIZE]; 
} CHARFORMAT; 

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

typedef struct { 
  DWORD        lStructSize; 
  HWND         hwndOwner; 
  HWND         hInstance; 
  COLORREF     rgbResult; 
  COLORREF*    lpCustColors; 
  DWORD        Flags; 
  LPARAM       lCustData; 
  LPCCHOOKPROC lpfnHook; 
  LPCTSTR      lpTemplateName; 
} CHOOSECOLOR, *LPCHOOSECOLOR; 

typedef struct tagLOGFONT { 
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
  TCHAR lfFaceName[LF_FACESIZE]; 
} LOGFONT, *LPLOGFONT, *PLOGFONT; 
 
typedef struct { 
  DWORD        lStructSize; 
  HWND         hwndOwner; 
  HDC          hDC; 
  LPLOGFONT    lpLogFont; 
  INT          iPointSize; 
  DWORD        Flags; 
  DWORD        rgbColors; 
  LPARAM       lCustData; 
  LPCFHOOKPROC lpfnHook; 
  LPCTSTR      lpTemplateName; 
  HINSTANCE    hInstance; 
  LPTSTR       lpszStyle; 
  WORD         nFontType; 
  WORD         ___MISSING_ALIGNMENT__; 
  INT          nSizeMin; 
  INT          nSizeMax; 
} CHOOSEFONT, *LPCHOOSEFONT; 

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
  UINT  CtlType; 
  UINT  CtlID; 
  HWND  hwndItem; 
  UINT  itemID1; 
  DWORD itemData1; 
  UINT  itemID2; 
  DWORD itemData2; 
} COMPAREITEMSTRUCT; 

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

typedef struct _CONSOLE_CURSOR_INFO {
  DWORD  dwSize; 
  WINBOOL   bVisible; 
} CONSOLE_CURSOR_INFO, *PCONSOLE_CURSOR_INFO; 

typedef struct _SMALL_RECT { 
  SHORT Left;      
  SHORT Top;       
  SHORT Right;     
  SHORT Bottom;    
} SMALL_RECT, *PSMALL_RECT; 
 
typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
  COORD      dwSize; 
  COORD      dwCursorPosition; 
  WORD       wAttributes; 
  SMALL_RECT srWindow; 
  COORD      dwMaximumWindowSize; 
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO ; 

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
} CONVINFO; 

typedef struct tagCOPYDATASTRUCT { 
  DWORD dwData; 
  DWORD cbData; 
  PVOID lpData; 
} COPYDATASTRUCT; 

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
 
typedef struct _currencyfmt { 
  UINT      NumDigits; 
  UINT      LeadingZero; 
  UINT      Grouping; 
  LPTSTR    lpDecimalSep; 
  LPTSTR    lpThousandSep; 
  UINT      NegativeOrder; 
  UINT      PositiveOrder; 
  LPTSTR    lpCurrencySymbol;
} CURRENCYFMT; 
 
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
 
typedef struct _DATATYPES_INFO_1 { 
  LPTSTR pName; 
} DATATYPES_INFO_1; 
 
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
 
typedef struct _devicemode {  
  BCHAR  dmDeviceName[CCHDEVICENAME]; 
  WORD   dmSpecVersion; 
  WORD   dmDriverVersion; 
  WORD   dmSize; 
  WORD   dmDriverExtra; 
  DWORD  dmFields; 
  short  dmOrientation; 
  short  dmPaperSize; 
  short  dmPaperLength; 
  short  dmPaperWidth; 
  short  dmScale; 
  short  dmCopies; 
  short  dmDefaultSource; 
  short  dmPrintQuality; 
  short  dmColor; 
  short  dmDuplex; 
  short  dmYResolution; 
  short  dmTTOption; 
  short  dmCollate; 
  BCHAR  dmFormName[CCHFORMNAME]; 
  WORD  dmLogPixels; 
  DWORD  dmBitsPerPel; 
  DWORD  dmPelsWidth; 
  DWORD  dmPelsHeight; 
  DWORD  dmDisplayFlags; 
  DWORD  dmDisplayFrequency; 
  DWORD  dmICMMethod;         
  DWORD  dmICMIntent;         
  DWORD  dmMediaType;         
  DWORD  dmDitherType;        
  DWORD  dmICCManufacturer;   
  DWORD  dmICCModel;          
} DEVMODE, *LPDEVMODE; 
 
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
 
typedef struct { 
  DWORD style; 
  DWORD dwExtendedStyle; 
  short x; 
  short y; 
  short cx; 
  short cy; 
  WORD  id; 
} DLGITEMTEMPLATE PACKED;

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
} DLGTEMPLATE PACKED;

typedef DLGTEMPLATE *LPDLGTEMPLATE; 
typedef const DLGTEMPLATE *LPCDLGTEMPLATE;
 
typedef struct _DOC_INFO_1 { 
  LPTSTR pDocName; 
  LPTSTR pOutputFile; 
  LPTSTR pDatatype; 
} DOC_INFO_1; 
 
typedef struct _DOC_INFO_2 { 
  LPTSTR pDocName; 
  LPTSTR pOutputFile; 
  LPTSTR pDatatype; 
  DWORD  dwMode; 
  DWORD  JobId; 
} DOC_INFO_2; 
 
typedef struct {    
  int     cbSize; 
  LPCTSTR lpszDocName; 
  LPCTSTR lpszOutput; 
  LPCTSTR lpszDatatype; 
  DWORD   fwType;       
} DOCINFO; 
 
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
 
 
 
typedef struct _DRIVER_INFO_1 { 
  LPTSTR pName; 
} DRIVER_INFO_1; 
 
typedef struct _DRIVER_INFO_2 { 
  DWORD  cVersion; 
  LPTSTR pName; 
  LPTSTR pEnvironment; 
  LPTSTR pDriverPath; 
  LPTSTR pDataFile; 
  LPTSTR pConfigFile; 
} DRIVER_INFO_2; 
 
typedef struct _DRIVER_INFO_3 { 
  DWORD  cVersion; 
  LPTSTR pName; 
  LPTSTR pEnvironment; 
  LPTSTR pDriverPath; 
  LPTSTR pDataFile; 
  LPTSTR pConfigFile; 
  LPTSTR pHelpFile; 
  LPTSTR pDependentFiles; 
  LPTSTR pMonitorName; 
  LPTSTR pDefaultDataType; 
} DRIVER_INFO_3; 
 
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
} LOGBRUSH; 
 
typedef struct tagEMRCREATEBRUSHINDIRECT
{
  EMR      emr; 
  DWORD    ihBrush; 
  LOGBRUSH lb; 
} EMRCREATEBRUSHINDIRECT, *PEMRCREATEBRUSHINDIRECT; 
 
typedef LONG LCSCSTYPE;
typedef LONG LCSGAMUTMATCH;

typedef struct tagLOGCOLORSPACE {
  DWORD         lcsSignature; 
  DWORD         lcsVersion; 
  DWORD         lcsSize; 

  LCSCSTYPE     lcsCSType; 
  LCSGAMUTMATCH lcsIntent; 
  CIEXYZTRIPLE  lcsEndpoints;
  DWORD         lcsGammaRed;
  DWORD         lcsGammaGreen; 
  DWORD         lcsGammaBlue;
  TCHAR         lcsFilename[MAX_PATH]; 
} LOGCOLORSPACE, *LPLOGCOLORSPACE; 
 
typedef struct tagEMRCREATECOLORSPACE
{
  EMR           emr; 
  DWORD         ihCS; 
  LOGCOLORSPACE lcs; 
} EMRCREATECOLORSPACE, *PEMRCREATECOLORSPACE; 
 
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
} LOGPEN; 
 
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

typedef struct tagEXTLOGFONT { 
    LOGFONT elfLogFont; 
    BCHAR    elfFullName[LF_FULLFACESIZE]; 
    BCHAR    elfStyle[LF_FACESIZE]; 
    DWORD   elfVersion; 
    DWORD   elfStyleSize; 
    DWORD   elfMatch; 
    DWORD   elfReserved; 
    BYTE    elfVendorId[ELF_VENDOR_SIZE]; 
    DWORD   elfCulture; 
    PANOSE  elfPanose; 
} EXTLOGFONT; 
 
typedef struct tagEMREXTCREATEFONTINDIRECTW
{
  EMR         emr; 
  DWORD       ihFont; 
  EXTLOGFONT  elfw; 
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
 
typedef struct _ENUM_SERVICE_STATUS { 
  LPTSTR lpServiceName; 
  LPTSTR lpDisplayName; 
  SERVICE_STATUS ServiceStatus; 
} ENUM_SERVICE_STATUS, *LPENUM_SERVICE_STATUS; 
 
typedef struct tagENUMLOGFONT { 
  LOGFONT elfLogFont; 
  BCHAR    elfFullName[LF_FULLFACESIZE]; 
  BCHAR    elfStyle[LF_FACESIZE]; 
} ENUMLOGFONT; 
 
typedef struct tagENUMLOGFONTEX {
  LOGFONT  elfLogFont;
  BCHAR  elfFullName[LF_FULLFACESIZE];
  BCHAR  elfStyle[LF_FACESIZE];
  BCHAR  elfScript[LF_FACESIZE];
} ENUMLOGFONTEX;
 
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
 
typedef struct _EXCEPTION_POINTERS { 
  PEXCEPTION_RECORD ExceptionRecord; 
  PCONTEXT ContextRecord; 
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS, *LPEXCEPTION_POINTERS; 
 
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
 
typedef struct {   
  DWORD        lStructSize; 
  HWND         hwndOwner; 
  HINSTANCE    hInstance; 
  DWORD        Flags; 
  LPTSTR        lpstrFindWhat; 
  LPTSTR        lpstrReplaceWith; 
  WORD         wFindWhatLen; 
  WORD         wReplaceWithLen; 
  LPARAM        lCustData; 
  LPFRHOOKPROC lpfnHook; 
  LPCTSTR       lpTemplateName; 
} FINDREPLACE, *LPFINDREPLACE; 
 
typedef struct _findtext { 
  CHARRANGE chrg; 
  LPSTR lpstrText; 
} FINDTEXT; 
 
typedef struct _findtextex { 
  CHARRANGE chrg;  
  LPSTR lpstrText; 
  CHARRANGE chrgText; 
} FINDTEXTEX; 
 
typedef struct _FMS_GETDRIVEINFO { 
  DWORD dwTotalSpace; 
  DWORD dwFreeSpace; 
  TCHAR  szPath[260]; 
  TCHAR  szVolume[14]; 
  TCHAR  szShare[128]; 
} FMS_GETDRIVEINFO; 
 
typedef struct _FMS_GETFILESEL { 
  FILETIME ftTime; 
  DWORD    dwSize; 
  BYTE     bAttr; 
  TCHAR     szName[260]; 
} FMS_GETFILESEL; 
 
typedef struct _FMS_LOAD { 
  DWORD dwSize; 
  TCHAR  szMenuName[MENU_TEXT_LEN]; 
  HMENU hMenu; 
  UINT  wMenuDelta; 
} FMS_LOAD; 
 
typedef struct _FMS_TOOLBARLOAD { 
  DWORD        dwSize; 
  LPEXT_BUTTON lpButtons; 
  WORD         cButtons; 
  WORD         cBitmaps; 
  WORD         idBitmap; 
  HBITMAP      hBitmap; 
} FMS_TOOLBARLOAD; 
 
typedef struct _FOCUS_EVENT_RECORD { 
  WINBOOL bSetFocus; 
} FOCUS_EVENT_RECORD; 
 
typedef struct _FORM_INFO_1 { 
  DWORD Flags; 
  LPTSTR pName; 
  SIZEL  Size; 
  RECTL  ImageableArea; 
} FORM_INFO_1; 
 
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
 
typedef struct tagGCP_RESULTS {
  DWORD  lStructSize;
  LPTSTR  lpOutString;
  UINT  *lpOrder;
  INT  *lpDx;
  INT  *lpCaretPos;
  LPTSTR lpClass;
  UINT  *lpGlyphs;
  UINT  nGlyphs;
  UINT  nMaxFit;
} GCP_RESULTS, *LPGCP_RESULTS;
 
 
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
 
typedef struct _HD_ITEM { 
  UINT    mask; 
  int     cxy; 
  LPTSTR   pszText; 
  HBITMAP hbm; 
  int     cchTextMax; 
  int     fmt; 
  LPARAM  lParam; 
} HD_ITEM; 
 
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
 
typedef struct _HD_NOTIFY { 
  NMHDR   hdr; 
  int     iItem; 
  int     iButton; 
  HD_ITEM  * pitem; 
} HD_NOTIFY; 
 
typedef  struct  tagHELPINFO { 
  UINT   cbSize; 
  int    iContextType; 
  int    iCtrlId; 
  HANDLE hItemHandle; 
  DWORD  dwContextId; 
  POINT  MousePos; 
} HELPINFO,   *LPHELPINFO; 
 
typedef struct {   
  int   wStructSize; 
  int   x; 
  int   y; 
  int   dx; 
  int   dy; 
  int   wMax; 
  TCHAR rgchMember[2]; 
} HELPWININFO; 
 
typedef struct tagHIGHCONTRAST {  
  UINT cbSize; 
  DWORD dwFlags; 
  LPTSTR lpszDefaultScheme; 
} HIGHCONTRAST,  * LPHIGHCONTRAST; 
 
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
 
typedef struct tagICONMETRICS { 
  UINT    cbSize; 
  int     iHorzSpacing; 
  int     iVertSpacing; 
  int     iTitleWrap; 
  LOGFONT lfFont; 
} ICONMETRICS,   *LPICONMETRICS; 
 
typedef struct _IMAGEINFO { 
  HBITMAP hbmImage; 
  HBITMAP hbmMask;  
  int     Unused1;  
  int     Unused2;  
  RECT    rcImage;  
} IMAGEINFO; 
 
typedef struct _WINDOW_BUFFER_SIZE_RECORD { 
  COORD dwSize; 
} WINDOW_BUFFER_SIZE_RECORD; 
 
typedef struct _MENU_EVENT_RECORD { 
  UINT dwCommandId; 
} MENU_EVENT_RECORD, *PMENU_EVENT_RECORD; 
 
typedef struct _INPUT_RECORD { 
  WORD EventType; 
  union { 
#ifndef __cplus_plus
    /* this will be the wrong size in c++ */
    KEY_EVENT_RECORD KeyEvent; 
#endif
    MOUSE_EVENT_RECORD MouseEvent; 
    WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent; 
    MENU_EVENT_RECORD MenuEvent; 
    FOCUS_EVENT_RECORD FocusEvent; 
  } Event; 
} INPUT_RECORD, *PINPUT_RECORD; 
 
typedef struct _SYSTEMTIME {  
  WORD wYear; 
  WORD wMonth; 
  WORD wDayOfWeek; 
  WORD wDay; 
  WORD wHour; 
  WORD wMinute; 
  WORD wSecond; 
  WORD wMilliseconds; 
} SYSTEMTIME, *LPSYSTEMTIME; 
 
typedef struct _JOB_INFO_1 {  
  DWORD  JobId; 
  LPTSTR pPrinterName; 
  LPTSTR pMachineName; 
  LPTSTR pUserName; 
  LPTSTR pDocument; 
  LPTSTR pDatatype; 
  LPTSTR pStatus; 
  DWORD  Status; 
  DWORD  Priority; 
  DWORD  Position; 
  DWORD  TotalPages; 
  DWORD  PagesPrinted; 
  SYSTEMTIME Submitted; 
} JOB_INFO_1; 
 
 
typedef struct _JOB_INFO_2 { 
  DWORD      JobId; 
  LPTSTR     pPrinterName; 
  LPTSTR     pMachineName; 
  LPTSTR     pUserName; 
  LPTSTR     pDocument; 
  LPTSTR     pNotifyName; 
  LPTSTR     pDatatype; 
  LPTSTR     pPrintProcessor; 
  LPTSTR     pParameters; 
  LPTSTR     pDriverName; 
  LPDEVMODE  pDevMode; 
  LPTSTR     pStatus; 
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
} JOB_INFO_2; 
 
typedef struct tagKERNINGPAIR { 
  WORD wFirst; 
  WORD wSecond; 
  int  iKernAmount; 
} KERNINGPAIR, *LPKERNINGPAIR; 
 
typedef struct _LANA_ENUM { 
  UCHAR length; 
  UCHAR lana[MAX_LANA]; 
} LANA_ENUM; 
 
typedef struct _LDT_ENTRY { 
  WORD LimitLow; 
  WORD BaseLow; 
  union { 
    struct { 
      BYTE BaseMid; 
      BYTE Flags1; 
      BYTE Flags2; 
      BYTE BaseHi; 
    } Bytes; 
    struct { 
      DWORD BaseMid : 8; 
      DWORD Type : 5; 
      DWORD Dpl : 2; 
      DWORD Pres : 1; 
      DWORD LimitHi : 4; 
      DWORD Sys : 1; 
      DWORD Reserved_0 : 1; 
      DWORD Default_Big : 1; 
      DWORD Granularity : 1; 
      DWORD BaseHi : 8; 
    } Bits; 
  } HighWord; 
} LDT_ENTRY, *PLDT_ENTRY, *LPLDT_ENTRY; 
 
typedef struct tagLOCALESIGNATURE {
  DWORD  lsUsb[4];
  DWORD  lsCsbDefault[2];
  DWORD  lsCsbSupported[2];
} LOCALESIGNATURE; 
 
typedef struct _LOCALGROUP_MEMBERS_INFO_0 {  
  PSID  lgrmi0_sid; 
} LOCALGROUP_MEMBERS_INFO_0; 
 
typedef struct _LOCALGROUP_MEMBERS_INFO_3 {  
  LPWSTR  lgrmi3_domainandname; 
} LOCALGROUP_MEMBERS_INFO_3; 
 
typedef long FXPT16DOT16,  * LPFXPT16DOT16; 


 
typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;

typedef struct _LV_COLUMN { 
  UINT mask;       
  int fmt;         
  int cx;          
  LPTSTR pszText;  
  int cchTextMax;  
  int iSubItem;    
} LV_COLUMN; 
 
typedef struct _LV_ITEM { 
  UINT   mask;         
  int    iItem;        
  int    iSubItem;     
  UINT   state;        
  UINT   stateMask;    
  LPTSTR  pszText;     
  int    cchTextMax;   
  int    iImage;      
  LPARAM lParam;      
} LV_ITEM; 
 
typedef struct tagLV_DISPINFO { 
  NMHDR   hdr;   
  LV_ITEM item;  
} LV_DISPINFO; 
 
typedef struct _LV_FINDINFO { 
  UINT flags;        
  LPCTSTR psz;        
  LPARAM lParam;     
  POINT pt;          
  UINT vkDirection;  
} LV_FINDINFO; 
 
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
} MAT2; 
 
typedef struct tagMDICREATESTRUCT { 
  LPCTSTR szClass; 
  LPCTSTR szTitle; 
  HANDLE  hOwner; 
  int     x; 
  int     y; 
  int     cx; 
  int     cy; 
  DWORD   style; 
  LPARAM  lParam; 
} MDICREATESTRUCT; 

typedef MDICREATESTRUCT *LPMDICREATESTRUCT;
 
typedef struct tagMEASUREITEMSTRUCT { 
  UINT  CtlType;      
  UINT  CtlID;        
  UINT  itemID;       
  UINT  itemWidth;    
  UINT  itemHeight;   
  DWORD itemData;     
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT; 
 
typedef struct _MEMORY_BASIC_INFORMATION { 
  PVOID BaseAddress;            
  PVOID AllocationBase;         
  DWORD AllocationProtect;      
  DWORD RegionSize;             
  DWORD State;                  
  DWORD Protect;                
  DWORD Type;                   
} MEMORY_BASIC_INFORMATION; 
typedef MEMORY_BASIC_INFORMATION *PMEMORY_BASIC_INFORMATION; 
 
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

typedef struct tagMENUITEMINFO {
  UINT    cbSize; 
  UINT    fMask; 
  UINT    fType; 
  UINT    fState; 
  UINT    wID; 
  HMENU   hSubMenu; 
  HBITMAP hbmpChecked; 
  HBITMAP hbmpUnchecked; 
  DWORD   dwItemData; 
  LPTSTR  dwTypeData; 
  UINT    cch; 
} MENUITEMINFO, *LPMENUITEMINFO; 
typedef MENUITEMINFO CONST *LPCMENUITEMINFO;
 
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
 
typedef struct tagMETAHEADER {  
  WORD  mtType; 
  WORD  mtHeaderSize; 
  WORD  mtVersion; 
  DWORD mtSize; 
  WORD  mtNoObjects; 
  DWORD mtMaxRecord; 
  WORD  mtNoParameters; 
} METAHEADER PACKED;
 
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
 
typedef struct tagMONHSZSTRUCT { 
  UINT   cb; 
  WINBOOL   fsAction; 
  DWORD  dwTime; 
  HSZ    hsz; 
  HANDLE hTask; 
  TCHAR   str[1]; 
} MONHSZSTRUCT; 
 
typedef struct _MONITOR_INFO_1 { 
  LPTSTR pName; 
} MONITOR_INFO_1; 
 
typedef struct _MONITOR_INFO_2 { 
  LPTSTR pName; 
  LPTSTR pEnvironment ; 
  LPTSTR pDLLName ; 
} MONITOR_INFO_2; 
 
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
 
typedef void CALLBACK (*MSGBOXCALLBACK) (LPHELPINFO lpHelpInfo);

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
 
typedef struct tagMULTIKEYHELP { 
  DWORD  mkSize; 
  TCHAR  mkKeylist; 
  TCHAR  szKeyphrase[1]; 
} MULTIKEYHELP; 
 
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
 
typedef struct _NDDESHAREINFO { 
  LONG   lRevision; 
  LPTSTR lpszShareName; 
  LONG   lShareType; 
  LPTSTR lpszAppTopicList; 
  LONG   fSharedFlag; 
  LONG   fService; 
  LONG   fStartAppFlag; 
  LONG   nCmdShow; 
  LONG   qModifyId[2]; 
  LONG   cNumItems; 
  LPTSTR lpszItemList; 
}NDDESHAREINFO; 
 
typedef struct _NETRESOURCE { 
  DWORD  dwScope; 
  DWORD  dwType; 
  DWORD  dwDisplayType; 
  DWORD  dwUsage; 
  LPTSTR lpLocalName; 
  LPTSTR lpRemoteName; 
  LPTSTR lpComment; 
  LPTSTR lpProvider; 
} NETRESOURCE, *LPNETRESOURCE; 
 
typedef struct tagNEWCPLINFO {
  DWORD dwSize; 
  DWORD dwFlags; 
  DWORD dwHelpContext; 
  LONG  lData; 
  HICON hIcon; 
  TCHAR  szName[32]; 
  TCHAR  szInfo[64]; 
  TCHAR  szHelpFile[128]; 
} NEWCPLINFO; 
 
typedef struct tagNEWTEXTMETRIC { 
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
  BCHAR  tmFirstChar; 
  BCHAR  tmLastChar; 
  BCHAR  tmDefaultChar; 
  BCHAR  tmBreakChar; 
  BYTE   tmItalic; 
  BYTE   tmUnderlined; 
  BYTE   tmStruckOut; 
  BYTE   tmPitchAndFamily; 
  BYTE   tmCharSet; 
  DWORD  ntmFlags; 
  UINT   ntmSizeEM; 
  UINT   ntmCellHeight; 
  UINT   ntmAvgWidth; 
} NEWTEXTMETRIC; 

 
typedef struct tagNEWTEXTMETRICEX {
  NEWTEXTMETRIC  ntmentm;
  FONTSIGNATURE  ntmeFontSignature;
} NEWTEXTMETRICEX;
 
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

typedef struct _TV_ITEM { 
  UINT       mask;           
  HTREEITEM  hItem;           
  UINT       state;           
  UINT       stateMask;     
  LPTSTR     pszText;        
  int        cchTextMax;      
  int        iImage;          
  int        iSelectedImage;  
  int        cChildren;       
  LPARAM     lParam;          
} TV_ITEM,   *LPTV_ITEM; 
 
typedef struct _NM_TREEVIEW { 
  NMHDR    hdr;       
  UINT     action;        
  TV_ITEM  itemOld;   
  TV_ITEM  itemNew;   
  POINT    ptDrag;    
} NM_TREEVIEW; 
typedef NM_TREEVIEW   *LPNM_TREEVIEW; 

typedef struct _NM_UPDOWN {
  NMHDR    hdr;    
  int     iPos;    
  int  iDelta;     
} NM_UPDOWNW; 
 
typedef struct tagNONCLIENTMETRICS { 
  UINT    cbSize; 
  int     iBorderWidth; 
  int     iScrollWidth; 
  int     iScrollHeight; 
  int     iCaptionWidth; 
  int     iCaptionHeight; 
  LOGFONT lfCaptionFont; 
  int     iSmCaptionWidth; 
  int     iSmCaptionHeight; 
  LOGFONT lfSmCaptionFont; 
  int     iMenuWidth; 
  int     iMenuHeight; 
  LOGFONT lfMenuFont; 
  LOGFONT lfStatusFont; 
  LOGFONT lfMessageFont; 
} NONCLIENTMETRICS,  * LPNONCLIENTMETRICS; 
 
typedef struct _SERVICE_ADDRESS { 
  DWORD   dwAddressType; 
  DWORD   dwAddressFlags; 
  DWORD   dwAddressLength; 
  DWORD   dwPrincipalLength; 
  BYTE   *lpAddress;  
  BYTE   *lpPrincipal; 
} SERVICE_ADDRESS; 
 
typedef struct _SERVICE_ADDRESSES { 
  DWORD   dwAddressCount; 
  SERVICE_ADDRESS   Addresses[1]; 
} SERVICE_ADDRESSES, *LPSERVICE_ADDRESSES; 
 
typedef struct _GUID
{ 
    unsigned long  Data1; 
    unsigned short  Data2; 
    unsigned short  Data3; 
    unsigned char Data4[8]; 
} GUID, *LPGUID;
typedef GUID CLSID, *LPCLSID;
 
typedef struct _SERVICE_INFO { 
  LPGUID   lpServiceType; 
  LPTSTR   lpServiceName; 
  LPTSTR   lpComment; 
  LPTSTR   lpLocale; 
  DWORD    dwDisplayHint; 
  DWORD    dwVersion; 
  DWORD    dwTime; 
  LPTSTR   lpMachineName; 
  LPSERVICE_ADDRESSES lpServiceAddress; 
  BLOB ServiceSpecificInfo; 
} SERVICE_INFO; 
 
typedef struct _NS_SERVICE_INFO { 
  DWORD   dwNameSpace; 
  SERVICE_INFO ServiceInfo; 
} NS_SERVICE_INFO; 
 
typedef struct _numberfmt { 
  UINT      NumDigits; 
  UINT      LeadingZero; 
  UINT      Grouping; 
  LPTSTR    lpDecimalSep; 
  LPTSTR    lpThousandSep; 
  UINT      NegativeOrder; 
} NUMBERFMT; 
 
typedef struct _OFSTRUCT { 
  BYTE cBytes; 
  BYTE fFixedDisk; 
  WORD nErrCode; 
  WORD Reserved1; 
  WORD Reserved2; 
  CHAR szPathName[OFS_MAXPATHNAME]; 
} OFSTRUCT, *LPOFSTRUCT; 
 
typedef struct tagOFN { 
  DWORD         lStructSize; 
  HWND          hwndOwner; 
  HINSTANCE     hInstance; 
  LPCTSTR       lpstrFilter; 
  LPTSTR        lpstrCustomFilter; 
  DWORD         nMaxCustFilter; 
  DWORD         nFilterIndex; 
  LPTSTR        lpstrFile; 
  DWORD         nMaxFile; 
  LPTSTR        lpstrFileTitle; 
  DWORD         nMaxFileTitle; 
  LPCTSTR       lpstrInitialDir; 
  LPCTSTR       lpstrTitle; 
  DWORD         Flags; 
  WORD          nFileOffset; 
  WORD          nFileExtension; 
  LPCTSTR       lpstrDefExt; 
  DWORD         lCustData; 
  LPOFNHOOKPROC lpfnHook; 
  LPCTSTR       lpTemplateName; 
} OPENFILENAME, *LPOPENFILENAME; 

typedef struct _OFNOTIFY {
  NMHDR          hdr; 
  LPOPENFILENAME lpOFN; 
  LPTSTR         pszFile; 
} OFNOTIFY, *LPOFNOTIFY; 
 
typedef struct _OSVERSIONINFO { 
  DWORD dwOSVersionInfoSize; 
  DWORD dwMajorVersion; 
  DWORD dwMinorVersion; 
  DWORD dwBuildNumber; 
  DWORD dwPlatformId; 
  TCHAR szCSDVersion[ 128 ]; 
} OSVERSIONINFO, *POSVERSIONINFO, *LPOSVERSIONINFO; 
 
typedef struct tagTEXTMETRIC { 
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
  BCHAR tmFirstChar; 
  BCHAR tmLastChar; 
  BCHAR tmDefaultChar; 
  BCHAR tmBreakChar; 
  BYTE tmItalic; 
  BYTE tmUnderlined; 
  BYTE tmStruckOut; 
  BYTE tmPitchAndFamily; 
  BYTE tmCharSet; 
} TEXTMETRIC, *LPTEXTMETRIC; 

typedef struct _OUTLINETEXTMETRIC { 
  UINT   otmSize; 
  TEXTMETRIC otmTextMetrics; 
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
} OUTLINETEXTMETRIC, *LPOUTLINETEXTMETRIC; 
 
typedef struct _OVERLAPPED { 
  DWORD  Internal; 
  DWORD  InternalHigh; 
  DWORD  Offset; 
  DWORD  OffsetHigh; 
  HANDLE hEvent; 
} OVERLAPPED, *LPOVERLAPPED; 

typedef struct tagPSD {
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
    LPCTSTR         lpPageSetupTemplateName; 
    HGLOBAL         hPageSetupTemplate; 
} PAGESETUPDLG, *LPPAGESETUPDLG; 

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
 
typedef struct _POLYTEXT { 
  int     x; 
  int     y; 
  UINT    n; 
  LPCTSTR lpstr; 
  UINT    uiFlags; 
  RECT    rcl; 
  int     *pdx; 
} POLYTEXT; 
 
typedef struct _PORT_INFO_1 { 
  LPTSTR pName; 
} PORT_INFO_1; 
 
typedef struct _PORT_INFO_2 { 
  LPSTR pPortName; 
  LPSTR pMonitorName; 
  LPSTR pDescription; 
  DWORD fPortType; 
  DWORD Reserved; 
} PORT_INFO_2; 
 
typedef struct _PREVENT_MEDIA_REMOVAL { 
  BOOLEAN PreventMediaRemoval; 
} PREVENT_MEDIA_REMOVAL ; 
 
typedef struct tagPD {  
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
  LPCTSTR    lpPrintTemplateName; 
  LPCTSTR    lpSetupTemplateName; 
  HANDLE    hPrintTemplate; 
  HANDLE    hSetupTemplate; 
} PRINTDLG PACKED, *LPPRINTDLG PACKED;
 
typedef struct _PRINTER_DEFAULTS {  
  LPTSTR      pDatatype; 
  LPDEVMODE   pDevMode; 
  ACCESS_MASK DesiredAccess; 
} PRINTER_DEFAULTS; 
 
typedef struct _PRINTER_INFO_1 { 
  DWORD  Flags; 
  LPTSTR pDescription; 
  LPTSTR pName; 
  LPTSTR pComment; 
} PRINTER_INFO_1, *PPRINTER_INFO_1, *LPPRINTER_INFO_1; 
 
typedef struct _PRINTER_INFO_2 { 
  LPTSTR    pServerName; 
  LPTSTR    pPrinterName; 
  LPTSTR    pShareName; 
  LPTSTR    pPortName; 
  LPTSTR    pDriverName; 
  LPTSTR    pComment; 
  LPTSTR    pLocation; 
  LPDEVMODE pDevMode; 
  LPTSTR    pSepFile; 
  LPTSTR    pPrintProcessor; 
  LPTSTR    pDatatype; 
  LPTSTR    pParameters; 
  PSECURITY_DESCRIPTOR pSecurityDescriptor; 
  DWORD     Attributes; 
  DWORD     Priority; 
  DWORD     DefaultPriority; 
  DWORD     StartTime; 
  DWORD     UntilTime; 
  DWORD     Status; 
  DWORD     cJobs; 
  DWORD     AveragePPM; 
} PRINTER_INFO_2; 
 
typedef struct _PRINTER_INFO_3 { 
  PSECURITY_DESCRIPTOR pSecurityDescriptor; 
} PRINTER_INFO_3; 
 
typedef struct _PRINTER_INFO_4 { 
  LPTSTR  pPrinterName; 
  LPTSTR  pServerName; 
  DWORD  Attributes; 
} PRINTER_INFO_4; 
 
typedef struct _PRINTER_INFO_5 { 
  LPTSTR    pPrinterName; 
  LPTSTR    pPortName; 
  DWORD     Attributes; 
  DWORD     DeviceNotSelectedTimeout; 
  DWORD     TransmissionRetryTimeout; 
} PRINTER_INFO_5; 
 
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
 
typedef struct _PRINTPROCESSOR_INFO_1 { 
  LPTSTR pName; 
} PRINTPROCESSOR_INFO_1; 
 
 
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
 
typedef UINT CALLBACK (*LPFNPSPCALLBACK) (HWND, UINT, LPVOID);

typedef struct _PROPSHEETPAGE { 
  DWORD     dwSize;     
  DWORD     dwFlags;    
  HINSTANCE hInstance;  
  union { 
    LPCTSTR        pszTemplate;      
    LPCDLGTEMPLATE pResource;        
  } u1; 
  union { 
    HICON  hIcon;     
    LPCTSTR pszIcon;  
  } u2; 
  LPCTSTR pszTitle;     
  DLGPROC pfnDlgProc;   
  LPARAM  lParam;       
  LPFNPSPCALLBACK pfnCallback;         
  UINT   * pcRefParent;              
} PROPSHEETPAGE,   *LPPROPSHEETPAGE; 
typedef const PROPSHEETPAGE   *LPCPROPSHEETPAGE; 
 
typedef struct _PSP *HPROPSHEETPAGE;
typedef struct _PROPSHEETHEADER { 
  DWORD      dwSize;     
  DWORD      dwFlags;    
  HWND       hwndParent; 
  HINSTANCE  hInstance;  
  union { 
    HICON  hIcon;      
    LPCTSTR pszIcon;   
  } u1; 
  LPCTSTR     pszCaption; 
  UINT       nPages; 
  union { 
    UINT  nStartPage; 
    LPCTSTR pStartPage; 
  } u2; 
  union { 
    LPCPROPSHEETPAGE    ppsp;   
    HPROPSHEETPAGE   *phpage; 
  } u3; 
  PFNPROPSHEETCALLBACK pfnCallback; 
} PROPSHEETHEADER,   *LPPROPSHEETHEADER; 
typedef const PROPSHEETHEADER   *LPCPROPSHEETHEADER; 

/* PropertySheet callbacks */
typedef WINBOOL CALLBACK (*LPFNADDPROPSHEETPAGE) (HPROPSHEETPAGE, LPARAM);
typedef WINBOOL CALLBACK (*LPFNADDPROPSHEETPAGES) (LPVOID, 
						   LPFNADDPROPSHEETPAGE, 
						   LPARAM);
 
typedef  struct _PROTOCOL_INFO {  
  DWORD  dwServiceFlags; 
  INT  iAddressFamily; 
  INT  iMaxSockAddr; 
  INT  iMinSockAddr; 
  INT  iSocketType; 
  INT  iProtocol; 
  DWORD  dwMessageSize; 
  LPTSTR  lpProtocol; 
} PROTOCOL_INFO; 
 
typedef struct _PROVIDOR_INFO_1 { 
  LPTSTR pName; 
  LPTSTR pEnvironment ; 
  LPTSTR pDLLName ; 
} PROVIDOR_INFO_1; 
 
typedef struct _PSHNOTIFY { 
  NMHDR hdr; 
  LPARAM lParam; 
} PSHNOTIFY,   *LPPSHNOTIFY; 
 
typedef struct _punctuation {
  UINT   iSize;          
  LPSTR  szPunctuation;  
} PUNCTUATION; 
 
typedef struct _QUERY_SERVICE_CONFIG { 
  DWORD dwServiceType; 
  DWORD dwStartType; 
  DWORD dwErrorControl; 
  LPTSTR lpBinaryPathName; 
  LPTSTR lpLoadOrderGroup; 
  DWORD dwTagId; 
  LPTSTR lpDependencies; 
  LPTSTR lpServiceStartName; 
  LPTSTR lpDisplayName; 
} QUERY_SERVICE_CONFIG, *LPQUERY_SERVICE_CONFIG; 
 
typedef struct _QUERY_SERVICE_LOCK_STATUS { 
  DWORD fIsLocked; 
  LPTSTR lpLockOwner; 
  DWORD dwLockDuration; 
} QUERY_SERVICE_LOCK_STATUS, *LPQUERY_SERVICE_LOCK_STATUS ; 
 
typedef  struct  _RASAMB {  
  DWORD    dwSize; 
  DWORD    dwError; 
  TCHAR    szNetBiosError[ NETBIOS_NAME_LEN + 1 ]; 
  BYTE     bLana; 
} RASAMB; 
 
typedef struct _RASCONN { 
  DWORD     dwSize; 
  HRASCONN  hrasconn; 
  TCHAR     szEntryName[RAS_MaxEntryName + 1]; 
 
  CHAR      szDeviceType[ RAS_MaxDeviceType + 1 ]; 
  CHAR      szDeviceName[ RAS_MaxDeviceName + 1 ]; 
} RASCONN ; 
 
typedef struct _RASCONNSTATUS { 
  DWORD         dwSize; 
  RASCONNSTATE  rasconnstate; 
  DWORD         dwError; 
  TCHAR         szDeviceType[RAS_MaxDeviceType + 1]; 
  TCHAR         szDeviceName[RAS_MaxDeviceName + 1]; 
} RASCONNSTATUS; 
 
typedef  struct  _RASDIALEXTENSIONS { 
  DWORD    dwSize; 
  DWORD    dwfOptions; 
  HWND    hwndParent; 
  DWORD    reserved; 
} RASDIALEXTENSIONS; 
 
typedef struct _RASDIALPARAMS { 
  DWORD  dwSize; 
  TCHAR  szEntryName[RAS_MaxEntryName + 1]; 
  TCHAR  szPhoneNumber[RAS_MaxPhoneNumber + 1]; 
  TCHAR  szCallbackNumber[RAS_MaxCallbackNumber + 1]; 
  TCHAR  szUserName[UNLEN + 1]; 
  TCHAR  szPassword[PWLEN + 1]; 
  TCHAR  szDomain[DNLEN + 1] ; 
} RASDIALPARAMS; 
 
typedef struct _RASENTRYNAME { 
  DWORD  dwSize; 
  TCHAR  szEntryName[RAS_MaxEntryName + 1]; 
}RASENTRYNAME; 
 
typedef  struct  _RASPPPIP { 
  DWORD    dwSize; 
  DWORD    dwError; 
  TCHAR    szIpAddress[ RAS_MaxIpAddress + 1 ]; 
} RASPPPIP; 
 
typedef  struct  _RASPPPIPX { 
  DWORD    dwSize; 
  DWORD    dwError; 
  TCHAR    szIpxAddress[ RAS_MaxIpxAddress + 1 ]; 
} RASPPPIPX; 
 
typedef  struct  _RASPPPNBF { 
  DWORD    dwSize; 
  DWORD    dwError; 
  DWORD    dwNetBiosError; 
  TCHAR    szNetBiosError[ NETBIOS_NAME_LEN + 1 ]; 
  TCHAR    szWorkstationName[ NETBIOS_NAME_LEN + 1 ]; 
  BYTE     bLana; 
} RASPPPNBF; 
 
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
 
typedef struct _REMOTE_NAME_INFO {
  LPTSTR  lpUniversalName; 
  LPTSTR  lpConnectionName; 
  LPTSTR  lpRemainingPath;    
} REMOTE_NAME_INFO; 
 
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
} RGNDATA, *LPRGNDATA; 
 
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
 
typedef struct _SECURITY_ATTRIBUTES { 
  DWORD  nLength; 
  LPVOID lpSecurityDescriptor; 
  WINBOOL   bInheritHandle; 
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES; 
 
typedef DWORD SECURITY_INFORMATION, *PSECURITY_INFORMATION; 
 
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
 
typedef struct _SERVICE_TABLE_ENTRY { 
  LPTSTR lpServiceName; 
  LPSERVICE_MAIN_FUNCTION lpServiceProc; 
} SERVICE_TABLE_ENTRY, *LPSERVICE_TABLE_ENTRY; 
 
typedef struct _SERVICE_TYPE_VALUE_ABS { 
  DWORD   dwNameSpace; 
  DWORD   dwValueType; 
  DWORD   dwValueSize; 
  LPTSTR  lpValueName; 
  PVOID   lpValue; 
} SERVICE_TYPE_VALUE_ABS; 
 
typedef struct _SERVICE_TYPE_INFO_ABS { 
  LPTSTR                  lpTypeName; 
  DWORD                   dwValueCount; 
  SERVICE_TYPE_VALUE_ABS  Values[1]; 
} SERVICE_TYPE_INFO_ABS; 
 
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
 
typedef struct _SET_PARTITION_INFORMATION { 
  BYTE PartitionType; 
} SET_PARTITION_INFORMATION ; 
 
typedef enum tagSHCONTF { 
  SHCONTF_FOLDERS = 32,         
  SHCONTF_NONFOLDERS = 64,      
  SHCONTF_INCLUDEHIDDEN = 128,  
} SHCONTF; 
 
typedef struct _SHFILEINFO { 
  HICON hIcon;                   
  int   iIcon;                   
  DWORD dwAttributes;            
  char  szDisplayName[MAX_PATH]; 
  char  szTypeName[80];          
} SHFILEINFO; 

typedef WORD FILEOP_FLAGS; 
typedef struct _SHFILEOPSTRUCT { 
  HWND         hwnd;                  
  UINT         wFunc;                 
  LPCSTR       pFrom;                 
  LPCSTR       pTo;                   
  FILEOP_FLAGS fFlags;                
  WINBOOL         fAnyOperationsAborted; 
  LPVOID       hNameMappings;         
  LPCSTR       lpszProgressTitle;     
} SHFILEOPSTRUCT,   *LPSHFILEOPSTRUCT; 
 
typedef enum tagSHGDN { 
  SHGDN_NORMAL = 0,           
  SHGDN_INFOLDER = 1,         
  SHGDN_FORPARSING = 0x8000,  
} SHGNO; 
 
typedef struct _SHNAMEMAPPING { 
  LPSTR pszOldPath; 
  LPSTR pszNewPath; 
  int   cchOldPath; 
  int   cchNewPath; 
} SHNAMEMAPPING,   *LPSHNAMEMAPPING; 
 
typedef struct _SID_AND_ATTRIBUTES { 
  PSID  Sid; 
  DWORD Attributes; 
} SID_AND_ATTRIBUTES ; 
 
typedef SID_AND_ATTRIBUTES SID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef SID_AND_ATTRIBUTES_ARRAY *PSID_AND_ATTRIBUTES_ARRAY;

 
typedef struct tagSOUNDSENTRY {  
  UINT cbSize; 
  DWORD dwFlags; 
  DWORD iFSTextEffect; 
  DWORD iFSTextEffectMSec; 
  DWORD iFSTextEffectColorBits; 
  DWORD iFSGrafEffect; 
  DWORD iFSGrafEffectMSec; 
  DWORD iFSGrafEffectColor; 
  DWORD iWindowsEffect; 
  DWORD iWindowsEffectMSec; 
  LPTSTR lpszWindowsEffectDLL; 
  DWORD iWindowsEffectOrdinal; 
} SOUNDSENTRY, *LPSOUNDSENTRY; 
 
typedef struct _STARTUPINFO { 
  DWORD   cb; 
  LPTSTR  lpReserved; 
  LPTSTR  lpDesktop; 
  LPTSTR  lpTitle; 
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
} STARTUPINFO, *LPSTARTUPINFO; 
 
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
 
typedef struct _SYSTEM_AUDIT_ACE { 
  ACE_HEADER  Header; 
  ACCESS_MASK Mask; 
  DWORD       SidStart; 
} SYSTEM_AUDIT_ACE; 
 
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
typedef struct SYSTEM_POWER_STATUS *LPSYSTEM_POWER_STATUS;
 
typedef struct _TAPE_ERASE { 
  ULONG Type; 
} TAPE_ERASE; 
 
typedef struct _TAPE_GET_DRIVE_PARAMETERS { 
  BOOLEAN ECC; 
  BOOLEAN Compression; 
  BOOLEAN DataPadding; 
  BOOLEAN ReportSetmarks; 
  ULONG   DefaultBlockSize; 
  ULONG   MaximumBlockSize; 
  ULONG   MinimumBlockSize; 
  ULONG   MaximumPartitionCount; 
  ULONG   FeaturesLow; 
  ULONG   FeaturesHigh; 
  ULONG   EOTWarningZoneSize; 
} TAPE_GET_DRIVE_PARAMETERS; 
 
typedef struct _TAPE_GET_MEDIA_PARAMETERS {  
  LARGE_INTEGER   Capacity; 
  LARGE_INTEGER   Remaining; 
  DWORD   BlockSize; 
  DWORD   PartitionCount; 
  BOOLEAN WriteProtected; 
} TAPE_GET_MEDIA_PARAMETERS; 
 
typedef struct _TAPE_GET_POSITION { 
  ULONG Type; 
  ULONG Partition; 
  ULONG OffsetLow; 
  ULONG OffsetHigh; 
} TAPE_GET_POSITION; 
 
typedef struct _TAPE_PREPARE { 
  ULONG Operation; 
} TAPE_PREPARE; 
 
typedef struct _TAPE_SET_DRIVE_PARAMETERS { 
  BOOLEAN ECC; 
  BOOLEAN Compression; 
  BOOLEAN DataPadding; 
  BOOLEAN ReportSetmarks; 
  ULONG   EOTWarningZoneSize; 
} TAPE_SET_DRIVE_PARAMETERS; 
 
typedef struct _TAPE_SET_MEDIA_PARAMETERS { 
  ULONG BlockSize; 
} TAPE_SET_MEDIA_PARAMETERS; 
 
typedef struct _TAPE_SET_POSITION { 
  ULONG Method; 
  ULONG Partition; 
  ULONG OffsetLow; 
  ULONG OffsetHigh; 
} TAPE_SET_POSITION; 
 
typedef struct _TAPE_WRITE_MARKS { 
  ULONG Type; 
  ULONG Count; 
} TAPE_WRITE_MARKS; 
 
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
  NMHDR hdr; 
  int iItem; 
  TBBUTTON tbButton; 
  int cchText; 
  LPTSTR pszText; 
} TBNOTIFY,  *LPTBNOTIFY; 
 
typedef struct { 
  HKEY hkr; 
  LPCTSTR pszSubKey; 
  LPCTSTR pszValueName; 
} TBSAVEPARAMS; 
 
typedef struct _TC_HITTESTINFO { 
  POINT pt;     
  UINT  flags;  
} TC_HITTESTINFO; 
 
typedef struct _TC_ITEM { 
  UINT mask;         
  UINT lpReserved1;  
  UINT lpReserved2;  
  LPTSTR pszText;     
  int cchTextMax;    
  int iImage;        
  LPARAM lParam;     
} TC_ITEM; 
 
typedef struct _TC_ITEMHEADER { 
  UINT mask;         
  UINT lpReserved1;  
  UINT lpReserved2;  
  LPTSTR pszText;     
  int cchTextMax;    
  int iImage;        
} TC_ITEMHEADER; 
 
typedef struct _TC_KEYDOWN { 
  NMHDR hdr;    
  WORD wVKey;   
  UINT flags;
} TC_KEYDOWN; 
 
typedef struct _textrange { 
  CHARRANGE chrg; 
  LPSTR lpstrText; 
} TEXTRANGE; 
 
typedef struct _TIME_ZONE_INFORMATION { 
  LONG       Bias; 
  WCHAR      StandardName[ 32 ]; 
  SYSTEMTIME StandardDate; 
  LONG       StandardBias; 
  WCHAR      DaylightName[ 32 ]; 
  SYSTEMTIME DaylightDate; 
  LONG       DaylightBias; 
} TIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION; 
 
typedef struct tagTOGGLEKEYS {   
  DWORD cbSize; 
  DWORD dwFlags; 
} TOGGLEKEYS; 
 
typedef struct _TOKEN_CONTROL { 
  LUID TokenId; 
  LUID AuthenticationId; 
  LUID ModifiedId; 
  TOKEN_SOURCE TokenSource; 
} TOKEN_CONTROL ; 
 
typedef struct _TOKEN_DEFAULT_DACL {  
  PACL DefaultDacl; 
} TOKEN_DEFAULT_DACL; 
 
typedef struct _TOKEN_GROUPS { 
  DWORD GroupCount; 
  SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY]; 
} TOKEN_GROUPS, *PTOKEN_GROUPS, *LPTOKEN_GROUPS; 
 
typedef struct _TOKEN_OWNER { 
  PSID Owner; 
} TOKEN_OWNER; 
 
typedef struct _TOKEN_PRIMARY_GROUP { 
  PSID PrimaryGroup; 
} TOKEN_PRIMARY_GROUP; 
 
typedef struct _TOKEN_PRIVILEGES { 
  DWORD PrivilegeCount; 
  LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY]; 
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES, *LPTOKEN_PRIVILEGES; 
 
typedef struct _TOKEN_STATISTICS { 
  LUID  TokenId; 
  LUID  AuthenticationId; 
  LARGE_INTEGER ExpirationTime; 
  TOKEN_TYPE    TokenType; 
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel; 
  DWORD DynamicCharged; 
  DWORD DynamicAvailable; 
  DWORD GroupCount; 
  DWORD PrivilegeCount; 
  LUID  ModifiedId; 
} TOKEN_STATISTICS; 
 
typedef struct _TOKEN_USER { 
  SID_AND_ATTRIBUTES User; 
} TOKEN_USER; 
 
typedef struct {  
  UINT      cbSize;    
  UINT      uFlags;    
  HWND      hwnd;      
  UINT      uId;       
  RECT      rect;      
  HINSTANCE hinst;     
  LPTSTR     lpszText;  
} TOOLINFO,   *PTOOLINFO,   *LPTOOLINFO; 
 
typedef struct { 
  NMHDR     hdr;        
  LPTSTR    lpszText;   
  char      szText[80]; 
  HINSTANCE hinst;      
  UINT      uFlags;     
} TOOLTIPTEXT,   *LPTOOLTIPTEXT; 
 
typedef struct tagTPMPARAMS { 
  UINT cbSize; 
  RECT rcExclude; 
} TPMPARAMS,   *LPTPMPARAMS; 
 
typedef struct _TRANSMIT_FILE_BUFFERS {  
  PVOID Head; 
  DWORD HeadLength; 
  PVOID Tail; 
  DWORD TailLength; 
} TRANSMIT_FILE_BUFFERS; 
 
typedef struct _TT_HITTESTINFO { 
  HWND hwnd;   
  POINT pt;    
  TOOLINFO ti; 
} TTHITTESTINFO,   * LPHITTESTINFO; 
 
typedef struct tagTTPOLYCURVE { 
  WORD    wType; 
  WORD    cpfx; 
  POINTFX apfx[1]; 
} TTPOLYCURVE,  * LPTTPOLYCURVE; 
 
typedef struct _TTPOLYGONHEADER { 
  DWORD   cb; 
  DWORD   dwType; 
  POINTFX pfxStart; 
} TTPOLYGONHEADER,  * LPTTPOLYGONHEADER; 
 
typedef struct _TV_DISPINFO { 
  NMHDR   hdr;  
  TV_ITEM item;  
} TV_DISPINFO; 
 
typedef struct _TVHITTESTINFO { 
  POINT     pt;     
  UINT      flags;  
  HTREEITEM hItem;  
} TV_HITTESTINFO,   *LPTV_HITTESTINFO; 
 
typedef struct _TV_INSERTSTRUCT { 
  HTREEITEM hParent;       
  HTREEITEM hInsertAfter;  
  TV_ITEM   item;          
} TV_INSERTSTRUCT,   *LPTV_INSERTSTRUCT; 
 
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
  
typedef struct _UNIVERSAL_NAME_INFO { 
  LPTSTR  lpUniversalName; 
} UNIVERSAL_NAME_INFO; 
 
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
 
typedef struct _WIN32_FIND_DATA { 
  DWORD dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  TCHAR    cFileName[ MAX_PATH ]; 
  TCHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA, *PWIN32_FIND_DATA; 
 
typedef struct _WIN32_STREAM_ID { 
  DWORD dwStreamId; 
  DWORD dwStreamAttributes; 
  LARGE_INTEGER Size; 
  DWORD dwStreamNameSize; 
  WCHAR *cStreamName ; 
} WIN32_STREAM_ID;
 
typedef struct _WINDOWPLACEMENT {  
  UINT  length;               
  UINT  flags;                
  UINT  showCmd;              
  POINT ptMinPosition;        
  POINT ptMaxPosition;        
  RECT  rcNormalPosition;     
} WINDOWPLACEMENT; 
 
typedef struct _WNDCLASS {  
  UINT    style; 
  WNDPROC lpfnWndProc; 
  int     cbClsExtra; 
  int     cbWndExtra; 
  HANDLE  hInstance; 
  HICON   hIcon; 
  HCURSOR hCursor; 
  HBRUSH  hbrBackground; 
  LPCTSTR lpszMenuName; 
  LPCTSTR lpszClassName; 
} WNDCLASS, *LPWNDCLASS; 
 
typedef struct _WNDCLASSEX { 
  UINT    cbSize; 
  UINT    style; 
  WNDPROC lpfnWndProc; 
  int     cbClsExtra; 
  int     cbWndExtra; 
  HANDLE  hInstance; 
  HICON   hIcon; 
  HCURSOR hCursor; 
  HBRUSH  hbrBackground; 
  LPCTSTR lpszMenuName; 
  LPCTSTR lpszClassName; 
  HICON   hIconSm; 
} WNDCLASSEX, *LPWNDCLASSEX; 

typedef struct _CONNECTDLGSTRUCT {
  DWORD cbStructure;
  HWND hwndOwner;
  LPNETRESOURCE lpConnRes;
  DWORD dwFlags;
  DWORD dwDevNum;
} CONNECTDLGSTRUCT, *LPCONNECTDLGSTRUCT;

typedef struct _DISCDLGSTRUCT {
  DWORD           cbStructure;
  HWND            hwndOwner;
  LPTSTR           lpLocalName;
  LPTSTR           lpRemoteName;
  DWORD           dwFlags;
} DISCDLGSTRUCT, *LPDISCDLGSTRUCT;

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

typedef int CALLBACK (*ENUMMETAFILEPROC) (HDC, HANDLETABLE, 
					  METARECORD, int, LPARAM);
typedef int CALLBACK (*ENHMETAFILEPROC) (HDC, HANDLETABLE, 
					 ENHMETARECORD, int, LPARAM);

typedef int CALLBACK (*ENUMFONTSPROC) (LPLOGFONT, LPTEXTMETRIC, DWORD, LPARAM);
typedef int CALLBACK (*FONTENUMPROC) (ENUMLOGFONT *, NEWTEXTMETRIC *, 
				      int, LPARAM);
typedef int CALLBACK (*FONTENUMEXPROC) (ENUMLOGFONTEX *, NEWTEXTMETRICEX *, 
				      int, LPARAM);

typedef VOID CALLBACK (*LPOVERLAPPED_COMPLETION_ROUTINE) (DWORD, DWORD, 
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

#ifndef WIN32_NO_PEHDR

typedef struct
{
  WORD e_magic;		/* Magic number, 0x5a4d */
  WORD e_cblp;		/* Bytes on last page of file, 0x90 */
  WORD e_cp;		/* Pages in file, 0x3 */
  WORD e_crlc;		/* Relocations, 0x0 */
  WORD e_cparhdr;	/* Size of header in paragraphs, 0x4 */
  WORD e_minalloc;	/* Minimum extra paragraphs needed, 0x0 */
  WORD e_maxalloc;	/* Maximum extra paragraphs needed, 0xFFFF */
  WORD e_ss;		/* Initial (relative) SS value, 0x0 */
  WORD e_sp;		/* Initial SP value, 0xb8 */
  WORD e_csum;		/* Checksum, 0x0 */
  WORD e_ip;		/* Initial IP value, 0x0 */
  WORD e_cs;		/* Initial (relative) CS value, 0x0 */
  WORD e_lfarlc;	/* File address of relocation table, 0x40 */
  WORD e_ovno;		/* Overlay number, 0x0 */
  WORD e_res[4];	/* Reserved words, all 0x0 */
  WORD e_oemid;		/* OEM identifier (for e_oeminfo), 0x0 */
  WORD e_oeminfo;	/* OEM information; e_oemid specific, 0x0 */
  WORD e_res2[10];	/* Reserved words, all 0x0 */
  LONG e_lfanew;	/* File address of new exe header, 0x80 */
  /* We leave out the next two fields, since they aren't in the
     Windows header file.  */
  /* DWORD dos_message[16]; * text which always follows dos header */
  /* DWORD nt_signature;    * required NT signature, 0x4550 */ 
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#endif /* WIN32_NO_PEHDR */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WIN32_LEAN_AND_MEAN */

#endif /* _GNU_H_WINDOWS32_STRUCTURES */

