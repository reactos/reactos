/* $Id: tcscat.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include "tchar.h"

.globl _tcscat

_tcscat:
 push  %esi
 push  %edi
 mov   0x0C(%esp), %edi
 mov   0x10(%esp), %esi

 xor   %eax, %eax
 mov   $-1, %ecx
 cld

 repne _tscas
 _tdec(%edi)

.L1:	
 _tlods
 _tstos
 test  %_treg(a), %_treg(a)
 jnz   .L1

 mov   0x0C(%esp), %eax
 pop   %edi
 pop   %esi
 ret

/* EOF */
