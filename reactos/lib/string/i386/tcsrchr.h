/* $Id: tcsrchr.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include "tchar.h"

.globl	_tcsrchr

_tcsrchr:
 push %esi
 mov  0x8(%esp), %esi
 mov  0xC(%esp), %edx

 cld
 mov  _tsize, %ecx

.L1:	
 _tlods
 cmp  %_treg(a), %_treg(d)
 jne  .L2
 mov  %esi, %ecx

.L2:	
 test %_treg(a), %_treg(a)
 jnz  .L1

 mov  %ecx, %eax
 _tdec(%eax)
 pop  %esi
 ret

/* EOF */
