/* $Id: tcsncat.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 */

#include "tchar.h"

.globl _tcsncat

_tcsncat:
 push  %esi
 push  %edi
 mov   0x0C(%esp), %edi
 mov   0x10(%esp), %esi
 cld

 xor   %eax, %eax
 mov   $-1, %ecx
 repne _tscas
 _tdec(%edi)

 mov   0x14(%esp),%ecx

.L1:	
 dec   %ecx
 js    .L2
 _tlods
 _tstos
 test  %_treg(a), %_treg(a)
 jne   .L1
 jmp   .L3

.L2:
 xor   %eax, %eax
 _tstos

.L3:		
 mov   0x0C(%esp), %eax
 pop   %edi
 pop   %esi

 ret

/* EOF */
