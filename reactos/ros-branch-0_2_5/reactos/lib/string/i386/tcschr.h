/* $Id: tcschr.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include "tchar.h"

.globl _tcschr

_tcschr:
 push %esi
 mov  0x8(%esp), %esi
 mov  0xC(%esp), %edx

 cld

.L1:
 _tlods
 cmp  %_treg(a), %_treg(d)
 je   .L2
 test %_treg(a), %_treg(a)
 jnz  .L1
 mov  _tsize, %esi

.L2:
 mov  %esi, %eax
 _tdec(%eax)

 pop  %esi
 ret

/* EOF */
