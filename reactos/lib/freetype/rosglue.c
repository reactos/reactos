/* $Id: rosglue.c,v 1.1 2003/04/01 08:38:28 gvg Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           FreeType implementation for ReactOS
 * PURPOSE:           Glue functions between FreeType
 * FILE:              thirdparty/freetype/rosglue.c
 * PROGRAMMER:        Ge van Geldorp (ge@gse.nl)
 * NOTES:
 */

#include <ddk/ntddk.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NDEBUG
#include <debug.h>

#define TAG_FREETYPE  TAG('F', 'T', 'Y', 'P')

/*
 * First some generic routines
 */

void *
memcpy(void *Dest, const void *Source, size_t Size)
{
  RtlMoveMemory(Dest, Source, Size);

  return Dest;
}


int
memcmp(const void *p1, const void *p2, size_t Size)
{
   unsigned int i;
   const char* s1;
   const char* s2;
   
   s1 = (const char *) p1;
   s2 = (const char *) p2;
   for (i = 0; i < Size; i++)
     {
	if ((*s1) != (*s2))
	  {
	     return((*s1) - (*s2));
	  }
	s1++;
	s2++;
     }
   return(0);
}

/* Hopefully we're only passed ASCII characters.... */
int
isalnum(int c)
{
  assert(0x20 <= c && c <= 0x7f);

  return (('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
}

/*
 * Memory allocation
 *
 * Because of realloc, we need to keep track of the size of the allocated
 * buffer (need to copy the old contents to the new buffer). So, allocate
 * extra space for a size_t, store the allocated size in there and return
 * the address just past it as the allocated buffer.
 */

void *
malloc(size_t Size)
{
  void *Object;

  Object = ExAllocatePoolWithTag(NonPagedPool, sizeof(size_t) + Size, TAG_FREETYPE);
  if (NULL != Object)
    {
    *((size_t *) Object) = Size;
    Object = (void *)((size_t *) Object + 1);
    }
    
  return Object;
}

void *
realloc(void *Object, size_t Size)
{
  void *NewObject;
  size_t CopySize;

  NewObject = ExAllocatePoolWithTag(NonPagedPool, sizeof(size_t) + Size, TAG_FREETYPE);
  if (NULL != NewObject)
    {
    *((size_t *) NewObject) = Size;
    NewObject = (void *)((size_t *) NewObject + 1);
    CopySize = *((size_t *) Object - 1);
    if (Size < CopySize)
      {
      CopySize = Size;
      }
    memcpy(NewObject, Object, CopySize);
    ExFreePool((size_t *) Object - 1);
    }

  return NewObject;
}

void
free(void *Object)
{
  ExFreePool((size_t *) Object - 1);
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
