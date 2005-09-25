/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Gerhard W. Gruber (sparhawk_at_gmx.at)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#include <malloc.h>

#define NDEBUG
#include "../include/debug.h"

/* GLOBALS *****************************************************************/

/* FUNCTIONS ****************************************************************/

static BOOL
AdjustFileAttributes (
	LPCWSTR ExistingFileName,
	LPCWSTR NewFileName
	)
{
	IO_STATUS_BLOCK IoStatusBlock;
	FILE_BASIC_INFORMATION ExistingInfo,
		NewInfo;
	HANDLE hFile;
	DWORD Attributes;
	NTSTATUS errCode;
	BOOL Result = FALSE;

	hFile = CreateFileW (ExistingFileName,
	                     FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
	                     FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL,
	                     NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		errCode = NtQueryInformationFile (hFile,
		                                  &IoStatusBlock,
		                                  &ExistingInfo,
		                                  sizeof(FILE_BASIC_INFORMATION),
		                                  FileBasicInformation);
		if (NT_SUCCESS (errCode))
		{
			if (0 != (ExistingInfo.FileAttributes & FILE_ATTRIBUTE_READONLY))
			{
				Attributes = ExistingInfo.FileAttributes;
				ExistingInfo.FileAttributes &= ~ FILE_ATTRIBUTE_READONLY;
				if (0 == (ExistingInfo.FileAttributes &
				          (FILE_ATTRIBUTE_HIDDEN |
				           FILE_ATTRIBUTE_SYSTEM |
				           FILE_ATTRIBUTE_ARCHIVE)))
				{
					ExistingInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
				}
				errCode = NtSetInformationFile (hFile,
				                                &IoStatusBlock,
				                                &ExistingInfo,
				                                sizeof(FILE_BASIC_INFORMATION),
				                                FileBasicInformation);
				if (!NT_SUCCESS(errCode))
				{
					DPRINT("Removing READONLY attribute from source failed with status 0x%08x\n", errCode);
				}
				ExistingInfo.FileAttributes = Attributes;
			}
			CloseHandle(hFile);

			if (NT_SUCCESS(errCode))
			{
				hFile = CreateFileW (NewFileName,
				                     FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
				                     FILE_SHARE_READ,
				                     NULL,
				                     OPEN_EXISTING,
			        	             FILE_ATTRIBUTE_NORMAL,
				                     NULL);
				if (INVALID_HANDLE_VALUE != hFile)
				{
					errCode = NtQueryInformationFile(hFile,
					                                 &IoStatusBlock,
					                                 &NewInfo,
					                                 sizeof(FILE_BASIC_INFORMATION),
					                                 FileBasicInformation);
					if (NT_SUCCESS(errCode))
					{
						NewInfo.FileAttributes = (NewInfo.FileAttributes &
						                          ~ (FILE_ATTRIBUTE_HIDDEN |
						                             FILE_ATTRIBUTE_SYSTEM |
						                             FILE_ATTRIBUTE_READONLY |
						                             FILE_ATTRIBUTE_NORMAL)) |
					                                 (ExistingInfo.FileAttributes &
					                                  (FILE_ATTRIBUTE_HIDDEN |
						                           FILE_ATTRIBUTE_SYSTEM |
						                           FILE_ATTRIBUTE_READONLY |
						                           FILE_ATTRIBUTE_NORMAL)) |
						                         FILE_ATTRIBUTE_ARCHIVE;
						NewInfo.CreationTime = ExistingInfo.CreationTime;
						NewInfo.LastAccessTime = ExistingInfo.LastAccessTime;
						NewInfo.LastWriteTime = ExistingInfo.LastWriteTime;
						errCode = NtSetInformationFile (hFile,
						                                &IoStatusBlock,
						                                &NewInfo,
						                                sizeof(FILE_BASIC_INFORMATION),
						                                FileBasicInformation);
						if (NT_SUCCESS(errCode))
						{
							Result = TRUE;
						}
						else
						{
							DPRINT("Setting attributes on dest file failed with status 0x%08x\n", errCode);
						}
					}
					else
					{
						DPRINT("Obtaining attributes from dest file failed with status 0x%08x\n", errCode);
					}
					CloseHandle(hFile);
				}
				else
				{
					DPRINT("Opening dest file to set attributes failed with code %d\n", GetLastError());
				}
			}
		}
		else
		{
			DPRINT("Obtaining attributes from source file failed with status 0x%08x\n", errCode);
			CloseHandle(hFile);
		}
	}
	else
	{
		DPRINT("Opening source file to obtain attributes failed with code %d\n", GetLastError());
	}

	return Result;
}

/***********************************************************************
 *           add_boot_rename_entry
 *
 * Adds an entry to the registry that is loaded when windows boots and
 * checks if there are some files to be removed or renamed/moved.
 * <fn1> has to be valid and <fn2> may be NULL. If both pointers are
 * non-NULL then the file is moved, otherwise it is deleted.  The
 * entry of the registrykey is always appended with two zero
 * terminated strings. If <fn2> is NULL then the second entry is
 * simply a single 0-byte. Otherwise the second filename goes
 * there. The entries are prepended with \??\ before the path and the
 * second filename gets also a '!' as the first character if
 * MOVEFILE_REPLACE_EXISTING is set. After the final string another
 * 0-byte follows to indicate the end of the strings.
 * i.e.:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * \??\D:\test\file2[0]
 * !\??\D:\test\file2_renamed[0]
 * [0]                        <- indicates end of strings
 *
 * or:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * [0]                        <- indicates end of strings
 *
 */
static BOOL add_boot_rename_entry( LPCWSTR source, LPCWSTR dest, DWORD flags )
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, source_name, dest_name;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HANDLE Reboot = 0;
    DWORD len1, len2;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;

    DPRINT("Add support to smss for keys created by MOVEFILE_DELAY_UNTIL_REBOOT\n");

    if (!RtlDosPathNameToNtPathName_U( (LPWSTR)source, &source_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    dest_name.Buffer = NULL;
    if (dest && !RtlDosPathNameToNtPathName_U( (LPWSTR)dest, &dest_name, NULL, NULL ))
    {
        RtlFreeUnicodeString( &source_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, SessionW );

    if (NtCreateKey( &Reboot, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)
    {
        DPRINT1("Error creating key for reboot managment [%s]\n",
             "SYSTEM\\CurrentControlSet\\Control\\Session Manager");
        RtlFreeUnicodeString( &source_name );
        RtlFreeUnicodeString( &dest_name );
        return FALSE;
    }

    len1 = source_name.Length + sizeof(WCHAR);
    if (dest)
    {
        len2 = dest_name.Length + sizeof(WCHAR);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            len2 += sizeof(WCHAR); /* Plus 1 because of the leading '!' */
    }
    else len2 = sizeof(WCHAR); /* minimum is the 0 characters for the empty second string */

    RtlInitUnicodeString( &nameW, ValueName );

    /* First we check if the key exists and if so how many bytes it already contains. */
    if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                         NULL, 0, &DataSize ) == STATUS_BUFFER_OVERFLOW)
    {
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
        if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                             Buffer, DataSize, &DataSize )) goto Quit;
        info = (KEY_VALUE_PARTIAL_INFORMATION *)Buffer;
        if (info->Type != REG_MULTI_SZ) goto Quit;
        if (DataSize > sizeof(info)) DataSize -= sizeof(WCHAR);  /* remove terminating null (will be added back later) */
    }
    else
    {
        DataSize = info_size;
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
    }

    memcpy( Buffer + DataSize, source_name.Buffer, len1 );
    DataSize += len1;
    p = (WCHAR *)(Buffer + DataSize);
    if (dest)
    {
        if (flags & MOVEFILE_REPLACE_EXISTING)
            *p++ = '!';
        memcpy( p, dest_name.Buffer, len2 );
        DataSize += len2;
    }
    else
    {
        *p = 0;
        DataSize += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(Buffer + DataSize);
    *p = 0;
    DataSize += sizeof(WCHAR);

    rc = !NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size);

 Quit:
    RtlFreeUnicodeString( &source_name );
    RtlFreeUnicodeString( &dest_name );
    if (Reboot) NtClose(Reboot);
    HeapFree( GetProcessHeap(), 0, Buffer );
    return(rc);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressW (
	LPCWSTR			lpExistingFileName,
	LPCWSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	PFILE_RENAME_INFORMATION FileRename;
	NTSTATUS errCode;
	BOOL Result;
	UNICODE_STRING DstPathU;
	BOOL folder = FALSE;

	DPRINT("MoveFileWithProgressW()\n");

	if (dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT)
		return add_boot_rename_entry( lpExistingFileName, lpNewFileName, dwFlags );

	hFile = CreateFileW (lpExistingFileName,
	                     GENERIC_ALL,
	                     FILE_SHARE_WRITE|FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_FLAG_BACKUP_SEMANTICS,
	                     NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
	   return FALSE;
	}

	
        /* validate & translate the filename */
        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewFileName,
				           &DstPathU,
				           NULL,
				           NULL))
        {
           DPRINT("Invalid destination path\n");
	   CloseHandle(hFile);
           SetLastError(ERROR_PATH_NOT_FOUND);
           return FALSE;
        }

	FileRename = alloca(sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length);
	if ((dwFlags & MOVEFILE_REPLACE_EXISTING) == MOVEFILE_REPLACE_EXISTING)
		FileRename->ReplaceIfExists = TRUE;
	else
		FileRename->ReplaceIfExists = FALSE;

	memcpy(FileRename->FileName, DstPathU.Buffer, DstPathU.Length);
        RtlFreeHeap (RtlGetProcessHeap (),
		     0,
		     DstPathU.Buffer);
	/*
	 * FIXME:
	 *   Is the length the count of characters or the length of the buffer?
	 */
	FileRename->FileNameLength = DstPathU.Length / sizeof(WCHAR);
	errCode = NtSetInformationFile (hFile,
	                                &IoStatusBlock,
	                                FileRename,
	                                sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length,
	                                FileRenameInformation);
	CloseHandle(hFile);

	if (GetFileAttributesW(lpExistingFileName) & FILE_ATTRIBUTE_DIRECTORY)
	{
           folder = TRUE;
	}

	
	/*
	 *  FIXME:
	 *  Fail now move the folder 
	 *  Before we fail at CreateFileW 
	 */
	 
	 
	if (NT_SUCCESS(errCode))
	{
		Result = TRUE;
	}
	else 
	{
 	        if (folder==FALSE)
		{
		    Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
 		    if (Result)
		    {
			/* Cleanup the source file */
			AdjustFileAttributes(lpExistingFileName, lpNewFileName);
	                Result = DeleteFileW (lpExistingFileName);
		    } 
                  }
		 else
		 {
		   /* move folder code start */
		   WIN32_FIND_DATAW findBuffer;
		   LPCWSTR lpExistingFileName2 = NULL;
		   LPCWSTR lpNewFileName2 = NULL; 
		   LPCWSTR lpDeleteFile = NULL;
		   INT size;
		   INT size2;
		   BOOL loop = TRUE;
		   BOOL Result = FALSE;

		   /* Build the string */
		   size = wcslen(lpExistingFileName); 

		   lpDeleteFile = (LPCWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,MAX_PATH * sizeof(WCHAR));
		   if (lpDeleteFile == NULL)
			   goto FreeMemAndExit;

		   lpNewFileName2 = (LPCWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,MAX_PATH * sizeof(WCHAR));
		   if (lpNewFileName2 == NULL)
			   goto FreeMemAndExit;

		   lpExistingFileName2 = (LPCWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,MAX_PATH * sizeof(WCHAR));
		   if (lpExistingFileName2 == NULL)
			   goto FreeMemAndExit;
	
		   if ((size+6)*sizeof(WCHAR)>MAX_PATH)
		   {
		     HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(VOID *) lpExistingFileName2,(size+6)*sizeof(WCHAR));
			 if (lpExistingFileName2 == NULL)
			     goto FreeMemAndExit;
		   }
	
		   wcscpy( (WCHAR *)lpExistingFileName2,lpExistingFileName);
		   wcscpy( (WCHAR *)&lpExistingFileName2[size],L"\\*.*\0");
	  
		   /* Get the file name */
		   memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
		   hFile = FindFirstFileW(lpExistingFileName2,  &findBuffer);
		   if (hFile == NULL) 
		       loop=FALSE;

		   if (findBuffer.cFileName[0] == L'\0')
		       loop=FALSE;
	
		   DPRINT("MoveFileWithProgressW : lpExistingFileName1 = %S\n",lpExistingFileName);
		   DPRINT("MoveFileWithProgressW : lpExistingFileName2 = %S\n",lpExistingFileName2);
		   DPRINT("MoveFileWithProgressW : lpNewFileName = %S\n",lpNewFileName);

		   DPRINT("MoveFileWithProgressW : loop = %d %d %d\n",TRUE, FALSE, loop);

		   
		   CreateDirectoryW(lpNewFileName,NULL);


		   /* search the file */
		   while (loop==TRUE)
		   {	
		     Result = TRUE;

		     if ((!wcscmp(findBuffer.cFileName,L"..")) || (!wcscmp(findBuffer.cFileName,L".")))
		     {		  
		       loop = FindNextFileW(hFile, &findBuffer);		  
		  		  
		       if (!loop)
		       {					 
		         size = wcslen(lpExistingFileName2)-4;
		         FindClose(hFile);
		         wcscpy( (WCHAR *)&lpExistingFileName2[size],L"\0");

		         if (wcsncmp(lpExistingFileName,lpExistingFileName2,size))
		         {  
		           FindClose(hFile);			     

				   /* delete folder */	

				   size = GetFullPathNameW(lpExistingFileName2, MAX_PATH,(LPWSTR) lpDeleteFile, NULL);
				   if (size>MAX_PATH)
				   {                  
				     lpDeleteFile = (LPCWSTR) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, 
				                    (VOID *) lpDeleteFile,size);

					 if (lpDeleteFile == NULL)
					 {
						 Result = FALSE;
			             goto FreeMemAndExit;
					 }

				     GetFullPathNameW(lpExistingFileName2, size,(LPWSTR) lpDeleteFile, NULL);
				   }
				 
				   DPRINT("MoveFileWithProgressW : folder : %s\n",lpDeleteFile);

				   Result = RemoveDirectoryW(lpDeleteFile);
				   if (Result == FALSE)
				       break;
				 				 
				   loop=TRUE;				 				 
				   size = wcslen(lpExistingFileName); 
				
				   if ((size+6)*sizeof(WCHAR)>MAX_PATH)
				   {
				     lpExistingFileName2 = (LPCWSTR) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, 
				                           (VOID *)lpExistingFileName2,(size+6)*sizeof(WCHAR));

					 if (lpExistingFileName2 == NULL)
					 {
						 Result = FALSE;
			             goto FreeMemAndExit;
					 }
				   }

				   wcscpy( (WCHAR *)lpExistingFileName2,lpExistingFileName);
				   wcscpy( (WCHAR *)&lpExistingFileName2[size],L"\\*.*\0");

				   /* Get the file name */
				   memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
				   hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);	                 
		         }
               } 		  
               continue;		  		  
             }
	  	  	  
		     if (findBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		     {	
               DPRINT("MoveFileWithProgressW : 1: %S %S\n",lpExistingFileName2,findBuffer.cFileName);

               /* Build the new string */		  		  		  
               size = wcslen(findBuffer.cFileName); 
               size2= wcslen(lpExistingFileName2);

               if ((size2+size+6)*sizeof(WCHAR)>MAX_PATH)
               {
	             lpExistingFileName2 = (LPCWSTR) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, 
				                       (VOID *)lpExistingFileName2, (size2+size+6)*sizeof(WCHAR));

				 if (lpExistingFileName2 == NULL)
				 {
				   Result = FALSE;
			       goto FreeMemAndExit;
				 }
	           }

	           wcscpy( (WCHAR *)&lpExistingFileName2[size2-3],findBuffer.cFileName);
	           wcscpy( (WCHAR *)&lpExistingFileName2[size2+size-3],L"\\*.*\0");
          
		  
	           /* Build the new dst string */
	           size = wcslen(lpExistingFileName2) + wcslen(lpNewFileName);
	           size2 = wcslen(lpExistingFileName);
		  
	           if (size>MAX_PATH)
	           {
	             lpNewFileName2 = (LPCWSTR) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, 
                                  (VOID *) lpNewFileName2, size*sizeof(WCHAR));

				 if (lpNewFileName2 == NULL)
				 {
				   Result = FALSE;
			       goto FreeMemAndExit;
				 }
	           }

	           wcscpy((WCHAR *) lpNewFileName2,lpNewFileName);		 		  
	           size = wcslen(lpNewFileName);		 
	           wcscpy((WCHAR *)&lpNewFileName2[size], (WCHAR *)&lpExistingFileName2[size2]);
	           size = wcslen(lpNewFileName2);
	           wcscpy( (WCHAR *)&lpNewFileName2[size-4],L"\0");

	           /* build dest path */
	           /* remove this code when it will be out into kernel32.dll ? */				 

	           size = GetFullPathNameW(lpNewFileName2, MAX_PATH,(LPWSTR) lpDeleteFile, NULL);
	           if (MAX_PATH>size2)
	           {
	            lpDeleteFile = (LPCWSTR) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, 
	                           (VOID *) lpDeleteFile,size) ;

				if (lpDeleteFile == NULL)
				{
				  Result = FALSE;
			      goto FreeMemAndExit;
				}

	            GetFullPathNameW(lpNewFileName2, size,(LPWSTR) lpDeleteFile, NULL);
	           }

	           /* Create Folder */
	          
	           DPRINT("MoveFileWithProgressW : CreateDirectoryW lpNewFileName2 : %S\n",lpNewFileName2);
	           DPRINT("MoveFileWithProgressW : CreateDirectoryW : %S\n",lpDeleteFile);

	           CreateDirectoryW(lpDeleteFile,NULL);

	           DPRINT("MoveFileWithProgressW : 1x: %S : %S \n",lpExistingFileName2, lpNewFileName2);

	           FindClose(hFile);
	           memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
	           hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);
	         }
		     else
		     {
		  
	           /* Build the new string */		  		  		  		  
	           size = wcslen(findBuffer.cFileName); 
	           size2= wcslen(lpExistingFileName2);
	           wcscpy( (WCHAR *)lpDeleteFile,lpExistingFileName2);	   
	           wcscpy( (WCHAR *)&lpDeleteFile[size2-3],findBuffer.cFileName);	   
		  
	           /* Build dest string */
	           size = wcslen(lpDeleteFile) + wcslen(lpNewFileName);
	           size2 = wcslen(lpExistingFileName);

	           if (size*sizeof(WCHAR)>MAX_PATH)
	           {
                 lpNewFileName2 = (LPCWSTR) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, 
                                  (VOID *) lpNewFileName2, size*sizeof(WCHAR)); 

				 if (lpNewFileName2 == NULL)
				 {
				   Result = FALSE;
			       goto FreeMemAndExit;
				 }
               }
		  
               wcscpy((WCHAR *) lpNewFileName2,lpNewFileName);		 		  
               size = wcslen(lpNewFileName);		 
               wcscpy((WCHAR *)&lpNewFileName2[size], (WCHAR *)&lpDeleteFile[size2]);
		 
               /* copy file */		  
               Result = CopyFileW(lpDeleteFile,lpNewFileName2, FALSE);               			   			              

               /* delete file */
               DPRINT("MoveFileWithProgressW : Delete file : %S : %S\n",lpDeleteFile, lpNewFileName2);
		  
           
               Result = DeleteFileW(lpDeleteFile);
               if (Result == FALSE)
               {  
                 DPRINT("MoveFileWithProgressW : Fails\n");
                 break;
               }
             }	  	 
             DPRINT("MoveFileWithProgressW : 2 : %S  : %S \n",lpExistingFileName2,findBuffer.cFileName);
             loop = FindNextFileW(hFile, &findBuffer);	  	  
           }

           FindClose(hFile);    
           memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
           hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);
           if (hFile == NULL) 
               loop=TRUE;

           if (findBuffer.cFileName[0] == L'\0')
               loop=TRUE;

           if (loop == FALSE)
		   {
		     FindClose(hFile);				 
		     Result = RemoveDirectoryW(lpExistingFileName);
		     DPRINT("MoveFileWithProgressW RemoveDirectoryW :%S",lpExistingFileName);
		   }

FreeMemAndExit:
		   DPRINT("MoveFileWithProgressW : result : r=%d, T=%d, F=%d",Result,TRUE,FALSE);

		   if (lpNewFileName2 != NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		     lpNewFileName2 = NULL;
		   }

		   if (lpExistingFileName2 != NULL)
		   {	  
		     HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		     lpExistingFileName2 = NULL;
		   }

		   if (lpDeleteFile != NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		     lpDeleteFile = NULL;
		   }

		   return Result;
		   // end move folder code		 
		  }
	}
	
#if 1
	/* FIXME file rename not yet implemented in all FSDs so it will always
	 * fail, even when the move is to the same device
	 */
	//else if (STATUS_NOT_IMPLEMENTED == errCode)
	{

		UNICODE_STRING SrcPathU;

		SrcPathU.Buffer = alloca(sizeof(WCHAR) * MAX_PATH);
		SrcPathU.MaximumLength = MAX_PATH * sizeof(WCHAR);
		SrcPathU.Length = GetFullPathNameW(lpExistingFileName, MAX_PATH, SrcPathU.Buffer, NULL);
		if (SrcPathU.Length >= MAX_PATH)
		{
		    SetLastError(ERROR_FILENAME_EXCED_RANGE);
		    return FALSE;
		}
		SrcPathU.Length *= sizeof(WCHAR);

		DstPathU.Buffer = alloca(sizeof(WCHAR) * MAX_PATH);
		DstPathU.MaximumLength = MAX_PATH * sizeof(WCHAR);
		DstPathU.Length = GetFullPathNameW(lpNewFileName, MAX_PATH, DstPathU.Buffer, NULL);
		if (DstPathU.Length >= MAX_PATH)
		{
		    SetLastError(ERROR_FILENAME_EXCED_RANGE);
		    return FALSE;
		}
		DstPathU.Length *= sizeof(WCHAR);

		if (0 == RtlCompareUnicodeString(&SrcPathU, &DstPathU, TRUE))
		{
		   /* Source and destination file are the same, nothing to do */
		   return TRUE;
		}

		Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
		if (Result)
		{
		    /* Cleanup the source file */
                    AdjustFileAttributes(lpExistingFileName, lpNewFileName);
		    Result = DeleteFileW (lpExistingFileName);
		}
	}
#endif
	
	return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressA (
	LPCSTR			lpExistingFileName,
	LPCSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	PWCHAR ExistingFileNameW;
   PWCHAR NewFileNameW;
	BOOL ret;

   if (!(ExistingFileNameW = FilenameA2W(lpExistingFileName, FALSE)))
      return FALSE;

   if (!(NewFileNameW= FilenameA2W(lpNewFileName, TRUE)))
      return FALSE;

   ret = MoveFileWithProgressW (ExistingFileNameW ,
                                   NewFileNameW,
	                                lpProgressRoutine,
	                                lpData,
	                                dwFlags);

   RtlFreeHeap (RtlGetProcessHeap (), 0, NewFileNameW);

	return ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName
	)
{
	return MoveFileExW (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileExW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	return MoveFileWithProgressW (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName
	)
{
	return MoveFileExA (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileExA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	return MoveFileWithProgressA (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}

/* EOF */
