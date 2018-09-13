// *******************************************************
// contexts
// *******************************************************
#define cLIB	  0x0001
#define cTYPE	  0x0002
#define cIMPLTYPE 0x0004
#define cFUNC	  0x0008
#define cVAR	  0x0010
#define cPARAM	  0x0020

// *******************************************************
//  Attributes
// *******************************************************

// bit flags indicating presence of each attribute
#define	fUUID		0x00000001L
#define	fVERSION	0x00000002L
#define	fDLLNAME	0x00000004L
#define	fENTRY		0x00000008L
#define	fRESTRICTED	0x00000010L
#define	fHELPSTRING	0x00000020L
#define	fHELPCONTEXT 	0x00000040L
#define	fHELPFILE	0x00000080L
#define	fLCID		0x00000100L
#define	fPROPGET	0x00000200L
#define	fPROPPUT	0x00000400L
#define	fPROPPUTREF	0x00000800L
#define	fOPTIONAL	0x00001000L
#define	fIN		0x00002000L
#define	fOUT		0x00004000L
#define	fPUBLIC		0x00008000L
#define	fSTRING		0x00010000L
#define	fID		0x00020000L
#define	fVARARG		0x00040000L
#define	fAPPOBJECT	0x00080000L
#define	fREADONLY	0x00100000L
#define	fODL		0x00200000L
#define	fDEFAULT	0x00400000L
#define	fSOURCE		0x00800000L
#define	fBINDABLE	0x01000000L
#define	fREQUESTEDIT	0x02000000L
#define	fDISPLAYBIND	0x04000000L
#define	fDEFAULTBIND	0x08000000L
#define fLICENSED	0x10000000L
#define fPREDECLID	0x20000000L
#define fHIDDEN 	0x40000000L
#define	fRETVAL		0x80000000L

#define fPropBits 	(fPROPGET | fPROPPUT | fPROPPUTREF)

// *******************************************************
// More Attributes (to be used only with fAttr2)
// *******************************************************

#define	f2PASCAL	0x00000001
#define	f2CDECL		0x00000002
#define	f2STDCALL	0x00000004
#define	f2CCDEFAULTED	0x00000008
#define	f2CONTROL	0x00000010
#define	f2DUAL		0x00000020
#define	f2NONEXTENSIBLE	0x00000040
#define	f2OLEAUTOMATION	0x00000080
	#define f2OACompatBits (f2DUAL | f2OLEAUTOMATION)	// shorthand
#define	f2GotConstVal	0x00000100	// const elem's value explictly given
#define	f2VALIDDUALBASE	0x00000200	// valid for use as base interface
					// of a dual interface

// the following bits are unused at present
#define	f2Unused07	0x00000400
#define	f2Unused08	0x00000800
#define	f2Unused09	0x00001000
#define	f2Unused10	0x00002000
#define	f2Unused11	0x00004000
#define	f2Unused12	0x00008000
#define	f2Unused13	0x00010000
#define	f2Unused14	0x00020000
#define	f2Unused15	0x00040000
#define	f2Unused16	0x00080000
#define	f2Unused17	0x00100000
#define	f2Unused18	0x00200000
#define	f2Unused19	0x00400000
#define	f2Unused20	0x00800000
#define	f2Unused21	0x01000000
#define	f2Unused22	0x02000000
#define	f2Unused23	0x04000000
#define	f2Unused24	0x08000000
#define	f2Unused25	0x10000000
#define	f2Unused26	0x20000000
#define	f2Unused27	0x40000000
#define	f2Unused28	0x80000000

typedef struct {
    DWORD	fAttr;		// bit flags
    DWORD	fAttr2;		// more bit flags
    WORD	fContext;	// the context of these attributes
    WORD	wFlags;		// flags associated with this item
    // the following attributes are allowed on most items
    GUID FAR *	lpUuid;
    WORD	wVerMajor;
    WORD	wVerMinor;
    DWORD	lHelpContext;
    LPSTR	lpszHelpString;
    // the following attributes are mutually-exclusive
    union {
    	DWORD	lLcid;		// only on Library
        LPSTR	lpszDllName;	// only on Module
	LPSTR	lpszProcName;	// only on Module function
    };
    // the following attributes are mutually-exclusive
    union {
        LPSTR	lpszHelpFile;	// only on Library
        DWORD	lId;		// on lots of ids (but not Library)
    };
} ATTR;

typedef ATTR FAR * LPATTR;


// *******************************************************
//  TYPE structures
// *******************************************************

// kind of entries in the type table
// WARNING: rgtkind in TYPOUT.CPP depends on this order
typedef enum {
    tTYPEDEF = 0,
    tSTRUCT,
    tENUM,
    tUNION,
    tMODULE,
    tINTERFACE,
    tDISPINTER,
    tCOCLASS,
    // the following don't have full-size entries for them
    tINTRINSIC,
    tREF,
    // tIMPORTED also doesn't have full-size entries

    // the following don't have ANY entries for them
    tANY,			// no specific type

    tIMPORTED=0x1000,		// imported type with qualification
    tQUAL=0x2000,		// imported type with qualification
    tFORWARD = 0x4000		// bit set if this is a forward definition
} TENTRYKIND;

// Used to describe intrinsic types, type references, and typedef [struct/enum]
typedef struct tagTYPE {
    struct tagTYPE FAR * pNext;	// pointer to next type item
			 	// MUST BE FIRST
    TENTRYKIND	tentrykind;	// kind of this type item
    LPSTR	szName;		// name of this type item
    TYPEDESC	tdesc;		// contains:
				//   vt = one of
    				//     intrinsic type ID
				//     VT_PTR ==> * to another type
				//     VT_USERDEFINED ==> typedef struct
				//			  typedef enum
				//			  typedef union
				// 			  interface,
				// 			  dispinterface,
				// 			  coclass,
				//			  modules
				//     VT_SAFEARRAY ==> array of another type
				//   indexreftypeinfo (only for VT_USERDEFINED)
				//   lptdesc (only for VT_PTR, VT_SAFEARRAY)
    ITypeInfo FAR* lptinfo;	// ITypeInfo for this guy
				// (filled in during output pass)
    union {
        struct {		// for tINTRINSIC, (VT_xxx)
	    BOOL   fUnsigned;	// non-zero if this type is an UNSIGNED type
        } intr;
        struct {		// for tREF (VT_PTR, VT_SAFEARRAY, or VT_CARRAY)
				// not a true type, just a reference to one
            struct tagTYPE FAR * ptypeBase;	// base type
	    SHORT  cIndirect;	// # of levels of indirection off base type
				// (VT_PTR only)
        } ref;
        struct {		// for tTYPEDEF (VT_USERDEFINED)
            struct tagTYPE FAR * ptypeAlias; // type this is an alias for
        } td;
        struct {		// for tINTERFACE/tDISPINTER/tCOCLASS
				// (VT_USERDEFINED)
            struct tagINTER FAR * interList;	// list of base interface(s)
						// NULL if none
        } inter;
        struct {		// for tSTRUCT/tENUM/tUNION (VT_USERDEFINED)
	    LPSTR  szTag;	// optional tag name (NULL if not present)
            struct tagELEM FAR * elemList;	// list of struct/enum/union
						// members
        } structenum;
        struct {		// for tIMPORTED | tXXXX, (VT_USERDEFINED)
	    WORD   wTypeFlags;	// flags for this type
        } import;
    };
} TYPE;
typedef TYPE FAR * LPTYPE;


// for element of struct, enum, union, function arg, property, etc.
typedef struct tagELEM {
  struct tagELEM FAR * pNext;
  ATTR	 attr;			// element attributes
  LPSTR	 szElemName;            // element name
  LPTYPE elemType;		// element type
  VARIANT FAR * lpElemVal;	// element value (for constants)
} ELEM;
typedef	ELEM FAR * LPELEM;


// For base interface of a interface/dispinterface, or implemented interface
// of a coclass.
typedef struct tagINTER {
  struct tagELEM FAR * pNext;
  LPTYPE ptypeInter;		// interface type
  DWORD	 fAttr;			// interface attributes (only used for COCLASS)
  DWORD  fAttr2;		// don't need the entire ATTR structure here,
				// since only valid attrs are RESTRICTED,
				// DEFAULT, and SOURCE, and they don't have
				// any other info in them.
} INTER;
typedef	INTER FAR * LPINTER;


// *******************************************************
//  Function definition structures
// *******************************************************

typedef struct tagFUNC {
    ELEM   func;		// function descr. (pNext, attr, name, type)
				// MUST BE FIRST -- folks assume that can use
				// a FUNC as an ELEM
    SHORT  cArgs;		// # of args
    SHORT  cOptArgs;		// # of optional args
    LPELEM argList;		// arg info
} FUNC;
typedef FUNC FAR * LPFUNC;


// *******************************************************
//  Component structures
// *******************************************************

// module <modulename> { ... }
typedef struct {
    LPFUNC funcList;            // list of functions
    LPELEM constList;		// list of constants
} MODULE;

// interface <interfacename> : <baseinterface> { ... }
typedef struct {
    LPFUNC funcList;		// list of interface functions
} INTERF;

// dispinterface <interfacename> { ... }
typedef struct {
    LPELEM propList;		// list of properties
    LPFUNC methList;		// list of methods
} DISPINTER;

#if FV_PROPSET
// property_set <propsetname> : <base propsetname> { ... }
typedef struct {
    LPSTR  szBaseName;		// base property set name, NULL if none
    LPELEM propList;		// list of properties
} PROPSET;
#endif //FV_PROPSET

typedef struct tagENTRY {
    TYPE      type;  			// type of this item
					// (contains pNext, name, etc)
					// MUST BE FIRST
    ATTR      attr;			// attributes for this item
    struct tagENTRY FAR * lpEntryForward; // pointer to forward declaration
					  // for this item, NULL if none.
    ICreateTypeInfo FAR* lpdtinfo;	// ICreateTypeInfo for this guy
					// (filled in during output pass)
    union	{			// extra info about the item
	MODULE    module;
	INTERF	  inter;
	DISPINTER dispinter;
#if FV_PROPSET
	PROPSET   propset;
#endif //FV_PROPSET
    };
} ENTRY;

typedef ENTRY FAR * LPENTRY;


// importlib (<filename>)
typedef struct tagIMPORTLIB {
    struct tagIMPORTLIB FAR * pNext;	// link field
    LPSTR lpszFileName;		// filename specified by user
    LPSTR lpszLibName;		// library name of this type library
    ITypeLib FAR* lptlib;	// ITypeLib for this type library
    ITypeComp FAR* lptcomp;	// ITypeComp for this type library
    LPTLIBATTR lptlibattr;	// type library attributes
} IMPORTLIB;
typedef IMPORTLIB FAR * LPIMPORTLIB;


typedef	struct {
  ATTR	     attr;		// library attributes
  LPSTR	     szLibName;		// library name
  LPENTRY    pEntry;		// circularly-linked list of library items
  LPIMPORTLIB pImpLib;		// circularly-linked list of imported libraries
} TYPLIB;


// #defines that simplify linked-list management.  Assumes circularly-linked lists, and
// that all lists have the pNext field at the same position (usually first).

// Given pList, returns a pointer to the last element in the list.
#define	ListLast(pList) (pList)

// Given pList, returns a pointer to the first element in the list.
// Since circularly-linked, Next(Last) points to first element.
#define	ListFirst(pList) (((LPTYPE)pList)->pNext)

#ifdef	__cplusplus
extern "C" {
#endif

extern VOID FAR ListInsert(LPVOID ppList, WORD cbElem);

#ifdef	__cplusplus
}
#endif
