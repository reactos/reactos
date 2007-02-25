/*
 * INF file parsing tests
 *
 * Copyright 2002, 2005 Alexandre Julliard for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

static const char tmpfile[] = ".\\tmp.inf";

/* some large strings */
#define A255 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define A256 "a" A255
#define A400 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaa" A256
#define A511 A255 A256
#define A4097 "a" A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256

#define STD_HEADER "[Version]\r\nSignature=\"$CHICAGO$\"\r\n"

#define STR_SECTION "[Strings]\nfoo=aaa\nbar=bbb\nloop=%loop2%\nloop2=%loop%\n" \
                    "per%%cent=abcd\nper=1\ncent=2\n22=foo\n" \
                    "big=" A400 "\n" \
                    "verybig=" A400 A400 A400 "\n"

/* create a new file with specified contents and open it */
static HINF test_file_contents( const char *data, UINT *err_line )
{
    DWORD res;
    HANDLE handle = CreateFileA( tmpfile, GENERIC_READ|GENERIC_WRITE,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return 0;
    if (!WriteFile( handle, data, strlen(data), &res, NULL )) trace( "write error\n" );
    CloseHandle( handle );
    return SetupOpenInfFileA( tmpfile, 0, INF_STYLE_WIN4, err_line );
}

static const char *get_string_field( INFCONTEXT *context, DWORD index )
{
    static char buffer[MAX_INF_STRING_LENGTH+32];
    if (SetupGetStringFieldA( context, index, buffer, sizeof(buffer), NULL )) return buffer;
    return NULL;
}

static const char *get_line_text( INFCONTEXT *context )
{
    static char buffer[MAX_INF_STRING_LENGTH+32];
    if (SetupGetLineTextA( context, 0, 0, 0, buffer, sizeof(buffer), NULL )) return buffer;
    return NULL;
}


/* Test various valid/invalid file formats */

static const struct
{
    const char *data;
    DWORD error;
    UINT err_line;
    int todo;
} invalid_files[] =
{
    /* file contents                                         expected error (or 0)     errline  todo */
    { "\r\n",                                                ERROR_WRONG_INF_STYLE,       0,    0 },
    { "abcd\r\n",                                            ERROR_WRONG_INF_STYLE,       0,    1 },
    { "[Version]\r\n",                                       ERROR_WRONG_INF_STYLE,       0,    0 },
    { "[Version]\nSignature=",                               ERROR_WRONG_INF_STYLE,       0,    0 },
    { "[Version]\nSignature=foo",                            ERROR_WRONG_INF_STYLE,       0,    0 },
    { "[version]\nsignature=$chicago$",                      0,                           0,    0 },
    { "[VERSION]\nSIGNATURE=$CHICAGO$",                      0,                           0,    0 },
    { "[Version]\nSignature=$chicago$,abcd",                 0,                           0,    0 },
    { "[Version]\nabc=def\nSignature=$chicago$",             0,                           0,    0 },
    { "[Version]\nabc=def\n[Version]\nSignature=$chicago$",  0,                           0,    0 },
    { STD_HEADER,                                            0,                           0,    0 },
    { STD_HEADER "[]\r\n",                                   0,                           0,    0 },
    { STD_HEADER "]\r\n",                                    0,                           0,    0 },
    { STD_HEADER "[" A255 "]\r\n",                           0,                           0,    0 },
    { STD_HEADER "[ab\r\n",                                  ERROR_BAD_SECTION_NAME_LINE, 3,    0 },
    { STD_HEADER "\n\n[ab\x1a]\n",                           ERROR_BAD_SECTION_NAME_LINE, 5,    0 },
    { STD_HEADER "[" A256 "]\r\n",                           ERROR_SECTION_NAME_TOO_LONG, 3,    0 },
    { "[abc]\n" STD_HEADER,                                  0,                           0,    0 },
    { "abc\r\n" STD_HEADER,                                  ERROR_EXPECTED_SECTION_NAME, 1,    0 },
    { ";\n;\nabc\r\n" STD_HEADER,                            ERROR_EXPECTED_SECTION_NAME, 3,    0 },
    { ";\n;\nab\nab\n" STD_HEADER,                           ERROR_EXPECTED_SECTION_NAME, 3,    0 },
    { ";aa\n;bb\n" STD_HEADER,                               0,                           0,    0 },
    { STD_HEADER " [TestSection\x00] \n",                    ERROR_BAD_SECTION_NAME_LINE, 3,    0 },
    { STD_HEADER " [Test\x00Section] \n",                    ERROR_BAD_SECTION_NAME_LINE, 3,    0 },
    { STD_HEADER " [TestSection\x00] \n",                    ERROR_BAD_SECTION_NAME_LINE, 3,    0 },
    { STD_HEADER " [Test\x00Section] \n",                    ERROR_BAD_SECTION_NAME_LINE, 3,    0 },
};

static void test_invalid_files(void)
{
    unsigned int i;
    UINT err_line;
    HINF hinf;
    DWORD err;

    for (i = 0; i < sizeof(invalid_files)/sizeof(invalid_files[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        err_line = 0xdeadbeef;
        hinf = test_file_contents( invalid_files[i].data, &err_line );
        err = GetLastError();
        trace( "hinf=%p err=%lx line=%d\n", hinf, err, err_line );
        if (invalid_files[i].error)  /* should fail */
        {
            ok( hinf == INVALID_HANDLE_VALUE, "file %u: Open succeeded\n", i );
            if (invalid_files[i].todo) todo_wine
            {
                ok( err == invalid_files[i].error, "file %u: Bad error %lx/%lx\n",
                    i, err, invalid_files[i].error );
                ok( err_line == invalid_files[i].err_line, "file %u: Bad error line %d/%d\n",
                    i, err_line, invalid_files[i].err_line );
            }
            else
            {
                ok( err == invalid_files[i].error, "file %u: Bad error %lx/%lx\n",
                    i, err, invalid_files[i].error );
                ok( err_line == invalid_files[i].err_line, "file %u: Bad error line %d/%d\n",
                    i, err_line, invalid_files[i].err_line );
            }
        }
        else  /* should succeed */
        {
            ok( hinf != INVALID_HANDLE_VALUE, "file %u: Open failed\n", i );
            ok( err == 0, "file %u: Error code set to %lx\n", i, err );
        }
        if (hinf != INVALID_HANDLE_VALUE) SetupCloseInfFile( hinf );
    }
}


/* Test various section names */

static const struct
{
    const char *data;
    const char *section;
    DWORD error;
} section_names[] =
{
    /* file contents                              section name       error code */
    { STD_HEADER "[TestSection]",                 "TestSection",         0 },
    { STD_HEADER "[TestSection]\n",               "TestSection",         0 },
    { STD_HEADER "[TESTSECTION]\r\n",             "TestSection",         0 },
    { STD_HEADER "[TestSection]\n[abc]",          "testsection",         0 },
    { STD_HEADER ";[TestSection]\n",              "TestSection",         ERROR_SECTION_NOT_FOUND },
    { STD_HEADER "[TestSection]\n",               "Bad name",            ERROR_SECTION_NOT_FOUND },
    /* spaces */
    { STD_HEADER "[TestSection]   \r\n",          "TestSection",         0 },
    { STD_HEADER "   [TestSection]\r\n",          "TestSection",         0 },
    { STD_HEADER "   [TestSection]   dummy\r\n",  "TestSection",         0 },
    { STD_HEADER "   [TestSection]   [foo]\r\n",  "TestSection",         0 },
    { STD_HEADER " [  Test Section  ] dummy\r\n", "  Test Section  ",    0 },
    { STD_HEADER "[TestSection] \032\ndummy",     "TestSection",         0 },
    { STD_HEADER "[TestSection] \n\032dummy",     "TestSection",         0 },
    /* special chars in section name */
    { STD_HEADER "[Test[Section]\r\n",            "Test[Section",        0 },
    { STD_HEADER "[Test[S]ection]\r\n",           "Test[S",              0 },
    { STD_HEADER "[Test[[[Section]\r\n",          "Test[[[Section",      0 },
    { STD_HEADER "[]\r\n",                        "",                    0 },
    { STD_HEADER "[[[]\n",                        "[[",                  0 },
    { STD_HEADER "[Test\"Section]\r\n",           "Test\"Section",       0 },
    { STD_HEADER "[Test\\Section]\r\n",           "Test\\Section",       0 },
    { STD_HEADER "[Test\\ Section]\r\n",          "Test\\ Section",      0 },
    { STD_HEADER "[Test;Section]\r\n",            "Test;Section",        0 },
    /* various control chars */
    { STD_HEADER " [Test\r\b\tSection] \n",       "Test\r\b\tSection", 0 },
    /* nulls */
};

static void test_section_names(void)
{
    unsigned int i;
    UINT err_line;
    HINF hinf;
    DWORD err;
    LONG ret;

    for (i = 0; i < sizeof(section_names)/sizeof(section_names[0]); i++)
    {
        SetLastError( 0xdeadbeef );
        hinf = test_file_contents( section_names[i].data, &err_line );
        ok( hinf != INVALID_HANDLE_VALUE, "line %u: open failed err %lx\n", i, GetLastError() );
        if (hinf == INVALID_HANDLE_VALUE) continue;

        ret = SetupGetLineCountA( hinf, section_names[i].section );
        err = GetLastError();
        trace( "hinf=%p ret=%ld err=%lx\n", hinf, ret, err );
        if (ret != -1)
        {
            ok( !section_names[i].error, "line %u: section name %s found\n",
                i, section_names[i].section );
            ok( !err, "line %u: bad error code %lx\n", i, err );
        }
        else
        {
            ok( section_names[i].error, "line %u: section name %s not found\n",
                i, section_names[i].section );
            ok( err == section_names[i].error, "line %u: bad error %lx/%lx\n",
                i, err, section_names[i].error );
        }
        SetupCloseInfFile( hinf );
    }
}


/* Test various key and value names */

static const struct
{
    const char *data;
    const char *key;
    const char *fields[10];
} key_names[] =
{
/* file contents          expected key       expected fields */
 { "ab=cd",                "ab",            { "cd" } },
 { "ab=cd,ef,gh,ij",       "ab",            { "cd", "ef", "gh", "ij" } },
 { "ab",                   "ab",            { "ab" } },
 { "ab,cd",                NULL,            { "ab", "cd" } },
 { "ab,cd=ef",             NULL,            { "ab", "cd=ef" } },
 { "=abcd,ef",             "",              { "abcd", "ef" } },
 /* backslashes */
 { "ba\\\ncd=ef",          "bacd",          { "ef" } },
 { "ab  \\  \ncd=ef",      "abcd",          { "ef" } },
 { "ab\\\ncd,ef",          NULL,            { "abcd", "ef" } },
 { "ab  \\ ;cc\ncd=ef",    "abcd",          { "ef" } },
 { "ab \\ \\ \ncd=ef",     "abcd",          { "ef" } },
 { "ba \\ dc=xx",          "ba \\ dc",      { "xx" } },
 { "ba \\\\ \nc=d",        "bac",           { "d" } },
 { "a=b\\\\c",             "a",             { "b\\\\c" } },
 { "ab=cd \\ ",            "ab",            { "cd" } },
 { "ba=c \\ \n \\ \n a",   "ba",            { "ca" } },
 { "ba=c \\ \n \\ a",      "ba",            { "c\\ a" } },
 { "  \\ a= \\ b",         "\\ a",          { "\\ b" } },
 /* quotes */
 { "Ab\"Cd\"=Ef",          "AbCd",          { "Ef" } },
 { "Ab\"Cd=Ef\"",          "AbCd=Ef",       { "AbCd=Ef" } },
 { "ab\"\"\"cd,ef=gh\"",   "ab\"cd,ef=gh",  { "ab\"cd,ef=gh" } },
 { "ab\"\"cd=ef",          "abcd",          { "ef" } },
 { "ab\"\"cd=ef,gh",       "abcd",          { "ef", "gh" } },
 { "ab=cd\"\"ef",          "ab",            { "cdef" } },
 { "ab=cd\",\"ef",         "ab",            { "cd,ef" } },
 { "ab=cd\",ef",           "ab",            { "cd,ef" } },
 { "ab=cd\",ef\\\nab",     "ab",            { "cd,ef\\" } },
 /* spaces */
 { " a b = c , d \n",      "a b",           { "c", "d" } },
 { " a b = c ,\" d\" \n",  "a b",           { "c", " d" } },
 { " a b\r = c\r\n",       "a b",           { "c" } },
 /* empty fields */
 { "a=b,,,c,,,d",          "a",             { "b", "", "", "c", "", "", "d" } },
 { "a=b,\"\",c,\" \",d",   "a",             { "b", "", "c", " ", "d" } },
 { "=,,b",                 "",              { "", "", "b" } },
 { ",=,,b",                NULL,            { "", "=", "", "b" } },
 { "a=\n",                 "a",             { "" } },
 { "=",                    "",              { "" } },
 /* eof */
 { "ab=c\032d",            "ab",            { "c" } },
 { "ab\032=cd",            "ab",            { "ab" } },
 /* nulls */
 { "abcd=ef\x0gh",         "abcd",          { "ef" } },
 /* multiple sections with same name */
 { "[Test2]\nab\n[Test]\nee=ff\n",  "ee",    { "ff" } },
 /* string substitution */
 { "%foo%=%bar%\n" STR_SECTION,     "aaa",   { "bbb" } },
 { "%foo%xx=%bar%yy\n" STR_SECTION, "aaaxx", { "bbbyy" } },
 { "%% %foo%=%bar%\n" STR_SECTION,  "% aaa", { "bbb" } },
 { "%f\"o\"o%=ccc\n" STR_SECTION,   "aaa",   { "ccc" } },
 { "abc=%bar;bla%\n" STR_SECTION,   "abc",   { "%bar" } },
 { "loop=%loop%\n" STR_SECTION,     "loop",  { "%loop2%" } },
 { "%per%%cent%=100\n" STR_SECTION, "12",    { "100" } },
 { "a=%big%\n" STR_SECTION,         "a",     { A400 } },
 { "a=%verybig%\n" STR_SECTION,     "a",     { A511 } },  /* truncated to 511 */
 { "a=%big%%big%%big%%big%\n" STR_SECTION,   "a", { A400 A400 A400 A400 } },
 { "a=%big%%big%%big%%big%%big%%big%%big%%big%%big%\n" STR_SECTION,   "a", { A400 A400 A400 A400 A400 A400 A400 A400 A400 } },
 { "a=%big%%big%%big%%big%%big%%big%%big%%big%%big%%big%%big%\n" STR_SECTION,   "a", { A4097 /*MAX_INF_STRING_LENGTH+1*/ } },
};

/* check the key of a certain line */
static const char *check_key( INFCONTEXT *context, const char *wanted )
{
    const char *key = get_string_field( context, 0 );
    DWORD err = GetLastError();

    if (!key)
    {
        ok( !wanted, "missing key %s\n", wanted );
        ok( err == 0 || err == ERROR_INVALID_PARAMETER, "last error set to %lx\n", err );
    }
    else
    {
        ok( !strcmp( key, wanted ), "bad key %s/%s\n", key, wanted );
        ok( err == 0, "last error set to %lx\n", err );
    }
    return key;
}

static void test_key_names(void)
{
    char buffer[MAX_INF_STRING_LENGTH+32];
    const char *key, *line;
    unsigned int i, index, count;
    UINT err_line;
    HINF hinf;
    DWORD err;
    BOOL ret;
    INFCONTEXT context;

    for (i = 0; i < sizeof(key_names)/sizeof(key_names[0]); i++)
    {
        strcpy( buffer, STD_HEADER "[Test]\n" );
        strcat( buffer, key_names[i].data );
        SetLastError( 0xdeadbeef );
        hinf = test_file_contents( buffer, &err_line );
        ok( hinf != INVALID_HANDLE_VALUE, "line %u: open failed err %lx\n", i, GetLastError() );
        if (hinf == INVALID_HANDLE_VALUE) continue;

        ret = SetupFindFirstLineA( hinf, "Test", 0, &context );
        assert( ret );

        key = check_key( &context, key_names[i].key );

        buffer[0] = buffer[1] = 0;  /* build the full line */
        for (index = 0; ; index++)
        {
            const char *field = get_string_field( &context, index + 1 );
            err = GetLastError();
            if (field)
            {
                ok( err == 0, "line %u: bad error %lx\n", i, GetLastError() );
                if (key_names[i].fields[index])
                    ok( !strcmp( field, key_names[i].fields[index] ), "line %u: bad field %s/%s\n",
                        i, field, key_names[i].fields[index] );
                else
                    ok( 0, "line %u: got extra field %s\n", i, field );
                strcat( buffer, "," );
                strcat( buffer, field );
            }
            else
            {
                ok( err == 0 || err == ERROR_INVALID_PARAMETER,
                    "line %u: bad error %lx\n", i, GetLastError() );
                if (key_names[i].fields[index])
                    ok( 0, "line %u: missing field %s\n", i, key_names[i].fields[index] );
            }
            if (!key_names[i].fields[index]) break;
        }
        count = SetupGetFieldCount( &context );
        ok( count == index, "line %u: bad count %d/%d\n", i, index, count );

        line = get_line_text( &context );
        ok( line != NULL, "line %u: SetupGetLineText failed\n", i );
        if (line) ok( !strcmp( line, buffer+1 ), "line %u: bad text %s/%s\n", i, line, buffer+1 );

        SetupCloseInfFile( hinf );
    }

}

static void test_SetupCloseInfFile(void)
{
    /* try to close with invalid handles */
    SetupCloseInfFile( NULL );
    SetupCloseInfFile( INVALID_HANDLE_VALUE );
}

START_TEST(parser)
{
    test_invalid_files();
    test_section_names();
    test_key_names();
    test_SetupCloseInfFile();
    DeleteFileA( tmpfile );
}
