#ifndef _STRUTIL_H
#define _STRUTIL_H

typedef enum _EXT_STRING_TYPE
{
    /* unknown */
    stUnknown,
    /* Ansi-String */
    stAnsiStr,
    /* Unicode-String */
    stUnicodeStr,
    /* any Data (or unknown) */
    stData
} EXT_STRING_TYPE;

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
typedef struct _EXT_STRING_DATA
{
    USHORT bUsed;
    USHORT bAllocated;
    PBYTE  Buffer;
    EXT_STRING_TYPE typ;
} EXT_STRING_DATA, *PEXT_STRING_DATA;
typedef struct _EXT_STRING_DATA EXT_STRING_A, *PEXT_STRING_A;
typedef struct _EXT_STRING_DATA EXT_STRING_W, *PEXT_STRING_W;
typedef struct _EXT_STRING_DATA EXT_STRING, *PEXT_STRING;

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
    IN PEXT_STRING dst,
    IN WCHAR* newstr,
    IN size_t chLen);
#define ExtWStrSet(dst, newstr) ExtWStrSetN(dst, newstr, (size_t)-1)

/* Ansi */
BOOL
ExtAStrInit(
    IN PEXT_STRING dst,
    IN char* initstr);
/* TODO
void ExtAStrSetN(
    IN PEXT_STRING dst,
    IN char* newstr,
    IN size_t length);
*/

/* Data */

/* All */

void
ExtStrFree(
    IN PEXT_STRING s);

#endif
