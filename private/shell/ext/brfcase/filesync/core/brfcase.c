/*
 * brfcase.c - Briefcase ADT module.
 */

/*



*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "findbc.h"


/* Constants
 ************/

/* database file attributes */

#define DB_FILE_ATTR                (FILE_ATTRIBUTE_HIDDEN)

/* database cache lengths */

#define DEFAULT_DATABASE_CACHE_LEN  (32)
#define MAX_DATABASE_CACHE_LEN      (32 * 1024)

/* string table allocation constants */

#define NUM_NAME_HASH_BUCKETS       (67)


/* Types
 ********/

/* briefcase database description */

typedef struct _brfcasedb
{
   /*
    * handle to path of folder of open database (stored in briefcase path list)
    */

   HPATH hpathDBFolder;

   /* name of database file */

   LPTSTR pszDBName;

   /* handle to open cached database file */

   HCACHEDFILE hcfDB;

   /*
    * handle to path of folder that database was last saved in (stored in
    * briefcase's path list), only valid during OpenBriefcase() and
    * SaveBriefcase()
    */

   HPATH hpathLastSavedDBFolder;
}
BRFCASEDB;
DECLARE_STANDARD_TYPES(BRFCASEDB);

/*
 * briefcase flags
 *
 * N.b., the private BR_ flags must not collide with the public OB_ flags!
 */

typedef enum _brfcaseflags
{
   /* The briefcase database has been opened. */

   BR_FL_DATABASE_OPENED      = 0x00010000,

   /* The pimkRoot field is valid. */

   BR_FL_ROOT_MONIKER_VALID   = 0x00020000,

#ifdef DEBUG

   /* Briefcase is being deleted. */

   BR_FL_BEING_DELETED        = 0x01000000,

#endif

   /* flag combinations */

   ALL_BR_FLAGS               = (BR_FL_DATABASE_OPENED |
                                 BR_FL_ROOT_MONIKER_VALID
#ifdef DEBUG
                                 | BR_FL_BEING_DELETED
#endif
                                ),

   ALL_BRFCASE_FLAGS          = (ALL_OB_FLAGS |
                                 ALL_BR_FLAGS)
}
BRFCASEFLAGS;

/* briefcase structure */

typedef struct _brfcase
{
   /* flags */

   DWORD dwFlags;

   /* handle to name string table */

   HSTRINGTABLE hstNames;

   /* handle to list of paths */

   HPATHLIST hpathlist;

   /* handle to array of pointers to twin families */

   HPTRARRAY hpaTwinFamilies;

   /* handle to array of pointers to folder pairs */

   HPTRARRAY hpaFolderPairs;

   /*
    * handle to parent window, only valid if OB_FL_ALLOW_UI is set in dwFlags
    * field
    */

   HWND hwndOwner;

   /* briewfcase database folder moniker */

   PIMoniker pimkRoot;

   /* database description */

   BRFCASEDB bcdb;
}
BRFCASE;
DECLARE_STANDARD_TYPES(BRFCASE);

/* database briefcase structure */

typedef struct _dbbrfcase
{
   /* old handle to folder that database was saved in */

   HPATH hpathLastSavedDBFolder;
}
DBBRFCASE;
DECLARE_STANDARD_TYPES(DBBRFCASE);

#ifdef DEBUG

/* debug flags */

typedef enum _dbdebugflags
{
   BRFCASE_DFL_NO_DB_SAVE     = 0x0001,

   BRFCASE_DFL_NO_DB_RESTORE  = 0x0002,

   ALL_BRFCASE_DFLAGS         = (BRFCASE_DFL_NO_DB_SAVE |
                                 BRFCASE_DFL_NO_DB_RESTORE)
}
DBDEBUGFLAGS;

#endif


/* Module Variables
 *******************/

#pragma data_seg(DATA_SEG_SHARED)

/*
 * RAIDRAID: (16273) The use of MnrcsBriefcase in a shared data section is
 * broken under NT.  To run under NT, this code should be changed to use a
 * shared mutex.
 */

/* critical section used for briefcase access serialization */

PRIVATE_DATA NONREENTRANTCRITICALSECTION MnrcsBriefcase =
{
   { 0 },

#ifdef DEBUG
   INVALID_THREAD_ID,
#endif   /* DEBUG */

   FALSE
};

/* open briefcases */

PRIVATE_DATA HLIST MhlistOpenBriefcases = NULL;

/* database cache size */

PRIVATE_DATA DWORD MdwcbMaxDatabaseCacheLen = MAX_DATABASE_CACHE_LEN;

#pragma data_seg()

#ifdef DEBUG

#pragma data_seg(DATA_SEG_SHARED)

/* debug flags */

PRIVATE_DATA DWORD MdwBriefcaseModuleFlags = 0;

#pragma data_seg(DATA_SEG_READ_ONLY)

/* .ini file switch descriptions */

PRIVATE_DATA CBOOLINISWITCH cbisNoDatabaseSave =
{
   IST_BOOL,
   TEXT("NoDatabaseSave"),
   &MdwBriefcaseModuleFlags,
   BRFCASE_DFL_NO_DB_SAVE
};

PRIVATE_DATA CBOOLINISWITCH cbisNoDatabaseRestore =
{
   IST_BOOL,
   TEXT("NoDatabaseRestore"),
   &MdwBriefcaseModuleFlags,
   BRFCASE_DFL_NO_DB_RESTORE
};

PRIVATE_DATA CUNSDECINTINISWITCH cdiisMaxDatabaseCacheLen =
{
   IST_UNS_DEC_INT,
   TEXT("MaxDatabaseCacheLen"),
   (PUINT)&MdwcbMaxDatabaseCacheLen
};

PRIVATE_DATA const PCVOID MrgcpcvisBriefcaseModule[] =
{
   &cbisNoDatabaseSave,
   &cbisNoDatabaseRestore,
   &cdiisMaxDatabaseCacheLen
};

#pragma data_seg()

#endif   /* DEBUG */


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE TWINRESULT OpenBriefcaseDatabase(PBRFCASE, LPCTSTR);
PRIVATE_CODE TWINRESULT CloseBriefcaseDatabase(PBRFCASEDB);
PRIVATE_CODE BOOL CreateBriefcase(PBRFCASE *, DWORD, HWND);
PRIVATE_CODE void UnlinkBriefcase(PBRFCASE);
PRIVATE_CODE TWINRESULT DestroyBriefcase(PBRFCASE);
PRIVATE_CODE TWINRESULT MyWriteDatabase(PBRFCASE);
PRIVATE_CODE TWINRESULT MyReadDatabase(PBRFCASE, DWORD);

#ifdef DEBUG

PRIVATE_CODE BOOL DestroyBriefcaseWalker(PVOID, PVOID);

#endif

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCBRFCASE(PCBRFCASE);
PRIVATE_CODE BOOL IsValidPCBRFCASEDB(PCBRFCASEDB);
PRIVATE_CODE BOOL IsValidPCOPENBRFCASEINFO(PCOPENBRFCASEINFO);

#endif


/*
** OpenBriefcaseDatabase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT OpenBriefcaseDatabase(PBRFCASE pbr, LPCTSTR pcszPath)
{
   TWINRESULT tr;
   TCHAR rgchCanonicalPath[MAX_SEPARATED_PATH_LEN];
   DWORD dwOutFlags;
   TCHAR rgchNetResource[MAX_PATH_LEN];
   LPTSTR pszRootPathSuffix;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));

   if (GetCanonicalPathInfo(pcszPath, rgchCanonicalPath, &dwOutFlags,
                            rgchNetResource, &pszRootPathSuffix))
   {
      LPTSTR pszDBName;

      pszDBName = (LPTSTR)ExtractFileName(pszRootPathSuffix);

      ASSERT(IS_SLASH(*(pszDBName - 1)));

      if (StringCopy(pszDBName, &(pbr->bcdb.pszDBName)))
      {
         if (pszDBName == pszRootPathSuffix)
         {
            /* Database in root. */

            *pszDBName = TEXT('\0');

            ASSERT(IsRootPath(rgchCanonicalPath));
         }
         else
         {
            ASSERT(pszDBName > pszRootPathSuffix);
            *(pszDBName - 1) = TEXT('\0');
         }

         tr = TranslatePATHRESULTToTWINRESULT(
                  AddPath(pbr->hpathlist, rgchCanonicalPath,
                          &(pbr->bcdb.hpathDBFolder)));

         if (tr == TR_SUCCESS)
         {
            if (IsPathVolumeAvailable(pbr->bcdb.hpathDBFolder))
            {
               TCHAR rgchDBPath[MAX_PATH_LEN];
               CACHEDFILE cfDB;

               GetPathString(pbr->bcdb.hpathDBFolder, rgchDBPath);
               CatPath(rgchDBPath, pbr->bcdb.pszDBName);

               /* Assume sequential reads and writes. */

               /* Share read access, but not write access. */

               cfDB.pcszPath = rgchDBPath;
               cfDB.dwOpenMode = (GENERIC_READ | GENERIC_WRITE);
               cfDB.dwSharingMode = FILE_SHARE_READ;
               cfDB.psa = NULL;
               cfDB.dwCreateMode = OPEN_ALWAYS;
               cfDB.dwAttrsAndFlags = (DB_FILE_ATTR | FILE_FLAG_SEQUENTIAL_SCAN);
               cfDB.hTemplateFile = NULL;
               cfDB.dwcbDefaultCacheSize = DEFAULT_DATABASE_CACHE_LEN;

               tr = TranslateFCRESULTToTWINRESULT(
                     CreateCachedFile(&cfDB, &(pbr->bcdb.hcfDB)));

               if (tr == TR_SUCCESS)
               {
                  pbr->bcdb.hpathLastSavedDBFolder = NULL;

                  ASSERT(IS_FLAG_CLEAR(pbr->dwFlags, BR_FL_DATABASE_OPENED));
                  SET_FLAG(pbr->dwFlags, BR_FL_DATABASE_OPENED);
               }
               else
               {
                  DeletePath(pbr->bcdb.hpathDBFolder);
OPENBRIEFCASEDATABASE_BAIL:
                  FreeMemory(pbr->bcdb.pszDBName);
               }
            }
            else
            {
               tr = TR_UNAVAILABLE_VOLUME;
               goto OPENBRIEFCASEDATABASE_BAIL;
            }
         }
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TWINRESULTFromLastError(TR_INVALID_PARAMETER);

   return(tr);
}


/*
** CloseBriefcaseDatabase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT CloseBriefcaseDatabase(PBRFCASEDB pbcdb)
{
   TWINRESULT tr;
   TCHAR rgchDBPath[MAX_PATH_LEN];
   FILESTAMP fsDB;

   tr = CloseCachedFile(pbcdb->hcfDB) ? TR_SUCCESS : TR_BRIEFCASE_WRITE_FAILED;

   if (tr == TR_SUCCESS)
      TRACE_OUT((TEXT("CloseBriefcaseDatabase(): Closed cached briefcase database file %s\\%s."),
                 DebugGetPathString(pbcdb->hpathDBFolder),
                 pbcdb->pszDBName));
   else
      WARNING_OUT((TEXT("CloseBriefcaseDatabase(): Failed to close cached briefcase database file %s\\%s."),
                   DebugGetPathString(pbcdb->hpathDBFolder),
                   pbcdb->pszDBName));

   /* Try not to leave a 0-length database laying around. */

   GetPathString(pbcdb->hpathDBFolder, rgchDBPath);
   CatPath(rgchDBPath, pbcdb->pszDBName);

   MyGetFileStamp(rgchDBPath, &fsDB);

   if (fsDB.fscond == FS_COND_EXISTS &&
       (! fsDB.dwcbLowLength && ! fsDB.dwcbHighLength))
   {
      if (DeleteFile(rgchDBPath))
         WARNING_OUT((TEXT("CloseBriefcaseDatabase(): Deleted 0 length database %s\\%s."),
                      DebugGetPathString(pbcdb->hpathDBFolder),
                      pbcdb->pszDBName));
   }

   FreeMemory(pbcdb->pszDBName);
   DeletePath(pbcdb->hpathDBFolder);

   return(tr);
}


/*
** CreateBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateBriefcase(PBRFCASE *ppbr, DWORD dwInFlags,
                                  HWND hwndOwner)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_WRITE_PTR(ppbr, PBRFCASE));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_BRFCASE_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, OB_FL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));

   if (AllocateMemory(sizeof(**ppbr), ppbr))
   {
      DWORD dwCPLFlags;

      dwCPLFlags = (RLI_IFL_CONNECT |
                    RLI_IFL_UPDATE |
                    RLI_IFL_LOCAL_SEARCH);

      if (IS_FLAG_SET(dwInFlags, OB_FL_ALLOW_UI))
         SET_FLAG(dwCPLFlags, RLI_IFL_ALLOW_UI);

      if (CreatePathList(dwCPLFlags, hwndOwner, &((*ppbr)->hpathlist)))
      {
         NEWSTRINGTABLE nszt;

         nszt.hbc = NUM_NAME_HASH_BUCKETS;

         if (CreateStringTable(&nszt, &((*ppbr)->hstNames)))
         {
            if (CreateTwinFamilyPtrArray(&((*ppbr)->hpaTwinFamilies)))
            {
               if (CreateFolderPairPtrArray(&((*ppbr)->hpaFolderPairs)))
               {
                  HNODE hnode;

                  if (InsertNodeAtFront(MhlistOpenBriefcases, NULL, (*ppbr), &hnode))
                  {
                     (*ppbr)->dwFlags = dwInFlags;
                     (*ppbr)->hwndOwner = hwndOwner;

                     bResult = TRUE;
                  }
                  else
                  {
                     DestroyFolderPairPtrArray((*ppbr)->hpaFolderPairs);
CREATEBRIEFCASE_BAIL1:
                     DestroyTwinFamilyPtrArray((*ppbr)->hpaTwinFamilies);
CREATEBRIEFCASE_BAIL2:
                     DestroyStringTable((*ppbr)->hstNames);
CREATEBRIEFCASE_BAIL3:
                     DestroyPathList((*ppbr)->hpathlist);
CREATEBRIEFCASE_BAIL4:
                     FreeMemory((*ppbr));
                  }
               }
               else
                  goto CREATEBRIEFCASE_BAIL1;
            }
            else
               goto CREATEBRIEFCASE_BAIL2;
         }
         else
            goto CREATEBRIEFCASE_BAIL3;
      }
      else
         goto CREATEBRIEFCASE_BAIL4;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppbr, CBRFCASE));

   return(bResult);
}


/*
** UnlinkBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void UnlinkBriefcase(PBRFCASE pbr)
{
   BOOL bContinue;
   HNODE hnode;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));

   for (bContinue = GetFirstNode(MhlistOpenBriefcases, &hnode);
        bContinue;
        bContinue = GetNextNode(hnode, &hnode))
   {
      PBRFCASE pbrTest;

      pbrTest = GetNodeData(hnode);

      ASSERT(IS_VALID_STRUCT_PTR(pbrTest, CBRFCASE));

      if (pbrTest == pbr)
      {
         DeleteNode(hnode);
         break;
      }
   }

   ASSERT(bContinue);

   return;
}


/*
** DestroyBriefcase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT DestroyBriefcase(PBRFCASE pbr)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));

#ifdef DEBUG

   SET_FLAG(pbr->dwFlags, BR_FL_BEING_DELETED);

#endif

   if (IS_FLAG_SET(pbr->dwFlags, BR_FL_DATABASE_OPENED))
      tr = CloseBriefcaseDatabase(&(pbr->bcdb));
   else
      tr = TR_SUCCESS;

   if (IS_FLAG_SET(pbr->dwFlags, BR_FL_ROOT_MONIKER_VALID))
      pbr->pimkRoot->lpVtbl->Release(pbr->pimkRoot);

   DestroyFolderPairPtrArray(pbr->hpaFolderPairs);

   DestroyTwinFamilyPtrArray(pbr->hpaTwinFamilies);

   ASSERT(! GetStringCount(pbr->hstNames));
   DestroyStringTable(pbr->hstNames);

   ASSERT(! GetPathCount(pbr->hpathlist));
   DestroyPathList(pbr->hpathlist);

   FreeMemory(pbr);

   return(tr);
}


/*
** MyWriteDatabase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MyWriteDatabase(PBRFCASE pbr)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));

   ASSERT(IS_FLAG_SET(pbr->dwFlags, BR_FL_DATABASE_OPENED));

#ifdef DEBUG
   if (IS_FLAG_CLEAR(MdwBriefcaseModuleFlags, BRFCASE_DFL_NO_DB_SAVE))
#endif
   {
      /* Grow the database cache in preparation for writing. */

      ASSERT(MdwcbMaxDatabaseCacheLen > 0);

      if (SetCachedFileCacheSize(pbr->bcdb.hcfDB, MdwcbMaxDatabaseCacheLen)
          != FCR_SUCCESS)
         WARNING_OUT((TEXT("MyWriteDatabase(): Unable to grow database cache to %lu bytes.  Using default database write cache of %lu bytes."),
                      MdwcbMaxDatabaseCacheLen,
                      (DWORD)DEFAULT_DATABASE_CACHE_LEN));

      /* Write the database. */

      tr = WriteTwinDatabase(pbr->bcdb.hcfDB, (HBRFCASE)pbr);

      if (tr == TR_SUCCESS)
      {
         if (CommitCachedFile(pbr->bcdb.hcfDB))
         {
            /* Shrink the database cache back down to its default size. */

            EVAL(SetCachedFileCacheSize(pbr->bcdb.hcfDB,
                                        DEFAULT_DATABASE_CACHE_LEN)
                 == FCR_SUCCESS);

            TRACE_OUT((TEXT("MyWriteDatabase(): Wrote database %s\\%s."),
                       DebugGetPathString(pbr->bcdb.hpathDBFolder),
                       pbr->bcdb.pszDBName));
         }
         else
            tr = TR_BRIEFCASE_WRITE_FAILED;
      }
   }
#ifdef DEBUG
   else
   {
      WARNING_OUT((TEXT("MyWriteDatabase(): Twin database %s\\%s not saved, by request."),
                   DebugGetPathString(pbr->bcdb.hpathDBFolder),
                   pbr->bcdb.pszDBName));

      tr = TR_SUCCESS;
   }
#endif

   return(tr);
}


/*
** MyReadDatabase()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT MyReadDatabase(PBRFCASE pbr, DWORD dwInFlags)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_OB_FLAGS));

#ifdef DEBUG
   if (IS_FLAG_CLEAR(MdwBriefcaseModuleFlags, BRFCASE_DFL_NO_DB_RESTORE))
#endif
   {
      DWORD dwcbDatabaseSize;

      /* Is there an exising database to read? */

      dwcbDatabaseSize = GetCachedFileSize(pbr->bcdb.hcfDB);

      if (dwcbDatabaseSize > 0)
      {
         DWORD dwcbMaxCacheSize;

         /* Yes.  Grow the database cache in preparation for reading. */

         /*
          * Use file length instead of MdwcbMaxDatabaseCacheLen if file length
          * is smaller.
          */

         ASSERT(MdwcbMaxDatabaseCacheLen > 0);

         if (dwcbDatabaseSize < MdwcbMaxDatabaseCacheLen)
         {
            dwcbMaxCacheSize = dwcbDatabaseSize;

            WARNING_OUT((TEXT("MyReadDatabase(): Using file size %lu bytes as read cache size for database %s\\%s."),
                         dwcbDatabaseSize,
                         DebugGetPathString(pbr->bcdb.hpathDBFolder),
                         pbr->bcdb.pszDBName));
         }
         else
            dwcbMaxCacheSize = MdwcbMaxDatabaseCacheLen;

         if (TranslateFCRESULTToTWINRESULT(SetCachedFileCacheSize(
                                                            pbr->bcdb.hcfDB,
                                                            dwcbMaxCacheSize))
             != TR_SUCCESS)
            WARNING_OUT((TEXT("MyReadDatabase(): Unable to grow database cache to %lu bytes.  Using default database read cache of %lu bytes."),
                         dwcbMaxCacheSize,
                         (DWORD)DEFAULT_DATABASE_CACHE_LEN));

         tr = ReadTwinDatabase((HBRFCASE)pbr, pbr->bcdb.hcfDB);

         if (tr == TR_SUCCESS)
         {
            ASSERT(! pbr->bcdb.hpathLastSavedDBFolder ||
                   IS_VALID_HANDLE(pbr->bcdb.hpathLastSavedDBFolder, PATH));

            if (pbr->bcdb.hpathLastSavedDBFolder)
            {
               if (IS_FLAG_SET(dwInFlags, OB_FL_TRANSLATE_DB_FOLDER) &&
                   ComparePaths(pbr->bcdb.hpathLastSavedDBFolder,
                                pbr->bcdb.hpathDBFolder) != CR_EQUAL)
                  tr = MyTranslateFolder((HBRFCASE)pbr,
                                         pbr->bcdb.hpathLastSavedDBFolder,
                                         pbr->bcdb.hpathDBFolder);

               DeletePath(pbr->bcdb.hpathLastSavedDBFolder);
               pbr->bcdb.hpathLastSavedDBFolder = NULL;
            }

            if (tr == TR_SUCCESS)
               TRACE_OUT((TEXT("MyReadDatabase(): Read database %s\\%s."),
                          DebugGetPathString(pbr->bcdb.hpathDBFolder),
                          pbr->bcdb.pszDBName));
         }

         /* Shrink the database cache back down to its default size. */

         EVAL(TranslateFCRESULTToTWINRESULT(SetCachedFileCacheSize(
                                                   pbr->bcdb.hcfDB,
                                                   DEFAULT_DATABASE_CACHE_LEN))
              == TR_SUCCESS);
      }
      else
      {
         tr = TR_SUCCESS;

         WARNING_OUT((TEXT("MyReadDatabase(): Database %s\\%s not found."),
                      DebugGetPathString(pbr->bcdb.hpathDBFolder),
                      pbr->bcdb.pszDBName));
      }
   }
#ifdef DEBUG
   else
   {
      WARNING_OUT((TEXT("MyReadDatabase(): Twin database %s\\%s not read, by request."),
                   DebugGetPathString(pbr->bcdb.hpathDBFolder),
                   pbr->bcdb.pszDBName));

      tr = TR_SUCCESS;
   }
#endif

   return(tr);
}


#ifdef DEBUG

/*
** DestroyBriefcaseWalker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

#pragma warning(disable:4100) /* "unreferenced formal parameter" warning */

PRIVATE_CODE BOOL DestroyBriefcaseWalker(PVOID pbr, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pbr, CBRFCASE));
   ASSERT(! pvUnused);

   EVAL(DestroyBriefcase(pbr) == TR_SUCCESS);

   return(TRUE);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */

#endif


#ifdef VSTF

/*
** IsValidPCBRFCASE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCBRFCASE(PCBRFCASE pcbr)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcbr, CBRFCASE))
   {
      if (FLAGS_ARE_VALID(pcbr->dwFlags, ALL_BRFCASE_FLAGS))
      {
#ifdef DEBUG
         if (IS_FLAG_SET(pcbr->dwFlags, BR_FL_BEING_DELETED))
            bResult = TRUE;
         else
#endif
            bResult = (IS_VALID_HANDLE(pcbr->hstNames, STRINGTABLE) &&
                       IS_VALID_HANDLE(pcbr->hpathlist, PATHLIST) &&
                       IS_VALID_HANDLE(pcbr->hpaTwinFamilies, PTRARRAY) &&
                       IS_VALID_HANDLE(pcbr->hpaFolderPairs, PTRARRAY) &&
                       (IS_FLAG_CLEAR(pcbr->dwFlags, OB_FL_ALLOW_UI) ||
                        IS_VALID_HANDLE(pcbr->hwndOwner, WND)) &&
                       (IS_FLAG_CLEAR(pcbr->dwFlags, BR_FL_ROOT_MONIKER_VALID) ||
                        IS_VALID_STRUCT_PTR(pcbr->pimkRoot, CIMoniker)) &&
                       (IS_FLAG_CLEAR(pcbr->dwFlags, BR_FL_DATABASE_OPENED) ||
                        (IS_FLAG_SET(pcbr->dwFlags, OB_FL_OPEN_DATABASE) &&
                         IS_VALID_STRUCT_PTR(&(pcbr->bcdb), CBRFCASEDB))));
      }
   }

   return(bResult);
}


/*
** IsValidPCBRFCASEDB()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCBRFCASEDB(PCBRFCASEDB pcbcdb)
{
   return(IS_VALID_READ_PTR(pcbcdb, CBRFCASEDB) &&
          IS_VALID_HANDLE(pcbcdb->hpathDBFolder, PATH) &&
          IS_VALID_STRING_PTR(pcbcdb->pszDBName, STR) &&
          IS_VALID_HANDLE(pcbcdb->hcfDB, CACHEDFILE) &&
          (! pcbcdb->hpathLastSavedDBFolder ||
           IS_VALID_HANDLE(pcbcdb->hpathLastSavedDBFolder, PATH)));
}


/*
** IsValidPCOPENBRFCASEINFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCOPENBRFCASEINFO(PCOPENBRFCASEINFO pcobri)
{
   return(IS_VALID_READ_PTR(pcobri, COPENBRFCASEINFO) &&
          EVAL(pcobri->ulSize == sizeof(*pcobri)) &&
          FLAGS_ARE_VALID(pcobri->dwFlags, ALL_OB_FLAGS) &&
          ((IS_FLAG_CLEAR(pcobri->dwFlags, OB_FL_ALLOW_UI) &&
            ! pcobri->hwndOwner) ||
           (IS_FLAG_SET(pcobri->dwFlags, OB_FL_ALLOW_UI) &&
            IS_VALID_HANDLE(pcobri->hwndOwner, WND))) &&
          ((IS_FLAG_CLEAR(pcobri->dwFlags, OB_FL_OPEN_DATABASE) &&
            ! pcobri->hvid &&
            ! (pcobri->rgchDatabasePath[0]))) ||
          ((IS_FLAG_SET(pcobri->dwFlags, OB_FL_OPEN_DATABASE) &&
            IS_VALID_HANDLE(pcobri->hvid, VOLUMEID) &&
            IS_VALID_STRING_PTR(pcobri->rgchDatabasePath, CSTR))));
}

#endif


/****************************** Public Functions *****************************/


#ifdef DEBUG

/*
** SetBriefcaseModuleIniSwitches()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SetBriefcaseModuleIniSwitches(void)
{
   BOOL bResult;

   bResult = SetIniSwitches(MrgcpcvisBriefcaseModule,
                            ARRAY_ELEMENTS(MrgcpcvisBriefcaseModule));

   if (! EVAL(MdwcbMaxDatabaseCacheLen > 0))
   {
      MdwcbMaxDatabaseCacheLen = 1;

      WARNING_OUT((TEXT("SetBriefcaseModuleIniSwitches(): Using maximum database cache length of %lu."),
                   MdwcbMaxDatabaseCacheLen));
   }

   ASSERT(FLAGS_ARE_VALID(MdwBriefcaseModuleFlags, ALL_BRFCASE_DFLAGS));

   return(bResult);
}

#endif


/*
** InitBriefcaseModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL InitBriefcaseModule(void)
{
   NEWLIST nl;

   ASSERT(! MhlistOpenBriefcases);

   /* Create the module list of open briefcases. */

   ReinitializeNonReentrantCriticalSection(&MnrcsBriefcase);

   nl.dwFlags = 0;

   return(CreateList(&nl, &MhlistOpenBriefcases));
}


/*
** ExitBriefcaseModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ExitBriefcaseModule(void)
{

#ifdef DEBUG

   if (MhlistOpenBriefcases)
   {
      /* Destroy all open briefcases. */

      /*
       * Don't destroy the list of open briefcases in the retail build.  Assume
       * that callers will have closed all open briefcases, so that there are
       * no remaining connections to shut down.
       */

      EVAL(WalkList(MhlistOpenBriefcases, &DestroyBriefcaseWalker, NULL));

      /* Now wipe out the list. */

      DestroyList(MhlistOpenBriefcases);
      MhlistOpenBriefcases = NULL;
   }
   else
      WARNING_OUT((TEXT("ExitBriefcaseModule() called when MhlistOpenBriefcases is NULL.")));

#endif

   return;
}


/*
** GetBriefcaseNameStringTable()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HSTRINGTABLE GetBriefcaseNameStringTable(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hstNames);
}


/*
** GetBriefcaseTwinFamilyPtrArray()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HPTRARRAY GetBriefcaseTwinFamilyPtrArray(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hpaTwinFamilies);
}


/*
** GetBriefcaseFolderPairPtrArray()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HPTRARRAY GetBriefcaseFolderPairPtrArray(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hpaFolderPairs);
}


/*
** GetBriefcasePathList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HPATHLIST GetBriefcasePathList(HBRFCASE hbr)
{
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   return(((PCBRFCASE)hbr)->hpathlist);
}


/*
** GetBriefcaseRootMoniker()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HRESULT GetBriefcaseRootMoniker(HBRFCASE hbr, PIMoniker *pimk)
{
   HRESULT hr;

   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_WRITE_PTR(pimk, CIMoniker));

   if (IS_FLAG_CLEAR(((PCBRFCASE)hbr)->dwFlags, BR_FL_ROOT_MONIKER_VALID))
   {
      if (IS_FLAG_SET(((PCBRFCASE)hbr)->dwFlags, BR_FL_DATABASE_OPENED))
      {
         TCHAR rgchRoot[MAX_PATH_LEN];
         WCHAR rgwchUnicodeRoot[MAX_PATH_LEN];
         PIMoniker pimkRoot;

         GetPathString(((PCBRFCASE)hbr)->bcdb.hpathDBFolder, rgchRoot);

#ifdef UNICODE

         hr = CreateFileMoniker(rgchRoot, &pimkRoot);
#else
       
         /* Translate ANSI string into Unicode for OLE. */

         if (0 == MultiByteToWideChar(CP_ACP, 0, rgchRoot, -1, rgwchUnicodeRoot,
                                 ARRAY_ELEMENTS(rgwchUnicodeRoot)))
         {
            hr = MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
         }
         else
         {            
            hr = CreateFileMoniker(rgwchUnicodeRoot, &pimkRoot);
         }

#endif

         if (SUCCEEDED(hr))
         {
            ((PBRFCASE)hbr)->pimkRoot = pimkRoot;
            SET_FLAG(((PBRFCASE)hbr)->dwFlags, BR_FL_ROOT_MONIKER_VALID);

            TRACE_OUT((TEXT("GetBriefcaseRootMoniker(): Created briefcase root moniker %s."),
                       rgchRoot));
         }
      }
      else
         hr = MAKE_SCODE(SEVERITY_ERROR, FACILITY_WIN32, ERROR_PATH_NOT_FOUND);
   }
   else
      hr = S_OK;

   if (SUCCEEDED(hr))
      *pimk = ((PCBRFCASE)hbr)->pimkRoot;

   ASSERT(FAILED(hr) ||
          IS_VALID_STRUCT_PTR(*pimk, CIMoniker));

   return(hr);
}


/*
** BeginExclusiveBriefcaseAccess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL BeginExclusiveBriefcaseAccess(void)
{
   return(EnterNonReentrantCriticalSection(&MnrcsBriefcase));
}


/*
** EndExclusiveBriefcaseAccess()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void EndExclusiveBriefcaseAccess(void)
{
   LeaveNonReentrantCriticalSection(&MnrcsBriefcase);

   return;
}


#ifdef DEBUG

/*
** BriefcaseAccessIsExclusive()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL BriefcaseAccessIsExclusive(void)
{
   return(NonReentrantCriticalSectionIsOwned(&MnrcsBriefcase));
}

#endif   /* DEBUG */


/*
** IsValidHBRFCASE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHBRFCASE(HBRFCASE hbr)
{
   return(IS_VALID_STRUCT_PTR((PCBRFCASE)hbr, CBRFCASE));
}


/*
** WriteBriefcaseInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT WriteBriefcaseInfo(HCACHEDFILE hcf, HBRFCASE hbr)
{
   TWINRESULT tr;
   DBBRFCASE dbbr;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));

   /* Set up briefcase database structure. */

   ASSERT(IS_VALID_HANDLE(((PCBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder, PATH));

   dbbr.hpathLastSavedDBFolder = ((PCBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder;

   /* Save briefcase database structure. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbbr, sizeof(dbbr), NULL))
   {
      tr = TR_SUCCESS;

      TRACE_OUT((TEXT("WriteBriefcaseInfo(): Wrote last saved database folder %s."),
                 DebugGetPathString(dbbr.hpathLastSavedDBFolder)));
   }
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


/*
** ReadBriefcaseInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ReadBriefcaseInfo(HCACHEDFILE hcf, HBRFCASE hbr,
                                    HHANDLETRANS hhtFolderTrans)
{
   TWINRESULT tr;
   DBBRFCASE dbbr;
   DWORD dwcbRead;
   HPATH hpath;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hbr, BRFCASE));
   ASSERT(IS_VALID_HANDLE(hhtFolderTrans, HANDLETRANS));

   /* Read briefcase database structure. */

   if ((ReadFromCachedFile(hcf, &dbbr, sizeof(dbbr), &dwcbRead) &&
        dwcbRead == sizeof(dbbr)) &&
       TranslateHandle(hhtFolderTrans, (HGENERIC)(dbbr.hpathLastSavedDBFolder),
                       (PHGENERIC)&hpath))
   {
      HPATH hpathLastSavedDBFolder;

      /*
       * Bump last saved database folder path's lock count in the briefcase's
       * path list.
       */

      if (CopyPath(hpath, ((PCBRFCASE)hbr)->hpathlist, &hpathLastSavedDBFolder))
      {
         ((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder = hpathLastSavedDBFolder;

         tr = TR_SUCCESS;

         TRACE_OUT((TEXT("ReadBriefcaseInfo(): Read last saved database folder %s."),
                    DebugGetPathString(((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder)));
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | OpenBriefcase | Opens an existing briefcase database, or
creates a new briefcase.

@parm PCSTR | pcszPath | A pointer to a path string indicating the briefcase
database to be opened or created.  This parameter is ignored unless the
OB_FL_OPEN_DATABASE flag is set in dwFlags.

@parm DWORD | dwInFlags | A bit mask of flags.  This parameter may be any
combination of the following values:
   OB_FL_OPEN_DATABASE - Open the briefcase database specified by pcszPath.
   OB_FL_TRANSLATE_DB_FOLDER - Translate the folder where the briefcase
   database was last saved to the folder where the briefcase database was
   opened.
   OB_FL_ALLOW_UI - Allow interaction with the user during briefcase
   operations.

@parm HWND | hwndOwner | A handle to the parent window to be used when
requesting user interaction.  This parameter is ignored if the OB_FL_ALLOW_UI
flag is clear.

@parm PHBRFCASE | phbr | A pointer to an HBRFCASE to be filled in with a
handle to the open briefcase.  *phbr is only valid if TR_SUCCESS is returned.

@rdesc If the briefcase was opened or created successfully, TR_SUCCESS is
returned, and *phbr contains a handle to the open briefcase.  Otherwise, the
briefcase was not opened or created successfully, the return value indicates
the error that occurred, and *phbr is undefined.

@comm If the OB_FL_OPEN_DATABASE flag is set in dwFlags, the database specified
by pcszPath is associated with the briefcase.  If the database specified does
not exist, the database is created.<nl>
If the OB_FL_OPEN_DATABASE flag is clear in dwFlags, no persistent database is
associated with the briefcase.  SaveBriefcase() will fail if called on a
briefcase with no associated database.<nl>
Once the caller is finished with the briefcase handle returned by
OpenBriefcase(), CloseBriefcase() should be called to release the briefcase.
SaveBriefcase() may be called before CloseBriefcase() to save the current
contents of the briefcase.

@xref SaveBriefcase CloseBriefcase

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI OpenBriefcase(LPCTSTR pcszPath, DWORD dwInFlags,
                                           HWND hwndOwner, PHBRFCASE phbr)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(OpenBriefcase);

#ifdef EXPV
      /* Verify parameters. */

      if (FLAGS_ARE_VALID(dwInFlags, ALL_OB_FLAGS) &&
          IS_VALID_WRITE_PTR(phbr, HBRFCASE) &&
          (IS_FLAG_CLEAR(dwInFlags, OB_FL_OPEN_DATABASE) ||
           IS_VALID_STRING_PTR(pcszPath, CSTR)) &&
          (IS_FLAG_CLEAR(dwInFlags, OB_FL_ALLOW_UI) ||
           IS_VALID_HANDLE(hwndOwner, WND)))
#endif
      {
         PBRFCASE pbr;

         if (CreateBriefcase(&pbr, dwInFlags, hwndOwner))
         {
            if (IS_FLAG_SET(dwInFlags, OB_FL_OPEN_DATABASE))
            {
               tr = OpenBriefcaseDatabase(pbr, pcszPath);

               if (tr == TR_SUCCESS)
               {
                  tr = MyReadDatabase(pbr, dwInFlags);

                  if (tr == TR_SUCCESS)
                  {
                     if (IS_FLAG_SET(dwInFlags, OB_FL_LIST_DATABASE))
                        EVAL(AddBriefcaseToSystem(pcszPath) == TR_SUCCESS);

                     *phbr = (HBRFCASE)pbr;
                  }
                  else
                  {
OPENBRIEFCASE_BAIL:
                     UnlinkBriefcase(pbr);
                     EVAL(DestroyBriefcase(pbr) == TR_SUCCESS);
                  }
               }
               else
                  goto OPENBRIEFCASE_BAIL;
            }
            else
            {
               *phbr = (HBRFCASE)pbr;
               tr = TR_SUCCESS;

               TRACE_OUT((TEXT("OpenBriefcase(): Opened briefcase %#lx with no associated database, by request."),
                          *phbr));
            }
         }
         else
            tr = TR_OUT_OF_MEMORY;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(OpenBriefcase, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | SaveBriefcase | Saves the contents of an open briefcase to a
briefcase database.

@parm HBRFCASE | hbr | A handle to the briefcase to be saved.  This handle may
be obtained by calling OpenBriefcase() with a briefcase database path and with
the OB_FL_OPEN_DATABASE flag set.  SaveBriefcase() will return
TR_INVALID_PARAMETER if called on a briefcase with no associated briefcase
database.

@rdesc If the contents of the briefcase was saved to the briefcase database
successfully, TR_SUCCESS is returned.  Otherwise, the contents of the briefcase
was not saved to the briefcase database successfully, and the return value
indicates the error that occurred.

@xref OpenBriefcase CloseBriefcase

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI SaveBriefcase(HBRFCASE hbr)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(SaveBriefcase);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_FLAG_SET(((PBRFCASE)hbr)->dwFlags, BR_FL_DATABASE_OPENED))
#endif
      {
         ((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder = ((PCBRFCASE)hbr)->bcdb.hpathDBFolder;

         tr = MyWriteDatabase((PBRFCASE)hbr);

         ((PBRFCASE)hbr)->bcdb.hpathLastSavedDBFolder = NULL;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(SaveBriefcase, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | CloseBriefcase | Closes an open briefcase.

@parm HBRFCASE | hbr | A handle to the briefcase to be closed.  This handle may
be obtained by calling OpenBriefcase().

@rdesc If the briefcase was closed successfully, TR_SUCCESS is returned.
Otherwise, the briefcase was not closed successfully, and the return value
indicates the error that occurred.

@xref OpenBriefcase SaveBriefcase

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI CloseBriefcase(HBRFCASE hbr)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(CloseBriefcase);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE))
#endif
      {
         UnlinkBriefcase((PBRFCASE)hbr);

         tr = DestroyBriefcase((PBRFCASE)hbr);
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(CloseBriefcase, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | DeleteBriefcase | Deletes a closed briefcase's database file.

@parm PCSTR | pcszPath | A pointer to a path string indicating the briefcase
database that is to be deleted.

@rdesc If the briefcase database was deleted successfully, TR_SUCCESS is
returned.  Otherwise, the briefcase database was not deleted successfully, and
the return value indicates the error that occurred.

@comm Clients should call DeleteBriefcase() instead of DeleteFile() to delete
an unwanted briefcase database so that the the synchronization engine may
verify that the given briefcase database is not in use before deleting it.

@xref OpenBriefcase SaveBriefcase CloseBriefcase

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI DeleteBriefcase(LPCTSTR pcszPath)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(DeleteBriefcase);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_STRING_PTR(pcszPath, CSTR))
#endif
      {
         /*
          * RAIDRAID: (16275) Check database header here to verify that the
          * file is a briefcase database file.
          */

         if (DeleteFile(pcszPath))
         {
            EVAL(RemoveBriefcaseFromSystem(pcszPath) == TR_SUCCESS);

            tr = TR_SUCCESS;
         }
         else
         {
            switch (GetLastError())
            {
               /* Returned when file opened by local machine. */
               case ERROR_SHARING_VIOLATION:
                  tr = TR_BRIEFCASE_LOCKED;
                  break;

               default:
                  tr = TR_BRIEFCASE_OPEN_FAILED;
                  break;
            }
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(DeleteBriefcase, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | GetOpenBriefcaseInfo | Describes an open briefcase.

@parm HBRFCASE | hbr | A handle to the open briefcase to be described.

@parm POPENBRFCASEINFO | pobri | A pointer to an OPENBRFCASEINFO to be filled
in with information describing the open briefcase.  The ulSize field of the
OPENBRFCASEINFO structure should be filled in with sizeof(OPENBRFCASEINFO)
before calling GetOpenBriefcaseInfo().

@rdesc If the open briefcase was described successfully, TR_SUCCESS is
returned.  Otherwise, the open briefcase was not described successfully, and
the return value indicates the error that occurred.

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI GetOpenBriefcaseInfo(HBRFCASE hbr,
                                                  POPENBRFCASEINFO pobri)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(GetBriefcaseInfo);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE) &&
          IS_VALID_WRITE_PTR(pobri, OPENBRFCASEINFO) &&
          EVAL(pobri->ulSize == sizeof(*pobri)))
#endif
      {
         pobri->dwFlags = (((PBRFCASE)hbr)->dwFlags & ~ALL_BR_FLAGS);

         if (IS_FLAG_SET(((PBRFCASE)hbr)->dwFlags, OB_FL_ALLOW_UI))
            pobri->hwndOwner = ((PBRFCASE)hbr)->hwndOwner;
         else
         {
            pobri->hwndOwner = NULL;

            WARNING_OUT((TEXT("GetBriefcaseInfo(): Briefcase %#lx has no associated parent window."),
                         hbr));
         }

         if (IS_FLAG_SET(((PBRFCASE)hbr)->dwFlags, BR_FL_DATABASE_OPENED))
         {
            pobri->hvid = (HVOLUMEID)(((PCBRFCASE)hbr)->bcdb.hpathDBFolder);
            GetPathString(((PCBRFCASE)hbr)->bcdb.hpathDBFolder,
                          pobri->rgchDatabasePath);
            CatPath(pobri->rgchDatabasePath, ((PCBRFCASE)hbr)->bcdb.pszDBName);
         }
         else
         {
            pobri->hvid = NULL;
            pobri->rgchDatabasePath[0] = TEXT('\0');

            WARNING_OUT((TEXT("GetBriefcaseInfo(): Briefcase %#lx has no associated database."),
                         hbr));
         }

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      ASSERT(tr != TR_SUCCESS ||
             IS_VALID_STRUCT_PTR(pobri, COPENBRFCASEINFO));

      DebugExitTWINRESULT(GetBriefcaseInfo, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | ClearBriefcaseCache | Wipes out cached information in an open
briefcase.

@parm HBRFCASE | hbr | A handle to the open briefcase whose cached information
is to be cleared.

@rdesc If the open briefcase's cached information was cleared successfully,
TR_SUCCESS is returned.  Otherwise, the briefcase's cached information was not
cleared successfully, and the return value indicates the error that occurred.

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI ClearBriefcaseCache(HBRFCASE hbr)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(ClearBriefcaseCache);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hbr, BRFCASE))
#endif
      {
         ClearPathListInfo(((PBRFCASE)hbr)->hpathlist);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(ClearBriefcaseCache, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

