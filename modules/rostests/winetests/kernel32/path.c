/*
 * Unit test suite for various Path and Directory Functions
 *
 * Copyright 2002 Geoffrey Hausheer
 * Copyright 2006 Detlef Riekenberg
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
#include <stdio.h>
#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/test.h"

#define HAS_TRAIL_SLASH_A(string) (string[lstrlenA(string)-1]=='\\')

#define LONGFILE "Long File test.path"
#define SHORTFILE "pathtest.pth"
#define SHORTDIR "shortdir"
#define LONGDIR "Long Directory"
#define NONFILE_SHORT "noexist.pth"
#define NONFILE_LONG "NonExistent File"
#define NONDIR_SHORT "notadir"
#define NONDIR_LONG "NonExistent Directory"

#define NOT_A_VALID_DRIVE '@'

#ifdef __i386__
#define ARCH "x86"
#elif defined __aarch64__ || defined__arm64ec__
#define ARCH "arm64"
#elif defined __x86_64__
#define ARCH "amd64"
#elif defined __arm__
#define ARCH "arm"
#else
#define ARCH "none"
#endif

/* the following characters don't work well with GetFullPathNameA
   in Win98.  I don't know if this is a FAT thing, or if it is an OS thing
   but I don't test these characters now.
   NOTE: Win2k allows GetFullPathNameA to work with them though
      |<>"
*/
static const CHAR funny_chars[]="!@#$%^&*()=+{}[],?'`";
static const CHAR is_char_ok[] ="11111110111111111011";

/* Present in Win2003+ */
static BOOL  (WINAPI *pNeedCurrentDirectoryForExePathA)(LPCSTR);
static BOOL  (WINAPI *pNeedCurrentDirectoryForExePathW)(LPCWSTR);

static DLL_DIRECTORY_COOKIE (WINAPI *pAddDllDirectory)(const WCHAR*);
static BOOL     (WINAPI *pRemoveDllDirectory)(DLL_DIRECTORY_COOKIE);
static BOOL     (WINAPI *pSetSearchPathMode)(DWORD);
static BOOL     (WINAPI *pSetDllDirectoryW)(LPCWSTR);
static BOOL     (WINAPI *pSetDefaultDllDirectories)(DWORD);
static NTSTATUS (WINAPI *pRtlGetExePath)(LPCWSTR,LPWSTR*);
static NTSTATUS (WINAPI *pRtlGetSearchPath)(LPWSTR*);
static void     (WINAPI *pRtlReleasePath)(LPWSTR);
static NTSTATUS (WINAPI *pLdrGetDllPath)(LPCWSTR,ULONG,LPWSTR*,LPWSTR*);

static BOOL (WINAPI *pCheckNameLegalDOS8Dot3W)(const WCHAR *, char *, DWORD, BOOL *, BOOL *);
static BOOL (WINAPI *pCheckNameLegalDOS8Dot3A)(const char *, char *, DWORD, BOOL *, BOOL *);

/* a structure to deal with wine todos somewhat cleanly */
typedef struct {
  DWORD shortlen;
  DWORD shorterror;
  DWORD s2llen;
  DWORD s2lerror;
  DWORD longlen;
  DWORD longerror;
} SLpassfail;

/* function that tests GetFullPathNameA, GetShortPathNameA,GetLongPathNameA */
/* NOTE: the passfail structure is used to allow customizable todo checking
         for wine.  It is not very pretty, but it sure beats duplicating this
         function lots of times
*/
static void test_ValidPathA(const CHAR *curdir, const CHAR *subdir, const CHAR *filename,
                         CHAR *shortstr, SLpassfail *passfail, const CHAR *errstr)
{
  CHAR tmpstr[MAX_PATH],
       fullpath[MAX_PATH + 1],      /*full path to the file (not short/long) */
       subpath[MAX_PATH],       /*relative path to the file */
       fullpathshort[2 * MAX_PATH], /*absolute path to the file (short format) */
       fullpathlong[2 * MAX_PATH],  /*absolute path to the file (long format) */
       curdirshort[MAX_PATH],   /*absolute path to the current dir (short) */
       curdirlong[MAX_PATH];    /*absolute path to the current dir (long) */
  LPSTR strptr;                 /*ptr to the filename portion of the path */
  DWORD len;
/* if passfail is NULL, we can perform all checks within this function,
   otherwise, we will return the relevant data in the passfail struct, so
   we must initialize it first
*/
  if(passfail!=NULL) {
    passfail->shortlen=-1;passfail->s2llen=-1;passfail->longlen=-1;
    passfail->shorterror=0;passfail->s2lerror=0;passfail->longerror=0;
  }

  ok((len = GetLongPathNameA(curdir,curdirlong,MAX_PATH)), "%s: GetLongPathNameA failed\n", errstr);
/*GetLongPathNameA can return a trailing '\\' but shouldn't do so here */
  ok(!HAS_TRAIL_SLASH_A(curdirlong), "%s: GetLongPathNameA should not have a trailing \\\n", errstr);

  ok((len=GetShortPathNameA(curdir,curdirshort,MAX_PATH)),
     "%s: GetShortPathNameA failed\n",errstr);
/*GetShortPathNameA can return a trailing '\\' but shouldn't do so here */
  ok(! HAS_TRAIL_SLASH_A(curdirshort),
     "%s: GetShortPathNameA should not have a trailing \\\n",errstr);
/* build relative and absolute paths from inputs */
  if(*subdir) {
    sprintf(subpath,"%s\\%s",subdir,filename);
  } else {
    lstrcpyA(subpath,filename);
  }
  sprintf(fullpath,"%s\\%s",curdir,subpath);
  sprintf(fullpathshort,"%s\\%s",curdirshort,subpath);
  sprintf(fullpathlong,"%s\\%s",curdirlong,subpath);
/* Test GetFullPathNameA functionality */
  len=GetFullPathNameA(subpath,MAX_PATH,tmpstr,&strptr);
  ok(len, "GetFullPathNameA failed for: '%s'\n",subpath);
  if(HAS_TRAIL_SLASH_A(subpath)) {
    ok(strptr==NULL,
       "%s: GetFullPathNameA should not return a filename ptr\n",errstr);
    ok(lstrcmpiA(fullpath,tmpstr)==0,
       "%s: GetFullPathNameA returned '%s' instead of '%s'\n",
       errstr,tmpstr,fullpath);
  } else {
    ok(lstrcmpiA(strptr,filename)==0,
       "%s: GetFullPathNameA returned '%s' instead of '%s'\n",
       errstr,strptr,filename);
    ok(lstrcmpiA(fullpath,tmpstr)==0,
       "%s: GetFullPathNameA returned '%s' instead of '%s'\n",
       errstr,tmpstr,fullpath);
  }
/* Test GetShortPathNameA functionality */
  SetLastError(0);
  len=GetShortPathNameA(fullpathshort,shortstr,MAX_PATH);
  if(passfail==NULL) {
    ok(len, "%s: GetShortPathNameA failed\n",errstr);
  } else {
    passfail->shortlen=len;
    passfail->shorterror=GetLastError();
  }

    /* Test GetLongPathNameA functionality, both conversion from GetFullPathNameA and from GetShortPathNameA */
    if (len)
    {
        SetLastError(0);
        len = GetLongPathNameA(shortstr, tmpstr, MAX_PATH);
        if (!passfail)
        {
            ok(len, "%s: GetLongPathNameA failed during Short->Long conversion\n", errstr);
            ok(!lstrcmpiA(fullpathlong, tmpstr), "%s: GetLongPathNameA returned '%s' instead of '%s'\n", errstr,
                    tmpstr, fullpathlong);
        }
        else
        {
            passfail->s2llen = len;
            passfail->s2lerror = GetLastError();
        }
    }

    SetLastError(0);
    len = GetLongPathNameA(fullpath, tmpstr, MAX_PATH);
    if (!passfail)
    {
        ok(len, "%s: GetLongPathNameA failed\n",errstr);
        ok(!lstrcmpiA(fullpathlong, tmpstr), "%s: GetLongPathNameA returned '%s' instead of '%s'\n",
                errstr, tmpstr, fullpathlong);
    }
    else
    {
        passfail->longlen = len;
        passfail->longerror = GetLastError();
    }
}

/* split path into leading directory, and 8.3 filename */
static void test_SplitShortPathA(CHAR *path,CHAR *dir,CHAR *eight,CHAR *three) {
  BOOL done = FALSE, error = FALSE;
  int ext,fil;
  int len,i;
  len=lstrlenA(path);
  ext=len;
  fil=len;
/* walk backwards over path looking for '.' or '\\' separators */
  for(i=len-1;(i>=0) && (!done);i--) {
    if(path[i]=='.')
      if(ext!=len) error=TRUE; else ext=i;
    else if(path[i]=='\\') {
      if(i==len-1) {
        error=TRUE;
      } else {
        fil=i;
        done=TRUE;
      }
    }
  }
/* Check that we didn't find a trailing '\\' or multiple '.' */
  ok(!error,"Illegal file found in 8.3 path '%s'\n",path);
/* Separate dir, root, and extension */
  if(ext!=len) lstrcpyA(three,path+ext+1); else lstrcpyA(three,"");
  if(fil!=len) {
    lstrcpynA(eight,path+fil+1,ext-fil);
    lstrcpynA(dir,path,fil+1);
  } else {
    lstrcpynA(eight,path,ext+1);
    lstrcpyA(dir,"");
  }
/* Validate that root and extension really are 8.3 */
  ok(lstrlenA(eight)<=8 && lstrlenA(three)<=3,
     "GetShortPathNAmeA did not return an 8.3 path\n");
}

/* Check that GetShortPathNameA returns a valid 8.3 path */
static void test_LongtoShortA(CHAR *teststr,const CHAR *goodstr,
                              const CHAR *ext,const CHAR *errstr) {
  CHAR dir[MAX_PATH],eight[MAX_PATH],three[MAX_PATH];

  test_SplitShortPathA(teststr,dir,eight,three);
  ok(lstrcmpiA(dir,goodstr)==0,
     "GetShortPathNameA returned '%s' instead of '%s'\n",dir,goodstr);
  ok(lstrcmpiA(three,ext)==0,
     "GetShortPathNameA returned '%s' with incorrect extension\n",three);
}

/* Test that Get(Short|Long|Full)PathNameA work correctly with interesting
   characters in the filename.
     'valid' indicates whether this would be an allowed filename
     'todo' indicates that wine doesn't get this right yet.
   NOTE: We always call this routine with a nonexistent filename, so
         Get(Short|Long)PathNameA should never pass, but GetFullPathNameA
         should.
*/
static void test_FunnyChars(CHAR *curdir,CHAR *curdir_short,CHAR *filename, INT valid,CHAR *errstr)
{
  CHAR tmpstr[MAX_PATH],tmpstr1[MAX_PATH];
  SLpassfail passfail;

  test_ValidPathA(curdir,"",filename,tmpstr,&passfail,errstr);
  if(valid) {
    sprintf(tmpstr1,"%s\\%s",curdir_short,filename);
      ok((passfail.shortlen==0 &&
          (passfail.shorterror==ERROR_FILE_NOT_FOUND || passfail.shorterror==ERROR_PATH_NOT_FOUND || !passfail.shorterror)) ||
         (passfail.shortlen==strlen(tmpstr1) && lstrcmpiA(tmpstr,tmpstr1)==0),
         "%s: GetShortPathNameA error: len=%ld error=%ld tmpstr=[%s]\n",
         errstr,passfail.shortlen,passfail.shorterror,tmpstr);
  } else {
      ok(passfail.shortlen==0 &&
         (passfail.shorterror==ERROR_INVALID_NAME || passfail.shorterror==ERROR_FILE_NOT_FOUND || !passfail.shorterror),
         "%s: GetShortPathA should have failed len=%ld, error=%ld\n",
         errstr,passfail.shortlen,passfail.shorterror);
  }

    ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
    if (valid)
    {
        ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "%s: GetLongPathA unexpected error %ld.\n", errstr,
                passfail.longerror);
    }
    else
    {
        ok(passfail.longerror == ERROR_INVALID_NAME || passfail.longerror == ERROR_FILE_NOT_FOUND,
                "%s: GetLongPathA unexpected error %ld.\n", errstr, passfail.longerror);
    }
}

/* Routine to test that SetCurrentDirectory behaves as expected. */
static void test_setdir(CHAR *olddir,CHAR *newdir,
                        CHAR *cmprstr, INT pass, const CHAR *errstr)
{
  CHAR tmppath[MAX_PATH], *dirptr;
  DWORD val,len,chklen;

  val=SetCurrentDirectoryA(newdir);
  len=GetCurrentDirectoryA(MAX_PATH,tmppath);
/* if 'pass' then the SetDirectoryA was supposed to pass */
  if(pass) {
    dirptr=(cmprstr==NULL) ? newdir : cmprstr;
    chklen=lstrlenA(dirptr);
    ok(val,"%s: SetCurrentDirectoryA failed\n",errstr);
    ok(len==chklen,
       "%s: SetCurrentDirectory did not change the directory, though it passed\n",
       errstr);
    ok(lstrcmpiA(dirptr,tmppath)==0,
       "%s: SetCurrentDirectory did not change the directory, though it passed\n",
       errstr);
    ok(SetCurrentDirectoryA(olddir),
       "%s: Couldn't set directory to its original value\n",errstr);
  } else {
/* else test that it fails correctly */
    chklen=lstrlenA(olddir);
    ok(val==0,
       "%s: SetCurrentDirectoryA passed when it should have failed\n",errstr);
    ok(len==chklen,
       "%s: SetCurrentDirectory changed the directory, though it failed\n",
       errstr);
    ok(lstrcmpiA(olddir,tmppath)==0,
       "%s: SetCurrentDirectory changed the directory, though it failed\n",
       errstr);
  }
}
static void test_InitPathA(CHAR *newdir, CHAR *curDrive, CHAR *otherDrive)
{
  CHAR tmppath[MAX_PATH], /*path to TEMP */
       tmpstr[MAX_PATH],
       tmpstr1[MAX_PATH],
       invalid_dir[MAX_PATH + 29];

  DWORD len,len1,drives;
  INT id;
  HANDLE hndl;
  BOOL bRes;
  UINT unique;

  *curDrive = *otherDrive = NOT_A_VALID_DRIVE;

/* Get the current drive letter */
  if( GetCurrentDirectoryA( MAX_PATH, tmpstr))
    *curDrive = tmpstr[0];
  else
    trace( "Unable to discover current drive, some tests will not be conducted.\n");

/* Test GetTempPathA */
  len=GetTempPathA(MAX_PATH,tmppath);
  ok(len!=0 && len < MAX_PATH,"GetTempPathA failed\n");
  ok(HAS_TRAIL_SLASH_A(tmppath),
     "GetTempPathA returned a path that did not end in '\\'\n");
  lstrcpyA(tmpstr,"aaaaaaaa");
  len1=GetTempPathA(len,tmpstr);
  ok(len1==len+1,"GetTempPathA should return string length %ld instead of %ld\n",len+1,len1);

/* Test GetTmpFileNameA */
  ok((id=GetTempFileNameA(tmppath,"path",0,newdir)),"GetTempFileNameA failed\n");
  sprintf(tmpstr,"pat%.4x.tmp",id & 0xffff);
  sprintf(tmpstr1,"pat%x.tmp",id & 0xffff);
  ok(lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr)==0 ||
     lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr1)==0,
     "GetTempFileNameA returned '%s' which doesn't match '%s' or '%s'. id=%x\n",
     newdir,tmpstr,tmpstr1,id);
  ok(DeleteFileA(newdir),"Couldn't delete the temporary file we just created\n");     

  id=GetTempFileNameA(tmppath,NULL,0,newdir);
  sprintf(tmpstr,"%.4x.tmp",id & 0xffff);
  sprintf(tmpstr1,"%x.tmp",id & 0xffff);
  ok(lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr)==0 ||
     lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr1)==0,
     "GetTempFileNameA returned '%s' which doesn't match '%s' or '%s'. id=%x\n",
     newdir,tmpstr,tmpstr1,id);
  ok(DeleteFileA(newdir),"Couldn't delete the temporary file we just created\n");

  for(unique=0;unique<3;unique++) {
    /* Nonexistent path */
    sprintf(invalid_dir, "%s\\%s",tmppath,"non_existent_dir_1jwj3y32nb3");
    SetLastError(0xdeadbeef);
    ok(!GetTempFileNameA(invalid_dir,"tfn",unique,newdir),"GetTempFileNameA should have failed\n");
    ok(GetLastError()==ERROR_DIRECTORY,"got %lu, expected ERROR_DIRECTORY\n", GetLastError());

    /* Check return value for unique !=0 */
    if(unique) {
      ok((GetTempFileNameA(tmppath,"tfn",unique,newdir) == unique),"GetTempFileNameA unexpectedly failed\n");
      /* if unique != 0, the actual temp files are not created: */
      ok(!DeleteFileA(newdir) && GetLastError() == ERROR_FILE_NOT_FOUND,"Deleted a file that shouldn't exist!\n");
    }
  }

/* Find first valid drive letter that is neither newdir[0] nor curDrive */
  drives = GetLogicalDrives() & ~(1<<(newdir[0]-'A'));
  if( *curDrive != NOT_A_VALID_DRIVE)
    drives &= ~(1<<(*curDrive-'A'));
  if( drives)
    for( *otherDrive='A'; (drives & 1) == 0; drives>>=1, (*otherDrive)++);
  else
    trace( "Could not find alternative drive, some tests will not be conducted.\n");

/* Do some CreateDirectoryA tests */
/* It would be nice to do test the SECURITY_ATTRIBUTES, but I don't
   really understand how they work.
   More formal tests should be done along with CreateFile tests
*/
  ok((id=GetTempFileNameA(tmppath,"path",0,newdir)),"GetTempFileNameA failed\n");
  ok(CreateDirectoryA(newdir,NULL)==0,
     "CreateDirectoryA succeeded even though a file of the same name exists\n");
  ok(DeleteFileA(newdir),"Couldn't delete the temporary file we just created\n");
  ok(CreateDirectoryA(newdir,NULL),"CreateDirectoryA failed\n");
/* Create some files to test other functions.  Note, we will test CreateFileA
   at some later point
*/
  sprintf(tmpstr,"%s\\%s",newdir,SHORTDIR);
  ok(CreateDirectoryA(tmpstr,NULL),"CreateDirectoryA failed\n");
  sprintf(tmpstr,"%s\\%s",newdir,LONGDIR);
  ok(CreateDirectoryA(tmpstr,NULL),"CreateDirectoryA failed\n");
  sprintf(tmpstr,"%c:", *curDrive);
  bRes = CreateDirectoryA(tmpstr,NULL);
  ok(!bRes && (GetLastError() == ERROR_ACCESS_DENIED  ||
               GetLastError() == ERROR_ALREADY_EXISTS),
     "CreateDirectoryA(\"%s\" should have failed (%ld)\n", tmpstr, GetLastError());
  sprintf(tmpstr,"%c:\\", *curDrive);
  bRes = CreateDirectoryA(tmpstr,NULL);
  ok(!bRes && (GetLastError() == ERROR_ACCESS_DENIED  ||
               GetLastError() == ERROR_ALREADY_EXISTS),
     "CreateDirectoryA(\"%s\" should have failed (%ld)\n", tmpstr, GetLastError());
  sprintf(tmpstr,"%s\\%s\\%s",newdir,SHORTDIR,SHORTFILE);
  hndl=CreateFileA(tmpstr,GENERIC_WRITE,0,NULL,
                   CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  ok(hndl!=INVALID_HANDLE_VALUE,"CreateFileA failed\n");
  ok(CloseHandle(hndl),"CloseHandle failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",newdir,SHORTDIR,LONGFILE);
  hndl=CreateFileA(tmpstr,GENERIC_WRITE,0,NULL,
                   CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  ok(hndl!=INVALID_HANDLE_VALUE,"CreateFileA failed\n");
  ok(CloseHandle(hndl),"CloseHandle failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",newdir,LONGDIR,SHORTFILE);
  hndl=CreateFileA(tmpstr,GENERIC_WRITE,0,NULL,
                   CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  ok(hndl!=INVALID_HANDLE_VALUE,"CreateFileA failed\n");
  ok(CloseHandle(hndl),"CloseHandle failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",newdir,LONGDIR,LONGFILE);
  hndl=CreateFileA(tmpstr,GENERIC_WRITE,0,NULL,
                   CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  ok(hndl!=INVALID_HANDLE_VALUE,"CreateFileA failed\n");
  ok(CloseHandle(hndl),"CloseHandle failed\n");
}

/* Test GetCurrentDirectory & SetCurrentDirectory */
static void test_CurrentDirectoryA(CHAR *origdir, CHAR *newdir)
{
  CHAR tmpstr[MAX_PATH],tmpstr1[MAX_PATH];
  char *buffer;
  DWORD len,len1;
/* Save the original directory, so that we can return to it at the end
   of the test
*/
  len=GetCurrentDirectoryA(MAX_PATH,origdir);
  ok(len!=0 && len < MAX_PATH,"GetCurrentDirectoryA failed\n");
/* Make sure that CetCurrentDirectoryA doesn't overwrite the buffer when the
   buffer size is too small to hold the current directory
*/
  lstrcpyA(tmpstr,"aaaaaaa");
  len1=GetCurrentDirectoryA(len,tmpstr);
  ok(len1==len+1, "GetCurrentDirectoryA returned %ld instead of %ld\n",len1,len+1);
  ok(lstrcmpiA(tmpstr,"aaaaaaa")==0,
     "GetCurrentDirectoryA should not have modified the buffer\n");

  buffer = HeapAlloc( GetProcessHeap(), 0, 2 * 65536 );
  SetLastError( 0xdeadbeef );
  strcpy( buffer, "foo" );
  len = GetCurrentDirectoryA( 32767, buffer );
  ok( len != 0 && len < MAX_PATH, "GetCurrentDirectoryA failed %lu err %lu\n", len, GetLastError() );
  if (len) ok( !strcmp( buffer, origdir ), "wrong result %s\n", buffer );
  SetLastError( 0xdeadbeef );
  strcpy( buffer, "foo" );
  len = GetCurrentDirectoryA( 32768, buffer );
  ok( len != 0 && len < MAX_PATH, "GetCurrentDirectoryA failed %lu err %lu\n", len, GetLastError() );
  if (len) ok( !strcmp( buffer, origdir ), "wrong result %s\n", buffer );
  SetLastError( 0xdeadbeef );
  strcpy( buffer, "foo" );
  len = GetCurrentDirectoryA( 65535, buffer );
  ok( (len != 0 && len < MAX_PATH) || broken(!len), /* nt4, win2k, xp */ "GetCurrentDirectoryA failed %lu err %lu\n", len, GetLastError() );
  if (len) ok( !strcmp( buffer, origdir ), "wrong result %s\n", buffer );
  SetLastError( 0xdeadbeef );
  strcpy( buffer, "foo" );
  len = GetCurrentDirectoryA( 65536, buffer );
  ok( (len != 0 && len < MAX_PATH), "GetCurrentDirectoryA failed %lu err %lu\n", len, GetLastError() );
  if (len) ok( !strcmp( buffer, origdir ), "wrong result %s\n", buffer );
  SetLastError( 0xdeadbeef );
  strcpy( buffer, "foo" );
  len = GetCurrentDirectoryA( 2 * 65536, buffer );
  ok( (len != 0 && len < MAX_PATH), "GetCurrentDirectoryA failed %lu err %lu\n", len, GetLastError() );
  if (len) ok( !strcmp( buffer, origdir ), "wrong result %s\n", buffer );
  HeapFree( GetProcessHeap(), 0, buffer );

/* Check for crash prevention on swapped args. Crashes all but Win9x.
*/
  if (0)
  {
      GetCurrentDirectoryA( 42, (LPSTR)(MAX_PATH + 42) );
  }

/* SetCurrentDirectoryA shouldn't care whether the string has a
   trailing '\\' or not
*/
  sprintf(tmpstr,"%s\\",newdir);
  test_setdir(origdir,tmpstr,newdir,1,"check 1");
  test_setdir(origdir,newdir,NULL,1,"check 2");
/* Set the directory to the working area.  We just tested that this works,
   so why check it again.
*/
  SetCurrentDirectoryA(newdir);
/* Check that SetCurrentDirectory fails when a nonexistent dir is specified */
  sprintf(tmpstr,"%s\\%s\\%s",newdir,SHORTDIR,NONDIR_SHORT);
  test_setdir(newdir,tmpstr,NULL,0,"check 3");
/* Check that SetCurrentDirectory fails for a nonexistent long directory */
  sprintf(tmpstr,"%s\\%s\\%s",newdir,SHORTDIR,NONDIR_LONG);
  test_setdir(newdir,tmpstr,NULL,0,"check 4");
/* Check that SetCurrentDirectory passes with a long directory */
  sprintf(tmpstr,"%s\\%s",newdir,LONGDIR);
  test_setdir(newdir,tmpstr,NULL,1,"check 5");
/* Check that SetCurrentDirectory passes with a short relative directory */
  sprintf(tmpstr,"%s",SHORTDIR);
  sprintf(tmpstr1,"%s\\%s",newdir,SHORTDIR);
  test_setdir(newdir,tmpstr,tmpstr1,1,"check 6");
/* starting with a '.' */
  sprintf(tmpstr,".\\%s",SHORTDIR);
  test_setdir(newdir,tmpstr,tmpstr1,1,"check 7");
/* Check that SetCurrentDirectory passes with a short relative directory */
  sprintf(tmpstr,"%s",LONGDIR);
  sprintf(tmpstr1,"%s\\%s",newdir,LONGDIR);
  test_setdir(newdir,tmpstr,tmpstr1,1,"check 8");
/* starting with a '.' */
  sprintf(tmpstr,".\\%s",LONGDIR);
  test_setdir(newdir,tmpstr,tmpstr1,1,"check 9");
/* change to root without a trailing backslash. The function call succeeds
   but the directory is not changed.
*/
  sprintf(tmpstr, "%c:", newdir[0]);
  test_setdir(newdir,tmpstr,newdir,1,"check 10");
/* works however with a trailing backslash */
  sprintf(tmpstr, "%c:\\", newdir[0]);
  test_setdir(newdir,tmpstr,NULL,1,"check 11");
}

/* Cleanup the mess we made while executing these tests */
static void test_CleanupPathA(CHAR *origdir, CHAR *curdir)
{
  CHAR tmpstr[MAX_PATH + 35];
  sprintf(tmpstr,"%s\\%s\\%s",curdir,SHORTDIR,SHORTFILE);
  ok(DeleteFileA(tmpstr),"DeleteFileA failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",curdir,SHORTDIR,LONGFILE);
  ok(DeleteFileA(tmpstr),"DeleteFileA failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",curdir,LONGDIR,SHORTFILE);
  ok(DeleteFileA(tmpstr),"DeleteFileA failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",curdir,LONGDIR,LONGFILE);
  ok(DeleteFileA(tmpstr),"DeleteFileA failed\n");
  sprintf(tmpstr,"%s\\%s",curdir,SHORTDIR);
  ok(RemoveDirectoryA(tmpstr),"RemoveDirectoryA failed\n");
  sprintf(tmpstr,"%s\\%s",curdir,LONGDIR);
  ok(RemoveDirectoryA(tmpstr),"RemoveDirectoryA failed\n");
  ok(SetCurrentDirectoryA(origdir),"SetCurrentDirectoryA failed\n");
  ok(RemoveDirectoryA(curdir),"RemoveDirectoryA failed\n");
}

/* test that short path name functions work regardless of case */
static void test_ShortPathCase(const char *tmpdir, const char *dirname,
                               const char *filename)
{
    char buf[MAX_PATH], shortbuf[MAX_PATH];
    HANDLE hndl;
    size_t i;

    assert(strlen(tmpdir) + strlen(dirname) + strlen(filename) + 2 < sizeof(buf));
    sprintf(buf,"%s\\%s\\%s",tmpdir,dirname,filename);
    GetShortPathNameA(buf,shortbuf,sizeof(shortbuf));
    hndl = CreateFileA(shortbuf,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    ok(hndl!=INVALID_HANDLE_VALUE,"CreateFileA failed (%ld)\n",GetLastError());
    CloseHandle(hndl);
    /* Now for the real test */
    for(i=0;i<strlen(shortbuf);i++)
        if (i % 2)
            shortbuf[i] = tolower(shortbuf[i]);
    hndl = CreateFileA(shortbuf,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    ok(hndl!=INVALID_HANDLE_VALUE,"CreateFileA failed (%ld)\n",GetLastError());
    CloseHandle(hndl);
}

/* This routine will test Get(Full|Short|Long)PathNameA */
static void test_PathNameA(CHAR *curdir, CHAR curDrive, CHAR otherDrive)
{
  CHAR curdir_short[MAX_PATH],
       longdir_short[MAX_PATH];
  CHAR tmpstr[MAX_PATH + 15],tmpstr1[MAX_PATH + 22],tmpstr2[2 * MAX_PATH + 15];
  LPSTR strptr;                 /*ptr to the filename portion of the path */
  DWORD len;
  INT i;
  CHAR dir[MAX_PATH],eight[MAX_PATH],three[MAX_PATH];
  SLpassfail passfail;
  DWORD rc1, rc2;

/* Get the short form of the current directory */
  ok((len=GetShortPathNameA(curdir,curdir_short,MAX_PATH)),
     "GetShortPathNameA failed\n");
  ok(!HAS_TRAIL_SLASH_A(curdir_short),
     "GetShortPathNameA should not have a trailing \\\n");
/* Get the short form of the absolute-path to LONGDIR */
  sprintf(tmpstr,"%s\\%s",curdir_short,LONGDIR);
  ok((len=GetShortPathNameA(tmpstr,longdir_short,MAX_PATH)),
     "GetShortPathNameA failed\n");
  ok(lstrcmpiA(longdir_short+(len-1),"\\")!=0,
     "GetShortPathNameA should not have a trailing \\\n");

  sprintf(tmpstr,"%s\\%s\\%s",curdir,LONGDIR,LONGFILE);
  rc1 = GetLongPathNameA(tmpstr, NULL, 0);
  rc2 = GetLongPathNameA(curdir, NULL, 0);
  ok((rc1-strlen(tmpstr))==(rc2-strlen(curdir)),
     "GetLongPathNameA: wrong return code, %ld instead of %d\n",
     rc1, lstrlenA(tmpstr)+1);

  sprintf(dir,"%c:",curDrive);
  rc1= GetLongPathNameA(dir, tmpstr, sizeof(tmpstr));
  ok(!strcmp(dir,tmpstr), "GetLongPathNameA: returned '%s' instead of '%s' (rc=%ld)\n",
       tmpstr, dir, rc1);

/* Check the cases where both file and directory exist first */
/* Start with a 8.3 directory, 8.3 filename */
  test_ValidPathA(curdir,SHORTDIR,SHORTFILE,tmpstr,NULL,"test1");
  sprintf(tmpstr1,"%s\\%s\\%s",curdir_short,SHORTDIR,SHORTFILE);
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
     "GetShortPathNameA returned '%s' instead of '%s'\n",tmpstr,tmpstr1);
/* Now try a 8.3 directory, long file name */
  test_ValidPathA(curdir,SHORTDIR,LONGFILE,tmpstr,NULL,"test2");
  sprintf(tmpstr1,"%s\\%s",curdir_short,SHORTDIR);
  test_LongtoShortA(tmpstr,tmpstr1,"PAT","test2");
/* Next is a long directory, 8.3 file */
  test_ValidPathA(curdir,LONGDIR,SHORTFILE,tmpstr,NULL,"test3");
  sprintf(tmpstr1,"%s\\%s",longdir_short,SHORTFILE);
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
     "GetShortPathNameA returned '%s' instead of '%s'\n",tmpstr,tmpstr1);
/*Lastly a long directory, long file */
  test_ValidPathA(curdir,LONGDIR,LONGFILE,tmpstr,NULL,"test4");
  test_LongtoShortA(tmpstr,longdir_short,"PAT","test4");

/* Now check all of the invalid file w/ valid directory combinations */
/* Start with a 8.3 directory, 8.3 filename */
  test_ValidPathA(curdir,SHORTDIR,NONFILE_SHORT,tmpstr,&passfail,"test5");
  sprintf(tmpstr1,"%s\\%s\\%s",curdir_short,SHORTDIR,NONFILE_SHORT);
  ok((passfail.shortlen==0 &&
      (passfail.shorterror==ERROR_PATH_NOT_FOUND ||
       passfail.shorterror==ERROR_FILE_NOT_FOUND)) ||
     (passfail.shortlen==strlen(tmpstr1) && lstrcmpiA(tmpstr,tmpstr1)==0),
     "GetShortPathNameA error: len=%ld error=%ld tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);

  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "Unexpected error %ld.\n", passfail.longerror);

/* Now try a 8.3 directory, long file name */
  test_ValidPathA(curdir,SHORTDIR,NONFILE_LONG,tmpstr,&passfail,"test6");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "Unexpected error %ld.\n", passfail.longerror);

/* Next is a long directory, 8.3 file */
  test_ValidPathA(curdir,LONGDIR,NONFILE_SHORT,tmpstr,&passfail,"test7");
  sprintf(tmpstr2,"%s\\%s",curdir_short,LONGDIR);
  GetShortPathNameA(tmpstr2,tmpstr1,MAX_PATH);
  strcat(tmpstr1,"\\" NONFILE_SHORT);
  ok((passfail.shortlen==0 &&
      (passfail.shorterror==ERROR_PATH_NOT_FOUND ||
       passfail.shorterror==ERROR_FILE_NOT_FOUND)) ||
     (passfail.shortlen==strlen(tmpstr1) && lstrcmpiA(tmpstr,tmpstr1)==0),
     "GetShortPathNameA error: len=%ld error=%ld tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "Unexpected error %ld.\n", passfail.longerror);

/*Lastly a long directory, long file */
  test_ValidPathA(curdir,LONGDIR,NONFILE_LONG,tmpstr,&passfail,"test8");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "Unexpected error %ld.\n", passfail.longerror);

/* Now try again with directories that don't exist */
/* 8.3 directory, 8.3 filename */
  test_ValidPathA(curdir,NONDIR_SHORT,SHORTFILE,tmpstr,&passfail,"test9");
  sprintf(tmpstr1,"%s\\%s\\%s",curdir_short,NONDIR_SHORT,SHORTFILE);
  ok((passfail.shortlen==0 &&
      (passfail.shorterror==ERROR_PATH_NOT_FOUND ||
       passfail.shorterror==ERROR_FILE_NOT_FOUND)) ||
     (passfail.shortlen==strlen(tmpstr1) && lstrcmpiA(tmpstr,tmpstr1)==0),
     "GetShortPathNameA error: len=%ld error=%ld tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_PATH_NOT_FOUND || passfail.longerror == ERROR_FILE_NOT_FOUND,
     "Unexpected error %ld.\n", passfail.longerror);

/* Now try a 8.3 directory, long file name */
  test_ValidPathA(curdir,NONDIR_SHORT,LONGFILE,tmpstr,&passfail,"test10");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %ld and not 'ERROR_PATH_NOT_FOUND'\n",
      passfail.shorterror);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_PATH_NOT_FOUND || passfail.longerror == ERROR_FILE_NOT_FOUND,
     "Unexpected error %ld.\n", passfail.longerror);

/* Next is a long directory, 8.3 file */
  test_ValidPathA(curdir,NONDIR_LONG,SHORTFILE,tmpstr,&passfail,"test11");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %ld and not 'ERROR_PATH_NOT_FOUND'\n",
      passfail.shorterror);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_PATH_NOT_FOUND || passfail.longerror == ERROR_FILE_NOT_FOUND,
     "Unexpected error %ld.\n", passfail.longerror);

/*Lastly a long directory, long file */
  test_ValidPathA(curdir,NONDIR_LONG,LONGFILE,tmpstr,&passfail,"test12");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %ld and not 'ERROR_PATH_NOT_FOUND'\n",
      passfail.shorterror);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_PATH_NOT_FOUND || passfail.longerror == ERROR_FILE_NOT_FOUND,
     "Unexpected error %ld.\n", passfail.longerror);

/* Next try directories ending with '\\' */
/* Existing Directories */
  sprintf(tmpstr,"%s\\",SHORTDIR);
  test_ValidPathA(curdir,"",tmpstr,tmpstr1,NULL,"test13");
  sprintf(tmpstr,"%s\\",LONGDIR);
  test_ValidPathA(curdir,"",tmpstr,tmpstr1,NULL,"test14");
/* Nonexistent directories */
  sprintf(tmpstr,"%s\\",NONDIR_SHORT);
  test_ValidPathA(curdir,"",tmpstr,tmpstr1,&passfail,"test15");
  sprintf(tmpstr2,"%s\\%s",curdir_short,tmpstr);
  ok((passfail.shortlen==0 &&
      (passfail.shorterror==ERROR_PATH_NOT_FOUND ||
       passfail.shorterror==ERROR_FILE_NOT_FOUND)) ||
     (passfail.shortlen==strlen(tmpstr2) && lstrcmpiA(tmpstr1,tmpstr2)==0),
     "GetShortPathNameA error: len=%ld error=%ld tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "Unexpected error %ld.\n", passfail.longerror);

  sprintf(tmpstr,"%s\\",NONDIR_LONG);
  test_ValidPathA(curdir,"",tmpstr,tmpstr1,&passfail,"test16");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %ld and not 'ERROR_FILE_NOT_FOUND'\n",
      passfail.shorterror);
  ok(!passfail.longlen, "GetLongPathNameA passed when it shouldn't have\n");
  ok(passfail.longerror == ERROR_FILE_NOT_FOUND, "Unexpected error %ld.\n", passfail.longerror);

/* Test GetFullPathNameA with drive letters */
  if( curDrive != NOT_A_VALID_DRIVE) {
    sprintf(tmpstr,"%c:",curdir[0]);
    ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr2,&strptr),
       "GetFullPathNameA(%c:) failed\n", curdir[0]);
    GetCurrentDirectoryA(MAX_PATH,tmpstr);
    sprintf(tmpstr1,"%s\\",tmpstr);
    ok(lstrcmpiA(tmpstr,tmpstr2)==0 || lstrcmpiA(tmpstr1,tmpstr2)==0,
       "GetFullPathNameA(%c:) returned '%s' instead of '%s' or '%s'\n",
       curdir[0],tmpstr2,tmpstr,tmpstr1);

    sprintf(tmpstr,"%c:\\%s\\%s",curDrive,SHORTDIR,SHORTFILE);
    ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr1,&strptr),"GetFullPathNameA failed\n");
    ok(lstrcmpiA(tmpstr,tmpstr1)==0,
       "GetFullPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
    ok(lstrcmpiA(SHORTFILE,strptr)==0,
       "GetFullPathNameA returned part '%s' instead of '%s'\n",strptr,SHORTFILE);
  }
/* Without a leading slash, insert the current directory if on the current drive */
  sprintf(tmpstr,"%c:%s\\%s",curdir[0],SHORTDIR,SHORTFILE);
  ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr1,&strptr),"GetFullPathNameA failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",curdir,SHORTDIR,SHORTFILE);
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
      "GetFullPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  ok(lstrcmpiA(SHORTFILE,strptr)==0,
      "GetFullPathNameA returned part '%s' instead of '%s'\n",strptr,SHORTFILE);
/* Otherwise insert the missing leading slash */
  if( otherDrive != NOT_A_VALID_DRIVE) {
    /* FIXME: this test assumes that current directory on other drive is root */
    sprintf(tmpstr,"%c:%s\\%s",otherDrive,SHORTDIR,SHORTFILE);
    ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr1,&strptr),"GetFullPathNameA failed for %s\n", tmpstr);
    sprintf(tmpstr,"%c:\\%s\\%s",otherDrive,SHORTDIR,SHORTFILE);
    ok(lstrcmpiA(tmpstr,tmpstr1)==0,
       "GetFullPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
    ok(lstrcmpiA(SHORTFILE,strptr)==0,
       "GetFullPathNameA returned part '%s' instead of '%s'\n",strptr,SHORTFILE);
  }
/* Xilinx tools like to mix Unix and DOS formats, which Windows handles fine.
   So test for them. */
  if( curDrive != NOT_A_VALID_DRIVE) {
    sprintf(tmpstr,"%c:/%s\\%s",curDrive,SHORTDIR,SHORTFILE);
    ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr1,&strptr),"GetFullPathNameA failed\n");
    sprintf(tmpstr,"%c:\\%s\\%s",curDrive,SHORTDIR,SHORTFILE);
    ok(lstrcmpiA(tmpstr,tmpstr1)==0,
       "GetFullPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
    ok(lstrcmpiA(SHORTFILE,strptr)==0,
       "GetFullPathNameA returned part '%s' instead of '%s'\n",strptr,SHORTFILE);
  }
  /* Don't Starve relies on GetLongPathName returning the passed in filename,
     even if the actual file on disk has a different case or separator */
  len = lstrlenA(LONGDIR) + 1;
  sprintf(tmpstr,"%s/%s",LONGDIR,LONGFILE);
  ok(GetLongPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetLongPathNameA failed\n");
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
     "GetLongPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  tmpstr[len] = tolower(tmpstr[len]);
  ok(GetLongPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetLongPathNameA failed\n");
  ok(lstrcmpA(tmpstr,tmpstr1)==0,
     "GetLongPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  sprintf(tmpstr,"%s/%s",SHORTDIR,SHORTFILE);
  ok(GetLongPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetLongPathNameA failed\n");
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
     "GetLongPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  len = lstrlenA(SHORTDIR) + 1;
  tmpstr[len] = toupper(tmpstr[len]);
  ok(GetLongPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetLongPathNameA failed\n");
  ok(lstrcmpiA(tmpstr,tmpstr1)==0 && lstrcmpA(tmpstr,tmpstr1) != 0,
     "GetLongPathNameA returned '%s' instead of '%s/%s'\n",tmpstr1,SHORTDIR,SHORTFILE);

  sprintf(tmpstr,"%s/%s",SHORTDIR,SHORTFILE);
  ok(GetShortPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetShortPathNameA failed\n");
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
       "GetShortPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);

/**/
  sprintf(tmpstr,"%c:%s/%s",curdir[0],SHORTDIR,SHORTFILE);
  ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr1,&strptr),"GetFullPathNameA failed\n");
  sprintf(tmpstr,"%s\\%s\\%s",curdir,SHORTDIR,SHORTFILE);
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
      "GetFullPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  ok(lstrcmpiA(SHORTFILE,strptr)==0,
      "GetFullPathNameA returned part '%s' instead of '%s'\n",strptr,SHORTFILE);
/* Windows will insert a drive letter in front of an absolute UNIX path */
  sprintf(tmpstr,"/%s/%s",SHORTDIR,SHORTFILE);
  ok(GetFullPathNameA(tmpstr,MAX_PATH,tmpstr1,&strptr),"GetFullPathNameA failed\n");
  sprintf(tmpstr,"%c:\\%s\\%s",*tmpstr1,SHORTDIR,SHORTFILE);
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
     "GetFullPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
/* This passes in Wine because it still contains the pointer from the previous test */
  ok(lstrcmpiA(SHORTFILE,strptr)==0,
      "GetFullPathNameA returned part '%s' instead of '%s'\n",strptr,SHORTFILE);

/* Now try some relative paths */
  ok(GetShortPathNameA(LONGDIR,tmpstr,MAX_PATH),"GetShortPathNameA failed\n");
  test_SplitShortPathA(tmpstr,dir,eight,three);
  ok(GetLongPathNameA(tmpstr, tmpstr1, MAX_PATH), "GetLongPathNameA failed\n");
  ok(!lstrcmpiA(tmpstr1, LONGDIR), "GetLongPathNameA returned '%s' instead of '%s'\n", tmpstr1, LONGDIR);

  sprintf(tmpstr,".\\%s",LONGDIR);
  ok(GetShortPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetShortPathNameA failed\n");
  test_SplitShortPathA(tmpstr1,dir,eight,three);
  ok(lstrcmpiA(dir,".")==0 || dir[0]=='\0',
     "GetShortPathNameA did not keep relative directory [%s]\n",tmpstr1);
  ok(GetLongPathNameA(tmpstr1, tmpstr1, MAX_PATH), "GetLongPathNameA failed %s\n", tmpstr);
  ok(!lstrcmpiA(tmpstr1, tmpstr), "GetLongPathNameA returned '%s' instead of '%s'\n", tmpstr1, tmpstr);

/* Check out Get*PathNameA on some funny characters */
  for(i=0;i<lstrlenA(funny_chars);i++) {
    INT valid;
    valid=(is_char_ok[i]=='0') ? 0 : 1;
    sprintf(tmpstr1,"check%d-1",i);
    sprintf(tmpstr,"file%c000.ext",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
    sprintf(tmpstr1,"check%d-2",i);
    sprintf(tmpstr,"file000.e%ct",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
    sprintf(tmpstr1,"check%d-3",i);
    sprintf(tmpstr,"%cfile000.ext",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
    sprintf(tmpstr1,"check%d-4",i);
    sprintf(tmpstr,"file000%c.ext",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
    sprintf(tmpstr1,"check%d-5",i);
    sprintf(tmpstr,"Long %c File",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
    sprintf(tmpstr1,"check%d-6",i);
    sprintf(tmpstr,"%c Long File",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
    sprintf(tmpstr1,"check%d-7",i);
    sprintf(tmpstr,"Long File %c",funny_chars[i]);
    test_FunnyChars(curdir,curdir_short,tmpstr,valid,tmpstr1);
  }
  /* Now try it on mixed case short names */
  test_ShortPathCase(curdir,SHORTDIR,LONGFILE);
  test_ShortPathCase(curdir,LONGDIR,SHORTFILE);
  test_ShortPathCase(curdir,LONGDIR,LONGFILE);

  /* test double delimiters */
  sprintf(tmpstr,"%s\\\\%s", SHORTDIR,SHORTFILE);
  ok(GetShortPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetShortPathNameA failed\n");
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
       "GetShortPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  sprintf(tmpstr,".\\\\%s\\\\%s", SHORTDIR,SHORTFILE);
  ok(GetShortPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetShortPathNameA failed\n");
  ok(lstrcmpiA(tmpstr,tmpstr1)==0,
       "GetShortPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);

  sprintf(tmpstr,"%s\\\\%s",LONGDIR,LONGFILE);
  ok(GetLongPathNameA(tmpstr, tmpstr1, MAX_PATH), "GetLongPathNameA failed\n");
  ok(!lstrcmpiA(tmpstr,tmpstr1), "GetLongPathNameA returned '%s' instead of '%s'\n", tmpstr1, tmpstr);

  sprintf(tmpstr,".\\\\%s\\\\%s",LONGDIR,LONGFILE);
  ok(GetLongPathNameA(tmpstr, tmpstr1, MAX_PATH), "GetLongPathNameA failed\n");
  ok(!lstrcmpiA(tmpstr,tmpstr1), "GetLongPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
}

static void test_GetTempPathA(char* tmp_dir)
{
    DWORD len, slen, len_with_null;
    char buf[MAX_PATH];

    len_with_null = strlen(tmp_dir) + 1;

    lstrcpyA(buf, "foo");
    len = GetTempPathA(MAX_PATH, buf);
    ok(len <= MAX_PATH, "should fit into MAX_PATH\n");
    ok(lstrcmpiA(buf, tmp_dir) == 0, "expected [%s], got [%s]\n",tmp_dir,buf);
    ok(len == strlen(buf), "returned length should be equal to the length of string\n");

    /* Some versions of Windows touch the buffer, some don't so we don't
     * test that. Also, NT sometimes exaggerates the required buffer size
     * so we cannot test for an exact match. Finally, the
     * 'len_with_null - 1' case is so buggy on Windows it's not testable.
     * For instance in some cases Win98 returns len_with_null - 1 instead
     * of len_with_null.
     */
    len = GetTempPathA(1, buf);
    ok(len >= len_with_null, "Expected >= %lu, got %lu\n", len_with_null, len);

    len = GetTempPathA(0, NULL);
    ok(len >= len_with_null, "Expected >= %lu, got %lu\n", len_with_null, len);

    /* The call above gave us the buffer size that Windows thinks is needed
     * so the next call should work
     */
    lstrcpyA(buf, "foo");
    len = GetTempPathA(len, buf);
    ok(lstrcmpiA(buf, tmp_dir) == 0, "expected [%s], got [%s]\n",tmp_dir,buf);
    ok(len == strlen(buf), "returned length should be equal to the length of string\n");

    memset(buf, 'a', sizeof(buf));
    len = GetTempPathA(sizeof(buf), buf);
    ok(lstrcmpiA(buf, tmp_dir) == 0, "expected [%s], got [%s]\n",tmp_dir,buf);
    ok(len == strlen(buf), "returned length should be equal to the length of string\n");
    /* The rest of the buffer remains untouched */
    slen = len + 1;
    for(len++; len < sizeof(buf); len++)
        ok(buf[len] == 'a', "expected 'a' at [%ld], got 0x%x\n", len, buf[len]);

    /* When the buffer is not long enough it remains untouched */
    memset(buf, 'a', sizeof(buf));
    len = GetTempPathA(slen / 2, buf);
    ok(len == slen || broken(len == slen + 1) /* read the big comment above */ ,
       "expected %ld, got %ld\n", slen, len);
    for(len = 0; len < ARRAY_SIZE(buf); len++)
        ok(buf[len] == 'a', "expected 'a' at [%ld], got 0x%x\n", len, buf[len]);
}

static void test_GetTempPathW(char* tmp_dir)
{
    DWORD len, slen, len_with_null;
    WCHAR buf[MAX_PATH], *long_buf;
    WCHAR tmp_dirW[MAX_PATH];
    static const WCHAR fooW[] = {'f','o','o',0};

    MultiByteToWideChar(CP_ACP, 0, tmp_dir, -1, tmp_dirW, ARRAY_SIZE(tmp_dirW));
    len_with_null = lstrlenW(tmp_dirW) + 1;

    /* This one is different from ANSI version: ANSI version doesn't
     * touch the buffer, unicode version usually truncates the buffer
     * to zero size. NT still exaggerates the required buffer size
     * sometimes so we cannot test for an exact match. Finally, the
     * 'len_with_null - 1' case is so buggy on Windows it's not testable.
     * For instance on NT4 it will sometimes return a path without the
     * trailing '\\' and sometimes return an error.
     */

    lstrcpyW(buf, fooW);
    len = GetTempPathW(MAX_PATH, buf);
    ok(lstrcmpiW(buf, tmp_dirW) == 0, "GetTempPathW returned an incorrect temporary path\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");

    lstrcpyW(buf, fooW);
    len = GetTempPathW(1, buf);
    ok(buf[0] == 0, "unicode version should truncate the buffer to zero size\n");
    ok(len >= len_with_null, "Expected >= %lu, got %lu\n", len_with_null, len);

    len = GetTempPathW(0, NULL);
    ok(len >= len_with_null, "Expected >= %lu, got %lu\n", len_with_null, len);

    lstrcpyW(buf, fooW);
    len = GetTempPathW(len, buf);
    ok(lstrcmpiW(buf, tmp_dirW) == 0, "GetTempPathW returned an incorrect temporary path\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");

    for(len = 0; len < ARRAY_SIZE(buf); len++)
        buf[len] = 'a';
    len = GetTempPathW(len, buf);
    ok(lstrcmpiW(buf, tmp_dirW) == 0, "GetTempPathW returned an incorrect temporary path\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");
    /* The rest of the buffer must be zeroed */
    slen = len + 1;
    for(len++; len < ARRAY_SIZE(buf); len++)
        ok(buf[len] == '\0', "expected NULL at [%ld], got 0x%x\n", len, buf[len]);

    /* When the buffer is not long enough the length passed is zeroed */
    for(len = 0; len < ARRAY_SIZE(buf); len++)
        buf[len] = 'a';
    len = GetTempPathW(slen / 2, buf);
    ok(len == slen || broken(len == slen + 1) /* read the big comment above */ ,
       "expected %ld, got %ld\n", slen, len);

    {
        /* In Windows 8 when TMP var points to a drive only (like C:) instead of a
        * full directory the behavior changes. It will start filling the path but
        * will later truncate the buffer before returning. So the generic test
        * below will fail for this Windows 8 corner case.
        */
        char tmp_var[64];
        DWORD version = GetVersion();
        GetEnvironmentVariableA("TMP", tmp_var, sizeof(tmp_var));
        if (strlen(tmp_var) == 2 && version >= 0x00060002)
            return;
    }

    for(len = 0; len < slen / 2; len++)
        ok(buf[len] == '\0', "expected NULL at [%ld], got 0x%x\n", len, buf[len]);
    for(; len < ARRAY_SIZE(buf); len++)
        ok(buf[len] == 'a', "expected 'a' at [%ld], got 0x%x\n", len, buf[len]);

    /* bogus application from bug 38220 passes the count value in sizeof(buffer)
     * instead the correct count of WCHAR, this test catches this case. */
    slen = 65534;
    long_buf = HeapAlloc(GetProcessHeap(), 0, slen * sizeof(WCHAR));
    if (!long_buf)
    {
        skip("Could not allocate memory for the test\n");
        return;
    }
    for(len = 0; len < slen; len++)
        long_buf[len] = 0xCC;
    len = GetTempPathW(slen, long_buf);
    ok(lstrcmpiW(long_buf, tmp_dirW) == 0, "GetTempPathW returned an incorrect temporary path\n");
    ok(len == lstrlenW(long_buf), "returned length should be equal to the length of string\n");
    /* the remaining buffer must be zeroed up to different values in different OS versions.
     * <= XP - 32766
     *  > XP - 32767
     * to simplify testing we will test only until XP.
     */
    for(; len < 32767; len++)
        ok(long_buf[len] == '\0', "expected NULL at [%ld], got 0x%x\n", len, long_buf[len]);
    /* we will know skip the test that is in the middle of the OS difference by
     * incrementing len and then resume the test for the untouched part. */
    for(len++; len < slen; len++)
        ok(long_buf[len] == 0xcc, "expected 0xcc at [%ld], got 0x%x\n", len, long_buf[len]);

    HeapFree(GetProcessHeap(), 0, long_buf);
}

static void test_GetTempPath(void)
{
    char save_TMP[MAX_PATH];
    char windir[MAX_PATH];
    char buf[MAX_PATH];
    WCHAR curdir[MAX_PATH];

    if (!GetEnvironmentVariableA("TMP", save_TMP, sizeof(save_TMP))) save_TMP[0] = 0;

    /* test default configuration */
    trace("TMP=%s\n", save_TMP);
    if (save_TMP[0])
    {
        strcpy(buf,save_TMP);
        if (buf[strlen(buf)-1]!='\\')
            strcat(buf,"\\");
        test_GetTempPathA(buf);
        test_GetTempPathW(buf);
    }

    /* TMP=C:\WINDOWS */
    GetWindowsDirectoryA(windir, sizeof(windir));
    SetEnvironmentVariableA("TMP", windir);
    GetEnvironmentVariableA("TMP", buf, sizeof(buf));
    trace("TMP=%s\n", buf);
    strcat(windir,"\\");
    test_GetTempPathA(windir);
    test_GetTempPathW(windir);

    /* TMP=C:\ */
    GetWindowsDirectoryA(windir, sizeof(windir));
    windir[3] = 0;
    SetEnvironmentVariableA("TMP", windir);
    GetEnvironmentVariableA("TMP", buf, sizeof(buf));
    trace("TMP=%s\n", buf);
    test_GetTempPathA(windir);
    test_GetTempPathW(windir);

    GetCurrentDirectoryW(MAX_PATH, curdir);
    /* TMP=C: i.e. use current working directory of the specified drive */
    GetWindowsDirectoryA(windir, sizeof(windir));
    SetCurrentDirectoryA(windir);
    windir[2] = 0;
    SetEnvironmentVariableA("TMP", windir);
    GetEnvironmentVariableA("TMP", buf, sizeof(buf));
    trace("TMP=%s\n", buf);
    GetWindowsDirectoryA(windir, sizeof(windir));
    strcat(windir,"\\");
    test_GetTempPathA(windir);
    test_GetTempPathW(windir);

    SetEnvironmentVariableA("TMP", save_TMP);
    SetCurrentDirectoryW(curdir);
}

static void test_GetLongPathNameA(void)
{
    DWORD length, explength, hostsize;
    char tempfile[MAX_PATH], *name;
    char longpath[MAX_PATH];
    char unc_prefix[MAX_PATH];
    char unc_short[MAX_PATH], unc_long[MAX_PATH];
    char temppath[MAX_PATH], temppath2[MAX_PATH];
    HANDLE file;

    GetTempPathA(MAX_PATH, tempfile);
    name = tempfile + strlen(tempfile);

    strcpy(name, "*");
    SetLastError(0xdeadbeef);
    length = GetLongPathNameA(tempfile, temppath, MAX_PATH);
    ok(!length, "GetLongPathNameA should fail\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "wrong error %ld\n", GetLastError());

    strcpy(name, "longfilename.longext");

    file = CreateFileA(tempfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(file);

    /* Test a normal path with a small buffer size */
    memset(temppath, 0, MAX_PATH);
    length = GetLongPathNameA(tempfile, temppath, 4);
    /* We have a failure so length should be the minimum plus the terminating '0'  */
    ok(length >= strlen(tempfile) + 1, "Wrong length\n");
    ok(temppath[0] == 0, "Buffer should not have been touched\n");

    /* Some UNC syntax tests */

    memset(temppath, 0, MAX_PATH);
    memset(temppath2, 0, MAX_PATH);
    lstrcpyA(temppath2, "\\\\?\\");
    lstrcatA(temppath2, tempfile);
    explength = length + 4;

    SetLastError(0xdeadbeef);
    length = GetLongPathNameA(temppath2, NULL, 0);
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);

    length = GetLongPathNameA(temppath2, NULL, MAX_PATH);
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);

    length = GetLongPathNameA(temppath2, temppath, 4);
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);
    ok(temppath[0] == 0, "Buffer should not have been touched\n");

    /* Now an UNC path with the computername */
    lstrcpyA(unc_prefix, "\\\\");
    hostsize = sizeof(unc_prefix) - 2;
    GetComputerNameA(unc_prefix + 2, &hostsize);
    lstrcatA(unc_prefix, "\\");

    /* Create a short syntax for the whole unc path */
    memset(unc_short, 0, MAX_PATH);
    GetShortPathNameA(tempfile, temppath, MAX_PATH);
    lstrcpyA(unc_short, unc_prefix);
    unc_short[lstrlenA(unc_short)] = temppath[0];
    lstrcatA(unc_short, "$\\");
    lstrcatA(unc_short, strchr(temppath, '\\') + 1);

    /* Create a long syntax for reference */
    memset(longpath, 0, MAX_PATH);
    GetLongPathNameA(tempfile, temppath, MAX_PATH);
    lstrcpyA(longpath, unc_prefix);
    longpath[lstrlenA(longpath)] = temppath[0];
    lstrcatA(longpath, "$\\");
    lstrcatA(longpath, strchr(temppath, '\\') + 1);

    /* NULL test */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameA(unc_short, NULL, 0);
    if (length == 0 && GetLastError() == ERROR_BAD_NETPATH)
    {
        /* Seen on Window XP Home */
        win_skip("UNC with computername is not supported\n");
        DeleteFileA(tempfile);
        return;
    }
    explength = lstrlenA(longpath) + 1;
    todo_wine
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);

    length = GetLongPathNameA(unc_short, NULL, MAX_PATH);
    todo_wine
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);

    memset(unc_long, 0, MAX_PATH);
    length = GetLongPathNameA(unc_short, unc_long, lstrlenA(unc_short));
    /* length will include terminating '0' on failure */
    todo_wine
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);
    ok(unc_long[0] == 0, "Buffer should not have been touched\n");

    memset(unc_long, 0, MAX_PATH);
    length = GetLongPathNameA(unc_short, unc_long, length);
    /* length doesn't include terminating '0' on success */
    explength--;
    todo_wine
    {
    ok(length == explength, "Wrong length %ld, expected %ld\n", length, explength);
    ok(!lstrcmpiA(unc_long, longpath), "Expected (%s), got (%s)\n", longpath, unc_long);
    }

    DeleteFileA(tempfile);
}

static void test_GetLongPathNameW(void)
{
    DWORD length, expanded;
    BOOL ret;
    HANDLE file;
    WCHAR empty[MAX_PATH];
    WCHAR tempdir[MAX_PATH], name[200];
    WCHAR dirpath[4 + MAX_PATH + 200]; /* To ease removal */
    WCHAR shortpath[4 + MAX_PATH + 200 + 1 + 200];
    static const WCHAR prefix[] = { '\\','\\','?','\\', 0};
    static const WCHAR backslash[] = { '\\', 0};
    static const WCHAR letterX[] = { 'X', 0};

    SetLastError(0xdeadbeef); 
    length = GetLongPathNameW(NULL,NULL,0);
    ok(0==length,"GetLongPathNameW returned %ld but expected 0\n",length);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"GetLastError returned %ld but expected ERROR_INVALID_PARAMETER\n",GetLastError());

    SetLastError(0xdeadbeef); 
    empty[0]=0;
    length = GetLongPathNameW(empty,NULL,0);
    ok(0==length,"GetLongPathNameW returned %ld but expected 0\n",length);
    ok(GetLastError()==ERROR_PATH_NOT_FOUND,"GetLastError returned %ld but expected ERROR_PATH_NOT_FOUND\n",GetLastError());

    /* Create a long path name. The path needs to exist for these tests to
     * succeed so we need the "\\?\" prefix when creating directories and
     * files.
     */
    name[0] = 0;
    while (lstrlenW(name) < (ARRAY_SIZE(name) - 1))
        lstrcatW(name, letterX);

    GetTempPathW(MAX_PATH, tempdir);

    lstrcpyW(shortpath, prefix);
    lstrcatW(shortpath, tempdir);
    lstrcatW(shortpath, name);
    lstrcpyW(dirpath, shortpath);
    ret = CreateDirectoryW(shortpath, NULL);
    ok(ret, "Could not create the temporary directory : %ld\n", GetLastError());
    lstrcatW(shortpath, backslash);
    lstrcatW(shortpath, name);

    /* Path does not exist yet and we know it overruns MAX_PATH */

    /* No prefix */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath + 4, NULL, 0);
    ok(length == 0, "Expected 0, got %ld\n", length);
    todo_wine
    ok(GetLastError() == ERROR_PATH_NOT_FOUND,
       "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    /* With prefix */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath, NULL, 0);
    todo_wine
    {
    ok(length == 0, "Expected 0, got %ld\n", length);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    }

    file = CreateFileW(shortpath, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE,
       "Could not create the temporary file : %ld.\n", GetLastError());
    CloseHandle(file);

    /* Path exists */

    /* No prefix */
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath + 4, NULL, 0);
    todo_wine
    {
    ok(length == 0, "Expected 0, got %ld\n", length);
    ok(GetLastError() == ERROR_PATH_NOT_FOUND, "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());
    }
    /* With prefix */
    expanded = 4 + (GetLongPathNameW(tempdir, NULL, 0) - 1) + lstrlenW(name) + 1 + lstrlenW(name) + 1;
    SetLastError(0xdeadbeef);
    length = GetLongPathNameW(shortpath, NULL, 0);
    ok(length == expanded, "Expected %ld, got %ld\n", expanded, length);

    /* NULL buffer with length crashes on Windows */
    if (0)
        GetLongPathNameW(shortpath, NULL, 20);

    ok(DeleteFileW(shortpath), "Could not delete temporary file\n");
    ok(RemoveDirectoryW(dirpath), "Could not delete temporary directory\n");
}

static void test_GetShortPathNameW(void)
{
    static const WCHAR extended_prefix[] = {'\\','\\','?','\\',0};
    static const WCHAR test_path[] = { 'L', 'o', 'n', 'g', 'D', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', 'N', 'a', 'm', 'e',  0 };
    static const WCHAR name[] = { 't', 'e', 's', 't', 0 };
    static const WCHAR backSlash[] = { '\\', 0 };
    static const WCHAR a_bcdeW[] = {'a','.','b','c','d','e',0};
    static const WCHAR wildW[] = { '*',0 };
    WCHAR path[MAX_PATH], tmppath[MAX_PATH], *ptr;
    WCHAR short_path[MAX_PATH];
    DWORD length;
    HANDLE file;
    int ret;

    GetTempPathW( MAX_PATH, tmppath );

    lstrcpyW( path, tmppath );
    lstrcatW( path, test_path );
    lstrcatW( path, backSlash );
    ret = CreateDirectoryW( path, NULL );
    ok( ret, "Directory was not created. LastError = %ld\n", GetLastError() );

    /* Starting a main part of test */

    /* extended path \\?\C:\path\ */
    lstrcpyW( path, extended_prefix );
    lstrcatW( path, tmppath );
    lstrcatW( path, test_path );
    lstrcatW( path, backSlash );
    short_path[0] = 0;
    length = GetShortPathNameW( path, short_path, ARRAY_SIZE( short_path ));
    ok( length, "GetShortPathNameW returned 0.\n" );

    lstrcpyW( path, tmppath );
    lstrcatW( path, test_path );
    lstrcatW( path, backSlash );
    length = GetShortPathNameW( path, short_path, 0 );
    ok( length, "GetShortPathNameW returned 0.\n" );
    ret = GetShortPathNameW( path, short_path, length );
    ok( ret && ret == length-1, "GetShortPathNameW returned 0.\n" );

    lstrcatW( short_path, name );

    /* GetShortPathName for a non-existent short file name should fail */
    SetLastError(0xdeadbeef);
    length = GetShortPathNameW( short_path, path, 0 );
    ok(!length, "GetShortPathNameW should fail\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    file = CreateFileW( short_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok( file != INVALID_HANDLE_VALUE, "File was not created.\n" );
    CloseHandle( file );
    ret = DeleteFileW( short_path );
    ok( ret, "Cannot delete file.\n" );

    ptr = path + lstrlenW(path);
    lstrcpyW( ptr, a_bcdeW);
    file = CreateFileW( path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok( file != INVALID_HANDLE_VALUE, "File was not created.\n" );
    CloseHandle( file );

    length = GetShortPathNameW( path, short_path, ARRAY_SIZE( short_path ));
    ok( length, "GetShortPathNameW failed: %lu.\n", GetLastError() );

    lstrcpyW(ptr, wildW);
    SetLastError(0xdeadbeef);
    length = GetShortPathNameW( path, short_path, ARRAY_SIZE( short_path ) );
    ok(!length, "GetShortPathNameW should fail\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "wrong error %ld\n", GetLastError());

    lstrcpyW(ptr, a_bcdeW);
    ret = DeleteFileW( path );
    ok( ret, "Cannot delete file.\n" );
    *ptr = 0;

    /* End test */
    ret = RemoveDirectoryW( path );
    ok( ret, "Cannot delete directory.\n" );
}

static void test_GetSystemDirectory(void)
{
    CHAR    buffer[MAX_PATH + 4];
    DWORD   res;
    DWORD   total;

    SetLastError(0xdeadbeef);
    res = GetSystemDirectoryA(NULL, 0);
    /* res includes the terminating Zero */
    ok(res > 0, "returned %ld with %ld (expected '>0')\n", res, GetLastError());

    total = res;

    /* this crashes on XP */
    if (0)
        GetSystemDirectoryA(NULL, total);

    SetLastError(0xdeadbeef);
    res = GetSystemDirectoryA(NULL, total-1);
    ok( res == total, "returned %lu with %lu (expected '%lu')\n",
        res, GetLastError(), total );

    if (total > MAX_PATH) return;

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectoryA(buffer, total);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %ld with %ld and '%s' (expected '%ld' and a string)\n",
        res, GetLastError(), buffer, total-1);

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectoryA(buffer, total + 1);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %ld with %ld and '%s' (expected '%ld' and a string)\n",
        res, GetLastError(), buffer, total-1);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectoryA(buffer, total-1);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %ld with %ld and '%s' (expected '%ld')\n",
        res, GetLastError(), buffer, total);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectoryA(buffer, total-2);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %ld with %ld and '%s' (expected '%ld')\n",
        res, GetLastError(), buffer, total);
}

static void test_GetWindowsDirectory(void)
{
    CHAR    buffer[MAX_PATH + 4];
    DWORD   res;
    DWORD   total;

    SetLastError(0xdeadbeef);
    res = GetWindowsDirectoryA(NULL, 0);
    /* res includes the terminating Zero */
    ok(res > 0, "returned %ld with %ld (expected '>0')\n", res, GetLastError());

    total = res;
    /* this crashes on XP */
    if (0)
        GetWindowsDirectoryA(NULL, total);

    SetLastError(0xdeadbeef);
    res = GetWindowsDirectoryA(NULL, total-1);
    ok( res == total, "returned %lu with %lu (expected '%lu')\n",
        res, GetLastError(), total );

    if (total > MAX_PATH) return;

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectoryA(buffer, total);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %ld with %ld and '%s' (expected '%ld' and a string)\n",
        res, GetLastError(), buffer, total-1);

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectoryA(buffer, total + 1);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %ld with %ld and '%s' (expected '%ld' and a string)\n",
        res, GetLastError(), buffer, total-1);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectoryA(buffer, total-1);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %ld with %ld and '%s' (expected '%ld')\n",
        res, GetLastError(), buffer, total);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectoryA(buffer, total-2);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %ld with %ld and '%s' (expected '%ld')\n",
        res, GetLastError(), buffer, total);
}

static void test_NeedCurrentDirectoryForExePathA(void)
{
    if (!pNeedCurrentDirectoryForExePathA)
    {
        win_skip("NeedCurrentDirectoryForExePathA is not available\n");
        return;
    }

    /* Crashes in Windows */
    if (0)
        pNeedCurrentDirectoryForExePathA(NULL);

    SetEnvironmentVariableA("NoDefaultCurrentDirectoryInExePath", NULL);
    ok(pNeedCurrentDirectoryForExePathA("."), "returned FALSE for \".\"\n");
    ok(pNeedCurrentDirectoryForExePathA("c:\\"), "returned FALSE for \"c:\\\"\n");
    ok(pNeedCurrentDirectoryForExePathA("cmd.exe"), "returned FALSE for \"cmd.exe\"\n");

    SetEnvironmentVariableA("NoDefaultCurrentDirectoryInExePath", "nya");
    ok(!pNeedCurrentDirectoryForExePathA("."), "returned TRUE for \".\"\n");
    ok(pNeedCurrentDirectoryForExePathA("c:\\"), "returned FALSE for \"c:\\\"\n");
    ok(!pNeedCurrentDirectoryForExePathA("cmd.exe"), "returned TRUE for \"cmd.exe\"\n");
}

static void test_NeedCurrentDirectoryForExePathW(void)
{
    const WCHAR thispath[] = {'.', 0};
    const WCHAR fullpath[] = {'c', ':', '\\', 0};
    const WCHAR cmdname[] = {'c', 'm', 'd', '.', 'e', 'x', 'e', 0};

    if (!pNeedCurrentDirectoryForExePathW)
    {
        win_skip("NeedCurrentDirectoryForExePathW is not available\n");
        return;
    }

    /* Crashes in Windows */
    if (0)
        pNeedCurrentDirectoryForExePathW(NULL);

    SetEnvironmentVariableA("NoDefaultCurrentDirectoryInExePath", NULL);
    ok(pNeedCurrentDirectoryForExePathW(thispath), "returned FALSE for \".\"\n");
    ok(pNeedCurrentDirectoryForExePathW(fullpath), "returned FALSE for \"c:\\\"\n");
    ok(pNeedCurrentDirectoryForExePathW(cmdname), "returned FALSE for \"cmd.exe\"\n");

    SetEnvironmentVariableA("NoDefaultCurrentDirectoryInExePath", "nya");
    ok(!pNeedCurrentDirectoryForExePathW(thispath), "returned TRUE for \".\"\n");
    ok(pNeedCurrentDirectoryForExePathW(fullpath), "returned FALSE for \"c:\\\"\n");
    ok(!pNeedCurrentDirectoryForExePathW(cmdname), "returned TRUE for \"cmd.exe\"\n");
}

/* Call various path/file name retrieving APIs and check the case of
 * the returned drive letter. Some apps (for instance Adobe Photoshop CS3
 * installer) depend on the drive letter being in upper case.
 */
static void test_drive_letter_case(void)
{
    UINT ret;
    char buf[MAX_PATH];

#define is_upper_case_letter(a) ((a) >= 'A' && (a) <= 'Z')

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetWindowsDirectoryA(buf, sizeof(buf));
    ok(ret, "GetWindowsDirectory error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* re-use the buffer returned by GetFullPathName */
    buf[2] = '/';
    SetLastError(0xdeadbeef);
    ret = GetFullPathNameA(buf + 2, sizeof(buf), buf, NULL);
    ok(ret, "GetFullPathName error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetSystemDirectoryA(buf, sizeof(buf));
    ok(ret, "GetSystemDirectory error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetCurrentDirectoryA(sizeof(buf), buf);
    ok(ret, "GetCurrentDirectory error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* TEMP is an environment variable, so it can't be tested for case-sensitivity */
    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetTempPathA(sizeof(buf), buf);
    ok(ret, "GetTempPath error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    if (buf[0])
    {
        ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
        ok(buf[strlen(buf)-1] == '\\', "Temporary path (%s) doesn't end in a slash\n", buf);
    }

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetFullPathNameA(".", sizeof(buf), buf, NULL);
    ok(ret, "GetFullPathName error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* re-use the buffer returned by GetFullPathName */
    SetLastError(0xdeadbeef);
    ret = GetShortPathNameA(buf, buf, sizeof(buf));
    ok(ret, "GetShortPathName error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* re-use the buffer returned by GetShortPathName */
    SetLastError(0xdeadbeef);
    ret = GetLongPathNameA(buf, buf, sizeof(buf));
    ok(ret, "GetLongPathNameA error %lu\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);
#undef is_upper_case_letter
}

static const char manifest_dep[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"testdep1\" type=\"win32\" processorArchitecture=\"" ARCH "\"/>"
"    <file name=\"testdep.dll\" />"
"    <file name=\"ole32\" />"
"    <file name=\"kernel32.dll\" />"
"</assembly>";

static const char manifest_main[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\" />"
"<dependency>"
" <dependentAssembly>"
"  <assemblyIdentity type=\"win32\" name=\"testdep1\" version=\"1.2.3.4\" processorArchitecture=\"" ARCH "\" />"
" </dependentAssembly>"
"</dependency>"
"</assembly>";

static void create_manifest_file(const char *filename, const char *manifest)
{
    WCHAR path[MAX_PATH], manifest_path[MAX_PATH];
    HANDLE file;
    DWORD size;

    MultiByteToWideChar( CP_ACP, 0, filename, -1, path, MAX_PATH );

    GetTempPathW(ARRAY_SIZE(manifest_path), manifest_path);
    lstrcatW(manifest_path, path);

    file = CreateFileW(manifest_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    WriteFile(file, manifest, strlen(manifest), &size, NULL);
    CloseHandle(file);
}

static void delete_manifest_file(const char *filename)
{
    CHAR path[MAX_PATH];

    GetTempPathA(sizeof(path), path);
    strcat(path, filename);
    DeleteFileA(path);
}

static HANDLE test_create(const char *file)
{
    WCHAR path[MAX_PATH], manifest_path[MAX_PATH];
    ACTCTXW actctx;
    HANDLE handle;

    MultiByteToWideChar(CP_ACP, 0, file, -1, path, MAX_PATH);
    GetTempPathW(ARRAY_SIZE(manifest_path), manifest_path);
    lstrcatW(manifest_path, path);

    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = manifest_path;

    handle = CreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create context, error %lu\n", GetLastError());

    ok(actctx.cbSize == sizeof(actctx), "cbSize=%ld\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "dwFlags=%ld\n", actctx.dwFlags);
    ok(actctx.lpSource == manifest_path, "lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0, "wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL, "lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "lpApplicationName=%p\n", actctx.lpApplicationName);
    ok(actctx.hModule == NULL, "hModule=%p\n", actctx.hModule);

    return handle;
}

static void test_SearchPathA(void)
{
    static const CHAR testdepA[] = "testdep.dll";
    static const CHAR testdeprelA[] = "./testdep.dll";
    static const CHAR kernel32A[] = "kernel32.dll";
    static const CHAR fileA[] = "";
    CHAR pathA[MAX_PATH + 13], buffA[MAX_PATH], path2A[MAX_PATH], path3A[MAX_PATH + 13], curdirA[MAX_PATH];
    CHAR tmpdirA[MAX_PATH], *ptrA = NULL;
    ULONG_PTR cookie;
    HANDLE handle;
    BOOL bret;
    DWORD ret;

    GetWindowsDirectoryA(pathA, ARRAY_SIZE(pathA));

    /* NULL filename */
    SetLastError(0xdeadbeef);
    ret = SearchPathA(pathA, NULL, NULL, ARRAY_SIZE(buffA), buffA, &ptrA);
    ok(ret == 0, "Expected failure, got %ld\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

    /* empty filename */
    SetLastError(0xdeadbeef);
    ret = SearchPathA(pathA, fileA, NULL, ARRAY_SIZE(buffA), buffA, &ptrA);
    ok(ret == 0, "Expected failure, got %ld\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

    GetTempPathA(ARRAY_SIZE(pathA), pathA);
    strcpy(path2A, pathA);
    strcat(path2A, "testfile.ext.ext2");

    handle = CreateFileA(path2A, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "Failed to create test file.\n");
    CloseHandle(handle);

    buffA[0] = 0;
    ret = SearchPathA(pathA, "testfile.ext", NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(!ret, "Unexpected return value %lu.\n", ret);

    buffA[0] = 0;
    ret = SearchPathA(pathA, "testfile.ext", ".ext2", ARRAY_SIZE(buffA), buffA, NULL);
    ok(!ret, "Unexpected return value %lu.\n", ret);

    buffA[0] = 0;
    ret = SearchPathA(pathA, "testfile.ext.ext2", NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(ret && ret == strlen(path2A), "got %ld\n", ret);

    DeleteFileA(path2A);

    GetWindowsDirectoryA(pathA, ARRAY_SIZE(pathA));

    create_manifest_file("testdep1.manifest", manifest_dep);
    create_manifest_file("main.manifest", manifest_main);

    handle = test_create("main.manifest");
    delete_manifest_file("testdep1.manifest");
    delete_manifest_file("main.manifest");

    /* search fails without active context */
    ret = SearchPathA(NULL, testdepA, NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(ret == 0, "got %ld\n", ret);

    ret = SearchPathA(NULL, kernel32A, NULL, ARRAY_SIZE(path2A), path2A, NULL);
    ok(ret && ret == strlen(path2A), "got %ld\n", ret);

    ret = ActivateActCtx(handle, &cookie);
    ok(ret, "failed to activate context, %lu\n", GetLastError());

    /* works when activated */
    ret = SearchPathA(NULL, testdepA, NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(ret && ret == strlen(buffA), "got %ld\n", ret);

    ret = SearchPathA(NULL, "testdep.dll", ".ext", ARRAY_SIZE(buffA), buffA, NULL);
    ok(ret && ret == strlen(buffA), "got %ld\n", ret);

    ret = SearchPathA(NULL, "testdep", ".dll", ARRAY_SIZE(buffA), buffA, NULL);
    ok(ret && ret == strlen(buffA), "got %ld\n", ret);

    ret = SearchPathA(NULL, "testdep", ".ext", ARRAY_SIZE(buffA), buffA, NULL);
    ok(!ret, "got %ld\n", ret);

    /* name contains path */
    ret = SearchPathA(NULL, testdeprelA, NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(!ret, "got %ld\n", ret);

    /* fails with specified path that doesn't contain this file */
    ret = SearchPathA(pathA, testdepA, NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(!ret, "got %ld\n", ret);

    /* path is redirected for wellknown names too */
    ret = SearchPathA(NULL, kernel32A, NULL, ARRAY_SIZE(buffA), buffA, NULL);
    ok(ret && ret == strlen(buffA), "got %ld\n", ret);
    ok(strcmp(buffA, path2A), "got wrong path %s, %s\n", buffA, path2A);

    ret = DeactivateActCtx(0, cookie);
    ok(ret, "failed to deactivate context, %lu\n", GetLastError());
    ReleaseActCtx(handle);

    /* test the search path priority of the working directory */
    GetTempPathA(sizeof(tmpdirA), tmpdirA);
    ret = GetCurrentDirectoryA(MAX_PATH, curdirA);
    ok(ret, "failed to obtain working directory.\n");
    sprintf(pathA, "%s\\%s", tmpdirA, kernel32A);
    ret = SearchPathA(NULL, kernel32A, NULL, ARRAY_SIZE(path2A), path2A, NULL);
    ok(ret && ret == strlen(path2A), "got %ld\n", ret);
    bret = CopyFileA(path2A, pathA, FALSE);
    ok(bret != 0, "failed to copy test executable to temp directory, %lu\n", GetLastError());
    GetModuleFileNameA( GetModuleHandleA(0), path3A, sizeof(path3A) );
    strcpy( strrchr( path3A, '\\' ) + 1, kernel32A );
    bret = CopyFileA(path2A, path3A, FALSE);
    ok(bret != 0, "failed to copy test executable to launch directory, %lu\n", GetLastError());
    bret = SetCurrentDirectoryA(tmpdirA);
    ok(bret, "failed to change working directory\n");
    ret = SearchPathA(NULL, kernel32A, ".exe", sizeof(buffA), buffA, NULL);
    ok(ret && ret == strlen(buffA), "got %ld\n", ret);
    ok(strcmp(buffA, path3A) == 0, "expected %s, got %s\n", path3A, buffA);
    bret = SetCurrentDirectoryA(curdirA);
    ok(bret, "failed to reset working directory\n");
    DeleteFileA(path3A);
    DeleteFileA(pathA);
}

static void test_SearchPathW(void)
{
    static const WCHAR fileext2W[] = {'t','e','s','t','f','i','l','e','.','e','x','t','.','e','x','t','2',0};
    static const WCHAR fileextW[] = {'t','e','s','t','f','i','l','e','.','e','x','t',0};
    static const WCHAR testdeprelW[] = {'.','/','t','e','s','t','d','e','p','.','d','l','l',0};
    static const WCHAR testdepW[] = {'t','e','s','t','d','e','p','.','d','l','l',0};
    static const WCHAR testdep1W[] = {'t','e','s','t','d','e','p',0};
    static const WCHAR kernel32dllW[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2',0};
    static const WCHAR ole32W[] = {'o','l','e','3','2',0};
    static const WCHAR ext2W[] = {'.','e','x','t','2',0};
    static const WCHAR extW[] = {'.','e','x','t',0};
    static const WCHAR dllW[] = {'.','d','l','l',0};
    static const WCHAR fileW[] = { 0 };
    WCHAR pathW[MAX_PATH], buffW[MAX_PATH], path2W[MAX_PATH];
    WCHAR *ptrW = NULL;
    ULONG_PTR cookie;
    HANDLE handle;
    DWORD ret;

    GetWindowsDirectoryW(pathW, ARRAY_SIZE(pathW));

    /* NULL filename */
    ret = SearchPathW(pathW, NULL, NULL, ARRAY_SIZE(buffW), buffW, &ptrW);
    ok(ret == 0, "Expected failure, got %ld\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %#lx\n", GetLastError());

    /* empty filename */
    SetLastError(0xdeadbeef);
    ret = SearchPathW(pathW, fileW, NULL, ARRAY_SIZE(buffW), buffW, &ptrW);
    ok(ret == 0, "Expected failure, got %ld\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcpyW(path2W, pathW);
    lstrcatW(path2W, fileext2W);

    handle = CreateFileW(path2W, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "Failed to create test file.\n");
    CloseHandle(handle);

    buffW[0] = 0;
    ret = SearchPathW(pathW, fileextW, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(!ret, "Unexpected return value %lu.\n", ret);

    buffW[0] = 0;
    ret = SearchPathW(pathW, fileextW, ext2W, ARRAY_SIZE(buffW), buffW, NULL);
    ok(!ret, "Unexpected return value %lu.\n", ret);

    buffW[0] = 0;
    ret = SearchPathW(pathW, fileext2W, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret && ret == lstrlenW(path2W), "got %ld\n", ret);

    DeleteFileW(path2W);

    GetWindowsDirectoryW(pathW, ARRAY_SIZE(pathW));

    create_manifest_file("testdep1.manifest", manifest_dep);
    create_manifest_file("main.manifest", manifest_main);

    handle = test_create("main.manifest");
    delete_manifest_file("testdep1.manifest");
    delete_manifest_file("main.manifest");

    /* search fails without active context */
    ret = SearchPathW(NULL, testdepW, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret == 0, "got %ld\n", ret);

    ret = SearchPathW(NULL, kernel32dllW, NULL, ARRAY_SIZE(path2W), path2W, NULL);
    ok(ret && ret == lstrlenW(path2W), "got %ld\n", ret);

    /* full path, name without 'dll' extension */
    GetSystemDirectoryW(pathW, ARRAY_SIZE(pathW));
    ret = SearchPathW(pathW, kernel32W, NULL, ARRAY_SIZE(path2W), path2W, NULL);
    ok(ret == 0, "got %ld\n", ret);

    GetWindowsDirectoryW(pathW, ARRAY_SIZE(pathW));

    ret = ActivateActCtx(handle, &cookie);
    ok(ret, "failed to activate context, %lu\n", GetLastError());

    /* works when activated */
    ret = SearchPathW(NULL, testdepW, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret && ret == lstrlenW(buffW), "got %ld\n", ret);

    ret = SearchPathW(NULL, testdepW, extW, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret && ret == lstrlenW(buffW), "got %ld\n", ret);

    ret = SearchPathW(NULL, testdep1W, dllW, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret && ret == lstrlenW(buffW), "got %ld\n", ret);

    ret = SearchPathW(NULL, testdep1W, extW, ARRAY_SIZE(buffW), buffW, NULL);
    ok(!ret, "got %ld\n", ret);

    /* name contains path */
    ret = SearchPathW(NULL, testdeprelW, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(!ret, "got %ld\n", ret);

    /* fails with specified path that doesn't contain this file */
    ret = SearchPathW(pathW, testdepW, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(!ret, "got %ld\n", ret);

    /* path is redirected for wellknown names too, meaning it takes precedence over normal search order */
    ret = SearchPathW(NULL, kernel32dllW, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret && ret == lstrlenW(buffW), "got %ld\n", ret);
    ok(lstrcmpW(buffW, path2W), "got wrong path %s, %s\n", wine_dbgstr_w(buffW), wine_dbgstr_w(path2W));

    /* path is built using on manifest file name */
    ret = SearchPathW(NULL, ole32W, NULL, ARRAY_SIZE(buffW), buffW, NULL);
    ok(ret && ret == lstrlenW(buffW), "got %ld\n", ret);

    ret = DeactivateActCtx(0, cookie);
    ok(ret, "failed to deactivate context, %lu\n", GetLastError());
    ReleaseActCtx(handle);
}

static void test_GetFullPathNameA(void)
{
    char output[MAX_PATH], *filepart;
    DWORD ret;
    int i;
    UINT acp;

    const struct
    {
        LPCSTR name;
        DWORD len;
        LPSTR buffer;
        LPSTR *lastpart;
    } invalid_parameters[] =
    {
        {NULL, 0,        NULL,   NULL},
        {NULL, MAX_PATH, NULL,   NULL},
        {NULL, MAX_PATH, output, NULL},
        {NULL, MAX_PATH, output, &filepart},
        {"",   0,        NULL,   NULL},
        {"",   MAX_PATH, NULL,   NULL},
        {"",   MAX_PATH, output, NULL},
        {"",   MAX_PATH, output, &filepart},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        SetLastError(0xdeadbeef);
        strcpy(output, "deadbeef");
        filepart = (char *)0xdeadbeef;
        ret = GetFullPathNameA(invalid_parameters[i].name,
                               invalid_parameters[i].len,
                               invalid_parameters[i].buffer,
                               invalid_parameters[i].lastpart);
        ok(!ret, "[%d] Expected GetFullPathNameA to return 0, got %lu\n", i, ret);
        ok(!strcmp(output, "deadbeef"), "[%d] Expected the output buffer to be unchanged, got \"%s\"\n", i, output);
        ok(filepart == (char *)0xdeadbeef, "[%d] Expected output file part pointer to be untouched, got %p\n", i, filepart);
        ok(GetLastError() == 0xdeadbeef ||
           GetLastError() == ERROR_INVALID_NAME, /* Win7 */
           "[%d] Expected GetLastError() to return 0xdeadbeef, got %lu\n",
           i, GetLastError());
    }

    acp = GetACP();
    if (acp != 932)
        skip("Skipping DBCS(Japanese) GetFullPathNameA test in this codepage (%d)\n", acp);
    else {
        const struct dbcs_case {
            const char *input;
            const char *expected;
        } testset[] = {
            { "c:\\a\\\x95\x5c\x97\xa0.txt", "\x95\x5c\x97\xa0.txt" },
            { "c:\\\x83\x8f\x83\x43\x83\x93\\wine.c", "wine.c" },
            { "c:\\demo\\\x97\xa0\x95\x5c", "\x97\xa0\x95\x5c" }
        };
        for (i = 0; i < ARRAY_SIZE(testset); i++) {
            ret = GetFullPathNameA(testset[i].input, sizeof(output),
                                   output, &filepart);
            ok(ret, "[%d] GetFullPathName error %lu\n", i, GetLastError());
            ok(!lstrcmpA(filepart, testset[i].expected),
               "[%d] expected %s got %s\n", i, testset[i].expected, filepart);
        }
    }
}

static void test_GetFullPathNameW(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'d','e','a','d','b','e','e','f',0};

    WCHAR output[MAX_PATH], *filepart;
    DWORD ret;
    int i;

    const struct
    {
        LPCWSTR name;
        DWORD len;
        LPWSTR buffer;
        LPWSTR *lastpart;
        int win7_expect;
    } invalid_parameters[] =
    {
        {NULL,   0,        NULL,   NULL},
        {NULL,   0,        NULL,   &filepart, 1},
        {NULL,   MAX_PATH, NULL,   NULL},
        {NULL,   MAX_PATH, output, NULL},
        {NULL,   MAX_PATH, output, &filepart, 1},
        {emptyW, 0,        NULL,   NULL},
        {emptyW, 0,        NULL,   &filepart, 1},
        {emptyW, MAX_PATH, NULL,   NULL},
        {emptyW, MAX_PATH, output, NULL},
        {emptyW, MAX_PATH, output, &filepart, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        SetLastError(0xdeadbeef);
        lstrcpyW(output, deadbeefW);
        filepart = (WCHAR *)0xdeadbeef;
        ret = GetFullPathNameW(invalid_parameters[i].name,
                               invalid_parameters[i].len,
                               invalid_parameters[i].buffer,
                               invalid_parameters[i].lastpart);
        ok(!ret, "[%d] Expected GetFullPathNameW to return 0, got %lu\n", i, ret);
        ok(!lstrcmpW(output, deadbeefW), "[%d] Expected the output buffer to be unchanged, got %s\n", i, wine_dbgstr_w(output));
        ok(filepart == (WCHAR *)0xdeadbeef ||
           (invalid_parameters[i].win7_expect && filepart == NULL),
           "[%d] Expected output file part pointer to be untouched, got %p\n", i, filepart);
        ok(GetLastError() == 0xdeadbeef ||
           GetLastError() == ERROR_INVALID_NAME, /* Win7 */
           "[%d] Expected GetLastError() to return 0xdeadbeef, got %lu\n",
           i, GetLastError());
    }
}

static void init_pointers(void)
{
    HMODULE mod = GetModuleHandleA("kernel32.dll");

#define MAKEFUNC(f) (p##f = (void*)GetProcAddress(mod, #f))
    MAKEFUNC(NeedCurrentDirectoryForExePathA);
    MAKEFUNC(NeedCurrentDirectoryForExePathW);
    MAKEFUNC(SetSearchPathMode);
    MAKEFUNC(AddDllDirectory);
    MAKEFUNC(RemoveDllDirectory);
    MAKEFUNC(SetDllDirectoryW);
    MAKEFUNC(SetDefaultDllDirectories);
    MAKEFUNC(CheckNameLegalDOS8Dot3W);
    MAKEFUNC(CheckNameLegalDOS8Dot3A);
    mod = GetModuleHandleA("ntdll.dll");
    MAKEFUNC(LdrGetDllPath);
    MAKEFUNC(RtlGetExePath);
    MAKEFUNC(RtlGetSearchPath);
    MAKEFUNC(RtlReleasePath);
#undef MAKEFUNC
}

static void test_relative_path(void)
{
    char path[MAX_PATH], buf[MAX_PATH];
    HANDLE file;
    int ret;
    WCHAR curdir[MAX_PATH];

    GetCurrentDirectoryW(MAX_PATH, curdir);
    GetTempPathA(MAX_PATH, path);
    ret = SetCurrentDirectoryA(path);
    ok(ret, "SetCurrentDirectory error %ld\n", GetLastError());

    ret = CreateDirectoryA("foo", NULL);
    ok(ret, "CreateDirectory error %ld\n", GetLastError());
    file = CreateFileA("foo\\file", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "failed to create temp file\n");
    CloseHandle(file);
    ret = CreateDirectoryA("bar", NULL);
    ok(ret, "CreateDirectory error %ld\n", GetLastError());
    ret = SetCurrentDirectoryA("bar");
    ok(ret, "SetCurrentDirectory error %ld\n", GetLastError());

    ret = GetFileAttributesA("..\\foo\\file");
    ok(ret != INVALID_FILE_ATTRIBUTES, "GetFileAttributes error %ld\n", GetLastError());

    strcpy(buf, "deadbeef");
    ret = GetLongPathNameA(".", buf, MAX_PATH);
    ok(ret, "GetLongPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, "."), "expected ., got %s\n", buf);
    strcpy(buf, "deadbeef");
    ret = GetShortPathNameA(".", buf, MAX_PATH);
    ok(ret, "GetShortPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, "."), "expected ., got %s\n", buf);

    strcpy(buf, "deadbeef");
    ret = GetLongPathNameA("..", buf, MAX_PATH);
    ok(ret, "GetLongPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, ".."), "expected .., got %s\n", buf);
    strcpy(buf, "deadbeef");
    ret = GetShortPathNameA("..", buf, MAX_PATH);
    ok(ret, "GetShortPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, ".."), "expected .., got %s\n", buf);

    strcpy(buf, "deadbeef");
    ret = GetLongPathNameA("..\\foo\\file", buf, MAX_PATH);
    ok(ret, "GetLongPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, "..\\foo\\file"), "expected ..\\foo\\file, got %s\n", buf);
    strcpy(buf, "deadbeef");
    ret = GetShortPathNameA("..\\foo\\file", buf, MAX_PATH);
    ok(ret, "GetShortPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, "..\\foo\\file"), "expected ..\\foo\\file, got %s\n", buf);

    strcpy(buf, "deadbeef");
    ret = GetLongPathNameA(".\\..\\foo\\file", buf, MAX_PATH);
    ok(ret, "GetLongPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, ".\\..\\foo\\file"), "expected .\\..\\foo\\file, got %s\n", buf);
    strcpy(buf, "deadbeef");
    ret = GetShortPathNameA(".\\..\\foo\\file", buf, MAX_PATH);
    ok(ret, "GetShortPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, ".\\..\\foo\\file"), "expected .\\..\\foo\\file, got %s\n", buf);

    /* test double delimiters */
    strcpy(buf, "deadbeef");
    ret = GetLongPathNameA("..\\\\foo\\file", buf, MAX_PATH);
    ok(ret, "GetLongPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, "..\\\\foo\\file"), "expected ..\\\\foo\\file, got %s\n", buf);
    strcpy(buf, "deadbeef");
    ret = GetShortPathNameA("..\\\\foo\\file", buf, MAX_PATH);
    ok(ret, "GetShortPathName error %ld\n", GetLastError());
    ok(!strcmp(buf, "..\\\\foo\\file"), "expected ..\\\\foo\\file, got %s\n", buf);

    SetCurrentDirectoryA("..");
    DeleteFileA("foo\\file");
    RemoveDirectoryA("foo");
    RemoveDirectoryA("bar");
    SetCurrentDirectoryW(curdir);
}

static void test_CheckNameLegalDOS8Dot3(void)
{
    static const WCHAR has_driveW[] = {'C',':','\\','a','.','t','x','t',0};
    static const WCHAR has_pathW[] = {'b','\\','a','.','t','x','t',0};
    static const WCHAR too_longW[] = {'a','l','o','n','g','f','i','l','e','n','a','m','e','.','t','x','t',0};
    static const WCHAR twodotsW[] = {'t','e','s','t','.','e','s','t','.','t','x','t',0};
    static const WCHAR longextW[] = {'t','e','s','t','.','t','x','t','t','x','t',0};
    static const WCHAR emptyW[] = {0};
    static const WCHAR funnycharsW[] = {'!','#','$','%','&','\'','(',')','.','-','@','^',0};
    static const WCHAR length8W[] = {'t','e','s','t','t','e','s','t','.','t','x','t',0};
    static const WCHAR length1W[] = {'t',0};
    static const WCHAR withspaceW[] = {'t','e','s','t',' ','e','s','t','.','t','x','t',0};

    static const struct {
        const WCHAR *name;
        BOOL should_be_legal, has_space;
    } cases[] = {
        {has_driveW, FALSE, FALSE},
        {has_pathW, FALSE, FALSE},
        {too_longW, FALSE, FALSE},
        {twodotsW, FALSE, FALSE},
        {longextW, FALSE, FALSE},
        {emptyW, TRUE /* ! */, FALSE},
        {funnycharsW, TRUE, FALSE},
        {length8W, TRUE, FALSE},
        {length1W, TRUE, FALSE},
        {withspaceW, TRUE, TRUE},
    };

    BOOL br, is_legal, has_space;
    char astr[64];
    DWORD i;

    if(!pCheckNameLegalDOS8Dot3W){
        win_skip("Missing CheckNameLegalDOS8Dot3, skipping tests\n");
        return;
    }

    br = pCheckNameLegalDOS8Dot3W(NULL, NULL, 0, NULL, &is_legal);
    ok(br == FALSE, "CheckNameLegalDOS8Dot3W should have failed\n");

    br = pCheckNameLegalDOS8Dot3A(NULL, NULL, 0, NULL, &is_legal);
    ok(br == FALSE, "CheckNameLegalDOS8Dot3A should have failed\n");

    br = pCheckNameLegalDOS8Dot3W(length8W, NULL, 0, NULL, NULL);
    ok(br == FALSE, "CheckNameLegalDOS8Dot3W should have failed\n");

    br = pCheckNameLegalDOS8Dot3A("testtest.txt", NULL, 0, NULL, NULL);
    ok(br == FALSE, "CheckNameLegalDOS8Dot3A should have failed\n");

    for(i = 0; i < ARRAY_SIZE(cases); ++i){
        br = pCheckNameLegalDOS8Dot3W(cases[i].name, NULL, 0, &has_space, &is_legal);
        ok(br == TRUE, "CheckNameLegalDOS8Dot3W failed for %s\n", wine_dbgstr_w(cases[i].name));
        ok(is_legal == cases[i].should_be_legal, "Got wrong legality for %s\n", wine_dbgstr_w(cases[i].name));
        if(is_legal)
            ok(has_space == cases[i].has_space, "Got wrong space for %s\n", wine_dbgstr_w(cases[i].name));

        WideCharToMultiByte(CP_ACP, 0, cases[i].name, -1, astr, sizeof(astr), NULL, NULL);

        br = pCheckNameLegalDOS8Dot3A(astr, NULL, 0, &has_space, &is_legal);
        ok(br == TRUE, "CheckNameLegalDOS8Dot3W failed for %s\n", astr);
        ok(is_legal == cases[i].should_be_legal, "Got wrong legality for %s\n", astr);
        if(is_legal)
            ok(has_space == cases[i].has_space, "Got wrong space for %s\n", wine_dbgstr_w(cases[i].name));
    }
}

static void test_SetSearchPathMode(void)
{
    BOOL ret;
    char orig[MAX_PATH], buf[MAX_PATH], dir[MAX_PATH], expect[MAX_PATH];
    HANDLE handle;

    if (!pSetSearchPathMode)
    {
        win_skip( "SetSearchPathMode isn't available\n" );
        return;
    }
    GetCurrentDirectoryA( MAX_PATH, orig );
    GetTempPathA( MAX_PATH, buf );
    GetTempFileNameA( buf, "path", 0, dir );
    DeleteFileA( dir );
    CreateDirectoryA( dir, NULL );
    ret = SetCurrentDirectoryA( dir );
    ok( ret, "failed to switch to %s\n", dir );
    if (!ret)
    {
        RemoveDirectoryA( dir );
        return;
    }

    handle = CreateFileA( "kernel32.dll", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    CloseHandle( handle );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( 0 );
    ok( !ret, "SetSearchPathMode succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( 0x80 );
    ok( !ret, "SetSearchPathMode succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_PERMANENT );
    ok( !ret, "SetSearchPathMode succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = SearchPathA( NULL, "kernel32.dll", NULL, MAX_PATH, buf, NULL );
    ok( ret, "SearchPathA failed err %lu\n", GetLastError() );
    GetCurrentDirectoryA( MAX_PATH, expect );
    strcat( expect, "\\kernel32.dll" );
    ok( !lstrcmpiA( buf, expect ), "found %s expected %s\n", buf, expect );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE );
    ok( ret, "SetSearchPathMode failed err %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = SearchPathA( NULL, "kernel32.dll", NULL, MAX_PATH, buf, NULL );
    ok( ret, "SearchPathA failed err %lu\n", GetLastError() );
    GetSystemDirectoryA( expect, MAX_PATH );
    strcat( expect, "\\kernel32.dll" );
    ok( !lstrcmpiA( buf, expect ), "found %s expected %s\n", buf, expect );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE );
    ok( ret, "SetSearchPathMode failed err %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = SearchPathA( NULL, "kernel32.dll", NULL, MAX_PATH, buf, NULL );
    ok( ret, "SearchPathA failed err %lu\n", GetLastError() );
    GetCurrentDirectoryA( MAX_PATH, expect );
    strcat( expect, "\\kernel32.dll" );
    ok( !lstrcmpiA( buf, expect ), "found %s expected %s\n", buf, expect );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT );
    ok( !ret, "SetSearchPathMode succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT );
    ok( ret, "SetSearchPathMode failed err %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE );
    ok( !ret, "SetSearchPathMode succeeded\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE );
    ok( !ret, "SetSearchPathMode succeeded\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pSetSearchPathMode( BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT );
    ok( ret, "SetSearchPathMode failed err %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = SearchPathA( NULL, "kernel32.dll", NULL, MAX_PATH, buf, NULL );
    ok( ret, "SearchPathA failed err %lu\n", GetLastError() );
    GetSystemDirectoryA( expect, MAX_PATH );
    strcat( expect, "\\kernel32.dll" );
    ok( !lstrcmpiA( buf, expect ), "found %s expected %s\n", buf, expect );

    DeleteFileA( "kernel32.dll" );
    SetCurrentDirectoryA( orig );
    RemoveDirectoryA( dir );
}

static const WCHAR pathW[] = {'P','A','T','H',0};

static void build_search_path( WCHAR *buffer, UINT size, const WCHAR *module, const WCHAR *dlldir, BOOL safe )
{
    WCHAR *p;

    if (module) lstrcpynW( buffer, module, size );
    else GetModuleFileNameW( NULL, buffer, size );
    if (!(p = wcsrchr( buffer, '\\' ))) return;
    *p++ = ';';
    if (dlldir)
    {
        lstrcpyW( p, dlldir );
        p += lstrlenW( p );
        if (*dlldir) *p++ = ';';
    }
    else if (!safe)
    {
        *p++ = '.';
        *p++ = ';';
    }
    GetSystemDirectoryW( p, buffer + size - p );
    p = buffer + lstrlenW(buffer);
    *p++ = ';';
    GetSystemDirectoryW( p, buffer + size - p );
    p = buffer + lstrlenW(buffer) - 2;  /* remove "32" */
    *p++ = ';';
    GetWindowsDirectoryW( p, buffer + size - p );
    p = buffer + lstrlenW(buffer);
    *p++ = ';';
    if (!dlldir && safe)
    {
        *p++ = '.';
        *p++ = ';';
    }
    GetEnvironmentVariableW( pathW, p, buffer + size - p );
}

static BOOL path_equal( const WCHAR *path1, const WCHAR *path2 )
{
    for (;;)
    {
        while (*path1 && towlower(*path1) == towlower(*path2)) { path1++; path2++; }
        if (*path1 && *path1 != '\\' && *path1 != ';') return FALSE;
        while (*path1 && (*path1 == '\\' || *path1 == ';')) path1++;
        while (*path2 && (*path2 == '\\' || *path2 == ';')) path2++;
        if (!*path1 || !*path2) return !*path1 && !*path2;
    }
}

static void test_RtlGetSearchPath(void)
{
    NTSTATUS ret;
    WCHAR *path;
    WCHAR buffer[2048], old_path[2048], dlldir[4];

    if (!pRtlGetSearchPath)
    {
        win_skip( "RtlGetSearchPath isn't available\n" );
        return;
    }

    GetEnvironmentVariableW( pathW, old_path, ARRAY_SIZE(old_path) );
    GetWindowsDirectoryW( buffer, ARRAY_SIZE(buffer) );
    lstrcpynW( dlldir, buffer, ARRAY_SIZE(dlldir) );

    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetSearchPath( &path );
    ok( !ret, "RtlGetSearchPath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    SetEnvironmentVariableA( "PATH", "foo" );
    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetSearchPath( &path );
    ok( !ret, "RtlGetSearchPath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    if (pSetDllDirectoryW)
    {
        ok( pSetDllDirectoryW( dlldir ), "SetDllDirectoryW failed\n" );
        build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );
        path = (WCHAR *)0xdeadbeef;
        ret = pRtlGetSearchPath( &path );
        ok( !ret, "RtlGetSearchPath failed %lx\n", ret );
        ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
        pRtlReleasePath( path );
        pSetDllDirectoryW( NULL );
    }

    SetEnvironmentVariableW( pathW, old_path );
}

static void test_RtlGetExePath(void)
{
    static const WCHAR fooW[] = {'\\','f','o','o',0};
    static const WCHAR emptyW[1];
    NTSTATUS ret;
    WCHAR *path;
    WCHAR buffer[2048], old_path[2048], dlldir[4];

    if (!pRtlGetExePath)
    {
        win_skip( "RtlGetExePath isn't available\n" );
        return;
    }

    GetEnvironmentVariableW( pathW, old_path, ARRAY_SIZE(old_path) );
    GetWindowsDirectoryW( buffer, ARRAY_SIZE(buffer) );
    lstrcpynW( dlldir, buffer, ARRAY_SIZE(dlldir) );
    SetEnvironmentVariableA( "NoDefaultCurrentDirectoryInExePath", NULL );

    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, FALSE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetExePath( fooW, &path );
    ok( !ret, "RtlGetExePath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, FALSE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetExePath( fooW + 1, &path );
    ok( !ret, "RtlGetExePath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    SetEnvironmentVariableA( "NoDefaultCurrentDirectoryInExePath", "yes" );

    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, FALSE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetExePath( fooW, &path );
    ok( !ret, "RtlGetExePath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, emptyW, TRUE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetExePath( fooW + 1, &path );
    ok( !ret, "RtlGetExePath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    SetEnvironmentVariableA( "PATH", "foo" );
    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, FALSE );
    path = (WCHAR *)0xdeadbeef;
    ret = pRtlGetExePath( fooW, &path );
    ok( !ret, "RtlGetExePath failed %lx\n", ret );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    if (pSetDllDirectoryW)
    {
        ok( pSetDllDirectoryW( dlldir ), "SetDllDirectoryW failed\n" );
        build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, FALSE );
        path = (WCHAR *)0xdeadbeef;
        ret = pRtlGetExePath( fooW, &path );
        ok( !ret, "RtlGetExePath failed %lx\n", ret );
        ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
        pRtlReleasePath( path );
        pSetDllDirectoryW( NULL );
    }

    SetEnvironmentVariableW( pathW, old_path );
}

static void test_LdrGetDllPath(void)
{
    static const WCHAR fooW[] = {'f','o','o',0};
    NTSTATUS ret;
    WCHAR *path, *unknown_ptr, *p;
    WCHAR buffer[2048], old_path[2048], dlldir[4];

    if (!pLdrGetDllPath)
    {
        win_skip( "LdrGetDllPath isn't available\n" );
        return;
    }
    GetEnvironmentVariableW( pathW, old_path, ARRAY_SIZE(old_path) );
    GetWindowsDirectoryW( buffer, ARRAY_SIZE(buffer) );
    lstrcpynW( dlldir, buffer, ARRAY_SIZE(dlldir) );

    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );

    path = unknown_ptr = (WCHAR *)0xdeadbeef;
    ret = pLdrGetDllPath( 0, 0, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    SetEnvironmentVariableA( "PATH", "foo" );
    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );
    ret = pLdrGetDllPath( 0, 0, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    if (pSetDllDirectoryW)
    {
        ok( pSetDllDirectoryW( dlldir ), "SetDllDirectoryW failed\n" );
        build_search_path( buffer, ARRAY_SIZE(buffer), NULL, dlldir, TRUE );
        ret = pLdrGetDllPath( 0, 0, &path, &unknown_ptr );
        ok( !ret, "LdrGetDllPath failed %lx\n", ret );
        ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
        ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
        pRtlReleasePath( path );
        pSetDllDirectoryW( NULL );
    }

    ret = pLdrGetDllPath( 0, LOAD_LIBRARY_SEARCH_SYSTEM32, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    GetSystemDirectoryW( buffer, ARRAY_SIZE(buffer) );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( 0, LOAD_LIBRARY_SEARCH_APPLICATION_DIR, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    GetModuleFileNameW( NULL, buffer, ARRAY_SIZE(buffer) );
    if ((p = wcsrchr( buffer, '\\' ))) *p = 0;
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( fooW, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR, &path, &unknown_ptr );
    ok( ret == STATUS_INVALID_PARAMETER, "LdrGetDllPath failed %lx\n", ret );

    lstrcpyW( buffer, L"\\\\?\\" );
    lstrcatW( buffer, dlldir );
    p = buffer + lstrlenW(buffer);
    *p++ = '\\';
    lstrcpyW( p, fooW );
    ret = pLdrGetDllPath( buffer, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    ok( !memcmp( path, L"\\\\?\\", 4 * sizeof(WCHAR) ) && path_equal( path + 4, dlldir ),
        "got %s expected \\\\?\\%s\n", wine_dbgstr_w(path), wine_dbgstr_w(dlldir));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( L"\\\\?\\c:\\test.dll", LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    ok( !lstrcmpW( path, L"\\\\?\\c:" ), "got %s expected \\\\?\\c:\n", wine_dbgstr_w(path));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( fooW, LOAD_WITH_ALTERED_SEARCH_PATH, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( L"temp/foo", LOAD_WITH_ALTERED_SEARCH_PATH, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    build_search_path( buffer, ARRAY_SIZE(buffer), NULL, NULL, TRUE );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( L".\\foo\\foobar", LOAD_WITH_ALTERED_SEARCH_PATH, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    build_search_path( buffer, ARRAY_SIZE(buffer), L".\\foo\\foobar", NULL, TRUE );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( L"temp\\foo", LOAD_WITH_ALTERED_SEARCH_PATH, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    build_search_path( buffer, ARRAY_SIZE(buffer), L"temp\\foo", NULL, TRUE );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    ret = pLdrGetDllPath( L"c:\\temp\\foo", LOAD_WITH_ALTERED_SEARCH_PATH, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    build_search_path( buffer, ARRAY_SIZE(buffer), L"c:\\temp\\foo", NULL, TRUE );
    ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
    pRtlReleasePath( path );

    lstrcpyW( buffer, fooW );
    ret = pLdrGetDllPath( buffer, LOAD_WITH_ALTERED_SEARCH_PATH | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR, &path, &unknown_ptr );
    ok( ret == STATUS_INVALID_PARAMETER, "got %lx expected %lx\n", ret, STATUS_INVALID_PARAMETER );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );

    lstrcpyW( buffer, dlldir );
    p = buffer + lstrlenW(buffer);
    *p++ = '\\';
    lstrcpyW( p, fooW );
    ret = pLdrGetDllPath( buffer, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR, &path, &unknown_ptr );
    ok( !ret, "LdrGetDllPath failed %lx\n", ret );
    ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
    ok( path_equal( path, dlldir ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(dlldir));
    pRtlReleasePath( path );

    if (pAddDllDirectory)
    {
        DLL_DIRECTORY_COOKIE cookie = pAddDllDirectory( dlldir );
        ok( !!cookie, "AddDllDirectory failed\n" );
        ret = pLdrGetDllPath( 0, LOAD_LIBRARY_SEARCH_USER_DIRS, &path, &unknown_ptr );
        ok( !ret, "LdrGetDllPath failed %lx\n", ret );
        ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
        ok( path_equal( path, dlldir ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(dlldir));
        pRtlReleasePath( path );
        pRemoveDllDirectory( cookie );
    }

    if (pSetDefaultDllDirectories)
    {
        pSetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_SYSTEM32 );
        ret = pLdrGetDllPath( 0, 0, &path, &unknown_ptr );
        ok( !ret, "LdrGetDllPath failed %lx\n", ret );
        ok( !unknown_ptr, "unknown ptr %p\n", unknown_ptr );
        GetSystemDirectoryW( buffer, ARRAY_SIZE(buffer) );
        ok( path_equal( path, buffer ), "got %s expected %s\n", wine_dbgstr_w(path), wine_dbgstr_w(buffer));
        pRtlReleasePath( path );
        pSetDefaultDllDirectories( 0 );
    }

    SetEnvironmentVariableW( pathW, old_path );
}

START_TEST(path)
{
    CHAR origdir[MAX_PATH],curdir[MAX_PATH], curDrive, otherDrive;

    init_pointers();

    test_relative_path();
    test_InitPathA(curdir, &curDrive, &otherDrive);
    test_CurrentDirectoryA(origdir,curdir);
    test_PathNameA(curdir, curDrive, otherDrive);
    test_CleanupPathA(origdir,curdir);
    test_GetTempPath();
    test_GetLongPathNameA();
    test_GetLongPathNameW();
    test_GetShortPathNameW();
    test_GetSystemDirectory();
    test_GetWindowsDirectory();
    test_NeedCurrentDirectoryForExePathA();
    test_NeedCurrentDirectoryForExePathW();
    test_drive_letter_case();
    test_SearchPathA();
    test_SearchPathW();
    test_GetFullPathNameA();
    test_GetFullPathNameW();
    test_CheckNameLegalDOS8Dot3();
    test_SetSearchPathMode();
    test_RtlGetSearchPath();
    test_RtlGetExePath();
    test_LdrGetDllPath();
}
