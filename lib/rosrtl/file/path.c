#include <windows.h>
#include <string.h>


/***********************************************************************
 *           MakeSureDirectoryPathExistsExA
 *
 * If a dir is at the end and the path ends with a backslash, FileAtEnd
 * is ignored. If the path doesn't end with a backslash, FileAtEnd is
 * used to determine if the last part of the path is a file name or a
 * directory.
 *
 * Path may be absolute or relative to current dir.
 */
BOOL STDCALL MakeSureDirectoryPathExistsExA(LPCSTR DirPath, BOOL FileAtEnd)
{
   char Path[MAX_PATH];
   char *SlashPos = Path;
   char Slash;
   BOOL bRes;
   
   strcpy(Path, DirPath);
        
   while((SlashPos=strpbrk(SlashPos+1,"\\/")))
   {
      Slash = *SlashPos;
      *SlashPos = 0;
      
      bRes = CreateDirectoryA(Path, NULL);
      if (bRes == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return FALSE;
      }
      
      *SlashPos = Slash;
      
      if (*(SlashPos+1) == 0) return TRUE;
   }

   if (!FileAtEnd)
   {
      bRes = CreateDirectoryA(Path, NULL);
      if (bRes == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return FALSE;
      }
   }

   return TRUE;
}




/***********************************************************************
 *           MakeSureDirectoryPathExistsExW
 *
 * If a dir is at the end and the path ends with a backslash, FileAtEnd
 * is ignored. If the path doesn't end with a backslash, FileAtEnd is
 * used to determine if the last part of the path is a file name or a
 * directory.
 *
 * Path may be absolute or relative to current dir.
 */
BOOL STDCALL MakeSureDirectoryPathExistsExW(LPCWSTR DirPath, BOOL FileAtEnd)
{
   WCHAR Path[MAX_PATH];
   WCHAR *SlashPos = Path;
   WCHAR Slash;
   BOOL bRes;
   
   wcscpy(Path, DirPath);
        
   while((SlashPos=wcspbrk(SlashPos+1,L"\\/")))
   {
      Slash = *SlashPos;
      *SlashPos = 0;
      
      bRes = CreateDirectoryW(Path, NULL);
      if (bRes == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return FALSE;
      }
      
      *SlashPos = Slash;
      
      if (*(SlashPos+1) == 0) return TRUE;
   }

   if (!FileAtEnd)
   {
      bRes = CreateDirectoryW(Path, NULL);
      if (bRes == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
      {
         return FALSE;
      }
   }

   return TRUE;
}
