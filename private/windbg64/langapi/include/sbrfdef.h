// sdbfdef.h    Source Browser .SBR file definitions

#define SBR_L_UNDEF         0       // Undefined         
#define SBR_L_BASIC         1       // Basic
#define SBR_L_C             2       // C
#define SBR_L_FORTRAN       3       // Fortran
#define SBR_L_MASM          4       // Macro
#define SBR_L_PASCAL        5       // Pascal
#define SBR_L_COBOL			6		// Cobol
#define SBR_L_CXX			7		// C++

#define SBR_REC_HEADER      0x00    // Header
#define SBR_REC_MODULE      0x01    // Module definition
#define SBR_REC_LINDEF      0x02    // Line Number
#define SBR_REC_SYMDEF      0x03    // Symbol Definition
#define SBR_REC_SYMREFUSE   0x04    // Symbol Reference
#define SBR_REC_SYMREFSET   0x05    // Symbol Ref assign
#define SBR_REC_MACROBEG    0x06    // Macro Start
#define SBR_REC_MACROEND    0x07    // Macro End
#define SBR_REC_BLKBEG      0x08    // Block Start
#define SBR_REC_BLKEND      0x09    // Block End
#define SBR_REC_MODEND      0x0A    // Module End
#define SBR_REC_OWNER		0x0B	// Scope Owner
#define SBR_REC_BASE		0x0C	// Define base class of owner
#define SBR_REC_FRIEND		0x0D	// Define friend of owner
#define SBR_REC_ABORT		0x0E	// Compiler aborted, .sbr file ended early
#define SBR_REC_PCHNAME		0x0F	// Precompiler header include
#define SBR_REC_PCHMARK		0x10	// Precompiler header highwater mark

//  psuedo records for internal use.

#define SBR_REC_ERROR       0x11    // error-record -- xlated to symdef
#define SBR_REC_MACROREF    0x12	// this space for rent
#define SBR_REC_MACRODEF    0x13	// this space for rent
#define SBR_REC_IGNORE      0x14    // abort condition discovered
									// after record already enqueued

// icc related records (these actually occur in .sbr files)

#define SBR_REC_LINDELTA	0x15	// 16 bit signed line number delta
#define SBR_REC_INFOSEP		0x16	// begins any ICC section + ends previous
#define SBR_REC_PATCHTAB	0x17	// introduces patch table entry
#define SBR_REC_PATCHTERM	0x18	// ends the patch table (includes cookie)
#define SBR_REC_SYMDECL		0x19	// used in fe internally for icc browse

#define SBR_COOKIE_ICC		0x4A525301	// cookie at EOF if ICC patch present

//
//  records which have extended keys (when the keys don't fit in 16 bits.)
//
#define SBR_REC_SYMDEF_X    (SBR_REC_SYMDEF | SBR_REC_EXTENDED_KEYS)	// Extended key symbol definition
#define SBR_REC_SYMREFUSE_X (SBR_REC_SYMREFUSE | SBR_REC_EXTENDED_KEYS)	// Extended key symbol reference
#define SBR_REC_SYMREFSET_X (SBR_REC_SYMREFSET | SBR_REC_EXTENDED_KEYS)	// Extended key symbol ref assign
#define SBR_REC_OWNER_X     (SBR_REC_OWNER | SBR_REC_EXTENDED_KEYS)		// Extended key scope owner
#define SBR_REC_BASE_X      (SBR_REC_BASE | SBR_REC_EXTENDED_KEYS)		// Extended key base class
#define SBR_REC_FRIEND_X	(SBR_REC_FRIEND | SBR_REC_EXTENDED_KEYS)	// Extended key friend of owner

#define SBR_REC_BUMP_N      0x20    // Bump the line number N+1 times

#define	SBR_REC_EXTENDED_KEYS		0x40	// the keys are bigger than 16 bits.

#define	SBR_REC_EXTENDED_MASK		(SBR_REC_EXTENDED_KEYS)	//  add more as required

#define	GetSbrRecType(v)			(v & (~ SBR_REC_EXTENDED_MASK ))

#define	SetSbrRecExtendedAttrib(v,a)	(v |= (a))
#define	ClrSbrRecExtendedAttrib(v,a)	(v &= ~(a))
#define	IsSetSbrRecExtendedAttrib(v,a)	(v & (a))

#define SBR_REC_NOCOLUMN    1       // Missing column default 1

#define SBR_TYP_FUNCTION    0x01
#define SBR_TYP_LABEL       0x02
#define SBR_TYP_PARAMETER   0x03
#define SBR_TYP_VARIABLE    0x04
#define SBR_TYP_CONSTANT    0x05
#define SBR_TYP_MACRO       0x06
#define SBR_TYP_TYPEDEF     0x07
#define SBR_TYP_STRUCNAM    0x08
#define SBR_TYP_ENUMNAM     0x09
#define SBR_TYP_ENUMMEM     0x0A
#define SBR_TYP_UNIONNAM    0x0B
#define SBR_TYP_SEGMENT	    0x0C
#define SBR_TYP_GROUP	    0x0D
#define SBR_TYP_PROGRAM	    0x0E
#define SBR_TYP_CLASSNAM    0x0F
#define SBR_TYP_MEMFUNC	    0x10
#define SBR_TYP_MEMVAR	    0x11
#define SBR_TYP_ERROR		0x12

#define SBR_ATR_LOCAL	    0x001
#define SBR_ATR_STATIC	    0x002
#define SBR_ATR_SHARED	    0x004
#define SBR_ATR_NEAR	    0x008
#define SBR_ATR_COMMON	    0x010
#define SBR_ATR_DECL		0x020
#define SBR_ATR_DECL_ONLY	0x020	// synonym for the above
#define SBR_ATR_PUBLIC      0x040
#define SBR_ATR_NAMED	    0x080
#define SBR_ATR_MODULE		0x100
#define SBR_ATR_VIRTUAL		0x200
#define SBR_ATR_PRIVATE		0x400
#define SBR_ATR_PROTECT		0x800

#define SBR_ITYP_VIRTUAL	0x01
#define SBR_ITYP_PRIVATE	0x02
#define SBR_ITYP_PUBLIC		0x04
#define SBR_ITYP_PROTECT	0x08

#define SBR_VER_MAJOR       2       // Major version
#define SBR_VER_MINOR       0       // Minor version

#define SBR_LIMIT_ID_LENGTH	256		// At the moment (12Apr94) bscmake can't take longer names
