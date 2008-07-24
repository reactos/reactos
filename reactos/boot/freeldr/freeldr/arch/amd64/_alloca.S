/* stuff needed for libgcc on win32.  */
.intel_syntax noprefix

/*#ifdef L_chkstk*/
#ifdef WIN32

	.global ___chkstk
	.global	__alloca
___chkstk:
__alloca:
	push  rcx		/* save temp */
	mov   rcx, rsp	/* get sp */
	add   rcx, 8	/* and point to return addr */

probe:
	cmp  rax, 0x1000	/* > 4k ?*/
	jb   done		

	sub   rcx, 0x1000  		/* yes, move pointer down 4k*/
	or    qword ptr [rcx], 0x0	/* probe there */
	sub   rax, 0x1000	 	/* decrement count */
	jmp   probe           	/* and do it again */

done:
	sub   rcx, rax
	or    qword ptr [rcx], 0x0	/* less that 4k, just peek here */

	mov   rax, rsp
	mov   rsp, rcx	/* decrement stack */

	mov   rcx, [rax]	/* recover saved temp */
	mov   rax, [rax + 8]/* get return address */
	jmp   [rax]


#endif
