/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateFont
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#define INVALIDFONT "ThisFontDoesNotExist"

void Test_CreateFontA()
{
	HFONT hFont;
	LOGFONTA logfonta;
	INT result;

	/* Test invalid font name */
	hFont = CreateFontA(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
	                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
	                    DEFAULT_QUALITY, DEFAULT_PITCH, INVALIDFONT);
	ok(hFont != 0, "CreateFontA failed\n");

	result = GetObjectA(hFont, sizeof(LOGFONTA), &logfonta);
	ok(result == sizeof(LOGFONTA), "result = %d", result);

	ok(memcmp(logfonta.lfFaceName, INVALIDFONT, strlen(INVALIDFONT)) == 0, "not equal\n");
	ok(logfonta.lfWeight == FW_DONTCARE, "lfWeight=%ld\n", logfonta.lfWeight);

}

START_TEST(CreateFont)
{
    Test_CreateFontA();
}

