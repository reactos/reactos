//---------------------------------------------------------------------------
// makepfm.c
//---------------------------------------------------------------------------
// Create PFM file for Rev-3 fonts
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "windows.h"
#pragma pack(1)
#include "makepfm.h"
#pragma pack(4)

#include "fvscodes.h"  // FVS_xxxxxx (font validation status) codes and macros.

#ifdef WIN30
  #define LPCSTR LPSTR
#endif


#define WINATM 1
#if !WINATM
LPSZ stringtable[] = {
"MAKEPFM utility version %s released on %s.\n",
"Copyright (C) 1989-91, Adobe Systems Inc. All Rights Reserved.\n\n",
"Usage: makepfm [options] AFMfile\n",
"  -h n   - set device to PCL (n=1 for 1 byte typeface, 2 for 2 byte).\n",
"  -p n   - integral point size - only for PCL.\n",
"  -c str - PCL symbol set (9U for WinAnsi for example) - only for PCL.\n",
"  -d     - set orientation to landscape - only for PCL.\n",
"  -e str - encoding file.\n",
"  -o str - output file.\n",
"  -i str - fontinfo file.\n",
"  -l str - optional log file - defaults to \"user.log\".\n",
"  -f str - take input parameters from file instead of command line.\n",
"  -w     - display warning messages.\n",
"  -s n   - force dfCharSet to n.\n",
"Unrecognized command-line option: '%s'\n",
"Unable to open: %s\n",
"Too many track kerning data. Ignoring after %d.\n",
"Unexpected end of file - expected: %s\n",
"Expected: %s - current line: %s\n",
"Parsing character metrics - current line: %s\n",
"Parsing %s.\n",
"Missing \"MSFamily\" value\n",
"Can't create: %s\n",
"Disk is full...\n",
"Memory allocation\n",
"encoding file",
"Creating font metrics ( %s )",
"Finished.\n",
NULL
};
#endif

AFM afm = { 0 };
static ETM etm;
static PFM pfm;
static PFMEXT pfmext;
static DRIVERINFO d;

typedef LPSZ GlyphName;

/* CHAR rgbBuffer[2048]; The file buffer */
CHAR rgbBuffer[8704] = "";   /* increased to handle an additional 512 bytes of width info */
static INT cbBuffer;         /* The number of bytes in the buffer */
static LPSZ pbBuffer;        /* Ptr to current location in buffer */
static CHAR rgbLine[160];    /* The current line of text being processed */
static LPSZ szLine;          /* Ptr to the current location in the line */
static BOOL fEOF = FALSE;
static BOOL fUnGetLine = FALSE;

/*----------------------------------------------------------------------------*/
static LPSZ notdef = "";

#define IBULLET     0x095   /* 87-1-15 sec (was 1) */
#define ISPACE      0x20
#define IWINSPACE   0xA0

static BOOL parseError;
static float sf;             /* scale factor for converting to display widths */

/* flags type of PFM to build POSTSCRIPT vs PCL */
INT devType = POSTSCRIPT;

PCLINFO pclinfo = { PORTRAIT, WINANSI_SET, epsymGENERIC8, 0, 0, 2, 0, NULL };
static SHORT fiCapHeight;
static GlyphName *glyphArray;
extern GlyphName *SetupGlyphArray(LPSZ) ;
INT charset = -1;
static BOOL forceVariablePitch = TRUE;

/* names, pointers, and handles for output, log and data files */
CHAR encfile[FNAMEMAX] = "";
CHAR outfile[FNAMEMAX] = "";
CHAR infofile[FNAMEMAX] = "";

static INT fhIn;

#define TK_STARTKERNDATA      2
#define TK_STARTKERNPAIRS     3
#define TK_KPX                4
#define TK_ENDKERNPAIRS       5
#define TK_ENDKERNDATA        6
#define TK_FONTNAME           7
#define TK_WEIGHT             8
#define TK_ITALICANGLE        9
#define TK_ISFIXEDPITCH       10
#define TK_UNDERLINEPOSITION  11
#define TK_UNDERLINETHICKNESS 12
#define TK_FONTBBOX           13
#define TK_CAPHEIGHT          14
#define TK_XHEIGHT            15
#define TK_DESCENDER          16
#define TK_ASCENDER           17
#define TK_STARTCHARMETRICS   18
#define TK_ENDCHARMETRICS     19
#define TK_ENDFONTMETRICS     20
#define TK_STARTFONTMETRICS   21
#define TK_STARTTRACKKERN     22
#define TK_TRACKKERN          23
#define TK_ENDTRACKKERN       24

static KEY afmKeys[] = {
    "FontBBox",           TK_FONTBBOX,
    "StartFontMetrics",   TK_STARTFONTMETRICS,
    "FontName",           TK_FONTNAME,
    "Weight",             TK_WEIGHT,
    "ItalicAngle",        TK_ITALICANGLE,
    "IsFixedPitch",       TK_ISFIXEDPITCH,
    "UnderlinePosition",  TK_UNDERLINEPOSITION,
    "UnderlineThickness", TK_UNDERLINETHICKNESS,
    "CapHeight",          TK_CAPHEIGHT,
    "XHeight",            TK_XHEIGHT,
    "Descender",          TK_DESCENDER,
    "Ascender",           TK_ASCENDER,
    "StartCharMetrics",   TK_STARTCHARMETRICS,
    "EndCharMetrics",     TK_ENDCHARMETRICS,
    "StartKernData",      TK_STARTKERNDATA,
    "StartKernPairs",     TK_STARTKERNPAIRS,
    "KPX",                TK_KPX,
    "EndKernPairs",       TK_ENDKERNPAIRS,
    "EndKernData",        TK_ENDKERNDATA,
    "EndFontMetrics",     TK_ENDFONTMETRICS,
    "StartTrackKern",     TK_STARTTRACKKERN,
    "TrackKern",          TK_TRACKKERN,
    "EndTrackKern",       TK_ENDTRACKKERN,
    NULL,                 0
    };

#define CVTTOSCR(i)  (INT)(((float)(i) * sf) + 0.5)
#define DRIVERINFO_VERSION      (1)

/*----------------------------------------------------------------------------*/
VOID KxSort(KX *, KX *);
INT GetCharCode(LPSZ,  GlyphName *);
VOID ParseKernPairs(INT);
VOID ParseTrackKern(INT);
VOID ParseKernData(INT);
VOID ParseFontName(VOID);
VOID ParseMSFields(VOID);
VOID ParseCharMetrics(BOOL);
VOID ParseCharBox(BBOX *);
LPSZ ParseCharName(VOID);
INT ParseCharWidth(VOID);
INT ParseCharCode(VOID);
VOID ParseBoundingBox(BOOL);
VOID ParsePitchType(VOID);
VOID InitAfm(VOID);
short _MakePfm(VOID);
BOOL ReadFontInfo(INT);
VOID GetCharMetrics(INT, CM *);
VOID SetCharMetrics(INT, CM *);
VOID GetSmallCM(INT, CM *);
VOID SetFractionMetrics(INT, INT, INT, INT);
VOID FixCharWidths(VOID);
VOID SetAfm(VOID);
VOID SetAvgWidth(VOID);
VOID SetMaxWidth(VOID);

/*----------------------------------------------------------------------------*/
VOID ResetBuffer(VOID);
VOID PutByte(SHORT);
VOID PutRgb(LPSZ, INT);
VOID PutWord(SHORT);
VOID PutLong(long);
VOID SetDf(INT);
VOID PutString(LPSZ);
VOID PutDeviceName(LPSZ);
VOID PutFaceName(VOID);
BOOL MakeDf(BOOL, SHORT, LPSZ);
VOID PutPairKernTable(SHORT);
VOID PutTrackKernTable(SHORT);
VOID PutExtentOrWidthTable(INT);
BOOL WritePfm(LPSZ);

/*----------------------------------------------------------------------------*/
VOID SetDriverInfo(VOID);
VOID PutDriverInfo(INT);
LPSZ GetEscapeSequence(VOID);

/*----------------------------------------------------------------------------*/
VOID AfmToEtm(BOOL);
VOID PutEtm(BOOL);

/*----------------------------------------------------------------------------*/
VOID StartParse(VOID);
BOOL szIsEqual(LPSZ, LPSZ);
VOID szMove(LPSZ, LPSZ, INT);
BOOL GetBuffer(INT);
VOID UnGetLine(VOID);
BOOL GetLine(INT);
BOOL _GetLine(INT);
VOID EatWhite(VOID);
VOID GetWord(LPSZ, INT);
BOOL GetString(LPSZ, INT);
BOOL GetNumber(SHORT *);
BOOL GetFloat(float *, SHORT *);
INT MapToken(LPSZ, KEY *);
INT GetToken(INT, KEY *);

/*----------------------------------------------------------------------------*/
GlyphName *AllocateGlyphArray(INT);
VOID PutGlyphName(GlyphName *, INT, LPSZ);

/*----------------------------------------------------------------------------*/
#if DEBUG_MODE
VOID DumpAfm(VOID);
VOID DumpKernPairs(VOID);
VOID DumpKernTracks(VOID);
VOID DumpCharMetrics(VOID);
VOID DumpPfmHeader(VOID);
VOID DumpCharWidths(VOID);
VOID DumpPfmExtension(VOID);
VOID DumpDriverInfo(VOID);
VOID DumpEtm(VOID);
#endif

/*----------------------------------------------------------------------------*/
extern INT  OpenParseFile(LPSZ);                 /* main.c */
extern INT  OpenTargetFile(LPSZ);
// extern VOID cdecl PostWarning(LPCSTR,  ...);
// extern VOID cdecl PostError(LPCSTR, ...);
extern LPVOID AllocateMem(UINT);
extern VOID FreeAllMem(VOID);
extern VOID WriteDots(VOID);
extern GlyphName *SetupGlyphArray(LPSZ);
#if !WINATM
extern GlyphName *NewGlyphArray(INT);
extern LPSZ ReadLine(FILE *, LPSZ, INT);
extern LPSZ FirstTokenOnLine(FILE *, LPSZ, INT);
extern LPSZ Token(INT);
extern VOID ParseError(VOID);
#endif

/*----------------------------------------------------------------------------*/
/***************************************************************
* Name: KxSort()
* Action: Sort the pair kerning data using the quicksort algorithm.
******************************************************************/
VOID KxSort(pkx1, pkx2)
KX *pkx1;
KX *pkx2;
{
  static WORD iPivot;
  INT iKernAmount;
  KX *pkx1T;
  KX *pkx2T;

  if (pkx1>=pkx2) return;

  iPivot = pkx1->iKey;;
  iKernAmount = pkx1->iKernAmount;
  pkx1T = pkx1;
  pkx2T = pkx2;

  while (pkx1T < pkx2T)
    {
    while (pkx1T < pkx2T)
      {
      if (pkx2T->iKey < iPivot)
        {
        pkx1T->iKey = pkx2T->iKey;
        pkx1T->iKernAmount = pkx2T->iKernAmount;
        ++pkx1T;
        break;
        }
      else
        --pkx2T;
      }
    while (pkx1T < pkx2T)
      {
      if (pkx1T->iKey > iPivot)
        {
        pkx2T->iKey = pkx1T->iKey;
        pkx2T->iKernAmount = pkx1T->iKernAmount;
        --pkx2T;
        break;
        }
      else
        ++pkx1T;
      }
    }
  pkx2T->iKey = iPivot;
  pkx2T->iKernAmount = (SHORT)iKernAmount;
  ++pkx2T;
  if ((pkx1T - pkx1) < (pkx2 - pkx2T))
    {
    KxSort(pkx1, pkx1T);
    KxSort(pkx2T, pkx2);
    }
  else
    {
    KxSort(pkx2T, pkx2);
    KxSort(pkx1, pkx1T);
    }
}

/******************************************************************
* Name: GetCharCode(glyphname, glypharray)
* Action: Lookup glyphname in glypharray & return index.
********************************************************************/
INT GetCharCode(glyphname, glypharray)

LPSZ glyphname;
GlyphName *glypharray;
{
  register INT i;

  if ( STRCMP(glyphname, "") != 0 )
      for(i=0; glypharray[i]!=NULL; i++)
          if ( STRCMP(glypharray[i], glyphname) == 0 ) return(i);
  /* printf("GetCharCode: Undefined character = %s\n", glyphname); */
  return(-1);
}

/******************************************************************
* Name: ParseKernPairs()
* Action: Parse the pairwise kerning data.
********************************************************************/
VOID ParseKernPairs(pcl)
INT pcl;
{
  UINT iCh1, iCh2;
  KP *pkp;
  INT iToken;
  WORD cPairs, i;
  SHORT iKernAmount;
  CHAR szWord[80];

  GetNumber(&cPairs);
  if( cPairs == 0 )
      return;

  pkp = &afm.kp;
  pkp->cPairs = 0;
  pkp->rgPairs = (PKX) AllocateMem( (UINT) (sizeof(KX) * cPairs) );
  if( pkp->rgPairs == NULL ) {
      ; // PostError(str(MSG_PFM_BAD_MALLOC));
      parseError = TRUE;
      return;
      }

  for (i = 0; i < cPairs; ++i) {
      if( !GetLine(fhIn) ) break;
      if( GetToken(fhIn, afmKeys) != TK_KPX ) {
          UnGetLine();
          break;
          }
      GetWord(szWord, sizeof(szWord));
      iCh1 = (UINT)GetCharCode(szWord, glyphArray);
      GetWord(szWord, sizeof(szWord));
      iCh2 = (UINT)GetCharCode(szWord, glyphArray);
      GetNumber(&iKernAmount);

      /* no kern pairs for unencoded characters or miniscule kern amounts */
      if( (iCh1 == -1 || iCh2 == -1) || (pcl && CVTTOSCR(iKernAmount) == 0) )
          continue;

      pkp->rgPairs[pkp->cPairs].iKey = iCh2 << 8 | iCh1;
      pkp->rgPairs[pkp->cPairs++].iKernAmount =
                                       (pcl) ? CVTTOSCR(iKernAmount) : iKernAmount;
      }

  GetLine(fhIn);
  iToken = GetToken(fhIn, afmKeys);
  if( iToken == TK_EOF )
      ; // PostWarning(str(MSG_PFM_BAD_EOF), "EndKernPairs");
  else if( iToken != TK_ENDKERNPAIRS ) {
      ; // PostError(str(MSG_PFM_BAD_TOKEN), "EndKernPairs", rgbLine);
      parseError = TRUE;
      }
  KxSort(&afm.kp.rgPairs[0], &afm.kp.rgPairs[afm.kp.cPairs - 1]);
}

/******************************************************************
* Name: ParseTrackKern()
* Action: Parse the track kerning data.
********************************************************************/
VOID ParseTrackKern(pcl)
INT pcl;
{
  float one;
  INT i;
  KT *pkt;
  INT iToken;

  one = (float) 1;
  pkt = &afm.kt;
  GetNumber(&pkt->cTracks);
  if( pkt->cTracks > MAXTRACKS) ; // PostWarning(str(MSG_PFM_BAD_TRACK), MAXTRACKS);

  for (i = 0; i < pkt->cTracks; ++i) {
    if( !GetLine(fhIn) ) {
        ; // PostError(str(MSG_PFM_BAD_EOF), "EndTrackKern");
        parseError = TRUE;
        return;
        }
    if( GetToken(fhIn, afmKeys) != TK_TRACKKERN ) {
        ; // PostError(str(MSG_PFM_BAD_TOKEN), "EndTrackKern", rgbLine);
        parseError = TRUE;
        return;
        }
    if( i < MAXTRACKS) {
        GetNumber(&pkt->rgTracks[i].iDegree);
        GetFloat(&one, &pkt->rgTracks[i].iPtMin);
        (pcl) ? GetFloat(&sf, &pkt->rgTracks[i].iKernMin) :
                GetFloat(&one, &pkt->rgTracks[i].iKernMin);
        GetFloat(&one, &pkt->rgTracks[i].iPtMax);
        (pcl) ? GetFloat(&sf, &pkt->rgTracks[i].iKernMax) :
                GetFloat(&one, &pkt->rgTracks[i].iKernMax);
        }
    }

  GetLine(fhIn);
  iToken = GetToken(fhIn, afmKeys);
  if( iToken == TK_EOF ) {
    ; // PostError(str(MSG_PFM_BAD_EOF), "EndTrackKern");
    parseError = TRUE;
    }
  else if( iToken != TK_ENDTRACKKERN ) {
    ; // PostError(str(MSG_PFM_BAD_TOKEN), "EndTrackKern", rgbLine);
    parseError = TRUE;
    }
}

/********************************************************
* Name: ParseKernData()
* Action: Start processing the kerning data.
*************************************************************/
VOID ParseKernData(pcl)
INT pcl;
{
  INT iToken;
  do {
    if ( !GetLine(fhIn) ) {
        ; // PostError(str(MSG_PFM_BAD_EOF), "EndKernData");
        parseError = TRUE;
        }
    iToken = GetToken(fhIn, afmKeys);
    if( iToken == TK_STARTKERNPAIRS ) ParseKernPairs(pcl);
    else if( iToken == TK_STARTTRACKKERN ) ParseTrackKern(pcl);
    } while( iToken != TK_ENDKERNDATA);
}

/***********************************************************
* Name: ParseFontName()
* Action: Move the font name from the input buffer into the afm
*    structure.
**************************************************************/
VOID ParseFontName()
{
  EatWhite();
  szMove(afm.szFont, szLine, sizeof(afm.szFont));
}

/**************************************************************
* Name: ParseCharMetrics()
* Action: Parse the character metrics entry in the input file
*   and set the width and bounding box in the afm structure.
*****************************************************************/
VOID ParseCharMetrics(pcl)
BOOL pcl;
{
  SHORT cChars;
  INT i, iChar, iWidth;
  BBOX rcChar;

  if (afm.iFamily == FF_DECORATIVE)
      glyphArray = AllocateGlyphArray(255);
  else
      glyphArray = SetupGlyphArray(encfile);
  if( glyphArray == NULL ) {
      parseError = TRUE;
      return;
      }
  GetNumber(&cChars);
  for (i = 0; i < cChars; ++i) {
      if( !GetLine(fhIn) ) {
          ; // PostError(str(MSG_PFM_BAD_EOF), "EndCharMetrics");
          parseError = TRUE;
          return;
          }
      iChar = ParseCharCode();
      iWidth = ParseCharWidth();
      if( afm.iFamily == FF_DECORATIVE ) {
          if( iChar < 0 || iChar > 255 ) continue;
          PutGlyphName(glyphArray, iChar, ParseCharName());
      } else {
          iChar = GetCharCode(ParseCharName(), glyphArray);
          if( iChar == -1 ) continue;
          }
      ParseCharBox(&rcChar);
      if( parseError == TRUE ) return;

      afm.rgcm[iChar].iWidth = (pcl) ? CVTTOSCR(iWidth) : iWidth;
      afm.rgcm[iChar].rc.top = (pcl) ? CVTTOSCR(rcChar.top) : rcChar.top;
      afm.rgcm[iChar].rc.left = (pcl) ? CVTTOSCR(rcChar.left) : rcChar.left;
      afm.rgcm[iChar].rc.right = (pcl) ? CVTTOSCR(rcChar.right) : rcChar.right;
      afm.rgcm[iChar].rc.bottom = (pcl) ? CVTTOSCR(rcChar.bottom) : rcChar.bottom;
      }
  GetLine(fhIn);
  if (GetToken(fhIn, afmKeys)!=TK_ENDCHARMETRICS) {
      ; // PostError(str(MSG_PFM_BAD_TOKEN), "EndCharMetrics", rgbLine);
      parseError = TRUE;
      }
}

/***************************************************************
* Name: ParseCharBox()
* Action: Parse the character's bounding box and return its
*   dimensions in the destination rectangle.
*****************************************************************/
VOID ParseCharBox(prc)
BBOX *prc;   /* Pointer to the destination rectangle */
{
  CHAR szWord[16];

  GetWord(szWord, sizeof(szWord));
  if( szIsEqual("B", szWord) ) {
      GetNumber(&prc->left);
      GetNumber(&prc->bottom);
      GetNumber(&prc->right);
      GetNumber(&prc->top);
      }
  else {
      ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
      parseError = TRUE;
      return;
      }
  EatWhite();
  if (*szLine++ != ';') {
      ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
      parseError = TRUE;
      }
}

/*********************************************************
* Name: ParseCharName()
* Action: Parse a character's name
************************************************************/
LPSZ ParseCharName()
{
  static CHAR szWord[40];

  EatWhite();
  GetWord(szWord, sizeof(szWord));
  if (szIsEqual("N", szWord))
    GetWord(szWord, sizeof(szWord));
  else {
    ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
    parseError = TRUE;
    return(szWord);
    }
  EatWhite();
  if (*szLine++ != ';') {
    ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
    parseError = TRUE;
    }
  return(szWord);
}

/***********************************************************
* Name: ParseCharWidth()
* Action: Parse a character's width and return its numeric
*   value.
*************************************************************/
INT ParseCharWidth()
{
  SHORT iWidth;
  CHAR szWord[16];


  GetWord(szWord, sizeof(szWord));
  if (szIsEqual("WX", szWord)) {
    GetNumber(&iWidth);
    if (iWidth==0) ; // PostWarning(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
    EatWhite();
    if (*szLine++ != ';') {
        ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
        parseError = TRUE;
        }
    }
  else {
    ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
    parseError = TRUE;
    }
  return(iWidth);
}

/*****************************************************************
* Name: ParseCharCode()
* Action: Parse the ascii form of a character's code point and
*   return its numeric value.
******************************************************************/
INT ParseCharCode()
{
  SHORT iChar;
  CHAR szWord[16];

  iChar = 0;
  GetWord(szWord, sizeof(szWord));
  if (szIsEqual("C", szWord)) {
    GetNumber(&iChar);
    if (iChar==0) {
        ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
        parseError = TRUE;
        return(0);
        }
    EatWhite();
    if (*szLine++ != ';') {
        ; // PostError(str(MSG_PFM_BAD_CHARMETRICS), rgbLine);
        parseError = TRUE;
        }
    }
  return(iChar);
}

/****************************************************************
* Name: ParseBounding Box()
* Action: Parse a character's bounding box and return its size in
*   the afm structure.
*******************************************************************/
VOID ParseBoundingBox(pcl)
BOOL pcl;
{
  SHORT i;

  /*  8-26-91 yh  Note that values in rcBBox are not scaled for PCL either */
  GetNumber(&i);
//  afm.rcBBox.left = (pcl) ? CVTTOSCR(i) : i;
  afm.rcBBox.left = i;
  GetNumber(&i);
//  afm.rcBBox.bottom = (pcl) ? CVTTOSCR(i) : i;
  afm.rcBBox.bottom = i;
  GetNumber(&i);
//  afm.rcBBox.right = (pcl) ? CVTTOSCR(i) : i;
  afm.rcBBox.right = i;
  GetNumber(&i);
//  afm.rcBBox.top = (pcl) ? CVTTOSCR(i) : i;
  afm.rcBBox.top = i;
}

/************************************************************
* Name: ParsePitchType()
*
* Action: Parse the pitch type and set the variable pitch
*      flag in the afm structure.
*         Always set the pitch to be variable pitch for
*         our fonts in Windows
*
**********************************************************/
VOID ParsePitchType()
{
  CHAR szWord[16];

  EatWhite();
  GetWord(szWord, sizeof(szWord));
  if( !STRCMP(_strlwr(szWord), "true" ) ) {
      afm.fWasVariablePitch = FALSE;
      afm.fVariablePitch = forceVariablePitch;
      }
//  afm.fVariablePitch = TRUE;
}

/***********************************************************
* Name: InitAfm()
* Action: Initialize the afm structure.
************************************************************/
VOID InitAfm()
{
  register int i;

  afm.iFirstChar = 0x20;
  afm.iLastChar = 0x0ff;
  afm.iAvgWidth = 0;
  afm.iMaxWidth = 0;
  afm.iItalicAngle = 0;
  afm.iFamily = 0;
  afm.ulOffset = 0;
  afm.ulThick = 0;
  afm.iAscent = 0;
  afm.iDescent = 0;
  afm.fVariablePitch = TRUE;
  afm.fWasVariablePitch = TRUE;
  afm.szFont[0] = 0;
  afm.szFace[0] = 0;
  afm.iWeight = 400;
  afm.kp.cPairs = 0;
  afm.kt.cTracks = 0;
  afm.rcBBox.left = 0;
  afm.rcBBox.bottom = 0;
  afm.rcBBox.right = 0;
  afm.rcBBox.top = 0;

  for(i=0; i<256; i++ ) {
      afm.rgcm[i].rc.left = 0;
      afm.rgcm[i].rc.bottom = 0;
      afm.rgcm[i].rc.right = 0;
      afm.rgcm[i].rc.top = 0;
      afm.rgcm[i].iWidth = 0;
      }
}

/*----------------------------------------------------------------------------
** Returns: 16-bit encoded value indicating error and type of file where
**          error occurred.  (see fvscodes.h) for definitions.
**          The following table lists the "status" portion of the codes
**          returned.
**
**           FVS_SUCCESS
**           FVS_INVALID_FONTFILE
**           FVS_FILE_OPEN_ERR
**           FVS_FILE_BUILD_ERR
*/
short _MakePfm()
{
  INT hfile;
  SHORT i;
  float ten = (float) 10;
  BOOL fPrint = FALSE, fEndOfInput = FALSE, fStartInput = FALSE;
  BOOL bRes;

  // if ( devType == PCL ) sf = ((float)afm.iPtSize / 1000.0) * (300.0 / 72.0);
  InitAfm();

  if( (hfile = OpenParseFile(infofile)) == -1 ) {
      ; // PostError(str(MSG_PFM_BAD_FOPEN), infofile);
      return(FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_INF));
      }
  if( !ReadFontInfo(hfile) ) {
      CLOSE(hfile);
      ; // PostError(str(MSG_PFM_BAD_PARSE), infofile);
      return(FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_INF));
      }
  CLOSE(hfile);

  if( (fhIn = OpenParseFile(afm.szFile)) == -1 ) {
      ; // PostError(str(MSG_PFM_BAD_FOPEN), afm.szFile);
      return(FVS_MAKE_CODE(FVS_FILE_OPEN_ERR, FVS_FILE_AFM));
      }
  parseError = FALSE;
  while (!fEndOfInput) {
      if( !GetLine(fhIn) ) break;
      switch( GetToken(fhIn, afmKeys) ) {
          case TK_STARTFONTMETRICS:
              fStartInput = TRUE;
              break;
          case TK_STARTKERNDATA:
              ParseKernData(devType == PCL);
              break;
          case TK_FONTNAME:
              ParseFontName();
              break;
          case TK_WEIGHT:
              break;
          case TK_ITALICANGLE:
              GetFloat(&ten, &afm.iItalicAngle);
              break;
          case TK_ISFIXEDPITCH:
              ParsePitchType();
              break;
          case TK_UNDERLINEPOSITION:
              GetNumber(&i);
              afm.ulOffset = (devType==POSTSCRIPT) ? abs(i) : CVTTOSCR(abs(i));
              break;
          case TK_UNDERLINETHICKNESS:
              GetNumber(&i);
              afm.ulThick = (devType == POSTSCRIPT) ? i : CVTTOSCR(i);
              break;
          case TK_FONTBBOX:
              ParseBoundingBox(devType == PCL);
              break;
          case TK_CAPHEIGHT:
              GetNumber(&i);
              if( fiCapHeight == 0 ) fiCapHeight = i;
              break;
          case TK_XHEIGHT:
              break;
          case TK_DESCENDER:
              GetNumber(&i);
              afm.iDescent = (devType == POSTSCRIPT) ? i : CVTTOSCR(i);
              break;
          case TK_ASCENDER:
              GetNumber(&i);
              if (i < 667) i = 667;
              afm.iAscent = (devType == POSTSCRIPT) ? i : CVTTOSCR(i);
              break;
          case TK_STARTCHARMETRICS:
              if (afm.iFamily == 0) {
                  ; // PostError(str(MSG_PFM_MISSING_MSFAMILY));
                  CLOSE(fhIn);
                  return(FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_AFM));
                  }
              ParseCharMetrics(devType == PCL);
              break;
          case TK_ENDFONTMETRICS:
              fEndOfInput = TRUE;
              break;
          }
      if( parseError ) {
          CLOSE(fhIn);
          return(FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_AFM));
          }
      }
  CLOSE(fhIn);
  if( !fStartInput ) {
      ; // PostError(str(MSG_PFM_BAD_EOF), "StartFontMetrics");
      return(FVS_MAKE_CODE(FVS_INVALID_FONTFILE, FVS_FILE_AFM));
      }
  FixCharWidths();
  SetAfm();

#if DEBUG_MODE
  DumpAfm();
  DumpKernPairs();
  DumpKernTracks();
  DumpCharMetrics();
#endif
  bRes = MakeDf(FALSE, (SHORT)devType, outfile);
  FreeAllMem();
  return(bRes ? FVS_MAKE_CODE(FVS_SUCCESS, FVS_FILE_UNK) :
                FVS_MAKE_CODE(FVS_FILE_BUILD_ERR, FVS_FILE_PFM));
}

/*----------------------------------------------------------------------------*/
BOOL ReadFontInfo(hfile)
INT hfile;
{
  INT iToken;
  CHAR szTemp[6];
  BOOL found[LAST_FI_TOKEN+1];
  static KEY infKeys[] = {
      "MSMenuName",      TK_MSMENUNAME,
      "VPStyle",         TK_VPSTYLE,
      "Pi",              TK_PI,
      "Serif",           TK_SERIF,
      "PCLStyle",        TK_PCLSTYLE,
      "PCLStrokeWeight", TK_PCLSTROKEWEIGHT,
      "PCLTypefaceID",   TK_PCLTYPEFACEID,
      "CapHeight",       TK_INF_CAPHEIGHT,
      NULL, 0
      };

  fiCapHeight = 0;
  for(iToken=0; iToken<=LAST_FI_TOKEN; iToken++) found[iToken] = FALSE;
  while( GetLine(hfile) ) {
      iToken = GetToken(hfile,infKeys);
      found[iToken] = TRUE;
      switch(iToken) {
          case TK_MSMENUNAME:
              if( !GetString(afm.szFace, sizeof(afm.szFace)) ) return(FALSE);
              break;
          case TK_VPSTYLE:
              if( !GetString(szTemp, sizeof(szTemp)) ) return(FALSE);
              switch( toupper(szTemp[0]) ) {
                  case 'N':
                  case 'I': afm.iWeight = FW_NORMAL; break;
                  case 'B':
                  case 'T': afm.iWeight = FW_BOLD; break;
                  default:  return(FALSE); break;
                  }
              break;
          case TK_PI:
              GetWord(szTemp, sizeof(szTemp));
              if( !STRCMP(_strupr(szTemp), "TRUE") )
                  afm.iFamily = FF_DECORATIVE;
              else if( STRCMP(szTemp, "FALSE") ) return(FALSE);
              break;
          case TK_SERIF:
              GetWord(szTemp, sizeof(szTemp));
              if( !STRCMP(_strupr(szTemp), "TRUE") ) {
                  if( afm.iFamily != FF_DECORATIVE ) afm.iFamily = FF_ROMAN;
                  }
              else if( !STRCMP(szTemp, "FALSE") )  {
                  if( afm.iFamily != FF_DECORATIVE ) afm.iFamily = FF_SWISS;
                  }
              else return(FALSE);
              break;
          case TK_INF_CAPHEIGHT:
              GetNumber(&fiCapHeight);
              break;
          case TK_PCLSTYLE:
              GetNumber(&pclinfo.style);
              break;
          case TK_PCLSTROKEWEIGHT:
              GetNumber(&pclinfo.strokeWeight);
              break;
          case TK_PCLTYPEFACEID:
              GetNumber((SHORT *)&pclinfo.typeface);
              if( pclinfo.typefaceLen == 1 ) pclinfo.typeface &= 0xFF;
              break;
          }
      }
  if( found[TK_MSMENUNAME] == FALSE ||
      found[TK_VPSTYLE] == FALSE ||
      found[TK_PI] == FALSE ||
      found[TK_SERIF] == FALSE ||
      found[TK_INF_CAPHEIGHT] == FALSE ) return(FALSE);
  if ( devType == PCL )
      if( found[TK_PCLSTYLE] == FALSE ||
          found[TK_PCLSTROKEWEIGHT] == FALSE ||
          found[TK_PCLTYPEFACEID] == FALSE ) return(FALSE);
  return(TRUE);
}

#if DEBUG_MODE
/*----------------------------------------------------------------------------*/
VOID DumpAfm()
{
  printf("\nAFM HEADER\n");
  printf("afm.iFirstChar: %d\n", afm.iFirstChar);
  printf("afm.iLastChar: %d\n", afm.iLastChar);
  printf("afm.iPtSize: %d\n", afm.iPtSize);
  printf("afm.iAvgWidth: %d\n", afm.iAvgWidth);
  printf("afm.iMaxWidth: %d\n", afm.iMaxWidth);
  printf("afm.iItalicAngle: %d\n", afm.iItalicAngle);
  printf("afm.iFamily: %d\n", afm.iFamily);
  printf("afm.ulOffset: %d\n", afm.ulOffset);
  printf("afm.ulThick: %d\n", afm.ulThick);
  printf("afm.iAscent: %d\n", afm.iAscent);
  printf("afm.iDescent: %d\n", afm.iDescent);
  printf("afm.fVariablePitch: %d\n", afm.fVariablePitch);
  printf("afm.szFile: %s\n", afm.szFile);
  printf("afm.szFont: %s\n", afm.szFont);
  printf("afm.szFace: %s\n", afm.szFace);
  printf("afm.iWeight: %d\n", afm.iWeight);
  printf("afm.rcBBox - top: %d left: %d right: %d bottom: %d\n",
    afm.rcBBox.top, afm.rcBBox.left, afm.rcBBox.right, afm.rcBBox.bottom);
}
/*----------------------------------------------------------------------------*/
VOID DumpKernPairs()
{
  INT indx;

  printf("\nKERN PAIRS\n");
  printf("afm.kp.cPairs: %d\n", afm.kp.cPairs);
  for (indx = 0; indx < afm.kp.cPairs; indx++)
        printf("afm.kp.rgPairs[%d] - iKey: %u iKernAmount: %d\n", indx,
          afm.kp.rgPairs[indx].iKey, afm.kp.rgPairs[indx].iKernAmount);
}
/*----------------------------------------------------------------------------*/
VOID DumpKernTracks()
{
  INT indx;

  printf("\nKERN TRACKS\n");
  printf("afm.kt.cTracks: %d\n", afm.kt.cTracks);
  for (indx = 0; indx < afm.kt.cTracks; indx++) {
        printf("track: %d iDegree: %d iPtMin: %d iKernMin: %d iPtMax: %d iKernMax: %d\n",
          indx,
          afm.kt.rgTracks[indx].iDegree,
          afm.kt.rgTracks[indx].iPtMin,
          afm.kt.rgTracks[indx].iKernMin,
          afm.kt.rgTracks[indx].iPtMax,
          afm.kt.rgTracks[indx].iKernMax);
        }

}
/*----------------------------------------------------------------------------*/
VOID DumpCharMetrics()
{
  INT indx;

  printf("\nCHARACTER METRICS\n");
  for (indx = afm.iFirstChar; indx <= afm.iLastChar; ++indx) {
    printf("indx: %d width: %d top: %d left: %d right: %d bottom: %d\n",
          indx,
          afm.rgcm[indx].iWidth,
          afm.rgcm[indx].rc.top,
          afm.rgcm[indx].rc.left,
    afm.rgcm[indx].rc.right,
          afm.rgcm[indx].rc.bottom);
        }
}
/*----------------------------------------------------------------------------*/
#endif

/******************************************************
* Name: GetCharMetrics()
* Action: Get the character metrics for a specified character.
**********************************************************/
VOID GetCharMetrics(iChar, pcm)
INT iChar;
CM *pcm;
{
  CM *pcmSrc;

  pcmSrc = &afm.rgcm[iChar];
  pcm->iWidth = pcmSrc->iWidth;
  pcm->rc.top = pcmSrc->rc.top;
  pcm->rc.left = pcmSrc->rc.left;
  pcm->rc.bottom = pcmSrc->rc.bottom;
  pcm->rc.right = pcmSrc->rc.right;
}

/*************************************************************
* Name: SetCharMetrics()
* Action: Set the character metrics for a specified character.
***************************************************************/
VOID SetCharMetrics(iChar, pcm)
INT iChar;
CM *pcm;
{
  CM *pcmDst;

  pcmDst = &afm.rgcm[iChar];
  pcmDst->iWidth = pcm->iWidth;
  pcmDst->rc.top = pcm->rc.top;
  pcmDst->rc.left = pcm->rc.left;
  pcmDst->rc.bottom = pcm->rc.bottom;
  pcmDst->rc.right = pcm->rc.right;
}

/************************************************************
* Name: GetSmallCM()
* Action: Compute the character metrics for small sized characters
*   such as superscripts.
**************************************************************/
VOID GetSmallCM(iCh, pcm)
INT iCh;
CM *pcm;
{
  GetCharMetrics(iCh, pcm);
  pcm->iWidth = pcm->iWidth / 2;
  pcm->rc.bottom = pcm->rc.top + (pcm->rc.top - pcm->rc.bottom)/2;
  pcm->rc.right = pcm->rc.left + (pcm->rc.right - pcm->rc.left)/2;
}

/*************************************************************
* Name: SetFractionMetrics()
* Action: Set the character metrics for a fractional character
*   which must be simulated.
***************************************************************/
VOID SetFractionMetrics(iChar, iTop, iBottom, pcl)
INT iChar;        /* The character code point */
INT iTop;         /* The ascii numerator character */
INT iBottom;      /* The denominator character */
INT pcl;          /* device type */
{
  INT cxBottom;   /* The width of the denominator */
  CM cm;

#define IFRACTIONBAR  167

  /* Set denominator width to 60 percent of bottom character */
  GetCharMetrics(iBottom, &cm);
  cxBottom = (INT)((long)cm.iWidth * (long)((pcl) ? CVTTOSCR(60) : 60)
        / (long)((pcl) ? CVTTOSCR(100) : 100));

  /* Set numerator width to 40 percent of top character */
  GetCharMetrics(iTop, &cm);
  cxBottom = (INT)((long)cm.iWidth * (long)((pcl) ? CVTTOSCR(40) : 40)
        / (long)((pcl) ? CVTTOSCR(100) : 100));

  cm.iWidth = iTop + iBottom + (pcl) ? CVTTOSCR(IFRACTIONBAR) : IFRACTIONBAR;
  cm.rc.right = cm.rc.left + cm.iWidth;
  SetCharMetrics(iChar, &cm);
}

/***********************************************************************
* Name: FixCharWidths()
* Action: Fix up the character widths for those characters which
*   must be simulated in the driver.
*************************************************************************/
VOID FixCharWidths()
{
  CM cm;
  CM cmSubstitute;
  INT i;

#if 0
  if (afm.iFamily == FF_DECORATIVE) {
        GetCharMetrics(ISPACE, &cmSubstitute);
    for (i = afm.iFirstChar; i <= afm.iLastChar; ++i) {
          GetCharMetrics(i, &cm);
          if (cm.iWidth == 0) {
            SetCharMetrics(i, &cmSubstitute);
                }
          }
        return;
        }

  /* this is a text font */
  GetCharMetrics(IBULLET, &cmSubstitute);
  for (i=0x07f; i<0x091; ++i) SetCharMetrics(i, &cmSubstitute);
  for (i=0x098; i<0x0a1; ++i) SetCharMetrics(i, &cmSubstitute);
#else
  /* yh 8-27-91  Added some characters for Windows 3.1. */
  if (afm.iFamily == FF_DECORATIVE)
        GetCharMetrics(ISPACE, &cmSubstitute);
  else {                                  /* WINANSI encoding */
        GetCharMetrics(ISPACE, &cm);      /* 'space' is encoded twice */
        SetCharMetrics(IWINSPACE, &cm);
        GetCharMetrics(IBULLET, &cmSubstitute);
        }
  for (i = afm.iFirstChar; i <= afm.iLastChar; ++i) {
        GetCharMetrics(i, &cm);
        if (cm.iWidth == 0)
            SetCharMetrics(i, &cmSubstitute);
        }
#endif
}

/***************************************************************
* Name: SetAfm()
* Action: Set the character metrics in the afm to their default values.
********************************************************************/
VOID SetAfm()
{
  INT i, cx;

  afm.iFirstChar = 0x0020;
  afm.iLastChar = 0x00ff;

  if( !afm.fVariablePitch ) {
    cx = afm.rgcm[afm.iFirstChar].iWidth;
    for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
        afm.rgcm[i].iWidth = (SHORT)cx;
    }
  SetAvgWidth();
  SetMaxWidth();
}

/******************************************************************
* Name: SetAvgWidth()
* Action: This routine computes the average character width
*   from the character metrics in the afm structure.
********************************************************************/
VOID SetAvgWidth()
{
  CM *rgcm;
  INT i;
  long cx;    /* The average character width */
  long cb;    /* The number of characters */

  rgcm = afm.rgcm;

  cx = 0L;
  cb = (long) (afm.iLastChar - afm.iFirstChar + 1);
  for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
    cx += (long) rgcm[i].iWidth;
  afm.iAvgWidth = (INT) (cx / cb);
}

/*****************************************************************
* Name: SetMaxWidth()
* Action: This routine computes the maximum character width from
*   the character metrics in the afm structure.
*******************************************************************/
VOID SetMaxWidth()
{
  CM *rgcm;
  INT cx;
  INT i;

  rgcm = afm.rgcm;

  cx = 0;
  for (i=afm.iFirstChar; i<=afm.iLastChar; ++i)
    if (rgcm[i].iWidth > cx) cx = rgcm[i].iWidth;
  afm.iMaxWidth = (SHORT)cx;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/******************************************************************
* Name: ResetBuffer()
* Action: This function resets the output buffer.
********************************************************************/
VOID ResetBuffer()
{
  pbBuffer = rgbBuffer;
  cbBuffer = 0;
}

/****************************************************************
* Name: PutByte()
* Action: This function writes a byte to the output buffer.
******************************************************************/
VOID PutByte(iByte)
SHORT iByte;
{
  *pbBuffer++ = (BYTE) (iByte & 0x0ff);
  ++cbBuffer;
}

/****************************************************************
* Name: PutRgb()
* Action: This function writes an array of bytes to the output buffer.
******************************************************************/
VOID PutRgb(pb, cb)
LPSZ pb;
INT cb;
{
  while (--cb>=0)
  PutByte(*pb++);
}

/****************************************************************
* Name: PutWord()
* Action: This function writes a word to the output buffer.
******************************************************************/
VOID PutWord(iWord)
SHORT iWord;
{
  *pbBuffer++ = (CHAR) (iWord & 0x0ff);
  *pbBuffer++ = (CHAR) ( (iWord >> 8) & 0x0ff );
  cbBuffer += 2;
}

/****************************************************************
* Name: PutLong()
* Action: This function writes a long word to the output buffer.
******************************************************************/
VOID PutLong(lWord)
long lWord;
{
  PutWord((WORD) (lWord & 0x0ffffL));
  lWord >>= 16;
  PutWord((WORD) (lWord & 0x0ffffL));
}

/**************************************************************
* Name: SetDf()
* Action: This function sets the values in the device font structure
*         from the values in the afm structure.
*****************************************************************/
static CHAR szCopyright[] = "Copyright 1988-1991 Adobe Systems Inc.";
VOID SetDf(pcl)
INT pcl;
{
//WORD minAscent;
  WORD pixHeight;
  WORD internalLeading;
  SHORT leading;
  #ifndef FF_MASKFAMILY
    #define FF_MASKFAMILY ((BYTE) 0xF0)
  #endif
  #define MAX(a,b) ((a)>(b)?(a):(b))

  pfm.iVersion = 0x0100;        /* Version 1.00 */
  szMove(pfm.szCopyright, szCopyright, sizeof(pfm.szCopyright));
  pfm.iType = (pcl) ? PCL_FONTTYPE : PS_FONTTYPE;
  pfm.iCharSet = (charset == -1) ? (BYTE) ANSI_CHARSET : (BYTE) charset;
   /* (pcl && (afm.iFamily==FF_DECORATIVE)) ? PCL_PI_CHARSET : ANSI_CHARSET );
      Windows WRITE only displays fonts with CharSet=0 in the menu */
  pfm.iDefaultChar = (BYTE) (
       ( (afm.iFamily==FF_DECORATIVE) ? ISPACE : IBULLET ) - afm.iFirstChar );
  pfm.iBreakChar = (BYTE) (ISPACE - afm.iFirstChar);

  /* for a scalable font (i.e. PostScript) default to 80 column text */
  pfm.iPoints = (pcl) ? afm.iPtSize : 10;

  /* if we ever support other bitmapped printers we will no longer be able
  to assume that the default x and y res are 300. */
  pfm.iVertRes = 300;
  pfm.iHorizRes = 300;
  pfm.iItalic = (BYTE) ((afm.iItalicAngle != 0) ? 1 : 0);
  pfm.iWeight = afm.iWeight;
  pfm.iPitchAndFamily = (BYTE) afm.iFamily;
  pfm.iFirstChar = (BYTE) afm.iFirstChar;
  pfm.iLastChar = (BYTE) afm.iLastChar;
  pfm.iAvgWidth = afm.iAvgWidth;
  pfm.iMaxWidth = afm.iMaxWidth;
  pfm.iPixWidth = (afm.fVariablePitch) ? 0 : afm.iAvgWidth;
/* pfm.iPixHeight = afm.rcBBox.top - afm.rcBBox.bottom;
 * Changed to reduce round off error.  8-26-91 yh
 */
  pixHeight = afm.rcBBox.top - afm.rcBBox.bottom;
  pfm.iPixHeight = (pcl) ? CVTTOSCR(pixHeight) : pixHeight;
/*  pfm.iInternalLeading =
 *    (pcl) ? pfm.iPixHeight - ((afm.iPtSize * 300) / 72) : 0;
 *  Changed to match ATM.  7-31-91 yh
 *  Changed to reduce round off error.  8-26-91 yh
 */
  internalLeading = max(0, pixHeight - EM);
  pfm.iInternalLeading = (pcl) ? CVTTOSCR(internalLeading) : internalLeading;

/*  pfm.iAscent = afm.rcBBox.top;
 *  Changed to fix text alignment problem.  10-08-90 yh
 */
/*  pfm.iAscent = afm.iAscent;
 *  Changed to match ATM.  7-31-91 yh
 *  Changed to reduce round off error.  8-26-91 yh
 */
  pfm.iAscent = (pcl) ?
                CVTTOSCR(EM + afm.rcBBox.bottom) + CVTTOSCR(internalLeading) :
                EM + afm.rcBBox.bottom + internalLeading;
/* Deleted to match ATM.  yh 9-13-91
 * minAscent = (pcl) ? CVTTOSCR(667) : 667;          2/3 of EM
 * if( pfm.iAscent < minAscent ) pfm.iAscent = minAscent;
 */

/*  pfm.iExternalLeading = 196; */
/*  Changed to 0 to fix a bug in PCL landscape.  Was getting huge leading. */
  /*
   * yh 8-26-91  Changed ExternalLeading for pcl to match ATM .
   */
  if (!pcl)
      /* PostScript driver ignores this field and comes up with own
       * ExternalLeading value.
       *
       * !!! HACK ALERT !!!
       *
       * ATM needs to have ExternalLeading=0.  PFMs generated with Rev. 2
       * MAKEPFM have a bug in default & break character fields.  We had
       * encoding number instead of offsets.  ATM uses following algorithm
       * to recognize the Rev. 2 PFMs:
       *     rev2pfm = pfmRec->fmExternalLeading != 0 &&
       *               etmRec->etmStrikeOutOffset == 500 &&
       *               pfmRec->fmDefaultChar >= pfmRec->fmFirstChar;
       * So, we need to make sure that either ExternalLeading stays zero or
       * StrikeOutOffset is not 500.  With current algorithm, StrikeOutOffset
       * is very likely to be less than 500.
       *     etm.iStrikeOutOffset = fiCapHeight / 2 - (afm.ulThick / 2);
       */
      pfm.iExternalLeading = 0;
  else if (!afm.fWasVariablePitch)
      pfm.iExternalLeading = 0;
  else                               /* pcl & Variable pitch */
      {
      /* Adjust external leading such that we are compatible */
      /* with the values returned by the PostScript driver.  */
      /* Who did this code??  Microsoft?  Has to be! */
      switch (pfm.iPitchAndFamily & FF_MASKFAMILY)
        {
        case FF_ROMAN:  leading = (pfm.iVertRes + 18) / 36; //2-pnt leading
                        break;
        case FF_SWISS:  if (pfm.iPoints <= 12)
                          leading = (pfm.iVertRes + 18) / 36; //2-pnt leading
                        else if (pfm.iPoints < 14)
                          leading = (pfm.iVertRes + 12) / 24; //3-pnt leading
                        else
                          leading = (pfm.iVertRes + 9) / 18; //4-pnt leading
                        break;
        default:                /* Give 19.6% of the height for leading. */
                        leading = (short) (
                                  (long) (pfm.iPixHeight-pfm.iInternalLeading)
                                  * 196L / 1000L );
                        break;
        }

      pfm.iExternalLeading = MAX(0, (SHORT)(leading - pfm.iInternalLeading));
      }

  pfm.iWidthBytes = 0;
  if (afm.fVariablePitch) pfm.iPitchAndFamily |= 1;

  pfm.iUnderline = 0;
  pfm.iStrikeOut = 0;
  pfm.oBitsPointer = 0L;
  pfm.oBitsOffset = 0L;
}

/**********************************************************
* Name: PutString()
* Action: This function writes a null terminated string
*       to the output file.
***********************************************************/
VOID PutString(sz)
LPSZ sz;
{
  INT bCh;

  do    {
      bCh = *pbBuffer++ = *sz++;
      ++cbBuffer;
      } while( bCh );
}

/***************************************************************
* Name: PutdeviceName()
* Action: This function writes the device name to the output file.
**************************************************************/
VOID PutDeviceName(szDevice)
LPSZ szDevice;
{
  pfm.oDevice = cbBuffer;
  PutString(szDevice);
}

/***************************************************************
* Name: PutFaceName()
* Action: This function writes the font's face name to the output file.
**************************************************************/
VOID PutFaceName()
{
  pfm.oFace = cbBuffer;
  PutString(afm.szFace);
}

/**************************************************************
* Name: MakeDf()
* Action: This function writes the device font info structure
*       to the output file.
* Method: This function makes two passes over the data. On the first pass
* it collects offset data as it places data in the output buffer. On the
* second pass, it first resets the output buffer and then writes the data
* to the output buffer again with the offsets computed from pass 1.
***************************************************************/
BOOL MakeDf(fPass2, devType, outfile)
BOOL fPass2;            /* TRUE if this is the second pass */
SHORT devType;  /* 1=POSTSCRIPT 2=PCL */
LPSZ outfile;
{
  BOOL result = TRUE;
  INT iMarker;

  ResetBuffer();
  SetDf(devType == PCL);

  /* put out the PFM header structure */
  PutWord(pfm.iVersion);
  PutLong(pfm.iSize);
  PutRgb(pfm.szCopyright, 60);
  PutWord(pfm.iType);
  PutWord(pfm.iPoints);
  PutWord(pfm.iVertRes);
  PutWord(pfm.iHorizRes);
  PutWord(pfm.iAscent);
  PutWord(pfm.iInternalLeading);
  PutWord(pfm.iExternalLeading);
  PutByte(pfm.iItalic);
  PutByte(pfm.iUnderline);
  PutByte(pfm.iStrikeOut);
  PutWord(pfm.iWeight);
  PutByte(pfm.iCharSet);
  PutWord(pfm.iPixWidth);
  PutWord(pfm.iPixHeight);
  PutByte(pfm.iPitchAndFamily);
  PutWord(pfm.iAvgWidth);
  PutWord(pfm.iMaxWidth);
  PutByte(pfm.iFirstChar);
  PutByte(pfm.iLastChar);
  PutByte(pfm.iDefaultChar);
  PutByte(pfm.iBreakChar);
  PutWord(pfm.iWidthBytes);
  PutLong(pfm.oDevice);
  PutLong(pfm.oFace);
  PutLong(pfm.oBitsPointer);
  PutLong(pfm.oBitsOffset);

  /* need to determine if proportional etc. */
  if (devType == PCL) PutExtentOrWidthTable(1);

  /* put out the PFM extension structure */
  iMarker = cbBuffer;
  PutWord(pfmext.oSizeFields);
  PutLong(pfmext.oExtMetricsOffset);
  PutLong(pfmext.oExtentTable);
  PutLong(pfmext.oOriginTable);
  PutLong(pfmext.oPairKernTable);
  PutLong(pfmext.oTrackKernTable);
  PutLong(pfmext.oDriverInfo);
  PutLong(pfmext.iReserved);
  pfmext.oSizeFields = cbBuffer - iMarker;
  if (devType == POSTSCRIPT) {
    /* Put the extended text metrics table */
    pfmext.oExtMetricsOffset = cbBuffer;
    PutEtm(FALSE);

    PutDeviceName("PostScript");
    PutFaceName();
    PutDriverInfo(FALSE);

    /* Put the extent table */
    PutExtentOrWidthTable(0);

    pfmext.oOriginTable = 0;
    pfmext.iReserved = 0;
    PutPairKernTable(POSTSCRIPT);
    PutTrackKernTable(POSTSCRIPT);
    }

  if (devType == PCL) {
    PutFaceName();
    PutDeviceName("PCL/HP LaserJet");

    /* Put the extended text metrics table */
    pfmext.oExtMetricsOffset = cbBuffer;
    PutEtm(TRUE);

    PutPairKernTable(PCL);
    PutTrackKernTable(PCL);

    PutDriverInfo(TRUE);
    pfmext.oOriginTable = 0;
    pfmext.iReserved = 0;
    }

  if( !fPass2 ) {
    pfm.iSize = (long)cbBuffer;
    if( !MakeDf(TRUE, devType, outfile) ) result = FALSE;
    }
  else {
    if( !WritePfm(outfile) ) result = FALSE;
#if DEBUG_MODE
    DumpPfmHeader();
    DumpCharWidths();
    DumpPfmExtension();
#endif
    }
  return(result);
}

/*******************************************************************
* Name: PutPairKernTable(devType)
* Action: Send the pairwise kerning table to the output file.
*********************************************************************/
VOID PutPairKernTable(devType)
SHORT devType;  /* 1=POSTSCRIPT 2=PCL */
{
  WORD i;

  if( afm.kp.cPairs > 0 ) {
      pfmext.oPairKernTable = cbBuffer;
#if DEBUG_MODE
      printf("Pair Kern Table - pairs: %d\n", afm.kp.cPairs);
#endif
      if( devType == POSTSCRIPT ) PutWord(afm.kp.cPairs);
      for (i = 0; i < afm.kp.cPairs; ++i) {
          PutWord(afm.kp.rgPairs[i].iKey);
          PutWord(afm.kp.rgPairs[i].iKernAmount);
#if DEBUG_MODE
          printf("key: %x kern amount: %d\n",
          afm.kp.rgPairs[i].iKey, afm.kp.rgPairs[i].iKernAmount);
#endif
          }
      }
  else
      pfmext.oPairKernTable = 0;
}

/******************************************************************
* Name: PutTrackKernTable(devType)
* Action: Send the track kerning table to the output file.
********************************************************************/
VOID PutTrackKernTable(devType)
SHORT devType;  /* 1=POSTSCRIPT 2=PCL */
{
  INT i;

  if (afm.kt.cTracks == 0)
    {
    pfmext.oTrackKernTable = 0;
    return;
    }

  pfmext.oTrackKernTable = cbBuffer;
  if (devType == POSTSCRIPT) PutWord(afm.kt.cTracks);
  for (i=0; i<afm.kt.cTracks; ++i)
    {
    PutWord(afm.kt.rgTracks[i].iDegree);
    PutWord(afm.kt.rgTracks[i].iPtMin);
    PutWord(afm.kt.rgTracks[i].iKernMin);
    PutWord(afm.kt.rgTracks[i].iPtMax);
    PutWord(afm.kt.rgTracks[i].iKernMax);
    }
}

/***************************************************************
* Name: PutExtentTable()
* Action: Send the character extent information to the output file.
*****************************************************************/
VOID PutExtentOrWidthTable(width)
INT width; /* 0=extent 1=width */
{
  INT i;

  /* is the typeface proportional ?? */
  if (pfm.iPitchAndFamily & 1)
    {
    pfmext.oExtentTable = (width) ? 0 : cbBuffer;
    for (i = afm.iFirstChar; i <= afm.iLastChar; i++)
      PutWord(afm.rgcm[i].iWidth);
    if (width) PutWord(0);
    }
  else
    pfmext.oExtentTable = 0;
}

/***********************************************************
* Name: WritePfm()
* Action: Flush the ouput buffer to the file.  Note that this
*         function is only called after the entire pfm structure
*         has been built in the output buffer.
*************************************************************/
BOOL WritePfm(outfile)
LPSZ outfile;
{
  INT fh;

  if( (fh = OpenTargetFile(outfile) ) == -1 ) {
      ; // PostError(str(MSG_PFM_BAD_CREATE), outfile);
      return(FALSE);
      }

  if( cbBuffer > 0  )
    if( (WORD)WRITE_BLOCK(fh, rgbBuffer, cbBuffer) != (WORD)cbBuffer ) {
        CLOSE(fh);
        ; // PostError(str(MSG_PFM_DISK_FULL));
        return(FALSE);
        }
  CLOSE(fh);
  return(TRUE);
}

#if DEBUG_MODE
/*----------------------------------------------------------------------------*/
VOID DumpPfmHeader()
{
  printf("\nDUMP PFM HEADER\n");
  printf("pfm.iVersion=%d\n",pfm.iVersion);
  printf("pfm.iSize=%ld\n",pfm.iSize);
  printf("pfm.szCopyright=%s\n",pfm.szCopyright);
  printf("pfm.iType=%d\n",pfm.iType);
  printf("pfm.iPoints=%d\n",pfm.iPoints);
  printf("pfm.iVertRes=%d\n",pfm.iVertRes);
  printf("pfm.iHorizRes=%d\n",pfm.iHorizRes);
  printf("pfm.iAscent=%d\n",pfm.iAscent);
  printf("pfm.iInternalLeading=%d\n",pfm.iInternalLeading);
  printf("pfm.iExternalLeading=%d\n",pfm.iExternalLeading);
  printf("pfm.iItalic=%d\n",pfm.iItalic);
  printf("pfm.iUnderline=%d\n",pfm.iUnderline);
  printf("pfm.iStrikeOut=%d\n",pfm.iStrikeOut);
  printf("pfm.iWeight=%d\n",pfm.iWeight);
  printf("pfm.iCharSet=%d\n",pfm.iCharSet);
  printf("pfm.iPixWidth=%d\n",pfm.iPixWidth);
  printf("pfm.iPixHeight=%d\n",pfm.iPixHeight);
  printf("pfm.iPitchAndFamily=%d\n",pfm.iPitchAndFamily);
  printf("pfm.iAvgWidth=%d\n",pfm.iAvgWidth);
  printf("pfm.iMaxWidth=%d\n",pfm.iMaxWidth);
  printf("pfm.iFirstChar=%c\n",pfm.iFirstChar);
  printf("pfm.iLastChar=%c\n",pfm.iLastChar);
  printf("pfm.iDefaultChar=%d\n",pfm.iDefaultChar);
  printf("pfm.iBreakChar=%d\n",pfm.iBreakChar);
  printf("pfm.iWidthBytes=%d\n",pfm.iWidthBytes);
  printf("pfm.oDevice=%x\n",pfm.oDevice);
  printf("pfm.oFace=%x\n",pfm.oFace);
  printf("pfm.oBitsPointer=%ld\n",pfm.oBitsPointer);
  printf("pfm.oBitsOffset=%ld\n",pfm.oBitsOffset);
}
/*----------------------------------------------------------------------------*/
VOID DumpCharWidths()
{
  INT indx;

  printf("\nCHARACTER WIDTHS\n");
  for (indx = afm.iFirstChar; indx <= afm.iLastChar; indx++)
    printf("indx: %d width: %d\n", indx, afm.rgcm[indx].iWidth);
}
/*----------------------------------------------------------------------------*/
VOID DumpPfmExtension()
{
  printf("\nDUMP PFM EXTENSION\n");
  printf("pfmext.oSizeFields=%d\n",pfmext.oSizeFields);
  printf("pfmext.oExtMetricsOffset=%x\n",pfmext.oExtMetricsOffset);
  printf("pfmext.oExtentTable=%x\n",pfmext.oExtentTable);
  printf("pfmext.oOriginTable=%x\n",pfmext.oOriginTable);
  printf("pfmext.oPairKernTable=%x\n",pfmext.oPairKernTable);
  printf("pfmext.oTrackKernTable=%x\n",pfmext.oTrackKernTable);
  printf("pfmext.oDriverInfo=%x\n",pfmext.oDriverInfo);
  printf("pfm.iReserved=%x\n",pfm.iReserved);
}
#endif
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Main purpose of these structures is to set up a translation table which
allows the driver to translate the font from the character set indicated in
the dfCharset field into the printer-specific character set. */

#define AVGSIZE  (30 * 1024)

VOID SetDriverInfo()
{
  INT i;
  long sumWidth = 0L;

  for (i = afm.iFirstChar; i <= afm.iLastChar; i++)
      sumWidth = sumWidth + (long)afm.rgcm[i].iWidth;

  d.epSize = sizeof(DRIVERINFO);
  d.epVersion = DRIVERINFO_VERSION;
  d.epMemUsage = (long) ( ((sumWidth+7L) >> 3) * (long)pfm.iPixHeight + 63L );
  d.xtbl.symbolSet = pclinfo.symbolsetNum;
  d.xtbl.offset = 0L;
  d.xtbl.len = 0;
  d.xtbl.firstchar = 0;
  d.xtbl.lastchar = 0;
  pclinfo.epEscapeSequence = GetEscapeSequence();
}
/*----------------------------------------------------------------------------*/
VOID PutDriverInfo(pcl)
INT pcl;
{
  pfmext.oDriverInfo = cbBuffer;
  if (pcl) {
    SetDriverInfo();
    PutWord(d.epSize);
    PutWord(d.epVersion);
    PutLong(d.epMemUsage);
    PutLong(d.epEscape);
    PutWord((WORD)d.xtbl.symbolSet);
    PutLong(d.xtbl.offset);
    PutWord(d.xtbl.len);
    PutByte(d.xtbl.firstchar);
    PutByte(d.xtbl.lastchar);
    d.epEscape = cbBuffer;
    PutString(pclinfo.epEscapeSequence);
  } else
    PutString(afm.szFont);
}

/*--------------------------------------------------------------------------*/
LPSZ GetEscapeSequence()
{
  static char escapeStr[80];
  char fixedPitch[2], pitch[10], height[10], *cp;
  int enc;
  float size;

  size = (float) afm.iPtSize;
  if( afm.fWasVariablePitch == TRUE ) {
      STRCPY(fixedPitch, "1");
      enc = ISPACE;
      }
  else {
      STRCPY(fixedPitch, "");
      enc = afm.iFirstChar;
      }
  sprintf(pitch, "%1.3f", 300.0 / (float)afm.rgcm[enc].iWidth);
  if( cp = strchr(pitch, '.') ) cp[3] = '\0';

  sprintf(height, "%1.2f", size);

  sprintf(escapeStr, "\x01B&l%dO\x01B(%s\x01B(s%sp%sh%sv%ds%db%uT",
          pclinfo.orientation, pclinfo.symbolsetStr,
          fixedPitch, pitch, height,
          pclinfo.style, pclinfo.strokeWeight, pclinfo.typeface);
  return(escapeStr);
}

/*----------------------------------------------------------------------------*/
#if DEBUG_MODE
VOID DumpDriverInfo()
{
  printf("\nDUMP DRIVERINFO STRUCTURE\n");
  printf("d.epSize: %d\n", d.epSize);
  printf("d.epVersion: %d\n", d.epVersion);
  printf("d.epMemUsage: %ld\n", d.epMemUsage);
  printf("d.epEscape: %ld\n", d.epEscape);
  printf("d.xtbl.symbolSet: %d\n", d.xtbl.symbolSet);
  printf("d.xtbl.offset: %ld\n", d.xtbl.offset);
  printf("d.xtbl.len: %d\n", d.xtbl.len);
  printf("d.xtbl.firstchar: %d\n", d.xtbl.firstchar);
  printf("d.xtbl.lastchar: %d\n", d.xtbl.lastchar);
  printf("d.epEscapeSequence: %s\n", d.epEscapeSequence);
}
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Convert from PostScript to extended text metrics */
VOID AfmToEtm(pcl)
BOOL pcl;  /* true if this is a PCL type device */
{
  etm.iSize = 52;
  /* point size in twips */
  etm.iPointSize = afm.iPtSize * 20;
  etm.iOrientation = (pcl) ? pclinfo.orientation + 1 : 0;
  etm.iMasterHeight = (pcl) ? pfm.iPixHeight : 1000;
  etm.iMinScale = (pcl) ? etm.iMasterHeight : 3;
  etm.iMaxScale = (pcl) ? etm.iMasterHeight : 1000;

  etm.iMasterUnits = (pcl) ? etm.iMasterHeight : 1000;

  /* in general need to worry a little about what happens if these various
     glyphs are not present as in a decorative font. */

  etm.iCapHeight = afm.rgcm['H'].rc.top;
  etm.iXHeight = afm.rgcm['x'].rc.top;
  etm.iLowerCaseAscent =  afm.rgcm['d'].rc.top;
  etm.iLowerCaseDescent = - afm.rgcm['p'].rc.bottom;
  etm.iSlant = (pcl) ? afm.iItalicAngle * 10 : afm.iItalicAngle;
  etm.iSuperScript = (pcl) ? 0 : -500;
  etm.iSubScript = (pcl) ? 0 : 250;
  etm.iSuperScriptSize = (pcl) ? 0 : 500;
  etm.iSubScriptSize = (pcl) ? 0 : 500;
  etm.iUnderlineOffset = (pcl) ? 0 : afm.ulOffset;
  etm.iUnderlineWidth = (pcl) ? 1 : afm.ulThick;
  etm.iDoubleUpperUnderlineOffset = (pcl) ? 0 : afm.ulOffset / 2;
  etm.iDoubleLowerUnderlineOffset = (pcl) ? 0 : afm.ulOffset;
  etm.iDoubleUpperUnderlineWidth = (pcl) ? 1 : afm.ulThick / 2;
  etm.iDoubleLowerUnderlineWidth = (pcl) ? 1 : afm.ulThick / 2;
  etm.iStrikeOutOffset = (pcl) ? 0 : fiCapHeight / 2 - (afm.ulThick / 2);
  etm.iStrikeOutWidth = (pcl) ? 1 : afm.ulThick;
  etm.nKernPairs = afm.kp.cPairs;
  etm.nKernTracks = afm.kt.cTracks;
}
/*----------------------------------------------------------------------------*/
VOID PutEtm(pcl)
BOOL pcl;  /* true if this is a PCL type device */
{
  AfmToEtm(pcl);
  PutWord(etm.iSize);
  PutWord(etm.iPointSize);
  PutWord(etm.iOrientation);
  PutWord(etm.iMasterHeight);
  PutWord(etm.iMinScale);
  PutWord(etm.iMaxScale);
  PutWord(etm.iMasterUnits);
  PutWord(etm.iCapHeight);
  PutWord(etm.iXHeight);
  PutWord(etm.iLowerCaseAscent);
  PutWord(etm.iLowerCaseDescent);
  PutWord(etm.iSlant);
  PutWord(etm.iSuperScript);
  PutWord(etm.iSubScript);
  PutWord(etm.iSuperScriptSize);
  PutWord(etm.iSubScriptSize);
  PutWord(etm.iUnderlineOffset);
  PutWord(etm.iUnderlineWidth);
  PutWord(etm.iDoubleUpperUnderlineOffset);
  PutWord(etm.iDoubleLowerUnderlineOffset);
  PutWord(etm.iDoubleUpperUnderlineWidth);
  PutWord(etm.iDoubleLowerUnderlineWidth);
  PutWord(etm.iStrikeOutOffset);
  PutWord(etm.iStrikeOutWidth);
  PutWord(etm.nKernPairs);
  PutWord(etm.nKernTracks);
#if DEBUG_MODE
  DumpEtm();
#endif
}
/*----------------------------------------------------------------------------*/
#if DEBUG_MODE
VOID DumpEtm()
{
  printf("\nDUMP ETM STRUCTURE\n");
  printf("etm.iSize: %d\n", etm.iSize);
  printf("etm.iPointSize: %d\n", etm.iPointSize);
  printf("etm.iOrientation: %d\n", etm.iOrientation);
  printf("etm.iMasterHeight: %d\n", etm.iMasterHeight);
  printf("etm.iMinScale: %d\n", etm.iMinScale);
  printf("etm.iMaxScale: %d\n", etm.iMaxScale);
  printf("etm.iMasterUnits: %d\n", etm.iMasterUnits);
  printf("etm.iCapHeight: %d\n", etm.iCapHeight);
  printf("etm.iXHeight: %d\n", etm.iXHeight);
  printf("etm.iLowerCaseAscent: %d\n", etm.iLowerCaseAscent);
  printf("etm.iLowerCaseDescent: %d\n", etm.iLowerCaseDescent);
  printf("etm.iSlant: %d\n", etm.iSlant);
  printf("etm.iSuperScript: %d\n", etm.iSuperScript);
  printf("etm.iSubScript: %d\n", etm.iSubScript);
  printf("etm.iSuperScriptSize: %d\n", etm.iSuperScriptSize);
  printf("etm.iSubScriptSize: %d\n", etm.iSubScriptSize);
  printf("etm.iUnderlineOffset: %d\n", etm.iUnderlineOffset);
  printf("etm.iUnderlineWidth: %d\n", etm.iUnderlineWidth);
  printf("etm.iDoubleUpperUnderlineOffset: %d\n",
    etm.iDoubleUpperUnderlineOffset);
  printf("etm.iDoubleLowerUnderlineOffset: %d\n",
    etm.iDoubleLowerUnderlineOffset);
  printf("etm.iDoubleUpperUnderlineWidth: %d\n",
    etm.iDoubleUpperUnderlineWidth);
  printf("etm.iDoubleLowerUnderlineWidth: %d\n",
    etm.iDoubleLowerUnderlineWidth);
  printf("etm.iStrikeOutOffset: %d\n", etm.iStrikeOutOffset);
  printf("etm.iStrikeOutWidth: %d\n", etm.iStrikeOutWidth);
  printf("etm.nKernPairs: %d\n", etm.nKernPairs);
  printf("etm.nKernTracks: %d\n", etm.nKernTracks);
}
#endif
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**************************************************************
* Name: StartParse()
***************************************************************/
VOID StartParse()
{
  fEOF = FALSE;
  fUnGetLine = FALSE;
  cbBuffer = 0;
}

/**************************************************************
* Name: szIsEqual()
* Action: Compare two NULL terminated strings.
* Returns: TRUE if they are equal FALSE if they are different
***************************************************************/
BOOL szIsEqual(sz1, sz2)
LPSZ sz1;
LPSZ sz2;
{
  while (*sz1 && *sz2)
      if (*sz1++ != *sz2++) return(FALSE);
  return(*sz1 == *sz2);
}

/**************************************************************
* Name: szMove()
* Action: Copy a string.  This function will copy at most the
*   number of bytes in the destination area - 1.
***************************************************************/
VOID szMove(szDst, szSrc, cbDst)
LPSZ szDst;   /* Ptr to the destination area */
LPSZ szSrc;   /* Ptr to the source area */
INT cbDst;     /* The size of the destination area */
{
  while (*szDst++ = *szSrc++)
      if (--cbDst <= 0) {
          *(szDst-1) = 0;
          break;
          }
}

/*****************************************************************
* Name: GetBuffer()
* Action: Read a new buffer full of text from the input file.
******************************************************************/
BOOL GetBuffer(hfile)
INT hfile;
{
  cbBuffer = 0;
  if (!fEOF) {
      cbBuffer = READ_BLOCK(hfile, rgbBuffer, sizeof(rgbBuffer));
      if (cbBuffer<=0) {
          cbBuffer = 0;
          fEOF = TRUE;
          }
      }
  pbBuffer = rgbBuffer;
  return(!fEOF);
}

/*****************************************************************
* Name: UnGetLine()
* Action: This routine pushes the most recent line back into the
*   input buffer.
*******************************************************************/
VOID UnGetLine()
{
  fUnGetLine = TRUE;
  szLine = rgbLine;
}

/******************************************************************
* Name: GetLine()
* Action: This routine gets the next line of text out of the
*   input buffer.  Handles both binary & text mode.
********************************************************************/
BOOL GetLine(hfile)
INT hfile;
{
  CHAR szWord[10];

  // WriteDots();
  szLine = rgbLine;
  do {                                            /* skip comment lines */
      if( !_GetLine(hfile) ) return(FALSE);
      GetWord(szWord, sizeof(szWord));
      } while( szIsEqual("Comment", szWord) );
  szLine = rgbLine;
  return(TRUE);
}

BOOL _GetLine(hfile)
INT hfile;
{
  INT cbLine;
  CHAR bCh;

  if( fUnGetLine ) {
      szLine = rgbLine;
      fUnGetLine = FALSE;
      return(TRUE);
      }

  cbLine = 0;
  szLine = rgbLine;
  *szLine = 0;
  if( !fEOF )
  {
      while( TRUE )
      {
          if ( cbBuffer <= 0 )
              if( !GetBuffer(hfile) ) return(FALSE);
          while( --cbBuffer >= 0 )
          {
              bCh = *pbBuffer++;
              if( bCh=='\n' || ++cbLine > (sizeof(rgbLine)-1) )
              {
                  *szLine = 0;
                  szLine = rgbLine;
                  EatWhite();
                  if( *szLine != 0 ) goto DONE;
                  szLine = rgbLine;
                  cbLine = 0;
                  continue;
              }
              else if( bCh >= ' ' )
              {
                *szLine++ = bCh;
              }
          }
      }
  }
  *szLine = 0;

DONE:
  szLine = rgbLine;
  return(!fEOF);
}

/****************************************************************
* Name: EatWhite()
* Action: This routine moves the input buffer pointer forward to
*   the next non-white character.
******************************************************************/
VOID EatWhite()
{
  while (*szLine && (*szLine==' ' || *szLine=='\t'))
  ++szLine;
}

/*******************************************************************
* Name: GetWord()
* Action: This routine gets the next word delimited by white space
*   from the input buffer.
*********************************************************************/
VOID GetWord(szWord, cbWord)
LPSZ szWord;   /* Ptr to the destination area */
INT cbWord;     /* The size of the destination area */
{
  CHAR bCh;

  EatWhite();
  while (--cbWord>0) {
      switch(bCh = *szLine++) {
          case 0:
          case ' ':
          case '\t': --szLine;
                     goto DONE;
          case ';':  *szWord++ = bCh;
                     goto DONE;
          default:   *szWord++ = bCh;
                     break;
          }
      }
DONE:
  *szWord = 0;
}

/*******************************************************************
* Name: GetString()
* Action: This routine gets the next word delimited by parentheses
*   from the input buffer.
*********************************************************************/
BOOL GetString(szWord, cbWord)
LPSZ szWord;   /* Ptr to the destination area */
INT   cbWord;   /* The size of the destination area */
{
  CHAR bCh;
  BOOL result = TRUE;

  EatWhite();
  if( *szLine == '(' ) szLine++;
  else result = FALSE;
  while (--cbWord>0) {
      switch(bCh = *szLine++) {
          case 0:   result = FALSE;
                    goto DONE;
          case ')': --szLine;
                    goto DONE;
          default:  *szWord++ = bCh;
                    break;
          }
      }
DONE:
  *szWord = 0;
  return(result);
}

/************************************************************
* Name: GetNumber()
* Action: This routine parses an ASCII decimal number from the
*   input file stream and returns its value.
***************************************************************/
BOOL GetNumber(piVal)
SHORT *piVal;
{
  INT iVal;
  BOOL fNegative;

  fNegative = FALSE;

  iVal = 0;
  EatWhite();

  if (*szLine=='-') {
      fNegative = TRUE;
      ++szLine;
      }

  if (*szLine<'0' || *szLine>'9') {
      *piVal = 0;
      return(FALSE);
      }

  while (*szLine>='0' && *szLine<='9')
      iVal = iVal * 10 + (*szLine++ - '0');

  if (fNegative) iVal = - iVal;
  if (*szLine==0 || *szLine==' ' || *szLine=='\t' || *szLine==';') {
      *piVal = (SHORT)iVal;
      return(TRUE);
      }
  else {
      return(FALSE);
  }
}

/******************************************************************
* Name: GetFloat()
* Action: This routine parses an ASCII floating point decimal number
*   from the input file stream and returns its value scaled
*   by a specified amount.
*********************************************************************/
BOOL GetFloat(pScale, piVal)
float *pScale;     /* The amount to scale the value by */
SHORT *piVal;
{
  float scale;
  long lVal;
  long lDivisor;
  BOOL fNegative;

  scale = *pScale;
  EatWhite();
  fNegative = FALSE;
  lVal = 0L;

  if (*szLine=='-') {
      fNegative = TRUE;
      ++szLine;
      }

  if (*szLine<'0' || *szLine>'9') {
      *piVal = 0;
      return(FALSE);
      }

  while (*szLine>='0' && *szLine<='9') lVal = lVal * 10 + (*szLine++ - '0');

  lDivisor = 1L;
  if (*szLine=='.') {
      ++szLine;
      while (*szLine>='0' && *szLine<='9') {
          lVal = lVal * 10 + (*szLine++ - '0');
          lDivisor = lDivisor * 10;
          }
      }
  lVal = (lVal * (long) scale) / lDivisor;
  if (fNegative) lVal = - lVal;
  if (*szLine==0 || *szLine==' ' || *szLine=='\t' || *szLine==';') {
      *piVal = (INT) lVal;
      return(TRUE);
      }
   else {
      return(FALSE);
   }
}

/***************************************************************
* Name: MapToken()
* Action: This routine maps an ascii key word into an integer token.
* Returns: The token value.
******************************************************************/
INT MapToken(szWord, map)
LPSZ szWord;      /* Ptr to the ascii keyword string */
KEY *map;
{
  KEY *pkey;

  pkey = map;
  while (pkey->szKey) {
      if( szIsEqual(szWord, pkey->szKey) ) return(pkey->iValue);
      ++pkey;
      }
  return(TK_UNDEFINED);
}

/*********************************************************************
* Name: GetToken()
* Action: Get the next token from the input stream.
***********************************************************************/
INT GetToken(hfile, map)
INT hfile;
KEY *map;
{
  CHAR szWord[80];

  if (*szLine==0)
      if( !GetLine(hfile) ) return(TK_EOF);
  GetWord(szWord, sizeof(szWord));
  return(MapToken(szWord, map));
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
GlyphName *AllocateGlyphArray(arraymax)

INT arraymax;
{
  GlyphName *p;
  INT i;

  p = (GlyphName *) AllocateMem( (UINT) (sizeof(LPSZ) * (arraymax+2)) );
  if( p == NULL ) {
      ; // PostError(str(MSG_PFM_BAD_MALLOC));
      return(NULL);
      }
  for(i=0; i<=arraymax; i++)
      p[i] = notdef;
  p[i] = NULL;
  return(p);
}

/*--------------------------------------------------------------------------*/
VOID PutGlyphName(array, index, glyph)

GlyphName *array;
INT index;
LPSZ glyph;
{
  LPSZ p;

  if ( !STRCMP(glyph, ".notdef") )
      array[index] = notdef;
  else {
      p = (LPSZ) AllocateMem((UINT) (strlen(glyph)+1));
      STRCPY(p, glyph);
      array[index] = p;
      }
}

/*--------------------------------------------------------------------------*/

