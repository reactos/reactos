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
#define MAX_DEFINED_FILES 64

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
    ULONG_PTR addr;
    ULONG file;
    ULONG line;
    ULONG column;
    ULONG flags;
    ULONG isa;
};

int
dwarfpctoline(Dwarf *d, DwarfSym *proc, ULONG_PTR pc, char **file, char **dir, char **function, ULONG *line)
{
	char *cdir;
    uchar *prog, *opcount, *end, *dirs;
    ULONG off, unit, len, vers, x;
    ULONG_PTR lastline;
    int i, first, firstline, op, quantum, isstmt, linebase, linerange, opcodebase;
    ULONG_PTR a;
    LONG_PTR l;
    char *files, *s;
    DwarfBuf b;
    DwarfSym sym;
    State emit, cur, reset;
    char *defined_files[MAX_DEFINED_FILES];
    ULONG defined_count = 0;
    ULONG header_file_count = 0;

    memset(proc, 0, sizeof(*proc));

    int runit = dwarfaddrtounit(d, pc, &unit);
    if (runit < 0)
        return -1;
    int rtag = dwarflookuptag(d, unit, TagCompileUnit, &sym);
    if (rtag < 0)
        return -1;

    if(!sym.attrs.have.stmtlist){
        werrstr("no line mapping information for %p", (PVOID)pc);
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
        header_file_count++;
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
    defined_count = 0;
    if(trace) werrstr("program @ %lu ... %.*H opbase = %d", b.p - d->line.data, b.ep-b.p, b.p, opcodebase);
    first = 1;
    while(b.p != nil){
        firstline = 0;
        op = dwarfget1(&b);
        if(trace) werrstr("\tline %lu, addr %p, op %d %.10H", cur.line, (PVOID)cur.addr, op, b.p);
        if(op >= opcodebase){
            a = (op - opcodebase) / linerange;
            l = (op - opcodebase) % linerange + linebase;
            cur.line += l;
            cur.addr += a * quantum;
            if(trace) werrstr(" +%d,%d", a, l);
        emit:
            if(first){
                if(cur.addr > pc){
                    werrstr("found wrong line mapping %p for pc %p", (PVOID)cur.addr, (PVOID)pc);
                    /* This is an overzealous check.  gcc can produce discontiguous ranges
                       and reorder statements, so it's possible for a future line to start
                       ahead of pc and still find a matching one. */
                    /*goto out;*/
                    firstline = 1;
                }
                first = 0;
            }
            if(cur.addr > pc && !firstline)
                break;
            if(b.p == nil){
                werrstr("buffer underflow in line mapping");
                goto out;
            }
            emit = cur;
            if(emit.flags & EndSequence){
                /*
                 * End of sequence reached but PC not found in this sequence.
                 * The line program may have multiple sequences (e.g., .text + INIT).
                 * Reset state and continue to the next sequence.
                 */
                cur = reset;
                emit = reset;
                first = 1;
                continue;
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
                    if(trace) werrstr(" set %p", (PVOID)cur.addr);
                    break;
                case 3:	/* define file */
                    s = dwarfgetstring(&b);
                    if (defined_count < MAX_DEFINED_FILES)
                        defined_files[defined_count++] = s;
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
                if(trace) werrstr(" advance pc + %Iu", (ULONG_PTR)(a * quantum));
                cur.addr += a * quantum;
                break;
            case 3:	/* advance line */
                l = dwarfget128s(&b);
                if(trace) werrstr(" advance line + %lld", (LONGLONG)l);
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
                if(trace) werrstr(" const add pc + %Iu", (ULONG_PTR)a);
                cur.addr += a;
                break;
            case 9:	/* fixed advance pc */
                a = dwarfget2(&b);
                if(trace) werrstr(" fixed advance pc + %Iu", (ULONG_PTR)a);
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

    if (emit.file <= header_file_count) {
        /* Skip over first emit.file-1 file entries */
        b.p = (uchar*)files;
        for(i = 1; i < emit.file && b.p != nil && *b.p != 0; i++){
            dwarfgetstring(&b);
            dwarfget128(&b);
            dwarfget128(&b);
            dwarfget128(&b);
        }
        if(b.p == nil || *b.p == 0){
            werrstr("bad file index in mapping data");
            goto bad;
        }
        s = dwarfgetstring(&b);
        i = dwarfget128(&b);		/* directory */
        dwarfget128(&b);
        dwarfget128(&b);
    } else {
        ULONG idx = emit.file - header_file_count;
        DwarfBuf fb = b;

        if (idx == 0 || idx > defined_count){
            werrstr("bad file index in mapping data");
            goto bad;
        }

        fb.p = (uchar*)defined_files[idx - 1];
        fb.ep = b.ep;
        s = dwarfgetstring(&fb);
        i = dwarfget128(&fb);		/* directory */
        dwarfget128(&fb);
        dwarfget128(&fb);
    }
    *file = s;

    /* fetch dir name */
	cdir = sym.attrs.have.compdir ? sym.attrs.compdir : 0;

	char *dwarfdir;
	dwarfdir = nil;
	/* Directory index 0 means use compilation directory (cdir), not an entry from the table */
	if (i > 0) {
		b.p = dirs;
		for (x = 1; b.p && *b.p; x++) {
			char *d = dwarfgetstring(&b);
			if (x == i) {
				dwarfdir = d;
				break;
			}
		}
	}

	/* Use dwarfdir (from line table) if found, otherwise fall back to cdir */
	if (dwarfdir)
		cdir = dwarfdir;

	if (!cdir)
		cdir = "";
    if (dir)
        *dir = cdir;

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
                    *function = proc->attrs.name;
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
                    *function = proc->attrs.name;
                    goto done;
				}
                lastline = proc->attrs.declline;
			}
            renum = dwarfnextsym(d, proc);
        }
    }

    /* free at last, free at last */
done:
    return 0;
bad:
    werrstr("corrupted line mapping for %p", (PVOID)pc);
out:
    return -1;
}

VOID RosSymFreeInfo(PROSSYM_LINEINFO LineInfo)
{
    int i;
    LineInfo->FileName = NULL;
    LineInfo->DirectoryName = NULL;
    LineInfo->FunctionName = NULL;
    for (i = 0; i < sizeof(LineInfo->Parameters)/sizeof(LineInfo->Parameters[0]); i++)
    {
        LineInfo->Parameters[i].ValueName = NULL;
    }
}
