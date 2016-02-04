/*
 * Dwarf info parse and search.
 */

#include <precomp.h>
#define NDEBUG
#include <debug.h>

enum
{
    DwarfAttrSibling = 0x01,
    DwarfAttrLocation = 0x02,
    DwarfAttrName = 0x03,
    DwarfAttrOrdering = 0x09,
    DwarfAttrByteSize = 0x0B,
    DwarfAttrBitOffset = 0x0C,
    DwarfAttrBitSize = 0x0D,
    DwarfAttrStmtList = 0x10,
    DwarfAttrLowpc = 0x11,
    DwarfAttrHighpc = 0x12,
    DwarfAttrLanguage = 0x13,
    DwarfAttrDiscr = 0x15,
    DwarfAttrDiscrValue = 0x16,
    DwarfAttrVisibility = 0x17,
    DwarfAttrImport = 0x18,
    DwarfAttrStringLength = 0x19,
    DwarfAttrCommonRef = 0x1A,
    DwarfAttrCompDir = 0x1B,
    DwarfAttrConstValue = 0x1C,
    DwarfAttrContainingType = 0x1D,
    DwarfAttrDefaultValue = 0x1E,
    DwarfAttrInline = 0x20,
    DwarfAttrIsOptional = 0x21,
    DwarfAttrLowerBound = 0x22,
    DwarfAttrProducer = 0x25,
    DwarfAttrPrototyped = 0x27,
    DwarfAttrReturnAddr = 0x2A,
    DwarfAttrStartScope = 0x2C,
    DwarfAttrStrideSize = 0x2E,
    DwarfAttrUpperBound = 0x2F,
    DwarfAttrAbstractOrigin = 0x31,
    DwarfAttrAccessibility = 0x32,
    DwarfAttrAddrClass = 0x33,
    DwarfAttrArtificial = 0x34,
    DwarfAttrBaseTypes = 0x35,
    DwarfAttrCalling = 0x36,
    DwarfAttrCount = 0x37,
    DwarfAttrDataMemberLoc = 0x38,
    DwarfAttrDeclColumn = 0x39,
    DwarfAttrDeclFile = 0x3A,
    DwarfAttrDeclLine = 0x3B,
    DwarfAttrDeclaration = 0x3C,
    DwarfAttrDiscrList = 0x3D,
    DwarfAttrEncoding = 0x3E,
    DwarfAttrExternal = 0x3F,
    DwarfAttrFrameBase = 0x40,
    DwarfAttrFriend = 0x41,
    DwarfAttrIdentifierCase = 0x42,
    DwarfAttrMacroInfo = 0x43,
    DwarfAttrNamelistItem = 0x44,
    DwarfAttrPriority = 0x45,
    DwarfAttrSegment = 0x46,
    DwarfAttrSpecification = 0x47,
    DwarfAttrStaticLink = 0x48,
    DwarfAttrType = 0x49,
    DwarfAttrUseLocation = 0x4A,
    DwarfAttrVarParam = 0x4B,
    DwarfAttrVirtuality = 0x4C,
    DwarfAttrVtableElemLoc = 0x4D,
    DwarfAttrAllocated = 0x4E,
    DwarfAttrAssociated = 0x4F,
    DwarfAttrDataLocation = 0x50,
    DwarfAttrStride = 0x51,
    DwarfAttrEntrypc = 0x52,
    DwarfAttrUseUTF8 = 0x53,
    DwarfAttrExtension = 0x54,
    DwarfAttrRanges = 0x55,
    DwarfAttrTrampoline = 0x56,
    DwarfAttrCallColumn = 0x57,
    DwarfAttrCallFile = 0x58,
    DwarfAttrCallLine = 0x59,
    DwarfAttrDescription = 0x5A,
    DwarfAttrMax,

    FormAddr = 0x01,
    FormDwarfBlock2 = 0x03,
    FormDwarfBlock4 = 0x04,
    FormData2 = 0x05,
    FormData4 = 0x06,
    FormData8 = 0x07,
    FormString = 0x08,
    FormDwarfBlock = 0x09,
    FormDwarfBlock1 = 0x0A,
    FormData1 = 0x0B,
    FormFlag = 0x0C,
    FormSdata = 0x0D,
    FormStrp = 0x0E,
    FormUdata = 0x0F,
    FormRefAddr = 0x10,
    FormRef1 = 0x11,
    FormRef2 = 0x12,
    FormRef4 = 0x13,
    FormRef8 = 0x14,
    FormRefUdata = 0x15,
    FormIndirect = 0x16
};

static int parseattrs(Dwarf *d, DwarfBuf*, ulong, ulong, DwarfAbbrev*, DwarfAttrs*);
static int getulong(DwarfBuf*, int, ulong, ulong*, int*);
static int getuchar(DwarfBuf*, int, uchar*);
static int getstring(Dwarf *d, DwarfBuf*, int, char**);
static int getblock(DwarfBuf*, int, DwarfBlock*);
static int skipform(Dwarf *d, DwarfBuf*, int);

int
dwarflookupnameinunit(Dwarf *d, ulong unit, char *name, DwarfSym *s)
{
    DwarfSym compunit = { };
    if(dwarfenumunit(d, unit, &compunit) < 0)
        return -1;
    while(dwarfnextsymat(d, &compunit, s) == 0) {
        werrstr("got %s looking for %s\n", s->attrs.name, name);
        if(s->attrs.name && strcmp(s->attrs.name, name) == 0)
            return 0;
    }
    werrstr("symbol '%s' not found", name);
    return -1;
}

int
dwarflookupsubname(Dwarf *d, DwarfSym *parent, char *name, DwarfSym *s)
{
    *s = *parent;
    while(dwarfnextsymat(d, parent, s))
        if(s->attrs.name && strcmp(s->attrs.name, name) == 0)
            return 0;
    werrstr("symbol '%s' not found", name);
    return -1;
}

int
dwarflookupchildtag(Dwarf *d, DwarfSym *parent, ulong tag, DwarfSym *s)
{
    int rsym = dwarfnextsymat(d, parent, s);
    while (rsym == 0 && s->attrs.tag != tag) {
        if (s->attrs.haskids) {
            DwarfSym p = *s;
            int csym = dwarflookupchildtag(d, &p, tag, s);
            if (csym == 0) {
                return csym;
            }
        }
        rsym = dwarfnextsym(d, s);
    }
    return rsym;
}

int
dwarflookuptag(Dwarf *d, ulong unit, ulong tag, DwarfSym *s)
{
    DwarfSym compunit = { };
    if (dwarfenumunit(d, unit, &compunit) < 0) {
        return -1;
    }
    do {
        if (compunit.attrs.tag == tag) {
            *s = compunit;
            return 0;
        }
        if (dwarflookupchildtag(d, &compunit, tag, s) == 0)
            return 0;
    } while(dwarfnextsym(d, &compunit) == 0);
    werrstr("symbol with tag 0x%lux not found", tag);
    return -1;
}

int
dwarfseeksym(Dwarf *d, ulong unit, ulong off, DwarfSym *s)
{
    DwarfSym compunit = { };
    if(dwarfenumunit(d, unit, &compunit) < 0)
        return -1;
    werrstr("dwarfseeksym: unit %x off %x\n", unit, off);
    s->b.d = d;
    s->b.p = d->info.data + unit + off;
    s->b.ep = compunit.b.ep;
    if(dwarfnextsymat(d, &compunit, s) == -1)
        return -1;
    werrstr("dwarfseeksym: unit %x off %x, tag %x", unit, off, s->attrs.tag);
    return 0;
}

int
dwarflookupfn(Dwarf *d, ulong unit, ulong pc, DwarfSym *s)
{
    DwarfSym compunit = { };
    if(dwarfenumunit(d, unit, &compunit) < 0)
        return -1;
    while(dwarfnextsymat(d, &compunit, s) == 0){
        if(s->attrs.tag != TagSubprogram)
            continue;
        if(s->attrs.lowpc <= pc && pc < s->attrs.highpc)
            return 0;
    }
    werrstr("fn containing pc 0x%lux not found", pc);
    return -1;
}

int
dwarfenumunit(Dwarf *d, ulong unit, DwarfSym *s)
{
    int i;
    ulong aoff, len;

    if(unit >= d->info.len){
        werrstr("dwarf unit address 0x%x >= 0x%x out of range", unit, d->info.len);
        return -1;
    }
    memset(s, 0, sizeof *s);
    memset(&s->b, 0, sizeof s->b);

    s->b.d = d;
    s->b.p = d->info.data + unit;
    s->b.ep = d->info.data + d->info.len;
    len = dwarfget4(&s->b);
	s->unit = unit;
    s->nextunit = unit + 4 + len;
    s->b.ep = d->info.data + s->nextunit;

    if(s->b.ep - s->b.p < len){
    badheader:
        werrstr("bad dwarf unit header at unit 0x%lux end %x start %x len %x", unit, s->b.ep - d->info.data, s->b.p - d->info.data, len);
        return -1;
    }
    s->b.ep = s->b.p+len;
    if((i=dwarfget2(&s->b)) > 4)
        goto badheader;
    aoff = dwarfget4(&s->b);
    s->b.addrsize = dwarfget1(&s->b);
    if(d->addrsize == 0)
        d->addrsize = s->b.addrsize;
    if(s->b.p == nil)
        goto badheader;

    s->aoff = aoff;

    return dwarfnextsym(d, s);
}

int
dwarfnextsym(Dwarf *d, DwarfSym *s)
{
    ulong num;
    DwarfAbbrev *a;

    werrstr("sym at %x (left %x)\n", s->b.p - d->info.data, s->b.ep - s->b.p);

    num = dwarfget128(&s->b);
    werrstr("abbrev num %x\n", num);
    s->num = num;
    if(num == 0){
        return -1;
    }

    a = dwarfgetabbrev(d, s->aoff, num);
    werrstr("a %p\n", a);
    if(a == nil){
        werrstr("getabbrev %x %x for %x", s->aoff, num, s->unit);
        return -1;
    }

    if(parseattrs(d, &s->b, s->attrs.tag, s->unit, a, &s->attrs) < 0) {
        return -1;
    }

    if (s->attrs.haskids) {
        DwarfSym childSkip = { };
        s->childoff = s->b.p - d->info.data;
        werrstr("Set childoff at %x\n", s->childoff);
        int r = dwarfnextsymat(d, s, &childSkip);
        while (r == 0) {
            r = dwarfnextsym(d, &childSkip);
        }
        s->b = childSkip.b;
    } else {
        s->childoff = 0;
    }
    return 0;
}

int
dwarfnextsymat(Dwarf *d, DwarfSym *parent, DwarfSym *child)
{
    uint sib;

    if (!parent->attrs.haskids || !parent->childoff)
        return -1;

	child->unit = parent->unit;
    child->aoff = parent->aoff;
    child->depth = parent->depth + 1;
    if(child->attrs.have.sibling){
        sib = child->attrs.sibling;
        if(sib < d->info.len && d->info.data+sib > child->b.p)
            child->b.p = d->info.data+sib;
        else if (sib >= d->info.len) {
            werrstr("sibling reported as out of bounds %d vs %d", sib, d->info.len);
            return -1;
        } else if (d->info.data+sib+parent->unit < child->b.p) {
            werrstr("subsequent sibling is listed before prev %d vs %d", sib+parent->unit, child->b.p - d->info.data);
            return -1;
        }
    }

    // Uninitialized
    if (!child->b.d) {
        child->b = parent->b;
        child->b.p = parent->childoff + parent->b.d->info.data;
        werrstr("Rewound to childoff %x\n", parent->childoff);
    }

    return dwarfnextsym(d, child);
}

typedef struct Parse Parse;
struct Parse {
    const char *namestr;
    int name;
    int off;
    int haveoff;
    int type;
};

#define ATTR(x) (#x)+9, x
#define OFFSET(x) offsetof(DwarfAttrs, x), offsetof(DwarfAttrs, have.x)

static Parse plist[] = {	/* Font Tab 4 */
    { ATTR(DwarfAttrAbstractOrigin),	OFFSET(abstractorigin),		TReference },
    { ATTR(DwarfAttrAccessibility),	OFFSET(accessibility),		TConstant },
    { ATTR(DwarfAttrAddrClass), 		OFFSET(addrclass), 			TConstant },
    { ATTR(DwarfAttrBaseTypes),		OFFSET(basetypes),			TReference },
    { ATTR(DwarfAttrBitOffset),		OFFSET(bitoffset),			TConstant },
    { ATTR(DwarfAttrBitSize),		OFFSET(bitsize),			TConstant },
    { ATTR(DwarfAttrByteSize),		OFFSET(bytesize),			TConstant },
    { ATTR(DwarfAttrCalling),		OFFSET(calling),			TConstant },
    { ATTR(DwarfAttrCommonRef),		OFFSET(commonref),			TReference },
    { ATTR(DwarfAttrCompDir),		OFFSET(compdir),			TString },
    { ATTR(DwarfAttrConstValue),		OFFSET(constvalue),			TString|TConstant|TBlock },
    { ATTR(DwarfAttrContainingType),	OFFSET(containingtype),		TReference },
    { ATTR(DwarfAttrCount),			OFFSET(count),				TConstant|TReference },
    { ATTR(DwarfAttrDataMemberLoc),	OFFSET(datamemberloc),		TBlock|TConstant|TReference },
    { ATTR(DwarfAttrDeclColumn),		OFFSET(declcolumn),			TConstant },
    { ATTR(DwarfAttrDeclFile),		OFFSET(declfile),			TConstant },
    { ATTR(DwarfAttrDeclLine),		OFFSET(declline),			TConstant },
    { ATTR(DwarfAttrDefaultValue),	OFFSET(defaultvalue),		TReference },
    { ATTR(DwarfAttrDiscr),			OFFSET(discr),				TReference },
    { ATTR(DwarfAttrDiscrList),		OFFSET(discrlist),			TBlock },
    { ATTR(DwarfAttrDiscrValue),		OFFSET(discrvalue),			TConstant },
    { ATTR(DwarfAttrEncoding),		OFFSET(encoding),			TConstant },
    { ATTR(DwarfAttrFrameBase),		OFFSET(framebase),			TBlock|TConstant },
    { ATTR(DwarfAttrFriend),			OFFSET(friend),				TReference },
    { ATTR(DwarfAttrHighpc),			OFFSET(highpc),				TAddress },
    { ATTR(DwarfAttrEntrypc),         OFFSET(entrypc),            TAddress },
    { ATTR(DwarfAttrIdentifierCase),	OFFSET(identifiercase),		TConstant },
    { ATTR(DwarfAttrImport),			OFFSET(import),				TReference },
    { ATTR(DwarfAttrInline),			OFFSET(inlined),			TConstant },
    { ATTR(DwarfAttrArtificial),		OFFSET(isartificial), 		TFlag },
    { ATTR(DwarfAttrDeclaration),	    OFFSET(isdeclaration),		TFlag },
    { ATTR(DwarfAttrExternal),		OFFSET(isexternal),			TFlag },
    { ATTR(DwarfAttrIsOptional),		OFFSET(isoptional),			TFlag },
    { ATTR(DwarfAttrPrototyped),		OFFSET(isprototyped),		TFlag },
    { ATTR(DwarfAttrVarParam),		OFFSET(isvarparam),			TFlag },
    { ATTR(DwarfAttrLanguage),		OFFSET(language),			TConstant },
    { ATTR(DwarfAttrLocation),		OFFSET(location),			TReference|TBlock },
    { ATTR(DwarfAttrLowerBound),		OFFSET(lowerbound),			TConstant|TReference },
    { ATTR(DwarfAttrLowpc),			OFFSET(lowpc),				TAddress },
    { ATTR(DwarfAttrMacroInfo),		OFFSET(macroinfo),			TConstant },
    { ATTR(DwarfAttrName),			OFFSET(name),				TString },
    { ATTR(DwarfAttrNamelistItem),	OFFSET(namelistitem),		TBlock },
    { ATTR(DwarfAttrOrdering), 		OFFSET(ordering),			TConstant },
    { ATTR(DwarfAttrPriority),		OFFSET(priority),			TReference },
    { ATTR(DwarfAttrProducer),		OFFSET(producer),			TString },
    { ATTR(DwarfAttrRanges),			OFFSET(ranges),				TReference },
    { ATTR(DwarfAttrReturnAddr),		OFFSET(returnaddr),			TBlock|TConstant },
    { ATTR(DwarfAttrSegment),		OFFSET(segment),			TBlock|TConstant },
    { ATTR(DwarfAttrSibling),		OFFSET(sibling),			TReference },
    { ATTR(DwarfAttrSpecification),	OFFSET(specification),		TReference },
    { ATTR(DwarfAttrStartScope),		OFFSET(startscope),			TConstant },
    { ATTR(DwarfAttrStaticLink),		OFFSET(staticlink),			TBlock|TConstant },
    { ATTR(DwarfAttrStmtList),		OFFSET(stmtlist),			TConstant },
    { ATTR(DwarfAttrStrideSize),		OFFSET(stridesize),			TConstant },
    { ATTR(DwarfAttrStringLength),	OFFSET(stringlength),		TBlock|TConstant },
    { ATTR(DwarfAttrType),			OFFSET(type),				TReference },
    { ATTR(DwarfAttrUpperBound),		OFFSET(upperbound),			TConstant|TReference },
    { ATTR(DwarfAttrUseLocation),	OFFSET(uselocation),		TBlock|TConstant },
    { ATTR(DwarfAttrVirtuality),		OFFSET(virtuality),			TConstant },
    { ATTR(DwarfAttrVisibility),		OFFSET(visibility),			TConstant },
    { ATTR(DwarfAttrVtableElemLoc),	OFFSET(vtableelemloc),		TBlock|TReference },
    { }
};

static Parse ptab[DwarfAttrMax];

static int
parseattrs(Dwarf *d, DwarfBuf *b, ulong tag, ulong unit, DwarfAbbrev *a, DwarfAttrs *attrs)
{
    int i, f, n, got;
    static int nbad;
    void *v;

    /* initialize ptab first time through for quick access */
    if(ptab[DwarfAttrName].name != DwarfAttrName)
        for(i=0; plist[i].name; i++)
            ptab[plist[i].name] = plist[i];

    memset(attrs, 0, sizeof *attrs);
    attrs->tag = a->tag;
    attrs->haskids = a->haskids;

    for(i=0; i<a->nattr; i++){
        n = a->attr[i].name;
        f = a->attr[i].form;
        werrstr("struct: (@%x) n %x f %x (%d %d)\n", b->p - d->info.data, n, f, ptab[n].haveoff, ptab[n].off);
        if(n < 0 || n >= DwarfAttrMax || ptab[n].name==0) {
            if (skipform(d, b, f) < 0) {
                if(++nbad == 1)
                    werrstr("dwarf parse attrs: cannot skip form %d", f);
                return -1;
            }
            continue;
        }
        v = (char*)attrs + ptab[n].off;
        got = 0;
        if(f == FormIndirect)
            f = dwarfget128(b);
        if((ptab[n].type&(TConstant|TReference|TAddress))
           && getulong(b, f, unit, v, &got) >= 0)
            ;
        else if((ptab[n].type&TFlag) && getuchar(b, f, v) >= 0)
            got = TFlag;
        else if((ptab[n].type&TString) && getstring(d, b, f, v) >= 0)
            got = TString;
        else if((ptab[n].type&TBlock) && getblock(b, f, v) >= 0) {
            got = TBlock;
        } else {
            werrstr("Skipping form %x\n", f);
            if(skipform(d, b, f) < 0){
                //if(++nbad == 1)
                    werrstr("dwarf parse attrs: cannot skip form %d", f);
                return -1;
            }
        }
#if 0
        if(got == TBlock && (ptab[n].type&TConstant))
            got = constblock(b->d, v, v);
#endif
        *((uchar*)attrs+ptab[n].haveoff) = got;
    }

    if (attrs->have.name)
        werrstr("%s: tag %x kids %d (last %x)\n", attrs->name, attrs->tag, attrs->haskids, b->p - b->d->info.data);

    return 0;
}

static int
getulong(DwarfBuf *b, int form, ulong unit, ulong *u, int *type)
{
    static int nbad;
    uvlong uv;

    switch(form){
    default:
        return -1;

	/* addresses */
    case FormAddr:
        *type = TAddress;
        *u = dwarfgetaddr(b);
        return 0;

	/* references */
    case FormRefAddr:
        /* absolute ref in .debug_info */
        *type = TReference;
        *u = dwarfgetaddr(b);
        return 0;
    case FormRef1:
        *u = dwarfget1(b);
        goto relativeref;
    case FormRef2:
        *u = dwarfget2(b);
        goto relativeref;
    case FormRef4:
        *u = dwarfget4(b);
        goto relativeref;
    case FormRef8:
        *u = dwarfget8(b);
        goto relativeref;
    case FormRefUdata:
        *u = dwarfget128(b);
    relativeref:
        *u += unit;
        *type = TReference;
        return 0;

	/* constants */
    case FormData1:
        *u = dwarfget1(b);
        goto constant;
    case FormData2:
        *u = dwarfget2(b);
        goto constant;
    case FormData4:
        *u = dwarfget4(b);
        goto constant;
    case FormData8:
        uv = dwarfget8(b);
        *u = uv;
        if(uv != *u && ++nbad == 1)
            werrstr("dwarf: truncating 64-bit attribute constants");
        goto constant;
    case FormSdata:
        *u = dwarfget128s(b);
        goto constant;
    case FormUdata:
        *u = dwarfget128(b);
    constant:
        *type = TConstant;
        return 0;
    }
}

static int
getuchar(DwarfBuf *b, int form, uchar *u)
{
    switch(form){
    default:
        return -1;

    case FormFlag:
        *u = dwarfget1(b);
        return 0;
    }
}

static int
getstring(Dwarf *d, DwarfBuf *b, int form, char **s)
{
    static int nbad;
    ulong u, x;

    switch(form){
    default:
        return -1;

    case FormString:
        x = b->p - d->info.data;
        *s = dwarfgetstring(b);
        for (u = 0; (*s)[u]; u++) {
            assert(isprint((*s)[u]));
        }
        return 0;

    case FormStrp:
        u = dwarfget4(b);
        if(u >= b->d->str.len){
            if(++nbad == 1)
                werrstr("dwarf: bad string pointer 0x%lux in attribute", u);
            /* don't return error - maybe can proceed */
            *s = nil;
        }else
            *s = (char*)b->d->str.data + u;
        return 0;

    }
}

static int
getblock(DwarfBuf *b, int form, DwarfBlock *bl)
{
    ulong n;

    switch(form){
    default:
        return -1;
    case FormDwarfBlock:
        n = dwarfget128(b);
        goto copyn;
    case FormDwarfBlock1:
        n = dwarfget1(b);
        goto copyn;
    case FormDwarfBlock2:
        n = dwarfget2(b);
        goto copyn;
    case FormDwarfBlock4:
        n = dwarfget4(b);
    copyn:
        bl->data = dwarfgetnref(b, n);
        bl->len = n;
        if(bl->data == nil)
            return -1;
        return 0;
    }
}

/* last resort */
static int
skipform(Dwarf *d, DwarfBuf *b, int form)
{
    int type;
    DwarfVal val;

    if(getulong(b, form, 0, &val.c, &type) < 0
       && getuchar(b, form, (uchar*)&val) < 0
       && getstring(d, b, form, &val.s) < 0
       && getblock(b, form, &val.b) < 0)
        return -1;
    return 0;
}

void stackinit(DwarfStack *stack)
{
    memset(stack, 0, sizeof(*stack));
    stack->data = stack->storage;
    stack->length = 0; stack->max = sizeof(stack->storage) / sizeof(stack->storage[0]);
}

void stackpush(DwarfStack *stack, ulong value)
{
    if (stack->length == stack->max) {
        ulong *newstack = malloc(sizeof(ulong)*stack->max*2);
        memcpy(newstack, stack->data, sizeof(ulong)*stack->length);
        if (stack->data != stack->storage)
            free(stack->data);
        stack->data = newstack;
        stack->max *= 2;
    }
    werrstr("stack[%d] = %x", stack->length, value);
    stack->data[stack->length++] = value;
}

ulong stackpop(DwarfStack *stack) 
{
    ASSERT(stack->length > 0);
    ulong val = stack->data[--stack->length];
    werrstr("pop stack[%d] -> %x", stack->length, val);
    return val;
}

void stackfree(DwarfStack *stack)
{
    if (stack->data != stack->storage)
        free(stack->data);
}

// Returns -1 on failure
int dwarfgetarg(Dwarf *d, const char *name, DwarfBuf *buf, ulong cfa, PROSSYM_REGISTERS registers, ulong *result)
{
    int ret = 0;
    DwarfStack stack = { };
    stackinit(&stack);
    stackpush(&stack, cfa);
    while (buf->p < buf->ep) {
        int opcode = dwarfget1(buf);
        werrstr("opcode %x", opcode);
        switch (opcode) {
        case 0:
            buf->p = buf->ep;
            break;
        case OpAddr:
            if (d->addrsize == 4) {
                stackpush(&stack, dwarfget4(buf));
                break;
            } else {
                werrstr("%s: we only support 4 byte addrs", name);
                goto fatal;
            }
        case OpConst1s: {
            signed char c = dwarfget1(buf);
            stackpush(&stack, c);
        } break;
        case OpConst1u:
            stackpush(&stack, dwarfget1(buf));
            break;
        case OpConst2s: {
            signed short s = dwarfget2(buf);
            stackpush(&stack, s);
        } break;
        case OpConst2u:
            stackpush(&stack, dwarfget2(buf));
            break;
        case OpConst4s: {
            signed int i = dwarfget4(buf);
            stackpush(&stack, i);
        } break;
        case OpConst4u:
            stackpush(&stack, dwarfget4(buf));
            break;
        case OpConst8s:
        case OpConst8u:
            werrstr("const 8 not yet supported");
            goto fatal;
        case OpConsts:
            stackpush(&stack, dwarfget128s(buf));
            break;
        case OpConstu:
            stackpush(&stack, dwarfget128(buf));
            break;
        case OpDup: {
            ulong popped = stackpop(&stack);
            stackpush(&stack, popped);
            stackpush(&stack, popped);
        } break;
        case OpDrop:
            stackpop(&stack);
            break;
        case OpOver: {
            if (stack.length < 2) goto fatal;
            stackpush(&stack, stack.data[stack.length-2]);
        } break;
        case OpPick: {
            ulong arg = dwarfget1(buf);
            if (arg >= stack.length) goto fatal;
            arg = stack.data[stack.length-1-arg];
            stackpush(&stack, arg);
        } break;
        case OpSwap: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b);
            stackpush(&stack, a);
        } break;
        case OpRot: {
            ulong a = stackpop(&stack), b = stackpop(&stack), c = stackpop(&stack);
            stackpush(&stack, b);
            stackpush(&stack, c);
            stackpush(&stack, a);
        } break;
        case OpXderef:
        case OpXderefSize:
            werrstr("Xderef not yet supported");
            goto fatal;
        case OpAbs: {
            long a = stackpop(&stack);
            stackpush(&stack, a < 0 ? -a : a);
        } break;
        case OpAnd:
            stackpush(&stack, stackpop(&stack) & stackpop(&stack));
            break;
        case OpDiv: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b / a);
        } break;
        case OpMinus: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b - a);
        } break;
        case OpMod: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b % a);
        } break;
        case OpMul:
            stackpush(&stack, stackpop(&stack) * stackpop(&stack));
            break;
        case OpNeg:
            stackpush(&stack, -stackpop(&stack));
            break;
        case OpNot:
            stackpush(&stack, ~stackpop(&stack));
            break;
        case OpOr:
            stackpush(&stack, stackpop(&stack) | stackpop(&stack));
            break;
        case OpPlus:
            stackpush(&stack, stackpop(&stack) + stackpop(&stack));
            break;
        case OpPlusUconst:
            stackpush(&stack, stackpop(&stack) + dwarfget128(buf));
            break;
        case OpShl: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b << a);
        } break;
        case OpShr: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b >> a);
        } break;
        case OpShra: {
            ulong a = stackpop(&stack);
            long b = stackpop(&stack);
            if (b < 0)
                b = -(-b >> a);
            else
                b = b >> a;
            stackpush(&stack, b);
        } break;
        case OpXor:
            stackpush(&stack, stackpop(&stack) ^ stackpop(&stack));
            break;
        case OpSkip:
            buf->p += dwarfget2(buf);
            break;
        case OpBra: {
            ulong a = dwarfget2(buf);
            if (stackpop(&stack))
                buf->p += a;
        } break;
        case OpEq:
            stackpush(&stack, stackpop(&stack) == stackpop(&stack));
            break;
        case OpGe: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b >= a);
        } break;
        case OpGt: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b > a);
        } break;
        case OpLe: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b <= a);
        } break;
        case OpLt: {
            ulong a = stackpop(&stack), b = stackpop(&stack);
            stackpush(&stack, b < a);
        } break;
        case OpNe:
            stackpush(&stack, stackpop(&stack) != stackpop(&stack));
            break;
        case OpNop:
            break;
        case OpDeref: {
            ulong val;
            void* addr = (void*)stackpop(&stack);
            if (!RosSymCallbacks.MemGetProc
                (d->pe->fd,
                 &val,
                 addr,
                 d->addrsize))
                goto fatal;
            stackpush(&stack, val);
        } break;
        case OpDerefSize: {
            ulong val, size = dwarfget1(buf);
            void* addr = (void*)stackpop(&stack);
            if (!RosSymCallbacks.MemGetProc
                (d->pe->fd,
                 &val,
                 addr,
                 size))
                goto fatal;
            stackpush(&stack, val);
        } break;
        case OpFbreg: {
            ulong val, offset = dwarfget128s(buf);
            void* addr = (void*)cfa;
            werrstr("FBREG cfa %x offset %x", cfa, offset);
            if (!RosSymCallbacks.MemGetProc
                (d->pe->fd,
                 &val,
                 (PVOID)((ULONG_PTR)addr+offset),
                 d->addrsize))
                goto fatal;
            stackpush(&stack, val);
        } break;
        case OpPiece:
            werrstr("OpPiece not supported");
            goto fatal;
        default:
            if (opcode >= OpLit0 && opcode < OpReg0)
                stackpush(&stack, opcode - OpLit0);
            else if (opcode >= OpReg0 && opcode < OpBreg0) {
                ulong reg = opcode - OpReg0;
                werrstr("REG[%d] value %x", reg, (ulong)registers->Registers[reg]);
                stackpush(&stack, registers->Registers[reg]);
            } else if (opcode >= OpBreg0 && opcode < OpRegx) {
                ulong val, 
                    reg = opcode - OpBreg0, 
                    offset = dwarfget128s(buf);
                void* addr = (void*)(ULONG_PTR)registers->Registers[reg];
                werrstr("BREG[%d] reg %x offset %x", reg, addr, offset);
                if (!RosSymCallbacks.MemGetProc
                    ((PVOID)d->pe->fd,
                     &val,
                     (PVOID)((ULONG_PTR)addr + offset),
                     d->addrsize))
                    goto fatal;
                stackpush(&stack, val);
            } else {
                werrstr("opcode %x not supported", opcode);
                goto fatal;
            }
            break;
        }
    }
    if (stack.length < 1) goto fatal;
    *result = stackpop(&stack);
    werrstr("%s: value %x", name, *result);
    goto finish;

fatal:
    ret = -1;

finish:
    stackfree(&stack);
    return ret;
}

int dwarfargvalue(Dwarf *d, DwarfSym *proc, ulong pc, ulong cfa, PROSSYM_REGISTERS registers, DwarfParam *parameter)
{
    int gotarg;
    DwarfSym unit = { };

    if (dwarfenumunit(d, proc->unit, &unit) == -1)
        return -1;

    werrstr("lookup in unit %x-%x, pc %x", unit.attrs.lowpc, unit.attrs.highpc, pc);
    pc -= unit.attrs.lowpc;
    
    werrstr("paramblock %s -> unit %x type %x fde %x len %d registers %x", 
            parameter->name, 
            parameter->unit, 
            parameter->type, 
            parameter->fde, 
            parameter->len, 
            registers);

    // Seek our range in loc
    DwarfBuf locbuf;
    DwarfBuf instream = { };

    locbuf.d = d;
    locbuf.addrsize = d->addrsize;
    
    if (parameter->loctype == TConstant) {
        locbuf.p = d->loc.data + parameter->fde;
        locbuf.ep = d->loc.data + d->loc.len;
        ulong start, end, len;
        do {
            len = 0;
            start = dwarfget4(&locbuf);
            end = dwarfget4(&locbuf);
            if (start && end) {
                len = dwarfget2(&locbuf);
                instream = locbuf;
                instream.ep = instream.p + len;
                locbuf.p = instream.ep;
            }
            werrstr("ip %x s %x e %x (%x bytes)", pc, start, end, len);
        } while (start && end && (start > pc || end <= pc));
    } else if (parameter->loctype == TBlock) {
        instream = locbuf;
        instream.p = (void *)parameter->fde;
        instream.ep = instream.p + parameter->len;
    } else {
        werrstr("Wrong block type for parameter %s", parameter->name);
        return -1;
    }

    gotarg = dwarfgetarg(d, parameter->name, &instream, cfa, registers, &parameter->value);
    if (gotarg == -1)
        return -1;
    
    return 0;
}

void
dwarfdumpsym(Dwarf *d, DwarfSym *s)
{
    int j;
    werrstr("tag %x\n", s->attrs.tag);
    for (j = 0; plist[j].name; j++) {
        char *have = ((char*)&s->attrs) + plist[j].haveoff;
        char *attr = ((char*)&s->attrs) + plist[j].off;
        if (*have == TString) {
            char *str = *((char **)attr);
            werrstr("%s: %s\n", plist[j].namestr, str);
        } else if (*have == TReference) {
            DwarfVal *val = ((DwarfVal*)attr);
            werrstr("%s: %x:%x\n", plist[j].namestr, val->b.data, val->b.len);
        } else if (*have)
            werrstr("%s: (%x)\n", plist[j].namestr, *have);
    }
}

int
dwarfgetparams(Dwarf *d, DwarfSym *s, ulong pc, int pnum, DwarfParam *paramblocks)
{
	int ip = 0;
	DwarfSym param = { };
	int res = dwarfnextsymat(d, s, &param);
	while (res == 0 && ip < pnum) {
		if (param.attrs.tag == TagFormalParameter &&
			param.attrs.have.name && 
			param.attrs.have.location) {
			paramblocks[ip].name = malloc(strlen(param.attrs.name)+1);
			strcpy(paramblocks[ip].name, param.attrs.name);
			paramblocks[ip].unit = param.unit;
			paramblocks[ip].type = param.attrs.type;
            paramblocks[ip].loctype = param.attrs.have.location;
            paramblocks[ip].len = param.attrs.location.b.len;
			paramblocks[ip].fde = (ulong)param.attrs.location.b.data;
            werrstr("param[%d] block %s -> type %x loctype %x fde %x len %x", 
                   ip, 
                   paramblocks[ip].name, 
                   paramblocks[ip].type,
                   paramblocks[ip].loctype, 
                   paramblocks[ip].fde,
                   paramblocks[ip].len);
            ip++;
		}
		res = dwarfnextsymat(d, s, &param);
	}
	return ip;
}
