 /* Unit test suite for the wsprintf functions
 *
 * Copyright 2002 Bill Medland
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
#include "winuser.h"

static void wsprintfATest(void)
{
    char buf[25];
    int rc;

    rc=wsprintfA(buf, "%010ld", -1);
    ok(rc == 10, "wsPrintfA length failure: rc=%d error=%ld\n",rc,GetLastError());
    ok((lstrcmpA(buf, "-000000001") == 0),
       "wsprintfA zero padded negative value failure: buf=[%s]\n",buf);
}

static void wsprintfWTest(void)
{
    static const WCHAR fmt[] = {'%','0','1','0','l','d','\0'};
    static const WCHAR target[] = {'-','0','0','0','0','0','0','0','0','1', '\0'};
    WCHAR buf[25];
    int rc;

    rc=wsprintfW(buf, fmt, -1);
    if (rc==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(rc == 10, "wsPrintfW length failure: rc=%d error=%ld\n",rc,GetLastError());
    ok((lstrcmpW(buf, target) == 0),
       "wsprintfW zero padded negative value failure\n");
}

START_TEST(wsprintf)
{
    wsprintfATest();
    wsprintfWTest();
}
