/* $Id: tcslen.h,v 1.1 2003/07/06 23:04:19 hyperion Exp $
*/

#include "tchar.h"

.globl	_tcslen

_tcslen:
 push  %edi
 mov   0x8(%esp), %edi
 xor   %eax, %eax
 mov   $-1, %ecx
 cld

 repne _tscas

 not   %ecx
 dec   %ecx

 mov   %ecx, %eax
 pop   %edi
 ret

/* EOF */
