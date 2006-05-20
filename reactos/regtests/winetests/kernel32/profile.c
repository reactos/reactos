/*
 * Unit tests for profile functions
 *
 * Copyright (c) 2003 Stefan Leichter
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "windows.h"

#define KEY      "ProfileInt"
#define SECTION  "Test"
#define TESTFILE ".\\testwine.ini"
#define TESTFILE2 ".\\testwine2.ini"

struct _profileInt { 
    LPCSTR section;
    LPCSTR key;
    LPCSTR value;
    LPCSTR iniFile;
    INT defaultVal;
    UINT result;
    UINT result9x;
};

static void test_profile_int(void)
{
    struct _profileInt profileInt[]={
         { NULL,    NULL, NULL,          NULL,     70, 0          , 0}, /*  0 */
         { NULL,    NULL, NULL,          TESTFILE, -1, 4294967295U, 0},
         { NULL,    NULL, NULL,          TESTFILE,  1, 1          , 0},
         { SECTION, NULL, NULL,          TESTFILE, -1, 4294967295U, 0},
         { SECTION, NULL, NULL,          TESTFILE,  1, 1          , 0},
         { NULL,    KEY,  NULL,          TESTFILE, -1, 4294967295U, 0}, /*  5 */
         { NULL,    KEY,  NULL,          TESTFILE,  1, 1          , 0},
         { SECTION, KEY,  NULL,          TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  NULL,          TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "-1",          TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  "-1",          TESTFILE,  1, 4294967295U, 4294967295U}, /* 10 */
         { SECTION, KEY,  "1",           TESTFILE, -1, 1          , 1},
         { SECTION, KEY,  "1",           TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "+1",          TESTFILE, -1, 1          , 0},
         { SECTION, KEY,  "+1",          TESTFILE,  1, 1          , 0},
         { SECTION, KEY,  "4294967296",  TESTFILE, -1, 0          , 0}, /* 15 */
         { SECTION, KEY,  "4294967296",  TESTFILE,  1, 0          , 0},
         { SECTION, KEY,  "4294967297",  TESTFILE, -1, 1          , 1},
         { SECTION, KEY,  "4294967297",  TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "-4294967297", TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  "-4294967297", TESTFILE,  1, 4294967295U, 4294967295U}, /* 20 */
         { SECTION, KEY,  "42A94967297", TESTFILE, -1, 42         , 42},
         { SECTION, KEY,  "42A94967297", TESTFILE,  1, 42         , 42},
         { SECTION, KEY,  "B4294967297", TESTFILE, -1, 0          , 0},
         { SECTION, KEY,  "B4294967297", TESTFILE,  1, 0          , 0},
    };
    int i, num_test = (sizeof(profileInt)/sizeof(struct _profileInt));
    UINT res;

    DeleteFileA( TESTFILE);

    for (i=0; i < num_test; i++) {
        if (profileInt[i].value)
            WritePrivateProfileStringA(SECTION, KEY, profileInt[i].value, 
                                      profileInt[i].iniFile);

       res = GetPrivateProfileIntA(profileInt[i].section, profileInt[i].key, 
                 profileInt[i].defaultVal, profileInt[i].iniFile); 
       ok((res == profileInt[i].result) || (res == profileInt[i].result9x),
	    "test<%02d>: ret<%010u> exp<%010u><%010u>\n",
            i, res, profileInt[i].result, profileInt[i].result9x);
    }

    DeleteFileA( TESTFILE);
}

static void test_profile_string(void)
{
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    char *p;
    /* test that lines without an '=' will not be enumerated */
    /* in the case below, name2 is a key while name3 is not. */
    char content[]="[s]\r\nname1=val1\r\nname2=\r\nname3\r\nname4=val4\r\n";
    DeleteFileA( TESTFILE2);
    h = CreateFileA( TESTFILE2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", TESTFILE2);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* enumerate the keys */
    ret=GetPrivateProfileStringA( "s", NULL, "", buf, sizeof(buf),
        TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    /* and test */
    ok( ret == 18 && !strcmp( buf, "name1,name2,name4"), "wrong keys returned(%d): %s\n", ret,
            buf);

    ret=GetPrivateProfileSectionA("s", buf, sizeof(buf), TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    /* and test */
    ok( ret == 35 && !strcmp( buf, "name1=val1,name2=,name3,name4=val4"), "wrong section returned(%d): %s\n",
            ret, buf);
    
    /* add a new key to test that the file is quite usable */
    WritePrivateProfileStringA( "s", "name5", "val5", TESTFILE2); 
    ret=GetPrivateProfileStringA( "s", NULL, "", buf, sizeof(buf),
        TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    ok( ret == 24 && !strcmp( buf, "name1,name2,name4,name5"), "wrong keys returned(%d): %s\n",
            ret, buf);

    DeleteFileA( TESTFILE2);
}

START_TEST(profile)
{
    test_profile_int();
    test_profile_string();
}
