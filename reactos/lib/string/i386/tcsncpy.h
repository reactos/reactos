/* $Id: tcsncpy.h,v 1.3 2004/01/28 08:51:09 gvg Exp $
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
 rep  _tstos

.L2:
 mov  0x0C(%esp), %eax

 pop  %edi
 pop  %esi
 ret

/* EOF */
