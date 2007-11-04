/*
 * Unit test suite for memory functions
 *
 * Copyright 2003 Dimitrie O. Paun
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

#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include "wine/test.h"

static void (*p_aligned_free)(void*) = NULL;
static void * (*p_aligned_malloc)(size_t,size_t) = NULL;
static void * (*p_aligned_offset_malloc)(size_t,size_t,size_t) = NULL;
static void * (*p_aligned_realloc)(void*,size_t,size_t) = NULL;
static void * (*p_aligned_offset_realloc)(void*,size_t,size_t,size_t) = NULL;

static void test_aligned_malloc(size_t size, size_t alignment)
{
    void *mem;

    mem = p_aligned_malloc(size, alignment);

    if ((alignment & (alignment - 1)) == 0)
        ok(mem != NULL, "_aligned_malloc(%d, %d) failed\n", size, alignment);
    else
        ok(mem == NULL, "_aligned_malloc(%d, %d) should have failed\n", size, alignment);

    if (mem)
    {
        ok(((DWORD_PTR)mem & (alignment ? alignment - 1 : 0)) == 0,
           "_aligned_malloc(%d, %d) not aligned: %p\n", size, alignment, mem);
        if (winetest_debug > 1)
        {
            void *saved;
            saved = *(void **)((DWORD_PTR)((char *)mem - sizeof(void *)) & ~(sizeof(void *) - 1));
            trace("_aligned_malloc(%3d, %3d) returns %p, saved = %p, diff = %d\n",
                  size, alignment, mem, saved, (char *)saved - (char *)mem);
        }
        p_aligned_free(mem);
    }
    else
        ok(errno == EINVAL, "_aligned_malloc(%d, %d) errno: %d != %d\n", size, alignment, errno, EINVAL);
}

static void test_aligned_offset_malloc(size_t size, size_t alignment, size_t offset)
{
    void *mem;

    mem = p_aligned_offset_malloc(size, alignment, offset);

    if ((alignment & (alignment - 1)) == 0)
        if (offset < size)
            ok(mem != NULL, "_aligned_offset_malloc(%d, %d, %d) failed\n", size, alignment, offset);
        else
            ok(errno == EINVAL, "_aligned_offset_malloc(%d, %d, %d) errno: %d != %d\n", size, alignment, offset, errno, EINVAL);
    else
        ok(mem == NULL, "_aligned_offset_malloc(%d, %d, %d) should have failed\n", size, alignment, offset);

    if (mem)
    {
        ok(((DWORD_PTR)((char *)mem + offset) & (alignment ? alignment - 1 : 0)) == 0,
           "_aligned_offset_malloc(%d, %d, %d) not aligned: %p\n", size, alignment, offset, mem);
        if (winetest_debug > 1)
        {
            void *saved;
            saved = *(void **)((DWORD_PTR)((char *)mem - sizeof(void *)) & ~(sizeof(void *) - 1));
            trace("_aligned_offset_malloc(%3d, %3d, %3d) returns %p, saved = %p, diff = %d\n",
                  size, alignment, offset, mem, saved, (char *)saved - (char *)mem);
        }
        p_aligned_free(mem);
    }
    else
        ok(errno == EINVAL, "_aligned_offset_malloc(%d, %d, %d) errno: %d != %d\n", size, alignment, offset, errno, EINVAL);
}

static void test_aligned_realloc(size_t size1, size_t size2, size_t alignment)
{
    void *mem, *mem1, *mem2;

    mem = p_aligned_malloc(size1, alignment);

    if ((alignment & (alignment - 1)) == 0)
        ok(mem != NULL, "_aligned_malloc(%d, %d) failed\n", size1, alignment);
    else
        ok(mem == NULL, "_aligned_malloc(%d, %d) should have failed\n", size1, alignment);

    if (mem)
    {
	mem1 = malloc(size1);
        if (mem1)
        {
            int i;
            for (i = 0; i < size1; i++)
                ((char *)mem)[i] = i + 1;
            memcpy(mem1, mem, size1);
        }

        ok(((DWORD_PTR)mem & (alignment ? alignment - 1 : 0)) == 0,
           "_aligned_malloc(%d, %d) not aligned: %p\n", size1, alignment, mem);
        if (winetest_debug > 1)
        {
            void *saved;
            saved = *(void **)((DWORD_PTR)((char *)mem - sizeof(void *)) & ~(sizeof(void *) - 1));
            trace("_aligned_malloc(%3d, %3d) returns %p, saved = %p, diff = %d\n",
                  size1, alignment, mem, saved, (char *)saved - (char *)mem);
        }

        mem2 = p_aligned_realloc(mem, size2, alignment);

        ok(mem2 != NULL, "_aligned_realloc(%p, %d, %d) failed\n", mem, size2, alignment);

        if (mem2)
        {
            ok(((DWORD_PTR)mem2 & (alignment ? alignment - 1 : 0)) == 0,
               "_aligned_realloc(%p, %d, %d) not aligned: %p\n", mem, size2, alignment, mem2);
            if (winetest_debug > 1)
            {
                void *saved;
                saved = *(void **)((DWORD_PTR)((char *)mem2 - sizeof(void *)) & ~(sizeof(void *) - 1));
                trace("_aligned_realloc(%p, %3d, %3d) returns %p, saved = %p, diff = %d\n",
                      mem, size2, alignment, mem2, saved, (char *)saved - (char *)mem2);
            }
            if (mem1)
            {
                ok(memcmp(mem2, mem1, min(size1, size2))==0, "_aligned_realloc(%p, %d, %d) has different data\n", mem, size2, alignment);
                if (memcmp(mem2, mem1, min(size1, size2)) && winetest_debug > 1)
                {
                    int i;
                    for (i = 0; i < min(size1, size2); i++)
                    {
                        if (((char *)mem2)[i] != ((char *)mem1)[i])
                            trace("%d: %02x != %02x\n", i, ((char *)mem2)[i] & 0xff, ((char *)mem1)[i] & 0xff);
                    }
                }
            }
        }
        else
            ok(errno == EINVAL, "_aligned_realloc(%p, %d, %d) errno: %d != %d\n", mem, size2, alignment, errno, EINVAL);

        p_aligned_free(mem);
        free(mem1);
    }
    else
        ok(errno == EINVAL, "_aligned_malloc(%d, %d) errno: %d != %d\n", size1, alignment, errno, EINVAL);
}

static void test_aligned_offset_realloc(size_t size1, size_t size2, size_t alignment, size_t offset)
{
    void *mem, *mem1, *mem2;

    mem = p_aligned_offset_malloc(size1, alignment, offset);

    if ((alignment & (alignment - 1)) == 0)
        ok(mem != NULL, "_aligned_offset_malloc(%d, %d, %d) failed\n", size1, alignment, offset);
    else
        ok(mem == NULL, "_aligned_offset_malloc(%d, %d, %d) should have failed\n", size1, alignment, offset);

    if (mem)
    {
	mem1 = malloc(size1);
        if (mem1)
        {
            int i;
            for (i = 0; i < size1; i++)
                ((char *)mem)[i] = i + 1;
            memcpy(mem1, mem, size1);
        }

        ok(((DWORD_PTR)((char *)mem + offset) & (alignment ? alignment - 1 : 0)) == 0,
           "_aligned_offset_malloc(%d, %d, %d) not aligned: %p\n", size1, alignment, offset, mem);
        if (winetest_debug > 1)
        {
            void *saved;
            saved = *(void **)((DWORD_PTR)((char *)mem - sizeof(void *)) & ~(sizeof(void *) - 1));
            trace("_aligned_offset_malloc(%3d, %3d, %3d) returns %p, saved = %p, diff = %d\n",
                  size1, alignment, offset, mem, saved, (char *)saved - (char *)mem);
        }

        mem2 = p_aligned_offset_realloc(mem, size2, alignment, offset);

        ok(mem2 != NULL, "_aligned_offset_realloc(%p, %d, %d, %d) failed\n", mem, size2, alignment, offset);

        if (mem2)
        {
            ok(((DWORD_PTR)((char *)mem + offset) & (alignment ? alignment - 1 : 0)) == 0,
               "_aligned_offset_realloc(%p, %d, %d, %d) not aligned: %p\n", mem, size2, alignment, offset, mem2);
            if (winetest_debug > 1)
            {
                void *saved;
                saved = *(void **)((DWORD_PTR)((char *)mem2 - sizeof(void *)) & ~(sizeof(void *) - 1));
                trace("_aligned_offset_realloc(%p, %3d, %3d, %3d) returns %p, saved = %p, diff = %d\n",
                      mem, size2, alignment, offset, mem2, saved, (char *)saved - (char *)mem2);
            }
            if (mem1)
            {
                ok(memcmp(mem2, mem1, min(size1, size2))==0, "_aligned_offset_realloc(%p, %d, %d, %d) has different data\n", mem, size2, alignment, offset);
                if (memcmp(mem2, mem1, min(size1, size2)) && winetest_debug > 1)
                {
                    int i;
                    for (i = 0; i < min(size1, size2); i++)
                    {
                        if (((char *)mem2)[i] != ((char *)mem1)[i])
                            trace("%d: %02x != %02x\n", i, ((char *)mem2)[i] & 0xff, ((char *)mem1)[i] & 0xff);
                    }
                }
            }
        }
        else
            ok(errno == EINVAL, "_aligned_offset_realloc(%p, %d, %d, %d) errno: %d != %d\n", mem, size2, alignment, offset, errno, EINVAL);

        p_aligned_free(mem);
        free(mem1);
    }
    else
        ok(errno == EINVAL, "_aligned_offset_malloc(%d, %d) errno: %d != %d\n", size1, alignment, errno, EINVAL);
}

static void test_aligned(void)
{
    HMODULE msvcrt = GetModuleHandle("msvcrt.dll");

    if (msvcrt == NULL)
        return;

    p_aligned_free = (void*)GetProcAddress(msvcrt, "_aligned_free");
    p_aligned_malloc = (void*)GetProcAddress(msvcrt, "_aligned_malloc");
    p_aligned_offset_malloc = (void*)GetProcAddress(msvcrt, "_aligned_offset_malloc");
    p_aligned_realloc = (void*)GetProcAddress(msvcrt, "_aligned_realloc");
    p_aligned_offset_realloc = (void*)GetProcAddress(msvcrt, "_aligned_offset_realloc");

    if (!p_aligned_free || !p_aligned_malloc || !p_aligned_offset_malloc || !p_aligned_realloc || !p_aligned_offset_realloc)
    {
        skip("aligned memory tests skipped\n");
        return;
    }

    test_aligned_malloc(256, 0);
    test_aligned_malloc(256, 1);
    test_aligned_malloc(256, 2);
    test_aligned_malloc(256, 3);
    test_aligned_malloc(256, 4);
    test_aligned_malloc(256, 8);
    test_aligned_malloc(256, 16);
    test_aligned_malloc(256, 32);
    test_aligned_malloc(256, 64);
    test_aligned_malloc(256, 127);
    test_aligned_malloc(256, 128);

    test_aligned_offset_malloc(256, 0, 0);
    test_aligned_offset_malloc(256, 1, 0);
    test_aligned_offset_malloc(256, 2, 0);
    test_aligned_offset_malloc(256, 3, 0);
    test_aligned_offset_malloc(256, 4, 0);
    test_aligned_offset_malloc(256, 8, 0);
    test_aligned_offset_malloc(256, 16, 0);
    test_aligned_offset_malloc(256, 32, 0);
    test_aligned_offset_malloc(256, 64, 0);
    test_aligned_offset_malloc(256, 127, 0);
    test_aligned_offset_malloc(256, 128, 0);

    test_aligned_offset_malloc(256, 0, 4);
    test_aligned_offset_malloc(256, 1, 4);
    test_aligned_offset_malloc(256, 2, 4);
    test_aligned_offset_malloc(256, 3, 4);
    test_aligned_offset_malloc(256, 4, 4);
    test_aligned_offset_malloc(256, 8, 4);
    test_aligned_offset_malloc(256, 16, 4);
    test_aligned_offset_malloc(256, 32, 4);
    test_aligned_offset_malloc(256, 64, 4);
    test_aligned_offset_malloc(256, 127, 4);
    test_aligned_offset_malloc(256, 128, 4);

    test_aligned_offset_malloc(256, 8, 7);
    test_aligned_offset_malloc(256, 8, 9);
    test_aligned_offset_malloc(256, 8, 16);
    test_aligned_offset_malloc(256, 8, 17);
    test_aligned_offset_malloc(256, 8, 254);
    test_aligned_offset_malloc(256, 8, 255);
    test_aligned_offset_malloc(256, 8, 256);
    test_aligned_offset_malloc(256, 8, 512);

    test_aligned_realloc(256, 512, 0);
    test_aligned_realloc(256, 128, 0);
    test_aligned_realloc(256, 512, 4);
    test_aligned_realloc(256, 128, 4);
    test_aligned_realloc(256, 512, 8);
    test_aligned_realloc(256, 128, 8);
    test_aligned_realloc(256, 512, 16);
    test_aligned_realloc(256, 128, 16);
    test_aligned_realloc(256, 512, 32);
    test_aligned_realloc(256, 128, 32);
    test_aligned_realloc(256, 512, 64);
    test_aligned_realloc(256, 128, 64);

    test_aligned_offset_realloc(256, 512, 0, 0);
    test_aligned_offset_realloc(256, 128, 0, 0);
    test_aligned_offset_realloc(256, 512, 4, 0);
    test_aligned_offset_realloc(256, 128, 4, 0);
    test_aligned_offset_realloc(256, 512, 8, 0);
    test_aligned_offset_realloc(256, 128, 8, 0);
    test_aligned_offset_realloc(256, 512, 16, 0);
    test_aligned_offset_realloc(256, 128, 16, 0);
    test_aligned_offset_realloc(256, 512, 32, 0);
    test_aligned_offset_realloc(256, 128, 32, 0);
    test_aligned_offset_realloc(256, 512, 64, 0);
    test_aligned_offset_realloc(256, 128, 64, 0);

    test_aligned_offset_realloc(256, 512, 0, 4);
    test_aligned_offset_realloc(256, 128, 0, 4);
    test_aligned_offset_realloc(256, 512, 4, 4);
    test_aligned_offset_realloc(256, 128, 4, 4);
    test_aligned_offset_realloc(256, 512, 8, 4);
    test_aligned_offset_realloc(256, 128, 8, 4);
    test_aligned_offset_realloc(256, 512, 16, 4);
    test_aligned_offset_realloc(256, 128, 16, 4);
    test_aligned_offset_realloc(256, 512, 32, 4);
    test_aligned_offset_realloc(256, 128, 32, 4);
    test_aligned_offset_realloc(256, 512, 64, 4);
    test_aligned_offset_realloc(256, 128, 64, 4);
}

START_TEST(heap)
{
    void *mem;

    mem = malloc(0);
    ok(mem != NULL, "memory not allocated for size 0\n");

    mem = realloc(NULL, 10);
    ok(mem != NULL, "memory not allocated\n");

    mem = realloc(mem, 20);
    ok(mem != NULL, "memory not reallocated\n");

    mem = realloc(mem, 0);
    ok(mem == NULL, "memory not freed\n");

    mem = realloc(NULL, 0);
    ok(mem != NULL, "memory not (re)allocated for size 0\n");

    test_aligned();
}
