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
typedef struct _EXT_STRING
{
    USHORT bUsed;
    USHORT bAllocated;
    PBYTE  Buffer;
    EXT_STRING_TYPE typ;
} EXT_STRING, *PEXT_STRING;

typedef void* (*_strutil_alloc_proc)(size_t size);
typedef void (*_strutil_free_proc)(void* p);

/* call only once
 * initializes the memmory manager for string allocations
 */
BOOL
init_strutil(
    IN _strutil_alloc_proc ap,
    IN _strutil_free_proc fp);

#endif
