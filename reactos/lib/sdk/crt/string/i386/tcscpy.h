/* $Id$
 */

#include "tchar.h"

.globl	_tcscpy

_tcscpy:
 push %esi
 push %edi
 mov  0x0C(%esp), %edi
 mov  0x10(%esp), %esi
 cld

.L1:
 _tlods
 _tstos
 test %_treg(a), %_treg(a)
 jnz  .L1

 mov  0x0C(%esp), %eax

 pop  %edi
 pop  %esi
 ret

/* EOF */
