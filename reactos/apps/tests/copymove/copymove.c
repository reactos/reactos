/*
 * CopyFile, MoveFile and related routines test
 */

#include <ctype.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

static TCHAR
FindOtherDrive()
{
	DWORD drives = GetLogicalDrives();
	BOOL found = FALSE;
	TCHAR drive;
	TCHAR rootdir[] = _T( "?:\\" );
	TCHAR currentdir[MAX_PATH + 1];

	if (0 != GetCurrentDirectory(MAX_PATH + 1, currentdir)) {
		for (drive = _T('A'); ! found && drive <= _T('Z'); drive++) {
			if (0 != (drives & (1 << (drive - _T('A'))))&&
			    drive != _totupper(currentdir[0])) {
				rootdir[0] = drive;
				found = (DRIVE_FIXED == GetDriveType(rootdir));
			}
		}
	}

	return found ? drive - 1 : _T( ' ' );
}

static void
DeleteTestFile(LPCTSTR filename)
{
	SetFileAttributes(filename, FILE_ATTRIBUTE_NORMAL);
	DeleteFile(filename);
}

static void
CreateTestFile(LPCTSTR filename, DWORD attributes)
{
	HANDLE file;
	char buffer[4096];
	DWORD wrote;
	int c;

	DeleteTestFile(filename);   
	file = CreateFile(filename, 
	                  GENERIC_READ | GENERIC_WRITE, 
	                  0, 
	                  NULL, 
	                  CREATE_ALWAYS, 
	                  0, 
	                  0);
   
	if (INVALID_HANDLE_VALUE == file) {
		fprintf(stderr, "CreateFile failed with code %d\n", GetLastError());
		exit(1);
	}
	for(c = 0; c < sizeof(buffer); c++) {
		buffer[c] = (char) c;
	}
	if (! WriteFile(file, buffer, sizeof(buffer), &wrote, NULL)) {
		fprintf(stderr, "WriteFile failed with code %d\n", GetLastError());
		exit(1);
	}
	CloseHandle(file);

	if (! SetFileAttributes(filename, attributes)) {
		fprintf(stderr, "SetFileAttributes failed with code %d\n", GetLastError());
		exit(1);
	}
}

static void
DeleteTestDir(LPCTSTR dirname)
{
	RemoveDirectory(dirname);
}

static void
CreateTestDir(LPCTSTR dirname)
{
	if (! CreateDirectory(dirname, NULL)) {
		fprintf(stderr, "CreateDirectory failed with code %d\n", GetLastError());
		exit(1);
	}
}

static void
CheckTestFile(LPCTSTR filename, DWORD attributes)
{
	HANDLE file;
	char buffer[4096];
	DWORD read;
	int c;
	DWORD diskattr;

	file = CreateFile(filename,
	                  GENERIC_READ,
	                  0,
	                  NULL,
	                  OPEN_EXISTING,
	                  0,
	                  0);

	if (INVALID_HANDLE_VALUE == file) {
		fprintf(stderr, "CreateFile failed with code %d\n", GetLastError());
		exit(1);
	}

	if (! ReadFile(file, buffer, sizeof(buffer), &read, NULL)) {
		fprintf(stderr, "ReadFile failed with code %d\n", GetLastError());
		exit(1);
	}
	if (read != sizeof(buffer)) {
		fprintf(stderr, "Trying to read %d bytes but got %d bytes\n", sizeof(buffer), read);
		exit(1);
	}
	for(c = 0; c < sizeof(buffer); c++) {
		if (buffer[c] != (char) c) {
			fprintf(stderr, "File contents changed at position %d\n", c);
			exit(1);
		}
	}

	CloseHandle(file);
	
	diskattr = GetFileAttributes(filename);
	if (INVALID_FILE_ATTRIBUTES == diskattr) {
		fprintf(stderr, "GetFileAttributes failed with code %d\n", GetLastError());
		exit(1);
	}
	if (diskattr != attributes) {
		fprintf(stderr, "Attribute mismatch, expected 0x%08x found 0x%08x\n", attributes, diskattr);
		exit(1);
	}
}

int
main(int argc, char *argv[])
{
	TCHAR otherdrive;
	TCHAR otherfile[ ] = _T("?:\\other.dat");

	otherdrive = FindOtherDrive();

	printf("Testing simple move\n");
	CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
	DeleteTestFile(_T("end.dat"));
	if (! MoveFile(_T("begin.dat"), _T("end.dat"))) {
		fprintf(stderr, "MoveFile failed with code %d\n", GetLastError());
		exit(1);
	}
	CheckTestFile(_T("end.dat"), FILE_ATTRIBUTE_ARCHIVE);
	DeleteTestFile(_T("end.dat"));

	printf("Testing move of non-existing file\n");
	DeleteTestFile(_T("begin.dat"));
	DeleteTestFile(_T("end.dat"));
	if (MoveFile(_T("begin.dat"), _T("end.dat"))) {
		fprintf(stderr, "MoveFile succeeded but shouldn't have\n");
		exit(1);
	} else if (ERROR_FILE_NOT_FOUND != GetLastError()) {
		fprintf(stderr, "MoveFile failed with unexpected code %d\n", GetLastError());
		exit(1);
	}
	DeleteTestFile(_T("end.dat"));

/* Not correctly implemented in ros, destination file is kept open after this */
#if 0
	printf("Testing move to existing file\n");
	CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
	CreateTestFile(_T("end.dat"), FILE_ATTRIBUTE_ARCHIVE);
	if (MoveFile(_T("begin.dat"), _T("end.dat"))) {
		fprintf(stderr, "MoveFile succeeded but shouldn't have\n");
		exit(1);
	} else if (ERROR_ALREADY_EXISTS != GetLastError()) {
		fprintf(stderr, "MoveFile failed with unexpected code %d\n", GetLastError());
		exit(1);
	}
	DeleteTestFile(_T("begin.dat"));
	DeleteTestFile(_T("end.dat"));
#endif

/* Not implemented yet in ros */
#if 0
	printf("Testing directory move\n");
	CreateTestDir(_T("begin"));
	CreateTestFile(_T("begin\\file.dat"), FILE_ATTRIBUTE_NORMAL);
	DeleteTestDir(_T("end"));
	if (! MoveFile(_T("begin"), _T("end"))) {
		fprintf(stderr, "MoveFile failed with code %d\n", GetLastError());
		exit(1);
	}
	CheckTestFile(_T("end\\file.dat"), FILE_ATTRIBUTE_NORMAL);
	DeleteTestFile(_T("end\\file.dat"));
	DeleteTestDir(_T("end"));
#endif

	printf("Testing file move to different directory\n");
	CreateTestFile(_T("file.dat"), FILE_ATTRIBUTE_NORMAL);
	CreateTestDir(_T("end"));
	if (! MoveFile(_T("file.dat"), _T("end\\file.dat"))) {
		fprintf(stderr, "MoveFile failed with code %d\n", GetLastError());
		exit(1);
	}
	CheckTestFile(_T("end\\file.dat"), FILE_ATTRIBUTE_ARCHIVE);
	DeleteTestFile(_T("end\\file.dat"));
	DeleteTestDir(_T("end"));

	printf("Testing move of read-only file\n");
	CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_READONLY);
	DeleteTestFile(_T("end.dat"));
	if (! MoveFile(_T("begin.dat"), _T("end.dat"))) {
		fprintf(stderr, "MoveFile failed with code %d\n", GetLastError());
		exit(1);
	}
	CheckTestFile(_T("end.dat"), FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY);
	DeleteTestFile(_T("end.dat"));

	printf("Testing move to different drive\n");
	if (_T(' ') != otherdrive) {
		otherfile[0] = otherdrive;
		CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
		DeleteTestFile(otherfile);
		if (! MoveFile(_T("begin.dat"), otherfile)) {
			fprintf(stderr, "MoveFile failed with code %d\n", GetLastError());
			exit(1);
		}
		CheckTestFile(otherfile, FILE_ATTRIBUTE_ARCHIVE);
		DeleteTestFile(otherfile);
	} else {
		printf("  Test skipped, no other drive available\n");
	}

	printf("Testing move/overwrite of existing file\n");
	CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
	CreateTestFile(_T("end.dat"), FILE_ATTRIBUTE_ARCHIVE);
	if (! MoveFileEx(_T("begin.dat"), _T("end.dat"), MOVEFILE_REPLACE_EXISTING)) {
		fprintf(stderr, "MoveFileEx failed with code %d\n", GetLastError());
		exit(1);
	}
	DeleteTestFile(_T("begin.dat"));
	DeleteTestFile(_T("end.dat"));

/* Not (correctly) implemented in ros yet */
#if 0
	printf("Testing move/overwrite of existing readonly file\n");
	CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
	CreateTestFile(_T("end.dat"), FILE_ATTRIBUTE_READONLY);
	if (MoveFileEx(_T("begin.dat"), _T("end.dat"), MOVEFILE_REPLACE_EXISTING)) {
		fprintf(stderr, "MoveFileEx succeeded but shouldn't have\n");
		exit(1);
	} else if (ERROR_ALREADY_EXISTS != GetLastError() &&
	           ERROR_ACCESS_DENIED != GetLastError()) {
		fprintf(stderr, "MoveFileEx failed with unexpected code %d\n", GetLastError());
		exit(1);
	}
	DeleteTestFile(_T("begin.dat"));
	DeleteTestFile(_T("end.dat"));
#endif

/* Not implemented in ros yet */
#if 0
	printf("Testing move to different drive without COPY_ALLOWED\n");
	if (_T(' ') != otherdrive) {
		otherfile[0] = otherdrive;
		CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
		DeleteTestFile(otherfile);
		if (MoveFileEx(_T("begin.dat"), otherfile, 0)) {
			fprintf(stderr, "MoveFileEx succeeded but shouldn't have\n");
			exit(1);
		} else if (ERROR_NOT_SAME_DEVICE != GetLastError()) {
			fprintf(stderr, "MoveFileEx failed with unexpected code %d\n", GetLastError());
			exit(1);
		}
		DeleteTestFile(otherfile);
	} else {
		printf("  Test skipped, no other drive available\n");
	}
#endif

	printf("Testing move to different drive with COPY_ALLOWED\n");
	if (_T(' ') != otherdrive) {
		otherfile[0] = otherdrive;
		CreateTestFile(_T("begin.dat"), FILE_ATTRIBUTE_ARCHIVE);
		DeleteTestFile(otherfile);
		if (! MoveFileEx(_T("begin.dat"), otherfile, MOVEFILE_COPY_ALLOWED)) {
			fprintf(stderr, "MoveFileEx failed with code %d\n", GetLastError());
			exit(1);
		}
		CheckTestFile(otherfile, FILE_ATTRIBUTE_ARCHIVE);
		DeleteTestFile(otherfile);
	} else {
		printf("  Test skipped, no other drive available\n");
	}

	printf("All tests successfully completed\n");

	return 0;
}
