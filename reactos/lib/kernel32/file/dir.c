/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/*
 * NOTES: Changed to using ZwCreateFile
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <string.h>
#include <wchar.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL CreateDirectoryA(LPCSTR lpPathName,
				 LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   return CreateDirectoryExA(NULL,lpPathName,lpSecurityAttributes);
}

WINBOOL STDCALL CreateDirectoryExA(LPCSTR lpTemplateDirectory,
				   LPCSTR lpNewDirectory,
				   LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   WCHAR TemplateDirectoryW[MAX_PATH];
   WCHAR NewDirectoryW[MAX_PATH];
   ULONG i;

   DPRINT("lpTemplateDirectory %s lpNewDirectory %s lpSecurityAttributes %p\n",
          lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);

   if (lpTemplateDirectory)
     {
        i = 0;
        while ((*lpTemplateDirectory)!=0 && i < MAX_PATH)
          {
             TemplateDirectoryW[i] = *lpTemplateDirectory;
             lpTemplateDirectory++;
             i++;
          }
        TemplateDirectoryW[i] = 0;
     }
   DPRINT ("\n");

   if (lpNewDirectory)
     {
        i = 0;
        while ((*lpNewDirectory)!=0 && i < MAX_PATH)
          {
             NewDirectoryW[i] = *lpNewDirectory;
             lpNewDirectory++;
             i++;
          }
        NewDirectoryW[i] = 0;
     }
   DPRINT ("\n");

   return CreateDirectoryExW(lpTemplateDirectory?TemplateDirectoryW:NULL,
                             lpNewDirectory?NewDirectoryW:NULL,
                             lpSecurityAttributes);
}


WINBOOL STDCALL CreateDirectoryW(LPCWSTR lpPathName, 
				 LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   return CreateDirectoryExW(NULL,lpPathName,lpSecurityAttributes);
}

WINBOOL STDCALL CreateDirectoryExW(LPCWSTR lpTemplateDirectory,
				   LPCWSTR lpNewDirectory,
				   LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
   NTSTATUS errCode;
   HANDLE DirectoryHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING DirectoryNameString;
   IO_STATUS_BLOCK IoStatusBlock;

   DPRINT("lpTemplateDirectory %w lpNewDirectory %w lpSecurityAttributes %p\n",
           lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);

   if ( lpTemplateDirectory != NULL && *lpTemplateDirectory != 0 )
     {
	// get object attributes from template directory
	DPRINT("KERNEL32:FIXME:%s:%d\n",__FILE__,__LINE__);
	return(FALSE);
     }

   DirectoryNameString.Length = lstrlenW(lpNewDirectory)*sizeof(WCHAR);
   DirectoryNameString.Buffer = (WCHAR *)lpNewDirectory;
   DirectoryNameString.MaximumLength = DirectoryNameString.Length+sizeof(WCHAR);

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &DirectoryNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE | OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;

   errCode = ZwCreateFile(&DirectoryHandle,
			  DIRECTORY_ALL_ACCESS,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  NULL,
			  FILE_ATTRIBUTE_DIRECTORY,
			  0,
			  FILE_CREATE,
			  0,
			  NULL,
			  0);
   DPRINT("errCode: %x\n", errCode);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }

   NtClose(DirectoryHandle);

   return TRUE;
}



WINBOOL STDCALL RemoveDirectoryA(LPCSTR lpPathName)
{
	WCHAR PathNameW[MAX_PATH];
	ULONG i;
	i = 0;
   	while ((*lpPathName)!=0 && i < MAX_PATH)
     	{
		PathNameW[i] = *lpPathName;
		lpPathName++;
		i++;
     	}
   	PathNameW[i] = 0;	
	return RemoveDirectoryW(PathNameW);
}


WINBOOL STDCALL RemoveDirectoryW(LPCWSTR lpPathName)
{
   NTSTATUS errCode;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PathNameString;
   
   PathNameString.Length = lstrlenW(lpPathName)*sizeof(WCHAR);
   PathNameString.Buffer = (WCHAR *)lpPathName;
   PathNameString.MaximumLength = PathNameString.Length+sizeof(WCHAR);
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &PathNameString;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   errCode = NtDeleteFile(&ObjectAttributes);

   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   return TRUE;
}


static DWORD GetDotSequenceLengthA (PSTR p)
{
    DWORD dwCount = 0;

    for (;;)
    {
	if (*p == '.')
	    dwCount++;
	else if ((*p == '\\' || *p == '\0') && dwCount)
	    return dwCount;
	else
	    return 0;
	p++;
    }
    return 0;
}

DWORD STDCALL GetFullPathNameA(LPCSTR lpFileName,
			       DWORD nBufferLength,
			       LPSTR lpBuffer,
			       LPSTR *lpFilePart)
{
   PSTR p;
   PSTR prev = NULL;
   
   DPRINT("GetFullPathNameA(lpFileName %s, nBufferLength %d, lpBuffer %p, "
          "lpFilePart %p)\n",lpFileName,nBufferLength,lpBuffer,lpFilePart);
   
   if (!lpFileName || !lpBuffer)
        return 0;
   
   if (isalpha(lpFileName[0]) && lpFileName[1] == ':')
     {
        if (lpFileName[2] == '\\')
          {
             lstrcpyA(lpBuffer, lpFileName);
          }
        else
          {
             CHAR  szRoot[4] = "A:\\";
             DWORD len;

             szRoot[0] = lpFileName[0];
             len = GetCurrentDirectoryA(nBufferLength, lpBuffer);
             if (lpBuffer[len - 1] != '\\')
               {
                  lpBuffer[len] = '\\';
                  lpBuffer[len + 1] = 0;
               }
             lstrcatA(lpBuffer, &lpFileName[2]);
          }
     }
   else if (lpFileName[0] == '\\')
     {
        GetCurrentDirectoryA(nBufferLength, lpBuffer);
        lstrcpyA(&lpBuffer[2], lpFileName);
     }
   else
     {
        DWORD len;
        len = GetCurrentDirectoryA(nBufferLength, lpBuffer);
        if (lpBuffer[len - 1] != '\\')
          {
             lpBuffer[len] = '\\';
             lpBuffer[len + 1] = 0;
          }
        lstrcatA(lpBuffer, lpFileName);
     }
   
   DPRINT("lpBuffer %s\n",lpBuffer);

   p = lpBuffer + 2;
   prev = p;

   while ((*p) != 0 || ((*p) == '\\' && (*(p+1)) == 0))
     {
         DWORD dwDotLen;

         dwDotLen = GetDotSequenceLengthA (p+1);
         DPRINT("DotSequenceLength %u\n", dwDotLen);
         DPRINT("prev %s p %s\n",prev,p);

         if (dwDotLen == 0)
           {
              prev = p;
              do
		{
                   p++;
		}
              while ((*p) != 0 && (*p) != '\\');
           }
         else if (dwDotLen == 1)
           {
              lstrcpyA(p, p+2);
           }
         else
           {
              if (dwDotLen > 2)
		{
                   int n = dwDotLen - 2;

                   while (n > 0 && prev > (lpBuffer+2))
                     {
                        prev--;
                        if ((*prev) == '\\')
                             n--;
                     }
		}

              if (*(p+dwDotLen+1) == 0)
                   *(prev+1) = 0;
              else
                   lstrcpyA (prev, p+dwDotLen+1);
              p = prev;
              if (prev > (lpBuffer+2))
		{
                   prev--;
                   while ((*prev) != '\\')
                     {
                        prev--;
                     }
		}
           }
     }
   
   if (lpFilePart != NULL)
     {
	(*lpFilePart) = prev + 1;
     }
   
   DPRINT("lpBuffer %s\n",lpBuffer);
   
   return strlen(lpBuffer);
}


static DWORD GetDotSequenceLengthW (PWSTR p)
{
   DWORD dwCount = 0;

   for (;;)
     {
	if (*p == '.')
             dwCount++;
	else if ((*p == '\\' || *p == '\0') && dwCount)
             return dwCount;
	else
             return 0;
	p++;
     }
   return 0;
}


DWORD STDCALL GetFullPathNameW(LPCWSTR lpFileName,
			       DWORD nBufferLength,
			       LPWSTR lpBuffer,
			       LPWSTR *lpFilePart)
{
   PWSTR p;
   PWSTR prev = NULL;
   
   DPRINT("GetFullPathNameW(lpFileName %w, nBufferLength %d, lpBuffer %p, "
          "lpFilePart %p)\n",lpFileName,nBufferLength,lpBuffer,lpFilePart);
   
   if (!lpFileName || !lpBuffer)
        return 0;
   
   if (isalpha(lpFileName[0]) && lpFileName[1] == L':')
     {
        if (lpFileName[2] == L'\\')
          {
             lstrcpyW(lpBuffer, lpFileName);
          }
        else
          {
             WCHAR szRoot[4] = L"A:\\";
             DWORD len;

             szRoot[0] = lpFileName[0];
             len = GetCurrentDirectoryW(nBufferLength, lpBuffer);
             if (lpBuffer[len - 1] != L'\\')
               {
                  lpBuffer[len] = L'\\';
                  lpBuffer[len + 1] = 0;
               }
             lstrcatW(lpBuffer, &lpFileName[2]);
          }
     }
   else if (lpFileName[0] == L'\\')
     {
        GetCurrentDirectoryW(nBufferLength, lpBuffer);
	lstrcpyW(&lpBuffer[2], lpFileName);
     }
   else
     {
        DWORD len;
        len = GetCurrentDirectoryW(nBufferLength, lpBuffer);
        if (lpBuffer[len - 1] != L'\\')
          {
             lpBuffer[len] = L'\\';
             lpBuffer[len + 1] = 0;
          }
	lstrcatW(lpBuffer, lpFileName);
     }
   
   DPRINT("lpBuffer %w\n",lpBuffer);
   
   p = lpBuffer + 2;
   prev = p;
   
   while ((*p) != 0 || ((*p) == L'\\' && (*(p+1)) == 0)) 
     {
	DWORD dwDotLen;

	dwDotLen = GetDotSequenceLengthW (p+1);
	DPRINT("DotSequenceLength %u\n", dwDotLen);
	DPRINT("prev %w p %w\n",prev,p);

	if (dwDotLen == 0)
          {
             prev = p;
             do
               {
                  p++;
               }
             while ((*p) != 0 && (*p) != L'\\');
          }
	else if (dwDotLen == 1)
          {
             lstrcpyW(p, p+2);
          }
	else
          {
             if (dwDotLen > 2)
               {
                  int n = dwDotLen - 2;

                  while (n > 0 && prev > (lpBuffer+2))
                    {
                       prev--;
                       if ((*prev) == L'\\')
                            n--;
                    }
               }

             if (*(p+dwDotLen+1) == 0)
                  *(prev+1) = 0;
             else
                  lstrcpyW (prev, p+dwDotLen+1);
             p = prev;
             if (prev > (lpBuffer+2))
               {
                  prev--;
                  while ((*prev) != L'\\')
                    {
                       prev--;
                    }
               }
          }
     }
 
   if (lpFilePart != NULL)
     {
	(*lpFilePart) = prev + 1;
     }
   
   DPRINT("lpBuffer %w\n",lpBuffer);
   
   return wcslen(lpBuffer);
}

DWORD
STDCALL
GetShortPathNameA(
		  LPCSTR lpszLongPath,
		  LPSTR  lpszShortPath,
		  DWORD    cchBuffer
		  )
{
	//1 remove unicode chars and spaces
	//2 remove preceding and trailing periods. 
	//3 remove embedded periods except the last one
	
	//4 Split the string in two parts before and after the period
	//	truncate the part before the period to 6 chars and add ~1
	//	truncate the part after the period to 3 chars
	//3 Put the new name in uppercase
	
	//4 Increment the ~1 string if the resulting name allready exists

}	

DWORD
STDCALL
GetShortPathNameW(
    LPCWSTR lpszLongPath,
    LPWSTR  lpszShortPath,
    DWORD    cchBuffer
    )
{
	
}

DWORD
STDCALL
SearchPathA(
    LPCSTR lpPath,
    LPCSTR lpFileName,
    LPCSTR lpExtension,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    )
{
	WCHAR PathW[MAX_PATH];
	WCHAR FileNameW[MAX_PATH];
	WCHAR ExtensionW[MAX_PATH];

	WCHAR BufferW[MAX_PATH];
	WCHAR *FilePartW;

	ULONG i;
	DWORD RetValue;

	i = 0;
	while ((*lpPath)!=0 && i < MAX_PATH)
     	{
		PathW[i] = *lpPath;
		lpPath++;
		i++;
     	}
   	PathW[i] = 0;
  	
	i = 0;
	while ((*lpFileName)!=0 && i < MAX_PATH)
     	{
		FileNameW[i] = *lpFileName;
		lpFileName++;
		i++;
     	}
   	FileNameW[i] = 0;

	i = 0;
	while ((*lpExtension)!=0 && i < MAX_PATH)
     	{
		ExtensionW[i] = *lpExtension;
		lpExtension++;
		i++;
     	}
   	ExtensionW[i] = 0;

	RetValue = SearchPathW(PathW,FileNameW,ExtensionW,nBufferLength,BufferW,&FilePartW);
	for(i=0;i<nBufferLength;i++)
		lpBuffer[i] = (char)BufferW[i];

        *lpFilePart = strrchr(lpBuffer,'\\')+1;
	return RetValue;
}

DWORD STDCALL SearchPathW(LPCWSTR lpPath,	 
			  LPCWSTR lpFileName,	 
			  LPCWSTR lpExtension, 
			  DWORD nBufferLength, 
			  LPWSTR lpBuffer,	
			  LPWSTR *lpFilePart)
/*
 * FUNCTION: Searches for the specified file
 * ARGUMENTS:
 *       lpPath = Points to a null-terminated string that specified the
 *                path to be searched. If this parameters is NULL then
 *                the following directories are searched
 *                          The directory from which the application loaded
 *                          The current directory
 *                          The system directory
 *                          The 16-bit system directory
 *                          The windows directory
 *                          The directories listed in the PATH environment
 *                          variable
 *        lpFileName = Specifies the filename to search for
 *        lpExtension = Points to the null-terminated string that specifies
 *                      an extension to be added to the filename when
 *                      searching for the file. The first character of the
 *                      filename extension must be a period (.). The
 *                      extension is only added if the specified filename
 *                      doesn't end with an extension
 *                      
 *                      If the filename extension is not required or if the
 *                      filename contains an extension, this parameters can be
 *                      NULL
 *        nBufferLength = The length in characters of the buffer for output
 *        lpBuffer = Points to the buffer for the valid path and filename of
 *                   file found
 *        lpFilePart = Points to the last component of the valid path and
 *                     filename 
 * RETURNS: On success, the length, in characters, of the string copied to the
 *          buffer
 *          On failure, zero.
 */
{
	

	NTSTATUS errCode;
	DWORD retCode = 0;
	HANDLE FileHandle = NULL;

	ULONG i,j;

	WCHAR BufferW[MAX_PATH];
	WCHAR FileAndExtensionW[MAX_PATH];
	WCHAR *EnvironmentBufferW = NULL;
	
	UNICODE_STRING PathString;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	dprintf("SearchPath\n");

	
	if ( lpPath == NULL )  {
		
		// check the directory from which the application loaded

		if ( GetCurrentDirectoryW( MAX_PATH, BufferW  ) > 0 ) {
			retCode = SearchPathW(BufferW,lpFileName, lpExtension, nBufferLength, 	lpBuffer,	 lpFilePart 	 );
			if ( retCode != 0 )
				return retCode;
		}
		if ( GetSystemDirectoryW( BufferW,	  MAX_PATH  ) > 0 ) {
			retCode = SearchPathW(BufferW,lpFileName, lpExtension, nBufferLength,  lpBuffer,	 lpFilePart 	 );
			if ( retCode != 0 )
				return retCode;
		}

		if ( GetWindowsDirectoryW( BufferW,	  MAX_PATH  ) > 0 ) {
			retCode = SearchPathW(BufferW,lpFileName, lpExtension, nBufferLength, 	lpBuffer,	 lpFilePart 	 );
			if ( retCode != 0  )
				return retCode;
		}
	
		j = GetEnvironmentVariableW(L"Path",EnvironmentBufferW,0);
		EnvironmentBufferW = (WCHAR *) HeapAlloc(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,(j+1)*sizeof(WCHAR));

		j = GetEnvironmentVariableW(L"Path",EnvironmentBufferW,j+1);

		for(i=0;i<j;i++) {
			if ( EnvironmentBufferW[i] == L';' )
				EnvironmentBufferW[i] = 0;
		}
		i = 0;
		while ( retCode == 0  && i < j ) {
			if (  EnvironmentBufferW[i] != 0 )
				retCode = SearchPathW(&EnvironmentBufferW[i],lpFileName, lpExtension, nBufferLength, 	lpBuffer,	 lpFilePart 	 );
			i += lstrlenW(&EnvironmentBufferW[i]) + 1;
			
			
		}
		
		
		HeapFree(GetProcessHeap(),0,EnvironmentBufferW);

		

		return retCode;

	}
	else {

		FileAndExtensionW[0] = 0;
		lpBuffer[0] = 0;
		i = lstrlenW(lpFileName);
		j = lstrlenW(lpPath);

		if ( i + j + 8 < nBufferLength )
			return i + j + 9;

		if ( lpExtension != NULL ) {
			if ( lpFileName[i-4] != L'.' ) {
				wcscpy(FileAndExtensionW,lpFileName);
				wcscat(FileAndExtensionW,lpExtension);
			}
			else
				wcscpy(FileAndExtensionW,lpFileName);
		}
		else
			wcscpy(FileAndExtensionW,lpFileName);
        
		
		lstrcatW(BufferW,L"\\??\\");
		lstrcatW(BufferW,lpPath);
		
	
		//printf("%S\n",FileAndExtensionW);


		i = wcslen(BufferW);
		if ( BufferW[i-1] != L'\\' ) {
			BufferW[i] = L'\\';
			BufferW[i+1] = 0;
		}
		if ( lpFilePart != NULL )
			*lpFilePart = &BufferW[wcslen(BufferW)+1];
		wcscat(BufferW,FileAndExtensionW);
		//printf("%S\n",lpBuffer);

		PathString.Buffer = BufferW;
		PathString.Length = lstrlenW(PathString.Buffer)*sizeof(WCHAR);
   		PathString.MaximumLength = PathString.Length + sizeof(WCHAR);
   
   
    	

  		ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
		ObjectAttributes.RootDirectory = NULL;
   		ObjectAttributes.ObjectName = &PathString;
		ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE| OBJ_INHERIT;
		ObjectAttributes.SecurityDescriptor = NULL;
		ObjectAttributes.SecurityQualityOfService = NULL;

		errCode = NtOpenFile(
		&FileHandle,
		GENERIC_ALL|FILE_LIST_DIRECTORY,
		&ObjectAttributes,
		&IoStatusBlock,   
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,         
   		0		                                                                
		);
	

		if ( !NT_SUCCESS(errCode) ) {
			return 0;
		}
		else {
			NtClose(FileHandle);
			wcscpy(lpBuffer,&BufferW[4]);
                        *lpFilePart = wcsrchr(lpBuffer,'\\')+1;
		}

	}

	return lstrlenW(lpBuffer);
}

