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

static void (__cdecl *p_aligned_free)(void*);
static void * (__cdecl *p_aligned_malloc)(size_t,size_t);
static void * (__cdecl *p_aligned_offset_malloc)(size_t,size_t,size_t);
static void * (__cdecl *p_aligned_realloc)(void*,size_t,size_t);
static void * (__cdecl *p_aligned_offset_realloc)(void*,size_t,size_t,size_t);
static int (__cdecl *p__set_sbh_threshold)(size_t);
static size_t (__cdecl *p__get_sbh_threshold)(void);

static void test_aligned_malloc(unsigned int size, unsigned int alignment)
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
            trace("_aligned_malloc(%3d, %3d) returns %p, saved = %p\n", size, alignment, mem, saved );
        }
        p_aligned_free(mem);
    }
    else
        ok(errno == EINVAL, "_aligned_malloc(%d, %d) errno: %d != %d\n", size, alignment, errno, EINVAL);
}

static void test_aligned_offset_malloc(unsigned int size, unsigned int alignment, unsigned int offset)
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
            trace("_aligned_offset_malloc(%3d, %3d, %3d) returns %p, saved = %p\n",
                  size, alignment, offset, mem, saved);
        }
        p_aligned_free(mem);
    }
    else
        ok(errno == EINVAL, "_aligned_offset_malloc(%d, %d, %d) errno: %d != %d\n", size, alignment, offset, errno, EINVAL);
}

static void test_aligned_realloc(unsigned int size1, unsigned int size2, unsigned int alignment)
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
            unsigned int i;
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
            trace("_aligned_malloc(%3d, %3d) returns %p, saved = %p\n", size1, alignment, mem, saved);
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
                trace("_aligned_realloc(%p, %3d, %3d) returns %p, saved = %p\n",
                      mem, size2, alignment, mem2, saved);
            }
            if (mem1)
            {
                ok(memcmp(mem2, mem1, min(size1, size2))==0, "_aligned_realloc(%p, %d, %d) has different data\n", mem, size2, alignment);
                if (memcmp(mem2, mem1, min(size1, size2)) && winetest_debug > 1)
                {
                    unsigned int i;
                    for (i = 0; i < min(size1, size2); i++)
                    {
                        if (((char *)mem2)[i] != ((char *)mem1)[i])
                            trace("%d: %02x != %02x\n", i, ((char *)mem2)[i] & 0xff, ((char *)mem1)[i] & 0xff);
                    }
                }
            }
            p_aligned_free(mem2);
        } else {
            ok(errno == EINVAL, "_aligned_realloc(%p, %d, %d) errno: %d != %d\n", mem, size2, alignment, errno, EINVAL);
            p_aligned_free(mem);
        }

        free(mem1);
    }
    else
        ok(errno == EINVAL, "_aligned_malloc(%d, %d) errno: %d != %d\n", size1, alignment, errno, EINVAL);
}

static void test_aligned_offset_realloc(unsigned int size1, unsigned int size2,
                                        unsigned int alignment, unsigned int offset)
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
            unsigned int i;
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
            trace("_aligned_offset_malloc(%3d, %3d, %3d) returns %p, saved = %p\n",
                  size1, alignment, offset, mem, saved);
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
                trace("_aligned_offset_realloc(%p, %3d, %3d, %3d) returns %p, saved = %p\n",
                      mem, size2, alignment, offset, mem2, saved);
            }
            if (mem1)
            {
                ok(memcmp(mem2, mem1, min(size1, size2))==0, "_aligned_offset_realloc(%p, %d, %d, %d) has different data\n", mem, size2, alignment, offset);
                if (memcmp(mem2, mem1, min(size1, size2)) && winetest_debug > 1)
                {
                    unsigned int i;
                    for (i = 0; i < min(size1, size2); i++)
                    {
                        if (((char *)mem2)[i] != ((char *)mem1)[i])
                            trace("%d: %02x != %02x\n", i, ((char *)mem2)[i] & 0xff, ((char *)mem1)[i] & 0xff);
                    }
                }
            }
            p_aligned_free(mem2);
        } else {
            ok(errno == EINVAL, "_aligned_offset_realloc(%p, %d, %d, %d) errno: %d != %d\n", mem, size2, alignment, offset, errno, EINVAL);
            p_aligned_free(mem);
        }

        free(mem1);
    }
    else
        ok(errno == EINVAL, "_aligned_offset_malloc(%d, %d) errno: %d != %d\n", size1, alignment, errno, EINVAL);
}

static void test_aligned(void)
{
    HMODULE msvcrt = GetModuleHandleA("msvcrt.dll");

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

    test_aligned_offset_realloc(256, 512, 0, 8);
    test_aligned_offset_realloc(256, 128, 0, 8);
    test_aligned_offset_realloc(256, 512, 4, 8);
    test_aligned_offset_realloc(256, 128, 4, 8);
    test_aligned_offset_realloc(256, 512, 8, 8);
    test_aligned_offset_realloc(256, 128, 8, 8);
    test_aligned_offset_realloc(256, 512, 16, 8);
    test_aligned_offset_realloc(256, 128, 16, 8);
    test_aligned_offset_realloc(256, 512, 32, 8);
    test_aligned_offset_realloc(256, 128, 32, 8);
    test_aligned_offset_realloc(256, 512, 64, 8);
    test_aligned_offset_realloc(256, 128, 64, 8);

    test_aligned_offset_realloc(256, 512, 0, 16);
    test_aligned_offset_realloc(256, 128, 0, 16);
    test_aligned_offset_realloc(256, 512, 4, 16);
    test_aligned_offset_realloc(256, 128, 4, 16);
    test_aligned_offset_realloc(256, 512, 8, 16);
    test_aligned_offset_realloc(256, 128, 8, 16);
    test_aligned_offset_realloc(256, 512, 16, 16);
    test_aligned_offset_realloc(256, 128, 16, 16);
    test_aligned_offset_realloc(256, 512, 32, 16);
    test_aligned_offset_realloc(256, 128, 32, 16);
    test_aligned_offset_realloc(256, 512, 64, 16);
    test_aligned_offset_realloc(256, 128, 64, 16);

    test_aligned_offset_realloc(256, 512, 0, 32);
    test_aligned_offset_realloc(256, 128, 0, 32);
    test_aligned_offset_realloc(256, 512, 4, 32);
    test_aligned_offset_realloc(256, 128, 4, 32);
    test_aligned_offset_realloc(256, 512, 8, 32);
    test_aligned_offset_realloc(256, 128, 8, 32);
    test_aligned_offset_realloc(256, 512, 16, 32);
    test_aligned_offset_realloc(256, 128, 16, 32);
    test_aligned_offset_realloc(256, 512, 32, 32);
    test_aligned_offset_realloc(256, 128, 32, 32);
    test_aligned_offset_realloc(256, 512, 64, 32);
    test_aligned_offset_realloc(256, 128, 64, 32);

    test_aligned_offset_realloc(256, 512, 0, 64);
    test_aligned_offset_realloc(256, 128, 0, 64);
    test_aligned_offset_realloc(256, 512, 4, 64);
    test_aligned_offset_realloc(256, 128, 4, 64);
    test_aligned_offset_realloc(256, 512, 8, 64);
    test_aligned_offset_realloc(256, 128, 8, 64);
    test_aligned_offset_realloc(256, 512, 16, 64);
    test_aligned_offset_realloc(256, 128, 16, 64);
    test_aligned_offset_realloc(256, 512, 32, 64);
    test_aligned_offset_realloc(256, 128, 32, 64);
    test_aligned_offset_realloc(256, 512, 64, 64);
    test_aligned_offset_realloc(256, 128, 64, 64);

    test_aligned_offset_realloc(256, 512, 0, 96);
    test_aligned_offset_realloc(256, 128, 0, 96);
    test_aligned_offset_realloc(256, 512, 4, 96);
    test_aligned_offset_realloc(256, 128, 4, 96);
    test_aligned_offset_realloc(256, 512, 8, 96);
    test_aligned_offset_realloc(256, 128, 8, 96);
    test_aligned_offset_realloc(256, 512, 16, 96);
    test_aligned_offset_realloc(256, 128, 16, 96);
    test_aligned_offset_realloc(256, 512, 32, 96);
    test_aligned_offset_realloc(256, 128, 32, 96);
    test_aligned_offset_realloc(256, 512, 64, 96);
    test_aligned_offset_realloc(256, 128, 64, 96);

    test_aligned_offset_realloc(256, 512, 0, 112);
    test_aligned_offset_realloc(256, 128, 0, 112);
    test_aligned_offset_realloc(256, 512, 4, 112);
    test_aligned_offset_realloc(256, 128, 4, 112);
    test_aligned_offset_realloc(256, 512, 8, 112);
    test_aligned_offset_realloc(256, 128, 8, 112);
    test_aligned_offset_realloc(256, 512, 16, 112);
    test_aligned_offset_realloc(256, 128, 16, 112);
    test_aligned_offset_realloc(256, 512, 32, 112);
    test_aligned_offset_realloc(256, 128, 32, 112);
    test_aligned_offset_realloc(256, 512, 64, 112);
    test_aligned_offset_realloc(256, 128, 64, 112);
}

static void test_sbheap(void)
{
    HMODULE msvcrt = GetModuleHandleA("msvcrt.dll");
    void *mem;
    int threshold;

    p__set_sbh_threshold = (void*)GetProcAddress(msvcrt, "_set_sbh_threshold");
    p__get_sbh_threshold = (void*)GetProcAddress(msvcrt, "_get_sbh_threshold");
    if (!p__set_sbh_threshold || !p__get_sbh_threshold)
    {
        win_skip("_set_sbh_threshold not available\n");
        return;
    }

    if(sizeof(void*) == 8) {
        ok(!p__set_sbh_threshold(0), "_set_sbh_threshold succeeded\n");
        ok(!p__set_sbh_threshold(1000), "_set_sbh_threshold succeeded\n");
        return;
    }

    mem = malloc(1);
    ok(mem != NULL, "malloc failed\n");

    ok(p__set_sbh_threshold(1), "_set_sbh_threshold failed\n");
    threshold = p__get_sbh_threshold();
    ok(threshold == 16, "threshold = %d\n", threshold);

    ok(p__set_sbh_threshold(8), "_set_sbh_threshold failed\n");
    threshold = p__get_sbh_threshold();
    ok(threshold == 16, "threshold = %d\n", threshold);

    ok(p__set_sbh_threshold(1000), "_set_sbh_threshold failed\n");
    threshold = p__get_sbh_threshold();
    ok(threshold == 1008, "threshold = %d\n", threshold);

    free(mem);

    mem = malloc(1);
    ok(mem != NULL, "malloc failed\n");
    ok(!((UINT_PTR)mem & 0xf), "incorrect alignment (%p)\n", mem);

    mem = realloc(mem, 10);
    ok(mem != NULL, "realloc failed\n");
    ok(!((UINT_PTR)mem & 0xf), "incorrect alignment (%p)\n", mem);

    ok(p__set_sbh_threshold(0), "_set_sbh_threshold failed\n");
    threshold = p__get_sbh_threshold();
    ok(threshold == 0, "threshold = %d\n", threshold);

    free(mem);
}

static void test_malloc(void)
{
    /* use function pointers to bypass gcc builtins */
    void *(__cdecl *p_malloc)(size_t);
    void *(__cdecl *p_realloc)(void *,size_t);
    void *mem;

    p_malloc = (void *)GetProcAddress( GetModuleHandleA("msvcrt.dll"), "malloc");
    p_realloc = (void *)GetProcAddress( GetModuleHandleA("msvcrt.dll"), "realloc");

    mem = p_malloc(0);
    ok(mem != NULL, "memory not allocated for size 0\n");
    free(mem);

    mem = p_realloc(NULL, 10);
    ok(mem != NULL, "memory not allocated\n");

    mem = p_realloc(mem, 20);
    ok(mem != NULL, "memory not reallocated\n");

    mem = p_realloc(mem, 0);
    ok(mem == NULL, "memory not freed\n");

    mem = p_realloc(NULL, 0);
    ok(mem != NULL, "memory not (re)allocated for size 0\n");

    free(mem);
}

static void test_calloc(void)
{
    /* use function pointer to bypass gcc builtin */
    void *(__cdecl *p_calloc)(size_t, size_t);
    void *ptr;

    p_calloc = (void *)GetProcAddress( GetModuleHandleA("msvcrt.dll"), "calloc");

    ptr = p_calloc(1, 0);
    ok(ptr != NULL, "got %p\n", ptr);
    free(ptr);

    ptr = p_calloc(0, 0);
    ok(ptr != NULL, "got %p\n", ptr);
    free(ptr);

    ptr = p_calloc(0, 1);
    ok(ptr != NULL, "got %p\n", ptr);
    free(ptr);

    errno = 0;
    ptr = p_calloc(~(size_t)0 / 2, ~(size_t)0 / 2);
    ok(ptr == NULL || broken(ptr != NULL) /* winxp sp0 */, "got %p\n", ptr);
    ok(errno == ENOMEM || broken(errno == 0) /* winxp, win2k3 */, "got errno %d\n", errno);
    free(ptr);
}

static void test__get_heap_handle(void)
{
    ok((HANDLE)_get_heap_handle() != GetProcessHeap(), "Expected _get_heap_handle() not to return GetProcessHeap()\n");
}

START_TEST(heap)
{
    test_aligned();
    test_sbheap();
    test_malloc();
    test_calloc();
    test__get_heap_handle();
}
