
#ifdef USE_ICONV_H
#include <iconv.h>
#include <windows.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#else
#include "win_iconv.c"
#endif

#include <stdio.h>

const char *
tohex(const char *str, int size)
{
    static char buf[BUFSIZ];
    char *pbuf = buf;
    int i;
    buf[0] = 0;
    for (i = 0; i < size; ++i)
        pbuf += sprintf(pbuf, "%02X", str[i] & 0xFF);
    return buf;
}

const char *
errstr(int errcode)
{
    static char buf[BUFSIZ];
    switch (errcode)
    {
    case 0: return "NOERROR";
    case EINVAL: return "EINVAL";
    case EILSEQ: return "EILSEQ";
    case E2BIG: return "E2BIG";
    }
    sprintf(buf, "%d\n", errcode);
    return buf;
}

#ifdef USE_LIBICONV_DLL
int use_dll;

int
setdll(const char *dllpath)
{
    char buf[BUFSIZ];
    rec_iconv_t cd;

    sprintf(buf, "WINICONV_LIBICONV_DLL=%s", dllpath);
    putenv(buf);
    if (libiconv_iconv_open(&cd, "ascii", "ascii"))
    {
        FreeLibrary(cd.hlibiconv);
        use_dll = TRUE;
        return TRUE;
    }
    use_dll = FALSE;
    return FALSE;
}
#endif

/*
 * We can test the codepage that is installed in the system.
 */
int
check_enc(const char *encname, int codepage)
{
    iconv_t cd;
    int cp;
    cd = iconv_open("utf-8", encname);
    if (cd == (iconv_t)(-1))
    {
        printf("%s(%d) IS NOT SUPPORTED: SKIP THE TEST\n", encname, codepage);
        return FALSE;
    }
#ifndef USE_ICONV_H
    cp = ((rec_iconv_t *)cd)->from.codepage;
    if (cp != codepage)
    {
        printf("%s(%d) ALIAS IS MAPPED TO DIFFERENT CODEPAGE (%d)\n", encname, codepage, cp);
        exit(1);
    }
#endif
    iconv_close(cd);
    return TRUE;
}

void
test(const char *from, const char *fromstr, int fromsize, const char *to, const char *tostr, int tosize, int errcode, int bufsize, int line)
{
    char outbuf[BUFSIZ];
    const char *pin;
    char *pout;
    size_t inbytesleft;
    size_t outbytesleft;
    iconv_t cd;
    size_t r;
#ifdef USE_LIBICONV_DLL
    char dllpath[_MAX_PATH];
#endif

    cd = iconv_open(to, from);
    if (cd == (iconv_t)(-1))
    {
        printf("%s -> %s: NG: INVALID ENCODING NAME: line=%d\n", from, to, line);
        exit(1);
    }

#ifdef USE_LIBICONV_DLL
    if (((rec_iconv_t *)cd)->hlibiconv != NULL)
        GetModuleFileNameA(((rec_iconv_t *)cd)->hlibiconv, dllpath, sizeof(dllpath));

    if (use_dll && ((rec_iconv_t *)cd)->hlibiconv == NULL)
    {
        printf("%s: %s -> %s: NG: FAILED TO USE DLL: line=%d\n", dllpath, from, to, line);
        exit(1);
    }
    else if (!use_dll && ((rec_iconv_t *)cd)->hlibiconv != NULL)
    {
        printf("%s: %s -> %s: NG: DLL IS LOADED UNEXPECTEDLY: line=%d\n", dllpath, from, to, line);
        exit(1);
    }
#endif

    errno = 0;

    pin = (char *)fromstr;
    pout = outbuf;
    inbytesleft = fromsize;
    outbytesleft = bufsize;
    r = iconv(cd, &pin, &inbytesleft, &pout, &outbytesleft);
    if (r != (size_t)(-1))
        r = iconv(cd, NULL, NULL, &pout, &outbytesleft);
    *pout = 0;

#ifdef USE_LIBICONV_DLL
    if (use_dll)
        printf("%s: ", dllpath);
#endif
    printf("%s(%s) -> ", from, tohex(fromstr, fromsize));
    printf("%s(%s%s%s): ", to, tohex(tostr, tosize),
            errcode == 0 ? "" : ":",
            errcode == 0 ? "" : errstr(errcode));
    if (strcmp(outbuf, tostr) == 0 && errno == errcode)
        printf("OK\n");
    else
    {
        printf("RESULT(%s:%s): ", tohex(outbuf, sizeof(outbuf) - outbytesleft),
                errstr(errno));
        printf("NG: line=%d\n", line);
        exit(1);
    }
}

#define STATIC_STRLEN(arr) (sizeof(arr) - 1)

#define success(from, fromstr, to, tostr) test(from, fromstr, STATIC_STRLEN(fromstr), to, tostr, STATIC_STRLEN(tostr), 0, BUFSIZ, __LINE__)
#define einval(from, fromstr, to, tostr) test(from, fromstr, STATIC_STRLEN(fromstr), to, tostr, STATIC_STRLEN(tostr), EINVAL, BUFSIZ, __LINE__)
#define eilseq(from, fromstr, to, tostr) test(from, fromstr, STATIC_STRLEN(fromstr), to, tostr, STATIC_STRLEN(tostr), EILSEQ, BUFSIZ, __LINE__)
#define e2big(from, fromstr, to, tostr, bufsize) test(from, fromstr, STATIC_STRLEN(fromstr), to, tostr, STATIC_STRLEN(tostr), E2BIG, bufsize, __LINE__)

int
main(int argc, char **argv)
{
#ifdef USE_LIBICONV_DLL
    /* test use of dll if $DEFAULT_LIBICONV_DLL was defined. */
    if (setdll(""))
    {
        success("ascii", "ABC", "ascii", "ABC");
        success("ascii", "ABC", "utf-16be", "\x00\x41\x00\x42\x00\x43");
    }
    else
    {
        printf("\nDLL TEST IS SKIPPED\n\n");
    }

    setdll("none");
#endif

    if (check_enc("ascii", 20127))
    {
        success("ascii", "ABC", "ascii", "ABC");
        /* MSB is dropped.  Hmm... */
        success("ascii", "\x80\xFF", "ascii", "\x00\x7F");
    }

    /* unicode (CP1200 CP1201 CP12000 CP12001 CP65001) */
    if (check_enc("utf-8", 65001)
            && check_enc("utf-16be", 1201) && check_enc("utf-16le", 1200)
            && check_enc("utf-32be", 12001) && check_enc("utf-32le", 12000)
            )
    {
        /* Test the BOM behavior
         * 1. Remove the BOM when "fromcode" is utf-16 or utf-32.
         * 2. Add the BOM when "tocode" is utf-16 or utf-32.  */
        success("utf-16", "\xFE\xFF\x01\x02", "utf-16be", "\x01\x02");
        success("utf-16", "\xFF\xFE\x02\x01", "utf-16be", "\x01\x02");
        success("utf-32", "\x00\x00\xFE\xFF\x00\x00\x01\x02", "utf-32be", "\x00\x00\x01\x02");
        success("utf-32", "\xFF\xFE\x00\x00\x02\x01\x00\x00", "utf-32be", "\x00\x00\x01\x02");
        success("utf-16", "\xFE\xFF\x00\x01", "utf-8", "\x01");
#ifndef GLIB_COMPILATION
        success("utf-8", "\x01", "utf-16", "\xFE\xFF\x00\x01");
        success("utf-8", "\x01", "utf-32", "\x00\x00\xFE\xFF\x00\x00\x00\x01");
#else
        success("utf-8", "\x01", "utf-16", "\xFF\xFE\x01\x00");
        success("utf-8", "\x01", "utf-32", "\xFF\xFE\x00\x00\x01\x00\x00\x00");
#endif

        success("utf-16be", "\xFE\xFF\x01\x02", "utf-16be", "\xFE\xFF\x01\x02");
        success("utf-16le", "\xFF\xFE\x02\x01", "utf-16be", "\xFE\xFF\x01\x02");
        success("utf-32be", "\x00\x00\xFE\xFF\x00\x00\x01\x02", "utf-32be", "\x00\x00\xFE\xFF\x00\x00\x01\x02");
        success("utf-32le", "\xFF\xFE\x00\x00\x02\x01\x00\x00", "utf-32be", "\x00\x00\xFE\xFF\x00\x00\x01\x02");
        success("utf-16be", "\xFE\xFF\x00\x01", "utf-8", "\xEF\xBB\xBF\x01");
        success("utf-8", "\xEF\xBB\xBF\x01", "utf-8", "\xEF\xBB\xBF\x01");

        success("utf-16be", "\x01\x02", "utf-16le", "\x02\x01");
        success("utf-16le", "\x02\x01", "utf-16be", "\x01\x02");
        success("utf-16be", "\xFE\xFF", "utf-16le", "\xFF\xFE");
        success("utf-16le", "\xFF\xFE", "utf-16be", "\xFE\xFF");
        success("utf-32be", "\x00\x00\x03\x04", "utf-32le", "\x04\x03\x00\x00");
        success("utf-32le", "\x04\x03\x00\x00", "utf-32be", "\x00\x00\x03\x04");
        success("utf-32be", "\x00\x00\xFF\xFF", "utf-16be", "\xFF\xFF");
        success("utf-16be", "\xFF\xFF", "utf-32be", "\x00\x00\xFF\xFF");
        success("utf-32be", "\x00\x01\x00\x00", "utf-16be", "\xD8\x00\xDC\x00");
        success("utf-16be", "\xD8\x00\xDC\x00", "utf-32be", "\x00\x01\x00\x00");
        success("utf-32be", "\x00\x10\xFF\xFF", "utf-16be", "\xDB\xFF\xDF\xFF");
        success("utf-16be", "\xDB\xFF\xDF\xFF", "utf-32be", "\x00\x10\xFF\xFF");
        eilseq("utf-32be", "\x00\x11\x00\x00", "utf-16be", "");
        eilseq("utf-16be", "\xDB\xFF\xE0\x00", "utf-32be", "");
        success("utf-8", "\xE3\x81\x82", "utf-16be", "\x30\x42");
        einval("utf-8", "\xE3", "utf-16be", "");
    }

    /* Japanese (CP932 CP20932 CP50220 CP50221 CP50222 CP51932) */
    if (check_enc("cp932", 932)
            && check_enc("cp20932", 20932) && check_enc("euc-jp", 51932)
            && check_enc("cp50220", 50220) && check_enc("cp50221", 50221)
            && check_enc("cp50222", 50222) && check_enc("iso-2022-jp", 50221))
    {
        /* Test the compatibility for each other Japanese codepage.
         * And validate the escape sequence handling for iso-2022-jp. */
        success("utf-16be", "\xFF\x5E", "cp932", "\x81\x60");
        success("utf-16be", "\x30\x1C", "cp932", "\x81\x60");
        success("utf-16be", "\xFF\x5E", "cp932//nocompat", "\x81\x60");
        eilseq("utf-16be", "\x30\x1C", "cp932//nocompat", "");
        success("euc-jp", "\xA4\xA2", "utf-16be", "\x30\x42");
        einval("euc-jp", "\xA4\xA2\xA4", "utf-16be", "\x30\x42");
        eilseq("euc-jp", "\xA4\xA2\xFF\xFF", "utf-16be", "\x30\x42");
        success("cp932", "\x81\x60", "iso-2022-jp", "\x1B\x24\x42\x21\x41\x1B\x28\x42");
        success("UTF-16BE", "\xFF\x5E", "iso-2022-jp", "\x1B\x24\x42\x21\x41\x1B\x28\x42");
        eilseq("UTF-16BE", "\x30\x1C", "iso-2022-jp//nocompat", "");
        success("UTF-16BE", "\x30\x42\x30\x44", "iso-2022-jp", "\x1B\x24\x42\x24\x22\x24\x24\x1B\x28\x42");
        success("iso-2022-jp", "\x1B\x24\x42\x21\x41\x1B\x28\x42", "UTF-16BE", "\xFF\x5E");
    }

    /*
     * test for //translit
     * U+FF41 (FULLWIDTH LATIN SMALL LETTER A) <-> U+0062 (LATIN SMALL LETTER A)
     */
    eilseq("UTF-16BE", "\xFF\x41", "iso-8859-1", "");
    success("UTF-16BE", "\xFF\x41", "iso-8859-1//translit", "a");

    /*
     * test for //translit
     * Some character, not in "to" encoding -> DEFAULT CHARACTER (maybe "?")
     */
    eilseq("UTF-16BE", "\x30\x42", "ascii", "");
    success("UTF-16BE", "\x30\x42", "ascii//translit", "?");

    /*
     * test for //ignore
     */
    eilseq("UTF-8", "\xFF A \xFF B", "ascii//ignore", " A  B");
    eilseq("UTF-8", "\xEF\xBC\xA1 A \xEF\xBC\xA2 B", "ascii//ignore", " A  B");
    eilseq("UTF-8", "\xEF\x01 A \xEF\x02 B", "ascii//ignore", "\x01 A \x02 B");

    /*
     * TODO:
     * Test for state after iconv() failed.
     * Ensure iconv() error is safe and continuable.
     */

    return 0;
}

