/********************************************************************
 *
 *  Header Name : font.h 
 *  Font structures defines for MLang fontlink2
 *
 ********************************************************************/

#ifndef __FONT_H__
#define __FONT_H__

#define  TWO_BYTE_NUM(p)   (((p[0])<<8)|(p[1]))
#define  FOUR_BYTE_NUM(p)  (((p[0])<<24)|((p[1])<<16)|((p[2])<<8)|(p[3]))
#define  OFFSET_OS2CPRANGE sizeof(SHORT) * 24 + sizeof(PANOSE) + sizeof(ULONG) * 4 + sizeof(CHAR) * 4 
#define  MAX_FONT_FILE_NAME     48
#define  FONT_TABLE_INIT_SIZE   100
#define  FONT_DATA_FILE_NAME    TEXT("mlfcache.dat")
#define  FONT_DATA_SIGNATURE    "mlang font data"
#define  REGFONTKEYNT           TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts")
#define  REGFONTKEY95           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Fonts")
#define  FONT_FOLDER            TEXT("fonts")
#define  MAX_FONT_INDEX         30
#define  FONTDATATABLENUM       2

// Font database file format:
// Table header
// Table Entry [Table Entry]
// Table data [Table data]
typedef struct tagFontDataHeader{
    char    FileSig[16];
    DWORD   dwVersion;
    DWORD   dwCheckSum;
    WORD    nTable;
} FONTDATAHEADER;

typedef struct tagFontTable{
    char    szName[4];
    DWORD   dwOffset;
    DWORD   dwSize;
} FONTDATATABLE;

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
#define FONT_NAME           4
#define MICROSOFT_PLATFORM  3
#define UNICODE_INDEXING    1
#define CMAP_FORMAT_FOUR    4
#define APPLE_UNICODE_PLATFORM  0
#define APPLE_UNICODE_INDEXING  3
#define UNICODE_SYMBOL_INDEXING 0


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
  BYTE  Platform[2];  // = 3 if Microsoft
  BYTE  Encoding[2];  // = 1 if string is in Unicode
  BYTE  Offset  [4];
} OS2_TABLE;

typedef struct {
  BYTE  Format       [2];  // must be 4
  BYTE  Length       [2];
  BYTE  Version      [2];
  BYTE  SegCountX2   [2];
  BYTE  SeachgRange  [2];
  BYTE  EntrySelector[2];
  BYTE  RangeShift   [2];
} CMAP_FORMAT;

// font table
typedef struct tagFontInfo{
    TCHAR   szFaceName[LF_FACESIZE];
    TCHAR   szFileName[MAX_FONT_FILE_NAME];
    DWORD   dwCodePages[2];
    LOGFONT lf;
    DWORD   dwUniSubRanges[4];
    SCRIPT_IDS scripts;
} FONTINFO;

typedef struct tagSCRIPT
{
    SCRIPT_ID   sid;
    UINT        uidDescription;     // script name (localization needed)
    UINT        uiCodePage;         // can be NULL to indicate no Windows code pages 
    WCHAR       wcCandidate;        // primary candiate for the script
    UINT        uidFixedWidthFont;  // default fixed width font (localization needed)
    UINT        uidProportionalFont;// default proportional font (localization needed)
    DWORD       dwFlags;            // script level flag
} SCRIPT;

//Unicode range table
typedef struct tagURangeFont{
    WCHAR wcFrom;
    WCHAR wcTo;
    int   nFonts;
    int   *pFontIndex;
} URANGEFONT;

extern UINT     g_cScript;

extern const    SCRIPT ScriptTable[];

//Script 


#endif  // __FONT_H__
