.globl _RtlRosSwitchStackForExit@16

_RtlRosSwitchStackForExit@16:
 movl  0x10(%esp), %edx /* Parameter */
 movl  0xC(%esp),  %eax  /* ExitRoutine */
 movl  0x8(%esp),  %ecx  /* StackSize */
 movl  0x4(%esp),  %esp  /* StackBase */
 addl  %ecx, %esp
 subl  $0x4, %esp
 pushl %edx
 call  *%eax

/* EOF */
