/*
 * volume.c - Volume ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "volume.h"


/* Constants
 ************/

/* VOLUMELIST PTRARRAY allocation parameters */

#define NUM_START_VOLUMES        (16)
#define NUM_VOLUMES_TO_ADD       (16)

/* VOLUMELIST string table allocation parameters */

#define NUM_VOLUME_HASH_BUCKETS  (31)


/* Types
 ********/

/* volume list */

typedef struct _volumelist
{
   /* array of pointers to VOLUMEs */

   HPTRARRAY hpa;

   /* table of volume root path strings */

   HSTRINGTABLE hst;

   /* flags from RESOLVELINKINFOINFLAGS */

   DWORD dwFlags;

   /*
    * handle to parent window, only valid if RLI_IFL_ALLOW_UI is set in dwFlags
    * field
    */

   HWND hwndOwner;
}
VOLUMELIST;
DECLARE_STANDARD_TYPES(VOLUMELIST);

/* VOLUME flags */

typedef enum _volumeflags
{
   /* The volume root path string indicated by hsRootPath is valid. */

   VOLUME_FL_ROOT_PATH_VALID  = 0x0001,

   /*
    * The net resource should be disconnected by calling DisconnectLinkInfo()
    * when finished.
    */

   VOLUME_FL_DISCONNECT       = 0x0002,

   /* Any cached volume information should be verified before use. */

   VOLUME_FL_VERIFY_VOLUME    = 0x0004,

   /* flag combinations */

   ALL_VOLUME_FLAGS           = (VOLUME_FL_ROOT_PATH_VALID |
                                 VOLUME_FL_DISCONNECT |
                                 VOLUME_FL_VERIFY_VOLUME)
}
VOLUMEFLAGS;

/* VOLUME states */

typedef enum _volumestate
{
   VS_UNKNOWN,

   VS_AVAILABLE,

   VS_UNAVAILABLE
}
VOLUMESTATE;
DECLARE_STANDARD_TYPES(VOLUMESTATE);

/* volume structure */

typedef struct _volume
{
   /* reference count */

   ULONG ulcLock;

   /* bit mask of flags from VOLUMEFLAGS */

   DWORD dwFlags;

   /* volume state */

   VOLUMESTATE vs;

   /* pointer to LinkInfo structure indentifying volume */

   PLINKINFO pli;

   /*
    * handle to volume root path string, only valid if
    * VOLUME_FL_ROOT_PATH_VALID is set in dwFlags field
    */

   HSTRING hsRootPath;

   /* pointer to parent volume list */

   PVOLUMELIST pvlParent;
}
VOLUME;
DECLARE_STANDARD_TYPES(VOLUME);

/* database volume list header */

typedef struct _dbvolumelistheader
{
   /* number of volumes in list */

   LONG lcVolumes;

   /* length of longest LinkInfo structure in volume list in bytes */

   UINT ucbMaxLinkInfoLen;
}
DBVOLUMELISTHEADER;
DECLARE_STANDARD_TYPES(DBVOLUMELISTHEADER);

/* database volume structure */

typedef struct _dbvolume
{
   /* old handle to volume */

   HVOLUME hvol;

   /* old LinkInfo structure follows */

   /* first DWORD of LinkInfo structure is total size in bytes */
}
DBVOLUME;
DECLARE_STANDARD_TYPES(DBVOLUME);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE COMPARISONRESULT VolumeSortCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT VolumeSearchCmp(PCVOID, PCVOID);
PRIVATE_CODE BOOL SearchForVolumeByRootPathCmp(PCVOID, PCVOID);
PRIVATE_CODE BOOL UnifyVolume(PVOLUMELIST, PLINKINFO, PVOLUME *);
PRIVATE_CODE BOOL CreateVolume(PVOLUMELIST, PLINKINFO, PVOLUME *);
PRIVATE_CODE void UnlinkVolume(PCVOLUME);
PRIVATE_CODE BOOL DisconnectVolume(PVOLUME);
PRIVATE_CODE void DestroyVolume(PVOLUME);
PRIVATE_CODE void LockVolume(PVOLUME);
PRIVATE_CODE BOOL UnlockVolume(PVOLUME);
PRIVATE_CODE void InvalidateVolumeInfo(PVOLUME);
PRIVATE_CODE void ClearVolumeInfo(PVOLUME);
PRIVATE_CODE void GetUnavailableVolumeRootPath(PCLINKINFO, LPTSTR);
PRIVATE_CODE BOOL VerifyAvailableVolume(PVOLUME);
PRIVATE_CODE void ExpensiveResolveVolumeRootPath(PVOLUME, LPTSTR);
PRIVATE_CODE void ResolveVolumeRootPath(PVOLUME, LPTSTR);
PRIVATE_CODE VOLUMERESULT VOLUMERESULTFromLastError(VOLUMERESULT);
PRIVATE_CODE TWINRESULT WriteVolume(HCACHEDFILE, PVOLUME);
PRIVATE_CODE TWINRESULT ReadVolume(HCACHEDFILE, PVOLUMELIST, PLINKINFO, UINT, HHANDLETRANS);

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsValidPCVOLUMELIST(PCVOLUMELIST);
PRIVATE_CODE BOOL IsValidVOLUMESTATE(VOLUMESTATE);
PRIVATE_CODE BOOL IsValidPCVOLUME(PCVOLUME);

#endif

#ifdef DEBUG

PRIVATE_CODE BOOL IsValidPCVOLUMEDESC(PCVOLUMEDESC);

#endif


/*
** VolumeSortCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volumes are sorted by:
**    1) LinkInfo volume
**    2) pointer
*/
PRIVATE_CODE COMPARISONRESULT VolumeSortCmp(PCVOID pcvol1, PCVOID pcvol2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcvol1, CVOLUME));
   ASSERT(IS_VALID_STRUCT_PTR(pcvol2, CVOLUME));

   cr = CompareLinkInfoVolumes(((PCVOLUME)pcvol1)->pli,
                               ((PCVOLUME)pcvol2)->pli);

   if (cr == CR_EQUAL)
      cr = ComparePointers(pcvol1, pcvol1);

   return(cr);
}


/*
** VolumeSearchCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volumes are searched by:
**    1) LinkInfo volume
*/
PRIVATE_CODE COMPARISONRESULT VolumeSearchCmp(PCVOID pcli, PCVOID pcvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcvol, CVOLUME));

   return(CompareLinkInfoVolumes(pcli, ((PCVOLUME)pcvol)->pli));
}


/*
** SearchForVolumeByRootPathCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volumes are searched by:
**    1) available volume root path
*/
PRIVATE_CODE BOOL SearchForVolumeByRootPathCmp(PCVOID pcszFullPath,
                                               PCVOID pcvol)
{
   BOOL bDifferent;

   ASSERT(IsFullPath(pcszFullPath));
   ASSERT(IS_VALID_STRUCT_PTR(pcvol, CVOLUME));

   if (((PCVOLUME)pcvol)->vs == VS_AVAILABLE &&
       IS_FLAG_SET(((PCVOLUME)pcvol)->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
   {
      LPCTSTR pcszVolumeRootPath;

      pcszVolumeRootPath = GetString(((PCVOLUME)pcvol)->hsRootPath);

      bDifferent = MyLStrCmpNI(pcszFullPath, pcszVolumeRootPath,
                               lstrlen(pcszVolumeRootPath));
   }
   else
      bDifferent = TRUE;

   return(bDifferent);
}


/*
** UnifyVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnifyVolume(PVOLUMELIST pvl, PLINKINFO pliRoot,
                              PVOLUME *ppvol)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pvl, CVOLUMELIST));
   ASSERT(IS_VALID_STRUCT_PTR(pliRoot, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(ppvol, PVOLUME));

   if (AllocateMemory(sizeof(**ppvol), ppvol))
   {
      if (CopyLinkInfo(pliRoot, &((*ppvol)->pli)))
      {
         ARRAYINDEX aiUnused;

         (*ppvol)->ulcLock = 0;
         (*ppvol)->dwFlags = 0;
         (*ppvol)->vs = VS_UNKNOWN;
         (*ppvol)->hsRootPath = NULL;
         (*ppvol)->pvlParent = pvl;

         if (AddPtr(pvl->hpa, VolumeSortCmp, *ppvol, &aiUnused))
            bResult = TRUE;
         else
         {
            FreeMemory((*ppvol)->pli);
UNIFYVOLUME_BAIL:
            FreeMemory(*ppvol);
         }
      }
      else
         goto UNIFYVOLUME_BAIL;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppvol, CVOLUME));

   return(bResult);
}


/*
** CreateVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateVolume(PVOLUMELIST pvl, PLINKINFO pliRoot,
                               PVOLUME *ppvol)
{
   BOOL bResult;
   PVOLUME pvol;
   ARRAYINDEX aiFound;

   ASSERT(IS_VALID_STRUCT_PTR(pvl, CVOLUMELIST));
   ASSERT(IS_VALID_STRUCT_PTR(pliRoot, CLINKINFO));
   ASSERT(IS_VALID_WRITE_PTR(ppvol, PVOLUME));

   /* Does a volume for the given root path already exist? */

   if (SearchSortedArray(pvl->hpa, &VolumeSearchCmp, pliRoot, &aiFound))
   {
      pvol = GetPtr(pvl->hpa, aiFound);
      bResult = TRUE;
   }
   else
      bResult = UnifyVolume(pvl, pliRoot, &pvol);

   if (bResult)
   {
      LockVolume(pvol);
      *ppvol = pvol;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppvol, CVOLUME));

   return(bResult);
}


/*
** UnlinkVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void UnlinkVolume(PCVOLUME pcvol)
{
   HPTRARRAY hpa;
   ARRAYINDEX aiFound;

   ASSERT(IS_VALID_STRUCT_PTR(pcvol, CVOLUME));

   hpa = pcvol->pvlParent->hpa;

   if (EVAL(SearchSortedArray(hpa, &VolumeSortCmp, pcvol, &aiFound)))
   {
      ASSERT(GetPtr(hpa, aiFound) == pcvol);

      DeletePtr(hpa, aiFound);
   }

   return;
}


/*
** DisconnectVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DisconnectVolume(PVOLUME pvol)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_DISCONNECT))
   {
      bResult = DisconnectLinkInfo(pvol->pli);

      CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_DISCONNECT);
   }
   else
      bResult = TRUE;

   return(bResult);
}


/*
** DestroyVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyVolume(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   ClearVolumeInfo(pvol);

   FreeMemory(pvol->pli);
   FreeMemory(pvol);

   return;
}


/*
** LockVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void LockVolume(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   ASSERT(pvol->ulcLock < ULONG_MAX);
   pvol->ulcLock++;

   return;
}


/*
** UnlockVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnlockVolume(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   if (EVAL(pvol->ulcLock > 0))
      pvol->ulcLock--;

   return(pvol->ulcLock > 0);
}


/*
** InvalidateVolumeInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void InvalidateVolumeInfo(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   SET_FLAG(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME);

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   return;
}


/*
** ClearVolumeInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void ClearVolumeInfo(PVOLUME pvol)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   DisconnectVolume(pvol);

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
   {
      DeleteString(pvol->hsRootPath);

      CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID);
   }

   CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME);

   pvol->vs = VS_UNKNOWN;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   return;
}


/*
** GetUnavailableVolumeRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void GetUnavailableVolumeRootPath(PCLINKINFO pcli,
                                               LPTSTR pszRootPathBuf)
{
   LPCSTR pcszLinkInfoData;

   ASSERT(IS_VALID_STRUCT_PTR(pcli, CLINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));

   /*
    * Try unavailable volume root paths in the following order:
    *    1) last redirected device
    *    2) net resource name
    *    3) local path           ...and take the _last_ good one!
    */

   if (GetLinkInfoData(pcli, LIDT_REDIRECTED_DEVICE, &pcszLinkInfoData) ||
       GetLinkInfoData(pcli, LIDT_NET_RESOURCE, &pcszLinkInfoData) ||
       GetLinkInfoData(pcli, LIDT_LOCAL_BASE_PATH, &pcszLinkInfoData))
   {
      //ASSERT(IS_VALID_STRING_PTR(pcszLinkInfoData, CSTR));
      ASSERT(lstrlenA(pcszLinkInfoData) < MAX_PATH_LEN);

      // BUGBUG somewhere, someone might need to handle unicode base paths 

#ifdef UNICODE
      {
        TCHAR szTmp[MAX_PATH] = TEXT("");
        MultiByteToWideChar(CP_ACP, 0, pcszLinkInfoData, -1, szTmp, MAX_PATH);
        ComposePath(pszRootPathBuf, szTmp, TEXT("\\"));
      }
#else

      ComposePath(pszRootPathBuf, pcszLinkInfoData, TEXT("\\"));

#endif

   }
   else
   {
      pszRootPathBuf[0] = TEXT('\0');

      ERROR_OUT((TEXT("GetUnavailableVolumeRootPath(): Net resource name and local base path unavailable.  Using empty string as unavailable root path.")));
   }

   ASSERT(IsRootPath(pszRootPathBuf) &&
          EVAL(lstrlen(pszRootPathBuf) < MAX_PATH_LEN));

   return;
}


/*
** VerifyAvailableVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL VerifyAvailableVolume(PVOLUME pvol)
{
   BOOL bResult = FALSE;
   PLINKINFO pli;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   ASSERT(pvol->vs == VS_AVAILABLE);
   ASSERT(IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID));

   WARNING_OUT((TEXT("VerifyAvailableVolume(): Calling CreateLinkInfo() to verify volume on %s."),
                GetString(pvol->hsRootPath)));

   if (CreateLinkInfo(GetString(pvol->hsRootPath), &pli))
   {
      bResult = (CompareLinkInfoReferents(pvol->pli, pli) == CR_EQUAL);

      DestroyLinkInfo(pli);

      if (bResult)
         TRACE_OUT((TEXT("VerifyAvailableVolume(): Volume %s has not changed."),
                    GetString(pvol->hsRootPath)));
      else
         WARNING_OUT((TEXT("VerifyAvailableVolume(): Volume %s has changed."),
                      GetString(pvol->hsRootPath)));
   }
   else
      WARNING_OUT((TEXT("VerifyAvailableVolume(): CreateLinkInfo() failed for %s."),
                   GetString(pvol->hsRootPath)));

   return(bResult);
}


/*
** ExpensiveResolveVolumeRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void ExpensiveResolveVolumeRootPath(PVOLUME pvol,
                                                 LPTSTR pszVolumeRootPathBuf)
{
   BOOL bResult;
   DWORD dwOutFlags;
   PLINKINFO pliUpdated;
   HSTRING hsRootPath;

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszVolumeRootPathBuf, STR, MAX_PATH_LEN));

   if (pvol->vs == VS_UNKNOWN ||
       pvol->vs == VS_AVAILABLE)
   {
      /*
       * Only request a connection if connections are still permitted in this
       * volume list.
       */

      WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Calling ResolveLinkInfo() to determine volume availability and root path.")));

      bResult = ResolveLinkInfo(pvol->pli, pszVolumeRootPathBuf,
                                pvol->pvlParent->dwFlags,
                                pvol->pvlParent->hwndOwner, &dwOutFlags,
                                &pliUpdated);

      if (bResult)
      {
         pvol->vs = VS_AVAILABLE;

         if (IS_FLAG_SET(dwOutFlags, RLI_OFL_UPDATED))
         {
            PLINKINFO pliUpdatedCopy;

            ASSERT(IS_FLAG_SET(pvol->pvlParent->dwFlags, RLI_IFL_UPDATE));

            if (CopyLinkInfo(pliUpdated, &pliUpdatedCopy))
            {
               FreeMemory(pvol->pli);
               pvol->pli = pliUpdatedCopy;
            }

            DestroyLinkInfo(pliUpdated);

            WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Updating LinkInfo for volume %s."),
                         pszVolumeRootPathBuf));
         }

         if (IS_FLAG_SET(dwOutFlags, RLI_OFL_DISCONNECT))
         {
            SET_FLAG(pvol->dwFlags, VOLUME_FL_DISCONNECT);

            WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Volume %s must be disconnected when finished."),
                         pszVolumeRootPathBuf));
         }

         TRACE_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Volume %s is available."),
                    pszVolumeRootPathBuf));
      }
      else
         ASSERT(GetLastError() != ERROR_INVALID_PARAMETER);
   }
   else
   {
      ASSERT(pvol->vs == VS_UNAVAILABLE);
      bResult = FALSE;
   }

   if (! bResult)
   {
      pvol->vs = VS_UNAVAILABLE;

      if (GetLastError() == ERROR_CANCELLED)
      {
         ASSERT(IS_FLAG_SET(pvol->pvlParent->dwFlags, RLI_IFL_CONNECT));

         CLEAR_FLAG(pvol->pvlParent->dwFlags, RLI_IFL_CONNECT);

         WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Connection attempt cancelled.  No subsequent connections will be attempted.")));
      }

      GetUnavailableVolumeRootPath(pvol->pli, pszVolumeRootPathBuf);

      WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Using %s as unavailable volume root path."),
                   pszVolumeRootPathBuf));
   }

   /* Add volume root path string to volume list's string table. */

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
   {
      CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID);
      DeleteString(pvol->hsRootPath);
   }

   if (AddString(pszVolumeRootPathBuf, pvol->pvlParent->hst, GetHashBucketIndex, &hsRootPath))
   {
      SET_FLAG(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID);
      pvol->hsRootPath = hsRootPath;
   }
   else
      WARNING_OUT((TEXT("ExpensiveResolveVolumeRootPath(): Unable to save %s as volume root path."),
                   pszVolumeRootPathBuf));

   return;
}


/*
** ResolveVolumeRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void ResolveVolumeRootPath(PVOLUME pvol,
                                        LPTSTR pszVolumeRootPathBuf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszVolumeRootPathBuf, STR, MAX_PATH_LEN));

   /* Do we have a cached volume root path to use? */

   if (IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID) &&
       (IS_FLAG_CLEAR(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME) ||
        (pvol->vs == VS_AVAILABLE &&
         VerifyAvailableVolume(pvol))))
   {
      /* Yes. */

      MyLStrCpyN(pszVolumeRootPathBuf, GetString(pvol->hsRootPath), MAX_PATH_LEN);
      ASSERT(lstrlen(pszVolumeRootPathBuf) < MAX_PATH_LEN);

      ASSERT(pvol->vs != VS_UNKNOWN);
   }
   else
      /* No.  Welcome in I/O City. */
      ExpensiveResolveVolumeRootPath(pvol, pszVolumeRootPathBuf);

   CLEAR_FLAG(pvol->dwFlags, VOLUME_FL_VERIFY_VOLUME);

   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   return;
}


/*
** VOLUMERESULTFromLastError()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE VOLUMERESULT VOLUMERESULTFromLastError(VOLUMERESULT vr)
{
   switch (GetLastError())
   {
      case ERROR_OUTOFMEMORY:
         vr = VR_OUT_OF_MEMORY;
         break;

      case ERROR_BAD_PATHNAME:
         vr = VR_INVALID_PATH;
         break;

      default:
         break;
   }

   return(vr);
}


/*
** WriteVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WriteVolume(HCACHEDFILE hcf, PVOLUME pvol)
{
   TWINRESULT tr;
   DBVOLUME dbvol;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pvol, CVOLUME));

   /* Write database volume followed by LinkInfo structure. */

   dbvol.hvol = (HVOLUME)pvol;

   if (WriteToCachedFile(hcf, (PCVOID)&dbvol, sizeof(dbvol), NULL) &&
       WriteToCachedFile(hcf, pvol->pli, *(PDWORD)(pvol->pli), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


/*
** ReadVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ReadVolume(HCACHEDFILE hcf, PVOLUMELIST pvl,
                                   PLINKINFO pliBuf, UINT ucbLinkInfoBufLen,
                                   HHANDLETRANS hhtVolumes)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   DBVOLUME dbvol;
   DWORD dwcbRead;
   UINT ucbLinkInfoLen;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(pvl, CVOLUMELIST));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pliBuf, LINKINFO, ucbLinkInfoBufLen));
   ASSERT(IS_VALID_HANDLE(hhtVolumes, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbvol, sizeof(dbvol), &dwcbRead) &&
       dwcbRead == sizeof(dbvol) &&
       ReadFromCachedFile(hcf, &ucbLinkInfoLen, sizeof(ucbLinkInfoLen), &dwcbRead) &&
       dwcbRead == sizeof(ucbLinkInfoLen) &&
       ucbLinkInfoLen <= ucbLinkInfoBufLen)
   {
      /* Read the remainder of the LinkInfo structure into memory. */

      DWORD dwcbRemainder;

      pliBuf->ucbSize = ucbLinkInfoLen;
      dwcbRemainder = ucbLinkInfoLen - sizeof(ucbLinkInfoLen);

      if (ReadFromCachedFile(hcf, (PBYTE)pliBuf + sizeof(ucbLinkInfoLen),
                             dwcbRemainder, &dwcbRead) &&
          dwcbRead == dwcbRemainder &&
          IsValidLinkInfo(pliBuf))
      {
         PVOLUME pvol;

         if (CreateVolume(pvl, pliBuf, &pvol))
         {
            /*
             * To leave read volumes with 0 initial lock count, we must undo
             * the LockVolume() performed by CreateVolume().
             */

            UnlockVolume(pvol);

            if (AddHandleToHandleTranslator(hhtVolumes,
                                            (HGENERIC)(dbvol.hvol),
                                            (HGENERIC)pvol))
               tr = TR_SUCCESS;
            else
            {
               UnlinkVolume(pvol);
               DestroyVolume(pvol);

               tr = TR_OUT_OF_MEMORY;
            }
         }
         else
            tr = TR_OUT_OF_MEMORY;
      }
   }

   return(tr);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidPCVOLUMELIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCVOLUMELIST(PCVOLUMELIST pcvl)
{
   return(IS_VALID_READ_PTR(pcvl, CVOLUMELIST) &&
          IS_VALID_HANDLE(pcvl->hpa, PTRARRAY) &&
          IS_VALID_HANDLE(pcvl->hst, STRINGTABLE) &&
          FLAGS_ARE_VALID(pcvl->dwFlags, ALL_RLI_IFLAGS) &&
          (IS_FLAG_CLEAR(pcvl->dwFlags, RLI_IFL_ALLOW_UI) ||
           IS_VALID_HANDLE(pcvl->hwndOwner, WND)));
}


/*
** IsValidVOLUMESTATE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidVOLUMESTATE(VOLUMESTATE vs)
{
   BOOL bResult;

   switch (vs)
   {
      case VS_UNKNOWN:
      case VS_AVAILABLE:
      case VS_UNAVAILABLE:
         bResult = TRUE;
         break;

      default:
         ERROR_OUT((TEXT("IsValidVOLUMESTATE(): Invalid VOLUMESTATE %d."),
                    vs));
         bResult = FALSE;
         break;
   }

   return(bResult);
}


/*
** IsValidPCVOLUME()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCVOLUME(PCVOLUME pcvol)
{
   return(IS_VALID_READ_PTR(pcvol, CVOLUME) &&
          FLAGS_ARE_VALID(pcvol->dwFlags, ALL_VOLUME_FLAGS) &&
          EVAL(IsValidVOLUMESTATE(pcvol->vs)) &&
          IS_VALID_STRUCT_PTR(pcvol->pli, CLINKINFO) &&
          (IS_FLAG_CLEAR(pcvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID) ||
           IS_VALID_HANDLE(pcvol->hsRootPath, STRING)) &&
          IS_VALID_STRUCT_PTR(pcvol->pvlParent, CVOLUMELIST));
}

#endif


#ifdef DEBUG

/*
** IsValidPCVOLUMEDESC()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCVOLUMEDESC(PCVOLUMEDESC pcvoldesc)
{
   /*
    * A set dwSerialNumber may be any value.  An unset dwSerialNumber must be
    * 0.  A set strings may be any valid string.  An unset string must be the
    * empty string.
    */

   return(IS_VALID_READ_PTR(pcvoldesc, CVOLUMEDESC) &&
          EVAL(pcvoldesc->ulSize == sizeof(*pcvoldesc)) &&
          FLAGS_ARE_VALID(pcvoldesc->dwFlags, ALL_VD_FLAGS) &&
          (IS_FLAG_SET(pcvoldesc->dwFlags, VD_FL_SERIAL_NUMBER_VALID) ||
           ! pcvoldesc->dwSerialNumber) &&
          ((IS_FLAG_CLEAR(pcvoldesc->dwFlags, VD_FL_VOLUME_LABEL_VALID) &&
            ! pcvoldesc->rgchVolumeLabel[0]) ||
           (IS_FLAG_SET(pcvoldesc->dwFlags, VD_FL_VOLUME_LABEL_VALID) &&
            IS_VALID_STRING_PTR(pcvoldesc->rgchVolumeLabel, CSTR) &&
            EVAL(lstrlen(pcvoldesc->rgchVolumeLabel) < ARRAYSIZE(pcvoldesc->rgchVolumeLabel)))) &&
          ((IS_FLAG_CLEAR(pcvoldesc->dwFlags, VD_FL_NET_RESOURCE_VALID) &&
            ! pcvoldesc->rgchNetResource[0]) ||
           (IS_FLAG_SET(pcvoldesc->dwFlags, VD_FL_NET_RESOURCE_VALID) &&
            IS_VALID_STRING_PTR(pcvoldesc->rgchNetResource, CSTR) &&
            EVAL(lstrlen(pcvoldesc->rgchNetResource) < ARRAYSIZE(pcvoldesc->rgchNetResource)))));
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateVolumeList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateVolumeList(DWORD dwFlags, HWND hwndOwner,
                                  PHVOLUMELIST phvl)
{
   BOOL bResult = FALSE;
   PVOLUMELIST pvl;

   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_RLI_IFLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RLI_IFL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_PTR(phvl, HVOLUMELIST));

   if (AllocateMemory(sizeof(*pvl), &pvl))
   {
      NEWSTRINGTABLE nszt;

      /* Create string table for volume root path strngs. */

      nszt.hbc = NUM_VOLUME_HASH_BUCKETS;

      if (CreateStringTable(&nszt, &(pvl->hst)))
      {
         NEWPTRARRAY npa;

         /* Create pointer array of volumes. */

         npa.aicInitialPtrs = NUM_START_VOLUMES;
         npa.aicAllocGranularity = NUM_VOLUMES_TO_ADD;
         npa.dwFlags = NPA_FL_SORTED_ADD;

         if (CreatePtrArray(&npa, &(pvl->hpa)))
         {
            pvl->dwFlags = dwFlags;
            pvl->hwndOwner = hwndOwner;

            *phvl = (HVOLUMELIST)pvl;
            bResult = TRUE;
         }
         else
         {
            DestroyStringTable(pvl->hst);
CREATEVOLUMELIST_BAIL:
            FreeMemory(pvl);
         }
      }
      else
         goto CREATEVOLUMELIST_BAIL;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phvl, VOLUMELIST));

   return(bResult);
}


/*
** DestroyVolumeList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyVolumeList(HVOLUMELIST hvl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   /* First free all volumes in array. */

   aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyVolume(GetPtr(((PCVOLUMELIST)hvl)->hpa, ai));

   /* Now wipe out the array. */

   DestroyPtrArray(((PCVOLUMELIST)hvl)->hpa);

   ASSERT(! GetStringCount(((PCVOLUMELIST)hvl)->hst));
   DestroyStringTable(((PCVOLUMELIST)hvl)->hst);

   FreeMemory((PVOLUMELIST)hvl);

   return;
}


/*
** InvalidateVolumeListInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void InvalidateVolumeListInfo(HVOLUMELIST hvl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      InvalidateVolumeInfo(GetPtr(((PCVOLUMELIST)hvl)->hpa, ai));

   WARNING_OUT((TEXT("InvalidateVolumeListInfo(): Volume cache invalidated.")));

   return;
}


/*
** ClearVolumeListInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ClearVolumeListInfo(HVOLUMELIST hvl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      ClearVolumeInfo(GetPtr(((PCVOLUMELIST)hvl)->hpa, ai));

   WARNING_OUT((TEXT("ClearVolumeListInfo(): Volume cache cleared.")));

   return;
}


/*
** AddVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE VOLUMERESULT AddVolume(HVOLUMELIST hvl, LPCTSTR pcszPath,
                                   PHVOLUME phvol, LPTSTR pszPathSuffixBuf)
{
   VOLUMERESULT vr;
   TCHAR rgchPath[MAX_PATH_LEN];
   LPTSTR pszFileName;
   DWORD dwPathLen;

   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phvol, HVOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathSuffixBuf, STR, MAX_PATH_LEN));

   dwPathLen = GetFullPathName(pcszPath, ARRAYSIZE(rgchPath), rgchPath,
                               &pszFileName);

   if (dwPathLen > 0 && dwPathLen < ARRAYSIZE(rgchPath))
   {
      ARRAYINDEX aiFound;

      /* Does a volume for this root path already exist? */

      if (LinearSearchArray(((PVOLUMELIST)hvl)->hpa,
                            &SearchForVolumeByRootPathCmp, rgchPath,
                            &aiFound))
      {
         PVOLUME pvol;
         LPCTSTR pcszVolumeRootPath;

         /* Yes. */

         pvol = GetPtr(((PVOLUMELIST)hvl)->hpa, aiFound);

         LockVolume(pvol);

         ASSERT(pvol->vs == VS_AVAILABLE &&
                IS_FLAG_SET(pvol->dwFlags, VOLUME_FL_ROOT_PATH_VALID));

         pcszVolumeRootPath = GetString(pvol->hsRootPath);

         ASSERT(lstrlen(pcszVolumeRootPath) <= lstrlen(rgchPath));

         lstrcpy(pszPathSuffixBuf, rgchPath + lstrlen(pcszVolumeRootPath));

         *phvol = (HVOLUME)pvol;
         vr = VR_SUCCESS;
      }
      else
      {
         DWORD dwOutFlags;
         TCHAR rgchNetResource[MAX_PATH_LEN];
         LPTSTR pszRootPathSuffix;

         /* No.  Create a new volume. */

         if (GetCanonicalPathInfo(pcszPath, rgchPath, &dwOutFlags,
                                  rgchNetResource, &pszRootPathSuffix))
         {
            PLINKINFO pli;

            lstrcpy(pszPathSuffixBuf, pszRootPathSuffix);
            *pszRootPathSuffix = TEXT('\0');

            WARNING_OUT((TEXT("AddVolume(): Creating LinkInfo for root path %s."),
                         rgchPath));

            if (CreateLinkInfo(rgchPath, &pli))
            {
               PVOLUME pvol;

               if (CreateVolume((PVOLUMELIST)hvl, pli, &pvol))
               {
                  TCHAR rgchUnusedVolumeRootPath[MAX_PATH_LEN];

                  ResolveVolumeRootPath(pvol, rgchUnusedVolumeRootPath);

                  *phvol = (HVOLUME)pvol;
                  vr = VR_SUCCESS;
               }
               else
                  vr = VR_OUT_OF_MEMORY;

               DestroyLinkInfo(pli);
            }
            else
               /*
                * Differentiate between VR_UNAVAILABLE_VOLUME and
                * VR_OUT_OF_MEMORY.
                */
               vr = VOLUMERESULTFromLastError(VR_UNAVAILABLE_VOLUME);
         }
         else
            vr = VOLUMERESULTFromLastError(VR_INVALID_PATH);
      }
   }
   else
   {
      ASSERT(! dwPathLen);

      vr = VOLUMERESULTFromLastError(VR_INVALID_PATH);
   }

   ASSERT(vr != VR_SUCCESS ||
          (IS_VALID_HANDLE(*phvol, VOLUME) &&
           EVAL(IsValidPathSuffix(pszPathSuffixBuf))));

   return(vr);
}


/*
** DeleteVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DeleteVolume(HVOLUME hvol)
{
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));

   if (! UnlockVolume((PVOLUME)hvol))
   {
      UnlinkVolume((PVOLUME)hvol);
      DestroyVolume((PVOLUME)hvol);
   }

   return;
}


/*
** CompareVolumes()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT CompareVolumes(HVOLUME hvolFirst,
                                            HVOLUME hvolSecond)
{
   ASSERT(IS_VALID_HANDLE(hvolFirst, VOLUME));
   ASSERT(IS_VALID_HANDLE(hvolSecond, VOLUME));

   /* This comparison works across volume lists. */

   return(CompareLinkInfoVolumes(((PCVOLUME)hvolFirst)->pli,
                                 ((PCVOLUME)hvolSecond)->pli));
}


/*
** CopyVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CopyVolume(HVOLUME hvolSrc, HVOLUMELIST hvlDest,
                            PHVOLUME phvolCopy)
{
   BOOL bResult;
   PVOLUME pvol;

   ASSERT(IS_VALID_HANDLE(hvolSrc, VOLUME));
   ASSERT(IS_VALID_HANDLE(hvlDest, VOLUMELIST));
   ASSERT(IS_VALID_WRITE_PTR(phvolCopy, HVOLUME));

   /* Is the destination volume list the source volume's volume list? */

   if (((PCVOLUME)hvolSrc)->pvlParent == (PCVOLUMELIST)hvlDest)
   {
      /* Yes.  Use the source volume. */

      LockVolume((PVOLUME)hvolSrc);
      pvol = (PVOLUME)hvolSrc;
      bResult = TRUE;
   }
   else
      bResult = CreateVolume((PVOLUMELIST)hvlDest, ((PCVOLUME)hvolSrc)->pli,
                             &pvol);

   if (bResult)
      *phvolCopy = (HVOLUME)pvol;

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phvolCopy, VOLUME));

   return(bResult);
}


/*
** IsVolumeAvailable()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsVolumeAvailable(HVOLUME hvol)
{
   TCHAR rgchUnusedVolumeRootPath[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));

   ResolveVolumeRootPath((PVOLUME)hvol, rgchUnusedVolumeRootPath);

   ASSERT(IsValidVOLUMESTATE(((PCVOLUME)hvol)->vs) &&
          ((PCVOLUME)hvol)->vs != VS_UNKNOWN);

   return(((PCVOLUME)hvol)->vs == VS_AVAILABLE);
}


/*
** GetVolumeRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void GetVolumeRootPath(HVOLUME hvol, LPTSTR pszRootPathBuf)
{
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));

   ResolveVolumeRootPath((PVOLUME)hvol, pszRootPathBuf);

   ASSERT(IsRootPath(pszRootPathBuf));

   return;
}


#ifdef DEBUG

/*
** DebugGetVolumeRootPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** N.b., DebugGetVolumeRootPath() must be non-intrusive.
*/
PUBLIC_CODE LPTSTR DebugGetVolumeRootPath(HVOLUME hvol, LPTSTR pszRootPathBuf)
{
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));

   if (IS_FLAG_SET(((PVOLUME)hvol)->dwFlags, VOLUME_FL_ROOT_PATH_VALID))
      MyLStrCpyN(pszRootPathBuf, GetString(((PVOLUME)hvol)->hsRootPath), MAX_PATH_LEN);
   else
      GetUnavailableVolumeRootPath(((PVOLUME)hvol)->pli, pszRootPathBuf);

   ASSERT(IsRootPath(pszRootPathBuf));

   return(pszRootPathBuf);
}


/*
** GetVolumeCount()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE ULONG GetVolumeCount(HVOLUMELIST hvl)
{
   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   return(GetPtrCount(((PCVOLUMELIST)hvl)->hpa));
}

#endif


/*
** DescribeVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DescribeVolume(HVOLUME hvol, PVOLUMEDESC pvoldesc)
{
   PCVOID pcv;

   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IS_VALID_WRITE_PTR(pvoldesc, VOLUMEDESC));

   ASSERT(pvoldesc->ulSize == sizeof(*pvoldesc));

   pvoldesc->dwFlags = 0;

   if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_VOLUME_SERIAL_NUMBER, &pcv))
   {
      pvoldesc->dwSerialNumber = *(PCDWORD)pcv;
      SET_FLAG(pvoldesc->dwFlags, VD_FL_SERIAL_NUMBER_VALID);
   }
   else
      pvoldesc->dwSerialNumber = 0;

   if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_VOLUME_LABELW, &pcv) && pcv)
   {
      lstrcpy(pvoldesc->rgchVolumeLabel, pcv);
      SET_FLAG(pvoldesc->dwFlags, VD_FL_VOLUME_LABEL_VALID);
   }
   else if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_VOLUME_LABEL, &pcv) && pcv)
   {
      MultiByteToWideChar(CP_ACP, 0, pcv, -1, pvoldesc->rgchVolumeLabel, MAX_PATH);
      SET_FLAG(pvoldesc->dwFlags, VD_FL_VOLUME_LABEL_VALID);
   }
   else
   {
      pvoldesc->rgchVolumeLabel[0] = TEXT('\0');
   }

   if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_NET_RESOURCEW, &pcv) && pcv)
   {
        lstrcpy(pvoldesc->rgchNetResource, pcv);
        SET_FLAG(pvoldesc->dwFlags, VD_FL_NET_RESOURCE_VALID);
   }
   else if (GetLinkInfoData(((PCVOLUME)hvol)->pli, LIDT_NET_RESOURCE, &pcv) && pcv)
   {
        MultiByteToWideChar(CP_ACP, 0, pcv, -1, pvoldesc->rgchNetResource, MAX_PATH);
        SET_FLAG(pvoldesc->dwFlags, VD_FL_NET_RESOURCE_VALID);
   }
   else
      pvoldesc->rgchNetResource[0] = TEXT('\0');

   ASSERT(IS_VALID_STRUCT_PTR(pvoldesc, CVOLUMEDESC));

   return;
}


/*
** WriteVolumeList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT WriteVolumeList(HCACHEDFILE hcf, HVOLUMELIST hvl)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbDBVolumeListHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));

   /* Save initial file position. */

   dwcbDBVolumeListHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbDBVolumeListHeaderOffset != INVALID_SEEK_POSITION)
   {
      DBVOLUMELISTHEADER dbvlh;

      /* Leave space for volume list header. */

      ZeroMemory(&dbvlh, sizeof(dbvlh));

      if (WriteToCachedFile(hcf, (PCVOID)&dbvlh, sizeof(dbvlh), NULL))
      {
         ARRAYINDEX aicPtrs;
         ARRAYINDEX ai;
         UINT ucbMaxLinkInfoLen = 0;
         LONG lcVolumes = 0;

         tr = TR_SUCCESS;

         aicPtrs = GetPtrCount(((PCVOLUMELIST)hvl)->hpa);

         /* Write all volumes. */

         for (ai = 0; ai < aicPtrs; ai++)
         {
            PVOLUME pvol;

            pvol = GetPtr(((PCVOLUMELIST)hvl)->hpa, ai);

            /*
             * As a sanity check, don't save any volume with a lock count of 0.
             * A 0 lock count implies that the volume has not been referenced
             * since it was restored from the database, or something is broken.
             */

            if (pvol->ulcLock > 0)
            {
               tr = WriteVolume(hcf, pvol);

               if (tr == TR_SUCCESS)
               {
                  ASSERT(lcVolumes < LONG_MAX);
                  lcVolumes++;

                  if (pvol->pli->ucbSize > ucbMaxLinkInfoLen)
                     ucbMaxLinkInfoLen = pvol->pli->ucbSize;
               }
               else
                  break;
            }
            else
               ERROR_OUT((TEXT("WriteVolumeList(): VOLUME has 0 lock count and will not be written.")));
         }

         /* Save volume list header. */

         if (tr == TR_SUCCESS)
         {
            dbvlh.lcVolumes = lcVolumes;
            dbvlh.ucbMaxLinkInfoLen = ucbMaxLinkInfoLen;

            tr = WriteDBSegmentHeader(hcf, dwcbDBVolumeListHeaderOffset,
                                      &dbvlh, sizeof(dbvlh));

            TRACE_OUT((TEXT("WriteVolumeList(): Wrote %ld volumes; maximum LinkInfo length %u bytes."),
                       dbvlh.lcVolumes,
                       dbvlh.ucbMaxLinkInfoLen));
         }
      }
   }

   return(tr);
}


/*
** ReadVolumeList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ReadVolumeList(HCACHEDFILE hcf, HVOLUMELIST hvl,
                                      PHHANDLETRANS phht)
{
   TWINRESULT tr;
   DBVOLUMELISTHEADER dbvlh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));
   ASSERT(IS_VALID_WRITE_PTR(phht, HHANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbvlh, sizeof(dbvlh), &dwcbRead) &&
       dwcbRead == sizeof(dbvlh))
   {
      HHANDLETRANS hht;

      tr = TR_OUT_OF_MEMORY;

      if (CreateHandleTranslator(dbvlh.lcVolumes, &hht))
      {
         PLINKINFO pliBuf;

         if (AllocateMemory(dbvlh.ucbMaxLinkInfoLen, &pliBuf))
         {
            LONG l;

            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("ReadPathList(): Reading %ld volumes; maximum LinkInfo length %u bytes."),
                       dbvlh.lcVolumes,
                       dbvlh.ucbMaxLinkInfoLen));

            for (l = 0; l < dbvlh.lcVolumes; l++)
            {
               tr = ReadVolume(hcf, (PVOLUMELIST)hvl, pliBuf,
                               dbvlh.ucbMaxLinkInfoLen, hht);

               if (tr != TR_SUCCESS)
                  break;
            }

            if (tr == TR_SUCCESS)
            {
               PrepareForHandleTranslation(hht);
               *phht = hht;

               ASSERT(IS_VALID_HANDLE(hvl, VOLUMELIST));
               ASSERT(IS_VALID_HANDLE(*phht, HANDLETRANS));
            }
            else
               DestroyHandleTranslator(hht);

            FreeMemory(pliBuf);
         }
      }
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   ASSERT(tr != TR_SUCCESS ||
          (IS_VALID_HANDLE(hvl, VOLUMELIST) &&
           IS_VALID_HANDLE(*phht, HANDLETRANS)));

   return(tr);
}


/*
** IsValidHVOLUME()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHVOLUME(HVOLUME hvol)
{
   return(IS_VALID_STRUCT_PTR((PCVOLUME)hvol, CVOLUME));
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidHVOLUMELIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHVOLUMELIST(HVOLUMELIST hvl)
{
   return(IS_VALID_STRUCT_PTR((PCVOLUMELIST)hvl, CVOLUMELIST));
}

#endif

