// Interface for minimum rebuild engine
//

#if !defined(_mrengine_h)
#define _mrengine_h

// get rid of baggage we don't need from windows.h
#define WIN32_LEAN_AND_MEAN
//#define NOGDI
#define NOUSER
#define NONLS

// include pdb, msf, and nameserver 
#include <pdb.h>
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stddef.h>

//
// Get the calling conventions and declspec stuff out of the way
//
#if defined(_X86_) || defined(_M_IX86)
#define	MRECALL			__stdcall
#else
#define MRECALL			__cdecl
#endif

#if defined(MR_ENGINE_STATIC)
#define MREAPI(rtype)		rtype MRECALL
#elif !defined(MR_ENGINE_IMPL)
#define	MREAPI(rtype)		rtype __declspec(dllimport) MRECALL
#else
#define MREAPI(rtype)		rtype __declspec(dllexport) MRECALL
#endif

//
// Our C++ interfaces
//
interface					MREngine;
typedef interface MREngine	MREngine;
typedef MREngine *			PMREngine;

interface					MREDrv;
typedef interface MREDrv	MREDrv;
typedef MREDrv *			PMREDrv;

interface					MRECmp;
typedef interface MRECmp	MRECmp;
typedef MRECmp *			PMRECmp;

interface					MREUtil;
typedef interface MREUtil	MREUtil;
typedef MREUtil *			PMREUtil;

interface					MREFile;
typedef interface MREFile	MREFile;
typedef MREFile *			PMREFile;

interface					MREBag;
typedef interface MREBag	MREBag;
typedef MREBag *			PMREBag;

interface					MRELog;
typedef interface MRELog	MRELog;
typedef MRELog *			PMRELog;

//
// other interesting things
//
#if !defined(FALSE)
#define FALSE 0
#define TRUE 1
#endif

#if !defined(fFalse)
#define fFalse FALSE
#define fTrue TRUE
#endif

// compile time assert
#if !defined(CASSERT)
	#if defined(_DEBUG)
		#define CASSERT(x) extern char dummyAssert[ (x) ]
	#else
		#define CASSERT(x)
	#endif
#endif

typedef _TCHAR *			SZ;			// String
typedef const _TCHAR *		SZC;		// const String
typedef DWORD				_CB;

#pragma warning(disable:4200)

typedef struct DepData {
	size_t	cb;
	BYTE	rgb[];
	} DepData, *PDepData;

#pragma warning(default:4200)

#if _INTEGRAL_MAX_BITS >= 64
typedef unsigned __int64	QWORD;		// 64-bit unsigned int
#else
typedef ULARGE_INTEGER		QWORD;
#endif

typedef PDB *				PPDB;
typedef NameMap *			PNMP;

typedef enum DEPON {			// depends on bits
	deponName = 0,					// nothing...requires a name
	deponVtshape = 0x1,				// depon virtual functions/bases
	deponShape = 0x2,				// depon size or offsets of class
	deponAll = -1,					// pseudo deps, not stored, but union
									// 	of all the above...
	} DEPON;

// file out-of-date structure/enumerations/flags
typedef enum YNM {	// Yes No Maybe
	ynmNo,
	ynmMaybe,
	ynmYes
	} YNM;
typedef struct SRCTARG *	PSRCTARG;
typedef struct SRCTARG {
	PSRCTARG	psrctargNext;
	BOOL		fCpp;
	SZC			szSrc;
	SZC			szTarg;
	SZC			szOptions;
	DWORD		dwWeightMaybe;
	} SRCTARG, **PPSRCTARG;

typedef struct CAList {		// Compile Action List
	PSRCTARG	pstDoCompile;
	PSRCTARG	pstMaybeCompile;
	PSRCTARG	pstDontCompile;
	PSRCTARG	pstDone;
	PSRCTARG	pstError;
	} CAList, *PCAList;

typedef enum TrgType {
	trgtypeObject,
	trgtypeSbr
	} TrgType;

// inline functions to manipulate lists of SRCTARGs
//
// Insert at a particular pst location, can be anywhere in the list
__inline PPSRCTARG
InsertSrcTarg ( PPSRCTARG ppstAt, PSRCTARG pst ) {
	pst->psrctargNext = *ppstAt;
	*ppstAt = pst;
	return &pst->psrctargNext;
	}

// Delete does a find on the pst then snips from the list
//
__inline void
DeleteSrcTarg ( PPSRCTARG ppstListHead, PSRCTARG pst ) {
	PPSRCTARG	ppstCur = ppstListHead;
	PPSRCTARG	ppstPrev = ppstCur;

	// find the matching pst
	while ( *ppstCur && *ppstCur != pst ) {
		ppstPrev = ppstCur;
		ppstCur = &(*ppstCur)->psrctargNext;
		}
	if ( *ppstCur ) {
		*ppstPrev = (*ppstCur)->psrctargNext;
		}
	}

// Delete a particular pst, given its pst and a pst whose psrctargNext
//	points to pst.
__inline void
DeleteSrcTargAt ( PSRCTARG pst, PPSRCTARG ppstPrev ) {
	*ppstPrev = pst->psrctargNext;
	}

// NextSrcTarg, so simple, no comment
//
__inline PSRCTARG
PstNext ( PSRCTARG pst ) {
	return pst ? pst->psrctargNext : 0;
	}

// Return the address of the last pst (a PPSRCTARG) in the list
//
__inline PPSRCTARG
PpstLast ( PPSRCTARG ppstHead ) {
	PSRCTARG	pst = *ppstHead;
	PPSRCTARG	ppstRet = ppstHead;

	while ( pst = PstNext ( pst ) ) {
		ppstRet = &pst->psrctargNext;
		}

	return ppstRet;
	}
//
// PDB api required by MRE.  Passed in in cases where the caller is using
// a wide bandwidth api to the PDB already.
//
typedef struct MreToPdb {
	PDB *		ppdb;			// the IDB file pdb api
	NameMap *	pnamemap;		// the namemap in the IDB
	TPI *		ptpi;			// the tpi in the PDB
	PDB *		ppdbPdbFile;	// the PDB file pdb api
	void *		pvReserved1;
	void *		pvReserved2;
	} MreToPdb, *PMreToPdb;

#if defined(__cplusplus)

// C++ bindings

// callback functions and datatypes for enumerating files and dependencies
enum EnumType { etSource, etDep };

enum ChgState { chgstUnknown, chgstClass, chgstRude };

// File Status Masks
enum FSM {
	fsmNil = 0,
	fsmOutOfDate = 0x1,
	fsmHasTarget = 0x2,
	fsmIsTarget = 0x4,
	fsmVisited = 0x8,
	fsmIgnoreDep = 0x10,
	fsmInclByPch = 0x20,
	fsmIsPch = 0x40,
	fsmCreatesPch = 0x80,
	fsmIsetPch = 0xffff0000,
	};

union MREFT {			// MRE FileTime
	FILETIME	ft;
	QWORD		qwft;
	};

typedef unsigned long	BldId;
const BldId	bldidMax = ULONG_MAX;

struct FILEINFO {
	NI		niFile;			// main index value
	union {
		DWORD	dwStatus;		// file status bits
		// these are not used, just added for reference during debugging...
		struct {
			unsigned	fOutOfDate		: 1;
			unsigned	fHasTarget		: 1;
			unsigned	fIsTarget		: 1;
			unsigned	fVisited		: 1;
			unsigned	fIgnoreDep		: 1;
			unsigned	fInclByPch		: 1;
			unsigned	fIsPch			: 1;
			unsigned	fCreatesPch		: 1;
			unsigned	fUnused			: 8;
			unsigned	isetPch			: 16;
			};
		};
	MREFT	fiTime;			// time stamp
	QWORD	fiSize;			// file size
	NI		niOptions;		// option string (src only)
	NI		niRelated;		// for src, it is the target, for target, it is src
	BldId	bldid;			// tell when (wrt the pdb) last built (src only)
	DWORD	dwReserved;		// internal padding to 8 byte alignment
	};

// make sure the sizes and offsets are what we expect, as well as 8byte alignment
CASSERT(offsetof(FILEINFO, fiTime) == sizeof(NI)+sizeof(DWORD));
CASSERT((sizeof(FILEINFO) & 0x7) == 0);

struct FILESUMMARYINFO {
	DWORD	cFileDeps;
	DWORD	cClasses;
	};
typedef FILESUMMARYINFO	FSI;

struct EnumFile {
	SZC			szFileSrc;
	SZC			szFileTrg;
	FILEINFO	fiSrc;
	FILEINFO	fiTrg;
	FSI			fsiSrc;
	SZC			szOptions;
	void *		pvContext;
	};

struct EnumClass {
	NI			niClass;
	SZC			szClassName;
	DWORD		depon;
	DWORD		cMembersHit;
	DWORD		cMembersBits;
	TI			tiClass;
	void *		pvContext;
	};

// enumfile source (root) file callback
typedef BOOL (MRECALL * PfnFEFSrcCallBack) ( PMREUtil, EnumFile &, EnumType et =etSource );
// enumfile dep file callback
typedef BOOL (MRECALL * PfnFEFDepCallBack) ( PMREUtil, EnumFile &, EnumType et =etDep );
// enumfile all files callback
typedef BOOL (MRECALL * PfnFEFAllCallBack) ( PMREUtil, EnumFile & );
// enumclass callback
typedef BOOL (MRECALL * PfnFEClassCallBack) ( PMREUtil, EnumClass & );

// status (bytes, sizes, etc.)
struct StreamUtilization {
	DWORD	cbUsed;			// actually used by data we need
	DWORD	cbInternal;		// internal size (maps, sets)
	DWORD	cbExternal;		// external size (stream)

	StreamUtilization &
	operator += ( const StreamUtilization & su ) {
		cbUsed += su.cbUsed;
		cbInternal += su.cbInternal;
		cbExternal += su.cbExternal;
		return *this;
		}
	StreamUtilization
	operator + ( const StreamUtilization & su ) {
		StreamUtilization	suNew = *this;
		return suNew += su;
		}

	};

struct MreStats {
	DWORD				cSrcFiles;
	DWORD				cTrgFiles;
	DWORD				cDepFiles;
	DWORD				cClasses;
	DWORD				cBoringClasses;
	DWORD				cDedicatedStreams;
	DWORD				cbFilesInNamemap;
	DWORD				cbClassesInNamemap;
	StreamUtilization	suFileInfo;
	StreamUtilization	suClassInfo;
	StreamUtilization	suPerFileInfo;
	StreamUtilization	suTotal;
	};

interface MREDrv {	// compiler driver port
	// release this interface
	virtual BOOL		FRelease() pure;

	// expensive operation!  the driver will call this once per list of
	//	files to compile.
	virtual BOOL		FRefreshFileSysInfo() pure;

	// called by the driver after each successful compilation, this lets the
	//	mr engine know that a file was successfully compiled or not.
	virtual BOOL		FSuccessfulCompile (
							BOOL	fOk,
							SZC		szFileSrc,
							SZC		szFileTarg
							) pure;

	// To check if a file is really out of date wrt our fine grained deps...
	virtual YNM			YnmFileOutOfDate ( SRCTARG & ) pure;

	// alternate way, batch the files up, will segregate files into the
	//	appropriate lists inside of the CAList structure.
	virtual BOOL		FFilesOutOfDate ( PCAList pst ) pure;
							
	// post pass for files we didn't actually compile, but may need to update
	virtual BOOL		FUpdateTargetFile ( SZC szTrg, TrgType ) pure;

	// one-time driver init function.  called once per driver invocation ONLY.
	virtual void		OneTimeInit() pure;
	};

interface MRECmp {	// compiler front end port
	// release this interface
	virtual BOOL		FRelease() pure;

	// this method is used to get things rolling on each compiland.
	virtual BOOL		FOpenCompiland (
							OUT PMREFile * ppmrefile,
							IN SZC szFileSrc,
							IN SZC szFileTarg
							) pure;

	// after all is said and done with the compile, this needs to get called
	//	to do folding, propagation, and flattening, after the compiler
	//	has run successfully.
	virtual BOOL		FCloseCompiland ( PMREFile pmrefile, BOOL fCommit ) pure;

	// called when moving to an #include'ed file
	virtual BOOL		FPushFile (
							OUT PMREFile * ppmrefile,
							IN SZC szFile,
							IN HANDLE hFile =INVALID_HANDLE_VALUE
							) pure;

	// called when leaving an #include'ed file.
	//	returns PMREFile of enclosing (previous) file.
	virtual PMREFile	PmrefilePopFile() pure;

	// save/restore the dependencies (at pch create/restore time).
	//	Call FStoreDepData with a pointer to a DepData structure with a valid
	//	size  (the _cb field needs to be included).  The smallest possible
	//	DepBlob size is sizeof(CB).  A good size to start with is probably 8K. 
	//	This call will fill in the actual size of the data so the 
	//	block of memory can be trimmed back if you like.  Likewise, if the
	//	block is not big enough, this will return false with the size needed
	//	filled into the DepData.  If that size is 0, then there is no DepData
	//	to store.
	virtual BOOL		FStoreDepData ( PDepData ) pure;
	virtual BOOL		FRestoreDepData ( PDepData ) pure;

	// tell me a class in not interesting to store deps for (not changing much?)
	virtual void		ClassIsBoring ( NI niClass ) pure;

	};

interface MREUtil {	// utility port, stats, enumerations, etc.
	// release this interface
	virtual BOOL		FRelease() pure;

	// support for offline tools to dump and otherwise peruse the
	//	mregine storage
	virtual void		EnumSrcFiles (
                            PfnFEFSrcCallBack,
                            SZC szFileSrc = NULL,
                            void * pvContext = NULL
                            ) pure;
	virtual void		EnumDepFiles ( EnumFile &, PfnFEFDepCallBack ) pure;
	virtual void		EnumAllFiles (
                            PfnFEFAllCallBack,
                            SZC szFileSrc = NULL,
                            void * pvContext = NULL
                            ) pure;
	virtual void		EnumClasses (
                            PfnFEClassCallBack,
                            SZC szFileSrc,
                            void * pvContext = NULL
                            ) pure;
	virtual void		SummaryStats ( MreStats & ) pure;

	};

// basically, we have a hierarchy: an MRE has MREFiles which in turn 
//	have MREBags.
// 
interface MREngine {
	// open methods by pdb * and pdb name
	static MREAPI(BOOL)	FOpen (
		OUT PMREngine *	ppmre,
		PPDB			ppdb,
		PNMP			pnmp,
		BOOL			fWrite,
		BOOL			fClosePdb = fFalse
		);
	static MREAPI(BOOL)	FOpen (
		OUT PMREngine *	ppmre,
		SZC				szPdb,
		EC &			ec,
		_TCHAR			szErr[ cbErrMax ],
		BOOL			fReproSig,
		BOOL			fWrite
		);
	static MREAPI(BOOL) FOpen (
		OUT PMREngine *	ppmre,
		PMreToPdb		pmretopdb,
		BOOL			fWrite,
		BOOL			fClosePdb = fFalse
		);
		

	// remove all MRE related streams from the PDB.
	virtual BOOL		FDelete() pure;

	// close and optionally commit new data
	virtual BOOL		FClose ( BOOL fCommit ) pure;
	
	// get the pdb api we are using
	virtual void		QueryPdbApi ( PDB *& rppdb, NameMap *& rpnamemap ) pure;

	// get the MreLog api
	virtual void		QueryMreLog ( PMRELog & rpmrelog ) pure;

	// get the various interface ptrs
	virtual void		QueryMreDrv ( PMREDrv & rpmredrv ) pure;
	virtual void		QueryMreCmp ( PMRECmp & rpmrecmp, TPI * ) pure;
	virtual void		QueryMreUtil ( PMREUtil & rpmreutil ) pure;

	// commit the global portions (actually, just write the streams...
	// a PDB/MSF commit must happen as well)
	virtual BOOL		FCommit() pure;

	};

//
// ClassChanged enumeration types for MREFile interface
//
typedef BOOL (MREFile::* MfnNoteClassTI) ( NI, TI );
typedef BOOL (__cdecl * PfnEnumClassChange) ( NI, PMREFile, MfnNoteClassTI );

interface MREFile {

	// icc -> mr flags
	//
	// iccfClassMrEdit implies that the class has a change that is detectable
	//	via cv type info changes downstream from the icc.
	//
	// iccfClassRudeEdit implies that something changed in the class that
	//	is not detectable downstream via type info and that at the very least,
	//	any code dependent in any way on the class needs to be recompiled.
	//
	// iccfFileRudeEdit means that something in the header file is rude,
	//	either added, removed, or continuing to be rude.
	//
	// iccfMethodEdit means that a change to class::method needs to be
	//	generated and any code dependent on class::method compiled.
	//
	// iccfAllCodeCompiled is used to detect when all code in a compiland
	//	has been compiled or not.  the incrememtal compiler may skip functions,
	//	in which case, the MR will merge the previous class deps with the new
	//	class deps in order to not lose dependency information.
	enum {
		iccfClassRudeEdit = 0x1,
		iccfClassMrEdit = 0x2,
		iccfFileRudeEdit = 0x4,
		iccfMethodEdit = 0x8,
		iccfAllCodeCompiled = 0x20,
		iccfAnyRude = iccfClassRudeEdit | iccfFileRudeEdit,
		};

	virtual BOOL		FOpenBag ( OUT PMREBag * ppmrebag, NI niNameBag ) pure;
	virtual BOOL		FnoteEndInclude ( DWORD dwFlags ) pure;
	virtual BOOL		FnoteClassMod ( NI niClass, DWORD dwFlags ) pure;
	virtual BOOL		FnoteInlineMethodMod ( 
							NI		niClass,
							SZC		szMember,
							DWORD	dwFlags
							) pure;
	virtual BOOL		FnoteLineDelta ( DWORD dwLineBase, INT delta ) pure;
	virtual void		EnumerateChangedClasses ( PfnEnumClassChange ) pure;
	virtual BOOL		FnoteClassTI ( NI, TI ) pure;
	virtual BOOL		FIsBoring() pure;
	virtual BOOL		FnotePchCreateUse (
							SZC	szPchCreate,
							SZC szPchUse
							) pure;
	};

interface MREBag {
	virtual BOOL		FAddDep (
							NI		niDep,				// class name's NI
							TI		tiDep,				// class type index
							SZC		szMemberName,		// member name
							DEPON	deponHow,			// how it depends on it
							DWORD	dwLine =0			// where referenced
							) pure;
	virtual BOOL		FClose() pure;
	};

interface MRELog {
	virtual void		TranslateToText ( BOOL fClear, _TCHAR ** pptch ) pure ;
	virtual void		TranslateToText ( BOOL fClear, FILE * pfile ) pure;
	};


#endif	// __cpluscplus

#if defined(__cplusplus)
extern "C" {
#endif

// C interfaces for MRE
MREAPI(BOOL)		MREFOpenEx (
						OUT PMREngine *	ppmre,
						PMreToPdb		pmretopdb,
						BOOL			fWrite
						);
MREAPI ( BOOL )		MREFOpen (
						OUT PMREngine * ppmre,
						PPDB			ppdb,
						PNMP			pnmp,
						BOOL			fWrite
						);
MREAPI ( BOOL )		MREFOpenByName (
						OUT PMREngine *	ppmre,
						SZC				szPdb,
						EC *			pec,
						_TCHAR			szErr[ cbErrMax ],
						BOOL			fReproSig,
						BOOL			fWrite
						);
MREAPI ( BOOL )		MREFDelete ( PMREngine );
MREAPI ( BOOL )		MREFClose ( PMREngine, BOOL fCommit ) ;
MREAPI ( void )		MREQueryMreDrv ( PMREngine, OUT PMREDrv *);
MREAPI ( void )		MREQueryMreCmp ( PMREngine, OUT PMRECmp *, IN TPI * );
MREAPI ( void )		MREQueryMreUtil ( PMREngine, OUT PMREUtil * );

// C interfaces for MREDrv
MREAPI ( BOOL )		MREDrvFRelease ( PMREDrv );
MREAPI ( BOOL )		MREDrvFRefreshFileSysInfo ( PMREDrv );
MREAPI ( BOOL )		MREDrvFSuccessfulCompile (
						PMREDrv,
						BOOL fOk,
						SZC szFileSrc,
						SZC szFileTarg
						);
#if !defined(NO_YNM)
MREAPI ( YNM )		MREDrvYnmFileOutOfDate ( PMREDrv, SRCTARG * );
#endif

MREAPI ( BOOL )		MREDrvFFilesOutOfDate ( PMREDrv, PCAList pCAList );
MREAPI ( BOOL )		MREDrvFUpdateTargetFile ( PMREDrv, SZC szTrg, TrgType );
MREAPI ( void )		MREDrvOneTimeInit ( PMREDrv );

// C interface for MRECmp
MREAPI ( BOOL )		MRECmpFRelease ( PMRECmp );
MREAPI ( BOOL )		MRECmpFOpenCompiland (
						PMRECmp,
						OUT PMREFile *	ppmrefile,
						IN SZC			szFileSrc,
						IN SZC			szFileTarg
						);
MREAPI ( BOOL )		MRECmpFCloseCompiland ( PMRECmp, PMREFile, BOOL fCommit );
MREAPI ( BOOL )		MRECmpFPushFile ( 
						PMRECmp,
						OUT PMREFile *	ppmrefile,
						IN SZC			szFile,
						IN HANDLE		hfile
						);
MREAPI ( PMREFile )	MRECmpPmrefilePopFile ( PMRECmp ) ;
MREAPI ( BOOL )		MRECmpFStoreDepData ( PMRECmp, PDepData );
MREAPI ( BOOL )		MRECmpFRestoreDepData ( PMRECmp, PDepData );
MREAPI ( void )		MRECmpClassIsBoring ( PMRECmp, NI );



// C interfaces for MREFile
MREAPI ( BOOL )		MREFileFOpenBag ( PMREFile, OUT PMREBag *, NI );

// C interfaces for MREBag
MREAPI ( BOOL )		MREBagFAddDep (
						PMREBag,
						NI niDep,
						TI tiDep,
						SZC szMemberName,
						DEPON depon
						);
MREAPI ( BOOL )		MREBagFClose ( PMREBag );

#if defined(__cplusplus)
}	// extern "C"
#endif

#endif	// _mrengine_h
