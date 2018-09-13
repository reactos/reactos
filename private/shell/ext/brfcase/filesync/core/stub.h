/*
 * stub.h - Stub ADT description.
 */


/* Types
 ********/

/* stub types */

typedef enum _stubtype
{
   ST_OBJECTTWIN,

   ST_TWINFAMILY,

   ST_FOLDERPAIR
}
STUBTYPE;
DECLARE_STANDARD_TYPES(STUBTYPE);

/* stub flags */

typedef enum _stubflags
{
   /* This stub was marked for deletion while it was locked. */

   STUB_FL_UNLINKED           = 0x0001,

   /* This stub has already been used for some operation. */

   STUB_FL_USED               = 0x0002,

   /*
    * The file stamp of this object twin stub is valid.  (Only used for object
    * twins to cache file stamp from folder twin expansion for RECNODE
    * creation.)
    */

   STUB_FL_FILE_STAMP_VALID   = 0x0004,

   /*
    * This twin family stub or folder twin stub is in the process of being
    * deleted.  (Only used for twin families and folder twins.)
    */

   STUB_FL_BEING_DELETED      = 0x0008,

   /*
    * This folder twin stub is in the process of being translated.  (Only used
    * for folder twins.)
    */

   STUB_FL_BEING_TRANSLATED   = 0x0010,

   /*
    * This object twin stub was explicitly added a an object twin through
    * AddObjectTwin().  (Only used for object twins.)
    */

   STUB_FL_FROM_OBJECT_TWIN   = 0x0100,

   /*
    * This object twin stub was not reconciled the last time its twin family
    * was reconciled, and some members of the twin family were known to have
    * changed.  (Only used for object twins.)
    */

   STUB_FL_NOT_RECONCILED     = 0x0200,

   /*
    * The subtree of the root folder of this folder twin stub is to be included
    * in reconciliation.  (Only used for folder twins.)
    */

   STUB_FL_SUBTREE            = 0x0400,

   /*
    * The object twins in this twin family are pending deletion because an
    * object twin was deleted, and no object twins have changed since that
    * object twins was deleted.  This folder twin is pending deletion because
    * its folder root is last known deleted.  (Only used for twin families and
    * folder twins.)
    */

   STUB_FL_DELETION_PENDING   = 0x0800,

   /*
    * The client indicated that this object twin should not be deleted.  (Only
    * used for object twins.)
    */

   STUB_FL_KEEP               = 0x1000,

   /* stub flag combinations */

   ALL_STUB_FLAGS             = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_FILE_STAMP_VALID |
                                 STUB_FL_BEING_DELETED |
                                 STUB_FL_BEING_TRANSLATED |
                                 STUB_FL_FROM_OBJECT_TWIN |
                                 STUB_FL_NOT_RECONCILED |
                                 STUB_FL_SUBTREE |
                                 STUB_FL_DELETION_PENDING |
                                 STUB_FL_KEEP),

   ALL_OBJECT_TWIN_FLAGS      = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_FILE_STAMP_VALID |
                                 STUB_FL_NOT_RECONCILED |
                                 STUB_FL_FROM_OBJECT_TWIN |
                                 STUB_FL_KEEP),

   ALL_TWIN_FAMILY_FLAGS      = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_BEING_DELETED |
                                 STUB_FL_DELETION_PENDING),

   ALL_FOLDER_TWIN_FLAGS      = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_BEING_DELETED |
                                 STUB_FL_BEING_TRANSLATED |
                                 STUB_FL_SUBTREE |
                                 STUB_FL_DELETION_PENDING),

   /* bit mask used to save stub flags in briefcase database */

   DB_STUB_FLAGS_MASK         = 0xff00
}
STUBFLAGS;

/*
 * common stub - These fields must appear at the start of TWINFAMILY,
 * OBJECTTWIN, and FOLDERPAIR in the same order.
 */

typedef struct _stub
{
   /* structure tag */

   STUBTYPE st;

   /* lock count */

   ULONG ulcLock;

   /* flags */

   DWORD dwFlags;
}
STUB;
DECLARE_STANDARD_TYPES(STUB);

/* object twin family */

typedef struct _twinfamily
{
   /* common stub */

   STUB stub;

   /* handle to name string */

   HSTRING hsName;

   /* handle to list of object twins */

   HLIST hlistObjectTwins;

   /* handle to parent briefcase */

   HBRFCASE hbr;
}
TWINFAMILY;
DECLARE_STANDARD_TYPES(TWINFAMILY);

/* object twin */

typedef struct _objecttwin
{
   /* common stub */

   STUB stub;

   /* handle to folder path */

   HPATH hpath;

   /* file stamp at last reconciliation time */

   FILESTAMP fsLastRec;

   /* pointer to parent twin family */

   PTWINFAMILY ptfParent;

   /* source folder twins count */

   ULONG ulcSrcFolderTwins;

   /*
    * current file stamp, only valid if STUB_FL_FILE_STAMP_VALID is set in
    * stub's flags
    */

   FILESTAMP fsCurrent;
}
OBJECTTWIN;
DECLARE_STANDARD_TYPES(OBJECTTWIN);

/* folder pair data */

typedef struct _folderpairdata
{
   /* handle to name of included objects - may contain wildcards */

   HSTRING hsName;

   /* attributes to match */

   DWORD dwAttributes;

   /* handle to parent briefcase */

   HBRFCASE hbr;
}
FOLDERPAIRDATA;
DECLARE_STANDARD_TYPES(FOLDERPAIRDATA);

/* folder pair */

typedef struct _folderpair
{
   /* common stub */

   STUB stub;

   /* handle to folder path */

   HPATH hpath;

   /* pointer to folder pair data */

   PFOLDERPAIRDATA pfpd;

   /* pointer to other half of folder pair */

   struct _folderpair *pfpOther;
}
FOLDERPAIR;
DECLARE_STANDARD_TYPES(FOLDERPAIR);

/*
 * EnumGeneratedObjectTwins() callback function
 *
 * Called as:
 *
 * bContinue = EnumGeneratedObjectTwinsProc(pot, pvRefData);
 */

typedef BOOL (*ENUMGENERATEDOBJECTTWINSPROC)(POBJECTTWIN, PVOID);

/*
 * EnumGeneratingFolderTwins() callback function
 *
 * Called as:
 *
 * bContinue = EnumGeneratingFolderTwinsProc(pfp, pvRefData);
 */

typedef BOOL (*ENUMGENERATINGFOLDERTWINSPROC)(PFOLDERPAIR, PVOID);


/* Prototypes
 *************/

/* stub.c */

extern void InitStub(PSTUB, STUBTYPE);
extern TWINRESULT DestroyStub(PSTUB);
extern void LockStub(PSTUB);
extern void UnlockStub(PSTUB);
extern DWORD GetStubFlags(PCSTUB);
extern void SetStubFlag(PSTUB, DWORD);
extern void ClearStubFlag(PSTUB, DWORD);
extern BOOL IsStubFlagSet(PCSTUB, DWORD);
extern BOOL IsStubFlagClear(PCSTUB, DWORD);

#ifdef VSTF

extern BOOL IsValidPCSTUB(PCSTUB);

#endif

/* twin.c */

extern BOOL FindObjectTwin(HBRFCASE, HPATH, LPCTSTR, PHNODE);
extern TWINRESULT TwinObjects(HBRFCASE, HCLSIFACECACHE, HPATH, HPATH, LPCTSTR, POBJECTTWIN *, POBJECTTWIN *);
extern BOOL CreateObjectTwin(PTWINFAMILY, HPATH, POBJECTTWIN *);
extern TWINRESULT UnlinkObjectTwin(POBJECTTWIN);
extern void DestroyObjectTwin(POBJECTTWIN);
extern TWINRESULT UnlinkTwinFamily(PTWINFAMILY);
extern void MarkTwinFamilyNeverReconciled(PTWINFAMILY);
extern void MarkObjectTwinNeverReconciled(PVOID);
extern void DestroyTwinFamily(PTWINFAMILY);
extern void MarkTwinFamilyDeletionPending(PTWINFAMILY);
extern void UnmarkTwinFamilyDeletionPending(PTWINFAMILY);
extern BOOL IsTwinFamilyDeletionPending(PCTWINFAMILY);
extern void ClearTwinFamilySrcFolderTwinCount(PTWINFAMILY);
extern BOOL EnumObjectTwins(HBRFCASE, ENUMGENERATEDOBJECTTWINSPROC, PVOID);
extern BOOL ApplyNewFolderTwinsToTwinFamilies(PCFOLDERPAIR);
extern TWINRESULT TransplantObjectTwin(POBJECTTWIN, HPATH, HPATH);
extern BOOL IsFolderObjectTwinName(LPCTSTR);


#ifdef VSTF

extern BOOL IsValidPCTWINFAMILY(PCTWINFAMILY);
extern BOOL IsValidPCOBJECTTWIN(PCOBJECTTWIN);

#endif

/* foldtwin.c */

extern void LockFolderPair(PFOLDERPAIR);
extern void UnlockFolderPair(PFOLDERPAIR);
extern TWINRESULT UnlinkFolderPair(PFOLDERPAIR);
extern void DestroyFolderPair(PFOLDERPAIR);
extern BOOL ApplyNewObjectTwinsToFolderTwins(HLIST);
extern BOOL BuildPathForMatchingObjectTwin(PCFOLDERPAIR, PCOBJECTTWIN, HPATHLIST, PHPATH);
extern BOOL EnumGeneratedObjectTwins(PCFOLDERPAIR, ENUMGENERATEDOBJECTTWINSPROC, PVOID);
extern BOOL EnumGeneratingFolderTwins(PCOBJECTTWIN, ENUMGENERATINGFOLDERTWINSPROC, PVOID, PULONG);
extern BOOL FolderTwinGeneratesObjectTwin(PCFOLDERPAIR, HPATH, LPCTSTR);

#ifdef VSTF

extern BOOL IsValidPCFOLDERPAIR(PCFOLDERPAIR);

#endif

extern void RemoveObjectTwinFromAllFolderPairs(POBJECTTWIN);

/* expandft.c */

extern BOOL ClearStubFlagWrapper(PSTUB, PVOID);
extern BOOL SetStubFlagWrapper(PSTUB, PVOID);
extern TWINRESULT ExpandIntersectingFolderTwins(PFOLDERPAIR, CREATERECLISTPROC, LPARAM);
extern TWINRESULT TryToGenerateObjectTwin(HBRFCASE, HPATH, LPCTSTR, PBOOL, POBJECTTWIN *);

