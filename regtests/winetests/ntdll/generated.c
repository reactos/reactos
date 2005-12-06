/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "ntdll_test.h"

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

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
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
           (int)FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

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

static void test_pack_DWORD32(void)
{
    /* DWORD32 */
    TEST_TYPE(DWORD32, 4, 4);
    TEST_TYPE_UNSIGNED(DWORD32);
}

static void test_pack_DWORD64(void)
{
    /* DWORD64 */
    TEST_TYPE(DWORD64, 8, 8);
    TEST_TYPE_UNSIGNED(DWORD64);
}

static void test_pack_DWORD_PTR(void)
{
    /* DWORD_PTR */
    TEST_TYPE(DWORD_PTR, 4, 4);
}

static void test_pack_HALF_PTR(void)
{
    /* HALF_PTR */
    TEST_TYPE(HALF_PTR, 2, 2);
    TEST_TYPE_SIGNED(HALF_PTR);
}

static void test_pack_INT16(void)
{
    /* INT16 */
    TEST_TYPE(INT16, 2, 2);
    TEST_TYPE_SIGNED(INT16);
}

static void test_pack_INT32(void)
{
    /* INT32 */
    TEST_TYPE(INT32, 4, 4);
    TEST_TYPE_SIGNED(INT32);
}

static void test_pack_INT64(void)
{
    /* INT64 */
    TEST_TYPE(INT64, 8, 8);
    TEST_TYPE_SIGNED(INT64);
}

static void test_pack_INT8(void)
{
    /* INT8 */
    TEST_TYPE(INT8, 1, 1);
    TEST_TYPE_SIGNED(INT8);
}

static void test_pack_INT_PTR(void)
{
    /* INT_PTR */
    TEST_TYPE(INT_PTR, 4, 4);
    TEST_TYPE_SIGNED(INT_PTR);
}

static void test_pack_LONG32(void)
{
    /* LONG32 */
    TEST_TYPE(LONG32, 4, 4);
    TEST_TYPE_SIGNED(LONG32);
}

static void test_pack_LONG64(void)
{
    /* LONG64 */
    TEST_TYPE(LONG64, 8, 8);
    TEST_TYPE_SIGNED(LONG64);
}

static void test_pack_LONG_PTR(void)
{
    /* LONG_PTR */
    TEST_TYPE(LONG_PTR, 4, 4);
    TEST_TYPE_SIGNED(LONG_PTR);
}

static void test_pack_SIZE_T(void)
{
    /* SIZE_T */
    TEST_TYPE(SIZE_T, 4, 4);
}

static void test_pack_SSIZE_T(void)
{
    /* SSIZE_T */
    TEST_TYPE(SSIZE_T, 4, 4);
}

static void test_pack_UHALF_PTR(void)
{
    /* UHALF_PTR */
    TEST_TYPE(UHALF_PTR, 2, 2);
    TEST_TYPE_UNSIGNED(UHALF_PTR);
}

static void test_pack_UINT16(void)
{
    /* UINT16 */
    TEST_TYPE(UINT16, 2, 2);
    TEST_TYPE_UNSIGNED(UINT16);
}

static void test_pack_UINT32(void)
{
    /* UINT32 */
    TEST_TYPE(UINT32, 4, 4);
    TEST_TYPE_UNSIGNED(UINT32);
}

static void test_pack_UINT64(void)
{
    /* UINT64 */
    TEST_TYPE(UINT64, 8, 8);
    TEST_TYPE_UNSIGNED(UINT64);
}

static void test_pack_UINT8(void)
{
    /* UINT8 */
    TEST_TYPE(UINT8, 1, 1);
    TEST_TYPE_UNSIGNED(UINT8);
}

static void test_pack_UINT_PTR(void)
{
    /* UINT_PTR */
    TEST_TYPE(UINT_PTR, 4, 4);
    TEST_TYPE_UNSIGNED(UINT_PTR);
}

static void test_pack_ULONG32(void)
{
    /* ULONG32 */
    TEST_TYPE(ULONG32, 4, 4);
    TEST_TYPE_UNSIGNED(ULONG32);
}

static void test_pack_ULONG64(void)
{
    /* ULONG64 */
    TEST_TYPE(ULONG64, 8, 8);
    TEST_TYPE_UNSIGNED(ULONG64);
}

static void test_pack_ULONG_PTR(void)
{
    /* ULONG_PTR */
    TEST_TYPE(ULONG_PTR, 4, 4);
    TEST_TYPE_UNSIGNED(ULONG_PTR);
}

static void test_pack_ACCESS_ALLOWED_ACE(void)
{
    /* ACCESS_ALLOWED_ACE (pack 4) */
    TEST_TYPE(ACCESS_ALLOWED_ACE, 12, 4);
    TEST_FIELD(ACCESS_ALLOWED_ACE, ACE_HEADER, Header, 0, 4, 2);
    TEST_FIELD(ACCESS_ALLOWED_ACE, DWORD, Mask, 4, 4, 4);
    TEST_FIELD(ACCESS_ALLOWED_ACE, DWORD, SidStart, 8, 4, 4);
}

static void test_pack_ACCESS_DENIED_ACE(void)
{
    /* ACCESS_DENIED_ACE (pack 4) */
    TEST_TYPE(ACCESS_DENIED_ACE, 12, 4);
    TEST_FIELD(ACCESS_DENIED_ACE, ACE_HEADER, Header, 0, 4, 2);
    TEST_FIELD(ACCESS_DENIED_ACE, DWORD, Mask, 4, 4, 4);
    TEST_FIELD(ACCESS_DENIED_ACE, DWORD, SidStart, 8, 4, 4);
}

static void test_pack_ACCESS_MASK(void)
{
    /* ACCESS_MASK */
    TEST_TYPE(ACCESS_MASK, 4, 4);
    TEST_TYPE_UNSIGNED(ACCESS_MASK);
}

static void test_pack_ACE_HEADER(void)
{
    /* ACE_HEADER (pack 4) */
    TEST_TYPE(ACE_HEADER, 4, 2);
    TEST_FIELD(ACE_HEADER, BYTE, AceType, 0, 1, 1);
    TEST_FIELD(ACE_HEADER, BYTE, AceFlags, 1, 1, 1);
    TEST_FIELD(ACE_HEADER, WORD, AceSize, 2, 2, 2);
}

static void test_pack_ACL(void)
{
    /* ACL (pack 4) */
    TEST_TYPE(ACL, 8, 2);
    TEST_FIELD(ACL, BYTE, AclRevision, 0, 1, 1);
    TEST_FIELD(ACL, BYTE, Sbz1, 1, 1, 1);
    TEST_FIELD(ACL, WORD, AclSize, 2, 2, 2);
    TEST_FIELD(ACL, WORD, AceCount, 4, 2, 2);
    TEST_FIELD(ACL, WORD, Sbz2, 6, 2, 2);
}

static void test_pack_ACL_REVISION_INFORMATION(void)
{
    /* ACL_REVISION_INFORMATION (pack 4) */
    TEST_TYPE(ACL_REVISION_INFORMATION, 4, 4);
    TEST_FIELD(ACL_REVISION_INFORMATION, DWORD, AclRevision, 0, 4, 4);
}

static void test_pack_ACL_SIZE_INFORMATION(void)
{
    /* ACL_SIZE_INFORMATION (pack 4) */
    TEST_TYPE(ACL_SIZE_INFORMATION, 12, 4);
    TEST_FIELD(ACL_SIZE_INFORMATION, DWORD, AceCount, 0, 4, 4);
    TEST_FIELD(ACL_SIZE_INFORMATION, DWORD, AclBytesInUse, 4, 4, 4);
    TEST_FIELD(ACL_SIZE_INFORMATION, DWORD, AclBytesFree, 8, 4, 4);
}

static void test_pack_BOOLEAN(void)
{
    /* BOOLEAN */
    TEST_TYPE(BOOLEAN, 1, 1);
    TEST_TYPE_UNSIGNED(BOOLEAN);
}

static void test_pack_CCHAR(void)
{
    /* CCHAR */
    TEST_TYPE(CCHAR, 1, 1);
    TEST_TYPE_SIGNED(CCHAR);
}

static void test_pack_CHAR(void)
{
    /* CHAR */
    TEST_TYPE(CHAR, 1, 1);
    TEST_TYPE_SIGNED(CHAR);
}

static void test_pack_DWORDLONG(void)
{
    /* DWORDLONG */
    TEST_TYPE(DWORDLONG, 8, 8);
    TEST_TYPE_UNSIGNED(DWORDLONG);
}

static void test_pack_EXCEPTION_POINTERS(void)
{
    /* EXCEPTION_POINTERS (pack 4) */
    TEST_TYPE(EXCEPTION_POINTERS, 8, 4);
    TEST_FIELD(EXCEPTION_POINTERS, PEXCEPTION_RECORD, ExceptionRecord, 0, 4, 4);
    TEST_FIELD(EXCEPTION_POINTERS, PCONTEXT, ContextRecord, 4, 4, 4);
}

static void test_pack_EXCEPTION_RECORD(void)
{
    /* EXCEPTION_RECORD (pack 4) */
    TEST_TYPE(EXCEPTION_RECORD, 80, 4);
    TEST_FIELD(EXCEPTION_RECORD, DWORD, ExceptionCode, 0, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, DWORD, ExceptionFlags, 4, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, struct _EXCEPTION_RECORD *, ExceptionRecord, 8, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, PVOID, ExceptionAddress, 12, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, DWORD, NumberParameters, 16, 4, 4);
    TEST_FIELD(EXCEPTION_RECORD, ULONG_PTR[EXCEPTION_MAXIMUM_PARAMETERS], ExceptionInformation, 20, 60, 4);
}

static void test_pack_EXECUTION_STATE(void)
{
    /* EXECUTION_STATE */
    TEST_TYPE(EXECUTION_STATE, 4, 4);
    TEST_TYPE_UNSIGNED(EXECUTION_STATE);
}

static void test_pack_FLOATING_SAVE_AREA(void)
{
    /* FLOATING_SAVE_AREA (pack 4) */
    TEST_TYPE(FLOATING_SAVE_AREA, 112, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, ControlWord, 0, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, StatusWord, 4, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, TagWord, 8, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, ErrorOffset, 12, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, ErrorSelector, 16, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, DataOffset, 20, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, DataSelector, 24, 4, 4);
    TEST_FIELD(FLOATING_SAVE_AREA, BYTE[SIZE_OF_80387_REGISTERS], RegisterArea, 28, 80, 1);
    TEST_FIELD(FLOATING_SAVE_AREA, DWORD, Cr0NpxState, 108, 4, 4);
}

static void test_pack_FPO_DATA(void)
{
    /* FPO_DATA (pack 4) */
    TEST_TYPE(FPO_DATA, 16, 4);
    TEST_FIELD(FPO_DATA, DWORD, ulOffStart, 0, 4, 4);
    TEST_FIELD(FPO_DATA, DWORD, cbProcSize, 4, 4, 4);
    TEST_FIELD(FPO_DATA, DWORD, cdwLocals, 8, 4, 4);
    TEST_FIELD(FPO_DATA, WORD, cdwParams, 12, 2, 2);
}

static void test_pack_GENERIC_MAPPING(void)
{
    /* GENERIC_MAPPING (pack 4) */
    TEST_TYPE(GENERIC_MAPPING, 16, 4);
    TEST_FIELD(GENERIC_MAPPING, ACCESS_MASK, GenericRead, 0, 4, 4);
    TEST_FIELD(GENERIC_MAPPING, ACCESS_MASK, GenericWrite, 4, 4, 4);
    TEST_FIELD(GENERIC_MAPPING, ACCESS_MASK, GenericExecute, 8, 4, 4);
    TEST_FIELD(GENERIC_MAPPING, ACCESS_MASK, GenericAll, 12, 4, 4);
}

static void test_pack_HANDLE(void)
{
    /* HANDLE */
    TEST_TYPE(HANDLE, 4, 4);
}

static void test_pack_HRESULT(void)
{
    /* HRESULT */
    TEST_TYPE(HRESULT, 4, 4);
}

static void test_pack_IMAGE_ARCHIVE_MEMBER_HEADER(void)
{
    /* IMAGE_ARCHIVE_MEMBER_HEADER (pack 4) */
    TEST_TYPE(IMAGE_ARCHIVE_MEMBER_HEADER, 60, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[16], Name, 0, 16, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[12], Date, 16, 12, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[6], UserID, 28, 6, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[6], GroupID, 34, 6, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[8], Mode, 40, 8, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[10], Size, 48, 10, 1);
    TEST_FIELD(IMAGE_ARCHIVE_MEMBER_HEADER, BYTE[2], EndHeader, 58, 2, 1);
}

static void test_pack_IMAGE_AUX_SYMBOL(void)
{
    /* IMAGE_AUX_SYMBOL (pack 2) */
}

static void test_pack_IMAGE_BASE_RELOCATION(void)
{
    /* IMAGE_BASE_RELOCATION (pack 4) */
    TEST_TYPE(IMAGE_BASE_RELOCATION, 8, 4);
    TEST_FIELD(IMAGE_BASE_RELOCATION, DWORD, VirtualAddress, 0, 4, 4);
    TEST_FIELD(IMAGE_BASE_RELOCATION, DWORD, SizeOfBlock, 4, 4, 4);
}

static void test_pack_IMAGE_BOUND_FORWARDER_REF(void)
{
    /* IMAGE_BOUND_FORWARDER_REF (pack 4) */
    TEST_TYPE(IMAGE_BOUND_FORWARDER_REF, 8, 4);
    TEST_FIELD(IMAGE_BOUND_FORWARDER_REF, DWORD, TimeDateStamp, 0, 4, 4);
    TEST_FIELD(IMAGE_BOUND_FORWARDER_REF, WORD, OffsetModuleName, 4, 2, 2);
    TEST_FIELD(IMAGE_BOUND_FORWARDER_REF, WORD, Reserved, 6, 2, 2);
}

static void test_pack_IMAGE_BOUND_IMPORT_DESCRIPTOR(void)
{
    /* IMAGE_BOUND_IMPORT_DESCRIPTOR (pack 4) */
    TEST_TYPE(IMAGE_BOUND_IMPORT_DESCRIPTOR, 8, 4);
    TEST_FIELD(IMAGE_BOUND_IMPORT_DESCRIPTOR, DWORD, TimeDateStamp, 0, 4, 4);
    TEST_FIELD(IMAGE_BOUND_IMPORT_DESCRIPTOR, WORD, OffsetModuleName, 4, 2, 2);
    TEST_FIELD(IMAGE_BOUND_IMPORT_DESCRIPTOR, WORD, NumberOfModuleForwarderRefs, 6, 2, 2);
}

static void test_pack_IMAGE_COFF_SYMBOLS_HEADER(void)
{
    /* IMAGE_COFF_SYMBOLS_HEADER (pack 4) */
    TEST_TYPE(IMAGE_COFF_SYMBOLS_HEADER, 32, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, NumberOfSymbols, 0, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, LvaToFirstSymbol, 4, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, NumberOfLinenumbers, 8, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, LvaToFirstLinenumber, 12, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, RvaToFirstByteOfCode, 16, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, RvaToLastByteOfCode, 20, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, RvaToFirstByteOfData, 24, 4, 4);
    TEST_FIELD(IMAGE_COFF_SYMBOLS_HEADER, DWORD, RvaToLastByteOfData, 28, 4, 4);
}

static void test_pack_IMAGE_DATA_DIRECTORY(void)
{
    /* IMAGE_DATA_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_DATA_DIRECTORY, 8, 4);
    TEST_FIELD(IMAGE_DATA_DIRECTORY, DWORD, VirtualAddress, 0, 4, 4);
    TEST_FIELD(IMAGE_DATA_DIRECTORY, DWORD, Size, 4, 4, 4);
}

static void test_pack_IMAGE_DEBUG_DIRECTORY(void)
{
    /* IMAGE_DEBUG_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_DEBUG_DIRECTORY, 28, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, DWORD, Characteristics, 0, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, DWORD, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, WORD, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, WORD, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, DWORD, Type, 12, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, DWORD, SizeOfData, 16, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, DWORD, AddressOfRawData, 20, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_DIRECTORY, DWORD, PointerToRawData, 24, 4, 4);
}

static void test_pack_IMAGE_DEBUG_MISC(void)
{
    /* IMAGE_DEBUG_MISC (pack 4) */
    TEST_TYPE(IMAGE_DEBUG_MISC, 16, 4);
    TEST_FIELD(IMAGE_DEBUG_MISC, DWORD, DataType, 0, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_MISC, DWORD, Length, 4, 4, 4);
    TEST_FIELD(IMAGE_DEBUG_MISC, BYTE, Unicode, 8, 1, 1);
    TEST_FIELD(IMAGE_DEBUG_MISC, BYTE[ 3 ], Reserved, 9, 3, 1);
    TEST_FIELD(IMAGE_DEBUG_MISC, BYTE[ 1 ], Data, 12, 1, 1);
}

static void test_pack_IMAGE_DOS_HEADER(void)
{
    /* IMAGE_DOS_HEADER (pack 2) */
    TEST_TYPE(IMAGE_DOS_HEADER, 64, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_magic, 0, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_cblp, 2, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_cp, 4, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_crlc, 6, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_cparhdr, 8, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_minalloc, 10, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_maxalloc, 12, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_ss, 14, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_sp, 16, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_csum, 18, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_ip, 20, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_cs, 22, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_lfarlc, 24, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_ovno, 26, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD[4], e_res, 28, 8, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_oemid, 36, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD, e_oeminfo, 38, 2, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, WORD[10], e_res2, 40, 20, 2);
    TEST_FIELD(IMAGE_DOS_HEADER, DWORD, e_lfanew, 60, 4, 2);
}

static void test_pack_IMAGE_EXPORT_DIRECTORY(void)
{
    /* IMAGE_EXPORT_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_EXPORT_DIRECTORY, 40, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, Characteristics, 0, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, WORD, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, WORD, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, Name, 12, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, Base, 16, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, NumberOfFunctions, 20, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, NumberOfNames, 24, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, AddressOfFunctions, 28, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, AddressOfNames, 32, 4, 4);
    TEST_FIELD(IMAGE_EXPORT_DIRECTORY, DWORD, AddressOfNameOrdinals, 36, 4, 4);
}

static void test_pack_IMAGE_FILE_HEADER(void)
{
    /* IMAGE_FILE_HEADER (pack 4) */
    TEST_TYPE(IMAGE_FILE_HEADER, 20, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, WORD, Machine, 0, 2, 2);
    TEST_FIELD(IMAGE_FILE_HEADER, WORD, NumberOfSections, 2, 2, 2);
    TEST_FIELD(IMAGE_FILE_HEADER, DWORD, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, DWORD, PointerToSymbolTable, 8, 4, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, DWORD, NumberOfSymbols, 12, 4, 4);
    TEST_FIELD(IMAGE_FILE_HEADER, WORD, SizeOfOptionalHeader, 16, 2, 2);
    TEST_FIELD(IMAGE_FILE_HEADER, WORD, Characteristics, 18, 2, 2);
}

static void test_pack_IMAGE_FUNCTION_ENTRY(void)
{
    /* IMAGE_FUNCTION_ENTRY (pack 4) */
    TEST_TYPE(IMAGE_FUNCTION_ENTRY, 12, 4);
    TEST_FIELD(IMAGE_FUNCTION_ENTRY, DWORD, StartingAddress, 0, 4, 4);
    TEST_FIELD(IMAGE_FUNCTION_ENTRY, DWORD, EndingAddress, 4, 4, 4);
    TEST_FIELD(IMAGE_FUNCTION_ENTRY, DWORD, EndOfPrologue, 8, 4, 4);
}

static void test_pack_IMAGE_IMPORT_BY_NAME(void)
{
    /* IMAGE_IMPORT_BY_NAME (pack 4) */
    TEST_TYPE(IMAGE_IMPORT_BY_NAME, 4, 2);
    TEST_FIELD(IMAGE_IMPORT_BY_NAME, WORD, Hint, 0, 2, 2);
    TEST_FIELD(IMAGE_IMPORT_BY_NAME, BYTE[1], Name, 2, 1, 1);
}

static void test_pack_IMAGE_IMPORT_DESCRIPTOR(void)
{
    /* IMAGE_IMPORT_DESCRIPTOR (pack 4) */
}

static void test_pack_IMAGE_LINENUMBER(void)
{
    /* IMAGE_LINENUMBER (pack 2) */
}

static void test_pack_IMAGE_LOAD_CONFIG_DIRECTORY(void)
{
    /* IMAGE_LOAD_CONFIG_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_LOAD_CONFIG_DIRECTORY, 72, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, Size, 0, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, WORD, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, WORD, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, GlobalFlagsClear, 12, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, GlobalFlagsSet, 16, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, CriticalSectionDefaultTimeout, 20, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, DeCommitFreeBlockThreshold, 24, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, DeCommitTotalFreeThreshold, 28, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, PVOID, LockPrefixTable, 32, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, MaximumAllocationSize, 36, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, VirtualMemoryThreshold, 40, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, ProcessHeapFlags, 44, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, ProcessAffinityMask, 48, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, WORD, CSDVersion, 52, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, WORD, Reserved1, 54, 2, 2);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, PVOID, EditList, 56, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, SecurityCookie, 60, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, SEHandlerTable, 64, 4, 4);
    TEST_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, DWORD, SEHandlerCount, 68, 4, 4);
}

static void test_pack_IMAGE_NT_HEADERS(void)
{
    /* IMAGE_NT_HEADERS (pack 4) */
    TEST_TYPE(IMAGE_NT_HEADERS, 248, 4);
    TEST_FIELD(IMAGE_NT_HEADERS, DWORD, Signature, 0, 4, 4);
    TEST_FIELD(IMAGE_NT_HEADERS, IMAGE_FILE_HEADER, FileHeader, 4, 20, 4);
    TEST_FIELD(IMAGE_NT_HEADERS, IMAGE_OPTIONAL_HEADER, OptionalHeader, 24, 224, 4);
}

static void test_pack_IMAGE_OPTIONAL_HEADER(void)
{
    /* IMAGE_OPTIONAL_HEADER (pack 4) */
    TEST_TYPE(IMAGE_OPTIONAL_HEADER, 224, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, Magic, 0, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, BYTE, MajorLinkerVersion, 2, 1, 1);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, BYTE, MinorLinkerVersion, 3, 1, 1);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfCode, 4, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfInitializedData, 8, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfUninitializedData, 12, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, AddressOfEntryPoint, 16, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, BaseOfCode, 20, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, BaseOfData, 24, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, ImageBase, 28, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SectionAlignment, 32, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, FileAlignment, 36, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, MajorOperatingSystemVersion, 40, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, MinorOperatingSystemVersion, 42, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, MajorImageVersion, 44, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, MinorImageVersion, 46, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, MajorSubsystemVersion, 48, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, MinorSubsystemVersion, 50, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, Win32VersionValue, 52, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfImage, 56, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfHeaders, 60, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, CheckSum, 64, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, Subsystem, 68, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, WORD, DllCharacteristics, 70, 2, 2);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfStackReserve, 72, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfStackCommit, 76, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfHeapReserve, 80, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, SizeOfHeapCommit, 84, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, LoaderFlags, 88, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, DWORD, NumberOfRvaAndSizes, 92, 4, 4);
    TEST_FIELD(IMAGE_OPTIONAL_HEADER, IMAGE_DATA_DIRECTORY[IMAGE_NUMBEROF_DIRECTORY_ENTRIES], DataDirectory, 96, 128, 4);
}

static void test_pack_IMAGE_OS2_HEADER(void)
{
    /* IMAGE_OS2_HEADER (pack 2) */
    TEST_TYPE(IMAGE_OS2_HEADER, 64, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_magic, 0, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, BYTE, ne_ver, 2, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, BYTE, ne_rev, 3, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_enttab, 4, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_cbenttab, 6, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, LONG, ne_crc, 8, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_flags, 12, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_autodata, 14, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_heap, 16, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_stack, 18, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, DWORD, ne_csip, 20, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, DWORD, ne_sssp, 24, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_cseg, 28, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_cmod, 30, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_cbnrestab, 32, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_segtab, 34, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_rsrctab, 36, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_restab, 38, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_modtab, 40, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_imptab, 42, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, DWORD, ne_nrestab, 44, 4, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_cmovent, 48, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_align, 50, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_cres, 52, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, BYTE, ne_exetyp, 54, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, BYTE, ne_flagsothers, 55, 1, 1);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_pretthunks, 56, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_psegrefbytes, 58, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_swaparea, 60, 2, 2);
    TEST_FIELD(IMAGE_OS2_HEADER, WORD, ne_expver, 62, 2, 2);
}

static void test_pack_IMAGE_RELOCATION(void)
{
    /* IMAGE_RELOCATION (pack 2) */
}

static void test_pack_IMAGE_RESOURCE_DATA_ENTRY(void)
{
    /* IMAGE_RESOURCE_DATA_ENTRY (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DATA_ENTRY, 16, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, DWORD, OffsetToData, 0, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, DWORD, Size, 4, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, DWORD, CodePage, 8, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DATA_ENTRY, DWORD, Reserved, 12, 4, 4);
}

static void test_pack_IMAGE_RESOURCE_DIRECTORY(void)
{
    /* IMAGE_RESOURCE_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DIRECTORY, 16, 4);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, DWORD, Characteristics, 0, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, DWORD, TimeDateStamp, 4, 4, 4);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, WORD, MajorVersion, 8, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, WORD, MinorVersion, 10, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, WORD, NumberOfNamedEntries, 12, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY, WORD, NumberOfIdEntries, 14, 2, 2);
}

static void test_pack_IMAGE_RESOURCE_DIRECTORY_ENTRY(void)
{
    /* IMAGE_RESOURCE_DIRECTORY_ENTRY (pack 4) */
}

static void test_pack_IMAGE_RESOURCE_DIRECTORY_STRING(void)
{
    /* IMAGE_RESOURCE_DIRECTORY_STRING (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DIRECTORY_STRING, 4, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY_STRING, WORD, Length, 0, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIRECTORY_STRING, CHAR[ 1 ], NameString, 2, 1, 1);
}

static void test_pack_IMAGE_RESOURCE_DIR_STRING_U(void)
{
    /* IMAGE_RESOURCE_DIR_STRING_U (pack 4) */
    TEST_TYPE(IMAGE_RESOURCE_DIR_STRING_U, 4, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIR_STRING_U, WORD, Length, 0, 2, 2);
    TEST_FIELD(IMAGE_RESOURCE_DIR_STRING_U, WCHAR[ 1 ], NameString, 2, 2, 2);
}

static void test_pack_IMAGE_SECTION_HEADER(void)
{
    /* IMAGE_SECTION_HEADER (pack 4) */
    TEST_FIELD(IMAGE_SECTION_HEADER, BYTE[IMAGE_SIZEOF_SHORT_NAME], Name, 0, 8, 1);
}

static void test_pack_IMAGE_SEPARATE_DEBUG_HEADER(void)
{
    /* IMAGE_SEPARATE_DEBUG_HEADER (pack 4) */
    TEST_TYPE(IMAGE_SEPARATE_DEBUG_HEADER, 48, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, WORD, Signature, 0, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, WORD, Flags, 2, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, WORD, Machine, 4, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, WORD, Characteristics, 6, 2, 2);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, TimeDateStamp, 8, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, CheckSum, 12, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, ImageBase, 16, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, SizeOfImage, 20, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, NumberOfSections, 24, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, ExportedNamesSize, 28, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, DebugDirectorySize, 32, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD, SectionAlignment, 36, 4, 4);
    TEST_FIELD(IMAGE_SEPARATE_DEBUG_HEADER, DWORD[ 2 ], Reserved, 40, 8, 4);
}

static void test_pack_IMAGE_SYMBOL(void)
{
    /* IMAGE_SYMBOL (pack 2) */
}

static void test_pack_IMAGE_THUNK_DATA(void)
{
    /* IMAGE_THUNK_DATA (pack 4) */
}

static void test_pack_IMAGE_TLS_DIRECTORY(void)
{
    /* IMAGE_TLS_DIRECTORY (pack 4) */
    TEST_TYPE(IMAGE_TLS_DIRECTORY, 24, 4);
    TEST_FIELD(IMAGE_TLS_DIRECTORY, DWORD, StartAddressOfRawData, 0, 4, 4);
    TEST_FIELD(IMAGE_TLS_DIRECTORY, DWORD, EndAddressOfRawData, 4, 4, 4);
    TEST_FIELD(IMAGE_TLS_DIRECTORY, LPDWORD, AddressOfIndex, 8, 4, 4);
    TEST_FIELD(IMAGE_TLS_DIRECTORY, PIMAGE_TLS_CALLBACK *, AddressOfCallBacks, 12, 4, 4);
    TEST_FIELD(IMAGE_TLS_DIRECTORY, DWORD, SizeOfZeroFill, 16, 4, 4);
    TEST_FIELD(IMAGE_TLS_DIRECTORY, DWORD, Characteristics, 20, 4, 4);
}

static void test_pack_IMAGE_VXD_HEADER(void)
{
    /* IMAGE_VXD_HEADER (pack 2) */
    TEST_TYPE(IMAGE_VXD_HEADER, 196, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, WORD, e32_magic, 0, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, BYTE, e32_border, 2, 1, 1);
    TEST_FIELD(IMAGE_VXD_HEADER, BYTE, e32_worder, 3, 1, 1);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_level, 4, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, WORD, e32_cpu, 8, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, WORD, e32_os, 10, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_ver, 12, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_mflags, 16, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_mpages, 20, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_startobj, 24, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_eip, 28, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_stackobj, 32, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_esp, 36, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_pagesize, 40, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_lastpagesize, 44, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_fixupsize, 48, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_fixupsum, 52, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_ldrsize, 56, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_ldrsum, 60, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_objtab, 64, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_objcnt, 68, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_objmap, 72, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_itermap, 76, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_rsrctab, 80, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_rsrccnt, 84, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_restab, 88, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_enttab, 92, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_dirtab, 96, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_dircnt, 100, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_fpagetab, 104, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_frectab, 108, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_impmod, 112, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_impmodcnt, 116, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_impproc, 120, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_pagesum, 124, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_datapage, 128, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_preload, 132, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_nrestab, 136, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_cbnrestab, 140, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_nressum, 144, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_autodata, 148, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_debuginfo, 152, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_debuglen, 156, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_instpreload, 160, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_instdemand, 164, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_heapsize, 168, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, BYTE[12], e32_res3, 172, 12, 1);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_winresoff, 184, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, DWORD, e32_winreslen, 188, 4, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, WORD, e32_devid, 192, 2, 2);
    TEST_FIELD(IMAGE_VXD_HEADER, WORD, e32_ddkver, 194, 2, 2);
}

static void test_pack_IO_COUNTERS(void)
{
    /* IO_COUNTERS (pack 8) */
    TEST_TYPE(IO_COUNTERS, 48, 8);
    TEST_FIELD(IO_COUNTERS, ULONGLONG, ReadOperationCount, 0, 8, 8);
    TEST_FIELD(IO_COUNTERS, ULONGLONG, WriteOperationCount, 8, 8, 8);
    TEST_FIELD(IO_COUNTERS, ULONGLONG, OtherOperationCount, 16, 8, 8);
    TEST_FIELD(IO_COUNTERS, ULONGLONG, ReadTransferCount, 24, 8, 8);
    TEST_FIELD(IO_COUNTERS, ULONGLONG, WriteTransferCount, 32, 8, 8);
    TEST_FIELD(IO_COUNTERS, ULONGLONG, OtherTransferCount, 40, 8, 8);
}

static void test_pack_LANGID(void)
{
    /* LANGID */
    TEST_TYPE(LANGID, 2, 2);
    TEST_TYPE_UNSIGNED(LANGID);
}

static void test_pack_LARGE_INTEGER(void)
{
    /* LARGE_INTEGER (pack 4) */
}

static void test_pack_LCID(void)
{
    /* LCID */
    TEST_TYPE(LCID, 4, 4);
    TEST_TYPE_UNSIGNED(LCID);
}

static void test_pack_LIST_ENTRY(void)
{
    /* LIST_ENTRY (pack 4) */
    TEST_TYPE(LIST_ENTRY, 8, 4);
    TEST_FIELD(LIST_ENTRY, struct _LIST_ENTRY *, Flink, 0, 4, 4);
    TEST_FIELD(LIST_ENTRY, struct _LIST_ENTRY *, Blink, 4, 4, 4);
}

static void test_pack_LONG(void)
{
    /* LONG */
    TEST_TYPE(LONG, 4, 4);
    TEST_TYPE_SIGNED(LONG);
}

static void test_pack_LONGLONG(void)
{
    /* LONGLONG */
    TEST_TYPE(LONGLONG, 8, 8);
    TEST_TYPE_SIGNED(LONGLONG);
}

static void test_pack_LPTOP_LEVEL_EXCEPTION_FILTER(void)
{
    /* LPTOP_LEVEL_EXCEPTION_FILTER */
    TEST_TYPE(LPTOP_LEVEL_EXCEPTION_FILTER, 4, 4);
}

static void test_pack_LUID(void)
{
    /* LUID (pack 4) */
    TEST_TYPE(LUID, 8, 4);
    TEST_FIELD(LUID, DWORD, LowPart, 0, 4, 4);
    TEST_FIELD(LUID, LONG, HighPart, 4, 4, 4);
}

static void test_pack_LUID_AND_ATTRIBUTES(void)
{
    /* LUID_AND_ATTRIBUTES (pack 4) */
    TEST_TYPE(LUID_AND_ATTRIBUTES, 12, 4);
    TEST_FIELD(LUID_AND_ATTRIBUTES, LUID, Luid, 0, 8, 4);
    TEST_FIELD(LUID_AND_ATTRIBUTES, DWORD, Attributes, 8, 4, 4);
}

static void test_pack_MEMORY_BASIC_INFORMATION(void)
{
    /* MEMORY_BASIC_INFORMATION (pack 4) */
    TEST_TYPE(MEMORY_BASIC_INFORMATION, 28, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, LPVOID, BaseAddress, 0, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, LPVOID, AllocationBase, 4, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, DWORD, AllocationProtect, 8, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, DWORD, RegionSize, 12, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, DWORD, State, 16, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, DWORD, Protect, 20, 4, 4);
    TEST_FIELD(MEMORY_BASIC_INFORMATION, DWORD, Type, 24, 4, 4);
}

static void test_pack_MESSAGE_RESOURCE_BLOCK(void)
{
    /* MESSAGE_RESOURCE_BLOCK (pack 4) */
    TEST_TYPE(MESSAGE_RESOURCE_BLOCK, 12, 4);
    TEST_FIELD(MESSAGE_RESOURCE_BLOCK, DWORD, LowId, 0, 4, 4);
    TEST_FIELD(MESSAGE_RESOURCE_BLOCK, DWORD, HighId, 4, 4, 4);
    TEST_FIELD(MESSAGE_RESOURCE_BLOCK, DWORD, OffsetToEntries, 8, 4, 4);
}

static void test_pack_MESSAGE_RESOURCE_DATA(void)
{
    /* MESSAGE_RESOURCE_DATA (pack 4) */
    TEST_TYPE(MESSAGE_RESOURCE_DATA, 16, 4);
    TEST_FIELD(MESSAGE_RESOURCE_DATA, DWORD, NumberOfBlocks, 0, 4, 4);
    TEST_FIELD(MESSAGE_RESOURCE_DATA, MESSAGE_RESOURCE_BLOCK[ 1 ], Blocks, 4, 12, 4);
}

static void test_pack_MESSAGE_RESOURCE_ENTRY(void)
{
    /* MESSAGE_RESOURCE_ENTRY (pack 4) */
    TEST_TYPE(MESSAGE_RESOURCE_ENTRY, 6, 2);
    TEST_FIELD(MESSAGE_RESOURCE_ENTRY, WORD, Length, 0, 2, 2);
    TEST_FIELD(MESSAGE_RESOURCE_ENTRY, WORD, Flags, 2, 2, 2);
    TEST_FIELD(MESSAGE_RESOURCE_ENTRY, BYTE[1], Text, 4, 1, 1);
}

static void test_pack_NT_TIB(void)
{
    /* NT_TIB (pack 4) */
    TEST_FIELD(NT_TIB, struct _EXCEPTION_REGISTRATION_RECORD *, ExceptionList, 0, 4, 4);
    TEST_FIELD(NT_TIB, PVOID, StackBase, 4, 4, 4);
    TEST_FIELD(NT_TIB, PVOID, StackLimit, 8, 4, 4);
    TEST_FIELD(NT_TIB, PVOID, SubSystemTib, 12, 4, 4);
}

static void test_pack_OBJECT_TYPE_LIST(void)
{
    /* OBJECT_TYPE_LIST (pack 4) */
    TEST_TYPE(OBJECT_TYPE_LIST, 8, 4);
    TEST_FIELD(OBJECT_TYPE_LIST, WORD, Level, 0, 2, 2);
    TEST_FIELD(OBJECT_TYPE_LIST, WORD, Sbz, 2, 2, 2);
    TEST_FIELD(OBJECT_TYPE_LIST, GUID *, ObjectType, 4, 4, 4);
}

static void test_pack_PACCESS_ALLOWED_ACE(void)
{
    /* PACCESS_ALLOWED_ACE */
    TEST_TYPE(PACCESS_ALLOWED_ACE, 4, 4);
    TEST_TYPE_POINTER(PACCESS_ALLOWED_ACE, 12, 4);
}

static void test_pack_PACCESS_DENIED_ACE(void)
{
    /* PACCESS_DENIED_ACE */
    TEST_TYPE(PACCESS_DENIED_ACE, 4, 4);
    TEST_TYPE_POINTER(PACCESS_DENIED_ACE, 12, 4);
}

static void test_pack_PACCESS_TOKEN(void)
{
    /* PACCESS_TOKEN */
    TEST_TYPE(PACCESS_TOKEN, 4, 4);
}

static void test_pack_PACE_HEADER(void)
{
    /* PACE_HEADER */
    TEST_TYPE(PACE_HEADER, 4, 4);
    TEST_TYPE_POINTER(PACE_HEADER, 4, 2);
}

static void test_pack_PACL(void)
{
    /* PACL */
    TEST_TYPE(PACL, 4, 4);
    TEST_TYPE_POINTER(PACL, 8, 2);
}

static void test_pack_PACL_REVISION_INFORMATION(void)
{
    /* PACL_REVISION_INFORMATION */
    TEST_TYPE(PACL_REVISION_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PACL_REVISION_INFORMATION, 4, 4);
}

static void test_pack_PACL_SIZE_INFORMATION(void)
{
    /* PACL_SIZE_INFORMATION */
    TEST_TYPE(PACL_SIZE_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PACL_SIZE_INFORMATION, 12, 4);
}

static void test_pack_PCCH(void)
{
    /* PCCH */
    TEST_TYPE(PCCH, 4, 4);
    TEST_TYPE_POINTER(PCCH, 1, 1);
}

static void test_pack_PCH(void)
{
    /* PCH */
    TEST_TYPE(PCH, 4, 4);
    TEST_TYPE_POINTER(PCH, 1, 1);
}

static void test_pack_PCSTR(void)
{
    /* PCSTR */
    TEST_TYPE(PCSTR, 4, 4);
    TEST_TYPE_POINTER(PCSTR, 1, 1);
}

static void test_pack_PCTSTR(void)
{
    /* PCTSTR */
    TEST_TYPE(PCTSTR, 4, 4);
}

static void test_pack_PCWCH(void)
{
    /* PCWCH */
    TEST_TYPE(PCWCH, 4, 4);
    TEST_TYPE_POINTER(PCWCH, 2, 2);
}

static void test_pack_PCWSTR(void)
{
    /* PCWSTR */
    TEST_TYPE(PCWSTR, 4, 4);
    TEST_TYPE_POINTER(PCWSTR, 2, 2);
}

static void test_pack_PEXCEPTION_POINTERS(void)
{
    /* PEXCEPTION_POINTERS */
    TEST_TYPE(PEXCEPTION_POINTERS, 4, 4);
    TEST_TYPE_POINTER(PEXCEPTION_POINTERS, 8, 4);
}

static void test_pack_PEXCEPTION_RECORD(void)
{
    /* PEXCEPTION_RECORD */
    TEST_TYPE(PEXCEPTION_RECORD, 4, 4);
    TEST_TYPE_POINTER(PEXCEPTION_RECORD, 80, 4);
}

static void test_pack_PFLOATING_SAVE_AREA(void)
{
    /* PFLOATING_SAVE_AREA */
    TEST_TYPE(PFLOATING_SAVE_AREA, 4, 4);
    TEST_TYPE_POINTER(PFLOATING_SAVE_AREA, 112, 4);
}

static void test_pack_PFPO_DATA(void)
{
    /* PFPO_DATA */
    TEST_TYPE(PFPO_DATA, 4, 4);
    TEST_TYPE_POINTER(PFPO_DATA, 16, 4);
}

static void test_pack_PGENERIC_MAPPING(void)
{
    /* PGENERIC_MAPPING */
    TEST_TYPE(PGENERIC_MAPPING, 4, 4);
    TEST_TYPE_POINTER(PGENERIC_MAPPING, 16, 4);
}

static void test_pack_PHANDLE(void)
{
    /* PHANDLE */
    TEST_TYPE(PHANDLE, 4, 4);
    TEST_TYPE_POINTER(PHANDLE, 4, 4);
}

static void test_pack_PIMAGE_ARCHIVE_MEMBER_HEADER(void)
{
    /* PIMAGE_ARCHIVE_MEMBER_HEADER */
    TEST_TYPE(PIMAGE_ARCHIVE_MEMBER_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_ARCHIVE_MEMBER_HEADER, 60, 1);
}

static void test_pack_PIMAGE_AUX_SYMBOL(void)
{
    /* PIMAGE_AUX_SYMBOL */
    TEST_TYPE(PIMAGE_AUX_SYMBOL, 4, 4);
}

static void test_pack_PIMAGE_BASE_RELOCATION(void)
{
    /* PIMAGE_BASE_RELOCATION */
    TEST_TYPE(PIMAGE_BASE_RELOCATION, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_BASE_RELOCATION, 8, 4);
}

static void test_pack_PIMAGE_BOUND_FORWARDER_REF(void)
{
    /* PIMAGE_BOUND_FORWARDER_REF */
    TEST_TYPE(PIMAGE_BOUND_FORWARDER_REF, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_BOUND_FORWARDER_REF, 8, 4);
}

static void test_pack_PIMAGE_BOUND_IMPORT_DESCRIPTOR(void)
{
    /* PIMAGE_BOUND_IMPORT_DESCRIPTOR */
    TEST_TYPE(PIMAGE_BOUND_IMPORT_DESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_BOUND_IMPORT_DESCRIPTOR, 8, 4);
}

static void test_pack_PIMAGE_COFF_SYMBOLS_HEADER(void)
{
    /* PIMAGE_COFF_SYMBOLS_HEADER */
    TEST_TYPE(PIMAGE_COFF_SYMBOLS_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_COFF_SYMBOLS_HEADER, 32, 4);
}

static void test_pack_PIMAGE_DATA_DIRECTORY(void)
{
    /* PIMAGE_DATA_DIRECTORY */
    TEST_TYPE(PIMAGE_DATA_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DATA_DIRECTORY, 8, 4);
}

static void test_pack_PIMAGE_DEBUG_DIRECTORY(void)
{
    /* PIMAGE_DEBUG_DIRECTORY */
    TEST_TYPE(PIMAGE_DEBUG_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DEBUG_DIRECTORY, 28, 4);
}

static void test_pack_PIMAGE_DEBUG_MISC(void)
{
    /* PIMAGE_DEBUG_MISC */
    TEST_TYPE(PIMAGE_DEBUG_MISC, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DEBUG_MISC, 16, 4);
}

static void test_pack_PIMAGE_DOS_HEADER(void)
{
    /* PIMAGE_DOS_HEADER */
    TEST_TYPE(PIMAGE_DOS_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_DOS_HEADER, 64, 2);
}

static void test_pack_PIMAGE_EXPORT_DIRECTORY(void)
{
    /* PIMAGE_EXPORT_DIRECTORY */
    TEST_TYPE(PIMAGE_EXPORT_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_EXPORT_DIRECTORY, 40, 4);
}

static void test_pack_PIMAGE_FILE_HEADER(void)
{
    /* PIMAGE_FILE_HEADER */
    TEST_TYPE(PIMAGE_FILE_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_FILE_HEADER, 20, 4);
}

static void test_pack_PIMAGE_FUNCTION_ENTRY(void)
{
    /* PIMAGE_FUNCTION_ENTRY */
    TEST_TYPE(PIMAGE_FUNCTION_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_FUNCTION_ENTRY, 12, 4);
}

static void test_pack_PIMAGE_IMPORT_BY_NAME(void)
{
    /* PIMAGE_IMPORT_BY_NAME */
    TEST_TYPE(PIMAGE_IMPORT_BY_NAME, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_IMPORT_BY_NAME, 4, 2);
}

static void test_pack_PIMAGE_IMPORT_DESCRIPTOR(void)
{
    /* PIMAGE_IMPORT_DESCRIPTOR */
    TEST_TYPE(PIMAGE_IMPORT_DESCRIPTOR, 4, 4);
}

static void test_pack_PIMAGE_LINENUMBER(void)
{
    /* PIMAGE_LINENUMBER */
    TEST_TYPE(PIMAGE_LINENUMBER, 4, 4);
}

static void test_pack_PIMAGE_LOAD_CONFIG_DIRECTORY(void)
{
    /* PIMAGE_LOAD_CONFIG_DIRECTORY */
    TEST_TYPE(PIMAGE_LOAD_CONFIG_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_LOAD_CONFIG_DIRECTORY, 72, 4);
}

static void test_pack_PIMAGE_NT_HEADERS(void)
{
    /* PIMAGE_NT_HEADERS */
    TEST_TYPE(PIMAGE_NT_HEADERS, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_NT_HEADERS, 248, 4);
}

static void test_pack_PIMAGE_OPTIONAL_HEADER(void)
{
    /* PIMAGE_OPTIONAL_HEADER */
    TEST_TYPE(PIMAGE_OPTIONAL_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_OPTIONAL_HEADER, 224, 4);
}

static void test_pack_PIMAGE_OS2_HEADER(void)
{
    /* PIMAGE_OS2_HEADER */
    TEST_TYPE(PIMAGE_OS2_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_OS2_HEADER, 64, 2);
}

static void test_pack_PIMAGE_RELOCATION(void)
{
    /* PIMAGE_RELOCATION */
    TEST_TYPE(PIMAGE_RELOCATION, 4, 4);
}

static void test_pack_PIMAGE_RESOURCE_DATA_ENTRY(void)
{
    /* PIMAGE_RESOURCE_DATA_ENTRY */
    TEST_TYPE(PIMAGE_RESOURCE_DATA_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DATA_ENTRY, 16, 4);
}

static void test_pack_PIMAGE_RESOURCE_DIRECTORY(void)
{
    /* PIMAGE_RESOURCE_DIRECTORY */
    TEST_TYPE(PIMAGE_RESOURCE_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DIRECTORY, 16, 4);
}

static void test_pack_PIMAGE_RESOURCE_DIRECTORY_ENTRY(void)
{
    /* PIMAGE_RESOURCE_DIRECTORY_ENTRY */
    TEST_TYPE(PIMAGE_RESOURCE_DIRECTORY_ENTRY, 4, 4);
}

static void test_pack_PIMAGE_RESOURCE_DIRECTORY_STRING(void)
{
    /* PIMAGE_RESOURCE_DIRECTORY_STRING */
    TEST_TYPE(PIMAGE_RESOURCE_DIRECTORY_STRING, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DIRECTORY_STRING, 4, 2);
}

static void test_pack_PIMAGE_RESOURCE_DIR_STRING_U(void)
{
    /* PIMAGE_RESOURCE_DIR_STRING_U */
    TEST_TYPE(PIMAGE_RESOURCE_DIR_STRING_U, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_RESOURCE_DIR_STRING_U, 4, 2);
}

static void test_pack_PIMAGE_SECTION_HEADER(void)
{
    /* PIMAGE_SECTION_HEADER */
    TEST_TYPE(PIMAGE_SECTION_HEADER, 4, 4);
}

static void test_pack_PIMAGE_SEPARATE_DEBUG_HEADER(void)
{
    /* PIMAGE_SEPARATE_DEBUG_HEADER */
    TEST_TYPE(PIMAGE_SEPARATE_DEBUG_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_SEPARATE_DEBUG_HEADER, 48, 4);
}

static void test_pack_PIMAGE_SYMBOL(void)
{
    /* PIMAGE_SYMBOL */
    TEST_TYPE(PIMAGE_SYMBOL, 4, 4);
}

static void test_pack_PIMAGE_THUNK_DATA(void)
{
    /* PIMAGE_THUNK_DATA */
    TEST_TYPE(PIMAGE_THUNK_DATA, 4, 4);
}

static void test_pack_PIMAGE_TLS_CALLBACK(void)
{
    /* PIMAGE_TLS_CALLBACK */
    TEST_TYPE(PIMAGE_TLS_CALLBACK, 4, 4);
}

static void test_pack_PIMAGE_TLS_DIRECTORY(void)
{
    /* PIMAGE_TLS_DIRECTORY */
    TEST_TYPE(PIMAGE_TLS_DIRECTORY, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_TLS_DIRECTORY, 24, 4);
}

static void test_pack_PIMAGE_VXD_HEADER(void)
{
    /* PIMAGE_VXD_HEADER */
    TEST_TYPE(PIMAGE_VXD_HEADER, 4, 4);
    TEST_TYPE_POINTER(PIMAGE_VXD_HEADER, 196, 2);
}

static void test_pack_PIO_COUNTERS(void)
{
    /* PIO_COUNTERS */
    TEST_TYPE(PIO_COUNTERS, 4, 4);
    TEST_TYPE_POINTER(PIO_COUNTERS, 48, 8);
}

static void test_pack_PISECURITY_DESCRIPTOR(void)
{
    /* PISECURITY_DESCRIPTOR */
    TEST_TYPE(PISECURITY_DESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PISECURITY_DESCRIPTOR, 20, 4);
}

static void test_pack_PISECURITY_DESCRIPTOR_RELATIVE(void)
{
    /* PISECURITY_DESCRIPTOR_RELATIVE */
    TEST_TYPE(PISECURITY_DESCRIPTOR_RELATIVE, 4, 4);
    TEST_TYPE_POINTER(PISECURITY_DESCRIPTOR_RELATIVE, 20, 4);
}

static void test_pack_PISID(void)
{
    /* PISID */
    TEST_TYPE(PISID, 4, 4);
    TEST_TYPE_POINTER(PISID, 12, 4);
}

static void test_pack_PLARGE_INTEGER(void)
{
    /* PLARGE_INTEGER */
    TEST_TYPE(PLARGE_INTEGER, 4, 4);
}

static void test_pack_PLIST_ENTRY(void)
{
    /* PLIST_ENTRY */
    TEST_TYPE(PLIST_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PLIST_ENTRY, 8, 4);
}

static void test_pack_PLUID(void)
{
    /* PLUID */
    TEST_TYPE(PLUID, 4, 4);
    TEST_TYPE_POINTER(PLUID, 8, 4);
}

static void test_pack_PLUID_AND_ATTRIBUTES(void)
{
    /* PLUID_AND_ATTRIBUTES */
    TEST_TYPE(PLUID_AND_ATTRIBUTES, 4, 4);
    TEST_TYPE_POINTER(PLUID_AND_ATTRIBUTES, 12, 4);
}

static void test_pack_PMEMORY_BASIC_INFORMATION(void)
{
    /* PMEMORY_BASIC_INFORMATION */
    TEST_TYPE(PMEMORY_BASIC_INFORMATION, 4, 4);
    TEST_TYPE_POINTER(PMEMORY_BASIC_INFORMATION, 28, 4);
}

static void test_pack_PMESSAGE_RESOURCE_BLOCK(void)
{
    /* PMESSAGE_RESOURCE_BLOCK */
    TEST_TYPE(PMESSAGE_RESOURCE_BLOCK, 4, 4);
    TEST_TYPE_POINTER(PMESSAGE_RESOURCE_BLOCK, 12, 4);
}

static void test_pack_PMESSAGE_RESOURCE_DATA(void)
{
    /* PMESSAGE_RESOURCE_DATA */
    TEST_TYPE(PMESSAGE_RESOURCE_DATA, 4, 4);
    TEST_TYPE_POINTER(PMESSAGE_RESOURCE_DATA, 16, 4);
}

static void test_pack_PMESSAGE_RESOURCE_ENTRY(void)
{
    /* PMESSAGE_RESOURCE_ENTRY */
    TEST_TYPE(PMESSAGE_RESOURCE_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PMESSAGE_RESOURCE_ENTRY, 6, 2);
}

static void test_pack_PNT_TIB(void)
{
    /* PNT_TIB */
    TEST_TYPE(PNT_TIB, 4, 4);
}

static void test_pack_POBJECT_TYPE_LIST(void)
{
    /* POBJECT_TYPE_LIST */
    TEST_TYPE(POBJECT_TYPE_LIST, 4, 4);
    TEST_TYPE_POINTER(POBJECT_TYPE_LIST, 8, 4);
}

static void test_pack_PPRIVILEGE_SET(void)
{
    /* PPRIVILEGE_SET */
    TEST_TYPE(PPRIVILEGE_SET, 4, 4);
    TEST_TYPE_POINTER(PPRIVILEGE_SET, 20, 4);
}

static void test_pack_PRIVILEGE_SET(void)
{
    /* PRIVILEGE_SET (pack 4) */
    TEST_TYPE(PRIVILEGE_SET, 20, 4);
    TEST_FIELD(PRIVILEGE_SET, DWORD, PrivilegeCount, 0, 4, 4);
    TEST_FIELD(PRIVILEGE_SET, DWORD, Control, 4, 4, 4);
    TEST_FIELD(PRIVILEGE_SET, LUID_AND_ATTRIBUTES[ANYSIZE_ARRAY], Privilege, 8, 12, 4);
}

static void test_pack_PRLIST_ENTRY(void)
{
    /* PRLIST_ENTRY */
    TEST_TYPE(PRLIST_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PRLIST_ENTRY, 8, 4);
}

static void test_pack_PRTL_CRITICAL_SECTION(void)
{
    /* PRTL_CRITICAL_SECTION */
    TEST_TYPE(PRTL_CRITICAL_SECTION, 4, 4);
    TEST_TYPE_POINTER(PRTL_CRITICAL_SECTION, 24, 4);
}

static void test_pack_PRTL_CRITICAL_SECTION_DEBUG(void)
{
    /* PRTL_CRITICAL_SECTION_DEBUG */
    TEST_TYPE(PRTL_CRITICAL_SECTION_DEBUG, 4, 4);
    TEST_TYPE_POINTER(PRTL_CRITICAL_SECTION_DEBUG, 32, 4);
}

static void test_pack_PRTL_OSVERSIONINFOEXW(void)
{
    /* PRTL_OSVERSIONINFOEXW */
    TEST_TYPE(PRTL_OSVERSIONINFOEXW, 4, 4);
    TEST_TYPE_POINTER(PRTL_OSVERSIONINFOEXW, 284, 4);
}

static void test_pack_PRTL_OSVERSIONINFOW(void)
{
    /* PRTL_OSVERSIONINFOW */
    TEST_TYPE(PRTL_OSVERSIONINFOW, 4, 4);
    TEST_TYPE_POINTER(PRTL_OSVERSIONINFOW, 276, 4);
}

static void test_pack_PRTL_RESOURCE_DEBUG(void)
{
    /* PRTL_RESOURCE_DEBUG */
    TEST_TYPE(PRTL_RESOURCE_DEBUG, 4, 4);
    TEST_TYPE_POINTER(PRTL_RESOURCE_DEBUG, 32, 4);
}

static void test_pack_PSECURITY_DESCRIPTOR(void)
{
    /* PSECURITY_DESCRIPTOR */
    TEST_TYPE(PSECURITY_DESCRIPTOR, 4, 4);
}

static void test_pack_PSECURITY_QUALITY_OF_SERVICE(void)
{
    /* PSECURITY_QUALITY_OF_SERVICE */
    TEST_TYPE(PSECURITY_QUALITY_OF_SERVICE, 4, 4);
}

static void test_pack_PSID(void)
{
    /* PSID */
    TEST_TYPE(PSID, 4, 4);
}

static void test_pack_PSID_IDENTIFIER_AUTHORITY(void)
{
    /* PSID_IDENTIFIER_AUTHORITY */
    TEST_TYPE(PSID_IDENTIFIER_AUTHORITY, 4, 4);
    TEST_TYPE_POINTER(PSID_IDENTIFIER_AUTHORITY, 6, 1);
}

static void test_pack_PSINGLE_LIST_ENTRY(void)
{
    /* PSINGLE_LIST_ENTRY */
    TEST_TYPE(PSINGLE_LIST_ENTRY, 4, 4);
    TEST_TYPE_POINTER(PSINGLE_LIST_ENTRY, 4, 4);
}

static void test_pack_PSTR(void)
{
    /* PSTR */
    TEST_TYPE(PSTR, 4, 4);
    TEST_TYPE_POINTER(PSTR, 1, 1);
}

static void test_pack_PSYSTEM_ALARM_ACE(void)
{
    /* PSYSTEM_ALARM_ACE */
    TEST_TYPE(PSYSTEM_ALARM_ACE, 4, 4);
    TEST_TYPE_POINTER(PSYSTEM_ALARM_ACE, 12, 4);
}

static void test_pack_PSYSTEM_AUDIT_ACE(void)
{
    /* PSYSTEM_AUDIT_ACE */
    TEST_TYPE(PSYSTEM_AUDIT_ACE, 4, 4);
    TEST_TYPE_POINTER(PSYSTEM_AUDIT_ACE, 12, 4);
}

static void test_pack_PTOKEN_GROUPS(void)
{
    /* PTOKEN_GROUPS */
    TEST_TYPE(PTOKEN_GROUPS, 4, 4);
    TEST_TYPE_POINTER(PTOKEN_GROUPS, 12, 4);
}

static void test_pack_PTOKEN_PRIVILEGES(void)
{
    /* PTOKEN_PRIVILEGES */
    TEST_TYPE(PTOKEN_PRIVILEGES, 4, 4);
    TEST_TYPE_POINTER(PTOKEN_PRIVILEGES, 16, 4);
}

static void test_pack_PTOKEN_USER(void)
{
    /* PTOKEN_USER */
    TEST_TYPE(PTOKEN_USER, 4, 4);
    TEST_TYPE_POINTER(PTOKEN_USER, 8, 4);
}

static void test_pack_PTOP_LEVEL_EXCEPTION_FILTER(void)
{
    /* PTOP_LEVEL_EXCEPTION_FILTER */
    TEST_TYPE(PTOP_LEVEL_EXCEPTION_FILTER, 4, 4);
}

static void test_pack_PTSTR(void)
{
    /* PTSTR */
    TEST_TYPE(PTSTR, 4, 4);
}

static void test_pack_PULARGE_INTEGER(void)
{
    /* PULARGE_INTEGER */
    TEST_TYPE(PULARGE_INTEGER, 4, 4);
}

static void test_pack_PVECTORED_EXCEPTION_HANDLER(void)
{
    /* PVECTORED_EXCEPTION_HANDLER */
    TEST_TYPE(PVECTORED_EXCEPTION_HANDLER, 4, 4);
}

static void test_pack_PVOID(void)
{
    /* PVOID */
    TEST_TYPE(PVOID, 4, 4);
}

static void test_pack_PWCH(void)
{
    /* PWCH */
    TEST_TYPE(PWCH, 4, 4);
    TEST_TYPE_POINTER(PWCH, 2, 2);
}

static void test_pack_PWSTR(void)
{
    /* PWSTR */
    TEST_TYPE(PWSTR, 4, 4);
    TEST_TYPE_POINTER(PWSTR, 2, 2);
}

static void test_pack_RTL_CRITICAL_SECTION(void)
{
    /* RTL_CRITICAL_SECTION (pack 4) */
    TEST_TYPE(RTL_CRITICAL_SECTION, 24, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, PRTL_CRITICAL_SECTION_DEBUG, DebugInfo, 0, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, LONG, LockCount, 4, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, LONG, RecursionCount, 8, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, HANDLE, OwningThread, 12, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, HANDLE, LockSemaphore, 16, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION, ULONG_PTR, SpinCount, 20, 4, 4);
}

static void test_pack_RTL_CRITICAL_SECTION_DEBUG(void)
{
    /* RTL_CRITICAL_SECTION_DEBUG (pack 4) */
    TEST_TYPE(RTL_CRITICAL_SECTION_DEBUG, 32, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, WORD, Type, 0, 2, 2);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, WORD, CreatorBackTraceIndex, 2, 2, 2);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, struct _RTL_CRITICAL_SECTION *, CriticalSection, 4, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, LIST_ENTRY, ProcessLocksList, 8, 8, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, DWORD, EntryCount, 16, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, DWORD, ContentionCount, 20, 4, 4);
    TEST_FIELD(RTL_CRITICAL_SECTION_DEBUG, DWORD[ 2 ], Spare, 24, 8, 4);
}

static void test_pack_RTL_OSVERSIONINFOEXW(void)
{
    /* RTL_OSVERSIONINFOEXW (pack 4) */
    TEST_TYPE(RTL_OSVERSIONINFOEXW, 284, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, WCHAR[128], szCSDVersion, 20, 256, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, WORD, wServicePackMajor, 276, 2, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, WORD, wServicePackMinor, 278, 2, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, WORD, wSuiteMask, 280, 2, 2);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, BYTE, wProductType, 282, 1, 1);
    TEST_FIELD(RTL_OSVERSIONINFOEXW, BYTE, wReserved, 283, 1, 1);
}

static void test_pack_RTL_OSVERSIONINFOW(void)
{
    /* RTL_OSVERSIONINFOW (pack 4) */
    TEST_TYPE(RTL_OSVERSIONINFOW, 276, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, DWORD, dwOSVersionInfoSize, 0, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, DWORD, dwPlatformId, 16, 4, 4);
    TEST_FIELD(RTL_OSVERSIONINFOW, WCHAR[128], szCSDVersion, 20, 256, 2);
}

static void test_pack_RTL_RESOURCE_DEBUG(void)
{
    /* RTL_RESOURCE_DEBUG (pack 4) */
    TEST_TYPE(RTL_RESOURCE_DEBUG, 32, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, WORD, Type, 0, 2, 2);
    TEST_FIELD(RTL_RESOURCE_DEBUG, WORD, CreatorBackTraceIndex, 2, 2, 2);
    TEST_FIELD(RTL_RESOURCE_DEBUG, struct _RTL_CRITICAL_SECTION *, CriticalSection, 4, 4, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, LIST_ENTRY, ProcessLocksList, 8, 8, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, DWORD, EntryCount, 16, 4, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, DWORD, ContentionCount, 20, 4, 4);
    TEST_FIELD(RTL_RESOURCE_DEBUG, DWORD[ 2 ], Spare, 24, 8, 4);
}

static void test_pack_SECURITY_CONTEXT_TRACKING_MODE(void)
{
    /* SECURITY_CONTEXT_TRACKING_MODE */
    TEST_TYPE(SECURITY_CONTEXT_TRACKING_MODE, 1, 1);
}

static void test_pack_SECURITY_DESCRIPTOR(void)
{
    /* SECURITY_DESCRIPTOR (pack 4) */
    TEST_TYPE(SECURITY_DESCRIPTOR, 20, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, BYTE, Revision, 0, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR, BYTE, Sbz1, 1, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR, SECURITY_DESCRIPTOR_CONTROL, Control, 2, 2, 2);
    TEST_FIELD(SECURITY_DESCRIPTOR, PSID, Owner, 4, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, PSID, Group, 8, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, PACL, Sacl, 12, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR, PACL, Dacl, 16, 4, 4);
}

static void test_pack_SECURITY_DESCRIPTOR_CONTROL(void)
{
    /* SECURITY_DESCRIPTOR_CONTROL */
    TEST_TYPE(SECURITY_DESCRIPTOR_CONTROL, 2, 2);
    TEST_TYPE_UNSIGNED(SECURITY_DESCRIPTOR_CONTROL);
}

static void test_pack_SECURITY_DESCRIPTOR_RELATIVE(void)
{
    /* SECURITY_DESCRIPTOR_RELATIVE (pack 4) */
    TEST_TYPE(SECURITY_DESCRIPTOR_RELATIVE, 20, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, BYTE, Revision, 0, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, BYTE, Sbz1, 1, 1, 1);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, SECURITY_DESCRIPTOR_CONTROL, Control, 2, 2, 2);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, DWORD, Owner, 4, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, DWORD, Group, 8, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, DWORD, Sacl, 12, 4, 4);
    TEST_FIELD(SECURITY_DESCRIPTOR_RELATIVE, DWORD, Dacl, 16, 4, 4);
}

static void test_pack_SECURITY_INFORMATION(void)
{
    /* SECURITY_INFORMATION */
    TEST_TYPE(SECURITY_INFORMATION, 4, 4);
    TEST_TYPE_UNSIGNED(SECURITY_INFORMATION);
}

static void test_pack_SECURITY_QUALITY_OF_SERVICE(void)
{
    /* SECURITY_QUALITY_OF_SERVICE (pack 4) */
    TEST_FIELD(SECURITY_QUALITY_OF_SERVICE, DWORD, Length, 0, 4, 4);
}

static void test_pack_SHORT(void)
{
    /* SHORT */
    TEST_TYPE(SHORT, 2, 2);
    TEST_TYPE_SIGNED(SHORT);
}

static void test_pack_SID(void)
{
    /* SID (pack 4) */
    TEST_TYPE(SID, 12, 4);
    TEST_FIELD(SID, BYTE, Revision, 0, 1, 1);
    TEST_FIELD(SID, BYTE, SubAuthorityCount, 1, 1, 1);
    TEST_FIELD(SID, SID_IDENTIFIER_AUTHORITY, IdentifierAuthority, 2, 6, 1);
    TEST_FIELD(SID, DWORD[1], SubAuthority, 8, 4, 4);
}

static void test_pack_SID_AND_ATTRIBUTES(void)
{
    /* SID_AND_ATTRIBUTES (pack 4) */
    TEST_TYPE(SID_AND_ATTRIBUTES, 8, 4);
    TEST_FIELD(SID_AND_ATTRIBUTES, PSID, Sid, 0, 4, 4);
    TEST_FIELD(SID_AND_ATTRIBUTES, DWORD, Attributes, 4, 4, 4);
}

static void test_pack_SID_IDENTIFIER_AUTHORITY(void)
{
    /* SID_IDENTIFIER_AUTHORITY (pack 4) */
    TEST_TYPE(SID_IDENTIFIER_AUTHORITY, 6, 1);
    TEST_FIELD(SID_IDENTIFIER_AUTHORITY, BYTE[6], Value, 0, 6, 1);
}

static void test_pack_SINGLE_LIST_ENTRY(void)
{
    /* SINGLE_LIST_ENTRY (pack 4) */
    TEST_TYPE(SINGLE_LIST_ENTRY, 4, 4);
    TEST_FIELD(SINGLE_LIST_ENTRY, struct _SINGLE_LIST_ENTRY *, Next, 0, 4, 4);
}

static void test_pack_SYSTEM_ALARM_ACE(void)
{
    /* SYSTEM_ALARM_ACE (pack 4) */
    TEST_TYPE(SYSTEM_ALARM_ACE, 12, 4);
    TEST_FIELD(SYSTEM_ALARM_ACE, ACE_HEADER, Header, 0, 4, 2);
    TEST_FIELD(SYSTEM_ALARM_ACE, DWORD, Mask, 4, 4, 4);
    TEST_FIELD(SYSTEM_ALARM_ACE, DWORD, SidStart, 8, 4, 4);
}

static void test_pack_SYSTEM_AUDIT_ACE(void)
{
    /* SYSTEM_AUDIT_ACE (pack 4) */
    TEST_TYPE(SYSTEM_AUDIT_ACE, 12, 4);
    TEST_FIELD(SYSTEM_AUDIT_ACE, ACE_HEADER, Header, 0, 4, 2);
    TEST_FIELD(SYSTEM_AUDIT_ACE, DWORD, Mask, 4, 4, 4);
    TEST_FIELD(SYSTEM_AUDIT_ACE, DWORD, SidStart, 8, 4, 4);
}

static void test_pack_TCHAR(void)
{
    /* TCHAR */
    TEST_TYPE(TCHAR, 1, 1);
}

static void test_pack_TOKEN_DEFAULT_DACL(void)
{
    /* TOKEN_DEFAULT_DACL (pack 4) */
    TEST_TYPE(TOKEN_DEFAULT_DACL, 4, 4);
    TEST_FIELD(TOKEN_DEFAULT_DACL, PACL, DefaultDacl, 0, 4, 4);
}

static void test_pack_TOKEN_GROUPS(void)
{
    /* TOKEN_GROUPS (pack 4) */
    TEST_TYPE(TOKEN_GROUPS, 12, 4);
    TEST_FIELD(TOKEN_GROUPS, DWORD, GroupCount, 0, 4, 4);
    TEST_FIELD(TOKEN_GROUPS, SID_AND_ATTRIBUTES[ANYSIZE_ARRAY], Groups, 4, 8, 4);
}

static void test_pack_TOKEN_OWNER(void)
{
    /* TOKEN_OWNER (pack 4) */
    TEST_TYPE(TOKEN_OWNER, 4, 4);
    TEST_FIELD(TOKEN_OWNER, PSID, Owner, 0, 4, 4);
}

static void test_pack_TOKEN_PRIMARY_GROUP(void)
{
    /* TOKEN_PRIMARY_GROUP (pack 4) */
    TEST_TYPE(TOKEN_PRIMARY_GROUP, 4, 4);
    TEST_FIELD(TOKEN_PRIMARY_GROUP, PSID, PrimaryGroup, 0, 4, 4);
}

static void test_pack_TOKEN_PRIVILEGES(void)
{
    /* TOKEN_PRIVILEGES (pack 4) */
    TEST_TYPE(TOKEN_PRIVILEGES, 16, 4);
    TEST_FIELD(TOKEN_PRIVILEGES, DWORD, PrivilegeCount, 0, 4, 4);
    TEST_FIELD(TOKEN_PRIVILEGES, LUID_AND_ATTRIBUTES[ANYSIZE_ARRAY], Privileges, 4, 12, 4);
}

static void test_pack_TOKEN_SOURCE(void)
{
    /* TOKEN_SOURCE (pack 4) */
    TEST_TYPE(TOKEN_SOURCE, 16, 4);
    TEST_FIELD(TOKEN_SOURCE, char[TOKEN_SOURCE_LENGTH], SourceName, 0, 8, 1);
    TEST_FIELD(TOKEN_SOURCE, LUID, SourceIdentifier, 8, 8, 4);
}

static void test_pack_TOKEN_STATISTICS(void)
{
    /* TOKEN_STATISTICS (pack 4) */
    TEST_FIELD(TOKEN_STATISTICS, LUID, TokenId, 0, 8, 4);
    TEST_FIELD(TOKEN_STATISTICS, LUID, AuthenticationId, 8, 8, 4);
    TEST_FIELD(TOKEN_STATISTICS, LARGE_INTEGER, ExpirationTime, 16, 8, 4);
}

static void test_pack_TOKEN_USER(void)
{
    /* TOKEN_USER (pack 4) */
    TEST_TYPE(TOKEN_USER, 8, 4);
    TEST_FIELD(TOKEN_USER, SID_AND_ATTRIBUTES, User, 0, 8, 4);
}

static void test_pack_ULARGE_INTEGER(void)
{
    /* ULARGE_INTEGER (pack 4) */
}

static void test_pack_ULONGLONG(void)
{
    /* ULONGLONG */
    TEST_TYPE(ULONGLONG, 8, 8);
    TEST_TYPE_UNSIGNED(ULONGLONG);
}

static void test_pack_WAITORTIMERCALLBACKFUNC(void)
{
    /* WAITORTIMERCALLBACKFUNC */
    TEST_TYPE(WAITORTIMERCALLBACKFUNC, 4, 4);
}

static void test_pack_WCHAR(void)
{
    /* WCHAR */
    TEST_TYPE(WCHAR, 2, 2);
    TEST_TYPE_UNSIGNED(WCHAR);
}

static void test_pack_ATOM(void)
{
    /* ATOM */
    TEST_TYPE(ATOM, 2, 2);
    TEST_TYPE_UNSIGNED(ATOM);
}

static void test_pack_BOOL(void)
{
    /* BOOL */
    TEST_TYPE(BOOL, 4, 4);
    TEST_TYPE_SIGNED(BOOL);
}

static void test_pack_BYTE(void)
{
    /* BYTE */
    TEST_TYPE(BYTE, 1, 1);
    TEST_TYPE_UNSIGNED(BYTE);
}

static void test_pack_COLORREF(void)
{
    /* COLORREF */
    TEST_TYPE(COLORREF, 4, 4);
    TEST_TYPE_UNSIGNED(COLORREF);
}

static void test_pack_DWORD(void)
{
    /* DWORD */
    TEST_TYPE(DWORD, 4, 4);
    TEST_TYPE_UNSIGNED(DWORD);
}

static void test_pack_FARPROC(void)
{
    /* FARPROC */
    TEST_TYPE(FARPROC, 4, 4);
}

static void test_pack_FLOAT(void)
{
    /* FLOAT */
    TEST_TYPE(FLOAT, 4, 4);
}

static void test_pack_GLOBALHANDLE(void)
{
    /* GLOBALHANDLE */
    TEST_TYPE(GLOBALHANDLE, 4, 4);
}

static void test_pack_HCURSOR(void)
{
    /* HCURSOR */
    TEST_TYPE(HCURSOR, 4, 4);
    TEST_TYPE_UNSIGNED(HCURSOR);
}

static void test_pack_HFILE(void)
{
    /* HFILE */
    TEST_TYPE(HFILE, 4, 4);
    TEST_TYPE_SIGNED(HFILE);
}

static void test_pack_HGDIOBJ(void)
{
    /* HGDIOBJ */
    TEST_TYPE(HGDIOBJ, 4, 4);
}

static void test_pack_HGLOBAL(void)
{
    /* HGLOBAL */
    TEST_TYPE(HGLOBAL, 4, 4);
}

static void test_pack_HLOCAL(void)
{
    /* HLOCAL */
    TEST_TYPE(HLOCAL, 4, 4);
}

static void test_pack_HMODULE(void)
{
    /* HMODULE */
    TEST_TYPE(HMODULE, 4, 4);
    TEST_TYPE_UNSIGNED(HMODULE);
}

static void test_pack_INT(void)
{
    /* INT */
    TEST_TYPE(INT, 4, 4);
    TEST_TYPE_SIGNED(INT);
}

static void test_pack_LOCALHANDLE(void)
{
    /* LOCALHANDLE */
    TEST_TYPE(LOCALHANDLE, 4, 4);
}

static void test_pack_LPARAM(void)
{
    /* LPARAM */
    TEST_TYPE(LPARAM, 4, 4);
}

static void test_pack_LPCRECT(void)
{
    /* LPCRECT */
    TEST_TYPE(LPCRECT, 4, 4);
    TEST_TYPE_POINTER(LPCRECT, 16, 4);
}

static void test_pack_LPCRECTL(void)
{
    /* LPCRECTL */
    TEST_TYPE(LPCRECTL, 4, 4);
    TEST_TYPE_POINTER(LPCRECTL, 16, 4);
}

static void test_pack_LPCVOID(void)
{
    /* LPCVOID */
    TEST_TYPE(LPCVOID, 4, 4);
}

static void test_pack_LPPOINT(void)
{
    /* LPPOINT */
    TEST_TYPE(LPPOINT, 4, 4);
    TEST_TYPE_POINTER(LPPOINT, 8, 4);
}

static void test_pack_LPPOINTS(void)
{
    /* LPPOINTS */
    TEST_TYPE(LPPOINTS, 4, 4);
    TEST_TYPE_POINTER(LPPOINTS, 4, 2);
}

static void test_pack_LPRECT(void)
{
    /* LPRECT */
    TEST_TYPE(LPRECT, 4, 4);
    TEST_TYPE_POINTER(LPRECT, 16, 4);
}

static void test_pack_LPRECTL(void)
{
    /* LPRECTL */
    TEST_TYPE(LPRECTL, 4, 4);
    TEST_TYPE_POINTER(LPRECTL, 16, 4);
}

static void test_pack_LPSIZE(void)
{
    /* LPSIZE */
    TEST_TYPE(LPSIZE, 4, 4);
    TEST_TYPE_POINTER(LPSIZE, 8, 4);
}

static void test_pack_LRESULT(void)
{
    /* LRESULT */
    TEST_TYPE(LRESULT, 4, 4);
}

static void test_pack_POINT(void)
{
    /* POINT (pack 4) */
    TEST_TYPE(POINT, 8, 4);
    TEST_FIELD(POINT, LONG, x, 0, 4, 4);
    TEST_FIELD(POINT, LONG, y, 4, 4, 4);
}

static void test_pack_POINTL(void)
{
    /* POINTL (pack 4) */
    TEST_TYPE(POINTL, 8, 4);
    TEST_FIELD(POINTL, LONG, x, 0, 4, 4);
    TEST_FIELD(POINTL, LONG, y, 4, 4, 4);
}

static void test_pack_POINTS(void)
{
    /* POINTS (pack 4) */
    TEST_TYPE(POINTS, 4, 2);
    TEST_FIELD(POINTS, SHORT, x, 0, 2, 2);
    TEST_FIELD(POINTS, SHORT, y, 2, 2, 2);
}

static void test_pack_PPOINT(void)
{
    /* PPOINT */
    TEST_TYPE(PPOINT, 4, 4);
    TEST_TYPE_POINTER(PPOINT, 8, 4);
}

static void test_pack_PPOINTL(void)
{
    /* PPOINTL */
    TEST_TYPE(PPOINTL, 4, 4);
    TEST_TYPE_POINTER(PPOINTL, 8, 4);
}

static void test_pack_PPOINTS(void)
{
    /* PPOINTS */
    TEST_TYPE(PPOINTS, 4, 4);
    TEST_TYPE_POINTER(PPOINTS, 4, 2);
}

static void test_pack_PRECT(void)
{
    /* PRECT */
    TEST_TYPE(PRECT, 4, 4);
    TEST_TYPE_POINTER(PRECT, 16, 4);
}

static void test_pack_PRECTL(void)
{
    /* PRECTL */
    TEST_TYPE(PRECTL, 4, 4);
    TEST_TYPE_POINTER(PRECTL, 16, 4);
}

static void test_pack_PROC(void)
{
    /* PROC */
    TEST_TYPE(PROC, 4, 4);
}

static void test_pack_PSIZE(void)
{
    /* PSIZE */
    TEST_TYPE(PSIZE, 4, 4);
    TEST_TYPE_POINTER(PSIZE, 8, 4);
}

static void test_pack_PSZ(void)
{
    /* PSZ */
    TEST_TYPE(PSZ, 4, 4);
}

static void test_pack_RECT(void)
{
    /* RECT (pack 4) */
    TEST_TYPE(RECT, 16, 4);
    TEST_FIELD(RECT, LONG, left, 0, 4, 4);
    TEST_FIELD(RECT, LONG, top, 4, 4, 4);
    TEST_FIELD(RECT, LONG, right, 8, 4, 4);
    TEST_FIELD(RECT, LONG, bottom, 12, 4, 4);
}

static void test_pack_RECTL(void)
{
    /* RECTL (pack 4) */
    TEST_TYPE(RECTL, 16, 4);
    TEST_FIELD(RECTL, LONG, left, 0, 4, 4);
    TEST_FIELD(RECTL, LONG, top, 4, 4, 4);
    TEST_FIELD(RECTL, LONG, right, 8, 4, 4);
    TEST_FIELD(RECTL, LONG, bottom, 12, 4, 4);
}

static void test_pack_SIZE(void)
{
    /* SIZE (pack 4) */
    TEST_TYPE(SIZE, 8, 4);
    TEST_FIELD(SIZE, LONG, cx, 0, 4, 4);
    TEST_FIELD(SIZE, LONG, cy, 4, 4, 4);
}

static void test_pack_SIZEL(void)
{
    /* SIZEL */
    TEST_TYPE(SIZEL, 8, 4);
}

static void test_pack_UCHAR(void)
{
    /* UCHAR */
    TEST_TYPE(UCHAR, 1, 1);
    TEST_TYPE_UNSIGNED(UCHAR);
}

static void test_pack_UINT(void)
{
    /* UINT */
    TEST_TYPE(UINT, 4, 4);
    TEST_TYPE_UNSIGNED(UINT);
}

static void test_pack_ULONG(void)
{
    /* ULONG */
    TEST_TYPE(ULONG, 4, 4);
    TEST_TYPE_UNSIGNED(ULONG);
}

static void test_pack_USHORT(void)
{
    /* USHORT */
    TEST_TYPE(USHORT, 2, 2);
    TEST_TYPE_UNSIGNED(USHORT);
}

static void test_pack_WORD(void)
{
    /* WORD */
    TEST_TYPE(WORD, 2, 2);
    TEST_TYPE_UNSIGNED(WORD);
}

static void test_pack_WPARAM(void)
{
    /* WPARAM */
    TEST_TYPE(WPARAM, 4, 4);
}

static void test_pack(void)
{
    test_pack_ACCESS_ALLOWED_ACE();
    test_pack_ACCESS_DENIED_ACE();
    test_pack_ACCESS_MASK();
    test_pack_ACE_HEADER();
    test_pack_ACL();
    test_pack_ACL_REVISION_INFORMATION();
    test_pack_ACL_SIZE_INFORMATION();
    test_pack_ATOM();
    test_pack_BOOL();
    test_pack_BOOLEAN();
    test_pack_BYTE();
    test_pack_CCHAR();
    test_pack_CHAR();
    test_pack_COLORREF();
    test_pack_DWORD();
    test_pack_DWORD32();
    test_pack_DWORD64();
    test_pack_DWORDLONG();
    test_pack_DWORD_PTR();
    test_pack_EXCEPTION_POINTERS();
    test_pack_EXCEPTION_RECORD();
    test_pack_EXECUTION_STATE();
    test_pack_FARPROC();
    test_pack_FLOAT();
    test_pack_FLOATING_SAVE_AREA();
    test_pack_FPO_DATA();
    test_pack_GENERIC_MAPPING();
    test_pack_GLOBALHANDLE();
    test_pack_HALF_PTR();
    test_pack_HANDLE();
    test_pack_HCURSOR();
    test_pack_HFILE();
    test_pack_HGDIOBJ();
    test_pack_HGLOBAL();
    test_pack_HLOCAL();
    test_pack_HMODULE();
    test_pack_HRESULT();
    test_pack_IMAGE_ARCHIVE_MEMBER_HEADER();
    test_pack_IMAGE_AUX_SYMBOL();
    test_pack_IMAGE_BASE_RELOCATION();
    test_pack_IMAGE_BOUND_FORWARDER_REF();
    test_pack_IMAGE_BOUND_IMPORT_DESCRIPTOR();
    test_pack_IMAGE_COFF_SYMBOLS_HEADER();
    test_pack_IMAGE_DATA_DIRECTORY();
    test_pack_IMAGE_DEBUG_DIRECTORY();
    test_pack_IMAGE_DEBUG_MISC();
    test_pack_IMAGE_DOS_HEADER();
    test_pack_IMAGE_EXPORT_DIRECTORY();
    test_pack_IMAGE_FILE_HEADER();
    test_pack_IMAGE_FUNCTION_ENTRY();
    test_pack_IMAGE_IMPORT_BY_NAME();
    test_pack_IMAGE_IMPORT_DESCRIPTOR();
    test_pack_IMAGE_LINENUMBER();
    test_pack_IMAGE_LOAD_CONFIG_DIRECTORY();
    test_pack_IMAGE_NT_HEADERS();
    test_pack_IMAGE_OPTIONAL_HEADER();
    test_pack_IMAGE_OS2_HEADER();
    test_pack_IMAGE_RELOCATION();
    test_pack_IMAGE_RESOURCE_DATA_ENTRY();
    test_pack_IMAGE_RESOURCE_DIRECTORY();
    test_pack_IMAGE_RESOURCE_DIRECTORY_ENTRY();
    test_pack_IMAGE_RESOURCE_DIRECTORY_STRING();
    test_pack_IMAGE_RESOURCE_DIR_STRING_U();
    test_pack_IMAGE_SECTION_HEADER();
    test_pack_IMAGE_SEPARATE_DEBUG_HEADER();
    test_pack_IMAGE_SYMBOL();
    test_pack_IMAGE_THUNK_DATA();
    test_pack_IMAGE_TLS_DIRECTORY();
    test_pack_IMAGE_VXD_HEADER();
    test_pack_INT();
    test_pack_INT16();
    test_pack_INT32();
    test_pack_INT64();
    test_pack_INT8();
    test_pack_INT_PTR();
    test_pack_IO_COUNTERS();
    test_pack_LANGID();
    test_pack_LARGE_INTEGER();
    test_pack_LCID();
    test_pack_LIST_ENTRY();
    test_pack_LOCALHANDLE();
    test_pack_LONG();
    test_pack_LONG32();
    test_pack_LONG64();
    test_pack_LONGLONG();
    test_pack_LONG_PTR();
    test_pack_LPARAM();
    test_pack_LPCRECT();
    test_pack_LPCRECTL();
    test_pack_LPCVOID();
    test_pack_LPPOINT();
    test_pack_LPPOINTS();
    test_pack_LPRECT();
    test_pack_LPRECTL();
    test_pack_LPSIZE();
    test_pack_LPTOP_LEVEL_EXCEPTION_FILTER();
    test_pack_LRESULT();
    test_pack_LUID();
    test_pack_LUID_AND_ATTRIBUTES();
    test_pack_MEMORY_BASIC_INFORMATION();
    test_pack_MESSAGE_RESOURCE_BLOCK();
    test_pack_MESSAGE_RESOURCE_DATA();
    test_pack_MESSAGE_RESOURCE_ENTRY();
    test_pack_NT_TIB();
    test_pack_OBJECT_TYPE_LIST();
    test_pack_PACCESS_ALLOWED_ACE();
    test_pack_PACCESS_DENIED_ACE();
    test_pack_PACCESS_TOKEN();
    test_pack_PACE_HEADER();
    test_pack_PACL();
    test_pack_PACL_REVISION_INFORMATION();
    test_pack_PACL_SIZE_INFORMATION();
    test_pack_PCCH();
    test_pack_PCH();
    test_pack_PCSTR();
    test_pack_PCTSTR();
    test_pack_PCWCH();
    test_pack_PCWSTR();
    test_pack_PEXCEPTION_POINTERS();
    test_pack_PEXCEPTION_RECORD();
    test_pack_PFLOATING_SAVE_AREA();
    test_pack_PFPO_DATA();
    test_pack_PGENERIC_MAPPING();
    test_pack_PHANDLE();
    test_pack_PIMAGE_ARCHIVE_MEMBER_HEADER();
    test_pack_PIMAGE_AUX_SYMBOL();
    test_pack_PIMAGE_BASE_RELOCATION();
    test_pack_PIMAGE_BOUND_FORWARDER_REF();
    test_pack_PIMAGE_BOUND_IMPORT_DESCRIPTOR();
    test_pack_PIMAGE_COFF_SYMBOLS_HEADER();
    test_pack_PIMAGE_DATA_DIRECTORY();
    test_pack_PIMAGE_DEBUG_DIRECTORY();
    test_pack_PIMAGE_DEBUG_MISC();
    test_pack_PIMAGE_DOS_HEADER();
    test_pack_PIMAGE_EXPORT_DIRECTORY();
    test_pack_PIMAGE_FILE_HEADER();
    test_pack_PIMAGE_FUNCTION_ENTRY();
    test_pack_PIMAGE_IMPORT_BY_NAME();
    test_pack_PIMAGE_IMPORT_DESCRIPTOR();
    test_pack_PIMAGE_LINENUMBER();
    test_pack_PIMAGE_LOAD_CONFIG_DIRECTORY();
    test_pack_PIMAGE_NT_HEADERS();
    test_pack_PIMAGE_OPTIONAL_HEADER();
    test_pack_PIMAGE_OS2_HEADER();
    test_pack_PIMAGE_RELOCATION();
    test_pack_PIMAGE_RESOURCE_DATA_ENTRY();
    test_pack_PIMAGE_RESOURCE_DIRECTORY();
    test_pack_PIMAGE_RESOURCE_DIRECTORY_ENTRY();
    test_pack_PIMAGE_RESOURCE_DIRECTORY_STRING();
    test_pack_PIMAGE_RESOURCE_DIR_STRING_U();
    test_pack_PIMAGE_SECTION_HEADER();
    test_pack_PIMAGE_SEPARATE_DEBUG_HEADER();
    test_pack_PIMAGE_SYMBOL();
    test_pack_PIMAGE_THUNK_DATA();
    test_pack_PIMAGE_TLS_CALLBACK();
    test_pack_PIMAGE_TLS_DIRECTORY();
    test_pack_PIMAGE_VXD_HEADER();
    test_pack_PIO_COUNTERS();
    test_pack_PISECURITY_DESCRIPTOR();
    test_pack_PISECURITY_DESCRIPTOR_RELATIVE();
    test_pack_PISID();
    test_pack_PLARGE_INTEGER();
    test_pack_PLIST_ENTRY();
    test_pack_PLUID();
    test_pack_PLUID_AND_ATTRIBUTES();
    test_pack_PMEMORY_BASIC_INFORMATION();
    test_pack_PMESSAGE_RESOURCE_BLOCK();
    test_pack_PMESSAGE_RESOURCE_DATA();
    test_pack_PMESSAGE_RESOURCE_ENTRY();
    test_pack_PNT_TIB();
    test_pack_POBJECT_TYPE_LIST();
    test_pack_POINT();
    test_pack_POINTL();
    test_pack_POINTS();
    test_pack_PPOINT();
    test_pack_PPOINTL();
    test_pack_PPOINTS();
    test_pack_PPRIVILEGE_SET();
    test_pack_PRECT();
    test_pack_PRECTL();
    test_pack_PRIVILEGE_SET();
    test_pack_PRLIST_ENTRY();
    test_pack_PROC();
    test_pack_PRTL_CRITICAL_SECTION();
    test_pack_PRTL_CRITICAL_SECTION_DEBUG();
    test_pack_PRTL_OSVERSIONINFOEXW();
    test_pack_PRTL_OSVERSIONINFOW();
    test_pack_PRTL_RESOURCE_DEBUG();
    test_pack_PSECURITY_DESCRIPTOR();
    test_pack_PSECURITY_QUALITY_OF_SERVICE();
    test_pack_PSID();
    test_pack_PSID_IDENTIFIER_AUTHORITY();
    test_pack_PSINGLE_LIST_ENTRY();
    test_pack_PSIZE();
    test_pack_PSTR();
    test_pack_PSYSTEM_ALARM_ACE();
    test_pack_PSYSTEM_AUDIT_ACE();
    test_pack_PSZ();
    test_pack_PTOKEN_GROUPS();
    test_pack_PTOKEN_PRIVILEGES();
    test_pack_PTOKEN_USER();
    test_pack_PTOP_LEVEL_EXCEPTION_FILTER();
    test_pack_PTSTR();
    test_pack_PULARGE_INTEGER();
    test_pack_PVECTORED_EXCEPTION_HANDLER();
    test_pack_PVOID();
    test_pack_PWCH();
    test_pack_PWSTR();
    test_pack_RECT();
    test_pack_RECTL();
    test_pack_RTL_CRITICAL_SECTION();
    test_pack_RTL_CRITICAL_SECTION_DEBUG();
    test_pack_RTL_OSVERSIONINFOEXW();
    test_pack_RTL_OSVERSIONINFOW();
    test_pack_RTL_RESOURCE_DEBUG();
    test_pack_SECURITY_CONTEXT_TRACKING_MODE();
    test_pack_SECURITY_DESCRIPTOR();
    test_pack_SECURITY_DESCRIPTOR_CONTROL();
    test_pack_SECURITY_DESCRIPTOR_RELATIVE();
    test_pack_SECURITY_INFORMATION();
    test_pack_SECURITY_QUALITY_OF_SERVICE();
    test_pack_SHORT();
    test_pack_SID();
    test_pack_SID_AND_ATTRIBUTES();
    test_pack_SID_IDENTIFIER_AUTHORITY();
    test_pack_SINGLE_LIST_ENTRY();
    test_pack_SIZE();
    test_pack_SIZEL();
    test_pack_SIZE_T();
    test_pack_SSIZE_T();
    test_pack_SYSTEM_ALARM_ACE();
    test_pack_SYSTEM_AUDIT_ACE();
    test_pack_TCHAR();
    test_pack_TOKEN_DEFAULT_DACL();
    test_pack_TOKEN_GROUPS();
    test_pack_TOKEN_OWNER();
    test_pack_TOKEN_PRIMARY_GROUP();
    test_pack_TOKEN_PRIVILEGES();
    test_pack_TOKEN_SOURCE();
    test_pack_TOKEN_STATISTICS();
    test_pack_TOKEN_USER();
    test_pack_UCHAR();
    test_pack_UHALF_PTR();
    test_pack_UINT();
    test_pack_UINT16();
    test_pack_UINT32();
    test_pack_UINT64();
    test_pack_UINT8();
    test_pack_UINT_PTR();
    test_pack_ULARGE_INTEGER();
    test_pack_ULONG();
    test_pack_ULONG32();
    test_pack_ULONG64();
    test_pack_ULONGLONG();
    test_pack_ULONG_PTR();
    test_pack_USHORT();
    test_pack_WAITORTIMERCALLBACKFUNC();
    test_pack_WCHAR();
    test_pack_WORD();
    test_pack_WPARAM();
}

START_TEST(generated)
{
    test_pack();
}
