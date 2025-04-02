/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           FreeType implementation for ReactOS
 * PURPOSE:           Glue functions between FreeType
 * FILE:              win32ss/drivers/font/ftfd/rosglue.c
 * PROGRAMMER:        Ge van Geldorp (ge@gse.nl)
 * NOTES:
 */

#include "ftfd.h"

#define NDEBUG
#include <debug.h>

#define TAG_FREETYPE  'PYTF'

/*
 * First some generic routines
 */

ULONG
DbgPrint(IN PCCH Format, IN ...)
{
    va_list args;

    va_start(args, Format);
    EngDebugPrint("ft2: ", (PCHAR)Format, args);
    va_end(args);
    return 0;
}

/*
 * Memory allocation
 *
 * Because of realloc, we need to keep track of the size of the allocated
 * buffer (need to copy the old contents to the new buffer). So, allocate
 * extra space for a size_t, store the allocated size in there and return
 * the address just past it as the allocated buffer.
 * On win64 we need to align the allocation to 16 bytes, otherwise 8 bytes.
 */
typedef struct _MALLOC_HEADER
{
    SIZE_T Size;
    SIZE_T Alignment;
} MALLOC_HEADER, * PMALLOC_HEADER;

void *
malloc(size_t Size)
{
    PMALLOC_HEADER Header;

    Header = EngAllocMem(0, sizeof(MALLOC_HEADER) + Size, TAG_FREETYPE);
    if (Header == NULL)
    {
        return NULL;
    }

    Header->Size = Size;
    Header->Alignment = -1;
    return (Header + 1);
}

void *
realloc(void *Object, size_t Size)
{
    PVOID NewObject;
    PMALLOC_HEADER OldHeader;
    size_t CopySize;

    NewObject = malloc(Size);
    if (NewObject == NULL)
    {
        return NULL;
    }

    if (Object == NULL)
    {
        return NewObject;
    }

    OldHeader = (PMALLOC_HEADER)Object - 1;
    CopySize = min(OldHeader->Size, Size);
    memcpy(NewObject, Object, CopySize);

    free(Object);

    return NewObject;
}

void
free(void *Object)
{
    if (Object != NULL)
    {
        EngFreeMem((PMALLOC_HEADER)Object - 1);
    }
}

/*
 * File I/O
 *
 * This is easy, we don't want FreeType to do any I/O. So return an
 * error on each I/O attempt. Note that errno is not being set, it is
 * not used by FreeType.
 */

FILE *
fopen(const char *FileName, const char *Mode)
{
    DPRINT1("Freetype tries to open file %s\n", FileName);
    return NULL;
}

int
fseek(FILE *Stream, long Offset, int Origin)
{
    DPRINT1("Doubleplus ungood: freetype shouldn't fseek!\n");
    return -1;
}

long
ftell(FILE *Stream)
{
    DPRINT1("Doubleplus ungood: freetype shouldn't ftell!\n");
    return -1;
}

size_t
fread(void *Buffer, size_t Size, size_t Count, FILE *Stream)
{
    DPRINT1("Doubleplus ungood: freetype shouldn't fread!\n");
    return 0;
}

int
fclose(FILE *Stream)
{
    DPRINT1("Doubleplus ungood: freetype shouldn't fclose!\n");
    return EOF;
}
