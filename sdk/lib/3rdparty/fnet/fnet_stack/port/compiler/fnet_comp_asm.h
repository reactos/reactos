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
* Compiler-specific definitions that resolve
* differences between different assemblers.
*
***************************************************************************/

#ifndef _FNET_COMP_ASM_H_

#define _FNET_COMP_ASM_H_

#define __FNET_ASM_CODE /* To eliminate non-assembler code.*/

#include "fnet_config.h"

/* --------------- CodeWarrior ---------------*/
#if FNET_CFG_COMP_CW

/* Code section. */
#if FNET_MCF    
	/* CW compiler adds leading underscores to assembly symbols.*/
    #define FNET_COMP_ASM_PREFIX(x) _##x
#endif      

#if FNET_MK      
    #define FNET_COMP_ASM_PREFIX(x) x
#endif  

/* Macro for the equate directive. */    
FNET_COMP_ASM_EQU:  .macro  label,  value
label   .equ    value
        .endm    

/* Extern. */
FNET_COMP_ASM_EXTERN:  .macro  value
	.extern  value
        .endm
        
/* Global.*/
FNET_COMP_ASM_GLOBAL: .macro label
	.global label
	.endm
            
/* Align. */
FNET_COMP_ASM_ALIGN:  .macro  value
	.align  value
        .endm

/* DC.W */
FNET_COMP_ASM_DC16:   .macro value
	.word value
	.endm
/* DC.L */
FNET_COMP_ASM_DC32:    .macro value
	.long value
	.endm

#define FNET_COMP_ASM_LABEL(x)    FNET_COMP_ASM_PREFIX(x):

/* Code section. */
#define FNET_COMP_ASM_CODE  .text            

/* No END in CW.*/
#define FNET_COMP_ASM_END
   
#endif /* FNET_CFG_COMP_CW */

/* --------------- IAR ----------------------*/
#if FNET_CFG_COMP_IAR

/* IAR compiler does not add any leading underscores to assembly symbols*/
#define FNET_COMP_ASM_PREFIX(x) x

/* Macro for the equate directive */
FNET_COMP_ASM_EQU MACRO label, value
label   EQU  value
	ENDM

/* Extern. */
FNET_COMP_ASM_EXTERN  MACRO value
	EXTERN  value
	ENDM

/* Global.*/
FNET_COMP_ASM_GLOBAL MACRO label
	PUBLIC label
	ENDM

/* Align. */
FNET_COMP_ASM_ALIGN  MACRO value
	ALIGN  value
	ENDM
/*DC.W*/
FNET_COMP_ASM_DC16  MACRO value
	DC16 value
	ENDM

/* DC.L */
FNET_COMP_ASM_DC32 MACRO value
	DC32 value
	ENDM


/* Code section. */
#if FNET_MCF      
    #define FNET_COMP_ASM_CODE  RSEG CODE 
#endif      

#if FNET_MK || FNET_LPC || FNET_MIMXRT    
    #define FNET_COMP_ASM_CODE  RSEG .text:CODE
#endif          

#define FNET_COMP_ASM_LABEL(x)    FNET_COMP_ASM_PREFIX(x):

/*END - END directive is placed after the last
statement of a program to tell the assembler that this is the end
of the program module. The assembler will ignore any
statement after an END directive. Carriage return is required
after the END directive.*/
#define FNET_COMP_ASM_END   END 

/* Registers.*/
#if FNET_MCF       
FLASHBAR  DEFINE  0xC04
RAMBAR    DEFINE  0xC05
#endif
    
#endif /* FNET_CFG_COMP_IAR */

/* --------------- Keil uVision ----------------------*/
#if FNET_CFG_COMP_UV

/* uVision compiler does not add any leading underscores to assembly symbols*/
#define FNET_COMP_ASM_PREFIX(x) x

/* Macro for the equate directive */
	MACRO
	FNET_COMP_ASM_EQU $label, $value
$label   EQU  $value
	MEND

/* Extern. */
	MACRO
	FNET_COMP_ASM_EXTERN  $value
	EXTERN  $value   ; IMPORT
	MEND

/* Global.*/
	MACRO
	FNET_COMP_ASM_GLOBAL $label
	EXPORT $label
	MEND

/* Align. */
	MACRO
	FNET_COMP_ASM_ALIGN $value
	ALIGN  $value
	MEND
/*DC.W*/
	MACRO
	FNET_COMP_ASM_DC16  $value
	DCWU $value
	MEND

/* DC.L */
	MACRO
	FNET_COMP_ASM_DC32 $value
	DCDU $value
	MEND

#define FNET_COMP_ASM_LABEL(x)    FNET_COMP_ASM_PREFIX(x)

/* Code section. */
#define FNET_COMP_ASM_CODE	AREA    |.text|, CODE, READONLY


/*END - END directive is placed after the last
statement of a program to tell the assembler that this is the end
of the program module. The assembler will ignore any
statement after an END directive. Carriage return is required
after the END directive.*/
#define FNET_COMP_ASM_END   END 
    
#endif /* FNET_CFG_COMP_UV */

/* --------------- GCC ----------------------*/
#if FNET_CFG_COMP_GNUC

/* Code section. */
/* GCC compiler adds leading underscores to assembly symbols.*/
#define FNET_COMP_ASM_PREFIX(x) x

/* Macro for the equate directive. */
.macro FNET_COMP_ASM_EQU label, value
   \label   .equ    \value
   .endm

/* Extern. */
.macro FNET_COMP_ASM_EXTERN value
   .extern \value
   .endm

/* Global.*/
.macro FNET_COMP_ASM_GLOBAL label
   .global \label
   .endm

/* Align. */
.macro FNET_COMP_ASM_ALIGN value
   .align \value
   .endm

/* DC.W */
.macro FNET_COMP_ASM_DC16 value
   .word \value
   .endm

/* DC.L */
.macro FNET_COMP_ASM_DC32 value
   .long \value
   .endm

#if FNET_MCF 

#define a0 %a0
#define a1 %a1
#define a2 %a2
#define a3 %a3
#define a4 %a4
#define a5 %a5
#define a6 %a6
#define a7 %a7
#define sp %sp
#define d0 %d0
#define d1 %d1
#define d2 %d2
#define d3 %d3
#define d4 %d4
#define d5 %d5
#define d6 %d6
#define d7 %d7
#define ccr %ccr
#define pc %pc
#define VBR %vbr
#define cacr %cacr
#define sr %sr

#endif /* FNET_MCF */

#define FNET_COMP_ASM_LABEL(x)    FNET_COMP_ASM_PREFIX(x):

/* Code section. */
#define FNET_COMP_ASM_CODE  .text

/* No END in CW.*/
#define FNET_COMP_ASM_END

#endif /* FNET_CFG_COMP_GNUC */

#endif /* _FNET_COMP_ASM_H_ */

