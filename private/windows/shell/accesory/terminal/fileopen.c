/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define  NOGDICAPMASKS     TRUE
#define  NOICONS	         TRUE
#define  NOKEYSTATES       TRUE
#define  NOSYSCOMMANDS     TRUE
#define  NOATOM	         TRUE
#define  NOCLIPBOARD       TRUE
#define  NODRAWTEXT	      TRUE
#define  NOMINMAX	         TRUE
#define  NOOPENFILE	      TRUE
#define  NOSCROLL	         TRUE
#define  NOHELP            TRUE
#define  NOPROFILER	      TRUE
#define  NODEFERWINDOWPOS  TRUE
#define  NOPEN             TRUE
#define  NO_TASK_DEFINES   TRUE
#define  NOLSTRING         TRUE
#define  WIN31 	         TRUE

#include <windows.h>
#include <port1632.h>
#include <dcrc.h>                            /* mbbx 2.00 ... */
#include "commdlg.h"
#include <dynacomm.h>
#include <fileopen.h>
#include "dlgs.h"
#include <direct.h> /* adding this for chdir prototype -sdj*/

/*---------------------------------------------------------------------------*/
/* FileOpen() -                                                        [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR PASCAL PoundToNull(LPSTR str)
{

   while(*str)
   {
      if(*str == '#')
      	*str = 0x00;
      str++;
   }
}

VOID NEAR PASCAL LoadFilterString(HANDLE hInst,WORD ResID,LPSTR szFilter,
	                               INT len,DWORD *FilterIndex)
{

	switch(ResID)
   {
      case FO_DBSNDTEXT:
      case FO_DBRCVTEXT:
	      LoadString(hInst,STR_FILTERALL,
 	      &szFilter[LoadString(hInst,STR_FILTERTXT,szFilter,len)],len);	
         *FilterIndex = 1;
         break;

      case FO_DBSNDFILE:
      case FO_DBRCVFILE:
         LoadString(hInst,STR_FILTERALL,szFilter,len);
	      *FilterIndex = 1;
      break;

      default:
	      LoadString(hInst,STR_FILTERTRM,szFilter,len);
         *FilterIndex = 1;
    }

	 PoundToNull(szFilter);
}


VOID NEAR PASCAL AddDirMod(CHAR *path)
{
	INT len=0;

	while(path[len]){
		if(path[len] == '.')
			return;
		if(++len == (FO_MAXPATHLENGTH-1)){
			*path = 0;
			return;
		}
	}
	path[len] =  '\\';
	path[len+1] = 0;
}




BOOL FileOpen(BYTE *filePath, BYTE *fileName1,BYTE *fileName2, BYTE *fileExt,BYTE * titleText,WORD  wResID,FARPROC lpFilter, WORD wMode)
{
   BOOL           FileOpen = FALSE;
   // -sdj was unreferenced local var: LPDTA	   saveDTA;
   BYTE           savePath[FO_MAXPATHLENGTH];
   FILEOPENDATA   FOData;
   LPOFNHOOKPROC  lpProc;
   // -sdj was unreferenced local var: BYTE	   work[80];
   HWND           whichParent;
   CHAR           szFilter[75];		         /* default filter text/spec. for above  */
   CHAR           szFileName[STR255];	      /* Fully qualified name of file         */
   OPENFILENAME   OFN;
   INT            rc;

#ifndef BUGBYPASS
   if (TRUE)
#else
   if(setPath(filePath, FALSE, savePath))
#endif
   {
      setFilePath(fileName1);

      strcpy(FOData.file1, fileName1);
      strcpy(FOData.file2, (fileName2 != NULL) ? fileName2 : NULL_STR);
      strcpy(FOData.extent, fileExt);
      strcpy(FOData.title, (titleText != NULL) ? titleText : NULL_STR);
      FOData.wResID = ((wResID != 0) ? wResID : (!(wMode & FO_PUTFILE) ? FO_DBFILEOPEN : FO_DBFILESAVE));   /* mbbx 1.10: CUA... */
      FOData.lpFilter = lpFilter;
      FOData.wMode = wMode;
      pFOData = &FOData;

      lpProc = (LPOFNHOOKPROC)MakeProcInstance(dbFileOpen, hInst);
      whichParent = GetActiveWindow();
      if ( (whichParent == NULL) || (!IsChild(hItWnd,whichParent)) ) 
         whichParent = hItWnd; /* jtf 3.15 */

      /* OFN structure intialization for common open dialog. 02/19/91*/
      LoadFilterString(hInst,FOData.wResID,(LPSTR)&szFilter,sizeof(szFilter),&OFN.nFilterIndex);
      szFileName[0] = '\0';

      OFN.lStructSize	    = sizeof(OPENFILENAME);
      OFN.lpstrTitle 	    = NULL;	     /* Address  later to whichever dialog*/
      OFN.lpstrCustomFilter = NULL;
      OFN.nMaxCustFilter    = 0L;
      OFN.lpstrDefExt       = NULL;   /* ?? address later forcing trm. */ 
      OFN.lpstrInitialDir   = NULL;
      OFN.lpstrFile 	       = szFileName;
      OFN.nMaxFile	       = sizeof(szFileName);
      OFN.lpfnHook	       = lpProc;
      OFN.lCustData	       = 0L;
      OFN.lpTemplateName    = MAKEINTRESOURCE(FOData.wResID);
      OFN.hInstance	       = hInst;
      OFN.hwndOwner	       = whichParent;
      OFN.Flags 	          = OFN_HIDEREADONLY|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK;
      OFN.lpstrFileTitle    = NULL;	
      OFN.nMaxFileTitle	    = 0L;	
      OFN.lpstrFilter	    = szFilter;

      if((FOData.wResID == FO_DBFILESAVE) || 
         (FOData.wResID == FO_DBRCVFILE)  || 
         (FOData.wResID == FO_DBRCVTEXT))
      {
        FileOpen = GetSaveFileName(&OFN);
      }
      else
      {
        FileOpen = GetOpenFileName(&OFN);
      }

      if(rc = CommDlgExtendedError())
      {
         testBox(whichParent,-(MB_ICONHAND|MB_SYSTEMMODAL|MB_OK),STR_ERRCAPTION,NoMemStr);
         FileOpen = FALSE;
      }
	
      FreeProcInstance(lpProc);

#ifdef ORGCODE
         _getcwd(filePath);
#else
         _getcwd(filePath,PATHLEN);
#endif

      if(FileOpen)
      {


	 // sdj: the way this code is working right now, terminal
	 // sdj: calls the commdlg fileopen with a dbFileOpen hook
	 // sdj: function which saves FOData.file with just the name
	 // sdj: part of the file, and the called of FileOpen does
	 // sdj: the cat of getcwd with this filename. This is broken
	 // sdj: in case where terminal sets chdir to c:\ and commdlg
	 // sdj: while exiting sets the dir back to where it was so
	 // sdj: instead of c:\boot.ini user views d:\...\boot.ini!
	 // sdj: solution is to set filename1 to FileName part of szfilename
	 // sdj: which is what comdlg gives as a FQN and set filePath to
	 // sdj: the path part of this. it is very unlikely that commdlg
	 // sdj: will return szFileName with no '\\' in it but if strrchr
	 // sdj: fails, stick with the original pathrelated bug!

	 if (strrchr(szFileName,'\\'))
	    {
	    strcpy(fileName1,(strrchr(szFileName,'\\')+1));
	    *(strrchr(szFileName,'\\')) = 0;
	    strcpy(filePath,szFileName);
	    }

	 else{


         switch(FOData.wResID)
	 {
         case FO_DBRCVFILE:
	 case FO_DBRCVTEXT:
	    strcpy(fileName1, FOData.file);
            break;
         case FO_DBFILESAVE:
            strcpy(fileName1, szFileName);
            break;
         default:
	    strcpy(fileName1, FOData.file1);
            break;
	 }/* switch(resID) */
	}// end of if{!strrchr}else{sw:original stuff}

         if(fileName2 != NULL)
            strcpy(fileName2, FOData.file2);

         if(!OFN.nFileExtension)
         {
            if(!strchr(fileName1, '.'))
               strcat(fileName1, ".");
         }
         else if(FOData.wMode & FO_FORCEEXTENT)
         {
            if(strchr(fileExt, '.'))
               strcpy(fileExt, strchr(fileExt, '.') + 1);
            
            if(strchr(fileName1, '.'))
            {
               strcpy(strchr(fileName1, '.') + 1, fileExt);
            }
            else
            {
               strcat(fileName1, ".");
               strcat(fileName1, fileExt);
            }

         }
         else
            strcpy(fileExt, FOData.extent);

         if(wResID == FO_DBFILETYPE)
            FileOpen = FOData.nType;
      }

      setPath(savePath, FALSE, NULL);
   }

   SetFocus(hTermWnd);

   return(FileOpen);
}


/*---------------------------------------------------------------------------*/
/* dbFileOpen() -                                                      [mbb] */
/*---------------------------------------------------------------------------*/

BOOL  APIENTRY dbFileOpen(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   BOOL  result;
   BYTE  OEMname[STR255];            /* jtf 3.20 */
   BYTE  savePath[STR255]; /* rjs bugs 018 */

   updateTimer();                            /* code specific to DynaComm!!! */

   switch(message)
   {
   case WM_INITDIALOG:
      // use the OFN stuct to set this

      if(*(pFOData->title) != 0)
         SetWindowText(hDlg, pFOData->title); /* jtf terminal */

      SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, FO_MAXPATHLENGTH, 0L);
      SendDlgItemMessage(hDlg, edt1, EM_SETSEL, GET_EM_SETSEL_MPS(0, MAKELONG(0,32767)));

      if(pFOData->wResID == FO_DBFILETYPE)   /* mbbx 1.10: CUA... */
         (*(pFOData->lpFilter)) (hDlg, message, wParam, lParam);  /* mbbx 2.00: new FO hook scheme... */

      switch(pFOData->wResID)                /* mbbx 2.00: new FO hook scheme... */
      {
      case FO_DBFILETYPE:
         break;

      case FO_DBSNDFILE:
      case FO_DBRCVFILE:
         ShowWindow(GetDlgItem(hDlg, FO_IDPROMPT2), SW_HIDE);
         ShowWindow(GetDlgItem(hDlg, FO_IDFILENAME2), SW_HIDE);
         SendDlgItemMessage(hDlg, FO_IDFILENAME2, EM_LIMITTEXT, FO_MAXPATHLENGTH, 0L);
         break;

      default:
         if(pFOData->lpFilter != NULL)
            (*(pFOData->lpFilter)) (hDlg, message, wParam, lParam);
         break;
      }

      if((pFOData->wMode & FO_PUTFILE) && !FO_SaveFileName(hDlg))    /* mbbx 2.00 ... */
      {
         SetDlgItemText(hDlg, edt1, (LPSTR) pFOData->file1);
         strcpy(pFOData->title, pFOData->file1);
      }
      if(pFOData->lpFilter != NULL)       /* jtf 3.22 */
          (*(pFOData->lpFilter)) (hDlg, message, wParam, lParam);
      return(TRUE);     /* Bug 7115, let dlgmgr set focus.  clarkc */

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
      {
         HWND focus = GetFocus();	

         if(focus == GetDlgItem(hDlg,edt1) || 
            focus == GetDlgItem(hDlg,lst1) ||
		      focus == GetDlgItem(hDlg,IDOK)) 
         {
            if((pFOData->wMode & FO_GETFILENAME2) && (pFOData->wMode & FO_NONDOSFILE))
            {
               if(!GetDlgItemText(hDlg, edt1, (LPSTR) pFOData->file2, FO_MAXPATHLENGTH))
                  pFOData->wMode &= ~FO_GETFILENAME2;
               if(!GetDlgItemText(hDlg, FO_IDFILENAME2, (LPSTR) pFOData->file, FO_MAXPATHLENGTH))
                  strcpy(pFOData->file, (pFOData->wMode & FO_GETFILENAME2) ? pFOData->file2 : pFOData->file1);
            }
            else
            { 
               if(!GetDlgItemText(hDlg, edt1, (LPSTR) pFOData->file, FO_MAXPATHLENGTH))
                  strcpy(pFOData->file, pFOData->file1);
               if(pFOData->wMode & FO_GETFILENAME2)
                  if(!GetDlgItemText(hDlg, FO_IDFILENAME2, (LPSTR) pFOData->file2, FO_MAXPATHLENGTH))
                     pFOData->wMode &= ~FO_GETFILENAME2;
            }

            result = TRUE;
            break;	      
         }
         else
            return(FALSE);
      }

      case IDCANCEL:
         *(pFOData->file2) = 0;
         pFOData->wMode &= ~FO_GETFILENAME2;
         result = FALSE;
         break;

      case edt1:
         if(GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
         {
            if(!((pFOData->wMode & FO_BATCHMODE) && (pFOData->wMode & FO_PUTFILE)))    /* mbbx 1.01: ymodem */
               EnableWindow(GetDlgItem(hDlg, IDOK), SendDlgItemMessage(hDlg, edt1, WM_GETTEXTLENGTH, 0, 0L));

            if(pFOData->wMode & FO_GETFILENAME2)
            {
               *(pFOData->file2) = 0;
               pFOData->wMode &= ~FO_GETFILENAME2;
               ShowWindow(GetDlgItem(hDlg, FO_IDPROMPT2), SW_HIDE);
               ShowWindow(GetDlgItem(hDlg, FO_IDFILENAME2), SW_HIDE);
            } 

            if(pFOData->lpFilter != NULL)    /* mbbx 2.00: new FO hook scheme... */
               (*(pFOData->lpFilter)) (hDlg, message, wParam, lParam);
         }
         return(FALSE);

      case lst1:
         if(GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
         {
            FO_SetListItem(hDlg, lst1, FALSE);
            if(pFOData->lpFilter != NULL)    /* mbbx 2.00: new FO hook scheme... */
               (*(pFOData->lpFilter)) (hDlg, message, wParam, lParam);
            return(FALSE);
         }
         result = TRUE;
         if(GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
                  break;
         return(FALSE);
         break;

      default:
         switch(pFOData->wResID)             /* mbbx 2.00: new FO hook scheme... */
         {
         case FO_DBFILETYPE:                 /* mbbx 1.10: CUA... */
			/* BUG BUG, what does this call, wParam/lParam may need fixing!*/
            if(!(*(pFOData->lpFilter)) (hDlg, message, wParam, lParam))
            {
               SetDlgItemText(hDlg, FO_IDFILENAME, (LPSTR) pFOData->file1);
            }
            break;

         default:
            if(pFOData->lpFilter != NULL)
               (*(pFOData->lpFilter)) (hDlg, message, wParam, lParam);
            break;
         }

         return(FALSE);
      }
      break;

   default:
      return(FALSE);
   }

   if(result)
   {
      if(!setPath(pFOData->file, TRUE, savePath))
      {
         FO_ErrProc(STRDRIVEDIR, MB_OK | MB_ICONHAND, hDlg);
         return(FO_SetCtrlFocus(hDlg, GetFocus()));
      }

      if(!((pFOData->wMode & FO_BATCHMODE) && (GetKeyState(VK_CONTROL) & 0x8000)))  /* mbbx 1.01: ymodem */
      {
         AnsiUpper((LPSTR) pFOData->file);
         strcpy(pFOData->file1, pFOData->file);

         if(FO_AddFileType(pFOData->file, pFOData->extent))
      	 	return(FALSE);

         if(!FO_IsLegalDOSFN(pFOData->file))    /* mbbx 2.00: no forced extents... */
         {
            if(((pFOData->wMode & FO_NONDOSFILE) && (pFOData->wMode & FO_GETFILENAME2)) || 
               (!(pFOData->wMode & FO_NONDOSFILE) && !(pFOData->wMode & FO_GETFILENAME2)) || 
               !FO_IsLegalFN(pFOData->file))
            {
               FO_ErrProc(FO_STR_BADFILENAME, MB_OK | MB_ICONHAND,hDlg);
               return(FO_SetCtrlFocus(hDlg, GetFocus()));
            }
            else if(!(pFOData->wMode & FO_GETFILENAME2))
            {
               pFOData->wMode |= FO_GETFILENAME2;
               ShowWindow(GetDlgItem(hDlg, FO_IDPROMPT2), SW_SHOW);
               ShowWindow(GetDlgItem(hDlg, FO_IDFILENAME2), SW_SHOW);
               return(FO_SetCtrlFocus(hDlg, GetFocus()));
            }
         }

         if(!getFileType(pFOData->file, pFOData->extent))   /* mbbx 2.00 ... */
         {
            forceExtension(pFOData->file, pFOData->extent+2, FALSE);    /* mbbx 2.00: no forced extents... */
         }


         // JYF -- replace these two lines with the below if() to remove the use of AnsiToOem()
         //
         //AnsiToOem((LPSTR) pFOData->file, (LPSTR) OEMname); /* jtf 3.20 */
         //if((pFOData->wMode & FO_FILEEXIST) && !fileExist(OEMname)) /* jtf 3.20 */

         if ((pFOData->wMode & FO_FILEEXIST) && !fileExist(pFOData->file))
         {
/* rjs bugs 018 */
            if(!(pFOData->wMode & FO_PUTFILE))  /* mbbx 1.10: CUA... */
               DlgDirList(hDlg, (LPSTR) pFOData->file, lst1, 0, FO_LBFILE);
            else
               strcpy(pFOData->file, pFOData->title);
            SetDlgItemText(hDlg, ((pFOData->wMode & FO_GETFILENAME2) && (pFOData->wMode & FO_NONDOSFILE)) ? 
                           FO_IDFILENAME2 : FO_IDFILENAME, (LPSTR) pFOData->file);
            setFilePath(savePath);
/* end of rjs bugs 018 */
            FO_ErrProc(FO_STR_FILENOTFOUND, MB_OK | MB_ICONHAND,hDlg);
            return(FO_SetCtrlFocus(hDlg, GetFocus()));
         }

         if((pFOData->wMode & FO_REMOTEFILE) && !(pFOData->wMode & FO_GETFILENAME2))
         {
            pFOData->wMode |= FO_GETFILENAME2;
            SetDlgItemText(hDlg, FO_IDFILENAME2, (LPSTR) pFOData->file);
            ShowWindow(GetDlgItem(hDlg, FO_IDPROMPT2), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, FO_IDFILENAME2), SW_SHOW);
            return(FO_SetCtrlFocus(hDlg, GetDlgItem(hDlg, FO_IDFILENAME2)));
         } 


         // JYF -- replace these two lines with following if() to remove the use of AnsiToOem()
         //
         //AnsiToOem((LPSTR) pFOData->file, (LPSTR) OEMname); /* jtf 3.20 */
         //if(((pFOData->wMode & (FO_PUTFILE | FO_FILEEXIST)) == FO_PUTFILE) && fileExist(OEMname))  /* jtf 3.20 */

         if (((pFOData->wMode & (FO_PUTFILE | FO_FILEEXIST)) == FO_PUTFILE) && fileExist(pFOData->file))
         {
            if(FO_ErrProc(FO_STR_REPLACEFILE, MB_YESNO | MB_ICONEXCLAMATION,hDlg) == IDNO)
               return(FO_SetCtrlFocus(hDlg, GetFocus()));
         }
      }

      AnsiUpper((LPSTR) pFOData->file);      /* mbbx 2.00: new FO hook scheme... */
      strcpy(pFOData->file1, pFOData->file);
      AnsiUpper((LPSTR) pFOData->file2);
   }

   if(pFOData->lpFilter != NULL)             /* mbbx 2.00: new FO hook scheme... */
      (*(pFOData->lpFilter)) (hDlg, WM_NULL, result, 0L);

   return(FALSE);
}


/*---------------------------------------------------------------------------*/
/* FO_SaveFileName() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR FO_SaveFileName(HWND  hDlg)
{
   register BYTE  *pch;

   GetDlgItemText(hDlg, ((pFOData->wMode & FO_GETFILENAME2) && (pFOData->wMode & FO_NONDOSFILE)) ? 
                  FO_IDFILENAME2 : edt1, (LPSTR) pFOData->file, 32);

   for(pch = pFOData->file; *pch != 0; pch += 1)
      if((*pch == '*') || (*pch == '?') || (*pch == '\\') || (*pch == ':'))
         return(FALSE);

   strcpy(pFOData->title, pFOData->file);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* FO_SetListItem() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR FO_SetListItem(HWND hDlg,WORD  wCtrlID,BOOL bSetSel)
{
   if(bSetSel)
      SendDlgItemMessage(hDlg, wCtrlID, LB_SETCURSEL, 0, 0L);


   if(SendDlgItemMessage(hDlg, wCtrlID, LB_GETCURSEL, 0, 0L) != LB_ERR)
   {
      MDlgDirSelect(hDlg, (LPSTR) pFOData->file, FO_MAXFILELENGTH, wCtrlID);

      if(wCtrlID == lst2)
      {
         FO_NewFilePath(hDlg, ((pFOData->wMode & FO_GETFILENAME2) && (pFOData->wMode & FO_NONDOSFILE)) ? 
                        FO_IDFILENAME2 : edt1, pFOData->file, pFOData->extent);
      }

      SetDlgItemText(hDlg, ((pFOData->wMode & FO_GETFILENAME2) && (pFOData->wMode & FO_NONDOSFILE)) ? 
                     FO_IDFILENAME2 : edt1, (LPSTR) pFOData->file);
   }
}


/*---------------------------------------------------------------------------*/
/* FO_NewFilePath() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR FO_NewFilePath(HWND hDlg, WORD wCtrlID,BYTE *fileName,BYTE *fileExt)
{
   register BYTE  *pch;
   BOOL           bWild;
   BYTE           tempName[FO_MAXPATHLENGTH];

   GetDlgItemText(hDlg, wCtrlID, (LPSTR) tempName, FO_MAXPATHLENGTH);

   pch = tempName+(strlen(tempName)-1);
   bWild = ((*pch == '*') || (*pch == ':'));

   while(pch > tempName)
   {
      pch--;
      if((*pch == '*') || (*pch == '?'))
         bWild = TRUE;
      if((*pch == '\\') || (*pch == ':'))
      {
         pch++;
         break;
      }
   }

   if(bWild)
      strcpy(fileName+strlen(fileName), pch);
   else
      strcpy(fileName+strlen(fileName), fileExt+1);
}


/*---------------------------------------------------------------------------*/
/* FO_AddFileType() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR FO_AddFileType(BYTE  *fileName, BYTE  *fileExt)
{
   register BYTE  *pch;
   INT            j;
   BOOL           bWild;

   if((pch = fileName+strlen(fileName)) == fileName)
      j = 1;
   else if((pch == fileName+2) && (fileName[0] == '.') && (fileName[1] == '.'))
      j = 0;
   else
   {
      bWild = FALSE;                         /* mbbx 2.00: no forced extents... */
      while(--pch >= fileName)
      {
         if((*pch == '*') || (*pch == '?'))
         {
            bWild = TRUE;
            break;
         }
      }

      if(getFileType(fileName, fileExt))
         return(bWild);

      pch = fileName+strlen(fileName);
      j = ((*(pch-1) == '\\') ? 1 : (bWild ? 2 : 0));
   }

   //sdj: if the extention is "\*.*" which it would be if someone is doing
   //sdj: recvfile open with "test" as name, why add \*.* to test?
   //sdj: it  is ok to do this for open/save/saveas where you can force .trm
   //sdj: not not in other cases of opening the file

   if (*fileExt == '\\' && *(fileExt+1) == '*' && *(fileExt+2) == '.')
      {
      return FALSE;
      }

   strcpy(pch, fileExt+j);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* FO_StripFileType() -                                                [mbb] */
/*---------------------------------------------------------------------------*/

VOID NEAR FO_StripFileType(BYTE  *fileName)
{
   register BYTE  *pch;

   for(pch = fileName+(strlen(fileName)-1); pch > fileName; pch--)
   {
      if(*pch == '\\')
      {
         *pch = 0;
         break;
      }
   }
}


/*---------------------------------------------------------------------------*/
/* FO_IsLegalDOSFN() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

#define FO_MAXDOSFILELEN	    FO_MAXFILELENGTH //-sdj bug#4587 fix
// -sdj this was 8 to check for dosfilename length. with ntfs filenames
// -sdj being >8 this test used to fail. made maxdosfilelen to fo_maxfilelength
// -sdj which is 16 at present. This will let users use prefix name upto
// -sdj 16 chars, but still force them to have .trm extension, AND by
// -sdj restricting the name to FO_MAXFILELENGTH, ensure that no other
// -sdj global buffer overflows (most of the buffers use this define)
// -sdj eventually, right solution is to set fo_maxfilelength to ntfs max
// -sdj name, but this is an intermediate solution which is safe, and still
// -sdj allow users upto 16 chars of trmfilenames.
// -sdj along with this change the other change needed is to increase
// -sdj the termData element sizes to > 12 which was a pain! this define
// -sdj in dynacomm.h is increased to 32 for the time being.


#define FO_MAXDOSEXTLEN             3

BOOL NEAR FO_IsLegalDOSFN(BYTE  *fileName)
{
   register INT   i;
   INT            j;

   for(i = 0; fileName[i] == ' '; i++);
   if(i > 0)
      strcpy(fileName, fileName+i);

   for(i = 0; fileName[i] != 0; i++)
   {
      if(fileName[i] == '.')
      {
         for(j = 1; fileName[i+j] != 0; j++)
         {
            if((j > FO_MAXDOSEXTLEN) || !FO_IsLegalDOSCH(fileName[i+j]) || (fileName[i+j]=='.') ) /* jtf 3.31 */
               return(FALSE);
         }

         break;
      }

      if((i >= FO_MAXDOSFILELEN) || !FO_IsLegalDOSCH(fileName[i]))
	 return(FALSE);
   }

   return(i != 0);
}


/*---------------------------------------------------------------------------*/
/* FO_IsLegalDOSCH() -                                                       */
/*---------------------------------------------------------------------------*/

#define FO_NONDOSFNCHARS            "\\/[]:|<>+=;,\""

BOOL NEAR FO_IsLegalDOSCH(BYTE ch)
{
   register BYTE  *pch;

   if(ch <= ' ')
      return(FALSE);

   for(pch = FO_NONDOSFNCHARS; *pch != '\0'; pch++)
   {
      if(*pch == ch)
         return(FALSE);
   }

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* FO_IsLegalFN() -                                                    [scf] */
/*---------------------------------------------------------------------------*/

BOOL NEAR FO_IsLegalFN(BYTE  *fileName)
{
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* FO_ErrProc() -                                                            */
/*---------------------------------------------------------------------------*/

INT NEAR FO_ErrProc(WORD errMess, WORD errType, HWND hDlg)
{
   BYTE temp1[STR255];
   BYTE temp2[STR255];
   GetWindowText(hItWnd, (LPSTR) temp1, 254);    /* mbbx 2.00: new FO hook scheme... */
   sscanf(temp1, "%s", temp2);
   LoadString(hInst, (errType & MB_ICONHAND) ? FO_STR_ERRCAPTION : FO_STR_WARNCAPTION, (LPSTR) temp1, 80);
   strcpy(temp2+strlen(temp2), temp1);
   LoadString(hInst, errMess, (LPSTR) temp1, 254);

   MessageBeep(0);
   return(MessageBox(hDlg, (LPSTR) temp1, (LPSTR) temp2, errType)); /* jtf 3.14 */
}


/*---------------------------------------------------------------------------*/
/* FO_SetCtrlFocus() -                                                 [mbb] */
/*---------------------------------------------------------------------------*/

BOOL NEAR FO_SetCtrlFocus(HWND  hDlg, HWND  hCtrl)
{
   if(hCtrl == NULL)                         /* mbbx 2.00 ... */
      hCtrl = GetDlgItem(hDlg, (pFOData->wMode & FO_GETFILENAME2) ? FO_IDFILENAME2 : edt1);

#ifdef ORGCODE
   switch(GetWindowWord(hCtrl, GWW_ID))
#else
   switch(GetWindowLong(hCtrl, GWL_ID))
#endif
   {
   case edt1:
   case FO_IDFILENAME2:
      SendMessage(hCtrl, EM_SETSEL, GET_EM_SETSEL_MPS(0, 0x7FFF));
      break;

   case lst2:
      break;

   case lst1:
      if(SendMessage(hCtrl, LB_GETCURSEL, 0, 0L) != LB_ERR)
         break;

   case IDOK:
      if(pFOData->wMode & FO_PUTFILE)
      {
         hCtrl = GetDlgItem(hDlg, (pFOData->wMode & FO_GETFILENAME2) ? FO_IDFILENAME2 : edt1);
         SendMessage(hCtrl, EM_SETSEL, GET_EM_SETSEL_MPS(0, 0x7FFF));
      }
      else
      {
         hCtrl = GetDlgItem(hDlg, lst1);
         if(SendMessage(hCtrl, LB_GETCOUNT, 0, 0L) > 0)
            FO_SetListItem(hDlg, lst1, TRUE);
         else
            hCtrl = GetDlgItem(hDlg, lst2);   /* mbbx 1.10: CUA... */
      }
      break;
   }

   SetFocus(hCtrl);
   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* setPath() -                                                         [mbb] */
/*---------------------------------------------------------------------------*/

BOOL setPath(BYTE  *newPath, BOOL  bFileName, BYTE  *oldPath)
{
   BYTE  work[FO_MAXPATHLENGTH];

DEBOUT("Enter setPath: with newpath=%lx\n",newPath);
DEBOUT("      setPath: with bFileName=%lx\n",bFileName);
DEBOUT("      setPath: with oldPath=%lx\n",oldPath);

   if(oldPath)
#ifdef ORGCODE
{      _getcwd(oldPath); }
#else
      {
      _getcwd(oldPath,PATHLEN);
DEBOUT("setPath: getcwd returned %s\n",oldPath);
      }
#endif

   if(bFileName)
   {
      if(!setFilePath(newPath)){
DEBOUT("setPath: setFilePath(newPath) returned 0, calling sFP(%lx) and ret FALSE\n",oldPath);
      	 setFilePath(oldPath);
         return(FALSE);
      }
   }
   else
   {
      strcpy(work, newPath);
      DEBOUT("setPath: bFil=0,call setfp and chdir with work as %s\n",work);
      if(!setFilePath(work) || ((work[0] != 0) && (_chdir(work) == -1))){
DEBOUT("setPath: setFP(work) ret0||somethingelse, calling sFP(%lx) and ret FALSE\n",oldPath);
	 setFilePath(oldPath);
         return(FALSE);
      }
   }

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* setFilePath() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

BOOL setFilePath(BYTE  *fileName)
{
   INT   ndx;
   BYTE  temp[FO_MAXPATHLENGTH];
   //sdj: just added to debug if chdir is working BYTE	tmpgetcwd[PATHLEN];

DEBOUT("Enter setFilePath with filename=%lx\n",fileName);

#ifdef ORGCODE
#else
if (!fileName)
{
   DEBOUT("setFilePath: HACK %s\n","called with 0 as filename");
   DEBOUT("setFilePath: HACK %s\n","setting chdir to c:\\nt");
   _chdir("C:\\NT");
   return TRUE;
}
#endif

   ndx = strlen(fileName);

#ifdef ORGCODE
   if((ndx >= 2) && (fileName[1] == ':'))    /* mbbx 2.00 ... */
   {
      if(fileName[0] > 'Z')
         fileName[0] -= ' ';
      if(!setdrive(fileName[0]))
         return(FALSE);

      strcpy(fileName, fileName+2);
      ndx -= 2;                              /* mbbx 2.00 */
   }

#else
/* in NT there is no concept of currentdrive AND currentWorkDir -sdj
   so dont do any setdrive, and keep the fileName as a fully    -sdj
   qualified path name including the drive letter               -sdj*/
#endif

   while(--ndx >= 0)                         /* mbbx 2.00 ... */
   {
      if(fileName[ndx] == '\\')
         break;
   }

   if(ndx >= 0)
   {
// sdj: 11may93: FileOpen
      strcpy(temp, fileName+ndx+1);

      if(ndx == 0)
	 ndx += 1;

      fileName[ndx] = '\0';

// -sdj fix for readcmdline not opening .trm file 03jun92
//
// if filename was c:\scratch\foo.trm, do chdir(c:\) not chdir(c:)
// if chdir(c:) is done,later getcwd will not give c:\ but something else

   if (fileName[ndx-1] == ':')
      {
      fileName[ndx] = '\\';
      fileName[ndx+1] = '\0';
      }

// end of fix

      if(_chdir(fileName) == -1)
         return(FALSE);

      //getcwd(tmpgetcwd,PATHLEN);

      strcpy(fileName, temp);
   }

   return(TRUE);
}


/*---------------------------------------------------------------------------*/
/* forceExtension() -                                                  [mbb] */
/*---------------------------------------------------------------------------*/

VOID forceExtension(BYTE  *fileName, BYTE  *fileExt, BOOL  bReplace)
{
   register BYTE  *pch;
   WORD           len;

   for(pch = fileName+((len = strlen(fileName))-1); pch > fileName; pch -= 1)
   {
      if((*pch == '.') || (*pch == '\\'))
         break;
   }

   if((pch <= fileName) || (*pch != '.'))
      pch = fileName + len;
   else if(!bReplace)
      return;

   strcpy(pch, fileExt);

   while(*(++pch) != 0)
      if((*pch == '*') || (*pch == '?'))
      {
         *pch = 0;
         break;
      }
}


/*---------------------------------------------------------------------------*/
/* getFileType() -                                                     [mbb] */
/*---------------------------------------------------------------------------*/

BOOL getFileType(BYTE  *fileName, BYTE  *fileExt)
{
   register BYTE  *pch;

   for(pch = fileName+(strlen(fileName)-1); pch > fileName; pch -= 1)
   {
      if(*pch == '.')
      {
         AnsiUpper((LPSTR) pch);
         forceExtension(fileExt, pch, TRUE);    /* mbbx 2.00: no forced extents */
         return(TRUE);
      }
      else if(*pch == '\\')
         break;
   }

   return(FALSE);
}

#ifdef ORGCODE
#else

BOOL   fileExist(LPSTR lpfilename)
{
   HANDLE hFile;
   CHAR   chCurDir[FO_MAXPATHLENGTH+1];
   DWORD  dwRc;

   dwRc = GetCurrentDirectory(FO_MAXPATHLENGTH,chCurDir);

   DEBOUT("Rc of getcdir = %lx\n",dwRc);
   DEBOUT("Current Dir in which file is being opened=%s\n",chCurDir);
   DEBOUT("Existence being checked for the file=%s\n",lpfilename);

   hFile = CreateFile(lpfilename,
                      GENERIC_READ,            /* no access desired please */
                      FILE_SHARE_READ|FILE_SHARE_WRITE,     /* dont be greedy */
                      NULL,                   /* no security */
                      OPEN_EXISTING,          /* open only if existing */
                      FILE_ATTRIBUTE_NORMAL,  /* who cares what attr it is */
                      NULL);                  /* why do we need template here */

   if( (hFile == (HANDLE)-1) || (hFile == NULL) )
   {
      /* cant open this one, so return false, looks like doesnt exist */
      DEBOUT("fileExist FALSE: %s does not exit\n",lpfilename);
      return FALSE;
   }

   DEBOUT("fileExist TRUE: %s does exit\n",lpfilename);

   CloseHandle(hFile);
   return TRUE;

}

#endif
