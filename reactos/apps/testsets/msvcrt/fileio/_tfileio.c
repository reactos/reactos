/*
 *  ReactOS test program - 
 *
 *  _tfileio.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <stdio.h>


#ifdef UNICODE

#define _tfopen      _wfopen
#define _tunlink     _wunlink
#define _TEOF        WEOF
#define _gettchar    getwchar 
#define _puttchar    putwchar  
#define _THEX_FORMAT _T("0x%04x ")
#define TEST_B1_FILE_SIZE    85
#define TEST_B2_FILE_SIZE    332
#define TEST_B3_FILE_SIZE    83
#define TEST_B4_FILE_SIZE    324

#else /*UNICODE*/

#define _tfopen      fopen
#define _tunlink     _unlink
#define _TEOF        EOF
#define _gettchar    getchar
#define _puttchar    putchar
#define _THEX_FORMAT "0x%02x "
#define TEST_B1_FILE_SIZE    170
#define TEST_B2_FILE_SIZE    162
#define TEST_B3_FILE_SIZE    166
#define TEST_B4_FILE_SIZE    162

#endif /*UNICODE*/


#define TEST_BUFFER_SIZE 200

extern BOOL verbose_flagged;
extern TCHAR test_buffer[TEST_BUFFER_SIZE];


static TCHAR dos_data[] = _T("line1: this is a bunch of readable text.\r\n")\
                   _T("line2: some more printable text and punctuation !@#$%^&*()\r\n")\
                   _T("line3: followed up with some numerals 1234567890\r\n")\
                   _T("line4: done.\r\n");

static TCHAR nix_data[] = _T("line1: this is a bunch of readable text.\n")\
                   _T("line2: some more printable text and punctuation !@#$%^&*()\n")\
                   _T("line3: followed up with some numerals 1234567890\n")\
                   _T("line4: done.\n");


static BOOL create_output_file(TCHAR* file_name, TCHAR* file_mode, TCHAR* file_data)
{
    BOOL result = FALSE;
    FILE *file = _tfopen(file_name, file_mode);
    if (file == NULL) {
        _tprintf(_T("ERROR: Can't open file \"%s\" for writing\n"), file_name);
        return FALSE;
    }
    if (_fputts(file_data, file) != _TEOF) {
        result = TRUE;
    } else {
        _tprintf(_T("ERROR: failed to write to file \"%s\"\n"), file_name);
    }
    fclose(file);
    return result;
}

static BOOL verify_output_file(TCHAR* file_name, TCHAR* file_mode, TCHAR* file_data)
{
    int offset = 0;
    int line_num = 0;
    BOOL result = FALSE;
    FILE* file = _tfopen(file_name, file_mode);
    if (file == NULL) {
        _tprintf(_T("ERROR: Can't open file \"%s\" for reading\n"), file_name);
        return FALSE;
    }
    while (_fgetts(test_buffer, TEST_BUFFER_SIZE, file)) {
        int length = _tcslen(test_buffer);
        _tprintf(_T("STATUS: Verifying %d bytes read from line %d\n"), length, ++line_num);
        if (_tcsncmp(test_buffer, file_data+offset, length) == 0) {
            result = TRUE;
        } else {
            _tprintf(_T("WARNING: failed to verify data from file \"%s\"\n"), file_name);
        }
        offset += length;
    }
    if (!line_num) {
        _tprintf(_T("ERROR: failed to read from file \"%s\"\n"), file_name);
    }
    fclose(file);
    return result;
}

static int test_A(TCHAR* file_name, TCHAR* write_mode, TCHAR* read_mode, TCHAR* file_data)
{
    _tprintf(_T("STATUS: Attempting to create output file %s\n"), file_name);
    if (create_output_file(file_name, write_mode, file_data)) {
        _tprintf(_T("STATUS: Attempting to verify output file %s\n"), file_name);
        if (verify_output_file(file_name, read_mode, file_data)) {
            _tprintf(_T("SUCCESS: %s verified ok\n"), file_name);
        } else {
            _tprintf(_T("ERROR: Can't verify output file %s\n"), file_name);
            return 2;
        }
    } else {
        _tprintf(_T("ERROR: Can't create output file %s\n"), file_name);
        return 1;
    }
    return 0;
}

static int test_B(TCHAR* file_name, TCHAR* file_mode, int expected)
{
    int count = 0;
    FILE* file;
    TCHAR ch;

    _tprintf(_T("STATUS: checking %s in %s mode\n"), file_name, _tcschr(file_mode, _T('b')) ? _T("binary") : _T("text"));

    file = _tfopen(file_name, file_mode);
    if (file == NULL) {
        _tprintf(_T("ERROR: Can't open file \"%s\" for reading\n"), file_name);
        return 1;
    }
    while ((ch = _fgettc(file)) != _TEOF) {
        if (verbose_flagged) {
            _tprintf(_THEX_FORMAT, ch);
        }
        ++count;
    }
    if (verbose_flagged) {
        _puttchar(_T('\n'));
    }
    fclose(file);
    if (count == expected) {
        _tprintf(_T("PASSED: read %d bytes from %s as expected\n"), count, file_name);
    } else {
        _tprintf(_T("ERROR: read %d bytes from %s, expected %d\n"), count, file_name, expected);
    }
    return count;
}

static int test_C(void)
{
    TCHAR buffer[81];
    TCHAR ch;
    int i;

    _tprintf(_T("Enter a line: "));
    for (i = 0; (i < 80) && ((ch = _gettchar()) != _TEOF) && (ch != _T('\n')); i++) {
        buffer[i] = (TCHAR)ch;
    }
    buffer[i] = _T('\0');
    _tprintf(_T("%s\n"), buffer);
    return 0;
}

static int test_D(void)
{
    int result = 0;
    TCHAR ch;

    while ((ch = _gettchar()) != _TEOF) {
        _tprintf(_THEX_FORMAT, ch);
    }
    return result;
}

static int clean_files(void)
{
    int result = 0;

    result |= _tunlink(_T("binary.dos"));
    result |= _tunlink(_T("binary.nix"));
    result |= _tunlink(_T("text.dos"));
    result |= _tunlink(_T("text.nix"));
    return result;
}

static int test_files(int test_num, char* type)
{
    int result = 0;

    printf("performing test: %d (%s)\n", test_num, type);

    switch (test_num) {
    case 1:
        result = test_A(_T("text.dos"), _T("w"), _T("r"), dos_data);
        break;
    case 2:
        result = test_A(_T("binary.dos"), _T("wb"), _T("rb"), dos_data);
        break;
    case 3:
        result = test_A(_T("text.nix"), _T("w"), _T("r"), nix_data);
        break;
    case 4:
        result = test_A(_T("binary.nix"), _T("wb"), _T("rb"), nix_data);
        break;

    case 5:
        result = test_B(_T("text.dos"), _T("r"), 166);
        result = test_B(_T("text.dos"), _T("rb"), TEST_B1_FILE_SIZE);
        break;
    case 6:
        result = test_B(_T("binary.dos"), _T("r"), TEST_B2_FILE_SIZE);
        result = test_B(_T("binary.dos"), _T("rb"), 166);
        break;
    case 7:
        result = test_B(_T("text.nix"), _T("r"), 162);
        result = test_B(_T("text.nix"), _T("rb"), TEST_B3_FILE_SIZE);
        break;
    case 8:
        result = test_B(_T("binary.nix"), _T("r"), TEST_B4_FILE_SIZE);
        result = test_B(_T("binary.nix"), _T("rb"), 162);
        break;

    case 9:
        result = test_C();
        break;
    case 0:
        result = test_D();
        break;
    case -1:
        result = clean_files();
        break;
    default:
        _tprintf(_T("no test number selected\n"));
        break;
    }
    return result;
}
