/* $Id: stubs.c,v 1.2 2004/08/12 15:41:36 weiden Exp $
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
/* #define NDEBUG */
#include "libsky.h"

/*
 * @unimplemented
 */
void __cdecl
__libc_init_memory(void *end,
                   void *__bss_end__,
                   void *__bss_start__)
{
  STUB("__libc_init_memory: end=0x%x __bss_end__=0x%x __bss_start__=0x%x\n", end, __bss_end__, __bss_start__);
  #if 1
  RtlZeroMemory(__bss_start__, (PCHAR)__bss_end__ - (PCHAR)__bss_start__);
  #else
  RtlCopyMemory(__bss_start__, /* I think this function should initialize the data in the .bss section */
                end, /* The source should be the pointer to raw of the EOF Extra Data, but how to get it?! It's not loaded to memory in win/ros */
                (PCHAR)__bss_end__ - (PCHAR)__bss_start__); /* Or should we rather copy 0x2000 (raw size of EOF Extra data)? */
  #endif
}


typedef void (__cdecl *func_ptr) (void);

/*
 * @unimplemented
 */
void __cdecl
ctor_dtor_initialize(func_ptr *__CTOR_LIST__,
                     func_ptr *__DTOR_LIST__,
                     void *unknown)
{
  STUB("ctor_dtor_initialize: __CTOR_LIST__=0x%x __DTOR_LIST__=0x%x unknown=0x%x\n", __CTOR_LIST__, __DTOR_LIST__, unknown);
  
  /* unknown apparently is the virtual address of the .bss section, but what should
   * we do with it?! Perhaps load a list of constructor/destructor addresses to this
   * address before we call them?
   */
  
  /*
   * Call constructors
   */
  if(__CTOR_LIST__ != NULL)
  {
    unsigned long nptrs;
    /*
     * If the first entry in the constructor list is -1 then the list
     * is terminated with a null entry. Otherwise the first entry was
     * the number of pointers in the list.
     */
    DBG("Calling constructors...\n");
    nptrs = (unsigned long)__CTOR_LIST__[0];
    if (nptrs == -1)
    {
      for(nptrs = 0; __CTOR_LIST__[nptrs + 1] != NULL; nptrs++);
    }
    DBG("There are %d constructors to call...\n", nptrs);
    
    /*
     * Go through the list backwards calling constructors.
     * FIXME - backwards?! This is ripped off crtdll\misc\gccmain.c
     */
    for(; nptrs > 0; nptrs--)
    {
      DBG("call constructor 0x%x\n", __CTOR_LIST__[nptrs]);
      __CTOR_LIST__[nptrs]();
    }
    DBG("Called all constructors\n");
  }
  
  /*
   * Call destructors
   */
  if(__DTOR_LIST__ != NULL)
  {
    unsigned long nptrs;
    /*
     * If the first entry in the destructor list is -1 then the list
     * is terminated with a null entry. Otherwise the first entry was
     * the number of pointers in the list.
     */
    DBG("Calling destructors...\n");
    nptrs = (unsigned long)__DTOR_LIST__[0];
    if (nptrs == -1)
    {
      for(nptrs = 0; __DTOR_LIST__[nptrs + 1] != NULL; nptrs++);
    }
    DBG("There are %d destructors to call...\n", nptrs);

    /*
     * Go through the list backwards calling constructors.
     * FIXME - backwards?! This is ripped off crtdll\misc\gccmain.c
     */
    for(; nptrs > 0; nptrs--)
    {
      DBG("call destructor 0x%x\n", __DTOR_LIST__[nptrs]);
      __DTOR_LIST__[nptrs]();
    }
    DBG("Called all destructors\n");
  }
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
