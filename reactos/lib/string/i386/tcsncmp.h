/* $Id: tcsncmp.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include "tchar.h"

.globl	_tcsncmp

_tcsncmp:
 push %esi
 push %edi
 mov  0x0C(%esp), %esi /* s1 */
 mov  0x10(%esp), %edi /* s2 */
 mov  0x14(%esp), %ecx /* n */

 xor  %eax,%eax
 cld

.L1:
 dec  %ecx
 js   .L2
 _tlods
 _tscas
 jne  .L3
 test %eax, %eax
 jne  .L1

.L2:	
 xor  %eax, %eax
 jmp  .L4

.L3:
 sbb  %eax, %eax
 or   $1, %al		

.L4:
 pop  %edi
 pop  %esi
 ret

/* EOF */
