/****************************Module*Header******************************\
* Module Name: filedlg.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#define NOWINSTYLES
#define NODRAWFRAME
#define NOKEYSTATES
#define NOATOM
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

/* These should remain in uppercase because ReplaceExtension depends on that
 */
static TCHAR szStarDotPCX[] = TEXT("*.PCX");
static TCHAR szStarDotBMP[] = TEXT("*.BMP");
static TCHAR szStarDotMSP[] = TEXT("*.MSP");
static TCHAR *szWildExt[] =
  {
    szStarDotPCX, szStarDotBMP, szStarDotBMP, szStarDotBMP, szStarDotBMP,
    szStarDotMSP
  } ;

extern WORD wFileType;
extern int fileMode;
extern TCHAR *namePtr, *pathPtr, *wildCard;
extern int DlgCaptionNo;
extern BITMAPFILEHEADER_VER1 BitmapHeader;
extern RECT pickRect;
extern int imagePlanes, imagePixels;

WORD PBGetFileType(LPTSTR lpFilename)
{
   LPTSTR lpStr;

   lpStr = lpFilename + lstrlen(lpFilename);
#ifdef DBCS
   while (lpStr > lpFilename && *lpStr != TEXT('.'))
      lpStr = CharPrev(lpFilename,lpStr);
#else
   while (lpStr > lpFilename && *lpStr != TEXT('.'))
      --lpStr;
#endif

   if (!*lpStr++)
      return (WORD)(-1);

   if (lstrcmpi(lpStr,TEXT("BMP")) == 0) return BITMAPFILE;
   if (lstrcmpi(lpStr,TEXT("DIB")) == 0) return BITMAPFILE;
   if (lstrcmpi(lpStr,TEXT("PCX")) == 0) return PCXFILE;
   if (lstrcmpi(lpStr,TEXT("MSP")) == 0) return MSPFILE;

   DB_OUT("Error in PBGetFileType\n");

   return (WORD)(-1);
}

#define CASE_LOWER -1
#define CASE_NONE 0
#define CASE_UPPER 1

int NEAR PASCAL FindCase(BYTE cTemp)
{
  int nCase = CASE_NONE;

  /* Attempt to match the case of the given string;
   * notice that nCase will not be changed on any punctuation
   * marks
   */
  if (LOBYTE((DWORD)CharUpper((LPTSTR) MAKEINTRESOURCE(cTemp))) != cTemp)
      nCase = CASE_LOWER;
  if (LOBYTE((DWORD)CharLower((LPTSTR) MAKEINTRESOURCE(cTemp))) != cTemp)
      nCase = CASE_UPPER;

  return(nCase);
}


void ReplaceExtension(LPTSTR lpFilename, int iFileType)
{
  LPTSTR lpLastBS, lpLastDot;
  int nCase = CASE_NONE;

  for (lpLastBS=lpLastDot=lpFilename; ; lpFilename=CharNext(lpFilename))
    {
      switch (*lpFilename)
        {
          case TEXT('\0'):
            goto FoundEnd;

          case TEXT('*'):
          case TEXT('?'):
            /* Don't replace the extension on a search spec
             */
            return;

          case TEXT('\\'):
          case TEXT('/'):
          case TEXT(':'):
            lpLastBS = lpFilename;
            break;

          case TEXT('.'):
            lpLastDot = lpFilename;
            break;

          default:
            if (nCase == CASE_NONE)
                nCase = FindCase((BYTE)*lpFilename);
            break;
        }
    }
FoundEnd:

  if (lpLastBS < lpLastDot)
    {
      /* Don't add an extension if we have "foo."
       */
      if (!lpLastDot[1])
          return;

      /* Match the case of the extension if there is one, otherwise
       * match the case of the first letter found (in the default case above)
       */
      nCase = FindCase((BYTE)lpLastDot[1]);
      lpFilename = lpLastDot;
    }

  lstrcpy(lpFilename, szWildExt[iFileType] + 1);
  if (nCase == CASE_LOWER)
      CharLower(lpFilename);
}
