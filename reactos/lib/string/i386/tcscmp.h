/* $Id: tcscmp.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include "tchar.h"

.globl	_tcscmp

_tcscmp:
 push %esi
 push %edi
 mov  0x0C(%esp), %esi
 mov  0x10(%esp), %edi
 xor  %eax, %eax
 cld

.L1:
 _tlods
 _tscas
 jne  .L2
 test %eax, %eax
 jne  .L1
 xor  %eax, %eax
 jmp  .L3

.L2:
 sbb  %eax, %eax
 or   $1, %al		

.L3:
 pop  %edi
 pop  %esi
 ret

/* EOF */
