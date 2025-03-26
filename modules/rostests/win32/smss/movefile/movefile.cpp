/*
 * PROJECT:         ReactOS Test applications
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/testsets/smss/movefile.cpp
 * PURPOSE:         Provides testing for the "move file after reboot"
 *                  function of smss.exe/kernel32.dll
 * PROGRAMMERS:     Dmitriy Philippov (shedon@mail.ru)
 */


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include "windows.h"
#include <stdio.h>
#include <tchar.h>
#include "stdlib.h"
#include "string.h"


void Usage()
{
	printf(" Usage: smssTest.exe -g|c|s|d\n\
			g - generate test files\n\
			c - check files after reboot\n\
			s - show registry entry\n\
			d - delete registry value\n");
}

int ShowRegValue()
{
	BYTE lpBuff[255];
	memset(lpBuff, 0, sizeof(lpBuff));

	DWORD lSize = sizeof(lpBuff);
	HKEY hKey;
	LONG retValue;
	// test registry entry
	retValue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0, KEY_QUERY_VALUE, &hKey);
	if( ERROR_SUCCESS != retValue ) {
		printf("RegOpenKeyEx err=%ld \n", retValue);
		return 1;
	}

	retValue = RegQueryValueEx(hKey, "PendingFileRenameOperations", NULL, NULL, lpBuff, &lSize);
	if( ERROR_SUCCESS != retValue ) {
		printf("RegQueryValueEx err=%ld \n", retValue);
		lSize = 0;
	}

	printf("reg data: \n");
	for(UINT i=0; i<lSize; i++) {
		printf("%c", lpBuff[i]);
	}
	printf("\n");

	RegCloseKey(hKey);

	return 0;
}

int DeleteValue()
{
	HKEY hKey;
	LONG retValue;
	// test registry entry
	retValue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0, KEY_SET_VALUE, &hKey);
	if( ERROR_SUCCESS != retValue ) {
		printf("RegOpenKeyEx err=%ld \n", retValue);
		return 1;
	}

	retValue = RegDeleteValue(hKey, "PendingFileRenameOperations");
	if( ERROR_SUCCESS != retValue ) {
		printf("RegDeleteValue err=%ld \n", retValue);
	}

	RegCloseKey(hKey);

	return 0;
}

int Generate()
{
	char sBuf[255];
	DWORD dwSize;
	HANDLE hFile = NULL;
	BOOL fReturnValue;

	const char szxReplacedFile[] = "c:\\testFileIsReplaced";
	const char szxMovedFileWithRepl[] = "c:\\testFileShouldBeMovedW";
	const char szxMovedFile[] = "c:\\testFileShouldBeMoved";
	const char szxNewMovedFile[] = "c:\\testFileIsMoved";
	const char szxDeletedFile[] = "c:\\testFileShouldBeDeleted";

	memset(sBuf, 0xaa, sizeof(sBuf));

	// create the first file for moving
	hFile = CreateFile(
		szxMovedFile,
		FILE_ALL_ACCESS,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(NULL == hFile) {
		printf("Can't create the %s file, err=%ld \n", szxMovedFile, GetLastError());
		return 1;
	}
	WriteFile(hFile, sBuf, sizeof(sBuf), &dwSize, NULL);
	CloseHandle(hFile);

	// create the second file for removing
	hFile = CreateFile(
		szxDeletedFile,
		FILE_ALL_ACCESS,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(NULL == hFile) {
		printf("Can't create the %s file, err=%ld \n", szxDeletedFile, GetLastError());
		return 1;
	}
	WriteFile(hFile, sBuf, sizeof(sBuf), &dwSize, NULL);
	CloseHandle(hFile);

	hFile = CreateFile(
		szxReplacedFile,
		FILE_ALL_ACCESS,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(NULL == hFile) {
		printf("Can't create the %s file, err=%ld \n", szxReplacedFile, GetLastError());
		return 1;
	}
	WriteFile(hFile, sBuf, sizeof(sBuf), &dwSize, NULL);
	CloseHandle(hFile);


	hFile = CreateFile(
		szxMovedFileWithRepl,
		FILE_ALL_ACCESS,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(NULL == hFile) {
		printf("Can't create the %s file, err=%ld \n", szxMovedFileWithRepl, GetLastError());
		return 1;
	}
	WriteFile(hFile, sBuf, sizeof(sBuf), &dwSize, NULL);
	CloseHandle(hFile);


	fReturnValue = MoveFileEx(
		szxDeletedFile,
		NULL,
		MOVEFILE_DELAY_UNTIL_REBOOT);
	if( !fReturnValue ) {
		printf("Can't move the %s file, err=%ld \n", szxDeletedFile, GetLastError());
		return 1;
	}

	ShowRegValue();

	fReturnValue = MoveFileEx(
		szxMovedFile,
		szxNewMovedFile,
		MOVEFILE_DELAY_UNTIL_REBOOT);
	if( !fReturnValue ) {
		printf("Can't move the %s file, err=%ld \n", szxMovedFile, GetLastError());
		return 1;
	}

	ShowRegValue();

	fReturnValue = MoveFileEx(
		szxMovedFileWithRepl,
		szxReplacedFile,
		MOVEFILE_DELAY_UNTIL_REBOOT|MOVEFILE_REPLACE_EXISTING);
	if( !fReturnValue ) {
		printf("Can't move the %s file, err=%ld \n", szxMovedFileWithRepl, GetLastError());
		return 1;
	}

	ShowRegValue();

	return 0;
}

int Check()
{
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if( argc<2 ) {
		Usage();
		return 1;
	}

	if( 0 == strncmp(argv[1], "-g", 2)  )
	{
		// generate test files and registry values
		return Generate();
	}
	else if( 0 == strncmp(argv[1], "-c", 2) )
	{
		// check generated files
		return Check();
	}
	else if( 0 == strncmp(argv[1], "-s", 2) )
	{
		//
		return ShowRegValue();
	}
	else if( 0 == strncmp(argv[1], "-d", 2) )
	{
		return DeleteValue();
	}
	else
	{
		Usage();
		return 1;
	}

	return 0;
}

