/* $Id: testfsd.c,v 1.1 2001/05/01 04:35:08 rex Exp $
 *
 * FILE:          testFSD.c
 * PURPOSE:       A test set for the File System Driver
 * PROJECT:       ReactOS kernel
 * COPYRIGHT:     See COPYING in the top level directory
 * PROGRAMMER:    Rex Jolliff (rex@lvcablemodem.com)
 *
 */

#include <stdio.h>
#include <windows.h>

#define  ASSERT(x) doAssertion (x, #x, __FUNCTION__, __LINE__)

const char *rootDir = "c:\\";
const char *systemRootDir = "c:\\ReactOS";
const char *systemDllDir = "c:\\ReactOS\\System32";
const char *bogusRootFile = "c:\\bogus";
const char *bogusDirAndFile = "c:\\bogus\\bogus";
const char *bogusSystemRootFile = "c:\\ReactOS\\bogus";
const char *bogusSystemDllFile = "c:\\ReactOS\\System32\\bogus";

int  tests, assertions, failures, successes;

void  doAssertion (BOOL pTest, PCHAR pTestText, PCHAR pFunction, int pLine);

void  testOpenExistant (void);
void  testOpenNonExistant (void);
void  testCreateExistant (void);
void  testCreateNonExistant (void);
void  testOverwriteExistant (void);
void  testOverwriteNonExistant (void);

void  testOpenWithBlankPathName (void);
void  testOpenWithBlankDirElement (void);

int main (void)
{
  tests = assertions = failures = successes = 0;

  testOpenExistant ();
  testOpenNonExistant ();
  testCreateExistant ();
  testCreateNonExistant ();
  testOpenWithBlankPathName ();
  testOpenWithBlankDirElement ();
//  testFullObjectPaths ();

  printf ("\nTotals: tests: %d assertions: %d successes: %d failures: %d\n",
          tests,
          assertions,
          successes,
          failures);
  if (failures == 0)
  {
    printf ("\n*** OK ***\n");
  }
  else
  {
    printf ("\n*** FAIL ***\n");
  }
}

void  testOpenExistant (void)
{
  HANDLE  fileHandle;

  tests++;

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

  tests++;

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

  tests++;

printf ("before CreateFile (%s)\n", rootDir);
  fileHandle  = CreateFile (rootDir, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
printf ("after CreateFile (%s)\n", rootDir);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_ALREADY_EXISTS);
printf ("before CreateFile (%s)\n", systemRootDir);
  fileHandle  = CreateFile (systemRootDir, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
printf ("after CreateFile (%s)\n", rootDir);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_ALREADY_EXISTS);
printf ("before CreateFile (%s)\n", systemDllDir);
  fileHandle  = CreateFile (systemDllDir, GENERIC_READ, 0, 0, 
                            CREATE_NEW, 0, 0);
printf ("after CreateFile (%s)\n", rootDir);
  ASSERT (fileHandle == INVALID_HANDLE_VALUE);
  status = GetLastError ();
  ASSERT (status == ERROR_ALREADY_EXISTS);
}

void  testCreateNonExistant (void)
{
  tests++;

}

void  testOpenWithBlankPathName (void)
{
  tests++;

}

void  testOpenWithBlankDirElement (void)
{
  tests++;

}

void  doAssertion (BOOL pTest, PCHAR pTestText, PCHAR pFunction, int pLine)
{
  assertions++;
  if (!pTest) 
  {
    printf ("%s(%d): assertion \"%s\" failed", pFunction, pLine, pTestText); 
    failures++;
  }
  else
  {
    successes++;
  }
}


