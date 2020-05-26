/*
 * PROJECT:     mspatcha_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for mspatcha
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windows.h>

#include "wine/test.h"
#include <patchapi.h>


/* Headers created with ExtractPatchHeaderToFileA */
unsigned char in1_bin[8] =
{
    0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61,                                                     /* testdata         */
};
unsigned char out1_bin[26] =
{
    0x74, 0x65, 0x73, 0x74, 0x20, 0x44, 0x61, 0x74, 0x61, 0x0d, 0x0a, 0x57, 0x69, 0x74, 0x68, 0x20,     /* test Data..With  */
    0x73, 0x6f, 0x6d, 0x65, 0x20, 0x65, 0x78, 0x74, 0x72, 0x61,                                         /* some extra       */
};
unsigned char patch1_bin[75] =
{
    0x50, 0x41, 0x31, 0x39, 0x01, 0x00, 0xc4, 0x00, 0x4e, 0x28, 0xb3, 0x5b, 0x9a, 0x08, 0xd1, 0x51,     /* PA19....N(.[...Q */
    0xf6, 0x01, 0xd2, 0x5e, 0x36, 0xe5, 0x99, 0x00, 0x00, 0x80, 0xac, 0x2a, 0x00, 0x00, 0x30, 0xa0,     /* ...^6......*..0. */
    0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x74, 0x65, 0x73,     /* .............tes */
    0x74, 0x20, 0x44, 0x61, 0x74, 0x61, 0x0d, 0x0a, 0x57, 0x69, 0x74, 0x68, 0x20, 0x73, 0x6f, 0x6d,     /* t Data..With som */
    0x65, 0x20, 0x65, 0x78, 0x74, 0x72, 0x61, 0xa7, 0x19, 0x4a, 0x68,                                   /* e extra..Jh      */
};
unsigned char patch1_header_bin[31] =
{
    0x50, 0x41, 0x31, 0x39, 0x01, 0x00, 0xc4, 0x00, 0x4e, 0x28, 0xb3, 0x5b, 0x9a, 0x08, 0xd1, 0x51,     /* PA19....N(.[...Q */
    0xf6, 0x01, 0xd2, 0x5e, 0x36, 0xe5, 0x99, 0x00, 0x00, 0x80, 0xac, 0xe9, 0xf0, 0x53, 0xf7,           /* ...^6........S.  */
};

unsigned char in2_bin[25] =
{
    0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xaa,     /* ........"3DUfw.. */
    0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00,                                               /* .........        */
};
unsigned char out2_bin[25] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xbb,     /* ........"3DUfw.. */
    0x00, 0xbb, 0x00, 0xbb, 0x00, 0xbb, 0x00, 0xbb, 0x00,                                               /* .........        */
};
unsigned char patch2_bin[75] =
{
    0x50, 0x41, 0x31, 0x39, 0x01, 0x00, 0xc4, 0x00, 0x62, 0x35, 0xb3, 0x5b, 0x99, 0xfa, 0xa0, 0x8a,     /* PA19....b5.[.... */
    0x02, 0x01, 0x80, 0x1d, 0xc2, 0x54, 0xfc, 0x00, 0x00, 0x80, 0xac, 0x2a, 0x00, 0x00, 0x30, 0x90,     /* .....T.....*..0. */
    0x01, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,     /* ................ */
    0xff, 0xff, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0xbb, 0x00, 0xbb, 0x00,     /* ....."3DUfw..... */
    0xbb, 0x00, 0xbb, 0x00, 0xbb, 0x00, 0x00, 0x14, 0x06, 0xeb, 0x61,                                   /* ..........a      */
};
unsigned char patch2_header_bin[31] =
{
    0x50, 0x41, 0x31, 0x39, 0x01, 0x00, 0xc4, 0x00, 0x62, 0x35, 0xb3, 0x5b, 0x99, 0xfa, 0xa0, 0x8a,     /* PA19....b5.[.... */
    0x02, 0x01, 0x80, 0x1d, 0xc2, 0x54, 0xfc, 0x00, 0x00, 0x80, 0xac, 0xce, 0x5d, 0x8c, 0xed,           /* .....T......]..  */
};

/* Minimum output size to trigger compression */
unsigned char in3_bin[138] =
{
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,                                         /* ..........       */
};
unsigned char out3_bin[152] =
{
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,     /* ................ */
    0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb,                                                     /* ........         */
};
unsigned char patch3_bin[88] =
{
    0x50, 0x41, 0x31, 0x39, 0x01, 0x00, 0xc4, 0x00, 0xb1, 0x57, 0xb3, 0x5b, 0x18, 0x81, 0xf9, 0xd8,     /* PA19.....W.[.... */
    0x1d, 0x41, 0x01, 0xce, 0xd9, 0x52, 0x0a, 0xf1, 0x00, 0x00, 0x80, 0xb8, 0x36, 0x00, 0x00, 0x10,     /* .A...R......6... */
    0x82, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0f, 0x22, 0xff, 0xff, 0x35, 0xd8,     /* ........ .."..5. */
    0xfb, 0x8f, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x03, 0xb7, 0x08, 0xf7, 0x7d,     /* ..........!....} */
    0x7e, 0xdf, 0x40, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x0d, 0x20, 0x7d, 0xbe,     /* ~.@L......@.. }. */
    0xf9, 0xbe, 0x68, 0xf4, 0x1f, 0x45, 0x3e, 0x35,                                                     /* ..h..E>5         */
};
unsigned char patch3_header_bin[32] =
{
    0x50, 0x41, 0x31, 0x39, 0x01, 0x00, 0xc4, 0x00, 0xb1, 0x57, 0xb3, 0x5b, 0x18, 0x81, 0xf9, 0xd8,     /* PA19.....W.[.... */
    0x1d, 0x41, 0x01, 0xce, 0xd9, 0x52, 0x0a, 0xf1, 0x00, 0x00, 0x80, 0xb8, 0xbd, 0xeb, 0x70, 0xdd,     /* .A...R........p. */
};

typedef struct _bin_file
{
    unsigned char* data;
    size_t len;
} bin_file;

typedef struct _patch_data
{
    char* name;
    bin_file input;
    char* input_signature;
    bin_file output;
    char* output_signature;
    bin_file patch;
    bin_file patch_header;
} patch_data;

#define MAKE_BIN(data_)  { data_, sizeof(data_) }

char temp_path[MAX_PATH];

BOOL extract2(char* filename, const unsigned char* data, size_t len)
{
    HANDLE hFile;
    BOOL bRet;
    DWORD dwWritten;

    if (!GetTempFileNameA(temp_path, "mpa", 0, filename))
    {
        ok(0, "GetTempFileNameA failed %lu\n", GetLastError());
        return FALSE;
    }

    hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ok(0, "CreateFileA failed %lu\n", GetLastError());
        DeleteFileA(filename);
        return FALSE;
    }

    bRet = WriteFile(hFile, data, (DWORD)len, &dwWritten, NULL);
    CloseHandle(hFile);
    bRet = bRet && (dwWritten == len);

    if (!bRet)
    {
        ok(0, "WriteFile failed %lu\n", GetLastError());
        DeleteFileA(filename);
    }

    return bRet;
}

BOOL extract(char* filename, const bin_file* bin)
{
    return extract2(filename, bin->data, bin->len);
}

HANDLE open_file(char* filename, BOOL bWrite)
{
    DWORD dwAccess = GENERIC_READ | (bWrite ? GENERIC_WRITE : 0);
    DWORD dwAttr = (bWrite ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_READONLY);
    return CreateFileA(filename, dwAccess, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       dwAttr, NULL);
}

void compare_file_(char* filename, const unsigned char* data, size_t len, const char* test_name, int line)
{
    char* buf;
    size_t size;
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_READONLY, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ok_(__FILE__, line)(0, "Unable to open file %s(%lu), %s\n", filename, GetLastError(), test_name);
        return;
    }
    size = GetFileSize(hFile, NULL);
    ok(size == len, "Filesize is %u instead of %u, %s\n", size, len, test_name);

    buf = malloc(size);
    if (buf)
    {
        DWORD dwRead;
        if (ReadFile(hFile, buf, (DWORD)size, &dwRead, NULL) && dwRead == size)
        {
            ok(!memcmp(buf, data, min(size,len)), "Data mismatch, %s\n", test_name);
        }
        else
        {
            ok_(__FILE__, line)(0, "Unable to read %s(%lu), %s\n", filename, GetLastError(), test_name);
        }

        free(buf);
    }
    else
    {
        ok_(__FILE__, line)(0, "Unable to allocate %u, %s\n", size, test_name);
    }

    CloseHandle(hFile);
}

#define compare_file(filename, data, len, test_name)   compare_file_(filename, data, len, test_name, __LINE__)

static void validate_signature(const char* casename, const char* fieldname, const bin_file* bin, const char* expected)
{
    char filename[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    HANDLE hFile;
    unsigned char data[0x100];
    char signature[0x20] = {0};
    WCHAR signatureW[0x20] = {0};
    BOOL bResult;

    memset(signature, 0xaa, sizeof(signature));
    memcpy(data, bin->data, bin->len);

    if (!extract(filename, bin))
        return;

    memset(signature, 0xaa, sizeof(signature)-1);
    bResult = GetFilePatchSignatureA(filename, 0, NULL, 0, NULL, 0, NULL, sizeof(signature), signature);
    ok(bResult, "GetFilePatchSignatureA failed %lu, %s.%s\n", GetLastError(), casename, fieldname);
    if (bResult)
    {
        // Signature is crc32 of data
        ok(!_stricmp(expected, signature), "Got %s for %s.%s\n", signature, casename, fieldname);
    }

    memset(signature, 0xaa, sizeof(signature)-1);
    memset(signatureW, 0xaa, sizeof(signatureW)-sizeof(WCHAR));
    // Widechar version has a widechar signature
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, _countof(filenameW));
    bResult = GetFilePatchSignatureW(filenameW, 0, NULL, 0, NULL, 0, NULL, sizeof(signatureW), signatureW);
    ok(bResult, "GetFilePatchSignatureW failed %lu, %s.%s\n", GetLastError(), casename, fieldname);
    if (bResult)
    {
        WideCharToMultiByte(CP_ACP, 0, signatureW, -1, signature, _countof(signature), NULL, NULL);
        // Signature is crc32 of data
        ok(!_stricmp(expected, signature), "Got %s for %s.%s\n", signature, casename, fieldname);
    }

    memset(signature, 0xaa, sizeof(signature)-1);
    // 'Handle' version has ansi signature
    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "Unable to open file %lu\n", GetLastError());
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bResult = GetFilePatchSignatureByHandle(hFile, 0, NULL, 0, NULL, 0, NULL, sizeof(signature), signature);
        ok(bResult, "GetFilePatchSignatureByHandle failed %lu, %s.%s\n", GetLastError(), casename, fieldname);
        if (bResult)
        {
            // Signature is crc32 of data
            ok(!_stricmp(expected, signature), "Got %s for %s.%s\n", signature, casename, fieldname);
        }

        CloseHandle(hFile);
    }

    DeleteFileA(filename);
}

/* Copyright (C) 1986 Gary S. Brown
 * Modified by Robert Shearman. You may use the following calc_crc32 code or
 * tables extracted from it, as desired without restriction. */
static const unsigned int crc_32_tab[] =
{ /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

unsigned int crc32(unsigned int seed, unsigned char* msg, unsigned int msglen)
{
    unsigned int rem = seed;
    unsigned int i;

    for (i = 0; i < msglen; i++)
    {
        rem = crc_32_tab[(rem ^ msg[i]) & 0xff] ^ (rem >> 8);
    }

    return rem;
}

static void validate_patch(patch_data* current)
{
    UINT crc;

    crc = crc32(~0, current->patch.data, (UINT)current->patch.len);
    ok(crc == 0, "Invalid patch crc 0x%x for %s\n", crc, current->name);

    crc = crc32(~0, current->patch_header.data, (UINT)current->patch_header.len);
    ok(crc == 0, "Invalid patch_header crc 0x%x for %s\n", crc, current->name);
}

static void apply_patch(patch_data* current)
{
    char patchname[MAX_PATH], targetname[MAX_PATH], outputname[MAX_PATH];
    BOOL bResult;
    DWORD dwErr;
    HANDLE hPatch, hTarget, hFind;
    WIN32_FIND_DATAA wfd = { sizeof(wfd) };

    if (!GetTempFileNameA(temp_path, "MPO", 0, outputname))
    {
        ok(0, "GetTempFileNameA failed %lu, %s\n", GetLastError(), current->name);
        return;
    }
    DeleteFileA(outputname);

    if (!extract(patchname, &current->patch))
        return;
    if (!extract(targetname, &current->input))
    {
        DeleteFileA(patchname);
        return;
    }
    // There is a bug in 2k3, where calling 'TestApplyPatchToFileA' gives an AV...
#if 0
    bResult = TestApplyPatchToFileA(patchname, targetname, 0);
#else
    hPatch = open_file(patchname, FALSE);
    hTarget = open_file(targetname, FALSE);
    bResult = TestApplyPatchToFileByHandles(hPatch, hTarget, 0);
    CloseHandle(hPatch);
    CloseHandle(hTarget);
#endif
    ok(bResult, "TestApplyPatchToFileA failed %lu, %s\n", GetLastError(), current->name);
    // Files are not touched
    compare_file(patchname, current->patch.data, current->patch.len, current->name);
    compare_file(targetname, current->input.data, current->input.len, current->name);
    DeleteFileA(patchname);
    DeleteFileA(targetname);


    if (!extract2(patchname, current->patch.data, current->patch.len -1))
        return;
    if (!extract(targetname, &current->input))
    {
        DeleteFileA(patchname);
        return;
    }
    hPatch = open_file(patchname, FALSE);
    hTarget = open_file(targetname, FALSE);
    bResult = TestApplyPatchToFileByHandles(hPatch, hTarget, 0);
    dwErr = GetLastError();
    CloseHandle(hPatch);
    CloseHandle(hTarget);
    ok(!bResult, "TestApplyPatchToFileA succeeded, %s\n", current->name);
    ok(dwErr == ERROR_PATCH_CORRUPT, "TestApplyPatchToFileA GetLastError %lx, %s\n", dwErr, current->name);
    // Files are not touched
    compare_file(patchname, current->patch.data, current->patch.len - 1, current->name);
    compare_file(targetname, current->input.data, current->input.len, current->name);
    DeleteFileA(patchname);
    DeleteFileA(targetname);

    if (!extract(patchname, &current->patch))
        return;
    if (!extract2(targetname, current->input.data, current->input.len -1))
    {
        DeleteFileA(patchname);
        return;
    }
    hPatch = open_file(patchname, FALSE);
    hTarget = open_file(targetname, FALSE);
    bResult = TestApplyPatchToFileByHandles(hPatch, hTarget, 0);
    dwErr = GetLastError();
    CloseHandle(hPatch);
    CloseHandle(hTarget);
    ok(!bResult, "TestApplyPatchToFileA succeeded, %s\n", current->name);
    ok(dwErr == ERROR_PATCH_WRONG_FILE, "TestApplyPatchToFileA GetLastError %lx, %s\n", dwErr, current->name);
    // Files are not touched
    compare_file(patchname, current->patch.data, current->patch.len, current->name);
    compare_file(targetname, current->input.data, current->input.len -1, current->name);
    DeleteFileA(patchname);
    DeleteFileA(targetname);

    if (!extract(patchname, &current->patch))
        return;
    if (!extract(targetname, &current->input))
    {
        DeleteFileA(patchname);
        return;
    }
    bResult = ApplyPatchToFileA(patchname, targetname, outputname, APPLY_OPTION_TEST_ONLY);

    ok(bResult, "ApplyPatchToFileA failed %lu, %s\n", GetLastError(), current->name);
    // Files are not touched
    compare_file(patchname, current->patch.data, current->patch.len, current->name);
    compare_file(targetname, current->input.data, current->input.len, current->name);
    // W2k3 creates an empty file, W10 does not create a file
    hFind = FindFirstFileA(outputname, &wfd);
    ok(hFind == INVALID_HANDLE_VALUE || wfd.nFileSizeLow == 0, "Got a (non-empty) file, %s\n", current->name);
    if (hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);
    DeleteFileA(patchname);
    DeleteFileA(targetname);
    DeleteFileA(outputname);

    if (!extract(patchname, &current->patch))
        return;
    if (!extract(targetname, &current->input))
    {
        DeleteFileA(patchname);
        return;
    }
    bResult = ApplyPatchToFileA(patchname, targetname, outputname, 0);
    ok(bResult, "ApplyPatchToFileA failed %lu, %s\n", GetLastError(), current->name);
    // Files are not touched
    compare_file(patchname, current->patch.data, current->patch.len, current->name);
    compare_file(targetname, current->input.data, current->input.len, current->name);
    // One output file
    compare_file(outputname, current->output.data, current->output.len, current->name);
    DeleteFileA(patchname);
    DeleteFileA(targetname);
    DeleteFileA(outputname);
}


void test_one(patch_data* current)
{
    validate_signature(current->name, "input", &current->input, current->input_signature);
    validate_signature(current->name, "output", &current->output, current->output_signature);
    apply_patch(current);
    validate_patch(current);
}



static patch_data data[] =
{
    {
        "in1_text",
        MAKE_BIN(in1_bin),
        "99e5365e",
        MAKE_BIN(out1_bin),
        "f651d108",
        MAKE_BIN(patch1_bin),
        MAKE_BIN(patch1_header_bin),
    },
    {
        "in2_binary",
        MAKE_BIN(in2_bin),
        "fc54c21d",
        MAKE_BIN(out2_bin),
        "028aa0fa",
        MAKE_BIN(patch2_bin),
        MAKE_BIN(patch2_header_bin),
    },
    {
        "in3_compression",
        MAKE_BIN(in3_bin),
        "f10a52d9",
        MAKE_BIN(out3_bin),
        "411dd8f9",
        MAKE_BIN(patch3_bin),
        MAKE_BIN(patch3_header_bin),
    },
};


START_TEST(mspatcha)
{
    size_t n;

    GetTempPathA(_countof(temp_path), temp_path);

    for (n = 0; n < _countof(data); ++n)
    {
        test_one(data + n);
    }
}
