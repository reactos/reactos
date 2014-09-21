/*
 * Dwarf call frame unwinding.
 *
 * The call frame unwinding values are encoded using a state machine
 * like the pc<->line mapping, but it's a different machine.
 * The expressions to generate the old values are similar in function to the
 * ``dwarf expressions'' used for locations in the code, but of course not
 * the same encoding.
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define trace 1

typedef struct State State;
struct State
{
	ulong loc;
	ulong endloc;
	ulong iquantum;
	ulong dquantum;
	char *augmentation;
	int version;
	ulong rareg;
	DwarfBuf init;
	DwarfExpr *cfa;
	DwarfExpr *ra;
	DwarfExpr *r;
	DwarfExpr *initr;
	int nr;
	DwarfExpr **stack;
	int nstack;
};

static int findfde(Dwarf*, ulong, State*, DwarfBuf*);
static int dexec(DwarfBuf*, State*, int);

int
dwarfunwind(Dwarf *d, ulong pc, DwarfExpr *cfa, DwarfExpr *ra, DwarfExpr *r, int nr)
{
	int i, ret;
	DwarfBuf fde, b;
	DwarfExpr *initr;
	State s;

	initr = mallocz(nr*sizeof(initr[0]), 1);
	if(initr == 0)
		return -1;

	memset(&s, 0, sizeof s);
	s.loc = 0;
	s.cfa = cfa;
	s.ra = ra;
	s.r = r;
	s.nr = nr;

	if(findfde(d, pc, &s, &fde) < 0){
		free(initr);
		return -1;
	}

	memset(r, 0, nr*sizeof(r[0]));
	for(i=0; i<nr; i++)
		r[i].type = RuleSame;
	if(trace) werrstr("s.init %p-%p, fde %p-%p", s.init.p, s.init.ep, fde.p, fde.ep);
	b = s.init;
	if(dexec(&b, &s, 0) < 0)
		goto err;

	s.initr = initr;
	memmove(initr, r, nr*sizeof(initr[0]));

	if(trace) werrstr("s.loc 0x%lx pc 0x%lx", s.loc, pc);
	while(s.loc < pc){
		if(trace) werrstr("s.loc 0x%lx pc 0x%lx", s.loc, pc);
		if(dexec(&fde, &s, 1) < 0)
			goto err;
	}
	*ra = s.r[s.rareg];

	ret = 0;
	goto out;

err:
	ret = -1;
out:
	free(initr);
	for(i=0; i<s.nstack; i++)
		free(s.stack[i]);
	free(s.stack);
	return ret;
}

/*
 * XXX This turns out to be much more expensive than the actual
 * running of the machine in dexec.  It probably makes sense to
 * cache the last 10 or so fde's we've found, since stack traces
 * will keep asking for the same info over and over.
 */
static int
findfde(Dwarf *d, ulong pc, State *s, DwarfBuf *fde)
{
	static int nbad;
	char *aug;
	uchar *next;
	int i, vers;
	ulong len, id, base, size;
	DwarfBuf b;

	if(d->frame.data == nil){
		werrstr("no frame debugging information");
		return -1;
	}
	b.d = d;
	b.p = d->frame.data;
	b.ep = b.p + d->frame.len;
	b.addrsize = d->addrsize;
	if(b.addrsize == 0)
		b.addrsize = 4;	/* where should i find this? */

	for(; b.p < b.ep; b.p = next){
		if((i = (b.p - d->frame.data) % b.addrsize))
			b.p += b.addrsize - i;
		len = dwarfget4(&b);
		if(len > b.ep-b.p){
			werrstr("bad length in cie/fde header");
			return -1;
		}
		next = b.p+len;
		id = dwarfget4(&b);
		if(id == 0xFFFFFFFF){	/* CIE */
			vers = dwarfget1(&b);
			if (trace) werrstr("CIE len %x id %x vers %x", len, id, vers);
			if(vers != 1 && vers != 2 && vers != 3){
				if(++nbad == 1)
					werrstr("unknown cie version %d (wanted 1-3)", vers);
				continue;
			}
			aug = dwarfgetstring(&b);
			if(aug && *aug){
				if(++nbad == 1)
					werrstr("unknown augmentation: %s", aug);
				continue;
			}
			s->iquantum = dwarfget128(&b);
			s->dquantum = dwarfget128s(&b);
			s->rareg = dwarfget128(&b);
			if(s->rareg > s->nr){
				werrstr("return address is register %d but only have %d registers",
					s->rareg, s->nr);
				return -1;
			}
			s->init.p = b.p;
			s->init.ep = next;
		}else{	/* FDE */
			base = dwarfgetaddr(&b);
			size = dwarfgetaddr(&b);
            if (trace) werrstr("FDE: base %x-%x (want pc %x)", base, base+size, pc);
			fde->p = b.p;
			fde->ep = next;
			s->loc = base;
			s->endloc = base+size;
			if(base <= pc && pc < base+size)
				return 0;
		}
	}
	werrstr("cannot find call frame information for pc 0x%lx", pc);
	return -1;

}

static int
checkreg(State *s, long r)
{
	if(r < 0 || r >= s->nr){
		werrstr("bad register number 0x%lx", r);
		return -1;
	}
	return 0;
}

static int
dexec(DwarfBuf *b, State *s, int locstop)
{
	int c;
	long arg1, arg2;
	DwarfExpr *e;

	for(;;){
		if(b->p == b->ep){
			if(s->initr)
				s->loc = s->endloc;
            werrstr("end dexec");
			return 0;
		}
		c = dwarfget1(b);
		if(b->p == nil){
			werrstr("ran out of instructions during cfa program");
			if(trace) werrstr("%r");
			return -1;
		}
		if(trace) werrstr("+ loc=0x%x op 0x%x ", s->loc, c);
		switch(c>>6){
		case 1:	/* advance location */
			arg1 = c&0x3F;
		advance:
			if(trace) werrstr("loc += %ld", arg1*s->iquantum);
			s->loc += arg1 * s->iquantum;
			if(locstop)
				return 0;
			continue;

		case 2:	/* offset rule */
			arg1 = c&0x3F;
			arg2 = dwarfget128(b);
		offset:
			if(trace) werrstr("r%ld += %ld", arg1, arg2*s->dquantum);
			if(checkreg(s, arg1) < 0)
				return -1;
			s->r[arg1].type = RuleCfaOffset;
			s->r[arg1].offset = arg2 * s->dquantum;
			continue;

		case 3:	/* restore initial setting */
			arg1 = c&0x3F;
		restore:
			if(trace) werrstr("r%ld = init", arg1);
			if(checkreg(s, arg1) < 0)
				return -1;
			s->r[arg1] = s->initr[arg1];
			continue;
		}

		switch(c){
		case 0:	/* nop */
			if(trace) werrstr("nop");
			continue;

		case 0x01:	/* set location */
			s->loc = dwarfgetaddr(b);
			if(trace) werrstr("loc = 0x%lx", s->loc);
			if(locstop)
				return 0;
			continue;

		case 0x02:	/* advance loc1 */
			arg1 = dwarfget1(b);
			goto advance;

		case 0x03:	/* advance loc2 */
			arg1 = dwarfget2(b);
			goto advance;

		case 0x04:	/* advance loc4 */
			arg1 = dwarfget4(b);
			goto advance;

		case 0x05:	/* offset extended */
			arg1 = dwarfget128(b);
			arg2 = dwarfget128(b);
			goto offset;

		case 0x06:	/* restore extended */
			arg1 = dwarfget128(b);
			goto restore;

		case 0x07:	/* undefined */
			arg1 = dwarfget128(b);
			if(trace) werrstr("r%ld = undef", arg1);
			if(checkreg(s, arg1) < 0)
				return -1;
			s->r[arg1].type = RuleUndef;
			continue;

		case 0x08:	/* same value */
			arg1 = dwarfget128(b);
			if(trace) werrstr("r%ld = same", arg1);
			if(checkreg(s, arg1) < 0)
				return -1;
			s->r[arg1].type = RuleSame;
			continue;

		case 0x09:	/* register */
			arg1 = dwarfget128(b);
			arg2 = dwarfget128(b);
			if(trace) werrstr("r%ld = r%ld", arg1, arg2);
			if(checkreg(s, arg1) < 0 || checkreg(s, arg2) < 0)
				return -1;
			s->r[arg1].type = RuleRegister;
			s->r[arg1].reg = arg2;
			continue;

		case 0x0A:	/* remember state */
			e = malloc(s->nr*sizeof(e[0]));
			if(trace) werrstr("push");
			if(e == nil)
				return -1;
			void *newstack = malloc(s->nstack*sizeof(s->stack[0]));
			RtlMoveMemory(newstack, s->stack, s->nstack*sizeof(s->stack[0]));
			if (newstack) {
				free(s->stack);
				s->stack = newstack;
			} else {
				free(e);
				return -1;
			}
			if(b->p == nil){
				free(e);
				return -1;
			}
			s->stack[s->nstack++] = e;
			memmove(e, s->r, s->nr*sizeof(e[0]));
			continue;

		case 0x0B:	/* restore state */
			if(trace) werrstr("pop");
			if(s->nstack == 0){
				werrstr("restore state underflow");
				return -1;
			}
			e = s->stack[s->nstack-1];
			memmove(s->r, e, s->nr*sizeof(e[0]));
			free(e);
			s->nstack--;
			continue;

		case 0x0C:	/* def cfa */
			arg1 = dwarfget128(b);
			arg2 = dwarfget128(b);
		defcfa:
			if(trace) werrstr("cfa %ld(r%ld)", arg2, arg1);
			if(checkreg(s, arg1) < 0)
				return -1;
			s->cfa->type = RuleRegOff;
			s->cfa->reg = arg1;
			s->cfa->offset = arg2;
			continue;

		case 0x0D:	/* def cfa register */
			arg1 = dwarfget128(b);
			if(trace) werrstr("cfa reg r%ld", arg1);
			if(s->cfa->type != RuleRegOff){
				werrstr("change CFA register but CFA not in register+offset form");
				return -1;
			}
			if(checkreg(s, arg1) < 0)
				return -1;
			s->cfa->reg = arg1;
			continue;

		case 0x0E:	/* def cfa offset */
			arg1 = dwarfget128(b);
		cfaoffset:
			if(trace) werrstr("cfa off %ld", arg1);
			if(s->cfa->type != RuleRegOff){
				werrstr("change CFA offset but CFA not in register+offset form");
				return -1;
			}
			s->cfa->offset = arg1;
			continue;

		case 0x0F:	/* def cfa expression */
			if(trace) werrstr("cfa expr");
			s->cfa->type = RuleLocation;
			s->cfa->loc.len = dwarfget128(b);
			s->cfa->loc.data = dwarfgetnref(b, s->cfa->loc.len);
			continue;

		case 0x10:	/* def reg expression */
			arg1 = dwarfget128(b);
			if(trace) werrstr("reg expr r%ld", arg1);
			if(checkreg(s, arg1) < 0)
				return -1;
			s->r[arg1].type = RuleLocation;
			s->r[arg1].loc.len = dwarfget128(b);
			s->r[arg1].loc.data = dwarfgetnref(b, s->r[arg1].loc.len);
			continue;

		case 0x11:	/* offset extended */
			arg1 = dwarfget128(b);
			arg2 = dwarfget128s(b);
			goto offset;

		case 0x12:	/* cfa sf */
			arg1 = dwarfget128(b);
			arg2 = dwarfget128s(b);
			goto defcfa;

		case 0x13:	/* cfa offset sf */
			arg1 = dwarfget128s(b);
			goto cfaoffset;

		default:	/* unknown */
			werrstr("unknown opcode 0x%x in cfa program", c);
			return -1;
		}
	}
	/* not reached */
}

int dwarfcomputecfa(Dwarf *d, DwarfExpr *cfa, PROSSYM_REGISTERS registers, ulong *cfaLocation)
{
    switch (cfa->type) {
    case RuleRegOff:
        *cfaLocation = registers->Registers[cfa->reg] + cfa->offset;
        werrstr("cfa reg %d (%x) offset %x = %x", cfa->reg, (ulong)registers->Registers[cfa->reg], cfa->offset, cfaLocation);
        break;
    default:
        werrstr("cfa->type %x", cfa->type);
        return -1;
    }

    return 0;
}

int dwarfregunwind(Dwarf *d, ulong pc, ulong fde, DwarfExpr *cfa, PROSSYM_REGISTERS registers)
{
	int i;
	State s = { };
	DwarfExpr initr[sizeof(registers->Registers) / sizeof(registers->Registers[0])] = { };
	DwarfExpr r[sizeof(registers->Registers) / sizeof(registers->Registers[0])] = { };
    DwarfExpr ra;

	int nr = s.nr = sizeof(registers->Registers) / sizeof(registers->Registers[0]);
	s.initr = initr;
	s.r = r;
	for (i = 0; i < sizeof(r) / sizeof(r[0]); i++) {
		r[i].type = RuleRegister;
		r[i].offset = registers->Registers[i];
		r[i].reg = i;
	}

    int res = dwarfunwind(d, pc, cfa, &ra, initr, sizeof(initr) / sizeof(initr[0]));
    if (res == -1) return -1;

    ulong cfaLocation;

    if (dwarfcomputecfa(d, cfa, registers, &cfaLocation) == -1)
        return -1;

    for (i = 0; i < nr; i++) {
        switch (r[i].type) {
        case RuleUndef:
            werrstr("Undefined register slot %d", i);
            assert(FALSE);
            break;
        case RuleSame:
            break;
        case RuleRegister:
            registers->Registers[i] = registers->Registers[r[i].reg];
            break;
        case RuleRegOff: {
            BOOLEAN success = 
                RosSymCallbacks.MemGetProc
                (d->pe->fd, 
                 &registers->Registers[i],
                 r[i].offset + registers->Registers[r[i].reg],
                 d->addrsize);            
            if (!success) return -1;
        } break;
        case RuleCfaOffset:
        {
            BOOLEAN success = 
                RosSymCallbacks.MemGetProc
                (d->pe->fd, 
                 &registers->Registers[i],
                 r[i].offset + cfaLocation,
                 d->addrsize);
            werrstr("reg[%d] = %x: cfa offset (cfa %x, offset %x) -> @%x", i, (ulong)registers->Registers[i], cfaLocation, initr[i].offset, r[i].offset + cfaLocation);
            if (!success) return -1;
        } break;
        default:
            werrstr("We don't yet support cfa rule %d in slot %d", r[i].type, i);
            assert(FALSE);
            break;
        }
    }

    ulong cfaSpace[4];
    for (i = 0; i < sizeof(cfaSpace) / sizeof(cfaSpace[0]); i++) {
        RosSymCallbacks.MemGetProc
            (d->pe->fd, &cfaSpace[i], cfaLocation + (i * 4), d->addrsize);
    }
    werrstr("CFA(%x) [%08x, %08x, %08x, %08x]", 
            cfaLocation, cfaSpace[0], cfaSpace[1], cfaSpace[2], cfaSpace[3]);

    return 0;
}
