/* $Id: testfsd.c,v 1.4 2001/07/04 03:07:54 rex Exp $
 *
 * FILE:          testFSD.c
 * PURPOSE:       A test set for the File System Driver
 * PROJECT:       ReactOS kernel
 * COPYRIGHT:     See COPYING in the top level directory
 * PROGRAMMER:    Rex Jolliff (rex@lvcablemodem.com)
 *
 */

#include <errno.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "testsuite.h"

const char *rootDir = "c:\\";
const char *systemRootDir = "c:\\ReactOS";
const char *systemDllDir = "c:\\ReactOS\\System32";
const char *bogusRootFile = "c:\\bogus";
const char *bogusDirAndFile = "c:\\bogus\\bogus";
const char *bogusSystemRootFile = "c:\\ReactOS\\bogus";
const char *bogusSystemDllFile = "c:\\ReactOS\\System32\\bogus";
const char *shortFile = "c:\\ReactOS\\boot.bat";
const char *longFile = "c:\\ReactOS\\loadros.com";

void  testOpenExistant (void)
{
  HANDLE  fileHandle;

  fileHandle  = CreateFile (rootDir, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (systemRootDir, GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (systemDllDir, GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
}

void  testOpenNonExistant (void)
{
  DWORD  status;
  HANDLE  fileHandle;

  fileHandle  = CreateFile (bogusRootFile, GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_FILE_NOT_FOUND);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (bogusDirAndFile, GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_PATH_NOT_FOUND);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (bogusSystemRootFile, GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_FILE_NOT_FOUND);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (bogusSystemDllFile, GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_FILE_NOT_FOUND);
  CloseHandle (fileHandle);
}

void  testCreateExistant (void)
{
  DWORD  status;
  HANDLE  fileHandle;

  fileHandle  = CreateFile (rootDir, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_ALREADY_EXISTS);
  fileHandle  = CreateFile (systemRootDir, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_ALREADY_EXISTS);
  fileHandle  = CreateFile (systemDllDir, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_ALREADY_EXISTS);
}

void  testCreateNonExistant (void)
{
  DWORD  status;
  HANDLE  fileHandle;

  fileHandle  = CreateFile (bogusRootFile, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (bogusSystemRootFile, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (bogusSystemDllFile, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
#if 0
  fileHandle  = CreateFile (bogusDirAndFile, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_PATH_NOT_FOUND);
  CloseHandle (fileHandle);
#endif
}

void testDeleteFiles (void)
{
  BOOL  returnValue;
  HANDLE  fileHandle;

  fileHandle  = CreateFile ("bogus", GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  returnValue = DeleteFile ("bogus");
  ASSERT_MSG (returnValue, 
              "Delete of bogus failed, error:%d", 
              GetLastError ());

#if 0
  returnValue = DeleteFile (bogusRootFile);
  ASSERT_MSG (returnValue, 
              "Delete of %s failed, error:%d", 
              bogusRootFile, 
              GetLastError ());
  returnValue = DeleteFile (bogusRootFile);
  ASSERT_MSG (!returnValue, 
              "2nd Delete of %s succeeded but should fail",
              bogusRootFile);
  returnValue = DeleteFile (bogusSystemRootFile);
  ASSERT_MSG (returnValue, 
              "Delete of %s failed, error:%d", 
              bogusSystemRootFile,
              GetLastError ());
  returnValue = DeleteFile (bogusSystemRootFile);
  ASSERT_MSG (!returnValue, 
              "2nd Delete of %s succeeded but should fail",
              bogusSystemRootFile);
  returnValue = DeleteFile (bogusSystemDllFile);
  ASSERT_MSG (returnValue, 
              "Delete of %s failed, error:%d", 
              bogusSystemDllFile,
              GetLastError ());
  returnValue = DeleteFile (bogusSystemDllFile);
  ASSERT_MSG (!returnValue, 
              "2nd Delete of %s succeeded but should fail",
              bogusSystemDllFile);
  returnValue = DeleteFile (bogusDirAndFile);
  ASSERT_MSG (!returnValue, 
              "Delete of %s succeded but should fail",
              bogusDirAndFile);
#endif
}

void  testOpenWithBlankPathElements (void)
{
  HANDLE  fileHandle;

  fileHandle = CreateFile ("c:", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle = CreateFile ("c:\\\\", GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle = CreateFile ("c:\\\\reactos\\", GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle = CreateFile ("c:\\reactos\\\\", GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle = CreateFile ("c:\\reactos\\\\system32\\", GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle = CreateFile ("c:\\reactos\\system32\\\\", GENERIC_READ, 0, 0, 
                            OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
}

void testReadOnInvalidHandle (void)
{
  BOOL  returnValue;
  char  buffer [256];
  DWORD  bytesRead;

  returnValue = ReadFile (INVALID_HANDLE_VALUE, 
                          buffer, 
                          256, 
                          &bytesRead, 
                          NULL);
  ASSERT_MSG (!returnValue,
              "Read from invalid handle succeeded but should fail",0);
  ASSERT (GetLastError () != ENOFILE);
}

void testReadOnZeroLengthFile (void)
{
  BOOL  returnValue;
  HANDLE  fileHandle;
  char  buffer [256];
  DWORD  bytesRead;

  fileHandle  = CreateFile (bogusRootFile, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  CloseHandle (fileHandle);
  fileHandle  = CreateFile (bogusRootFile, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  returnValue = ReadFile (fileHandle, buffer, 256, &bytesRead, NULL);
  ASSERT_MSG (!returnValue,
              "Read from zero length file succeeded but should fail",0);
  ASSERT (GetLastError () != ERROR_HANDLE_EOF);
  CloseHandle (fileHandle);
  returnValue = DeleteFile (bogusRootFile);
  ASSERT_MSG (returnValue, 
              "Delete of %s failed, error:%d", 
              bogusRootFile, 
              GetLastError ());
}

void testReadOnShortFile (void)
{
  BOOL  returnValue;
  HANDLE  fileHandle;
  char  buffer [256];
  DWORD  bytesRead;

  fileHandle  = CreateFile (shortFile, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  returnValue = ReadFile (fileHandle, 
                          buffer, 
                          256, 
                          &bytesRead, 
                          NULL);
  ASSERT_MSG (returnValue,
              "Read from short length file failed, error:%d",
              GetLastError ());
  ASSERT (bytesRead > 0 && bytesRead < 256);
  CloseHandle (fileHandle);
}

void testReadOnLongFile (void)
{
  BOOL  returnValue;
  HANDLE  fileHandle;
  char  buffer [256];
  DWORD  bytesRead;
  int  count;

  fileHandle  = CreateFile (longFile, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  ASSERT (fileHandle != INVALID_HANDLE_VALUE);
  for (count = 0; count < 20; count++)
  {
    returnValue = ReadFile (fileHandle, 
                            buffer, 
                            256, 
                            &bytesRead, 
                            NULL); 
    ASSERT_MSG (returnValue,
                "Read from short length file failed, error:%d",
                GetLastError ());
    ASSERT (bytesRead == 256);
  }
  CloseHandle (fileHandle);
}

int main (void)
{
  TEST_RUNNER  testRunner;
  TEST_SUITE  testsToRun [] =
  {
    ADD_TEST (testOpenExistant),
    ADD_TEST (testOpenNonExistant),
    ADD_TEST (testCreateExistant),
    ADD_TEST (testCreateNonExistant),
    ADD_TEST (testDeleteFiles),
/*    ADD_TEST (testOverwriteExistant),*/
/*    ADD_TEST (testOverwriteNonExistant),*/
    ADD_TEST (testOpenWithBlankPathElements),
    ADD_TEST (testReadOnInvalidHandle),
    ADD_TEST (testReadOnZeroLengthFile),
    ADD_TEST (testReadOnShortFile),
    ADD_TEST (testReadOnLongFile),
/*    ADD_TEST (test), */
    END_TESTS
  };

  memset (&testRunner, 0, sizeof (TEST_RUNNER));
  tsRunTests (&testRunner, testsToRun);
  tsReportResults (&testRunner);
}


