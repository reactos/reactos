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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

/* function pointers */
static HMODULE hSetupAPI;
static LPCSTR (WINAPI *pSetupGetFieldA)(PINFCONTEXT,DWORD);
static LPCWSTR (WINAPI *pSetupGetFieldW)(PINFCONTEXT,DWORD);
static BOOL (WINAPI *pSetupEnumInfSectionsA)( HINF hinf, UINT index, PSTR buffer, DWORD size, UINT *need );

static void init_function_pointers(void)
{
    hSetupAPI = GetModuleHandleA("setupapi.dll");

    /* Nice, pSetupGetField is either A or W depending on the Windows version! The actual test
     * takes care of this difference */
    pSetupGetFieldA = (void *)GetProcAddress(hSetupAPI, "pSetupGetField");
    pSetupGetFieldW = (void *)GetProcAddress(hSetupAPI, "pSetupGetField");
    pSetupEnumInfSectionsA = (void *)GetProcAddress(hSetupAPI, "SetupEnumInfSectionsA" );
}

static const char tmpfilename[] = ".\\tmp.inf";

/* some large strings */
#define A255 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define A256 "a" A255
#define A400 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
             "aaaaaaaaaaaaaaaa" A256
#define A1200 A400 A400 A400
#define A511 A255 A256
#define A4096 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256 A256
#define A4097 "a" A4096

#define STD_HEADER "[Version]\r\nSignature=\"$CHICAGO$\"\r\n"

#define STR_SECTION "[Strings]\nfoo=aaa\nbar=bbb\nloop=%loop2%\nloop2=%loop%\n" \
                    "per%%cent=abcd\nper=1\ncent=2\n22=foo\n" \
                    "big=" A400 "\n" \
                    "mydrive=\"C:\\\"\n" \
                    "verybig=" A1200 "\n"

/* create a new file with specified contents and open it */
static HINF test_file_contents( const char *data, int size, UINT *err_line )
{
    DWORD res;
    HANDLE handle = CreateFileA( tmpfilename, GENERIC_READ|GENERIC_WRITE,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return 0;
    if (!WriteFile( handle, data, size, &res, NULL )) trace( "write error\n" );
    CloseHandle( handle );
    return SetupOpenInfFileA( tmpfilename, 0, INF_STYLE_WIN4, err_line );
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
    int data_size;
    DWORD error;
    UINT err_line;
    BOOL todo;
} invalid_files[] =
{
#define C(s) s, sizeof(s)-1
    /* file contents                                            expected error (or 0)     errline  todo */
    { C("\r\n"),                                                ERROR_WRONG_INF_STYLE,       0,    FALSE },
    { C("abcd\r\n"),                                            ERROR_WRONG_INF_STYLE,       0,    TRUE },
    { C("[Version]\r\n"),                                       ERROR_WRONG_INF_STYLE,       0,    FALSE },
    { C("[Version]\nSignature="),                               ERROR_WRONG_INF_STYLE,       0,    FALSE },
    { C("[Version]\nSignature=foo"),                            ERROR_WRONG_INF_STYLE,       0,    FALSE },
    { C("[version]\nsignature=$chicago$"),                      0,                           0,    FALSE },
    { C("[VERSION]\nSIGNATURE=$CHICAGO$"),                      0,                           0,    FALSE },
    { C("[Version]\nSignature=$chicago$,abcd"),                 0,                           0,    FALSE },
    { C("[Version]\nabc=def\nSignature=$chicago$"),             0,                           0,    FALSE },
    { C("[Version]\nabc=def\n[Version]\nSignature=$chicago$"),  0,                           0,    FALSE },
    { C(STD_HEADER),                                            0,                           0,    FALSE },
    { C(STD_HEADER "[]\r\n"),                                   0,                           0,    FALSE },
    { C(STD_HEADER "]\r\n"),                                    0,                           0,    FALSE },
    { C(STD_HEADER "[" A255 "]\r\n"),                           0,                           0,    FALSE },
    { C(STD_HEADER "[ab\r\n"),                                  ERROR_BAD_SECTION_NAME_LINE, 3,    FALSE },
    { C(STD_HEADER "\n\n[ab\x1a]\n"),                           ERROR_BAD_SECTION_NAME_LINE, 5,    FALSE },
    { C(STD_HEADER "[" A256 "]\r\n"),                           ERROR_SECTION_NAME_TOO_LONG, 3,    FALSE },
    { C("[abc]\n" STD_HEADER),                                  0,                           0,    FALSE },
    { C("abc\r\n" STD_HEADER),                                  ERROR_EXPECTED_SECTION_NAME, 1,    FALSE },
    { C(";\n;\nabc\r\n" STD_HEADER),                            ERROR_EXPECTED_SECTION_NAME, 3,    FALSE },
    { C(";\n;\nab\nab\n" STD_HEADER),                           ERROR_EXPECTED_SECTION_NAME, 3,    FALSE },
    { C(";aa\n;bb\n" STD_HEADER),                               0,                           0,    FALSE },
    { C(STD_HEADER " [TestSection\x00]\n"),                     0,                           0,    TRUE },
    { C(STD_HEADER " [Test\x00Section]\n"),                     0,                           0,    TRUE },
    { C(STD_HEADER " [TestSection\x00]\n"),                     0,                           0,    TRUE },
    { C(STD_HEADER " [Test\x00Section]\n"),                     0,                           0,    TRUE },
    { C("garbage1\ngarbage2\n[abc]\n" STD_HEADER),              ERROR_EXPECTED_SECTION_NAME, 1,    FALSE },
    { C("garbage1\ngarbage2\n[Strings]\n" STD_HEADER),          0,                           0,    FALSE },
    { C(";comment\ngarbage1\ngarbage2\n[abc]\n" STD_HEADER),    ERROR_EXPECTED_SECTION_NAME, 2,    FALSE },
    { C(";comment\ngarbage1\ngarbage2\n[Strings]\n" STD_HEADER), 0,                          0,    FALSE },
    { C(" \t\ngarbage1\ngarbage2\n[abc]\n" STD_HEADER),         ERROR_EXPECTED_SECTION_NAME, 2,    FALSE },
    { C(" \t\ngarbage1\ngarbage2\n[Strings]\n" STD_HEADER),     0,                           0,    FALSE },
    { C("garbage1\ngarbage2\n" STD_HEADER "[abc]\n"),           ERROR_EXPECTED_SECTION_NAME, 1,    FALSE },
    { C("garbage1\ngarbage2\n" STD_HEADER "[Strings]\n"),       0,                           0,    FALSE },
    { C(";comment\ngarbage1\ngarbage2\n" STD_HEADER "[abc]\n"), ERROR_EXPECTED_SECTION_NAME, 2,    FALSE },
    { C(";comment\ngarbage1\ngarbage2\n" STD_HEADER "[Strings]\n"), 0,                       0,    FALSE },
    { C(" \t\ngarbage1\ngarbage2\n" STD_HEADER "[abc]\n"),      ERROR_EXPECTED_SECTION_NAME, 2,    FALSE },
    { C(" \t\ngarbage1\ngarbage2\n" STD_HEADER "[Strings]\n"),  0,                           0,    FALSE },
#undef C
};

static void test_invalid_files(void)
{
    unsigned int i;
    UINT err_line;
    HINF hinf;
    DWORD err;

    for (i = 0; i < ARRAY_SIZE(invalid_files); i++)
    {
        SetLastError( 0xdeadbeef );
        err_line = 0xdeadbeef;
        hinf = test_file_contents( invalid_files[i].data, invalid_files[i].data_size, &err_line );
        err = GetLastError();
        trace( "hinf=%p err=0x%x line=%d\n", hinf, err, err_line );
        if (invalid_files[i].error)  /* should fail */
        {
            ok( hinf == INVALID_HANDLE_VALUE, "file %u: Open succeeded\n", i );
            todo_wine_if (invalid_files[i].todo)
            {
                ok( err == invalid_files[i].error, "file %u: Bad error %u/%u\n",
                    i, err, invalid_files[i].error );
                ok( err_line == invalid_files[i].err_line, "file %u: Bad error line %d/%d\n",
                    i, err_line, invalid_files[i].err_line );
            }
        }
        else  /* should succeed */
        {
            todo_wine_if (invalid_files[i].todo)
            {
                ok( hinf != INVALID_HANDLE_VALUE, "file %u: Open failed\n", i );
                ok( err == 0, "file %u: Error code set to %u\n", i, err );
            }
        }
        SetupCloseInfFile( hinf );
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

    for (i = 0; i < ARRAY_SIZE(section_names); i++)
    {
        SetLastError( 0xdeadbeef );
        hinf = test_file_contents( section_names[i].data, strlen(section_names[i].data), &err_line );
        ok( hinf != INVALID_HANDLE_VALUE, "line %u: open failed err %u\n", i, GetLastError() );
        if (hinf == INVALID_HANDLE_VALUE) continue;

        ret = SetupGetLineCountA( hinf, section_names[i].section );
        err = GetLastError();
        trace( "hinf=%p ret=%d err=0x%x\n", hinf, ret, err );
        if (ret != -1)
        {
            ok( !section_names[i].error, "line %u: section name %s found\n",
                i, section_names[i].section );
            ok( !err, "line %u: bad error code %u\n", i, err );
        }
        else
        {
            ok( section_names[i].error, "line %u: section name %s not found\n",
                i, section_names[i].section );
            ok( err == section_names[i].error, "line %u: bad error %u/%u\n",
                i, err, section_names[i].error );
        }
        SetupCloseInfFile( hinf );
    }
}

static void test_enum_sections(void)
{
    static const char *contents = STD_HEADER "[s1]\nfoo=bar\n[s2]\nbar=foo\n[s3]\n[strings]\na=b\n";

    BOOL ret;
    HINF hinf;
    UINT err, index, len;
    char buffer[256];

    if (!pSetupEnumInfSectionsA)
    {
        win_skip( "SetupEnumInfSectionsA not available\n" );
        return;
    }

    hinf = test_file_contents( contents, strlen(contents), &err );
    ok( hinf != NULL, "Expected valid INF file\n" );

    for (index = 0; ; index++)
    {
        SetLastError( 0xdeadbeef );
        ret = pSetupEnumInfSectionsA( hinf, index, NULL, 0, &len );
        err = GetLastError();
        if (!ret && GetLastError() == ERROR_NO_MORE_ITEMS) break;
        ok( ret, "SetupEnumInfSectionsA failed\n" );
        ok( len == 3 || len == 8, "wrong len %u\n", len );

        SetLastError( 0xdeadbeef );
        ret = pSetupEnumInfSectionsA( hinf, index, NULL, sizeof(buffer), &len );
        err = GetLastError();
        ok( !ret, "SetupEnumInfSectionsA succeeded\n" );
        ok( err == ERROR_INVALID_USER_BUFFER, "wrong error %u\n", err );
        ok( len == 3 || len == 8, "wrong len %u\n", len );

        SetLastError( 0xdeadbeef );
        ret = pSetupEnumInfSectionsA( hinf, index, buffer, sizeof(buffer), &len );
        ok( ret, "SetupEnumInfSectionsA failed err %u\n", GetLastError() );
        ok( len == 3 || len == 8, "wrong len %u\n", len );
        ok( !lstrcmpiA( buffer, "version" ) || !lstrcmpiA( buffer, "s1" ) ||
            !lstrcmpiA( buffer, "s2" ) || !lstrcmpiA( buffer, "s3" ) || !lstrcmpiA( buffer, "strings" ),
            "bad section '%s'\n", buffer );
    }
    SetupCloseInfFile( hinf );
}


/* Test various key and value names */

static const struct
{
    const char *data;
    int data_size;
    const char *key;
    const char *fields[10];
} key_names[] =
{
#define C(s) s, sizeof(s)-1
/* file contents            expected key       expected fields */
 { C("ab=cd"),                "ab",            { "cd" } },
 { C("ab=cd,ef,gh,ij"),       "ab",            { "cd", "ef", "gh", "ij" } },
 { C("ab"),                   "ab",            { "ab" } },
 { C("ab,cd"),                NULL,            { "ab", "cd" } },
 { C("ab,cd=ef"),             NULL,            { "ab", "cd=ef" } },
 { C("=abcd,ef"),             "",              { "abcd", "ef" } },
 /* backslashes */
 { C("ba\\\ncd=ef"),          "bacd",          { "ef" } },
 { C("ab  \\  \ncd=ef"),      "abcd",          { "ef" } },
 { C("ab\\\ncd,ef"),          NULL,            { "abcd", "ef" } },
 { C("ab  \\ ;cc\ncd=ef"),    "abcd",          { "ef" } },
 { C("ab \\ \\ \ncd=ef"),     "abcd",          { "ef" } },
 { C("ba \\ dc=xx"),          "ba \\ dc",      { "xx" } },
 { C("ba \\\\ \nc=d"),        "bac",           { "d" } },
 { C("a=b\\\\c"),             "a",             { "b\\\\c" } },
 { C("ab=cd \\ "),            "ab",            { "cd" } },
 { C("ba=c \\ \n \\ \n a"),   "ba",            { "ca" } },
 { C("ba=c \\ \n \\ a"),      "ba",            { "c\\ a" } },
 { C("  \\ a= \\ b"),         "\\ a",          { "\\ b" } },
 /* quotes */
 { C("Ab\"Cd\"=Ef"),          "AbCd",          { "Ef" } },
 { C("Ab\"Cd=Ef\""),          "AbCd=Ef",       { "AbCd=Ef" } },
 { C("ab\"\"\"cd,ef=gh\""),   "ab\"cd,ef=gh",  { "ab\"cd,ef=gh" } },
 { C("ab\"\"cd=ef"),          "abcd",          { "ef" } },
 { C("ab\"\"cd=ef,gh"),       "abcd",          { "ef", "gh" } },
 { C("ab=cd\"\"ef"),          "ab",            { "cdef" } },
 { C("ab=cd\",\"ef"),         "ab",            { "cd,ef" } },
 { C("ab=cd\",ef"),           "ab",            { "cd,ef" } },
 { C("ab=cd\",ef\\\nab"),     "ab",            { "cd,ef\\" } },

 /* single quotes (unhandled)*/
 { C("HKLM,A,B,'C',D"),       NULL,            { "HKLM", "A","B","'C'","D" } },
 /* spaces */
 { C(" a b = c , d \n"),      "a b",           { "c", "d" } },
 { C(" a b = c ,\" d\" \n"),  "a b",           { "c", " d" } },
 { C(" a b\r = c\r\n"),       "a b",           { "c" } },
 /* empty fields */
 { C("a=b,,,c,,,d"),          "a",             { "b", "", "", "c", "", "", "d" } },
 { C("a=b,\"\",c,\" \",d"),   "a",             { "b", "", "c", " ", "d" } },
 { C("=,,b"),                 "",              { "", "", "b" } },
 { C(",=,,b"),                NULL,            { "", "=", "", "b" } },
 { C("a=\n"),                 "a",             { "" } },
 { C("="),                    "",              { "" } },
 /* eof */
 { C("ab=c\032d"),            "ab",            { "c" } },
 { C("ab\032=cd"),            "ab",            { "ab" } },
 /* nulls */
 { C("abcd=ef\x0gh"),         "abcd",          { "ef gh" } },
 { C("foo=%bar%\n[Strings]\nbar=bbb\0\n"), "foo", { "bbb" } },
 { C("foo=%bar%\n[Strings]\nbar=bbb \0\n"), "foo", { "bbb" } },
 { C("foo=%bar%\n[Strings]\nbar=aaa\0bbb \0\n"), "foo", { "aaa bbb" } },
 /* multiple sections with same name */
 { C("[Test2]\nab\n[Test]\nee=ff\n"),  "ee",    { "ff" } },
 /* string substitution */
 { C("%foo%=%bar%\n" STR_SECTION),     "aaa",   { "bbb" } },
 { C("%foo%xx=%bar%yy\n" STR_SECTION), "aaaxx", { "bbbyy" } },
 { C("%% %foo%=%bar%\n" STR_SECTION),  "% aaa", { "bbb" } },
 { C("%f\"o\"o%=ccc\n" STR_SECTION),   "aaa",   { "ccc" } },
 { C("abc=%bar;bla%\n" STR_SECTION),   "abc",   { "%bar" } },
 { C("loop=%loop%\n" STR_SECTION),     "loop",  { "%loop2%" } },
 { C("%per%%cent%=100\n" STR_SECTION), "12",    { "100" } },
 { C("a=%big%\n" STR_SECTION),         "a",     { A400 } },
 { C("a=%verybig%\n" STR_SECTION),     "a",     { A511 } },  /* truncated to 511, not on Vista/W2K8 */
 { C("a=%big%%big%%big%%big%\n" STR_SECTION),   "a", { A400 A400 A400 A400 } },
 { C("a=%big%%big%%big%%big%%big%%big%%big%%big%%big%\n" STR_SECTION),   "a", { A400 A400 A400 A400 A400 A400 A400 A400 A400 } },
 { C("a=%big%%big%%big%%big%%big%%big%%big%%big%%big%%big%%big%\n" STR_SECTION),   "a", { A4097 /*MAX_INF_STRING_LENGTH+1*/ } },

 /* Prove expansion of system entries removes extra \'s and string
    replacements doesn't                                            */
 { C("ab=\"%24%\"\n" STR_SECTION),           "ab", { "C:\\" } },
 { C("ab=\"%mydrive%\"\n" STR_SECTION),      "ab", { "C:\\" } },
 { C("ab=\"%24%\\fred\"\n" STR_SECTION),     "ab", { "C:\\fred" } },
 { C("ab=\"%mydrive%\\fred\"\n" STR_SECTION),"ab", { "C:\\\\fred" } },
 /* Confirm duplicate \'s kept */
 { C("ab=\"%24%\\\\fred\""),      "ab",            { "C:\\\\fred" } },
 { C("ab=C:\\\\FRED"),            "ab",            { "C:\\\\FRED" } },
};

/* check the key of a certain line */
static const char *check_key( INFCONTEXT *context, const char *wanted )
{
    const char *key = get_string_field( context, 0 );
    DWORD err = GetLastError();

    if (!key)
    {
        ok( !wanted, "missing key %s\n", wanted );
        ok( err == 0 || err == ERROR_INVALID_PARAMETER, "last error set to %u\n", err );
    }
    else
    {
        ok( !strcmp( key, wanted ), "bad key %s/%s\n", key, wanted );
        ok( err == 0, "last error set to %u\n", err );
    }
    return key;
}

static void test_key_names(void)
{
    char buffer[MAX_INF_STRING_LENGTH+32];
    const char *line;
    unsigned int i, index, count;
    UINT err_line;
    HINF hinf;
    DWORD err;
    BOOL ret;
    INFCONTEXT context;

    for (i = 0; i < ARRAY_SIZE(key_names); i++)
    {
        int data_size;

        strcpy( buffer, STD_HEADER "[Test]\n" );
        data_size = strlen(buffer);
        memcpy( buffer + data_size, key_names[i].data, key_names[i].data_size );
        data_size += key_names[i].data_size;
        SetLastError( 0xdeadbeef );
        hinf = test_file_contents( buffer, data_size, &err_line );
        ok( hinf != INVALID_HANDLE_VALUE, "line %u: open failed err %u\n", i, GetLastError() );
        if (hinf == INVALID_HANDLE_VALUE) continue;

        ret = SetupFindFirstLineA( hinf, "Test", key_names[i].key, &context );
        ok(ret, "Test %d: failed to find key %s\n", i, key_names[i].key);

        if (!strncmp( key_names[i].data, "%foo%", strlen( "%foo%" ) ))
        {
            ret = SetupFindFirstLineA( hinf, "Test", "%foo%", &context );
            ok(!ret, "SetupFindFirstLine() should not match unsubstituted keys\n");
            ok(GetLastError() == ERROR_LINE_NOT_FOUND, "got wrong error %u\n", GetLastError());
        }

        ret = SetupFindFirstLineA( hinf, "Test", 0, &context );
        ok(ret, "SetupFindFirstLineA failed: le=%u\n", GetLastError());
        if (!ret)
        {
            SetupCloseInfFile( hinf );
            continue;
        }

        check_key( &context, key_names[i].key );

        buffer[0] = buffer[1] = 0;  /* build the full line */
        for (index = 0; ; index++)
        {
            const char *field = get_string_field( &context, index + 1 );
            err = GetLastError();
            if (field)
            {
                ok( err == 0, "line %u: bad error %u\n", i, err );
                if (key_names[i].fields[index])
                {
                    if (i == 52)
                        ok( !strcmp( field, key_names[i].fields[index] ) ||
                            !strcmp( field, A1200), /* Vista, W2K8 */
                            "line %u: bad field %s/%s\n",
                            i, field, key_names[i].fields[index] );
                    else if (i == 55)
                        ok( !strcmp( field, key_names[i].fields[index] ) ||
                            !strcmp( field, A4096), /* Win10 >= 1709 */
                            "line %u: bad field %s/%s\n",
                            i, field, key_names[i].fields[index] );
                    else  /* don't compare drive letter of paths */
                        if (field[0] && field[1] == ':' && field[2] == '\\')
                        ok( !strcmp( field + 1, key_names[i].fields[index] + 1 ),
                            "line %u: bad field %s/%s\n",
                            i, field, key_names[i].fields[index] );
                    else
                        ok( !strcmp( field, key_names[i].fields[index] ), "line %u: bad field %s/%s\n",
                            i, field, key_names[i].fields[index] );
                }
                else
                    ok( 0, "line %u: got extra field %s\n", i, field );
                strcat( buffer, "," );
                strcat( buffer, field );
            }
            else
            {
                ok( err == 0 || err == ERROR_INVALID_PARAMETER,
                    "line %u: bad error %u\n", i, err );
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

static void test_close_inf_file(void)
{
    SetLastError(0xdeadbeef);
    SetupCloseInfFile(NULL);
    ok(GetLastError() == 0xdeadbeef ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x, WinMe */
        "Expected 0xdeadbeef, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    SetupCloseInfFile(INVALID_HANDLE_VALUE);
    ok(GetLastError() == 0xdeadbeef ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x, WinMe */
        "Expected 0xdeadbeef, got %u\n", GetLastError());
}

static const char *contents = "[Version]\n"
                              "Signature=\"$Windows NT$\"\n"
                              "FileVersion=5.1.1.2\n"
                              "[FileBranchInfo]\n"
                              "RTMQFE=\"%RTMGFE_NAME%\",SP1RTM,"A4097"\n"
                              "[Strings]\n"
                              "RTMQFE_NAME = \"RTMQFE\"\n";

static const CHAR getfield_resA[][20] =
{
    "RTMQFE",
    "%RTMGFE_NAME%",
    "SP1RTM",
};

static const WCHAR getfield_resW[][20] =
{
    {'R','T','M','Q','F','E',0},
    {'%','R','T','M','G','F','E','_','N','A','M','E','%',0},
    {'S','P','1','R','T','M',0},
};

static void test_pSetupGetField(void)
{
    UINT err;
    BOOL ret;
    HINF hinf;
    LPCSTR fieldA;
    LPCWSTR fieldW;
    INFCONTEXT context;
    int i;
    int len;
    BOOL unicode = TRUE;

    SetLastError(0xdeadbeef);
    lstrcmpW(NULL, NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Using A-functions instead of W\n");
        unicode = FALSE;
    }

    hinf = test_file_contents( contents, strlen(contents), &err );
    ok( hinf != NULL, "Expected valid INF file\n" );

    ret = SetupFindFirstLineA( hinf, "FileBranchInfo", NULL, &context );
    ok( ret, "Failed to find first line\n" );

    /* native Windows crashes if a NULL context is sent in */

    for ( i = 0; i < 3; i++ )
    {
        if (unicode)
        {
            fieldW = pSetupGetFieldW( &context, i );
            ok( fieldW != NULL, "Failed to get field %i\n", i );
            ok( !lstrcmpW( getfield_resW[i], fieldW ), "Wrong string returned\n" );
        }
        else
        {
            fieldA = pSetupGetFieldA( &context, i );
            ok( fieldA != NULL, "Failed to get field %i\n", i );
            ok( !lstrcmpA( getfield_resA[i], fieldA ), "Wrong string returned\n" );
        }
    }

    if (unicode)
    {
        fieldW = pSetupGetFieldW( &context, 3 );
        ok( fieldW != NULL, "Failed to get field 3\n" );
        len = lstrlenW( fieldW );
        ok( len == 511 || /* NT4, W2K, XP and W2K3 */
            len == 4096,  /* Vista */
            "Unexpected length, got %d\n", len );

        fieldW = pSetupGetFieldW( &context, 4 );
        ok( fieldW == NULL, "Expected NULL, got %p\n", fieldW );
        ok( GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError() );
    }
    else
    {
        fieldA = pSetupGetFieldA( &context, 3 );
        ok( fieldA != NULL, "Failed to get field 3\n" );
        len = lstrlenA( fieldA );
        ok( len == 511, /* Win9x, WinME */
            "Unexpected length, got %d\n", len );

        fieldA = pSetupGetFieldA( &context, 4 );
        ok( fieldA == NULL, "Expected NULL, got %p\n", fieldA );
        ok( GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError() );
    }

    SetupCloseInfFile( hinf );
}

static void test_SetupGetIntField(void)
{
    static const struct
    {
        const char *key;
        const char *fields;
        DWORD index;
        INT value;
        DWORD err;
    } keys[] =
    {
    /* key     fields            index   expected int  errorcode */
    {  "Key", "48",             1,      48,           ERROR_SUCCESS },
    {  "Key", "48",             0,      -1,           ERROR_INVALID_DATA },
    {  "123", "48",             0,      123,          ERROR_SUCCESS },
    {  "Key", "0x4",            1,      4,            ERROR_SUCCESS },
    {  "Key", "Field1",         1,      -1,           ERROR_INVALID_DATA },
    {  "Key", "Field1,34",      2,      34,           ERROR_SUCCESS },
    {  "Key", "Field1,,Field3", 2,      0,            ERROR_SUCCESS },
    {  "Key", "Field1,",        2,      0,            ERROR_SUCCESS }
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(keys); i++)
    {
        HINF hinf;
        char buffer[MAX_INF_STRING_LENGTH];
        INFCONTEXT context;
        UINT err;
        BOOL retb;
        INT intfield;

        strcpy( buffer, STD_HEADER "[TestSection]\n" );
        strcat( buffer, keys[i].key );
        strcat( buffer, "=" );
        strcat( buffer, keys[i].fields );
        hinf = test_file_contents( buffer, strlen(buffer), &err);
        ok( hinf != NULL, "Expected valid INF file\n" );

        SetupFindFirstLineA( hinf, "TestSection", keys[i].key, &context );
        SetLastError( 0xdeadbeef );
        intfield = -1;
        retb = SetupGetIntField( &context, keys[i].index, &intfield );
        if ( keys[i].err == ERROR_SUCCESS )
        {
            ok( retb, "%u: Expected success\n", i );
            ok( GetLastError() == ERROR_SUCCESS ||
                GetLastError() == 0xdeadbeef /* win9x, NT4 */,
                "%u: Expected ERROR_SUCCESS or 0xdeadbeef, got %u\n", i, GetLastError() );
        }
        else
        {
            ok( !retb, "%u: Expected failure\n", i );
            ok( GetLastError() == keys[i].err,
                "%u: Expected %d, got %u\n", i, keys[i].err, GetLastError() );
        }
        ok( intfield == keys[i].value, "%u: Expected %d, got %d\n", i, keys[i].value, intfield );

        SetupCloseInfFile( hinf );
    }
}

static void test_GLE(void)
{
    static const char *inf =
        "[Version]\n"
        "Signature=\"$Windows NT$\"\n"
        "[Sectionname]\n"
        "Keyname1=Field1,Field2,Field3\n"
        "\n"
        "Keyname2=Field4,Field5\n";
    HINF hinf;
    UINT err;
    INFCONTEXT context;
    BOOL retb;
    LONG retl;
    char buf[MAX_INF_STRING_LENGTH];
    int bufsize = MAX_INF_STRING_LENGTH;
    DWORD retsize;

    hinf = test_file_contents( inf, strlen(inf), &err );
    ok( hinf != NULL, "Expected valid INF file\n" );

    SetLastError(0xdeadbeef);
    retb = SetupFindFirstLineA( hinf, "ImNotThere", NULL, &context );
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupFindFirstLineA( hinf, "ImNotThere", "ImNotThere", &context );
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupFindFirstLineA( hinf, "Sectionname", NULL, &context );
    ok(retb, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupFindFirstLineA( hinf, "Sectionname", "ImNotThere", &context );
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupFindFirstLineA( hinf, "Sectionname", "Keyname1", &context );
    ok(retb, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupFindNextMatchLineA( &context, "ImNotThere", &context );
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupFindNextMatchLineA( &context, "Keyname2", &context );
    ok(retb, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retl = SetupGetLineCountA( hinf, "ImNotThere");
    ok(retl == -1, "Expected -1, got %d\n", retl);
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND,
        "Expected ERROR_SECTION_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retl = SetupGetLineCountA( hinf, "Sectionname");
    ok(retl == 2, "Expected 2, got %d\n", retl);
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupGetLineTextA( NULL, hinf, "ImNotThere", "ImNotThere", buf, bufsize, &retsize);
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupGetLineTextA( NULL, hinf, "Sectionname", "ImNotThere", buf, bufsize, &retsize);
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupGetLineTextA( NULL, hinf, "Sectionname", "Keyname1", buf, bufsize, &retsize);
    ok(retb, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupGetLineByIndexA( hinf, "ImNotThere", 1, &context );
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupGetLineByIndexA( hinf, "Sectionname", 1, &context );
    ok(retb, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS,
        "Expected ERROR_SUCCESS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    retb = SetupGetLineByIndexA( hinf, "Sectionname", 3, &context );
    ok(!retb, "Expected failure\n");
    ok(GetLastError() == ERROR_LINE_NOT_FOUND,
        "Expected ERROR_LINE_NOT_FOUND, got %08x\n", GetLastError());

    SetupCloseInfFile( hinf );
}

START_TEST(parser)
{
    init_function_pointers();
    test_invalid_files();
    test_section_names();
    test_enum_sections();
    test_key_names();
    test_close_inf_file();
    test_pSetupGetField();
    test_SetupGetIntField();
    test_GLE();
    DeleteFileA( tmpfilename );
}
