#include "compat.h"

typedef struct Dwarf Dwarf;
typedef struct DwarfAttrs DwarfAttrs;
typedef struct DwarfBlock DwarfBlock;
typedef struct DwarfBuf DwarfBuf;
typedef struct DwarfExpr DwarfExpr;
typedef struct DwarfSym DwarfSym;
typedef union DwarfVal DwarfVal;

enum
{
	TagArrayType = 0x01,
	TagClassType = 0x02,
	TagEntryPoint = 0x03,
	TagEnumerationType = 0x04,
	TagFormalParameter = 0x05,
	TagImportedDeclaration = 0x08,
	TagLabel = 0x0A,
	TagLexDwarfBlock = 0x0B,
	TagMember = 0x0D,
	TagPointerType = 0x0F,
	TagReferenceType = 0x10,
	TagCompileUnit = 0x11,
	TagStringType = 0x12,
	TagStructType = 0x13,
	TagSubroutineType = 0x15,
	TagTypedef = 0x16,
	TagUnionType = 0x17,
	TagUnspecifiedParameters = 0x18,
	TagVariant = 0x19,
	TagCommonDwarfBlock = 0x1A,
	TagCommonInclusion = 0x1B,
	TagInheritance = 0x1C,
	TagInlinedSubroutine = 0x1D,
	TagModule = 0x1E,
	TagPtrToMemberType = 0x1F,
	TagSetType = 0x20,
	TagSubrangeType = 0x21,
	TagWithStmt = 0x22,
	TagAccessDeclaration = 0x23,
	TagBaseType = 0x24,
	TagCatchDwarfBlock = 0x25,
	TagConstType = 0x26,
	TagConstant = 0x27,
	TagEnumerator = 0x28,
	TagFileType = 0x29,
	TagFriend = 0x2A,
	TagNamelist = 0x2B,
	TagNamelistItem = 0x2C,
	TagPackedType = 0x2D,
	TagSubprogram = 0x2E,
	TagTemplateTypeParameter = 0x2F,
	TagTemplateValueParameter = 0x30,
	TagThrownType = 0x31,
	TagTryDwarfBlock = 0x32,
	TagVariantPart = 0x33,
	TagVariable = 0x34,
	TagVolatileType = 0x35,
	TagDwarfProcedure = 0x36,
	TagRestrictType = 0x37,
	TagInterfaceType = 0x38,
	TagNamespace = 0x39,
	TagImportedModule = 0x3A,
	TagUnspecifiedType = 0x3B,
	TagPartialUnit = 0x3C,
	TagImportedUnit = 0x3D,
	TagMutableType = 0x3E,

	TypeAddress = 0x01,
	TypeBoolean = 0x02,
	TypeComplexFloat = 0x03,
	TypeFloat = 0x04,
	TypeSigned = 0x05,
	TypeSignedChar = 0x06,
	TypeUnsigned = 0x07,
	TypeUnsignedChar = 0x08,
	TypeImaginaryFloat = 0x09,

	AccessPublic = 0x01,
	AccessProtected = 0x02,
	AccessPrivate = 0x03,

	VisLocal = 0x01,
	VisExported = 0x02,
	VisQualified = 0x03,

	VirtNone = 0x00,
	VirtVirtual = 0x01,
	VirtPureVirtual = 0x02,

	LangC89 = 0x0001,
	LangC = 0x0002,
	LangAda83 = 0x0003,
	LangCplusplus = 0x0004,
	LangCobol74 = 0x0005,
	LangCobol85 = 0x0006,
	LangFortran77 = 0x0007,
	LangFortran90 = 0x0008,
	LangPascal83 = 0x0009,
	LangModula2 = 0x000A,
	LangJava = 0x000B,
	LangC99 = 0x000C,
	LangAda95 = 0x000D,
	LangFortran95 = 0x000E,
	LangPLI = 0x000F,
	/* 0x8000-0xFFFF reserved */

	IdCaseSensitive = 0x00,
	IdCaseUpper = 0x01,
	IdCaseLower = 0x02,
	IdCaseInsensitive = 0x03,

	CallingNormal = 0x01,
	CallingProgram = 0x02,
	CallingNocall = 0x03,
	/* 0x40-0xFF reserved */

	InNone = 0x00,
	InInlined = 0x01,
	InDeclaredNotInlined = 0x02,
	InDeclaredInlined = 0x03,

	OrderRowMajor = 0x00,
	OrderColumnMajor = 0x01,

	DiscLabel = 0x00,
	DiscRange = 0x01,

	TReference = 1<<0,
	TBlock = 1<<1,
	TConstant = 1<<2,
	TString = 1<<3,
	TFlag = 1<<4,
	TAddress = 1<<5,

	OpAddr = 0x03,	/* 1 op, const addr */
	OpDeref = 0x06,
	OpConst1u = 0x08,	/* 1 op, 1 byte const */
	OpConst1s = 0x09,	/*	" signed */
	OpConst2u = 0x0A,	/* 1 op, 2 byte const  */
	OpConst2s = 0x0B,	/*	" signed */
	OpConst4u = 0x0C,	/* 1 op, 4 byte const */
	OpConst4s = 0x0D,	/*	" signed */
	OpConst8u = 0x0E,	/* 1 op, 8 byte const */
	OpConst8s = 0x0F,	/*	" signed */
	OpConstu = 0x10,	/* 1 op, LEB128 const */
	OpConsts = 0x11,	/*	" signed */
	OpDup = 0x12,
	OpDrop = 0x13,
	OpOver = 0x14,
	OpPick = 0x15,		/* 1 op, 1 byte stack index */
	OpSwap = 0x16,
	OpRot = 0x17,
	OpXderef = 0x18,
	OpAbs = 0x19,
	OpAnd = 0x1A,
	OpDiv = 0x1B,
	OpMinus = 0x1C,
	OpMod = 0x1D,
	OpMul = 0x1E,
	OpNeg = 0x1F,
	OpNot = 0x20,
	OpOr = 0x21,
	OpPlus = 0x22,
	OpPlusUconst = 0x23,	/* 1 op, ULEB128 addend */
	OpShl = 0x24,
	OpShr = 0x25,
	OpShra = 0x26,
	OpXor = 0x27,
	OpSkip = 0x2F,		/* 1 op, signed 2-byte constant */
	OpBra = 0x28,		/* 1 op, signed 2-byte constant */
	OpEq = 0x29,
	OpGe = 0x2A,
	OpGt = 0x2B,
	OpLe = 0x2C,
	OpLt = 0x2D,
	OpNe = 0x2E,
	OpLit0 = 0x30,
		/* OpLitN = OpLit0 + N for N = 0..31 */
	OpReg0 = 0x50,
		/* OpRegN = OpReg0 + N for N = 0..31 */
	OpBreg0 = 0x70,	/* 1 op, signed LEB128 constant */
		/* OpBregN = OpBreg0 + N for N = 0..31 */
	OpRegx = 0x90,	/* 1 op, ULEB128 register */
	OpFbreg = 0x91,	/* 1 op, SLEB128 offset */
	OpBregx = 0x92,	/* 2 op, ULEB128 reg, SLEB128 off */
	OpPiece = 0x93,	/* 1 op, ULEB128 size of piece */
	OpDerefSize = 0x94,	/* 1-byte size of data retrieved */
	OpXderefSize = 0x95,	/* 1-byte size of data retrieved */
	OpNop = 0x96,
	/* next four new in Dwarf v3 */
	OpPushObjAddr = 0x97,
	OpCall2 = 0x98,	/* 2-byte offset of DIE */
	OpCall4 = 0x99,	/* 4-byte offset of DIE */
	OpCallRef = 0x9A	/* 4- or 8- byte offset of DIE */
	/* 0xE0-0xFF reserved for user-specific */
};

struct DwarfBlock
{
	uchar *data;
	ulong len;
};

/* not for consumer use */
struct DwarfBuf
{
	Dwarf *d;
	uchar *p;
	uchar *ep;
	uint addrsize;
};

union DwarfVal
{
	char *s;
	ulong c;
	ulong r;
	DwarfBlock b;
};

struct DwarfAttrs
{
	ulong	tag;
	uchar	haskids;

	/* whether we have it, along with type */
	struct {
		uchar	abstractorigin;
		uchar	accessibility;
		uchar	addrclass;
		uchar	basetypes;
		uchar	bitoffset;
		uchar	bitsize;
		uchar	bytesize;
		uchar	calling;
		uchar	commonref;
		uchar	compdir;
		uchar	constvalue;
		uchar	containingtype;
		uchar	count;
		uchar	datamemberloc;
		uchar	declcolumn;
		uchar	declfile;
		uchar	declline;
		uchar	defaultvalue;
		uchar	discr;
		uchar	discrlist;
		uchar	discrvalue;
		uchar	encoding;
		uchar	framebase;
		uchar	friend;
		uchar	highpc;
		uchar   entrypc;
		uchar	identifiercase;
		uchar	import;
		uchar	inlined;
		uchar	isartificial;
		uchar	isdeclaration;
		uchar	isexternal;
		uchar	isoptional;
		uchar	isprototyped;
		uchar	isvarparam;
		uchar	language;
		uchar	location;
		uchar	lowerbound;
		uchar	lowpc;
		uchar	macroinfo;
		uchar	name;
		uchar	namelistitem;
		uchar	ordering;
		uchar	priority;
		uchar	producer;
		uchar	ranges;
		uchar	returnaddr;
		uchar	segment;
		uchar	sibling;
		uchar	specification;
		uchar	startscope;
		uchar	staticlink;
		uchar	stmtlist;
		uchar	stridesize;
		uchar	stringlength;
		uchar	type;
		uchar	upperbound;
		uchar	uselocation;
		uchar	virtuality;
		uchar	visibility;
		uchar	vtableelemloc;
	} have;

	ulong	abstractorigin;
	ulong	accessibility;
	ulong	addrclass;
	ulong	basetypes;
	ulong	bitoffset;
	ulong	bitsize;
	ulong	bytesize;
	ulong	calling;
	ulong	commonref;
	char*	compdir;
	DwarfVal	constvalue;
	ulong	containingtype;
	ulong	count;
	DwarfVal	datamemberloc;
	ulong	declcolumn;
	ulong	declfile;
	ulong	declline;
	ulong	defaultvalue;
	ulong	discr;
	DwarfBlock	discrlist;
	ulong	discrvalue;
	ulong	encoding;
	DwarfVal	framebase;
	ulong	friend;
	ulong	highpc;
	ulong   entrypc;
	ulong	identifiercase;
	ulong	import;
	ulong	inlined;
	uchar	isartificial;
	uchar	isdeclaration;
	uchar	isexternal;
	uchar	isoptional;
	uchar	isprototyped;
	uchar	isvarparam;
	ulong	language;
	DwarfVal	location;
	ulong	lowerbound;
	ulong	lowpc;
	ulong	macroinfo;
	char*	name;
	DwarfBlock	namelistitem;
	ulong	ordering;
	ulong	priority;
	char*	producer;
	ulong	ranges;
	DwarfVal	returnaddr;
	DwarfVal	segment;
	ulong	sibling;
	ulong	specification;
	ulong	startscope;
	DwarfVal	staticlink;
	ulong	stmtlist;
	ulong	stridesize;
	DwarfVal	stringlength;
	ulong	type;
	ulong	upperbound;
	DwarfVal	uselocation;
	ulong	virtuality;
	ulong	visibility;
	DwarfVal	vtableelemloc;
};

enum
{
	RuleUndef,
	RuleSame,
	RuleCfaOffset,
	RuleRegister,
	RuleRegOff,
	RuleLocation
};
struct DwarfExpr
{
	int type;
	long offset;
	ulong reg;
	DwarfBlock loc;
};

struct DwarfSym
{
	DwarfAttrs attrs;

/* not for consumer use... */
	DwarfBuf b;
	ulong unit;
	uint uoff;
	ulong aoff;
	int depth;
	int allunits;
	ulong nextunit;
};


struct _Pe;
Dwarf *dwarfopen(struct _Pe *elf);
void dwarfclose(Dwarf*);
int dwarfaddrtounit(Dwarf*, ulong, ulong*);
int dwarflookupfn(Dwarf*, ulong, ulong, DwarfSym*);
int dwarflookupname(Dwarf*, char*, DwarfSym*);
int dwarflookupnameinunit(Dwarf*, ulong, char*, DwarfSym*);
int dwarflookupsubname(Dwarf*, DwarfSym*, char*, DwarfSym*);
int dwarflookuptag(Dwarf*, ulong, ulong, DwarfSym*);
int dwarfenumunit(Dwarf*, ulong, DwarfSym*);
int dwarfseeksym(Dwarf*, ulong, ulong, DwarfSym*);
int dwarfenum(Dwarf*, DwarfSym*);
int dwarfnextsym(Dwarf*, DwarfSym*);
int dwarfnextsymat(Dwarf*, DwarfSym*, int);
int dwarfpctoline(Dwarf*, ulong, char**, char**, char**, char **, ulong*, ulong*, ulong*);
int dwarfunwind(Dwarf*, ulong, DwarfExpr*, DwarfExpr*, DwarfExpr*, int);
ulong dwarfget1(DwarfBuf*);
ulong dwarfget2(DwarfBuf*);
ulong dwarfget4(DwarfBuf*);
uvlong dwarfget8(DwarfBuf*);
ulong dwarfget128(DwarfBuf*);
long dwarfget128s(DwarfBuf*);
ulong dwarfgetaddr(DwarfBuf*);
int dwarfgetn(DwarfBuf*, uchar*, int);
uchar *dwarfgetnref(DwarfBuf*, ulong);
char *dwarfgetstring(DwarfBuf*);


typedef struct DwarfAbbrev DwarfAbbrev;
typedef struct DwarfAttr DwarfAttr;

struct DwarfAttr
{
	ulong name;
	ulong form;
};

struct DwarfAbbrev
{
	ulong num;
	ulong tag;
	uchar haskids;
	DwarfAttr *attr;
	int nattr;
};

struct _Pe;

struct Dwarf
{
	struct _Pe *pe;

	char **reg;
	int nreg;
	int addrsize;
	DwarfBlock abbrev;
	DwarfBlock aranges;
	DwarfBlock frame;
	DwarfBlock info;
	DwarfBlock line;
	DwarfBlock pubnames;
	DwarfBlock pubtypes;
	DwarfBlock ranges;
	DwarfBlock str;

	/* little cache */
	struct {
		DwarfAbbrev *a;
		int na;
		ulong off;
	} acache;
};

DwarfAbbrev *dwarfgetabbrev(Dwarf*, ulong, ulong);

int dwarfgetinfounit(Dwarf*, ulong, DwarfBlock*);

extern int dwarf386nregs;
extern char *dwarf386regs[];
extern char *dwarf386fp;

#define SYMBOL_SIZE 18
#define MAXIMUM_DWARF_NAME_SIZE 64
#define MAXIMUM_COFF_SYMBOL_LENGTH 256
