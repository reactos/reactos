/* $Id: directory.cpp,v 1.1 2002/09/04 22:19:47 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * PART:			 doscalls.dll
 * FILE:             directory.cpp
 * PURPOSE:          Kernelservices for OS/2 apps
 * CONTAINS:		 Directory related CP-functions.
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *  10-11-2002  Created
 */


#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include "ros2.h"


 
 
/******************************************
 DosDelete removes a file name from a    
 directory. The deleted file may be      
 recoverable.                            

 pszFile (PSZ) - input 
    Address of the name of the file to be deleted. 
 
 ulrc (APIRET) - returns 
    Return Code. 

  DosDelete returns one of the following values: 
      0         NO_ERROR 
      2         ERROR_FILE_NOT_FOUND 
      3         ERROR_PATH_NOT_FOUND 
      5         ERROR_ACCESS_DENIED 
      26        ERROR_NOT_DOS_DISK 
      32        ERROR_SHARING_VIOLATION 
      36        ERROR_SHARING_BUFFER_EXCEEDED 
      87        ERROR_INVALID_PARAMETER 
      206       ERROR_FILENAME_EXCED_RANGE 
*******************************************/
APIRET DosDelete(PSZ pszFile)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/******************************************
	DosDeleteDir removes a subdirectory     
	from the specified disk.    
	
 pszDir (PSZ) - input 
    Address of the fully qualified path name of the subdirectory to be removed. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosDeleteDir returns one of the following values: 

      0         NO_ERROR 
      2         ERROR_FILE_NOT_FOUND 
      3         ERROR_PATH_NOT_FOUND 
      5         ERROR_ACCESS_DENIED 
      16        ERROR_CURRENT_DIRECTORY 
      26        ERROR_NOT_DOS_DISK 
      87        ERROR_INVALID_PARAMETER 
      108       ERROR_DRIVE_LOCKED 
      206       ERROR_FILENAME_EXCED_RANGE 
******************************************/
APIRET DosDeleteDir(PSZ pszDir)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}




/*******************************************
 DosCopy copies the source file or       
 subdirectory to the destination file or 
 subdirectory.                           

 pszOld (PSZ) - input 
    Address of the ASCIIZ path name of the source file, 
    subdirectory, or character device. 

    Global file-name characters are not allowed. 
 
 pszNew (PSZ) - input 
    Address of the ASCIIZ path name of the target file, 
    subdirectory, or character device. 

    Global file-name characters are not allowed. 
 
 option (ULONG) - input 
    ULONG bit flags that define how the DosCopy 
    function is done. 

      
      Bit       Description 

      31-3      Reserved. These bits must be set to 
                zero. 

      2         DCPY_FAILEAS (0x00000004) 
                Discard the EAs if the source file 
                contains EAs and the destination file 
                system does not support EAs. 

           0   Discard the EAs (extended attributes) if 
               the destination file system does not 
               support EAs. 

           1   Fail the copy if the destination file 
               system does not support EAs. 

      1         DCPY_APPEND (x00000002) 
                Append the source file to the target 
                file's end of data. 

           0   Replace the target file with the source 
               file. 
           1   Append the source file to the target file's 
               end of data. 

                This is ignored when copying a 
                directory, or if the target file does not 
                exist. 

      0         DCPY_EXISTING (0x00000001) 
                Existing Target File Disposition. 

           0   Do not copy the source file to the target 
               if the file name already exists within the 
               target directory. If a single file is being 
               copied and the target already exists, an 
               error is returned. 

           1   Copy the source file to the target even if 
               the file name already exists within the 
               target directory. 

    Bit flag DCPY_FAILEAS can be used in 
    combination with bit flag DCPY_APPEND or 
    DCPY_EXISTING. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosCopy returns one of the following values: 

      0         NO_ERROR 
      2         ERROR_FILE_NOT_FOUND 
      3         ERROR_PATH_NOT_FOUND 
      5         ERROR_ACCESS_DENIED 
      26        ERROR_NOT_DOS_DISK 
      32        ERROR_SHARING_VIOLATION 
      36        ERROR_SHARING_BUFFER_EXCEEDED 
      87        ERROR_INVALID_PARAMETER 
      108       ERROR_DRIVE_LOCKED 
      112       ERROR_DISK_FULL 
      206       ERROR_FILENAME_EXCED_RANGE 
      267       ERROR_DIRECTORY 
      282       ERROR_EAS_NOT_SUPPORTED 
      283       ERROR_NEED_EAS_FOUND 
*******************************************/
APIRET DosCopy(PSZ pszOld,PSZ pszNew, ULONG option)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}
 



/* EOF */
