/* $Id$
*/

#include "tchar.h"

.globl	_tcslen

_tcslen:
 push  %edi
 mov   0x8(%esp), %edi
 xor   %eax, %eax
 test  %edi,%edi
 jz   _tcslen_end

 mov   $-1, %ecx
 cld

 repne _tscas

 not   %ecx
 dec   %ecx

 mov   %ecx, %eax

_tcslen_end:
 pop   %edi
 ret

/* EOF */
