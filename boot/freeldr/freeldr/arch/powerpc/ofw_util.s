	.section .text
	.globl ofw_functions
	.globl ofw_call_addr
	.globl call_ofw
call_ofw:	
       /* R3 has the function offset to call (n * 4)
        * Other arg registers are unchanged. */
       subi    %r1,%r1,0x100
       stw     %r8,24(%r1)
       mflr    %r8
       stw     %r8,0(%r1)
       stw     %r3,4(%r1)
       stw     %r4,8(%r1)
       stw     %r5,12(%r1)
       stw     %r6,16(%r1)
       stw     %r7,20(%r1)
       stw     %r9,28(%r1)
       stw     %r10,32(%r1)
       stw     %r20,36(%r1)

       lis     %r10,ofw_functions@ha
       addi    %r8,%r10,ofw_functions@l
       add     %r8,%r3,%r8
       lwz     %r9,0(%r8)
       mtctr   %r9

       mr      %r3,%r4
       mr      %r4,%r5
       mr      %r5,%r6
       mr      %r6,%r7
       mr      %r7,%r8
       mr      %r8,%r9

       /* Call ofw proxy function */
       bctrl

       lwz     %r8,0(%r1)
       mtlr    %r8
       lwz     %r4,8(%r1)
       lwz     %r5,12(%r1)
       lwz     %r6,16(%r1)
       lwz     %r7,20(%r1)
       lwz     %r8,24(%r1)
       lwz     %r9,28(%r1)
       lwz     %r10,32(%r1)
       lwz     %r20,36(%r1)
       addi    %r1,%r1,0x100
       blr

