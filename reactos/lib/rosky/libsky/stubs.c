/* $Id: stubs.c,v 1.1 2004/08/12 02:50:35 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         SkyOS library
 * FILE:            lib/libsky/stubs.c
 * PURPOSE:         libsky.dll stubs
 * NOTES:           If you implement a function, remove it from this file
 *
 * UPDATE HISTORY:
 *      08/12/2004  Created
 */
#include <windows.h>
#include <libsky.h>

/*
 * @unimplemented
 */
void __cdecl
__libc_init_memory(void * end,
                   void * __bss_end__,
                   void * __bss_start__)
{
  STUB("__libc_init_memory: end=0x%x __bss_end__=0x%x __bss_start__=0x%x\n", end, __bss_end__, __bss_start__);
}


/*
 * @unimplemented
 */
void __cdecl
ctor_dtor_initialize(void * __CTOR_LIST__,
                      void * __DTOR_LIST__,
                      void * unknown)
{
  STUB("ctor_dtor_initialize: __CTOR_LIST__=0x%x __DTOR_LIST__=0x%x unknown=0x%x\n", __CTOR_LIST__, __DTOR_LIST__, unknown);
}

/*
 * @unimplemented
 */
unsigned long long __cdecl
get_usec_counter(void)
{
  STUB("get_usec_counter() returns 0x0LL!\n");
  return 0x0LL;
}

/* EOF */
