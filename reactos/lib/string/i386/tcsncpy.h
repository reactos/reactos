/* $Id: tcsncpy.h,v 1.2 2004/01/27 21:43:47 gvg Exp $
 */

#include "tchar.h"

.globl _tcsncpy

_tcsncpy:
 push %esi
 push %edi
 mov  0x0C(%esp), %edi /* s1 */
 mov  0x10(%esp), %esi /* s2 */
 mov  0x14(%esp), %ecx /* n */

 xor  %eax, %eax
 cld

.L1:	
 dec  %ecx
 js   .L2
 _tlods
 _tstos
 test %_treg(a), %_treg(a)
 jnz  .L1

.L2:
 mov  0x0C(%esp), %eax

 pop  %edi
 pop  %esi
 ret

/* EOF */
