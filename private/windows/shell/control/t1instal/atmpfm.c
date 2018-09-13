/*--------------------------------------------------------------------------*/
/* WINATM version only                                                      */
/*--------------------------------------------------------------------------*/
//
//      Copyright 1990, 1991 -- Adobe Systems, Inc.
//      PostScript is a trademark of Adobe Systems, Inc.
//
// NOTICE:  All information contained herein or attendant hereto is, and
// remains, the property of Adobe Systems, Inc.  Many of the intellectual
// and technical concepts contained herein are proprietary to Adobe Systems,
// Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
// are protected as trade secrets.  Any dissemination of this information or
// reproduction of this material are strictly forbidden unless prior written
// permission is obtained from Adobe Systems, Inc.
//
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <io.h>

#include "windows.h"
#pragma pack(1)
#include "makepfm.h"
#pragma pack(4)

#include "fvscodes.h"  // FVS_xxxxxx (font validation status) codes and macros.

#ifdef WIN30
  #define LPCSTR LPSTR
#endif

typedef LPSZ GlyphName;
extern  AFM  afm;

// bodind added these

#define str_DotINF ".INF"
#define str_DotPFM ".PFM"

/*--------------------------------------------------------------------------*/
static CHAR msgbuff[128];
static HANDLE hMemArray[258];  // #_of_glyphnames + 1_glypharray + 1_kernpairs
static INT indexMemArray = 0;
static GlyphName winEnc[] =
/*  8-27-91 yh  Added some characters for Windows 3.1 */
{   /*   0 */    "",
    /*   1 */    "",
    /*   2 */    "",
    /*   3 */    "",
    /*   4 */    "",
    /*   5 */    "",
    /*   6 */    "",
    /*   7 */    "",
    /*   8 */    "",
    /*   9 */    "",
    /*  10 */    "",
    /*  11 */    "",
    /*  12 */    "",
    /*  13 */    "",
    /*  14 */    "",
    /*  15 */    "",
    /*  16 */    "",
    /*  17 */    "",
    /*  18 */    "",
    /*  19 */    "",
    /*  20 */    "",
    /*  21 */    "",
    /*  22 */    "",
    /*  23 */    "",
    /*  24 */    "",
    /*  25 */    "",
    /*  26 */    "",
    /*  27 */    "",
    /*  28 */    "",
    /*  29 */    "",
    /*  30 */    "",
    /*  31 */    "",
    /*  32 */    "space",
    /*  33 */    "exclam",
    /*  34 */    "quotedbl",
    /*  35 */    "numbersign",
    /*  36 */    "dollar",
    /*  37 */    "percent",
    /*  38 */    "ampersand",
    /*  39 */    "quotesingle",
    /*  40 */    "parenleft",
    /*  41 */    "parenright",
    /*  42 */    "asterisk",
    /*  43 */    "plus",
    /*  44 */    "comma",
    /*  45 */    "hyphen",
    /*  46 */    "period",
    /*  47 */    "slash",
    /*  48 */    "zero",
    /*  49 */    "one",
    /*  50 */    "two",
    /*  51 */    "three",
    /*  52 */    "four",
    /*  53 */    "five",
    /*  54 */    "six",
    /*  55 */    "seven",
    /*  56 */    "eight",
    /*  57 */    "nine",
    /*  58 */    "colon",
    /*  59 */    "semicolon",
    /*  60 */    "less",
    /*  61 */    "equal",
    /*  62 */    "greater",
    /*  63 */    "question",
    /*  64 */    "at",
    /*  65 */    "A",
    /*  66 */    "B",
    /*  67 */    "C",
    /*  68 */    "D",
    /*  69 */    "E",
    /*  70 */    "F",
    /*  71 */    "G",
    /*  72 */    "H",
    /*  73 */    "I",
    /*  74 */    "J",
    /*  75 */    "K",
    /*  76 */    "L",
    /*  77 */    "M",
    /*  78 */    "N",
    /*  79 */    "O",
    /*  80 */    "P",
    /*  81 */    "Q",
    /*  82 */    "R",
    /*  83 */    "S",
    /*  84 */    "T",
    /*  85 */    "U",
    /*  86 */    "V",
    /*  87 */    "W",
    /*  88 */    "X",
    /*  89 */    "Y",
    /*  90 */    "Z",
    /*  91 */    "bracketleft",
    /*  92 */    "backslash",
    /*  93 */    "bracketright",
    /*  94 */    "asciicircum",
    /*  95 */    "underscore",
    /*  96 */    "grave",
    /*  97 */    "a",
    /*  98 */    "b",
    /*  99 */    "c",
    /* 100 */    "d",
    /* 101 */    "e",
    /* 102 */    "f",
    /* 103 */    "g",
    /* 104 */    "h",
    /* 105 */    "i",
    /* 106 */    "j",
    /* 107 */    "k",
    /* 108 */    "l",
    /* 109 */    "m",
    /* 110 */    "n",
    /* 111 */    "o",
    /* 112 */    "p",
    /* 113 */    "q",
    /* 114 */    "r",
    /* 115 */    "s",
    /* 116 */    "t",
    /* 117 */    "u",
    /* 118 */    "v",
    /* 119 */    "w",
    /* 120 */    "x",
    /* 121 */    "y",
    /* 122 */    "z",
    /* 123 */    "braceleft",
    /* 124 */    "bar",
    /* 125 */    "braceright",
    /* 126 */    "asciitilde",
    /* 127 */    "",
    /* 128 */    "",
    /* 129 */    "",
    /* 130 */    "quotesinglbase",
    /* 131 */    "florin",
    /* 132 */    "quotedblbase",
    /* 133 */    "ellipsis",
    /* 134 */    "dagger",
    /* 135 */    "daggerdbl",
    /* 136 */    "circumflex",
    /* 137 */    "perthousand",
    /* 138 */    "Scaron",
    /* 139 */    "guilsinglleft",
    /* 140 */    "OE",
    /* 141 */    "",
    /* 142 */    "",
    /* 143 */    "",
    /* 144 */    "",
    /* 145 */    "quoteleft",
    /* 146 */    "quoteright",
    /* 147 */    "quotedblleft",
    /* 148 */    "quotedblright",
    /* 149 */    "bullet",
    /* 150 */    "endash",
    /* 151 */    "emdash",
    /* 152 */    "tilde",
    /* 153 */    "trademark",
    /* 154 */    "scaron",
    /* 155 */    "guilsinglright",
    /* 156 */    "oe",
    /* 157 */    "",
    /* 158 */    "",
    /* 159 */    "Ydieresis",
    /* 160 */    "space",
    /* 161 */    "exclamdown",
    /* 162 */    "cent",
    /* 163 */    "sterling",
    /* 164 */    "currency",
    /* 165 */    "yen",
    /* 166 */    "brokenbar",
    /* 167 */    "section",
    /* 168 */    "dieresis",
    /* 169 */    "copyright",
    /* 170 */    "ordfeminine",
    /* 171 */    "guillemotleft",
    /* 172 */    "logicalnot",
    /* 173 */    "minus",
    /* 174 */    "registered",
    /* 175 */    "macron",
    /* 176 */    "degree",
    /* 177 */    "plusminus",
    /* 178 */    "twosuperior",
    /* 179 */    "threesuperior",
    /* 180 */    "acute",
    /* 181 */    "mu",
    /* 182 */    "paragraph",
    /* 183 */    "periodcentered",
    /* 184 */    "cedilla",
    /* 185 */    "onesuperior",
    /* 186 */    "ordmasculine",
    /* 187 */    "guillemotright",
    /* 188 */    "onequarter",
    /* 189 */    "onehalf",
    /* 190 */    "threequarters",
    /* 191 */    "questiondown",
    /* 192 */    "Agrave",
    /* 193 */    "Aacute",
    /* 194 */    "Acircumflex",
    /* 195 */    "Atilde",
    /* 196 */    "Adieresis",
    /* 197 */    "Aring",
    /* 198 */    "AE",
    /* 199 */    "Ccedilla",
    /* 200 */    "Egrave",
    /* 201 */    "Eacute",
    /* 202 */    "Ecircumflex",
    /* 203 */    "Edieresis",
    /* 204 */    "Igrave",
    /* 205 */    "Iacute",
    /* 206 */    "Icircumflex",
    /* 207 */    "Idieresis",
    /* 208 */    "Eth",
    /* 209 */    "Ntilde",
    /* 210 */    "Ograve",
    /* 211 */    "Oacute",
    /* 212 */    "Ocircumflex",
    /* 213 */    "Otilde",
    /* 214 */    "Odieresis",
    /* 215 */    "multiply",
    /* 216 */    "Oslash",
    /* 217 */    "Ugrave",
    /* 218 */    "Uacute",
    /* 219 */    "Ucircumflex",
    /* 220 */    "Udieresis",
    /* 221 */    "Yacute",
    /* 222 */    "Thorn",
    /* 223 */    "germandbls",
    /* 224 */    "agrave",
    /* 225 */    "aacute",
    /* 226 */    "acircumflex",
    /* 227 */    "atilde",
    /* 228 */    "adieresis",
    /* 229 */    "aring",
    /* 230 */    "ae",
    /* 231 */    "ccedilla",
    /* 232 */    "egrave",
    /* 233 */    "eacute",
    /* 234 */    "ecircumflex",
    /* 235 */    "edieresis",
    /* 236 */    "igrave",
    /* 237 */    "iacute",
    /* 238 */    "icircumflex",
    /* 239 */    "idieresis",
    /* 240 */    "eth",
    /* 241 */    "ntilde",
    /* 242 */    "ograve",
    /* 243 */    "oacute",
    /* 244 */    "ocircumflex",
    /* 245 */    "otilde",
    /* 246 */    "odieresis",
    /* 247 */    "divide",
    /* 248 */    "oslash",
    /* 249 */    "ugrave",
    /* 250 */    "uacute",
    /* 251 */    "ucircumflex",
    /* 252 */    "udieresis",
    /* 253 */    "yacute",
    /* 254 */    "thorn",
    /* 255 */    "ydieresis",
                 NULL,
};

extern CHAR encfile[MAX_PATH];
extern CHAR outfile[MAX_PATH];
extern CHAR infofile[MAX_PATH];
extern INT charset;
extern INT devType;
extern BOOL forceVariablePitch;

/*--------------------------------------------------------------------------*/
BOOL GetINFFontDescription(LPSZ, LPSZ, LPSZ);
BOOL MakePfm(LPSZ, LPSZ, LPSZ);

VOID GetFilename(LPSZ, LPSZ);
INT OpenParseFile(LPSZ);
INT OpenTargetFile(LPSZ);
VOID WriteDots(VOID);
LPVOID AllocateMem(UINT);
VOID FreeAllMem(VOID);
GlyphName *SetupGlyphArray(LPSZ);

extern short _MakePfm(VOID);          /* afm.c */
extern VOID StartParse(VOID);         /* token.c */
extern BOOL GetLine(INT);
extern VOID GetWord(CHAR *, INT);
extern BOOL GetString(CHAR *, INT);
extern INT  GetToken(INT, KEY *);

/*--------------------------------------------------------------------------*/

#ifdef ADOBE_CODE_WE_DO_NOT_USE


BOOL GetINFFontDescription(
  LPSZ    lpszInf,
  LPSZ    lpszDescription,
  LPSZ    lpszPSFontName
)
{
  INT         hfile, iToken;
  CHAR        szName[128];
  CHAR        szAngle[10];
  CHAR        szStyle[2];
  CHAR        szMods[30];
  BOOL        bAddItalic = FALSE;
  CHAR        szBold[20];
  CHAR        szItalic[20];

  static KEY infKeys[] = {
      "FontName",   TK_PSNAME,
      "MSMenuName", TK_MSMENUNAME,
      "VPStyle",    TK_VPSTYLE,
      "ItalicAngle",TK_ANGLE,
      NULL, 0
      };

  hfile = OpenParseFile( lpszInf );
  if( hfile == -1 ) return(FALSE);

  szName[0] = szStyle[0] = szMods[0] = lpszPSFontName[0] = 0;

  // bodind replaced AtmGetString by strcpy

  strcpy(szBold, "Bold");
  strcpy(szItalic, "Italic");
  //AtmGetString( RCN(STR_BOLD), szBold, sizeof(szBold) );
  //AtmGetString( RCN(STR_ITALIC), szItalic, sizeof(szItalic) );

  while( GetLine(hfile) ) {
      iToken = GetToken(hfile,infKeys);
      switch(iToken) {
          case TK_MSMENUNAME:
              GetString(szName, sizeof(szName));
              break;
          case TK_PSNAME:
              GetString(lpszPSFontName, MAX_PATH);
              break;
          case TK_ANGLE:
              GetWord(szAngle, sizeof(szAngle));
              if ( strcmp (szAngle, "0") )
                 bAddItalic = TRUE;
              break;
          case TK_VPSTYLE:
              GetString(szStyle, sizeof(szStyle));
              switch( toupper(szStyle[0]) ) {
                  case 'N': break;
                  case 'B': strcpy(szMods, szBold);    break;
                  case 'T': strcpy(szMods, szBold);
                  case 'I':
                     strcat(szMods, szItalic);
                     bAddItalic = FALSE;
                     break;
                  /* default:  break; */
                  }
              break;
          }
      }
  _lclose(hfile);

  if( !szName[0] ) return(FALSE);

  strcpy( lpszDescription, szName );
  if( szMods[0] ) {
      strcat( lpszDescription, "," );
      strcat( lpszDescription, szMods );
      if (bAddItalic)
         strcat(lpszDescription, szItalic);
      }         
  else
     {
      if (bAddItalic)
        {
        strcat (lpszDescription, "," );
        strcat (lpszDescription, szItalic);
        }
     }
  return(TRUE);
} // end of GetINFFontDescription


/*--------------------------------------------------------------------------*/
INT MakePfm(afmpath, infdir, pfmdir) /* MEF */
LPSZ afmpath, infdir, pfmdir;
{
  #define FATALERROR  2
  #define NOERROR     0

  CHAR bname[9];

  indexMemArray = 0;              /* init global */

  strcpy(afm.szFile, afmpath);
  GetFilename(afmpath, bname);
  if( infdir[strlen(infdir)-1] == '\\' )
      sprintf(infofile, "%s%s%s", infdir, bname, str_DotINF);
  else
      sprintf(infofile, "%s\\%s%s", infdir, bname, str_DotINF);
  if( pfmdir[strlen(pfmdir)-1] == '\\' )
      sprintf(outfile,  "%s%s%s", pfmdir, bname, str_DotPFM);
  else
      sprintf(outfile,  "%s\\%s%s", pfmdir, bname, str_DotPFM);

  afm.iPtSize = 12;
  encfile[0] = EOS;
  devType = POSTSCRIPT;
  if( !strcmp(_strupr(bname), "SY______") ) charset = SYMBOL_CHARSET;
  else charset = -1;
/*
 * yh 8/16/91 -- Keep forceVariablePitch to TRUE for now to be compatible
 * with bitmaps generated by Font Foundry.  ATM and device driver will
 * report different value for PitchAndFamily for monospaced fonts.
 *
 * forceVariablePitch = FALSE;
 */

  if( !_MakePfm() ) {
      return FATALERROR;
      }
  return(NOERROR);
}

#endif // ADOBE_CODE_WE_DO_NOT_USE

/*--------------------------------------------------------------------------*/
VOID GetFilename(path, name)
LPSZ path, name;
{
  LPSZ p;
  INT i;

  if( (p = strrchr(path,'\\')) == NULL )
      if( (p = strrchr(path,':') ) == NULL ) p = path;
  if( p != NULL ) p++;
  for(i=0; i<8; i++) {
      if( p[i]=='.' || p[i]==EOS ) break;
      name[i] = p[i];
      }
  name[i] = EOS;
}

/*----------------------------------------------------------------------------*/
INT OpenParseFile(lpszPath)
LPSZ lpszPath;
{
  OFSTRUCT    of;

  StartParse();
  return( OpenFile(lpszPath, &of, OF_READ) );
}

/*----------------------------------------------------------------------------*/
INT OpenTargetFile(lpszPath)
LPSZ lpszPath;
{
  OFSTRUCT    of;

  return( OpenFile(lpszPath, &of, OF_CREATE | OF_WRITE) );
}

LPVOID AllocateMem(size)
UINT size;
{
  HANDLE hmem;

  if( !(hmem=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size)) ) return(NULL);
  hMemArray[indexMemArray++] = hmem;
  return( GlobalLock(hmem) );
}

/*--------------------------------------------------------------------------*/
VOID FreeAllMem()
{
  INT i;

  for(i=0; i<indexMemArray; i++) {
      GlobalUnlock( hMemArray[i] );
      GlobalFree( hMemArray[i] );
      }
}

/*--------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
GlyphName *SetupGlyphArray(encFilePath)

LPSZ encFilePath;
{
  return(winEnc);
}

/*----------------------------------------------------------------------------*/



/******************************Public*Routine******************************\
*
* BOOL bGetDescFromInf(char * pszINF, DWORD cjDesc, char *pszDesc)
*
* Not same as adobe's routine, we use font name from which we weed out
* hyphes '-'
*
* History:
*  28-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bGetDescFromInf(char * pszINF, DWORD cjDesc, char *pszDesc)
{
  INT         hfile;

  static KEY akeyInf[] = {
      "FontName",   TK_PSNAME,
      NULL, 0
      };

  hfile = OpenParseFile( pszINF );
  if( hfile == -1 ) return(FALSE);

  pszDesc[0] = 0;

  while( GetLine(hfile) )
  {
    if (GetToken(hfile,akeyInf) == TK_PSNAME)
    {
       GetString(pszDesc, cjDesc);
       break;
    }
  }
  _lclose(hfile);

  if( !pszDesc[0] ) return(FALSE);

// weed out hyphens

  for ( ; *pszDesc; pszDesc++)
  {
    if (*pszDesc == '-')
        *pszDesc = ' ';
  }

  return(TRUE);

}

/******************************Public*Routine******************************\
*
* short CreatePFM(char *pszINF, char *pszAFM, char *pszPFM);
*
* slightly modified adobe's routine
*
* History:
*  28-Apr-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
*
* Returns: 16-bit encoded value indicating error and type of file where
*          error occurred.  (see fvscodes.h) for definitions.
*          The following table lists the "status" portion of the codes
*          returned.
*
*           FVS_SUCCESS           
*           FVS_INVALID_FONTFILE  
*           FVS_FILE_OPEN_ERR   
*           FVS_FILE_BUILD_ERR  
*
\**************************************************************************/
short CreatePFM(char *pszINF, char *pszAFM, char *pszPFM)
{
  CHAR bname[9];

  indexMemArray = 0;              /* init global */

  strcpy(afm.szFile, pszAFM);
  GetFilename(pszAFM, bname);

  strcpy (infofile, pszINF);
  strcpy (outfile, pszPFM);

  afm.iPtSize = 12;
  encfile[0] = EOS;
  devType = POSTSCRIPT;

// this is something that would have never come to my mind [bodind]

  if( !strcmp(_strupr(bname), "SY______") )
    charset = SYMBOL_CHARSET;
  else
    charset = -1;

/*
 * yh 8/16/91 -- Keep forceVariablePitch to TRUE for now to be compatible
 * with bitmaps generated by Font Foundry.  ATM and device driver will
 * report different value for PitchAndFamily for monospaced fonts.
 *
 * forceVariablePitch = FALSE;
 */

  return _MakePfm();
}
