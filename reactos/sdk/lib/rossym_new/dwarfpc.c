/*
 * Dwarf pc to source line conversion.
 * 
 * Maybe should do the reverse here, but what should the interface look like?
 * One possibility is to use the Plan 9 line2addr interface:
 *
 *	long line2addr(ulong line, ulong basepc)
 *
 * which returns the smallest pc > basepc with line number line (ignoring file name).
 *
 * The encoding may be small, but it sure isn't simple!
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>
#define trace 0

enum
{
    Isstmt = 1<<0,
    BasicDwarfBlock = 1<<1,
    EndSequence = 1<<2,
    PrologueEnd = 1<<3,
    EpilogueBegin = 1<<4
};

typedef struct State State;
struct State
{
    ulong addr;
    ulong file;
    ulong line;
    ulong column;
    ulong flags;
    ulong isa;
};

int
dwarfpctoline(Dwarf *d, DwarfSym *proc, ulong pc, char **file, char **function, ulong *line)
{
	char *cdir;
    uchar *prog, *opcount, *end, *dirs;
    ulong off, unit, len, vers, x, start, lastline;
    int i, first, firstline, op, a, l, quantum, isstmt, linebase, linerange, opcodebase, nf;
    char *files, *s;
    DwarfBuf b;
    DwarfSym sym;
    State emit, cur, reset;
    char **f, **newf;

    f = nil;
    memset(proc, 0, sizeof(*proc));

    int runit = dwarfaddrtounit(d, pc, &unit);
    if (runit < 0)
        return -1;
    int rtag = dwarflookuptag(d, unit, TagCompileUnit, &sym);
    if (rtag < 0)
        return -1;

    if(!sym.attrs.have.stmtlist){
        werrstr("no line mapping information for 0x%x", pc);
        return -1;
    }
    off = sym.attrs.stmtlist;
    if(off >= d->line.len){
        werrstr("bad stmtlist");
        goto bad;
    }

    if(trace) werrstr("unit 0x%x stmtlist 0x%x", unit, sym.attrs.stmtlist);

    memset(&b, 0, sizeof b);
    b.d = d;
    b.p = d->line.data + off;
    b.ep = b.p + d->line.len;
    b.addrsize = sym.b.addrsize;	/* should i get this from somewhere else? */

    len = dwarfget4(&b);
    if(b.p==nil || b.p+len > b.ep || b.p+len < b.p){
        werrstr("bad len");
        goto bad;
    }

    b.ep = b.p+len;
    vers = dwarfget2(&b);
    if(vers != 2){
        werrstr("bad dwarf version 0x%x", vers);
        return -1;
    }

    len = dwarfget4(&b);
    if(b.p==nil || b.p+len > b.ep || b.p+len < b.p){
        werrstr("another bad len");
        goto bad;
    }
    prog = b.p+len;

    quantum = dwarfget1(&b);
    isstmt = dwarfget1(&b);
    linebase = (schar)dwarfget1(&b);
    linerange = (schar)dwarfget1(&b);
    opcodebase = dwarfget1(&b);

    opcount = b.p-1;
    dwarfgetnref(&b, opcodebase-1);
    if(b.p == nil){
        werrstr("bad opcode chart");
        goto bad;
    }

    /* just skip the files and dirs for now; we'll come back */
    dirs = b.p;
    while (b.p && *b.p)
        dwarfgetstring(&b);
    dwarfget1(&b);

    files = (char*)b.p;
    while(b.p!=nil && *b.p!=0){
        dwarfgetstring(&b);
        dwarfget128(&b);
        dwarfget128(&b);
        dwarfget128(&b);
    }
    dwarfget1(&b);

    /* move on to the program */
    if(b.p == nil || b.p > prog){
        werrstr("bad header");
        goto bad;
    }
    b.p = prog;

    reset.addr = 0;
    reset.file = 1;
    reset.line = 1;
    reset.column = 0;
    reset.flags = isstmt ? Isstmt : 0;
    reset.isa = 0;

    cur = reset;
    emit = reset;
    nf = 0;
    start = 0;
    if(trace) werrstr("program @ %lu ... %.*H opbase = %d", b.p - d->line.data, b.ep-b.p, b.p, opcodebase);
    first = 1;
    while(b.p != nil){
        firstline = 0;
        op = dwarfget1(&b);
        if(trace) werrstr("\tline %lu, addr 0x%x, op %d %.10H", cur.line, cur.addr, op, b.p);
        if(op >= opcodebase){
            a = (op - opcodebase) / linerange;
            l = (op - opcodebase) % linerange + linebase;
            cur.line += l;
            cur.addr += a * quantum;
            if(trace) werrstr(" +%d,%d", a, l);
        emit:
            if(first){
                if(cur.addr > pc){
                    werrstr("found wrong line mapping 0x%x for pc 0x%x", cur.addr, pc);
                    /* This is an overzealous check.  gcc can produce discontiguous ranges
                       and reorder statements, so it's possible for a future line to start
                       ahead of pc and still find a matching one. */
                    /*goto out;*/
                    firstline = 1;
                }
                first = 0;
                start = cur.addr;
            }
            if(cur.addr > pc && !firstline)
                break;
            if(b.p == nil){
                werrstr("buffer underflow in line mapping");
                goto out;
            }
            emit = cur;
            if(emit.flags & EndSequence){
                werrstr("found wrong line mapping 0x%x-0x%x for pc 0x%x", start, cur.addr, pc);
                goto out;
            }
            cur.flags &= ~(BasicDwarfBlock|PrologueEnd|EpilogueBegin);
        }else{
            switch(op){
            case 0:	/* extended op code */
                if(trace) werrstr(" ext");
                len = dwarfget128(&b);
                end = b.p+len;
                if(b.p == nil || end > b.ep || end < b.p || len < 1)
                    goto bad;
                switch(dwarfget1(&b)){
                case 1:	/* end sequence */
                    if(trace) werrstr(" end");
                    cur.flags |= EndSequence;
                    goto emit;
                case 2:	/* set address */
                    cur.addr = dwarfgetaddr(&b);
                    if(trace) werrstr(" set pc 0x%x", cur.addr);
                    break;
                case 3:	/* define file */
                    newf = malloc(nf+1*sizeof(f[0]));
                    if (newf)
                        RtlMoveMemory(newf, f, nf*sizeof(f[0]));
                    if(newf == nil)
                        goto out;
					free(f);
                    f = newf;
					f[nf++] = s = dwarfgetstring(&b);
					DPRINT1("str %s", s);
                    dwarfget128(&b);
                    dwarfget128(&b);
                    dwarfget128(&b);
                    if(trace) werrstr(" def file %s", s);
                    break;
                }
                if(b.p == nil || b.p > end)
                    goto bad;
                b.p = end;
                break;
            case 1:	/* emit */
                if(trace) werrstr(" emit");
                goto emit;
            case 2:	/* advance pc */
                a = dwarfget128(&b);
                if(trace) werrstr(" advance pc + %lu", a*quantum);
                cur.addr += a * quantum;
                break;
            case 3:	/* advance line */
                l = dwarfget128s(&b);
                if(trace) werrstr(" advance line + %ld", l);
                cur.line += l;
                break;
            case 4:	/* set file */
                if(trace) werrstr(" set file");
                cur.file = dwarfget128s(&b);
                break;
            case 5:	/* set column */
                if(trace) werrstr(" set column");
                cur.column = dwarfget128(&b);
                break;
            case 6:	/* negate stmt */
                if(trace) werrstr(" negate stmt");
                cur.flags ^= Isstmt;
                break;
            case 7:	/* set basic block */
                if(trace) werrstr(" set basic block");
                cur.flags |= BasicDwarfBlock;
                break;
            case 8:	/* const add pc */
                a = (255 - opcodebase) / linerange * quantum;
                if(trace) werrstr(" const add pc + %d", a);
                cur.addr += a;
                break;
            case 9:	/* fixed advance pc */
                a = dwarfget2(&b);
                if(trace) werrstr(" fixed advance pc + %d", a);
                cur.addr += a;
                break;
            case 10:	/* set prologue end */
                if(trace) werrstr(" set prologue end");
                cur.flags |= PrologueEnd;
                break;
            case 11:	/* set epilogue begin */
                if(trace) werrstr(" set epilogue begin");
                cur.flags |= EpilogueBegin;
                break;
            case 12:	/* set isa */
                if(trace) werrstr(" set isa");
                cur.isa = dwarfget128(&b);
                break;
            default:	/* something new - skip it */
                if(trace) werrstr(" unknown %d", opcount[op]);
                for(i=0; i<opcount[op]; i++)
                    dwarfget128(&b);
                break;
            }
        }
    }
    if(b.p == nil)
        goto bad;

    /* finally!  the data we seek is in "emit" */

    if(emit.file == 0){
        werrstr("invalid file index in mapping data");
        goto out;
    }
    if(line)
        *line = emit.line;

    /* skip over first emit.file-2 guys */
    b.p = (uchar*)files;
    for(i=emit.file-1; i > 0 && b.p!=nil && *b.p!=0; i--){
        dwarfgetstring(&b);
        dwarfget128(&b);
        dwarfget128(&b);
        dwarfget128(&b);
    }
    if(b.p == nil){
        werrstr("problem parsing file data second time (cannot happen)");
        goto bad;
    }
    if(*b.p == 0){
        if(i >= nf){
            werrstr("bad file index in mapping data");
            goto bad;
        }
        b.p = (uchar*)f[i];
    }
    s = dwarfgetstring(&b);
	*file = s;
    i = dwarfget128(&b);		/* directory */
    x = dwarfget128(&b);
    x = dwarfget128(&b);

    /* fetch dir name */
	cdir = sym.attrs.have.compdir ? sym.attrs.compdir : 0;

	char *dwarfdir;
	dwarfdir = nil;
	b.p = dirs;
	for (x = 1; b.p && *b.p; x++) {
		dwarfdir = dwarfgetstring(&b);
		if (x == i) break;
	}

	if (!cdir && dwarfdir)
		cdir = dwarfdir;
	
	char *filefull = malloc(strlen(cdir) + strlen(*file) + 2);
	strcpy(filefull, cdir);
	strcat(filefull, "/");
	strcat(filefull, *file);
	*file = filefull;
	
    *function = nil;
    lastline = 0;
	
    runit = dwarfaddrtounit(d, pc, &unit);
    if (runit == 0) {
        DwarfSym compunit = { };
        int renum = dwarfenumunit(d, unit, &compunit);
        if (renum < 0)
            return -1;
        renum = dwarfnextsymat(d, &compunit, proc);
        while (renum == 0) {
            if (proc->attrs.tag == TagSubprogram && 
				proc->attrs.have.name)
			{
                if (proc->attrs.lowpc <= pc && proc->attrs.highpc > pc) {
                    *function = malloc(strlen(proc->attrs.name)+1);
					strcpy(*function, proc->attrs.name);
                    goto done;
				}
			}
            renum = dwarfnextsym(d, proc);
        }
    }

    // Next search by declaration
    runit = dwarfaddrtounit(d, pc, &unit);
    if (runit == 0) {
        DwarfSym compunit = { };
        int renum = dwarfenumunit(d, unit, &compunit);
        if (renum < 0)
            return -1;
        renum = dwarfnextsymat(d, &compunit, proc);
        while (renum == 0) {
            if (proc->attrs.tag == TagSubprogram && 
				proc->attrs.have.name &&
				proc->attrs.declfile == emit.file)
			{
                if (proc->attrs.declline <= *line &&
                    proc->attrs.declline > lastline) {
                    free(*function);
                    *function = malloc(strlen(proc->attrs.name)+1);
					strcpy(*function, proc->attrs.name);
                    goto done;
				}
                lastline = proc->attrs.declline;
			}
            renum = dwarfnextsym(d, proc);
        }
    }

    /* free at last, free at last */
done:
    free(f);
    return 0;
bad:
    werrstr("corrupted line mapping for 0x%x", pc);
out:
    free(f);
    return -1;
}

VOID RosSymFreeInfo(PROSSYM_LINEINFO LineInfo)
{
    int i;
	free(LineInfo->FileName);
	LineInfo->FileName = NULL;
	free(LineInfo->FunctionName);
	LineInfo->FunctionName = NULL;
    for (i = 0; i < sizeof(LineInfo->Parameters)/sizeof(LineInfo->Parameters[0]); i++)
        free(LineInfo->Parameters[i].ValueName);
}
