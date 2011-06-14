

Int386:
    /* Get the interupt vector and patch the opcode */
    mov al, byte ptr ds:[BSS_IntVector]
    mov byte ptr ds:[Int386_vector_opcode], al

	/* Setup the registers */
	mov ax, word ptr cs:[BSS_RegisterSet + REGS_DS]
	mov ds, ax					/* DS register */
	mov ax, word ptr cs:[BSS_RegisterSet + REGS_ES]
	mov es, ax					/* ES register */
	mov ax, word ptr cs:[BSS_RegisterSet + REGS_FS]
	mov fs, ax					/* FS register */
	mov ax, word ptr cs:[BSS_RegisterSet + REGS_GS]
	mov gs, ax					/* GS register */

	mov eax, dword ptr cs:[BSS_RegisterSet + REGS_EAX]	/* EAX register */
	mov ebx, dword ptr cs:[BSS_RegisterSet + REGS_EBX]	/* EBX register */
	mov ecx, dword ptr cs:[BSS_RegisterSet + REGS_ECX]	/* ECX register */
	mov edx, dword ptr cs:[BSS_RegisterSet + REGS_EDX]	/* EDX register */
	mov esi, dword ptr cs:[BSS_RegisterSet + REGS_ESI]	/* ESI register */
	mov edi, dword ptr cs:[BSS_RegisterSet + REGS_EDI]	/* EDI register */

	/* Do not set the flags register */
	/* only return its value in regsout */

	/* Call the interrupt vector */
	/*int		Int386_vector*/
	.byte	0xcd
Int386_vector_opcode:
	.byte	0x00

	/* Save the registers */
	mov dword ptr cs:[BSS_RegisterSet + REGS_EAX], eax	/* EAX register */
	mov dword ptr cs:[BSS_RegisterSet + REGS_EBX], ebx	/* EBX register */
	mov dword ptr cs:[BSS_RegisterSet + REGS_ECX], ecx	/* ECX register */
	mov dword ptr cs:[BSS_RegisterSet + REGS_EDX], edx	/* EDX register */
	mov dword ptr cs:[BSS_RegisterSet + REGS_ESI], esi	/* ESI register */
	mov dword ptr cs:[BSS_RegisterSet + REGS_EDI], edi	/* EDI register */

	mov ax, ds					/* DS register */
	mov word ptr cs:[BSS_RegisterSet + REGS_DS], ax
	mov ax, es					/* ES register */
	mov word ptr cs:[BSS_RegisterSet + REGS_ES], ax
	mov ax, fs					/* FS register */
	mov word ptr cs:[BSS_RegisterSet + REGS_FS], ax
	mov ax, gs					/* GS register */
	mov word ptr cs:[BSS_RegisterSet + REGS_GS], ax

	pushf
	pop dword ptr cs:[BSS_RegisterSet + REGS_EFLAGS]	/* EFLAGS register */

    ret



