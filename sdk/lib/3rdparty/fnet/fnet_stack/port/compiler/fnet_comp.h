/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  C-compiler-specific definitions.
*
***************************************************************************/


#ifndef _FNET_COMP_H_

#define _FNET_COMP_H_

#include "fnet_config.h"

#if FNET_CFG_COMP_CW
    /* To use _Pragma(), enable C99 Extensions. */
    #pragma c99 on

    /* For CW10.1 ARM compiler. After CW10.2 Release is obsolete. */
    #if FNET_MK
        #pragma load_store_elimination off   /*  Disable Load Store Optimization, workaround of CW10.x bug. */
        #define FNET_COMP_PACKED __attribute__ ((aligned (1)))
    #endif

    #if 1 /* Newest CWs pragma */
        #define FNET_COMP_PACKED_BEGIN  _Pragma("pack(1)")
        #define FNET_COMP_PACKED_END    _Pragma("pack()")
    #else /* Old CWs pragma */
        #define FNET_COMP_PACKED_BEGIN  _Pragma("options align = packed")
        #define FNET_COMP_PACKED_END    _Pragma("options align = reset")
    #endif

#endif

#if FNET_CFG_COMP_IAR
    /* Disable some warnings.*/
    #pragma diag_suppress=Pa039

    /* To use _Pragma(), enable C99 Extensions. */
    #define FNET_COMP_PACKED_BEGIN  _Pragma("pack(1)")
    #define FNET_COMP_PACKED_END    _Pragma("pack()")

    /* Inline assembler */
    #define FNET_COMP_ASM(x)        __asm(x)
#endif

#if FNET_CFG_COMP_UV

#if __CC_ARM /* Keil v5*/
    /* To use anonimous unions.*/
    #pragma anon_unions
#endif

    /* To use _Pragma(), enable C99 Extensions. */
    #define FNET_COMP_PACKED_BEGIN  _Pragma("pack(1)") //TBD ("pack(push, 1)")
    #define FNET_COMP_PACKED_END    _Pragma("pack(8)") //TBD ("pack(pop)")

    #define FNET_COMP_PACKED_VAR    __packed

    /* Inline assembler */
    #define FNET_COMP_ASM(x)        __asm(x)
#endif

#if FNET_CFG_COMP_GNUC
    #define FNET_COMP_PACKED_BEGIN  _Pragma("pack(push, 1)")
    #define FNET_COMP_PACKED_END    _Pragma("pack(pop)")

    /* Inline assembler */
    #define FNET_COMP_ASM(x)        __asm(x)
#endif

#if FNET_CFG_COMP_GHS
    #define FNET_COMP_PACKED_BEGIN  _Pragma("pack(1)")
    #define FNET_COMP_PACKED_END    _Pragma("pack()")

    #define FNET_COMP_PACKED_VAR    __packed

    /* Inline assembler */
    #define FNET_COMP_ASM(x)        __asm(x)
#endif

#if FNET_CFG_COMP_DCC
    #pragma option -Xalign-min=1
    #define FNET_COMP_PACKED_BEGIN  #pragma pack(2,2)
    #define FNET_COMP_PACKED_END    #pragma pack(4,2)

    /* Inline assembler */
    #define FNET_COMP_ASM(x)        __asm(x)
#endif

/* Changes the current alignment mode for one member in a structure
 *  so that member will be packed on a 1-byte boundary */
#ifndef FNET_COMP_PACKED
    #define FNET_COMP_PACKED
#endif

/* Modifies the current alignment mode in the structure,
which causes that member to be packed on a 1-byte boundary
*/
#ifndef FNET_COMP_PACKED_BEGIN
    #define FNET_COMP_PACKED_BEGIN
#endif

/* Modifies the current alignment mode of the variable to unaligned.
*/
#ifndef FNET_COMP_PACKED_VAR
    #define FNET_COMP_PACKED_VAR
#endif

/* Restore alignment mode.*/
#ifndef FNET_COMP_PACKED_END
    #define FNET_COMP_PACKED_END
#endif

/* Mark unused function argument. To avoid compiler warnings.*/
#ifndef FNET_COMP_UNUSED_ARG
    #define FNET_COMP_UNUSED_ARG(x) (void)(x)
#endif

/* Inline assembler */
#ifndef FNET_COMP_ASM
    #define FNET_COMP_ASM(x)
#endif

#endif /* _FNET_COMP_H_ */
