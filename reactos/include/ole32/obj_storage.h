/*
 * Defines the COM interfaces and APIs related to structured data storage.
 */

#ifndef __WINE_WINE_OBJ_STORAGE_H
#define __WINE_WINE_OBJ_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/*****************************************************************************
 * Predeclare the structures
 */
typedef enum tagLOCKTYPE
{
	LOCK_WRITE = 1,
	LOCK_EXCLUSIVE = 2,
	LOCK_ONLYONCE = 4
} LOCKTYPE;

typedef struct tagStorageLayout
{
    DWORD LayoutType;
    OLECHAR16* pwcsElementName;
    LARGE_INTEGER cOffset;
    LARGE_INTEGER cBytes;
} StorageLayout;

typedef struct tagSTATSTG {
    LPOLESTR	pwcsName;
    DWORD	type;
    ULARGE_INTEGER cbSize;
    FILETIME	mtime;
    FILETIME	ctime;
    FILETIME	atime;
    DWORD	grfMode;
    DWORD	grfLocksSupported;
    CLSID	clsid;
    DWORD	grfStateBits;
    DWORD	reserved;
} STATSTG;

typedef struct tagSTATSTG16 {
    LPOLESTR16	pwcsName;
    DWORD	type;
    ULARGE_INTEGER cbSize;
    FILETIME	mtime;
    FILETIME	ctime;
    FILETIME	atime;
    DWORD	grfMode;
    DWORD	grfLocksSupported;
    CLSID	clsid;
    DWORD	grfStateBits;
    DWORD	reserved;
} STATSTG16;

typedef LPOLESTR16 *SNB16;
typedef LPOLESTR *SNB;


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IEnumSTATSTG,	0x0000000dL, 0, 0);
typedef struct IEnumSTATSTG IEnumSTATSTG,*LPENUMSTATSTG;

DEFINE_GUID   (IID_IFillLockBytes,	0x99caf010L, 0x415e, 0x11cf, 0x88, 0x14, 0x00, 0xaa, 0x00, 0xb5, 0x69, 0xf5);
typedef struct IFillLockBytes IFillLockBytes,*LPFILLLOCKBYTES;

DEFINE_GUID   (IID_ILayoutStorage,	0x0e6d4d90L, 0x6738, 0x11cf, 0x96, 0x08, 0x00, 0xaa, 0x00, 0x68, 0x0d, 0xb4);
typedef struct ILayoutStorage ILayoutStorage,*LPLAYOUTSTORAGE;

DEFINE_OLEGUID(IID_ILockBytes,		0x0000000aL, 0, 0);
typedef struct ILockBytes ILockBytes,*LPLOCKBYTES;

DEFINE_OLEGUID(IID_IPersist,		0x0000010cL, 0, 0);
typedef struct IPersist IPersist,*LPPERSIST;

DEFINE_OLEGUID(IID_IPersistFile,	0x0000010bL, 0, 0);
typedef struct IPersistFile IPersistFile,*LPPERSISTFILE;

DEFINE_OLEGUID(IID_IPersistStorage,	0x0000010aL, 0, 0);
typedef struct IPersistStorage IPersistStorage,*LPPERSISTSTORAGE;

DEFINE_OLEGUID(IID_IPersistStream,	0x00000109L, 0, 0);
typedef struct IPersistStream IPersistStream,*LPPERSISTSTREAM;

DEFINE_GUID   (IID_IProgressNotify,	0xa9d758a0L, 0x4617, 0x11cf, 0x95, 0xfc, 0x00, 0xaa, 0x00, 0x68, 0x0d, 0xb4);
typedef struct IProgressNotify IProgressNotify,*LPPROGRESSNOTIFY;

DEFINE_OLEGUID(IID_IRootStorage,	0x00000012L, 0, 0);
typedef struct IRootStorage IRootStorage,*LPROOTSTORAGE;

DEFINE_GUID   (IID_ISequentialStream,	0x0c733a30L, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
typedef struct ISequentialStream ISequentialStream,*LPSEQUENTIALSTREAM;

DEFINE_OLEGUID(IID_IStorage,		0x0000000bL, 0, 0);
typedef struct IStorage16 IStorage16,*LPSTORAGE16;
typedef struct IStorage IStorage,*LPSTORAGE;

DEFINE_OLEGUID(IID_IStream,		0x0000000cL, 0, 0);
typedef struct IStream16 IStream16,*LPSTREAM16;
typedef struct IStream IStream,*LPSTREAM;


/*****************************************************************************
 * STGM enumeration
 *
 * See IStorage and IStream
 */
#define STGM_DIRECT		0x00000000
#define STGM_TRANSACTED		0x00010000
#define STGM_SIMPLE		0x08000000
#define STGM_READ		0x00000000
#define STGM_WRITE		0x00000001
#define STGM_READWRITE		0x00000002
#define STGM_SHARE_DENY_NONE	0x00000040
#define STGM_SHARE_DENY_READ	0x00000030
#define STGM_SHARE_DENY_WRITE	0x00000020
#define STGM_SHARE_EXCLUSIVE	0x00000010
#define STGM_PRIORITY		0x00040000
#define STGM_DELETEONRELEASE	0x04000000
#define STGM_CREATE		0x00001000
#define STGM_CONVERT		0x00020000
#define STGM_FAILIFTHERE	0x00000000
#define STGM_NOSCRATCH		0x00100000
#define STGM_NOSNAPSHOT		0x00200000

/*****************************************************************************
 * STGTY enumeration
 *
 * See IStorage
 */
#define STGTY_STORAGE 1
#define STGTY_STREAM  2
#define STGTY_LOCKBYTES 3
#define STGTY_PROPERTY  4

/*****************************************************************************
 * STATFLAG enumeration
 *
 * See IStorage and IStream
 */
#define STATFLAG_DEFAULT 0
#define STATFLAG_NONAME  1

/*****************************************************************************
 * STREAM_SEEK enumeration
 *
 * See IStream
 */
#define STREAM_SEEK_SET 0
#define STREAM_SEEK_CUR 1
#define STREAM_SEEK_END 2

/*****************************************************************************
 * IEnumSTATSTG interface
 */
#define ICOM_INTERFACE IEnumSTATSTG
#define IEnumSTATSTG_METHODS \
    ICOM_METHOD3(HRESULT,Next,  ULONG,celt, STATSTG*,rgelt, ULONG*,pceltFethed) \
    ICOM_METHOD1(HRESULT,Skip,  ULONG,celt) \
    ICOM_METHOD (HRESULT,Reset) \
    ICOM_METHOD1(HRESULT,Clone, IEnumSTATSTG**,ppenum)
#define IEnumSTATSTG_IMETHODS \
    IUnknown_IMETHODS \
    IEnumSTATSTG_METHODS
ICOM_DEFINE(IEnumSTATSTG,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IEnumSTATSTG_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IEnumSTATSTG_AddRef(p)             ICOM_CALL (AddRef,p)
#define IEnumSTATSTG_Release(p)            ICOM_CALL (Release,p)
/*** IEnumSTATSTG methods ***/
#define IEnumSTATSTG_Next(p,a,b,c)         ICOM_CALL3(Next,p,a,b,c)
#define IEnumSTATSTG_Skip(p,a)             ICOM_CALL1(Skip,p,a)
#define IEnumSTATSTG_Reset(p)              ICOM_CALL(Reset,p)
#define IEnumSTATSTG_Clone(p,a)            ICOM_CALL1(Clone,p,a)


/*****************************************************************************
 * IFillLockBytes interface
 */
#define ICOM_INTERFACE IFillLockBytes
#define IFillLockBytes_METHODS \
    ICOM_METHOD3(HRESULT,FillAppend,  const void*,pv, ULONG,cb, ULONG*,pcbWritten) \
    ICOM_METHOD4(HRESULT,FillAt,      ULARGE_INTEGER,ulOffset, const void*,pv, ULONG,cb, ULONG*,pcbWritten) \
    ICOM_METHOD1(HRESULT,SetFillSize, ULARGE_INTEGER,ulSize) \
    ICOM_METHOD1(HRESULT,Terminate,   BOOL,bCanceled)
#define IFillLockBytes_IMETHODS \
    IUnknown_IMETHODS \
    IFillLockBytes_METHODS
ICOM_DEFINE(IFillLockBytes,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IFillLockBytes_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IFillLockBytes_AddRef(p)             ICOM_CALL (AddRef,p)
#define IFillLockBytes_Release(p)            ICOM_CALL (Release,p)
/*** IFillLockBytes methods ***/
#define IFillLockBytes_FillAppend(p,a,b,c) ICOM_CALL3(FillAppend,p,a,b,c)
#define IFillLockBytes_FillAt(p,a,b,c,d)   ICOM_CALL4(FillAt,p,a,b,c,d)
#define IFillLockBytes_SetFillSize(p,a)    ICOM_CALL1(SetFillSize,p,a)
#define IFillLockBytes_Terminate(p,a)      ICOM_CALL1(Terminate,p,a)


/*****************************************************************************
 * ILayoutStorage interface
 */
#define ICOM_INTERFACE ILayoutStorage
#define ILayoutStorage_METHODS \
    ICOM_METHOD2(HRESULT,LayoutScript,                DWORD,nEntries, DWORD,glfInterleavedFlag) \
    ICOM_METHOD (HRESULT,BeginMonitor) \
    ICOM_METHOD (HRESULT,EndMonitor) \
    ICOM_METHOD1(HRESULT,ReLayoutDocfile,             OLECHAR16*,pwcsNewDfName) \
    ICOM_METHOD1(HRESULT,ReLayoutDocfileOnILockBytes, ILockBytes*,pILockBytes)
#define ILayoutStorage_IMETHODS \
    IUnknown_IMETHODS \
    ILayoutStorage_METHODS
ICOM_DEFINE(ILayoutStorage,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ILayoutStorage_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define ILayoutStorage_AddRef(p)             ICOM_CALL (AddRef,p)
#define ILayoutStorage_Release(p)            ICOM_CALL (Release,p)
/*** ILayoutStorage methods ***/
#define ILayoutStorage_LayoutScript(p,a,b)              ICOM_CALL2(LayoutScript,p,a,b)
#define ILayoutStorage_BeginMonitor(p)                  ICOM_CALL (BeginMonitor,p)
#define ILayoutStorage_EndMonitor(p)                    ICOM_CALL (EndMonitor,p)
#define ILayoutStorage_ReLayoutDocfile(p,a)             ICOM_CALL1(ReLayoutDocfile,p,a)
#define ILayoutStorage_ReLayoutDocfileOnILockBytes(p,a) ICOM_CALL1(ReLayoutDocfileOnILockBytes,p,a)


/*****************************************************************************
 * ILockBytes interface
 */
#define ICOM_INTERFACE ILockBytes
#define ILockBytes_METHODS \
    ICOM_METHOD4(HRESULT,ReadAt,       ULARGE_INTEGER,ulOffset, void*,pv, ULONG,cb, ULONG*,pcbRead) \
    ICOM_METHOD4(HRESULT,WriteAt,      ULARGE_INTEGER,ulOffset, const void*,pv, ULONG,cb, ULONG*,pcbWritten) \
    ICOM_METHOD (HRESULT,Flush) \
    ICOM_METHOD1(HRESULT,SetSize,      ULARGE_INTEGER,cb) \
    ICOM_METHOD3(HRESULT,LockRegion,   ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType) \
    ICOM_METHOD3(HRESULT,UnlockRegion, ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType) \
    ICOM_METHOD2(HRESULT,Stat,         STATSTG*,pstatstg, DWORD,grfStatFlag)
#define ILockBytes_IMETHODS \
    IUnknown_IMETHODS \
    ILockBytes_METHODS
ICOM_DEFINE(ILockBytes,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ILockBytes_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define ILockBytes_AddRef(p)             ICOM_CALL (AddRef,p)
#define ILockBytes_Release(p)            ICOM_CALL (Release,p)
/*** ILockBytes methods ***/
#define ILockBytes_ReadAt(p,a,b,c,d)     ICOM_CALL4(ReadAt,p,a,b,c,d)
#define ILockBytes_WriteAt(p,a,b,c,d)    ICOM_CALL4(WriteAt,p,a,b,c,d)
#define ILockBytes_Flush(p)              ICOM_CALL (Flush,p)
#define ILockBytes_SetSize(p,a)          ICOM_CALL1(SetSize,p,a)
#define ILockBytes_LockRegion(p,a,b,c)   ICOM_CALL3(LockRegion,p,a,b,c)
#define ILockBytes_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define ILockBytes_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)


/*****************************************************************************
 * IPersist interface
 */
#define ICOM_INTERFACE IPersist
#define IPersist_METHODS \
    ICOM_METHOD1(HRESULT,GetClassID, CLSID*,pClassID)
#define IPersist_IMETHODS \
    IUnknown_IMETHODS \
    IPersist_METHODS
ICOM_DEFINE(IPersist,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersist_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersist_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersist_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersist_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)


/*****************************************************************************
 * IPersistFile interface
 */
#define ICOM_INTERFACE IPersistFile
#define IPersistFile_METHODS \
    ICOM_METHOD (HRESULT,IsDirty) \
    ICOM_METHOD2 (HRESULT,Load,          LPCOLESTR,pszFileName, DWORD,dwMode) \
    ICOM_METHOD2 (HRESULT,Save,          LPCOLESTR,pszFileName, BOOL,fRemember) \
    ICOM_METHOD1 (HRESULT,SaveCompleted, LPCOLESTR,pszFileName) \
    ICOM_METHOD1(HRESULT,GetCurFile,    LPOLESTR*,ppszFileName)
#define IPersistFile_IMETHODS \
    IPersist_IMETHODS \
    IPersistFile_METHODS
ICOM_DEFINE(IPersistFile,IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistFile_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistFile_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistFile_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistFile_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
/*** IPersistFile methods ***/
#define IPersistFile_IsDirty(p)         ICOM_CALL(IsDirty,p)
#define IPersistFile_Load(p,a,b)        ICOM_CALL2(Load,p,a,b)
#define IPersistFile_Save(p,a,b)        ICOM_CALL2(Save,p,a,b)
#define IPersistFile_SaveCompleted(p,a) ICOM_CALL1(SaveCompleted,p,a)
#define IPersistFile_GetCurFile(p,a)    ICOM_CALL1(GetCurFile,p,a)


/*****************************************************************************
 * IPersistStorage interface
 */
#define ICOM_INTERFACE IPersistStorage
#define IPersistStorage_METHODS \
    ICOM_METHOD (HRESULT,IsDirty) \
    ICOM_METHOD1(HRESULT,InitNew,        IStorage*,pStg) \
    ICOM_METHOD1(HRESULT,Load,           IStorage*,pStg) \
    ICOM_METHOD2(HRESULT,Save,           IStorage*,pStg, BOOL,fSameAsLoad) \
    ICOM_METHOD1(HRESULT,SaveCompleted,  IStorage*,pStgNew) \
		ICOM_METHOD (HRESULT,HandsOffStorage)
#define IPersistStorage_IMETHODS \
    IPersist_IMETHODS \
    IPersistStorage_METHODS
ICOM_DEFINE(IPersistStorage,IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistStorage_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistStorage_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistStorage_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistStorage_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
/*** IPersistStorage methods ***/
#define IPersistStorage_IsDirty(p)         ICOM_CALL (IsDirty,p)
#define IPersistStorage_InitNew(p,a)			 ICOM_CALL1(InitNew,p,a)
#define IPersistStorage_Load(p,a)          ICOM_CALL1(Load,p,a)
#define IPersistStorage_Save(p,a,b)        ICOM_CALL2(Save,p,a,b)
#define IPersistStorage_SaveCompleted(p,a) ICOM_CALL1(SaveCompleted,p,a)
#define IPersistStorage_HandsOffStorage(p) ICOM_CALL (HandsOffStorage,p)


/*****************************************************************************
 * IPersistStream interface
 */
#define ICOM_INTERFACE IPersistStream
#define IPersistStream_METHODS \
    ICOM_METHOD (HRESULT,IsDirty) \
    ICOM_METHOD1(HRESULT,Load,       IStream*,pStm) \
    ICOM_METHOD2(HRESULT,Save,       IStream*,pStm, BOOL,fClearDirty) \
    ICOM_METHOD1(HRESULT,GetSizeMax, ULARGE_INTEGER*,pcbSize)
#define IPersistStream_IMETHODS \
    IPersist_IMETHODS \
    IPersistStream_METHODS
ICOM_DEFINE(IPersistStream,IPersist)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IPersistStream_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IPersistStream_AddRef(p)             ICOM_CALL (AddRef,p)
#define IPersistStream_Release(p)            ICOM_CALL (Release,p)
/*** IPersist methods ***/
#define IPersistStream_GetClassID(p,a) ICOM_CALL1(GetClassID,p,a)
/*** IPersistStream methods ***/
#define IPersistStream_IsDirty(p)      ICOM_CALL (IsDirty,p)
#define IPersistStream_Load(p,a)       ICOM_CALL1(Load,p,a)
#define IPersistStream_Save(p,a,b)     ICOM_CALL2(Save,p,a,b)
#define IPersistStream_GetSizeMax(p,a) ICOM_CALL1(GetSizeMax,p,a)


/*****************************************************************************
 * IProgressNotify interface
 */
#define ICOM_INTERFACE IProgressNotify
#define IProgressNotify_METHODS \
    ICOM_METHOD4(HRESULT,OnProgress, DWORD,dwProgressCurrent, DWORD,dwProgressMaximum, BOOL,fAccurate, BOOL,fOwner)
#define IProgressNotify_IMETHODS \
    IUnknown_IMETHODS \
    IProgressNotify_METHODS
ICOM_DEFINE(IProgressNotify,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IProgressNotify_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IProgressNotify_AddRef(p)             ICOM_CALL (AddRef,p)
#define IProgressNotify_Release(p)            ICOM_CALL (Release,p)
/*** IProgressNotify methods ***/
#define IProgressNotify_OnProgress(p,a,b,c,d) ICOM_CALL4(OnProgress,p,a,b,c,d)


/*****************************************************************************
 * IRootStorage interface
 */
#define ICOM_INTERFACE IRootStorage
#define IRootStorage_METHODS \
    ICOM_METHOD1(HRESULT,SwitchToFile, LPOLESTR,pszFile)
#define IRootStorage_IMETHODS \
    IUnknown_IMETHODS \
    IRootStorage_METHODS
ICOM_DEFINE(IRootStorage,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IRootStorage_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IRootStorage_AddRef(p)             ICOM_CALL (AddRef,p)
#define IRootStorage_Release(p)            ICOM_CALL (Release,p)
/*** IRootStorage methods ***/
#define IRootStorage_SwitchToFile(p,a) ICOM_CALLSwitchToFile(,p,a)


/*****************************************************************************
 * ISequentialStream interface
 */
#define ICOM_INTERFACE ISequentialStream
#define ISequentialStream_METHODS \
    ICOM_METHOD3(HRESULT,Read,  void*,pv, ULONG,cb, ULONG*,pcbRead) \
    ICOM_METHOD3(HRESULT,Write, const void*,pv, ULONG,cb, ULONG*,pcbWritten)
#define ISequentialStream_IMETHODS \
    IUnknown_IMETHODS \
    ISequentialStream_METHODS
ICOM_DEFINE(ISequentialStream,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ISequentialStream_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define ISequentialStream_AddRef(p)             ICOM_CALL (AddRef,p)
#define ISequentialStream_Release(p)            ICOM_CALL (Release,p)
/*** ISequentialStream methods ***/
#define ISequentialStream_Read(p,a,b,c)  ICOM_CALL3(Read,p,a,b,c)
#define ISequentialStream_Write(p,a,b,c) ICOM_CALL3(Write,p,a,b,c)


/*****************************************************************************
 * IStorage interface
 */
#define ICOM_INTERFACE IStorage16
#define IStorage16_METHODS \
    ICOM_METHOD5(HRESULT,CreateStream,   LPCOLESTR16,pwcsName, DWORD,grfMode, DWORD,reserved1, DWORD,reserved2, IStream16**,ppstm) \
    ICOM_METHOD5(HRESULT,OpenStream,     LPCOLESTR16,pwcsName, void*,reserved1, DWORD,grfMode, DWORD,reserved2, IStream16**,ppstm) \
    ICOM_METHOD5(HRESULT,CreateStorage,  LPCOLESTR16,pwcsName, DWORD,grfMode, DWORD,dwStgFmt, DWORD,reserved2, IStorage16**,ppstg) \
    ICOM_METHOD6(HRESULT,OpenStorage,    LPCOLESTR16,pwcsName, IStorage16*,pstgPriority, DWORD,grfMode, SNB16,snb16Exclude, DWORD,reserved, IStorage16**,ppstg) \
    ICOM_METHOD4(HRESULT,CopyTo,         DWORD,ciidExclude, const IID*,rgiidExclude, SNB16,snb16Exclude, IStorage16*,pstgDest) \
    ICOM_METHOD4(HRESULT,MoveElementTo,  LPCOLESTR16,pwcsName, IStorage16*,pstgDest, LPCOLESTR16,pwcsNewName, DWORD,grfFlags) \
    ICOM_METHOD1(HRESULT,Commit,         DWORD,grfCommitFlags) \
    ICOM_METHOD (HRESULT,Revert) \
    ICOM_METHOD4(HRESULT,EnumElements,   DWORD,reserved1, void*,reserved2, DWORD,reserved3, IEnumSTATSTG**,ppenum) \
    ICOM_METHOD1(HRESULT,DestroyElement, LPCOLESTR16,pwcsName) \
    ICOM_METHOD2(HRESULT,RenameElement,  LPCOLESTR16,pwcsOldName, LPOLESTR16,pwcsNewName) \
    ICOM_METHOD4(HRESULT,SetElementTimes,LPCOLESTR16,pwcsName, const FILETIME*,pctime, const FILETIME*,patime, const FILETIME*,pmtime) \
    ICOM_METHOD1(HRESULT,SetClass,       REFCLSID,clsid) \
    ICOM_METHOD2(HRESULT,SetStateBits,   DWORD,grfStateBits, DWORD,grfMask) \
    ICOM_METHOD2(HRESULT,Stat,           STATSTG*,pstatstg, DWORD,grfStatFlag)
#define IStorage16_IMETHODS \
    IUnknown_IMETHODS \
    IStorage16_METHODS
ICOM_DEFINE(IStorage16,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IStorage16_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IStorage16_AddRef(p)             ICOM_CALL (AddRef,p)
#define IStorage16_Release(p)            ICOM_CALL (Release,p)
/*** IStorage16 methods ***/
#define IStorage16_CreateStream(p,a,b,c,d,e)  ICOM_CALL5(CreateStream,p,a,b,c,d,e)
#define IStorage16_OpenStream(p,a,b,c,d,e)    ICOM_CALL5(OpenStream,p,a,b,c,d,e)
#define IStorage16_CreateStorage(p,a,b,c,d,e) ICOM_CALL5(CreateStorage,p,a,b,c,d,e)
#define IStorage16_OpenStorage(p,a,b,c,d,e,f) ICOM_CALL6(OpenStorage,p,a,b,c,d,e,f)
#define IStorage16_CopyTo(p,a,b,c,d)          ICOM_CALL4(CopyTo,p,a,b,c,d)
#define IStorage16_MoveElementTo(p,a,b,c,d)   ICOM_CALL4(MoveElementTo,p,a,b,c,d)
#define IStorage16_Commit(p,a)                ICOM_CALL1(Commit,p,a)
#define IStorage16_Revert(p)                  ICOM_CALL (Revert,p)
#define IStorage16_EnumElements(p,a,b,c,d)    ICOM_CALL4(EnumElements,p,a,b,c,d)
#define IStorage16_DestroyElement(p,a)        ICOM_CALL1(DestroyElement,p,a)
#define IStorage16_RenameElement(p,a,b)       ICOM_CALL2(RenameElement,p,a,b)
#define IStorage16_SetElementTimes(p,a,b,c,d) ICOM_CALL4(SetElementTimes,p,a,b,c,d)
#define IStorage16_SetClass(p,a)              ICOM_CALL1(SetClass,p,a)
#define IStorage16_SetStateBits(p,a,b)        ICOM_CALL2(SetStateBits,p,a,b)
#define IStorage16_Stat(p,a,b)                ICOM_CALL2(Stat,p,a,b)


#define ICOM_INTERFACE IStorage
#define IStorage_METHODS \
    ICOM_METHOD5(HRESULT,CreateStream,   LPCOLESTR,pwcsName, DWORD,grfMode, DWORD,reserved1, DWORD,reserved2, IStream**,ppstm) \
    ICOM_METHOD5(HRESULT,OpenStream,     LPCOLESTR,pwcsName, void*,reserved1, DWORD,grfMode, DWORD,reserved2, IStream**,ppstm) \
    ICOM_METHOD5(HRESULT,CreateStorage,  LPCOLESTR,pwcsName, DWORD,grfMode, DWORD,dwStgFmt, DWORD,reserved2, IStorage**,ppstg) \
    ICOM_METHOD6(HRESULT,OpenStorage,    LPCOLESTR,pwcsName, IStorage*,pstgPriority, DWORD,grfMode, SNB,snb16Exclude, DWORD,reserved, IStorage**,ppstg) \
    ICOM_METHOD4(HRESULT,CopyTo,         DWORD,ciidExclude, const IID*,rgiidExclude, SNB,snb16Exclude, IStorage*,pstgDest) \
    ICOM_METHOD4(HRESULT,MoveElementTo,  LPCOLESTR,pwcsName, IStorage*,pstgDest, LPCOLESTR,pwcsNewName, DWORD,grfFlags) \
    ICOM_METHOD1(HRESULT,Commit,         DWORD,grfCommitFlags) \
    ICOM_METHOD (HRESULT,Revert) \
    ICOM_METHOD4(HRESULT,EnumElements,   DWORD,reserved1, void*,reserved2, DWORD,reserved3, IEnumSTATSTG**,ppenum) \
    ICOM_METHOD1(HRESULT,DestroyElement, LPCOLESTR,pwcsName) \
    ICOM_METHOD2(HRESULT,RenameElement,  LPCOLESTR,pwcsOldName, LPCOLESTR,pwcsNewName) \
    ICOM_METHOD4(HRESULT,SetElementTimes,LPCOLESTR,pwcsName, const FILETIME*,pctime, const FILETIME*,patime, const FILETIME*,pmtime) \
    ICOM_METHOD1(HRESULT,SetClass,       REFCLSID,clsid) \
    ICOM_METHOD2(HRESULT,SetStateBits,   DWORD,grfStateBits, DWORD,grfMask) \
    ICOM_METHOD2(HRESULT,Stat,           STATSTG*,pstatstg, DWORD,grfStatFlag)
#define IStorage_IMETHODS \
    IUnknown_IMETHODS \
    IStorage_METHODS
ICOM_DEFINE(IStorage,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IStorage_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IStorage_AddRef(p)             ICOM_CALL (AddRef,p)
#define IStorage_Release(p)            ICOM_CALL (Release,p)
/*** IStorage methods ***/
#define IStorage_CreateStream(p,a,b,c,d,e)  ICOM_CALL5(CreateStream,p,a,b,c,d,e)
#define IStorage_OpenStream(p,a,b,c,d,e)    ICOM_CALL5(OpenStream,p,a,b,c,d,e)
#define IStorage_CreateStorage(p,a,b,c,d,e) ICOM_CALL5(CreateStorage,p,a,b,c,d,e)
#define IStorage_OpenStorage(p,a,b,c,d,e,f) ICOM_CALL6(OpenStorage,p,a,b,c,d,e,f)
#define IStorage_CopyTo(p,a,b,c,d)          ICOM_CALL4(CopyTo,p,a,b,c,d)
#define IStorage_MoveElementTo(p,a,b,c,d)   ICOM_CALL4(MoveElementTo,p,a,b,c,d)
#define IStorage_Commit(p,a)                ICOM_CALL1(Commit,p,a)
#define IStorage_Revert(p)                  ICOM_CALL (Revert,p)
#define IStorage_EnumElements(p,a,b,c,d)    ICOM_CALL4(EnumElements,p,a,b,c,d)
#define IStorage_DestroyElement(p,a)        ICOM_CALL1(DestroyElement,p,a)
#define IStorage_RenameElement(p,a,b)       ICOM_CALL2(RenameElement,p,a,b)
#define IStorage_SetElementTimes(p,a,b,c,d) ICOM_CALL4(SetElementTimes,p,a,b,c,d)
#define IStorage_SetClass(p,a)              ICOM_CALL1(SetClass,p,a)
#define IStorage_SetStateBits(p,a,b)        ICOM_CALL2(SetStateBits,p,a,b)
#define IStorage_Stat(p,a,b)                ICOM_CALL2(Stat,p,a,b)


/*****************************************************************************
 * IStream interface
 */
#define ICOM_INTERFACE IStream16
#define IStream16_METHODS \
    ICOM_METHOD3(HRESULT,Seek,        LARGE_INTEGER,dlibMove, DWORD,dwOrigin, ULARGE_INTEGER*,plibNewPosition) \
    ICOM_METHOD1(HRESULT,SetSize,     ULARGE_INTEGER,libNewSize) \
    ICOM_METHOD4(HRESULT,CopyTo,      IStream16*,pstm, ULARGE_INTEGER,cb, ULARGE_INTEGER*,pcbRead, ULARGE_INTEGER*,pcbWritten) \
    ICOM_METHOD1(HRESULT,Commit,      DWORD,grfCommitFlags) \
    ICOM_METHOD (HRESULT,Revert) \
    ICOM_METHOD3(HRESULT,LockRegion,  ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType) \
    ICOM_METHOD3(HRESULT,UnlockRegion,ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType) \
    ICOM_METHOD2(HRESULT,Stat,        STATSTG*,pstatstg, DWORD,grfStatFlag) \
    ICOM_METHOD1(HRESULT,Clone,       IStream16**,ppstm)
#define IStream16_IMETHODS \
    ISequentialStream_IMETHODS \
    IStream16_METHODS
ICOM_DEFINE(IStream16,ISequentialStream)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IStream16_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IStream16_AddRef(p)             ICOM_CALL (AddRef,p)
#define IStream16_Release(p)            ICOM_CALL (Release,p)
/*** ISequentialStream methods ***/
#define IStream16_Read(p,a,b,c)  ICOM_CALL3(Read,p,a,b,c)
#define IStream16_Write(p,a,b,c) ICOM_CALL3(Write,p,a,b,c)
/*** IStream16 methods ***/
#define IStream16_Seek(p)               ICOM_CALL3(Seek,p)
#define IStream16_SetSize(p,a,b)        ICOM_CALL1(SetSize,p,a,b)
#define IStream16_CopyTo(pa,b,c,d)      ICOM_CALL4(CopyTo,pa,b,c,d)
#define IStream16_Commit(p,a)           ICOM_CALL1(Commit,p,a)
#define IStream16_Revert(p)             ICOM_CALL (Revert,p)
#define IStream16_LockRegion(pa,b,c)    ICOM_CALL3(LockRegion,pa,b,c)
#define IStream16_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define IStream16_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)
#define IStream16_Clone(p,a)            ICOM_CALL1(Clone,p,a)


#define ICOM_INTERFACE IStream
#define IStream_METHODS \
    ICOM_METHOD3(HRESULT,Seek,        LARGE_INTEGER,dlibMove, DWORD,dwOrigin, ULARGE_INTEGER*,plibNewPosition) \
    ICOM_METHOD1(HRESULT,SetSize,     ULARGE_INTEGER,libNewSize) \
    ICOM_METHOD4(HRESULT,CopyTo,      IStream*,pstm, ULARGE_INTEGER,cb, ULARGE_INTEGER*,pcbRead, ULARGE_INTEGER*,pcbWritten) \
    ICOM_METHOD1(HRESULT,Commit,      DWORD,grfCommitFlags) \
    ICOM_METHOD (HRESULT,Revert) \
    ICOM_METHOD3(HRESULT,LockRegion,  ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType) \
    ICOM_METHOD3(HRESULT,UnlockRegion,ULARGE_INTEGER,libOffset, ULARGE_INTEGER,cb, DWORD,dwLockType) \
    ICOM_METHOD2(HRESULT,Stat,        STATSTG*,pstatstg, DWORD,grfStatFlag) \
    ICOM_METHOD1(HRESULT,Clone,       IStream**,ppstm)
#define IStream_IMETHODS \
    ISequentialStream_IMETHODS \
    IStream_METHODS
ICOM_DEFINE(IStream,ISequentialStream)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IStream_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IStream_AddRef(p)             ICOM_CALL (AddRef,p)
#define IStream_Release(p)            ICOM_CALL (Release,p)
/*** ISequentialStream methods ***/
#define IStream_Read(p,a,b,c)  ICOM_CALL3(Read,p,a,b,c)
#define IStream_Write(p,a,b,c) ICOM_CALL3(Write,p,a,b,c)
/*** IStream methods ***/
#define IStream_Seek(p,a,b,c)         ICOM_CALL3(Seek,p,a,b,c)
#define IStream_SetSize(p,a)          ICOM_CALL1(SetSize,p,a)
#define IStream_CopyTo(p,a,b,c,d)     ICOM_CALL4(CopyTo,p,a,b,c,d)
#define IStream_Commit(p,a)           ICOM_CALL1(Commit,p,a)
#define IStream_Revert(p)             ICOM_CALL (Revert,p)
#define IStream_LockRegion(p,a,b,c)   ICOM_CALL3(LockRegion,p,a,b,c)
#define IStream_UnlockRegion(p,a,b,c) ICOM_CALL3(UnlockRegion,p,a,b,c)
#define IStream_Stat(p,a,b)           ICOM_CALL2(Stat,p,a,b)
#define IStream_Clone(p,a)            ICOM_CALL1(Clone,p,a)


/*****************************************************************************
 * StgXXX API
 */
/* FIXME: many functions are missing */
HRESULT WINAPI StgCreateDocFile16(LPCOLESTR16 pwcsName,DWORD grfMode,DWORD reserved,IStorage16 **ppstgOpen);
HRESULT WINAPI StgCreateDocfile(LPCOLESTR pwcsName,DWORD grfMode,DWORD reserved,IStorage **ppstgOpen);

HRESULT WINAPI StgIsStorageFile16(LPCOLESTR16 fn);
HRESULT WINAPI StgIsStorageFile(LPCOLESTR fn);
HRESULT WINAPI StgIsStorageILockBytes(ILockBytes *plkbyt);

HRESULT WINAPI StgOpenStorage16(const OLECHAR16* pwcsName,IStorage16* pstgPriority,DWORD grfMode,SNB16 snbExclude,DWORD reserved,IStorage16**ppstgOpen);
HRESULT WINAPI StgOpenStorage(const OLECHAR* pwcsName,IStorage* pstgPriority,DWORD grfMode,SNB snbExclude,DWORD reserved,IStorage**ppstgOpen);

HRESULT WINAPI WriteClassStg(IStorage* pStg, REFCLSID rclsid);
HRESULT WINAPI ReadClassStg(IStorage *pstg,CLSID *pclsid);

HRESULT WINAPI WriteClassStm(IStream *pStm,REFCLSID rclsid);
HRESULT WINAPI ReadClassStm(IStream *pStm,REFCLSID rclsid);

HRESULT WINAPI StgCreateDocfileOnILockBytes(ILockBytes *plkbyt,DWORD grfMode, DWORD reserved, IStorage** ppstgOpen);
HRESULT WINAPI StgOpenStorageOnILockBytes(ILockBytes *plkbyt, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstgOpen);

/*****************************************************************************
 * Other storage API
 */

/* FIXME: not implemented */
BOOL WINAPI CoDosDateTimeToFileTime(WORD nDosDate, WORD nDosTime, FILETIME* lpFileTime);

/* FIXME: not implemented */
BOOL WINAPI CoFileTimeToDosDateTime(FILETIME* lpFileTime, WORD* lpDosDate, WORD* lpDosTime);

HRESULT WINAPI GetHGlobalFromILockBytes(ILockBytes* plkbyt, HGLOBAL* phglobal);

HRESULT WINAPI OleSaveToStream(IPersistStream *pPStm,IStream *pStm);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_STORAGE_H */
