/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "windows.h"

#include "wine/test.h"

/***********************************************************************
 * Compability macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define FIELD_ALIGNMENT(type, field) __alignof(((type*)0)->field)
#elif defined(__GNUC__)
# define FIELD_ALIGNMENT(type, field) __alignof__(((type*)0)->field)
#else
/* FIXME: Not sure if is possible to do without compiler extension */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: Not sure if is possible to do without compiler extension
 *        (if type is not just a name that is, if so the normal)
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(FIELD_ALIGNMENT(type, field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", _TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", sizeof(type))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field_type, field_name, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE(field_type, field_size); \
  TEST_FIELD_ALIGNMENT(type, field_name, field_align); \
  TEST_FIELD_OFFSET(type, field_name, field_offset); \

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_LPOSVERSIONINFOA(void)
{
    /* LPOSVERSIONINFOA */
    TEST_TYPE(LPOSVERSIONINFOA, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOA, 148, 4);
}

static void test_pack_LPOSVERSIONINFOEXA(void)
{
    /* LPOSVERSIONINFOEXA */
    TEST_TYPE(LPOSVERSIONINFOEXA, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOEXA, 156, 4);
}

static void test_pack_LPOSVERSIONINFOEXW(void)
{
    /* LPOSVERSIONINFOEXW */
    TEST_TYPE(LPOSVERSIONINFOEXW, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOEXW, 284, 4);
}

static void test_pack_LPOSVERSIONINFOW(void)
{
    /* LPOSVERSIONINFOW */
    TEST_TYPE(LPOSVERSIONINFOW, 4, 4);
    TEST_TYPE_POINTER(LPOSVERSIONINFOW, 276, 4);
}

static void test_pack_OSVERSIONINFOA(void)
{
    /* OSVERSIONINFOA (pack 4) */
    TEST_TYPE(OSVERSIONINFOA, 148, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOA, CHAR[128], szCSDVersion, 20, 128, 1);
}

static void test_pack_OSVERSIONINFOEXA(void)
{
    /* OSVERSIONINFOEXA (pack 4) */
    TEST_TYPE(OSVERSIONINFOEXA, 156, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXA, CHAR[128], szCSDVersion, 20, 128, 1);
    TEST_FIELD(OSVERSIONINFOEXA, WORD, wServicePackMajor, 148, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, WORD, wServicePackMinor, 150, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, WORD, wSuiteMask, 152, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXA, BYTE, wProductType, 154, 1, 1);
    TEST_FIELD(OSVERSIONINFOEXA, BYTE, wReserved, 155, 1, 1);
}

static void test_pack_OSVERSIONINFOEXW(void)
{
    /* OSVERSIONINFOEXW (pack 4) */
    TEST_TYPE(OSVERSIONINFOEXW, 284, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOEXW, WCHAR[128], szCSDVersion, 20, 256, 2);
    TEST_FIELD(OSVERSIONINFOEXW, WORD, wServicePackMajor, 276, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, WORD, wServicePackMinor, 278, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, WORD, wSuiteMask, 280, 2, 2);
    TEST_FIELD(OSVERSIONINFOEXW, BYTE, wProductType, 282, 1, 1);
    TEST_FIELD(OSVERSIONINFOEXW, BYTE, wReserved, 283, 1, 1);
}

static void test_pack_OSVERSIONINFOW(void)
{
    /* OSVERSIONINFOW (pack 4) */
    TEST_TYPE(OSVERSIONINFOW, 276, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(OSVERSIONINFOW, WCHAR[128], szCSDVersion, 20, 256, 2);
}

static void test_pack_POSVERSIONINFOA(void)
{
    /* POSVERSIONINFOA */
    TEST_TYPE(POSVERSIONINFOA, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOA, 148, 4);
}

static void test_pack_POSVERSIONINFOEXA(void)
{
    /* POSVERSIONINFOEXA */
    TEST_TYPE(POSVERSIONINFOEXA, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOEXA, 156, 4);
}

static void test_pack_POSVERSIONINFOEXW(void)
{
    /* POSVERSIONINFOEXW */
    TEST_TYPE(POSVERSIONINFOEXW, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOEXW, 284, 4);
}

static void test_pack_POSVERSIONINFOW(void)
{
    /* POSVERSIONINFOW */
    TEST_TYPE(POSVERSIONINFOW, 4, 4);
    TEST_TYPE_POINTER(POSVERSIONINFOW, 276, 4);
}

static void test_pack_LPLONG(void)
{
    /* LPLONG */
    TEST_TYPE(LPLONG, 4, 4);
}

static void test_pack_LPVOID(void)
{
    /* LPVOID */
    TEST_TYPE(LPVOID, 4, 4);
}

static void test_pack_PHKEY(void)
{
    /* PHKEY */
    TEST_TYPE(PHKEY, 4, 4);
}

static void test_pack_ACTCTXA(void)
{
    /* ACTCTXA (pack 4) */
    TEST_TYPE(ACTCTXA, 32, 4);
    TEST_FIELD(ACTCTXA, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTXA, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(ACTCTXA, LPCSTR, lpSource, 8, 4, 4);
    TEST_FIELD(ACTCTXA, USHORT, wProcessorArchitecture, 12, 2, 2);
    TEST_FIELD(ACTCTXA, LANGID, wLangId, 14, 2, 2);
    TEST_FIELD(ACTCTXA, LPCSTR, lpAssemblyDirectory, 16, 4, 4);
    TEST_FIELD(ACTCTXA, LPCSTR, lpResourceName, 20, 4, 4);
    TEST_FIELD(ACTCTXA, LPCSTR, lpApplicationName, 24, 4, 4);
    TEST_FIELD(ACTCTXA, HMODULE, hModule, 28, 4, 4);
}

static void test_pack_ACTCTXW(void)
{
    /* ACTCTXW (pack 4) */
    TEST_TYPE(ACTCTXW, 32, 4);
    TEST_FIELD(ACTCTXW, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTXW, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(ACTCTXW, LPCWSTR, lpSource, 8, 4, 4);
    TEST_FIELD(ACTCTXW, USHORT, wProcessorArchitecture, 12, 2, 2);
    TEST_FIELD(ACTCTXW, LANGID, wLangId, 14, 2, 2);
    TEST_FIELD(ACTCTXW, LPCWSTR, lpAssemblyDirectory, 16, 4, 4);
    TEST_FIELD(ACTCTXW, LPCWSTR, lpResourceName, 20, 4, 4);
    TEST_FIELD(ACTCTXW, LPCWSTR, lpApplicationName, 24, 4, 4);
    TEST_FIELD(ACTCTXW, HMODULE, hModule, 28, 4, 4);
}

static void test_pack_ACTCTX_SECTION_KEYED_DATA(void)
{
    /* ACTCTX_SECTION_KEYED_DATA (pack 4) */
    TEST_TYPE(ACTCTX_SECTION_KEYED_DATA, 64, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, ulDataFormatVersion, 4, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, PVOID, lpData, 8, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, ulLength, 12, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, PVOID, lpSectionGlobalData, 16, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, ulSectionGlobalDataLength, 20, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, PVOID, lpSectionBase, 24, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, ulSectionTotalLength, 28, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, HANDLE, hActCtx, 32, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, ulAssemblyRosterIndex, 36, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ULONG, ulFlags, 40, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA, ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, AssemblyMetadata, 44, 20, 4);
}

static void test_pack_ACTCTX_SECTION_KEYED_DATA_2600(void)
{
    /* ACTCTX_SECTION_KEYED_DATA_2600 (pack 4) */
    TEST_TYPE(ACTCTX_SECTION_KEYED_DATA_2600, 40, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ULONG, ulDataFormatVersion, 4, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, PVOID, lpData, 8, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ULONG, ulLength, 12, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, PVOID, lpSectionGlobalData, 16, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ULONG, ulSectionGlobalDataLength, 20, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, PVOID, lpSectionBase, 24, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ULONG, ulSectionTotalLength, 28, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, HANDLE, hActCtx, 32, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_2600, ULONG, ulAssemblyRosterIndex, 36, 4, 4);
}

static void test_pack_ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA(void)
{
    /* ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA (pack 4) */
    TEST_TYPE(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 20, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, PVOID, lpInformation, 0, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, PVOID, lpSectionBase, 4, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, ULONG, ulSectionLength, 8, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, PVOID, lpSectionGlobalDataBase, 12, 4, 4);
    TEST_FIELD(ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, ULONG, ulSectionGlobalDataLength, 16, 4, 4);
}

static void test_pack_ACTIVATION_CONTEXT_BASIC_INFORMATION(void)
{
    /* ACTIVATION_CONTEXT_BASIC_INFORMATION (pack 4) */
    TEST_TYPE(ACTIVATION_CONTEXT_BASIC_INFORMATION, 8, 4);
    TEST_FIELD(ACTIVATION_CONTEXT_BASIC_INFORMATION, HANDLE, hActCtx, 0, 4, 4);
    TEST_FIELD(ACTIVATION_CONTEXT_BASIC_INFORMATION, DWORD, dwFlags, 4, 4, 4);
}

static void test_pack_BY_HANDLE_FILE_INFORMATION(void)
{
    /* BY_HANDLE_FILE_INFORMATION (pack 4) */
    TEST_TYPE(BY_HANDLE_FILE_INFORMATION, 52, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, dwVolumeSerialNumber, 28, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileSizeHigh, 32, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileSizeLow, 36, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nNumberOfLinks, 40, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileIndexHigh, 44, 4, 4);
    TEST_FIELD(BY_HANDLE_FILE_INFORMATION, DWORD, nFileIndexLow, 48, 4, 4);
}

static void test_pack_COMMPROP(void)
{
    /* COMMPROP (pack 4) */
    TEST_TYPE(COMMPROP, 64, 4);
    TEST_FIELD(COMMPROP, WORD, wPacketLength, 0, 2, 2);
    TEST_FIELD(COMMPROP, WORD, wPacketVersion, 2, 2, 2);
    TEST_FIELD(COMMPROP, DWORD, dwServiceMask, 4, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwReserved1, 8, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwMaxTxQueue, 12, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwMaxRxQueue, 16, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwMaxBaud, 20, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvSubType, 24, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvCapabilities, 28, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwSettableParams, 32, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwSettableBaud, 36, 4, 4);
    TEST_FIELD(COMMPROP, WORD, wSettableData, 40, 2, 2);
    TEST_FIELD(COMMPROP, WORD, wSettableStopParity, 42, 2, 2);
    TEST_FIELD(COMMPROP, DWORD, dwCurrentTxQueue, 44, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwCurrentRxQueue, 48, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvSpec1, 52, 4, 4);
    TEST_FIELD(COMMPROP, DWORD, dwProvSpec2, 56, 4, 4);
    TEST_FIELD(COMMPROP, WCHAR[1], wcProvChar, 60, 2, 2);
}

static void test_pack_COMMTIMEOUTS(void)
{
    /* COMMTIMEOUTS (pack 4) */
    TEST_TYPE(COMMTIMEOUTS, 20, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, ReadIntervalTimeout, 0, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, ReadTotalTimeoutMultiplier, 4, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, ReadTotalTimeoutConstant, 8, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, WriteTotalTimeoutMultiplier, 12, 4, 4);
    TEST_FIELD(COMMTIMEOUTS, DWORD, WriteTotalTimeoutConstant, 16, 4, 4);
}

static void test_pack_CREATE_PROCESS_DEBUG_INFO(void)
{
    /* CREATE_PROCESS_DEBUG_INFO (pack 4) */
    TEST_TYPE(CREATE_PROCESS_DEBUG_INFO, 40, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, HANDLE, hFile, 0, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, HANDLE, hProcess, 4, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, HANDLE, hThread, 8, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPVOID, lpBaseOfImage, 12, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, DWORD, dwDebugInfoFileOffset, 16, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, DWORD, nDebugInfoSize, 20, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPVOID, lpThreadLocalBase, 24, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPTHREAD_START_ROUTINE, lpStartAddress, 28, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, LPVOID, lpImageName, 32, 4, 4);
    TEST_FIELD(CREATE_PROCESS_DEBUG_INFO, WORD, fUnicode, 36, 2, 2);
}

static void test_pack_CREATE_THREAD_DEBUG_INFO(void)
{
    /* CREATE_THREAD_DEBUG_INFO (pack 4) */
    TEST_TYPE(CREATE_THREAD_DEBUG_INFO, 12, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, HANDLE, hThread, 0, 4, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, LPVOID, lpThreadLocalBase, 4, 4, 4);
    TEST_FIELD(CREATE_THREAD_DEBUG_INFO, LPTHREAD_START_ROUTINE, lpStartAddress, 8, 4, 4);
}

static void test_pack_CRITICAL_SECTION(void)
{
    /* CRITICAL_SECTION */
    TEST_TYPE(CRITICAL_SECTION, 24, 4);
}

static void test_pack_CRITICAL_SECTION_DEBUG(void)
{
    /* CRITICAL_SECTION_DEBUG */
    TEST_TYPE(CRITICAL_SECTION_DEBUG, 32, 4);
}

static void test_pack_ENUMRESLANGPROCA(void)
{
    /* ENUMRESLANGPROCA */
    TEST_TYPE(ENUMRESLANGPROCA, 4, 4);
}

static void test_pack_ENUMRESLANGPROCW(void)
{
    /* ENUMRESLANGPROCW */
    TEST_TYPE(ENUMRESLANGPROCW, 4, 4);
}

static void test_pack_ENUMRESNAMEPROCA(void)
{
    /* ENUMRESNAMEPROCA */
    TEST_TYPE(ENUMRESNAMEPROCA, 4, 4);
}

static void test_pack_ENUMRESNAMEPROCW(void)
{
    /* ENUMRESNAMEPROCW */
    TEST_TYPE(ENUMRESNAMEPROCW, 4, 4);
}

static void test_pack_ENUMRESTYPEPROCA(void)
{
    /* ENUMRESTYPEPROCA */
    TEST_TYPE(ENUMRESTYPEPROCA, 4, 4);
}

static void test_pack_ENUMRESTYPEPROCW(void)
{
    /* ENUMRESTYPEPROCW */
    TEST_TYPE(ENUMRESTYPEPROCW, 4, 4);
}

static void test_pack_EXCEPTION_DEBUG_INFO(void)
{
    /* EXCEPTION_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXCEPTION_DEBUG_INFO, 84, 4);
    TEST_FIELD(EXCEPTION_DEBUG_INFO, EXCEPTION_RECORD, ExceptionRecord, 0, 80, 4);
    TEST_FIELD(EXCEPTION_DEBUG_INFO, DWORD, dwFirstChance, 80, 4, 4);
}

static void test_pack_EXIT_PROCESS_DEBUG_INFO(void)
{
    /* EXIT_PROCESS_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXIT_PROCESS_DEBUG_INFO, 4, 4);
    TEST_FIELD(EXIT_PROCESS_DEBUG_INFO, DWORD, dwExitCode, 0, 4, 4);
}

static void test_pack_EXIT_THREAD_DEBUG_INFO(void)
{
    /* EXIT_THREAD_DEBUG_INFO (pack 4) */
    TEST_TYPE(EXIT_THREAD_DEBUG_INFO, 4, 4);
    TEST_FIELD(EXIT_THREAD_DEBUG_INFO, DWORD, dwExitCode, 0, 4, 4);
}

static void test_pack_HW_PROFILE_INFOA(void)
{
    /* HW_PROFILE_INFOA (pack 4) */
    TEST_TYPE(HW_PROFILE_INFOA, 124, 4);
    TEST_FIELD(HW_PROFILE_INFOA, DWORD, dwDockInfo, 0, 4, 4);
    TEST_FIELD(HW_PROFILE_INFOA, CHAR[HW_PROFILE_GUIDLEN], szHwProfileGuid, 4, 39, 1);
    TEST_FIELD(HW_PROFILE_INFOA, CHAR[MAX_PROFILE_LEN], szHwProfileName, 43, 80, 1);
}

static void test_pack_HW_PROFILE_INFOW(void)
{
    /* HW_PROFILE_INFOW (pack 4) */
    TEST_TYPE(HW_PROFILE_INFOW, 244, 4);
    TEST_FIELD(HW_PROFILE_INFOW, DWORD, dwDockInfo, 0, 4, 4);
    TEST_FIELD(HW_PROFILE_INFOW, WCHAR[HW_PROFILE_GUIDLEN], szHwProfileGuid, 4, 78, 2);
    TEST_FIELD(HW_PROFILE_INFOW, WCHAR[MAX_PROFILE_LEN], szHwProfileName, 82, 160, 2);
}

static void test_pack_LOAD_DLL_DEBUG_INFO(void)
{
    /* LOAD_DLL_DEBUG_INFO (pack 4) */
    TEST_TYPE(LOAD_DLL_DEBUG_INFO, 24, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, HANDLE, hFile, 0, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, LPVOID, lpBaseOfDll, 4, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, DWORD, dwDebugInfoFileOffset, 8, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, DWORD, nDebugInfoSize, 12, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, LPVOID, lpImageName, 16, 4, 4);
    TEST_FIELD(LOAD_DLL_DEBUG_INFO, WORD, fUnicode, 20, 2, 2);
}

static void test_pack_LPBY_HANDLE_FILE_INFORMATION(void)
{
    /* LPBY_HANDLE_FILE_INFORMATION */
    TEST_TYPE(LPBY_HANDLE_FILE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(LPBY_HANDLE_FILE_INFORMATION, 52, 4);
}

static void test_pack_LPCOMMPROP(void)
{
    /* LPCOMMPROP */
    TEST_TYPE(LPCOMMPROP, 4, 4);
    TEST_TYPE_POINTER(LPCOMMPROP, 64, 4);
}

static void test_pack_LPCOMMTIMEOUTS(void)
{
    /* LPCOMMTIMEOUTS */
    TEST_TYPE(LPCOMMTIMEOUTS, 4, 4);
    TEST_TYPE_POINTER(LPCOMMTIMEOUTS, 20, 4);
}

static void test_pack_LPCRITICAL_SECTION(void)
{
    /* LPCRITICAL_SECTION */
    TEST_TYPE(LPCRITICAL_SECTION, 4, 4);
}

static void test_pack_LPCRITICAL_SECTION_DEBUG(void)
{
    /* LPCRITICAL_SECTION_DEBUG */
    TEST_TYPE(LPCRITICAL_SECTION_DEBUG, 4, 4);
}

static void test_pack_LPEXCEPTION_POINTERS(void)
{
    /* LPEXCEPTION_POINTERS */
    TEST_TYPE(LPEXCEPTION_POINTERS, 4, 4);
}

static void test_pack_LPEXCEPTION_RECORD(void)
{
    /* LPEXCEPTION_RECORD */
    TEST_TYPE(LPEXCEPTION_RECORD, 4, 4);
}

static void test_pack_LPFIBER_START_ROUTINE(void)
{
    /* LPFIBER_START_ROUTINE */
    TEST_TYPE(LPFIBER_START_ROUTINE, 4, 4);
}

static void test_pack_LPHW_PROFILE_INFOA(void)
{
    /* LPHW_PROFILE_INFOA */
    TEST_TYPE(LPHW_PROFILE_INFOA, 4, 4);
    TEST_TYPE_POINTER(LPHW_PROFILE_INFOA, 124, 4);
}

static void test_pack_LPHW_PROFILE_INFOW(void)
{
    /* LPHW_PROFILE_INFOW */
    TEST_TYPE(LPHW_PROFILE_INFOW, 4, 4);
    TEST_TYPE_POINTER(LPHW_PROFILE_INFOW, 244, 4);
}

static void test_pack_LPMEMORYSTATUS(void)
{
    /* LPMEMORYSTATUS */
    TEST_TYPE(LPMEMORYSTATUS, 4, 4);
    TEST_TYPE_POINTER(LPMEMORYSTATUS, 32, 4);
}

static void test_pack_LPOFSTRUCT(void)
{
    /* LPOFSTRUCT */
    TEST_TYPE(LPOFSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPOFSTRUCT, 136, 2);
}

static void test_pack_LPOVERLAPPED(void)
{
    /* LPOVERLAPPED */
    TEST_TYPE(LPOVERLAPPED, 4, 4);
    TEST_TYPE_POINTER(LPOVERLAPPED, 20, 4);
}

static void test_pack_LPOVERLAPPED_COMPLETION_ROUTINE(void)
{
    /* LPOVERLAPPED_COMPLETION_ROUTINE */
    TEST_TYPE(LPOVERLAPPED_COMPLETION_ROUTINE, 4, 4);
}

static void test_pack_LPPROCESS_INFORMATION(void)
{
    /* LPPROCESS_INFORMATION */
    TEST_TYPE(LPPROCESS_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(LPPROCESS_INFORMATION, 16, 4);
}

static void test_pack_LPPROGRESS_ROUTINE(void)
{
    /* LPPROGRESS_ROUTINE */
    TEST_TYPE(LPPROGRESS_ROUTINE, 4, 4);
}

static void test_pack_LPSECURITY_ATTRIBUTES(void)
{
    /* LPSECURITY_ATTRIBUTES */
    TEST_TYPE(LPSECURITY_ATTRIBUTES, 4, 4);
    TEST_TYPE_POINTER(LPSECURITY_ATTRIBUTES, 12, 4);
}

static void test_pack_LPSTARTUPINFOA(void)
{
    /* LPSTARTUPINFOA */
    TEST_TYPE(LPSTARTUPINFOA, 4, 4);
    TEST_TYPE_POINTER(LPSTARTUPINFOA, 68, 4);
}

static void test_pack_LPSTARTUPINFOW(void)
{
    /* LPSTARTUPINFOW */
    TEST_TYPE(LPSTARTUPINFOW, 4, 4);
    TEST_TYPE_POINTER(LPSTARTUPINFOW, 68, 4);
}

static void test_pack_LPSYSTEMTIME(void)
{
    /* LPSYSTEMTIME */
    TEST_TYPE(LPSYSTEMTIME, 4, 4);
    TEST_TYPE_POINTER(LPSYSTEMTIME, 16, 2);
}

static void test_pack_LPSYSTEM_POWER_STATUS(void)
{
    /* LPSYSTEM_POWER_STATUS */
    TEST_TYPE(LPSYSTEM_POWER_STATUS, 4, 4);
    TEST_TYPE_POINTER(LPSYSTEM_POWER_STATUS, 12, 4);
}

static void test_pack_LPTHREAD_START_ROUTINE(void)
{
    /* LPTHREAD_START_ROUTINE */
    TEST_TYPE(LPTHREAD_START_ROUTINE, 4, 4);
}

static void test_pack_LPTIME_ZONE_INFORMATION(void)
{
    /* LPTIME_ZONE_INFORMATION */
    TEST_TYPE(LPTIME_ZONE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(LPTIME_ZONE_INFORMATION, 172, 4);
}

static void test_pack_LPWIN32_FILE_ATTRIBUTE_DATA(void)
{
    /* LPWIN32_FILE_ATTRIBUTE_DATA */
    TEST_TYPE(LPWIN32_FILE_ATTRIBUTE_DATA, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_FILE_ATTRIBUTE_DATA, 36, 4);
}

static void test_pack_LPWIN32_FIND_DATAA(void)
{
    /* LPWIN32_FIND_DATAA */
    TEST_TYPE(LPWIN32_FIND_DATAA, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_FIND_DATAA, 320, 4);
}

static void test_pack_LPWIN32_FIND_DATAW(void)
{
    /* LPWIN32_FIND_DATAW */
    TEST_TYPE(LPWIN32_FIND_DATAW, 4, 4);
    TEST_TYPE_POINTER(LPWIN32_FIND_DATAW, 592, 4);
}

static void test_pack_MEMORYSTATUS(void)
{
    /* MEMORYSTATUS (pack 4) */
    TEST_TYPE(MEMORYSTATUS, 32, 4);
    TEST_FIELD(MEMORYSTATUS, DWORD, dwLength, 0, 4, 4);
    TEST_FIELD(MEMORYSTATUS, DWORD, dwMemoryLoad, 4, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwTotalPhys, 8, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwAvailPhys, 12, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwTotalPageFile, 16, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwAvailPageFile, 20, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwTotalVirtual, 24, 4, 4);
    TEST_FIELD(MEMORYSTATUS, SIZE_T, dwAvailVirtual, 28, 4, 4);
}

static void test_pack_OFSTRUCT(void)
{
    /* OFSTRUCT (pack 4) */
    TEST_TYPE(OFSTRUCT, 136, 2);
    TEST_FIELD(OFSTRUCT, BYTE, cBytes, 0, 1, 1);
    TEST_FIELD(OFSTRUCT, BYTE, fFixedDisk, 1, 1, 1);
    TEST_FIELD(OFSTRUCT, WORD, nErrCode, 2, 2, 2);
    TEST_FIELD(OFSTRUCT, WORD, Reserved1, 4, 2, 2);
    TEST_FIELD(OFSTRUCT, WORD, Reserved2, 6, 2, 2);
    TEST_FIELD(OFSTRUCT, BYTE[OFS_MAXPATHNAME], szPathName, 8, 128, 1);
}

static void test_pack_OUTPUT_DEBUG_STRING_INFO(void)
{
    /* OUTPUT_DEBUG_STRING_INFO (pack 4) */
    TEST_TYPE(OUTPUT_DEBUG_STRING_INFO, 8, 4);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, LPSTR, lpDebugStringData, 0, 4, 4);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, WORD, fUnicode, 4, 2, 2);
    TEST_FIELD(OUTPUT_DEBUG_STRING_INFO, WORD, nDebugStringLength, 6, 2, 2);
}

static void test_pack_OVERLAPPED(void)
{
    /* OVERLAPPED (pack 4) */
    TEST_TYPE(OVERLAPPED, 20, 4);
    TEST_FIELD(OVERLAPPED, DWORD, Internal, 0, 4, 4);
    TEST_FIELD(OVERLAPPED, DWORD, InternalHigh, 4, 4, 4);
    TEST_FIELD(OVERLAPPED, DWORD, Offset, 8, 4, 4);
    TEST_FIELD(OVERLAPPED, DWORD, OffsetHigh, 12, 4, 4);
    TEST_FIELD(OVERLAPPED, HANDLE, hEvent, 16, 4, 4);
}

static void test_pack_PACTCTXA(void)
{
    /* PACTCTXA */
    TEST_TYPE(PACTCTXA, 4, 4);
    TEST_TYPE_POINTER(PACTCTXA, 32, 4);
}

static void test_pack_PACTCTXW(void)
{
    /* PACTCTXW */
    TEST_TYPE(PACTCTXW, 4, 4);
    TEST_TYPE_POINTER(PACTCTXW, 32, 4);
}

static void test_pack_PACTCTX_SECTION_KEYED_DATA(void)
{
    /* PACTCTX_SECTION_KEYED_DATA */
    TEST_TYPE(PACTCTX_SECTION_KEYED_DATA, 4, 4);
    TEST_TYPE_POINTER(PACTCTX_SECTION_KEYED_DATA, 64, 4);
}

static void test_pack_PACTCTX_SECTION_KEYED_DATA_2600(void)
{
    /* PACTCTX_SECTION_KEYED_DATA_2600 */
    TEST_TYPE(PACTCTX_SECTION_KEYED_DATA_2600, 4, 4);
    TEST_TYPE_POINTER(PACTCTX_SECTION_KEYED_DATA_2600, 40, 4);
}

static void test_pack_PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA(void)
{
    /* PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA */
    TEST_TYPE(PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 4, 4);
    TEST_TYPE_POINTER(PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 20, 4);
}

static void test_pack_PACTIVATION_CONTEXT_BASIC_INFORMATION(void)
{
    /* PACTIVATION_CONTEXT_BASIC_INFORMATION */
    TEST_TYPE(PACTIVATION_CONTEXT_BASIC_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PACTIVATION_CONTEXT_BASIC_INFORMATION, 8, 4);
}

static void test_pack_PAPCFUNC(void)
{
    /* PAPCFUNC */
    TEST_TYPE(PAPCFUNC, 4, 4);
}

static void test_pack_PBY_HANDLE_FILE_INFORMATION(void)
{
    /* PBY_HANDLE_FILE_INFORMATION */
    TEST_TYPE(PBY_HANDLE_FILE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PBY_HANDLE_FILE_INFORMATION, 52, 4);
}

static void test_pack_PCACTCTXA(void)
{
    /* PCACTCTXA */
    TEST_TYPE(PCACTCTXA, 4, 4);
    TEST_TYPE_POINTER(PCACTCTXA, 32, 4);
}

static void test_pack_PCACTCTXW(void)
{
    /* PCACTCTXW */
    TEST_TYPE(PCACTCTXW, 4, 4);
    TEST_TYPE_POINTER(PCACTCTXW, 32, 4);
}

static void test_pack_PCACTCTX_SECTION_KEYED_DATA(void)
{
    /* PCACTCTX_SECTION_KEYED_DATA */
    TEST_TYPE(PCACTCTX_SECTION_KEYED_DATA, 4, 4);
    TEST_TYPE_POINTER(PCACTCTX_SECTION_KEYED_DATA, 64, 4);
}

static void test_pack_PCACTCTX_SECTION_KEYED_DATA_2600(void)
{
    /* PCACTCTX_SECTION_KEYED_DATA_2600 */
    TEST_TYPE(PCACTCTX_SECTION_KEYED_DATA_2600, 4, 4);
    TEST_TYPE_POINTER(PCACTCTX_SECTION_KEYED_DATA_2600, 40, 4);
}

static void test_pack_PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA(void)
{
    /* PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA */
    TEST_TYPE(PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 4, 4);
    TEST_TYPE_POINTER(PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, 20, 4);
}

static void test_pack_PCRITICAL_SECTION(void)
{
    /* PCRITICAL_SECTION */
    TEST_TYPE(PCRITICAL_SECTION, 4, 4);
}

static void test_pack_PCRITICAL_SECTION_DEBUG(void)
{
    /* PCRITICAL_SECTION_DEBUG */
    TEST_TYPE(PCRITICAL_SECTION_DEBUG, 4, 4);
}

static void test_pack_PFIBER_START_ROUTINE(void)
{
    /* PFIBER_START_ROUTINE */
    TEST_TYPE(PFIBER_START_ROUTINE, 4, 4);
}

static void test_pack_POFSTRUCT(void)
{
    /* POFSTRUCT */
    TEST_TYPE(POFSTRUCT, 4, 4);
    TEST_TYPE_POINTER(POFSTRUCT, 136, 2);
}

static void test_pack_PPROCESS_INFORMATION(void)
{
    /* PPROCESS_INFORMATION */
    TEST_TYPE(PPROCESS_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PPROCESS_INFORMATION, 16, 4);
}

static void test_pack_PQUERYACTCTXW_FUNC(void)
{
    /* PQUERYACTCTXW_FUNC */
    TEST_TYPE(PQUERYACTCTXW_FUNC, 4, 4);
}

static void test_pack_PROCESS_INFORMATION(void)
{
    /* PROCESS_INFORMATION (pack 4) */
    TEST_TYPE(PROCESS_INFORMATION, 16, 4);
    TEST_FIELD(PROCESS_INFORMATION, HANDLE, hProcess, 0, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, HANDLE, hThread, 4, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, DWORD, dwProcessId, 8, 4, 4);
    TEST_FIELD(PROCESS_INFORMATION, DWORD, dwThreadId, 12, 4, 4);
}

static void test_pack_PSECURITY_ATTRIBUTES(void)
{
    /* PSECURITY_ATTRIBUTES */
    TEST_TYPE(PSECURITY_ATTRIBUTES, 4, 4);
    TEST_TYPE_POINTER(PSECURITY_ATTRIBUTES, 12, 4);
}

static void test_pack_PSYSTEMTIME(void)
{
    /* PSYSTEMTIME */
    TEST_TYPE(PSYSTEMTIME, 4, 4);
    TEST_TYPE_POINTER(PSYSTEMTIME, 16, 2);
}

static void test_pack_PTIMERAPCROUTINE(void)
{
    /* PTIMERAPCROUTINE */
    TEST_TYPE(PTIMERAPCROUTINE, 4, 4);
}

static void test_pack_PTIME_ZONE_INFORMATION(void)
{
    /* PTIME_ZONE_INFORMATION */
    TEST_TYPE(PTIME_ZONE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PTIME_ZONE_INFORMATION, 172, 4);
}

static void test_pack_PWIN32_FIND_DATAA(void)
{
    /* PWIN32_FIND_DATAA */
    TEST_TYPE(PWIN32_FIND_DATAA, 4, 4);
    TEST_TYPE_POINTER(PWIN32_FIND_DATAA, 320, 4);
}

static void test_pack_PWIN32_FIND_DATAW(void)
{
    /* PWIN32_FIND_DATAW */
    TEST_TYPE(PWIN32_FIND_DATAW, 4, 4);
    TEST_TYPE_POINTER(PWIN32_FIND_DATAW, 592, 4);
}

static void test_pack_RIP_INFO(void)
{
    /* RIP_INFO (pack 4) */
    TEST_TYPE(RIP_INFO, 8, 4);
    TEST_FIELD(RIP_INFO, DWORD, dwError, 0, 4, 4);
    TEST_FIELD(RIP_INFO, DWORD, dwType, 4, 4, 4);
}

static void test_pack_SECURITY_ATTRIBUTES(void)
{
    /* SECURITY_ATTRIBUTES (pack 4) */
    TEST_TYPE(SECURITY_ATTRIBUTES, 12, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, DWORD, nLength, 0, 4, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, LPVOID, lpSecurityDescriptor, 4, 4, 4);
    TEST_FIELD(SECURITY_ATTRIBUTES, BOOL, bInheritHandle, 8, 4, 4);
}

static void test_pack_STARTUPINFOA(void)
{
    /* STARTUPINFOA (pack 4) */
    TEST_TYPE(STARTUPINFOA, 68, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, cb, 0, 4, 4);
    TEST_FIELD(STARTUPINFOA, LPSTR, lpReserved, 4, 4, 4);
    TEST_FIELD(STARTUPINFOA, LPSTR, lpDesktop, 8, 4, 4);
    TEST_FIELD(STARTUPINFOA, LPSTR, lpTitle, 12, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwX, 16, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwY, 20, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwXSize, 24, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwYSize, 28, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwXCountChars, 32, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwYCountChars, 36, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwFillAttribute, 40, 4, 4);
    TEST_FIELD(STARTUPINFOA, DWORD, dwFlags, 44, 4, 4);
    TEST_FIELD(STARTUPINFOA, WORD, wShowWindow, 48, 2, 2);
    TEST_FIELD(STARTUPINFOA, WORD, cbReserved2, 50, 2, 2);
    TEST_FIELD(STARTUPINFOA, BYTE *, lpReserved2, 52, 4, 4);
    TEST_FIELD(STARTUPINFOA, HANDLE, hStdInput, 56, 4, 4);
    TEST_FIELD(STARTUPINFOA, HANDLE, hStdOutput, 60, 4, 4);
    TEST_FIELD(STARTUPINFOA, HANDLE, hStdError, 64, 4, 4);
}

static void test_pack_STARTUPINFOW(void)
{
    /* STARTUPINFOW (pack 4) */
    TEST_TYPE(STARTUPINFOW, 68, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, cb, 0, 4, 4);
    TEST_FIELD(STARTUPINFOW, LPWSTR, lpReserved, 4, 4, 4);
    TEST_FIELD(STARTUPINFOW, LPWSTR, lpDesktop, 8, 4, 4);
    TEST_FIELD(STARTUPINFOW, LPWSTR, lpTitle, 12, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwX, 16, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwY, 20, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwXSize, 24, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwYSize, 28, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwXCountChars, 32, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwYCountChars, 36, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwFillAttribute, 40, 4, 4);
    TEST_FIELD(STARTUPINFOW, DWORD, dwFlags, 44, 4, 4);
    TEST_FIELD(STARTUPINFOW, WORD, wShowWindow, 48, 2, 2);
    TEST_FIELD(STARTUPINFOW, WORD, cbReserved2, 50, 2, 2);
    TEST_FIELD(STARTUPINFOW, BYTE *, lpReserved2, 52, 4, 4);
    TEST_FIELD(STARTUPINFOW, HANDLE, hStdInput, 56, 4, 4);
    TEST_FIELD(STARTUPINFOW, HANDLE, hStdOutput, 60, 4, 4);
    TEST_FIELD(STARTUPINFOW, HANDLE, hStdError, 64, 4, 4);
}

static void test_pack_SYSLEVEL(void)
{
    /* SYSLEVEL (pack 4) */
    TEST_TYPE(SYSLEVEL, 28, 4);
    TEST_FIELD(SYSLEVEL, CRITICAL_SECTION, crst, 0, 24, 4);
    TEST_FIELD(SYSLEVEL, INT, level, 24, 4, 4);
}

static void test_pack_SYSTEMTIME(void)
{
    /* SYSTEMTIME (pack 4) */
    TEST_TYPE(SYSTEMTIME, 16, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wYear, 0, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wMonth, 2, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wDayOfWeek, 4, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wDay, 6, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wHour, 8, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wMinute, 10, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wSecond, 12, 2, 2);
    TEST_FIELD(SYSTEMTIME, WORD, wMilliseconds, 14, 2, 2);
}

static void test_pack_SYSTEM_POWER_STATUS(void)
{
    /* SYSTEM_POWER_STATUS (pack 4) */
    TEST_TYPE(SYSTEM_POWER_STATUS, 12, 4);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, ACLineStatus, 0, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, BatteryFlag, 1, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, BatteryLifePercent, 2, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, BYTE, Reserved1, 3, 1, 1);
    TEST_FIELD(SYSTEM_POWER_STATUS, DWORD, BatteryLifeTime, 4, 4, 4);
    TEST_FIELD(SYSTEM_POWER_STATUS, DWORD, BatteryFullLifeTime, 8, 4, 4);
}

static void test_pack_TIME_ZONE_INFORMATION(void)
{
    /* TIME_ZONE_INFORMATION (pack 4) */
    TEST_TYPE(TIME_ZONE_INFORMATION, 172, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, LONG, Bias, 0, 4, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, WCHAR[32], StandardName, 4, 64, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, SYSTEMTIME, StandardDate, 68, 16, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, LONG, StandardBias, 84, 4, 4);
    TEST_FIELD(TIME_ZONE_INFORMATION, WCHAR[32], DaylightName, 88, 64, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, SYSTEMTIME, DaylightDate, 152, 16, 2);
    TEST_FIELD(TIME_ZONE_INFORMATION, LONG, DaylightBias, 168, 4, 4);
}

static void test_pack_UNLOAD_DLL_DEBUG_INFO(void)
{
    /* UNLOAD_DLL_DEBUG_INFO (pack 4) */
    TEST_TYPE(UNLOAD_DLL_DEBUG_INFO, 4, 4);
    TEST_FIELD(UNLOAD_DLL_DEBUG_INFO, LPVOID, lpBaseOfDll, 0, 4, 4);
}

static void test_pack_WAITORTIMERCALLBACK(void)
{
    /* WAITORTIMERCALLBACK */
    TEST_TYPE(WAITORTIMERCALLBACK, 4, 4);
}

static void test_pack_WIN32_FILE_ATTRIBUTE_DATA(void)
{
    /* WIN32_FILE_ATTRIBUTE_DATA (pack 4) */
    TEST_TYPE(WIN32_FILE_ATTRIBUTE_DATA, 36, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, DWORD, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FILE_ATTRIBUTE_DATA, DWORD, nFileSizeLow, 32, 4, 4);
}

static void test_pack_WIN32_FIND_DATAA(void)
{
    /* WIN32_FIND_DATAA (pack 4) */
    TEST_TYPE(WIN32_FIND_DATAA, 320, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, nFileSizeLow, 32, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, dwReserved0, 36, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, DWORD, dwReserved1, 40, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAA, CHAR[260], cFileName, 44, 260, 1);
    TEST_FIELD(WIN32_FIND_DATAA, CHAR[14], cAlternateFileName, 304, 14, 1);
}

static void test_pack_WIN32_FIND_DATAW(void)
{
    /* WIN32_FIND_DATAW (pack 4) */
    TEST_TYPE(WIN32_FIND_DATAW, 592, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, dwFileAttributes, 0, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, FILETIME, ftCreationTime, 4, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, FILETIME, ftLastAccessTime, 12, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, FILETIME, ftLastWriteTime, 20, 8, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, nFileSizeHigh, 28, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, nFileSizeLow, 32, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, dwReserved0, 36, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, DWORD, dwReserved1, 40, 4, 4);
    TEST_FIELD(WIN32_FIND_DATAW, WCHAR[260], cFileName, 44, 520, 2);
    TEST_FIELD(WIN32_FIND_DATAW, WCHAR[14], cAlternateFileName, 564, 28, 2);
}

static void test_pack(void)
{
    test_pack_ACTCTXA();
    test_pack_ACTCTXW();
    test_pack_ACTCTX_SECTION_KEYED_DATA();
    test_pack_ACTCTX_SECTION_KEYED_DATA_2600();
    test_pack_ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA();
    test_pack_ACTIVATION_CONTEXT_BASIC_INFORMATION();
    test_pack_BY_HANDLE_FILE_INFORMATION();
    test_pack_COMMPROP();
    test_pack_COMMTIMEOUTS();
    test_pack_CREATE_PROCESS_DEBUG_INFO();
    test_pack_CREATE_THREAD_DEBUG_INFO();
    test_pack_CRITICAL_SECTION();
    test_pack_CRITICAL_SECTION_DEBUG();
    test_pack_ENUMRESLANGPROCA();
    test_pack_ENUMRESLANGPROCW();
    test_pack_ENUMRESNAMEPROCA();
    test_pack_ENUMRESNAMEPROCW();
    test_pack_ENUMRESTYPEPROCA();
    test_pack_ENUMRESTYPEPROCW();
    test_pack_EXCEPTION_DEBUG_INFO();
    test_pack_EXIT_PROCESS_DEBUG_INFO();
    test_pack_EXIT_THREAD_DEBUG_INFO();
    test_pack_HW_PROFILE_INFOA();
    test_pack_HW_PROFILE_INFOW();
    test_pack_LOAD_DLL_DEBUG_INFO();
    test_pack_LPBY_HANDLE_FILE_INFORMATION();
    test_pack_LPCOMMPROP();
    test_pack_LPCOMMTIMEOUTS();
    test_pack_LPCRITICAL_SECTION();
    test_pack_LPCRITICAL_SECTION_DEBUG();
    test_pack_LPEXCEPTION_POINTERS();
    test_pack_LPEXCEPTION_RECORD();
    test_pack_LPFIBER_START_ROUTINE();
    test_pack_LPHW_PROFILE_INFOA();
    test_pack_LPHW_PROFILE_INFOW();
    test_pack_LPLONG();
    test_pack_LPMEMORYSTATUS();
    test_pack_LPOFSTRUCT();
    test_pack_LPOSVERSIONINFOA();
    test_pack_LPOSVERSIONINFOEXA();
    test_pack_LPOSVERSIONINFOEXW();
    test_pack_LPOSVERSIONINFOW();
    test_pack_LPOVERLAPPED();
    test_pack_LPOVERLAPPED_COMPLETION_ROUTINE();
    test_pack_LPPROCESS_INFORMATION();
    test_pack_LPPROGRESS_ROUTINE();
    test_pack_LPSECURITY_ATTRIBUTES();
    test_pack_LPSTARTUPINFOA();
    test_pack_LPSTARTUPINFOW();
    test_pack_LPSYSTEMTIME();
    test_pack_LPSYSTEM_POWER_STATUS();
    test_pack_LPTHREAD_START_ROUTINE();
    test_pack_LPTIME_ZONE_INFORMATION();
    test_pack_LPVOID();
    test_pack_LPWIN32_FILE_ATTRIBUTE_DATA();
    test_pack_LPWIN32_FIND_DATAA();
    test_pack_LPWIN32_FIND_DATAW();
    test_pack_MEMORYSTATUS();
    test_pack_OFSTRUCT();
    test_pack_OSVERSIONINFOA();
    test_pack_OSVERSIONINFOEXA();
    test_pack_OSVERSIONINFOEXW();
    test_pack_OSVERSIONINFOW();
    test_pack_OUTPUT_DEBUG_STRING_INFO();
    test_pack_OVERLAPPED();
    test_pack_PACTCTXA();
    test_pack_PACTCTXW();
    test_pack_PACTCTX_SECTION_KEYED_DATA();
    test_pack_PACTCTX_SECTION_KEYED_DATA_2600();
    test_pack_PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA();
    test_pack_PACTIVATION_CONTEXT_BASIC_INFORMATION();
    test_pack_PAPCFUNC();
    test_pack_PBY_HANDLE_FILE_INFORMATION();
    test_pack_PCACTCTXA();
    test_pack_PCACTCTXW();
    test_pack_PCACTCTX_SECTION_KEYED_DATA();
    test_pack_PCACTCTX_SECTION_KEYED_DATA_2600();
    test_pack_PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA();
    test_pack_PCRITICAL_SECTION();
    test_pack_PCRITICAL_SECTION_DEBUG();
    test_pack_PFIBER_START_ROUTINE();
    test_pack_PHKEY();
    test_pack_POFSTRUCT();
    test_pack_POSVERSIONINFOA();
    test_pack_POSVERSIONINFOEXA();
    test_pack_POSVERSIONINFOEXW();
    test_pack_POSVERSIONINFOW();
    test_pack_PPROCESS_INFORMATION();
    test_pack_PQUERYACTCTXW_FUNC();
    test_pack_PROCESS_INFORMATION();
    test_pack_PSECURITY_ATTRIBUTES();
    test_pack_PSYSTEMTIME();
    test_pack_PTIMERAPCROUTINE();
    test_pack_PTIME_ZONE_INFORMATION();
    test_pack_PWIN32_FIND_DATAA();
    test_pack_PWIN32_FIND_DATAW();
    test_pack_RIP_INFO();
    test_pack_SECURITY_ATTRIBUTES();
    test_pack_STARTUPINFOA();
    test_pack_STARTUPINFOW();
    test_pack_SYSLEVEL();
    test_pack_SYSTEMTIME();
    test_pack_SYSTEM_POWER_STATUS();
    test_pack_TIME_ZONE_INFORMATION();
    test_pack_UNLOAD_DLL_DEBUG_INFO();
    test_pack_WAITORTIMERCALLBACK();
    test_pack_WIN32_FILE_ATTRIBUTE_DATA();
    test_pack_WIN32_FIND_DATAA();
    test_pack_WIN32_FIND_DATAW();
}

START_TEST(generated)
{
    test_pack();
}
