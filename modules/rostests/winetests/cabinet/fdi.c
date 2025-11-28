/*
 * Unit tests for the File Decompression Interface
 *
 * Copyright (C) 2006 James Hawkins
 * Copyright (C) 2013 Dmitry Timoshkov
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

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <windows.h>
#include "fci.h"
#include "fdi.h"
#include "wine/test.h"

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          999999999
#define FOLDER_THRESHOLD    900000

static CHAR CURR_DIR[MAX_PATH];

#include <pshpack1.h>

struct CFHEADER
{
    UCHAR signature[4];  /* file signature */
    ULONG reserved1;    /* reserved */
    ULONG cbCabinet;    /* size of this cabinet file in bytes */
    ULONG reserved2;    /* reserved */
    ULONG coffFiles;    /* offset of the first CFFILE entry */
    ULONG reserved3;    /* reserved */
    UCHAR versionMinor;  /* cabinet file format version, minor */
    UCHAR versionMajor;  /* cabinet file format version, major */
    USHORT cFolders;    /* number of CFFOLDER entries in this cabinet */
    USHORT cFiles;      /* number of CFFILE entries in this cabinet */
    USHORT flags;       /* cabinet file option indicators */
    USHORT setID;       /* must be the same for all cabinets in a set */
    USHORT iCabinet;    /* number of this cabinet file in a set */
#if 0
    USHORT cbCFHeader;  /* (optional) size of per-cabinet reserved area */
    UCHAR cbCFFolder;    /* (optional) size of per-folder reserved area */
    UCHAR cbCFData;      /* (optional) size of per-datablock reserved area */
    UCHAR abReserve;     /* (optional) per-cabinet reserved area */
    UCHAR szCabinetPrev; /* (optional) name of previous cabinet file */
    UCHAR szDiskPrev;    /* (optional) name of previous disk */
    UCHAR szCabinetNext; /* (optional) name of next cabinet file */
    UCHAR szDiskNext;    /* (optional) name of next disk */
#endif
};

struct CFFOLDER
{
    ULONG coffCabStart;  /* offset of the first CFDATA block in this folder */
    USHORT cCFData;      /* number of CFDATA blocks in this folder */
    USHORT typeCompress; /* compression type indicator */
#if 0
    UCHAR abReserve[];    /* (optional) per-folder reserved area */
#endif
};

struct CFFILE
{
    ULONG cbFile;          /* uncompressed size of this file in bytes */
    ULONG uoffFolderStart; /* uncompressed offset of this file in the folder */
    USHORT iFolder;        /* index into the CFFOLDER area */
    USHORT date;           /* date stamp for this file */
    USHORT time;           /* time stamp for this file */
    USHORT attribs;        /* attribute flags for this file */
#if 0
    UCHAR szName[];         /* name of this file */
#endif
};

struct CFDATA
{
    ULONG csum;       /* checksum of this CFDATA entry */
    USHORT cbData;    /* number of compressed bytes in this block */
    USHORT cbUncomp;  /* number of uncompressed bytes in this block */
#if 0
    UCHAR abReserve[]; /* (optional) per-datablock reserved area */
    UCHAR ab[cbData];  /* compressed data bytes */
#endif
};

static const struct
{
    struct CFHEADER header;
    struct CFFOLDER folder;
    struct CFFILE file;
    UCHAR szName[sizeof("file.dat")];
    struct CFDATA data;
    UCHAR ab[sizeof("Hello World!")-1];
} cab_data =
{
    { {'M','S','C','F'}, 0, 0x59, 0, sizeof(struct CFHEADER) + sizeof(struct CFFOLDER), 0, 3,1, 1, 1, 0, 0x1225, 0x2013 },
    { sizeof(struct CFHEADER) + sizeof(struct CFFOLDER) + sizeof(struct CFFILE) + sizeof("file.dat"), 1, tcompTYPE_NONE },
    { sizeof("Hello World!")-1, 0, 0x1234, 0x1225, 0x2013, 0xa114 },
    { 'f','i','l','e','.','d','a','t',0 },
    { 0, sizeof("Hello World!")-1, sizeof("Hello World!")-1 },
    { 'H','e','l','l','o',' ','W','o','r','l','d','!' }
};

#include <poppack.h>

struct mem_data
{
    const char *base;
    LONG size, pos;
};

/* FDI callbacks */

static void * CDECL fdi_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void * CDECL fdi_alloc_bad(ULONG cb)
{
    return NULL;
}

static void CDECL fdi_free(void *pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

static INT_PTR CDECL fdi_open(char *pszFile, int oflag, int pmode)
{
    HANDLE handle;
    handle = CreateFileA(pszFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                          OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    return (INT_PTR) handle;
}

static UINT CDECL fdi_read(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwRead;
    if (ReadFile(handle, pv, cb, &dwRead, NULL))
        return dwRead;
    return 0;
}

static UINT CDECL fdi_write(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE) hf;
    DWORD dwWritten;
    if (WriteFile(handle, pv, cb, &dwWritten, NULL))
        return dwWritten;
    return 0;
}

static int CDECL fdi_close(INT_PTR hf)
{
    HANDLE handle = (HANDLE) hf;
    return CloseHandle(handle) ? 0 : -1;
}

static LONG CDECL fdi_seek(INT_PTR hf, LONG dist, int seektype)
{
    HANDLE handle = (HANDLE) hf;
    return SetFilePointer(handle, dist, NULL, seektype);
}

/* Callbacks for testing FDIIsCabinet with hf == 0 */
static INT_PTR static_fdi_handle;

static INT_PTR CDECL fdi_open_static(char *pszFile, int oflag, int pmode)
{
    ok(0, "FDIIsCabinet shouldn't call pfnopen\n");
    return 1;
}

static UINT CDECL fdi_read_static(INT_PTR hf, void *pv, UINT cb)
{
    ok(hf == 0, "unexpected hf %Ix\n", hf);
    return fdi_read(static_fdi_handle, pv, cb);
}

static UINT CDECL fdi_write_static(INT_PTR hf, void *pv, UINT cb)
{
    ok(0, "FDIIsCabinet shouldn't call pfnwrite\n");
    return 0;
}

static int CDECL fdi_close_static(INT_PTR hf)
{
    ok(0, "FDIIsCabinet shouldn't call pfnclose\n");
    return 0;
}

static LONG CDECL fdi_seek_static(INT_PTR hf, LONG dist, int seektype)
{
    ok(hf == 0, "unexpected hf %Ix\n", hf);
    return fdi_seek(static_fdi_handle, dist, seektype);
}

static void test_FDICreate(void)
{
    HFDI hfdi;
    ERF erf;

    /* native crashes if pfnalloc is NULL */

    /* FDICreate does not crash with a NULL pfnfree,
     * but FDIDestroy will crash when it tries to access it.
     */
    if (0)
    {
        SetLastError(0xdeadbeef);
        erf.erfOper = 0x1abe11ed;
        erf.erfType = 0x5eed1e55;
        erf.fError = 0x1ead1e55;
        hfdi = FDICreate(fdi_alloc, NULL, fdi_open, fdi_read,
                         fdi_write, fdi_close, fdi_seek,
                         cpuUNKNOWN, &erf);
        ok(hfdi != NULL, "Expected non-NULL context\n");
        ok(GetLastError() == 0xdeadbeef,
           "Expected 0xdeadbeef, got %ld\n", GetLastError());
        ok(erf.erfOper == 0x1abe11ed, "Expected 0x1abe11ed, got %d\n", erf.erfOper);
        ok(erf.erfType == 0x5eed1e55, "Expected 0x5eed1e55, got %d\n", erf.erfType);
        ok(erf.fError == 0x1ead1e55, "Expected 0x1ead1e55, got %d\n", erf.fError);

        FDIDestroy(hfdi);
    }

    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, NULL, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok((erf.erfOper == 0x1abe11ed || erf.erfOper == 0 /* Vista */), "Expected 0x1abe11ed or 0, got %d\n", erf.erfOper);
    ok((erf.erfType == 0x5eed1e55 || erf.erfType == 0 /* Vista */), "Expected 0x5eed1e55 or 0, got %d\n", erf.erfType);
    ok((erf.fError == 0x1ead1e55 || erf.fError == 0 /* Vista */), "Expected 0x1ead1e55 or 0, got %d\n", erf.fError);

    FDIDestroy(hfdi);

    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, NULL,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok((erf.erfOper == 0x1abe11ed || erf.erfOper == 0 /* Vista */), "Expected 0x1abe11ed or 0, got %d\n", erf.erfOper);
    ok((erf.erfType == 0x5eed1e55 || erf.erfType == 0 /* Vista */), "Expected 0x5eed1e55 or 0, got %d\n", erf.erfType);
    ok((erf.fError == 0x1ead1e55 || erf.fError == 0 /* Vista */), "Expected 0x1ead1e55 or 0, got %d\n", erf.fError);

    FDIDestroy(hfdi);

    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     NULL, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok((erf.erfOper == 0x1abe11ed || erf.erfOper == 0 /* Vista */), "Expected 0x1abe11ed or 0, got %d\n", erf.erfOper);
    ok((erf.erfType == 0x5eed1e55 || erf.erfType == 0 /* Vista */), "Expected 0x5eed1e55 or 0, got %d\n", erf.erfType);
    ok((erf.fError == 0x1ead1e55 || erf.fError == 0 /* Vista */), "Expected 0x1ead1e55 or 0, got %d\n", erf.fError);

    FDIDestroy(hfdi);

    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, NULL, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok((erf.erfOper == 0x1abe11ed || erf.erfOper == 0 /* Vista */), "Expected 0x1abe11ed or 0, got %d\n", erf.erfOper);
    ok((erf.erfType == 0x5eed1e55 || erf.erfType == 0 /* Vista */), "Expected 0x5eed1e55 or 0, got %d\n", erf.erfType);
    ok((erf.fError == 0x1ead1e55 || erf.fError == 0 /* Vista */), "Expected 0x1ead1e55 or 0, got %d\n", erf.fError);

    FDIDestroy(hfdi);

    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, NULL,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok((erf.erfOper == 0x1abe11ed || erf.erfOper == 0 /* Vista */), "Expected 0x1abe11ed or 0, got %d\n", erf.erfOper);
    ok((erf.erfType == 0x5eed1e55 || erf.erfType == 0 /* Vista */), "Expected 0x5eed1e55 or 0, got %d\n", erf.erfType);
    ok((erf.fError == 0x1ead1e55 || erf.fError == 0 /* Vista */), "Expected 0x1ead1e55 or 0, got %d\n", erf.fError);

    FDIDestroy(hfdi);

    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, NULL);
    /* XP sets hfdi to a non-NULL value, but Vista sets it to NULL! */
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    /* NULL is passed to FDICreate instead of &erf, so don't retest the erf member values. */

    FDIDestroy(hfdi);

    /* bad cpu type */
    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     0xcafebabe, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok((erf.erfOper == 0x1abe11ed || erf.erfOper == 0 /* Vista */), "Expected 0x1abe11ed or 0, got %d\n", erf.erfOper);
    ok((erf.erfType == 0x5eed1e55 || erf.erfType == 0 /* Vista */), "Expected 0x5eed1e55 or 0, got %d\n", erf.erfType);
    ok((erf.fError == 0x1ead1e55 || erf.fError == 0 /* Vista */), "Expected 0x1ead1e55 or 0, got %d\n", erf.fError);

    FDIDestroy(hfdi);

    /* pfnalloc fails */
    SetLastError(0xdeadbeef);
    erf.erfOper = 0x1abe11ed;
    erf.erfType = 0x5eed1e55;
    erf.fError = 0x1ead1e55;
    hfdi = FDICreate(fdi_alloc_bad, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi == NULL, "Expected NULL context, got %p\n", hfdi);
    ok(erf.erfOper == FDIERROR_ALLOC_FAIL,
       "Expected FDIERROR_ALLOC_FAIL, got %d\n", erf.erfOper);
    ok(erf.fError == TRUE, "Expected TRUE, got %d\n", erf.fError);
    ok(GetLastError() == 0xdeadbeef,
       "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(erf.erfType == 0, "Expected 0, got %d\n", erf.erfType);
}

static void test_FDIDestroy(void)
{
    HFDI hfdi;
    ERF erf;
    BOOL ret;

    /* native crashes if hfdi is NULL or invalid */

    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");

    /* successfully destroy hfdi */
    ret = FDIDestroy(hfdi);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);

    /* native crashes if you try to destroy hfdi twice */
    if (0)
    {
        /* try to destroy hfdi again */
        ret = FDIDestroy(hfdi);
        ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    }
}

static void createTestFile(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

static void create_test_files(void)
{
    createTestFile("a.txt");
    createTestFile("b.txt");
    CreateDirectoryA("testdir", NULL);
    createTestFile("testdir\\c.txt");
    createTestFile("testdir\\d.txt");
}

static void delete_test_files(void)
{
    DeleteFileA("a.txt");
    DeleteFileA("b.txt");
    DeleteFileA("testdir\\c.txt");
    DeleteFileA("testdir\\d.txt");
    RemoveDirectoryA("testdir");

    DeleteFileA("extract.cab");
}

/* FCI callbacks */

static void * CDECL mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void CDECL mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL CDECL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    return TRUE;
}

static LONG CDECL progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int CDECL file_placed(PCCAB pccab, char *pszFile, LONG cbFile,
                             BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR CDECL fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;

    dwAccess = GENERIC_READ | GENERIC_WRITE;
    /* FILE_SHARE_DELETE is not supported by Windows Me/98/95 */
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT CDECL fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;

    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT CDECL fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int CDECL fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static LONG CDECL fci_seek(INT_PTR hf, LONG dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;

    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int CDECL fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL CDECL get_temp_file(char *pszTempName, int cbTempName, void *pv)
{
    LPSTR tempname;

    tempname = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    GetTempFileNameA(".", "xx", 0, tempname);

    if (tempname && (strlen(tempname) < (unsigned)cbTempName))
    {
        lstrcpyA(pszTempName, tempname);
        HeapFree(GetProcessHeap(), 0, tempname);
        return TRUE;
    }

    HeapFree(GetProcessHeap(), 0, tempname);

    return FALSE;
}

static INT_PTR CDECL get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                                   USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFileA(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszName);

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");

    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    attrs = GetFileAttributesA(pszName);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Failed to GetFileAttributes\n");
    /* fixme: should convert attrs to *pattribs, make sure
     * have a test that catches the fact that we don't?
     */

    return (INT_PTR)handle;
}

static void add_file(HFCI hfci, char *file)
{
    char path[MAX_PATH];
    BOOL res;

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    res = FCIAddFile(hfci, path, file, FALSE, get_next_cabinet, progress,
                     get_open_info, tcompTYPE_MSZIP);
    ok(res, "Expected FCIAddFile to succeed\n");
}

static void set_cab_parameters(PCCAB pCabParams)
{
    ZeroMemory(pCabParams, sizeof(CCAB));

    pCabParams->cb = MEDIA_SIZE;
    pCabParams->cbFolderThresh = FOLDER_THRESHOLD;
    pCabParams->setID = 0xbeef;
    lstrcpyA(pCabParams->szCabPath, CURR_DIR);
    lstrcatA(pCabParams->szCabPath, "\\");
    lstrcpyA(pCabParams->szCab, "extract.cab");
}

static void create_cab_file(void)
{
    CCAB cabParams;
    HFCI hfci;
    ERF erf;
    static CHAR a_txt[]         = "a.txt",
                b_txt[]         = "b.txt",
                testdir_c_txt[] = "testdir\\c.txt",
                testdir_d_txt[] = "testdir\\d.txt";
    BOOL res;

    set_cab_parameters(&cabParams);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    add_file(hfci, a_txt);
    add_file(hfci, b_txt);
    add_file(hfci, testdir_c_txt);
    add_file(hfci, testdir_d_txt);

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static void test_FDIIsCabinet(void)
{
    ERF erf;
    BOOL ret;
    HFDI hfdi;
    INT_PTR fd;
    FDICABINETINFO cabinfo;
    char temp[] = "temp.txt";
    char extract[] = "extract.cab";

    create_test_files();
    create_cab_file();

    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");

    /* native crashes if hfdi or cabinfo are NULL or invalid */

    /* invalid file handle */
    ZeroMemory(&cabinfo, sizeof(FDICABINETINFO));
    SetLastError(0xdeadbeef);
    ret = FDIIsCabinet(hfdi, -1, &cabinfo);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(cabinfo.cbCabinet == 0, "Expected 0, got %ld\n", cabinfo.cbCabinet);
    ok(cabinfo.cFiles == 0, "Expected 0, got %d\n", cabinfo.cFiles);
    ok(cabinfo.cFolders == 0, "Expected 0, got %d\n", cabinfo.cFolders);
    ok(cabinfo.iCabinet == 0, "Expected 0, got %d\n", cabinfo.iCabinet);
    ok(cabinfo.setID == 0, "Expected 0, got %d\n", cabinfo.setID);

    createTestFile("temp.txt");
    fd = fdi_open(temp, 0, 0);

    /* file handle doesn't point to a cabinet */
    ZeroMemory(&cabinfo, sizeof(FDICABINETINFO));
    SetLastError(0xdeadbeef);
    ret = FDIIsCabinet(hfdi, fd, &cabinfo);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(cabinfo.cbCabinet == 0, "Expected 0, got %ld\n", cabinfo.cbCabinet);
    ok(cabinfo.cFiles == 0, "Expected 0, got %d\n", cabinfo.cFiles);
    ok(cabinfo.cFolders == 0, "Expected 0, got %d\n", cabinfo.cFolders);
    ok(cabinfo.iCabinet == 0, "Expected 0, got %d\n", cabinfo.iCabinet);
    ok(cabinfo.setID == 0, "Expected 0, got %d\n", cabinfo.setID);

    fdi_close(fd);
    DeleteFileA("temp.txt");

    /* try a real cab */
    fd = fdi_open(extract, 0, 0);
    ZeroMemory(&cabinfo, sizeof(FDICABINETINFO));
    SetLastError(0xdeadbeef);
    ret = FDIIsCabinet(hfdi, fd, &cabinfo);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(cabinfo.cFiles == 4, "Expected 4, got %d\n", cabinfo.cFiles);
    ok(cabinfo.cFolders == 1, "Expected 1, got %d\n", cabinfo.cFolders);
    ok(cabinfo.setID == 0xbeef, "Expected 0xbeef, got %d\n", cabinfo.setID);
    ok(cabinfo.cbCabinet == 182, "Expected 182, got %ld\n", cabinfo.cbCabinet);
    ok(cabinfo.iCabinet == 0, "Expected 0, got %d\n", cabinfo.iCabinet);

    fdi_close(fd);
    FDIDestroy(hfdi);

    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open_static, fdi_read_static,
                     fdi_write_static, fdi_close_static, fdi_seek_static,
                     cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "Expected non-NULL context\n");

    /* FDIIsCabinet accepts hf == 0 even though it's not a valid result of pfnopen */
    static_fdi_handle = fdi_open(extract, 0, 0);
    ZeroMemory(&cabinfo, sizeof(FDICABINETINFO));
    SetLastError(0xdeadbeef);
    ret = FDIIsCabinet(hfdi, 0, &cabinfo);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(cabinfo.cFiles == 4, "Expected 4, got %d\n", cabinfo.cFiles);
    ok(cabinfo.cFolders == 1, "Expected 1, got %d\n", cabinfo.cFolders);
    ok(cabinfo.setID == 0xbeef, "Expected 0xbeef, got %d\n", cabinfo.setID);
    ok(cabinfo.cbCabinet == 182, "Expected 182, got %ld\n", cabinfo.cbCabinet);
    ok(cabinfo.iCabinet == 0, "Expected 0, got %d\n", cabinfo.iCabinet);

    fdi_close(static_fdi_handle);
    FDIDestroy(hfdi);

    delete_test_files();
}


static INT_PTR __cdecl CopyProgress(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    return 37;  /* doc says 0, but anything != -1 apparently means success as well */
}

static INT_PTR CDECL fdi_mem_open(char *name, int oflag, int pmode)
{
    static const char expected[] = "memory\\block";
    struct mem_data *data;

    ok(!strcmp(name, expected), "expected %s, got %s\n", expected, name);
    ok(oflag == _O_BINARY, "expected _O_BINARY, got %x\n", oflag);
    ok(pmode == (_S_IREAD | _S_IWRITE), "expected _S_IREAD | _S_IWRITE, got %x\n", pmode);

    data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data));
    if (!data) return -1;

    data->base = (const char *)&cab_data;
    data->size = sizeof(cab_data);
    data->pos = 0;

    trace("mem_open(%s,%x,%x) => %p\n", name, oflag, pmode, data);
    return (INT_PTR)data;
}

static UINT CDECL fdi_mem_read(INT_PTR hf, void *pv, UINT cb)
{
    struct mem_data *data = (struct mem_data *)hf;
    UINT available, cb_read;

    available = data->size - data->pos;
    cb_read = (available >= cb) ? cb : available;

    memcpy(pv, data->base + data->pos, cb_read);
    data->pos += cb_read;

    /*trace("mem_read(%p,%p,%u) => %u\n", hf, pv, cb, cb_read);*/
    return cb_read;
}

static UINT CDECL fdi_mem_write(INT_PTR hf, void *pv, UINT cb)
{
    static const char expected[] = "Hello World!";

    trace("mem_write(%#Ix,%p,%u)\n", hf, pv, cb);

    ok(hf == 0x12345678, "expected 0x12345678, got %#Ix\n", hf);
    ok(cb == 12, "expected 12, got %u\n", cb);
    ok(!memcmp(pv, expected, 12), "expected %s, got %s\n", expected, (const char *)pv);

    return cb;
}

static int CDECL fdi_mem_close(INT_PTR hf)
{
    HeapFree(GetProcessHeap(), 0, (void *)hf);
    return 0;
}

static LONG CDECL fdi_mem_seek(INT_PTR hf, LONG dist, int seektype)
{
    struct mem_data *data = (struct mem_data *)hf;

    switch (seektype)
    {
    case SEEK_SET:
        data->pos = dist;
        break;

    case SEEK_CUR:
        data->pos += dist;
        break;

    case SEEK_END:
    default:
        ok(0, "seek: not expected type %d\n", seektype);
        return -1;
    }

    if (data->pos < 0) data->pos = 0;
    if (data->pos > data->size) data->pos = data->size;

    /*mem_seek(%p,%d,%d) => %u\n", hf, dist, seektype, data->pos);*/
    return data->pos;
}

static INT_PTR CDECL fdi_mem_notify(FDINOTIFICATIONTYPE fdint, FDINOTIFICATION *info)
{
    static const char expected[9] = "file.dat\0";

    switch (fdint)
    {
    case fdintCLOSE_FILE_INFO:
        trace("mem_notify: CLOSE_FILE_INFO %s, handle %#Ix\n", info->psz1, info->hf);

        ok(!strcmp(info->psz1, expected), "expected %s, got %s\n", expected, info->psz1);
        ok(info->date == 0x1225, "expected 0x1225, got %#x\n", info->date);
        ok(info->time == 0x2013, "expected 0x2013, got %#x\n", info->time);
        ok(info->attribs == 0xa114, "expected 0xa114, got %#x\n", info->attribs);
        ok(info->iFolder == 0x1234, "expected 0x1234, got %#x\n", info->iFolder);
        return 1;

    case fdintCOPY_FILE:
    {
        trace("mem_notify: COPY_FILE %s, %ld bytes\n", info->psz1, info->cb);

        ok(info->cb == 12, "expected 12, got %lu\n", info->cb);
        ok(!strcmp(info->psz1, expected), "expected %s, got %s\n", expected, info->psz1);
        ok(info->iFolder == 0x1234, "expected 0x1234, got %#x\n", info->iFolder);
        return 0x12345678; /* call write() callback */
    }

    default:
        trace("mem_notify(%d,%p)\n", fdint, info);
        return 0;
    }

    return 0;
}

static void test_FDICopy(void)
{
    CCAB cabParams;
    HFDI hfdi;
    HFCI hfci;
    ERF erf;
    BOOL ret;
    char name[] = "extract.cab";
    char path[MAX_PATH + 1];
    char memory_block[] = "memory\\block";
    char memory[] = "memory\\";
    char block[] = "block";
    FDICABINETINFO info;
    INT_PTR fd;

    set_cab_parameters(&cabParams);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                     fci_read, fci_write, fci_close, fci_seek,
                     fci_delete, get_temp_file, &cabParams, NULL);

    ret = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(ret, "Failed to flush the cabinet\n");

    FCIDestroy(hfci);

    lstrcpyA(path, CURR_DIR);

    /* path doesn't have a trailing backslash */
    if (lstrlenA(path) > 2)
    {
        hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                         fdi_write, fdi_close, fdi_seek,
                         cpuUNKNOWN, &erf);

        SetLastError(0xdeadbeef);
        ret = FDICopy(hfdi, name, path, 0, CopyProgress, NULL, 0);
        ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
        ok(GetLastError() == ERROR_INVALID_HANDLE,
           "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

        FDIDestroy(hfdi);
    }
    else
        skip("Running on a root drive directory.\n");

    lstrcatA(path, "\\");

    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_open, fdi_read,
                     fdi_write, fdi_close, fdi_seek,
                     cpuUNKNOWN, &erf);

    /* cabinet with no files or folders */
    SetLastError(0xdeadbeef);
    ret = FDICopy(hfdi, name, path, 0, CopyProgress, NULL, 0);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(GetLastError() == 0, "Expected 0f, got %ld\n", GetLastError());

    FDIDestroy(hfdi);

    DeleteFileA(name);

    /* test extracting from a memory block */
    hfdi = FDICreate(fdi_alloc, fdi_free, fdi_mem_open, fdi_mem_read,
                     fdi_mem_write, fdi_mem_close, fdi_mem_seek, cpuUNKNOWN, &erf);
    ok(hfdi != NULL, "FDICreate error %d\n", erf.erfOper);

    fd = fdi_mem_open(memory_block, _O_BINARY, _S_IREAD | _S_IWRITE);
    ok(fd != -1, "fdi_open failed\n");

    memset(&info, 0, sizeof(info));
    ret = FDIIsCabinet(hfdi, fd, &info);
    ok(ret, "FDIIsCabinet error %d\n",  erf.erfOper);
    ok(info.cbCabinet == 0x59, "expected 0x59, got %#lx\n", info.cbCabinet);
    ok(info.cFiles == 1, "expected 1, got %d\n", info.cFiles);
    ok(info.cFolders == 1, "expected 1, got %d\n", info.cFolders);
    ok(info.setID == 0x1225, "expected 0x1225, got %#x\n", info.setID);
    ok(info.iCabinet == 0x2013, "expected 0x2013, got %#x\n", info.iCabinet);

    fdi_mem_close(fd);

    ret = FDICopy(hfdi, block, memory, 0, fdi_mem_notify, NULL, 0);
    ok(ret, "FDICopy error %d\n", erf.erfOper);

    FDIDestroy(hfdi);
}


START_TEST(fdi)
{
    int len;

    len = GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    if (len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    test_FDICreate();
    test_FDIDestroy();
    test_FDIIsCabinet();
    test_FDICopy();
}
