/********************************************************************
 *
 *  Header Name : uce.h
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation. 
 *
 ********************************************************************/

#ifndef __UCE_H__
#define __UCE_H__

#include "cmdlg.h"
#include "ucefile.h"

#define BTOC(bytes)      ((bytes) / sizeof(TCHAR))
#define CTOB(cch)        ((cch) * sizeof(TCHAR))
#define UCHAR            unsigned char
#define UTCHAR           unsigned short

#define ASCII_BEG        0x21
#define ASCII_END        0x7F

#define HIANSI_BEG       0x80
#define HIANSI_END       0xFF

#define TRAILBYTE_BEG    0x40
#define TRAILBYTE_END    0xFE

#define DELETE_CHAR      0x7f
#define UNICODE_CODEPAGE 1200

#define cchFullMap       (200)

#define  TWO_BYTE_NUM(p)                           (((p[0])<<8)|(p[1]))
#define FOUR_BYTE_NUM(p) (((p[0])<<24)|((p[1])<<16)|((p[2])<<8)|(p[3]))

// Font types

#define TRUETYPE_FONT         0x00000004
#ifndef PS_OPENTYPE_FONTTYPE
#define PS_OPENTYPE_FONTTYPE  0x00010000
#endif
#ifndef TT_OPENTYPE_FONTTYPE
#define TT_OPENTYPE_FONTTYPE  0x00020000
#endif
#ifndef TYPE1_FONTTYPE
#define TYPE1_FONTTYPE        0x00040000
#endif
#define EUDC_FONTTYPE         0x10000000
#define SYMBOL_FONTTYPE       0x20000000
#define OEM_FONTTYPE          0x40000000
#define DBCS_FONTTYPE         0x80000000

//should be hardcoded but let's lock it during localization.
//we'll hardcode it after Win2K
//#define RTFFMT                TEXT("Rich Text Format")

typedef struct {
  char  TTCTag    [4];
  BYTE  Version   [4];
  BYTE  DirCount  [4];
  BYTE  OffsetTTF1[4];
} TTC_HEAD;

typedef struct {
  BYTE  Version      [4];
  BYTE  NumTables    [2];
  BYTE  SearchRange  [2];
  BYTE  EntrySelector[2];
  BYTE  RangeShift   [2];
} TTF_HEAD;

typedef struct {
  char  Tag     [4];
  BYTE  CheckSum[4];
  BYTE  Offset  [4];
  BYTE  Length  [4];
} TABLE_DIR;

typedef struct {
  BYTE  Format[2];
  BYTE  NumRec[2];
  BYTE  Offset[2];
} NAME_TABLE;

#define FONT_SUBFAMILY_NAME 2
#define MICROSOFT_PLATFORM  3
#define UNICODE_INDEXING    1
#define CMAP_FORMAT_FOUR    4

typedef struct {
  BYTE  Platform[2];
  BYTE  Encoding[2];  // = 1 if string is in Unicode
  BYTE  LangID  [2];
  BYTE  NameID  [2];  // = 2 for font subfamily name
  BYTE  Length  [2];
  BYTE  Offset  [2];
} NAME_RECORD;

typedef struct {
  BYTE  Version  [2];
  BYTE  NumTables[2];
} CMAP_HEAD;

typedef struct {
  BYTE  Platform[2];  // = 3 if Microsoft
  BYTE  Encoding[2];  // = 1 if string is in Unicode
  BYTE  Offset  [4];
} CMAP_TABLE;

typedef struct {
  BYTE  Format       [2];  // must be 4
  BYTE  Length       [2];
  BYTE  Version      [2];
  BYTE  SegCountX2   [2];
  BYTE  SeachgRange  [2];
  BYTE  EntrySelector[2];
  BYTE  RangeShift   [2];
} CMAP_FORMAT;

typedef struct {
  WCHAR wcFrom;
  WCHAR wcTo;
} URANGE;

typedef struct tagSYCM
  {
    INT dxpBox;
    INT dypBox;
    INT dxpCM;
    INT dypCM;
    INT xpCh;
    INT ypCh;
    INT dxpMag;
    INT dypMag;
    INT xpMagCurr;
    INT ypMagCurr;
    INT ypDest;
    INT xpCM;
    INT ypCM;
    INT CPgNum;
    INT ChWidth;

    BOOL fHasFocus;
    BOOL fFocusState;
    BOOL fMouseDn;
    BOOL fCursorOff;
    BOOL fMagnify;
    BOOL fAnsiFont;
    UTCHAR chCurr;
    HFONT hFontMag;
    HFONT hFont;
    HDC hdcMag;
    HBITMAP hbmMag;
    INT rgdxp[256];
  } SYCM;
typedef SYCM *PSYCM;

typedef struct tagITEMDATA
  {
    SHORT FontType;
    BYTE CharSet;
    BYTE PitchAndFamily;
  } ITEMDATA;

#define LF_SUBSETSIZE    128
#define LF_CODEPAGESIZE  128
#define LF_EUDCFACESIZE  256 // wingdi defines length of FACESIZE as 32 chars.
                             // charmap attaches string "Private Characters" to
                             // face name. 256 should be long enough to handle
                             // face names. 

typedef struct tagUSUBSET
  {
    INT BeginRange;
    INT EndRange;
    INT StringResId;
    TCHAR Name[LF_SUBSETSIZE];
  } USUBSET;

typedef struct tagUCODEPAGE
  {
    INT BeginRange;
    INT EndRange;
    INT StringResId;
    TCHAR Name[LF_CODEPAGESIZE];
  } UCODEPAGE;

#define MAX_LEN 50

typedef struct _tagFontInfo {
    TCHAR  szFaceName[LF_EUDCFACESIZE];
    BYTE   CharSet;
    DWORD  FontType;
    BYTE   PitchAndFamily;
    URANGE *pUniRange;
    UINT   nNumofUniRange;
} FONTINFO,*LPFONTINFO;

typedef struct tagValidateData {
    int nControlId;
    int iMinValue;
    int iMaxValue;
    int iDefaultValue;
    int iMaxChars;
} ValidateData;

/* Function declarations. */

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, INT);
LRESULT  APIENTRY UCEWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT  APIENTRY CharGridWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MsgProc(HWND, UINT, WPARAM, LPARAM);
UINT ChFromSymLParam(PSYCM, LPARAM);
VOID DrawSymChOutlineHwnd(PSYCM, HWND, UTCHAR, BOOL, BOOL);
VOID RecalcUCE(HWND, PSYCM, INT, BOOL);
VOID DrawSymbolMap(PSYCM, HDC);
VOID DrawSymbolGrid(PSYCM, HDC);
VOID DrawSymbolChars(PSYCM, HDC);
VOID DrawSymChOutline(PSYCM, HDC, UTCHAR, BOOL, BOOL);
VOID MoveSymbolSel(PSYCM, UTCHAR, BOOL fRepaint=FALSE);
VOID RestoreSymMag(HWND, PSYCM);
HANDLE GetEditText(HWND);
INT PointsToHeight(INT);

VOID PaintStatusLine(HDC, BOOL, BOOL);
VOID UpdateKeystrokeText( HDC hdc, UTCHAR chNew, BOOL fRedraw);

INT KeyboardVKeyFromChar(UTCHAR);
BOOL DrawFamilyComboItem(LPDRAWITEMSTRUCT);
HBITMAP LoadBitmaps(INT);
VOID DoHelp(HWND, BOOL);

VOID ExitMagnify(HWND, PSYCM);

BOOL CALLBACK SubSetDlgProc(HWND, UINT, WPARAM, LPARAM);

VOID UpdateSymbolSelection(HWND, BOOL);

VOID UpdateSymbolRange( HWND hwnd, INT FirstChar, INT LastChar );

VOID SubSetChanged( HWND hwnd);

DWORD GetCurFontCharSet(HWND hWnd);
int ConvertAnsifontToUnicode(HWND hWnd, char* mb, WCHAR* wc); 
int ConvertUnicodeToAnsiFont(HWND hWnd, WCHAR* wc, char* mb); 
VOID ProcessScrollMsg( HWND hwnd, int nCode, int nPos );
INT ScrollMapPage( HWND hwndDlg, BOOL fUp, BOOL fRePaint );
BOOL ScrollMap( HWND hwndDlg, INT cchScroll, BOOL fRePaint );
void SetEditCtlFont( HWND hwndDlg, int idCtl, HFONT hfont );
void GetFonts(void);
LONG WCharCP(UINT CodePage, WCHAR *lpWC);
URANGE* EUDC_Range(TCHAR* Path, DWORD* pSize);
void UnicodeBar(void);
int ShowHideAdvancedControls(HWND hWnd, WPARAM wParam, LPARAM lParam);
void ComputeExpandedAndShrunkHeight(HWND hDlg);
void ResizeDialog(HWND hWnd);

void CopyTextToClipboard(HWND hWndDlg);
void SetRichEditFont(HWND hwndDlg, int idCtl, HFONT hFont);
int DoDragScroll(HWND hWnd, WPARAM wParam, LPARAM lParam);
int UnicodeRangeChecked(HWND hWnd);
int EnableSURControls(HWND hWnd, BOOL fForceDisable=FALSE);
int SURangeChanged(HWND hWnd);
static BOOL SetHexEditProc(HWND hWndEdit);
static LRESULT CALLBACK HexEditControlProc(HWND hWnd,UINT uMessage, WPARAM  wParam, LPARAM  lParam);
static BOOL SetSearchEditProc(HWND hWndEdit);
static LRESULT CALLBACK SearchEditControlProc(HWND hWnd,UINT uMessage, WPARAM  wParam, LPARAM  lParam);
int ValidateValues(HWND hWnd);
void ResizeTipsArea(HWND hWndGrid, PSYCM psycm);

extern HINSTANCE hInst;
extern HWND hwndDialog;
extern HWND hwndCharGrid;

extern HWND ghwndList;
extern HWND ghwndGrid;
extern BOOL fDisplayAdvControls;
extern LPWSTR pCode;
extern SYCM   sycm;
extern int    nDragDelay, nDragMinDist;
extern FONTINFO *Font_pList;


#define M64K 65536


BOOL CodePage_InitList();
BOOL CodePage_DeleteList();
BOOL CodePage_AddToList(LONG);
BOOL CodePage_FillToComboBox(HWND ,UINT);
LONG CodePage_GetCurSelCodePage(HWND,UINT);
LONG CodePage_GetCurCodePageVal();
BOOL CodePage_SetCurrent( LONG  , HWND  , UINT  );
BOOL IsCodePageOnList( WORD );

BOOL Display_DeleteList();
BOOL Display_InitList();
LPWSTR Display_CreateDispBuffer(LPWSTR,INT,URANGE *,INT,BOOL);
LPWSTR Display_CreateSubsetDispBuffer(LPWSTR,INT,URANGE *,INT,BOOL,int,int);

BOOL Subset_FillComboBox( HWND hWnd , UINT uID );
BOOL Subset_GetUnicodeCharsToDisplay(HWND,UINT,LONG,LPWSTR *,UINT *,BOOL *);
BOOL GetUnicodeBufferOfCodePage( LONG , PWSTR *, UINT *) ;
BOOL Subset_GetUnicode( HWND, PUCE_MEMORY_FILE , PWSTR * , UINT *, BOOL *);
BOOL GetUnicodeCharsFromList( HWND , PUCE_MEMORY_FILE , PWSTR , UINT * , BOOL *);
BOOL Subset_OnSelChange( HWND hWnd , UINT uID ) ;

BOOL Font_InitList(HWND hWnd);
BOOL Font_DeleteList();
BOOL Font_AddToList(LPLOGFONT,DWORD,URANGE *,INT);
BOOL Font_FillToComboBox(HWND ,UINT);
BYTE Font_GetSelFontCharSet(HWND,UINT,INT);
BOOL Font_SelectByCharSet(HWND,UINT,UINT);
BOOL Font_GetCharWidth32(HDC ,UINT ,UINT ,LPINT, LPWSTR);
BOOL Font_GetCurCMapTable(HWND,UINT,URANGE **,UINT*);
BOOL Font_Avail(UINT);
UINT Font_DBCS_CharSet();
UINT CharSetToCodePage(BYTE cs);
DWORD URanges(UINT CodePage, URANGE *pUR);


// Grid & List Window

HWND CreateListWindow( HWND  , PWSTR  );
void CreateResources( HINSTANCE, HWND );
void DeleteResources( void );
LRESULT CALLBACK ListWndProc( HWND , UINT , WPARAM  , LPARAM );
LRESULT CALLBACK GridWndProc( HWND , UINT , WPARAM  , LPARAM );
void FillGroupsInListBox( HWND , PUCE_MEMORY_FILE ) ;
void DestroyListWindow( void );
void DestroyGridWindow( void );
void DestroyAllListWindows( void ) ;
BOOL IsListWindow( PUCE_MEMORY_FILE );
HWND CreateGridWindow( HWND hwndParent , UINT uID , PUCE_MEMORY_FILE pUceMemFile );
void GetWindowGridSize( PUCE_MEMORY_FILE  , POINT * , INT * , INT * );
BOOL GridHScroll( HWND  , UINT  , WPARAM  , LPARAM  );
BOOL GridVScroll( HWND  , UINT  , WPARAM  , LPARAM  );
void DrawGridCell( HDC  , RECT * , BOOL  , WCHAR  , BOOL );
void DoPaint( HWND  , HDC  );
__inline BOOL GetCurrentRect( RECT * );
__inline void GetCurrentGroupChar( PWSTR );
__inline BOOL GridSamePointHit( POINT );
BOOL GetUnicodeCharsFromGridWindow( HWND , PWSTR  , UINT * , BOOL *);
BOOL IsGridWindowAlive( void );
BOOL CreateNewGridFont( HWND  , UINT  );
BOOL CreateNewListFont( HWND  , UINT  );
BOOL UpdateGridFont( HWND  , UINT  );
BOOL UpdateListFont( HWND  , UINT  );
BOOL IsAnyListWindow( void );
void GetWChars( INT  , WCHAR * , WCHAR * , UINT * , BOOL *);
__inline BOOL IsCellEnabled( INT  , INT  );

INT  LoadCurrentSelection(HWND, UINT, LPTSTR, LPTSTR);
BOOL SaveCurrentSelection(HWND, UINT, LPTSTR);
VOID GetSystemPathName(PWSTR ,PWSTR ,UINT );
INT  LoadAdvancedSelection(HWND, UINT, LPTSTR);
BOOL SaveAdvancedSelection(HWND, UINT,LPTSTR);

BOOL LoadNeedMessage();
void SaveNeedMessage(BOOL nMsg);

void SetSearched(void);

#define SZ_CODEPAGE         TEXT("CodePage")
#define SZ_CODEPAGE_DEFAULT TEXT("1200")
#define SZ_SUBSET           TEXT("Subset")
#define SZ_SUBSET_DEFAULT   TEXT("All")         // cannot be macro ???
#define SZ_SUBFUNC          TEXT("SubFunc")
#define SZ_SUBFUNC_DEFAULT  TEXT("0")           // cannot be macro ???
#define SZ_FONT             TEXT("Font")
#define SZ_FONT_DEFAULT     TEXT("Arial")       // cannot be macro ???
#define SZ_ADVANCED         TEXT("Advanced")

#define ID_SUBFUNCCHANGED   501

// If debug functionality is needed turn to 1
//#define DBG 0

#ifdef _DEBUG
#define TRACE    OutputDebugString

#else // _DEBUG

#define TRACE    NOP_FUNCTION

#endif // _DEBUG
#endif  // __UCE_H__
