void _CDECL DbgDumpCpu(int flags)
{
	CPUR_ALL sts;
	CPUR_ALL *psts = &sts;
	CPU_GDTE *pgdt;
	CPU_IDTE *pidt;
	CPU_TSS *ptss;
	i16u sel;
	i32 x;

	DbgPrintf("cpu state:\n");
	memset(psts, 0, sizeof(CPUR_ALL));

	CpuRegSave(psts, flags);

	if(flags & DBG_DUMPCPU_GP)
	{
		DbgPrintf("eflags=%08x, ", psts->f.f);
		DbgPrintf("eax=%08x, ebx=%08x, ecx=%08x, edx=%08x, esi=%08x, edi=%08x, ebp=%08x, esp=%08x\n",
			psts->gp.eax, psts->gp.ebx, psts->gp.ecx, psts->gp.edx, psts->gp.esi, psts->gp.edi, psts->gp.ebp, psts->gp.esp);
	}
	if(flags & DBG_DUMPCPU_SEG)
	{
		DbgPrintf("cs=%04hx, ss=%04hx, ds=%04hx, es=%04hx, fs=%04hx, gs=%04hx\n",
			psts->seg.cs, psts->seg.ss, psts->seg.ds, psts->seg.es, psts->seg.fs, psts->seg.gs);
	}
	if(flags & DBG_DUMPCPU_C)
	{
		DbgPrintf("cr0=%08x, cr2=%08x, cr3=%08x, cr4=%08x\n",
			psts->c.cr0, psts->c.cr2, psts->c.cr3, psts->c.cr4);
		DbgPrintf("gdtr=%08x.%04hx, idtr=%08x.%04hx, ldtr=%04hx, tr=%04hx\n",
			psts->c.gdtr.base, psts->c.gdtr.limit, psts->c.idtr.base, psts->c.idtr.limit, psts->c.ldtr, psts->c.tr);
	}
	if(flags & DBG_DUMPCPU_GDT)
	{
		DbgPrintf("gdt @ %08x.%04hx:\n", psts->c.gdtr.base, psts->c.gdtr.limit);
		for(sel=0, pgdt=(CPU_GDTE *)psts->c.gdtr.base; sel <= psts->c.gdtr.limit; sel+=8, pgdt++)
		{
			DbgPrintf("%04hx: 0-31=%08x, 32-63=%08x, p=%x, base=%x, limit=%x, g=%x, type=%X, siz=%x, dpl=%x, avl=%x\n",
				sel, *((i32 *)pgdt), *(((i32 *)pgdt)+1), pgdt->p,
				(i32)pgdt->base0 | (i32)pgdt->base16<<16 | (i32)pgdt->base24<<24,
				(i32)pgdt->limit0 | (i32)pgdt->limit16<<16, pgdt->g,
				pgdt->type, pgdt->siz, pgdt->dpl, pgdt->avl); 
		}
	}
	if(flags & DBG_DUMPCPU_IDT)
	{
		DbgPrintf("idt @ %08x.%04hx:\n", psts->c.idtr.base, psts->c.idtr.limit);
		for(sel=0, pidt=(CPU_IDTE *)psts->c.idtr.base; sel <= psts->c.idtr.limit; sel+=8, pidt++)
		{
			DbgPrintf("%04hx: 0-31=%08x, 32-63=%08x, p=%x, sel=%x, offset=%x, type=%x, siz=%x, dpl=%x\n",
				sel, *((i32 *)pidt), *(((i32 *)pidt)+1), pidt->p,
				pidt->sel, (i32)pidt->offset0 | (i32)pidt->offset16<<16,
				pidt->type, pidt->siz, pidt->dpl);
		}
	}
	if((flags & DBG_DUMPCPU_TSS) && psts->c.tr)
	{
		pgdt = (CPU_GDTE *)(((i8*)psts->c.gdtr.base) + psts->c.tr);
		ptss = (CPU_TSS *)((i32)pgdt->base0 | (i32)pgdt->base16<<16 | (i32)pgdt->base24<<24);
		x = (i32)pgdt->limit0 | ((i32)pgdt->limit16<<16) + 1;
		DbgPrintf("tss @ %08x.%08x:\n", (int)ptss, x);
		DbgDumpMem(ptss, x);
	}
}
