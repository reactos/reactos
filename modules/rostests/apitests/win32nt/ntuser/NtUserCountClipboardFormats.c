/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCountClipboardFormats
 * PROGRAMMERS:     Colin Finck <colin@reactos.org>
 *                  Amine Khaldi <amine.khaldi@reactos.org>
 */

#include <win32nt.h>


START_TEST(NtUserCountClipboardFormats)
{
	RTEST(NtUserCountClipboardFormats() < 1000);
}

