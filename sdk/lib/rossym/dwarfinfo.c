/*
 * Dwarf info parse and search.
 */

#define NTOSAPI
#include <ntifs.h>
#include <ndk/ntndk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"
#include <windef.h>

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

static int parseattrs(DwarfBuf*, ulong, DwarfAbbrev*, DwarfAttrs*);
static int getulong(DwarfBuf*, int, ulong, ulong*, int*);
static int getuchar(DwarfBuf*, int, uchar*);
static int getstring(DwarfBuf*, int, char**);
static int getblock(DwarfBuf*, int, DwarfBlock*);
static int skipform(DwarfBuf*, int);
static int constblock(Dwarf*, DwarfBlock*, ulong*);

int
dwarflookupnameinunit(Dwarf *d, ulong unit, char *name, DwarfSym *s)
{
	if(dwarfenumunit(d, unit, s) < 0)
		return -1;

	dwarfnextsymat(d, s, 0);	/* s is now the CompileUnit */
	while(dwarfnextsymat(d, s, 1) == 1)
		if(s->attrs.name && strcmp(s->attrs.name, name) == 0)
			return 0;
	werrstr("symbol '%s' not found", name);
	return -1;
}


int
dwarflookupsubname(Dwarf *d, DwarfSym *parent, char *name, DwarfSym *s)
{
	*s = *parent;
	while(dwarfnextsymat(d, s, parent->depth+1))
		if(s->attrs.name && strcmp(s->attrs.name, name) == 0)
			return 0;
	werrstr("symbol '%s' not found", name);
	return -1;
}

int
dwarflookuptag(Dwarf *d, ulong unit, ulong tag, DwarfSym *s)
{
	if(dwarfenumunit(d, unit, s) < 0) {
		return -1;
	}

	dwarfnextsymat(d, s, 0);	/* s is now the CompileUnit */
	if(s->attrs.tag == tag) {
		return 0;
	}
	while(dwarfnextsymat(d, s, 1) == 1)
		if(s->attrs.tag == tag) {
			return 0;
		}
	werrstr("symbol with tag 0x%lux not found", tag);
	return -1;
}

int
dwarfseeksym(Dwarf *d, ulong unit, ulong off, DwarfSym *s)
{
	if(dwarfenumunit(d, unit, s) < 0)
		return -1;
	s->b.p = d->info.data + unit + off;
	if(dwarfnextsymat(d, s, 0) != 1)
		return -1;
	return 0;
}

int
dwarflookupfn(Dwarf *d, ulong unit, ulong pc, DwarfSym *s)
{
	if(dwarfenumunit(d, unit, s) < 0)
		return -1;

	if(dwarfnextsymat(d, s, 0) != 1)
		return -1;
	/* s is now the CompileUnit */

	while(dwarfnextsymat(d, s, 1) == 1){
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
	s->nextunit = unit + 4 + len;

	if(s->b.ep - s->b.p < len){
	badheader:
		werrstr("bad dwarf unit header at unit 0x%lux", unit);
		return -1;
	}
	s->b.ep = s->b.p+len;
	if((i=dwarfget2(&s->b)) != 2)
		goto badheader;
	aoff = dwarfget4(&s->b);
	s->b.addrsize = dwarfget1(&s->b);
	if(d->addrsize == 0)
		d->addrsize = s->b.addrsize;
	if(s->b.p == nil)
		goto badheader;

	s->aoff = aoff;
	s->unit = unit;
	s->depth = 0;
	return 0;
}

int
dwarfenum(Dwarf *d, DwarfSym *s)
{
	if(dwarfenumunit(d, 0, s) < 0)
		return -1;
	s->allunits = 1;
	return 0;
}

int
dwarfnextsym(Dwarf *d, DwarfSym *s)
{
	ulong num;
	DwarfAbbrev *a;

	if(s->attrs.haskids)
		s->depth++;
top:
	if(s->b.p >= s->b.ep){
		if(s->allunits && s->nextunit < d->info.len){
			if(dwarfenumunit(d, s->nextunit, s) < 0) {
				return -1;
			}
			s->allunits = 1;
			goto top;
		}
		return 0;
	}

	s->uoff = s->b.p - (d->info.data+s->unit);
	num = dwarfget128(&s->b);
	if(num == 0){
		if(s->depth == 0) {
			return 0;
		}
		if(s->depth > 0)
			s->depth--;
		goto top;
	}

	a = dwarfgetabbrev(d, s->aoff, num);
	if(a == nil){
		werrstr("getabbrev %ud %ud for %ud,%ud: %r\n", s->aoff, num, s->unit, s->uoff);
		return -1;
	}
	if(parseattrs(&s->b, s->unit, a, &s->attrs) < 0) {
		return -1;
	}
	return 1;
}

int
dwarfnextsymat(Dwarf *d, DwarfSym *s, int depth)
{
	int r;
	DwarfSym t;
	uint sib;

	if(s->depth == depth && s->attrs.have.sibling){
		sib = s->attrs.sibling;
		if(sib < d->info.len && d->info.data+sib >= s->b.p)
			s->b.p = d->info.data+sib;
		s->attrs.haskids = 0;
	}

	/*
	 * The funny game with t and s make sure that
	 * if we get to the end of a run of a particular
	 * depth, we leave s so that a call to nextsymat with depth-1
	 * will actually produce the desired guy.  We could change
	 * the interface to dwarfnextsym instead, but I'm scared
	 * to touch it.
	 */
	t = *s;
	for(;;){
		if((r = dwarfnextsym(d, &t)) != 1) {
			return r;
		}
		if(t.depth < depth){
			/* went too far - nothing to see */
			return 0;
		}
		*s = t;
		if(t.depth == depth) {
			return 1;
		}
	}
}

typedef struct Parse Parse;
struct Parse {
	int name;
	int off;
	int haveoff;
	int type;
};

#define OFFSET(x) offsetof(DwarfAttrs, x), offsetof(DwarfAttrs, have.x)

static Parse plist[] = {	/* Font Tab 4 */
	{ DwarfAttrAbstractOrigin,	OFFSET(abstractorigin),		TReference },
	{ DwarfAttrAccessibility,	OFFSET(accessibility),		TConstant },
	{ DwarfAttrAddrClass, 		OFFSET(addrclass), 			TConstant },
	{ DwarfAttrArtificial,		OFFSET(isartificial), 		TFlag },
	{ DwarfAttrBaseTypes,		OFFSET(basetypes),			TReference },
	{ DwarfAttrBitOffset,		OFFSET(bitoffset),			TConstant },
	{ DwarfAttrBitSize,		OFFSET(bitsize),			TConstant },
	{ DwarfAttrByteSize,		OFFSET(bytesize),			TConstant },
	{ DwarfAttrCalling,		OFFSET(calling),			TConstant },
	{ DwarfAttrCommonRef,		OFFSET(commonref),			TReference },
	{ DwarfAttrCompDir,		OFFSET(compdir),			TString },
	{ DwarfAttrConstValue,		OFFSET(constvalue),			TString|TConstant|TBlock },
	{ DwarfAttrContainingType,	OFFSET(containingtype),		TReference },
	{ DwarfAttrCount,			OFFSET(count),				TConstant|TReference },
	{ DwarfAttrDataMemberLoc,	OFFSET(datamemberloc),		TBlock|TConstant|TReference },
	{ DwarfAttrDeclColumn,		OFFSET(declcolumn),			TConstant },
	{ DwarfAttrDeclFile,		OFFSET(declfile),			TConstant },
	{ DwarfAttrDeclLine,		OFFSET(declline),			TConstant },
	{ DwarfAttrDeclaration,	OFFSET(isdeclaration),		TFlag },
	{ DwarfAttrDefaultValue,	OFFSET(defaultvalue),		TReference },
	{ DwarfAttrDiscr,			OFFSET(discr),				TReference },
	{ DwarfAttrDiscrList,		OFFSET(discrlist),			TBlock },
	{ DwarfAttrDiscrValue,		OFFSET(discrvalue),			TConstant },
	{ DwarfAttrEncoding,		OFFSET(encoding),			TConstant },
	{ DwarfAttrExternal,		OFFSET(isexternal),			TFlag },
	{ DwarfAttrFrameBase,		OFFSET(framebase),			TBlock|TConstant },
	{ DwarfAttrFriend,			OFFSET(friend),				TReference },
	{ DwarfAttrHighpc,			OFFSET(highpc),				TAddress },
	{ DwarfAttrEntrypc,         OFFSET(entrypc),            TAddress },
	{ DwarfAttrIdentifierCase,	OFFSET(identifiercase),		TConstant },
	{ DwarfAttrImport,			OFFSET(import),				TReference },
	{ DwarfAttrInline,			OFFSET(inlined),			TConstant },
	{ DwarfAttrIsOptional,		OFFSET(isoptional),			TFlag },
	{ DwarfAttrLanguage,		OFFSET(language),			TConstant },
	{ DwarfAttrLocation,		OFFSET(location),			TBlock|TConstant },
	{ DwarfAttrLowerBound,		OFFSET(lowerbound),			TConstant|TReference },
	{ DwarfAttrLowpc,			OFFSET(lowpc),				TAddress },
	{ DwarfAttrMacroInfo,		OFFSET(macroinfo),			TConstant },
	{ DwarfAttrName,			OFFSET(name),				TString },
	{ DwarfAttrNamelistItem,	OFFSET(namelistitem),		TBlock },
	{ DwarfAttrOrdering, 		OFFSET(ordering),			TConstant },
	{ DwarfAttrPriority,		OFFSET(priority),			TReference },
	{ DwarfAttrProducer,		OFFSET(producer),			TString },
	{ DwarfAttrPrototyped,		OFFSET(isprototyped),		TFlag },
	{ DwarfAttrRanges,			OFFSET(ranges),				TReference },
	{ DwarfAttrReturnAddr,		OFFSET(returnaddr),			TBlock|TConstant },
	{ DwarfAttrSegment,		OFFSET(segment),			TBlock|TConstant },
	{ DwarfAttrSibling,		OFFSET(sibling),			TReference },
	{ DwarfAttrSpecification,	OFFSET(specification),		TReference },
	{ DwarfAttrStartScope,		OFFSET(startscope),			TConstant },
	{ DwarfAttrStaticLink,		OFFSET(staticlink),			TBlock|TConstant },
	{ DwarfAttrStmtList,		OFFSET(stmtlist),			TConstant },
	{ DwarfAttrStrideSize,		OFFSET(stridesize),			TConstant },
	{ DwarfAttrStringLength,	OFFSET(stringlength),		TBlock|TConstant },
	{ DwarfAttrType,			OFFSET(type),				TReference },
	{ DwarfAttrUpperBound,		OFFSET(upperbound),			TConstant|TReference },
	{ DwarfAttrUseLocation,	OFFSET(uselocation),		TBlock|TConstant },
	{ DwarfAttrVarParam,		OFFSET(isvarparam),			TFlag },
	{ DwarfAttrVirtuality,		OFFSET(virtuality),			TConstant },
	{ DwarfAttrVisibility,		OFFSET(visibility),			TConstant },
	{ DwarfAttrVtableElemLoc,	OFFSET(vtableelemloc),		TBlock|TReference },
	{ }
};

static Parse ptab[DwarfAttrMax];

static int
parseattrs(DwarfBuf *b, ulong unit, DwarfAbbrev *a, DwarfAttrs *attrs)
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
		if(n < 0 || n >= DwarfAttrMax || ptab[n].name==0){
			if(++nbad == 1)
				werrstr("dwarf parse attrs: unexpected attribute name 0x%x", n);
			continue; //return -1;
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
		else if((ptab[n].type&TString) && getstring(b, f, v) >= 0)
			got = TString;
		else if((ptab[n].type&TBlock) && getblock(b, f, v) >= 0)
			got = TBlock;
		else{
			if(skipform(b, f) < 0){
				if(++nbad == 1)
					werrstr("dwarf parse attrs: cannot skip form %d", f);
				return -1;
			}
		}
		if(got == TBlock && (ptab[n].type&TConstant))
			got = constblock(b->d, v, v);
		*((uchar*)attrs+ptab[n].haveoff) = got;
	}
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
getstring(DwarfBuf *b, int form, char **s)
{
	static int nbad;
	ulong u;

	switch(form){
	default:
		return -1;

	case FormString:
		*s = dwarfgetstring(b);
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

static int
constblock(Dwarf *d, DwarfBlock *bl, ulong *pval)
{
	DwarfBuf b;

	memset(&b, 0, sizeof b);
	b.p = bl->data;
	b.ep = bl->data+bl->len;
	b.d = d;

	switch(dwarfget1(&b)){
	case OpAddr:
		*pval = dwarfgetaddr(&b);
		return TConstant;
	case OpConst1u:
		*pval = dwarfget1(&b);
		return TConstant;
	case OpConst1s:
		*pval = (schar)dwarfget1(&b);
		return TConstant;
	case OpConst2u:
		*pval = dwarfget2(&b);
		return TConstant;
	case OpConst2s:
		*pval = (s16int)dwarfget2(&b);
		return TConstant;
	case OpConst4u:
		*pval = dwarfget4(&b);
		return TConstant;
	case OpConst4s:
		*pval = (s32int)dwarfget4(&b);
		return TConstant;
	case OpConst8u:
		*pval = (u64int)dwarfget8(&b);
		return TConstant;
	case OpConst8s:
		*pval = (s64int)dwarfget8(&b);
		return TConstant;
	case OpConstu:
		*pval = dwarfget128(&b);
		return TConstant;
	case OpConsts:
		*pval = dwarfget128s(&b);
		return TConstant;
	case OpPlusUconst:
		*pval = dwarfget128(&b);
		return TConstant;
	default:
		return TBlock;
	}
}

/* last resort */
static int
skipform(DwarfBuf *b, int form)
{
	int type;
	DwarfVal val;

	if(getulong(b, form, 0, &val.c, &type) < 0
	&& getuchar(b, form, (uchar*)&val) < 0
	&& getstring(b, form, &val.s) < 0
	&& getblock(b, form, &val.b) < 0)
		return -1;
	return 0;
}

