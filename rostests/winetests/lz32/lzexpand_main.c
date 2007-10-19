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

static char filename_[] = "testfile.xx_";
#if 0
static char filename[] = "testfile.xxx";
#endif
static char filename2[] = "testfile.yyy";

/* This is the hex string representation of the file created by compressing
   a simple text file with the contents "This is a test file."

   The file was created using COMPRESS.EXE from the Windows Server 2003
   Resource Kit from Microsoft.  The resource kit was retrieved from the
   following URL:

   http://www.microsoft.com/downloads/details.aspx?FamilyID=9d467a69-57ff-4ae7-96ee-b18c4790cffd&displaylang=en
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

static void test_lzopenfile(void)
{
  OFSTRUCT test;
  DWORD retval;
  INT file;

  /* Check for nonexistent file. */
  file = LZOpenFile("badfilename_", &test, OF_READ);
  ok(file == LZERROR_BADINHANDLE,
     "LZOpenFile succeeded on nonexistent file\n");
  LZClose(file);

  /* Create an empty file. */
  file = LZOpenFile(filename_, &test, OF_CREATE);
  ok(file >= 0, "LZOpenFile failed on creation\n");
  LZClose(file);
  retval = GetFileAttributes(filename_);
  ok(retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributes: error %ld\n",
     GetLastError());

  /* Check various opening options. */
  file = LZOpenFile(filename_, &test, OF_READ);
  ok(file >= 0, "LZOpenFile failed on read\n");
  LZClose(file);
  file = LZOpenFile(filename_, &test, OF_WRITE);
  ok(file >= 0, "LZOpenFile failed on write\n");
  LZClose(file);
  file = LZOpenFile(filename_, &test, OF_READWRITE);
  ok(file >= 0, "LZOpenFile failed on read/write\n");
  LZClose(file);
  file = LZOpenFile(filename_, &test, OF_EXIST);
  ok(file >= 0, "LZOpenFile failed on read/write\n");
  LZClose(file);


  /* If the file "foo.xxx" does not exist, LZOpenFile should then
     check for the file "foo.xx_" and open that -- at least on some
     operating systems.  Doesn't seem to on my copy of Win98.
     The Wine testing guidelines say we should accept the behavior of
     any valid version of Windows.  Thus it seems we cannot check this?!
     Revisit this at some point to see if this can be tested somehow.
   */
#if 0
  file = LZOpenFile(filename, &test, OF_EXIST);
  ok(file != LZERROR_BADINHANDLE,
     "LZOpenFile \"filename_\" check failed\n");
  LZClose(file);
#endif

  /* Delete the file then make sure it doesn't exist anymore. */
  file = LZOpenFile(filename_, &test, OF_DELETE);
  ok(file >= 0, "LZOpenFile failed on delete\n");
  LZClose(file);

  retval = GetFileAttributes(filename_);
  ok(retval == INVALID_FILE_ATTRIBUTES,
     "GetFileAttributes succeeded on deleted file\n");

}

static void test_lzread(void)
{
  HANDLE file;
  DWORD ret;
  int cfile;
  OFSTRUCT test;
  BOOL retok;

  /* Create the compressed file. */
  file = CreateFile(filename_, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
  ok(file != INVALID_HANDLE_VALUE, "Could not create test file\n");
  retok = WriteFile(file, compressed_file, compressed_file_size, &ret, 0);
  ok( retok, "WriteFile: error %ld\n", GetLastError());
  ok(ret == compressed_file_size, "Wrote wrong number of bytes with WriteFile?\n");
  CloseHandle(file);

  cfile = LZOpenFile(filename_, &test, OF_READ);
  ok(cfile > 0, "LZOpenFile failed\n");

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

  ret = DeleteFile(filename_);
  ok(ret, "DeleteFile: error %ld\n", GetLastError());
}

static void test_lzcopy(void)
{
  HANDLE file;
  DWORD ret;
  int source, dest;
  OFSTRUCT stest, dtest;
  BOOL retok;

  /* Create the compressed file. */
  file = CreateFile(filename_, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
  ok(file != INVALID_HANDLE_VALUE,
     "CreateFile: error %ld\n", GetLastError());
  retok = WriteFile(file, compressed_file, compressed_file_size, &ret, 0);
  ok( retok, "WriteFile error %ld\n", GetLastError());
  ok(ret == compressed_file_size, "Wrote wrong number of bytes\n");
  CloseHandle(file);

  source = LZOpenFile(filename_, &stest, OF_READ);
  ok(source >= 0, "LZOpenFile failed on compressed file\n");
  dest = LZOpenFile(filename2, &dtest, OF_CREATE);
  ok(dest >= 0, "LZOpenFile failed on creating new file %d\n", dest);

  ret = LZCopy(source, dest);
  ok(ret > 0, "LZCopy error\n");

  LZClose(source);
  LZClose(dest);

  file = CreateFile(filename2, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		    0, 0);
  ok(file != INVALID_HANDLE_VALUE,
     "CreateFile: error %ld\n", GetLastError());

  retok = ReadFile(file, buf, uncompressed_data_size*2, &ret, 0);
  ok( retok && ret == uncompressed_data_size, "ReadFile: error %ld\n", GetLastError());
  /* Compare what we read with what we think we should read. */
  ok(!memcmp(buf, uncompressed_data, uncompressed_data_size),
     "buffer contents mismatch\n");
  CloseHandle(file);

  ret = DeleteFile(filename_);
  ok(ret, "DeleteFile: error %ld\n", GetLastError());
  ret = DeleteFile(filename2);
  ok(ret, "DeleteFile: error %ld\n", GetLastError());
}

START_TEST(lzexpand_main)
{
  buf = malloc(uncompressed_data_size * 2);
  test_lzopenfile();
  test_lzread();
  test_lzcopy();
  free(buf);
}
