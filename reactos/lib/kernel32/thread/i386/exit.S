.extern _K32ExitThread@4

.globl _K32SwitchStackAndExitThread@12

_K32SwitchStackAndExitThread@12:
 movl  0xC(%esp), %edx /* nExitCode */
 movl  0x8(%esp), %ecx /* nStackSize */
 movl  0x4(%esp), %esp /* pStackBase */
 addl  %ecx, %esp
 subl  $0x4, %esp
 pushl %edx
 call  _K32ExitThread@4

/* EOF */
