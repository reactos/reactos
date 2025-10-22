/*
 * Unit test suite for exception unwinding
 *
 * Copyright 2009, 2024 Alexandre Julliard
 * Copyright 2020, 2021 Martin Storsjö
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

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winnt.h"
#include "winternl.h"
#include "rtlsupportapi.h"
#include "wine/test.h"

#ifndef __i386__

static void *code_mem;
static HMODULE ntdll;

static PRUNTIME_FUNCTION (WINAPI *pRtlLookupFunctionEntry)(ULONG_PTR, ULONG_PTR*, UNWIND_HISTORY_TABLE*);
static PRUNTIME_FUNCTION (WINAPI *pRtlLookupFunctionTable)(ULONG_PTR, ULONG_PTR*, ULONG*);
static BOOLEAN   (CDECL *pRtlInstallFunctionTableCallback)(DWORD64, DWORD64, DWORD, PGET_RUNTIME_FUNCTION_CALLBACK, PVOID, PCWSTR);
static BOOLEAN   (CDECL  *pRtlAddFunctionTable)(RUNTIME_FUNCTION*, DWORD, DWORD64);
static BOOLEAN   (CDECL  *pRtlDeleteFunctionTable)(RUNTIME_FUNCTION*);
static DWORD     (WINAPI *pRtlAddGrowableFunctionTable)(void**, RUNTIME_FUNCTION*, DWORD, DWORD, ULONG_PTR, ULONG_PTR);
static void      (WINAPI *pRtlGrowFunctionTable)(void*, DWORD);
static void      (WINAPI *pRtlDeleteGrowableFunctionTable)(void*);
static NTSTATUS  (WINAPI *pRtlVirtualUnwind2)(ULONG,ULONG_PTR,ULONG_PTR,RUNTIME_FUNCTION*,CONTEXT*,BOOLEAN*,void**,ULONG_PTR*,KNONVOLATILE_CONTEXT_POINTERS*,ULONG_PTR*,ULONG_PTR*,PEXCEPTION_ROUTINE*,ULONG);
static NTSTATUS  (WINAPI *pNtAllocateVirtualMemoryEx)(HANDLE,PVOID*,SIZE_T*,ULONG,ULONG,MEM_EXTENDED_PARAMETER*,ULONG);

#ifdef __arm__

#define UWOP_TWOBYTES(x)   (((x) >> 8) & 0xff), ((x) & 0xff)
#define UWOP_THREEBYTES(x) (((x) >> 16) & 0xff), (((x) >> 8) & 0xff), ((x) & 0xff)
#define UWOP_FOURBYTES(x)  (((x) >> 24) & 0xff), (((x) >> 16) & 0xff), (((x) >> 8) & 0xff), ((x) & 0xff)

#define UWOP_ALLOC_SMALL(size)         (0x00 | (size/4)) /* Max 0x7f * 4 */
#define UWOP_SAVE_REGSW(regmask)       UWOP_TWOBYTES((0x80 << 8) | (regmask))
#define UWOP_SET_FP(reg)               (0xC0 | reg)
#define UWOP_SAVE_RANGE_4_7_LR(reg,lr) (0xD0 | (reg - 4) | ((lr) ? 0x04 : 0))
#define UWOP_SAVE_RANGE_4_11_LR(reg,lr)(0xD8 | (reg - 8) | ((lr) ? 0x04 : 0))
#define UWOP_SAVE_D8_RANGE(reg)        (0xE0 | (reg - 8))
#define UWOP_ALLOC_MEDIUMW(size)       UWOP_TWOBYTES((0xE8 << 8) | (size/4)) /* Max 0x3ff * 4 */
#define UWOP_SAVE_REGS(regmask)        UWOP_TWOBYTES((0xEC << 8) | ((regmask) & 0xFF) | (((regmask) & (1<<lr)) ? 0x100 : 0))
#define UWOP_SAVE_LR(offset)           UWOP_TWOBYTES((0xEF << 8) | (offset/4))
#define UWOP_SAVE_D0_RANGE(first,last) UWOP_TWOBYTES((0xF5 << 8) | (first << 4) | (last))
#define UWOP_SAVE_D16_RANGE(first,last)UWOP_TWOBYTES((0xF6 << 8) | ((first - 16) << 4) | (last - 16))
#define UWOP_ALLOC_LARGE(size)         UWOP_THREEBYTES((0xF7 << 16) | (size/4))
#define UWOP_ALLOC_HUGE(size)          UWOP_FOURBYTES((0xF8u << 24) | (size/4))
#define UWOP_ALLOC_LARGEW(size)        UWOP_THREEBYTES((0xF9 << 16) | (size/4))
#define UWOP_ALLOC_HUGEW(size)         UWOP_FOURBYTES((0xFAu << 24) | (size/4))
#define UWOP_MSFT_OP_MACHINE_FRAME     0xEE,0x01
#define UWOP_MSFT_OP_CONTEXT           0xEE,0x02
#define UWOP_NOP16                     0xFB
#define UWOP_NOP32                     0xFC
#define UWOP_END_NOP16                 0xFD
#define UWOP_END_NOP32                 0xFE
#define UWOP_END                       0xFF

struct results_arm
{
    int pc_offset;      /* pc offset from code start */
    int fp_offset;      /* fp offset from stack pointer */
    int handler;        /* expect handler to be set? */
    ULONG_PTR pc;       /* expected final pc value */
    ULONG_PTR frame;    /* expected frame return value */
    int frame_offset;   /* whether the frame return value is an offset or an absolute value */
    LONGLONG regs[47][2];/* expected values for registers */
};

struct unwind_test_arm
{
    const BYTE *function;
    size_t function_size;
    const BYTE *unwind_info;
    size_t unwind_size;
    const struct results_arm *results;
    unsigned int nb_results;
};

enum regs
{
    /* Note, lr and sp are swapped to allow using 'lr' in register bitmasks. */
    r0,  r1,  r2,  r3,  r4,  r5,  r6,  r7,
    r8,  r9,  r10, r11, r12, lr,  sp,
    d0,  d1,  d2,  d3,  d4,  d5,  d6,  d7,
    d8,  d9,  d10, d11, d12, d13, d14, d15,
    d16, d17, d18, d19, d20, d21, d22, d23,
    d24, d25, d26, d27, d28, d29, d30, d31,
};

static const char * const reg_names_arm[47] =
{
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9",  "r10", "r11", "r12", "lr",  "sp",
    "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
    "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
    "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
    "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
};

#define ORIG_LR 0xCCCCCCCC

static void call_virtual_unwind_arm( int testnum, const struct unwind_test_arm *test )
{
    static const int code_offset = 1024;
    static const int unwind_offset = 2048;
    void *data;
    CONTEXT context;
    NTSTATUS status;
    PEXCEPTION_ROUTINE handler;
    RUNTIME_FUNCTION runtime_func;
    KNONVOLATILE_CONTEXT_POINTERS ctx_ptr;
    UINT i, j, k;
    ULONG fake_stack[256];
    ULONG_PTR frame, orig_pc, orig_fp, unset_reg;
    ULONGLONG unset_reg64;
    static const UINT nb_regs = ARRAY_SIZE(test->results[i].regs);

    memcpy( (char *)code_mem + code_offset, test->function, test->function_size );
    if (test->unwind_info)
    {
        memcpy( (char *)code_mem + unwind_offset, test->unwind_info, test->unwind_size );
        runtime_func.BeginAddress = code_offset;
        if (test->unwind_size)
            runtime_func.UnwindData = unwind_offset;
        else
            memcpy(&runtime_func.UnwindData, test->unwind_info, 4);
    }

    for (i = 0; i < test->nb_results; i++)
    {
        memset( &ctx_ptr, 0, sizeof(ctx_ptr) );
        memset( &context, 0x55, sizeof(context) );
        memset( &unset_reg, 0x55, sizeof(unset_reg) );
        memset( &unset_reg64, 0x55, sizeof(unset_reg64) );
        for (j = 0; j < 256; j++) fake_stack[j] = j * 4;

        context.Sp = (ULONG_PTR)fake_stack;
        context.Lr = (ULONG_PTR)ORIG_LR;
        context.R11 = (ULONG_PTR)fake_stack + test->results[i].fp_offset;
        orig_fp = context.R11;
        orig_pc = (ULONG_PTR)code_mem + code_offset + test->results[i].pc_offset;

        trace( "%u/%u: pc=%p (%02x) fp=%p sp=%p\n", testnum, i,
               (void *)orig_pc, *(UINT *)orig_pc, (void *)orig_fp, (void *)context.Sp );

        if (test->results[i].handler == -2) orig_pc = context.Lr;

        if (pRtlVirtualUnwind2)
        {
            CONTEXT new_context = context;

            handler = (void *)0xdeadbeef;
            data = (void *)0xdeadbeef;
            frame = 0xdeadbeef;
            status = pRtlVirtualUnwind2( UNW_FLAG_EHANDLER, (ULONG)code_mem, orig_pc,
                                         test->unwind_info ? &runtime_func : NULL, &new_context,
                                         NULL, &data, &frame, &ctx_ptr, NULL, NULL, &handler, 0 );
            if (test->results[i].handler > 0)
            {
                ok( !status, "RtlVirtualUnwind2 failed %lx\n", status );
                ok( (char *)handler == (char *)code_mem + 0x200,
                    "%u/%u: wrong handler %p/%p\n", testnum, i, handler, (char *)code_mem + 0x200 );
                if (handler) ok( *(DWORD *)data == 0x08070605,
                                 "%u/%u: wrong handler data %lx\n", testnum, i, *(DWORD *)data );
            }
            else if (test->results[i].handler < -1)
            {
                ok( status == STATUS_BAD_FUNCTION_TABLE, "RtlVirtualUnwind2 failed %lx\n", status );
                ok( handler == (void *)0xdeadbeef, "handler set to %p\n", handler );
                ok( data == (void *)0xdeadbeef, "handler data set to %p\n", data );
            }
            else
            {
                ok( !status, "RtlVirtualUnwind2 failed %lx\n", status );
                ok( handler == NULL, "handler %p instead of NULL\n", handler );
                ok( data == NULL, "handler data set to %p\n", data );
            }
        }

        data = (void *)0xdeadbeef;
        frame = 0xdeadbeef;
        handler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, (ULONG)code_mem, orig_pc,
                                    test->unwind_info ? &runtime_func : NULL,
                                    &context, &data, &frame, &ctx_ptr );
        if (test->results[i].handler > 0)
        {
            ok( (char *)handler == (char *)code_mem + 0x200,
                "%u/%u: wrong handler %p/%p\n", testnum, i, handler, (char *)code_mem + 0x200 );
            if (handler) ok( *(DWORD *)data == 0x08070605,
                             "%u/%u: wrong handler data %lx\n", testnum, i, *(DWORD *)data );
        }
        else
        {
            ok( handler == NULL, "%u/%u: handler %p instead of NULL\n", testnum, i, handler );
            ok( data == (test->results[i].handler < -1 ? (void *)0xdeadbeef : NULL),
                "%u/%u: handler data set to %p/%p\n", testnum, i, data,
                (test->results[i].handler < 0 ? (void *)0xdeadbeef : NULL) );
        }

        ok( context.Pc == test->results[i].pc, "%u/%u: wrong pc %p/%p\n",
            testnum, i, (void *)context.Pc, (void*)test->results[i].pc );
        ok( frame == (test->results[i].frame_offset ? (ULONG)fake_stack : 0) + test->results[i].frame, "%u/%u: wrong frame %x/%x\n",
            testnum, i, (int)((char *)frame - (char *)(test->results[i].frame_offset ? fake_stack : NULL)), test->results[i].frame );

        for (j = 0; j < 47; j++)
        {
            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                {
                    k = nb_regs;
                    break;
                }
                if (test->results[i].regs[k][0] == j) break;
            }

            if (j >= 4 && j <= 11 && (&ctx_ptr.R4)[j - 4])
            {
                ok( k < nb_regs, "%u/%u: register %s should not be set to %lx\n",
                    testnum, i, reg_names_arm[j], (&context.R0)[j] );
                if (k < nb_regs)
                    ok( (&context.R0)[j] == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names_arm[j], (void *)(&context.R0)[j], (int)test->results[i].regs[k][1] );
            }
            else if (j == lr && ctx_ptr.Lr)
            {
                ok( k < nb_regs, "%u/%u: register %s should not be set to %lx\n",
                    testnum, i, reg_names_arm[j], context.Lr );
                if (k < nb_regs)
                    ok( context.Lr == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names_arm[j], (void *)context.Lr, (int)test->results[i].regs[k][1] );
            }
            else if (j == sp)
            {
                if (k < nb_regs)
                    ok( context.Sp == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names_arm[j], (void *)context.Sp, (int)test->results[i].regs[k][1] );
                else if (test->results[i].frame == 0xdeadbeef)
                    ok( (void *)context.Sp == fake_stack, "%u/%u: wrong sp %p/%p\n",
                        testnum, i, (void *)context.Sp, fake_stack);
                else
                    ok( context.Sp == frame, "%u/%u: wrong sp %p/%p\n",
                        testnum, i, (void *)context.Sp, (void *)frame);
            }
            else if (j >= d8 && j <= d15 && (&ctx_ptr.D8)[j - d8])
            {
                ok( k < nb_regs, "%u/%u: register %s should not be set to %llx\n",
                    testnum, i, reg_names_arm[j], context.D[j - d0] );
                if (k < nb_regs)
                    ok( context.D[j - d0] == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %llx/%llx\n",
                        testnum, i, reg_names_arm[j], context.D[j - d0], test->results[i].regs[k][1] );
            }
            else if (k < nb_regs)
            {
                if (j <= r12)
                  ok( (&context.R0)[j] == test->results[i].regs[k][1],
                      "%u/%u: register %s wrong %p/%x\n",
                      testnum, i, reg_names_arm[j], (void *)(&context.R0)[j], (int)test->results[i].regs[k][1] );
                else if (j == lr)
                  ok( context.Lr == test->results[i].regs[k][1],
                      "%u/%u: register %s wrong %p/%x\n",
                      testnum, i, reg_names_arm[j], (void *)context.Lr, (int)test->results[i].regs[k][1] );
                else
                  ok( context.D[j - d0] == test->results[i].regs[k][1],
                      "%u/%u: register %s wrong %llx/%llx\n",
                      testnum, i, reg_names_arm[j], context.D[j - d0], test->results[i].regs[k][1] );
            }
            else
            {
                ok( k == nb_regs, "%u/%u: register %s should be set\n", testnum, i, reg_names_arm[j] );
                if (j == lr)
                    ok( context.Lr == ORIG_LR, "%u/%u: register lr wrong %p/unset\n",
                        testnum, i, (void *)context.Lr );
                else if (j == r11)
                    ok( context.R11 == orig_fp, "%u/%u: register fp wrong %p/unset\n",
                        testnum, i, (void *)context.R11 );
                else if (j < d0)
                    ok( (&context.R0)[j] == unset_reg,
                        "%u/%u: register %s wrong %p/unset\n",
                        testnum, i, reg_names_arm[j], (void *)(&context.R0)[j]);
                else
                    ok( context.D[j - d0] == unset_reg64,
                        "%u/%u: register %s wrong %llx/unset\n",
                        testnum, i, reg_names_arm[j], context.D[j - d0]);
            }
        }
    }
}

#define DW(dword) ((dword >> 0) & 0xff), ((dword >> 8) & 0xff), ((dword >> 16) & 0xff), ((dword >> 24) & 0xff)

static void test_virtual_unwind_arm(void)
{

    static const BYTE function_0[] =
    {
        0x70, 0xb5,               /* 00: push   {r4-r6, lr} */
        0x88, 0xb0,               /* 02: sub    sp,  sp,  #32 */
        0x2d, 0xed, 0x06, 0x8b,   /* 04: vpush  {d8-d10} */
        0x00, 0xbf,               /* 08: nop */
        0x2d, 0xed, 0x06, 0x3b,   /* 0a: vpush  {d3-d5} */
        0xaf, 0x3f, 0x00, 0x80,   /* 0e: nop.w */
        0x6d, 0xed, 0x06, 0x1b,   /* 12: vpush  {d17-d19} */
        0x2d, 0xe9, 0x00, 0x15,   /* 16: push.w {r8, r10, r12} */
        0xeb, 0x46,               /* 1a: mov    r11, sp */
        0x00, 0xbf,               /* 1c: nop */
        0xbd, 0xec, 0x06, 0x8b,   /* 1e: vpop   {d8-d10} */
        0xdd, 0x46,               /* 22: mov    sp,  r11 */
        0x08, 0xb0,               /* 24: add    sp,  sp,  #32 */
        0x70, 0xbd,               /* 26: pop    {r4-r6, pc} */
    };

    static const DWORD unwind_info_0_header =
        (sizeof(function_0)/2) | /* function length */
        (1 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* F */
        (1 << 23) | /* epilog */
        (5 << 28);  /* codes, (sizeof(unwind_info_0)-headers+3)/4 */
    static const DWORD unwind_info_0_epilog0 =
        (15  <<  0) | /* offset = 0x1e / 2 = 15 */
        (0xE << 20) | /* condition, 0xE = always */
        (13  << 24);  /* index, byte offset to epilog opcodes */

    static const BYTE unwind_info_0[] =
    {
        DW(unwind_info_0_header),
        DW(unwind_info_0_epilog0),

        UWOP_SET_FP(11),              /* mov    r11, sp */
        UWOP_SAVE_REGSW((1<<r8)|(1<<r10)|(1<<r12)), /* push.w {r8, r10, r12} */
        UWOP_SAVE_D16_RANGE(17,19),   /* vpush  {d17-d19} */
        UWOP_NOP32,                   /* nop.w */
        UWOP_SAVE_D0_RANGE(3,5),      /* vpush  {d3-d5} */
        UWOP_NOP16,                   /* nop */
        UWOP_SAVE_D8_RANGE(10),       /* vpush  {d8-d10} */
        UWOP_ALLOC_SMALL(32),         /* sub    sp,  sp,  #32 */
        UWOP_SAVE_RANGE_4_7_LR(6, 1), /* push   {r4-r6,lr} */
        UWOP_END,

        UWOP_SAVE_D8_RANGE(10),       /* vpop {d8-d10} */
        UWOP_SET_FP(11),              /* mov sp,  r11 */
        UWOP_ALLOC_SMALL(32),         /* add sp,  sp,  #32 */
        UWOP_SAVE_RANGE_4_7_LR(6, 1), /* pop {r4-r6,pc} */
        UWOP_END,

        0, 0,                         /* align */
        0x00, 0x02, 0x00, 0x00,       /* handler */
        0x05, 0x06, 0x07, 0x08,       /* data */
    };

    static const struct results_arm results_0[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     0x0c,    0x010, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {lr,0x0c}, {-1,-1} }},
        { 0x04,  0x10,  0,     0x2c,    0x030, TRUE, { {r4,0x20}, {r5,0x24}, {r6,0x28}, {lr,0x2c}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x44,    0x048, TRUE, { {r4,0x38}, {r5,0x3c}, {r6,0x40}, {lr,0x44}, {d8, 0x400000000}, {d9, 0xc00000008}, {d10, 0x1400000010}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x44,    0x048, TRUE, { {r4,0x38}, {r5,0x3c}, {r6,0x40}, {lr,0x44}, {d8, 0x400000000}, {d9, 0xc00000008}, {d10, 0x1400000010}, {-1,-1} }},
        { 0x0e,  0x10,  0,     0x5c,    0x060, TRUE, { {r4,0x50}, {r5,0x54}, {r6,0x58}, {lr,0x5c}, {d8, 0x1c00000018}, {d9, 0x2400000020}, {d10, 0x2c00000028}, {d3, 0x400000000}, {d4, 0xc00000008}, {d5, 0x1400000010}, {-1,-1} }},
        { 0x12,  0x10,  0,     0x5c,    0x060, TRUE, { {r4,0x50}, {r5,0x54}, {r6,0x58}, {lr,0x5c}, {d8, 0x1c00000018}, {d9, 0x2400000020}, {d10, 0x2c00000028}, {d3, 0x400000000}, {d4, 0xc00000008}, {d5, 0x1400000010}, {-1,-1} }},
        { 0x16,  0x10,  0,     0x74,    0x078, TRUE, { {r4,0x68}, {r5,0x6c}, {r6,0x70}, {lr,0x74}, {d8, 0x3400000030}, {d9, 0x3c00000038}, {d10, 0x4400000040}, {d3, 0x1c00000018}, {d4, 0x2400000020}, {d5, 0x2c00000028}, {d17, 0x400000000}, {d18, 0xc00000008}, {d19, 0x1400000010}, {-1,-1} }},
        { 0x1a,  0x10,  0,     0x80,    0x084, TRUE, { {r4,0x74}, {r5,0x78}, {r6,0x7c}, {lr,0x80}, {d8, 0x400000003c}, {d9, 0x4800000044}, {d10, 0x500000004c}, {d3, 0x2800000024}, {d4, 0x300000002c}, {d5, 0x3800000034}, {d17, 0x100000000c}, {d18, 0x1800000014}, {d19, 0x200000001c}, {r8,0x00}, {r10,0x04}, {r12,0x08}, {-1,-1} }},
        { 0x1c,  0x10,  1,     0x90,    0x094, TRUE, { {r4,0x84}, {r5,0x88}, {r6,0x8c}, {lr,0x90}, {d8, 0x500000004c}, {d9, 0x5800000054}, {d10, 0x600000005c}, {d3, 0x3800000034}, {d4, 0x400000003c}, {d5, 0x4800000044}, {d17, 0x200000001c}, {d18, 0x2800000024}, {d19, 0x300000002c}, {r8,0x10}, {r10,0x14}, {r12,0x18}, {-1,-1} }},
        { 0x1e,  0x10,  0,     0x3c,    0x040, TRUE, { {r4,0x30}, {r5,0x34}, {r6,0x38}, {lr,0x3c}, {d8, 0x400000000}, {d9, 0xc00000008}, {d10, 0x1400000010}, {-1,-1} }},
        { 0x22,  0x10,  0,     0x3c,    0x040, TRUE, { {r4,0x30}, {r5,0x34}, {r6,0x38}, {lr,0x3c}, {-1,-1} }},
        { 0x24,  0x10,  0,     0x2c,    0x030, TRUE, { {r4,0x20}, {r5,0x24}, {r6,0x28}, {lr,0x2c}, {-1,-1} }},
        { 0x26,  0x10,  0,     0x0c,    0x010, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {lr,0x0c}, {-1,-1} }},
    };

    static const BYTE function_1[] =
    {
        0x30, 0xb4,               /* 00: push   {r4-r5} */
        0x4d, 0xf8, 0x20, 0xed,   /* 02: str    lr, [sp, #-32]! */
        0x00, 0xbf,               /* 06: nop */
        0x5d, 0xf8, 0x20, 0xeb,   /* 08: ldr    lr, [sp], #32 */
        0x30, 0xbc,               /* 0c: pop    {r4-r5} */
        0x70, 0x47,               /* 0e: bx     lr */
    };

    static const DWORD unwind_info_1_header =
        (sizeof(function_1)/2) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* F */
        (0 << 23) | /* epilog */
        (0 << 28);  /* codes */
    static const DWORD unwind_info_1_header2 =
        (1 <<  0) | /* epilog */
        (2 << 16);  /* codes, (sizeof(unwind_info_1)-headers+3)/4 */
    static const DWORD unwind_info_1_epilog0 =
        (4   <<  0) | /* offset = 0x08 / 2 = 4 */
        (0xE << 20) | /* condition, 0xE = always */
        (4   << 24);  /* index, byte offset to epilog opcodes */

    static const BYTE unwind_info_1[] = {
        DW(unwind_info_1_header),
        DW(unwind_info_1_header2),
        DW(unwind_info_1_epilog0),

        UWOP_SAVE_LR(32),             /* str    lr, [sp, #-32]! */
        UWOP_SAVE_RANGE_4_7_LR(5, 0), /* push   {r4-r5} */
        UWOP_END_NOP16,

        UWOP_SAVE_LR(32),             /* ldr    lr, [sp], #32 */
        UWOP_SAVE_RANGE_4_7_LR(5, 0), /* pop    {r4-r5} */
        UWOP_END_NOP16,               /* bx     lr */
    };

    static const struct results_arm results_1[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x00,  0,     ORIG_LR, 0x008, TRUE, { {r4,0x00}, {r5,0x04}, {-1,-1} }},
        { 0x06,  0x00,  0,     0x00,    0x028, TRUE, { {r4,0x20}, {r5,0x24}, {lr,0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x00,    0x028, TRUE, { {r4,0x20}, {r5,0x24}, {lr,0x00}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x008, TRUE, { {r4,0x00}, {r5,0x04}, {-1,-1} }},
        { 0x0e,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_2[] =
    {
        0x6f, 0x46,               /* 00: mov    r7,  sp */
        0x80, 0xb4,               /* 02: push   {r7} */
        0x84, 0xb0,               /* 04: sub    sp,  sp,  #16 */
        0x00, 0xbf,               /* 06: nop */
        0x04, 0xb0,               /* 08: add    sp,  sp,  #16 */
        0x80, 0xbc,               /* 0a: push   {r7} */
        0xbd, 0x46,               /* 0c: mov    sp,  r7 */
        0x00, 0xf0, 0x00, 0xb8,   /* 0e: b      tailcall */
    };

    static const DWORD unwind_info_2_header =
        (sizeof(function_2)/2) | /* function length */
        (0 << 20) | /* X */
        (1 << 21) | /* E */
        (0 << 22) | /* F */
        (0 << 23) | /* epilog */
        (2 << 28);  /* codes, (sizeof(unwind_info_2)-headers+3)/4 */

    static const BYTE unwind_info_2[] =
    {
        DW(unwind_info_2_header),

        UWOP_ALLOC_SMALL(16),         /* sub    sp,  sp,  #16 */
        UWOP_SAVE_REGS((1<<r7)),      /* push   {r7} */
        UWOP_SET_FP(7),               /* mov    r7,  sp */
        UWOP_END_NOP32,               /* b      tailcall */
    };

    static const struct results_arm results_2[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE,  { {-1,-1} }},
        { 0x02,  0x00,  0,     ORIG_LR, 0x55555555, FALSE,  { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x000, FALSE,  { {r7,0x00}, {-1,-1} }},
        { 0x06,  0x00,  0,     ORIG_LR, 0x010, FALSE,  { {r7,0x10}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x010, FALSE,  { {r7,0x10}, {-1,-1} }},
        { 0x0a,  0x00,  0,     ORIG_LR, 0x000, FALSE,  { {r7,0x00}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x55555555, FALSE,  { {-1,-1} }},
        { 0x0e,  0x00,  0,     ORIG_LR, 0x000, TRUE,  { {-1,-1} }},
    };

    static const BYTE function_3[] =
    {
        0xaf, 0x3f, 0x00, 0x80,   /* 00: nop.w */
        0x00, 0xbf,               /* 04: nop */
        0x00, 0xbf,               /* 06: nop */
        0x04, 0xb0,               /* 08: add    sp,  sp,  #16 */
        0xbd, 0xe8, 0xf0, 0x8f,   /* 0a: pop.w  {r4-r11,pc} */
    };

    /* Testing F=1, no prologue */
    static const DWORD unwind_info_3_header =
        (sizeof(function_3)/2) | /* function length */
        (0 << 20) | /* X */
        (1 << 21) | /* E */
        (1 << 22) | /* F */
        (0 << 23) | /* epilog */
        (1 << 28);  /* codes, (sizeof(unwind_info_3)-headers+3)/4 */

    static const BYTE unwind_info_3[] =
    {
        DW(unwind_info_3_header),

        UWOP_ALLOC_SMALL(16),         /* sub    sp,  sp,  #16 */
        UWOP_SAVE_RANGE_4_11_LR(11, 1), /* pop.w {r4-r11,pc} */
        UWOP_END,
    };

    static const struct results_arm results_3[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     0x30, 0x034, TRUE,  { {r4,0x10}, {r5,0x14}, {r6,0x18}, {r7,0x1c}, {r8,0x20}, {r9,0x24}, {r10,0x28}, {r11,0x2c}, {lr,0x30}, {-1,-1} }},
        { 0x04,  0x00,  0,     0x30, 0x034, TRUE,  { {r4,0x10}, {r5,0x14}, {r6,0x18}, {r7,0x1c}, {r8,0x20}, {r9,0x24}, {r10,0x28}, {r11,0x2c}, {lr,0x30}, {-1,-1} }},
        { 0x06,  0x00,  0,     0x30, 0x034, TRUE,  { {r4,0x10}, {r5,0x14}, {r6,0x18}, {r7,0x1c}, {r8,0x20}, {r9,0x24}, {r10,0x28}, {r11,0x2c}, {lr,0x30}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x30, 0x034, TRUE,  { {r4,0x10}, {r5,0x14}, {r6,0x18}, {r7,0x1c}, {r8,0x20}, {r9,0x24}, {r10,0x28}, {r11,0x2c}, {lr,0x30}, {-1,-1} }},
        { 0x0a,  0x00,  0,     0x20, 0x024, TRUE,  { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r8,0x10}, {r9,0x14}, {r10,0x18}, {r11,0x1c}, {lr,0x20}, {-1,-1} }},
    };

    static const BYTE function_4[] =
    {
        0x2d, 0xe9, 0x00, 0x55,   /* 00: push.w {r8, r10, r12, lr} */
        0x50, 0xb4,               /* 04: push   {r4, r6} */
        0x00, 0xbf,               /* 06: nop */
        0x50, 0xbc,               /* 08: pop    {r4, r6} */
        0xbd, 0xe8, 0x00, 0x95,   /* 0a: pop.w  {r8, r10, r12, pc} */
    };

    static const DWORD unwind_info_4_header =
        (sizeof(function_4)/2) | /* function length */
        (0 << 20) | /* X */
        (1 << 21) | /* E */
        (0 << 22) | /* F */
        (0 << 23) | /* epilog */
        (2 << 28);  /* codes, (sizeof(unwind_info_4)-headers+3)/4 */

    static const BYTE unwind_info_4[] =
    {
        DW(unwind_info_4_header),

        UWOP_SAVE_REGS((1<<r4)|(1<<r6)), /* push {r4, r6} */
        UWOP_SAVE_REGSW((1<<r8)|(1<<r10)|(1<<r12)|(1<<lr)), /* push.w {r8, r10, r12, lr} */
        UWOP_END,
    };

    static const struct results_arm results_4[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x00000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x0c,    0x00010, TRUE, { {r8,0x00}, {r10,0x04}, {r12,0x08}, {lr,0x0c}, {-1,-1} }},
        { 0x06,  0x10,  0,     0x14,    0x00018, TRUE, { {r8,0x08}, {r10,0x0c}, {r12,0x10}, {lr,0x14}, {r4,0x00}, {r6,0x04}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x14,    0x00018, TRUE, { {r8,0x08}, {r10,0x0c}, {r12,0x10}, {lr,0x14}, {r4,0x00}, {r6,0x04}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x0c,    0x00010, TRUE, { {r8,0x00}, {r10,0x04}, {r12,0x08}, {lr,0x0c}, {-1,-1} }},
    };

    static const BYTE function_5[] =
    {
        0x50, 0xb5,               /* 00: push   {r4, r6, lr} */
        0xad, 0xf2, 0x08, 0x0d,   /* 02: subw   sp,  sp,  #8 */
        0x84, 0xb0,               /* 06: sub    sp,  sp,  #16 */
        0x88, 0xb0,               /* 08: sub    sp,  sp,  #32 */
        0xad, 0xf2, 0x40, 0x0d,   /* 0a: subw   sp,  sp,  #64 */
        0xad, 0xf2, 0x80, 0x0d,   /* 0e: subw   sp,  sp,  #128 */
        0x00, 0xbf,               /* 12: nop */
        0x50, 0xbd,               /* 14: pop    {r4, r6, pc} */
    };

    static const DWORD unwind_info_5_header =
        (sizeof(function_5)/2) | /* function length */
        (0  << 20) | /* X */
        (1  << 21) | /* E */
        (0  << 22) | /* F */
        (16 << 23) | /* epilog */
        (5  << 28);  /* codes, (sizeof(unwind_info_4)-headers+3)/4 */

    static const BYTE unwind_info_5[] =
    {
        DW(unwind_info_5_header),

        UWOP_ALLOC_HUGEW(128),        /* subw   sp,  sp,  #128 */
        UWOP_ALLOC_LARGEW(64),        /* subw   sp,  sp,  #64 */
        UWOP_ALLOC_HUGE(32),          /* sub    sp,  sp,  #32 */
        UWOP_ALLOC_LARGE(16),         /* sub    sp,  sp,  #16 */
        UWOP_ALLOC_MEDIUMW(8),        /* subw   sp,  sp,  #8 */
        UWOP_SAVE_REGS((1<<r4)|(1<<r6)|(1<<lr)), /* push {r4, r6, lr} */
        UWOP_END,
    };

    static const struct results_arm results_5[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x00000, TRUE, { {-1,-1} }},
        { 0x02,  0x00,  0,     0x008,   0x0000c, TRUE, { {r4,0x00}, {r6,0x04}, {lr,0x08}, {-1,-1} }},
        { 0x06,  0x00,  0,     0x010,   0x00014, TRUE, { {r4,0x08}, {r6,0x0c}, {lr,0x10}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x020,   0x00024, TRUE, { {r4,0x18}, {r6,0x1c}, {lr,0x20}, {-1,-1} }},
        { 0x0a,  0x00,  0,     0x040,   0x00044, TRUE, { {r4,0x38}, {r6,0x3c}, {lr,0x40}, {-1,-1} }},
        { 0x0e,  0x00,  0,     0x080,   0x00084, TRUE, { {r4,0x78}, {r6,0x7c}, {lr,0x80}, {-1,-1} }},
        { 0x12,  0x00,  0,     0x100,   0x00104, TRUE, { {r4,0xf8}, {r6,0xfc}, {lr,0x100}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x008,   0x0000c, TRUE, { {r4,0x00}, {r6,0x04}, {lr,0x08}, {-1,-1} }},
    };

    static const BYTE function_6[] =
    {
        0x00, 0xbf,               /* 00: nop */
        0x00, 0xbf,               /* 02: nop */
        0x00, 0xbf,               /* 04: nop */
        0x70, 0x47,               /* 06: bx     lr */
    };

    static const DWORD unwind_info_6_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_6)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (7 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_6[] = { DW(unwind_info_6_packed) };

    static const struct results_arm results_6[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x06,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_7[] =
    {
        0x10, 0xb4,               /* 00: push   {r4} */
        0x00, 0xbf,               /* 02: nop */
        0x10, 0xbc,               /* 04: pop    {r4} */
        0x70, 0x47,               /* 06: bx     lr */
    };

    static const DWORD unwind_info_7_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_7)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_7[] = { DW(unwind_info_7_packed) };

    static const struct results_arm results_7[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x00,  0,     ORIG_LR, 0x004, TRUE, { {r4,0x00}, {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x004, TRUE, { {r4,0x00}, {-1,-1} }},
        { 0x06,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_8[] =
    {
        0x10, 0xb5,               /* 00: push   {r4, lr} */
        0x00, 0xbf,               /* 02: nop */
        0xbd, 0xe8, 0x10, 0x40,   /* 04: pop    {r4, lr} */
        0x70, 0x47,               /* 08: bx     lr */
    };

    static const DWORD unwind_info_8_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_8)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_8[] = { DW(unwind_info_8_packed) };

    static const struct results_arm results_8[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x00,  0,     0x004,   0x008, TRUE, { {r4,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x04,  0x00,  0,     0x004,   0x008, TRUE, { {r4,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x06,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }}, /* Note, there's no instruction at 0x06, but the pop is surprisingly a 4 byte instruction. */
        { 0x08,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_9[] =
    {
        0x00, 0xb5,               /* 00: push   {lr} */
        0x2d, 0xed, 0x02, 0x8b,   /* 02: vpush  {d8} */
        0x88, 0xb0,               /* 06: sub    sp,  sp,  #32 */
        0x00, 0xbf,               /* 08: nop */
        0x08, 0xb0,               /* 0a: add    sp,  sp,  #32 */
        0xbd, 0xec, 0x02, 0x8b,   /* 0c: vpop   {d8} */
        0x5d, 0xf8, 0x04, 0xeb,   /* 10: ldr    lr, [sp], #4 */
        0x00, 0xf0, 0x00, 0xb8,   /* 14: b      tailcall */
    };

    static const DWORD unwind_info_9_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_9)/2 << 2) | /* FunctionLength */
        (2 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (8 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_9[] = { DW(unwind_info_9_packed) };

    static const struct results_arm results_9[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x00,  0,     0x00,    0x004, TRUE, { {lr,0x00}, {-1,-1} }},
        { 0x06,  0x00,  0,     0x08,    0x00c, TRUE, { {lr,0x08}, {d8,0x400000000}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x28,    0x02c, TRUE, { {lr,0x28}, {d8,0x2400000020}, {-1,-1} }},
        { 0x0a,  0x00,  0,     0x28,    0x02c, TRUE, { {lr,0x28}, {d8,0x2400000020}, {-1,-1} }},
#if 0
        /* L=1, R=1, Ret>0 seems to get incorrect handling of the epilogue */
        { 0x0c,  0x00,  0,     ORIG_LR, 0x008, TRUE, { {d8,0x400000000}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
#endif
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_10[] =
    {
        0x2d, 0xe9, 0x00, 0x48,   /* 00: push.w {r11, lr} */
        0xeb, 0x46,               /* 04: mov    r11, sp */
        0x2d, 0xed, 0x04, 0x8b,   /* 06: vpush  {d8-d9} */
        0x84, 0xb0,               /* 0a: sub    sp,  sp,  #16 */
        0x00, 0xbf,               /* 0c: nop */
        0x04, 0xb0,               /* 0e: add    sp,  sp,  #16 */
        0xbd, 0xec, 0x04, 0x8b,   /* 10: vpop   {d8-d9} */
        0xbd, 0xe8, 0x00, 0x48,   /* 14: pop.w  {r11, lr} */
        0x70, 0x47,               /* 18: bx     lr */
    };

    static const DWORD unwind_info_10_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_10)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (1 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (4 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_10[] = { DW(unwind_info_10_packed) };

    static const struct results_arm results_10[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     0x04,    0x008, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x06,  0x00,  0,     0x04,    0x008, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x0a,  0x00,  0,     0x14,    0x018, TRUE, { {r11,0x10}, {lr,0x14}, {d8,0x400000000}, {d9,0xc00000008}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x24,    0x028, TRUE, { {r11,0x20}, {lr,0x24}, {d8,0x1400000010}, {d9,0x1c00000018}, {-1,-1} }},
        { 0x0e,  0x00,  0,     0x24,    0x028, TRUE, { {r11,0x20}, {lr,0x24}, {d8,0x1400000010}, {d9,0x1c00000018}, {-1,-1} }},
#if 0
        /* L=1, R=1, Ret>0 seems to get incorrect handling of the epilogue */
        { 0x10,  0x00,  0,     0x14,    0x018, TRUE, { {r11,0x10}, {lr,0x14}, {d8,0x400000000}, {d9,0xc00000008}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x04,    0x008, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
#endif
        { 0x18,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_11[] =
    {
        0x2d, 0xe9, 0x00, 0x48,   /* 00: push.w {r11, lr} */
        0xeb, 0x46,               /* 04: mov    r11, sp */
        0x2d, 0xed, 0x04, 0x8b,   /* 06: vpush  {d8-d9} */
        0x84, 0xb0,               /* 0a: sub    sp,  sp,  #16 */
        0x00, 0xbf,               /* 0c: nop */
        0x04, 0xb0,               /* 0e: add    sp,  sp,  #16 */
        0xbd, 0xec, 0x04, 0x8b,   /* 10: vpop   {d8-d9} */
        0xbd, 0xe8, 0x00, 0x88,   /* 14: pop.w  {r11, pc} */
    };

    static const DWORD unwind_info_11_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_11)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (1 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (4 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_11[] = { DW(unwind_info_11_packed) };

    static const struct results_arm results_11[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     0x04,    0x008, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x06,  0x00,  0,     0x04,    0x008, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x0a,  0x00,  0,     0x14,    0x018, TRUE, { {r11,0x10}, {lr,0x14}, {d8,0x400000000}, {d9,0xc00000008}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x24,    0x028, TRUE, { {r11,0x20}, {lr,0x24}, {d8,0x1400000010}, {d9,0x1c00000018}, {-1,-1} }},
        { 0x0e,  0x00,  0,     0x24,    0x028, TRUE, { {r11,0x20}, {lr,0x24}, {d8,0x1400000010}, {d9,0x1c00000018}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x14,    0x018, TRUE, { {r11,0x10}, {lr,0x14}, {d8,0x400000000}, {d9,0xc00000008}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x04,    0x008, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
    };

    static const BYTE function_12[] =
    {
        0x2d, 0xed, 0x0e, 0x8b,   /* 00: vpush  {d8-d14} */
        0x84, 0xb0,               /* 04: sub    sp,  sp,  #16 */
        0x00, 0xbf,               /* 06: nop */
        0x04, 0xb0,               /* 08: add    sp,  sp,  #16 */
        0xbd, 0xec, 0x0e, 0x8b,   /* 0a: vpop   {d8-d14} */
        0x00, 0xf0, 0x00, 0xb8,   /* 0e: b      tailcall */
    };

    static const DWORD unwind_info_12_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_12)/2 << 2) | /* FunctionLength */
        (2 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (6 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (4 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_12[] = { DW(unwind_info_12_packed) };

    static const struct results_arm results_12[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x038, TRUE, { {d8,0x400000000}, {d9,0xc00000008}, {d10,0x1400000010}, {d11,0x1c00000018}, {d12,0x2400000020}, {d13,0x2c00000028}, {d14,0x3400000030}, {-1,-1} }},
        { 0x06,  0x00,  0,     ORIG_LR, 0x048, TRUE, { {d8,0x1400000010}, {d9,0x1c00000018}, {d10,0x2400000020}, {d11,0x2c00000028}, {d12,0x3400000030}, {d13,0x3c00000038}, {d14,0x4400000040}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x048, TRUE, { {d8,0x1400000010}, {d9,0x1c00000018}, {d10,0x2400000020}, {d11,0x2c00000028}, {d12,0x3400000030}, {d13,0x3c00000038}, {d14,0x4400000040}, {-1,-1} }},
        { 0x0a,  0x00,  0,     ORIG_LR, 0x038, TRUE, { {d8,0x400000000}, {d9,0xc00000008}, {d10,0x1400000010}, {d11,0x1c00000018}, {d12,0x2400000020}, {d13,0x2c00000028}, {d14,0x3400000030}, {-1,-1} }},
        { 0x0e,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_13[] =
    {
        0x2d, 0xe9, 0xf0, 0x4f,   /* 00: push.w {r4-r11, lr} */
        0x0d, 0xf1, 0x1c, 0x0b,   /* 04: add.w  r11, sp,  #28 */
        0x85, 0xb0,               /* 08: sub    sp,  sp,  #20 */
        0x00, 0xbf,               /* 0a: nop */
        0x05, 0xb0,               /* 0c: add    sp,  sp,  #20 */
        0x2d, 0xe8, 0xf0, 0x8f,   /* 0e: pop.w  {r4-r11, lr} */
    };

    static const DWORD unwind_info_13_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_13)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (6 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (5 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_13[] = { DW(unwind_info_13_packed) };

    static const struct results_arm results_13[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x20,    0x024, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r8,0x10}, {r9,0x14}, {r10,0x18}, {r11,0x1c}, {lr,0x20}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x20,    0x024, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r8,0x10}, {r9,0x14}, {r10,0x18}, {r11,0x1c}, {lr,0x20}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x34,    0x038, TRUE, { {r4,0x14}, {r5,0x18}, {r6,0x1c}, {r7,0x20}, {r8,0x24}, {r9,0x28}, {r10,0x2c}, {r11,0x30}, {lr,0x34}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x34,    0x038, TRUE, { {r4,0x14}, {r5,0x18}, {r6,0x1c}, {r7,0x20}, {r8,0x24}, {r9,0x28}, {r10,0x2c}, {r11,0x30}, {lr,0x34}, {-1,-1} }},
        { 0x0e,  0x10,  0,     0x20,    0x024, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r8,0x10}, {r9,0x14}, {r10,0x18}, {r11,0x1c}, {lr,0x20}, {-1,-1} }},
    };

    static const BYTE function_14[] =
    {
        0x2d, 0xe9, 0xf0, 0x4f,   /* 00: push.w {r4-r11, lr} */
        0x85, 0xb0,               /* 04: sub    sp,  sp,  #20 */
        0x00, 0xbf,               /* 06: nop */
        0x05, 0xb0,               /* 08: add    sp,  sp,  #20 */
        0x2d, 0xe8, 0xf0, 0x8f,   /* 0a: pop.w  {r4-r11, lr} */
    };

    static const DWORD unwind_info_14_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_14)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (7 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (5 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_14[] = { DW(unwind_info_14_packed) };

    static const struct results_arm results_14[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x20,    0x024, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r8,0x10}, {r9,0x14}, {r10,0x18}, {r11,0x1c}, {lr,0x20}, {-1,-1} }},
        { 0x06,  0x10,  0,     0x34,    0x038, TRUE, { {r4,0x14}, {r5,0x18}, {r6,0x1c}, {r7,0x20}, {r8,0x24}, {r9,0x28}, {r10,0x2c}, {r11,0x30}, {lr,0x34}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x34,    0x038, TRUE, { {r4,0x14}, {r5,0x18}, {r6,0x1c}, {r7,0x20}, {r8,0x24}, {r9,0x28}, {r10,0x2c}, {r11,0x30}, {lr,0x34}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x20,    0x024, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r8,0x10}, {r9,0x14}, {r10,0x18}, {r11,0x1c}, {lr,0x20}, {-1,-1} }},
    };

    static const BYTE function_15[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x10, 0xb5,               /* 02: push   {r4,lr} */
        0xad, 0xf5, 0x00, 0x7d,   /* 04: sub    sp,  sp,  #512 */
        0x00, 0xbf,               /* 08: nop */
        0x0d, 0xf5, 0x00, 0x7d,   /* 0a: add    sp,  sp,  #512 */
        0x10, 0xb5,               /* 0e: pop    {r4} */
        0x5d, 0xf8, 0x14, 0xfb,   /* 10: ldr    pc, [sp], #20 */
    };

    static const DWORD unwind_info_15_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_15)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (128 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_15[] = { DW(unwind_info_15_packed) };

    static const struct results_arm results_15[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x04,    0x018, TRUE, { {r4,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
        { 0x0e,  0x10,  0,     0x04,    0x018, TRUE, { {r4,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x10,  0x10,  0,     0x00,    0x014, TRUE, { {lr,0x00}, {-1,-1} }},
    };

    static const BYTE function_16[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x2d, 0xe9, 0x00, 0x48,   /* 02: push.w {r11,lr} */
        0xeb, 0x46,               /* 06: mov    r11, sp */
        0x00, 0xbf,               /* 08: nop */
        0xbd, 0xe8, 0x10, 0x40,   /* 0a: pop.w  {r11,lr} */
        0x04, 0xb0,               /* 0e: add    sp,  sp,  #16 */
        0x00, 0xf0, 0x00, 0xb8,   /* 10: b      tailcall */
    };

    static const DWORD unwind_info_16_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_16)/2 << 2) | /* FunctionLength */
        (2 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (7 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (0 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_16[] = { DW(unwind_info_16_packed) };

    static const struct results_arm results_16[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x06,  0x10,  0,     0x04,    0x018, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x04,    0x018, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x04,    0x018, TRUE, { {r11,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x0e,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x10,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_17[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x10, 0xb4,               /* 02: push   {r4} */
        0xad, 0xf5, 0x00, 0x7d,   /* 04: sub    sp,  sp,  #512 */
        0x00, 0xbf,               /* 08: nop */
        0x0d, 0xf5, 0x00, 0x7d,   /* 0a: add    sp,  sp,  #512 */
        0x10, 0xbc,               /* 0e: pop    {r4} */
        0x04, 0xb0,               /* 10: add    sp,  sp,  #16 */
        0x70, 0x47,               /* 12: bx     lr */
    };

    static const DWORD unwind_info_17_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_17)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (128 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_17[] = { DW(unwind_info_17_packed) };

    static const struct results_arm results_17[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x014, TRUE, { {r4,0x00}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x214, TRUE, { {r4,0x200}, {-1,-1} }},
        { 0x0a,  0x10,  0,     ORIG_LR, 0x214, TRUE, { {r4,0x200}, {-1,-1} }},
        { 0x0e,  0x10,  0,     ORIG_LR, 0x014, TRUE, { {r4,0x00}, {-1,-1} }},
        { 0x10,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x12,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_18[] =
    {
        0x08, 0xb5,               /* 00: push   {r3,lr} */
        0x00, 0xbf,               /* 02: nop */
        0x08, 0xbd,               /* 04: pop    {r3,pc} */
    };

    static const DWORD unwind_info_18_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_18)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (7 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0x3fcu << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_18[] = { DW(unwind_info_18_packed) };

    static const struct results_arm results_18[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     0x04,    0x008, TRUE, { {lr,0x04}, {-1,-1} }},
        { 0x04,  0x10,  0,     0x04,    0x008, TRUE, { {lr,0x04}, {-1,-1} }},
    };

    static const BYTE function_19[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x14, 0xb4,               /* 02: push   {r0-r4} */
        0x00, 0xbf,               /* 04: nop */
        0x1f, 0xbc,               /* 06: pop    {r0-r4} */
        0x04, 0xb0,               /* 08: add    sp,  sp,  #16 */
        0x70, 0x47,               /* 0a: bx     lr */
    };

    static const DWORD unwind_info_19_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_19)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0x3ffu << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_19[] = { DW(unwind_info_19_packed) };

    static const struct results_arm results_19[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x024, TRUE, { {r4,0x10}, {-1,-1} }},
        { 0x06,  0x10,  0,     ORIG_LR, 0x024, TRUE, { {r4,0x10}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x0a,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_20[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x14, 0xb4,               /* 02: push   {r0-r4} */
        0x00, 0xbf,               /* 04: nop */
        0x04, 0xb0,               /* 06: add    sp,  sp,  #16 */
        0x10, 0xbc,               /* 08: pop    {r4} */
        0x04, 0xb0,               /* 0a: add    sp,  sp,  #16 */
        0x70, 0x47,               /* 0c: bx     lr */
    };

    static const DWORD unwind_info_20_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_20)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0x3f7u << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_20[] = { DW(unwind_info_20_packed) };

    static const struct results_arm results_20[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x024, TRUE, { {r4,0x10}, {-1,-1} }},
        { 0x06,  0x10,  0,     ORIG_LR, 0x024, TRUE, { {r4,0x10}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x014, TRUE, { {r4,0x00}, {-1,-1} }},
        { 0x0a,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x0c,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_21[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x10, 0xb4,               /* 02: push   {r4} */
        0x84, 0xb0,               /* 04: sub    sp,  sp,  #16 */
        0x00, 0xbf,               /* 06: nop */
        0x1f, 0xbc,               /* 08: pop    {r0-r4} */
        0x04, 0xb0,               /* 0a: add    sp,  sp,  #16 */
        0x70, 0x47,               /* 0c: bx     lr */
    };

    static const DWORD unwind_info_21_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_21)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0x3fbu << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_21[] = { DW(unwind_info_21_packed) };

    static const struct results_arm results_21[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x014, TRUE, { {r4,0x00}, {-1,-1} }},
        { 0x06,  0x10,  0,     ORIG_LR, 0x024, TRUE, { {r4,0x10}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x024, TRUE, { {r4,0x10}, {-1,-1} }},
        { 0x0a,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x0c,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_22[] =
    {
        0x00, 0xbf,               /* 00: nop */
        0x00, 0xbf,               /* 02: nop */
        0x0d, 0xf5, 0x00, 0x7d,   /* 04: add    sp,  sp,  #512 */
        0x10, 0xb5,               /* 08: pop    {r4} */
        0x5d, 0xf8, 0x14, 0xfb,   /* 0a: ldr    pc, [sp], #20 */
    };

    static const DWORD unwind_info_22_packed =
        (2 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_22)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (128 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_22[] = { DW(unwind_info_22_packed) };

    static const struct results_arm results_22[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
        { 0x02,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
        { 0x04,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x04,    0x018, TRUE, { {r4,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x00,    0x014, TRUE, { {lr,0x00}, {-1,-1} }},
    };

    static const BYTE function_23[] =
    {
        0x0f, 0xb4,               /* 00: push   {r0-r3} */
        0x10, 0xb5,               /* 02: push   {r4,lr} */
        0xad, 0xf5, 0x00, 0x7d,   /* 04: sub    sp,  sp,  #512 */
        0x00, 0xbf,               /* 08: nop */
        0x00, 0xbf,               /* 0a: nop */
    };

    static const DWORD unwind_info_23_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_23)/2 << 2) | /* FunctionLength */
        (3 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (1 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (128 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_23[] = { DW(unwind_info_23_packed) };

    static const struct results_arm results_23[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x04,    0x018, TRUE, { {r4,0x00}, {lr,0x04}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x204,   0x218, TRUE, { {r4,0x200}, {lr,0x204}, {-1,-1} }},
    };

    static const BYTE function_24[] =
    {
        0x2d, 0xe9, 0xfc, 0x48,   /* 00: push.w {r2-r7,r11,lr} */
        0x0d, 0xf1, 0x18, 0x0b,   /* 04: add    r11, sp,  #24 */
        0x00, 0xbf,               /* 08: nop */
        0x02, 0xb0,               /* 0a: add    sp,  sp,  #8 */
        0xbd, 0xe8, 0x10, 0x48,   /* 0c: pop.w  {r4-r7,r11,pc} */
    };

    static const DWORD unwind_info_24_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_24)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (3 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (0x3f5u << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_24[] = { DW(unwind_info_24_packed) };

    static const struct results_arm results_24[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x1c,    0x020, TRUE, { {r4,0x08}, {r5,0x0c}, {r6,0x10}, {r7,0x14}, {r11,0x18}, {lr,0x1c}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x1c,    0x020, TRUE, { {r4,0x08}, {r5,0x0c}, {r6,0x10}, {r7,0x14}, {r11,0x18}, {lr,0x1c}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x1c,    0x020, TRUE, { {r4,0x08}, {r5,0x0c}, {r6,0x10}, {r7,0x14}, {r11,0x18}, {lr,0x1c}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x14,    0x018, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r11,0x10}, {lr,0x14}, {-1,-1} }},
    };

    static const BYTE function_25[] =
    {
        0x2d, 0xe9, 0xf0, 0x48,   /* 00: push.w {r4-r7,r11,lr} */
        0x0d, 0xf1, 0x10, 0x0b,   /* 04: add    r11, sp,  #16 */
        0x82, 0xb0,               /* 08: sub    sp,  sp,  #8 */
        0x00, 0xbf,               /* 0a: nop */
        0xbd, 0xe8, 0xfc, 0x48,   /* 0c: pop.w  {r2-r7,r11,pc} */
    };

    static const DWORD unwind_info_25_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_25)/2 << 2) | /* FunctionLength */
        (0 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (3 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (1 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (0x3f9u << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_25[] = { DW(unwind_info_25_packed) };

    static const struct results_arm results_25[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     0x14,    0x018, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r11,0x10}, {lr,0x14}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x14,    0x018, TRUE, { {r4,0x00}, {r5,0x04}, {r6,0x08}, {r7,0x0c}, {r11,0x10}, {lr,0x14}, {-1,-1} }},
        { 0x0a,  0x10,  0,     0x1c,    0x020, TRUE, { {r4,0x08}, {r5,0x0c}, {r6,0x10}, {r7,0x14}, {r11,0x18}, {lr,0x1c}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x1c,    0x020, TRUE, { {r4,0x08}, {r5,0x0c}, {r6,0x10}, {r7,0x14}, {r11,0x18}, {lr,0x1c}, {-1,-1} }},
    };

    static const BYTE function_26[] =
    {
        0x2d, 0xe9, 0x10, 0x08,   /* 00: push.w {r4, r11} */
        0x0d, 0xf1, 0x1c, 0x0b,   /* 04: add.w  r11, sp,  #28 */
        0x84, 0xb0,               /* 08: sub    sp,  sp,  #16 */
        0x00, 0xbf,               /* 0a: nop */
        0x04, 0xb0,               /* 0c: add    sp,  sp,  #16 */
        0xbd, 0xe8, 0x10, 0x08,   /* 0e: pop.w  {r4, r11} */
        0x70, 0x47,               /* 12: bx     lr */
    };

    /* C=1, L=0 is disallowed by doc */
    static const DWORD unwind_info_26_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_26)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (0 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (0 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (1 << 21) | /* C - hook up r11 */
        (4 << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_26[] = { DW(unwind_info_26_packed) };

    static const struct results_arm results_26[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x008, TRUE, { {r4,0x00}, {r11,0x04}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x008, TRUE, { {r4,0x00}, {r11,0x04}, {-1,-1} }},
        { 0x0a,  0x10,  0,     ORIG_LR, 0x018, TRUE, { {r4,0x10}, {r11,0x14}, {-1,-1} }},
        { 0x0c,  0x10,  0,     ORIG_LR, 0x018, TRUE, { {r4,0x10}, {r11,0x14}, {-1,-1} }},
        { 0x0e,  0x10,  0,     ORIG_LR, 0x008, TRUE, { {r4,0x00}, {r11,0x04}, {-1,-1} }},
        { 0x12,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_27[] =
    {
        0x0e, 0xb4,               /* 00: push   {r1-r3} */
        0x00, 0xbf,               /* 02: nop */
        0x03, 0xb0,               /* 04: add    sp,  sp,  #12 */
        0x70, 0x47,               /* 06: bx     lr */
    };

    static const DWORD unwind_info_27_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_27)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (7 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0x3f6u << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_27[] = { DW(unwind_info_27_packed) };

    static const struct results_arm results_27[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x00c, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x00c, TRUE, { {-1,-1} }},
        { 0x06,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_28[] =
    {
        0x0e, 0xb4,               /* 00: push   {r1-r3} */
        0x00, 0xbf,               /* 02: nop */
        0x03, 0xb0,               /* 04: add    sp,  sp,  #12 */
        0x70, 0x47,               /* 06: bx     lr */
    };

    static const DWORD unwind_info_28_packed =
        (1 << 0)  | /* Flag, 01 has prologue, 10 (2) fragment (no prologue) */
        (sizeof(function_28)/2 << 2) | /* FunctionLength */
        (1 << 13) | /* Ret (00 pop, 01 16 bit branch, 10 32 bit branch, 11 no epilogue) */
        (0 << 15) | /* H (homing, 16 bytes push of r0-r3 at start) */
        (7 << 16) | /* Reg r4 - r(4+N), or d8 - d(8+N) */
        (1 << 19) | /* R (0 integer registers, 1 float registers, R=1, Reg=7 no registers */
        (0 << 20) | /* L, push LR */
        (0 << 21) | /* C - hook up r11 */
        (0x3fau << 22);  /* StackAdjust, stack/4. 0x3F4 special, + (0-3) stack adjustment, 4 PF (prologue folding), 8 EF (epilogue folding) */

    static const BYTE unwind_info_28[] = { DW(unwind_info_28_packed) };

    static const struct results_arm results_28[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x02,  0x10,  0,     ORIG_LR, 0x00c, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x00c, TRUE, { {-1,-1} }},
        { 0x06,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_29[] =
    {
        0x00, 0xbf,               /* 00: nop */
        0x00, 0xbf,               /* 02: nop */
    };

    static const DWORD unwind_info_29_header =
        (sizeof(function_29)/2) | /* function length */
        (0  << 20) | /* X */
        (0  << 21) | /* E */
        (0  << 22) | /* F */
        (0  << 23) | /* epilog */
        (1  << 28);  /* codes, (sizeof(unwind_info_29)-headers+3)/4 */

    static const BYTE unwind_info_29[] =
    {
        DW(unwind_info_29_header),
        UWOP_MSFT_OP_CONTEXT,
        UWOP_END,
    };

    static const struct results_arm results_29[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     0x40,    0x38, FALSE, { {r0,0x04}, {r1,0x08}, {r2,0x0c}, {r3,0x10}, {r4,0x14}, {r5,0x18}, {r6,0x1c}, {r7,0x20}, {r8,0x24}, {r9,0x28}, {r10,0x2c}, {r11,0x30}, {r12,0x34}, {sp,0x38}, {lr,0x3c},
                                                       {d0,0x5400000050}, {d1,0x5c00000058}, {d2,0x6400000060}, {d3,0x6c00000068}, {d4,0x7400000070}, {d5,0x7c00000078}, {d6,0x8400000080}, {d7,0x8c00000088},
                                                       {d8,0x9400000090}, {d9,0x9c00000098}, {d10,0xa4000000a0}, {d11,0xac000000a8}, {d12,0xb4000000b0}, {d13,0xbc000000b8}, {d14,0xc4000000c0}, {d15,0xcc000000c8},
                                                       {d16,0xd4000000d0}, {d17,0xdc000000d8}, {d18,0xe4000000e0}, {d19,0xec000000e8}, {d20,0xf4000000f0}, {d21,0xfc000000f8}, {d22,0x10400000100}, {d23,0x10c00000108},
                                                       {d24,0x11400000110}, {d25,0x11c00000118}, {d26,0x12400000120}, {d27,0x12c00000128}, {d28,0x13400000130}, {d29,0x13c00000138}, {d30,0x14400000140}, {d31,0x14c00000148} }},
    };

    static const BYTE function_30[] =
    {
        0x00, 0xbf,               /* 00: nop */
        0x00, 0xbf,               /* 02: nop */
        0x00, 0xbf,               /* 04: nop */
        0x00, 0xbf,               /* 06: nop */
    };

    static const DWORD unwind_info_30_header =
        (sizeof(function_30)/2) | /* function length */
        (0  << 20) | /* X */
        (0  << 21) | /* E */
        (0  << 22) | /* F */
        (0  << 23) | /* epilog */
        (2  << 28);  /* codes, (sizeof(unwind_info_30)-headers+3)/4 */

    static const BYTE unwind_info_30[] =
    {
        DW(unwind_info_30_header),
        UWOP_ALLOC_SMALL(12),         /* sub    sp, sp, #12 */
        UWOP_SAVE_REGS((1<<lr)),      /* push   {lr} */
        UWOP_MSFT_OP_MACHINE_FRAME,
        UWOP_END,
    };

    static const struct results_arm results_30[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     0x04,    0x00, FALSE, { {sp,0x00}, {-1,-1} }},
        { 0x02,  0x10,  0,     0x08,    0x04, FALSE, { {lr,0x00}, {sp,0x04}, {-1,-1} }},
        { 0x04,  0x10,  0,     0x14,    0x10, FALSE, { {lr,0x0c}, {sp,0x10}, {-1,-1} }},
    };

    static const BYTE function_31[] =
    {
        0x00, 0xbf,               /* 00: nop */
        0x00, 0xbf,               /* 02: nop */
    };

    static const struct results_arm results_31[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0,    -1,     ORIG_LR, 0x00, TRUE,  { {-1,-1} }},
        { 0x02,  0,    -1,     ORIG_LR, 0x00, TRUE,  { {-1,-1} }},
        { 0x04,  0,    -2,     0, 0xdeadbeef, FALSE, { {-1,-1} }},
    };

    static const struct unwind_test_arm tests[] =
    {
#define TEST(func, unwind, size, results) \
        { func, sizeof(func), unwind, size, results, ARRAY_SIZE(results) }
        TEST(function_0, unwind_info_0, sizeof(unwind_info_0), results_0),
        TEST(function_1, unwind_info_1, sizeof(unwind_info_1), results_1),
        TEST(function_2, unwind_info_2, sizeof(unwind_info_2), results_2),
        TEST(function_3, unwind_info_3, sizeof(unwind_info_3), results_3),
        TEST(function_4, unwind_info_4, sizeof(unwind_info_4), results_4),
        TEST(function_5, unwind_info_5, sizeof(unwind_info_5), results_5),
        TEST(function_6, unwind_info_6, 0, results_6),
        TEST(function_7, unwind_info_7, 0, results_7),
        TEST(function_8, unwind_info_8, 0, results_8),
        TEST(function_9, unwind_info_9, 0, results_9),
        TEST(function_10, unwind_info_10, 0, results_10),
        TEST(function_11, unwind_info_11, 0, results_11),
        TEST(function_12, unwind_info_12, 0, results_12),
        TEST(function_13, unwind_info_13, 0, results_13),
        TEST(function_14, unwind_info_14, 0, results_14),
        TEST(function_15, unwind_info_15, 0, results_15),
        TEST(function_16, unwind_info_16, 0, results_16),
        TEST(function_17, unwind_info_17, 0, results_17),
        TEST(function_18, unwind_info_18, 0, results_18),
        TEST(function_19, unwind_info_19, 0, results_19),
        TEST(function_20, unwind_info_20, 0, results_20),
        TEST(function_21, unwind_info_21, 0, results_21),
        TEST(function_22, unwind_info_22, 0, results_22),
        TEST(function_23, unwind_info_23, 0, results_23),
        TEST(function_24, unwind_info_24, 0, results_24),
        TEST(function_25, unwind_info_25, 0, results_25),
        TEST(function_26, unwind_info_26, 0, results_26),
        TEST(function_27, unwind_info_27, 0, results_27),
        TEST(function_28, unwind_info_28, 0, results_28),
        TEST(function_29, unwind_info_29, sizeof(unwind_info_29), results_29),
        TEST(function_30, unwind_info_30, sizeof(unwind_info_30), results_30),
        TEST(function_31, NULL,           0, results_31),
#undef TEST
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
        call_virtual_unwind_arm( i, &tests[i] );
}

#endif  /* __arm__ */

#if defined(__aarch64__) || defined(__x86_64__)

#define UWOP_TWOBYTES(x) (((x) >> 8) & 0xff), ((x) & 0xff)

#define UWOP_ALLOC_SMALL(size)         (0x00 | (size/16))
#define UWOP_SAVE_R19R20_X(offset)     (0x20 | (offset/8))
#define UWOP_SAVE_FPLR(offset)         (0x40 | (offset/8))
#define UWOP_SAVE_FPLR_X(offset)       (0x80 | (offset/8 - 1))
#define UWOP_ALLOC_MEDIUM(size)        UWOP_TWOBYTES((0xC0 << 8) | (size/16))
#define UWOP_SAVE_REGP(reg, offset)    UWOP_TWOBYTES((0xC8 << 8) | ((reg - 19) << 6) | (offset/8))
#define UWOP_SAVE_REGP_X(reg, offset)  UWOP_TWOBYTES((0xCC << 8) | ((reg - 19) << 6) | (offset/8 - 1))
#define UWOP_SAVE_REG(reg, offset)     UWOP_TWOBYTES((0xD0 << 8) | ((reg - 19) << 6) | (offset/8))
#define UWOP_SAVE_REG_X(reg, offset)   UWOP_TWOBYTES((0xD4 << 8) | ((reg - 19) << 5) | (offset/8 - 1))
#define UWOP_SAVE_LRP(reg, offset)     UWOP_TWOBYTES((0xD6 << 8) | ((reg - 19)/2 << 6) | (offset/8))
#define UWOP_SAVE_FREGP(reg, offset)   UWOP_TWOBYTES((0xD8 << 8) | ((reg - 8) << 6) | (offset/8))
#define UWOP_SAVE_FREGP_X(reg, offset) UWOP_TWOBYTES((0xDA << 8) | ((reg - 8) << 6) | (offset/8 - 1))
#define UWOP_SAVE_FREG(reg, offset)    UWOP_TWOBYTES((0xDC << 8) | ((reg - 8) << 6) | (offset/8))
#define UWOP_SAVE_FREG_X(reg, offset)  UWOP_TWOBYTES((0xDE << 8) | ((reg - 8) << 5) | (offset/8 - 1))
#define UWOP_ALLOC_LARGE(size)         UWOP_TWOBYTES((0xE0 << 8) | ((size/16) >> 16)), UWOP_TWOBYTES(size/16)
#define UWOP_SET_FP                    0xE1
#define UWOP_ADD_FP(offset)            UWOP_TWOBYTES((0xE2 << 8) | (offset/8))
#define UWOP_NOP                       0xE3
#define UWOP_END                       0xE4
#define UWOP_END_C                     0xE5
#define UWOP_SAVE_NEXT                 0xE6
#define UWOP_SAVE_ANY_REG(reg,offset)  0xE7,(reg),(offset)
#define UWOP_TRAP_FRAME                0xE8
#define UWOP_MACHINE_FRAME             0xE9
#define UWOP_CONTEXT                   0xEA
#define UWOP_EC_CONTEXT                0xEB
#define UWOP_CLEAR_UNWOUND_TO_CALL     0xEC

struct results_arm64
{
    int pc_offset;      /* pc offset from code start */
    int fp_offset;      /* fp offset from stack pointer */
    int handler;        /* expect handler to be set? */
    ULONG_PTR pc;       /* expected final pc value */
    ULONG_PTR frame;    /* expected frame return value */
    int frame_offset;   /* whether the frame return value is an offset or an absolute value */
    ULONG_PTR regs[48][2]; /* expected values for registers */
};

struct unwind_test_arm64
{
    const BYTE *function;
    size_t function_size;
    const BYTE *unwind_info;
    size_t unwind_size;
    const struct results_arm64 *results;
    unsigned int nb_results;
    int unwound_clear;
    int last_set_reg_ptr;
    int stack_value_index;
    ULONG64 stack_value;
};

enum regs_arm64
{
    x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,
    x8,  x9,  x10, x11, x12, x13, x14, x15,
    x16, x17, x18, x19, x20, x21, x22, x23,
    x24, x25, x26, x27, x28, x29, lr,  sp,
    d0,  d1,  d2,  d3,  d4,  d5,  d6,  d7,
    d8,  d9,  d10, d11, d12, d13, d14, d15
};

static const char * const reg_names_arm64[48] =
{
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "lr",  "sp",
    "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
    "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
};

#define ORIG_LR 0xCCCCCCCC

static void call_virtual_unwind_arm64( void *code_mem, int testnum, const struct unwind_test_arm64 *test )
{
    static const int code_offset = 1024;
    static const int unwind_offset = 2048;
    void *data;
#ifdef __x86_64__
    ARM64EC_NT_CONTEXT context, new_context;
#else
    ARM64_NT_CONTEXT context, new_context;
#endif
    PEXCEPTION_ROUTINE handler;
    ARM64_RUNTIME_FUNCTION runtime_func;
    KNONVOLATILE_CONTEXT_POINTERS ctx_ptr;
    UINT i, j, k;
    NTSTATUS status;
    ULONG64 fake_stack[256];
    ULONG64 frame, orig_pc, orig_fp, unset_reg, sp_offset = 0, regval, *regptr;
    static const UINT nb_regs = ARRAY_SIZE(test->results[i].regs);

    memcpy( (char *)code_mem + code_offset, test->function, test->function_size );
    if (test->unwind_info)
    {
        memcpy( (char *)code_mem + unwind_offset, test->unwind_info, test->unwind_size );
        runtime_func.BeginAddress = code_offset;
        if (test->unwind_size)
            runtime_func.UnwindData = unwind_offset;
        else
            memcpy(&runtime_func.UnwindData, test->unwind_info, 4);
    }

    for (i = 0; i < test->nb_results; i++)
    {
#ifdef __x86_64__
        if (test->results[i].handler == -2) continue;  /* skip invalid leaf function test */
#endif
        winetest_push_context( "%u/%u", testnum, i );
        memset( &ctx_ptr, 0x55, sizeof(ctx_ptr) );
        memset( &context, 0x55, sizeof(context) );
        memset( &unset_reg, 0x55, sizeof(unset_reg) );
        for (j = 0; j < 256; j++) fake_stack[j] = j * 8;
        if (test->stack_value_index != -1) fake_stack[test->stack_value_index] = test->stack_value;

        context.Sp = (ULONG_PTR)fake_stack;
        context.Lr = (ULONG_PTR)ORIG_LR;
        context.Fp = (ULONG_PTR)fake_stack + test->results[i].fp_offset;
        context.ContextFlags = 0xcccc;
        if (test->unwound_clear) context.ContextFlags |= CONTEXT_ARM64_UNWOUND_TO_CALL;

        orig_fp = context.Fp;
        orig_pc = (ULONG64)code_mem + code_offset + test->results[i].pc_offset;

        trace( "pc=%p (%02x) fp=%p sp=%p\n", (void *)orig_pc, *(UINT *)orig_pc, (void *)orig_fp, (void *)context.Sp );

        if (test->results[i].handler == -2) orig_pc = context.Lr;

        if (pRtlVirtualUnwind2)
        {
            new_context = context;
            handler = (void *)0xdeadbeef;
            data = (void *)0xdeadbeef;
            frame = 0xdeadbeef;
            status = pRtlVirtualUnwind2( UNW_FLAG_EHANDLER, (ULONG_PTR)code_mem, orig_pc,
                                         test->unwind_info ? (RUNTIME_FUNCTION *)&runtime_func : NULL,
                                         (CONTEXT *)&new_context, NULL, &data,
                                         &frame, &ctx_ptr, NULL, NULL, &handler, 0 );
            if (test->results[i].handler > 0)
            {
                ok( !status, "RtlVirtualUnwind2 failed %lx\n", status );
                ok( (char *)handler == (char *)code_mem + 0x200,
                    "wrong handler %p/%p\n", handler, (char *)code_mem + 0x200 );
                if (handler) ok( *(DWORD *)data == 0x08070605,
                                 "wrong handler data %lx\n", *(DWORD *)data );
            }
            else if (test->results[i].handler < -1)
            {
                ok( status == STATUS_BAD_FUNCTION_TABLE, "RtlVirtualUnwind2 failed %lx\n", status );
                ok( handler == (void *)0xdeadbeef, "handler set to %p\n", handler );
                ok( data == (void *)0xdeadbeef, "handler data set to %p\n", data );
            }
            else
            {
                ok( !status, "RtlVirtualUnwind2 failed %lx\n", status );
                ok( handler == NULL, "handler %p instead of NULL\n", handler );
                ok( data == NULL, "handler data set to %p\n", data );
            }
        }

        data = (void *)0xdeadbeef;
        frame = 0xdeadbeef;
        handler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, (ULONG64)code_mem, orig_pc,
                                    test->unwind_info ? (RUNTIME_FUNCTION *)&runtime_func : NULL,
                                    (CONTEXT *)&context, &data, &frame, &ctx_ptr );
        if (test->results[i].handler > 0)
        {
            ok( (char *)handler == (char *)code_mem + 0x200,
                "wrong handler %p/%p\n", handler, (char *)code_mem + 0x200 );
            if (handler) ok( *(DWORD *)data == 0x08070605,
                             "wrong handler data %lx\n", *(DWORD *)data );
        }
        else
        {
            ok( handler == NULL, "handler %p instead of NULL\n", handler );
            ok( data == (test->results[i].handler < -1 ? (void *)0xdeadbeef : NULL),
                "handler data set to %p/%p\n", data,
                (test->results[i].handler < 0 ? (void *)0xdeadbeef : NULL) );
        }

        ok( context.Pc == test->results[i].pc, "wrong pc %p/%p\n",
            (void *)context.Pc, (void*)test->results[i].pc );
        ok( frame == (test->results[i].frame_offset ? (ULONG64)fake_stack : 0) + test->results[i].frame, "wrong frame %p/%p\n",
            (void *)frame, (char *)(test->results[i].frame_offset ? fake_stack : NULL) + test->results[i].frame );
        if (test->results[i].handler == -2) /* invalid leaf function */
        {
            ok( context.ContextFlags == 0xcccc, "wrong flags %lx\n", context.ContextFlags );
            ok( context.Sp == (ULONG_PTR)fake_stack, "wrong sp %p/%p\n", (void *)context.Sp, fake_stack);
        }
        else
        {
            if (!test->unwound_clear || i < test->unwound_clear)
                ok( context.ContextFlags == (0xcccc | CONTEXT_ARM64_UNWOUND_TO_CALL),
                    "wrong flags %lx\n", context.ContextFlags );
            else
                ok( context.ContextFlags == 0xcccc,
                    "wrong flags %lx\n", context.ContextFlags );

            sp_offset = 0;
            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                    break;
                if (test->results[i].regs[k][0] == sp) {
                    /* If sp is part of the registers list, treat it as an offset
                     * between the returned frame pointer and the sp register. */
                    sp_offset = test->results[i].regs[k][1];
                    break;
                }
            }
            ok( frame - sp_offset == context.Sp, "wrong sp %p/%p\n",
                (void *)(frame - sp_offset), (void *)context.Sp);
        }

#ifdef __x86_64__
        for (j = 0; j < sizeof(ctx_ptr)/sizeof(void*); j++)
            ok( ((void **)&ctx_ptr)[j] == (void *)unset_reg,
                "ctx_ptr %u set to %p\n", j, ((void **)&ctx_ptr)[j] );
#endif

        for (j = 0; j < 48; j++)
        {
            switch (j)
            {
#define GET(i) case i: regval = context.X##i; break
            GET(0); GET(1); GET(2); GET(3); GET(4); GET(5); GET(6); GET(7);
            GET(8); GET(9); GET(10); GET(11); GET(12);
            GET(15); GET(19); GET(20); GET(21); GET(22); GET(25); GET(26); GET(27);
#ifdef __x86_64__
            case x13: case x14: continue;
            case x16: regval = context.X16_0 | ((DWORD64)context.X16_1 << 16) | ((DWORD64)context.X16_2 << 32) | ((DWORD64)context.X16_3 << 48); break;
            case x17: regval = context.X17_0 | ((DWORD64)context.X17_1 << 16) | ((DWORD64)context.X17_2 << 32) | ((DWORD64)context.X17_3 << 48); break;
            case x18: case x23: case x24: case x28: continue;
#else
            GET(13); GET(14); GET(16); GET(17); GET(18); GET(23); GET(24); GET(28);
#endif
#undef GET
            case x29: regval = context.Fp; break;
            case lr: regval = context.Lr; break;
            case sp: continue; /* Handling sp separately above */
            default: regval = context.V[j - d0].Low; break;
            }

            regptr = NULL;
#ifndef __x86_64__
            if (j >= 19 && j <= 30) regptr = (&ctx_ptr.X19)[j - 19];
            else if (j >= d8 && j <= d15) regptr = (&ctx_ptr.D8)[j - d8];
#endif

            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                {
                    k = nb_regs;
                    break;
                }
                if (test->results[i].regs[k][0] == j) break;
            }

            if (k < nb_regs)
            {
                ok( regval == test->results[i].regs[k][1],
                    "register %s wrong %I64x/%I64x\n", reg_names_arm64[j], regval, test->results[i].regs[k][1] );
                if (regptr)
                {
                    if (test->last_set_reg_ptr && j > test->last_set_reg_ptr && j <= 30)
                        ok( regptr == (void *)unset_reg, "register %s should not have pointer set\n", reg_names_arm64[j] );
                    else
                    {
                        ok( regptr != (void *)unset_reg, "register %s should have pointer set\n", reg_names_arm64[j] );
                        if (regptr != (void *)unset_reg)
                            ok( *regptr == regval, "register %s should have reg pointer to %I64x / %I64x\n",
                                reg_names_arm64[j], *regptr, regval );
                    }
                }
            }
            else
            {
                ok( k == nb_regs, "register %s should be set\n", reg_names_arm64[j] );
                ok( !regptr || regptr == (void *)unset_reg, "register %s should not have pointer set\n", reg_names_arm64[j] );
                if (j == lr)
                    ok( context.Lr == ORIG_LR, "register lr wrong %I64x/unset\n", context.Lr );
                else if (j == x29)
                    ok( context.Fp == orig_fp, "register fp wrong %I64x/unset\n", context.Fp );
                else
                    ok( regval == unset_reg, "register %s wrong %I64x/unset\n", reg_names_arm64[j], regval);
            }
        }
        winetest_pop_context();
    }
}

#ifndef __REACTOS__
#define DW(dword) ((dword >> 0) & 0xff), ((dword >> 8) & 0xff), ((dword >> 16) & 0xff), ((dword >> 24) & 0xff)

static void test_virtual_unwind_arm64(void)
{
    static const BYTE function_0[] =
    {
        0xff, 0x83, 0x00, 0xd1,   /* 00: sub sp, sp, #32 */
        0xf3, 0x53, 0x01, 0xa9,   /* 04: stp x19, x20, [sp, #16] */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xf3, 0x53, 0x41, 0xa9,   /* 0c: ldp x19, x20, [sp, #16] */
        0xff, 0x83, 0x00, 0x91,   /* 10: add sp, sp, #32 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_0_header =
        (sizeof(function_0)/4) | /* function length */
        (1 << 20) | /* X */
        (0 << 21) | /* E */
        (1 << 22) | /* epilog */
        (2 << 27);  /* codes */
    static const DWORD unwind_info_0_epilog0 =
        (3 <<  0) | /* offset */
        (4 << 22);  /* index */

    static const BYTE unwind_info_0[] =
    {
        DW(unwind_info_0_header),
        DW(unwind_info_0_epilog0),

        UWOP_SAVE_REGP(19, 16), /* stp x19, x20, [sp, #16] */
        UWOP_ALLOC_SMALL(32),   /* sub sp,  sp,  #32 */
        UWOP_END,

        UWOP_SAVE_REGP(19, 16), /* stp x19, x20, [sp, #16] */
        UWOP_ALLOC_SMALL(32),   /* sub sp,  sp,  #32 */
        UWOP_END,

        0x00, 0x02, 0x00, 0x00, /* handler */
        0x05, 0x06, 0x07, 0x08, /* data */
    };

    static const struct results_arm64 results_0[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {-1,-1} }},
        { 0x08,  0x00,  1,     ORIG_LR, 0x020, TRUE, { {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };


    static const BYTE function_1[] =
    {
        0xf3, 0x53, 0xbe, 0xa9,   /* 00: stp x19, x20, [sp, #-32]! */
        0xfe, 0x0b, 0x00, 0xf9,   /* 04: str x30, [sp, #16] */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xff, 0x43, 0x00, 0x91,   /* 10: add sp, sp, #16 */
        0xfe, 0x0b, 0x40, 0xf9,   /* 14: ldr x30, [sp, #16] */
        0xf3, 0x53, 0xc2, 0xa8,   /* 18: ldp x19, x20, [sp], #32 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 1c: ret */
    };

    static const DWORD unwind_info_1_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_1)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (2 << 16) | /* RegI */
        (0 << 20) | /* H */
        (1 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_1[] = { DW(unwind_info_1_packed) };

    static const struct results_arm64 results_1[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x10,    0x020, TRUE, { {x19,0x00}, {x20,0x08}, {lr,0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x20,    0x030, TRUE, { {x19,0x10}, {x20,0x18}, {lr,0x20}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x20,    0x030, TRUE, { {x19,0x10}, {x20,0x18}, {lr,0x20}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x10,    0x020, TRUE, { {x19,0x00}, {x20,0x08}, {lr,0x10}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_2[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 10: ret */
    };

    static const DWORD unwind_info_2_header =
        (sizeof(function_2)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (1 << 27);  /* codes */

    static const BYTE unwind_info_2[] =
    {
        DW(unwind_info_2_header),

        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_MACHINE_FRAME,
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_END,
    };

    /* Partial prologues with the custom frame opcodes (machine frame,
     * context) behave like there's one less instruction to skip, because the
     * custom frame is set up externally without an explicit instruction. */
    static const struct results_arm64 results_2[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x010, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  0,     0x0008,  0x010, FALSE, { {-1,-1} }},
        { 0x08,  0x00,  0,     0x0018,  0x020, FALSE, { {-1,-1} }},
        { 0x0c,  0x00,  0,     0x0018,  0x020, FALSE, { {-1,-1} }},
        { 0x10,  0x00,  0,     0x0018,  0x020, FALSE, { {-1,-1} }},
    };

    static const BYTE function_3[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 10: ret */
    };

    static const DWORD unwind_info_3_header =
        (sizeof(function_3)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (1 << 27);  /* codes */

    static const BYTE unwind_info_3[] =
    {
        DW(unwind_info_3_header),

        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_CONTEXT,
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_END,
    };

    static const struct results_arm64 results_3[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x010, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  0 ,    0x0108,  0x110, FALSE, { {x0, 0x08}, {x1, 0x10}, {x2, 0x18}, {x3, 0x20}, {x4, 0x28}, {x5, 0x30}, {x6, 0x38}, {x7, 0x40}, {x8, 0x48}, {x9, 0x50}, {x10, 0x58}, {x11, 0x60}, {x12, 0x68}, {x13, 0x70}, {x14, 0x78}, {x15, 0x80}, {x16, 0x88}, {x17, 0x90}, {x18, 0x98}, {x19, 0xA0}, {x20, 0xA8}, {x21, 0xB0}, {x22, 0xB8}, {x23, 0xC0}, {x24, 0xC8}, {x25, 0xD0}, {x26, 0xD8}, {x27, 0xE0}, {x28, 0xE8}, {x29, 0xF0}, {lr, 0xF8}, {d0, 0x110}, {d1, 0x120}, {d2, 0x130}, {d3, 0x140}, {d4, 0x150}, {d5, 0x160}, {d6, 0x170}, {d7, 0x180}, {d8, 0x190}, {d9, 0x1a0}, {d10, 0x1b0}, {d11, 0x1c0}, {d12, 0x1d0}, {d13, 0x1e0}, {d14, 0x1f0}, {d15, 0x200}, {-1,-1} }},
        { 0x08,  0x00,  0 ,    0x0118,  0x120, FALSE, { {x0, 0x18}, {x1, 0x20}, {x2, 0x28}, {x3, 0x30}, {x4, 0x38}, {x5, 0x40}, {x6, 0x48}, {x7, 0x50}, {x8, 0x58}, {x9, 0x60}, {x10, 0x68}, {x11, 0x70}, {x12, 0x78}, {x13, 0x80}, {x14, 0x88}, {x15, 0x90}, {x16, 0x98}, {x17, 0xA0}, {x18, 0xA8}, {x19, 0xB0}, {x20, 0xB8}, {x21, 0xC0}, {x22, 0xC8}, {x23, 0xD0}, {x24, 0xD8}, {x25, 0xE0}, {x26, 0xE8}, {x27, 0xF0}, {x28, 0xF8}, {x29, 0x100}, {lr, 0x108}, {d0, 0x120}, {d1, 0x130}, {d2, 0x140}, {d3, 0x150}, {d4, 0x160}, {d5, 0x170}, {d6, 0x180}, {d7, 0x190}, {d8, 0x1a0}, {d9, 0x1b0}, {d10, 0x1c0}, {d11, 0x1d0}, {d12, 0x1e0}, {d13, 0x1f0}, {d14, 0x200}, {d15, 0x210}, {-1,-1} }},
        { 0x0c,  0x00,  0 ,    0x0118,  0x120, FALSE, { {x0, 0x18}, {x1, 0x20}, {x2, 0x28}, {x3, 0x30}, {x4, 0x38}, {x5, 0x40}, {x6, 0x48}, {x7, 0x50}, {x8, 0x58}, {x9, 0x60}, {x10, 0x68}, {x11, 0x70}, {x12, 0x78}, {x13, 0x80}, {x14, 0x88}, {x15, 0x90}, {x16, 0x98}, {x17, 0xA0}, {x18, 0xA8}, {x19, 0xB0}, {x20, 0xB8}, {x21, 0xC0}, {x22, 0xC8}, {x23, 0xD0}, {x24, 0xD8}, {x25, 0xE0}, {x26, 0xE8}, {x27, 0xF0}, {x28, 0xF8}, {x29, 0x100}, {lr, 0x108}, {d0, 0x120}, {d1, 0x130}, {d2, 0x140}, {d3, 0x150}, {d4, 0x160}, {d5, 0x170}, {d6, 0x180}, {d7, 0x190}, {d8, 0x1a0}, {d9, 0x1b0}, {d10, 0x1c0}, {d11, 0x1d0}, {d12, 0x1e0}, {d13, 0x1f0}, {d14, 0x200}, {d15, 0x210}, {-1,-1} }},
        { 0x10,  0x00,  0 ,    0x0118,  0x120, FALSE, { {x0, 0x18}, {x1, 0x20}, {x2, 0x28}, {x3, 0x30}, {x4, 0x38}, {x5, 0x40}, {x6, 0x48}, {x7, 0x50}, {x8, 0x58}, {x9, 0x60}, {x10, 0x68}, {x11, 0x70}, {x12, 0x78}, {x13, 0x80}, {x14, 0x88}, {x15, 0x90}, {x16, 0x98}, {x17, 0xA0}, {x18, 0xA8}, {x19, 0xB0}, {x20, 0xB8}, {x21, 0xC0}, {x22, 0xC8}, {x23, 0xD0}, {x24, 0xD8}, {x25, 0xE0}, {x26, 0xE8}, {x27, 0xF0}, {x28, 0xF8}, {x29, 0x100}, {lr, 0x108}, {d0, 0x120}, {d1, 0x130}, {d2, 0x140}, {d3, 0x150}, {d4, 0x160}, {d5, 0x170}, {d6, 0x180}, {d7, 0x190}, {d8, 0x1a0}, {d9, 0x1b0}, {d10, 0x1c0}, {d11, 0x1d0}, {d12, 0x1e0}, {d13, 0x1f0}, {d14, 0x200}, {d15, 0x210}, {-1,-1} }},
    };

    static const BYTE function_4[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp,  sp,  #16 */
        0xff, 0x03, 0x08, 0xd1,   /* 04: sub sp,  sp,  #512 */
        0xff, 0x43, 0x40, 0xd1,   /* 08: sub sp,  sp,  #65536 */
        0xfd, 0x03, 0x00, 0x91,   /* 0c: mov x29, sp */
        0xf3, 0x53, 0xbe, 0xa9,   /* 10: stp x19, x20, [sp, #-32]! */
        0xf5, 0x5b, 0x01, 0xa9,   /* 14: stp x21, x22, [sp, #16] */
        0xf7, 0x0f, 0x1e, 0xf8,   /* 18: str x23,      [sp, #-32]! */
        0xf8, 0x07, 0x00, 0xf9,   /* 1c: str x24,      [sp, #8] */
        0xf9, 0x7b, 0x01, 0xa9,   /* 20: stp x25, x30, [sp, #16] */
        0xfd, 0x7b, 0x03, 0xa9,   /* 24: stp x29, x30, [sp, #48] */
        0xfd, 0x7b, 0xbe, 0xa9,   /* 28: stp x29, x30, [sp, #-32]! */
        0xf3, 0x53, 0xbe, 0xa9,   /* 2c: stp x19, x20, [sp, #-32]! */
        0xe8, 0x27, 0xbe, 0x6d,   /* 30: stp d8,  d9,  [sp, #-32]! */
        0xea, 0x2f, 0x01, 0x6d,   /* 34: stp d10, d11, [sp, #16] */
        0xec, 0x0f, 0x1e, 0xfc,   /* 38: str d12,      [sp, #-32]! */
        0xed, 0x07, 0x00, 0xfd,   /* 3c: str d13,      [sp, #8] */
        0xfd, 0x43, 0x00, 0x91,   /* 40: add x29, sp,  #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 44: ret */
    };

    static const DWORD unwind_info_4_header =
        (sizeof(function_4)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (8 << 27);  /* codes */

    static const BYTE unwind_info_4[] =
    {
        DW(unwind_info_4_header),

        UWOP_ADD_FP(16),          /* 40: add x29, sp, #16 */
        UWOP_SAVE_FREG(13, 8),    /* 3c: str d13,      [sp, #8] */
        UWOP_SAVE_FREG_X(12, 32), /* 38: str d12,      [sp, #-32]! */
        UWOP_SAVE_FREGP(10, 16),  /* 34: stp d10, d11, [sp, #16] */
        UWOP_SAVE_FREGP_X(8, 32), /* 30: stp d8,  d9,  [sp, #-32]! */
        UWOP_SAVE_R19R20_X(32),   /* 2c: stp x19, x20, [sp, #-32]! */
        UWOP_SAVE_FPLR_X(32),     /* 28: stp x29, x30, [sp, #-32]! */
        UWOP_SAVE_FPLR(16),       /* 24: stp x29, x30, [sp, #16] */
        UWOP_SAVE_LRP(25, 16),    /* 20: stp x25, x30, [sp, #16] */
        UWOP_SAVE_REG(24, 8),     /* 1c: str x24,      [sp, #8] */
        UWOP_SAVE_REG_X(23, 32),  /* 18: str x23,      [sp, #-32]! */
        UWOP_SAVE_REGP(21, 16),   /* 14: stp x21, x22, [sp, #16] */
        UWOP_SAVE_REGP_X(19, 32), /* 10: stp x19, x20, [sp, #-32]! */
        UWOP_SET_FP,              /* 0c: mov x29, sp */
        UWOP_ALLOC_LARGE(65536),  /* 08: sub sp,  sp,  #65536 */
        UWOP_ALLOC_MEDIUM(512),   /* 04: sub sp,  sp,  #512 */
        UWOP_ALLOC_SMALL(16),     /* 00: sub sp,  sp,  #16 */
        UWOP_END,
    };

    static const struct results_arm64 results_4[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x00000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x00010, TRUE, { {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x00210, TRUE, { {-1,-1} }},
        { 0x0c,  0x10,  0,     ORIG_LR, 0x10210, TRUE, { {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {x22, 0x18}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x10210, TRUE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {x24, 0x08}, {-1,-1} }},
        { 0x24,  0x00,  0,     0x0018,  0x10210, TRUE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x28,  0x00,  0,     0x0018,  0x10220, FALSE, { {x19, 0x20}, {x20, 0x28}, {x21, 0x30}, {x22, 0x38}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {lr, 0x18}, {x29, 0x10}, {-1,-1} }},
        { 0x2c,  0x00,  0,     0x0038,  0x10240, FALSE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x20}, {x24, 0x28}, {x25, 0x30}, {lr, 0x38}, {x29, 0x30}, {-1,-1} }},
        { 0x30,  0x00,  0,     0x0058,  0x10260, FALSE, { {x19, 0x60}, {x20, 0x68}, {x21, 0x70}, {x22, 0x78}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {lr, 0x58}, {x29, 0x50}, {-1,-1} }},
        { 0x34,  0x00,  0,     0x0078,  0x10280, FALSE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x60}, {x24, 0x68}, {x25, 0x70}, {lr, 0x78}, {x29, 0x70}, {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
        { 0x38,  0x00,  0,     0x0078,  0x10280, FALSE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x60}, {x24, 0x68}, {x25, 0x70}, {lr, 0x78}, {x29, 0x70}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {-1,-1} }},
        { 0x3c,  0x00,  0,     0x0098,  0x102a0, FALSE, { {x19, 0xa0}, {x20, 0xa8}, {x21, 0xb0}, {x22, 0xb8}, {x23, 0x80}, {x24, 0x88}, {x25, 0x90}, {lr, 0x98}, {x29, 0x90}, {d8, 0x20}, {d9, 0x28}, {d10, 0x30}, {d11, 0x38}, {d12, 0x00}, {-1,-1} }},
        { 0x40,  0x00,  0,     0x0098,  0x102a0, FALSE, { {x19, 0xa0}, {x20, 0xa8}, {x21, 0xb0}, {x22, 0xb8}, {x23, 0x80}, {x24, 0x88}, {x25, 0x90}, {lr, 0x98}, {x29, 0x90}, {d8, 0x20}, {d9, 0x28}, {d10, 0x30}, {d11, 0x38}, {d12, 0x00}, {d13, 0x08}, {-1,-1} }},
        { 0x44,  0x20,  0,     0x00a8,  0x102b0, FALSE, { {x19, 0xb0}, {x20, 0xb8}, {x21, 0xc0}, {x22, 0xc8}, {x23, 0x90}, {x24, 0x98}, {x25, 0xa0}, {lr, 0xa8}, {x29, 0xa0}, {d8, 0x30}, {d9, 0x38}, {d10, 0x40}, {d11, 0x48}, {d12, 0x10}, {d13, 0x18}, {-1,-1} }},
    };

    static const BYTE function_5[] =
    {
        0xf3, 0x53, 0xbe, 0xa9,   /* 00: stp x19, x20, [sp, #-32]! */
        0xf5, 0x5b, 0x01, 0xa9,   /* 04: stp x21, x22, [sp, #16] */
        0xf7, 0x63, 0xbc, 0xa9,   /* 08: stp x23, x24, [sp, #-64]! */
        0xf9, 0x6b, 0x01, 0xa9,   /* 0c: stp x25, x26, [sp, #16] */
        0xfb, 0x73, 0x02, 0xa9,   /* 10: stp x27, x28, [sp, #32] */
        0xfd, 0x7b, 0x03, 0xa9,   /* 14: stp x29, x30, [sp, #48] */
        0xe8, 0x27, 0xbc, 0x6d,   /* 18: stp d8,  d9,  [sp, #-64]! */
        0xea, 0x2f, 0x01, 0x6d,   /* 1c: stp d10, d11, [sp, #16] */
        0xec, 0x37, 0x02, 0x6d,   /* 20: stp d12, d13, [sp, #32] */
        0xee, 0x3f, 0x03, 0x6d,   /* 24: stp d14, d15, [sp, #48] */
        0xc0, 0x03, 0x5f, 0xd6,   /* 28: ret */
    };

    static const DWORD unwind_info_5_header =
        (sizeof(function_5)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (4 << 27);  /* codes */

    static const BYTE unwind_info_5[] =
    {
        DW(unwind_info_5_header),

        UWOP_SAVE_NEXT,           /* 24: stp d14, d15, [sp, #48] */
        UWOP_SAVE_FREGP(12, 32),  /* 20: stp d12, d13, [sp, #32] */
        UWOP_SAVE_NEXT,           /* 1c: stp d10, d11, [sp, #16] */
        UWOP_SAVE_FREGP_X(8, 64), /* 18: stp d8,  d9,  [sp, #-64]! */
        UWOP_SAVE_NEXT,           /* 14: stp x29, x30, [sp, #48] */
        UWOP_SAVE_REGP(27, 32),   /* 10: stp x27, x28, [sp, #32] */
        UWOP_SAVE_NEXT,           /* 0c: stp x25, x26, [sp, #16] */
        UWOP_SAVE_REGP_X(23, 64), /* 08: stp x23, x24, [sp, #-64]! */
        UWOP_SAVE_NEXT,           /* 04: stp x21, x22, [sp, #16] */
        UWOP_SAVE_R19R20_X(32),   /* 00: stp x19, x20, [sp, #-32]! */
        UWOP_END,
        UWOP_NOP                  /* padding */
    };

    /* Windows seems to only save one register for UWOP_SAVE_NEXT for
     * float registers, contrary to what the documentation says. The tests
     * for those cases are commented out; they succeed in wine but fail
     * on native windows. */
    static const struct results_arm64 results_5[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x00000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x00020, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x00020, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {x22, 0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {x26, 0x18}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {x26, 0x18}, {x27, 0x20}, {x28, 0x28}, {-1,-1} }},
        { 0x18,  0x00,  0,     0x38,    0x00060, TRUE, { {x19, 0x40}, {x20, 0x48}, {x21, 0x50}, {x22, 0x58}, {x23, 0x00}, {x24, 0x08}, {x25, 0x10}, {x26, 0x18}, {x27, 0x20}, {x28, 0x28}, {x29, 0x30}, {lr, 0x38}, {-1,-1} }},
        { 0x1c,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
#if 0
        { 0x20,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {-1,-1} }},
        { 0x24,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {d12, 0x20}, {d13, 0x28}, {-1,-1} }},
        { 0x28,  0x00,  0,     0x78,    0x000a0, TRUE, { {x19, 0x80}, {x20, 0x88}, {x21, 0x90}, {x22, 0x98}, {x23, 0x40}, {x24, 0x48}, {x25, 0x50}, {x26, 0x58}, {x27, 0x60}, {x28, 0x68}, {x29, 0x70}, {lr, 0x78}, {d8, 0x00}, {d9, 0x08}, {d10, 0x10}, {d11, 0x18}, {d12, 0x20}, {d13, 0x28}, {d14, 0x30}, {d15, 0x38}, {-1,-1} }},
#endif
    };

    static const BYTE function_6[] =
    {
        0xf3, 0x53, 0xbd, 0xa9,   /* 00: stp x19, x20, [sp, #-48]! */
        0xf5, 0x0b, 0x00, 0xf9,   /* 04: str x21,      [sp, #16] */
        0xe8, 0xa7, 0x01, 0x6d,   /* 08: stp d8,  d9,  [sp, #24] */
        0xea, 0x17, 0x00, 0xfd,   /* 0c: str d10,      [sp, #40] */
        0xff, 0x03, 0x00, 0xd1,   /* 10: sub sp,  sp,  #0 */
        0x1f, 0x20, 0x03, 0xd5,   /* 14: nop */
        0xff, 0x03, 0x00, 0x91,   /* 18: add sp,  sp,  #0 */
        0xea, 0x17, 0x40, 0xfd,   /* 1c: ldr d10,      [sp, #40] */
        0xe8, 0xa7, 0x41, 0x6d,   /* 20: ldp d8,  d9,  [sp, #24] */
        0xf5, 0x0b, 0x40, 0xf9,   /* 24: ldr x21,      [sp, #16] */
        0xf3, 0x53, 0xc3, 0xa8,   /* 28: ldp x19, x20, [sp], #48 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 2c: ret */
    };

    static const DWORD unwind_info_6_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_6)/4 << 2) | /* FunctionLength */
        (2 << 13) | /* RegF */
        (3 << 16) | /* RegI */
        (0 << 20) | /* H */
        (0 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_6[] = { DW(unwind_info_6_packed) };

    static const struct results_arm64 results_6[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {d10, 0x28}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {d8, 0x18}, {d9, 0x20}, {-1,-1} }},
        { 0x24,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {x21, 0x10}, {-1,-1} }},
        { 0x28,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19,0x00}, {x20,0x08}, {-1,-1} }},
        { 0x2c,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_7[] =
    {
        0xf3, 0x0f, 0x1d, 0xf8,   /* 00: str x19,      [sp, #-48]! */
        0xe8, 0xa7, 0x00, 0x6d,   /* 04: stp d8,  d9,  [sp, #8] */
        0xea, 0xaf, 0x01, 0x6d,   /* 08: stp d10, d11, [sp, #24] */
        0xff, 0x03, 0x00, 0xd1,   /* 0c: sub sp,  sp,  #0 */
        0x1f, 0x20, 0x03, 0xd5,   /* 10: nop */
        0xff, 0x03, 0x00, 0x91,   /* 14: add sp,  sp,  #0 */
        0xea, 0xaf, 0x41, 0x6d,   /* 18: ldp d10, d11, [sp, #24] */
        0xe8, 0xa7, 0x40, 0x6d,   /* 1c: ldp d8,  d9,  [sp, #8] */
        0xf3, 0x07, 0x43, 0xf8,   /* 20: ldr x19,      [sp], #48 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 24: ret */
    };

    static const DWORD unwind_info_7_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_7)/4 << 2) | /* FunctionLength */
        (3 << 13) | /* RegF */
        (1 << 16) | /* RegI */
        (0 << 20) | /* H */
        (0 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_7[] = { DW(unwind_info_7_packed) };

    static const struct results_arm64 results_7[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {d10, 0x18}, {d11, 0x20}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {d8, 0x08}, {d9, 0x10}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x24,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_8[] =
    {
        0xe8, 0x27, 0xbf, 0x6d,   /* 00: stp d8,  d9,  [sp, #-16]! */
        0xff, 0x83, 0x00, 0xd1,   /* 04: sub sp,  sp,  #32 */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xff, 0x83, 0x00, 0x91,   /* 0c: add sp,  sp,  #32 */
        0xe8, 0x27, 0xc1, 0x6c,   /* 10: ldp d8,  d9,  [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_8_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_8)/4 << 2) | /* FunctionLength */
        (1 << 13) | /* RegF */
        (0 << 16) | /* RegI */
        (0 << 20) | /* H */
        (0 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_8[] = { DW(unwind_info_8_packed) };

    static const struct results_arm64 results_8[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x010, TRUE, { {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {d8, 0x20}, {d9, 0x28}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x030, TRUE, { {d8, 0x20}, {d9, 0x28}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x010, TRUE, { {d8, 0x00}, {d9, 0x08}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_9[] =
    {
        0xf3, 0x0f, 0x1b, 0xf8,   /* 00: str x19,      [sp, #-80]! */
        0xe0, 0x87, 0x00, 0xa9,   /* 04: stp x0,  x1,  [sp, #8] */
        0xe2, 0x8f, 0x01, 0xa9,   /* 08: stp x2,  x3,  [sp, #24] */
        0xe4, 0x97, 0x02, 0xa9,   /* 0c: stp x4,  x5,  [sp, #40] */
        0xe6, 0x9f, 0x03, 0xa9,   /* 10: stp x6,  x7,  [sp, #56] */
        0xff, 0x83, 0x00, 0xd1,   /* 14: sub sp,  sp,  #32 */
        0x1f, 0x20, 0x03, 0xd5,   /* 18: nop */
        0xff, 0x83, 0x00, 0x91,   /* 1c: add sp,  sp,  #32 */
        0x1f, 0x20, 0x03, 0xd5,   /* 20: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 24: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 28: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 2c: nop */
        0xf3, 0x0f, 0x1b, 0xf8,   /* 30: ldr x19,      [sp], #80 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 34: ret */
    };

    static const DWORD unwind_info_9_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_9)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (1 << 16) | /* RegI */
        (1 << 20) | /* H */
        (0 << 21) | /* CR */
        (7 << 23);  /* FrameSize */

    static const BYTE unwind_info_9[] = { DW(unwind_info_9_packed) };

    static const struct results_arm64 results_9[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x20,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x24,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x28,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x2c,  0x00,  0,     ORIG_LR, 0x070, TRUE, { {x19, 0x20}, {-1,-1} }},
        { 0x30,  0x00,  0,     ORIG_LR, 0x050, TRUE, { {x19, 0x00}, {-1,-1} }},
        { 0x34,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_10[] =
    {
        0xfe, 0x0f, 0x1f, 0xf8,   /* 00: str lr,       [sp, #-16]! */
        0xff, 0x43, 0x00, 0xd1,   /* 04: sub sp,  sp,  #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xff, 0x43, 0x00, 0x91,   /* 0c: add sp,  sp,  #16 */
        0xfe, 0x07, 0x41, 0xf8,   /* 10: ldr lr,       [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_10_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_10)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (0 << 16) | /* RegI */
        (0 << 20) | /* H */
        (1 << 21) | /* CR */
        (2 << 23);  /* FrameSize */

    static const BYTE unwind_info_10[] = { DW(unwind_info_10_packed) };

    static const struct results_arm64 results_10[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     0x00,    0x010, TRUE, { {lr, 0x00}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x10,    0x020, TRUE, { {lr, 0x10}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x10,    0x020, TRUE, { {lr, 0x10}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x00,    0x010, TRUE, { {lr, 0x00}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_11[] =
    {
        0xf3, 0x53, 0xbe, 0xa9,   /* 00: stp x19, x20, [sp, #-32]! */
        0xf5, 0x7b, 0x01, 0xa9,   /* 04: stp x21, lr,  [sp, #16] */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp,  sp,  #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xff, 0x43, 0x00, 0x91,   /* 10: add sp,  sp,  #16 */
        0xf5, 0x7b, 0x41, 0xa9,   /* 14: ldp x21, lr,  [sp, #16] */
        0xf3, 0x53, 0xc2, 0xa8,   /* 18: ldp x19, x20, [sp], #32 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 1c: ret */
    };

    static const DWORD unwind_info_11_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_11)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (3 << 16) | /* RegI */
        (0 << 20) | /* H */
        (1 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_11[] = { DW(unwind_info_11_packed) };

    static const struct results_arm64 results_11[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x18,    0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x28,    0x030, TRUE, { {x19, 0x10}, {x20, 0x18}, {x21, 0x20}, {lr, 0x28}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x28,    0x030, TRUE, { {x19, 0x10}, {x20, 0x18}, {x21, 0x20}, {lr, 0x28}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x18,    0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {x21, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x18,  0x00,  0,     ORIG_LR, 0x020, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x1c,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_12[] =
    {
        0xf3, 0x53, 0xbf, 0xa9,   /* 00: stp x19, x20, [sp, #-16]! */
        0xfd, 0x7b, 0xbe, 0xa9,   /* 04: stp x29, lr,  [sp, #-32]! */
        0xfd, 0x03, 0x00, 0x91,   /* 08: mov x29, sp */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xbf, 0x03, 0x00, 0x91,   /* 10: mov sp,  x29 */
        0xfd, 0x7b, 0xc2, 0xa8,   /* 14: ldp x29, lr,  [sp], #32 */
        0xf3, 0x53, 0xc1, 0xa8,   /* 18: ldp x19, x20, [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 1c: ret */
    };

    static const DWORD unwind_info_12_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_12)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (2 << 16) | /* RegI */
        (0 << 20) | /* H */
        (3 << 21) | /* CR */
        (3 << 23);  /* FrameSize */

    static const BYTE unwind_info_12[] = { DW(unwind_info_12_packed) };

    static const struct results_arm64 results_12[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x10,  0,     0x08,    0x030, TRUE, { {x19, 0x20}, {x20, 0x28}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x18,    0x040, TRUE, { {x19, 0x30}, {x20, 0x38}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x10,  0x10,  0,     0x18,    0x040, TRUE, { {x19, 0x30}, {x20, 0x38}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x14,  0x10,  0,     0x08,    0x030, TRUE, { {x19, 0x20}, {x20, 0x28}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x18,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x1c,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_13[] =
    {
        0xf3, 0x53, 0xbf, 0xa9,   /* 00: stp x19, x20, [sp, #-16]! */
        0xff, 0x43, 0x08, 0xd1,   /* 04: sub sp,  sp,  #528 */
        0xfd, 0x7b, 0x00, 0xd1,   /* 08: stp x29, lr,  [sp] */
        0xfd, 0x03, 0x00, 0x91,   /* 0c: mov x29, sp */
        0x1f, 0x20, 0x03, 0xd5,   /* 10: nop */
        0xbf, 0x03, 0x00, 0x91,   /* 14: mov sp,  x29 */
        0xfd, 0x7b, 0x40, 0xa9,   /* 18: ldp x29, lr,  [sp] */
        0xff, 0x43, 0x08, 0x91,   /* 1c: add sp,  sp,  #528 */
        0xf3, 0x53, 0xc1, 0xa8,   /* 20: ldp x19, x20, [sp], #16 */
        0xc0, 0x03, 0x5f, 0xd6,   /* 24: ret */
    };

    static const DWORD unwind_info_13_packed =
        (1 << 0)  | /* Flag */
        (sizeof(function_13)/4 << 2) | /* FunctionLength */
        (0 << 13) | /* RegF */
        (2 << 16) | /* RegI */
        (0 << 20) | /* H */
        (3 << 21) | /* CR */
        (34 << 23);  /* FrameSize */

    static const BYTE unwind_info_13[] = { DW(unwind_info_13_packed) };

    static const struct results_arm64 results_13[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x08,  0x10,  0,     ORIG_LR, 0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {-1,-1} }},
        { 0x0c,  0x10,  0,     0x08,    0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x10,  0x10,  0,     0x18,    0x230, TRUE, { {x19, 0x220}, {x20, 0x228}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x14,  0x10,  0,     0x18,    0x230, TRUE, { {x19, 0x220}, {x20, 0x228}, {x29, 0x10}, {lr, 0x18}, {-1,-1} }},
        { 0x18,  0x10,  0,     0x08,    0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {x29, 0x00}, {lr, 0x08}, {-1,-1} }},
        { 0x1c,  0x10,  0,     ORIG_LR, 0x220, TRUE, { {x19, 0x210}, {x20, 0x218}, {-1,-1} }},
        { 0x20,  0x10,  0,     ORIG_LR, 0x010, TRUE, { {x19, 0x00}, {x20, 0x08}, {-1,-1} }},
        { 0x24,  0x10,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
    };

    static const BYTE function_14[] =
    {
        0xe6, 0x9f, 0xba, 0xad,  /* 00: stp q6, q7, [sp, #-0xb0]! */
        0xe8, 0x27, 0x01, 0xad,  /* 04: stp q8, q9, [sp, #0x20] */
        0xea, 0x2f, 0x02, 0xad,  /* 08: stp q10, q11, [sp, #0x40] */
        0xec, 0x37, 0x03, 0xad,  /* 0c: stp q12, q13, [sp, #0x60] */
        0xee, 0x3f, 0x04, 0xad,  /* 10: stp q14, q15, [sp, #0x80] */
        0xfd, 0x7b, 0x0a, 0xa9,  /* 14: stp x29, x30, [sp, #0xa0] */
        0xfd, 0x83, 0x02, 0x91,  /* 18: add x29, sp, #0xa0 */
        0x1f, 0x20, 0x03, 0xd5,  /* 1c: nop */
        0xfd, 0x7b, 0x4a, 0xa9,  /* 20: ldp x29, x30, [sp, #0xa0] */
        0xee, 0x3f, 0x44, 0xad,  /* 24: ldp q14, q15, [sp, #0x80] */
        0xec, 0x37, 0x43, 0xad,  /* 28: ldp q12, q13, [sp, #0x60] */
        0xea, 0x2f, 0x42, 0xad,  /* 2c: ldp q10, q11, [sp, #0x40] */
        0xe8, 0x27, 0x41, 0xad,  /* 30: ldp q8, q9, [sp, #0x20] */
        0xe6, 0x9f, 0xc5, 0xac,  /* 34: ldp q6, q7, [sp], #0xb0 */
        0xc0, 0x03, 0x5f, 0xd6,  /* 38: ret */
    };

    static const DWORD unwind_info_14_header =
        (sizeof(function_14)/4) | /* function length */
        (0 << 20) | /* X */
        (1 << 21) | /* E */
        (2 << 22) | /* epilog */
        (5 << 27);  /* codes */

    static const BYTE unwind_info_14[] =
    {
        DW(unwind_info_14_header),
        UWOP_ADD_FP(0xa0),             /* 18: add x29, sp, #0xa0 */
        UWOP_SAVE_FPLR(0xa0),          /* 14: stp x29, x30, [sp, #0xa0] */
        UWOP_SAVE_ANY_REG(0x4e,0x88),  /* 10: stp q14, q15, [sp, #0x80] */
        UWOP_SAVE_NEXT,                /* 0c: stp q12, q13, [sp, #0x60] */
        UWOP_SAVE_ANY_REG(0x4a,0x84),  /* 08: stp q10, q11, [sp, #0x40] */
        UWOP_SAVE_ANY_REG(0x48,0x82),  /* 04: stp q8, q9, [sp, #0x20] */
        UWOP_SAVE_ANY_REG(0x66,0x8a),  /* 00: stp q6, q7, [sp, #-0xb0]! */
        UWOP_END,
        UWOP_NOP                  /* padding */
    };

    static const struct results_arm64 results_14[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x000, TRUE, { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {d8, 0x20}, {d9, 0x30}, {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {d8, 0x20}, {d9, 0x30}, {d10, 0x40}, {d11, 0x50}, {-1,-1} }},
        { 0x10,  0x00,  0,     ORIG_LR, 0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {d8, 0x20}, {d9, 0x30}, {d10, 0x40}, {d11, 0x50}, {d12, 0x60}, {d13, 0x70}, {-1,-1} }},
        { 0x14,  0x00,  0,     ORIG_LR, 0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {d8, 0x20}, {d9, 0x30}, {d10, 0x40}, {d11, 0x50}, {d12, 0x60}, {d13, 0x70}, {d14, 0x80}, {d15, 0x90}, {-1,-1} }},
        { 0x18,  0x00,  0,     0xa8,    0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {d8, 0x20}, {d9, 0x30}, {d10, 0x40}, {d11, 0x50}, {d12, 0x60}, {d13, 0x70}, {d14, 0x80}, {d15, 0x90}, {lr, 0xa8}, {x29, 0xa0}, {-1,-1} }},
        { 0x1c,  0xa0,  0,     0xa8,    0x0b0, TRUE, { {d6, 0x00}, {d7, 0x10}, {d8, 0x20}, {d9, 0x30}, {d10, 0x40}, {d11, 0x50}, {d12, 0x60}, {d13, 0x70}, {d14, 0x80}, {d15, 0x90}, {lr, 0xa8}, {x29, 0xa0}, {-1,-1} }},
    };

    static const BYTE function_15[] =
    {
        0x1f, 0x20, 0x03, 0xd5,   /* 00: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 10: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 14: ret */
    };

    static const DWORD unwind_info_15_header =
        (sizeof(function_15)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (2 << 27);  /* codes */

    static const BYTE unwind_info_15[] =
    {
        DW(unwind_info_15_header),
        UWOP_END_C,
        UWOP_SET_FP,              /* mov x29, sp */
        UWOP_SAVE_REGP(19, 0x10), /* stp r19, r20, [sp, #0x10] */
        UWOP_SAVE_FPLR_X(0x20),   /* stp r29, lr, [sp,-#0x20]! */
        UWOP_END,
        UWOP_NOP,                 /* padding */
        UWOP_NOP,                 /* padding */
    };

    static const struct results_arm64 results_15[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     0x08,    0x020, TRUE, { {x29, 0x00}, {lr, 0x08}, {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x04,  0x00,  0,     0x08,    0x020, TRUE, { {x29, 0x00}, {lr, 0x08}, {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x08,  0x00,  0,     0x08,    0x020, TRUE, { {x29, 0x00}, {lr, 0x08}, {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x0c,  0x00,  0,     0x08,    0x020, TRUE, { {x29, 0x00}, {lr, 0x08}, {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x10,  0x00,  0,     0x08,    0x020, TRUE, { {x29, 0x00}, {lr, 0x08}, {x19,0x10}, {x20,0x18}, {-1,-1} }},
        { 0x14,  0x00,  0,     0x08,    0x020, TRUE, { {x29, 0x00}, {lr, 0x08}, {x19,0x10}, {x20,0x18}, {-1,-1} }},
    };

    static const BYTE function_16[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0xff, 0x43, 0x00, 0xd1,   /* 08: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 0c: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 10: ret */
    };

    static const DWORD unwind_info_16_header =
        (sizeof(function_16)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (1 << 27);  /* codes */

    static const BYTE unwind_info_16[] =
    {
        DW(unwind_info_16_header),

        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_EC_CONTEXT,
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_END,
    };

    static const struct results_arm64 results_16[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x010, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  0 ,    0x00f8,  0x0a8, FALSE, { {x0, 0x80}, {x1, 0x88}, {x2, 0xb8}, {x3, 0xc0}, {x4, 0xc8}, {x5, 0xd0}, {x6, 0x130}, {x7, 0x140}, {x8, 0x78}, {x9, 0x150}, {x10, 0x160}, {x11, 0x170}, {x12, 0x180}, {x13, 0}, {x14, 0}, {x15, 0x190}, {x16, 0x0158014801380128}, {x17, 0x0198018801780168}, {x18, 0}, {x19, 0xd8}, {x20, 0xe0}, {x21, 0xe8}, {x22, 0x0f0}, {x23, 0}, {x24, 0}, {x25, 0xa8}, {x26, 0xb0}, {x27, 0x90}, {x28, 0}, {x29, 0xa0}, {lr, 0x120}, {d0, 0x1a0}, {d1, 0x1b0}, {d2, 0x1c0}, {d3, 0x1d0}, {d4, 0x1e0}, {d5, 0x1f0}, {d6, 0x200}, {d7, 0x210}, {d8, 0x220}, {d9, 0x230}, {d10, 0x240}, {d11, 0x250}, {d12, 0x260}, {d13, 0x270}, {d14, 0x280}, {d15, 0x290}, {-1,-1} }},
        { 0x08,  0x00,  0 ,    0x0108,  0x0b8, FALSE, { {x0, 0x90}, {x1, 0x98}, {x2, 0xc8}, {x3, 0xd0}, {x4, 0xd8}, {x5, 0xe0}, {x6, 0x140}, {x7, 0x150}, {x8, 0x88}, {x9, 0x160}, {x10, 0x170}, {x11, 0x180}, {x12, 0x190}, {x13, 0}, {x14, 0}, {x15, 0x1a0}, {x16, 0x0168015801480138}, {x17, 0x01a8019801880178}, {x18, 0}, {x19, 0xe8}, {x20, 0xf0}, {x21, 0xf8}, {x22, 0x100}, {x23, 0}, {x24, 0}, {x25, 0xb8}, {x26, 0xc0}, {x27, 0xa0}, {x28, 0}, {x29, 0xb0}, {lr, 0x130}, {d0, 0x1b0}, {d1, 0x1c0}, {d2, 0x1d0}, {d3, 0x1e0}, {d4, 0x1f0}, {d5, 0x200}, {d6, 0x210}, {d7, 0x220}, {d8, 0x230}, {d9, 0x240}, {d10, 0x250}, {d11, 0x260}, {d12, 0x270}, {d13, 0x280}, {d14, 0x290}, {d15, 0x2a0}, {-1,-1} }},
        { 0x0c,  0x00,  0 ,    0x0108,  0x0b8, FALSE, { {x0, 0x90}, {x1, 0x98}, {x2, 0xc8}, {x3, 0xd0}, {x4, 0xd8}, {x5, 0xe0}, {x6, 0x140}, {x7, 0x150}, {x8, 0x88}, {x9, 0x160}, {x10, 0x170}, {x11, 0x180}, {x12, 0x190}, {x13, 0}, {x14, 0}, {x15, 0x1a0}, {x16, 0x0168015801480138}, {x17, 0x01a8019801880178}, {x18, 0}, {x19, 0xe8}, {x20, 0xf0}, {x21, 0xf8}, {x22, 0x100}, {x23, 0}, {x24, 0}, {x25, 0xb8}, {x26, 0xc0}, {x27, 0xa0}, {x28, 0}, {x29, 0xb0}, {lr, 0x130}, {d0, 0x1b0}, {d1, 0x1c0}, {d2, 0x1d0}, {d3, 0x1e0}, {d4, 0x1f0}, {d5, 0x200}, {d6, 0x210}, {d7, 0x220}, {d8, 0x230}, {d9, 0x240}, {d10, 0x250}, {d11, 0x260}, {d12, 0x270}, {d13, 0x280}, {d14, 0x290}, {d15, 0x2a0}, {-1,-1} }},
        { 0x10,  0x00,  0 ,    0x0108,  0x0b8, FALSE, { {x0, 0x90}, {x1, 0x98}, {x2, 0xc8}, {x3, 0xd0}, {x4, 0xd8}, {x5, 0xe0}, {x6, 0x140}, {x7, 0x150}, {x8, 0x88}, {x9, 0x160}, {x10, 0x170}, {x11, 0x180}, {x12, 0x190}, {x13, 0}, {x14, 0}, {x15, 0x1a0}, {x16, 0x0168015801480138}, {x17, 0x01a8019801880178}, {x18, 0}, {x19, 0xe8}, {x20, 0xf0}, {x21, 0xf8}, {x22, 0x100}, {x23, 0}, {x24, 0}, {x25, 0xb8}, {x26, 0xc0}, {x27, 0xa0}, {x28, 0}, {x29, 0xb0}, {lr, 0x130}, {d0, 0x1b0}, {d1, 0x1c0}, {d2, 0x1d0}, {d3, 0x1e0}, {d4, 0x1f0}, {d5, 0x200}, {d6, 0x210}, {d7, 0x220}, {d8, 0x230}, {d9, 0x240}, {d10, 0x250}, {d11, 0x260}, {d12, 0x270}, {d13, 0x280}, {d14, 0x290}, {d15, 0x2a0}, {-1,-1} }},
    };

    static const BYTE function_17[] =
    {
        0xff, 0x43, 0x00, 0xd1,   /* 00: sub sp, sp, #16 */
        0xff, 0x43, 0x00, 0xd1,   /* 04: sub sp, sp, #16 */
        0x1f, 0x20, 0x03, 0xd5,   /* 08: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 0c: ret */
    };

    static const DWORD unwind_info_17_header =
        (sizeof(function_17)/4) | /* function length */
        (0 << 20) | /* X */
        (0 << 21) | /* E */
        (0 << 22) | /* epilog */
        (1 << 27);  /* codes */

    static const BYTE unwind_info_17[] =
    {
        DW(unwind_info_17_header),

        UWOP_CLEAR_UNWOUND_TO_CALL,
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_ALLOC_SMALL(16),   /* sub sp,  sp,  #16 */
        UWOP_END,
    };

    static const struct results_arm64 results_17[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  0,     ORIG_LR, 0x010, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  0,     ORIG_LR, 0x020, TRUE,  { {-1,-1} }},
        { 0x08,  0x00,  0,     ORIG_LR, 0x020, TRUE,  { {-1,-1} }},
        { 0x0c,  0x00,  0,     ORIG_LR, 0x020, TRUE,  { {-1,-1} }},
    };

    static const BYTE function_18[] =
    {
        0x1f, 0x20, 0x03, 0xd5,   /* 00: nop */
        0x1f, 0x20, 0x03, 0xd5,   /* 04: nop */
        0xc0, 0x03, 0x5f, 0xd6,   /* 08: ret */
    };

    static const struct results_arm64 results_18[] =
    {
      /* offset  fp    handler  pc      frame offset  registers */
        { 0x00,  0x00,  -1,     ORIG_LR, 0x000, TRUE,  { {-1,-1} }},
        { 0x04,  0x00,  -1,     ORIG_LR, 0x000, TRUE,  { {-1,-1} }},
        { 0x08,  0x00,  -1,     ORIG_LR, 0x000, TRUE,  { {-1,-1} }},
        { 0x0c,  0x00,  -2,     0, 0xdeadbeef, FALSE, { {-1,-1} }},
    };

    static const struct unwind_test_arm64 tests[] =
    {
#define TEST(func, unwind, size, results, unwound_clear, last_ptr, stack_value_index, stack_value) \
        { func, sizeof(func), unwind, size, results, ARRAY_SIZE(results), unwound_clear, last_ptr, stack_value_index, stack_value }
        TEST(function_0, unwind_info_0, sizeof(unwind_info_0), results_0, 0, 0, -1, 0),
        TEST(function_1, unwind_info_1, 0, results_1, 0, 0, -1, 0),
        TEST(function_2, unwind_info_2, sizeof(unwind_info_2), results_2, 1, 0, -1, 0),
        TEST(function_3, unwind_info_3, sizeof(unwind_info_3), results_3, 2, x28, 0, CONTEXT_ARM64_UNWOUND_TO_CALL),
        TEST(function_4, unwind_info_4, sizeof(unwind_info_4), results_4, 0, 0, -1, 0),
        TEST(function_5, unwind_info_5, sizeof(unwind_info_5), results_5, 0, 0, -1, 0),
        TEST(function_6, unwind_info_6, 0, results_6, 0, 0, -1, 0),
        TEST(function_7, unwind_info_7, 0, results_7, 0, 0, -1, 0),
        TEST(function_8, unwind_info_8, 0, results_8, 0, 0, -1, 0),
        TEST(function_9, unwind_info_9, 0, results_9, 0, 0, -1, 0),
        TEST(function_10, unwind_info_10, 0, results_10, 0, 0, -1, 0),
        TEST(function_11, unwind_info_11, 0, results_11, 0, 0, -1, 0),
        TEST(function_12, unwind_info_12, 0, results_12, 0, 0, -1, 0),
        TEST(function_13, unwind_info_13, 0, results_13, 0, 0, -1, 0),
        TEST(function_14, unwind_info_14, sizeof(unwind_info_14), results_14, 0, 0, -1, 0),
        TEST(function_15, unwind_info_15, sizeof(unwind_info_15), results_15, 0, 0, -1, 0),
        TEST(function_16, unwind_info_16, sizeof(unwind_info_16), results_16, 2, x18, 6, CONTEXT_ARM64_UNWOUND_TO_CALL),
        TEST(function_17, unwind_info_17, sizeof(unwind_info_17), results_17, 2, 0, -1, 0),
        TEST(function_18, NULL, 0, results_18, 0, 0, -1, 0),
#undef TEST
    };
    unsigned int i;

#ifdef __x86_64__
    void *code_mem = NULL;
    SIZE_T code_size = 0x10000;
    MEM_EXTENDED_PARAMETER param = { 0 };

    param.Type = MemExtendedParameterAttributeFlags;
    param.ULong64 = MEM_EXTENDED_PARAMETER_EC_CODE;
    if (!pNtAllocateVirtualMemoryEx ||
        pNtAllocateVirtualMemoryEx( GetCurrentProcess(), &code_mem, &code_size, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_EXECUTE_READWRITE, &param, 1 ))
        return;
    trace( "running arm64ec tests\n" );
#endif

    for (i = 0; i < ARRAY_SIZE(tests); i++)
        call_virtual_unwind_arm64( code_mem, i, &tests[i] );
}

#endif // __REACTOS__

#undef UWOP_ALLOC_SMALL
#undef UWOP_ALLOC_LARGE

#endif  /* __aarch64__ || __x86_64__ */

#ifdef __x86_64__

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

struct results_x86
{
    int rip_offset;   /* rip offset from code start */
    int rbp_offset;   /* rbp offset from stack pointer */
    int handler;      /* expect handler to be set? */
    int rip;          /* expected final rip value */
    int frame;        /* expected frame return value */
    int regs[8][2];   /* expected values for registers */
};

struct unwind_test_x86
{
    const BYTE *function;
    size_t function_size;
    const BYTE *unwind_info;
    const struct results_x86 *results;
    unsigned int nb_results;
    const struct results_x86 *broken_results;
};

enum regs
{
    rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
    r8,  r9,  r10, r11, r12, r13, r14, r15
};

static const char * const reg_names_x86[16] =
{
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};

#define UWOP(code,info) (UWOP_##code | ((info) << 4))

static void call_virtual_unwind_x86( int testnum, const struct unwind_test_x86 *test )
{
    static const int code_offset = 1024;
    static const int unwind_offset = 2048;
    void *data;
    NTSTATUS status;
    CONTEXT context;
    PEXCEPTION_ROUTINE handler;
    RUNTIME_FUNCTION runtime_func;
    KNONVOLATILE_CONTEXT_POINTERS ctx_ptr;
    UINT i, j, k, broken_k;
    ULONG64 fake_stack[256];
    ULONG64 frame, orig_rip, orig_rbp, unset_reg;
    void *expected_handler, *broken_handler;

    memcpy( (char *)code_mem + code_offset, test->function, test->function_size );
    if (test->unwind_info)
    {
        UINT unwind_size = 4 + 2 * test->unwind_info[2] + 8;
        memcpy( (char *)code_mem + unwind_offset, test->unwind_info, unwind_size );
        runtime_func.BeginAddress = code_offset;
        runtime_func.EndAddress = code_offset + test->function_size;
        runtime_func.UnwindData = unwind_offset;
    }

    trace( "code: %p stack: %p\n", code_mem, fake_stack );

    for (i = 0; i < test->nb_results; i++)
    {
        memset( &ctx_ptr, 0, sizeof(ctx_ptr) );
        memset( &context, 0x55, sizeof(context) );
        memset( &unset_reg, 0x55, sizeof(unset_reg) );
        for (j = 0; j < 256; j++) fake_stack[j] = j * 8;

        context.Rsp = (ULONG_PTR)fake_stack;
        context.Rbp = (ULONG_PTR)fake_stack + test->results[i].rbp_offset;
        orig_rbp = context.Rbp;
        orig_rip = (ULONG64)code_mem + code_offset + test->results[i].rip_offset;

        trace( "%u/%u: rip=%p (%02x) rbp=%p rsp=%p\n", testnum, i,
               (void *)orig_rip, *(BYTE *)orig_rip, (void *)orig_rbp, (void *)context.Rsp );

        if (!test->unwind_info) fake_stack[0] = 0x1234;
        expected_handler = test->results[i].handler ? (char *)code_mem + 0x200 : NULL;
        broken_handler = test->broken_results && test->broken_results[i].handler ? (char *)code_mem + 0x200 : NULL;

        if (pRtlVirtualUnwind2)
        {
            CONTEXT new_context = context;

            handler = (void *)0xdeadbeef;
            data = (void *)0xdeadbeef;
            status = pRtlVirtualUnwind2( UNW_FLAG_EHANDLER, (ULONG_PTR)code_mem, orig_rip,
                                         test->unwind_info ? &runtime_func : NULL, &new_context,
                                         NULL, &data, &frame, &ctx_ptr, NULL, NULL, &handler, 0 );
            ok( !status, "RtlVirtualUnwind2 failed %lx\n", status );

            ok( handler == expected_handler || broken( test->broken_results && handler == broken_handler ),
                "%u/%u: wrong handler %p/%p\n", testnum, i, handler, expected_handler );
            if (handler)
                ok( *(DWORD *)data == 0x08070605, "%u/%u: wrong handler data %lx\n", testnum, i, *(DWORD *)data );
            else
                ok( data == (test->unwind_info ? (void *)0xdeadbeef : NULL),
                    "%u/%u: handler data set to %p\n", testnum, i, data );
        }

        data = (void *)0xdeadbeef;
        handler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, (ULONG64)code_mem, orig_rip,
                                    test->unwind_info ? &runtime_func : NULL,
                                    &context, &data, &frame, &ctx_ptr );

        expected_handler = test->results[i].handler ? (char *)code_mem + 0x200 : NULL;
        broken_handler = test->broken_results && test->broken_results[i].handler ? (char *)code_mem + 0x200 : NULL;

        ok( handler == expected_handler || broken( test->broken_results && handler == broken_handler ),
                "%u/%u: wrong handler %p/%p\n", testnum, i, handler, expected_handler );
        if (handler)
            ok( *(DWORD *)data == 0x08070605, "%u/%u: wrong handler data %lx\n", testnum, i, *(DWORD *)data );
        else
            ok( data == (test->unwind_info ? (void *)0xdeadbeef : NULL),
                "%u/%u: handler data set to %p\n", testnum, i, data );

        ok( context.Rip == test->results[i].rip
                || broken( test->broken_results && context.Rip == test->broken_results[i].rip ),
                "%u/%u: wrong rip %p/%x\n", testnum, i, (void *)context.Rip, test->results[i].rip );
        ok( frame == (ULONG64)fake_stack + test->results[i].frame
                || broken( test->broken_results && frame == (ULONG64)fake_stack + test->broken_results[i].frame ),
                "%u/%u: wrong frame %p/%p\n",
                testnum, i, (void *)frame, (char *)fake_stack + test->results[i].frame );

        for (j = 0; j < 16; j++)
        {
            static const UINT nb_regs = ARRAY_SIZE(test->results[i].regs);

            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                {
                    k = nb_regs;
                    break;
                }
                if (test->results[i].regs[k][0] == j) break;
            }

            if (test->broken_results)
            {
                for (broken_k = 0; broken_k < nb_regs; broken_k++)
                {
                    if (test->broken_results[i].regs[broken_k][0] == -1)
                    {
                        broken_k = nb_regs;
                        break;
                    }
                    if (test->broken_results[i].regs[broken_k][0] == j)
                        break;
                }
            }
            else
            {
                broken_k = k;
            }

            if (j == rsp)  /* rsp is special */
            {
                ULONG64 expected_rsp, broken_rsp;

                ok( !ctx_ptr.IntegerContext[j],
                    "%u/%u: rsp should not be set in ctx_ptr\n", testnum, i );
                expected_rsp = test->results[i].regs[k][1] < 0
                        ? -test->results[i].regs[k][1] : (ULONG64)fake_stack + test->results[i].regs[k][1];
                if (test->broken_results)
                    broken_rsp = test->broken_results[i].regs[k][1] < 0
                            ? -test->broken_results[i].regs[k][1]
                            : (ULONG64)fake_stack + test->broken_results[i].regs[k][1];
                else
                    broken_rsp = expected_rsp;

                ok( context.Rsp == expected_rsp || broken( context.Rsp == broken_rsp ),
                    "%u/%u: register rsp wrong %p/%p\n",
                    testnum, i, (void *)context.Rsp, (void *)expected_rsp );
                continue;
            }

            if (ctx_ptr.IntegerContext[j])
            {
                ok( k < nb_regs || broken( broken_k < nb_regs ), "%u/%u: register %s should not be set to %Ix\n",
                    testnum, i, reg_names_x86[j], *(&context.Rax + j) );
                ok( k == nb_regs || *(&context.Rax + j) == test->results[i].regs[k][1]
                        || broken( broken_k == nb_regs || *(&context.Rax + j)
                        == test->broken_results[i].regs[broken_k][1] ),
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names_x86[j], (void *)*(&context.Rax + j), test->results[i].regs[k][1] );
            }
            else
            {
                ok( k == nb_regs || broken( broken_k == nb_regs ), "%u/%u: register %s should be set\n",
                        testnum, i, reg_names_x86[j] );
                if (j == rbp)
                    ok( context.Rbp == orig_rbp, "%u/%u: register rbp wrong %p/unset\n",
                        testnum, i, (void *)context.Rbp );
                else
                    ok( *(&context.Rax + j) == unset_reg,
                        "%u/%u: register %s wrong %p/unset\n",
                        testnum, i, reg_names_x86[j], (void *)*(&context.Rax + j));
            }
        }
    }
}

static void test_virtual_unwind_x86(void)
{
    static const BYTE function_0[] =
    {
        0xff, 0xf5,                                  /* 00: push %rbp */
        0x48, 0x81, 0xec, 0x10, 0x01, 0x00, 0x00,    /* 02: sub $0x110,%rsp */
        0x48, 0x8d, 0x6c, 0x24, 0x30,                /* 09: lea 0x30(%rsp),%rbp */
        0x48, 0x89, 0x9d, 0xf0, 0x00, 0x00, 0x00,    /* 0e: mov %rbx,0xf0(%rbp) */
        0x48, 0x89, 0xb5, 0xf8, 0x00, 0x00, 0x00,    /* 15: mov %rsi,0xf8(%rbp) */
        0x90,                                        /* 1c: nop */
        0x48, 0x8b, 0x9d, 0xf0, 0x00, 0x00, 0x00,    /* 1d: mov 0xf0(%rbp),%rbx */
        0x48, 0x8b, 0xb5, 0xf8, 0x00, 0x00, 0x00,    /* 24: mov 0xf8(%rbp),%rsi */
        0x48, 0x8d, 0xa5, 0xe0, 0x00, 0x00, 0x00,    /* 2b: lea 0xe0(%rbp),%rsp */
        0x5d,                                        /* 32: pop %rbp */
        0xc3                                         /* 33: ret */
    };

    static const BYTE unwind_info_0[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x1c,                          /* prolog size */
        8,                             /* opcode count */
        (0x03 << 4) | rbp,             /* frame reg rbp offset 0x30 */

        0x1c, UWOP(SAVE_NONVOL, rsi), 0x25, 0, /* 1c: mov %rsi,0x128(%rsp) */
        0x15, UWOP(SAVE_NONVOL, rbx), 0x24, 0, /* 15: mov %rbx,0x120(%rsp) */
        0x0e, UWOP(SET_FPREG, rbp),            /* 0e: lea 0x30(%rsp),rbp */
        0x09, UWOP(ALLOC_LARGE, 0), 0x22, 0,   /* 09: sub $0x110,%rsp */
        0x02, UWOP(PUSH_NONVOL, rbp),          /* 02: push %rbp */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results_x86 results_0[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x40,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x02,  0x40,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbp,0x000}, {-1,-1} }},
        { 0x09,  0x40,  FALSE, 0x118, 0x000, { {rsp,0x120}, {rbp,0x110}, {-1,-1} }},
        { 0x0e,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1} }},
        { 0x15,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {-1,-1} }},
        { 0x1c,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x1d,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x24,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x2b,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1}}},
        { 0x32,  0x40,  FALSE, 0x008, 0x010, { {rsp,0x010}, {rbp,0x000}, {-1,-1}}},
        { 0x33,  0x40,  FALSE, 0x000, 0x010, { {rsp,0x008}, {-1,-1}}},
    };

    static const struct results_x86 broken_results_0[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x40,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x02,  0x40,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbp,0x000}, {-1,-1} }},
        { 0x09,  0x40,  FALSE, 0x118, 0x000, { {rsp,0x120}, {rbp,0x110}, {-1,-1} }},
        { 0x0e,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1} }},
        { 0x15,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {-1,-1} }},
        { 0x1c,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x1d,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x24,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        /* On Win11 output frame in epilogue corresponds to context->Rsp - 0x8 when fpreg is set. */
        { 0x2b,  0x40,  FALSE, 0x128, 0x128, { {rsp,0x130}, {rbp,0x120}, {-1,-1}}},
        { 0x32,  0x40,  FALSE, 0x008, 0x008, { {rsp,0x010}, {rbp,0x000}, {-1,-1}}},
        { 0x33,  0x40,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1}}},
    };

    static const BYTE function_1[] =
    {
        0x53,                     /* 00: push %rbx */
        0x55,                     /* 01: push %rbp */
        0x56,                     /* 02: push %rsi */
        0x57,                     /* 03: push %rdi */
        0x41, 0x54,               /* 04: push %r12 */
        0x48, 0x83, 0xec, 0x30,   /* 06: sub $0x30,%rsp */
        0x90, 0x90,               /* 0a: nop; nop */
        0x48, 0x83, 0xc4, 0x30,   /* 0c: add $0x30,%rsp */
        0x41, 0x5c,               /* 10: pop %r12 */
        0x5f,                     /* 12: pop %rdi */
        0x5e,                     /* 13: pop %rsi */
        0x5d,                     /* 14: pop %rbp */
        0x5b,                     /* 15: pop %rbx */
        0xc3                      /* 16: ret */
     };

    static const BYTE unwind_info_1[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0a,                          /* prolog size */
        6,                             /* opcode count */
        0,                             /* frame reg */

        0x0a, UWOP(ALLOC_SMALL, 5),   /* 0a: sub $0x30,%rsp */
        0x06, UWOP(PUSH_NONVOL, r12), /* 06: push %r12 */
        0x04, UWOP(PUSH_NONVOL, rdi), /* 04: push %rdi */
        0x03, UWOP(PUSH_NONVOL, rsi), /* 03: push %rsi */
        0x02, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x01, UWOP(PUSH_NONVOL, rbx), /* 01: push %rbx */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results_x86 results_1[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x50,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x01,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbx,0x000}, {-1,-1} }},
        { 0x02,  0x50,  FALSE, 0x010, 0x000, { {rsp,0x018}, {rbx,0x008}, {rbp,0x000}, {-1,-1} }},
        { 0x03,  0x50,  FALSE, 0x018, 0x000, { {rsp,0x020}, {rbx,0x010}, {rbp,0x008}, {rsi,0x000}, {-1,-1} }},
        { 0x04,  0x50,  FALSE, 0x020, 0x000, { {rsp,0x028}, {rbx,0x018}, {rbp,0x010}, {rsi,0x008}, {rdi,0x000}, {-1,-1} }},
        { 0x06,  0x50,  FALSE, 0x028, 0x000, { {rsp,0x030}, {rbx,0x020}, {rbp,0x018}, {rsi,0x010}, {rdi,0x008}, {r12,0x000}, {-1,-1} }},
        { 0x0a,  0x50,  TRUE,  0x058, 0x000, { {rsp,0x060}, {rbx,0x050}, {rbp,0x048}, {rsi,0x040}, {rdi,0x038}, {r12,0x030}, {-1,-1} }},
        { 0x0c,  0x50,  FALSE, 0x058, 0x000, { {rsp,0x060}, {rbx,0x050}, {rbp,0x048}, {rsi,0x040}, {rdi,0x038}, {r12,0x030}, {-1,-1} }},
        { 0x10,  0x50,  FALSE, 0x028, 0x000, { {rsp,0x030}, {rbx,0x020}, {rbp,0x018}, {rsi,0x010}, {rdi,0x008}, {r12,0x000}, {-1,-1} }},
        { 0x12,  0x50,  FALSE, 0x020, 0x000, { {rsp,0x028}, {rbx,0x018}, {rbp,0x010}, {rsi,0x008}, {rdi,0x000}, {-1,-1} }},
        { 0x13,  0x50,  FALSE, 0x018, 0x000, { {rsp,0x020}, {rbx,0x010}, {rbp,0x008}, {rsi,0x000}, {-1,-1} }},
        { 0x14,  0x50,  FALSE, 0x010, 0x000, { {rsp,0x018}, {rbx,0x008}, {rbp,0x000}, {-1,-1} }},
        { 0x15,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbx,0x000}, {-1,-1} }},
        { 0x16,  0x50,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
    };

    static const BYTE function_2[] =
    {
        0x55,                     /* 00: push %rbp */
        0x90, 0x90,               /* 01: nop; nop */
        0x5d,                     /* 03: pop %rbp */
        0xc3                      /* 04: ret */
     };

    static const BYTE unwind_info_2[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0,                           /* prolog size */
        2,                             /* opcode count */
        0,                             /* frame reg */

        0x01, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x00, UWOP(PUSH_MACHFRAME, 0), /* 00 */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results_x86 results_2[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  TRUE, 0x008, 0x000, { {rsp,-0x020}, {rbp,0x000}, {-1,-1} }},
    };

    static const BYTE unwind_info_3[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0,                           /* prolog size */
        2,                             /* opcode count */
        0,                             /* frame reg */

        0x01, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x00, UWOP(PUSH_MACHFRAME, 1), /* 00 */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results_x86 results_3[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  TRUE, 0x010, 0x000, { {rsp,-0x028}, {rbp,0x000}, {-1,-1} }},
    };

    static const BYTE function_4[] =
    {
        0x55,                     /* 00: push %rbp */
        0x5d,                     /* 01: pop %rbp */
        0xc3                      /* 02: ret */
     };

    static const BYTE unwind_info_4[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0,                           /* prolog size */
        0,                             /* opcode count */
        0,                             /* frame reg */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results_x86 results_4[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  TRUE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
    };

    static const struct results_x86 broken_results_4[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbp,0x000}, {-1,-1} }},
    };

#if 0
    static const BYTE function_5[] =
    {
        0x90,                     /* 00: nop */
        0x90,                     /* 01: nop */
        0xc3                      /* 02: ret */
     };

    static const struct results_x86 results_5[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x01,  0x00,  FALSE, 0x1234, 0x000, { {rsp,0x08}, {-1,-1} }},
        { 0x02,  0x00,  FALSE, 0x1234, 0x000, { {rsp,0x08}, {-1,-1} }},
    };
#endif

    static const struct unwind_test_x86 tests[] =
    {
        { function_0, sizeof(function_0), unwind_info_0, results_0, ARRAY_SIZE(results_0), broken_results_0 },
        { function_1, sizeof(function_1), unwind_info_1, results_1, ARRAY_SIZE(results_1) },
        { function_2, sizeof(function_2), unwind_info_2, results_2, ARRAY_SIZE(results_2) },
        { function_2, sizeof(function_2), unwind_info_3, results_3, ARRAY_SIZE(results_3) },

        /* Broken before Win10 1809. */
        { function_4, sizeof(function_4), unwind_info_4, results_4, ARRAY_SIZE(results_4), broken_results_4 },
#if 0  /* crashes before Win10 21H2 */
        { function_5, sizeof(function_5), NULL,          results_5, ARRAY_SIZE(results_5) },
#endif
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
        call_virtual_unwind_x86( i, &tests[i] );
}

#endif  /* __x86_64__ */

#ifdef __x86_64__
#define SET_RUNTIME_FUNC_LEN(func,len) do { (func)->EndAddress = (func)->BeginAddress + (len); } while(0)
#elif defined(__arm__)
#define SET_RUNTIME_FUNC_LEN(func,len) do { (func)->FunctionLength = len / 2; (func)->Flag = 1; } while(0)
#else
#define SET_RUNTIME_FUNC_LEN(func,len) do { (func)->FunctionLength = len / 4; (func)->Flag = 1; } while(0)
#endif

static RUNTIME_FUNCTION * CALLBACK dynamic_unwind_callback( DWORD_PTR pc, PVOID context )
{
    static const int code_offset = 1024;
    static RUNTIME_FUNCTION runtime_func;
    (*(DWORD *)context)++;

    runtime_func.BeginAddress = code_offset + 16;
    runtime_func.UnwindData   = 0;
    SET_RUNTIME_FUNC_LEN( &runtime_func, 16 );
    return &runtime_func;
}

static void test_dynamic_unwind(void)
{
    static const int code_offset = 1024;
    char buf[2 * sizeof(RUNTIME_FUNCTION) + 4];
    MEM_EXTENDED_PARAMETER param = { 0 };
    RUNTIME_FUNCTION *runtime_func, *func;
    ULONG_PTR table, base, ec_code;
    void *growable_table, *ptr;
    NTSTATUS status;
    SIZE_T size = 0x1000;
    DWORD count;
    ULONG len, len2;

    if (!pRtlInstallFunctionTableCallback || !pRtlLookupFunctionEntry)
    {
        win_skip( "Dynamic unwind functions not found\n" );
        return;
    }

    /* Test RtlAddFunctionTable with aligned RUNTIME_FUNCTION pointer */
    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->UnwindData   = 0;
    SET_RUNTIME_FUNC_LEN( runtime_func, 16 );
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (aligned)\n", runtime_func );

    /* Lookup function outside of any function table */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry returned unexpected function, expected: NULL, got: %p\n", func );
    ok( !base || broken(base == 0xdeadbeef),
        "RtlLookupFunctionEntry modified base address, expected: 0, got: %Ix\n", base );

    /* Test with pointer inside of our function */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == runtime_func,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %Ix, got: %Ix\n", (ULONG_PTR)code_mem, base );

    /* Test RtlDeleteFunctionTable */
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (aligned)\n", runtime_func );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable returned success for nonexistent table runtime_func = %p\n", runtime_func );

    /* Unaligned RUNTIME_FUNCTION pointer */
    runtime_func = (RUNTIME_FUNCTION *)((ULONG_PTR)buf | 0x3);
    runtime_func->BeginAddress = code_offset;
    runtime_func->UnwindData   = 0;
    SET_RUNTIME_FUNC_LEN( runtime_func, 16 );
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (unaligned)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (unaligned)\n", runtime_func );

    /* Attempt to insert the same entry twice */
    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->UnwindData   = 0;
    SET_RUNTIME_FUNC_LEN( runtime_func, 16 );
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (first attempt)\n", runtime_func );
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (second attempt)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (first attempt)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (second attempt)\n", runtime_func );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable returned success for nonexistent table runtime_func = %p\n", runtime_func );

    /* Empty table */
    ok( pRtlAddFunctionTable( runtime_func, 0, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for empty table\n" );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for empty table\n" );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable succeeded twice for empty table\n" );

    /* Test RtlInstallFunctionTableCallback with both low bits unset */
    table = (ULONG_PTR)code_mem;
    ok( !pRtlInstallFunctionTableCallback( table, (ULONG_PTR)code_mem, code_offset + 32, &dynamic_unwind_callback, (PVOID*)&count, NULL ),
        "RtlInstallFunctionTableCallback returned success for table = %Ix\n", table );

    /* Test RtlInstallFunctionTableCallback with both low bits set */
    table = (ULONG_PTR)code_mem | 0x3;
    ok( pRtlInstallFunctionTableCallback( table, (ULONG_PTR)code_mem, code_offset + 32, &dynamic_unwind_callback, (PVOID*)&count, NULL ),
        "RtlInstallFunctionTableCallback failed for table = %Ix\n", table );

    /* Lookup function outside of any function table */
    count = 0;
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 32, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry returned unexpected function, expected: NULL, got: %p\n", func );
    ok( !base || broken(base == 0xdeadbeef),
        "RtlLookupFunctionEntry modified base address, expected: 0, got: %Ix\n", base );
    ok( !count,
        "RtlLookupFunctionEntry issued %ld unexpected calls to dynamic_unwind_callback\n", count );

    /* Test with pointer inside of our function */
    count = 0;
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 24, &base, NULL );
    ok( count == 1 || broken(!count), /* win10 arm */
        "RtlLookupFunctionEntry issued %ld calls to dynamic_unwind_callback, expected: 1\n", count );
    if (count)
    {
        ok( func != NULL && func->BeginAddress == code_offset + 16,
            "RtlLookupFunctionEntry didn't return expected function, got: %p\n", func );
        ok( base == (ULONG_PTR)code_mem,
            "RtlLookupFunctionEntry returned invalid base: %Ix / %Ix\n", (ULONG_PTR)code_mem, base );
    }

    /* Clean up again */
    ok( pRtlDeleteFunctionTable( (PRUNTIME_FUNCTION)table ),
        "RtlDeleteFunctionTable failed for table = %p\n", (PVOID)table );
    ok( !pRtlDeleteFunctionTable( (PRUNTIME_FUNCTION)table ),
        "RtlDeleteFunctionTable returned success for nonexistent table = %p\n", (PVOID)table );

    if (!pRtlAddGrowableFunctionTable)
    {
        win_skip("Growable function tables are not supported.\n");
        return;
    }

    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->UnwindData   = 0;
    SET_RUNTIME_FUNC_LEN( runtime_func, 16 );
    runtime_func++;
    runtime_func->BeginAddress = code_offset + 16;
    runtime_func->UnwindData   = 0;
    SET_RUNTIME_FUNC_LEN( runtime_func, 16 );
    runtime_func = (RUNTIME_FUNCTION *)buf;

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 1, 1, (ULONG_PTR)code_mem, (ULONG_PTR)code_mem + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#lx.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");
    pRtlDeleteGrowableFunctionTable( growable_table );

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 2, 2, (ULONG_PTR)code_mem, (ULONG_PTR)code_mem + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#lx.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");
    pRtlDeleteGrowableFunctionTable( growable_table );

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 1, 2, (ULONG_PTR)code_mem, (ULONG_PTR)code_mem + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#lx.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");
    pRtlDeleteGrowableFunctionTable( growable_table );

    growable_table = NULL;
    status = pRtlAddGrowableFunctionTable( &growable_table, runtime_func, 0, 2, (ULONG_PTR)code_mem,
            (ULONG_PTR)code_mem + code_offset + 64 );
    ok(!status, "RtlAddGrowableFunctionTable failed for runtime_func = %p (aligned), %#lx.\n", runtime_func, status );
    ok(growable_table != 0, "Unexpected table value.\n");

    /* Current count is 0. */
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );

    pRtlGrowFunctionTable( growable_table, 1 );

    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == runtime_func,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %Ix, got: %Ix\n", (ULONG_PTR)code_mem, base );

    /* Second function is inaccessible yet. */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );

    pRtlGrowFunctionTable( growable_table, 2 );

    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == runtime_func + 1,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %Ix, got: %Ix\n", (ULONG_PTR)code_mem, base );

    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 32, &base, NULL );
    ok( func == NULL, "RtlLookupFunctionEntry got %p\n", func );
    ok( base == 0xdeadbeef, "RtlLookupFunctionTable wrong base, got: %Ix\n", base );

    base = 0xdeadbeef;
    func = pRtlLookupFunctionTable( (ULONG_PTR)code_mem + code_offset + 8, &base, &len );
    ok( func == NULL, "RtlLookupFunctionTable wrong table, got: %p\n", func );
    ok( base == 0xdeadbeef, "RtlLookupFunctionTable wrong base, got: %Ix\n", base );

    base = 0xdeadbeef;
    len = 0xdeadbeef;
    func = pRtlLookupFunctionTable( (ULONG_PTR)pRtlLookupFunctionEntry, &base, &len );
    ok( base == (ULONG_PTR)GetModuleHandleA("ntdll.dll"),
        "RtlLookupFunctionTable wrong base, got: %Ix / %p\n", base, GetModuleHandleA("ntdll.dll") );
    ptr = RtlImageDirectoryEntryToData( (void *)base, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &len2 );
    ok( func == ptr, "RtlLookupFunctionTable wrong table, got: %p / %p\n", func, ptr );
    ok( len == len2 || !ptr, "RtlLookupFunctionTable wrong len, got: %lu / %lu\n", len, len2 );

    pRtlDeleteGrowableFunctionTable( growable_table );

#ifndef __REACTOS__
    param.Type = MemExtendedParameterAttributeFlags;
    param.ULong64 = MEM_EXTENDED_PARAMETER_EC_CODE;
    ec_code = 0;
    if (pNtAllocateVirtualMemoryEx &&
        !pNtAllocateVirtualMemoryEx( GetCurrentProcess(), (void **)&ec_code, &size,
                                     MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE, &param, 1 ))
    {
        static const BYTE fast_forward[] = { 0x48, 0x8b, 0xc4, 0x48, 0x89, 0x58, 0x20, 0x55, 0x5d, 0xe9 };
        IMAGE_ARM64EC_METADATA *metadata;
        ARM64_RUNTIME_FUNCTION *arm64func = (ARM64_RUNTIME_FUNCTION *)buf;

        trace( "running arm64ec tests\n" );

        if (!memcmp( pRtlLookupFunctionEntry, fast_forward, sizeof(fast_forward) ))
        {
            ptr = (char *)pRtlLookupFunctionEntry + sizeof(fast_forward);
            ptr = (char *)ptr + 4 + *(int *)ptr;
            base = 0xdeadbeef;
            func = pRtlLookupFunctionTable( (ULONG_PTR)ptr, &base, &len );
            ok( base == (ULONG_PTR)GetModuleHandleA("ntdll.dll"),
                "RtlLookupFunctionTable wrong base, got: %Ix / %p\n", base, GetModuleHandleA("ntdll.dll") );
            ptr = RtlImageDirectoryEntryToData( (void *)base, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &len2 );
            ok( func != ptr, "RtlLookupFunctionTable wrong table, got: %p / %p\n", func, ptr );
            ptr = RtlImageDirectoryEntryToData( (void *)base, TRUE, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &len2 );
            metadata = (void *)((IMAGE_LOAD_CONFIG_DIRECTORY *)ptr)->CHPEMetadataPointer;
            ok( (char *)func == (char *)base + metadata->ExtraRFETable,
                "RtlLookupFunctonTable wrong table, got: %p / %p\n", func, (char *)base + metadata->ExtraRFETable );
            ok( len == metadata->ExtraRFETableSize, "RtlLookupFunctionTable wrong len, got: %lu / %lu\n",
                len, metadata->ExtraRFETableSize );
        }

        arm64func->BeginAddress = code_offset;
        arm64func->Flag = 1;
        arm64func->FunctionLength = 4;
        arm64func->RegF = 1;
        arm64func->RegI = 1;
        arm64func->H = 1;
        arm64func->CR = 1;
        arm64func->FrameSize = 1;
        arm64func++;
        arm64func->BeginAddress = code_offset + 16;
        arm64func->Flag = 1;
        arm64func->FunctionLength = 4;
        arm64func->RegF = 1;
        arm64func->RegI = 1;
        arm64func->H = 1;
        arm64func->CR = 1;
        arm64func->FrameSize = 1;

        growable_table = NULL;
        status = pRtlAddGrowableFunctionTable( &growable_table, (RUNTIME_FUNCTION *)buf,
                                               2, 2, ec_code, ec_code + code_offset + 64 );
        ok( !status, "RtlAddGrowableFunctionTable failed %lx\n", status );

        base = 0xdeadbeef;
        func = pRtlLookupFunctionEntry( ec_code + code_offset + 8, &base, NULL );
        ok( func == (RUNTIME_FUNCTION *)buf, "RtlLookupFunctionEntry expected func: %p, got: %p\n",
            buf, func );
        ok( base == ec_code, "RtlLookupFunctionEntry expected base: %Ix, got: %Ix\n",
            ec_code, base );

        base = 0xdeadbeef;
        func = pRtlLookupFunctionEntry( ec_code + code_offset + 16, &base, NULL );
        ok( func == (RUNTIME_FUNCTION *)(buf + sizeof(*arm64func)),
            "RtlLookupFunctionEntry expected func: %p, got: %p\n", buf + sizeof(*arm64func), func );
        ok( base == ec_code, "RtlLookupFunctionEntry expected base: %Ix, got: %Ix\n", ec_code, base );

        base = 0xdeadbeef;
        func = pRtlLookupFunctionEntry( ec_code + code_offset + 32, &base, NULL );
        ok( !func, "RtlLookupFunctionEntry got: %p\n", func );
        ok( base == 0xdeadbeef, "RtlLookupFunctionEntry got: %Ix\n", base );

        pRtlDeleteGrowableFunctionTable( growable_table );
        VirtualFree( (void *)ec_code, 0, MEM_RELEASE );
    }
#endif // __REACTOS__
}


START_TEST(unwind)
{
    ntdll = GetModuleHandleA("ntdll.dll");
    code_mem = VirtualAlloc( NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );

#define X(f) p##f = (void*)GetProcAddress(ntdll, #f)
    X(NtAllocateVirtualMemoryEx);
    X(RtlAddFunctionTable);
    X(RtlAddGrowableFunctionTable);
    X(RtlDeleteFunctionTable);
    X(RtlDeleteGrowableFunctionTable);
    X(RtlGrowFunctionTable);
    X(RtlInstallFunctionTableCallback);
    X(RtlLookupFunctionEntry);
    X(RtlLookupFunctionTable);
    X(RtlVirtualUnwind2);
#undef X

#ifdef __arm__
    test_virtual_unwind_arm();
#elif defined(__aarch64__)
    test_virtual_unwind_arm64();
#elif defined(__x86_64__)
    test_virtual_unwind_x86();
#ifndef __REACTOS__
    test_virtual_unwind_arm64();
#endif // __REACTOS__
#endif

    test_dynamic_unwind();
}

#else  /* !__i386__ */

START_TEST(unwind)
{
}

#endif  /* !__i386__ */
