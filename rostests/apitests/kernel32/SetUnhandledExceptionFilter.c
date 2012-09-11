/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SetUnhandledExceptionFilter
 * PROGRAMMER:      Mike "tamlin" Nordell
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <wine/test.h>
#include <ndk/rtltypes.h>

/*
 * Keep these returning different values, to prevent compiler folding
 * them into a single function, thereby voiding the test
 */
WINAPI LONG Filter1(LPEXCEPTION_POINTERS p) { return 0; }
WINAPI LONG Filter2(LPEXCEPTION_POINTERS p) { return 1; }


/*
 * Verify that SetUnhandledExceptionFilter actually returns the
 * _previous_ handler.
 */
static
VOID
TestSetUnhandledExceptionFilter(VOID)
{
	LPTOP_LEVEL_EXCEPTION_FILTER p1, p2;
	p1 = SetUnhandledExceptionFilter(Filter1);
	p2 = SetUnhandledExceptionFilter(Filter2);
	ok(p1 != Filter1, "SetUnhandledExceptionFilter returned what was set, not prev\n");
	ok(p2 != Filter2, "SetUnhandledExceptionFilter returned what was set, not prev\n");
	ok(p2 == Filter1, "SetUnhandledExceptionFilter didn't return previous filter\n");
	ok(p1 != p2, "SetUnhandledExceptionFilter seems to return random stuff\n");
}

START_TEST(SetUnhandledExceptionFilter)
{
    TestSetUnhandledExceptionFilter();
}
