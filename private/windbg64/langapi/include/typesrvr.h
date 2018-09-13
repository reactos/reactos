// Type server interface.

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#if (defined (_M_IX86) && (_M_IX86) < 300) || (_MSC_VER < 800)
#define PDBFAR	__far
#else
#define PDBFAR
#endif

typedef void PDBFAR*	TS;		// type server, opaque type
typedef unsigned short	TI;		// type index
typedef unsigned long	TSIG;	// type database signature
typedef unsigned long	TVER;	// type server version
typedef unsigned long	TAGE;	// type database age
typedef char PDBFAR*		PB;		// pointer to some bytes
typedef unsigned long	CB;		// count of bytes
typedef CB PDBFAR*	PCB;	// pointer to count of bytes
typedef PB				SZ;		// zero terminated string
typedef int				PDBBOOL;// Boolean

#define	tsNil			((TS)0)
#define	tiNil			((TI)0)

#define	tdbWrite				"w"
#define	tdbRead					"r"
#define	tdbGetTiOnly			"i"
#define	tdbGetRecordsOnly		"c"

#ifdef __cplusplus
extern "C" {
#endif

TVER	TypesQueryServerVersion(void);
TS		TypesOpen(SZ szDatabase, SZ szMode, TSIG sig);
TSIG	TypesQuerySignature(TS ts);
TAGE	TypesQueryAge(TS ts);
TI		TypesQueryTiForCVRecord(TS ts, PB pb);
PB		TypesQueryPbCVRecordForTi(TS ts, TI ti);
void	TypesQueryCVRecordForTi(TS ts, TI ti, PB pb, PCB pcb);
TI		TypesQueryTiMin(TS ts);
TI		TypesQueryTiMac(TS ts);
CB		TypesQueryCb(TS ts);
PDBBOOL	TypesCommit(TS ts);
PDBBOOL	TypesClose(TS ts);

#ifdef __cplusplus
};
#endif

struct THDR { // type database header:
	char	szMagic[0x2C];
	TVER	tver;			// version which created this file
	TSIG	tsig;			// signature
	TAGE	tage;			// age (no. of times written)
	TI		tiMin;			// lowest TI
	TI		tiMac;			// highest TI + 1
	CB		cb;				// count of bytes used by the gprec which follows.
	// rest of file is "REC gprec[];"
};
