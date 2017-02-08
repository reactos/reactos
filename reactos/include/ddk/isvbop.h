/*
 * isvbop.h
 *
 * Windows NT Device Driver Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * The corresponding ASM header of this file is isvbop.inc.
 */

#pragma once

/* BOP Identifiers */
#define BOP_3RDPARTY    0x58    // 3rd-party VDD BOP
#define BOP_UNSIMULATE  0xFE    // Stop execution

#if defined(__GNUC__)

#define RegisterModule()    __asm__(".byte 0xC4, 0xC4, %c0, 0" : : "i"(BOP_3RDPARTY))
#define UnRegisterModule()  __asm__(".byte 0xC4, 0xC4, %c0, 1" : : "i"(BOP_3RDPARTY))
#define DispatchCall()      __asm__(".byte 0xC4, 0xC4, %c0, 2" : : "i"(BOP_3RDPARTY))
#define VDDUnSimulate16()   __asm__(".byte 0xC4, 0xC4, %c0"    : : "i"(BOP_UNSIMULATE))

#elif defined(_MSC_VER)

#define RegisterModule()    _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_3RDPARTY _asm _emit 0
#define UnRegisterModule()  _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_3RDPARTY _asm _emit 1
#define DispatchCall()      _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_3RDPARTY _asm _emit 2
#define VDDUnSimulate16()   _asm _emit 0xC4 _asm _emit 0xC4 _asm _emit BOP_UNSIMULATE

#else
#error Unknown compiler for inline assembler
#endif

/* EOF */
