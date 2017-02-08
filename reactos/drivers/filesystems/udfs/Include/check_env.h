////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __CHECK_EXECUTION_ENVIRONMENT__H__
#define __CHECK_EXECUTION_ENVIRONMENT__H__

/*
// check mode
#ifdef NT_KERNEL_MODE
  #ifdef NT_NATIVE_MODE
    #error Error cannot combine Kernel and Natime
  #endif //
#endif //NT_KERNEL_MODE
#ifdef NT_KERNEL_MODE
  #if defined(NT_NATIVE_MODE) || defined(WIN_32_MODE)
    #error !!!! Execution mode definition conflict !!!!
  #endif //
#endif //NT_KERNEL_MODE
#ifdef NT_NATIVE_MODE
  #if defined(WIN_32_MODE)
    #error !!!! Execution mode definition conflict !!!!
  #endif //
#endif //NT_NATIVE_MODE

// include appropriate header(s)
#ifdef NT_KERNEL_MODE
  
  #ifdef NT_DEV_DRV_ENV
    #include <ntddk.h>
  #endif //NT_DEV_DRV_ENV

  #ifdef NT_FS_DRV_ENV
    #include <ntifs.h>
  #endif //NT_DEV_DRV_ENV
  #include "Include/ntddk_ex.h"

  #ifdef WIN_32_ENV
    #error Error: Win32 environment is not supported in Kernel Mode
  #endif //WIN_32_ENV

#endif //NT_KERNEL_MODE

#ifdef NT_NATIVE_MODE
  
  #include "Include/nt_native.h"
  #ifdef NT_DEV_DRV_ENV
    #include "LibCdrw/env_spec_cdrw_w32.h"
  #endif //NT_DEV_DRV_ENV

  #ifdef NT_FS_DRV_ENV
    #error Error: FS Driver environment is not supported in Native Mode
  #endif //NT_DEV_DRV_ENV

  #ifdef WIN_32_ENV
  #endif //WIN_32_ENV

#endif //NT_NATIVE_MODE

#ifdef WIN_32_MODE

  #include "windows.h"
  #ifdef NT_DEV_DRV_ENV
    #include "LibCdrw/env_spec_cdrw_w32.h"
  #endif //NT_DEV_DRV_ENV

  #ifdef NT_FS_DRV_ENV
    #error Error: FS Driver environment is not supported in Win32 Mode
  #endif //NT_DEV_DRV_ENV

  #ifdef WIN_32_ENV
  #endif //WIN_32_ENV

#endif //WIN_32_MODE
*/


#ifdef NT_INCLUDED
#define NT_KERNEL_MODE
#endif //NT_INCLUDED

#ifdef NT_NATIVE_MODE
//#define USER_MODE
#endif //NT_NATIVE_MODE

// default to Win32 environment
#if (!defined(NT_KERNEL_MODE) && !defined(NT_NATIVE_MODE)) || defined(WIN_32_MODE)
//#warning !!!! Execution mode defaulted to WIN_32 !!!!
//#define USER_MODE
#define WIN_32_MODE
#endif 

// check mode
#ifdef NT_KERNEL_MODE
  #if defined(NT_NATIVE_MODE) || defined(WIN_32_MODE)
    #error !!!! Execution mode definition conflict !!!!
  #endif //
#endif //NT_KERNEL_MODE
#ifdef NT_NATIVE_MODE
  #if defined(WIN_32_MODE)
    #error !!!! Execution mode definition conflict !!!!
  #endif //
#endif //NT_NATIVE_MODE


#endif //__CHECK_EXECUTION_ENVIRONMENT__H__
