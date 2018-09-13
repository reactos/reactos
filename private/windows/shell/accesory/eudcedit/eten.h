//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
/*
CWIN31 EUDC Format and ETEN format
The structures are not aligned to word order. For the reason, it is need to use
'char' array base structures for the purpose of R/W files.
For the further item, the word is strored in Intel monor, use set/get functions.

+---------------------------+
| File Header  256 bytes    | -----> USERFONTHEADER
+---------------------------+
| Code Bank ID  2 bytes     | ~|
+---------------------------+  |---> CODEELEMENT
| DBCS Code    2 bytes      | _|
+---------------------------+
| Patten     ?? bytes       | -----> Glyph Patten depend on width and height
+---------------------------+
.              .
.              .
.              .
+---------------------------+
| Code Bank ID  2 bytes     |
+---------------------------+
| DBCS Code    2 bytes      |
+---------------------------+
| Patten     ?? bytes       |	---> Byte Boundary,remaining bits are not stable
+---------------------------+
*/

#ifdef BUILD_ON_WINNT
#pragma pack(1)
#endif // BUILD_ON_WINNT

struct CFONTINFO {
unsigned short	uInfoSize;		// Size of this structure.
unsigned short  idCP;			// Code page ID 938 for Taiwan.
	char	idCharSet;		// Character set is CHINESEBIG5_CHARSET
	char	fbTypeFace;  		// Type face.
	char	achFontName[12];	// Font Name.
unsigned long	ulCharDefine;		// Number of usable characters.
unsigned short  uCellWidth;		// Width of font cell.
unsigned short  uCellHeight;		// Height of font cell.
unsigned short  uCharHeight;		// Height of character height.
unsigned short  uBaseLine;		//
unsigned short  uUnderLine;		//
unsigned short  uUnlnHeight;		// Height of underline.
	char	fchStrokeWeight;	// Weight of font (Bold or Thin)
unsigned short  fCharStyle;		// italy
	char	fbFontAttrib;		//
unsigned long	ulCellWidthMax;		// Max width of font cell.
unsigned long	ulCellHeightMax;	// Max height of font cell.
	};

struct ETENHEADER {
unsigned short  uHeaderSize;	 	// Size of this structure.
	char	idUserFontSign[8];	// Must be "CWIN_PTN", "CMEX_PTN"
	char	idMajor;		// Version number if it is 1.0 then
	char	idMinor;		// idMajor is 1 and idMinor is 0
unsigned long	ulCharCount;		// Number of characters in file
unsigned short  uCharWidth;		// Width of the character.
unsigned short  uCharHeight;		// Height of the character.
unsigned long	cPatternSize;		// size of pattern in byte.
	char	uchBankID;	  	// if data is in same bank.
unsigned short  idInternalBankID;	// Internal code bank ID.
	char	achReserved1[37];	// must be zero.
struct	CFONTINFO  sFontInfo;		// chinese font structure.
	char	achReserved2[18];	// must be zero.
	char	achCopyRightMsg[128];	// Copyright message.
	};

struct CODEELEMENT {
	unsigned short   nBankID;		// BankID
	unsigned short   nInternalCode;		// Internal Code
	};


struct R_CFONTINFO {
unsigned char   uInfoSize[2];		// Size of this structure.
unsigned char   idCP[2];		// Code page ID 938 for Taiwan.
	char	idCharSet;		// Character set is CHINESEBIG5_CHARSET
	char	fbTypeFace;		// Type face.
	char	achFontName[12];	// Font Name.
unsigned char	ulCharDefine[4];	// Number of usable characters.
unsigned char   uCellWidth[2];		// Width of font cell.
unsigned char   uCellHeight[2];		// Height of font cell.
unsigned char   uCharHeight[2];		// Height of character height.
unsigned char   uBaseLine[2];		//
unsigned char   uUnderLine[2]; 		//
unsigned char   uUnlnHeight[2];		// Height of underline.
	char	fchStrokeWeight;	// Weight of font (Bold or Thin)
unsigned char   fCharStyle[2];		// italy
	char	fbFontAttrib;		//
unsigned char	ulCellWidthMax[4];	// Max width of font cell.
unsigned char	ulCellHeightMax[4];	// Max height of font cell.
	};

struct R_ETENHEADER {
unsigned char	uHeaderSize[2];		// Size of this structure.
	char	idUserFontSign[8];	// Must be "CWIN_PTN", "CMEX_PTN"
	char	idMajor;		// Version number if it is 1.0 then
	char	idMinor;		// idMajor is 1 and idMinor is 0
unsigned char	ulCharCount[4];		// Number of characters in file
unsigned char	uCharWidth[2];		// Width of the character.
unsigned char	uCharHeight[2];		// Height of the character.
unsigned char	cPatternSize[4];	// size of pattern in byte.
	char	uchBankID;		// if data is in same bank.
unsigned char	idInternalBankID[2];	// Internal code bank ID.
	char	achReserved1[37];	// must bezero.
struct R_CFONTINFO sFontInfo;		// chinese font structure.
	char	achReserved2[18]; 	// must be zero.
	char	achCopyRightMsg[128];	// Copyright message.
	};

struct R_CODEELEMENT {
	unsigned char   nBankID[2];		// BankID
	unsigned char   nInternalCode[2];		// Internal Code
	};

/* Open Mode */
#define	ETEN_READ	0
#define	ETEN_WRITE	1

int  openETENBMP(TCHAR  *path,int  md);
int  closeETENBMP(void);
int  createETENBMP(TCHAR  *path,int  width,int  height);
int  readETENBMPRec(int  rec,LPBYTE buf,int  bufsiz,unsigned short  *code);
int  appendETENBMP(LPBYTE buf,unsigned short  code);

#ifdef BUILD_ON_WINNT
#pragma pack()
#endif // BUILD_ON_WINNT
/* EOF */
