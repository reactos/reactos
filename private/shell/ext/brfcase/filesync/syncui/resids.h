//---------------------------------------------------------------------------
// Base values
//---------------------------------------------------------------------------

#define ID_BASE         0x1000
#define IDS_BASE        (ID_BASE + 0x0000)

//---------------------------------------------------------------------------
// Strings
//---------------------------------------------------------------------------

// Range of indexes are 0x000 - 0x7ff
#define IDS_ERR_BASE                    (IDS_BASE + 0x0000)
#define IDS_OOM_BASE                    (IDS_BASE + 0x0800)
#define IDS_MSG_BASE                    (IDS_BASE + 0x1000)
#define IDS_RANDO_BASE                  (IDS_BASE + 0x1800)

// Error strings
#define IDS_ERR_READONLY                (IDS_ERR_BASE + 0x000)
#define IDS_ERR_ADD_SUBTREECYCLE        (IDS_ERR_BASE + 0x003)
#define IDS_ERR_CORRUPTDB               (IDS_ERR_BASE + 0x004)
#define IDS_ERR_SAMEGUYIDIOT            (IDS_ERR_BASE + 0x005)
#define IDS_ERR_BOGUSVOLUME             (IDS_ERR_BASE + 0x006)
#define IDS_ERR_FULLDISK                (IDS_ERR_BASE + 0x007)
#define IDS_ERR_FULLDISKSAVE            (IDS_ERR_BASE + 0x008)
#define IDS_ERR_CANTADDBRIEFCASE        (IDS_ERR_BASE + 0x009)
#define IDS_ERR_BCALREADYEXISTS         (IDS_ERR_BASE + 0x00a)
#define IDS_ERR_CANTCREATEBC            (IDS_ERR_BASE + 0x00b)
#define IDS_ERR_BRIEFCASE_LOCKED        (IDS_ERR_BASE + 0x00c)
#define IDS_ERR_UPD_UNAVAIL_VOL         (IDS_ERR_BASE + 0x00d)
#define IDS_ERR_FILE_CHANGED            (IDS_ERR_BASE + 0x00e)
#define IDS_ERR_SOURCE_FILE             (IDS_ERR_BASE + 0x00f)
#define IDS_ERR_ADDFILE_UNAVAIL_VOL     (IDS_ERR_BASE + 0x010)
#define IDS_ERR_ADDFOLDER_UNAVAIL_VOL   (IDS_ERR_BASE + 0x011)
#define IDS_ERR_NEWER_BRIEFCASE         (IDS_ERR_BASE + 0x012)
#define IDS_ERR_ADD_READONLY            (IDS_ERR_BASE + 0x013)
#define IDS_ERR_ADD_FULLDISK            (IDS_ERR_BASE + 0x014)
#define IDS_ERR_ADD_SOURCE_FILE         (IDS_ERR_BASE + 0x015)
#define IDS_ERR_ADD_UNAVAIL_VOL         (IDS_ERR_BASE + 0x016)
#define IDS_ERR_SAVE_UNAVAIL_VOL        (IDS_ERR_BASE + 0x017)
#define IDS_ERR_CLOSE_UNAVAIL_VOL       (IDS_ERR_BASE + 0x018)
#define IDS_ERR_OPEN_UNAVAIL_VOL        (IDS_ERR_BASE + 0x019)
#define IDS_ERR_UNAVAIL_VOL             (IDS_ERR_BASE + 0x01a)
#define IDS_ERR_OPEN_SUBTREECYCLE       (IDS_ERR_BASE + 0x01b)
#define IDS_ERR_OPEN_ACCESS_DENIED      (IDS_ERR_BASE + 0x01c)
#define IDS_ERR_NO_MERGE_HANDLER        (IDS_ERR_BASE + 0x01d)
#define IDS_ERR_ADDFILE_TOOLONG         (IDS_ERR_BASE + 0x01e)
#define IDS_ERR_ADDFOLDER_TOOLONG       (IDS_ERR_BASE + 0x01f)
#define IDS_ERR_OPEN_TOOLONG            (IDS_ERR_BASE + 0x020)
#define IDS_ERR_CREATE_TOOLONG          (IDS_ERR_BASE + 0x021)
#define IDS_ERR_ADDFILE_TOOMANY         (IDS_ERR_BASE + 0x022)
#define IDS_ERR_ADDFOLDER_TOOMANY       (IDS_ERR_BASE + 0x023)
#define IDS_ERR_ADD_SYNCFOLDER          (IDS_ERR_BASE + 0x024)
#define IDS_ERR_ADD_SYNCFOLDER_SRC      (IDS_ERR_BASE + 0x025)
#define IDS_ERR_CREATE_INANOTHER        (IDS_ERR_BASE + 0x026)

#define IDS_ERR_F_CantSplit             (IDS_ERR_BASE + 0x100)
#define IDS_ERR_1_CantSplit             (IDS_ERR_BASE + 0x101)
#define IDS_ERR_2_CantSplit             (IDS_ERR_BASE + 0x102)

#define IDS_ERR_F_CorruptDB             (IDS_ERR_BASE + 0x104)
#define IDS_ERR_1_CorruptDB             (IDS_ERR_BASE + 0x105)
#define IDS_ERR_2_CorruptDB             (IDS_ERR_BASE + 0x106)

#define IDS_ERR_F_FullDiskSave          (IDS_ERR_BASE + 0x108)
#define IDS_ERR_1_FullDiskSave          (IDS_ERR_BASE + 0x109)
#define IDS_ERR_2_FullDiskSave          (IDS_ERR_BASE + 0x10a)


// Out-of-memory strings
#define IDS_OOM_ADD                     (IDS_OOM_BASE + 0x000)
#define IDS_OOM_CHANGETYPES             (IDS_OOM_BASE + 0x001)
#define IDS_OOM_STATUS                  (IDS_OOM_BASE + 0x002)
#define IDS_OOM_INFO                    (IDS_OOM_BASE + 0x003)
#define IDS_OOM_FILLTYPES               (IDS_OOM_BASE + 0x004)
#define IDS_OOM_UPDATEDIALOG            (IDS_OOM_BASE + 0x005)
#define IDS_OOM_OPENBRIEFCASE           (IDS_OOM_BASE + 0x006)
#define IDS_OOM_UPDATE                  (IDS_OOM_BASE + 0x007)
#define IDS_OOM_ADDFOLDER               (IDS_OOM_BASE + 0x008)


// Messages
#define IDS_MSG_SPECIFYTYPE             (IDS_MSG_BASE + 0x010)
#define IDS_MSG_ONDESKTOP               (IDS_MSG_BASE + 0x011)
#define IDS_MSG_ATPATH                  (IDS_MSG_BASE + 0x012)
#define IDS_MSG_CantFindOriginal        (IDS_MSG_BASE + 0x013)
#define IDS_MSG_ConfirmFileSplit        (IDS_MSG_BASE + 0x014)
#define IDS_MSG_ConfirmFolderSplit      (IDS_MSG_BASE + 0x015)
#define IDS_MSG_ConfirmMultiSplit       (IDS_MSG_BASE + 0x016)
#define IDS_MSG_FileAlreadyOrphan       (IDS_MSG_BASE + 0x017)
#define IDS_MSG_FolderAlreadyOrphan     (IDS_MSG_BASE + 0x018)
#define IDS_MSG_FileTombstone           (IDS_MSG_BASE + 0x019)
#define IDS_MSG_FolderTombstone         (IDS_MSG_BASE + 0x01a)
#define IDS_MSG_UpdateOnDock            (IDS_MSG_BASE + 0x01b)
#define IDS_MSG_UpdateBeforeUndock      (IDS_MSG_BASE + 0x01c)
#define IDS_MSG_NoMatchingFiles         (IDS_MSG_BASE + 0x01d)
#define IDS_MSG_CHECKING                (IDS_MSG_BASE + 0x01e)

#define IDS_MSG_NoFiles                 (IDS_MSG_BASE + 0x040)
#define IDS_MSG_AllOrphans              (IDS_MSG_BASE + 0x041)
#define IDS_MSG_AllUptodate             (IDS_MSG_BASE + 0x042)
#define IDS_MSG_AllSomeUnavailable      (IDS_MSG_BASE + 0x043)

// The file/folder ids below must be interleaved, with the
// folder id = the file id + 1.
#define IDS_MSG_FileOrphan              (IDS_MSG_BASE + 0x044)
#define IDS_MSG_FolderOrphan            (IDS_MSG_BASE + 0x045)
#define IDS_MSG_FileUptodate            (IDS_MSG_BASE + 0x046)
#define IDS_MSG_FolderUptodate          (IDS_MSG_BASE + 0x047)
#define IDS_MSG_FileUnavailable         (IDS_MSG_BASE + 0x048)
#define IDS_MSG_FolderUnavailable       (IDS_MSG_BASE + 0x049)
#define IDS_MSG_FolderSubfolder         (IDS_MSG_BASE + 0x04a)

#define IDS_MSG_MultiOrphans            (IDS_MSG_BASE + 0x050)
#define IDS_MSG_MultiUptodate           (IDS_MSG_BASE + 0x051)
#define IDS_MSG_MultiUptodateOrphan     (IDS_MSG_BASE + 0x052)
#define IDS_MSG_MultiUnavailable        (IDS_MSG_BASE + 0x053)
#define IDS_MSG_MultiSubfolder          (IDS_MSG_BASE + 0x054)


// Menu strings
#define IDS_MENU_REPLACE                (IDS_RANDO_BASE + 0x000)
#define IDS_MENU_CREATE                 (IDS_RANDO_BASE + 0x001)
#define IDS_MENU_WHATSTHIS              (IDS_RANDO_BASE + 0x002)
//#define IDS_MENU_HELPFINDER             (IDS_RANDO_BASE + 0x003)
#define IDS_MENU_UPDATE                 (IDS_RANDO_BASE + 0x004)
#define IDS_MENU_UPDATEALL              (IDS_RANDO_BASE + 0x005)
#define IDS_MENU_SKIP                   (IDS_RANDO_BASE + 0x006)
#define IDS_MENU_MERGE                  (IDS_RANDO_BASE + 0x007)
#define IDS_MENU_DELETE                 (IDS_RANDO_BASE + 0x008)
#define IDS_MENU_DONTDELETE             (IDS_RANDO_BASE + 0x009)

// Captions                             
#define IDS_CAP_ADD                     (IDS_RANDO_BASE + 0x010)
#define IDS_CAP_UPDATE                  (IDS_RANDO_BASE + 0x011)
#define IDS_CAP_INFO                    (IDS_RANDO_BASE + 0x012)
#define IDS_CAP_OPEN                    (IDS_RANDO_BASE + 0x013)
#define IDS_CAP_STATUS                  (IDS_RANDO_BASE + 0x014)
#define IDS_CAP_SAVE                    (IDS_RANDO_BASE + 0x015)
#define IDS_CAP_CREATE                  (IDS_RANDO_BASE + 0x016)
#define IDS_CAP_ReplaceFile             (IDS_RANDO_BASE + 0x017)    // Old
#define IDS_CAP_ReplaceFolder           (IDS_RANDO_BASE + 0x018)
#define IDS_CAP_ConfirmSplit            (IDS_RANDO_BASE + 0x019)
#define IDS_CAP_ConfirmMultiSplit       (IDS_RANDO_BASE + 0x01a)
#define IDS_CAP_Split                   (IDS_RANDO_BASE + 0x01b)
#define IDS_CAP_UpdateFmt               (IDS_RANDO_BASE + 0x01c)
#define IDS_CAP_UPDATING                (IDS_RANDO_BASE + 0x01d)
#define IDS_CAP_CHECKING                (IDS_RANDO_BASE + 0x01e)

// Random stuff
#define IDS_YES                         (IDS_RANDO_BASE + 0x100)
#define IDS_NO                          (IDS_RANDO_BASE + 0x101)
#define IDS_OK                          (IDS_RANDO_BASE + 0x102)
#define IDS_CANCEL                      (IDS_RANDO_BASE + 0x103)
#define IDS_RETRY                       (IDS_RANDO_BASE + 0x104)
#define IDS_YESTOALL                    (IDS_RANDO_BASE + 0x105)

//#define IDS_NoOriginal                  (IDS_RANDO_BASE + 0x110)    // old
#define IDS_InLocation                  (IDS_RANDO_BASE + 0x111)
#define IDS_InBriefcase                 (IDS_RANDO_BASE + 0x112)
#define IDS_BYTES                       (IDS_RANDO_BASE + 0x113)
#define IDS_BOGUSDBTEMPLATE             (IDS_RANDO_BASE + 0x114)
#define IDS_BC_DATABASE                 (IDS_RANDO_BASE + 0x115)
#define IDS_BC_NAME                     (IDS_RANDO_BASE + 0x116)
#define IDS_DATESIZELINE                (IDS_RANDO_BASE + 0x117)
#define IDS_ORDERKB                     (IDS_RANDO_BASE + 0x118)
#define IDS_ORDERMB                     (IDS_RANDO_BASE + 0x119)
#define IDS_ORDERGB                     (IDS_RANDO_BASE + 0x11a)
#define IDS_ORDERTB                     (IDS_RANDO_BASE + 0x11b)
#define IDS_BC_DATABASE_SHORT           (IDS_RANDO_BASE + 0x11c)
#define IDS_BC_NAME_SHORT               (IDS_RANDO_BASE + 0x11d)
#define IDS_ALTNAME                     (IDS_RANDO_BASE + 0x11e)

// Status Property sheet
#define IDS_STATPROP_SubfolderTwin      (IDS_RANDO_BASE + 0x300)
#define IDS_STATPROP_OrphanFolder       (IDS_RANDO_BASE + 0x301)
#define IDS_STATPROP_OrphanFile         (IDS_RANDO_BASE + 0x302)
#define IDS_STATPROP_Uptodate           (IDS_RANDO_BASE + 0x303)
#define IDS_STATPROP_PressButton        (IDS_RANDO_BASE + 0x304)
#define IDS_STATPROP_Unavailable        (IDS_RANDO_BASE + 0x305)
#define IDS_STATPROP_Update             (IDS_RANDO_BASE + 0x306)
#define IDS_STATPROP_Conflict           (IDS_RANDO_BASE + 0x307)

// Confirm Replace dialog
//#define IDS_REPLACE_ReplaceFile         (IDS_RANDO_BASE + 0x340)    // Old
//#define IDS_REPLACE_ReplaceFolder       (IDS_RANDO_BASE + 0x341)    // Old
//#define IDS_REPLACE_WithFile            (IDS_RANDO_BASE + 0x342)    // Old
//#define IDS_REPLACE_WithFolder          (IDS_RANDO_BASE + 0x343)    // Old
//#define IDS_REPLACE_ReplaceOrphan       (IDS_RANDO_BASE + 0x344)    // Old
#define IDS_MSG_ConfirmFileReplace      (IDS_RANDO_BASE + 0x345)
#define IDS_MSG_ConfirmFileReplace_RO   (IDS_RANDO_BASE + 0x346)
#define IDS_MSG_ConfirmFileReplace_Sys  (IDS_RANDO_BASE + 0x347)
#define IDS_MSG_ConfirmFolderReplace    (IDS_RANDO_BASE + 0x348)

// Update progress dialog
#define IDS_UPDATE_Copy                 (IDS_RANDO_BASE + 0x380)
#define IDS_UPDATE_Merge                (IDS_RANDO_BASE + 0x381)
#define IDS_UPDATE_Delete               (IDS_RANDO_BASE + 0x382)

// States of sync copies
#define IDS_STATE_Creates               (IDS_RANDO_BASE + 0x400)
#define IDS_STATE_Replaces              (IDS_RANDO_BASE + 0x401)
#define IDS_STATE_Skip                  (IDS_RANDO_BASE + 0x402)
#define IDS_STATE_Conflict              (IDS_RANDO_BASE + 0x403)
#define IDS_STATE_Merge                 (IDS_RANDO_BASE + 0x404)
#define IDS_STATE_Uptodate              (IDS_RANDO_BASE + 0x405)
#define IDS_STATE_NeedToUpdate          (IDS_RANDO_BASE + 0x406)
#define IDS_STATE_Orphan                (IDS_RANDO_BASE + 0x407)
#define IDS_STATE_Subfolder             (IDS_RANDO_BASE + 0x408)
#define IDS_STATE_Changed               (IDS_RANDO_BASE + 0x409)
#define IDS_STATE_Unchanged             (IDS_RANDO_BASE + 0x40a)
#define IDS_STATE_NewFile               (IDS_RANDO_BASE + 0x40b)
#define IDS_STATE_Unavailable           (IDS_RANDO_BASE + 0x40c)
#define IDS_STATE_UptodateInBrf         (IDS_RANDO_BASE + 0x40d)
#define IDS_STATE_SystemFile            (IDS_RANDO_BASE + 0x40e)
#define IDS_STATE_Delete                (IDS_RANDO_BASE + 0x40f)
#define IDS_STATE_DontDelete            (IDS_RANDO_BASE + 0x410)
#define IDS_STATE_DoesNotExist          (IDS_RANDO_BASE + 0x411)
#define IDS_STATE_Deleted               (IDS_RANDO_BASE + 0x412)
                                          
