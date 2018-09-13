// ILStore API
// Copyright (C) 1994, Microsoft Corp.  All Rights Reserved.

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef __ILSTORE_INCLUDED__
#define __ILSTORE_INCLUDED__

#ifndef __PDB_INCLUDED__
#include "pdb.h"
#endif

typedef unsigned long KEY;		// temporary
typedef unsigned long ILSig; 	// signature of an ILStream
typedef unsigned char ILSType;	// ILStream type: GL, EX, SY, IN, etc.
typedef unsigned char ILSpace;	// storage domain hint: module, shared, PCH, etc.
typedef unsigned short ILVer;	// version (no. of times changed) of an IL contribution

enum {
	ilstypeNil,
	ilstypeGL,
	ilstypeEX,
	ilstypeSY,
	ilstypeIN,
	ilstypeEEA
};

enum {
	ilspaceNil,
	ilspaceMod,
	ilspaceShared,
};

enum {
	ilverNil,
	ilverNew,
	ilverMax = 65535
};

struct ILStore;
struct ILMod;
struct Buf;
typedef struct ILStore ILStore;
typedef struct ILMod ILMod;
typedef struct Buf Buf;

#if defined(__cplusplus)

interface ILStore {
	static PDBAPI(BOOL) open(PDB* ppdb, BOOL write, OUT ILStore** pilstore);
	virtual BOOL release() pure;

	virtual BOOL reset() pure;
	virtual BOOL getILMod(const char* szModule, OUT ILMod** ppilmod) pure;
	virtual BOOL getEnumILModNames(OUT EnumNameMap** ppenum) pure;

	virtual BOOL getILSType(const char* szILSType, OUT ILSType* pilstype) pure;
	// virtual BOOL getILSTypeMap(const char* szILSType, OUT const NameMap** ppnmILSType) pure;
	virtual BOOL getILSpace(const char* szILSpace, OUT ILSpace* pilspace) pure;
	// virtual BOOL getILSpaceMap(const char* szILSpace, OUT const NameMap** ppnmILSpace) pure;
#ifdef _DEBUG
	virtual BOOL getInfo( OUT long *pcStreamSz,
		OUT long *pcTotalILU,	OUT ULONG *pnumberOfILU,
		OUT long *pcTotShILU=NULL, OUT ULONG *pNumSharedILU=NULL ) pure;
#endif
};

interface EnumKeyType : Enum {
	virtual void get(OUT KEY* pkey, OUT ILSType *pilt, OUT ILSpace *pils) pure;
};

interface ILMod {
	virtual BOOL release() pure;

	virtual BOOL reset() pure;
	virtual BOOL getIL(KEY key, ILSType ilstype, OUT Buf *pbuf, OUT SIG* psig) pure;
	virtual BOOL getILVer(KEY key, OUT ILVer* pilver) pure;
	virtual BOOL putIL(KEY key, ILSType ilstype, Buf buf, ILSpace ilspace) pure;
	virtual BOOL getEnumILKT(OUT EnumKeyType** ppenum) pure;
	// virtual BOOL getEnumILStreams(ILSpace ilspace, OUT EnumStreams** ppenum) pure;
	// gets information and checks for inconsistencies

	// only succeeds in debug builds
	virtual BOOL getInfo( OUT long *pcStreamSz, 
		OUT long *pcTotalILU,	OUT ULONG *pnumberOfILU,
		OUT long *pcTotShILU=NULL, OUT ULONG *pNumSharedILU=NULL ) pure;

	virtual BOOL getAllIL(ILSType ilstype, OUT Buf* pbuf) pure;
	virtual BOOL deleteIL(KEY key, ILSType ilstype) pure;
};
#endif // __cplusplus

struct Buf {
	BYTE* pb;
	long cb;
#ifdef __cplusplus
	Buf() { }
	Buf(BYTE* pb_, long cb_) {
		pb = pb_;
		cb = cb_;
	}
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

PDBAPI(BOOL) ILStoreOpen(PDB* ppdb, BOOL write, OUT ILStore** pilstore);
PDBAPI(BOOL) ILStoreRelease(ILStore* pilstore);
PDBAPI(BOOL) ILStoreReset(ILStore* pilstore);
PDBAPI(BOOL) ILStoreGetILMod(ILStore* pilstore, const char* szModule, OUT ILMod** ppilmod);
PDBAPI(BOOL) ILStoreGetILSType(ILStore* pilstore, const char* szILSType, OUT ILSType* pilstype);
PDBAPI(BOOL) ILStoreGetILSpace(ILStore* pilstore, const char* szILSpace, OUT ILSpace* pilspace);

PDBAPI(BOOL) ILModRelease(ILMod* pilmod);
PDBAPI(BOOL) ILModReset(ILMod* pilmod);
PDBAPI(BOOL) ILModGetIL(ILMod* pilmod, KEY key, ILSType ilstype, OUT Buf *pbuf, OUT SIG* psig);
PDBAPI(BOOL) ILModGetAllIL(ILMod* pilmod, ILSType ilstype, OUT Buf *pbuf);
PDBAPI(BOOL) ILModGetILVer(ILMod* pilmod, KEY key, OUT ILVer* pilver);
PDBAPI(BOOL) ILModPutIL(ILMod* pilmod, KEY key, ILSType ilstype, Buf buf, ILSpace ilspace);

#ifdef __cplusplus
};
#endif

#endif //!__ILSTORE_INCLUDED__
