/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE  /* jtf win3 mod */
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"
#include "fileopen.h"
#include "task.h"
#include <direct.h> /* adding this for getcwd prototype - sdj */


/*---------------------------------------------------------------------------*/
/* File Document Data Routines                                         [mbb] */
/*---------------------------------------------------------------------------*/

VOID getFileDocData(FILEDOCTYPE fileDocType, BYTE *filePath, BYTE *fileName,
                    BYTE *fileExt, BYTE *title)
{
   if(filePath != NULL)
      strcpy(filePath, fileDocData[fileDocType].filePath);
   if(fileName != NULL)
      strcpy(fileName, fileDocData[fileDocType].fileName);
   if(fileExt != NULL)
      strcpy(fileExt, fileDocData[fileDocType].fileExt);
   if(title != NULL)
      strcpy(title, fileDocData[fileDocType].title);
}


VOID setFileDocData(FILEDOCTYPE fileDocType, BYTE *filePath, BYTE *fileName,
                    BYTE *fileExt, BYTE *title)
{
   if(filePath != NULL)
      strcpy(fileDocData[fileDocType].filePath, filePath);
   if(fileName != NULL)
      strcpy(fileDocData[fileDocType].fileName, fileName);
   if(fileExt != NULL)
      strcpy(fileDocData[fileDocType].fileExt, fileExt);
   if(title != NULL)
   {
      if(strlen(title) >= FO_MAXPATHLENGTH)
         title[FO_MAXPATHLENGTH-1] = 0;
      strcpy(fileDocData[fileDocType].title, title);
   }
}


VOID getDataPath(FILEDOCTYPE fileDocType, BYTE *filePath, BYTE *fileName)
{
   BYTE  savePath[FO_MAXPATHLENGTH];

   if(setPath(fileDocData[fileDocType].filePath, FALSE, savePath))
   {
      setFilePath(fileName);
#ifdef ORGCODE
      _getcwd(filePath);
#else
      _getcwd(filePath,PATHLEN);
#endif

      setPath(savePath, FALSE, NULL);
   }
}


/*---------------------------------------------------------------------------*/
/* dbFileType() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbFileType(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
//HWND  hDlg;
//UINT  message;
//WPARAM wParam;
//LONG  lParam;
{
   updateTimer();

   switch(message)
   {
   case WM_INITDIALOG:
      initDlgPos(hDlg);

      switch(saveFileType)
      {
      case FILE_NDX_SETTINGS:
         wParam = ITMSETTINGS;
         break;
      }
      return(TRUE);

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         switch(whichGrpButton(hDlg, ITMSETTINGS, ITMMEMO))
         {
         case ITMSETTINGS:
            wParam = FILE_NDX_SETTINGS;
            break;
         }
         break;

      case IDCANCEL:
         wParam = FALSE;
         break;

      default:
         return(TRUE);
      }
      break;

   default:
      return(FALSE);
   }

   EndDialog(hDlg, wParam);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* doFileNew() -                                                       [mbb] */
/*---------------------------------------------------------------------------*/


VOID doFileNew()
{
   INT      fileType;
   BYTE     szTitle[MINRESSTR];

   fileType = FILE_NDX_SETTINGS;

   if(childZoomStatus(0x0001, 0))
      childZoomStatus(0, 0x8000);

   LoadString(hInst, STR_TERMINAL, (LPSTR) szTitle, MINRESSTR);
   termFile(fileDocData[fileType].filePath, NULL_STR, fileDocData[fileType].fileExt, 
            (*fileDocData[fileType].title == 0) ? szTitle : fileDocData[fileType].title, 
            (*fileDocData[fileType].title == 0) ? TF_DEFTITLE : 0);
}


/*---------------------------------------------------------------------------*/
/* doFileOpen() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY FO_FileOpenType(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   INT   fileType;

   switch(message)
   {
   case WM_INITDIALOG:
      switch(pFOData->nType = saveFileType)
      {
      case FILE_NDX_SETTINGS:
         wParam = FO_IDSETTINGS;
         break;
      }
      break;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case FO_IDSETTINGS:
         fileType = FILE_NDX_SETTINGS;
         break;
      default:
         return(TRUE);
      }
      if(pFOData->nType == fileType)
         return(TRUE);
      pFOData->nType = fileType;
      break;

   case WM_NULL:
      if(wParam)
         saveFileType = pFOData->nType;
      return(FALSE);
   }

   return(FALSE);
}


VOID doFileOpen()
{
   BYTE     filePath[FO_MAXPATHLENGTH];
   BYTE     fileName[FO_MAXFILELENGTH];
   BYTE     fileExt[FO_MAXEXTLENGTH];
   BYTE     szTitle[MINRESSTR];

   *filePath = 0;
   *fileName = 0;
   *fileExt  = 0;

   strcpy(fileName, "*.TRM");
   strcpy(fileExt, "TRM");

   if(termData.filePath[strlen(termData.filePath) - 1] != '\\')
      strcat(termData.filePath, "\\");

   if(FileOpen(termData.filePath, fileName, NULL, fileExt, NULL, FO_DBFILETYPE, 
               FO_FileOpenType, FO_FILEEXIST | FO_FORCEEXTENT))
   {
      strcpy(fileDocData[saveFileType].filePath, filePath);
      strcpy(fileDocData[saveFileType].fileName, fileName);

      if(childZoomStatus(0x0001, 0))
         childZoomStatus(0, 0x8000);

      LoadString(hInst, STR_TERMINAL, (LPSTR) szTitle, MINRESSTR);
      termFile(termData.filePath, fileName, fileDocData[FILE_NDX_SETTINGS].fileExt,
               (*fileDocData[FILE_NDX_SETTINGS].title == 0) ? szTitle : fileDocData[FILE_NDX_SETTINGS].title, 
               (*fileDocData[FILE_NDX_SETTINGS].title == 0) ? TF_DEFTITLE : 0);
   }
}


/*---------------------------------------------------------------------------*/
/* doFileClose() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

VOID doFileClose()
{
      termCloseFile();
}


/*---------------------------------------------------------------------------*/
/* doFileSave() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

VOID doFileSave()
{
      termSaveFile(FALSE);
}


/*---------------------------------------------------------------------------*/
/* doFileSaveAs() -                                                    [mbb] */
/*---------------------------------------------------------------------------*/

VOID doFileSaveAs()
{
      termSaveFile(TRUE);
}
