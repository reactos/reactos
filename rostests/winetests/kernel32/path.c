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
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"

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

/* the following characters don't work well with GetFullPathNameA
   in Win98.  I don't know if this is a FAT thing, or if it is an OS thing
   but I don't test these characters now.
   NOTE: Win2k allows GetFullPathNameA to work with them though
      |<>"
*/
static const CHAR funny_chars[]="!@#$%^&*()=+{}[],?'`";
static const CHAR is_char_ok[] ="11111110111111111011";

static DWORD (WINAPI *pGetLongPathNameA)(LPCSTR,LPSTR,DWORD);
static DWORD (WINAPI *pGetLongPathNameW)(LPWSTR,LPWSTR,DWORD);

/* Present in Win2003+ */
static BOOL  (WINAPI *pNeedCurrentDirectoryForExePathA)(LPCSTR);
static BOOL  (WINAPI *pNeedCurrentDirectoryForExePathW)(LPCWSTR);

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
       fullpath[MAX_PATH],      /*full path to the file (not short/long) */
       subpath[MAX_PATH],       /*relative path to the file */
       fullpathshort[MAX_PATH], /*absolute path to the file (short format) */
       fullpathlong[MAX_PATH],  /*absolute path to the file (long format) */
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
/* GetLongPathNameA is only supported on Win2k+ and Win98+ */
  if(pGetLongPathNameA) {
    ok((len=pGetLongPathNameA(curdir,curdirlong,MAX_PATH)),
       "%s: GetLongPathNameA failed\n",errstr);
/*GetLongPathNameA can return a trailing '\\' but shouldn't do so here */
    ok(! HAS_TRAIL_SLASH_A(curdirlong),
       "%s: GetLongPathNameA should not have a trailing \\\n",errstr);
  }
  ok((len=GetShortPathNameA(curdir,curdirshort,MAX_PATH)),
     "%s: GetShortPathNameA failed\n",errstr);
/*GetShortPathNameA can return a trailing '\\' but shouldn't do so here */
  ok(! HAS_TRAIL_SLASH_A(curdirshort),
     "%s: GetShortPathNameA should not have a trailing \\\n",errstr);
/* build relative and absolute paths from inputs */
  if(lstrlenA(subdir)) {
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
/* Test GetLongPathNameA functionality
   We test both conversion from GetFullPathNameA and from GetShortPathNameA
*/
  if(pGetLongPathNameA) {
    if(len!=0) {
      SetLastError(0);
      len=pGetLongPathNameA(shortstr,tmpstr,MAX_PATH);
      if(passfail==NULL) {
        ok(len,
          "%s: GetLongPathNameA failed during Short->Long conversion\n", errstr);
        ok(lstrcmpiA(fullpathlong,tmpstr)==0,
           "%s: GetLongPathNameA returned '%s' instead of '%s'\n",
           errstr,tmpstr,fullpathlong);
      } else {
        passfail->s2llen=len;
        passfail->s2lerror=GetLastError();
      }
    }
    SetLastError(0);
    len=pGetLongPathNameA(fullpath,tmpstr,MAX_PATH);
    if(passfail==NULL) {
      ok(len, "%s: GetLongPathNameA failed\n",errstr);
      if(HAS_TRAIL_SLASH_A(fullpath)) {
        ok(lstrcmpiA(fullpathlong,tmpstr)==0,
           "%s: GetLongPathNameA returned '%s' instead of '%s'\n",
           errstr,tmpstr,fullpathlong);
      } else {
        ok(lstrcmpiA(fullpathlong,tmpstr)==0,
          "%s: GetLongPathNameA returned '%s' instead of '%s'\n",
          errstr,tmpstr,fullpathlong);
      }
    } else {
      passfail->longlen=len;
      passfail->longerror=GetLastError();
    }
  }
}

/* split path into leading directory, and 8.3 filename */
static void test_SplitShortPathA(CHAR *path,CHAR *dir,CHAR *eight,CHAR *three) {
  int done,error;
  int ext,fil;
  int len,i;
  len=lstrlenA(path);
  ext=len; fil=len; done=0; error=0;
/* walk backwards over path looking for '.' or '\\' separators */
  for(i=len-1;(i>=0) && (!done);i--) {
    if(path[i]=='.')
      if(ext!=len) error=1; else ext=i;
    else if(path[i]=='\\') {
      if(i==len-1) {
        error=1;
      } else {
        fil=i;
        done=1;
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
         "%s: GetShortPathNameA error: len=%d error=%d tmpstr=[%s]\n",
         errstr,passfail.shortlen,passfail.shorterror,tmpstr);
  } else {
      ok(passfail.shortlen==0 &&
         (passfail.shorterror==ERROR_INVALID_NAME || passfail.shorterror==ERROR_FILE_NOT_FOUND || !passfail.shorterror),
         "%s: GetShortPathA should have failed len=%d, error=%d\n",
         errstr,passfail.shortlen,passfail.shorterror);
  }
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    if(valid) {
      ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
         "%s: GetLongPathA returned %d and not %d\n",
         errstr,passfail.longerror,ERROR_FILE_NOT_FOUND);
    } else {
      ok(passfail.longerror==ERROR_INVALID_NAME ||
         passfail.longerror==ERROR_FILE_NOT_FOUND,
         "%s: GetLongPathA returned %d and not %d or %d'\n",
         errstr, passfail.longerror,ERROR_INVALID_NAME,ERROR_FILE_NOT_FOUND);
    }
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
       "%s: Couldn't set directory to it's original value\n",errstr);
  } else {
/* else thest that it fails correctly */
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
       tmpstr1[MAX_PATH];
  DWORD len,len1,drives;
  INT id;
  HANDLE hndl;
  BOOL bRes;

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
  ok(len1==len+1,
     "GetTempPathA should return string length %d instead of %d\n",len+1,len1);

/* Test GetTmpFileNameA
   The only test we do here is whether GetTempFileNameA passes or not.
   We do not thoroughly test this function yet (specifically, whether
   it behaves correctly when 'unique' is non zero)
*/
  ok((id=GetTempFileNameA(tmppath,"path",0,newdir)),"GetTempFileNameA failed\n");
  sprintf(tmpstr,"pat%.4x.tmp",id & 0xffff);
  sprintf(tmpstr1,"pat%x.tmp",id & 0xffff);
  ok(lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr)==0 ||
     lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr1)==0,
     "GetTempFileNameA returned '%s' which doesn't match '%s' or '%s'. id=%x\n",
     newdir,tmpstr,tmpstr1,id);
  ok(DeleteFileA(newdir),"Couldn't delete the temporary file we just created\n");     

  id=GetTempFileNameA(tmppath,NULL,0,newdir);
/* Windows 95, 98 return 0==id, while Windows 2000, XP return 0!=id */
  if (id)
  {
    sprintf(tmpstr,"%.4x.tmp",id & 0xffff);
    sprintf(tmpstr1,"%x.tmp",id & 0xffff);
    ok(lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr)==0 ||
       lstrcmpiA(newdir+lstrlenA(tmppath),tmpstr1)==0,
       "GetTempFileNameA returned '%s' which doesn't match '%s' or '%s'. id=%x\n",
       newdir,tmpstr,tmpstr1,id);
    ok(DeleteFileA(newdir),"Couldn't delete the temporary file we just created\n");
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
  bRes = CreateDirectoryA("c:",NULL);
  ok(!bRes && (GetLastError() == ERROR_ACCESS_DENIED  || 
               GetLastError() == ERROR_ALREADY_EXISTS),
     "CreateDirectoryA(\"c:\" should have failed (%d)\n", GetLastError());
  bRes = CreateDirectoryA("c:\\",NULL);
  ok(!bRes && (GetLastError() == ERROR_ACCESS_DENIED  ||
               GetLastError() == ERROR_ALREADY_EXISTS),
     "CreateDirectoryA(\"c:\\\" should have failed (%d)\n", GetLastError());
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
  ok(len1==len+1, "GetCurrentDirectoryA returned %d instead of %d\n",len1,len+1);
  ok(lstrcmpiA(tmpstr,"aaaaaaa")==0,
     "GetCurrentDirectoryA should not have modified the buffer\n");
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
/* Check that SetCurrentDirectory fails for a nonexistent lond directory */
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
  strcpy(tmpstr,"C:");
  test_setdir(newdir,tmpstr,newdir,1,"check 10");
/* works however with a trailing backslash */
  strcpy(tmpstr,"C:\\");
  test_setdir(newdir,tmpstr,NULL,1,"check 11");
}

/* Cleanup the mess we made while executing these tests */
static void test_CleanupPathA(CHAR *origdir, CHAR *curdir)
{
  CHAR tmpstr[MAX_PATH];
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

/* This routine will test Get(Full|Short|Long)PathNameA */
static void test_PathNameA(CHAR *curdir, CHAR curDrive, CHAR otherDrive)
{
  CHAR curdir_short[MAX_PATH],
       longdir_short[MAX_PATH];
  CHAR tmpstr[MAX_PATH],tmpstr1[MAX_PATH],tmpstr2[MAX_PATH];
  LPSTR strptr;                 /*ptr to the filename portion of the path */
  DWORD len;
  INT i;
  CHAR dir[MAX_PATH],eight[MAX_PATH],three[MAX_PATH];
  SLpassfail passfail;

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

  if (pGetLongPathNameA) {
    DWORD rc1,rc2;
    sprintf(tmpstr,"%s\\%s\\%s",curdir,LONGDIR,LONGFILE);
    rc1=(*pGetLongPathNameA)(tmpstr,NULL,0);
    rc2=(*pGetLongPathNameA)(curdir,NULL,0);
    ok((rc1-strlen(tmpstr))==(rc2-strlen(curdir)),
       "GetLongPathNameA: wrong return code, %d instead of %d\n",
       rc1, lstrlenA(tmpstr)+1);

    sprintf(dir,"%c:",curDrive);
    rc1=(*pGetLongPathNameA)(dir,tmpstr,sizeof(tmpstr));
    ok(strcmp(dir,tmpstr)==0,
       "GetLongPathNameA: returned '%s' instead of '%s' (rc=%d)\n",
       tmpstr,dir,rc1);
  }

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
     "GetShortPathNameA error: len=%d error=%d tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetlongPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  }
/* Now try a 8.3 directory, long file name */
  test_ValidPathA(curdir,SHORTDIR,NONFILE_LONG,tmpstr,&passfail,"test6");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetlongPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  }
/* Next is a long directory, 8.3 file */
  test_ValidPathA(curdir,LONGDIR,NONFILE_SHORT,tmpstr,&passfail,"test7");
  sprintf(tmpstr2,"%s\\%s",curdir_short,LONGDIR);
  GetShortPathNameA(tmpstr2,tmpstr1,MAX_PATH);
  strcat(tmpstr1,"\\" NONFILE_SHORT);
  ok((passfail.shortlen==0 &&
      (passfail.shorterror==ERROR_PATH_NOT_FOUND ||
       passfail.shorterror==ERROR_FILE_NOT_FOUND)) ||
     (passfail.shortlen==strlen(tmpstr1) && lstrcmpiA(tmpstr,tmpstr1)==0),
     "GetShortPathNameA error: len=%d error=%d tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
      "GetlongPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  }
/*Lastly a long directory, long file */
  test_ValidPathA(curdir,LONGDIR,NONFILE_LONG,tmpstr,&passfail,"test8");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetlongPathA should have returned 'ERROR_FILE_NOT_FOUND'\n");
  }
/* Now try again with directories that don't exist */
/* 8.3 directory, 8.3 filename */
  test_ValidPathA(curdir,NONDIR_SHORT,SHORTFILE,tmpstr,&passfail,"test9");
  sprintf(tmpstr1,"%s\\%s\\%s",curdir_short,NONDIR_SHORT,SHORTFILE);
  ok((passfail.shortlen==0 &&
      (passfail.shorterror==ERROR_PATH_NOT_FOUND ||
       passfail.shorterror==ERROR_FILE_NOT_FOUND)) ||
     (passfail.shortlen==strlen(tmpstr1) && lstrcmpiA(tmpstr,tmpstr1)==0),
     "GetShortPathNameA error: len=%d error=%d tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_PATH_NOT_FOUND ||
       passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetLongPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
       passfail.longerror);
  }
/* Now try a 8.3 directory, long file name */
  test_ValidPathA(curdir,NONDIR_SHORT,LONGFILE,tmpstr,&passfail,"test10");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
      passfail.shorterror);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_PATH_NOT_FOUND ||
       passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetLongPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
       passfail.longerror);
  }
/* Next is a long directory, 8.3 file */
  test_ValidPathA(curdir,NONDIR_LONG,SHORTFILE,tmpstr,&passfail,"test11");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
      passfail.shorterror);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_PATH_NOT_FOUND ||
       passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetLongPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
       passfail.longerror);
  }
/*Lastly a long directory, long file */
  test_ValidPathA(curdir,NONDIR_LONG,LONGFILE,tmpstr,&passfail,"test12");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
      passfail.shorterror);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_PATH_NOT_FOUND ||
       passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetLongPathA returned %d and not 'ERROR_PATH_NOT_FOUND'\n",
       passfail.longerror);
  }
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
     "GetShortPathNameA error: len=%d error=%d tmpstr=[%s]\n",
     passfail.shortlen,passfail.shorterror,tmpstr);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetLongPathA returned %d and not 'ERROR_FILE_NOT_FOUND'\n",
       passfail.longerror);
  }
  sprintf(tmpstr,"%s\\",NONDIR_LONG);
  test_ValidPathA(curdir,"",tmpstr,tmpstr1,&passfail,"test16");
  ok(passfail.shortlen==0,"GetShortPathNameA passed when it shouldn't have\n");
  ok(passfail.shorterror==ERROR_PATH_NOT_FOUND ||
     passfail.shorterror==ERROR_FILE_NOT_FOUND ||
     !passfail.shorterror,
     "GetShortPathA returned %d and not 'ERROR_FILE_NOT_FOUND'\n",
      passfail.shorterror);
  if(pGetLongPathNameA) {
    ok(passfail.longlen==0,"GetLongPathNameA passed when it shouldn't have\n");
    ok(passfail.longerror==ERROR_FILE_NOT_FOUND,
       "GetLongPathA returned %d and not 'ERROR_FILE_NOT_FOUND'\n",
       passfail.longerror);
  }
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
  if(pGetLongPathNameA) {
    ok(pGetLongPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetLongPathNameA failed\n");
    ok(lstrcmpiA(tmpstr1,LONGDIR)==0,
       "GetLongPathNameA returned '%s' instead of '%s'\n",tmpstr1,LONGDIR);
  }
  sprintf(tmpstr,".\\%s",LONGDIR);
  ok(GetShortPathNameA(tmpstr,tmpstr1,MAX_PATH),"GetShortPathNameA failed\n");
  test_SplitShortPathA(tmpstr1,dir,eight,three);
  ok(lstrcmpiA(dir,".")==0 || dir[0]=='\0',
     "GetShortPathNameA did not keep relative directory [%s]\n",tmpstr1);
  if(pGetLongPathNameA) {
    ok(pGetLongPathNameA(tmpstr1,tmpstr1,MAX_PATH),"GetLongPathNameA failed %s\n",
       tmpstr);
    ok(lstrcmpiA(tmpstr1,tmpstr)==0,
       "GetLongPathNameA returned '%s' instead of '%s'\n",tmpstr1,tmpstr);
  }
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
}

static void test_GetTempPathA(char* tmp_dir)
{
    DWORD len, len_with_null;
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
    ok(len >= len_with_null, "Expected >= %u, got %u\n", len_with_null, len);

    len = GetTempPathA(0, NULL);
    ok(len >= len_with_null, "Expected >= %u, got %u\n", len_with_null, len);

    /* The call above gave us the buffer size that Windows thinks is needed
     * so the next call should work
     */
    lstrcpyA(buf, "foo");
    len = GetTempPathA(len, buf);
    ok(lstrcmpiA(buf, tmp_dir) == 0, "expected [%s], got [%s]\n",tmp_dir,buf);
    ok(len == strlen(buf), "returned length should be equal to the length of string\n");
}

static void test_GetTempPathW(char* tmp_dir)
{
    DWORD len, len_with_null;
    WCHAR buf[MAX_PATH];
    WCHAR tmp_dirW[MAX_PATH];
    static const WCHAR fooW[] = {'f','o','o',0};

    MultiByteToWideChar(CP_ACP,0,tmp_dir,-1,tmp_dirW,sizeof(tmp_dirW)/sizeof(*tmp_dirW));
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
    if (len==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(lstrcmpiW(buf, tmp_dirW) == 0, "GetTempPathW returned an incorrect temporary path\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");

    lstrcpyW(buf, fooW);
    len = GetTempPathW(1, buf);
    ok(buf[0] == 0, "unicode version should truncate the buffer to zero size\n");
    ok(len >= len_with_null, "Expected >= %u, got %u\n", len_with_null, len);

    len = GetTempPathW(0, NULL);
    ok(len >= len_with_null, "Expected >= %u, got %u\n", len_with_null, len);

    lstrcpyW(buf, fooW);
    len = GetTempPathW(len, buf);
    ok(lstrcmpiW(buf, tmp_dirW) == 0, "GetTempPathW returned an incorrect temporary path\n");
    ok(len == lstrlenW(buf), "returned length should be equal to the length of string\n");
}

static void test_GetTempPath(void)
{
    char save_TMP[MAX_PATH];
    char windir[MAX_PATH];
    char buf[MAX_PATH];

    GetEnvironmentVariableA("TMP", save_TMP, sizeof(save_TMP));

    /* test default configuration */
    trace("TMP=%s\n", save_TMP);
    strcpy(buf,save_TMP);
    if (buf[strlen(buf)-1]!='\\')
        strcat(buf,"\\");
    test_GetTempPathA(buf);
    test_GetTempPathW(buf);

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
}

static void test_GetLongPathNameW(void)
{
    DWORD length; 
    WCHAR empty[MAX_PATH];

    /* Not present in all windows versions */
    if(pGetLongPathNameW) 
    {
    SetLastError(0xdeadbeef); 
    length = pGetLongPathNameW(NULL,NULL,0);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("GetLongPathNameW is not implemented\n");
        return;
    }
    ok(0==length,"GetLongPathNameW returned %d but expected 0\n",length);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"GetLastError returned %d but expected ERROR_INVALID_PARAMETER\n",GetLastError());

    SetLastError(0xdeadbeef); 
    empty[0]=0;
    length = pGetLongPathNameW(empty,NULL,0);
    ok(0==length,"GetLongPathNameW returned %d but expected 0\n",length);
    ok(GetLastError()==ERROR_PATH_NOT_FOUND,"GetLastError returned %d but expected ERROR_PATH_NOT_FOUND\n",GetLastError());
    }
}

static void test_GetShortPathNameW(void)
{
    WCHAR test_path[] = { 'L', 'o', 'n', 'g', 'D', 'i', 'r', 'e', 'c', 't', 'o', 'r', 'y', 'N', 'a', 'm', 'e',  0 };
    WCHAR path[MAX_PATH];
    WCHAR short_path[MAX_PATH];
    DWORD length;
    HANDLE file;
    int ret;
    WCHAR name[] = { 't', 'e', 's', 't', 0 };
    WCHAR backSlash[] = { '\\', 0 };

    SetLastError(0xdeadbeef);
    GetTempPathW( MAX_PATH, path );
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("GetTempPathW is not implemented\n");
        return;
    }

    lstrcatW( path, test_path );
    lstrcatW( path, backSlash );
    ret = CreateDirectoryW( path, NULL );
    ok( ret, "Directory was not created. LastError = %d\n", GetLastError() );

    /* Starting a main part of test */
    length = GetShortPathNameW( path, short_path, 0 );
    ok( length, "GetShortPathNameW returned 0.\n" );
    ret = GetShortPathNameW( path, short_path, length );
    ok( ret, "GetShortPathNameW returned 0.\n" );
    lstrcatW( short_path, name );
    file = CreateFileW( short_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok( file != INVALID_HANDLE_VALUE, "File was not created.\n" );

    /* End test */
    CloseHandle( file );
    ret = DeleteFileW( short_path );
    ok( ret, "Cannot delete file.\n" );
    ret = RemoveDirectoryW( path );
    ok( ret, "Cannot delete directory.\n" );
}

static void test_GetSystemDirectory(void)
{
    CHAR    buffer[MAX_PATH + 4];
    DWORD   res;
    DWORD   total;

    SetLastError(0xdeadbeef);
    res = GetSystemDirectory(NULL, 0);
    /* res includes the terminating Zero */
    ok(res > 0, "returned %d with %d (expected '>0')\n", res, GetLastError());

    total = res;

    /* this crashes on XP */
    if (0) res = GetSystemDirectory(NULL, total);

    SetLastError(0xdeadbeef);
    res = GetSystemDirectory(NULL, total-1);
    /* 95+NT: total (includes the terminating Zero)
       98+ME: 0 with ERROR_INVALID_PARAMETER */
    ok( (res == total) || (!res && (GetLastError() == ERROR_INVALID_PARAMETER)),
        "returned %d with %d (expected '%d' or: '0' with "
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError(), total);

    if (total > MAX_PATH) return;

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectory(buffer, total);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %d with %d and '%s' (expected '%d' and a string)\n",
        res, GetLastError(), buffer, total-1);

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectory(buffer, total + 1);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %d with %d and '%s' (expected '%d' and a string)\n",
        res, GetLastError(), buffer, total-1);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectory(buffer, total-1);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %d with %d and '%s' (expected '%d')\n",
        res, GetLastError(), buffer, total);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetSystemDirectory(buffer, total-2);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %d with %d and '%s' (expected '%d')\n",
        res, GetLastError(), buffer, total);
}

static void test_GetWindowsDirectory(void)
{
    CHAR    buffer[MAX_PATH + 4];
    DWORD   res;
    DWORD   total;

    SetLastError(0xdeadbeef);
    res = GetWindowsDirectory(NULL, 0);
    /* res includes the terminating Zero */
    ok(res > 0, "returned %d with %d (expected '>0')\n", res, GetLastError());

    total = res;
    /* this crashes on XP */
    if (0) res = GetWindowsDirectory(NULL, total);

    SetLastError(0xdeadbeef);
    res = GetWindowsDirectory(NULL, total-1);
    /* 95+NT: total (includes the terminating Zero)
       98+ME: 0 with ERROR_INVALID_PARAMETER */
    ok( (res == total) || (!res && (GetLastError() == ERROR_INVALID_PARAMETER)),
        "returned %d with %d (expected '%d' or: '0' with "
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError(), total);

    if (total > MAX_PATH) return;

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectory(buffer, total);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %d with %d and '%s' (expected '%d' and a string)\n",
        res, GetLastError(), buffer, total-1);

    buffer[0] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectory(buffer, total + 1);
    /* res does not include the terminating Zero */
    ok( (res == (total-1)) && (buffer[0]),
        "returned %d with %d and '%s' (expected '%d' and a string)\n",
        res, GetLastError(), buffer, total-1);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectory(buffer, total-1);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %d with %d and '%s' (expected '%d')\n",
        res, GetLastError(), buffer, total);

    memset(buffer, '#', total + 1);
    buffer[total + 2] = '\0';
    SetLastError(0xdeadbeef);
    res = GetWindowsDirectory(buffer, total-2);
    /* res includes the terminating Zero) */
    ok( res == total, "returned %d with %d and '%s' (expected '%d')\n",
        res, GetLastError(), buffer, total);
}

static void test_NeedCurrentDirectoryForExePathA(void)
{
    /* Crashes in Windows */
    if (0)
        ok(pNeedCurrentDirectoryForExePathA(NULL), "returned FALSE for NULL\n");

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

    /* Crashes in Windows */
    if (0)
        ok(pNeedCurrentDirectoryForExePathW(NULL), "returned FALSE for NULL\n");

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
    ret = GetWindowsDirectory(buf, sizeof(buf));
    ok(ret, "GetWindowsDirectory error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* re-use the buffer returned by GetFullPathName */
    buf[2] = '/';
    SetLastError(0xdeadbeef);
    ret = GetFullPathName(buf + 2, sizeof(buf), buf, NULL);
    ok(ret, "GetFullPathName error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetSystemDirectory(buf, sizeof(buf));
    ok(ret, "GetSystemDirectory error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetCurrentDirectory(sizeof(buf), buf);
    ok(ret, "GetCurrentDirectory error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* TEMP is an environment variable, so it can't be tested for case-sensitivity */
    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetTempPath(sizeof(buf), buf);
    ok(ret, "GetTempPath error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(buf[strlen(buf)-1] == '\\', "Temporary path (%s) doesn't end in a slash\n", buf);

    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    ret = GetFullPathName(".", sizeof(buf), buf, NULL);
    ok(ret, "GetFullPathName error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    /* re-use the buffer returned by GetFullPathName */
    SetLastError(0xdeadbeef);
    ret = GetShortPathName(buf, buf, sizeof(buf));
    ok(ret, "GetShortPathName error %u\n", GetLastError());
    ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
    ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
    ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);

    if (pGetLongPathNameA)
    {
        /* re-use the buffer returned by GetShortPathName */
        SetLastError(0xdeadbeef);
        ret = pGetLongPathNameA(buf, buf, sizeof(buf));
        ok(ret, "GetLongPathNameA error %u\n", GetLastError());
        ok(ret < sizeof(buf), "buffer should be %u bytes\n", ret);
        ok(buf[1] == ':', "expected buf[1] == ':' got %c\n", buf[1]);
        ok(is_upper_case_letter(buf[0]), "expected buf[0] upper case letter got %c\n", buf[0]);
    }
#undef is_upper_case_letter
}

START_TEST(path)
{
    CHAR origdir[MAX_PATH],curdir[MAX_PATH], curDrive, otherDrive;
    pGetLongPathNameA = (void*)GetProcAddress( GetModuleHandleA("kernel32.dll"),
                                               "GetLongPathNameA" );
    pGetLongPathNameW = (void*)GetProcAddress(GetModuleHandleA("kernel32.dll") ,
                                               "GetLongPathNameW" );
    pNeedCurrentDirectoryForExePathA =
        (void*)GetProcAddress( GetModuleHandleA("kernel32.dll"),
                               "NeedCurrentDirectoryForExePathA" );
    pNeedCurrentDirectoryForExePathW =
        (void*)GetProcAddress( GetModuleHandleA("kernel32.dll"),
                               "NeedCurrentDirectoryForExePathW" );

    test_InitPathA(curdir, &curDrive, &otherDrive);
    test_CurrentDirectoryA(origdir,curdir);
    test_PathNameA(curdir, curDrive, otherDrive);
    test_CleanupPathA(origdir,curdir);
    test_GetTempPath();
    test_GetLongPathNameW();
    test_GetShortPathNameW();
    test_GetSystemDirectory();
    test_GetWindowsDirectory();
    if (pNeedCurrentDirectoryForExePathA)
    {
        test_NeedCurrentDirectoryForExePathA();
    }
    if (pNeedCurrentDirectoryForExePathW)
    {
        test_NeedCurrentDirectoryForExePathW();
    }
    test_drive_letter_case();
}
