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

struct _profileInt { 
    LPCSTR section;
    LPCSTR key;
    LPCSTR value;
    LPCSTR iniFile;
    INT defaultVal;
    UINT result;
};

static void test_profile_int(void)
{
    struct _profileInt profileInt[]={
         { NULL,    NULL, NULL,          NULL,     70, 0          }, /*  0 */
         { NULL,    NULL, NULL,          TESTFILE, -1, 4294967295U},
         { NULL,    NULL, NULL,          TESTFILE,  1, 1          },
         { SECTION, NULL, NULL,          TESTFILE, -1, 4294967295U},
         { SECTION, NULL, NULL,          TESTFILE,  1, 1          },
         { NULL,    KEY,  NULL,          TESTFILE, -1, 4294967295U}, /*  5 */
         { NULL,    KEY,  NULL,          TESTFILE,  1, 1          },
         { SECTION, KEY,  NULL,          TESTFILE, -1, 4294967295U},
         { SECTION, KEY,  NULL,          TESTFILE,  1, 1          },
         { SECTION, KEY,  "-1",          TESTFILE, -1, 4294967295U},
         { SECTION, KEY,  "-1",          TESTFILE,  1, 4294967295U}, /* 10 */
         { SECTION, KEY,  "1",           TESTFILE, -1, 1          },
         { SECTION, KEY,  "1",           TESTFILE,  1, 1          },
         { SECTION, KEY,  "+1",          TESTFILE, -1, 1          },
         { SECTION, KEY,  "+1",          TESTFILE,  1, 1          },
         { SECTION, KEY,  "4294967296",  TESTFILE, -1, 0          }, /* 15 */
         { SECTION, KEY,  "4294967296",  TESTFILE,  1, 0          },
         { SECTION, KEY,  "4294967297",  TESTFILE, -1, 1          },
         { SECTION, KEY,  "4294967297",  TESTFILE,  1, 1          },
         { SECTION, KEY,  "-4294967297", TESTFILE, -1, 4294967295U},
         { SECTION, KEY,  "-4294967297", TESTFILE,  1, 4294967295U}, /* 20 */
         { SECTION, KEY,  "42A94967297", TESTFILE, -1, 42         },
         { SECTION, KEY,  "42A94967297", TESTFILE,  1, 42         },
         { SECTION, KEY,  "B4294967297", TESTFILE, -1, 0          },
         { SECTION, KEY,  "B4294967297", TESTFILE,  1, 0          },
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
       ok(res == profileInt[i].result, "test<%02d>: ret<%010u> exp<%010u>\n",
                                       i, res, profileInt[i].result);
    }

    DeleteFileA( TESTFILE);
}

START_TEST(profile)
{
    test_profile_int();
}
