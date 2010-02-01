
void _NAKED _CDECL CpuRegSave(CPUR_ALL *p, i32 flags)
{
	_ASM_BEGIN
	push ebx
	mov ebx, [esp+8]

	pushfd
	pop CPUR_ALL.f.f[ebx]

	mov CPUR_ALL.gp.eax[ebx], eax
	mov CPUR_ALL.gp.ecx[ebx], ecx
	mov CPUR_ALL.gp.edx[ebx], edx
	mov eax, [esp]
	mov CPUR_ALL.gp.ebx[ebx], eax
	mov CPUR_ALL.gp.esp[ebx], esp
	mov CPUR_ALL.gp.ebp[ebx], ebp
	mov CPUR_ALL.gp.esi[ebx], esi
	mov CPUR_ALL.gp.edi[ebx], edi

	// test dword ptr [esp+12], CPUREGSAVE_SEG
	// jz noseg
	mov CPUR_ALL.seg.cs[ebx], cs
	mov CPUR_ALL.seg.ss[ebx], ss
	mov CPUR_ALL.seg.ds[ebx], ds
	mov CPUR_ALL.seg.es[ebx], es
	mov CPUR_ALL.seg.fs[ebx], fs
	mov CPUR_ALL.seg.gs[ebx], gs
noseg:

	test dword ptr [esp+12], CPUREGSAVE_C
	jz noc
	mov eax, cr0
	mov CPUR_ALL.c.cr0[ebx], eax
	mov eax, cr2
	mov CPUR_ALL.c.cr2[ebx], eax
	mov eax, cr3
	mov CPUR_ALL.c.cr3[ebx], eax
	// mov eax, cr4							// legal instruction not recognized, msc bug
	mov_eax_cr4
	mov CPUR_ALL.c.cr4[ebx], eax
	sgdt CPUR_ALL.c.gdtr.limit[ebx]
	sidt CPUR_ALL.c.idtr.limit[ebx]
	sldt CPUR_ALL.c.ldtr[ebx]
	str CPUR_ALL.c.tr[ebx]
noc:

	pop ebx
	ret
	_ASM_END
}
