/* $Id: tcsnlen.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
*/

#include "tchar.h"

.globl _tcsnlen

_tcsnlen:
 push  %edi
 mov   0x8(%esp), %edi
 mov   0xC(%esp), %ecx
 xor   %eax, %eax
 test  %ecx, %ecx
 jz    .L1
 mov   %ecx, %edx

 cld

 repne _tscas

 sete  %al
 sub   %ecx, %edx
 sub   %eax, %edx
 mov   %edx, %eax

.L1:
 pop   %edi
 ret

/* EOF */
