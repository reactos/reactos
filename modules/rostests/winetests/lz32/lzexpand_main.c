/*
 * Unit test suite for lz32 functions
 *
 * Copyright 2004 Evan Parry, Daniel Kegel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <stdlib.h>
#include <winerror.h>
#include <lzexpand.h>

#include "wine/test.h"

/* Compressed file names end with underscore. */
static char filename [] = "testfile.xxx";
static char filename_[] = "testfile.xx_";
static WCHAR filenameW [] = L"testfile.xxx";
static WCHAR filenameW_[] = L"testfile.xx_";

static char dotless [] = "dotless";
static char dotless_[] = "dotless._";
static WCHAR dotlessW [] = L"dotless";
static WCHAR dotlessW_[] = L"dotless._";

static char extless [] = "extless.";
static char extless_[] = "extless._";
static WCHAR extlessW [] = L"extless.";
static WCHAR extlessW_[] = L"extless._";

static char _terminated [] = "_terminated.xxxx_";
static char _terminated_[] = "_terminated.xxxx_";
static WCHAR _terminatedW [] = L"_terminated.xxxx_";
static WCHAR _terminatedW_[] = L"_terminated.xxxx_";

static char filename2[] = "testfile.yyy";

/* This is the hex string representation of the file created by compressing
   a simple text file with the contents "This is a test file."
 
   The file was created using COMPRESS.EXE from the Windows Server 2003
   Resource Kit from Microsoft.
 */
static const unsigned char compressed_file[] = 
  {0x53,0x5A,0x44,0x44,0x88,0xF0,0x27,0x33,0x41,
   0x74,0x75,0x14,0x00,0x00,0xDF,0x54,0x68,0x69,
   0x73,0x20,0xF2,0xF0,0x61,0x20,0xFF,0x74,0x65,
   0x73,0x74,0x20,0x66,0x69,0x6C,0x03,0x65,0x2E};
static const DWORD compressed_file_size = sizeof(compressed_file);

static const char uncompressed_data[] = "This is a test file.";
static const DWORD uncompressed_data_size = sizeof(uncompressed_data) - 1;

static char *buf;

static void full_file_path_name_in_a_CWD(const char *src, char *dst, BOOL expect_short)
{
  DWORD retval;
  char shortname[MAX_PATH];

  retval = GetCurrentDirectoryA(MAX_PATH, dst);
  ok(retval > 0, "GetCurrentDirectoryA returned %ld, GLE=%ld\n",
     retval, GetLastError());
  if(dst[retval-1] != '\\')
    /* Append backslash only when it's missing */
      lstrcatA(dst, "\\");
  lstrcatA(dst, src);
  if(expect_short) 
  {
    memcpy(shortname, dst, MAX_PATH);
    retval = GetShortPathNameA(shortname, dst, MAX_PATH-1);
    ok(retval > 0, "GetShortPathNameA returned %ld for '%s', GLE=%ld\n",
       retval, dst, GetLastError());
  }
}

static void create_file(char *fname)
{
  INT file;
  OFSTRUCT ofs;
  DWORD retval;

  file = LZOpenFileA(fname, &ofs, OF_CREATE);
  ok(file >= 0, "LZOpenFileA failed to create '%s'\n", fname);
  LZClose(file);
  retval = GetFileAttributesA(fname);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributesA('%s'): error %ld\n", ofs.szPathName, GetLastError());
}

static void delete_file(char *fname)
{
  INT file;
  OFSTRUCT ofs;
  DWORD retval;

  file = LZOpenFileA(fname, &ofs, OF_DELETE);
  ok(file >= 0, "LZOpenFileA failed to delete '%s'\n", fname);
  LZClose(file);
  retval = GetFileAttributesA(fname);
  ok(retval == INVALID_FILE_ATTRIBUTES, "GetFileAttributesA succeeded on deleted file ('%s')\n", ofs.szPathName);
}

static void test_LZOpenFileA_existing_compressed(void)
{
  OFSTRUCT test;
  INT file;
  char expected[MAX_PATH];
  char short_expected[MAX_PATH];
  char filled_0xA5[OFS_MAXPATHNAME];

  /* Try to open existing compressed files: */
  create_file(filename_);
  create_file(dotless_);
  create_file(extless_);
  create_file(_terminated_);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(filename_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* a, using 8.3-conformant file name. */
  file = LZOpenFileA(filename, &test, OF_EXIST);
  /* If the file "foo.xxx" does not exist, LZOpenFileA should then
     check for the file "foo.xx_" and open that -- at least on some
     operating systems.  Doesn't seem to on my copy of Win98.   
   */
  ok(file >= 0, "LZOpenFileA returns negative file descriptor for '%s'\n", filename);
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == 0, "LZOpenFileA set test.nErrCode to %d\n",
     test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(dotless_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* b, using dotless file name. */
  file = LZOpenFileA(dotless, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileA returns negative file descriptor for '%s'\n", dotless);
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == 0, "LZOpenFileA set test.nErrCode to %d\n",
     test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(extless_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* c, using extensionless file name. */
  file = LZOpenFileA(extless, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileA returns negative file descriptor for '%s'\n", extless);
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == 0, "LZOpenFileA set test.nErrCode to %d\n",
     test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(_terminated_, expected, FALSE);
  full_file_path_name_in_a_CWD(_terminated_, short_expected, TRUE);

  /* d, using underscore-terminated file name. */
  file = LZOpenFileA(_terminated, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileA failed on switching to a compressed file name\n");
  ok(test.cBytes == sizeof(OFSTRUCT), "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == 0, "LZOpenFileA set test.nErrCode to %d\n", 
     test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, short_expected);
  LZClose(file);

  delete_file(filename_);
  delete_file(dotless_);
  delete_file(extless_);
  delete_file(_terminated_);
}

static void test_LZOpenFileA_nonexisting_compressed(void)
{
  OFSTRUCT test;
  INT file;
  char expected[MAX_PATH];
  char filled_0xA5[OFS_MAXPATHNAME];

  /* Try to open nonexisting compressed files: */
  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(filename_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* a, using 8.3-conformant file name. */
  file = LZOpenFileA(filename, &test, OF_EXIST);
  /* If the file "foo.xxx" does not exist, LZOpenFileA should then
     check for the file "foo.xx_" and open that -- at least on some
     operating systems.  Doesn't seem to on my copy of Win98.   
   */
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileA succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(dotless_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* b, using dotless file name. */
  file = LZOpenFileA(dotless, &test, OF_EXIST);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileA succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(extless_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* c, using extensionless file name. */
  file = LZOpenFileA(extless, &test, OF_EXIST);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileA succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(_terminated_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* d, using underscore-terminated file name. */
  file = LZOpenFileA(_terminated, &test, OF_EXIST);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileA succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);
}

static void test_LZOpenFileA(void)
{
  OFSTRUCT test;
  DWORD retval;
  INT file;
  static char badfilename_[] = "badfilename_";
  char expected[MAX_PATH];
  char short_expected[MAX_PATH];

  SetLastError(0xfaceabee);
  /* Check for nonexistent file. */
  file = LZOpenFileA(badfilename_, &test, OF_READ);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileA succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND, 
     "GetLastError() returns %ld\n", GetLastError());
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(filename_, expected, FALSE);

  /* Create an empty file. */
  file = LZOpenFileA(filename_, &test, OF_CREATE);
  ok(file >= 0, "LZOpenFileA failed on creation\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  retval = GetFileAttributesA(filename_);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributesA: error %ld\n",
     GetLastError());

  /* Check various opening options: */
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(filename_, short_expected, TRUE);

  /* a, for reading. */
  file = LZOpenFileA(filename_, &test, OF_READ);
  ok(file >= 0, "LZOpenFileA failed on read\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d '%s'('%s')\n", test.cBytes, expected, short_expected);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n",
     test.szPathName, expected, short_expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* b, for writing. */
  file = LZOpenFileA(filename_, &test, OF_WRITE);
  ok(file >= 0, "LZOpenFileA failed on write\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n",
     test.szPathName, expected, short_expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* c, for reading and writing. */
  file = LZOpenFileA(filename_, &test, OF_READWRITE);
  ok(file >= 0, "LZOpenFileA failed on read/write\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n",
     test.szPathName, expected, short_expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* d, for checking file existence. */
  file = LZOpenFileA(filename_, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileA failed on read/write\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n",
     test.szPathName, expected, short_expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* Delete the file then make sure it doesn't exist anymore. */
  file = LZOpenFileA(filename_, &test, OF_DELETE);
  ok(file >= 0, "LZOpenFileA failed on delete\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileA set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileA set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileA returned '%s', but was expected to return '%s' or '%s'\n",
     test.szPathName, expected, short_expected);
  LZClose(file);

  retval = GetFileAttributesA(filename_);
  ok(retval == INVALID_FILE_ATTRIBUTES, 
     "GetFileAttributesA succeeded on deleted file\n");

  test_LZOpenFileA_existing_compressed();
  test_LZOpenFileA_nonexisting_compressed();
}

static void test_LZRead(void)
{
  HANDLE file;
  DWORD ret;
  int cfile;
  OFSTRUCT test;
  BOOL retok;

  /* Create the compressed file. */
  file = CreateFileA(filename_, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
  ok(file != INVALID_HANDLE_VALUE, "Could not create test file\n");
  retok = WriteFile(file, compressed_file, compressed_file_size, &ret, 0);
  ok( retok, "WriteFile: error %ld\n", GetLastError());
  ok(ret == compressed_file_size, "Wrote wrong number of bytes with WriteFile?\n");
  CloseHandle(file);

  cfile = LZOpenFileA(filename_, &test, OF_READ);
  ok(cfile > 0, "LZOpenFileA failed\n");

  ret = LZRead(cfile, buf, uncompressed_data_size);
  ok(ret == uncompressed_data_size, "Read wrong number of bytes\n");

  /* Compare what we read with what we think we should read. */
  ok(memcmp(buf, uncompressed_data, uncompressed_data_size) == 0,
     "buffer contents mismatch\n");

  todo_wine {
     /* Wine returns the number of bytes actually read instead of an error */
     ret = LZRead(cfile, buf, uncompressed_data_size);
     ok(ret == LZERROR_READ, "Expected read-past-EOF to return LZERROR_READ\n");
  }

  LZClose(cfile);

  ret = DeleteFileA(filename_);
  ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

static void test_LZCopy(void)
{
  HANDLE file;
  DWORD ret;
  int source, dest;
  OFSTRUCT stest, dtest;
  BOOL retok;

  /* Create the compressed file. */
  file = CreateFileA(filename_, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
  ok(file != INVALID_HANDLE_VALUE, 
     "CreateFileA: error %ld\n", GetLastError());
  retok = WriteFile(file, compressed_file, compressed_file_size, &ret, 0);
  ok( retok, "WriteFile error %ld\n", GetLastError());
  ok(ret == compressed_file_size, "Wrote wrong number of bytes\n");
  CloseHandle(file);

  source = LZOpenFileA(filename_, &stest, OF_READ);
  ok(source >= 0, "LZOpenFileA failed on compressed file\n");
  dest = LZOpenFileA(filename2, &dtest, OF_CREATE);
  ok(dest >= 0, "LZOpenFileA failed on creating new file %d\n", dest);

  ret = LZCopy(source, dest);
  ok(ret > 0, "LZCopy error\n");

  LZClose(source);
  LZClose(dest);

  file = CreateFileA(filename2, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
  ok(file != INVALID_HANDLE_VALUE,
     "CreateFileA: error %ld\n", GetLastError());

  retok = ReadFile(file, buf, uncompressed_data_size*2, &ret, 0);
  ok( retok && ret == uncompressed_data_size, "ReadFile: error %ld\n", GetLastError());
  /* Compare what we read with what we think we should read. */
  ok(!memcmp(buf, uncompressed_data, uncompressed_data_size),
     "buffer contents mismatch\n");
  CloseHandle(file);

  ret = DeleteFileA(filename_);
  ok(ret, "DeleteFileA: error %ld\n", GetLastError());
  ret = DeleteFileA(filename2);
  ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

static void create_fileW(WCHAR *fnameW)
{
  INT file;
  OFSTRUCT ofs;
  DWORD retval;

  file = LZOpenFileW(fnameW, &ofs, OF_CREATE);
  ok(file >= 0, "LZOpenFileW failed on creation\n");
  LZClose(file);
  retval = GetFileAttributesW(fnameW);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributesW('%s'): error %ld\n", ofs.szPathName, GetLastError());
}

static void delete_fileW(WCHAR *fnameW)
{
  INT file;
  OFSTRUCT ofs;
  DWORD retval;

  file = LZOpenFileW(fnameW, &ofs, OF_DELETE);
  ok(file >= 0, "LZOpenFileW failed on delete\n");
  LZClose(file);
  retval = GetFileAttributesW(fnameW);
  ok(retval == INVALID_FILE_ATTRIBUTES, "GetFileAttributesW succeeded on deleted file ('%s')\n", ofs.szPathName);
}

static void test_LZOpenFileW_existing_compressed(void)
{
  OFSTRUCT test;
  INT file;
  char expected[MAX_PATH];

  /* Try to open existing compressed files: */
  create_fileW(filenameW_);
  create_fileW(dotlessW_);
  create_fileW(extlessW_);
  create_fileW(_terminatedW_);

  full_file_path_name_in_a_CWD(filename_, expected, FALSE);
  memset(&test, 0xA5, sizeof(test));

  /* a, using 8.3-conformant file name. */
  file = LZOpenFileW(filenameW, &test, OF_EXIST);
  /* If the file "foo.xxx" does not exist, LZOpenFileW should then
     check for the file "foo.xx_" and open that.
   */
  ok(file >= 0, "LZOpenFileW failed on switching to a compressed file name\n");
  ok(test.cBytes == sizeof(OFSTRUCT), 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS, "LZOpenFileW set test.nErrCode to %d\n", 
     test.nErrCode);
  /* Note that W-function returns A-string by a OFSTRUCT.szPathName: */
  ok(lstrcmpA(test.szPathName, expected) == 0, 
     "LZOpenFileW returned '%s', but was expected to return '%s'\n", 
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(dotless_, expected, FALSE);

  /* b, using dotless file name. */
  file = LZOpenFileW(dotlessW, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileW failed on switching to a compressed file name\n");
  ok(test.cBytes == sizeof(OFSTRUCT), 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS, "LZOpenFileW set test.nErrCode to %d\n", 
     test.nErrCode);
  /* Note that W-function returns A-string by a OFSTRUCT.szPathName: */
  ok(lstrcmpA(test.szPathName, expected) == 0, 
     "LZOpenFileW returned '%s', but was expected to return '%s'\n", 
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(extless_, expected, FALSE);

  /* c, using extensionless file name. */
  file = LZOpenFileW(extlessW, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileW failed on switching to a compressed file name\n");
  ok(test.cBytes == sizeof(OFSTRUCT), 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS, "LZOpenFileW set test.nErrCode to %d\n", 
     test.nErrCode);
  /* Note that W-function returns A-string by a OFSTRUCT.szPathName: */
  ok(lstrcmpA(test.szPathName, expected) == 0, 
     "LZOpenFileW returned '%s', but was expected to return '%s'\n", 
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(_terminated_, expected, FALSE);

  /* d, using underscore-terminated file name. */
  file = LZOpenFileW(_terminatedW, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileW failed on switching to a compressed file name\n");
  ok(test.cBytes == sizeof(OFSTRUCT), 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS, "LZOpenFileW set test.nErrCode to %d\n", 
     test.nErrCode);
  /* Note that W-function returns A-string by a OFSTRUCT.szPathName: */
  ok(lstrcmpA(test.szPathName, expected) == 0, 
     "LZOpenFileW returned '%s', but was expected to return '%s'\n", 
     test.szPathName, expected);
  LZClose(file);

  delete_fileW(filenameW_);
  delete_fileW(dotlessW_);
  delete_fileW(extlessW_);
  delete_fileW(_terminatedW_);
}

static void test_LZOpenFileW_nonexisting_compressed(void)
{
  OFSTRUCT test;
  INT file;
  char expected[MAX_PATH];
  char filled_0xA5[OFS_MAXPATHNAME];

  /* Try to open nonexisting compressed files: */
  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(filename_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* a, using 8.3-conformant file name. */
  file = LZOpenFileW(filenameW, &test, OF_EXIST);
  /* If the file "foo.xxx" does not exist, LZOpenFileA should then
     check for the file "foo.xx_" and open that -- at least on some
     operating systems.  Doesn't seem to on my copy of Win98.   
   */
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileW succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(dotless_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* b, using dotless file name. */
  file = LZOpenFileW(dotlessW, &test, OF_EXIST);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileW succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(extless_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* c, using extensionless file name. */
  file = LZOpenFileW(extlessW, &test, OF_EXIST);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileW succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s' or '%s'\n", 
     test.szPathName, expected, filled_0xA5);

  memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(_terminated_, expected, FALSE);
  SetLastError(0xfaceabee);

  /* d, using underscore-terminated file name. */
  file = LZOpenFileW(_terminatedW, &test, OF_EXIST);
  ok(file == LZERROR_BADINHANDLE, 
     "LZOpenFileW succeeded on nonexistent file\n");
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(test.cBytes == 0xA5, 
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_FILE_NOT_FOUND, 
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s' or '%s'\n",
     test.szPathName, expected, filled_0xA5);
}

static void test_LZOpenFileW(void)
{
  OFSTRUCT test;
  DWORD retval;
  INT file;
  static WCHAR badfilenameW[] = L"badfilename.xtn";
  char expected[MAX_PATH];

  SetLastError(0xfaceabee);
  /* Check for nonexistent file. */
  file = LZOpenFileW(badfilenameW, &test, OF_READ);
  if(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("LZOpenFileW call is not implemented\n");
    return;
  }
  ok(GetLastError() == ERROR_FILE_NOT_FOUND,
     "GetLastError() returns %ld\n", GetLastError());
  ok(file == LZERROR_BADINHANDLE, "LZOpenFileW succeeded on nonexistent file\n");
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));
  full_file_path_name_in_a_CWD(filename_, expected, FALSE);

  /* Create an empty file. */
  file = LZOpenFileW(filenameW_, &test, OF_CREATE);
  ok(file >= 0, "LZOpenFile failed on creation\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  retval = GetFileAttributesW(filenameW_);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributes: error %ld\n",
    GetLastError());

  /* Check various opening options: */
  memset(&test, 0xA5, sizeof(test));

  /* a, for reading. */
  file = LZOpenFileW(filenameW_, &test, OF_READ);
  ok(file >= 0, "LZOpenFileW failed on read\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* b, for writing. */
  file = LZOpenFileW(filenameW_, &test, OF_WRITE);
  ok(file >= 0, "LZOpenFileW failed on write\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* c, for reading and writing. */
  file = LZOpenFileW(filenameW_, &test, OF_READWRITE);
  ok(file >= 0, "LZOpenFileW failed on read/write\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* d, for checking file existence. */
  file = LZOpenFileW(filenameW_, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFileW failed on read/write\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  memset(&test, 0xA5, sizeof(test));

  /* Delete the file then make sure it doesn't exist anymore. */
  file = LZOpenFileW(filenameW_, &test, OF_DELETE);
  ok(file >= 0, "LZOpenFileW failed on delete\n");
  ok(test.cBytes == sizeof(OFSTRUCT),
     "LZOpenFileW set test.cBytes to %d\n", test.cBytes);
  ok(test.nErrCode == ERROR_SUCCESS,
     "LZOpenFileW set test.nErrCode to %d\n", test.nErrCode);
  ok(lstrcmpA(test.szPathName, expected) == 0,
     "LZOpenFileW returned '%s', but was expected to return '%s'\n",
     test.szPathName, expected);
  LZClose(file);

  retval = GetFileAttributesW(filenameW_);
  ok(retval == INVALID_FILE_ATTRIBUTES, 
     "GetFileAttributesW succeeded on deleted file\n");

  test_LZOpenFileW_existing_compressed();
  test_LZOpenFileW_nonexisting_compressed();
}


START_TEST(lzexpand_main)
{
  buf = HeapAlloc(GetProcessHeap(), 0, uncompressed_data_size * 2);
  test_LZOpenFileA();
  test_LZOpenFileW();
  test_LZRead();
  test_LZCopy();
  HeapFree(GetProcessHeap(), 0, buf);
}
