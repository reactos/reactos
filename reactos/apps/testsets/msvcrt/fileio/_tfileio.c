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
#else /*UNICODE*/
#define _tfopen      fopen
#define _tunlink     _unlink
#define _TEOF        EOF
#define _gettchar    getchar
#define _puttchar    putchar
#define _THEX_FORMAT "0x%02x "
#endif /*UNICODE*/


#define TEST_BUFFER_SIZE 200
#define TEST_FILE_LINES  4

extern BOOL verbose_flagged;
extern BOOL status_flagged;
extern TCHAR test_buffer[TEST_BUFFER_SIZE];


static TCHAR dos_data[] = _T("line1: this is a bunch of readable text.\r\n")\
                   _T("line2: some more printable text and punctuation !@#$%^&*()\r\n")\
                   _T("line3: followed up with some numerals 1234567890\r\n")\
                   _T("line4: done.\r\n");

static TCHAR nix_data[] = _T("line1: this is a bunch of readable text.\n")\
                   _T("line2: some more printable text and punctuation !@#$%^&*()\n")\
                   _T("line3: followed up with some numerals 1234567890\n")\
                   _T("line4: done.\n");

#ifdef UNICODE
#define TEST_B1_FILE_SIZE ((((sizeof(dos_data)/2)-1)+TEST_FILE_LINES)/2) // (166+4)/2=85
#define TEST_B2_FILE_SIZE (((sizeof(dos_data)/2)-1)*2)                   // (166*2)  =332
#define TEST_B3_FILE_SIZE ((((sizeof(nix_data)/2)-1)+TEST_FILE_LINES)/2) // (162+4)/2=83
#define TEST_B4_FILE_SIZE (((sizeof(nix_data)/2)-1)*2)                   // (162*2)  =324
#else /*UNICODE*/
#define TEST_B1_FILE_SIZE (sizeof(dos_data)-1+TEST_FILE_LINES) // (166+4)=170
#define TEST_B2_FILE_SIZE (sizeof(dos_data)-1-TEST_FILE_LINES) // (166-4)=162
#define TEST_B3_FILE_SIZE (sizeof(nix_data)-1+TEST_FILE_LINES) // (162+4)=166
#define TEST_B4_FILE_SIZE (sizeof(nix_data)-1)                 // (162)  =162
#endif /*UNICODE*/





static BOOL create_output_file(TCHAR* file_name, TCHAR* file_mode, TCHAR* file_data)
{
    BOOL result = FALSE;
    FILE *file = _tfopen(file_name, file_mode);
    if (file != NULL) {
#ifndef _NO_NEW_DEPENDS_
        if (_fputts(file_data, file) != _TEOF) {
            result = TRUE;
        } else {
            _tprintf(_T("ERROR: failed to write data to file \"%s\"\n"), file_name);
            _tprintf(_T("ERROR: ferror returned %d\n"), ferror(file));
        }
#endif
        fclose(file);
    } else {
        _tprintf(_T("ERROR: failed to open/create file \"%s\" for output\n"), file_name);
        _tprintf(_T("ERROR: ferror returned %d\n"), ferror(file));
    }
    return result;
}

static BOOL verify_output_file(TCHAR* file_name, TCHAR* file_mode, TCHAR* file_data)
{
    int error_code;
    int offset = 0;
    int line_num = 0;
    BOOL result = FALSE;
    BOOL error_flagged = FALSE;
    FILE* file = _tfopen(file_name, file_mode);
    if (file == NULL) {
        _tprintf(_T("ERROR: (%s) Can't open file for reading\n"), file_name);
        _tprintf(_T("ERROR: ferror returned %d\n"), ferror(file));
        return FALSE;
    } else if (verbose_flagged) {
        _tprintf(_T("STATUS: (%s) opened file for reading\n"), file_name);
    }
#ifndef _NO_NEW_DEPENDS_
    while (_fgetts(test_buffer, TEST_BUFFER_SIZE, file)) {
        int length = _tcslen(test_buffer);
        ++line_num;
        if (verbose_flagged) {
            _tprintf(_T("STATUS: Verifying %d bytes read from line %d\n"), length, line_num);
        }
        if (_tcsncmp(test_buffer, file_data+offset, length) == 0) {
            result = TRUE;
        } else {
            if (verbose_flagged) {
                int i;
                _tprintf(_T("WARNING: (%s) failed to verify file\n"), file_name);
                //_tprintf(_T("expected: \"%s\"\n"), file_data+offset);
                //_tprintf(_T("   found: \"%s\"\n"), test_buffer);
                _tprintf(_T("expected: "));
                for (i = 0; i < length, i < 10; i++) {
                    _tprintf(_T("0x%02x "), file_data+offset+i);
                }
                _tprintf(_T("\n   found: "));
                for (i = 0; i < length, i < 10; i++) {
                    _tprintf(_T("0x%02x "), test_buffer+i);
                }
                _tprintf(_T("\n"));
            } else {
                error_flagged = TRUE;
            }
        }
        offset += length;
    }
    error_code = ferror(file);
    if (error_code) {
         _tprintf(_T("ERROR: (%s) ferror returned %d after reading\n"), file_name, error_code);
         perror("Read error");
    }
    if (!line_num) {
        _tprintf(_T("ERROR: (%s) failed to read from file\n"), file_name);
    }
    if (error_flagged == TRUE) {
        _tprintf(_T("ERROR: (%s) failed to verify file\n"), file_name);
        result = FALSE;
    }
#endif
    fclose(file);
    return result;
}

static int create_test_file(TCHAR* file_name, TCHAR* write_mode, TCHAR* read_mode, TCHAR* file_data)
{
    if (status_flagged) {
        _tprintf(_T("STATUS: Attempting to create output file %s\n"), file_name);
    }
    if (create_output_file(file_name, write_mode, file_data)) {
        if (status_flagged) {
            _tprintf(_T("STATUS: Attempting to verify output file %s\n"), file_name);
        }
        if (verify_output_file(file_name, read_mode, file_data)) {
            if (status_flagged) {
                _tprintf(_T("SUCCESS: %s verified ok\n"), file_name);
            }
        } else {
            //_tprintf(_T("ERROR: failed to verify file %s\n"), file_name);
            return 2;
        }
    } else {
        _tprintf(_T("ERROR: failed to create file %s\n"), file_name);
        return 1;
    }
    return 0;
}

static int check_file_size(TCHAR* file_name, TCHAR* file_mode, int expected)
{
    int count = 0;
    FILE* file;
    TCHAR ch;
    int error_code;

    if (status_flagged) {
        //_tprintf(_T("STATUS: (%s) checking for %d bytes in %s mode\n"), file_name, expected, _tcschr(file_mode, _T('b')) ? _T("binary") : _T("text"));
        _tprintf(_T("STATUS: (%s) checking for %d bytes with mode %s\n"), file_name, expected, file_mode);
    }

    file = _tfopen(file_name, file_mode);
    if (file == NULL) {
        _tprintf(_T("ERROR: (%s) failed to open file for reading\n"), file_name);
        return 1;
    }
    while ((ch = _fgettc(file)) != _TEOF) {
        if (verbose_flagged) {
            _tprintf(_THEX_FORMAT, ch);
        }
        ++count;
    }
    error_code = ferror(file);
    if (error_code) {
         _tprintf(_T("ERROR: (%s) ferror returned %d after reading\n"), file_name, error_code);
         perror("Read error");
    }

    if (verbose_flagged) {
//        _puttc(_T('\n'), stdout);
    }
    fclose(file);
    if (count == expected) {
        if (status_flagged) {
            _tprintf(_T("PASSED: (%s) read %d bytes\n"), file_name, count);
        }
    } else {
        _tprintf(_T("FAILED: (%s) read %d bytes but expected %d using mode \"%s\"\n"), file_name, count, expected, file_mode);
    }
    return (count == expected) ? 0 : -1;
}

static int test_console_io(void)
{
#ifndef _NO_NEW_DEPENDS_
    TCHAR buffer[81];
    TCHAR ch;
    int i, j;

    //printf("Enter a line for echoing:\n");
    _tprintf(_T("Enter a line for echoing:\n"));

    //for (i = 0; (i < 80) && ((ch = _gettchar()) != _TEOF) && (ch != _T('\n')); i++) {
    for (i = 0; (i < 80) && ((ch = _gettc(stdin)) != _TEOF) && (ch != _T('\n')); i++) {
        buffer[i] = (TCHAR)ch;
    }
    buffer[i] = _T('\0');
    for (j = 0; j < i; j++) {
        _puttc(buffer[j], stdout);
    }
    _puttc(_T('\n'), stdout);
    _tprintf(_T("%s\n"), buffer);
#endif
    return 0;
}

static int test_console_getchar(void)
{
    int result = 0;
#ifndef _NO_NEW_DEPENDS_
    TCHAR ch;

    //printf("Enter lines for dumping or <ctrl-z><nl> to finish:\n");
    _tprintf(_T("Enter lines for dumping or <ctrl-z><nl> to finish:\n"));

    //while ((ch = _gettchar()) != _TEOF) {
    while ((ch = _gettc(stdin)) != _TEOF) {
        _tprintf(_THEX_FORMAT, ch);
        //printf("0x%04x ", ch);
    }
#endif
    return result;
}

static int test_console_putch(void)
{
    int result = 0;
    //TCHAR ch;

    _putch('1');
    _putch('@');
    _putch('3');
    _putch(':');
    _putch('\n');
    _putch('a');
    _putch('B');
    _putch('c');
    _putch(':');
    _putch('\n');


    return result;
}

static int test_unlink_files(void)
{
    int result = 0;

    //printf("sizeof dos_data: %d\n", sizeof(dos_data));
    //printf("sizeof nix_data: %d\n", sizeof(nix_data));

    result |= _tunlink(_T("binary.dos"));
    result |= _tunlink(_T("binary.nix"));
    result |= _tunlink(_T("text.dos"));
    result |= _tunlink(_T("text.nix"));
    return result;
}

static int test_text_fileio(TCHAR* file_name, TCHAR* file_data, int tsize, int bsize)
{
    int result = 0;

    result = create_test_file(file_name, _T("w"), _T("r"), file_data);
    result = check_file_size(file_name, _T("r"), tsize);
    result = check_file_size(file_name, _T("rb"), bsize);
    return result;
}

static int test_binary_fileio(TCHAR* file_name, TCHAR* file_data, int tsize, int bsize)
{
    int result = 0;

    result = create_test_file(file_name, _T("wb"), _T("rb"), file_data);
    result = check_file_size(file_name, _T("r"), tsize);
    result = check_file_size(file_name, _T("rb"), bsize);
    return result;
}

static int test_files(int test_num, char* type)
{
    int result = 0;

    printf("performing test: %d (%s)\n", test_num, type);

    switch (test_num) {
    case 1:
        result = test_text_fileio(_T("text.dos"), dos_data, 166, TEST_B1_FILE_SIZE);
        break;
    case 2:
        result = test_binary_fileio(_T("binary.dos"), dos_data, TEST_B2_FILE_SIZE, 166);
        break;
    case 3:
        result = test_text_fileio(_T("text.nix"), nix_data, 162, TEST_B3_FILE_SIZE);
        break;
    case 4:
        result = test_binary_fileio(_T("binary.nix"), nix_data, TEST_B4_FILE_SIZE, 162);
        break;
    case 5:
        result = test_console_io();
        break;
    case 6:
        result = test_console_getchar();
        break;
    case 7:
        result = test_console_putch();
        break;
    case -1:
        result = test_unlink_files();
        break;
    default:
        _tprintf(_T("no test number selected\n"));
        break;
    }
    return result;
}

#if 1

#else

static int test_files(int test_num, char* type)
{
    int result = 0;

    printf("performing test: %d (%s)\n", test_num, type);

    switch (test_num) {
    case 1:
        result = create_test_file(_T("text.dos"), _T("w"), _T("r"), dos_data);
        break;
    case 2:
        result = create_test_file(_T("binary.dos"), _T("wb"), _T("rb"), dos_data);
        break;
    case 3:
        result = create_test_file(_T("text.nix"), _T("w"), _T("r"), nix_data);
        break;
    case 4:
        result = create_test_file(_T("binary.nix"), _T("wb"), _T("rb"), nix_data);
        break;

    case 5:
        result = check_file_size(_T("text.dos"), _T("r"), 166);
        result = check_file_size(_T("text.dos"), _T("rb"), TEST_B1_FILE_SIZE);
        break;
    case 6:
        result = check_file_size(_T("binary.dos"), _T("r"), TEST_B2_FILE_SIZE);
        result = check_file_size(_T("binary.dos"), _T("rb"), 166);
        break;
    case 7:
        result = check_file_size(_T("text.nix"), _T("r"), 162);
        result = check_file_size(_T("text.nix"), _T("rb"), TEST_B3_FILE_SIZE);
        break;
    case 8:
        result = check_file_size(_T("binary.nix"), _T("r"), TEST_B4_FILE_SIZE);
        result = check_file_size(_T("binary.nix"), _T("rb"), 162);
        break;

    case 9:
        result = test_console_io();
        break;
    case 0:
        result = test_console_getchar();
        break;
    case -1:
        result = test_unlink_files();
        break;
    default:
        _tprintf(_T("no test number selected\n"));
        break;
    }
    return result;
}

#endif
