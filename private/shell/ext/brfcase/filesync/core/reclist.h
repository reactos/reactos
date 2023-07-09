/*
 * reclist.h - Reconciliation list ADT description.
 */


/* Macros
 *********/

#define RECNODE_EXISTS(prn)           ((prn)->fsCurrent.fscond == FS_COND_EXISTS)
#define RECNODE_DOES_NOT_EXIST(prn)   ((prn)->fsCurrent.fscond == FS_COND_DOES_NOT_EXIST)
#define RECNODE_IS_AVAILABLE(prn)     (RECNODE_EXISTS(prn) || RECNODE_DOES_NOT_EXIST(prn))
#define RECNODE_WAS_RECONCILED(prn)   (RECNODE_EXISTS(prn) && prn->rnaction != RNA_NOTHING)


/* Prototypes
 *************/

/* reclist.c */

extern BOOL IsReconciledFileStamp(PCFILESTAMP);
extern BOOL LastKnownNonExistent(PCFILESTAMP, PCFILESTAMP);
extern void DetermineDeletionPendingState(PCRECITEM);
extern BOOL DeleteTwinsFromRecItem(PCRECITEM);
extern BOOL DeleteTwinsFromRecList(PCRECLIST);
extern TWINRESULT FindCopySource(PCRECITEM, PRECNODE *);
extern void ChooseMergeDestination(PCRECITEM, PRECNODE *);
extern void ClearFlagInArrayOfStubs(HPTRARRAY, DWORD);
extern BOOL NotifyCreateRecListStatus(CREATERECLISTPROC, UINT, LPARAM, LPARAM);
extern COMPARISONRESULT CompareInts(int, int);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidFILESTAMPCONDITION(FILESTAMPCONDITION);
extern BOOL IsValidPCFILESTAMP(PCFILESTAMP);
extern BOOL IsFolderObjectTwinFileStamp(PCFILESTAMP);
extern BOOL IsValidPCRECNODE(PCRECNODE);
extern BOOL IsValidPCRECITEM(PCRECITEM);

#endif

