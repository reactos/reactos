/*
 * Test suite for imagehlp integrity functions
 *
 * Copyright 2009 Owen Rudge for CodeWeavers
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

#define COBJMACROS
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winnt.h"
#include "imagehlp.h"
#include "psapi.h"

static HMODULE hImageHlp, hPsapi;
static char test_dll_path[MAX_PATH];

static BOOL (WINAPI *pImageAddCertificate)(HANDLE, LPWIN_CERTIFICATE, PDWORD);
static BOOL (WINAPI *pImageEnumerateCertificates)(HANDLE, WORD, PDWORD, PDWORD, DWORD);
static BOOL (WINAPI *pImageGetCertificateData)(HANDLE, DWORD, LPWIN_CERTIFICATE, PDWORD);
static BOOL (WINAPI *pImageGetCertificateHeader)(HANDLE, DWORD, LPWIN_CERTIFICATE);
static BOOL (WINAPI *pImageRemoveCertificate)(HANDLE, DWORD);
static PIMAGE_NT_HEADERS (WINAPI *pCheckSumMappedFile)(PVOID, DWORD, PDWORD, PDWORD);

static BOOL (WINAPI *pGetModuleInformation)(HANDLE, HMODULE, LPMODULEINFO, DWORD cb);

static const char test_cert_data[] =
{0x30,0x82,0x02,0xE1,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x07,0x02
,0xA0,0x82,0x02,0xD2,0x30,0x82,0x02,0xCE,0x02,0x01,0x01,0x31,0x00,0x30,0x0B
,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x07,0x01,0xA0,0x82,0x02,0xB4
,0x30,0x82,0x02,0xB0,0x30,0x82,0x02,0x19,0xA0,0x03,0x02,0x01,0x02,0x02,0x09
,0x00,0xE2,0x59,0x17,0xA5,0x87,0x0F,0x88,0x89,0x30,0x0D,0x06,0x09,0x2A,0x86
,0x48,0x86,0xF7,0x0D,0x01,0x01,0x05,0x05,0x00,0x30,0x45,0x31,0x0B,0x30,0x09
,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x41,0x55,0x31,0x13,0x30,0x11,0x06,0x03
,0x55,0x04,0x08,0x13,0x0A,0x53,0x6F,0x6D,0x65,0x2D,0x53,0x74,0x61,0x74,0x65
,0x31,0x21,0x30,0x1F,0x06,0x03,0x55,0x04,0x0A,0x13,0x18,0x49,0x6E,0x74,0x65
,0x72,0x6E,0x65,0x74,0x20,0x57,0x69,0x64,0x67,0x69,0x74,0x73,0x20,0x50,0x74
,0x79,0x20,0x4C,0x74,0x64,0x30,0x1E,0x17,0x0D,0x30,0x39,0x31,0x31,0x32,0x30
,0x31,0x37,0x33,0x38,0x31,0x32,0x5A,0x17,0x0D,0x31,0x30,0x31,0x31,0x32,0x30
,0x31,0x37,0x33,0x38,0x31,0x32,0x5A,0x30,0x45,0x31,0x0B,0x30,0x09,0x06,0x03
,0x55,0x04,0x06,0x13,0x02,0x41,0x55,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04
,0x08,0x13,0x0A,0x53,0x6F,0x6D,0x65,0x2D,0x53,0x74,0x61,0x74,0x65,0x31,0x21
,0x30,0x1F,0x06,0x03,0x55,0x04,0x0A,0x13,0x18,0x49,0x6E,0x74,0x65,0x72,0x6E
,0x65,0x74,0x20,0x57,0x69,0x64,0x67,0x69,0x74,0x73,0x20,0x50,0x74,0x79,0x20
,0x4C,0x74,0x64,0x30,0x81,0x9F,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7
,0x0D,0x01,0x01,0x01,0x05,0x00,0x03,0x81,0x8D,0x00,0x30,0x81,0x89,0x02,0x81
,0x81,0x00,0x9B,0xC1,0x5E,0x28,0x70,0x32,0x81,0xEF,0x41,0x5C,0xCA,0x29,0x4A
,0xB0,0x12,0xF7,0xAE,0x1E,0x30,0x93,0x14,0x3E,0x54,0x7C,0xC3,0x60,0x8C,0xB2
,0x2F,0xC4,0x1F,0x20,0xEE,0x76,0xAC,0x83,0xD9,0xD4,0xC0,0x3C,0x78,0x6B,0xAA
,0xA2,0x35,0x08,0x72,0x4A,0x5F,0xAE,0xD6,0x7D,0x5A,0xD8,0x27,0xEC,0xE0,0x24
,0xBE,0xBE,0x62,0x86,0xF9,0x83,0x66,0x20,0xBC,0xF6,0x4B,0xC8,0x2D,0x1B,0x4C
,0x5C,0xFA,0x0C,0x42,0x9F,0x57,0x49,0xDC,0xB9,0xC7,0x88,0x53,0xFA,0x26,0x21
,0xC3,0xAB,0x4D,0x93,0x83,0x48,0x88,0xF1,0x14,0xB8,0x64,0x03,0x46,0x58,0x35
,0xAC,0xD2,0xD2,0x9C,0xD4,0x6F,0xA4,0xE4,0x88,0x83,0x1C,0xD8,0x98,0xEE,0x2C
,0xA3,0xEC,0x0C,0x4B,0xFB,0x1D,0x6E,0xBE,0xD9,0x77,0x02,0x03,0x01,0x00,0x01
,0xA3,0x81,0xA7,0x30,0x81,0xA4,0x30,0x1D,0x06,0x03,0x55,0x1D,0x0E,0x04,0x16
,0x04,0x14,0x3F,0xB3,0xC8,0x15,0x12,0xC7,0xD8,0xC0,0x13,0x3D,0xBE,0xF1,0x2F
,0x5A,0xB3,0x51,0x59,0x79,0x89,0xF8,0x30,0x75,0x06,0x03,0x55,0x1D,0x23,0x04
,0x6E,0x30,0x6C,0x80,0x14,0x3F,0xB3,0xC8,0x15,0x12,0xC7,0xD8,0xC0,0x13,0x3D
,0xBE,0xF1,0x2F,0x5A,0xB3,0x51,0x59,0x79,0x89,0xF8,0xA1,0x49,0xA4,0x47,0x30
,0x45,0x31,0x0B,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x41,0x55,0x31
,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x08,0x13,0x0A,0x53,0x6F,0x6D,0x65,0x2D
,0x53,0x74,0x61,0x74,0x65,0x31,0x21,0x30,0x1F,0x06,0x03,0x55,0x04,0x0A,0x13
,0x18,0x49,0x6E,0x74,0x65,0x72,0x6E,0x65,0x74,0x20,0x57,0x69,0x64,0x67,0x69
,0x74,0x73,0x20,0x50,0x74,0x79,0x20,0x4C,0x74,0x64,0x82,0x09,0x00,0xE2,0x59
,0x17,0xA5,0x87,0x0F,0x88,0x89,0x30,0x0C,0x06,0x03,0x55,0x1D,0x13,0x04,0x05
,0x30,0x03,0x01,0x01,0xFF,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D
,0x01,0x01,0x05,0x05,0x00,0x03,0x81,0x81,0x00,0x52,0x09,0xA5,0x81,0x63,0xEF
,0xF7,0x76,0x65,0x2B,0xA5,0x48,0xC1,0xC5,0xE0,0x73,0x60,0x9B,0x66,0x2E,0x21
,0xCF,0xF2,0xBD,0xFF,0x81,0xC4,0x99,0x39,0xD0,0x5D,0x1B,0x12,0xFD,0xAE,0x30
,0x5D,0x9C,0x1A,0xD4,0x76,0x8A,0x25,0x10,0x0A,0x7E,0x5D,0x78,0xB5,0x94,0xD8
,0x97,0xBD,0x9A,0x5A,0xD6,0x23,0xCA,0x5C,0x46,0x8C,0xC7,0x30,0x45,0xB4,0x77
,0x44,0x6F,0x16,0xDD,0xC6,0x58,0xFE,0x16,0x15,0xAD,0xB8,0x58,0x49,0x9A,0xFE
,0x6B,0x87,0x78,0xEE,0x13,0xFF,0x29,0x26,0x8E,0x13,0x83,0x0D,0x18,0xCA,0x9F
,0xA9,0x3E,0x6E,0x3C,0xA6,0x50,0x4A,0x04,0x71,0x9F,0x2E,0xCF,0x25,0xA6,0x03
,0x46,0xCA,0xEB,0xEA,0x67,0x89,0x49,0x7C,0x43,0xA2,0x52,0xD9,0x41,0xCC,0x65
,0xED,0x2D,0xA1,0x00,0x31,0x00};

static const char test_cert_data_2[] = {0xDE,0xAD,0xBE,0xEF,0x01,0x02,0x03};

static char test_pe_executable[] =
{
    0x4d,0x5a,0x90,0x00,0x03,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xff,0xff,0x00,
    0x00,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x80,0x00,0x00,0x00,0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,
    0x4c,0xcd,0x21,0x54,0x68,0x69,0x73,0x20,0x70,0x72,0x6f,0x67,0x72,0x61,0x6d,
    0x20,0x63,0x61,0x6e,0x6e,0x6f,0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6e,0x20,
    0x69,0x6e,0x20,0x44,0x4f,0x53,0x20,0x6d,0x6f,0x64,0x65,0x2e,0x0d,0x0d,0x0a,
    0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x45,0x00,0x00,0x4c,0x01,0x0f,
    0x00,0xfd,0x38,0xc9,0x55,0x00,0x24,0x01,0x00,0xea,0x04,0x00,0x00,0xe0,0x00,
    0x07,0x01,0x0b,0x01,0x02,0x18,0x00,0x1a,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,
    0x06,0x00,0x00,0xe0,0x14,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x30,0x00,0x00,
    0x00,0x00,0x40,0x00,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x00,0x04,0x00,0x00,
    0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,
    0x01,0x00,0x00,0x04,0x00,0x00,/* checksum */ 0x11,0xEF,0xCD,0xAB,0x03,0x00,
    0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00
};

static BOOL copy_dll_file(void)
{
    char sys_dir[MAX_PATH+15];
    char temp_path[MAX_PATH];

    if (GetSystemDirectoryA(sys_dir, MAX_PATH) == 0)
    {
        skip("Failed to get system directory. Skipping certificate/PE image tests.\n");
        return FALSE;
    }

    if (sys_dir[lstrlenA(sys_dir) - 1] != '\\')
        lstrcatA(sys_dir, "\\");

    lstrcatA(sys_dir, "imagehlp.dll");

    /* Copy DLL to a temp file */
    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "img", 0, test_dll_path);

    if (CopyFileA(sys_dir, test_dll_path, FALSE) == 0)
    {
        skip("Unable to create copy of imagehlp.dll for tests.\n");
        return FALSE;
    }

    return TRUE;
}

static DWORD get_file_size(void)
{
    HANDLE file;
    DWORD filesize = 0;

    file = CreateFileA(test_dll_path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return 0;

    filesize = GetFileSize(file, NULL);
    CloseHandle(file);

    return filesize;
}

static DWORD test_add_certificate(const char *cert_data, int len)
{
    HANDLE hFile;
    LPWIN_CERTIFICATE cert;
    DWORD cert_len;
    DWORD index;
    BOOL ret;

    hFile = CreateFileA(test_dll_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        skip("Unable to open %s, skipping test\n", test_dll_path);
        return ~0;
    }

    cert_len = sizeof(WIN_CERTIFICATE) + len;
    cert = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cert_len);

    if (!cert)
    {
        skip("Unable to allocate memory, skipping test\n");
        CloseHandle(hFile);
        return ~0;
    }

    cert->dwLength = cert_len;
    cert->wRevision = WIN_CERT_REVISION_1_0;
    cert->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
    CopyMemory(cert->bCertificate, cert_data, len);

    ret = pImageAddCertificate(hFile, cert, &index);
    ok(ret, "Unable to add certificate to image, error %x\n", GetLastError());
    trace("added cert index %d\n", index);

    HeapFree(GetProcessHeap(), 0, cert);
    CloseHandle(hFile);
    return index;
}

static void test_get_certificate(const char *cert_data, int index)
{
    HANDLE hFile;
    LPWIN_CERTIFICATE cert;
    DWORD cert_len = 0;
    DWORD err;
    BOOL ret;

    hFile = CreateFileA(test_dll_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        skip("Unable to open %s, skipping test\n", test_dll_path);
        return;
    }

    ret = pImageGetCertificateData(hFile, index, NULL, &cert_len);
    err = GetLastError();

    ok ((ret == FALSE) && (err == ERROR_INSUFFICIENT_BUFFER), "ImageGetCertificateData gave unexpected result; ret=%d / err=%x\n", ret, err);

    cert = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cert_len);

    if (!cert)
    {
        skip("Unable to allocate memory, skipping test\n");
        CloseHandle(hFile);
        return;
    }

    ret = pImageGetCertificateData(hFile, index, cert, &cert_len);
    ok(ret, "Unable to retrieve certificate; err=%x\n", GetLastError());
    ok(memcmp(cert->bCertificate, cert_data, cert_len - sizeof(WIN_CERTIFICATE)) == 0, "Certificate retrieved did not match original\n");

    HeapFree(GetProcessHeap(), 0, cert);
    CloseHandle(hFile);
}

static void test_remove_certificate(int index)
{
    DWORD orig_count = 0, count = 0;
    HANDLE hFile;
    BOOL ret;

    hFile = CreateFileA(test_dll_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        skip("Unable to open %s, skipping test\n", test_dll_path);
        return;
    }

    ret = pImageEnumerateCertificates(hFile, CERT_SECTION_TYPE_ANY, &orig_count, NULL, 0);
    ok (ret, "Unable to enumerate certificates in file; err=%x\n", GetLastError());
    ret = pImageRemoveCertificate(hFile, index);
    ok (ret, "Unable to remove certificate from file; err=%x\n", GetLastError());

    /* Test to see if the certificate has actually been removed */
    pImageEnumerateCertificates(hFile, CERT_SECTION_TYPE_ANY, &count, NULL, 0);
    ok (count == orig_count - 1, "Certificate count mismatch; orig=%d new=%d\n", orig_count, count);

    CloseHandle(hFile);
}

static DWORD _get_checksum_offset(PVOID base, PIMAGE_NT_HEADERS *nt_header, DWORD *checksum)
{
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    PIMAGE_NT_HEADERS32 Header32;
    PIMAGE_NT_HEADERS64 Header64;

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    Header32 = (IMAGE_NT_HEADERS32 *)((char *)dos + dos->e_lfanew);
    if (Header32->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    *nt_header = (PIMAGE_NT_HEADERS)Header32;

    if (Header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        *checksum = Header32->OptionalHeader.CheckSum;
        return (char *)&Header32->OptionalHeader.CheckSum - (char *)base;
    }
    else if (Header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        Header64 = (IMAGE_NT_HEADERS64 *)Header32;
        *checksum = Header64->OptionalHeader.CheckSum;
        return (char *)&Header64->OptionalHeader.CheckSum - (char *)base;
    }

    return 0;
}

static void test_pe_checksum(void)
{
    DWORD checksum_orig, checksum_new, checksum_off, checksum_correct;
    PIMAGE_NT_HEADERS nt_header;
    PIMAGE_NT_HEADERS ret;
    HMODULE quartz_data;
    char* quartz_base;
    MODULEINFO modinfo;
    char buffer[20];
    BOOL ret_bool;

    if (!pCheckSumMappedFile)
    {
        win_skip("CheckSumMappedFile not supported, skipping tests\n");
        return;
    }

    SetLastError(0xdeadbeef);
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(NULL, 0, &checksum_orig, &checksum_new);
    ok(!ret, "Expected CheckSumMappedFile to fail, got %p\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected err=0xdeadbeef, got %x\n", GetLastError());
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0, "Expected 0, got %x\n", checksum_new);

    SetLastError(0xdeadbeef);
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile((void *)0xdeadbeef, 0, &checksum_orig, &checksum_new);
    ok(!ret, "Expected CheckSumMappedFile to fail, got %p\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected err=0xdeadbeef, got %x\n", GetLastError());
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0, "Expected 0, got %x\n", checksum_new);

    if (0)
    {
        /* crashes on Windows */
        checksum_orig = checksum_new = 0xdeadbeef;
        pCheckSumMappedFile(0, 0x1000, &checksum_orig, &checksum_new);
        pCheckSumMappedFile((void *)0xdeadbeef, 0x1000, NULL, NULL);
    }

    /* basic checksum tests */
    memset(buffer, 0x11, sizeof(buffer));
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(buffer, sizeof(buffer), &checksum_orig, &checksum_new);
    ok(ret == NULL, "Expected NULL, got %p\n", ret);
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0xaabe, "Expected 0xaabe, got %x\n", checksum_new);

    memset(buffer, 0x22, sizeof(buffer));
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(buffer, sizeof(buffer), &checksum_orig, &checksum_new);
    ok(ret == NULL, "Expected NULL, got %p\n", ret);
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0x5569, "Expected 0x5569, got %x\n", checksum_new);

    memset(buffer, 0x22, sizeof(buffer));
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(buffer, 10, &checksum_orig, &checksum_new);
    ok(ret == NULL, "Expected NULL, got %p\n", ret);
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0xaab4, "Expected 0xaab4, got %x\n", checksum_new);

    memset(buffer, 0x22, sizeof(buffer));
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(buffer, 11, &checksum_orig, &checksum_new);
    ok(ret == NULL, "Expected NULL, got %p\n", ret);
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0xaad7, "Expected 0xaad7, got %x\n", checksum_new);

    /* test checksum of PE module */
    memset(buffer, 0x22, sizeof(buffer));
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(test_pe_executable, sizeof(test_pe_executable),
                              &checksum_orig, &checksum_new);
    ok((char *)ret == test_pe_executable + 0x80, "Expected %p, got %p\n", test_pe_executable + 0x80, ret);
    ok(checksum_orig == 0xabcdef11, "Expected 0xabcdef11, got %x\n", checksum_orig);
    ok(checksum_new == 0xaa4, "Expected 0xaa4, got %x\n", checksum_new);

    if (!pGetModuleInformation)
    {
        win_skip("GetModuleInformation not supported, skipping tests\n");
        return;
    }

    ret_bool = pGetModuleInformation(GetCurrentProcess(), GetModuleHandleA(NULL),
                                     &modinfo, sizeof(modinfo));
    ok(ret_bool, "GetModuleInformation failed, error: %x\n", GetLastError());

    if (0)
    {
        /* crashes on Windows */
        pCheckSumMappedFile(modinfo.lpBaseOfDll, modinfo.SizeOfImage, NULL, NULL);
    }

    SetLastError(0xdeadbeef);
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(modinfo.lpBaseOfDll, modinfo.SizeOfImage, &checksum_orig, &checksum_new);
    ok(ret != NULL, "Expected CheckSumMappedFile to succeed\n");
    ok(GetLastError() == 0xdeadbeef, "Expected err=0xdeadbeef, got %x\n", GetLastError());
    ok(checksum_orig != 0xdeadbeef, "Expected orig checksum != 0xdeadbeef\n");
    ok(checksum_new != 0xdeadbeef, "Expected new checksum != 0xdeadbeef\n");

    SetLastError(0xdeadbeef);
    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile((char *)modinfo.lpBaseOfDll + 100, modinfo.SizeOfImage - 100,
                              &checksum_orig, &checksum_new);
    ok(!ret, "Expected CheckSumMappedFile to fail, got %p\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected err=0xdeadbeef, got %x\n", GetLastError());
    ok(checksum_orig == 0, "Expected 0xdeadbeef, got %x\n", checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_off = _get_checksum_offset(modinfo.lpBaseOfDll, &nt_header, &checksum_correct);
    ok(checksum_off != 0, "Failed to get checksum offset\n");

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(modinfo.lpBaseOfDll, checksum_off, &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(modinfo.lpBaseOfDll, (char *)nt_header - (char *)modinfo.lpBaseOfDll,
                              &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(modinfo.lpBaseOfDll, sizeof(IMAGE_DOS_HEADER),
                              &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(modinfo.lpBaseOfDll, 0, &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    todo_wine ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile((char *)modinfo.lpBaseOfDll + 1, 0,
                              &checksum_orig, &checksum_new);
    ok(ret == NULL, "Expected NULL, got %p\n", ret);
    ok(checksum_orig == 0, "Expected 0, got %x\n", checksum_orig);
    ok(checksum_new == 0, "Expected 0, got %x\n", checksum_new);

    quartz_data = LoadLibraryExA("quartz.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!quartz_data)
    {
        skip("Failed to load quartz as datafile, skipping tests\n");
        return;
    }

    quartz_base = (char *)((DWORD_PTR)quartz_data & ~1);
    checksum_off = _get_checksum_offset(quartz_base, &nt_header, &checksum_correct);
    ok(checksum_off != 0, "Failed to get checksum offset\n");

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(quartz_base, checksum_off, &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(quartz_base, (char *)nt_header - quartz_base,
                              &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(quartz_base, sizeof(IMAGE_DOS_HEADER), &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    checksum_orig = checksum_new = 0xdeadbeef;
    ret = pCheckSumMappedFile(quartz_base, 0, &checksum_orig, &checksum_new);
    ok(ret == nt_header, "Expected %p, got %p\n", nt_header, ret);
    ok(checksum_orig == checksum_correct, "Expected %x, got %x\n", checksum_correct, checksum_orig);
    todo_wine ok(checksum_new != 0 && checksum_new != 0xdeadbeef, "Got unexpected value %x\n", checksum_new);

    FreeLibrary(quartz_data);
}

START_TEST(integrity)
{
    DWORD file_size, file_size_orig, first, second;

    hImageHlp = LoadLibraryA("imagehlp.dll");

    if (!hImageHlp)
    {
        win_skip("ImageHlp unavailable\n");
        return;
    }

    if (!copy_dll_file())
    {
        FreeLibrary(hImageHlp);
        return;
    }

    file_size_orig = get_file_size();
    /*
     * Align file_size_orig to an 8-byte boundary. This avoids tests failures where
     * the original dll is not correctly aligned (but when written to it will be).
     */
    if (file_size_orig % 8 != 0)
    {
        skip("We need to align to an 8-byte boundary\n");
        file_size_orig = (file_size_orig + 7) & ~7;
    }

    pImageAddCertificate = (void *) GetProcAddress(hImageHlp, "ImageAddCertificate");
    pImageEnumerateCertificates = (void *) GetProcAddress(hImageHlp, "ImageEnumerateCertificates");
    pImageGetCertificateData = (void *) GetProcAddress(hImageHlp, "ImageGetCertificateData");
    pImageGetCertificateHeader = (void *) GetProcAddress(hImageHlp, "ImageGetCertificateHeader");
    pImageRemoveCertificate = (void *) GetProcAddress(hImageHlp, "ImageRemoveCertificate");
    pCheckSumMappedFile = (void *) GetProcAddress(hImageHlp, "CheckSumMappedFile");

    hPsapi = LoadLibraryA("psapi.dll");
    if (hPsapi)
        pGetModuleInformation = (void *) GetProcAddress(hPsapi, "GetModuleInformation");

    first = test_add_certificate(test_cert_data, sizeof(test_cert_data));
    test_get_certificate(test_cert_data, first);
    test_remove_certificate(first);

    file_size = get_file_size();
    ok(file_size == file_size_orig, "File size different after add and remove (old: %d; new: %d)\n", file_size_orig, file_size);

    /* Try adding multiple certificates */
    first = test_add_certificate(test_cert_data, sizeof(test_cert_data));
    second = test_add_certificate(test_cert_data_2, sizeof(test_cert_data_2));
    ok(second == first + 1, "got %d %d\n", first, second);

    test_get_certificate(test_cert_data, first);
    test_get_certificate(test_cert_data_2, second);

    /* Remove the first one and verify the second certificate is intact */
    test_remove_certificate(first);
    second--;
    test_get_certificate(test_cert_data_2, second);

    test_remove_certificate(second);

    file_size = get_file_size();
    ok(file_size == file_size_orig, "File size different after add and remove (old: %d; new: %d)\n", file_size_orig, file_size);

    test_pe_checksum();

    if (hPsapi) FreeLibrary(hPsapi);
    FreeLibrary(hImageHlp);
    DeleteFileA(test_dll_path);
}
