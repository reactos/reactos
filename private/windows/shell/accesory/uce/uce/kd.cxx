/****************************************************************************

    kd.c

    PURPOSE: codepage related utilities for UCE

    Copyright (c) 1997-1999 Microsoft Corporation.
****************************************************************************/

#include "windows.h"
#include "commctrl.h"

#include "UCE.h"
#include "stdlib.h"
#include "tchar.h"
#include "stdio.h"
#include "winuser.h"
#include "string.h"
#include "search.h"
#include "getuname.h"

#include "winnls.h"
#include "wingdi.h"

#define  NOT_EUDC(wc)  ((wc)<0xE000||(wc)>0xF8FF)

///////////////////////////////////////////////////////////////
//
// For a give codePage,
//   fills in array of defined Unicode chars
//
//  Parameter : CodePage : 0 for Unicode
//                       : any valid codepage
//
//              lpWC     : WCHAR pointer to hold the result
//                       : set to NULL to get array size
//
//  Returns : 0 for a invalid codepage
//            number of WChar for a valid codepage
//
////////////////////////////////////////////////////////////////
long
WCharCP(
   UINT CodePage,
   WCHAR *lpWC)
{
  long   lRet = 0;
  CPINFO cpinfo;
  BYTE   mb[2];
  WCHAR  wc;
  WORD   ctype;

  if(CodePage == UNICODE_CODEPAGE)                  // cp-1200
  {
    for(wc = ASCII_BEG; wc < 0xFFFF; wc++)
    {
      if(NOT_EUDC(wc))                              // always include EUDC
      {
        GetStringTypeW(CT_CTYPE1, &wc, 1, &ctype);
        if((!ctype))  continue;
      }

      if(lpWC != NULL) lpWC[lRet] = wc;
      lRet++;
    }
    return lRet;
  }

  if(!IsValidCodePage(CodePage))    return 0L;
  if(!GetCPInfo(CodePage, &cpinfo)) return 0L;

  // ASCii : 0x21 ~ 0x7F
  for(mb[0] = ASCII_BEG; mb[0] <= ASCII_END; mb[0]++)
  {
    if(MultiByteToWideChar(
            CodePage,
            MB_ERR_INVALID_CHARS,
            (const char *)mb, 1,
            &wc, 1) == 0)
        continue;

    GetStringTypeW(CT_CTYPE1, &wc, 1, &ctype);
    if(ctype & C1_CNTRL)
        continue;

    if(lpWC != NULL)
        lpWC[lRet] = wc;

    lRet++;
  }

// Single-Byte codepage only; Extended chars 0x80 ~ 0xFF

  for(mb[0] = HIANSI_BEG; mb[0] > 0; (mb[0])++)
  {
    if(MultiByteToWideChar(
                  CodePage,
                  MB_ERR_INVALID_CHARS,
                  (const char *)mb, 1,
                  &wc, 1) == 0)
          continue;

    GetStringTypeW(CT_CTYPE1, &wc, 1, &ctype);
    if((!ctype) ||
       ( ctype & C1_CNTRL))
       continue;

    if(lpWC != NULL)
        lpWC[lRet] = wc;

    lRet++;
  }

  if(cpinfo.MaxCharSize == 1)
      return lRet++;

  // DBCS only
  for(mb[0] = HIANSI_BEG; mb[0] < HIANSI_END; (mb[0])++)
  {
    if(!IsDBCSLeadByteEx(CodePage, mb[0]))
        continue;

    for(mb[1] = TRAILBYTE_BEG; mb[1] <= TRAILBYTE_END; (mb[1])++)
    {
      if(mb[1] == DELETE_CHAR)               // 0x7F is not a trail byte
          continue;

      if(MultiByteToWideChar(
                    CodePage,
                    MB_ERR_INVALID_CHARS,
                    (const char *)mb, 2,
                    &wc, 1) == 0)
            continue;

      if(NOT_EUDC(wc))                       // always include EUDC
      {
        GetStringTypeW(CT_CTYPE1, &wc, 1, &ctype);
        if((!ctype) ||
           ( ctype & C1_CNTRL))
           continue;
      }

      if(lpWC != NULL)
          lpWC[lRet] = wc;

      lRet++;
    }
  }

  return lRet;
}

///////////////////////////////////////////////////////////
//
//  find defined glyphs from an EUDC TTF font (*.tte)
//
///////////////////////////////////////////////////////////

#define W_REVERSE(w)    ((w>>8)+(w<<8))
#define DW_REVERSE(dw)  ((dw>>24)+(dw<<24)+((dw&0x00FF0000)>>8)+((dw&0x0000FF00)<<8))

typedef struct
{
    char    cTag[4];        // ttcf
    DWORD   dwVersion;
    DWORD   dwDirCount;
    DWORD   dwOffset1;

} TTCHead;

typedef struct
{
    DWORD   dwVersion;
    WORD    wNumTables;
    WORD    wSearchRange;
    WORD    wEntrySelector;
    WORD    wRangeShift;

} Header;
    
typedef struct
{
    char    cTag[4];
    DWORD   dwCheckSum;
    DWORD   dwOffset;
    DWORD   dwLength;

} TableDir;

typedef struct
{
    DWORD   dwVersion;
    WORD    wNumGlyphs;

} maxp;

typedef struct
{
    DWORD   dwVersion;          // 00000001 (00 01 00 00)
    BYTE    Filler[46];
    WORD    IndexToLocFormat;   // 0 for short offset, 1 for long

} head;

typedef struct
{
    WORD    wVersion;           // 0
    WORD    wNumTables;

} cmapHead;

typedef struct
{
    WORD    wPlatform;           // 3 for Microsoft
    WORD    wEncoding;           // 1 for Unicode, 0 for Symbol
    DWORD   dwOffset;

} cmap;

typedef struct
{
    WORD    wFormat;             // 4
    WORD    wLength;
    WORD    wVersion;
    WORD    wSegCountX2;
    WORD    wSeachRange;
    WORD    wEntrySelector;
    WORD    wRangeShift;

} Format4;

typedef struct
{
    WORD    wBeg;
    WORD    wEnd;
    WORD    wDelta;

} URDelta;

//////////////////////////////////////////////////////////////////////////
//
//  Simulate cmap ranges for a EUDC TTTF font file (*.tte)
//
//      Path   : file name with path
//      pSize  : number of Unicode ranges returned
//
//      return : pointer to an array of Unicode ranges
//
//////////////////////////////////////////////////////////////////////////

URANGE* EUDC_Range(TCHAR* Path, DWORD* pSize)
{
    HANDLE   hTTF, hTTFMap;
    DWORD    dwFileSize;
    LPVOID   lpvTTF;
    BYTE    *lp;
    WORD     i;

    WORD     LocFormat;
    BYTE    *pLoca;
    DWORD    dwAdjustTTC;

    WORD     NumTables = 0;
    WORD     NumGlyphs = 0;
    WORD     NumURange = 0;
    URDelta *pURDelta  = 0;  
    URANGE  *pUR = 0;

    *pSize = 0;

	hTTF = CreateFile(Path,
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_READONLY,
                      NULL);
	if(hTTF == INVALID_HANDLE_VALUE)
	{
		return pUR;
	}
	
	dwFileSize = GetFileSize(hTTF, NULL);
	hTTFMap = CreateFileMapping(hTTF,
                                NULL,
                                PAGE_READONLY,
                                0,
                                dwFileSize,
                                NULL);
	if(hTTFMap == NULL)
	{
		CloseHandle(hTTF);
		return pUR;
	}

	lpvTTF = MapViewOfFile(hTTFMap, FILE_MAP_READ, 0, 0, 0);
	if(lpvTTF == NULL)
	{
		CloseHandle(hTTFMap);
		CloseHandle(hTTF);
		return pUR;
	}

    lp = (BYTE*) lpvTTF;
    dwAdjustTTC = 0;
	if(lp[0] == 't' && lp[1] == 't' && lp[2] == 'c' && lp[3] == 'f')
    {
        TTCHead* ptr = (TTCHead*) lpvTTF;
        dwAdjustTTC = DW_REVERSE(ptr->dwOffset1);
    }

    lpvTTF = (BYTE*) lpvTTF + dwAdjustTTC;
    NumTables = W_REVERSE(((Header*)lpvTTF)->wNumTables);
    if(NumTables == 0) goto NextTTF;

    lp = (BYTE*) lpvTTF + sizeof(Header);
	for(i = 0; i < NumTables; i++)
	{
        TableDir* pTD = (TableDir*) lp;

		if( lp[0] == 'm' && lp[1] == 'a' &&
			lp[2] == 'x' && lp[3] == 'p')
        {
            maxp*   pmaxp = (maxp*) ((BYTE*) lpvTTF + DW_REVERSE(pTD->dwOffset));

            NumGlyphs = W_REVERSE(pmaxp->wNumGlyphs);
        }
		else if( lp[0] == 'h' && lp[1] == 'e' &&
			     lp[2] == 'a' && lp[3] == 'd')
        {
            head*   phead = (head*) ((BYTE*) lpvTTF + DW_REVERSE(pTD->dwOffset));

            LocFormat = W_REVERSE(phead->IndexToLocFormat);
        }
		else if( lp[0] == 'l' && lp[1] == 'o' &&
			     lp[2] == 'c' && lp[3] == 'a')
        {
            pLoca = (BYTE*) lpvTTF + DW_REVERSE(pTD->dwOffset);
        }
		else if( lp[0] == 'c' && lp[1] == 'm' &&
			     lp[2] == 'a' && lp[3] == 'p')
        {
            cmapHead *pCH;
            cmap     *pCmap;
            Format4  *pF4;
            BYTE     *pByte;
            WORD      wNum;
            int       i;

            pCH   = (cmapHead*) ((BYTE*)lpvTTF + DW_REVERSE(pTD->dwOffset));
            wNum  = W_REVERSE(pCH->wNumTables);
            pCmap = (cmap*) ((BYTE*)pCH + sizeof(cmapHead));
            for(i = 0; i < wNum; i++)
            {
                WORD Platform = W_REVERSE(pCmap->wPlatform);
                WORD Encoding = W_REVERSE(pCmap->wEncoding);
                if( Platform == 3 && (Encoding == 1 || Encoding == 0 ))
                {
                    WORD Format;

                    pF4 = (Format4*) ((BYTE*)pCH + DW_REVERSE(pCmap->dwOffset));
                    Format = W_REVERSE(pF4->wFormat);
                    if(Format == 4)
                    {
                        int   k;
                        WORD  w;

                        NumURange = (W_REVERSE(pF4->wSegCountX2) >> 1) - 1;
                        pURDelta   = (URDelta*) malloc(sizeof(URDelta)*NumURange);
                        if(!pURDelta) goto NextTTF;
                        pByte = (BYTE*) pF4 + sizeof(Format4);
                        for(k = 0; k < NumURange; k++)
                        {
                            w = *(WORD*)(pByte + sizeof(WORD)*k);
                            pURDelta[k].wEnd   = W_REVERSE(w);
                            w = *(WORD*)(pByte + sizeof(WORD)*(NumURange+1)   + 2 + sizeof(WORD)*k);
                            pURDelta[k].wBeg   = W_REVERSE(w);
                            w = *(WORD*)(pByte + sizeof(WORD)*(NumURange+1)*2 + 2 + sizeof(WORD)*k);
                            pURDelta[k].wDelta = W_REVERSE(w);
                        }
                    }

                    break;
                }
                pCmap += sizeof(cmap);
            }
        }

        lp += sizeof(TableDir);
	}

    if(NumGlyphs == 0) goto NextTTF;
    if(pLoca     == 0) goto NextTTF;
    if(NumURange == 0) goto NextTTF;
    if(pURDelta  == 0) goto NextTTF;

    {
        WORD  wLen = 4;
        WORD  wc;
        WORD  wGIdx;
        BOOL  HasGlyph;

        if(LocFormat == 0) wLen = 2;

        for(i = 0; i < NumURange; i++)
        {
            for(wc = pURDelta[i].wBeg; wc <= pURDelta[i].wEnd; wc++)
            {
                HasGlyph = FALSE;
                wGIdx = wc + pURDelta[i].wDelta;
                if(wGIdx > NumGlyphs)
                {
                    if(pUR) free(pUR);
                    pUR = 0;
                    goto NextTTF;
                }

                if(LocFormat == 0)
                {
                    if(*(WORD*)(pLoca + wLen * wGIdx) != *(WORD*)(pLoca + wLen * (wGIdx + 1)))
                        HasGlyph = TRUE;
                }
                else
                {
                    if(*(DWORD*)(pLoca + wLen * wGIdx) != *(DWORD*)(pLoca + wLen * (wGIdx + 1)))
                        HasGlyph = TRUE;
                }

                if(HasGlyph)
                {
                    if(pUR == 0)
                    {
                        (*pSize)++;
                        pUR = (URANGE *) malloc (sizeof(URANGE));
                        if(pUR == 0) goto NextTTF;
                        pUR[0].wcFrom = wc;
                        pUR[0].wcTo   = wc;
                    }
                    else
                    {
                        if(wc - pUR[(*pSize)-1].wcTo == 1)
                        {
                            (pUR[(*pSize)-1].wcTo)++;
                        }
                        else
                        {
                            (*pSize)++;
                            pUR = (URANGE *) realloc ((void*)pUR, sizeof(URANGE)*(*pSize));
                            if(pUR == 0) goto NextTTF;

                            pUR[(*pSize)-1].wcFrom = wc;
                            pUR[(*pSize)-1].wcTo   = wc;

                        }
                    }
                }           // if(HasGlyph)
            }               // for each Unicode range
        }                   // for all Unicode ranges.
    }

NextTTF:

//	UnMapViewOfFile(lpvTTF);
	CloseHandle(hTTFMap);
	CloseHandle(hTTF);
    if(pURDelta) free(pURDelta);
    return pUR;
}
