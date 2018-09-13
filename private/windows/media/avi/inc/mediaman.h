/* MEDIAMAN.H
 * 
 * Public header for the MMSys media element manager DLL
 * 
 * include after windows.h
 * 
 */

#ifndef _MEDMAN_H_
#define _MEDMAN_H_

/***************   MEDIAMAN TYPES  ****************/

typedef	DWORD	MEDID;
typedef	WORD	MEDGID;
typedef	DWORD	FOURCC;
typedef	FOURCC	MEDTYPE;

typedef	WORD	MEDMSG;
typedef	WORD	MEDUSER;

/***************  MEDINFO INSTANCE BLOCK  ****************/
/*
 *  Resource instance structure, passed around as MEDINFO.
 *  This structure definition may change, DO NOT DEPEND ON IT, but
 *  use access macros defined below instead!!
 */
typedef struct _MedInfoStruct {
	DWORD	wFlags;		// maybe duplicate fpEnt flags
	WORD	wAccessCount;
	WORD	wLockCount;
	DWORD	dwAccessRet;
	DWORD	dwLockRet;
	char	achInst[2];
} MedInfoStruct;
typedef MedInfoStruct FAR *MEDINFO;

/*  Instance data access macros  */
#define medInfoInstance(medinfo)	((LPSTR) &((medinfo)->achInst[0]))
#define medInfoAccessCount(medinfo)	((medinfo)->wAccessCount)
#define medInfoLockCount(medinfo)	((medinfo)->wLockCount)
#define medInfoLockRead(medinfo)	((medinfo)->wFlags & 0x0800)
#define medInfoLockWrite(medinfo)	((medinfo)->wFlags & 0x0400)

#define MedInfoInstance(medinfo)	medInfoInstance(medinfo)
#define MedInfoAccessCount(medinfo)	medInfoAccessCount(medinfo)
#define MedInfoLockCount(medinfo)	medInfoLockCount(medinfo)
#define MedInfoLockRead(medinfo)	medInfoLockRead(medinfo)
#define MedInfoLockWrite(medinfo)	medInfoLockWrite(medinfo)

		

/**********  RESOURCE HANDLER DECLARATION  *************/

typedef DWORD (FAR PASCAL MedHandler)
		(MEDID medid, MEDMSG medmsg, MEDINFO medinfo,
		 LONG lParam1, LONG lParam2);
typedef MedHandler FAR *FPMedHandler;

/*  Logical resource handler messages  */
#define	MED_INIT		0x0010
#define MED_UNLOAD		0x0011
#define MED_LOCK		0x0012
#define MED_UNLOCK		0x0013
#define MED_EXPEL		0x0014
#define MED_DESTROY		0x0015
#define MED_CREATE		0x0016
#define MED_TYPEINIT		0x0020
#define MED_TYPEUNLOAD		0x0021
#define MED_SETPHYSICAL		0x0022
#define MED_COPY		0x0023
#define MED_NEWNAME		0x0024

#define MED_PAINT		0x002A
#define MED_REALIZEPALETTE	0x002B
#define MED_GETPAINTCAPS	0x002C
#define MED_GETCLIPBOARDDATA	0x002D

/*  PaintCaps flags  */
#define	MMC_PALETTEINFO		0x0001
#define MMC_BOUNDRECT		0x0002
#define MMC_CLIPFORMAT		0x0003

/*  Paint message flags  */
#define MMP_NORMPAL		0x0000
#define MMP_NOPALETTE		0x0001
#define MMP_PALBACKGROUND	0x0002
#define MMP_SHRINKTOFIT		0x0010
#define MMP_DSTANDSRCRECTS	0x0020

/* Load/Save messages */
#define MED_GETLOADPARAM	0x0030
#define MED_PRELOAD		0x0031
#define MED_LOAD		0x0032
#define MED_POSTLOAD		0x0033
#define MED_FREELOADPARAM	0x0034
#define MED_GETSAVEPARAM	0x0035
#define MED_PRESAVE		0x0036
#define MED_SAVE		0x0037
#define MED_POSTSAVE		0x0038
#define MED_FREESAVEPARAM	0x0039

/*  Sent to resource users  */
#define	MED_CHANGE		0x0060

/* Messages sent to MedDiskInfoCallback */
#define MED_DISKCBBEGIN		0x0065
#define MED_DISKCBUPDATE	0x0066
#define MED_DISKCBEND		0x0067
#define MED_DISKCBNEWCONV	0x0068

/*  Minimum value available for type-defined messages  */
#define MED_USER		0x0200



/*****************  RESOURCE LOAD/SAVE  *******************/

typedef struct _MedDisk {
	WORD	wFlags;
	DWORD	dwMessageData;
	DWORD	dwRetVal;

	DWORD	dwInstance1;
	DWORD	dwInstance2;

	DWORD	dwParam1;
	DWORD	dwParam2;

	DWORD	dwCbInstance1;
	DWORD	dwCbInstance2;

	HWND	hwndParentWindow;
	MEDID	medid;

	DWORD	dwReserved1;
	DWORD	dwReserved2;
	DWORD	dwReserved3;
	DWORD	dwReserved4;
} MedDisk;
typedef MedDisk	FAR	*FPMedDisk;
typedef MedDisk	NEAR	*NPMedDisk;

/*  Flags for wFlags field  */
#define MEDF_DISKSAVE	0x0001		// save taking place
#define MEDF_DISKLOAD	0x0002		// load taking place
#define MEDF_DISKVERIFY	0x0004		// verify the file format on load

typedef HANDLE	HMEDIABATCH;

/* Macros to check status of load/save */
#define medIsDiskVerify(FPDISK)		\
	(((FPMedDisk) (FPDISK))->wFlags & MEDF_DISKVERIFY)
#define medIsDiskLoad(FPDISK)		\
	(((FPMedDisk) (FPDISK))->wFlags & MEDF_DISKLOAD)
#define medIsDiskSave(FPDISK)		\
	(((FPMedDisk) (FPDISK))->wFlags & MEDF_DISKSAVE)

/* Values returned by physical handlers to MED_LOAD & MED_SAVE messages */
#define MEDF_OK		1
#define MEDF_ABORT	2
#define MEDF_ERROR	3
#define MEDF_BADFORMAT	4
#define MEDF_NOTPROCESSED	0L

/* Callback type used for information on disk save/load status */
typedef WORD (FAR PASCAL MedDiskInfoCallback)
		(WORD wmsg, FPMedDisk fpDisk, LONG lParam,
		 WORD wPercentDone, LPSTR lpszTextStatus);
typedef MedDiskInfoCallback FAR *FPMedDiskInfoCallback;


/*  Functions used by info callbacks  */
void	FAR PASCAL	medDiskCancel(FPMedDisk fpDisk);
WORD	FAR PASCAL	medUpdateProgress(FPMedDisk fpDisk,
				WORD wPercentDone, LPSTR lpszTextStatus);


/*************  LOGICAL I/O ROUTINES  *****************/

typedef struct _MedReturn {
	MEDID	medid;
	DWORD	dwReturn;
} MedReturn;
typedef MedReturn FAR *FPMedReturn;
 
WORD FAR PASCAL medSave(MEDID medid, LONG lParam,
			BOOL fYield, FPMedDiskInfoCallback lpfnCb,
			LONG lParamCb);
WORD FAR PASCAL medSaveAs(MEDID medid, FPMedReturn medReturn,
			LPSTR lpszName, LONG lParam, BOOL fYield,
			FPMedDiskInfoCallback lpfnCb, LONG lParamCb);
WORD FAR PASCAL medAccess(MEDID medid, LONG lParam,
			FPMedReturn medReturn, BOOL fYield,
			FPMedDiskInfoCallback lpfnCb, LONG lParamCb);
void FAR PASCAL medRelease(MEDID medid, LONG lParam);
DWORD FAR PASCAL medLock(MEDID medid, WORD wFlags, LONG lParam);
void FAR PASCAL medUnlock(MEDID medid, WORD wFlags,
			DWORD dwChangeInfo, LONG lParam);
DWORD FAR PASCAL medSendMessage(MEDID medid, MEDMSG medmsg,
			LONG lParam1, LONG lParam2);
DWORD FAR PASCAL medSendPhysMessage(MEDID medid, MEDMSG medmsg,
			LONG lParam1, LONG lParam2);
BOOL FAR PASCAL medCreate(FPMedReturn medReturn,
			MEDTYPE medtype, LONG lParam);
BOOL FAR PASCAL medIsDirty(MEDID medid);
BOOL FAR PASCAL medSetDirty(MEDID medid, BOOL fDirty);
WORD FAR PASCAL medIsAccessed(MEDID medid);
BOOL FAR PASCAL medIsShared(MEDID medid);


MEDINFO FAR PASCAL medGetMedinfo(MEDID medid);
BOOL FAR PASCAL medReleaseResinfo(MEDID medid, MEDINFO medinfo);

DWORD FAR PASCAL medSendPhysTypeMsg(MEDID medid, MEDTYPE medTypePhysical,
			MEDMSG medmsg, LONG lParam1, LONG lParam2);

typedef struct _MedAccessStruct {
	MEDID			medid;
	LONG			lParamLoad;
	BOOL			fYield;
	HWND			hwndParent;
	FPMedDiskInfoCallback	lpfnCb;
	LONG			lParamCb;
	DWORD			dwReturn;
} MedAccessStruct;
typedef MedAccessStruct FAR *FPMedAccessStruct;

WORD FAR PASCAL medAccessIndirect(FPMedAccessStruct fpAccess, WORD wSize);

typedef struct _MedSaveStruct {
	MEDID			medid;
	LONG			lParamSave;
	BOOL			fYield;
	HWND			hwndParent;
	FPMedDiskInfoCallback	lpfnCb;
	LONG			lParamCb;
	LPSTR			lpszNewName;
	
	MEDID			medidReturn;
	DWORD			dwReturn;
} MedSaveStruct;
typedef MedSaveStruct FAR *FPMedSaveStruct;

WORD FAR PASCAL medSaveIndirect(FPMedSaveStruct fpSave, WORD wSize);
WORD FAR PASCAL medSaveAsIndirect(FPMedSaveStruct fpSave, WORD wSize);

		
/*
 *  BATCH CONVERSION
 */
WORD FAR PASCAL medAccessBatch(HMEDIABATCH hmedBatch, FPMedReturn medReturn,
			   BOOL fYield, WORD wFlags);
WORD FAR PASCAL medSaveBatch(HMEDIABATCH hmedBatch, BOOL fYield);
WORD FAR PASCAL medSaveAsBatch(HMEDIABATCH hmedBatch, MEDID medidExisting,
			   LPSTR lpszName, FPMedReturn medReturn,
			   BOOL fYield, WORD wResetFlags);

HMEDIABATCH FAR PASCAL medAllocBatchBuffer(MEDID medid, HWND hwnd,
			WORD wFlags, BOOL fLoad, DWORD dwMsgData,
			FPMedDiskInfoCallback lpfnCb, LONG lParamCb);
BOOL FAR PASCAL medResetBatchBuffer(HMEDIABATCH hmedbatch, MEDID medid,
			WORD wFlags);
BOOL FAR PASCAL medFreeBatchBuffer(HMEDIABATCH hmedbatch);

#define MEDBATCH_RESETUPDATECB	0x0001

/* Flags for medLock */
#define	MEDF_READ	0x0001
#define MEDF_WRITE	0x0002
/* Flags for medUnlock */
#define MEDF_CHANGED	0x0004
#define MEDF_NOCHANGE	0x0000


/****************  RESOURCE USER  ******************/

MEDUSER FAR PASCAL medRegisterUser(HWND hWnd, DWORD dwInst);
void FAR PASCAL medUnregisterUser(MEDUSER meduser);
void FAR PASCAL medSendUserMessage(MEDID medid, MEDMSG medmsg, LONG lParam);
BOOL FAR PASCAL medRegisterUsage(MEDID medid, MEDUSER meduser);
BOOL FAR PASCAL medUnregisterUsage(MEDID medid, MEDUSER meduser);


typedef struct _MedUserMsgInfo {
        MEDID           medid;
        LONG            lParam;
        MEDINFO         medinfo;
        DWORD           dwInst;
} MedUserMsgInfo;
typedef MedUserMsgInfo FAR *FPMedUserMsgInfo;

#ifndef MM_MEDNOTIFY
#define MM_MEDNOTIFY         0x3BA
#endif

// obsolete but still required by bitedit and paledit

typedef DWORD (FAR PASCAL MedUser)
 		(MEDID medid, MEDMSG medmsg, MEDINFO medinfo,
 		LONG lParam, DWORD dwInst);
typedef MedUser FAR *FPMedUser;
 
MEDUSER FAR PASCAL medRegisterCallback(FPMedUser lpfnUser, DWORD dwInst);


/****************  TYPE TABLE  *****************/

typedef struct _MedTypeInfo {
	BOOL		fValid;		// Is this entry valid?
	WORD		wFlags;		// type flags
	MEDTYPE		medtype;	// the type id
	FPMedHandler	lpfnHandler;	// handler function for this type
	WORD		wInstanceSize;	// the byte count of instance data
	WORD		wRefcount;	// reference count on this type
} MedTypeInfo;
typedef MedTypeInfo FAR *FPMedTypeInfo;

/*  Flags to medRegisterType  */
#define MEDTYPE_LOGICAL		0x0001
#define MEDTYPE_PHYSICAL	0x0002

/*  Type creation macros  */
#define medMEDTYPE( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

#define medFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

BOOL FAR PASCAL medRegisterType(MEDTYPE medtype, FPMedHandler lpfnHandler,
				WORD wFlags);
BOOL FAR PASCAL medUnregisterType(MEDTYPE medtype);
BOOL FAR PASCAL medGetTypeInfo(MEDTYPE medtype, FPMedTypeInfo fpInfo);
WORD FAR PASCAL medIterTypes(WORD wIndex, FPMedTypeInfo fpInfo);
FOURCC FAR PASCAL medStringToFourCC(LPSTR lpszString);
void FAR PASCAL medFourCCToString( FOURCC fcc, LPSTR lpszString);

MEDTYPE FAR PASCAL medGetLogicalType(MEDID medid);
MEDTYPE FAR PASCAL medGetPhysicalType(MEDID medid);
BOOL FAR PASCAL medSetPhysicalType(MEDID medid, MEDTYPE medtype);


/*************  MISC FUNCTIONS  *****************/

WORD FAR PASCAL medGetErrorText(DWORD wErrno, LPSTR lpszBuf, WORD wSize);
DWORD FAR PASCAL medGetError(void);
void FAR PASCAL medSetExtError(WORD wErrno, HANDLE hInst);
BOOL FAR PASCAL medClientInit(void);
BOOL FAR PASCAL medClientExit(void);
HANDLE FAR PASCAL medLoadHandlerDLL(LPSTR lpszDLLName);
BOOL FAR PASCAL medUnloadHandlerDLL(HANDLE hModule);


/*************  RESOURCE LOCATION  *****************/

MEDID FAR PASCAL medLocate(LPSTR lpszMedName, MEDTYPE medtype,
			WORD wFlags, LPSTR lpszMedPath);
MEDID FAR PASCAL medSubLocate(MEDID medidParent, MEDTYPE medtype,
			DWORD dwOffset, DWORD dwSize);
HANDLE FAR PASCAL medGetAliases(MEDID medid);
		
/* Flags for medLocate */
#define MEDF_LOCATE	0x0001
#define MEDF_MAKEFILE	0x0002
#define MEDF_MEMORYFILE	0x0004
#define MEDF_NONSHARED	0x0008

typedef struct _MedMemoryFileStruct {
	LPSTR	lpszBuf;
	DWORD	dwSize;
} MedMemoryFile;
typedef MedMemoryFile FAR *FPMedMemoryFile;

		     
typedef struct _MedFileInfo {
	WORD	wFlags;
	WORD	wFilesysType;
	MEDID	medidParent;
	DWORD	dwSize;
	DWORD	dwOffset;
	/*  Information on memory file system  */
	LPSTR	lpszMemory;
	DWORD	dwMemSize;
} MedFileInfo;
typedef MedFileInfo FAR *FPMedFileInfo;

#define	MEDFILE_SUBELEMENT	0x01
#define MEDFILE_NONSHARED	0x02

BOOL FAR PASCAL medGetFileInfo(MEDID medid, FPMedFileInfo fpInfo, WORD wSize);
WORD FAR PASCAL medGetFileName(MEDID medid, LPSTR lpszBuf, WORD wBufSize);

#define	MEDNAME_ERROR	0
#define MEDNAME_DYNAMIC	1
#define MEDNAME_MEMORY	2
#define MEDNAME_FILE	3


/***************************************
 *
 *	MEDIAMAN DLL ERROR MESSAGES
 *
 *	Should be less than 0x200.
 *
 ***************************************/
#define MMERR_NOERROR		0x0000
#define MMERR_TYPELISTMEM	0x0001
#define MMERR_BADHANDLER	0x0002
#define MMERR_TYPELISTDUPL	0x0003
#define MMERR_INVALIDTYPE	0x0004
#define MMERR_TYPEREFNOTZERO	0x0005
#define MMERR_WRONGHANDLERTYPE	0x0006

#define MMERR_GROUPTABLEMEMORY	0x0010
#define MMERR_GROUPSTRUCTMEMORY	0x0011
#define MMERR_GROUPSTRINGTABLE	0x0012
#define MMERR_GROUPENTRYTABLE	0x0013

#define MMERR_USERTABLEMEMORY	0x0014
#define RMERR_INVALIDMEDUSER	0x0015
#define MMERR_GLOBALUSERMEMORY	0x0016

#define MMERR_INVALIDSUBPARENT	0x0030
#define MMERR_NOFILEEXISTS	0x0031

#define MMERR_LOGONNAMESPACE	0x0040
#define MMERR_FILENAMESTEP	0x0041
#define MMERR_MEDNOTACCESSED	0x0042
#define MMERR_UNNAMEDELEMENT	0x0043
#define MMERR_DISKOPABORT	0x0044
#define	MMERR_NOTCORRECTFILE	0x0045
#define MMERR_COPYFAILED	0x0046
#define MMERR_DISKOPINPROGRESS	0x0047
#define MMERR_MEMORY		0x0048

#define MMERR_READOFWRITELOCKED		0x00E0
#define MMERR_WRITEOFWRITELOCKED	0x00E1

#define	MMERR_INVALIDDLLNAME	0x00F0
#define MMERR_COULDNOTLOADDLL	0x00F1
#define MMERR_BADLIBINIT	0x00F2
#define MMERR_INVALIDMODULE	0x00F3

#define MMERR_UNKNOWN		0x0100

#define MMERR_HMEDREAD		0x0101
#define MMERR_HMEDWRITE		0x0102
#define MMERR_HMEDGET		0x0103
#define MMERR_HMEDPUT		0x0104
#define MMERR_HMEDCLOSE		0x0105
#define MMERR_HMEDFIND		0x0106
#define MMERR_HMEDFINDANY	0x0107
#define MMERR_HMEDUNGET		0x0108
#define MMERR_HMEDALLOC		0x0109
#define MMERR_HMEDLOCK		0x010a
#define MMERR_GETFILEINFO	0x010b
#define MMERR_HMEDASCEND	0x010c
#define MMERR_STACKASCEND	0x010d
#define MMERR_HMEDDESCEND	0x010e
#define MMERR_DESCENDSPACE	0x010f
#define MMERR_DESCENDGET	0x0110
#define MMERR_STACKDESCEND	0x0111
#define MMERR_HMEDRESIZE	0x0112
#define MMERR_STACKRESIZE	0x0113
#define MMERR_HMEDCREATE	0x0114
#define MMERR_STACKCREATE	0x0115
#define MMERR_CREATESPACE	0x0116
#define MMERR_CREATEPUT		0x0117
#define MMERR_HMEDSIZE		0x0118
#define MMERR_HMEDLEVEL		0x0119
#define MMERR_HMEDCKID		0x011a
#define RMERR_MEDIDOPEN		0x011b
#define MMERR_WRITEONLY		0x011c
#define MMERR_READONLY		0x011d
#define MMERR_PREVERROR		0x011e
#define MMERR_EOF		0x011f
#define MMERR_BEGIN		0x0120
#define MMERR_IOERROR		0x0121
#define MMERR_UNGETROOM		0x0122
#define MMERR_GETFILENAME	0x0123
#define MMERR_FINDFIRST		0x0124
#define MMERR_OPEN		0x0125
#define MMERR_SEEKINIT		0x0126
#define MMERR_HMEDSEEK		0x0127
#define MMERR_READ		0x0128
#define MMERR_HMEDCFOPEN	0x0129
#define MMERR_MEDGCFCLOSE	0x0130
#define MMERR_WRITE		0x0131



#define MMERR_MAXERROR		0x0200





/***************************************
 *
 *	MEDIAMAN PHYSICAL IO DEFINITIONS
 *
 ***************************************/

typedef FOURCC CKID;
typedef FOURCC FORMTYPE;
typedef DWORD CKSIZE;

/* 
 * RIFF stack elements
 */

typedef int RIFFSTACKPLACE;
typedef void huge * HPVOID;


typedef struct riff_stack_element {
    CKID		nID;		/* TOS current chunk id */
    CKSIZE		cbSize;		/* TOS current chunk size */
    LONG		nOffset;	/* Stacked nOffset for prev chunk */
    LONG		nEnd;		/* Stacked nEnd for prev chunk */
    WORD		info;		/* Stacked info for prev chunk */
} RIFFSTACKELEM;

typedef RIFFSTACKELEM FAR * FPRIFFSTACKELEM;

/*
 * Storage System Handler routine type definition 
 */
typedef LONG (FAR PASCAL MIOHANDLER)       \
		(struct medElement far * hMed, WORD wFlags, \
			DWORD lParam1, DWORD lParam2 );
typedef MIOHANDLER FAR * FPMIOHANDLER;


#define MEDIO_DOS_STORAGE	0
#define MEDIO_MEM_STORAGE	1
#define MEDIO_CF_STORAGE	2

/*
 * Handle to Resource ( once opened ) 
 */
typedef struct medElement {
    LONG lData1;		/*  data 1 2 and 3 sections hold */
    LONG lData2;		/* info specific to the storage system */
    LONG lData3;		/* eg the dos file handle for dos files */
    MEDID medid;		/* medid opened */
    HANDLE hMem;		/* handle to the memory of the HMED */
    FPMIOHANDLER fpfnHandler;	/* storage system handler */
    LONG nInitOffset;		/* for sub-resources - offset of start */
    LONG nOffset;		/* for riff offset start of chunk to start */
				/* NOTE nOffset doesn't include nInitOffset */
    LONG nCurrent;		/* offset of end of current buffer */
    LONG nEnd;			/* offset of end of current chunk/file */
    LONG nLeft;			/* space in buff from eof(chunk) to buffend */
    LONG nGet;			/* number of chars left to read in buff */
    LONG nPut;			/* number of chars left to write in buff */
				/* NOTE nGet, nPut are mutually exclusive */
    LONG nSize;			/* size of information read into buffer */
    LONG nUnGet;		/* num of chars ungotten before buff start */
    WORD flags;			/* RIOF_ info 'temperary' like error */
    WORD info;			/* RIOI_ info 'permanent' eg. extendable*/
    LPSTR fpchPlace;		/* place to get/put next char in buffer */
    LPSTR fpchMax;		/* max position of Place - for seek, unget */
    LONG cbBuffer;		/* the full buffer size */
    RIFFSTACKPLACE nRiffTop;	/* current top of riff stack, -1 is nothing */
    FPRIFFSTACKELEM fpRiffStack;
				/* FP into the data to for Riff Stack */
				/* riff stack index top by nRiffTop */
    LPSTR fpchBuffer;		/* FP into the data for the Buffer */
    char data[1];		/* the actual buffer */
} MIOELEMENT;

typedef MIOELEMENT FAR * HMED;

/*
 * HMED flags 'temporary' info
 */
#define MIOF_OK		0	/* no info */
#define MIOF_EOF	1	/* have reached eof (or end of chunk) */
#define MIOF_READING	2	/* info has been read into buffer */
#define MIOF_WRITING	4	/* info has been written to buffer */
#define MIOF_UNGET	8	/* have ungotten before buff start */
#define MIOF_ERROR	16	/* have gotten some form of error */

#define MIOF_BUFF_EOF	64	/* EOF at buffer end */
#define MIOF_BUFF_ERROR	128	/* error at buffer end or 'fatal' error */
				/* fatal as in can not do any more IO */
				/* unlike the error you get for */
				/* trying to unget too many chars */
#define MIOF_AFTERCURR	256	/* the characters in the buffer are located */
				/* after the hMed->nCurrent */


/*
 * HMED info 'permanent' info
 */
#define MIOI_NOTHING		0	/* no info */
#define MIOI_RESIZED		32	/* This chunk has been resized */
					/* possibly fix the size on ascend */
#define MIOI_AUTOSIZED		64	/* This chunk has been created */
					/* fix the size on ascend */
#define MIOI_BYTESWAPPED	128	/* riff chunk sizes are byteswapped */
#define MIOI_EXTENDABLE		256	/* the resource is extendable */
					/* unlike riff chunks */


/*
 * Return values
 */
#define MED_EOF		(-1)		/* universal something wrong return */

/*
 * Resource mode to open resource as 
 */
#define MOP_READ	0
#define MOP_WRITE	1
#define MOP_READ_WRITE	2

#define MOP_PRELOAD	0x0008
#define MOP_CREATE	0x0010
#define MOP_ZEROBUFFER	0x0100


/* CLOSE return flags */
#define MCERR_OK		0x0000
#define MCERR_UNSPECIFIC	0x0001
#define MCERR_FLUSH		0x0002
#define MCERR_STORAGE		0x0004


/* 
 * seek flags
 */
#define MOPS_SET	1
#define MOPS_CUR	2
#define MOPS_END	4
#define MOPS_NONLOCAL	128
#define MOPS_EXTEND	64



/*
 * Resize Chunk Flags
 */
#define MOPRC_AUTOSIZE		1

/* 
 * FUNCTION API's
 */
/* normal IO */
HMED FAR PASCAL medOpen( MEDID id, WORD wMode, WORD wSize );
WORD FAR PASCAL medClose( HMED hMed );
LONG FAR PASCAL medRead( HMED hMed, HPVOID hpBuffer, LONG lBytes );
LONG FAR PASCAL medWrite( HMED hMed, HPVOID hpBuffer, LONG lBytes );
LONG FAR PASCAL medSeek( HMED hMed, LONG lOffset, WORD wOrigin );

/* RIFF */
BOOL FAR PASCAL medAscend( HMED hMed );
CKID FAR PASCAL medDescend( HMED hMed );
CKID FAR PASCAL medCreateChunk(HMED hMed, CKID ckid, DWORD dwCkSize);
BOOL FAR PASCAL medResizeChunk(HMED hMed, DWORD dwCkSize, WORD wFlags);
int FAR PASCAL medGetChunkLevel( HMED hMed );
CKID FAR PASCAL medGetChunkID( HMED hMed );
CKSIZE FAR PASCAL medGetChunkSize( HMED hMed );
BOOL FAR PASCAL medFindAnyChunk( HMED hMed, CKID FAR * ackid );
BOOL FAR PASCAL medFindChunk( HMED hMed, CKID id );

LONG FAR PASCAL medGetSwapWORD( HMED hMed );  

/* 
 * FUNCTIONs used in macros and such
 */
LONG FAR PASCAL medIOFillBuff( HMED hMed, int size );
LONG FAR PASCAL medIOFlushBuff( HMED hMed, DWORD dwElem, int size );
BOOL FAR PASCAL medFlush( HMED hMed );
LONG FAR PASCAL medUnGet( HMED hMed, DWORD dwElem, int size );

/*
 * MACRO API's autodocked in riomac.d
 */
/* RIFF stuff */

#define medFCC3( fcc )	( (BYTE)( (fcc & 0xFF000000) >> 24 ) )
#define medFCC2( fcc )	( (BYTE)( (fcc & 0x00FF0000) >> 16 ) )
#define medFCC1( fcc )	( (BYTE)( (fcc & 0x0000FF00) >> 8 ) )
#define medFCC0( fcc )	( (BYTE)(fcc & 0x000000FF) )


/* constant RIFF chunk id */
#define ckidRIFF	medFOURCC( 'R', 'I', 'F', 'F' )
			  

/* lets the user set and check if the file is byteswapped */
#define medGetByteSwapped( hMed )	( (hMed)->info & MIOI_BYTESWAPPED )

#define medSetByteSwapped( hMed, fVal )   ( (hMed)->info = ( fVal ?       \
			( (hMed)->info | MIOI_BYTESWAPPED ) :		\
			~((~((hMed)->info))|MIOI_BYTESWAPPED) ) )


#define medHMEDtoMEDID( hMed )	( (hMed)->medid )

						
						

/*
 * UnGet Get and Put of BYTE WORD and DWORD and ByteSwapped versions 
 * NOTE the ByteSwap function is in WINCOM and so WINCOM.H must be included
 *	before the RESIO.H
 */						
#define medUnGetBYTE( hMed, ch )   ( (int)(medUnGet( hMed,(DWORD)ch,1 ) ) )
#define medUnGetWORD( hMed, w )	   ( (LONG)(medUnGet( hMed,(DWORD)w,2 ) ) )
#define medUnGetDWORD( hMed, dw )  ( (LONG)(medUnGet( hMed,(DWORD)dw,4) ) )
#define medUnGetSwapWORD(hMed,w)   ( medUnGetWORD( hMed,		\
						ByteSwapWORD((WORD)w) ) )
#define medUnGetSwapDWORD(hMed,dw) ( medUnGetDWORD( hMed,		\
						ByteSwapDWORD((DWORD)dw) ) )

#define medUnGetOpSwapWORD( hMed, w )   ( medGetByteSwapped( hMed ) ?	\
			medUnGetSwapWORD( hMed, w ) :			\
			medUnGetWORD( hMed, w ) )

#define medUnGetOpSwapDWORD( hMed, dw )   ( medGetByteSwapped( hMed ) ?	\
			medUnGetSwapDWORD( hMed, dw ) :			\
			medUnGetDWORD( hMed, dw ) )

							
							

#define medGetBYTE( hMed )	( (--((hMed)->nGet) >= 0) ?	             \
		(int)(BYTE)*(((hMed)->fpchPlace)++) :			     \
		(int)(medIOFillBuff( hMed, 1 )) )
						
#define medPutBYTE( hMed, ch )	( (--((hMed)->nPut) >= 0) ?                 \
		(int)(BYTE)((*(((hMed)->fpchPlace)++)) = (BYTE)ch) :	  \
		(int)(medIOFlushBuff(hMed,(DWORD)ch,1)) )
				
				
/* note in the following macros we want to advance the fpchPlace */
/* by the size after we get the value of it.  To do this we typecast */
/* them to int ( or long ) and add the size ( += size ) and then */
/* subtract 4 so the value used is not incremented yet */
							
							
#define medGetWORD( hMed )	( (((hMed)->nGet -= 2) >= 0) ?		     \
	(long)*((WORD FAR *)((((LONG)((hMed)->fpchPlace))+=2)-2)) :	     \
	(long)(medIOFillBuff( hMed, 2 )) )
					
#define medPutWORD( hMed, w )	( (((hMed)->nPut -= 2) >= 0) ?                 \
	(long)((*((WORD FAR *)((((LONG)((hMed)->fpchPlace))+=2)-2)))=(WORD)w) :\
	(long)(medIOFlushBuff( hMed, (DWORD)w, 2 )) )

#define medPutSwapWORD( hMed, w )  ( medPutWORD(hMed,ByteSwapWORD((WORD)w)) )
		
#define medGetOpSwapWORD( hMed )   ( medGetByteSwapped( hMed ) ?	\
			medGetSwapWORD( hMed ) :			\
			medGetWORD( hMed ) )

#define medPutOpSwapWORD( hMed, w )   ( medGetByteSwapped( hMed ) ?	\
			medPutSwapWORD( hMed, w ) :			\
			medPutWORD( hMed, w ) )




#define medGetDWORD( hMed )	( (((hMed)->nGet -= 4) >= 0) ?		     \
	(long)*((DWORD FAR *)((((LONG)((hMed)->fpchPlace))+=4)-4)) :	     \
	(long)(medIOFillBuff( hMed, 4 )) )
					
#define medPutDWORD( hMed, dw )	( (((hMed)->nPut -= 4) >= 0) ?                \
    (long)((*((DWORD FAR *)((((LONG)((hMed)->fpchPlace))+=4)-4)))=(DWORD)dw): \
    (long)(medIOFlushBuff( hMed, (DWORD)dw, 4 )) )

#define medGetSwapDWORD(hMed)    (ByteSwapDWORD((DWORD)medGetDWORD(hMed)))
#define medPutSwapDWORD(hMed,dw) (medPutDWORD(hMed,ByteSwapDWORD((DWORD)dw)))

#define medGetOpSwapDWORD( hMed )   ( medGetByteSwapped( hMed ) ?	\
			medGetSwapDWORD( hMed ) :			\
			medGetDWORD( hMed ) )

#define medPutOpSwapDWORD( hMed, dw )   ( medGetByteSwapped( hMed ) ?	\
			medPutSwapDWORD( hMed, dw ) :			\
			medPutDWORD( hMed, dw ) )


/* for RIFF, read the FORMTYPE */
#define medReadFormHeader( hMed )		medGetDWORD( hMed )
#define medWriteFormHeader( hMed, formtype )	medPutDWORD( hMed, formtype )
#define medGetFOURCC( hMed )			medGetDWORD( hMed )
#define medPutFOURCC( hMed, fcc )		medPutDWORD( hMed, fcc )

/* ERROR and EOF checks */
#define medGetIOError( hMed )	( (hMed)->flags & MIOF_ERROR )
#define medGetIOEOF( hMed )	( (hMed)->flags & MIOF_EOF )

#endif /* _MEDMAN_H_ */

