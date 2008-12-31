/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Gerhard W. Gruber (sparhawk_at_gmx.at)
 *                  Dmitry Philippov (shedon@mail.ru)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  DP (29/07/2006)
 *                      Fix some bugs in the add_boot_rename_entry function
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#include <malloc.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/* GLOBALS *****************************************************************/

/* FUNCTIONS ****************************************************************/
static BOOL
RemoveReadOnlyAttributeW(IN LPCWSTR lpFileName)
{
    DWORD Attributes;
    Attributes = GetFileAttributesW(lpFileName);
    if (Attributes != INVALID_FILE_ATTRIBUTES)
    {	
        return SetFileAttributesW(lpFileName,Attributes - 
			                      (Attributes & ~FILE_ATTRIBUTE_READONLY));
    }
 
    return FALSE;
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

    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Session Manager");

    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING nameW, source_name, dest_name;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HANDLE Reboot = NULL;
    DWORD len1, len2;
    DWORD DestLen = 0;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;
    NTSTATUS Status;

    TRACE("add_boot_rename_entry( %S, %S, %d ) \n", source, dest, flags);

    if(dest)
        DestLen = wcslen(dest);

    if (!RtlDosPathNameToNtPathName_U( source, &source_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    dest_name.Buffer = NULL;
    if (DestLen && !RtlDosPathNameToNtPathName_U( dest, &dest_name, NULL, NULL ))
    {
        RtlFreeHeap( RtlGetProcessHeap(), 0, source_name.Buffer );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

     Status = NtCreateKey(&Reboot, 
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          &ObjectAttributes,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          NULL);

     if (Status == STATUS_ACCESS_DENIED)
     {
         Status = NtCreateKey(
             &Reboot, 
             KEY_QUERY_VALUE | KEY_SET_VALUE,
             &ObjectAttributes,
             0,
             NULL,
             REG_OPTION_BACKUP_RESTORE,
             NULL);
     }

    if (!NT_SUCCESS(Status))
    {
        WARN("NtCreateKey() failed (Status 0x%lx)\n", Status);
        if (source_name.Buffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, source_name.Buffer);
        if (dest_name.Buffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, dest_name.Buffer);
        return FALSE;
    }

    len1 = source_name.Length + sizeof(WCHAR);
    if (DestLen)
    {
        len2 = dest_name.Length + sizeof(WCHAR);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            len2 += sizeof(WCHAR); /* Plus 1 because of the leading '!' */
    }
    else
    {
        len2 = sizeof(WCHAR); /* minimum is the 0 characters for the empty second string */
    }

    RtlInitUnicodeString( &nameW, ValueName );

    /* First we check if the key exists and if so how many bytes it already contains. */
    Status = NtQueryValueKey(
        Reboot,
        &nameW,
        KeyValuePartialInformation,
        NULL,
        0, 
        &DataSize );
    if ((Status == STATUS_BUFFER_OVERFLOW) ||
        (Status == STATUS_BUFFER_TOO_SMALL))
    {
        if (!(Buffer = HeapAlloc(GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR))))
            goto Quit;
        Status = NtQueryValueKey(Reboot, &nameW, KeyValuePartialInformation,
            Buffer, DataSize, &DataSize);
        if(!NT_SUCCESS(Status))
            goto Quit;
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
    if (DestLen)
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

    rc = NT_SUCCESS(NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size));

 Quit:
    RtlFreeHeap(RtlGetProcessHeap(), 0, source_name.Buffer);
    if (dest_name.Buffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, dest_name.Buffer);
    NtClose(Reboot);
    if(Buffer)
        HeapFree(GetProcessHeap(), 0, Buffer);
    return(rc);
}


/*
 * @implemented
 */
BOOL
WINAPI
MoveFileWithProgressW (
	LPCWSTR			lpExistingFileName,
	LPCWSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	HANDLE hFile = NULL, hNewFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
	PFILE_RENAME_INFORMATION FileRename;
	NTSTATUS errCode;
	BOOL Result;
	UNICODE_STRING DstPathU;
	BOOL folder = FALSE;

	TRACE("MoveFileWithProgressW()\n");

	if (dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT)
		return add_boot_rename_entry( lpExistingFileName, lpNewFileName, dwFlags );

    if (dwFlags & MOVEFILE_WRITE_THROUGH)
        FIXME("MOVEFILE_WRITE_THROUGH unimplemented\n");

    if (!lpNewFileName)
        return DeleteFileW(lpExistingFileName);

    /* validate & translate the filename */
    if (!RtlDosPathNameToNtPathName_U (lpNewFileName,
				           &DstPathU,
				           NULL,
				           NULL))
    {
        WARN("Invalid destination path\n");
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DstPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    errCode = NtOpenFile(&hNewFile, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes, &IoStatusBlock, 0,
                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );

	if (NT_SUCCESS(errCode)) /* Destination exists */
	{
        NtClose(hNewFile);

        if (!(dwFlags & MOVEFILE_REPLACE_EXISTING))
        {
			SetLastError(ERROR_ALREADY_EXISTS);
			return FALSE;
		}
		else if (GetFileAttributesW(lpNewFileName) & FILE_ATTRIBUTE_DIRECTORY)
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
	}

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

	FileRename = RtlAllocateHeap(
		RtlGetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length);
	if( !FileRename ) {
		CloseHandle(hFile);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	if( dwFlags & MOVEFILE_REPLACE_EXISTING ) {
		FileRename->ReplaceIfExists = TRUE;
	}
	else {
		FileRename->ReplaceIfExists = FALSE;
	}


	memcpy(FileRename->FileName, DstPathU.Buffer, DstPathU.Length);
        RtlFreeHeap (RtlGetProcessHeap (),
		     0,
		     DstPathU.Buffer);

	FileRename->FileNameLength = DstPathU.Length;
	errCode = NtSetInformationFile (hFile,
	                                &IoStatusBlock,
	                                FileRename,
	                                sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length,
	                                FileRenameInformation);
	CloseHandle(hFile);
	RtlFreeHeap(RtlGetProcessHeap(), 0, FileRename);

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
		                      (dwFlags & MOVEFILE_REPLACE_EXISTING) ? 0 : COPY_FILE_FAIL_IF_EXISTS);
 		    if (Result)
		    {
			/* Cleanup the source file */			
	                Result = DeleteFileW (lpExistingFileName);
		    }
                  }
		 else
		 {
		   /* move folder code start */
		   WIN32_FIND_DATAW findBuffer;
		   LPWSTR lpExistingFileName2 = NULL;
		   LPWSTR lpNewFileName2 = NULL; 
		   LPWSTR lpDeleteFile = NULL;
		   INT size;
		   INT size2;
		   BOOL loop = TRUE;
		   BOOL Result = FALSE;
		   INT max_size = MAX_PATH;


		   		   /* Build the string */
		   size = wcslen(lpExistingFileName); 
		   if (size+6> max_size)
			   max_size = size + 6;

		   lpDeleteFile = (LPWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,max_size * sizeof(WCHAR));
		   if (lpDeleteFile == NULL)		   
		       return FALSE;		  		  

		   lpNewFileName2 = (LPWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,max_size * sizeof(WCHAR));
		   if (lpNewFileName2 == NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *)  lpDeleteFile);
			 return FALSE;
		   }

		   lpExistingFileName2 = (LPWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,max_size * sizeof(WCHAR));
		   if (lpNewFileName2 == NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);		  	  		    		
		     HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		     return FALSE;
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
		

		    /* FIXME 
			 * remove readonly flag from source folder and do not set the readonly flag to dest folder 
			 */
		   RemoveReadOnlyAttributeW(lpExistingFileName);
           RemoveReadOnlyAttributeW(lpNewFileName);
		   //CreateDirectoryExW(lpExistingFileName,lpNewFileName,NULL);
		   CreateDirectoryW(lpNewFileName, NULL);
		  		   
		   /* search the files/folders and move them */
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
		         wcscpy( &lpExistingFileName2[size],L"\0");

		         if (wcsncmp(lpExistingFileName,lpExistingFileName2,size))
		         {  
				   DWORD Attributes;

		           FindClose(hFile);			     

				   /* delete folder */					  				 
				   TRACE("MoveFileWithProgressW : Delete folder : %S\n",lpDeleteFile);

				   /* remove system folder flag other wise we can not delete the folder */
				   Attributes = GetFileAttributesW(lpExistingFileName2);
                   if (Attributes != INVALID_FILE_ATTRIBUTES)
                   {	
                     SetFileAttributesW(lpExistingFileName2,(Attributes & ~FILE_ATTRIBUTE_SYSTEM));
				   }
				   
				   RemoveReadOnlyAttributeW(lpExistingFileName2);
					 				   
				   Result = RemoveDirectoryW(lpExistingFileName2);
				   if (Result == FALSE)
				       break;
				 				 
				   loop=TRUE;				 				 
				   size = wcslen(lpExistingFileName); 
				
				   if (size+6>max_size)
				   {				       
		              if (lpNewFileName2 != NULL)		          		
		                  HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		              if (lpExistingFileName2 != NULL)		          	  
		                  HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		              if (lpDeleteFile != NULL)		   		
		                  HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				      return FALSE;
				   }

				   wcscpy( lpExistingFileName2,lpExistingFileName);
				   wcscpy( &lpExistingFileName2[size],L"\\*.*\0");

				   /* Get the file name */
				   memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
				   hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);	                 
		         }
               } 		  
               continue;		  		  
             }
	  	  	  
		     if (findBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		     {	               

               /* Build the new src string */		  		  		  
               size = wcslen(findBuffer.cFileName); 
               size2= wcslen(lpExistingFileName2);

               if (size2+size+6>max_size)
               {
	              FindClose(hFile);    		 

		         if (lpNewFileName2 != NULL)		          		
		              HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		         if (lpExistingFileName2 != NULL)		          	  
		              HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		         if (lpDeleteFile != NULL)		   		
		              HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				 return FALSE;			      				 
	           }

	            wcscpy( &lpExistingFileName2[size2-3],findBuffer.cFileName);			   			  
                wcscpy( &lpExistingFileName2[size2+size-3],L"\0");
			   

			   /* Continue */
			   wcscpy( lpDeleteFile,lpExistingFileName2);
	           wcscpy( &lpExistingFileName2[size2+size-3],L"\\*.*\0");
          
		  
	           /* Build the new dst string */
	           size = wcslen(lpExistingFileName2) + wcslen(lpNewFileName);
	           size2 = wcslen(lpExistingFileName);
		  
	           if (size>max_size)
	           {
	             FindClose(hFile);    		 

		         if (lpNewFileName2 != NULL)		          		
		              HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		         if (lpExistingFileName2 != NULL)		          	  
		              HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		         if (lpDeleteFile != NULL)		   		
		              HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				 return FALSE;			     
	           }

	           wcscpy( lpNewFileName2,lpNewFileName);		 		  
	           size = wcslen(lpNewFileName);		 
	           wcscpy( &lpNewFileName2[size], &lpExistingFileName2[size2]);
	           size = wcslen(lpNewFileName2);
	           wcscpy( &lpNewFileName2[size-4],L"\0");
	           
	           /* Create Folder */	          

			   /* FIXME 
			    * remove readonly flag from source folder and do not set the readonly flag to dest folder 
				*/
			   RemoveReadOnlyAttributeW(lpDeleteFile);
			   RemoveReadOnlyAttributeW(lpNewFileName2);

			   CreateDirectoryW(lpNewFileName2,NULL);
	           //CreateDirectoryExW(lpDeleteFile, lpNewFileName2,NULL);
			   

			   /* set new search path  from src string */
	           FindClose(hFile);
	           memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
	           hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);
	         }
		     else
		     {
		  
	           /* Build the new string */		  		  		  		  
	           size = wcslen(findBuffer.cFileName); 
	           size2= wcslen(lpExistingFileName2);
	           wcscpy( lpDeleteFile,lpExistingFileName2);	   
	           wcscpy( &lpDeleteFile[size2-3],findBuffer.cFileName);	   
		  
	           /* Build dest string */
	           size = wcslen(lpDeleteFile) + wcslen(lpNewFileName);
	           size2 = wcslen(lpExistingFileName);

	           if (size>max_size)
	           {                 				  
			      FindClose(hFile);    		 

		          if (lpNewFileName2 != NULL)		          		
		              HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		          if (lpExistingFileName2 != NULL)		          	  
		              HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		          if (lpDeleteFile != NULL)		   		
		              HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				  return FALSE;
			   }
		  
               wcscpy( lpNewFileName2,lpNewFileName);		 		  
               size = wcslen(lpNewFileName);		 
               wcscpy(&lpNewFileName2[size],&lpDeleteFile[size2]);
		 
              
			   /* overrite existsen file, if the file got the flag have readonly 
			    * we need reomve that flag 
				*/
			   
			    /* copy file */
			   
			   TRACE("MoveFileWithProgressW : Copy file : %S to %S\n",lpDeleteFile, lpNewFileName2);
			   RemoveReadOnlyAttributeW(lpDeleteFile);
			   RemoveReadOnlyAttributeW(lpNewFileName2);
			  
			   Result = CopyFileExW (lpDeleteFile,
		                      lpNewFileName2,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      0);

			   if (Result == FALSE)                                
                   break;
              
               /* delete file */               		            
			   TRACE("MoveFileWithProgressW : remove readonly flag from file : %S\n",lpNewFileName2);
			   Result = RemoveReadOnlyAttributeW(lpDeleteFile);
			   if (Result == FALSE)
			       break;

               TRACE("MoveFileWithProgressW : Delete file : %S\n",lpDeleteFile);
			   Result = DeleteFileW(lpDeleteFile);
               if (Result == FALSE)                               
                   break;
              
             }	  	              
             loop = FindNextFileW(hFile, &findBuffer);	  	  
           }

           
		   /* Remove last folder */
           if ((loop == FALSE) && (Result != FALSE))
		   {
		     DWORD Attributes;

			 Attributes = GetFileAttributesW(lpDeleteFile);
             if (Attributes != INVALID_FILE_ATTRIBUTES)
             {	
                SetFileAttributesW(lpDeleteFile,(Attributes & ~FILE_ATTRIBUTE_SYSTEM));
			 }
					 				   				 
		     Result = RemoveDirectoryW(lpExistingFileName);		     
		   }
           
		   /* Cleanup */
		   FindClose(hFile);    
		   
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
	
	
	return Result;
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
