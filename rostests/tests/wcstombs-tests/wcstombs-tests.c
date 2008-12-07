/*
 * PROJECT:    ReactOS wcstombs Test Suite
 * LICENSE:    GPL v2 or any later version
 * FILE:       tests/wcstombs-tests/wcstombs-tests.c
 * PURPOSE:    Application for testing the CRT API's (wcstombs and wctomb) and the Win32 API WideCharToMultiByte for the Unicode to MultiByte string conversion
 * COPYRIGHT:  Copyright 2008 Colin Finck <colin@reactos.org>
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Macros for simplification */
#define SETLOCALE(locale) \
    loc = setlocale(LC_ALL, locale); \
    if(!loc) \
    { \
        puts("setlocale failed for " locale ", this locale is probably not installed on your system"); \
        return; \
    }

#define OK(condition, fail_message, ...) \
    if(!(condition)) \
        printf("%d: " fail_message "\n", __LINE__, ##__VA_ARGS__);

/* Global variables for easier handling */
char mbc;
char mbs[5];
int ret;
wchar_t wc1 = 228;                                  /* Western Windows-1252 character */
wchar_t wc2 = 1088;                                 /* Russian Windows-1251 character not displayable for Windows-1252 */
wchar_t wcs[5] = {'T', 'h', 1088, 'i', 0};          /* String with ASCII characters and a Russian character */
wchar_t dbwcs[3] = {28953, 25152, 0};               /* String with Chinese (codepage 950) characters */


void CRT_Tests()
{
    char* loc;

    puts("CRT-Tests");
    puts("---------");

    /* Current locale is "C", wcstombs should return the length of the input buffer without the terminating null character */
    ret = wcstombs(NULL, dbwcs, 0);
    OK(ret == 2, "ret is %d", ret);

    ret = wcstombs(mbs, dbwcs, ret);
    OK(ret == -1, "ret is %d", ret);
    OK(mbs[0] == 0, "mbs[0] is %d", mbs[0]);
    OK(errno == EILSEQ, "errno is %d", errno);

    ret = wcstombs(NULL, wcs, 0);
    OK(ret == 4, "ret is %d", ret);

    ret = wcstombs(mbs, wcs, ret);
    OK(ret == -1, "ret is %d", ret);
    OK(!strcmp(mbs, "Th"), "mbs is %s", mbs);
    OK(errno == EILSEQ, "errno is %d", errno);

    ret = wctomb(&mbc, wcs[0]);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == 84, "mbc is %d", mbc);

    mbc = 84;
    ret = wcstombs(&mbc, &dbwcs[0], 1);
    OK(ret == -1, "ret is %d", ret);
    OK(mbc == 84, "mbc is %d", mbc);

    ret = wcstombs(mbs, wcs, 0);
    OK(ret == 0, "ret is %d", ret);

    /* The length for the null character (in any locale) is 0, but if you pass a variable, it will be set to 0 and wctomb returns 1 */
    ret = wctomb(NULL, 0);
    OK(ret == 0, "ret is %d", ret);

    ret = wctomb(&mbc, 0);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == 0, "mbc is %d", mbc);

    /* msvcr80.dll changes mbc in the following call back to 0, msvcrt.dll from WinXP SP2 leaves it untouched */
    mbc = 84;
    ret = wctomb(&mbc, dbwcs[0]);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);
    OK(mbc == 84, "mbc is %d", mbc);

    /* With a real locale, -1 also becomes a possible return value in case of an invalid character */
    SETLOCALE("German");
    ret = wcstombs(NULL, dbwcs, 0);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);

    ret = wcstombs(NULL, wcs, 2);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);

    /* Test if explicitly setting the locale back to "C" also leads to the same results as above */
    SETLOCALE("C");

    ret = wcstombs(NULL, dbwcs, 0);
    OK(ret == 2, "ret is %d", ret);

    ret = wcstombs(NULL, wcs, 0);
    OK(ret == 4, "ret is %d", ret);

    /* Test wctomb() as well */
    SETLOCALE("English");

    ret = wctomb(&mbc, wc1);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == -28, "mbc is %d", mbc);

    ret = wctomb(&mbc, wc2);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);
    OK(mbc == 63, "mbc is %d", mbc);

    SETLOCALE("Russian");

    ret = wcstombs(mbs, wcs, sizeof(mbs));
    OK(ret == 4, "ret is %d", ret);
    OK(!strcmp(mbs, "Thði"), "mbs is %s", mbs);

    ret = wctomb(&mbc, wc2);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == -16, "mbc is %d", mbc);

    ret = wctomb(&mbc, wc1);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == 97, "mbc is %d", mbc);

    SETLOCALE("English");

    ret = wcstombs(&mbc, wcs, 1);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == 84, "mbc is %d", mbc);

    ZeroMemory(mbs, sizeof(mbs));
    ret = wcstombs(mbs, wcs, sizeof(mbs));
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);
    OK(!strcmp(mbs, "Th?i"), "mbs is %s", mbs);
    mbs[0] = 0;

    /* wcstombs mustn't add any null character automatically.
       So in this case, we should get the same string again, even if we only copied the first three bytes. */
    ret = wcstombs(mbs, wcs, 3);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);
    OK(!strcmp(mbs, "Th?i"), "mbs is %s", mbs);
    ZeroMemory(mbs, 5);

    /* Now this shouldn't be the case like above as we zeroed the complete string buffer. */
    ret = wcstombs(mbs, wcs, 3);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);
    OK(!strcmp(mbs, "Th?"), "mbs is %s", mbs);

    /* Double-byte tests */
    SETLOCALE("Chinese");
    ret = wcstombs(mbs, dbwcs, sizeof(mbs));
    OK(ret == 4, "ret is %d", ret);
    OK(!strcmp(mbs, "µH©Ò"), "mbs is %s", mbs);
    ZeroMemory(mbs, 5);

    /* Length-only tests */
    SETLOCALE("English");
    ret = wcstombs(NULL, wcs, 0);
    OK(ret == -1, "ret is %d", ret);
    OK(errno == EILSEQ, "errno is %d", errno);

    SETLOCALE("Chinese");
    ret = wcstombs(NULL, dbwcs, 0);
    OK(ret == 4, "ret is %d", ret);

    /* This call causes an ERROR_INSUFFICIENT_BUFFER in the called WideCharToMultiByte function.
       For some reason, wcstombs under Windows doesn't reset the last error to the previous value here, so we can check for ERROR_INSUFFICIENT_BUFFER with GetLastError().
       This could also be seen as an indication that Windows uses WideCharToMultiByte internally for wcstombs. */
    ret = wcstombs(mbs, dbwcs, 1);
    OK(ret == 0, "ret is %d", ret);
    OK(mbs[0] == 0, "mbs[0] is %d", mbs[0]);

    /* ERROR_INSUFFICIENT_BUFFER is also the result of this call with SBCS characters. WTF?!
       Anyway this is a Win32 error not related to the CRT, so we leave out this criteria. */
    ret = wcstombs(mbs, wcs, 1);
    OK(ret == 1, "ret is %d", ret);
    OK(mbs[0] == 84, "mbs[0] is %d", mbs[0]);

    putchar('\n');
}

void Win32_Tests(LPBOOL bUsedDefaultChar)
{
    /*int i;*/

    SetLastError(0xdeadbeef);

    puts("Win32-Tests");
    puts("-----------");

    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &wc1, 1, &mbc, 1, NULL, bUsedDefaultChar);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == -28, "mbc is %d", mbc);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &wc2, 1, &mbc, 1, NULL, bUsedDefaultChar);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == 63, "mbc is %d", mbc);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(1251, WC_NO_BEST_FIT_CHARS, &wc2, 1, &mbc, 1, NULL, bUsedDefaultChar);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == -16, "mbc is %d", mbc);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(1251, WC_NO_BEST_FIT_CHARS, &wc1, 1, &mbc, 1, NULL, bUsedDefaultChar);
    OK(ret == 1, "ret is %d", ret);
    OK(mbc == 97, "mbc is %d", mbc);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    /* This call triggers the last Win32 error */
    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, -1, &mbc, 1, NULL, bUsedDefaultChar);
    OK(ret == 0, "ret is %d", ret);
    OK(mbc == 84, "mbc is %d", mbc);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() is %lu", GetLastError());
    SetLastError(0xdeadbeef);

    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, -1, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 5, "ret is %d", ret);
    OK(!strcmp(mbs, "Th?i"), "mbs is %s", mbs);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());
    mbs[0] = 0;

    /* WideCharToMultiByte mustn't add any null character automatically.
       So in this case, we should get the same string again, even if we only copied the first three bytes. */
    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, 3, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 3, "ret is %d", ret);
    OK(!strcmp(mbs, "Th?i"), "mbs is %s", mbs);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());
    ZeroMemory(mbs, 5);

    /* Now this shouldn't be the case like above as we zeroed the complete string buffer. */
    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, 3, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 3, "ret is %d", ret);
    OK(!strcmp(mbs, "Th?"), "mbs is %s", mbs);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    /* Double-byte tests */
    ret = WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS, dbwcs, -1, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 5, "ret is %d", ret);
    OK(!strcmp(mbs, "µH©Ò"), "mbs is %s", mbs);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS, dbwcs, 1, &mbc, 1, NULL, bUsedDefaultChar);
    OK(ret == 0, "ret is %d", ret);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar == FALSE");
    OK(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() is %lu", GetLastError());
    SetLastError(0xdeadbeef);
    ZeroMemory(mbs, 5);

    ret = WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS, dbwcs, 1, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 2, "ret is %d", ret);
    OK(!strcmp(mbs, "µH"), "mbs is %s", mbs);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    /* Length-only tests */
    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &wc2, 1, NULL, 0, NULL, bUsedDefaultChar);
    OK(ret == 1, "ret is %d", ret);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, -1, NULL, 0, NULL, bUsedDefaultChar);
    OK(ret == 5, "ret is %d", ret);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS, dbwcs, 1, NULL, 0, NULL, bUsedDefaultChar);
    OK(ret == 2, "ret is %d", ret);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    ret = WideCharToMultiByte(950, WC_NO_BEST_FIT_CHARS, dbwcs, -1, NULL, 0, NULL, bUsedDefaultChar);
    OK(ret == 5, "ret is %d", ret);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == 0xdeadbeef, "GetLastError() is %lu", GetLastError());

    /* Abnormal uses of WideCharToMultiByte */
    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, NULL, 5, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 0, "ret is %d", ret);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == FALSE, "bUsedDefaultChar is %d", *bUsedDefaultChar);
    OK(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() is %lu", GetLastError());
    SetLastError(0xdeadbeef);

    ret = WideCharToMultiByte(0, WC_NO_BEST_FIT_CHARS, dbwcs, 5, mbs, sizeof(mbs), NULL, bUsedDefaultChar);
    OK(ret == 5, "ret is %d", ret);
    OK(!strcmp(mbs, "??"), "mbs is %s", mbs);
    if(bUsedDefaultChar) OK(*bUsedDefaultChar == TRUE, "bUsedDefaultChar is %d", *bUsedDefaultChar);

    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, -1, (LPSTR)wcs, 5, NULL, bUsedDefaultChar);
    OK(ret == 0, "ret is %d", ret);
    OK(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() is %lu", GetLastError());
    SetLastError(0xdeadbeef);

    ret = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wcs, -1, mbs, -1, NULL, bUsedDefaultChar);
    OK(ret == 0, "ret is %d", ret);
    OK(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() is %lu", GetLastError());
    SetLastError(0xdeadbeef);

    putchar('\n');
}

int main()
{
    BOOL UsedDefaultChar;

    CRT_Tests();

    /* There are two code pathes in WideCharToMultiByte, one when Flags || DefaultChar || UsedDefaultChar is set and one when it's not.
       Test both here. */
    Win32_Tests(NULL);
    Win32_Tests(&UsedDefaultChar);

    return 0;
}
