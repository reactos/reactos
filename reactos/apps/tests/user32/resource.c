/* Unit test suite for resources.
 *
 * Copyright 2004 Ferenc Wagner
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

#include <assert.h>
#include <windows.h>

#include "wine/test.h"

void
test_LoadStringA (void)
{
    HINSTANCE hInst = GetModuleHandle (NULL);
    const char str[] = "String resource"; /* same in resource.rc */
    char buf[128];
    struct string_test {
        int bufsiz;
        int expected;
    };
    struct string_test tests[] = {{sizeof buf, sizeof str - 1},
                                  {sizeof str, sizeof str - 1},
                                  {sizeof str - 1, sizeof str - 2}};
    int i;

    assert (sizeof str < sizeof buf);
    for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        const int bufsiz = tests[i].bufsiz;
        const int expected = tests[i].expected;
        const int len = LoadStringA (hInst, 0, buf, bufsiz);

        ok (len == expected, "bufsiz=%d: got %d, expected %d\n",
            bufsiz, len, expected);
        ok (!memcmp (buf, str, len),
            "bufsiz=%d: got '%s', expected '%.*s'\n",
            bufsiz, buf, len, str);
        ok (buf[len] == 0, "bufsiz=%d: NUL termination missing\n",
            bufsiz);
    }
}

START_TEST(resource)
{
    test_LoadStringA ();
}
