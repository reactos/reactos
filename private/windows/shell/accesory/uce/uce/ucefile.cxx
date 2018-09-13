/********************************************************************
 *
 *  Module Name : ucefile.c
 *
 *  UCE-File parser
 *
 *  History :
 *       Sep 02, 1997  [samera]    wrote it.
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation. 
 **********************************************************************/

// BUGBUG: This stuff should really go into a precomp header
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uce.h"
#include "ucefile.h"


// Current opened UCE files
UCE_MEMORY_FILE  gUceFiles[MAX_UCE_FILES];
PUCE_MEMORY_FILE gpUceFile=&gUceFiles[0];
INT              gnUceFiles=0;
INT              gnMaxUceFiles=MAX_UCE_FILES;


/******************************Public*Routine******************************\
* UCE_LoadFile
*
* Load a UCE file (by mapping it in the process address space & commit it)
* it returns both ptr+handle of file
* 
* Return Value:
* TRUE if successful, FALSE otherwise
* History:
*   Sept-03-1997  Samer Arafeh  [samera]      
*    wrote it
\**************************************************************************/
BOOL UCE_LoadFile( PWSTR pwszFileName , PUCE_MEMORY_FILE pMemFile )
{
  HANDLE hFile;
  HANDLE hMapFile;
  PVOID  pvFile;
  DWORD  dwFileSize;
  WORD   CodePage;
  DWORD  dwOffset;
  WCHAR *pWC;

  // Open and map file
  hFile = CreateFile( pwszFileName,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_READONLY,
                      NULL
                    );

  if( INVALID_HANDLE_VALUE == hFile )
    goto File_Err;

  dwFileSize = GetFileSize( hFile , NULL );

  hMapFile = CreateFileMapping( hFile ,
                                NULL,
                                PAGE_READONLY,
                                0,
                                dwFileSize,
                                NULL
                              );

  if( (NULL == hMapFile) || (dwFileSize<sizeof(UCE_HEADER)) )
    goto File_Err1;

  pvFile = MapViewOfFile( hMapFile ,
                          FILE_MAP_READ,
                          0,
                          0,
                          0
                        ) ;

  if( NULL == pvFile )
    goto File_Err2;

  // verify header
  if( *((DWORD *)pvFile) != 0x58454355L ) // XECU
    goto File_Err3;

//
// The Ideograph by Radical UCE file should be included
// only if one of the 4 DBCS font and NLS is installed.
//
  dwOffset = *(((DWORD*)pvFile)+1);
  pWC = (WCHAR*)(((BYTE*)pvFile)+dwOffset);
  if(lstrcmp(pWC, L"010200") == 0)             // Ideograf.UCE
  {
     if(Font_DBCS_CharSet() == 0)
         goto File_Err3;
     if((IsValidLanguageGroup(LGRPID_JAPANESE, LGRPID_INSTALLED) == FALSE) &&
        (IsValidLanguageGroup(LGRPID_SIMPLIFIED_CHINESE, LGRPID_INSTALLED) == FALSE) &&
        (IsValidLanguageGroup(LGRPID_KOREAN, LGRPID_INSTALLED) == FALSE) &&
        (IsValidLanguageGroup(LGRPID_TRADITIONAL_CHINESE, LGRPID_INSTALLED) == FALSE))
        goto File_Err3;
  }
  
//
// A UCE file with a Non-Unicode DBCS codepage
// is included only if font & NLS are installed.
//
  CodePage = *(((WORD*)pvFile)+4);
  if(CodePage != UNICODE_CODEPAGE)
  {
      CPINFO      cpi;
      CHARSETINFO csi;

      if(!IsValidCodePage(CodePage) ||         // invalid codepage
         !GetCPInfo(CodePage, &cpi) )
      {
         goto File_Err3;
      }
                                
      if(cpi.MaxCharSize >1  &&                // DBCS  
         TranslateCharsetInfo((DWORD*)CodePage, &csi, TCI_SRCCODEPAGE))
      {
          if(!Font_Avail(csi.ciCharset))       // no font with matched charset
          {
              goto File_Err3;
          }
      }
      else
      {
          //
          // SBCS or no GDI charset, include it anyway.
          //
      }

      if(CodePage == 932)
      {
         if(IsValidLanguageGroup(LGRPID_JAPANESE, LGRPID_INSTALLED) == FALSE)
             goto File_Err3;
      }
      else if(CodePage == 936)
      {
         if(IsValidLanguageGroup(LGRPID_SIMPLIFIED_CHINESE, LGRPID_INSTALLED) == FALSE)
             goto File_Err3;
      }
      else if(CodePage == 949)
      {
         if(IsValidLanguageGroup(LGRPID_KOREAN, LGRPID_INSTALLED) == FALSE)
             goto File_Err3;
      }
      else if(CodePage == 950)
      {
         if(IsValidLanguageGroup(LGRPID_TRADITIONAL_CHINESE, LGRPID_INSTALLED) == FALSE)
             goto File_Err3;
      }
  }

  // save handle 
  pMemFile->hFile    = hFile;
  pMemFile->hMapFile = hMapFile;
  pMemFile->pvData   = pvFile;

  return TRUE;


  // Error Handler
File_Err3:
  UnmapViewOfFile(pvFile);
File_Err2:
  CloseHandle(hMapFile);
File_Err1:
  CloseHandle(hFile);
File_Err:
  
  return FALSE;
}


/******************************Public*Routine******************************\
* Uce_AddFile
*
* Add a UCE_MEMORY_File to our global list
* 
* Return Value:
* TRUE if successful, FALSE otherwise
* History:
*   Sept-03-1997  Samer Arafeh  [samera]      
*    wrote it
\**************************************************************************/
BOOL Uce_AddFile( PUCE_MEMORY_FILE pUceMemFile )
{
  PVOID pv;
  BOOL  bRet=TRUE;

  // Check if we still have enough space on our global list
  if( gnMaxUceFiles == gnUceFiles )
  {
    // Need more memory space
    pv = LocalAlloc( LMEM_FIXED , (gnMaxUceFiles+MAX_UCE_FILES)*sizeof(UCE_MEMORY_FILE));
    if( pv )
    {
      memcpy( pv , gpUceFile , sizeof(UCE_MEMORY_FILE)*gnMaxUceFiles ) ;
      if( gpUceFile != &gUceFiles[0] )
      {
        LocalFree( gpUceFile ) ;
      }
      gpUceFile = (PUCE_MEMORY_FILE)pv ;
      gnMaxUceFiles += MAX_UCE_FILES;
    }
    else
    {
     bRet = FALSE;
    }
  }

  // Let's add the file now
  if( bRet )
  {
      int    i, j;
      WCHAR  wcBuf[256];
      WCHAR  wcBufNew[256];
      PWSTR  pwszSubsetName;

      UCE_GetTableName( pUceMemFile , &pwszSubsetName );
      if(*pwszSubsetName == L'0')
      {
          LoadString(hInst, _wtol(pwszSubsetName), wcBufNew, 255);
      }
      else
      {
          lstrcpy(wcBufNew, pwszSubsetName);
      }

      for(i = 0; i < gnUceFiles; i++)
      {
          UCE_GetTableName( &gpUceFile[i] , &pwszSubsetName );
          if(*pwszSubsetName == L'0')
          {
              LoadString(hInst, _wtol(pwszSubsetName), wcBuf, 255);
          }
          else
          {
              lstrcpy(wcBuf, pwszSubsetName);
          }

          if(CompareString(LOCALE_USER_DEFAULT, 0, wcBufNew, -1,  wcBuf, -1) == 1) break;
      }

      for(j = gnUceFiles; j > i; j--)
      {
          memcpy( &gpUceFile[j] , &gpUceFile[j-1] , sizeof(UCE_MEMORY_FILE) );
      }

      memcpy( &gpUceFile[i] , pUceMemFile , sizeof(UCE_MEMORY_FILE) );
      gnUceFiles++ ;
  }

  return bRet ;
}


/******************************Public*Routine******************************\
* UCE_EnumFiles
*
* Begin enumerate UCE files. Return number of correct find
* 
* Return Value:
*   Number of UCE entries found including the hardcoded 'All'
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]      
*    wrote it
\**************************************************************************/
UINT UCE_EnumFiles( void )
{
    WIN32_FIND_DATA W32FindData;
    HANDLE          hFile;
    WCHAR           wszSysDir[MAX_PATH];
    WCHAR           wszFilePath[MAX_PATH];
    UCE_MEMORY_FILE uceMemFile;
    INT             nLen;

    // Get UCE directory
    GetSystemDirectory( wszSysDir, MAX_PATH );
    nLen = lstrlen( wszSysDir ) ;
    if( nLen )
    {
      if( wszSysDir[nLen-1] != L'\\' )
      {
        wszSysDir[nLen] = L'\\';
        wszSysDir[nLen+1] = 0;
        nLen++;
      }
    }

    lstrcpy( wszFilePath , wszSysDir ) ;
    lstrcat(wszFilePath, L"*.uce");

    hFile = FindFirstFile( wszFilePath , &W32FindData );

    if( hFile != INVALID_HANDLE_VALUE ) 
    {
      do
      {
        if( !(W32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) 
        {
            lstrcpy( wszFilePath , wszSysDir ) ;
            lstrcat( wszFilePath , W32FindData.cFileName ) ;
          if( !UCE_LoadFile( wszFilePath , &uceMemFile ) ) 
          {
            //
            // Warnning, same extention, but isn't a valid UCE
            //

          }
          else
          {
            // Add file to UCE list
            Uce_AddFile( &uceMemFile );
#if DBG
            OutputDebugString(L"\nUCE File Loaded:");
            OutputDebugString(wszFilePath);
#endif 
          }
        }
        

      } while( FindNextFile( hFile , &W32FindData ));

      FindClose( hFile );
    }

    return (gnUceFiles+1);  // +1 is for default hard-coded one
}

/**************************************************************************\

  Close All UCE files, and free memory if needed

\**************************************************************************/
void UCE_CloseFiles( void )
{
  while( gnUceFiles>0 )
  {
    gnUceFiles--;
    UnmapViewOfFile( gpUceFile[gnUceFiles].pvData );
    CloseHandle( gpUceFile[gnUceFiles].hMapFile );
    CloseHandle( gpUceFile[gnUceFiles].hFile ) ;
  }

  if( gpUceFile != &gUceFiles[0] )
  {
    LocalFree( gpUceFile );
  }

  return ;
}


/******************************Public*Routine******************************\
* UCE_GetFiles
*
* Read in current list of UCE_list
* 
* Return Value:
*   Current UCE_MEMORY_FILEs loaded
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]      
*    wrote it
\**************************************************************************/
INT UCE_GetFiles( UCE_MEMORY_FILE **ppUceMemFile )
{
  *ppUceMemFile = gpUceFile ;

  return gnUceFiles;
}


/******************************Public*Routine******************************\
* UCE_GetTableName
*
* Position pointer to table name of current UCE_MEMORY_FILE
* 
* Return Value:
*
* History:
*   Sept-03-1997  Samer Arafeh  [samera]      
*    wrote it
\**************************************************************************/
BOOL UCE_GetTableName( PUCE_MEMORY_FILE pUceMemFile , PWSTR *ppszTableName )
{
  PUCE_HEADER pHeader = (PUCE_HEADER)(pUceMemFile->pvData);
  PSTR pFile= (PSTR)(pUceMemFile->pvData);

  *ppszTableName = (PWSTR)(pFile+pHeader->OffsetTableName);

  return TRUE;
}

/******************************Public*Routine******************************\
* UCE_GetCodepage
* 
* Return Value: table CodePage of current UCE_MEMORY_FILE
*
* History:
*   Nov-20-1997  kchang  created
\**************************************************************************/
WORD UCE_GetCodepage( PUCE_MEMORY_FILE pUceMemFile )
{
  PUCE_HEADER pHeader = (PUCE_HEADER)(pUceMemFile->pvData);
  PSTR pFile= (PSTR)(pUceMemFile->pvData);

  return (WORD)(pFile+pHeader->Codepage);
}
