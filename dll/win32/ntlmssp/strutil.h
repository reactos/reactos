#ifndef _STRUTIL_H
#define _STRUTIL_H

typedef enum _EXT_DATA_TYPE
{
    /* unknown */
    stUnknown,
    /* Ansi-String */
    stAnsiStr,
    /* OEM-String */
    stOEMStr,
    /* Unicode-String */
    stUnicodeStr,
    /* any Data (or unknown) */
    stData
} EXT_DATA_TYPE;

/* String of any/unknown type */
/*typedef struct _EXT_STRING_W
{
    USHORT bUsed;
    USHORT bAllocated;
    PBYTE  Buffer;
    EXT_STRING_TYPE typ;
} EXT_STRING, *PEXT_STRING;
typedef struct _EXT_STRING_A
{
    USHORT bUsed;
    USHORT bAllocated;
    PBYTE  Buffer;
    EXT_STRING_TYPE typ;
} EXT_STRING, *PEXT_STRING;*/
typedef struct _EXT_DATA
{
    USHORT bUsed;
    USHORT bAllocated;
    PBYTE  Buffer;
    EXT_DATA_TYPE typ;
} EXT_DATA, *PEXT_DATA;
typedef struct _EXT_DATA EXT_STRING_A, *PEXT_STRING_A;
typedef struct _EXT_DATA EXT_STRING_W, *PEXT_STRING_W;
typedef struct _EXT_DATA EXT_STRING, *PEXT_STRING;

typedef void* (*_strutil_alloc_proc)(size_t size);
typedef void (*_strutil_free_proc)(void* p);

/* call only once
 * initializes the memmory manager for string allocations
 */
BOOL
init_strutil(
    IN _strutil_alloc_proc ap,
    IN _strutil_free_proc fp);

/* Unicode */

/* initstr NULL sets all values to null */
BOOL
ExtWStrInit(
    IN PEXT_STRING dst,
    IN WCHAR* initstr);

/* concatenate strings */
BOOL
ExtWStrCat(
    IN OUT PEXT_STRING dst,
    IN WCHAR* s);
/*TODO
BOOL
ExtWStrCat(
    IN OUT PEXT_STRING dst,
    IN PEXT_STRING s);*/
void
ExtWStrUpper(
    IN OUT PEXT_STRING s);

/* chLen = length in chars */
/* chLen = -1 to auto-detect length and newstr is null-terminated */
BOOL
ExtWStrSetN(
    IN PEXT_STRING_W dst,
    IN WCHAR* newstr,
    IN size_t chLen);
#define ExtWStrSet(dst, newstr) ExtWStrSetN(dst, newstr, (size_t)-1)

BOOL
ExtWStrToAStr(
    IN OUT PEXT_STRING_A dst,
    IN PEXT_STRING_W src,
    IN BOOL cpOEM,
    IN BOOL bAlloc);

/* Ansi */
BOOL
ExtAStrInit(
    IN PEXT_STRING_A dst,
    IN char* initstr);
BOOL
ExtAStrSetN(
    IN PEXT_STRING_A dst,
    IN char* newstr,
    IN size_t chLen);
#define ExtAStrSet(dst, newstr) ExtAStrSetN(dst, newstr, (size_t)-1)
BOOL
ExtAStrIsEqual1(
    IN PEXT_STRING_A v1,
    IN PEXT_STRING_A v2);
BOOL
ExtAStrIsEqual2(
    IN PEXT_STRING_A v1,
    char* v2);
BOOL
ExtDataIsEqual1(
    IN PEXT_DATA v1,
    IN PEXT_DATA v2);

/*TODO
BOOL
ExtAStrIsEqual(
    IN PEXT_STRING v1,
    IN PEXT_STRING v2);
*/

BOOL
ExtDataIsEqual(
    IN PEXT_DATA v1,
    IN PEXT_DATA v2);

/* Data */
BOOL
ExtDataInit(
    IN PEXT_DATA dst,
    IN PBYTE initdata,
    IN ULONG len);
BOOL
ExtDataInit2(
    IN PEXT_DATA dst,
    IN ULONG minBytesToAlloc);

BOOL
ExtDataSetLength(
    IN PEXT_DATA dst,
    IN ULONG len,
    IN BOOL doZeroMem);

/* All */

void
ExtStrFree(
    IN PEXT_STRING s);

#endif
