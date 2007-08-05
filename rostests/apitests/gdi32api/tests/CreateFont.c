#include "../gdi32api.h"

#define INVALIDFONT "ThisFontDoesNotExist"

INT
Test_CreateFont(PTESTINFO pti)
{
	HFONT hFont;
	LOGFONTA logfonta;

	/* Test invalid font name */
	hFont = CreateFontA(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, 
	                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
	                    DEFAULT_QUALITY, DEFAULT_PITCH, INVALIDFONT);
	RTEST(hFont);
	RTEST(GetObjectA(hFont, sizeof(LOGFONTA), &logfonta) == sizeof(LOGFONTA));
	RTEST(memcmp(logfonta.lfFaceName, INVALIDFONT, strlen(INVALIDFONT)) == 0);
	RTEST(logfonta.lfWeight == FW_DONTCARE);


	return APISTATUS_NORMAL;
}


